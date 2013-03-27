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


typedef struct 
{
    float fPowKLow;                             //fPowKLow = pow( 2.0f, kLow)
    float fPowKHigh;                            //fPowKHigh = pow( 2.0f, kHigh)
    float fPow35;                               //fPow35 = pow( 2.0f, 3.5f)
    float fFStops;                              //F stops
    float fFStopsInv;                           //Invesrse fFStops value
    float fPowExposure;                         //fPowExposure = pow( 2.0f, exposure +  2.47393f )
    float fGamma;                               //Gamma correction parameter
    float fPowGamma;                            //Scale factor
    float fDefog;                               //Defog value
} CHDRData;

// "__attribute__((vec_type_hint(float4)))" can be enabled to avoid compiler autovectorization

__kernel void /*__attribute__((vec_type_hint(float4)))*/  ToneMappingLine(
	__global float4* inputImage, 
	__global float4* outputImage,
    __global CHDRData* pData,
    int iImageWidth
    )
{
	// Load tone mapping parameters
    float4 fPowKLow = (float4)pData->fPowKLow;
    float4 fFStops = (float4)pData->fFStops;
    float4 fFStopsInv = (float4)pData->fFStopsInv;
    float4 fPowExposure = (float4)pData->fPowExposure;
    float4 fDefog = (float4)pData->fDefog;
    float4 fGamma = (float4)pData->fGamma;
    float4 fPowGamma = (float4)pData->fPowGamma;


	// and define method constants.
    float4 fOne = 1.0f;
    float4 fZerro = 0.0f;
    float4 fSaturate = 255.f;
    
    
    int iRow = get_global_id(0);
    

    for (int iCol = 0; iCol< iImageWidth; iCol++)
    {
        float4 fColor = 0.0f;

        fColor = inputImage[iRow*iImageWidth+iCol];


        //Defog 
        fColor = fColor - fDefog;
        fColor = max(fZerro, fColor);


        // Multiply color by pow( 2.0f, exposure +  2.47393f )
        fColor = fColor * fPowExposure;

        int4 iCmpFlag = 0;
        //iCmpFlag = isgreater(fColor, fPowKLow);
        iCmpFlag = fColor > fPowKLow;

        if(any(iCmpFlag))
        {

            //fPowKLow = 2^kLow
            //fFStopsInv = 1/fFStops;
            //fTmpPixel = fPowKLow + log((fTmpPixel-fPowKLow) * fFStops + 1.0f)*fFStopsInv;
            float4 fTmpPixel = fColor - fPowKLow;
            fTmpPixel = fTmpPixel * fFStops;
            fTmpPixel = fTmpPixel + fOne;
            fTmpPixel = native_log( fTmpPixel );
            fTmpPixel = fTmpPixel * fFStopsInv;
            fTmpPixel = fTmpPixel + fPowKLow;

			//color channels update versions
            ///fColor = select(fTmpPixel, fColor, iCmpFlag);
            ///fColor = select(fColor, fTmpPixel, iCmpFlag);
            fColor = fTmpPixel;
        }

        //Gamma correction
        fColor = powr(fColor, fGamma);

        // Scale the values
        fColor = fColor * fSaturate;
        fColor = fColor * fPowGamma;

        //Saturate
        fColor = max(fColor, 0.f);
        fColor = min(fColor, fSaturate);

        //Store result pixel 
        outputImage[iRow*iImageWidth+iCol] = fColor;
    }
}

__kernel void /*__attribute__((vec_type_hint(float4)))*/  ToneMappingPerPixel(
	__global float4* inputImage, 
	__global float4* outputImage,
    __global CHDRData* pData,
    int iImageWidth
    )
{
	// Load tone mapping parameters
    float4 fPowKLow = (float4)pData->fPowKLow;
    float4 fFStops = (float4)pData->fFStops;
    float4 fFStopsInv = (float4)pData->fFStopsInv;
    float4 fPowExposure = (float4)pData->fPowExposure;
    float4 fDefog = (float4)pData->fDefog;
    float4 fGamma = (float4)pData->fGamma;
    float4 fPowGamma = (float4)pData->fPowGamma;


	// and define method constants.
    float4 fOne = 1.0f;
    float4 fZerro = 0.0f;
    float4 fSaturate = 255.f;
    
    
    int iRow = get_global_id(0);
    int iCol = get_global_id(1);

    ///for (int iCol = 0; iCol< iImageWidth; iCol++)
    {
        float4 fColor = 0.0f;

        fColor = inputImage[iRow*iImageWidth+iCol];


        //Defog 
        fColor = fColor - fDefog;
        fColor = max(fZerro, fColor);


        // Multiply color by pow( 2.0f, exposure +  2.47393f )
        fColor = fColor * fPowExposure;

        int4 iCmpFlag = 0;
        //iCmpFlag = isgreater(fColor, fPowKLow);
        iCmpFlag = fColor > fPowKLow;

        if(any(iCmpFlag))
        {

            //fPowKLow = 2^kLow
            //fFStopsInv = 1/fFStops;
            //fTmpPixel = fPowKLow + log((fTmpPixel-fPowKLow) * fFStops + 1.0f)*fFStopsInv;
            float4 fTmpPixel = fColor - fPowKLow;
            fTmpPixel = fTmpPixel * fFStops;
            fTmpPixel = fTmpPixel + fOne;
            fTmpPixel = native_log( fTmpPixel );
            fTmpPixel = fTmpPixel * fFStopsInv;
            fTmpPixel = fTmpPixel + fPowKLow;

			//color channels update versions
            ///fColor = select(fTmpPixel, fColor, iCmpFlag);
            ///fColor = select(fColor, fTmpPixel, iCmpFlag);
            fColor = fTmpPixel;
        }

        //Gamma correction
        fColor = powr(fColor, fGamma);

        // Scale the values
        fColor = fColor * fSaturate;
        fColor = fColor * fPowGamma;

        //Saturate
        fColor = max(fColor, 0.f);
        fColor = min(fColor, fSaturate);

        //Store result pixel 
        outputImage[iRow*iImageWidth+iCol] = fColor;
    }
}
