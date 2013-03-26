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

/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include "GEMM.hpp"

/******************************************************************************
 *Global function::toString()                                                 *
 *****************************************************************************/
 template<typename T>
 std::string toString(T t, std::ios_base &(*r)(std::ios_base&))
 {
      std::ostringstream output;
      output << r << t;
      return output.str();
 }

/******************************************************************************
* Implementation of GEMM::outputData()                                        *
******************************************************************************/
template <typename FPTYPE, bool TRANSA>
void GEMM<FPTYPE, TRANSA>::outputData(double workDone,
                                      const std::string &workRateUnit)
{
    if(iter >= 1) 
    {
        // Print time information while using array
        if(timing && (!arrayview))
        {
            double meanMs = (totalKernelTime * 1000.0) / iter;
            double workRate = workDone / (meanMs / 1000.0);
            int workRatePrecision = (workRate >= 100.0 ? 0 : 1);

            std::cout << "(Total time(sec): " << totalTime << ")\n";

            std::cout << "Time Information" << std::endl;
            std::string strArray[4] = {"Data Transfer to Accelerator(sec)", "Mean Execution time(sec)", workRateUnit, "Data Transfer to Host(sec)"};
            std::string stats[4];
            stats[0] = toString<double>(cpToDeviceTime, std::dec);
            stats[1] = toString<double>(meanMs / 1000.0, std::dec);
            stats[2] = toString<double>(workRate, std::dec);
            stats[3] = toString<double>(cpToHostTime, std::dec);

            this->AmpSample::printStats(strArray, stats, 4);
        }
        else if(timing && arrayview)
        {
            // Print time information while using array_view
            std::cout << "(Total time(sec): " << totalTime << ") " << std::endl;
        }
    }
    else
    {
       std::cout << " (insufficient runs for timing)\n";
    }

}

/******************************************************************************
* Implementation of GEMM::outputSummary()                                     *
******************************************************************************/
template <typename FPTYPE, bool TRANSA>
void GEMM<FPTYPE, TRANSA>::outputSummary()
{
    std::cout << "Sampling: " << iter;
    std::cout << " (" << (iter) << " sampled) benchmark runs.\n";
    if(!arrayview)
    {
        std::cout << "Using Array." << std::endl;
    }
    else
    {
        std::cout << "Using Array_view." << std::endl;
    }
    std::cout << "\n";
    std::cout.flush();
}

/******************************************************************************
* Implementation of GEMM::GEMM_Amp()                                          *
******************************************************************************/
template <typename FPTYPE, bool TRANSA>
void GEMM<FPTYPE, TRANSA>::GEMM_Amp(array<FPTYPE, 2> &devMatrixA,
                                    array<FPTYPE, 2> &devMatrixB,
                                    array<FPTYPE, 2> &devMatrixC,
                                    array<FPTYPE, 2> &devMatrixOut,
                                    FPTYPE alpha,
                                    FPTYPE beta,
                                    bool transA,
                                    int matrixEdge)
{
    if(!transA)
    {
        // Compute GEMM NN for arrays A, B, C and parameters alpha, beta.
        // N/4xN/4 grid, 16x4 tiles each responsible for 64x16 output.
        parallel_for_each(tiled_extent<4, 16>(extent<2>(matrixEdge / 4, matrixEdge / 4)),
            [=, &devMatrixA, &devMatrixB, &devMatrixC, &devMatrixOut](tiled_index<4, 16> idx) restrict(amp) 
        {
            // 16x16 block of array A staged inside a loop (below) which strides columns.
            tile_static FPTYPE blockA[16][16];

            // Zero partial sums for our thread's 1x16 output set (accumulated below).
            FPTYPE c[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

            // Pre-compute a tile/thread X offset into matrices B and C.
            // This corresponds to one unique column of both matrices per thread.
            const int     bcXOffset = (idx.tile[1] * 64) + (idx.local[0] * 16 + idx.local[1]);
            const FPTYPE *rowA      = &devMatrixA(idx.tile[0] * 16 + idx.local[0], idx.local[1]);
            const FPTYPE *columnB   = &devMatrixB(0, bcXOffset);

            for(int k = 0; k < matrixEdge; k += 16) 
            {
                // Copy the next 16x16 block of array A into tile memory.
                blockA[idx.local[0] +  0][idx.local[1]] = rowA[ 0 * matrixEdge];
                blockA[idx.local[0] +  4][idx.local[1]] = rowA[ 4 * matrixEdge];
                blockA[idx.local[0] +  8][idx.local[1]] = rowA[ 8 * matrixEdge];
                blockA[idx.local[0] + 12][idx.local[1]] = rowA[12 * matrixEdge];
                rowA += 16;
                idx.barrier.wait_with_tile_static_memory_fence();

                // Read the next 4 values for this tile from our column of B.
                FPTYPE nb0, nb1, nb2, nb3;
                nb0 = columnB[0 * matrixEdge];
                nb1 = columnB[1 * matrixEdge];
                nb2 = columnB[2 * matrixEdge];
                nb3 = columnB[3 * matrixEdge];

                // Macro to advance the partial sum set by one step.
                #define SAXPY(A, B, I)   \
                    c[ 0] += A[ 0][I] * B; \
                    c[ 1] += A[ 1][I] * B; \
                    c[ 2] += A[ 2][I] * B; \
                    c[ 3] += A[ 3][I] * B; \
                    c[ 4] += A[ 4][I] * B; \
                    c[ 5] += A[ 5][I] * B; \
                    c[ 6] += A[ 6][I] * B; \
                    c[ 7] += A[ 7][I] * B; \
                    c[ 8] += A[ 8][I] * B; \
                    c[ 9] += A[ 9][I] * B; \
                    c[10] += A[10][I] * B; \
                    c[11] += A[11][I] * B; \
                    c[12] += A[12][I] * B; \
                    c[13] += A[13][I] * B; \
                    c[14] += A[14][I] * B; \
                    c[15] += A[15][I] * B;

                // Advance partial sums for this block of A, while interleaving reads from B.
                columnB += 4 * matrixEdge;
                SAXPY(blockA, nb0,  0); nb0 = columnB[0 * matrixEdge];
                SAXPY(blockA, nb1,  1); nb1 = columnB[1 * matrixEdge];
                SAXPY(blockA, nb2,  2); nb2 = columnB[2 * matrixEdge];
                SAXPY(blockA, nb3,  3); nb3 = columnB[3 * matrixEdge];

                columnB += 4 * matrixEdge;
                SAXPY(blockA, nb0,  4); nb0 = columnB[0 * matrixEdge];
                SAXPY(blockA, nb1,  5); nb1 = columnB[1 * matrixEdge];
                SAXPY(blockA, nb2,  6); nb2 = columnB[2 * matrixEdge];
                SAXPY(blockA, nb3,  7); nb3 = columnB[3 * matrixEdge];

                columnB += 4 * matrixEdge;
                SAXPY(blockA, nb0,  8); nb0 = columnB[0 * matrixEdge];
                SAXPY(blockA, nb1,  9); nb1 = columnB[1 * matrixEdge];
                SAXPY(blockA, nb2, 10); nb2 = columnB[2 * matrixEdge];
                SAXPY(blockA, nb3, 11); nb3 = columnB[3 * matrixEdge];

                columnB += 4 * matrixEdge;
                SAXPY(blockA, nb0, 12);
                SAXPY(blockA, nb1, 13);
                SAXPY(blockA, nb2, 14);
                SAXPY(blockA, nb3, 15);

                #undef SAXPY

                // Ensure that reads from local memory are completed.
                idx.barrier.wait_with_tile_static_memory_fence();
            }

            // Compute and write this thread's 1x16 block of C.
            FPTYPE *columnC = &devMatrixC(idx.tile[0] * 16, bcXOffset);
            FPTYPE *columnOut = &devMatrixOut(idx.tile[0] * 16, bcXOffset);
            *columnOut = alpha * c[ 0] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[ 1] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[ 2] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[ 3] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[ 4] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[ 5] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[ 6] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[ 7] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[ 8] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[ 9] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[10] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[11] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[12] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[13] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[14] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[15] + beta * *columnC;
        }
        );
    }
    else 
    {
        // Compute GEMM TN for arrays A, B, C and parameters alpha, beta.
        // N/4xN/4 grid, 16x4 tiles each responsible for 64x16 output.
        parallel_for_each(tiled_extent<4, 16>(extent<2>(matrixEdge / 4, matrixEdge / 4)),
            [=,&devMatrixA, &devMatrixB, &devMatrixC,&devMatrixOut](tiled_index<4, 16> idx) restrict(amp) 
        {
            // 16x4 block of array A staged inside a loop (below) which strides rows.
            tile_static FPTYPE blockA[4][16];

            // Pre-compute tile/thread offsets into matrices A, B and C.
            const int     bcXOffset = (idx.tile[1] * 64) + (idx.local[0] * 16 + idx.local[1]);
            const FPTYPE *columnA   = &devMatrixA(idx.local[0], idx.tile[0] * 16 + idx.local[1]);
            const FPTYPE *columnB   = &devMatrixB(0, bcXOffset);

            // Read our thread's element of the next 16x4 block of array A.
            FPTYPE nextA = *columnA;
            columnA += 4 * matrixEdge;

            // Read the first 4 values for this tile from our column of B.
            FPTYPE nb0, nb1, nb2, nb3;
            nb0 = columnB[0 * matrixEdge];
            nb1 = columnB[1 * matrixEdge];
            nb2 = columnB[2 * matrixEdge];
            nb3 = columnB[3 * matrixEdge];
            columnB += 4 * matrixEdge;

            // Zero partial sums for our thread's 1x16 output set (accumulated below).
            FPTYPE c[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

            // Macro to advance the partial sum set by one step.
            #define SAXPY(A, B, I)   \
                c[ 0] += A[I][ 0] * B; \
                c[ 1] += A[I][ 1] * B; \
                c[ 2] += A[I][ 2] * B; \
                c[ 3] += A[I][ 3] * B; \
                c[ 4] += A[I][ 4] * B; \
                c[ 5] += A[I][ 5] * B; \
                c[ 6] += A[I][ 6] * B; \
                c[ 7] += A[I][ 7] * B; \
                c[ 8] += A[I][ 8] * B; \
                c[ 9] += A[I][ 9] * B; \
                c[10] += A[I][10] * B; \
                c[11] += A[I][11] * B; \
                c[12] += A[I][12] * B; \
                c[13] += A[I][13] * B; \
                c[14] += A[I][14] * B; \
                c[15] += A[I][15] * B;

            for(int k = 4; k < matrixEdge; k += 4) 
            {
                // Preserve our set of B elements.
                FPTYPE pb0, pb1, pb2, pb3;
                pb0 = nb0;
                pb1 = nb1;
                pb2 = nb2;
                pb3 = nb3;

                // Copy the next 16x4 block of array A into tile memory.
                blockA[idx.local[0]][idx.local[1]] = nextA;
                idx.barrier.wait_with_tile_static_memory_fence();

                // Read the next 4 values for this tile from our column of B.
                nb0 = columnB[0 * matrixEdge];
                nb1 = columnB[1 * matrixEdge];
                nb2 = columnB[2 * matrixEdge];
                nb3 = columnB[3 * matrixEdge];
                columnB += 4 * matrixEdge;

                // Read our element of the next 16x4 block of array A.
                nextA = *columnA;
                columnA += 4 * matrixEdge;

                // Advance partial sums for this block of A.
                SAXPY(blockA, pb0, 0);
                SAXPY(blockA, pb1, 1);
                SAXPY(blockA, pb2, 2);
                SAXPY(blockA, pb3, 3);

                // Ensure that reads from local memory are completed.
                idx.barrier.wait_with_tile_static_memory_fence();
            }

            // Copy the next 16x4 block of array A into tile memory.
            blockA[idx.local[0]][idx.local[1]] = nextA;
            idx.barrier.wait_with_tile_static_memory_fence();

            // Finalize partial sums for this block of A.
            SAXPY(blockA, nb0, 0);
            SAXPY(blockA, nb1, 1);
            SAXPY(blockA, nb2, 2);
            SAXPY(blockA, nb3, 3);

            #undef SAXPY

            // Compute and write this thread's 1x16 block of C.
            FPTYPE *columnC = &devMatrixC(idx.tile[0] * 16, bcXOffset);
            FPTYPE *columnOut = &devMatrixOut(idx.tile[0] * 16, bcXOffset);
            for(int i = 0; i < 16; ++i) 
            {
                *columnOut = alpha * c[i] + beta * *columnC;
                columnC += matrixEdge;
                columnOut += matrixEdge;
            }
        }
        );
    }
}

/******************************************************************************
* Implementation of GEMM::GEMM_Amp_View()                                     *
******************************************************************************/
template <typename FPTYPE, bool TRANSA>
void GEMM<FPTYPE, TRANSA>::GEMM_Amp_View(array_view<FPTYPE, 2> &devMatrixA,
                                         array_view<FPTYPE, 2> &devMatrixB,
                                         array_view<FPTYPE, 2> &devMatrixC,
                                         array_view<FPTYPE, 2> &devOutput,
                                         FPTYPE alpha,
                                         FPTYPE beta,
                                         bool transA,
                                         int matrixEdge)
{
    if(!transA) 
    {
        // Compute GEMM NN for arrays A, B, C and parameters alpha, beta.
        // N/4xN/4 grid, 16x4 tiles each responsible for 64x16 output.
        parallel_for_each(tiled_extent<4, 16>(extent<2>(matrixEdge / 4, matrixEdge / 4)),
            [=](tiled_index<4, 16> idx) restrict(amp) 
        {
            // 16x16 block of array A staged inside a loop (below) which strides columns.
            tile_static FPTYPE blockA[16][16];

            // Zero partial sums for our thread's 1x16 output set (accumulated below).
            FPTYPE c[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

            // Pre-compute a tile/thread X offset into matrices B and C.
            // This corresponds to one unique column of both matrices per thread.
            const int     bcXOffset = (idx.tile[1] * 64) + (idx.local[0] * 16 + idx.local[1]);
            const FPTYPE *rowA      = &devMatrixA(idx.tile[0] * 16 + idx.local[0], idx.local[1]);
            const FPTYPE *columnB   = &devMatrixB(0, bcXOffset);

            for(int k = 0; k < matrixEdge; k += 16) 
            {
                // Copy the next 16x16 block of array A into tile memory.
                blockA[idx.local[0] +  0][idx.local[1]] = rowA[ 0 * matrixEdge];
                blockA[idx.local[0] +  4][idx.local[1]] = rowA[ 4 * matrixEdge];
                blockA[idx.local[0] +  8][idx.local[1]] = rowA[ 8 * matrixEdge];
                blockA[idx.local[0] + 12][idx.local[1]] = rowA[12 * matrixEdge];
                rowA += 16;
                idx.barrier.wait_with_tile_static_memory_fence();

                // Read the next 4 values for this tile from our column of B.
                FPTYPE nb0, nb1, nb2, nb3;
                nb0 = columnB[0 * matrixEdge];
                nb1 = columnB[1 * matrixEdge];
                nb2 = columnB[2 * matrixEdge];
                nb3 = columnB[3 * matrixEdge];

                // Macro to advance the partial sum set by one step.
                #define SAXPY(A, B, I)   \
                    c[ 0] += A[ 0][I] * B; \
                    c[ 1] += A[ 1][I] * B; \
                    c[ 2] += A[ 2][I] * B; \
                    c[ 3] += A[ 3][I] * B; \
                    c[ 4] += A[ 4][I] * B; \
                    c[ 5] += A[ 5][I] * B; \
                    c[ 6] += A[ 6][I] * B; \
                    c[ 7] += A[ 7][I] * B; \
                    c[ 8] += A[ 8][I] * B; \
                    c[ 9] += A[ 9][I] * B; \
                    c[10] += A[10][I] * B; \
                    c[11] += A[11][I] * B; \
                    c[12] += A[12][I] * B; \
                    c[13] += A[13][I] * B; \
                    c[14] += A[14][I] * B; \
                    c[15] += A[15][I] * B;

                // Advance partial sums for this block of A, while interleaving reads from B.
                columnB += 4 * matrixEdge;
                SAXPY(blockA, nb0,  0); nb0 = columnB[0 * matrixEdge];
                SAXPY(blockA, nb1,  1); nb1 = columnB[1 * matrixEdge];
                SAXPY(blockA, nb2,  2); nb2 = columnB[2 * matrixEdge];
                SAXPY(blockA, nb3,  3); nb3 = columnB[3 * matrixEdge];

                columnB += 4 * matrixEdge;
                SAXPY(blockA, nb0,  4); nb0 = columnB[0 * matrixEdge];
                SAXPY(blockA, nb1,  5); nb1 = columnB[1 * matrixEdge];
                SAXPY(blockA, nb2,  6); nb2 = columnB[2 * matrixEdge];
                SAXPY(blockA, nb3,  7); nb3 = columnB[3 * matrixEdge];

                columnB += 4 * matrixEdge;
                SAXPY(blockA, nb0,  8); nb0 = columnB[0 * matrixEdge];
                SAXPY(blockA, nb1,  9); nb1 = columnB[1 * matrixEdge];
                SAXPY(blockA, nb2, 10); nb2 = columnB[2 * matrixEdge];
                SAXPY(blockA, nb3, 11); nb3 = columnB[3 * matrixEdge];

                columnB += 4 * matrixEdge;
                SAXPY(blockA, nb0, 12);
                SAXPY(blockA, nb1, 13);
                SAXPY(blockA, nb2, 14);
                SAXPY(blockA, nb3, 15);

                #undef SAXPY

                // Ensure that reads from local memory are completed.
                idx.barrier.wait_with_tile_static_memory_fence();
            }

            // Compute and write this thread's 1x16 block of C.
            FPTYPE *columnC = &devMatrixC(idx.tile[0] * 16, bcXOffset);
            FPTYPE *columnOut = &devOutput(idx.tile[0] * 16, bcXOffset);
            *columnOut = alpha * c[ 0] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[ 1] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[ 2] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[ 3] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[ 4] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[ 5] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[ 6] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[ 7] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[ 8] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[ 9] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[10] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[11] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[12] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[13] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[14] + beta * *columnC; columnC += matrixEdge; columnOut += matrixEdge;
            *columnOut = alpha * c[15] + beta * *columnC;

        }
        );
    }
    else 
    {
        // Compute GEMM TN for arrays A, B, C and parameters alpha, beta.
        // N/4xN/4 grid, 16x4 tiles each responsible for 64x16 output.
        parallel_for_each(tiled_extent<4, 16>(extent<2>(matrixEdge / 4, matrixEdge / 4)),
            [=](tiled_index<4, 16> idx) restrict(amp) 
        {
            // 16x4 block of array A staged inside a loop (below) which strides rows.
            tile_static FPTYPE blockA[4][16];

            // Pre-compute tile/thread offsets into matrices A, B and C.
            const int     bcXOffset = (idx.tile[1] * 64) + (idx.local[0] * 16 + idx.local[1]);
            const FPTYPE *columnA   = &devMatrixA(idx.local[0], idx.tile[0] * 16 + idx.local[1]);
            const FPTYPE *columnB   = &devMatrixB(0, bcXOffset);

            // Read our thread's element of the next 16x4 block of array A.
            FPTYPE nextA = *columnA;
            columnA += 4 * matrixEdge;

            // Read the first 4 values for this tile from our column of B.
            FPTYPE nb0, nb1, nb2, nb3;
            nb0 = columnB[0 * matrixEdge];
            nb1 = columnB[1 * matrixEdge];
            nb2 = columnB[2 * matrixEdge];
            nb3 = columnB[3 * matrixEdge];
            columnB += 4 * matrixEdge;

            // Zero partial sums for our thread's 1x16 output set (accumulated below).
            FPTYPE c[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

            // Macro to advance the partial sum set by one step.
            #define SAXPY(A, B, I)   \
                c[ 0] += A[I][ 0] * B; \
                c[ 1] += A[I][ 1] * B; \
                c[ 2] += A[I][ 2] * B; \
                c[ 3] += A[I][ 3] * B; \
                c[ 4] += A[I][ 4] * B; \
                c[ 5] += A[I][ 5] * B; \
                c[ 6] += A[I][ 6] * B; \
                c[ 7] += A[I][ 7] * B; \
                c[ 8] += A[I][ 8] * B; \
                c[ 9] += A[I][ 9] * B; \
                c[10] += A[I][10] * B; \
                c[11] += A[I][11] * B; \
                c[12] += A[I][12] * B; \
                c[13] += A[I][13] * B; \
                c[14] += A[I][14] * B; \
                c[15] += A[I][15] * B;

            for(int k = 4; k < matrixEdge; k += 4) 
            {
                // Preserve our set of B elements.
                FPTYPE pb0, pb1, pb2, pb3;
                pb0 = nb0;
                pb1 = nb1;
                pb2 = nb2;
                pb3 = nb3;

                // Copy the next 16x4 block of array A into tile memory.
                blockA[idx.local[0]][idx.local[1]] = nextA;
                idx.barrier.wait_with_tile_static_memory_fence();

                // Read the next 4 values for this tile from our column of B.
                nb0 = columnB[0 * matrixEdge];
                nb1 = columnB[1 * matrixEdge];
                nb2 = columnB[2 * matrixEdge];
                nb3 = columnB[3 * matrixEdge];
                columnB += 4 * matrixEdge;

                // Read our element of the next 16x4 block of array A.
                nextA = *columnA;
                columnA += 4 * matrixEdge;

                // Advance partial sums for this block of A.
                SAXPY(blockA, pb0, 0);
                SAXPY(blockA, pb1, 1);
                SAXPY(blockA, pb2, 2);
                SAXPY(blockA, pb3, 3);

                // Ensure that reads from local memory are completed.
                idx.barrier.wait_with_tile_static_memory_fence();
            }

            // Copy the next 16x4 block of array A into tile memory.
            blockA[idx.local[0]][idx.local[1]] = nextA;
            idx.barrier.wait_with_tile_static_memory_fence();

            // Finalize partial sums for this block of A.
            SAXPY(blockA, nb0, 0);
            SAXPY(blockA, nb1, 1);
            SAXPY(blockA, nb2, 2);
            SAXPY(blockA, nb3, 3);

            #undef SAXPY

            // Compute and write this thread's 1x16 block of C.
            FPTYPE *columnC = &devMatrixC(idx.tile[0] * 16, bcXOffset);
            FPTYPE *columnOut = &devOutput(idx.tile[0] * 16, bcXOffset);

            for(int i = 0; i < 16; ++i) 
            {
                *columnOut = alpha * c[i] + beta * *columnC;
                columnC += matrixEdge;
                columnOut += matrixEdge;
            }
        }
        );
    }
}

/******************************************************************************
* Implementation of GEMM::initialize()                                        *
******************************************************************************/
template <typename FPTYPE, bool TRANSA>
int GEMM<FPTYPE, TRANSA>::initialize()
{
    //Call base class Initialize to get default configuration 
    if(this->AmpSample::initialize())
        return AMP_FAILURE;

    ampsdk::Option* array_length = new ampsdk::Option;
    CHECK_ALLOCATION(array_length,"Memory Allocation error.(array_length)");

    array_length->_sVersion = "s";
    array_length->_lVersion = "size";
    array_length->_description = "Edge length of square matrices A, B and C";
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
    CHECK_ALLOCATION(arrayView,"Memory Allocation error.(zeroCopy)");

    arrayView->_sVersion = "V";
    arrayView->_lVersion = "arrayview";
    arrayView->_description = "use array_view instead of array";
    arrayView->_type = ampsdk::CA_NO_ARGUMENT;
    arrayView->_value = &arrayview;

    sampleArgs->AddOption(arrayView);
    delete arrayView;

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of GEMM::setup()                                             *
******************************************************************************/
template <typename FPTYPE, bool TRANSA>
int GEMM<FPTYPE, TRANSA>::setup()
{
    // Constrain problem size to multiples of 64 (tile size).
    if(length % 64 != 0 || length < 64) 
    {
        std::cerr << "Error: length (" << length << ") must be a multiple of 64 or bigger than 0.\n\n";
        return AMP_FAILURE;
    }

    if(iter <= 0)
    {
        std::cerr << "Error: iter (" << iter << ") should be bigger than 0. \n\n";
        return AMP_FAILURE;
    }

    // Randomize host-side matrices for validation.
    matrixEdge  = length;
    matrixElems = matrixEdge * matrixEdge;

    matrixA = std::vector<FPTYPE>(matrixElems);
    matrixB = std::vector<FPTYPE>(matrixElems);
    matrixC = std::vector<FPTYPE>(matrixElems);
    matrixOut = std::vector<FPTYPE>(matrixElems, 0);

    std::uniform_real_distribution<FPTYPE> rngDist(-1.0, 1.0);
    std::mt19937 rng;

    for(int i = 0; i < matrixElems; ++i) 
    {
        matrixA[i] = rngDist(rng);
        matrixB[i] = rngDist(rng);
        matrixC[i] = rngDist(rng);
    }

    // Randomize GEMM alpha and beta coefficients.
    alpha = rngDist(rng);
    beta  = rngDist(rng);

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of GEMM::run()                                               *
******************************************************************************/
template <typename FPTYPE, bool TRANSA>
int GEMM<FPTYPE, TRANSA>::run()
{
    std::stringstream modeStr;
    modeStr << (sizeof(FPTYPE) == 4 ? "S" : "D") << "GEMM ";
    modeStr << (TRANSA ? "TN" : "NN") << ".";
    std::cout << "Run " << modeStr.str() << std::fixed << std::setprecision(3) << "\n";

    // print out the MatrixA, MatrixB and MatrixC
    if(!quiet)
    {
        sampleCommon->printArray<FPTYPE>("MatrixA (the first 128 elements)",matrixA, 128, 1);

        sampleCommon->printArray<FPTYPE>("MatrixB (the first 128 elements)",matrixB, 128, 1);

        sampleCommon->printArray<FPTYPE>("MatrixC (the first 128 elements)",matrixC, 128, 1);
    }

    if(!arrayview)
    {
       if(runArray() != AMP_SUCCESS)
       {
           return AMP_FAILURE;
       }
    }
    else
    {
       if(runArrayView() != AMP_SUCCESS)
       {
           return AMP_FAILURE;
       }
    }

    // print the output, the data size is too big to print all out
    if(!quiet)
    {
        sampleCommon->printArray<FPTYPE>("Output (the first 128 elements)",matrixOut, 128, 1);
    }

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of GEMM::runArray()                                          *
******************************************************************************/
template <typename FPTYPE, bool TRANSA>
int GEMM<FPTYPE, TRANSA>::runArray()
{
    int status = 0;

    //Copy matrices A and B onto the device, allocate C.
    array<FPTYPE, 2> devMatrixA(matrixEdge, matrixEdge);
    array<FPTYPE, 2> devMatrixB(matrixEdge, matrixEdge);
    array<FPTYPE, 2> devMatrixC(matrixEdge, matrixEdge);
    array<FPTYPE, 2> devMatrixOut(matrixEdge, matrixEdge);

    // Extra array for warm up
    array<FPTYPE, 2> devWarmUp(matrixEdge, matrixEdge);

    // calculate data transfer time (host to device)
    int cpToDeviceTimer = sampleCommon->createTimer();
    status = sampleCommon->resetTimer(cpToDeviceTimer);
    status = sampleCommon->startTimer(cpToDeviceTimer);

    copy(matrixA.begin(), devMatrixA);
    copy(matrixB.begin(), devMatrixB);
    copy(matrixC.begin(), devMatrixC);

    status = sampleCommon->stopTimer(cpToDeviceTimer);
    if(status != AMP_SUCCESS)
    {
        return AMP_FAILURE;
    }
    cpToDeviceTime = sampleCommon->readTimer(cpToDeviceTimer);

    // Warm up
    GEMM_Amp(devMatrixA, devMatrixB, devMatrixC, devWarmUp,alpha, beta, TRANSA, matrixEdge);

    accelerator_view &accView = accelerator().default_view;
    accView.flush();
    accView.wait();

    // compute kernel execution time.
    int timer = sampleCommon->createTimer();
    status = sampleCommon->resetTimer(timer);
    status = sampleCommon->startTimer(timer);

    // Repeat the benchmark 
    for(int run = 0; run < iter; ++run) 
    { 
        GEMM_Amp(devMatrixA, devMatrixB, devMatrixC, 
            devMatrixOut,alpha, beta, TRANSA, matrixEdge);
        accView.flush();
    }
    accView.wait();

    status = sampleCommon->stopTimer(timer);
    if(status != AMP_SUCCESS)
    {
        return AMP_FAILURE;
    }
    totalKernelTime = sampleCommon->readTimer(timer);

    //copy the output data to matrixOut from device
    int cpToHostTimer = sampleCommon->createTimer();
    status = sampleCommon->resetTimer(cpToHostTimer);
    status = sampleCommon->startTimer(cpToHostTimer);

    copy(devMatrixOut, matrixOut.begin());

    status = sampleCommon->stopTimer(cpToHostTimer);
    if(status != AMP_SUCCESS)
    {
        return AMP_FAILURE;
    }
    cpToHostTime = sampleCommon->readTimer(cpToHostTimer);

    totalTime = cpToDeviceTime + totalKernelTime + cpToHostTime;

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of GEMM::runArrayView()                                      *
******************************************************************************/
template <typename FPTYPE, bool TRANSA>
int GEMM<FPTYPE, TRANSA>::runArrayView()
{
    int status = 0;

    // Copy matrices A and B onto the device, allocate C.
    array_view<FPTYPE, 2> devMatrixA(matrixEdge, matrixEdge, matrixA);
    array_view<FPTYPE, 2> devMatrixB(matrixEdge, matrixEdge, matrixB);
    array_view<FPTYPE, 2> devMatrixC(matrixEdge, matrixEdge, matrixC);
    array_view<FPTYPE, 2> devOutput(matrixEdge, matrixEdge, matrixOut);

    // Extra array view for warm up.
    std::vector<FPTYPE> warmUpA(matrixElems, 0);
    std::vector<FPTYPE> warmUpB(matrixElems, 0);
    std::vector<FPTYPE> warmUpC(matrixElems, 0);
    std::vector<FPTYPE> warmUpOut(matrixElems, 0);

    array_view<FPTYPE, 2> devWarmUpA(matrixEdge, matrixEdge, warmUpA);
    array_view<FPTYPE, 2> devWarmUpB(matrixEdge, matrixEdge, warmUpB);
    array_view<FPTYPE, 2> devWarmUpC(matrixEdge, matrixEdge, warmUpC);
    array_view<FPTYPE, 2> devWarmUpOut(matrixEdge, matrixEdge, warmUpOut);

    // accelerator view for synchronize
    accelerator_view &accView = accelerator().default_view;

    // warm up
    GEMM_Amp_View(devWarmUpA, devWarmUpB, devWarmUpC, 
                    devWarmUpOut, alpha, beta, TRANSA, matrixEdge);
    devWarmUpOut.synchronize();

    // compute a total execution time.
    int timer = sampleCommon->createTimer();
    status = sampleCommon->resetTimer(timer);
    status = sampleCommon->startTimer(timer);

    // Repeat the benchmark
    for(int run = 0; run < iter; ++run) 
    {
        GEMM_Amp_View(devMatrixA, devMatrixB, devMatrixC, 
                    devOutput, alpha, beta, TRANSA, matrixEdge);
        accView.flush();
    }
    devOutput.synchronize();

    status = sampleCommon->stopTimer(timer);
    if(status != AMP_SUCCESS)
    {
        return AMP_FAILURE;
    }
    totalTime = sampleCommon->readTimer(timer);

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of GEMM::verifyResults()                                     *
******************************************************************************/
template <typename FPTYPE, bool TRANSA>
int GEMM<FPTYPE, TRANSA>::verifyResults()
{
    if(verify)
    {
        // Compute matrix multiply into a reference array.
        std::vector<FPTYPE> refMatrixC(matrixElems, 0);

        // Tile loops by L1 cache line size for tractable compute time.
        const int l1TileSize = 64 / sizeof(FPTYPE);
        const int tilePtrAOuterInc = (TRANSA ? 1 : matrixEdge);
        const int tilePtrAInnerInc = (TRANSA ? matrixEdge : 1);

        std::cout << " [Validating... ";
        std::cout.flush();

#pragma omp parallel for
        for(int i = 0; i < matrixEdge; i += l1TileSize) 
        {
            for(int j = 0; j < matrixEdge; j += l1TileSize) 
            {
                for(int k = 0; k < matrixEdge; k += l1TileSize) 
                {
                    FPTYPE *tilePtrA    = &   matrixA[TRANSA ? (k * matrixEdge + i) : (i * matrixEdge + k)];
                    FPTYPE *tilePtrRefC = &refMatrixC[i * matrixEdge + j];

                    for(int i2 = 0; i2 < l1TileSize; ++i2, tilePtrA += tilePtrAOuterInc, tilePtrRefC += matrixEdge) 
                    {
                        FPTYPE *tilePtrB = &matrixB[k * matrixEdge + j];

                        for(int k2 = 0; k2 < l1TileSize; ++k2, tilePtrB += matrixEdge) 
                        {
                            const FPTYPE elemA = tilePtrA[k2 * tilePtrAInnerInc];

                            for(int j2 = 0; j2 < l1TileSize; ++j2) 
                            {
                                tilePtrRefC[j2] += elemA * tilePtrB[j2];
                            }
                        }
                    }
                }
            }
        }

        // Extend matrix multiply result to GEMM.
#pragma omp parallel for
        for(int i = 0; i < matrixEdge; ++i) 
        {
            for(int j = 0; j < matrixEdge; ++j) 
            {
                FPTYPE &refC = refMatrixC[i * matrixEdge + j];
                refC = alpha * refC + beta * matrixC[i * matrixEdge + j];
            }
        }

        // Compare the reference and AMP-computed arrays.

        for(int i = 0; i < matrixEdge; ++i) 
        {
            for(int j = 0; j < matrixEdge; ++j) 
            {
                // Compare the reference and computed values.
                FPTYPE refC    = refMatrixC[i * matrixEdge + j];
                FPTYPE actualC = matrixOut[i * matrixEdge + j];

                if(fabs(refC - actualC) > 0.00001) 
                {
                    std::cerr << "failed at " << j << "x" << i << ": ";
                    std::cerr << "expected " << refC << " but have " << actualC << "]\n\n";
                    std::cerr << "Benchmark aborted due to error\n";
                    exit(1);
                }
            }
        }

        std::cout << "passed]\n";
        std::cout << std::endl;
    }
    std::cout.flush();

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of GEMM::printStats()                                        *
******************************************************************************/
template <typename FPTYPE, bool TRANSA>
void GEMM<FPTYPE, TRANSA>::printStats()
{
    // Report mean/SD/throughput to the user.
    std::stringstream modeStr;
    modeStr << (sizeof(FPTYPE) == 4 ? "S" : "D") << "GEMM ";
    modeStr << (TRANSA ? "TN" : "NN") << " ";
    modeStr << matrixEdge << "x" << matrixEdge;

    std::cout << modeStr.str() << std::fixed << std::setprecision(3) << " finished.";

    int64_t nFLOP = int64_t(2 * matrixEdge + 3) * int64_t(matrixEdge * matrixEdge);
    double nGFLOP = nFLOP / 1000000000.0;
    outputData(nGFLOP, "GFLOPS");
    std::cout << "\n";
    std::cout << std::endl;
}

/**********************************************************************************
* Execution of program begins from here                                           *
**********************************************************************************/
int main(int argc,
         char *argv[])
{
    std::cout << "************************************************" << std::endl;
    std::cout << "              GEMM " << std::endl;
    std::cout << "************************************************" << std::endl;
    std::cout << std::endl;

    int status = 0;

    /*******************************************************************************
    * Create an object of GEMM object(float,false)                                 *
    *******************************************************************************/
    GEMM<float,false> gemmff("C++AMP GEMM float false");

    /*******************************************************************************
    * Initialize the options of the sample                                         *
    *******************************************************************************/
    status = gemmff.initialize();
    if(status != AMP_SUCCESS)
    {
        std::cout << "GEMM initialize failed." << std::endl;
        return AMP_FAILURE;
    }

    /*******************************************************************************
    * Parse command line options                                                   *
    *******************************************************************************/
    if(gemmff.parseCommandLine(argc, argv) != AMP_SUCCESS)
        return AMP_FAILURE;

    /*******************************************************************************
    * Print all devices                                                            *
    *******************************************************************************/
    gemmff.printDeviceList();

    /*******************************************************************************
    * Set default accelerator                                                      *
    *******************************************************************************/
    if(gemmff.setDefaultAccelerator() != AMP_SUCCESS)
        return AMP_FAILURE;

    /*******************************************************************************
    * output some useful information                                               *
    *******************************************************************************/
    gemmff.outputSummary();

    /*******************************************************************************
    * Initialize the random array of input vectors                                 *
    *******************************************************************************/
    status = gemmff.setup();
    if(status != AMP_SUCCESS)
    {
        std::cout << "GEMM setup failed." << std::endl;
        return AMP_FAILURE;
    }

    /*******************************************************************************
    * Execute GEMM, including the method array, array_view                         *
    *******************************************************************************/
    status = gemmff.run();
    if(status != AMP_SUCCESS)
    {
        std::cout << "GEMM run failed." << std::endl;
        return AMP_FAILURE;
    }

    /*******************************************************************************
    * Compare the results between cpu and gpu                                      *
    *******************************************************************************/
    if(gemmff.verifyResults() != AMP_SUCCESS)
        return AMP_FAILURE;

    /*******************************************************************************
    * print some timer information                                                 *
    *******************************************************************************/
    gemmff.printStats();

    if(accelerator().get_supports_limited_double_precision())
    {
        /***************************************************************************
        * Create an object of GEMM object(double,true)                             *
        ***************************************************************************/
        GEMM<double,false> gemmdf("C++AMP GEMM double false");

        /***************************************************************************
        * Initialize the options of the sample                                     *
        ***************************************************************************/
        status = gemmdf.initialize();
        if(status != AMP_SUCCESS)
        {
            std::cout << "GEMM initialize failed." << std::endl;
            return AMP_FAILURE;
        }

        /***************************************************************************
        * Parse command line options                                               *
        ***************************************************************************/
        if(gemmdf.parseCommandLine(argc, argv) != AMP_SUCCESS)
            return AMP_FAILURE;

        /***************************************************************************
        * Initialize the random array of input vectors                             *
        ***************************************************************************/
        status = gemmdf.setup();
        if(status != AMP_SUCCESS)
        {
            std::cout << "GEMM setup failed." << std::endl;
            return AMP_FAILURE;
        }

        /***************************************************************************
        * Execute GEMM, including the method array, array_view                     *
        ***************************************************************************/
        status = gemmdf.run();
        if(status != AMP_SUCCESS)
        {
            std::cout << "GEMM run failed." << std::endl;
            return AMP_FAILURE;
        }

        /***************************************************************************
        * Compare the results between cpu and gpu                                  *
        ***************************************************************************/
        if(gemmdf.verifyResults() != AMP_SUCCESS)
            return AMP_FAILURE;

        /***************************************************************************
        * print some timer information                                             *
        ***************************************************************************/
        gemmdf.printStats();
    }
    else
    {
        /***************************************************************************
        * print prompt message                                                     *
        ***************************************************************************/
        std::cout << "DGEMM NN skipped because the selected " \
                     "accelerator doesn't support double precision." 
                  << std::endl << std::endl;

    }

    /********************************************************************************
    * Create an object of GEMM object(float,false)                                  *
    ********************************************************************************/
    GEMM<float,true> gemmft("C++AMP GEMM float true");

    /********************************************************************************
    * Initialize the options of the sample                                          *
    ********************************************************************************/
    status = gemmft.initialize();
    if(status != AMP_SUCCESS)
    {
        std::cout << "GEMM initialize failed." << std::endl;
        return AMP_FAILURE;
    }
    
    /********************************************************************************
    * Parse command line options                                                    *
    ********************************************************************************/
    if(gemmft.parseCommandLine(argc, argv) != AMP_SUCCESS)
        return AMP_FAILURE;

    /********************************************************************************
    * Initialize the random array of input vectors                                  *
    ********************************************************************************/
    status = gemmft.setup();
    if(status != AMP_SUCCESS)
    {
        std::cout << "GEMM setup failed." << std::endl;
        return AMP_FAILURE;
    }

    /********************************************************************************
    * Execute GEMM, including the method array, array_view                          *
    ********************************************************************************/
    status = gemmft.run();
    if(status != AMP_SUCCESS)
    {
        std::cout << "GEMM run failed." << std::endl;
        return AMP_FAILURE;
    }

    /********************************************************************************
    * Compare the results between cpu and gpu                                       *
    ********************************************************************************/
    if(gemmft.verifyResults() != AMP_SUCCESS)
        return AMP_FAILURE;

    /********************************************************************************
    * print some timer information                                                  *
    ********************************************************************************/
    gemmft.printStats();

    if(accelerator().get_supports_limited_double_precision())
    {
        /****************************************************************************
        * Create an object of GEMM object(double,true)                              *
        ****************************************************************************/
        GEMM<double,true> gemmdt("C++AMP GEMM double true");

        /****************************************************************************
        * Initialize the options of the sample                                      *
        ****************************************************************************/
        status = gemmdt.initialize();
        if(status != AMP_SUCCESS)
        {
            std::cout << "GEMM initialize failed." << std::endl;
            return AMP_FAILURE;
        }

        /****************************************************************************
        * Parse command line options                                                *
        ****************************************************************************/
        if(gemmdt.parseCommandLine(argc, argv) != AMP_SUCCESS)
            return AMP_FAILURE;

        /****************************************************************************
        * Initialize the random array of input vectors                              *
        ****************************************************************************/
        status = gemmdt.setup();
        if(status != AMP_SUCCESS)
        {
            std::cout << "GEMM setup failed." << std::endl;
            return AMP_FAILURE;
        }

        /****************************************************************************
        * Execute GEMM, including the method array, array_view                      *
        ****************************************************************************/
        status = gemmdt.run();
        if(status != AMP_SUCCESS)
        {
            std::cout << "GEMM run failed." << std::endl;
            return AMP_FAILURE;
        }

        /****************************************************************************
        * Compare the results between cpu and gpu                                   *
        ****************************************************************************/
        if(gemmdt.verifyResults() != AMP_SUCCESS)
            return AMP_FAILURE;

        /****************************************************************************
        * print some timer information                                              *
        ****************************************************************************/
        gemmdt.printStats();
    }
    else
    {
        /****************************************************************************
        * print prompt message                                                      *
        ****************************************************************************/
        std::cout << "DGEMM TN skipped because the selected " \
                     "accelerator doesn't support double precision." 
                  << std::endl << std::endl;;
    }

    return AMP_SUCCESS;
}