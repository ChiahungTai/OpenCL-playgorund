/**********************************************************************
Copyright ©2012 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/


// This is 2 PI / 1024
#define ANGLE 0x1.921fb6p-8F

// Distance between first real element of successive 1K vectors
// It must be >= 1024, and a multiple of 4
#define VSTRIDE (1024+0)

// Twiddle factor stuff
#define TWGEN(I,C,S) \
    float C; \
    float S = k_sincos(tbase * I, &C)

#define TW4GEN(I,C,S) \
    float4 C; \
    float4 S = k_sincos4(tbase * I, &C)

#define TWAPPLY(ZR, ZI, C, S) \
    do { \
	float4 __r = C * ZR - S * ZI; \
	ZI = C * ZI + S * ZR; \
	ZR = __r; \
    } while (0)

# define TW4IDDLE4() \
    do { \
        TW4GEN(1, c1, s1); \
        TWAPPLY(zr1, zi1, c1, s1); \
        TW4GEN(2, c2, s2); \
        TWAPPLY(zr2, zi2, c2, s2); \
        TW4GEN(3, c3, s3); \
        TWAPPLY(zr3, zi3, c3, s3); \
    } while (0)
    
# define TWIDDLE4() \
    do { \
        TWGEN(1, c1, s1); \
        TWAPPLY(zr1, zi1, c1, s1); \
        TWGEN(2, c2, s2); \
        TWAPPLY(zr2, zi2, c2, s2); \
        TWGEN(3, c3, s3); \
        TWAPPLY(zr3, zi3, c3, s3); \
    } while (0)

// 4 point FFT
#define FFT4() \
    do { \
        float4 ar0 = zr0 + zr2; \
        float4 ar2 = zr1 + zr3; \
        float4 br0 = ar0 + ar2; \
        float4 br1 = zr0 - zr2; \
        float4 br2 = ar0 - ar2; \
        float4 br3 = zr1 - zr3; \
        float4 ai0 = zi0 + zi2; \
        float4 ai2 = zi1 + zi3; \
        float4 bi0 = ai0 + ai2; \
        float4 bi1 = zi0 - zi2; \
        float4 bi2 = ai0 - ai2; \
        float4 bi3 = zi1 - zi3; \
        zr0 = br0; \
        zi0 = bi0; \
        zr1 = br1 + bi3; \
        zi1 = bi1 - br3; \
        zr3 = br1 - bi3; \
        zi3 = br3 + bi1; \
        zr2 = br2; \
        zi2 = bi2; \
    } while (0)

//class
class FFT
{
private:
	__global float *gr;
	__global float *gi;
	__local float *lds;
	uint gid;
    uint me;
    uint dg;
public:
	FFT(){}
	~FFT(){}
	void Init(__global float *real,__global float *imag,__local float *Localds);
	void passAll();
	// Return sin and cos of -2*pi*i/1024
	 float k_sincos(int i, float *cretp);
	 float4 k_sincos4(int4 i, float4 *cretp);
	 // First pass of 1K FFT
	 void kfft_pass1();
	// Second pass of 1K FFT
	 void kfft_pass2();
	// Third pass of 1K FFT
	 void kfft_pass3();
	// Fourth pass of 1K FFT
	 void kfft_pass4();
	// Fifth and last pass of 1K FFT
	 void kfft_pass5();
};
void FFT::Init(__global float *real,__global float *imag,__local float *Localds)
{
	gr = real;
	gi = imag;
	lds = Localds;
	gid = get_global_id(0);
    me = gid & 0x3fU;
    dg = (gid >> 6) * VSTRIDE;
    gr += dg;
    gi += dg;
}

void FFT::passAll()
{
	kfft_pass1();
	kfft_pass2();
	kfft_pass3();
	kfft_pass4();
	kfft_pass5();
}


	 float FFT::k_sincos(int i, float *cretp)
	{
		if (i > 512)
		i -= 1024;

		float x = i * -ANGLE;
		*cretp = native_cos(x);
		return native_sin(x);
	}
	 float4 FFT::k_sincos4(int4 i, float4 *cretp)
	{
		i -= (i > 512) & 1024;
		float4 x = convert_float4(i) * -ANGLE;
		*cretp = native_cos(x);
		return native_sin(x);
	}
	 void FFT::kfft_pass1()
	{
		const __global float4 *gp;
    __local float *lp;

    // Pull in transform data
    gp = (const __global float4 *)(gr + (me << 2));
    float4 zr0 = gp[0*64];
    float4 zr1 = gp[1*64];
    float4 zr2 = gp[2*64];
    float4 zr3 = gp[3*64];

    gp = (const __global float4 *)(gi + (me << 2));
    float4 zi0 = gp[0*64];
    float4 zi1 = gp[1*64];
    float4 zi2 = gp[2*64];
    float4 zi3 = gp[3*64];

    FFT4();

    int4 tbase = (int)(me << 2) + (int4)(0, 1, 2, 3);
    TW4IDDLE4();

    // Save registers
    // Note that this pointer is not aligned enough to be cast to a float4*
    lp = lds + ((me << 2) + (me >> 3));

    lp[0] = zr0.x;
    lp[1] = zr0.y;
    lp[2] = zr0.z;
    lp[3] = zr0.w;
    lp += 66*4;

    lp[0] = zr1.x;
    lp[1] = zr1.y;
    lp[2] = zr1.z;
    lp[3] = zr1.w;
    lp += 66*4;

    lp[0] = zr2.x;
    lp[1] = zr2.y;
    lp[2] = zr2.z;
    lp[3] = zr2.w;
    lp += 66*4;

    lp[0] = zr3.x;
    lp[1] = zr3.y;
    lp[2] = zr3.z;
    lp[3] = zr3.w;
    lp += 66*4;

    // Imaginary part
    lp[0] = zi0.x;
    lp[1] = zi0.y;
    lp[2] = zi0.z;
    lp[3] = zi0.w;
    lp += 66*4;

    lp[0] = zi1.x;
    lp[1] = zi1.y;
    lp[2] = zi1.z;
    lp[3] = zi1.w;
    lp += 66*4;

    lp[0] = zi2.x;
    lp[1] = zi2.y;
    lp[2] = zi2.z;
    lp[3] = zi2.w;
    lp += 66*4;

    lp[0] = zi3.x;
    lp[1] = zi3.y;
    lp[2] = zi3.z;
    lp[3] = zi3.w;

    barrier(CLK_LOCAL_MEM_FENCE);
	}


	// Second pass of 1K FFT
	 void FFT::kfft_pass2()
	{
		__local float *lp;

    // Load registers
    lp = lds + (me + (me >> 5));

    float4 zr0, zr1, zr2, zr3;

    zr0.x = lp[0*66];
    zr1.x = lp[1*66];
    zr2.x = lp[2*66];
    zr3.x = lp[3*66];
    lp += 66*4;

    zr0.y = lp[0*66];
    zr1.y = lp[1*66];
    zr2.y = lp[2*66];
    zr3.y = lp[3*66];
    lp += 66*4;

    zr0.z = lp[0*66];
    zr1.z = lp[1*66];
    zr2.z = lp[2*66];
    zr3.z = lp[3*66];
    lp += 66*4;

    zr0.w = lp[0*66];
    zr1.w = lp[1*66];
    zr2.w = lp[2*66];
    zr3.w = lp[3*66];
    lp += 66*4;

    float4 zi0, zi1, zi2, zi3;

    zi0.x = lp[0*66];
    zi1.x = lp[1*66];
    zi2.x = lp[2*66];
    zi3.x = lp[3*66];
    lp += 66*4;

    zi0.y = lp[0*66];
    zi1.y = lp[1*66];
    zi2.y = lp[2*66];
    zi3.y = lp[3*66];
    lp += 66*4;

    zi0.z = lp[0*66];
    zi1.z = lp[1*66];
    zi2.z = lp[2*66];
    zi3.z = lp[3*66];
    lp += 66*4;

    zi0.w = lp[0*66];
    zi1.w = lp[1*66];
    zi2.w = lp[2*66];
    zi3.w = lp[3*66];

    // Transform and twiddle
    FFT4();

    int tbase = (int)(me << 2);
    TWIDDLE4();

    barrier(CLK_LOCAL_MEM_FENCE);

    // Store registers
    lp = lds + ((me << 2) + (me >> 3));

    lp[0] = zr0.x;
    lp[1] = zr1.x;
    lp[2] = zr2.x;
    lp[3] = zr3.x;
    lp += 66*4;

    lp[0] = zr0.y;
    lp[1] = zr1.y;
    lp[2] = zr2.y;
    lp[3] = zr3.y;
    lp += 66*4;

    lp[0] = zr0.z;
    lp[1] = zr1.z;
    lp[2] = zr2.z;
    lp[3] = zr3.z;
    lp += 66*4;

    lp[0] = zr0.w;
    lp[1] = zr1.w;
    lp[2] = zr2.w;
    lp[3] = zr3.w;
    lp += 66*4;

    // Imaginary part
    lp[0] = zi0.x;
    lp[1] = zi1.x;
    lp[2] = zi2.x;
    lp[3] = zi3.x;
    lp += 66*4;

    lp[0] = zi0.y;
    lp[1] = zi1.y;
    lp[2] = zi2.y;
    lp[3] = zi3.y;
    lp += 66*4;

    lp[0] = zi0.z;
    lp[1] = zi1.z;
    lp[2] = zi2.z;
    lp[3] = zi3.z;
    lp += 66*4;

    lp[0] = zi0.w;
    lp[1] = zi1.w;
    lp[2] = zi2.w;
    lp[3] = zi3.w;

    barrier(CLK_LOCAL_MEM_FENCE);
	}


	// Third pass of 1K FFT
	 void FFT::kfft_pass3()
	{
		__local float *lp;

    // Load registers
    lp = lds + (me + (me >> 5));

    float4 zr0, zr1, zr2, zr3;

    zr0.x = lp[0*66];
    zr1.x = lp[1*66];
    zr2.x = lp[2*66];
    zr3.x = lp[3*66];
    lp += 66*4;

    zr0.y = lp[0*66];
    zr1.y = lp[1*66];
    zr2.y = lp[2*66];
    zr3.y = lp[3*66];
    lp += 66*4;

    zr0.z = lp[0*66];
    zr1.z = lp[1*66];
    zr2.z = lp[2*66];
    zr3.z = lp[3*66];
    lp += 66*4;

    zr0.w = lp[0*66];
    zr1.w = lp[1*66];
    zr2.w = lp[2*66];
    zr3.w = lp[3*66];
    lp += 66*4;

    float4 zi0, zi1, zi2, zi3;

    zi0.x = lp[0*66];
    zi1.x = lp[1*66];
    zi2.x = lp[2*66];
    zi3.x = lp[3*66];
    lp += 66*4;

    zi0.y = lp[0*66];
    zi1.y = lp[1*66];
    zi2.y = lp[2*66];
    zi3.y = lp[3*66];
    lp += 66*4;

    zi0.z = lp[0*66];
    zi1.z = lp[1*66];
    zi2.z = lp[2*66];
    zi3.z = lp[3*66];
    lp += 66*4;

    zi0.w = lp[0*66];
    zi1.w = lp[1*66];
    zi2.w = lp[2*66];
    zi3.w = lp[3*66];

    // Transform and twiddle
    FFT4();

    int tbase = (int)((me >> 2) << 4);
    TWIDDLE4();

    barrier(CLK_LOCAL_MEM_FENCE);

    // Save registers
    lp = lds + me;

    lp[0*66] = zr0.x;
    lp[1*66] = zr0.y;
    lp[2*66] = zr0.z;
    lp[3*66] = zr0.w;
    lp += 66*4;

    lp[0*66] = zr1.x;
    lp[1*66] = zr1.y;
    lp[2*66] = zr1.z;
    lp[3*66] = zr1.w;
    lp += 66*4;

    lp[0*66] = zr2.x;
    lp[1*66] = zr2.y;
    lp[2*66] = zr2.z;
    lp[3*66] = zr2.w;
    lp += 66*4;

    lp[0*66] = zr3.x;
    lp[1*66] = zr3.y;
    lp[2*66] = zr3.z;
    lp[3*66] = zr3.w;
    lp += 66*4;

    // Imaginary part
    lp[0*66] = zi0.x;
    lp[1*66] = zi0.y;
    lp[2*66] = zi0.z;
    lp[3*66] = zi0.w;
    lp += 66*4;

    lp[0*66] = zi1.x;
    lp[1*66] = zi1.y;
    lp[2*66] = zi1.z;
    lp[3*66] = zi1.w;
    lp += 66*4;

    lp[0*66] = zi2.x;
    lp[1*66] = zi2.y;
    lp[2*66] = zi2.z;
    lp[3*66] = zi2.w;
    lp += 66*4;

    lp[0*66] = zi3.x;
    lp[1*66] = zi3.y;
    lp[2*66] = zi3.z;
    lp[3*66] = zi3.w;

    barrier(CLK_LOCAL_MEM_FENCE);
	}
	// Fourth pass of 1K FFT
	 void FFT::kfft_pass4()
	{
		    __local float *lp;

    // Load registers
    lp = lds + ((me & 0x3) + ((me >> 2) & 0x3)*(66*4) + ((me >> 4) << 2));

    float4 zr0, zr1, zr2, zr3;

    zr0.x = lp[0*66];
    zr0.y = lp[1*66];
    zr0.z = lp[2*66];
    zr0.w = lp[3*66];
    lp += 16;

    zr1.x = lp[0*66];
    zr1.y = lp[1*66];
    zr1.z = lp[2*66];
    zr1.w = lp[3*66];
    lp += 16;

    zr2.x = lp[0*66];
    zr2.y = lp[1*66];
    zr2.z = lp[2*66];
    zr2.w = lp[3*66];
    lp += 16;

    zr3.x = lp[0*66];
    zr3.y = lp[1*66];
    zr3.z = lp[2*66];
    zr3.w = lp[3*66];
    lp += 66*4*4 - 3*16;

    float4 zi0, zi1, zi2, zi3;

    zi0.x = lp[0*66];
    zi0.y = lp[1*66];
    zi0.z = lp[2*66];
    zi0.w = lp[3*66];
    lp += 16;

    zi1.x = lp[0*66];
    zi1.y = lp[1*66];
    zi1.z = lp[2*66];
    zi1.w = lp[3*66];
    lp += 16;

    zi2.x = lp[0*66];
    zi2.y = lp[1*66];
    zi2.z = lp[2*66];
    zi2.w = lp[3*66];
    lp += 16;

    zi3.x = lp[0*66];
    zi3.y = lp[1*66];
    zi3.z = lp[2*66];
    zi3.w = lp[3*66];

    // Transform and twiddle
    FFT4();

    int tbase = (int)((me >> 4) << 6);
    TWIDDLE4();

    barrier(CLK_LOCAL_MEM_FENCE);

    // Save registers in conflict free manner
    lp = lds + me;

    lp[0*68] = zr0.x;
    lp[1*68] = zr0.y;
    lp[2*68] = zr0.z;
    lp[3*68] = zr0.w;
    lp += 68*4;

    lp[0*68] = zr1.x;
    lp[1*68] = zr1.y;
    lp[2*68] = zr1.z;
    lp[3*68] = zr1.w;
    lp += 68*4;

    lp[0*68] = zr2.x;
    lp[1*68] = zr2.y;
    lp[2*68] = zr2.z;
    lp[3*68] = zr2.w;
    lp += 68*4;

    lp[0*68] = zr3.x;
    lp[1*68] = zr3.y;
    lp[2*68] = zr3.z;
    lp[3*68] = zr3.w;
    lp += 68*4;

    // Imaginary part
    lp[0*68] = zi0.x;
    lp[1*68] = zi0.y;
    lp[2*68] = zi0.z;
    lp[3*68] = zi0.w;
    lp += 68*4;

    lp[0*68] = zi1.x;
    lp[1*68] = zi1.y;
    lp[2*68] = zi1.z;
    lp[3*68] = zi1.w;
    lp += 68*4;

    lp[0*68] = zi2.x;
    lp[1*68] = zi2.y;
    lp[2*68] = zi2.z;
    lp[3*68] = zi2.w;
    lp += 68*4;

    lp[0*68] = zi3.x;
    lp[1*68] = zi3.y;
    lp[2*68] = zi3.z;
    lp[3*68] = zi3.w;

    barrier(CLK_LOCAL_MEM_FENCE);
	}
	// Fifth and last pass of 1K FFT
	 void FFT::kfft_pass5()
	{
		const __local float *lp;

    // Load registers
    lp = lds + ((me & 0xf) + (me >> 4)*(68*4));

    float4 zr0, zr1, zr2, zr3;

    zr0.x = lp[0*68];
    zr0.y = lp[1*68];
    zr0.z = lp[2*68];
    zr0.w = lp[3*68];
    lp += 16;

    zr1.x = lp[0*68];
    zr1.y = lp[1*68];
    zr1.z = lp[2*68];
    zr1.w = lp[3*68];
    lp += 16;

    zr2.x = lp[0*68];
    zr2.y = lp[1*68];
    zr2.z = lp[2*68];
    zr2.w = lp[3*68];
    lp += 16;

    zr3.x = lp[0*68];
    zr3.y = lp[1*68];
    zr3.z = lp[2*68];
    zr3.w = lp[3*68];

    lp += 68*4*4 - 3*16;

    float4 zi0, zi1, zi2, zi3;

    zi0.x = lp[0*68];
    zi0.y = lp[1*68];
    zi0.z = lp[2*68];
    zi0.w = lp[3*68];
    lp += 16;

    zi1.x = lp[0*68];
    zi1.y = lp[1*68];
    zi1.z = lp[2*68];
    zi1.w = lp[3*68];
    lp += 16;

    zi2.x = lp[0*68];
    zi2.y = lp[1*68];
    zi2.z = lp[2*68];
    zi2.w = lp[3*68];
    lp += 16;

    zi3.x = lp[0*68];
    zi3.y = lp[1*68];
    zi3.z = lp[2*68];
    zi3.w = lp[3*68];

    // Transform
    FFT4();

    // Save result
    __global float4 *gp = (__global float4 *)(gr + (me << 2));
    gp[0*64] = zr0;
    gp[1*64] = zr1;
    gp[2*64] = zr2;
    gp[3*64] = zr3;

    gp = (__global float4 *)(gi + (me << 2));
    gp[0*64] = zi0;
    gp[1*64] = zi1;
    gp[2*64] = zi2;
    gp[3*64] = zi3;
	}


__kernel void kfft(__global float *greal, __global float *gimag)
{
    // This is 8704 bytes
    __local float lds[68*4*4*2];

	FFT fftObj;
	fftObj.Init(greal,gimag,lds);
	fftObj.passAll();
}
