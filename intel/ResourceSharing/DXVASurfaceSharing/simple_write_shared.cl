// Copyright (c) 2009-2011 Intel Corporation
// All rights reserved.
// 
// WARRANTY DISCLAIMER
// 
// THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
// MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Intel Corporation is the author of the Materials, and requests that all
// problem reports or change requests be submitted to it directly

int clip(int x)
{
	if(x > 255)
		return 255;
	
	if(x < 0)
		return 0;
		
	return x;
}

__kernel void simple_write_shared(__global unsigned char *p, __write_only image2d_t YOut, __write_only image2d_t UVOut, __global int* orX, __global int* orY)
{
	const int x = get_global_id(0);		// Cuda equivalent : blockIdx.x*bw + tx
	const int y = get_global_id(1);		// Cuda equivalent : blockIdx.y*bh + ty

	float4 pixel = (float4)0;
	float4 color;

	int2 coord = (int2)(x, y); 
        const sampler_t smp = CLK_FILTER_NEAREST; 
	
	int width=640;
	int amount = width*480;
    int Y,U,V;
	float R,G,B;
	int originalY;
	
	int originX = *orX;
	int originY = *orY;
	
	int imageWidth=1920;
	
	R = (int)p[x/2*4 + originX*4 + 2 + y/2*imageWidth*4 + originY*imageWidth*4];
	G = (int)p[x/2*4 + originX*4 + 1 + y/2*imageWidth*4 + originY*imageWidth*4];
	B = (int)p[x/2*4 + originX*4 + 0 + y/2*imageWidth*4 + originY*imageWidth*4];
	
	clip(R);
	clip(G);
	clip(B);

	R=R/255.0;
	G=G/255.0;
	B=B/255.0;
	
	Y = ( 66 * R + 129 * G +  25 * B + 16);
    U = (-38 * R -  74 * G + 112 * B + 128);
    V = (112 * R - 94 * G -  18 * B + 128);
	
	pixel = (float4)(Y/255.0,0.0,0.0,0.0);
	write_imagef(YOut, coord, pixel); 

	if (x%2 == 0 && y%2 == 0)
	{
		int2 coordUV = (int2)(x/2, y/2); 
		pixel = (float4)(U/255.0,V/255.0,0.0,0.0);
		write_imagef(UVOut, coordUV, pixel); 
	}
	

	}
