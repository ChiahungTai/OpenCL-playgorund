/**********************************************************************
Copyright ©2012 Advanced Micro Devices, Inc. All rights reserved.

C++ AMP kernels within this source tree are derivatives of kernels
from the SHOC project. Source or binary distribution of this project must
disclose derivation and include the SHOC license:

SHOC 1.1.2  license Copyright ©2011, UT-Battelle, LLC. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.
•	Neither the name of Oak Ridge National Laboratory, nor UT-Battelle, LLC, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

/**
*********************************************************************************
* @file <FFT.cpp> 
* @brief Implementation of FFT algrithm
********************************************************************************/
#include "FFT.hpp"

/********************************************************************************
 *     template function::toString()                                            *
 *******************************************************************************/
 template<typename T>
 std::string toString(T t, std::ios_base &(*r)(std::ios_base&))
 {
     
      std::ostringstream output;
      output << r << t;
      return output.str();
 }
/********************************************************************************
* Support functions for FFT kernel below.                                       *
********************************************************************************/
template <typename FPTYPE> FPTYPE FFTPreciseCos(FPTYPE phi) restrict(amp);
template <> float  FFTPreciseCos<float >(float  phi) restrict(amp) { return precise_math::cosf(phi); }
template <> double FFTPreciseCos<double>(double phi) restrict(amp) { return precise_math::cos(phi);  }

template <typename FPTYPE> FPTYPE FFTPreciseSin(FPTYPE phi) restrict(amp);
template <> float  FFTPreciseSin<float >(float  phi) restrict(amp) { return precise_math::sinf(phi); }
template <> double FFTPreciseSin<double>(double phi) restrict(amp) { return precise_math::sin(phi);  }

/********************************************************************************
* Implementation of FFTComplexMul                                               *
********************************************************************************/
template <typename FPTYPE2>
FPTYPE2 FFTComplexMul(FPTYPE2 a, FPTYPE2 b) restrict(amp) 
{
    return FPTYPE2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

/********************************************************************************
* Implementation of short_vector<FPTYPE, 2>::type FFTExpI(FPTYPE phi)           *
********************************************************************************/
template <typename FPTYPE>
typename short_vector<FPTYPE, 2>::type FFTExpI(FPTYPE phi) restrict(amp) 
{
    typedef typename short_vector<FPTYPE, 2>::type FPTYPE2;
    return FPTYPE2(FFTPreciseCos(phi), FFTPreciseSin(phi));
}

/********************************************************************************
* Implementation of FFTLoad8                                                    *
********************************************************************************/
template <typename FPTYPE2>
void FFTLoad8(FPTYPE2 *t, FPTYPE2 *v, int vStride) restrict(amp)
{
    // Read strided t0-t7 elements from vector tile.
    t[0] = v[0 * vStride]; t[1] = v[1 * vStride]; t[2] = v[2 * vStride]; t[3] = v[3 * vStride];
    t[4] = v[4 * vStride]; t[5] = v[5 * vStride]; t[6] = v[6 * vStride]; t[7] = v[7 * vStride];
}

/********************************************************************************
* Implementation of FFTStore8                                                   *
********************************************************************************/
template <typename FPTYPE2>
void FFTStore8(FPTYPE2 *t, FPTYPE2 *v, int vStride) restrict(amp)
{
    // Write strided t0-t7 elements to vector tile.
    v[0 * vStride] = t[0]; v[1 * vStride] = t[4]; v[2 * vStride] = t[2]; v[3 * vStride] = t[6];
    v[4 * vStride] = t[1]; v[5 * vStride] = t[5]; v[6 * vStride] = t[3]; v[7 * vStride] = t[7];
}

/********************************************************************************
* Implementation of FFTStoreX8                                                  *
********************************************************************************/
template <typename FPTYPE, typename FPTYPE2>
void FFTStoreX8(FPTYPE2 *t, FPTYPE *tTmp, int tTmpStr) restrict(amp)
{
    // Write strided t0-t7.x components to local memory.
    // Swizzle order is 0, 4, 2, 6, 1, 5, 3, 7.
    tTmp[0 * tTmpStr] = t[0].x; tTmp[1 * tTmpStr] = t[4].x; tTmp[2 * tTmpStr] = t[2].x; tTmp[3 * tTmpStr] = t[6].x;
    tTmp[4 * tTmpStr] = t[1].x; tTmp[5 * tTmpStr] = t[5].x; tTmp[6 * tTmpStr] = t[3].x; tTmp[7 * tTmpStr] = t[7].x;
}

/********************************************************************************
* Implementation of FFTStoreY8                                                  *
********************************************************************************/
template <typename FPTYPE, typename FPTYPE2>
void FFTStoreY8(FPTYPE2 *t, FPTYPE *tTmp, int tTmpStr) restrict(amp)
{
    // Write strided t0-t7.y components to local memory.
    // Swizzle order is 0, 4, 2, 6, 1, 5, 3, 7.
    tTmp[0 * tTmpStr] = t[0].y; tTmp[1 * tTmpStr] = t[4].y; tTmp[2 * tTmpStr] = t[2].y; tTmp[3 * tTmpStr] = t[6].y;
    tTmp[4 * tTmpStr] = t[1].y; tTmp[5 * tTmpStr] = t[5].y; tTmp[6 * tTmpStr] = t[3].y; tTmp[7 * tTmpStr] = t[7].y;
}

/********************************************************************************
* Implementation of FFTLoadX8                                                   *
********************************************************************************/
template <typename FPTYPE, typename FPTYPE2>
void FFTLoadX8(FPTYPE2 *t, FPTYPE *tTmp, int tTmpStr) restrict(amp)
{
    // Read strided t0-t7.x components from local memory.
    t[0].x = tTmp[0 * tTmpStr]; t[1].x = tTmp[1 * tTmpStr]; t[2].x = tTmp[2 * tTmpStr]; t[3].x = tTmp[3 * tTmpStr];
    t[4].x = tTmp[4 * tTmpStr]; t[5].x = tTmp[5 * tTmpStr]; t[6].x = tTmp[6 * tTmpStr]; t[7].x = tTmp[7 * tTmpStr];
}

/********************************************************************************
* Implementation of FFTLoadY8                                                   *
********************************************************************************/
template <typename FPTYPE, typename FPTYPE2>
void FFTLoadY8(FPTYPE2 *t, FPTYPE *tTmp, int tTmpStr) restrict(amp)
{
    // Read strided t0-t7.y components from local memory.
    t[0].y = tTmp[0 * tTmpStr]; t[1].y = tTmp[1 * tTmpStr]; t[2].y = tTmp[2 * tTmpStr]; t[3].y = tTmp[3 * tTmpStr];
    t[4].y = tTmp[4 * tTmpStr]; t[5].y = tTmp[5 * tTmpStr]; t[6].y = tTmp[6 * tTmpStr]; t[7].y = tTmp[7 * tTmpStr];
}

/********************************************************************************
* Implementation of FFT<FPTYPE, INVERSE, FPTYPE2>::outputData()                 *
********************************************************************************/
template <typename FPTYPE, bool INVERSE, typename FPTYPE2>
void FFT<FPTYPE, INVERSE, FPTYPE2>::outputData(const std::string &modeText,
                                               double workDone,
                                               const std::string &workRateUnit)
{
    if(iter >= 1) 
    {
        // Print time information while using array
        if(timing && (!arrayview))
        {
            double meanMs = (totalkernelTime * 1000.0) / iter;
            double workRate = workDone / (meanMs / 1000.0);
            int workRatePrecision = (workRate >= 100.0 ? 0 : 1);

            std::cout << "(Total time(sec): " << totalTime << ")\n";       
            std::cout << "\nTime Information" << std::endl;
            std::string strArray[4] = {"Data Transfer to Accelerator(sec)", "Mean Execution Time (sec)", workRateUnit, "Data Transfer to Host(sec)"};
            std::string stats[4];
            stats[0] = toString<double>(cpyToGpuTime, std::dec);
            stats[1] = toString<double>(meanMs / 1000.0, std::dec);
            stats[2] = toString<double>(workRate, std::dec);
            stats[3] = toString<double>(cpyToHostTime, std::dec);

            this->AmpSample::printStats(strArray, stats, 4);
        }
        else if(timing && arrayview)
        {
            // Print time information while using array_view
            std::cout << "(Total time(sec): " << totalTime << ") ";
        }
    }
    else
    {
       std::cout << " (insufficient runs for timing)\n";
    }

}

/********************************************************************************
* Implementation of FFT<FPTYPE, INVERSE, FPTYPE2>::outputSummary()              *
********************************************************************************/
template <typename FPTYPE, bool INVERSE, typename FPTYPE2>
int FFT<FPTYPE, INVERSE, FPTYPE2>::outputSummary()
{
	std::cout << "\n";
	if(iter < 1)
	{
        std::cout << "Iteration should be bigger than 0." << std::endl;
		return AMP_FAILURE;
	}
    std::cout << "Sampling: " << iter;
    std::cout << " (" << iter << " sampled) benchmark runs\n";
    std::cout << "\n";
    std::cout.flush();

	return AMP_SUCCESS;
}

/********************************************************************************
* Implementation of FFT<FPTYPE, INVERSE, FPTYPE2>::FFT_Amp()                    *
********************************************************************************/
template <typename FPTYPE, bool INVERSE, typename FPTYPE2>
void FFT<FPTYPE, INVERSE, FPTYPE2>::FFT_Amp(array<FPTYPE2, 1> &devVector,
                                            array<FPTYPE2, 1> &devOutVector,
                                            int vectorElems)
{
    // Compute forwards or inverse 512-point FFT across a wide complex vector.
    // N/512x1 grid, 64x1 tiles each responsible for 512x1 output.
    parallel_for_each(tiled_extent<64>(extent<1>(vectorElems / (cNFFTPoints / 64))),
        [&devVector, &devOutVector,vectorElems](tiled_index<64> idx) restrict(amp) 
    {
        // 64x8 set of values (padded for bank conflicts) for tile exchange/transpose.
        tile_static FPTYPE tTmp[64 * 9];

        // Find the tile we're computing inside the vector.
        FPTYPE2 *vecTile = &devVector[idx.tile[0] * cNFFTPoints + idx.local[0]];
        FPTYPE2 *vecOutTile = &devOutVector[idx.tile[0] * cNFFTPoints + idx.local[0]];

        // Read 8 complex numbers from the tile at 64 element strides.
        FPTYPE2 t[8];
        FFTLoad8(t, vecTile, 64);

        // Radix-2 butterfly.
        #define FFT2(a0, a1)                                                                        \
        {                                                                                           \
            FPTYPE2 ta0 = a0;                                                                       \
            a0 = ta0 + a1;                                                                          \
            a1 = ta0 - a1;                                                                          \
        }

        // Radix-4 butterfly.
        #define FFT4(a0, a1, a2, a3)                                                                \
            FFT2(a0, a2);                                                                           \
            FFT2(a1, a3);                                                                           \
                a3 = FFTComplexMul(a3, FPTYPE2(0, -1 * DIR));                                       \
            FFT2(a0, a1);                                                                           \
            FFT2(a2, a3);

        // Radix-8 butterfly.
        #define FFT8()                                                                              \
            FFT2(t[0], t[4]);                                                                       \
            FFT2(t[1], t[5]);                                                                       \
            FFT2(t[2], t[6]);                                                                       \
            FFT2(t[3], t[7]);                                                                       \
                t[5] = FFTComplexMul(t[5], FPTYPE2( 1, -1 * DIR)) * FPTYPE(M_SQRT1_2);              \
                t[6] = FFTComplexMul(t[6], FPTYPE2( 0, -1 * DIR));                                  \
                t[7] = FFTComplexMul(t[7], FPTYPE2(-1, -1 * DIR)) * FPTYPE(M_SQRT1_2);              \
            FFT4(t[0], t[1], t[2], t[3]);                                                           \
            FFT4(t[4], t[5], t[6], t[7]);

        // Cooley-Tukey twiddle factors.
        #define FFTTwiddle8(i, n)                                                                   \
            t[1] = FFTComplexMul(t[1], FFTExpI((FPTYPE(-2.0 * DIR * M_PI * 4.0) * n) * FPTYPE(i))); \
            t[2] = FFTComplexMul(t[2], FFTExpI((FPTYPE(-2.0 * DIR * M_PI * 2.0) * n) * FPTYPE(i))); \
            t[3] = FFTComplexMul(t[3], FFTExpI((FPTYPE(-2.0 * DIR * M_PI * 6.0) * n) * FPTYPE(i))); \
            t[4] = FFTComplexMul(t[4], FFTExpI((FPTYPE(-2.0 * DIR * M_PI * 1.0) * n) * FPTYPE(i))); \
            t[5] = FFTComplexMul(t[5], FFTExpI((FPTYPE(-2.0 * DIR * M_PI * 5.0) * n) * FPTYPE(i))); \
            t[6] = FFTComplexMul(t[6], FFTExpI((FPTYPE(-2.0 * DIR * M_PI * 3.0) * n) * FPTYPE(i))); \
            t[7] = FFTComplexMul(t[7], FFTExpI((FPTYPE(-2.0 * DIR * M_PI * 7.0) * n) * FPTYPE(i)));

        // Cooley-Tukey transposition.
        #define FFTTranspose(tTmpStPtr, tTmpStStr, tTmpLdPtr, tTmpLdStr, barrierAfter)            \
            FFTStoreX8(t, tTmpStPtr, tTmpStStr);                                                  \
            idx.barrier.wait_with_tile_static_memory_fence();                                     \
            FFTLoadX8 (t, tTmpLdPtr, tTmpLdStr);                                                  \
            idx.barrier.wait_with_tile_static_memory_fence();                                     \
            FFTStoreY8(t, tTmpStPtr, tTmpStStr);                                                  \
            idx.barrier.wait_with_tile_static_memory_fence();                                     \
            FFTLoadY8 (t, tTmpLdPtr, tTmpLdStr);                                                  \
            if(barrierAfter) idx.barrier.wait_with_tile_static_memory_fence();

        int tileIdxHi = idx.local[0] >> 3;
        int tileIdxLo = idx.local[0]  & 7;

        // Successive steps of Cooley-Tukey.
        FFT8();
        FFTTwiddle8(idx.local[0], FPTYPE(1.0 / FPTYPE(cNFFTPoints)));
        FFTTranspose(&tTmp[tileIdxHi * 8 + tileIdxLo], 66, &tTmp[tileIdxLo * 66 + tileIdxHi], 8, false);
        FFT8();
        FFTTwiddle8(tileIdxHi, FPTYPE(1.0 / 64.0));
        FFTTranspose(&tTmp[tileIdxHi * 8 + tileIdxLo], 72, &tTmp[tileIdxHi * 72 + tileIdxLo], 8, true);
        FFT8();

        // Final scale only for IFFT.
        if(false && DIR == -1)
        {
            const FPTYPE scale = FPTYPE(1.0 / cNFFTPoints);
            t[0] *= scale; t[1] *= scale; t[2] *= scale; t[3] *= scale;
            t[4] *= scale; t[5] *= scale; t[6] *= scale; t[7] *= scale;
        }

        FFTStore8(t, vecOutTile,64);
        #undef FFTTranspose
        #undef FFTTwiddle8
        #undef FFT8
        #undef FFT4
        #undef FFT2
    }
    );
}

/********************************************************************************
* Implementation of FFT<FPTYPE, INVERSE, FPTYPE2>::FFT_Amp_View()               *
********************************************************************************/
template <typename FPTYPE, bool INVERSE, typename FPTYPE2>
void FFT<FPTYPE, INVERSE, FPTYPE2>::FFT_Amp_View(array_view<FPTYPE2, 1> &devVector,
                                                 array_view<FPTYPE2, 1> &devOutVector,
                                                 int vectorElems)
{

    // Compute forwards or inverse 512-point FFT across a wide complex vector.
    // N/512x1 grid, 64x1 tiles each responsible for 512x1 output.
    parallel_for_each(tiled_extent<64>(extent<1>(vectorElems / (cNFFTPoints / 64))),
        [=](tiled_index<64> idx) restrict(amp) 
    {
        // 64x8 set of values (padded for bank conflicts) for tile exchange/transpose.
        tile_static FPTYPE tTmp[64 * 9];

        //// Find the tile we're computing inside the vector.
        FPTYPE2 *vecTile = &devVector[idx.tile[0] * cNFFTPoints + idx.local[0]];
        FPTYPE2 *vecOutTile = &devOutVector[idx.tile[0] * cNFFTPoints + idx.local[0]];

        // Read 8 complex numbers from the tile at 64 element strides.
        FPTYPE2 t[8];
        FFTLoad8(t, vecTile, 64);

        // Radix-2 butterfly.
        #define FFT2(a0, a1)                                                                      \
            {                                                                                     \
                FPTYPE2 ta0 = a0;                                                                 \
                a0 = ta0 + a1;                                                                    \
                a1 = ta0 - a1;                                                                    \
            }

        // Radix-4 butterfly.
        #define FFT4(a0, a1, a2, a3)                                                              \
            FFT2(a0, a2);                                                                         \
            FFT2(a1, a3);                                                                         \
            a3 = FFTComplexMul(a3, FPTYPE2(0, -1 * DIR));                                         \
            FFT2(a0, a1);                                                                         \
            FFT2(a2, a3);

        // Radix-8 butterfly.
        #define FFT8()                                                                            \
            FFT2(t[0], t[4]);                                                                     \
            FFT2(t[1], t[5]);                                                                     \
            FFT2(t[2], t[6]);                                                                     \
            FFT2(t[3], t[7]);                                                                     \
            t[5] = FFTComplexMul(t[5], FPTYPE2( 1, -1 * DIR)) * FPTYPE(M_SQRT1_2);                \
            t[6] = FFTComplexMul(t[6], FPTYPE2( 0, -1 * DIR));                                    \
            t[7] = FFTComplexMul(t[7], FPTYPE2(-1, -1 * DIR)) * FPTYPE(M_SQRT1_2);                \
            FFT4(t[0], t[1], t[2], t[3]);                                                         \
            FFT4(t[4], t[5], t[6], t[7]);

        // Cooley-Tukey twiddle factors.
        #define FFTTwiddle8(i, n)                                                                 \
            t[1] = FFTComplexMul(t[1], FFTExpI((FPTYPE(-2.0 * DIR * M_PI * 4.0) * n) * FPTYPE(i))); \
            t[2] = FFTComplexMul(t[2], FFTExpI((FPTYPE(-2.0 * DIR * M_PI * 2.0) * n) * FPTYPE(i))); \
            t[3] = FFTComplexMul(t[3], FFTExpI((FPTYPE(-2.0 * DIR * M_PI * 6.0) * n) * FPTYPE(i))); \
            t[4] = FFTComplexMul(t[4], FFTExpI((FPTYPE(-2.0 * DIR * M_PI * 1.0) * n) * FPTYPE(i))); \
            t[5] = FFTComplexMul(t[5], FFTExpI((FPTYPE(-2.0 * DIR * M_PI * 5.0) * n) * FPTYPE(i))); \
            t[6] = FFTComplexMul(t[6], FFTExpI((FPTYPE(-2.0 * DIR * M_PI * 3.0) * n) * FPTYPE(i))); \
            t[7] = FFTComplexMul(t[7], FFTExpI((FPTYPE(-2.0 * DIR * M_PI * 7.0) * n) * FPTYPE(i)));

        // Cooley-Tukey transposition.
        #define FFTTranspose(tTmpStPtr, tTmpStStr, tTmpLdPtr, tTmpLdStr, barrierAfter)            \
            FFTStoreX8(t, tTmpStPtr, tTmpStStr);                                                  \
            idx.barrier.wait_with_tile_static_memory_fence();                                     \
            FFTLoadX8 (t, tTmpLdPtr, tTmpLdStr);                                                  \
            idx.barrier.wait_with_tile_static_memory_fence();                                     \
            FFTStoreY8(t, tTmpStPtr, tTmpStStr);                                                  \
            idx.barrier.wait_with_tile_static_memory_fence();                                     \
            FFTLoadY8 (t, tTmpLdPtr, tTmpLdStr);                                                  \
            if(barrierAfter) idx.barrier.wait_with_tile_static_memory_fence();

        int tileIdxHi = idx.local[0] >> 3;
        int tileIdxLo = idx.local[0]  & 7;

        // Successive steps of Cooley-Tukey.
        FFT8();
        FFTTwiddle8(idx.local[0], FPTYPE(1.0 / FPTYPE(cNFFTPoints)));
        FFTTranspose(&tTmp[tileIdxHi * 8 + tileIdxLo], 66, &tTmp[tileIdxLo * 66 + tileIdxHi], 8, false);
        FFT8();
        FFTTwiddle8(tileIdxHi, FPTYPE(1.0 / 64.0));
        FFTTranspose(&tTmp[tileIdxHi * 8 + tileIdxLo], 72, &tTmp[tileIdxHi * 72 + tileIdxLo], 8, true);
        FFT8();

        // Final scale only for IFFT.
        if(false && DIR == -1)
        {
            const FPTYPE scale = FPTYPE(1.0 / cNFFTPoints);
            t[0] *= scale; t[1] *= scale; t[2] *= scale; t[3] *= scale;
            t[4] *= scale; t[5] *= scale; t[6] *= scale; t[7] *= scale;
        }

        FFTStore8(t, vecOutTile,64);

        #undef FFTTranspose
        #undef FFTTwiddle8
        #undef FFT8
        #undef FFT4
        #undef FFT2
    });
    
}

/********************************************************************************
* Implementation of FFT<FPTYPE, INVERSE, FPTYPE2>::initialize()                 *
********************************************************************************/
template <typename FPTYPE, bool INVERSE, typename FPTYPE2>
int FFT<FPTYPE, INVERSE, FPTYPE2>::initialize()
{
    //Call base class Initialize to get default configuration 
    if(this->AmpSample::initialize())
        return AMP_FAILURE;

    ampsdk::Option* array_length = new ampsdk::Option;
    CHECK_ALLOCATION(array_length,"Memory Allocation error.(array_length)");

    array_length->_sVersion = "s";
    array_length->_lVersion = "size";
    array_length->_description = " Data size in millions with range [ 1,20 ]";
    array_length->_type = ampsdk::CA_ARG_INT;
    array_length->_value = &length;
    sampleArgs->AddOption(array_length);
    delete array_length;

    ampsdk::Option* iteration_option = new ampsdk::Option;
    CHECK_ALLOCATION(iteration_option,"Memory Allocation error.(iteration_option)");

    iteration_option->_sVersion = "i";
    iteration_option->_lVersion = "iterations";
    iteration_option->_description = "Number of times to repeat each algorithm";
    iteration_option->_type = ampsdk::CA_ARG_INT;
    iteration_option->_value = &iter;

    sampleArgs->AddOption(iteration_option);
    delete iteration_option;

    ampsdk::Option* arrayView = new ampsdk::Option;
    CHECK_ALLOCATION(arrayView,"Memory Allocation error.(arrayView)");

    arrayView->_sVersion = "V";
    arrayView->_lVersion = "arrayview";
    arrayView->_description = "use array_view instead of array";
    arrayView->_type = ampsdk::CA_NO_ARGUMENT;
    arrayView->_value = &arrayview;

    sampleArgs->AddOption(arrayView);
    delete arrayView;

    return AMP_SUCCESS;
}

/********************************************************************************
* Implementation of int FFT<FPTYPE, INVERSE, FPTYPE2>::setup()                  *
********************************************************************************/
template <typename FPTYPE, bool INVERSE, typename FPTYPE2>
int FFT<FPTYPE, INVERSE, FPTYPE2>::setup()

{
    if(length > 20 || length < 1)
    {
        std:: cout << "Length should be between 1 and 20." << std::endl;
		return AMP_FAILURE;
    }

    // Randomize host-side vector for validation.
    vectorElems = (length * 1024 * 1024);
    vector = std::vector<FPTYPE2>(vectorElems);
    refDFTVector = std::vector<FPTYPE2>(vectorElems);
    outVector = std::vector<FPTYPE2>(vectorElems);

    std::uniform_real_distribution<FPTYPE> rngDist(-1.0, 1.0);
    std::mt19937 rng;

    for(int i = 0; i < vectorElems; ++ i) 
    {
        vector[i].x = rngDist(rng);
        vector[i].y = rngDist(rng);
    }

    return AMP_SUCCESS;
}

/********************************************************************************
*                   Implementation of runArray()                                *
********************************************************************************/
template <typename FPTYPE, bool INVERSE, typename FPTYPE2>
int FFT<FPTYPE, INVERSE, FPTYPE2>::runArray()
{
    std::cout << "Using Array!" << std::endl;
    // Allocate vector buffer on the device.
    array<FPTYPE2, 1> devVector(vectorElems);
    array<FPTYPE2, 1> devOutVector(vectorElems);

    // Extra array for warm up
    array<FPTYPE2, 1> devWarmup(vectorElems);

    int cpytoGpuTimer = sampleCommon->createTimer();
    sampleCommon->resetTimer(cpytoGpuTimer);
    sampleCommon->startTimer(cpytoGpuTimer);

    copy(vector.begin(),devVector);

    sampleCommon->stopTimer(cpytoGpuTimer);
    cpyToGpuTime = sampleCommon->readTimer(cpytoGpuTimer);

    //warm up
    FFT_Amp(devVector, devWarmup, vectorElems);

    accelerator_view &accView = accelerator().default_view;
    accView.flush();
    accView.wait();

    // compute kernel execution time.
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    // Repeat the benchmark and compute a mean kernel execution time.
    for(int run = 0; run < iter; ++run) 
    {
        // Execute and time the kernel.
        try 
        {
            FFT_Amp(devVector, devOutVector,vectorElems);
            accView.flush();
        }
        catch(Concurrency::runtime_exception &ex) 
        {
            std::cerr << "AMP runtime threw an exception:\n  " << ex.what() << "\n";
            std::cerr << "\nBenchmark aborted due to error\n\n";
            exit(1);
        }
    }
    accView.wait();
    sampleCommon->stopTimer(timer);
    totalkernelTime = sampleCommon->readTimer(timer);

    //copy device to HOST
    int cpytoHostTimer = sampleCommon->createTimer();
    sampleCommon->resetTimer(cpytoHostTimer);
    sampleCommon->startTimer(cpytoHostTimer);

    copy(devOutVector, outVector.begin());

    sampleCommon->stopTimer(cpytoHostTimer);
    cpyToHostTime = sampleCommon->readTimer(cpytoHostTimer);

    totalTime = cpyToGpuTime + totalkernelTime + cpyToHostTime;

    if(verify)
    {
        for(int start = 0; start < vectorElems; start += cNFFTPoints)
        {
            // Start a new FFT for this set of 512 complex numbers.
            ReferenceFFT(&vector[start], &refDFTVector[start], 512, 1);
        }
    }
    return AMP_SUCCESS;
}

/********************************************************************************
*                   Implementation of runArray_View()                           *
********************************************************************************/
template <typename FPTYPE, bool INVERSE, typename FPTYPE2>
int FFT<FPTYPE, INVERSE, FPTYPE2>::runArray_View()
{
    std::cout << "Using Array_View!" << std::endl;
    // Allocate vector buffer on the device.
    array_view<FPTYPE2, 1> devVector(vectorElems,vector);
    array_view<FPTYPE2, 1> devOutVector(vectorElems,outVector);

    //Extra array view for warm up.
    std::vector<FPTYPE2> warmupVecIn(vectorElems,0);
    std::vector<FPTYPE2> warmupVecOut(vectorElems,0);

    array_view<FPTYPE2, 1> devWarmupIn(vectorElems,warmupVecIn);
    array_view<FPTYPE2, 1> devWarmupOut(vectorElems,warmupVecOut);

    // accelerator view for synchronize
    accelerator_view &accView = accelerator().default_view;

    // warm up
    FFT_Amp_View(devWarmupIn, devWarmupOut,vectorElems);
    devOutVector.synchronize();

    // compute a total execution time.
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);
    // Repeat the benchmark and compute a mean kernel execution time.
    for(int run = 0; run < iter; ++run)
    {
        // Execute and time the kernel.
        try
        {
            FFT_Amp_View(devVector, devOutVector, vectorElems);
            accView.flush();
        }
        catch(Concurrency::runtime_exception &ex)
        {
            std::cerr << "AMP runtime threw an exception:\n  " << ex.what() << "\n";
            std::cerr << "\nBenchmark aborted due to error\n\n";
            exit(1);
        }
    }

    devOutVector.synchronize();
    sampleCommon->stopTimer(timer);
    totalTime = sampleCommon->readTimer(timer);
    if(verify)
    {
        for(int start = 0; start < vectorElems; start += cNFFTPoints)
        {
            // Start a new FFT for this set of 512 complex numbers.
            ReferenceFFT(&vector[start], &refDFTVector[start], 512, 1);
        }
    }
    return AMP_SUCCESS;
}

/********************************************************************************
* Implementation of int FFT<FPTYPE, INVERSE, FPTYPE2>::run()                    *
********************************************************************************/
template <typename FPTYPE, bool INVERSE, typename FPTYPE2>
int FFT<FPTYPE, INVERSE, FPTYPE2>::run()
{
    std::stringstream modeStr;
    modeStr << (sizeof(FPTYPE) == 4 ? "SP" : "DP") << " ";
    modeStr << (INVERSE ? "I" : " ") << "FFT " << cNFFTPoints << "-pt ";
    modeStr << length << "M complex numbers";
    std::cout << "Run " << modeStr.str() << std::fixed << std::setprecision(3) << "\n";

    // print the input, the data size is too big to print all out
    if(!quiet)
    {
        std::cout<<"\nInput\n";
        for(int i = 0; i < 128; ++i) 
        {
            std::cout << vector[i].x <<" ";
            std::cout << vector[i].y <<" ";
        }
        std::cout <<"\n";
        std::cout <<"\n";
    }

     if(arrayview)
    {
        if(runArray_View() != AMP_SUCCESS)
            return AMP_FAILURE;
    }
    else
    {
        if(runArray() != AMP_SUCCESS)
            return AMP_FAILURE;
    }

    // print the output, the data size is too big to print all out
    if(!quiet)
    {
        std::cout<<"\nOutput\n";
        for(int i = 0; i < 128; ++i) 
        {
            std::cout << outVector[i].x <<" ";
            std::cout << outVector[i].y <<" ";
        }
        std::cout <<"\n";
        std::cout <<"\n";
    }

    return AMP_SUCCESS;
}

/********************************************************************************
* Implementation of int FFT<FPTYPE, INVERSE, FPTYPE2>::printStats()             *
********************************************************************************/
template <typename FPTYPE, bool INVERSE, typename FPTYPE2>
void FFT<FPTYPE, INVERSE, FPTYPE2>::printStats()
{
    // Report mean/SD/throughput to the user.
    std::stringstream modeStr;
    modeStr << (sizeof(FPTYPE) == 4 ? "SP" : "DP") << " ";
    modeStr << (INVERSE ? "I" : " ") << "FFT " << cNFFTPoints << "-pt ";
    modeStr << length << "M complex numbers";

    std::cout << modeStr.str() << std::fixed << std::setprecision(3) << " finished!";

    int64_t nFLOP = int64_t(5.0 * cNFFTPoints * (log(cNFFTPoints) / log(2.0)) * (double(vectorElems) / cNFFTPoints));
    double nGFLOP = nFLOP / 1000000000.0;
    outputData(modeStr.str(), nGFLOP, "GFLOPS");
    std::cout << "\n";
    std::cout << "\n";
}


/********************************************************************************
* Implementation of int FFT<FPTYPE, INVERSE, FPTYPE2>:: ReferenceFFT()          *
********************************************************************************/
template <typename FPTYPE, bool INVERSE, typename FPTYPE2>
void FFT<FPTYPE, INVERSE, FPTYPE2>:: ReferenceFFT(FPTYPE2 *vecTile,
                                                  FPTYPE2 *refDFTTile,
                                                  int nPoints,
                                                  int stride)
{
    if(nPoints == 1)
    {
        refDFTTile[0] = vecTile[0];
    }
    else
    {
        int hn = nPoints / 2;
        ReferenceFFT(vecTile,          refDFTTile,      hn, stride * 2);
        ReferenceFFT(vecTile + stride, refDFTTile + hn, hn, stride * 2);

        for(int i = 0; i < hn; ++i)
        {
            FPTYPE  phi = FPTYPE(-2.0 * M_PI) * FPTYPE(i) / FPTYPE(nPoints);
            FPTYPE2 expPhi(cos(phi), DIR * sin(phi));

            FPTYPE2 expPhiMul
                (
                expPhi.x * refDFTTile[i + hn].x - expPhi.y * refDFTTile[i + hn].y,
                expPhi.x * refDFTTile[i + hn].y + expPhi.y * refDFTTile[i + hn].x
                );

            FPTYPE2 oldVal = refDFTTile[i];
            refDFTTile[i     ] = oldVal + expPhiMul;
            refDFTTile[i + hn] = oldVal - expPhiMul;
        }
    }
}


/********************************************************************************
* Implementation of int FFT<FPTYPE, INVERSE, FPTYPE2>::verifyResults()          *
********************************************************************************/
template <typename FPTYPE, bool INVERSE, typename FPTYPE2>
int FFT<FPTYPE, INVERSE, FPTYPE2>::verifyResults()
{
    doValidation = verify;
    if(doValidation) 
    {
        // Compute (I)FFT into a reference array.
        std::cout << "  [Validating... ";
        std::cout.flush();

        for(int i = 0; i < vectorElems; ++ i) 
        {
            // Compare the reference and computed values.
            FPTYPE2 refDFT    = refDFTVector[i];
            FPTYPE2 actualDFT = outVector[i];

            if(fabs(refDFT.x - actualDFT.x) > 0.0001 ||
                fabs(refDFT.y - actualDFT.y) > 0.0001)
            {
                std::cerr << "failed at " << i << ": ";
                std::cerr << "expected " << refDFT.x << "x" << refDFT.y;
                std::cerr << " but have " << actualDFT.x << "x" << actualDFT.y << "]\n\n";
                std::cerr << "Benchmark aborted due to error\n";
                exit(1);
            }
        }

        std::cout << "passed]\n";
    }
    return AMP_SUCCESS;

}

/********************************************************************************
* Implementation of int FFT<FPTYPE, INVERSE, FPTYPE2>::cleanUp()                *
********************************************************************************/
template <typename FPTYPE, bool INVERSE, typename FPTYPE2>
void FFT<FPTYPE, INVERSE, FPTYPE2>::cleanUp()
{
    std::vector<FPTYPE2>().swap(this->vector);
    std::vector<FPTYPE2>().swap(this->refDFTVector);
    std::vector<FPTYPE2>().swap(this->outVector);
}

/********************************************************************************
* Execution of program begins from here                                         *
********************************************************************************/
int main(int argc,
         char *argv[])
{
    std::cout << "************************************************" << std::endl;
    std::cout << "                       FFT " << std::endl;
    std::cout << "************************************************" << std::endl;
    std::cout << std::endl << std::endl;
    
    int status = 0;

    /****************************************************************************
    * Create an object of FFT object(float,false)                               *
    ****************************************************************************/
    FFT<float,false,float2> fftff("C++AMP FFT float false");
   
    /****************************************************************************
    * Initialize the options of the sample                                      *
    ****************************************************************************/
    status = fftff.initialize();
    if(status != AMP_SUCCESS)
    {
        std::cout << "fft initialize failed.\n" << std::endl;
        return AMP_FAILURE;
    }

    /****************************************************************************
    * Parse command line options                                                *
    ****************************************************************************/
    if(fftff.parseCommandLine(argc, argv) != AMP_SUCCESS)
        return AMP_FAILURE;

    /****************************************************************************
    * Print all devices                                                         *
    ****************************************************************************/
    fftff.printDeviceList();

    /****************************************************************************
    * Set default accelerator                                                   *
    ****************************************************************************/
    if(fftff.setDefaultAccelerator() != AMP_SUCCESS)
        return AMP_FAILURE;

    /****************************************************************************
    * output some useful information                                            *
    ****************************************************************************/
    if(fftff.outputSummary()!= AMP_SUCCESS)
		return AMP_FAILURE;

    /****************************************************************************
    * Initialize the input data                                                 *
    ****************************************************************************/
    status = fftff.setup();
    if(status != AMP_SUCCESS)
    {
        std::cout << "FFT setup failed.\n" << std::endl;
        return AMP_FAILURE;
    }

    /****************************************************************************
    * Execute FFT, including the method array, array_view                       *
    ****************************************************************************/
    status = fftff.run();
    if(status != AMP_SUCCESS)
    {
        std::cout << "FFT run failed.\n" << std::endl;
        return AMP_FAILURE;
    }

    /****************************************************************************
    * Verify the results                                                        *
    ****************************************************************************/
    status = fftff.verifyResults();
    if(status != AMP_SUCCESS)
    {
        std::cout << "FFT verify Results failed.\n" << std::endl;
        return AMP_FAILURE;
    }
    
    /****************************************************************************
    * print some timer information                                              *
    ****************************************************************************/
    fftff.printStats();

    /****************************************************************************
    * clean up all vectors                                                      *
    ****************************************************************************/
    fftff.cleanUp();

    /****************************************************************************
    * Create an object of FFT object(double,false)                              *
    ****************************************************************************/
    FFT<double,false,double2> fftdf("C++AMP FFT double false");

    if(accelerator().supports_double_precision != 0)
    {
        /************************************************************************
        * Initialize the options of the sample                                  *
        ************************************************************************/
        status = fftdf.initialize();
        if(status != AMP_SUCCESS)
        {
            std::cout << "FFT initialize failed.\n" << std::endl;
            return AMP_FAILURE;
        }

        /************************************************************************
        * Parse command line options                                            *
        ************************************************************************/
        if(fftdf.parseCommandLine(argc, argv) != AMP_SUCCESS)
            return AMP_FAILURE;

        /************************************************************************
        * Initialize the input data                                             *
        ************************************************************************/
        status = fftdf.setup();
        if(status != AMP_SUCCESS)
        {
            std::cout << "FFT setup failed.\n" << std::endl;
            return AMP_FAILURE;
        }

        /************************************************************************
        * Execute FFT, including the method array, array_view                   *
        ************************************************************************/
        status = fftdf.run();
        if(status != AMP_SUCCESS)
        {
            std::cout << "FFT run failed.\n" << std::endl;
            return AMP_FAILURE;
        }

        /************************************************************************
        * Verify the results                                                    *
        ************************************************************************/
        status = fftdf.verifyResults();
        if(status != AMP_SUCCESS)
        {
            std::cout << "FFT verify Results failed.\n" << std::endl;
            return AMP_FAILURE;
        }

        /************************************************************************
        * print some timer information                                          *
        ************************************************************************/
        fftdf.printStats();

        /************************************************************************
        * clean up all vectors                                                  *
        ************************************************************************/
        fftdf.cleanUp();
    }
    else
    {
        /************************************************************************
        * print prompt message                                                  *
        ************************************************************************/
        std::cout << "\nDP FFT(double precision) skipped because the selected accelerator ";
        std::cout << "doesn't support double precision.\n" << std::endl;
    }

    /****************************************************************************
    * Create an object of FFT object(float,true)                                *
    ****************************************************************************/
    FFT<float,true,float2> fftft("C++AMP FFT float true");

    /****************************************************************************
    * Initialize the options of the sample                                      *
    ****************************************************************************/
    status = fftft.initialize();
    if(status != AMP_SUCCESS)
    {
        std::cout << "FFT initialize failed.\n" << std::endl;
        return AMP_FAILURE;
    }

    /****************************************************************************
    * Parse command line options                                                *
    ****************************************************************************/
    if(fftft.parseCommandLine(argc, argv) != AMP_SUCCESS)
        return AMP_FAILURE;

    /****************************************************************************
    * Initialize the input data                                                 *
    ****************************************************************************/
    status = fftft.setup();
    if(status != AMP_SUCCESS)
    {
        std::cout << "FFT setup failed.\n" << std::endl;
        return AMP_FAILURE;
    }

    /****************************************************************************
    * Execute FFT, including the method array, array_view                       *
    ****************************************************************************/
    status = fftft.run();
    if(status != AMP_SUCCESS)
    {
        std::cout << "FFT run failed.\n" << std::endl;
        return AMP_FAILURE;
    }

    /****************************************************************************
    * Verify the results                                                        *
    ****************************************************************************/
    status = fftft.verifyResults();
    if(status != AMP_SUCCESS)
    {
        std::cout << "FFT verify Results failed.\n" << std::endl;
        return AMP_FAILURE;
    }

    /****************************************************************************
    * print some timer information                                              *
    ****************************************************************************/
    fftft.printStats();

    /****************************************************************************
    * clean up all vectors                                                      *
    ****************************************************************************/
    fftft.cleanUp();

    /****************************************************************************
    * Create an object of FFT object(double,true)                               *
    ****************************************************************************/
    FFT<double,true,double2> fftdt("C++AMP FFT double true");

    if(accelerator().supports_double_precision != 0)
    {
        /************************************************************************
        * Initialize the options of the sample                                  *
        ************************************************************************/
        status = fftdt.initialize();
        if(status != AMP_SUCCESS)
        {
            std::cout << "FFT initialize failed.\n" << std::endl;
            return AMP_FAILURE;
        }

        /************************************************************************
        * Parse command line options                                            *
        ************************************************************************/
        if(fftdt.parseCommandLine(argc, argv) != AMP_SUCCESS)
            return AMP_FAILURE;

        /************************************************************************
        * Initialize the input data                                             *
        ************************************************************************/
        status = fftdt.setup();
        if(status != AMP_SUCCESS)
        {
            std::cout << "FFT setup failed.\n" << std::endl;
            return AMP_FAILURE;
        }

        /************************************************************************
        * Execute FFT, including the method array, array_view                   *
        ************************************************************************/
        status = fftdt.run();
        if(status != AMP_SUCCESS)
        {
            std::cout << "FFT run failed.\n" << std::endl;
            return AMP_FAILURE;
        }

        /************************************************************************
        * Verify the results                                                    *
        ************************************************************************/
        status = fftdt.verifyResults();
        if(status != AMP_SUCCESS)
        {
            std::cout<< "FFT verify Results failed.\n" << std::endl;
            return AMP_FAILURE;
        }

        /************************************************************************
        * print some timer information                                          *
        ************************************************************************/
        fftdt.printStats();

        /************************************************************************
        * clean up all vectors                                                  *
        ************************************************************************/
        fftdt.cleanUp();
    }
    else
    {
        /************************************************************************
        * print prompt message                                                  *
        ************************************************************************/
        std::cout << "\nDP IFFT(double precision) skipped because the selected accelerator ";
        std::cout << "doesn't support double precision.\n" << std::endl;
    }

    return AMP_SUCCESS;
}

