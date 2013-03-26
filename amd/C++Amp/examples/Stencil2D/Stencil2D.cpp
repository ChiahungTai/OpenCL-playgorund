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
#include "Stencil2D.h"

/******************************************************************************
* namespace std Concurrency                                                   *
******************************************************************************/
using namespace concurrency;


/******************************************************************************
* Global function::toString()                                                 *
******************************************************************************/
template<typename T>
std::string toString(T t, std::ios_base &(*r)(std::ios_base&))
{
    std::ostringstream output;
    output<<r<<t;
    return output.str();
}

/******************************************************************************
* Implementation of Stencil2D::initialize()                                   *
******************************************************************************/

template<class T>
int Stencil2D<T>::initialize()
{

    /**************************************************************************
    * Call base class Initialize to get default configuration                 *
    **************************************************************************/ 
    if(this->AmpSample::initialize())
        return AMP_FAILURE;

      ampsdk::Option* iterationTimes = new ampsdk::Option;
      CHECK_ALLOCATION(iterationTimes,"Memory Allocation error.(iteration)");

      iterationTimes->_sVersion = "i";
      iterationTimes->_lVersion = "iteration";
      iterationTimes->_description = "kernel execute times. default 10.";
      iterationTimes->_type = ampsdk::CA_ARG_INT;
      iterationTimes->_value = &iteration;

      sampleArgs->AddOption(iterationTimes);
      delete iterationTimes;

      ampsdk::Option* arrayView = new ampsdk::Option;
      CHECK_ALLOCATION(arrayView, "Memory Allocation error.(arrayView)");

      arrayView->_sVersion = "V";
      arrayView->_lVersion = "arrayView";
      arrayView->_description = "use array_view instead of array.";
      arrayView->_type = ampsdk::CA_NO_ARGUMENT;
      arrayView->_value = &arrayview;

      sampleArgs->AddOption(arrayView);
      delete arrayView;

    return AMP_SUCCESS;
}


/******************************************************************************
* Implementation of Stencil2D::setup()                                        *
******************************************************************************/
template<class T>
int Stencil2D<T>::setup()
{
    std::uniform_real_distribution<T> rngDist(-1.0, 1.0);
    std::mt19937 rng;

    /**************************************************************************
    * initialize input and output vector                                      *
    **************************************************************************/
    for(int i = 0; i < nMatrixElems; ++i) 
    {
            inVec.push_back( rngDist(rng));
            outVec.push_back(0);
    }

    /**************************************************************************
    * Randomize stencil coefficients.                                         *
    **************************************************************************/
     centerWeight   = rngDist(rng);
     cardinalWeight = rngDist(rng);
     diagonalWeight = rngDist(rng);

    /**************************************************************************
    * Kernel execute times boundary check                                     *
    **************************************************************************/
    if(iteration < 1)
    {
        std::cout<<"warning:Kernel execute times must >= 1, use 10 to execute."<<std::endl;
        iteration = 10;
    }

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of Stencil2D::run()                                          *
******************************************************************************/
template<class T>
int Stencil2D<T>::run()
{
    std::cout<<(sizeof(T) == 4 ? "SinglePrecision" : "DoublePrecision")<<"    3x3 stencil    ";
    std::cout<<"Size "<<matrixEdge<<"*"<<matrixEdge<<std::endl;

    if(!quiet)
    {
        size_t num = inVec.size() > 64 ? 64 : inVec.size();
        printArray("Input (portion of the input)", inVec, (int)num, 1, 4096);
    }

    if(arrayview)
    {
        std::cout<<"Running array_view mode.\n";
        if(runArrayView() != AMP_SUCCESS)
            return AMP_FAILURE;
    }
    else
    {
        std::cout<<"Running array mode.\n";
        if(runArray() != AMP_SUCCESS)
            return AMP_FAILURE;
    }

    std::cout<<"Running finished."<<std::endl;

    if(!quiet)
    {
        size_t num = outVec.size() > 64 ? 64 : outVec.size();
        printArray("Output (portion of the output)", outVec, (int)num, 1, 4096);
    }
    return AMP_SUCCESS;

}

/******************************************************************************
* Implementation of Stencil2D::runArray()                                     *
******************************************************************************/
template<class T>
int Stencil2D<T>::runArray()
{

    /**************************************************************************
    * calculate data copy time from cpu tp gpu                                *
    **************************************************************************/
    array<T, 2> dInMatrix(paddedMatrixEdge, paddedMatrixEdge);
    array<T, 2> dOutMatrix(paddedMatrixEdge, paddedMatrixEdge);
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer); 

    copy(inVec.begin(), dInMatrix);

    sampleCommon->stopTimer(timer);
    cpToDeviceTime = sampleCommon->readTimer(timer);


    /**************************************************************************
    * warm up                                                                 *
    **************************************************************************/
    stencil(dInMatrix, dOutMatrix, centerWeight, cardinalWeight, diagonalWeight);

    concurrency::accelerator_view accView = concurrency::accelerator().default_view;
    accView.flush();
    accView.wait();

    /**************************************************************************
    * calculate total kernel execute time                                     *
    **************************************************************************/
    int timer1 = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer1);
    sampleCommon->startTimer(timer1); 

    for (int i = 0; i < iteration; i++)
    {
        stencil(dInMatrix, dOutMatrix, centerWeight, cardinalWeight, diagonalWeight);

        accView.flush();
    }

    accView.wait();

    sampleCommon->stopTimer(timer1);
    totalKernelTime = sampleCommon->readTimer(timer1);
    averageKernelTime = totalKernelTime / iteration;

    /**************************************************************************
    * calculate data copy time from gpu to cpu                                *
    **************************************************************************/
    int timer2 = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer2);
    sampleCommon->startTimer(timer2); 

    copy(dOutMatrix, outVec.begin());

    sampleCommon->stopTimer(timer2);
    cpToHostTime = sampleCommon->readTimer(timer2);

    totalTime = cpToDeviceTime + totalKernelTime + cpToHostTime;

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of Stencil2D::runArrayView()                                 *
******************************************************************************/
template<class T>
int Stencil2D<T>::runArrayView()
{

    array_view<T,  2> dInMatrix(paddedMatrixEdge, paddedMatrixEdge, inVec);
    array_view<T,  2> dOutMatrix(paddedMatrixEdge, paddedMatrixEdge, outVec);

    accelerator_view &accView = accelerator().default_view;


    /**************************************************************************
    * warm up                                                                 *
    **************************************************************************/
    stencil(dInMatrix, dOutMatrix, centerWeight, cardinalWeight, diagonalWeight);
    dOutMatrix.synchronize();


    /**************************************************************************
    * array_view total time                                                   *
    **************************************************************************/
    int totaltimer = sampleCommon->createTimer();
    sampleCommon->resetTimer(totaltimer);
    sampleCommon->startTimer(totaltimer); 

    for(int i = 0; i < iteration; i++)
    {
        stencil(dInMatrix, dOutMatrix, centerWeight, cardinalWeight, diagonalWeight);
        accView.flush();
    }

    dOutMatrix.synchronize();

    sampleCommon->stopTimer(totaltimer);
    totalTime = sampleCommon->readTimer(totaltimer);

    return AMP_SUCCESS;
}


/******************************************************************************
* Implementation of Stencil2D::stencil()                                      *
******************************************************************************/
template <typename T>
int  Stencil2D<T>::stencil(array<T, 2>& inMatrix, array<T, 2>& outMatrix, T cW, T carW, T dW)
{
    parallel_for_each(tiled_extent<16, 16>(extent<2>(matrixEdge, matrixEdge)),
        [&inMatrix, &outMatrix, cW, carW,dW](tiled_index<16, 16> idx) restrict(amp)
    {

        /**********************************************************************
        * Copy tile of matrix to shared memory                                *
        **********************************************************************/
        tile_static T block[18][18];
        block[idx.local[0] + 1][idx.local[1] + 1] = inMatrix[idx.global[0] + 1][idx.global[1] + 1];

        if(idx.local[0] == 0)
        {
            block[0][idx.local[1] + 1] = inMatrix[idx.global[0] + 0][idx.global[1] + 1];
            block[17][idx.local[1] + 1] = inMatrix[idx.global[0] + 17][idx.global[1] + 1];
        }

        if(idx.local[1] == 0) 
        {
            block[idx.local[0] + 1][0] = inMatrix[idx.global[0] + 1][idx.global[1] + 0];
            block[idx.local[0] + 1][17] = inMatrix[idx.global[0] + 1][idx.global[1] + 17];
        }

        if(idx.local[0] == 0 && idx.local[1] == 0) 
        {
            block[0][0] = inMatrix[idx.global[0] + 0][idx.global[1] + 0];
            block[17][0] = inMatrix[idx.global[0] + 17][idx.global[1] + 0];
            block[0][17] = inMatrix[idx.global[0] + 0][idx.global[1] + 17];
            block[17][17] = inMatrix[idx.global[0] + 17][idx.global[1] + 17];
        }

        idx.barrier.wait_with_tile_static_memory_fence();


        /**********************************************************************
        * Apply stencil to thread's 3x3 block                                 *
        **********************************************************************/
        const T centerVal = block[idx.local[0] + 1][idx.local[1] + 1];

        const T cardinalSum =
            block[idx.local[0] + 0][idx.local[1] + 1] +
            block[idx.local[0] + 2][idx.local[1] + 1] +
            block[idx.local[0] + 1][idx.local[1] + 0] +
            block[idx.local[0] + 1][idx.local[1] + 2];

        const T diagonalSum =
            block[idx.local[0] + 0][idx.local[1] + 0] +
            block[idx.local[0] + 0][idx.local[1] + 2] +
            block[idx.local[0] + 2][idx.local[1] + 0] +
            block[idx.local[0] + 2][idx.local[1] + 2];

        outMatrix[idx.global[0] + 1][idx.global[1] + 1] =
            cW * centerVal +
            carW * cardinalSum +
            dW * diagonalSum;
    });

    return AMP_SUCCESS;
}


/******************************************************************************
* Implementation of Stencil2D::stencil()                                      *
******************************************************************************/
template <typename T>
int  Stencil2D<T>::stencil(array_view<T, 2>& inMatrix, array_view<T, 2>& outMatrix, T cW, T carW, T dW)
{
    parallel_for_each(tiled_extent<16, 16>(extent<2>(matrixEdge, matrixEdge)),
        [=](tiled_index<16, 16> idx) restrict(amp)
    {

        /**********************************************************************
        * Copy tile of matrix to shared memory.                               *
        **********************************************************************/
        tile_static T block[18][18];
        block[idx.local[0] + 1][idx.local[1] + 1] = inMatrix[idx.global[0] + 1][idx.global[1] + 1];

        if(idx.local[0] == 0)
        {
            block[0][idx.local[1] + 1] = inMatrix[idx.global[0] + 0][idx.global[1] + 1];
            block[17][idx.local[1] + 1] = inMatrix[idx.global[0] + 17][idx.global[1] + 1];
        }

        if(idx.local[1] == 0) 
        {
            block[idx.local[0] + 1][0] = inMatrix[idx.global[0] + 1][idx.global[1] + 0];
            block[idx.local[0] + 1][17] = inMatrix[idx.global[0] + 1][idx.global[1] + 17];
        }

        if(idx.local[0] == 0 && idx.local[1] == 0) 
        {
            block[0][0] = inMatrix[idx.global[0] + 0][idx.global[1] + 0];
            block[17][0] = inMatrix[idx.global[0] + 17][idx.global[1] + 0];
            block[0][17] = inMatrix[idx.global[0] + 0][idx.global[1] + 17];
            block[17][17] = inMatrix[idx.global[0] + 17][idx.global[1] + 17];
        }

        idx.barrier.wait_with_tile_static_memory_fence();


        /**********************************************************************
        * Apply stencil to thread's 3x3 block.                                *
        **********************************************************************/
        const T centerVal = block[idx.local[0] + 1][idx.local[1] + 1];

        const T cardinalSum =
            block[idx.local[0] + 0][idx.local[1] + 1] +
            block[idx.local[0] + 2][idx.local[1] + 1] +
            block[idx.local[0] + 1][idx.local[1] + 0] +
            block[idx.local[0] + 1][idx.local[1] + 2];

        const T diagonalSum =
            block[idx.local[0] + 0][idx.local[1] + 0] +
            block[idx.local[0] + 0][idx.local[1] + 2] +
            block[idx.local[0] + 2][idx.local[1] + 0] +
            block[idx.local[0] + 2][idx.local[1] + 2];

        outMatrix[idx.global[0] + 1][idx.global[1] + 1] =
            cW * centerVal   +
            carW * cardinalSum +
            dW * diagonalSum;
    });

    return AMP_SUCCESS;
}


/******************************************************************************
* Implementation of Stencil2D::verifyResults()                                *
******************************************************************************/
template<class T>
int Stencil2D<T>::verifyResults()
{
        if(verify)
        {
            for(int y = 1; y < matrixEdge + 1; ++y)
            {
                for(int x = 1; x < matrixEdge + 1; ++x)
                {

                    /**********************************************************
                    * Apply stencil at this location.                         *
                    **********************************************************/
                    T *center = &inVec[(y + 0) * paddedMatrixEdge + x];
                    int row = paddedMatrixEdge;

                    T cardinalSum = center[-1] + center[1] + center[-row] + center[row];
                    T diagonalSum = center[-row - 1] + center[-row + 1] + center[ row - 1] + center[row + 1];
                    T refElem = center[0] * centerWeight + cardinalSum * cardinalWeight + diagonalSum * diagonalWeight;


                    /**********************************************************
                    * Compare reference value with kernel output.             *
                    **********************************************************/
                    T outElem = outVec[y * paddedMatrixEdge + x];

                    if(fabs(refElem - outElem) > 0.000001) 
                    {
                        std::cout<<"Failed!"<<std::endl;
                        std::cout<<std::endl;
                        return AMP_FAILURE;
                    }
                }
            }
            std::cout<<"Passed."<<std::endl;
            std::cout<<std::endl;
        }

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of Stencil2D::printStatus()                                  *
******************************************************************************/
template<class T>
int Stencil2D<T>::printStatus()
{

    if (timing && (!arrayview))
    {

        double nGElems = matrixEdge * matrixEdge / 1000000000.0;
        double GElems = nGElems / averageKernelTime;

        std::cout<<"(Total time(sec): "<<totalTime<<")\n";
        std::cout<<"Time information"<<std::endl;
        std::string strArray[4] = {"Data Transfer to Accelerator(sec)",
                                   "Mean Execution time(sec)",
                                   "GElems/s", "Data Transfer to Host(sec)"};
        std::string stats[4];
        stats[0]  = toString<double>(cpToDeviceTime, std::dec);
        stats[1]  = toString<double>(averageKernelTime, std::dec);
        stats[2]  = toString<double>(GElems, std::dec);
        stats[3]  = toString<double>(cpToHostTime, std::dec);

        this->AmpSample::printStats(strArray, stats, 4);
    }

    if (timing && arrayview)
    {
        std::cout<<"Total time(sec):    "<<totalTime <<std::endl;
    }

     return AMP_SUCCESS;
}

    /**************************************************************************
    * Implementation of AmpCommon::printArray()                               *
    **************************************************************************/
    template<typename T> 
    void Stencil2D<T>::printArray(
                                            std::string header, 
                                            const std::vector<T>& data, 
                                            const int width,
                                            const int height,
                                            const int offset) 
    {
        std::cout<<"\n"<<header<<"\n";
        for(int i = 0; i < height; i++)
        {
            for(int j = 0; j < width; j++)
            {
                std::cout<<data[i*width+j+offset]<<" ";
            }
            std::cout<<"\n";
        }
        std::cout<<"\n";
    }

/******************************************************************************
* Execution of program begins from here                                       *
******************************************************************************/
int main(int argc, char* argv[])
{

    std::cout<<"************************************************"<<std::endl;
    std::cout<<"              Stencil2D"<<std::endl;
    std::cout<<"************************************************"<<std::endl;
    std::cout<<std::endl;


    /**************************************************************************
    * Create an object of Stencil                                             *
    **************************************************************************/
    Stencil2D<float> stcl("Stencil2D");


    /**************************************************************************
    * Initialize the options of the sample                                    *
    **************************************************************************/
    if(stcl.initialize() != AMP_SUCCESS)
        return AMP_FAILURE;


    /**************************************************************************
    * Parse command line options                                              *
    **************************************************************************/
    if(stcl.parseCommandLine(argc, argv) != AMP_SUCCESS)
        return AMP_FAILURE;


    /**************************************************************************
    * Print all devices                                                       *
    **************************************************************************/
    if(stcl.printDeviceList() != AMP_SUCCESS)
        return AMP_FAILURE;


    /**************************************************************************
    * Set default accelerator                                                 *
    **************************************************************************/
    if(stcl.setDefaultAccelerator() != AMP_SUCCESS)
        return AMP_FAILURE;


    /**************************************************************************
    * Initialize the random array of input vectors                            *
    **************************************************************************/
    if(stcl.setup() != AMP_SUCCESS)
        return AMP_FAILURE;


    /**************************************************************************
    * Execute  including the mode array, array_view                           *
    **************************************************************************/
    if(stcl.run() != AMP_SUCCESS)
        return AMP_FAILURE;


    /**************************************************************************
    * Compare the results between cpu and gpu                                 *
    **************************************************************************/
    if(stcl.verifyResults() != AMP_SUCCESS)
        return AMP_FAILURE;


    /**************************************************************************
    * print some timer information                                            *
    **************************************************************************/
    if (stcl.printStatus() != AMP_SUCCESS)
        return AMP_FAILURE;

    if(accelerator().get_supports_limited_double_precision())
    {
        /**************************************************************************
        * Create an object of Stencil                                             *
        **************************************************************************/
        std::cout<<std::endl;
        Stencil2D<double> stcl("Stencil2D");


        /**************************************************************************
        * Initialize the options of the sample                                    *
        **************************************************************************/
        if(stcl.initialize() != AMP_SUCCESS)
            return AMP_FAILURE;


        /**************************************************************************
        * Parse command line options                                              *
        **************************************************************************/
        if(stcl.parseCommandLine(argc, argv) != AMP_SUCCESS)
            return AMP_FAILURE;


        /**************************************************************************
        * Print all devices                                                       *
        **************************************************************************/
        if(stcl.printDeviceList() != AMP_SUCCESS)
            return AMP_FAILURE;


        /**************************************************************************
        * Set default accelerator                                                 *
        **************************************************************************/
        if(stcl.setDefaultAccelerator() != AMP_SUCCESS)
            return AMP_FAILURE;


        /**************************************************************************
        * Initialize the random array of input vectors                            *
        **************************************************************************/
        if(stcl.setup() != AMP_SUCCESS)
            return AMP_FAILURE;


        /**************************************************************************
        * Execute  including the mode array, array_view                           *
        **************************************************************************/
        if(stcl.run() != AMP_SUCCESS)
            return AMP_FAILURE;


        /**************************************************************************
        * Compare the results between cpu and gpu                                 *
        **************************************************************************/
        if(stcl.verifyResults() != AMP_SUCCESS)
            return AMP_FAILURE;


        /**************************************************************************
        * print some timer information                                            *
        **************************************************************************/
        if (stcl.printStatus() != AMP_SUCCESS)
            return AMP_FAILURE;
    }
    else
    {
        /***************************************************************************
        * print prompt message                                                     *
        ***************************************************************************/
        std::cout << "Stencil2D(double precision) skipped because the selected " \
                     "accelerator doesn't support double precision." 
                  << std::endl << std::endl;
    }

    return AMP_SUCCESS;
}