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

#include "stdafx.h"

void EvaluateRaw(float* inputArray, float* outputArray, CHDRData *pData, int arrayWidth, int iRow)
{
    __m128 fPowKLow = _mm_set_ps1( pData->fPowKLow);
    __m128 fFStops = _mm_set_ps1( pData->fFStops );
    __m128 fFStopsInv = _mm_set_ps1(pData->fFStopsInv);
    __m128 fPowExposure = _mm_set_ps1(pData->fPowExposure);
    __m128 fDefog = _mm_set_ps1(pData->fDefog);
    __m128 fGamma = _mm_set_ps1(pData->fGamma);
    __m128 fPowGamma = _mm_set_ps1(pData->fPowGamma);

    // and define method constants.
    __m128 fOne = _mm_set_ps1( 1.0f );
    __m128 fZerro = _mm_set_ps1( 0.0f );
    __m128 fSaturate = _mm_set_ps1( 255.f );

    for (int iCol = 0; iCol < arrayWidth; iCol++)
    {
        __m128 fColor = _mm_setzero_ps( );
        fColor = _mm_load_ps((float*)((char*)inputArray+iRow*arrayWidth*4*sizeof(float)+ iCol*4*sizeof(float)));

        //Defog 
        fColor = _mm_sub_ps(fColor, fDefog);
        fColor = _mm_max_ps(fZerro, fColor);

        // Multiply color by pow( 2.0f, exposure +  2.47393f )
        fColor = _mm_mul_ps( fColor, fPowExposure ); 

        // Apply a knee function (Please, refer to the OpenEXR algorithm).
        __m128 fCmpFlag = _mm_cmpge_ps( fColor, fPowKLow );

        if(( _mm_movemask_ps( fCmpFlag ) & 7) != 0 ) 
        {
            __m128 fTmpPixel = _mm_sub_ps( fColor, fPowKLow );
            fTmpPixel = _mm_mul_ps( fTmpPixel, fFStops );
            fTmpPixel = _mm_add_ps( fTmpPixel, fOne );

            //fTmpPixel = logf( fTmpPixel);
            ((float*)&fTmpPixel)[0] = logf(((float*)&fTmpPixel)[0]); 
            ((float*)&fTmpPixel)[1] = logf(((float*)&fTmpPixel)[1]);
            ((float*)&fTmpPixel)[2] = logf(((float*)&fTmpPixel)[2]);
            ((float*)&fTmpPixel)[3] = logf(((float*)&fTmpPixel)[3]);    

            fTmpPixel = _mm_mul_ps( fTmpPixel, fFStopsInv );
            fColor = _mm_add_ps( fTmpPixel, fPowKLow );
        }

        //Gamma correction
        //fColor = pow(fColor, fGamma);
        ((float*)&fColor)[0] = pow(((float*)&fColor)[0], ((float*)&fGamma)[0]); 
        ((float*)&fColor)[1] = pow(((float*)&fColor)[1], ((float*)&fGamma)[1]);
        ((float*)&fColor)[2] = pow(((float*)&fColor)[2], ((float*)&fGamma)[2]);
        ((float*)&fColor)[3] = pow(((float*)&fColor)[3], ((float*)&fGamma)[3]);    

        // Scale the values
        fColor = _mm_mul_ps( fColor, fSaturate );
        fColor = _mm_mul_ps( fColor, fPowGamma );

        //Saturate
        fColor = _mm_max_ps( fColor, _mm_setzero_ps( ));
        fColor = _mm_min_ps( fColor, fSaturate );

        //Store result pixel 
        _mm_store_ps((float*)((char*)outputArray+iRow*arrayWidth*4*sizeof(float)+ iCol*4*sizeof(float)), fColor);
    }
}
