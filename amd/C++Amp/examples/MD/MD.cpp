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
*Included header files                                                                                                           *
******************************************************************************/
#include "MD.hpp"

/******************************************************************************
*Global function::toString()                                                                                                   *
******************************************************************************/
template<typename T>
std::string toString(T t, std::ios_base &(*r)(std::ios_base&))
{
    std::ostringstream output;
    output << r << t;
    return output.str();
}

/******************************************************************************
*Implementation of MD<FPTYPE>::outputSummary()                                                          *
******************************************************************************/
template <typename FPTYPE>
void MD<FPTYPE>::outputSummary() 
{
    std::cout << "\n";
    std::cout << "Sampling: " << iter;
    std::cout << " (" << iter << " sampled) benchmark runs\n\n";
    std::cout.flush();
}

/******************************************************************************
*Implementation of MD<FPTYPE>::outputData()                                                                 *
******************************************************************************/
template <typename FPTYPE>
void MD<FPTYPE>::outputData(const std::string &modeText,
                            double workDone,
                            const std::string &workRateUnit)
{

        double meanMs = totalKernelTime / iter;
        double workRate = workDone / meanMs;
        int workRatePrecision = (workRate >= 100.0 ? 0 : 1);

        std::cout << modeText << std::fixed << std::setprecision(3) << " finished.";

        if(timing)
        {
        std::cout << " (Total Time(sec): " << totalTime << ")\n";

        std::cout << "\n\nTime Information" << std::endl;
        std::string strArray[4] = {"Data Transfer to Accelerator (sec)", "Mean Execution Time (sec)", workRateUnit, "Data Transfer to Host (sec)"};
        std::string stats[4];
        stats[0] = toString<double>(cpToGpuTime, std::dec);
        stats[1] = toString<double>(meanMs, std::dec);
        stats[2] = toString<double>(workRate, std::dec);
        stats[3] = toString<double>(cpToHostTime, std::dec);

        this->AmpSample::printStats(strArray, stats, 4);
        }

        if(!quiet)
        {
        printArray();
        }
}

/******************************************************************************
* Implementation of MD<FPTYPE>::LJ ()                                                                            *
******************************************************************************/
template <typename FPTYPE>
void MD<FPTYPE>::LJ(array<typename short_vector<FPTYPE, 4>::type> &positions,
                    array<int> &neighbors,
                    array<typename short_vector<FPTYPE, 4>::type> &forces,
                    const FPTYPE cutOffDistSqrd,
                    const FPTYPE ljConst1,
                    const FPTYPE ljConst2)
{
    typedef typename short_vector<FPTYPE, 4>::type FPTYPE4;

    parallel_for_each(extent<1>(forces.extent[0]),
        [&positions, &neighbors, &forces, cutOffDistSqrd, ljConst1, ljConst2](index<1> idx) restrict(amp)
    {
        // Accumulate forces exerted by each of our nearest neighbors.
        const FPTYPE4 ourPos = positions[idx[0]];
        const int *const myNeighbors = &neighbors[idx[0]];
        FPTYPE4 force = FPTYPE4(0);

        for(int i = 0; i < cMaxAtomNeighbors; ++i) 
        {
            // Calculate the position delta and distance of this neighbor.
            const FPTYPE4 nPos = positions[myNeighbors[i * forces.extent[0]]];
            const FPTYPE4 nDelta = nPos - ourPos;
            const FPTYPE  nDistPwr2 = nDelta.x * nDelta.x + nDelta.y * nDelta.y + nDelta.z * nDelta.z;

            // Only consider interactions within the cut-off distance.
            if(nDistPwr2 < cutOffDistSqrd) 
            {
                const FPTYPE nDistPwr2Inv = FPTYPE(1.0) / nDistPwr2;
                const FPTYPE nDistPwr6Inv = nDistPwr2Inv * nDistPwr2Inv * nDistPwr2Inv;
                const FPTYPE ljForce = nDistPwr2Inv * nDistPwr6Inv * (ljConst1 * nDistPwr6Inv - ljConst2);

                force += nDelta * ljForce;
            }
        }

        forces[idx] = force;
    });
}

/******************************************************************************
* Implementation of  MD<FPTYPE>::initialize()                                                                     *
******************************************************************************/
template <typename FPTYPE>
int MD<FPTYPE>::initialize()
{
    //Call base class Initialize to get default configuration 
    if(this->AmpSample::initialize())
        return AMP_FAILURE;

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
* Implementation of  MD<FPTYPE>::setup()                                                                        *
******************************************************************************/
template <typename FPTYPE>
int MD<FPTYPE>::setup()
{
    typedef typename short_vector<FPTYPE, 4>::type FPTYPE4;

    if(iter <= 0)
    {
        std::cerr << "Error: iter (" << iter << ") should be bigger than 0. \n\n";
        return AMP_FAILURE;
    }

    // Print benchmarking information.
    outputSummary();

    // Disable double precision tests if full support is unavailable.
    if(accelerator().supports_double_precision == 0) 
    {
        std::cerr << "Warning: AMP accelerator does not implement full double precision support.\n\n";

        doDPTests = false;
    }

    // Naive k-NN atom search is slow, so do it once here.
    randomizeInputData();

    // Downscale data for single-precision as well.
    spPositions = std::vector<float_4>(dpPositions.size());
    forces = std::vector<FPTYPE4>(nAtoms);

    for(size_t i = 0; i < dpPositions.size(); ++i) 
    {
        double_4 dpP = dpPositions[i];
        spPositions[i] = float_4(float(dpP.x), float(dpP.y), float(dpP.z), 0);
    }

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of  MD<FPTYPE>::run()                                                                           *
******************************************************************************/
template <typename FPTYPE>
int MD<FPTYPE>::run(std::vector<typename short_vector<FPTYPE, 4>::type> &positions)
{
    typedef typename short_vector<FPTYPE, 4>::type FPTYPE4;

    // Copy positions and neighbors to the device.
    nAtoms = length;

    std::cout << "Run MD " << std::endl;

    //calculate data copy time from cpu tp gpu
    int cpytoGpuTimer = sampleCommon->createTimer();
    sampleCommon->resetTimer(cpytoGpuTimer);
    sampleCommon->startTimer(cpytoGpuTimer);

    array<FPTYPE4> dPositions(nAtoms, positions.begin());
    array<int> dNeighbors(int(neighbors.size()), neighbors.begin());

    sampleCommon->stopTimer(cpytoGpuTimer);
    cpToGpuTime = sampleCommon->readTimer(cpytoGpuTimer);

    // Allocate space for output potential force.
    array<FPTYPE4> dForces(nAtoms);

    // Warm up
    LJ(dPositions, dNeighbors, dForces, cCutOffDistanceSqrd, cLJConst1, cLJConst2);

    concurrency::accelerator_view &accView = concurrency::accelerator().default_view;
    accView.flush();
    accView.wait();

    // Repeat the benchmark and compute a mean kernel execution time.
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    for(int run = 0; run < iter; run++) 
    {
        LJ(dPositions, dNeighbors, dForces, cCutOffDistanceSqrd, cLJConst1, cLJConst2);
    }

    accView.flush();
    accView.wait();

    sampleCommon->stopTimer(timer);
    totalKernelTime = sampleCommon->readTimer(timer);

    //calculate data copy time from gpu to cpu
    int cpytoHostTimer = sampleCommon->createTimer();
    sampleCommon->resetTimer(cpytoHostTimer);
    sampleCommon->startTimer(cpytoHostTimer);

    copy(dForces, forces.begin());

    sampleCommon->stopTimer(cpytoHostTimer);
    cpToHostTime = sampleCommon->readTimer(cpytoHostTimer);

    //calculate total time include data transfer time and total execution time
    totalTime = cpToGpuTime + totalKernelTime + cpToHostTime;

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of  MD<FPTYPE>::randomizeInputData()                                                  *
******************************************************************************/
template <typename FPTYPE>
int MD<FPTYPE>::randomizeInputData()
{
    std::cout << "[Precomputing molecular data structures, please wait... ";
    std::cout.flush();

    // Randomize atom positions.
    nAtoms = length;
    dpPositions.resize(nAtoms);

    std::uniform_real_distribution<double> posDist(0.0, cDomainEdge);
    std::mt19937 rng;

    for(int i = 0; i < nAtoms; ++i)
    {
        dpPositions[i].x = posDist(rng);
        dpPositions[i].y = posDist(rng);
        dpPositions[i].z = posDist(rng);
        dpPositions[i].w = 0;
    }

    // Find the nearest 64 (or fewer) neighbors within the cut-off distance of each atom.
    neighbors.resize(nAtoms * cMaxAtomNeighbors);

    // Use a priority queue for inserted sort by inverse distance.
    struct Neighbor 
    {
        Neighbor(int index, double distSqrd) : index(index), distSqrd(distSqrd) {}
        bool operator >(const Neighbor &other) const { return distSqrd > other.distSqrd; }
        int index;
        double distSqrd;
    };

    typedef std::priority_queue<Neighbor, std::vector<Neighbor>, std::greater<Neighbor>> NeighborQueue;

    // Naive N^2 k-NN algorithm. It's a little slow, sorry!
#pragma omp parallel for
    for(int i = 0; i < nAtoms; ++i) 
    {
        NeighborQueue nearestNeighbors;
        double_4 ourPos = dpPositions[i];

        for(int j = 0; j < nAtoms; ++j) 
        {
            // Not a neighbor of ourself.
            if(i == j) 
            {
                continue;
            }

            // Compute the distance^2 between us and this neighbor.
            double_4 nPos = dpPositions[j];
            double_4 nDelta = ourPos - nPos;
            double    nDistSqrd = nDelta.x * nDelta.x + nDelta.y * nDelta.y + nDelta.z * nDelta.z;

            // Insert neighbors within the cut-off distance into a priority queue.
            if(nDistSqrd < cCutOffDistanceSqrd) 
            {
                nearestNeighbors.push(Neighbor(j, nDistSqrd));
            }
        }

        // Now retrieve the nearest neighbors.
        for(int j = 0; j < cMaxAtomNeighbors; ++j) 
        {
            neighbors[j * nAtoms + i] = nearestNeighbors.top().index;
            nearestNeighbors.pop();
        }
    }

    std::cout << "done.]\n\n";

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of  MD<FPTYPE>::verifyResults()                                                             *
******************************************************************************/
template <typename FPTYPE>
int MD<FPTYPE>::verifyResults(std::vector<typename short_vector<FPTYPE, 4>::type> &positions)
{
    typedef typename short_vector<FPTYPE, 4>::type FPTYPE4;

    doValidation = verify;
    if(doValidation) 
    {
        std::cout << "  [Validating... ";

        for(int i = 0; i < nAtoms; ++i) 
        {
            // Accumulate forces exerted by each of our nearest neighbors.
            FPTYPE4 ourPos = positions[i];
            FPTYPE4 refForce = FPTYPE4(0);

            for(int j = 0; j < cMaxAtomNeighbors; ++j) 
            {
                // Calculate the position delta and distance of this neighbor.
                FPTYPE4 nPos = positions[neighbors[j * nAtoms + i]];
                FPTYPE4 nDelta = nPos - ourPos;
                FPTYPE  nDistPwr2 = nDelta.x * nDelta.x + nDelta.y * nDelta.y + nDelta.z * nDelta.z;

                // Only consider interactions within the cut-off distance.
                if(nDistPwr2 < FPTYPE(cCutOffDistanceSqrd)) 
                {
                    FPTYPE nDistPwr2Inv = FPTYPE(1.0) / nDistPwr2;
                    FPTYPE nDistPwr6Inv = nDistPwr2Inv * nDistPwr2Inv * nDistPwr2Inv;
                    FPTYPE ljForce = nDistPwr2Inv * nDistPwr6Inv * (FPTYPE(cLJConst1) * nDistPwr6Inv - FPTYPE(cLJConst2));

                    refForce += nDelta * ljForce;
                }
            }

            // Compare the computed and reference values.
            FPTYPE4 force = forces[i];
            FPTYPE4 deltaPc = (force - refForce) / force;
            double  refDiff = sqrt(deltaPc.x * deltaPc.x + deltaPc.y * deltaPc.y + deltaPc.z * deltaPc.z);

            if(fabs(refDiff) > 0.001) 
            {
                std::cerr << "failed at " << i << ": ";
                std::cerr << "expected " << refForce.x << "x" << refForce.y << "x" << refForce.z;
                std::cerr << " but have " << force.x << "x" << force.y << "x" << force.z << "\n\n";
                std::cerr << "Benchmark aborted due to error\n";
                exit(1);
            }
        }

        std::cout.flush();
        std::cout << "passed]\n";
    }

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of   MD<FPTYPE>::printStats()                                                                 *
******************************************************************************/
template <typename FPTYPE>
void MD<FPTYPE>::printStats()
{
    // Report mean/SD/throughput to the user.
    std::stringstream modeStr;
    modeStr << (sizeof(FPTYPE) == 4 ? "SP" : "DP");
    modeStr << " LJ potential " << length << " atoms";

    //We guarantee 100% in-cut-off to match OpenCL benchmark.
    int64_t nFLOP = (8 * nAtoms * cMaxAtomNeighbors) + (nAtoms * cMaxAtomNeighbors * 13);
    double nGFLOP = nFLOP / 1000000000.0;

    outputData(modeStr.str(), nGFLOP, "GFLOPS");
}

/******************************************************************************
* Implementation of MD::printArray()                                                                                    *
******************************************************************************/
template<typename FPTYPE>
int MD<FPTYPE>::printArray()
{
    std::cout << "\n\nInput point (The first 256 elements)    force (The first 256 elements)" << std::endl;
    for (int i = 0; i < 256; i++)
    {
        std::cout << "{  ";
        std::cout << std::setw(9) << spPositions[i].x << ", ";
        std::cout << std::setw(9) << spPositions[i].y << ", ";
        std::cout << std::setw(9) << spPositions[i].z << " ";
        std::cout << "} \t";

        std::cout << "{ ";
        std::cout << std::setw(12) << forces[i].x << ", ";
        std::cout << std::setw(12) << forces[i].y << ", ";
        std::cout << std::setw(12) << forces[i].z << " ";
        std::cout << "} " << std::endl;
    }
    std::cout<<std::endl;

    return AMP_FAILURE;
}

int main(int argc,
         char *argv[])
{
    std::cout << "************************************************" << std::endl;
    std::cout << "                    MD " << std::endl;
    std::cout << "************************************************" << std::endl;
    std::cout << std::endl;

    /********************************************************************************
    * Create an object of MD object(float)                                                                                      *
    ********************************************************************************/
    MD<float> mdf("C++AMP MD");

    /********************************************************************************
    * Initialize the options of the sample                                                                                          *
    ********************************************************************************/
    if(mdf.initialize() != AMP_SUCCESS)
        return AMP_FAILURE;

    /********************************************************************************
    * Parse command line options                                                                                                  *
    ********************************************************************************/
    if(mdf.parseCommandLine(argc, argv) != AMP_SUCCESS)
        return AMP_FAILURE;

    /********************************************************************************
    * Print all devices                                                                                                                     *
    ********************************************************************************/
    mdf.printDeviceList();

    /********************************************************************************
    * Set default accelerator                                                                                                            *
    ********************************************************************************/
    if(mdf.setDefaultAccelerator() != AMP_SUCCESS)
        return AMP_FAILURE;

    /********************************************************************************
    * Initialize the random array of input vectors                                                                              *
    ********************************************************************************/
    if(mdf.setup() != AMP_SUCCESS)
        return AMP_FAILURE;

    /********************************************************************************
    * Execute MD                                                                                                                          *
    ********************************************************************************/
    if(mdf.run(mdf.spPositions) != AMP_SUCCESS)
        return AMP_FAILURE;

    /********************************************************************************
    * Compare the results between cpu and gpu                                                                              *
    ********************************************************************************/
    if(mdf.verifyResults(mdf.spPositions) != AMP_SUCCESS)
        return AMP_FAILURE;

    /********************************************************************************
    * print some timer information                                                                                                  *
    ********************************************************************************/
    mdf. printStats();

    // IF double precision available, run double
    if(mdf.doDPTests)
    {
        /******************************************************************************
        * Create an object of MD object(double)                                                                               *
        ******************************************************************************/
        MD<double> mdd("C++AMP MD");

        /******************************************************************************
        * Initialize the options of the sample                                                                                       *
        ******************************************************************************/
        if(mdd.initialize() != AMP_SUCCESS)
            return AMP_FAILURE;

        /******************************************************************************
        * Parse command line options                                                                                               *
        ******************************************************************************/
        if(mdd.parseCommandLine(argc, argv) != AMP_SUCCESS)
            return AMP_FAILURE;

        /******************************************************************************
        * Set default accelerator                                                                                                        *
        ******************************************************************************/
        if(mdd.setDefaultAccelerator() != AMP_SUCCESS)
            return AMP_FAILURE;

        /******************************************************************************
        * Initialize the random array of input vectors                                                                          *
        ******************************************************************************/
        if(mdd.setup() != AMP_SUCCESS)
            return AMP_FAILURE;

        /******************************************************************************
        * Execute MD                                                                                                                      *
        ******************************************************************************/
        if(mdd.run(mdd.dpPositions) != AMP_SUCCESS)
            return AMP_FAILURE;

        /******************************************************************************
        * Compare the results between cpu and gpu                                                                          *
        ******************************************************************************/
        if(mdd.verifyResults(mdd.dpPositions) != AMP_SUCCESS)
            return AMP_FAILURE;

        /******************************************************************************
        * print some timer information                                                                                               *
        ******************************************************************************/
        mdd. printStats();
    }

    return AMP_SUCCESS;
}