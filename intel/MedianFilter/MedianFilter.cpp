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
#include "CL\cl.h"
#include "utils.h"

#define LOCAL_SIZE_X 8
#define LOCAL_SIZE_Y 8

cl_mem		g_inputBuffer = NULL;
cl_mem		g_outputBuffer = NULL;
cl_context	g_context = NULL;
cl_command_queue g_cmd_queue = NULL;
cl_program	g_program = NULL;
cl_kernel	g_kernel = NULL;
cl_int      g_min_align = 0;

bool g_bRunOnPG = false;

//for perf. counters
#include <Windows.h>

LARGE_INTEGER g_PerfFrequency;
LARGE_INTEGER g_PerformanceCountNDRangeStart;
LARGE_INTEGER g_PerformanceCountNDRangeStop;


void Cleanup_OpenCL()
{
    if( g_inputBuffer ) {clReleaseMemObject( g_inputBuffer ); g_inputBuffer = NULL;}
    if( g_outputBuffer ) {clReleaseMemObject( g_outputBuffer ); g_outputBuffer = NULL;}
    if( g_kernel ) {clReleaseKernel( g_kernel ); g_kernel = NULL;}
    if( g_program ) {clReleaseProgram( g_program ); g_program = NULL;}
    if( g_cmd_queue ) {clReleaseCommandQueue( g_cmd_queue ); g_cmd_queue = NULL;}
    if( g_context ) {clReleaseContext( g_context ); g_context = NULL;}
}

int Setup_OpenCL( const char *program_source)
{
    cl_device_id devices[16];
    size_t cb;
    cl_uint size_ret = 0;
    cl_int err;
    int num_cores;
    cl_device_id device_ID;
    char device_name[128] = {0};


	if(g_bRunOnPG)
	{
		printf("Trying to run on a Processor Graphics \n");
	}
	else
	{
		printf("Trying to run on a CPU \n");
	}

	cl_platform_id intel_platform_id = GetIntelOCLPlatform();
    if( intel_platform_id == NULL )
    {
        printf("ERROR: Failed to find Intel OpenCL platform.\n");
        return -1;
    }

    cl_context_properties context_properties[3] = {CL_CONTEXT_PLATFORM, (cl_context_properties)intel_platform_id, NULL };

    // create the OpenCL context on a CPU/PG 
	if(g_bRunOnPG)
	{
		g_context = clCreateContextFromType(context_properties, CL_DEVICE_TYPE_GPU, NULL, NULL, NULL);
	}
	else
	{
		g_context = clCreateContextFromType(context_properties, CL_DEVICE_TYPE_CPU, NULL, NULL, NULL);
	}
    if (g_context == (cl_context)0)
        return -1;

    // get the list of CPU devices associated with context
    err = clGetContextInfo(g_context, CL_CONTEXT_DEVICES, 0, NULL, &cb);
    clGetContextInfo(g_context, CL_CONTEXT_DEVICES, cb, devices, NULL);

    g_cmd_queue = clCreateCommandQueue(g_context, devices[0], 0, NULL);
    if (g_cmd_queue == (cl_command_queue)0)
    {
        Cleanup_OpenCL();
        return -1;
    }

    char *sources = ReadSources(program_source);	//read program .cl source file
    g_program = clCreateProgramWithSource(g_context, 1, (const char**)&sources, NULL, NULL);
    if (g_program == (cl_program)0)
    {
        printf("ERROR: Failed to create Program with source...\n");
        Cleanup_OpenCL();
        free(sources);
        return -1;
    }

    err = clBuildProgram(g_program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        printf("ERROR: Failed to build program...\n");
        BuildFailLog(g_program, devices[0]);
        Cleanup_OpenCL();
        free(sources);
        return -1;
    }

    g_kernel = clCreateKernel(g_program, "MedianFilterBitonic", NULL);
    if (g_kernel == (cl_kernel)0)
    {
        printf("ERROR: Failed to create kernel...\n");
        Cleanup_OpenCL();
        free(sources);
        return -1;
    }
    free(sources);

    // retrieve device information

    // use first device ID
    device_ID = devices[0];

    err = clGetDeviceInfo(device_ID, CL_DEVICE_NAME, 128, device_name, NULL);
    if (err!=CL_SUCCESS)
    {
        printf("ERROR: Failed to get device information (device name)...\n");
        Cleanup_OpenCL();
        return -1;
    }
    printf("Using device %s...\n", device_name);

    err = clGetDeviceInfo(device_ID, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &num_cores, NULL);
    if (err!=CL_SUCCESS)
    {
        printf("ERROR: Failed to get device information (max compute units)...\n");
        Cleanup_OpenCL();
        return -1;
    }
    printf("Using %d compute units...\n", num_cores);


    err = clGetDeviceInfo(device_ID, CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(cl_uint), &g_min_align, NULL);
    if (err!=CL_SUCCESS)
    {
        printf("ERROR: Failed to get device information (max memory base address align size)...\n");
        Cleanup_OpenCL();
        return -1;
    }
    g_min_align /= 8; //in bytes
    printf("Expected min alignment for buffers is %d bytes...\n", g_min_align);

    return 0; // success...
}

void generateInput(cl_uint* inputArray, size_t arrayWidth, size_t arrayHeight)
{

    srand(12345);

    // random initialization of input
    for (cl_uint i = 0; i <  arrayWidth * (arrayHeight+4); ++i)
    {
        inputArray[i] = (rand() | (rand()<<15) ) & 0xFFFFFF;
    }

    SaveImageAsBMP( inputArray, (int)arrayWidth, (int)arrayHeight, "MedianFilterInput.bmp");
}

//inline functions for reference kernel
typedef unsigned char (uchar4)[4];

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

__forceinline unsigned c4max(const unsigned& l, const unsigned& r)
{
    unsigned ur;
    unsigned char* res = (unsigned char*)&ur;
    unsigned char* left = (unsigned char*)&l;
    unsigned char* right = (unsigned char*)&r;

    res[0] = max(left[0],right[0]);
    res[1] = max(left[1],right[1]);
    res[2] = max(left[2],right[2]);
    res[3] = max(left[3],right[3]);

    return ur;
};

__forceinline unsigned c4min(const unsigned& l, const unsigned& r)
{
    unsigned ur;
    unsigned char* res = (unsigned char*)&ur;
    unsigned char* left = (unsigned char*)&l;
    unsigned char* right = (unsigned char*)&r;

    res[0] = min(left[0],right[0]);
    res[1] = min(left[1],right[1]);
    res[2] = min(left[2],right[2]);
    res[3] = min(left[3],right[3]);

    return ur;
};

void ExecuteMedianFilterReference(cl_uint* inputArray, cl_uint* outputArray, cl_int arrayWidth, cl_uint arrayHeight)
{
    memset(outputArray, 0,   arrayWidth * (arrayHeight+4));

    // do the Median 
    for(cl_uint y = 0; y < arrayHeight; y++)		// rows loop
    {
        int iOffset = (y+2) * arrayWidth;
        int iPrev = iOffset - arrayWidth;
        int iNext = iOffset + arrayWidth;

        for(int x = 0; x < arrayWidth; x++)		// columns loop
        {
            unsigned uiRGBA[9];	

            //get pixels within aperture
            uiRGBA[0] = inputArray[iPrev + x - 1];
            uiRGBA[1] = inputArray[iPrev + x];
            uiRGBA[2] = inputArray[iPrev + x + 1];

            uiRGBA[3] = inputArray[iOffset + x - 1];
            uiRGBA[4] = inputArray[iOffset + x];
            uiRGBA[5] = inputArray[iOffset + x + 1];

            uiRGBA[6] = inputArray[iNext + x - 1];
            uiRGBA[7] = inputArray[iNext + x];
            uiRGBA[8] = inputArray[iNext + x + 1];

            unsigned uiMin = c4min(uiRGBA[0], uiRGBA[1]);
            unsigned uiMax = c4max(uiRGBA[0], uiRGBA[1]);
            uiRGBA[0] = uiMin;
            uiRGBA[1] = uiMax;

            uiMin = c4min(uiRGBA[3], uiRGBA[2]);
            uiMax = c4max(uiRGBA[3], uiRGBA[2]);
            uiRGBA[3] = uiMin;
            uiRGBA[2] = uiMax;

            uiMin = c4min(uiRGBA[2], uiRGBA[0]);
            uiMax = c4max(uiRGBA[2], uiRGBA[0]);
            uiRGBA[2] = uiMin;
            uiRGBA[0] = uiMax;

            uiMin = c4min(uiRGBA[3], uiRGBA[1]);
            uiMax = c4max(uiRGBA[3], uiRGBA[1]);
            uiRGBA[3] = uiMin;
            uiRGBA[1] = uiMax;

            uiMin = c4min(uiRGBA[1], uiRGBA[0]);
            uiMax = c4max(uiRGBA[1], uiRGBA[0]);
            uiRGBA[1] = uiMin;
            uiRGBA[0] = uiMax;

            uiMin = c4min(uiRGBA[3], uiRGBA[2]);
            uiMax = c4max(uiRGBA[3], uiRGBA[2]);
            uiRGBA[3] = uiMin;
            uiRGBA[2] = uiMax;

            uiMin = c4min(uiRGBA[5], uiRGBA[4]);
            uiMax = c4max(uiRGBA[5], uiRGBA[4]);
            uiRGBA[5] = uiMin;
            uiRGBA[4] = uiMax;

            uiMin = c4min(uiRGBA[7], uiRGBA[8]);
            uiMax = c4max(uiRGBA[7], uiRGBA[8]);
            uiRGBA[7] = uiMin;
            uiRGBA[8] = uiMax;

            uiMin = c4min(uiRGBA[6], uiRGBA[8]);
            uiMax = c4max(uiRGBA[6], uiRGBA[8]);
            uiRGBA[6] = uiMin;
            uiRGBA[8] = uiMax;

            uiMin = c4min(uiRGBA[6], uiRGBA[7]);
            uiMax = c4max(uiRGBA[6], uiRGBA[7]);
            uiRGBA[6] = uiMin;
            uiRGBA[7] = uiMax;

            uiMin = c4min(uiRGBA[4], uiRGBA[8]);
            uiMax = c4max(uiRGBA[4], uiRGBA[8]);
            uiRGBA[4] = uiMin;
            uiRGBA[8] = uiMax;

            uiMin = c4min(uiRGBA[4], uiRGBA[6]);
            uiMax = c4max(uiRGBA[4], uiRGBA[6]);
            uiRGBA[4] = uiMin;
            uiRGBA[6] = uiMax;

            uiMin = c4min(uiRGBA[5], uiRGBA[7]);
            uiMax = c4max(uiRGBA[5], uiRGBA[7]);
            uiRGBA[5] = uiMin;
            uiRGBA[7] = uiMax;

            uiMin = c4min(uiRGBA[4], uiRGBA[5]);
            uiMax = c4max(uiRGBA[4], uiRGBA[5]);
            uiRGBA[4] = uiMin;
            uiRGBA[5] = uiMax;

            uiMin = c4min(uiRGBA[6], uiRGBA[7]);
            uiMax = c4max(uiRGBA[6], uiRGBA[7]);
            uiRGBA[6] = uiMin;
            uiRGBA[7] = uiMax;

            uiMin = c4min(uiRGBA[0], uiRGBA[8]);
            uiMax = c4max(uiRGBA[0], uiRGBA[8]);
            uiRGBA[0] = uiMin;
            uiRGBA[8] = uiMax;

            uiRGBA[4] = c4max(uiRGBA[0], uiRGBA[4]);
            uiRGBA[5] = c4max(uiRGBA[1], uiRGBA[5]);

            uiRGBA[6] = c4max(uiRGBA[2], uiRGBA[6]);
            uiRGBA[7] = c4max(uiRGBA[3], uiRGBA[7]);

            uiRGBA[4] = c4min(uiRGBA[4], uiRGBA[6]);
            uiRGBA[5] = c4min(uiRGBA[5], uiRGBA[7]);

            // convert and copy to output
            outputArray[(y+2) * arrayWidth + x] = c4min(uiRGBA[4], uiRGBA[5]);
        }
    } 
}

bool ExecuteMedianFilterKernel(cl_uint* inputArray, cl_uint* outputArray, cl_int arrayWidth, cl_uint arrayHeight)
{
    cl_int err = CL_SUCCESS;
    cl_uint numStages = 0;

    // allocate the buffer with some padding (to avoid boundaries checking)
    g_inputBuffer = clCreateBuffer(g_context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(cl_uint) * arrayWidth * (arrayHeight+4), inputArray, NULL);

    if (g_inputBuffer == (cl_mem)0)
    {
        printf("ERROR: Failed to create Input Buffer...\n");
        return false;
    }

    // allocate the buffer with some padding (to avoid boundaries checking)
    g_outputBuffer = clCreateBuffer(g_context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, sizeof(cl_uint) * arrayWidth * (arrayHeight+4), outputArray, NULL);

    if (g_outputBuffer == (cl_mem)0)
    {
        printf("ERROR: Failed to create Output Buffer...\n");
        return false;
    }


    err  =  clSetKernelArg(g_kernel, 0, sizeof(cl_mem), (void *) &g_inputBuffer);
    err  |= clSetKernelArg(g_kernel, 1, sizeof(cl_mem), (void *) &g_outputBuffer);
    if (err != CL_SUCCESS)
    {
        printf("ERROR: Failed to set input g_kernel arguments...\n");
        return false;
    }


	size_t global_work_size[2] = { arrayWidth, arrayHeight};
    size_t local_work_size[2]= { LOCAL_SIZE_X, LOCAL_SIZE_Y};					
	size_t offset[2]= { 0, 2};					

    QueryPerformanceCounter(&g_PerformanceCountNDRangeStart);
    // execute kernel
    if (CL_SUCCESS != (err = clEnqueueNDRangeKernel(g_cmd_queue, g_kernel, 2, offset, global_work_size, local_work_size, 0, NULL, NULL)))
    {
        printf("ERROR: Failed to run kernel...\n");
        return false;
    }
    err = clFinish(g_cmd_queue);
    QueryPerformanceCounter(&g_PerformanceCountNDRangeStop);

	void* tmp_ptr = NULL;
    tmp_ptr = clEnqueueMapBuffer(g_cmd_queue, g_outputBuffer, true, CL_MAP_READ, 0, sizeof(cl_uint) * arrayWidth * (arrayHeight+4), 0, NULL, NULL, NULL);
	if(tmp_ptr!=outputArray)
	{
		printf("ERROR: clEnqueueMapBuffer failed to return original pointer\n");
		return false;
	}


    err = clFinish(g_cmd_queue);

    clEnqueueUnmapMemObject(g_cmd_queue, g_outputBuffer, tmp_ptr, 0, NULL, NULL);

    clReleaseMemObject(g_inputBuffer); g_inputBuffer = NULL;
    clReleaseMemObject(g_outputBuffer); g_outputBuffer = NULL;

    return true;
}

void Usage()
{
    printf("Usage: MedianFilter.exe [--h] [-h <height>] [-w <width>] [-g]\n");
    printf("  where, --h prints this message\n");
    printf("    <height> and <width> are input dimensions\n");
    printf("    -g run on Processor Graphics\n");
    exit(-1);
}

// main execution routine - performs median filtering with 3x3 kernel size
int _tmain(int argc, TCHAR* argv[])
{
    cl_int arrayWidth = 1024;
    cl_int arrayHeight = 1024;

    int argn = 1;
    while (argn < argc)
    {
        if (_tcscmp(argv[argn], _T("--h")) == 0)
        {
            Usage();
        }
        else if (_tcscmp(argv[argn], _T("-w")) == 0)
        {
            if(++argn==argc)
                Usage();
            arrayWidth = _ttoi(argv[argn]);
            argn++;
        }
        else if (_tcscmp(argv[argn], _T("-h")) == 0)
        {
            if(++argn==argc)
                Usage();
            arrayHeight = _ttoi(argv[argn]);
            argn++;
        }
        else if (_tcscmp(argv[argn], _T("-g")) == 0)
        {
            g_bRunOnPG = true;
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

    //validate user input parameters
    {
        if( arrayWidth < 4 || arrayHeight < 4 || arrayWidth > 8192 || arrayHeight > 8192 )
        {
            printf("Input size in each dimension should be in the range [4, 8192]!\n");
            return -1;
        }

        cl_uint _logb = 0;

        for (int temp = arrayWidth; temp>1; temp >>= 1)
            _logb++;

        if( arrayWidth & ((1 << _logb)-1)  )
        {
            printf("Input size should be (2^N)!\n");
            return -1;
        }

        _logb = 0;

        for (int temp = arrayHeight; temp>1; temp >>= 1)
            _logb++;

        if( arrayHeight & ((1 << _logb)-1)  )
        {
            printf("Input size should be (2^N)!\n");
            return -1;
        }

    }

    //initialize Open CL objects (context, queue, etc.)
    if( 0 != Setup_OpenCL("MedianFilter.cl") )
        return -1;

    printf("Input size is %d X %d\n", arrayWidth, arrayHeight);

    // allocate buffers with some padding (to avoid boundaries checking)
    cl_uint* inputArray = (cl_uint*)_aligned_malloc(sizeof(cl_uint) * arrayWidth * (arrayHeight+4), g_min_align);
    cl_uint* outputArray = (cl_uint*)_aligned_malloc(sizeof(cl_uint) * arrayWidth * (arrayHeight+4), g_min_align);
    cl_uint* refArray = (cl_uint*)_aligned_malloc(sizeof(cl_uint) * arrayWidth * (arrayHeight+4), g_min_align);

    //random input
    generateInput(inputArray, arrayWidth, arrayHeight);

    //median filtering
    printf("Executing OpenCL kernel...\n");
    ExecuteMedianFilterKernel(inputArray, outputArray, arrayWidth, arrayHeight);
    SaveImageAsBMP( outputArray + 2*arrayWidth, arrayWidth, arrayHeight, "MedianFilterOutput.bmp");

    printf("Executing reference...\n");
    ExecuteMedianFilterReference(inputArray, refArray, arrayWidth, arrayHeight);
    SaveImageAsBMP( refArray + 2*arrayWidth, arrayWidth, arrayHeight, "MedianFilterOutputReference.bmp");

    printf("Performing verification...\n");
    bool result = true;
    for(cl_uint y = 0; y < (cl_uint)arrayHeight; y++)		// rows loop
    {
        for(cl_uint x = 0; x < (cl_uint)arrayWidth; x++)		// columns loop
        {
            unsigned int REF = (refArray+2*arrayWidth)[y * arrayWidth + x] & 0xFFFFFF;
            unsigned int RES = (outputArray+2*arrayWidth)[y * arrayWidth + x] & 0xFFFFFF;

            unsigned int REFup = REF + 0x010101;
            unsigned int REFdown = REF - 0x010101;
            if( (RES < REFdown) || (RES > REFup) )
            {
                printf("y = %d, x = %d, REF = %x %x %x, RES = %x %x %x\n",
                    y, x,
                    (REF & 0xFF),
                    (REF & 0xFF00) >> 8,
                    (REF & 0xFF0000) >> 16,

                    (RES & 0xFF),
                    (RES & 0xFF00) >> 8,
                    (RES & 0xFF0000) >> 16 );
                result = false;
            }
        }           
    }
    if(!result)
    {
        printf("ERROR: Verification failed.\n");
    }
    else
    {
        printf("Verification succeeded.\n");
    }

    //retrieve perf. counter frequency
    QueryPerformanceFrequency(&g_PerfFrequency);
    printf("NDRange perf. counter time %f ms.\n", 
        1000.0f*(float)(g_PerformanceCountNDRangeStop.QuadPart - g_PerformanceCountNDRangeStart.QuadPart)/(float)g_PerfFrequency.QuadPart);


    _aligned_free( refArray );
    _aligned_free( inputArray );
    _aligned_free( outputArray );

    Cleanup_OpenCL();

    return 0;
}

