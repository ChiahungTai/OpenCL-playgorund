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

/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include "TileStaticStorageBandwidth.hpp"




/******************************************************************************
* Implementation of TileStaticBandwidth<T>::setup()                           *
******************************************************************************/
template <class T>
int TileStaticBandwidth<T>::setup()
{
    if(printDeviceList() != AMP_SUCCESS)
        return AMP_FAILURE;
    if(setDefaultAccelerator() != AMP_SUCCESS)
        return AMP_FAILURE;
    for(unsigned int i = 0; i<outputLength; i++)
    {
        output.push_back(T(0));
    }

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of TileStaticBandwidth<T>::initialize()                      *
******************************************************************************/
template <class T>
int TileStaticBandwidth<T>::initialize()
{
    if(this->AmpSample::initialize())
        return AMP_FAILURE;

    ampsdk::Option* num_iterations = new ampsdk::Option;
    CHECK_ALLOCATION(num_iterations,"num_iterators memory allocation failed");

    num_iterations->_sVersion = "i";
    num_iterations->_lVersion = "iterations";
    num_iterations->_description = "Number of iterations for kernel execution";
    num_iterations->_type = ampsdk::CA_ARG_INT;
    num_iterations->_value = &iter;

    sampleArgs->AddOption(num_iterations);
    delete num_iterations;
    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of TileStaticBandwidth<T>::run()                             *
******************************************************************************/
template <class T>
int TileStaticBandwidth<T>::run()
{
    array<T,1> outputArray(outputLength,output.begin());

    cout << "\nTile Static Memory Read\nAccessType\t: single\n"<< endl;
    cout << "Bandwidth\t";
    measureReadSingle(outputArray);

    array<T,1> outputArray2(outputLength,output.begin());
    cout << "\nTile Static Memory Read\nAccessType\t: linear\n"<< endl;
    cout << "Bandwidth\t";
    measureReadLinear(outputArray2);

    array<T,1> outputArray3(outputLength,output.begin());
    cout << "\nTile Static Memory Write\nAccessType\t: linear\n"<< endl;
    cout << "Bandwidth\t";
    measureWriteLinear(outputArray3);

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of TileStaticBandwidth<T>::verifyResults()                   *
******************************************************************************/
template <class T>
int TileStaticBandwidth<T>::verifyResults()
{
    if(isPassed)
    {
        cout << "Passed !" << endl;
    }
    else
    {
        cout << "Failed !" << endl;
    }
    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of TileStaticBandwidth<T>::calBandwidth                      *
******************************************************************************/
template <class T>
double TileStaticBandwidth<T>::calBandwidth(double kernelTime)
{
    double d = LENGTH * NUM_ACCESS * sizeof(float);
    double dataSize = d/1024/1024/1024;
    double bandwidth = dataSize * iter / kernelTime;
    return bandwidth;
}

/*******************************************************************************
* Implementation of read single test                                           *
*******************************************************************************/

template <class T>
void TileStaticBandwidth<T>::readSingle(array<T> & outArray) {
	parallel_for_each(tiled_extent<TILE_SIZE>(outArray.extent)
						, [&outArray](tiled_index<TILE_SIZE> tidx) restrict(amp)
	{
		float vala =0;
		float valb =0;
		tile_static float lds[NUM_ACCESS];
		lds[tidx.local[0]] = 1;
		tidx.barrier.wait_with_tile_static_memory_fence();
        for(unsigned int i=0;i<NUM_ACCESS;i+=2)
        {
            vala+= lds[i];
            valb+= lds[i+1];
        }
        outArray[tidx.global[0]] = vala + valb;
    });
}

template <class T>
void TileStaticBandwidth<T>::measureReadSingle(array<T> & outArray)
{
    double sec = 0;
	accelerator_view accView = accelerator().default_view;
	// warm up run
	readSingle(outArray);
	accView.flush();
	accView.wait();

	// create a timer and start the performance measurement
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);
    for(unsigned int i = 0;i < iter; i++)
    {    
		readSingle(outArray);
        accView.flush();
    }
    accView.wait();
    sampleCommon->stopTimer(timer);
    sec = (double)(sampleCommon->readTimer(timer));
    cout << ": " << calBandwidth(sec) << " GB/s" << endl;
    if(verify)
    {
        copy(outArray,output.begin());
        vector<T>::iterator iter;
        for(iter = output.begin();iter < output.end();iter++)
        {
            if(*iter != 256)
            {
                isPassed = false;
                return ;
            }
        }
    }
    return ;
}

/*******************************************************************************
* Implementation of read linear test                                           *
*******************************************************************************/
template<class T>
void TileStaticBandwidth<T>::readLinear(array<T> & outArray) 
{
	parallel_for_each(tiled_extent<TILE_SIZE>(outArray.extent)
					, [&outArray](tiled_index<TILE_SIZE> tidx) restrict(amp)
	{
        float vala =0;
        float valb =0;
        tile_static float lds[NUM_ACCESS+TILE_SIZE];
		float t = (float)tidx.local[0];
        lds[tidx.local[0]] = t;
        lds[tidx.local[0]+TILE_SIZE] = t;
        tidx.barrier.wait_with_tile_static_memory_fence();
        for(unsigned int i=0;i<NUM_ACCESS;i+=2)
        {
            vala+= lds[i+tidx.local[0]];
            valb+= lds[i+1+tidx.local[0]];
        }
        outArray[tidx.global[0]] = vala + valb;
	});
}

template <class T>
void TileStaticBandwidth<T>::measureReadLinear(array<T> & outArray)
{
    double sec = 0;
    accelerator_view accView = accelerator().default_view;
	// warm up run
	readLinear(outArray);
	accView.flush();
	accView.wait();

	// create a timer and start the performance measurement
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);
    for(unsigned int i = 0;i < iter; i++)
    {
		readLinear(outArray);
        accView.flush();
    }
    accView.wait();
    sampleCommon->stopTimer(timer);
    sec = (double)(sampleCommon->readTimer(timer));
    cout << ": " << calBandwidth(sec) << " GB/s" << endl;
    if(verify)
    {
        copy(outArray,output.begin());
        vector<T>::iterator iter;

		// compute the reference output
		T ref[TILE_SIZE];
		for (int i = 0; i < TILE_SIZE; i++) 
		{
			T r = 0;
			for (int j = 0, ii = i; j < NUM_ACCESS; j++, ii++) 
			{
				r+=(T)(ii%TILE_SIZE);
			}
			ref[i] = r;
		}
		// verify the output data
		int i = 0;
        for(iter = output.begin();iter < output.end();iter++)
        {
            if(*iter != ref[i%TILE_SIZE])
            {
                isPassed = false;
                return;
            }
        }
    }
    return;
}

/*******************************************************************************
* Implementation of write linear test                                          *
*******************************************************************************/

template <class T>
void TileStaticBandwidth<T>::writeLinear(array<T> &outArray) {
	parallel_for_each(tiled_extent<TILE_SIZE>(outArray.extent)
					, [&outArray](tiled_index<TILE_SIZE> tidx) restrict(amp)
    {
        float vala =0;
        float valb =0;
        tile_static float lds[NUM_ACCESS+TILE_SIZE];
		float t = (float)tidx.tile_extent[0];
        for(unsigned int i=0;i<NUM_ACCESS;i+=2)
        {
            lds[i+tidx.local[0]] = t;
            lds[i+1+tidx.local[0]] = t;
        }
        tidx.barrier.wait_with_tile_static_memory_fence();
		vala = lds[tidx.local[0]];
		valb = lds[tidx.local[0]+1];
		outArray[tidx.global[0]] = vala + valb;
    });
}

template <class T>
void TileStaticBandwidth<T>::measureWriteLinear(array<T> &outArray)
{
    double sec = 0;
    accelerator_view accView = accelerator().default_view;

	// warm up run
	writeLinear(outArray);
	accView.flush();
	accView.wait();

	// create a timer and start the performance measurement
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);
    for(unsigned int i = 0;i < iter; i++)
    {    
		writeLinear(outArray);
        accView.flush();
    }
    accView.wait();
    sampleCommon->stopTimer(timer);
    sec = (double)(sampleCommon->readTimer(timer));
    cout << ": " << calBandwidth(sec) << " GB/s" << endl;
    if(verify)
    {
        copy(outArray,output.begin());
        vector<T>::iterator iter;
        for(iter = output.begin();iter < output.end();iter++)
        {
			if(*iter != 2*TILE_SIZE)
            {
                isPassed = false;
                return;
            }
        }
    }
    return;
}

/********************************************************************************
* Execution of program begins from here                                         *
********************************************************************************/
int main(int argc, char* argv[])
{
    std::cout << "************************************************" << std::endl;
    std::cout << "TileStaticStorageBandWidth " << std::endl;
    std::cout << "************************************************" << std::endl;
    std::cout << std::endl << std::endl;

    int status = 0;

    /****************************************************************************
    * Create an object of TileStaticBandwidth class                             *
    ****************************************************************************/
    TileStaticBandwidth<float> tileBandwidth("Tile Static bandwidth");

    /****************************************************************************
    * Initialize the options of the sample                                      *
    ****************************************************************************/
    if(tileBandwidth.initialize() != AMP_SUCCESS)
        return AMP_FAILURE;

    /****************************************************************************
    * Parse command line options                                                *
    ****************************************************************************/
    if(tileBandwidth.parseCommandLine(argc,argv) != AMP_SUCCESS)
        return AMP_FAILURE;

    /****************************************************************************
    * Initialize the random array of input samples                              *
    ****************************************************************************/
    status = tileBandwidth.setup();
    if(status != AMP_SUCCESS)
        return AMP_FAILURE;

    /****************************************************************************
    * Execute TileStaticBandwidth, including printstats and verifyResults       *
    ****************************************************************************/
    if(tileBandwidth.run() != AMP_SUCCESS)
        return AMP_FAILURE;

    /********************************************************************************
    * Compare the results between cpu and gpu                                                                              *
    ********************************************************************************/
	if(tileBandwidth.verifyResults() != AMP_SUCCESS)
		return AMP_FAILURE;

    return 0;
}

