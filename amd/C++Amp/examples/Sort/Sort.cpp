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
#include "Sort.hpp"

/******************************************************************************
* Implementation of Scan                                                      *
* Performs an exclusive scan across integer values provided by each thread    *
* The corresponding scanned values are returned back to each thread           *
******************************************************************************/
template <int TILESIZE>
unsigned int Scan(unsigned int value,
                  tiled_index<TILESIZE> &idx) restrict(amp)
{
    // Shared memory for in-tile scan.
    tile_static unsigned int scanIn[2 * TILESIZE];

    // Set the lower half of shared memory to zero.
    scanIn[idx.local[0]] = 0;

    // Copy each thread's value into the upper half.
    unsigned int *scanInUpper = &scanIn[TILESIZE];
    scanInUpper[idx.local[0]] = value;
    idx.barrier.wait_with_tile_static_memory_fence();

    // Perform a Kogge-Stone scan across values.
    for(int i = 1; i < TILESIZE; i <<= 1) 
    {
        unsigned int tmpVal = scanInUpper[idx.local[0] - i];
        idx.barrier.wait_with_tile_static_memory_fence();

        scanInUpper[idx.local[0]] += tmpVal;
        idx.barrier.wait_with_tile_static_memory_fence();
    }

    return scanInUpper[idx.local[0] - 1];
}

/******************************************************************************
* Implementation of Sort::checkCommand()                                      *
******************************************************************************/
int Sort::checkCommand()
{
    // length validation
    if(length > 20)
    {
        std::cout << "length is larger than 20 , set it equal to 20!" << std::endl;
        length = 20;  
    }

    if(length < 1)
    {
        std::cout << "length is less than 1 , set it equal to 1!" << std::endl;
        length = 1;  
    }

    // iteration validation
    if(iter <= 1)
    {
        std::cout << "Iterations should be bigger than 1, please try again!" << std::endl;
        return AMP_FAILURE;
    }

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of Sort::outputSummary()                                     *
******************************************************************************/
void Sort::outputSummary() 
{
    std::cout << "\n";
    std::cout << "Sampling: " << iter;
    std::cout << " (" << iter << " sampled) benchmark runs\n";
    std::cout.flush();
}

/******************************************************************************
* Implementation of Sort::outputData()                                        *
******************************************************************************/
void Sort::outputData(const std::string &modeText,
                double workDone,
                const std::string &workRateUnit)
{

    std::cout << modeText << std::fixed << std::setprecision(3) << " finished.";

    // print timing information
    if(timing)
    {
        // mean time
        double meanMs = (totalKernelTime * 1000.0) / iter;
        double workRate = workDone / (meanMs / 1000.0);
        int workRatePrecision = (workRate >= 100.0 ? 0 : 1);

        // print total time
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
}

/******************************************************************************
* Implementation of Sort::toString()                                          *
******************************************************************************/
template<typename T>
std::string Sort::toString(T t, std::ios_base &(*r)(std::ios_base&))
{
    std::ostringstream output;
    output << r << t;
    return output.str();
}

/******************************************************************************
* Implementation of Sort::initialize()                                        *
******************************************************************************/
int Sort::initialize()
{
    //Call base class Initialize to get default configuration 
    if(this->AmpSample::initialize())
        return AMP_FAILURE;

    ampsdk::Option* array_length = new ampsdk::Option;
    CHECK_ALLOCATION(array_length,"Memory Allocation error.(array_length)");

    array_length->_sVersion = "s";
    array_length->_lVersion = "size";
    array_length->_description = " Data size in millions with range [1,20]";
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

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of Sort::setup()                                             *
******************************************************************************/
int Sort::setup()
{
    // print all devices
    if(this->AmpSample::printDeviceList() != AMP_SUCCESS)
    {
        return AMP_FAILURE;
    }

    //set DeaultAccelerator
    if(this->AmpSample::setDefaultAccelerator() != AMP_SUCCESS)
    {
        return AMP_FAILURE;
    }

    // commands validation
    if(checkCommand() != AMP_SUCCESS)
    {
        return AMP_FAILURE;
    }

    // Print benchmarking information.
    outputSummary();

    nIntegerElems = length * 1024 * 1024;
    integers = std::vector<unsigned int>(nIntegerElems);
    sortedIntegers = std::vector<unsigned int>(nIntegerElems);

    std::uniform_int_distribution<unsigned int> rngDist;
    std::mt19937 rng;

    for(int i = 0; i < nIntegerElems; ++i) 
    {
        integers[i] = rngDist(rng);
    }

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of Sort::run()                                               *
******************************************************************************/
int Sort::run()
{
    int status = 0;

    // warm up memroy
    std::vector<unsigned int> inteWarmUp(nIntegerElems, 0);
    array<unsigned int> dInteWarmUp(nIntegerElems, inteWarmUp.begin());
    // Allocate space for temporary integers and histograms.
    array<unsigned int> dTmpInteWarmUp(nIntegerElems);
    array<unsigned int> dTmpHistWarmUp(cNTiles * 16);

    // calculate data transfer time (host to device)
    int cpToDeviceTimer = sampleCommon->createTimer();
    status = sampleCommon->resetTimer(cpToDeviceTimer);
    status = sampleCommon->startTimer(cpToDeviceTimer);

    // Copy vector to the device.
    array<unsigned int> dIntegers(nIntegerElems, integers.begin());
    // Allocate space for temporary integers and histograms.
    array<unsigned int> dTmpIntegers(nIntegerElems);
    array<unsigned int> dTmpHistograms(cNTiles * 16);

    status = sampleCommon->stopTimer(cpToDeviceTimer);
    if(status != AMP_SUCCESS)
    {
        return AMP_FAILURE;
    }
    cpToDeviceTime = sampleCommon->readTimer(cpToDeviceTimer);

    // warm up
    RadixSort(dInteWarmUp, dTmpInteWarmUp, dTmpHistWarmUp);

    // calculate kernel execution time.
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    // Repeat the benchmark and compute a mean kernel execution time.
    for(int run = 0; run < iter; ++run) 
    {
        RadixSort(dIntegers, dTmpIntegers, dTmpHistograms);
    }
    accelerator().default_view.wait();
    sampleCommon->stopTimer(timer);
    if(status != AMP_SUCCESS)
    {
        return AMP_FAILURE;
    }
    totalKernelTime = sampleCommon->readTimer(timer);

    // calculate data transfer time (device to host)
    int cpToHostTimer = sampleCommon->createTimer();
    status = sampleCommon->resetTimer(cpToHostTimer);
    status = sampleCommon->startTimer(cpToHostTimer);

    // data transfer(device to host)
    copy(dIntegers, sortedIntegers.begin());

    status = sampleCommon->stopTimer(cpToHostTimer);
    if(status != AMP_SUCCESS)
    {
        return AMP_FAILURE;
    }
    cpToHostTime = sampleCommon->readTimer(cpToHostTimer);

    totalTime = cpToDeviceTime + totalKernelTime + cpToHostTime;

    // print out the first 128 elements of the whole output
    if(!quiet)
    {
        sampleCommon->printArray<unsigned int>("Output (the first 128 elements)",sortedIntegers, 128, 1);
    }

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of Sort::verifyResults()                                     *
******************************************************************************/
int Sort::verifyResults()
{
    doValidation = verify;
    if(doValidation) 
    {
        // Check that the output vector is fully sorted.
        // As a further approximate inclusion test, compare the sums of both vectors.
        std::cout << " [Validating... ";
        std::cout.flush();

        uint64_t integerSum       = integers[0];
        uint64_t sortedIntegerSum = sortedIntegers[0];

        for(int i = 1; i < nIntegerElems; ++i) 
        {
            if(sortedIntegers[i] < sortedIntegers[i - 1]) 
            {
                std::cerr << "failed at " << i << ": ";
                std::cerr << sortedIntegers[i] << " < " << sortedIntegers[i - 1] << "]\n\n";
                std::cerr << "Benchmark aborted due to error\n";
                exit(1);
            }

            integerSum       += integers[i];
            sortedIntegerSum += sortedIntegers[i];
        }

        if(integerSum != sortedIntegerSum) 
        {
            std::cerr << "failed: input/output vector sum mismatch]\n\n";
            std::cerr << "Benchmark aborted due to error\n";
            exit(1);
        }

        std::cout << "passed]\n";
    }
    std::cout << "\n";
    std::cout.flush();

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of Sort::printStats()                                        *
******************************************************************************/
void Sort::printStats()
{
    // Report mean/SD/throughput to the user.
    std::stringstream modeStr;
    modeStr << "Radix sort " << length << "M (1M~20M) integer keys";

    double mKeys = double(nIntegerElems) / 1000000.0;
    outputData(modeStr.str(), mKeys, "Mkeys/s");
    std::cout << "\n";
}

/******************************************************************************
* Implementation of Sort::BuildHistograms()                                   *
* Computes an unmerged set of histograms of the low 4-bit values              *
* of a given set of integers, following a parameterized bitwise               *
* right shift of each element.                                                *
******************************************************************************/
void Sort::BuildHistograms(array<unsigned int> &integers,
                     array<unsigned int> &histograms,
                     int shift)
{
    parallel_for_each(tiled_extent<cTileSize>(extent<1>(cNTiles * cTileSize)),
        [&integers, &histograms, shift](tiled_index<cTileSize> idx) restrict(amp)
    {
        // Shared memory for in-tile histogram reduction.
        tile_static unsigned int reduceIn[cTileSize];

        // Private histogram (counts of 0-15) for this thread.
        unsigned int histogram[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0};

        // Compute our tile's assigned region of the integer set.
        const int regionSize  = integers.extent[0] / cNTiles;
        const int regionStart = idx.tile[0] * regionSize;
        const int regionEnd   = regionStart + regionSize;

        // Stride threads across the tile's region.
        for(int i = regionStart + idx.local[0];
                i < regionEnd;
                i += cTileSize)
        {
            // Build a histogram of the low 4 bytes (0-15) after a right shift.
            // Function is parameterized by shift [0,4,8,12,16,20,24,28].
            ++histogram[(integers[i] >> shift) & 0xF];
        }

        // Reduce histograms across the tile.
        for(int digit = 0; digit < 16; ++digit) 
        {
            // Write our value to shared memory.
            reduceIn[idx.local[0]] = histogram[digit];
            idx.barrier.wait_with_tile_static_memory_fence();

            // Subsets of threads reduce histogram entry to a single value.
            for(int i = cTileSize / 2; i > 0; i >>= 1) 
            {
                if(idx.local[0] < i) 
                {
                    reduceIn[idx.local[0]] += reduceIn[idx.local[0] + i];
                }
                idx.barrier.wait_with_tile_static_memory_fence();
            }

            // One thread writes reduced value to intermediate sum array.
            if(idx.local[0] == 0) 
            {
                histograms[digit * cNTiles + idx.tile[0]] = reduceIn[0];
            }
        }
    });
}

/********************************************************************************
* Implementation of Sort::ScanHistograms()                                      *
* Performs a scan of each digit in a given set of histograms (0-15 counts).     *
* Scanned histograms are not merged, but structured the same as the source array*
********************************************************************************/
void Sort::ScanHistograms(array<unsigned int> &histograms) 
{
    parallel_for_each(tiled_extent<cNTiles>(extent<1>(cNTiles)),
        [&histograms](tiled_index<cNTiles> idx) restrict(amp)
    {
        // Shared value recording the start of the current histogram's scan.
        // To break a scan into multiple small scans, need to carry across the
        // last value from one to the first value of the next.
        tile_static unsigned int scanBaseValue;
        scanBaseValue = 0;

        // Scan each digit across the histogram.
        for(int digit = 0; digit < 16; ++digit) 
        {
            // Perform a scan across the histogram value computed by each tile in BuildHistograms.
            // There is exactly one thread per histogram (cNTiles threads total).
            unsigned int digitCount = histograms[digit * cNTiles + idx.local[0]];
            unsigned int scannedDigitCount = Scan(digitCount, idx);
            histograms[digit * cNTiles + idx.local[0]] = scannedDigitCount + scanBaseValue;

            // Carry the highest scanned value across to the next histogram.
            // Scan is exclusive, so include digit count from the last thread.
            if(idx.local[0] == cNTiles - 1) 
            {
                scanBaseValue += scannedDigitCount + digitCount;
            }

            idx.barrier.wait_with_tile_static_memory_fence();
        }
    });
}

/********************************************************************************
* Implementation of Sort::SortIntegerKeys()                                     *
********************************************************************************/
void Sort::SortIntegerKeys(array<unsigned int> &integers,
                     array<unsigned int> &sortedIntegers,
                     array<unsigned int> &scannedHistograms,
                     int shift)
{
    parallel_for_each(tiled_extent<cTileSize>(extent<1>(cNTiles * cTileSize)),
      [&integers, &sortedIntegers, &scannedHistograms, shift](tiled_index<cTileSize> idx) restrict(amp)
    {
        // Shared cache of scanned digit (0-15) counts for this tile's assigned histogram.
        tile_static unsigned int scannedDigitCounts[16];

        // Cross-tile/region accumulation of histogram values (see next declaration).
        tile_static unsigned int tileHistogram[16];

        // Private histogram to track the next available
        // positions for integers sorted in this pass.
        unsigned int histogram[16];

        // Zero accumulated histogram and read scanned digit counts into cache.
        if(idx.local[0] < 16) 
        {
            tileHistogram[idx.local[0]] = 0;
            scannedDigitCounts[idx.local[0]] = scannedHistograms[idx.local[0] * cNTiles + idx.tile[0]];
        }

        // Work on four integer elements per iteration per thread.
        uint_4 *integers4 = (uint_4 *)&integers[0];
        int nIntegers4 = integers.extent[0] >> 2;

        // Compute our tile's region of the integer set.
        const int blockSize  = nIntegers4 / cNTiles;
        const int blockStart = idx.tile[0] * blockSize;
        const int blockEnd   = blockStart + blockSize;

        // Stride threads across the tile's region.
        // Note: Cannot initialize with (blockStart + idx.local[0]) because
        // HLSL cannot statically determine that control flow through barriers
        // inside the loop is not divergent.
        for(int i = blockStart; i < blockEnd; i += cTileSize) 
        {
            for(int j = 0; j < 16; ++j) 
            {
                histogram[j] = 0;
            }

            // Read four integers and shift/mask to extract the keys being sorted.
            uint_4 integer4 = integers4[i + idx.local[0]];
            uint_4 key4 = (integer4 >> shift) & 0xF;

            // Track digit counts locally prior to tile-wide scan.
            ++histogram[key4.x];
            ++histogram[key4.y];
            ++histogram[key4.z];
            ++histogram[key4.w];

            // Perform a tile-wide exclusive scan across new histogram values for each digit.
            // This assigns regions in the output for each set of digits in each thread.
            for(int j = 0; j < 16; ++j) 
            {
                idx.barrier.wait_with_tile_static_memory_fence();
                histogram[j] = Scan(histogram[j], idx);
            }

            // Write four integers to key-sorted regions of the output set.
            // Increment histogram counts further to make exclusive scan inclusive.
            unsigned int sortedIdx;

            sortedIdx = histogram[key4.x] + scannedDigitCounts[key4.x] + tileHistogram[key4.x];
            sortedIntegers[sortedIdx] = integer4.x;
            ++histogram[key4.x];

            sortedIdx = histogram[key4.y] + scannedDigitCounts[key4.y] + tileHistogram[key4.y];
            sortedIntegers[sortedIdx] = integer4.y;
            ++histogram[key4.y];

            sortedIdx = histogram[key4.z] + scannedDigitCounts[key4.z] + tileHistogram[key4.z];
            sortedIntegers[sortedIdx] = integer4.z;
            ++histogram[key4.z];

            sortedIdx = histogram[key4.w] + scannedDigitCounts[key4.w] + tileHistogram[key4.w];
            sortedIntegers[sortedIdx] = integer4.w;
            ++histogram[key4.w];

            // Record the final, inclusive-scanned locations for the next sorting pass.
            idx.barrier.wait_with_tile_static_memory_fence();

            if(idx.local[0] == (cTileSize - 1)) 
            {
                for(int j = 0; j < 16; ++j) 
                {
                    tileHistogram[j] += histogram[j];
                }
            }

            idx.barrier.wait_with_tile_static_memory_fence();
        }
    });
}

/********************************************************************************
* Implementation of Sort::RadixSort()                                           *
********************************************************************************/
int Sort::RadixSort(array<unsigned int> &integers,
          array<unsigned int> &tmpIntegers,
          array<unsigned int> &tmpHistograms)
{

    accelerator_view accView = accelerator().default_view;

    for(int shift = 0; shift < 32; shift += 4) 
    {
        // Swap input and output vectors on each iteration.
        bool swapVectors = (shift % 8 != 0);
        array<unsigned int> &srcIntegers = (swapVectors ? tmpIntegers : integers);
        array<unsigned int> &dstIntegers = (swapVectors ? integers    : tmpIntegers);

        BuildHistograms(srcIntegers, tmpHistograms, shift);
        accView.flush();
        ScanHistograms(tmpHistograms);
        accView.flush();
        SortIntegerKeys(srcIntegers, dstIntegers, tmpHistograms, shift);
        accView.flush();
    }

    return AMP_SUCCESS;
}

/********************************************************************************
* Execution of program begins from here                                         *
********************************************************************************/
int main(int argc,
         char *argv[])
{
    std::cout << "************************************************" << std::endl;
    std::cout << "RadixSort " << std::endl;
    std::cout << "************************************************" << std::endl;
    std::cout << std::endl << std::endl;

    /****************************************************************************
    * Create an object of Sort class                                            *
    ****************************************************************************/
    Sort sort("C++Amp Sort");

    /****************************************************************************
    * Initialize the options of the sample                                      *
    ****************************************************************************/
    if(sort.initialize() != AMP_SUCCESS)
        return AMP_FAILURE;

    /****************************************************************************
    * Parse command line options                                                *
    ****************************************************************************/
    if(sort.parseCommandLine(argc, argv) != AMP_SUCCESS)
        return AMP_FAILURE;

    /****************************************************************************
    * Initialize the random array of input samples                              *
    ****************************************************************************/
    if(sort.setup() != AMP_SUCCESS)
        return AMP_FAILURE;

    /****************************************************************************
    * Execute radixsort                                                         *
    ****************************************************************************/
    if(sort.run() != AMP_SUCCESS)
        return AMP_FAILURE;

    /****************************************************************************
    * Verify the results that were generated                                    *
    ****************************************************************************/
    if(sort.verifyResults() != AMP_SUCCESS)
        return AMP_FAILURE;

    /****************************************************************************
    * Print performance statistics                                              *
    ****************************************************************************/
    sort.printStats();

    return AMP_SUCCESS;
}