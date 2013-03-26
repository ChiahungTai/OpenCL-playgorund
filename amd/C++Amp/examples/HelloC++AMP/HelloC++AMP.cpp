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

/**
*******************************************************************************
* @file <HelloC++AMP.cpp>
* 
* @brief Implements GPU for saxpy in using AMP ArrayView comparing to cpu loop
******************************************************************************/

// header file for C++ AMP
#include <amp.h>
#include <iostream>
using namespace concurrency;
using std::vector;

/******************************************************************************
* Number of elements per vector
******************************************************************************/
const int numElement = 10000;

/******************************************************************************
* Execution of program begins from here 
******************************************************************************/
int main(int argc, char *argv[])
{
    /**************************************************************************
    * Create vectors and variable to be used as input and output                           
	**************************************************************************/
	float alpha = 0.5f;
    vector<float> vX(numElement);
    vector<float> vY(numElement);
	vector<float> vZ_GPU(numElement);
	vector<float> vZ_CPU(numElement);

    /**************************************************************************
    * Initialize input data.                                                
    **************************************************************************/
	for(int i = 0; i < numElement; i++)
	{
		vX[i] = (float)(rand() % numElement);
		vY[i] = (float)(rand() % numElement);
	}
    
    /**************************************************************************
    * Execute saxpy algorithm on CPU
    **************************************************************************/
	for(int i = 0; i < numElement; i++)
	{
		vZ_CPU[i] = alpha * vX[i] + vY[i];
	}

    /**************************************************************************
    * Execute saxpy algorithm on GPU
    **************************************************************************/

	// Wrap around the input vectors with array_view so that the GPU could access them
    array_view<const float, 1> avX(numElement, vX);
    array_view<const float, 1> avY(numElement, vY);

	// Wrap around the output vector with array_view.
	// discard_data is a hint to the runtime that content of vector will be overwritten.
    array_view<float, 1>       avZ_GPU(numElement, vZ_GPU);
	avZ_GPU.discard_data();

	// This instructs the runtime to launch "numElement" of threads to execute this lambda function in parallel
	parallel_for_each(avX.extent, [=](index<1> idx) restrict(amp)
    {
        // idx is the zero-based index of the current thread.
		// Thread #0 --> idx == 0, Thread #1 --> idx == 1, ...
        avZ_GPU[idx] = alpha * avX[idx] + avY[idx];
    });

	// Copy the results from the GPU back to CPU
    avZ_GPU.synchronize();

    /**************************************************************************
    * Verify the result calculated by GPU and CPU loop.
    **************************************************************************/
	for(int i = 0; i < numElement; i++)
	{
		if(vZ_GPU[i] != vZ_CPU[i])
		{
			std::cout << "Verification failed !!!" << std::endl;
			exit(1);
		}
	}
	std::cout << "Verification passed." << std::endl;

    return 0;
}