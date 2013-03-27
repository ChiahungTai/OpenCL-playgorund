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
//we want to use POSIX functions
#pragma warning( push )
#pragma warning( disable : 4996 )

#define INPUT_IMAGE "My5_TMMultiDevice.rgb"

//local work size
#define BLOCK_DIM 16

#define TEST_RUNS 20

//to enable kernel version with per pixel processing
#define PER_PIXEL

//memory objects used by the sample
cl_mem		g_inputBuffer = NULL;
cl_mem		g_inputBuffer1 = NULL;
cl_mem		g_inputBuffer2 = NULL;
cl_mem		g_outputBuffer = NULL;
cl_mem		g_inputSubBuffer = NULL;
cl_mem		g_outputSubBuffer = NULL;
cl_mem		g_inputSubBufferPG = NULL;
cl_mem		g_outputSubBufferPG = NULL;
cl_mem		g_HDRDataBuffer = NULL;

//context, either single- or multi-device ("shared") 
cl_context	g_context = NULL;

//command queue for CPU and Processor Graphics
cl_command_queue g_cmd_queue = NULL;
cl_command_queue g_cmd_queue_pg = NULL;

cl_program	g_program = NULL;
cl_kernel	g_kernel = NULL;

cl_uint 	g_globalWorkSize = 0;

//alignment required by the USE_HOST_PTR to avoid copying upon map/unmap, values for CPU and Processor Graphics devices resp
cl_int      g_min_align = 0;
cl_int      g_min_align_pg = 0;

//events list, one event for CPU and another for Processor Graphics
cl_event	g_events_list[2];


//set of flags to enable various processing branches/modes
bool g_bRunOnPG = false;//executing on Processor-Graphics (single-device mode)
bool g_bUseBothDevices = false;//executing simultaneously on CPU and Processor-Graphics (multi-device mode)
bool g_bSwapImage = false;//emulating the new input arriving each frame
bool g_bSilentMode = true;//disabling detailed staticstics




//performance counters
LARGE_INTEGER g_PerfFrequency;
LARGE_INTEGER g_PerformanceCountOverallStart;
LARGE_INTEGER g_PerformanceCountOverallStop;

//variables for load-balancing and averaging of the splitting ratio (CB - cyclic buffer to keep performance history of the prev frames)
cl_double g_NDRangeTime1 = 1;
cl_double g_NDRangeTime2 = 1;
cl_double g_NDRangeTimeRatioLast = 0.5;
cl_double g_NDRangeTimeRatioCB[256];
cl_uint g_NDRangeTimeRatioCBCounter=0;
const cl_uint g_NDRangeTimeRatioCBLength=10;
cl_bool g_NDRangeTimeRatioCBFull = false;

//counter for image swap mode
cl_uint g_ImageSwapCounter=0;


void Cleanup_OpenCL()
{
    //release kernel, program, memory etc objects
	if(!g_bSwapImage)
	{
		if( g_inputBuffer ) {clReleaseMemObject( g_inputBuffer ); g_inputBuffer = NULL;}
	}
	else
	{
		if( g_inputBuffer1 ) {clReleaseMemObject( g_inputBuffer1 ); g_inputBuffer1 = NULL;}
		if( g_inputBuffer2 ) {clReleaseMemObject( g_inputBuffer2 ); g_inputBuffer2 = NULL;}
	}
    if( g_outputBuffer ) {clReleaseMemObject( g_outputBuffer );  g_outputBuffer = NULL;}
    if( g_inputSubBuffer ) {clReleaseMemObject( g_inputSubBuffer ); g_inputSubBuffer = NULL;}
    if( g_outputSubBuffer ) {clReleaseMemObject( g_outputSubBuffer );  g_outputSubBuffer = NULL;}
    if( g_inputSubBufferPG ) {clReleaseMemObject( g_inputSubBufferPG ); g_inputSubBufferPG = NULL;}
    if( g_outputSubBufferPG ) {clReleaseMemObject( g_outputSubBufferPG );  g_outputSubBufferPG = NULL;}
    if( g_HDRDataBuffer ) {clReleaseMemObject( g_HDRDataBuffer );  g_HDRDataBuffer = NULL;}
    if( g_kernel ) {clReleaseKernel( g_kernel );  g_kernel = NULL;}
    if( g_program ) {clReleaseProgram( g_program );  g_program = NULL;}
    if( g_cmd_queue ) 
	{
		clFinish(g_cmd_queue);
		clReleaseCommandQueue( g_cmd_queue );  
		g_cmd_queue = NULL;
	}
    if( g_cmd_queue_pg ) 
	{
		clFinish(g_cmd_queue_pg);
		clReleaseCommandQueue( g_cmd_queue_pg );  
		g_cmd_queue_pg = NULL;
	}
    if( g_context ) {clReleaseContext( g_context );  g_context = NULL;}
}

bool Setup_OpenCL( const char *program_source )
{
    cl_device_id devices[16];
    size_t cb;
    cl_uint size_ret = 0;
    cl_int err;
    int num_units;
    cl_device_id device_ID;
    char device_name[128] = {0};
	
	printf("Trying to run on %s\n", g_bUseBothDevices? "CPU+Processor Graphics" : (g_bRunOnPG ? "Processor Graphics": "CPU"));

    cl_platform_id intel_platform_id = GetIntelOCLPlatform();
    if( intel_platform_id == NULL )
    {
        printf("ERROR: Failed to find Intel OpenCL platform.\n");
        return false;
    }

    cl_context_properties context_properties[3] = {CL_CONTEXT_PLATFORM, (cl_context_properties)intel_platform_id, NULL };

    // create the OpenCL context with CPU/PG or both
	cl_device_type dev = g_bUseBothDevices ? (CL_DEVICE_TYPE_CPU+CL_DEVICE_TYPE_GPU) : (g_bRunOnPG ? CL_DEVICE_TYPE_GPU:CL_DEVICE_TYPE_CPU);
	g_context = clCreateContextFromType(context_properties, dev, NULL, NULL, NULL);
	
	if (g_context == (cl_context)0)
	{
        printf("ERROR: Failed to clCreateContextFromType...\n");
        Cleanup_OpenCL();
        return false;
	}


    // get the official list of the devices associated with context
    err = clGetContextInfo(g_context, CL_CONTEXT_DEVICES, 0, NULL, &cb);
    clGetContextInfo(g_context, CL_CONTEXT_DEVICES, cb, devices, NULL);

    //queue for the first device(the only queue in the case of single-device scenario)
	g_cmd_queue = clCreateCommandQueue(g_context, devices[0], CL_QUEUE_PROFILING_ENABLE, NULL);
    if (g_cmd_queue == (cl_command_queue)0)
    {
        printf("ERROR: Failed to clCreateCommandQueue device 0 (CPU)...\n");
        Cleanup_OpenCL();
        return false;
    }

	if(g_bUseBothDevices)
	{
		//queue for the second device
		g_cmd_queue_pg = clCreateCommandQueue(g_context, devices[1], CL_QUEUE_PROFILING_ENABLE, NULL);
		if (g_cmd_queue_pg == (cl_command_queue)0)
		{
			printf("ERROR: Failed to clCreateCommandQueue device 1 (PG)...\n");
			Cleanup_OpenCL();
			return false;
		}
	}


    char *sources = ReadSources(program_source);	//read program .cl source file
    g_program = clCreateProgramWithSource(g_context, 1, (const char**)&sources, NULL, NULL);
    if (g_program == (cl_program)0)
    {
        printf("ERROR: Failed to create Program with source...\n");
        Cleanup_OpenCL();
        free(sources);
        return false;
    }

    err = clBuildProgram(g_program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        printf("ERROR: Failed to build program...\n");
        BuildFailLog(g_program, devices[0]);
        Cleanup_OpenCL();
        free(sources);
        return false;
    }
	//kernel object is shared by devices, similarly to program object
#ifdef PER_PIXEL
	g_kernel = clCreateKernel(g_program, "ToneMappingPerPixel", NULL);
#else
    g_kernel = clCreateKernel(g_program, "ToneMappingLine", NULL);
#endif
    if (g_kernel == (cl_kernel)0)
    {
        printf("ERROR: Failed to create kernel...\n");
        Cleanup_OpenCL();
        free(sources);
        return false;
    }

	free(sources);

    //let's retrieve info on the devices in the context

	// query caps of the first device
    device_ID = devices[0];

    err = clGetDeviceInfo(device_ID, CL_DEVICE_NAME, 128, device_name, NULL);
    if (err!=CL_SUCCESS)
    {
        printf("ERROR: Failed to get device information (device name)...\n");
        Cleanup_OpenCL();
        return false;
    }
    printf("Using device %s...\n", device_name);

    err = clGetDeviceInfo(device_ID, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &num_units, NULL);
    if (err!=CL_SUCCESS)
    {
        printf("ERROR: Failed to get device information (max compute units)...\n");
        Cleanup_OpenCL();
        return false;
    }
    printf("Using %d compute units...\n", num_units);


    err = clGetDeviceInfo(device_ID, CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(cl_uint), &g_min_align, NULL);
    if (err!=CL_SUCCESS)
    {
        printf("ERROR: Failed to get device information (max memory base address align size)...\n");
        Cleanup_OpenCL();
        return false;
    }
    g_min_align /= 8; //in bytes
    printf("Expected min alignment for buffers is %d bytes...\n", g_min_align);

	if(g_bUseBothDevices)
	{
		// now let's query caps of the second device
		device_ID = devices[1];

		err = clGetDeviceInfo(device_ID, CL_DEVICE_NAME, 128, device_name, NULL);
		if (err!=CL_SUCCESS)
		{
			printf("ERROR: Failed to get device information (device name)...\n");
			Cleanup_OpenCL();
			return false;
		}
		printf("Using device %s...\n", device_name);

		err = clGetDeviceInfo(device_ID, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &num_units, NULL);
		if (err!=CL_SUCCESS)
		{
			printf("ERROR: Failed to get device information (max compute units)...\n");
			Cleanup_OpenCL();
			return false;
		}
		printf("Using %d compute units...\n", num_units);


		err = clGetDeviceInfo(device_ID, CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(cl_uint), &g_min_align_pg, NULL);
		if (err!=CL_SUCCESS)
		{
			printf("ERROR: Failed to get device information (max memory base address align size)...\n");
			Cleanup_OpenCL();
			return false;
		}
		g_min_align_pg /= 8; //in bytes
		printf("Expected min alignment for buffers is %d bytes...\n", g_min_align_pg);
		g_min_align = max(g_min_align, g_min_align_pg);
	}

    return true; // success...
}

cl_float* readInput(cl_uint* arrayWidth, cl_uint* arrayHeight)
{

    //Load from HDR-image

    //Variables 
    int x = 0;
    int y = 0;
    int iMemSize = 0;
    int iResultMemSize = 0;
    float fTmpVal = 0.0f;
    int iWidth = 0;
    int iHeight = 0;
    cl_float* inputArray = 0;

    FILE* pRGBAFile = 0;
    pRGBAFile = fopen(INPUT_IMAGE,"rb");
    if(!pRGBAFile)
    {
        printf("HOST: Failed to open the HDR image file!\n");
        return 0;
    }

    fread((void*)&iWidth, sizeof(int), 1, pRGBAFile);
    fread((void*)&iHeight, sizeof(int), 1, pRGBAFile);
    printf("width = %d\n", iWidth);
    printf("height = %d\n", iHeight);

    if(iWidth<=0 || iHeight<=0 || iWidth > 1000000 || iHeight > 1000000)
    {
        printf("HOST: width or height values are invalid!\n");
        fclose(pRGBAFile);
        return 0;
    }

    // The image size in memory (bytes).
    iMemSize = iWidth*iHeight*sizeof(cl_float4); 

    // Allocate memory.
    inputArray = (cl_float*)_aligned_malloc(iMemSize, g_min_align);
    if(!inputArray)
    {
        printf("Failed to allocate memory for input HDR image!\n");
        fclose(pRGBAFile);
        return 0;
    }

    // Calculate global work size 
    g_globalWorkSize = iHeight;


    // Read data from the input file to memory. 
    fread((void*)inputArray, 1, iMemSize, pRGBAFile);

    // Extended dynamic range (4 channels pixel)
    for(int i = 0; i < iWidth*iHeight*4; i++)
    {
        inputArray[i] = 5.0f*inputArray[i];
    }


    // HDR-image height & width
    *arrayWidth = iWidth;
    *arrayHeight = iHeight;

    fclose(pRGBAFile);

    // Save input image in bitmap file (without tone mapping, just linear scale and crop)
    float fTmpFVal = 0.0f;
    cl_uint* outUIntBuf=0;
    outUIntBuf = (cl_uint*)malloc(iWidth*iHeight*sizeof(cl_uint));
    if(!outUIntBuf)
    {
        free(inputArray);
        printf("Failed to allocate memory for output image!\n");
        return 0;
    }
    for(int y = 0; y < iHeight; y++)
    {
        for(int x = 0; x < iWidth; x++)
        {
            // Ensure that no value is greater than 255.0
            cl_uint uiTmp[4]; // 4 - means 4-channel pixel
            fTmpFVal = (255.0f*inputArray[(y*iWidth+x)*4+0]);
            if(fTmpFVal>255.0f) 
                fTmpFVal=255.0f;
            uiTmp[0] = (cl_uint)(fTmpFVal);

            fTmpFVal = (255.0f*inputArray[(y*iWidth+x)*4+1]);
            if(fTmpFVal>255.0f) 
                fTmpFVal=255.0f;       
            uiTmp[1] = (cl_uint)(fTmpFVal);

            fTmpFVal = (255.0f*inputArray[(y*iWidth+x)*4+2]);
            if(fTmpFVal>255.0f) 
                fTmpFVal=255.0f;
            uiTmp[2] = (cl_uint)(fTmpFVal);

            inputArray[(y*iWidth+x)*4+3] = 0.0f;
            fTmpFVal = (255.0f*inputArray[(y*iWidth+x)*4+3]);
            if(fTmpFVal>255.0f) 
                fTmpFVal=255.0f;
            uiTmp[3] = (cl_uint)(fTmpFVal);	//Alfa

            outUIntBuf[(iHeight-1-y)*iWidth+x] = 0x000000FF & uiTmp[2];
            outUIntBuf[(iHeight-1-y)*iWidth+x] |= 0x0000FF00 & ((uiTmp[1]) << 8);
            outUIntBuf[(iHeight-1-y)*iWidth+x] |= 0x00FF0000 & ((uiTmp[0]) << 16);
            outUIntBuf[(iHeight-1-y)*iWidth+x] |= 0xFF000000 & ((uiTmp[3]) << 24);
        }
    }	
    //----
    SaveImageAsBMP( outUIntBuf, iWidth, iHeight, "ToneMappingMultiDeviceInput.bmp");
    free(outUIntBuf);

    return inputArray;
}

//declaration of the reference native function
void EvaluateRaw(float* inputArray, float* outputArray, CHDRData *pData, int arrayWidth, int iRow);

void ExecuteToneMappingReference(cl_float* inputArray, cl_float* outputArray, CHDRData *pData, cl_uint arrayWidth, cl_uint arrayHeight)
{
    //image height loop
    for(unsigned int j = 0; j < arrayHeight;j++)
    {
        EvaluateRaw(inputArray, outputArray, pData, arrayWidth, j);
    }
}

//helper function for calculation of the splitting ratio (for input/output buffers)
void ComputeSplittingRatio(cl_uint arrayHeight, cl_uint *arrayHeightDev1, cl_uint *arrayHeightDev2)
{
	//estimate ratio using the previous frame performance data
	cl_double dNDRangeRatio = (g_NDRangeTime2*g_NDRangeTimeRatioLast)/(g_NDRangeTime1*(1-g_NDRangeTimeRatioLast)+g_NDRangeTime2*g_NDRangeTimeRatioLast);

	//here we compute splitting ratio,while averaging it over last "frames"
	//fill cyclic buffer
	g_NDRangeTimeRatioCB[g_NDRangeTimeRatioCBCounter] = dNDRangeRatio;
	g_NDRangeTimeRatioCBCounter++;
	cl_double tmpNDRangeTimeRatioSum = 0.0;
	//average over cyclic buffer
	int num = g_NDRangeTimeRatioCBFull ? g_NDRangeTimeRatioCBLength : g_NDRangeTimeRatioCBCounter;
	for(int iii = 0; iii < num; iii++)
	{
		tmpNDRangeTimeRatioSum += g_NDRangeTimeRatioCB[iii];
	}
	tmpNDRangeTimeRatioSum = tmpNDRangeTimeRatioSum/num;//averaging
	//check cyclic buffer fullness
	if(g_NDRangeTimeRatioCBCounter==g_NDRangeTimeRatioCBLength)
	{
		g_NDRangeTimeRatioCBFull = true;
		g_NDRangeTimeRatioCBCounter = 0;//reset cyclic buffer counter
	}
	//update ratio
	dNDRangeRatio = tmpNDRangeTimeRatioSum;

	//estimate buffer split ratio 
	*arrayHeightDev1 = (cl_uint)(dNDRangeRatio*(cl_double)arrayHeight);
	*arrayHeightDev1 = (*arrayHeightDev1 / BLOCK_DIM)*BLOCK_DIM;//make the arrayHeightDev1 to be dividable by local size
	*arrayHeightDev2 = arrayHeight - *arrayHeightDev1;//the rest is for the second device

	g_NDRangeTimeRatioLast = dNDRangeRatio;

}

//prepare sub-buffers and constant buffer
bool PrepareResources(cl_uint arrayWidth, cl_uint *arrayHeightDev1, cl_uint *arrayHeightDev2)
{
	cl_int err = CL_SUCCESS;
	cl_buffer_region inputBufferRegion = { 0,  sizeof(cl_float4) * arrayWidth * (*arrayHeightDev1) };
	cl_buffer_region outputBufferRegion = { 0,  sizeof(cl_float4) * arrayWidth * (*arrayHeightDev1) };	
	cl_buffer_region inputBufferRegionPG = { sizeof(cl_float4) * arrayWidth * (*arrayHeightDev1),  sizeof(cl_float4) * arrayWidth * (*arrayHeightDev2) };
	cl_buffer_region outputBufferRegionPG = { sizeof(cl_float4) * arrayWidth * (*arrayHeightDev1),  sizeof(cl_float4) * arrayWidth * (*arrayHeightDev2) };

	//check alignment
	assert((sizeof(cl_float4) * arrayWidth * (*arrayHeightDev1))%g_min_align==0);


	g_inputSubBuffer = clCreateSubBuffer(g_inputBuffer, 0, CL_BUFFER_CREATE_TYPE_REGION, &inputBufferRegion, &err);
	if (err != CL_SUCCESS)
	{
		printf("ERROR: Failed to clCreateSubBuffer (1)...\n");
		return false;
	}
	g_outputSubBuffer = clCreateSubBuffer(g_outputBuffer, 0, CL_BUFFER_CREATE_TYPE_REGION, &outputBufferRegion, &err);
	if (err != CL_SUCCESS)
	{
		printf("ERROR: Failed to clCreateSubBuffer (2)...\n");
		return false;
	}
	g_inputSubBufferPG = clCreateSubBuffer(g_inputBuffer, 0, CL_BUFFER_CREATE_TYPE_REGION, &inputBufferRegionPG, &err);
	if (err != CL_SUCCESS)
	{
		printf("ERROR: Failed to clCreateSubBuffer (3)...\n");
		return false;
	}
	g_outputSubBufferPG = clCreateSubBuffer(g_outputBuffer, 0, CL_BUFFER_CREATE_TYPE_REGION, &outputBufferRegionPG, &err);
	if (err != CL_SUCCESS)
	{
		printf("ERROR: Failed to clCreateSubBuffer (4)...\n");
		return false;
	}
	return true;
}

//release sub-buffers
void ReleaseSubResources()
{
	if(g_inputSubBuffer) 
	{
		clReleaseMemObject(g_inputSubBuffer); g_inputSubBuffer = NULL;
	}
	if(g_outputSubBuffer) 
	{
		clReleaseMemObject(g_outputSubBuffer); g_outputSubBuffer = NULL;
	}
	if(g_inputSubBufferPG) 
	{
		clReleaseMemObject(g_inputSubBufferPG); g_inputSubBufferPG = NULL;
	}
	if(g_outputSubBufferPG) 
	{
		clReleaseMemObject(g_outputSubBufferPG); g_outputSubBufferPG = NULL;
	}
}

bool FrameExecutionStatistics()
{
	cl_ulong start = 0;
    cl_ulong end = 0;
    cl_int err = CL_SUCCESS;

    //notice that pure HW execution time is END-START
	err = clGetEventProfilingInfo(g_events_list[0], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
	if (err != CL_SUCCESS)
	{
		printf("ERROR: Failed to get clGetEventProfilingInfo CL_PROFILING_COMMAND_START...\n");
		return false;
	}
	err = clGetEventProfilingInfo(g_events_list[0], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
	if (err != CL_SUCCESS)
	{
		printf("ERROR: Failed to get clGetEventProfilingInfo CL_PROFILING_COMMAND_END...\n");
		return false;
	}
	g_NDRangeTime1 = (cl_double)(end - start)*(cl_double)(1e-06);
	printf("Execution time: for #1 device is %.3f ms", g_NDRangeTime1);

	if(g_bUseBothDevices)
	{
		err = clGetEventProfilingInfo(g_events_list[1], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
		if (err != CL_SUCCESS)
		{
			printf("ERROR: Failed to get clGetEventProfilingInfo CL_PROFILING_COMMAND_START...\n");
			return false;
		}
		err = clGetEventProfilingInfo(g_events_list[1], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
		if (err != CL_SUCCESS)
		{
			printf("ERROR: Failed to get clGetEventProfilingInfo CL_PROFILING_COMMAND_END...\n");
			return false;
		}
		g_NDRangeTime2 = (cl_double)(end - start)*(cl_double)(1e-06);
		printf(", for the #2 device: %.3f ms \nThe frame was splitted as %.1f%% by %.1f%% between device #1 and #2", g_NDRangeTime2, g_NDRangeTimeRatioLast*100, (1-g_NDRangeTimeRatioLast)*100);
	}
	printf("\n");
	return true;
}

bool SubmitJobsToQueues(const cl_uint arrayWidth, const cl_uint arrayHeight, const CHDRData* pData)
{
	cl_uint arrayHeightDev1;
	cl_uint arrayHeightDev2;
    cl_int err = CL_SUCCESS;
	
	if(g_bUseBothDevices)
	{
		//compute splitting ratio for buffers
		ComputeSplittingRatio(arrayHeight, &arrayHeightDev1, &arrayHeightDev2);

		//create sub-resources (buffers)
		if(!PrepareResources(arrayWidth, &arrayHeightDev1, &arrayHeightDev2))
		{
			return false;
		}
	}

	//set kernel args for the first device (also in case it is the only device)
	err  = clSetKernelArg(g_kernel, 0, sizeof(cl_mem),  (void *) & (g_bUseBothDevices ? g_inputSubBuffer  : g_inputBuffer ) );
	err  |= clSetKernelArg(g_kernel, 1, sizeof(cl_mem), (void *) & (g_bUseBothDevices ? g_outputSubBuffer : g_outputBuffer ) );
	err  |= clSetKernelArg(g_kernel, 2, sizeof(cl_mem), (void *) &g_HDRDataBuffer);
	err  |= clSetKernelArg(g_kernel, 3, sizeof(cl_int), (void *) &arrayWidth);
	if (err != CL_SUCCESS)
	{
		printf("ERROR: Failed to set input g_kernel arguments...\n");
		return false;
	}

    size_t globalWorkSize[2];
	size_t globalWorkSize2[2];

    size_t localWorkSize[2]= {BLOCK_DIM, BLOCK_DIM};

	if(g_bUseBothDevices)
	{
		globalWorkSize[0] = arrayHeightDev1;
		globalWorkSize2[0] = arrayHeightDev2;
		globalWorkSize[1] = arrayWidth;
		globalWorkSize2[1] = arrayWidth;
		if(!g_bSilentMode)
		{
			printf("Global work size device#1    %d\n", globalWorkSize[0]);
			printf("Global work size device#2    %d\n", globalWorkSize2[0]);
			printf("Original local work size     %d\n", localWorkSize[0]);
		}
	}
	else
	{
		globalWorkSize[0] = g_globalWorkSize;
		globalWorkSize[1] = arrayWidth;
		if(!g_bSilentMode)
		{
			printf("Original global work size %d\n", globalWorkSize[0]);
			printf("Original local work size %d\n", localWorkSize[0]);
		}
	}

#ifdef PER_PIXEL
	cl_uint workDim = 2;
#else
	cl_uint workDim = 1;
#endif

    // submit kernel command for the first device
    if (CL_SUCCESS != clEnqueueNDRangeKernel(g_cmd_queue, g_kernel, workDim, NULL, globalWorkSize, localWorkSize, 0, NULL, &g_events_list[0]))
    {
        printf("ERROR: Failed to run kernel...\n");
        return false;
    }
	if(g_bUseBothDevices)
	{
	    // submit kernel command for the second device
		err  = clSetKernelArg(g_kernel, 0, sizeof(cl_mem), (void *) &g_inputSubBufferPG);
		err  |= clSetKernelArg(g_kernel, 1, sizeof(cl_mem), (void *) &g_outputSubBufferPG);
		err  |= clSetKernelArg(g_kernel, 2, sizeof(cl_mem), (void *) &g_HDRDataBuffer);
		err  |= clSetKernelArg(g_kernel, 3, sizeof(cl_int), (void *) &arrayWidth);


		if (err != CL_SUCCESS)
		{
			printf("ERROR: Failed to set input g_kernel arguments...\n");
			return false;
		}
		if (CL_SUCCESS != clEnqueueNDRangeKernel(g_cmd_queue_pg, g_kernel, workDim, NULL, globalWorkSize2, localWorkSize, 0, NULL, &g_events_list[1]))
		{
			printf("ERROR: Failed to run kernel...\n");
			return false;
		}
	}
	return true;
}

bool ExecuteToneMappingKernelSimple(cl_float* inputArray, cl_float* outputArray,  cl_float* tmpArray, const CHDRData *pData, cl_uint arrayWidth, cl_uint arrayHeight)
{
    cl_int err = CL_SUCCESS;
    // allocate the buffer
	//the same frame is processed
	g_inputBuffer = clCreateBuffer(g_context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(cl_float4) * arrayWidth * arrayHeight, inputArray, NULL);
	if (g_inputBuffer == (cl_mem)0)
	{
		printf("ERROR: Failed to create Input Buffer...\n");
		return false;
	}
		
	SubmitJobsToQueues(arrayWidth,arrayHeight, pData);
	if(g_bUseBothDevices)
	{
		//flush both queues to get things rolling in parallel
		err = clFlush(g_cmd_queue_pg);//let's flush PG's queue first (before CPU device occupies all the cores with its commands)
		err = clFlush(g_cmd_queue);
		//now let's wait
		err = clWaitForEvents (2, &g_events_list[0]);
	}
	else
	{
		//single-device case is easy
		err = clFinish(g_cmd_queue);
	}
	FrameExecutionStatistics();
	if(g_bUseBothDevices)
	{
		ReleaseSubResources();
	}

	clReleaseMemObject(g_inputBuffer); g_inputBuffer = NULL;
    return true;
}

bool ExecuteToneMappingKernelWithInputFrameUpdate(bool bSwapImageAsync /*emulating the new input being updated in parallel with processing current frame*/,
	cl_float* inputArray, cl_float* outputArray,  cl_float* tmpArray, const CHDRData *pData, cl_uint arrayWidth, cl_uint arrayHeight)
{
    cl_int err = CL_SUCCESS;
	//emulating the new frame (with juggling by 2 buffers and mirroring values)
	g_inputBuffer = (g_ImageSwapCounter%2) ? g_inputBuffer1 :g_inputBuffer2;//flip-flop of the current input buffer 
	SubmitJobsToQueues(arrayWidth,arrayHeight, pData);
	if(g_bUseBothDevices)
	{
		//flush both queues to get things rolling in parallel
		err  = clFlush(g_cmd_queue_pg);
		err |= clFlush(g_cmd_queue);
		//if we don't want to do smth on the host in paralell (e.g. no buffers to re-fill/swap), let's immediately wait
		if(!bSwapImageAsync)err |= clWaitForEvents (2, &g_events_list[0]);
		
	}
	else
	{
		//if we don't want to do smth on the host in paralell (e.g. no buffers to re-fill/swap), let's immediately wait
		if(!bSwapImageAsync) err = clFinish(g_cmd_queue);
	}
	if (err != CL_SUCCESS)
	{
		printf("ERROR: Failed to wait for results...\n");
		return false;
	}

	//let's do smth in the host thread
	//re-fill the input buffer with mirrored input image (just to simulate new input frame)
	if(g_ImageSwapCounter%2)
	{
		for(unsigned int iii = 0; iii < arrayHeight; iii++)
		{
			memcpy((void*)&inputArray[iii*arrayWidth*4], (void*)&tmpArray[(arrayHeight-1-iii)*arrayWidth*4], sizeof(cl_float4)*arrayWidth);
		}
	}
	else
	{
		for(unsigned int iii = 0; iii < arrayHeight; iii++)
		{
			memcpy((void*)&tmpArray[iii*arrayWidth*4], (void*)&inputArray[(arrayHeight-1-iii)*arrayWidth*4], sizeof(cl_float4)*arrayWidth);
		}
	}
	g_ImageSwapCounter++;
	
	if(bSwapImageAsync)
	{
		//now if we are in async mode let's wait for results (notice buffer swapping/re-filling and OCL commands were executing in parallel)
		err = g_bUseBothDevices ? clWaitForEvents (2, &g_events_list[0]) : clFinish(g_cmd_queue);
		if (err != CL_SUCCESS)
		{
			printf("ERROR: Failed to wait for results (async mode)...\n");
			return false;
		}

	}
	FrameExecutionStatistics();

	if(g_bUseBothDevices)
	{
		ReleaseSubResources();
	}

    return true;
}

//calculate FStops value (HDR parameter) from the arguments
float CalculateFStopsParameter( float powKLow, float kHigh )
{
    float curveBoxWidth = pow( 2.0f, kHigh ) - powKLow;
    float curveBoxHeight = pow( 2.0f, 3.5f )  - powKLow;

    // Initial boundary values
    float fFStopsLow = 0.0f;
    float fFStopsHigh = 100.0f;
    int iterations = 23; //interval bisection iterations

    // Interval bisection to find the final knee function fStops parameter 
    for ( int i = 0; i < iterations; i++ )
    {
        float fFStopsMiddle = ( fFStopsLow + fFStopsHigh ) * 0.5f;
        if ( ( curveBoxWidth * fFStopsMiddle + 1.0f ) < exp( curveBoxHeight * fFStopsMiddle ) )
        {
            fFStopsHigh = fFStopsMiddle;
        }
        else
        {
            fFStopsLow = fFStopsMiddle;
        }
    }

    return ( fFStopsLow + fFStopsHigh ) * 0.5f;
}


void Usage()
{
    printf("Usage: ToneMappingMultiDevice.exe [--h] [-g] [-m] [-i] [-s]\n");
    printf("  where, --h prints this message\n");
    printf("    -g run on Processor Graphics OCL device\n");
	printf("    -m run on both devices (shared context), CPU + PG\n");
	printf("    -i updating the input frame  on each iteration, and compares sync./async. update mode\n");
	printf("    -s prints per iteration statistics\n");
    exit(-1);
}

// main execution routine - perform Tone Mapping post-processing on float4 vectors
int _tmain(int argc, _TCHAR* argv[])
{
    cl_uint arrayWidth;
    cl_uint arrayHeight;

    //init HDR parameters
    float kLow = -3.0f;
    float kHigh = 7.5f;
    float exposure = 3.0f;
    float gamma = 1.0f;
    float defog = 0.0f;

    //parse command line
    int argn = 1;
    while (argn < argc)
    {
        if (_tcscmp(argv[argn], _T("--h")) == 0)
        {
            Usage();
        }
        else if (_tcscmp(argv[argn], _T("-g")) == 0)
        {
            g_bRunOnPG = true;
            argn++;
        }
		else if (_tcscmp(argv[argn], _T("-m")) == 0)
        {
            g_bUseBothDevices = true;
            g_bRunOnPG = false;
            argn++;
        }
		else if (_tcscmp(argv[argn], _T("-i")) == 0)
        {
            g_bSwapImage = true;
            argn++;
        }
		else if (_tcscmp(argv[argn], _T("-s")) == 0)
        {
			g_bSilentMode = false;
			argn++;
		}
		else
        {
            argn++;
        }
    }
    if( argc < 2 )
    {
        printf("No command line arguments specified, using default values.\n");
    }


    //fill HDR parameters structure
    CHDRData HDRData;
    HDRData.fGamma = gamma;
    HDRData.fPowGamma = pow(2.0f, -3.5f*gamma);
    HDRData.fDefog = defog;

    HDRData.fPowKLow = pow( 2.0f, kLow );
    HDRData.fPowKHigh = pow( 2.0f, kHigh );
    HDRData.fPow35 = pow(2.0f, 3.5f);
    HDRData.fPowExposure = pow( 2.0f, exposure +  2.47393f );

    //calculate FStops
    HDRData.fFStops = CalculateFStopsParameter(HDRData.fPowKLow, kHigh);
    printf("CalculateFStopsParameter result = %f\n", HDRData.fFStops);

    HDRData.fFStopsInv = 1.0f/HDRData.fFStops;

    
    //initialize Open CL objects (context, queue, etc.)
    if(!Setup_OpenCL("ToneMappingMultiDevice.cl") )
        return -1;

    
    //fill input frame
    cl_float* inputArray = 0;
    //read input image
    inputArray = readInput(&arrayWidth, &arrayHeight);
    if(inputArray==0)
        return -1;

    //fill tmp frame
    cl_float* tmpArray = 0;
	if(g_bSwapImage)
	{
		//read input image
		tmpArray = readInput(&arrayWidth, &arrayHeight);
		if(tmpArray==0)
			return -1;
	}


    printf("Input size is %d X %d\n", arrayWidth, arrayHeight);
    cl_float* outputArray = (cl_float*)_aligned_malloc(sizeof(cl_float4) * arrayWidth * arrayHeight, g_min_align);
    cl_float* refArray = (cl_float*)_aligned_malloc(sizeof(cl_float4) * arrayWidth * arrayHeight, g_min_align);

	//create buffers
	if(g_bSwapImage)//we need 2 buffers (to flip-flop them)
	{
		g_inputBuffer1 = clCreateBuffer(g_context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(cl_float4) * arrayWidth * arrayHeight, inputArray, NULL);
		if (g_inputBuffer1 == (cl_mem)0)
		{
			printf("ERROR: Failed to create Input Buffer...\n");
			return false;
		}
		g_inputBuffer2 = clCreateBuffer(g_context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(cl_float4) * arrayWidth * arrayHeight, tmpArray, NULL);
		if (g_inputBuffer2 == (cl_mem)0)
		{
			printf("ERROR: Failed to create Input Buffer #2...\n");
			return false;
		}
	}

    g_outputBuffer = clCreateBuffer(g_context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, sizeof(cl_float4) * arrayWidth * arrayHeight, outputArray, NULL);
    if (g_outputBuffer == (cl_mem)0)
    {
        printf("ERROR: Failed to create Output Buffer...\n");
        return false;
    }
	//this constant buffer with technique parameters (exposure, etc)
    g_HDRDataBuffer = clCreateBuffer(g_context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(CHDRData), &HDRData, NULL);
    if (g_HDRDataBuffer == (cl_mem)0)
    {
        printf("ERROR: Failed to create Output Buffer...\n");
        return false;
    }

	QueryPerformanceFrequency(&g_PerfFrequency);
	if(!g_bSwapImage)
	{//no emulation of input data update
		QueryPerformanceCounter(&g_PerformanceCountOverallStart);
		for(int iter = 0; iter < TEST_RUNS; iter++)
		{
			//do tone mapping
			if(!g_bSilentMode)
			{
				printf("Executing OpenCL kernel...\n");
			}
			ExecuteToneMappingKernelSimple(inputArray, outputArray, tmpArray, &HDRData, arrayWidth, arrayHeight);

		}
		QueryPerformanceCounter(&g_PerformanceCountOverallStop);
		printf("Overall execution time for %d frames is %.3f ms.\n", TEST_RUNS,
			1000.0f*(float)(g_PerformanceCountOverallStop.QuadPart - g_PerformanceCountOverallStart.QuadPart)/(float)g_PerfFrequency.QuadPart);
	}
	else
	{
		QueryPerformanceCounter(&g_PerformanceCountOverallStart);
		for(int iter = 0; iter < TEST_RUNS; iter++)
		{
			//do kernel execution and frame update in default (sync) mode
			if(!g_bSilentMode)
			{
				printf("Executing OpenCL kernel...\n");
			}
			ExecuteToneMappingKernelWithInputFrameUpdate(false, inputArray, outputArray, tmpArray, &HDRData, arrayWidth, arrayHeight);

		}
		QueryPerformanceCounter(&g_PerformanceCountOverallStop);
		float fSyncModeTimeInMs = 1000.0f*(float)(g_PerformanceCountOverallStop.QuadPart - g_PerformanceCountOverallStart.QuadPart)/(float)g_PerfFrequency.QuadPart;
		printf("Overall execution time for %d frames is %.3f ms (sync mode)\n", TEST_RUNS, fSyncModeTimeInMs);
		
		//now the same stuff in async way
		QueryPerformanceCounter(&g_PerformanceCountOverallStart);
		for(int iter = 0; iter < TEST_RUNS; iter++)
		{
			//do kernel execution and frame update in async  mode
			if(!g_bSilentMode)
			{
				printf("Executing OpenCL kernel...\n");
			}
			ExecuteToneMappingKernelWithInputFrameUpdate(true, inputArray, outputArray, tmpArray, &HDRData, arrayWidth, arrayHeight);

		}
		QueryPerformanceCounter(&g_PerformanceCountOverallStop);
		float fAsyncModeTimeInMs = 1000.0f*(float)(g_PerformanceCountOverallStop.QuadPart - g_PerformanceCountOverallStart.QuadPart)/(float)g_PerfFrequency.QuadPart;
		printf("Overall execution time for %d frames is %.3f ms (async mode)\n", TEST_RUNS, fAsyncModeTimeInMs);
		if(fSyncModeTimeInMs<fAsyncModeTimeInMs)
			printf("Async mode is nowhere faster than sync, probably we are already bandwidth-limited\n");
		else
			printf("Async mode is %.3f%% faster than sync\n", 100*(fSyncModeTimeInMs/fAsyncModeTimeInMs-1));
	}
	
	if(!g_bSilentMode)
	{
		printf("Executing reference...\n");
	}
	ExecuteToneMappingReference(inputArray, refArray, &HDRData, arrayWidth, arrayHeight);

	//doing map/unmap to sync the memory content with the host mem pointed by outputArray (this is required by spec)
	void* tmp_ptr = NULL;
	tmp_ptr = clEnqueueMapBuffer(g_cmd_queue, g_outputBuffer, true, CL_MAP_READ, 0, sizeof(cl_float4) * arrayWidth * arrayHeight , 0, NULL, NULL, NULL);
	if(tmp_ptr!=outputArray)
	{
		printf("ERROR: clEnqueueMapBuffer failed to return original pointer\n");//since we used CL_USE_HOST_PTR we want to operate on the same mem not copy
		return false;
	}
	clEnqueueUnmapMemObject(g_cmd_queue, g_outputBuffer, tmp_ptr, 0, NULL, NULL);
	//save results in bitmap files
	if(!g_bSilentMode)
	{
		float fTmpFVal = 0.0f;
		cl_uint* outUIntBuf=0;
		outUIntBuf = (cl_uint*)malloc(arrayWidth*arrayHeight*sizeof(cl_uint));
		if(!outUIntBuf)
		{
			printf("Failed to allocate memory for output BMP image!\n");
			return -1;
		}
		for(unsigned int y = 0; y < arrayHeight; y++)
		{
			for(unsigned int x = 0; x < arrayWidth; x++)
			{
				cl_uint uiTmp[4]; // 4 - means 4-channel pixel
				fTmpFVal = (outputArray[(y*arrayWidth+x)*4+0]);
				uiTmp[0] = (cl_uint)(fTmpFVal);

				fTmpFVal = (outputArray[(y*arrayWidth+x)*4+1]);
				uiTmp[1] = (cl_uint)(fTmpFVal);

				fTmpFVal = (outputArray[(y*arrayWidth+x)*4+2]);
				uiTmp[2] = (cl_uint)(fTmpFVal);

				fTmpFVal = (outputArray[(y*arrayWidth+x)*4+3]);
				uiTmp[3] = (cl_uint)(fTmpFVal);	//Alfa

				outUIntBuf[(arrayHeight-1-y)*arrayWidth+x] = 0x000000FF & uiTmp[2];
				outUIntBuf[(arrayHeight-1-y)*arrayWidth+x] |= 0x0000FF00 & ((uiTmp[1]) << 8);
				outUIntBuf[(arrayHeight-1-y)*arrayWidth+x] |= 0x00FF0000 & ((uiTmp[0]) << 16);
				outUIntBuf[(arrayHeight-1-y)*arrayWidth+x] |= 0xFF000000 & ((uiTmp[3]) << 24);
			}
		}	
		//----
		SaveImageAsBMP( outUIntBuf, arrayWidth, arrayHeight, "ToneMappingMultiDeviceOutput.bmp");

		for(unsigned int y = 0; y < arrayHeight; y++)
		{
			for(unsigned int x = 0; x < arrayWidth; x++)
			{
				cl_uint uiTmp[4]; // 4 - means 4-channel pixel
				fTmpFVal = (refArray[(y*arrayWidth+x)*4+0]);
				uiTmp[0] = (cl_uint)(fTmpFVal);

				fTmpFVal = (refArray[(y*arrayWidth+x)*4+1]);
				uiTmp[1] = (cl_uint)(fTmpFVal);

				fTmpFVal = (refArray[(y*arrayWidth+x)*4+2]);
				uiTmp[2] = (cl_uint)(fTmpFVal);

				fTmpFVal = (refArray[(y*arrayWidth+x)*4+3]);
				uiTmp[3] = (cl_uint)(fTmpFVal);	//Alfa

				outUIntBuf[(arrayHeight-1-y)*arrayWidth+x] = 0x000000FF & uiTmp[2];
				outUIntBuf[(arrayHeight-1-y)*arrayWidth+x] |= 0x0000FF00 & ((uiTmp[1]) << 8);
				outUIntBuf[(arrayHeight-1-y)*arrayWidth+x] |= 0x00FF0000 & ((uiTmp[0]) << 16);
				outUIntBuf[(arrayHeight-1-y)*arrayWidth+x] |= 0xFF000000 & ((uiTmp[3]) << 24);
			}
		}	
		//----
		SaveImageAsBMP( outUIntBuf, arrayWidth, arrayHeight, "ToneMappingMultiDeviceOutputReference.bmp");
		free(outUIntBuf);
	}

	//Do verification
	if(!g_bSilentMode)
	{
		printf("Performing verification...\n");
	}

	bool result = true;
	for(unsigned int i = 0; i < arrayWidth*arrayHeight*4; i++)	// 4 - means 4-channel pixel
	{
		//Compare the data
		if( fabsf(outputArray[i] - refArray[i]) > 0.01f )	
		{
			printf("Error at location %d,  outputArray = %f, refArray = %f \n", i, outputArray[i], refArray[i]);
			result = false;
		}
	}
	if(!result)
	{
		printf("ERROR: Verification failed.\n");
	}
	else
	{
		if(!g_bSilentMode)
		{
			printf("Verification succeeded.\n");
		}
	}

    _aligned_free( refArray );
    _aligned_free( inputArray );
    _aligned_free( outputArray );
	if(g_bSwapImage)
	{
		_aligned_free( tmpArray );
	}

    Cleanup_OpenCL();

    return 0;
}

#pragma warning( pop )
