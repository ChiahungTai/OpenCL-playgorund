/**********************************************************************
Copyright ?012 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

?Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
?Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/
#include "IntroStaticCPPKernel.hpp"

#include <vector>
#include <CL/cl.hpp>
#include <iostream>
#include <fstream>
#include <string>


using namespace cl;

/**
* Class Rect
* Class Rect describes rectangles with lengths and widths.
*/
//template <typename T> class Rect;

template <typename T>
class Rect {
	T myLength; /**< length of the rectangle */
	T myWidth; /**< width of the rectangle */
public:
	Rect () {
	}
	Rect (T a, T b) {
		myLength = a;
		myWidth = b;
	}
	T get_length(){
		return myLength;
	}

	T get_width(){
		return myWidth;
	}
};



/******************************************************************************* 
*  @fn     verification 
*  @brief  This function is to check if the.   
*             
*  @param[in] area : area calculated by CPU 
*  @param[in] devArea : area calculated by GPU
*  @param[in] count : size of array 
*           
*  @return int : 0 if successful; otherwise 1. 
*******************************************************************************/

template<typename T>
int verification(T* area, T* devArea,  int count, const char* option)
{

	std::cout <<std::endl<<"The area of the first myRectangle by CPU is:      "<<area[0]<<std::endl; 
	std::cout <<"The area of the first myRectangle by GPU is:      "<<devArea[0]<< "      "<<std::endl;
	std::cout <<std::endl<< "Continue validating..." <<std::endl;

	int isCorrect = 1;
	for (int i = 1; i <= count; i++)
	{
		if((area[i-1] - devArea[i-1])>1e-4)
		{
			isCorrect = 0;
			break;
		}
	}


	if(isCorrect == 1)
	{
		std::cout <<option<<" static C++ kernel is working correctly!!!"<<std::endl<<std::endl<<std::endl;  
		return 0;
	}
	else
	{
		std::cout <<option<<" static C++ kernel is NOT working correctly!!!"<<std::endl<<std::endl<<std::endl;   
		return 1;
	}
};





int main(int argc, char * argv[]) {

	cl_int status = 0;
	const char* buildOption ="-x clc++ ";
	const int LIST_SIZE = 128;




	// Create and initiate arrays of class Rect <float> myRectangle and areas.

	Rect <float> myRectangleF[LIST_SIZE];
	float myAreaF[LIST_SIZE];
	float myDevAreaF[LIST_SIZE];

	Rect <int> myRectangleI[LIST_SIZE];
	int myAreaI[LIST_SIZE];
	int myDevAreaI[LIST_SIZE];

	srand (10);
	for(int i = 0; i < LIST_SIZE; i++) 
	{
		myRectangleF[i] =Rect <float> ((rand() % 15) + (float) 0.1, (rand() % 15) + (float) 0.1);
		myAreaF[i] = myRectangleF[i].get_length() * myRectangleF[i].get_width();
		myDevAreaF[i] = 0;

		myRectangleI[i] =Rect <int> ((rand() % 15) + 1, (rand() % 15) + 1);
		myAreaI[i] = myRectangleI[i].get_length() * myRectangleI[i].get_width();
		myDevAreaI[i] = 0;
	}

	// Get available platforms
	std::vector<Platform> platforms;
	status = Platform::get(&platforms);
	if (status != CL_SUCCESS)
	{
		std::cout<<"Error: Getting platforms!"<<std::endl;
		return SDK_FAILURE;
	}
	std::vector<cl::Platform>::iterator iter;
	for(iter = platforms.begin(); iter != platforms.end(); ++iter)
	{
		if(!strcmp((*iter).getInfo<CL_PLATFORM_VENDOR>().c_str(), "Advanced Micro Devices, Inc."))
		{
			break;
		}
	}
	cl_context_properties cps[3] = {CL_CONTEXT_PLATFORM, (cl_context_properties)(*iter) (), 0};
	Context context = cl::Context(CL_DEVICE_TYPE_GPU, cps, NULL, NULL, &status);

	if (status != CL_SUCCESS)
	{
		std::cout<<"GPU not found, falling back to CPU!"<<std::endl;
		context = cl::Context(CL_DEVICE_TYPE_CPU, cps, NULL, NULL, &status);
		if (status != CL_SUCCESS)
		{
			std::cout<<"Error: Creating context!"<<std::endl;
			return SDK_FAILURE;
		}
	}

	try
	{

		// Get a list of devices on this platform
		std::vector<Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();

		// Create a command queue and use the first device
		CommandQueue queue = CommandQueue(context, devices[0]);

		// Read source file
		std::ifstream sourceFile("IntroStaticCPPKernel_Kernels.cl");
		std::string sourceCode(
			std::istreambuf_iterator<char>(sourceFile),
			(std::istreambuf_iterator<char>()));
		Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length()+1));

		// Make program of the source code in the context
		Program program = Program(context, source);

		// Build program for these specific devices
		program.build(devices, buildOption);

		// Make kernel
		Kernel kernelF(program, "AreaCalculation_float");
		Kernel kernelI(program, "AreaCalculation_int");

		// Create memory buffers
		Buffer bufferMyRectangleF = Buffer(context, CL_MEM_READ_WRITE, LIST_SIZE * sizeof(Rect <float>));
		Buffer bufferMyAreaF = Buffer(context, CL_MEM_READ_WRITE, LIST_SIZE * sizeof(float));
		Buffer bufferMyRectangleI = Buffer(context, CL_MEM_READ_WRITE, LIST_SIZE * sizeof(Rect <int>));
		Buffer bufferMyAreaI = Buffer(context, CL_MEM_READ_WRITE, LIST_SIZE * sizeof(int));

		// Copy myRectangle[LIST_SIZE] to the memory buffers
		queue.enqueueWriteBuffer(bufferMyRectangleF, CL_TRUE, 0, LIST_SIZE * sizeof(Rect <float>), myRectangleF);
		queue.enqueueWriteBuffer(bufferMyRectangleI, CL_TRUE, 0, LIST_SIZE * sizeof(Rect <int>), myRectangleI);

		// Set arguments to kernel
		kernelF.setArg(0, bufferMyRectangleF);
		kernelF.setArg(1, bufferMyAreaF);
		kernelI.setArg(0, bufferMyRectangleI);
		kernelI.setArg(1, bufferMyAreaI);

		// Run the kernel on specific ND range
		NDRange global(LIST_SIZE);  
		NDRange local(LIST_SIZE / 2);      
		queue.enqueueNDRangeKernel(kernelF, NullRange, global, local);
		queue.enqueueNDRangeKernel(kernelI, NullRange, global, local);

		// Read buffer back to CPU
		queue.enqueueReadBuffer(bufferMyAreaF, CL_TRUE, 0, LIST_SIZE * sizeof(float), myDevAreaF);
		queue.enqueueReadBuffer(bufferMyAreaI, CL_TRUE, 0, LIST_SIZE * sizeof(int), myDevAreaI);

	}catch(cl::Error e){
		std::cout << "Line "<< __LINE__<<": Error in "<<e.what() <<std::endl;
		return SDK_FAILURE;
	}

	verification(myAreaF, myDevAreaF, LIST_SIZE, "FLOAT");
	verification(myAreaI, myDevAreaI, LIST_SIZE, "INT");

	return SDK_SUCCESS;

}


