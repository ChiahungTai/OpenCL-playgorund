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

cl_mem		g_inputBuffer = NULL;
cl_context	g_context = NULL;
cl_command_queue g_cmd_queue = NULL;
cl_program	g_program = NULL;
cl_kernel	g_kernel = NULL;

bool g_bRunOnPG = false;

//for perf. counters
#include <Windows.h>

LARGE_INTEGER g_PerfFrequency;
LARGE_INTEGER g_PerformanceCountNDRangeStart;
LARGE_INTEGER g_PerformanceCountNDRangeStop;


void Cleanup_OpenCL()
{
    if( g_inputBuffer ) {clReleaseMemObject( g_inputBuffer ); g_inputBuffer = NULL;}
    if( g_kernel ) {clReleaseKernel( g_kernel ); g_kernel = NULL;}
    if( g_program ) {clReleaseProgram( g_program ); g_program = NULL;}
    if( g_cmd_queue ) {clReleaseCommandQueue( g_cmd_queue ); g_cmd_queue = NULL;}
    if( g_context ) {clReleaseContext( g_context ); g_context = NULL;}
}

int Setup_OpenCL( const char *program_source, cl_uint* alignment)
{
    cl_device_id devices[16];
    size_t cb;
    cl_uint size_ret = 0;
    cl_int err;

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

    if( alignment )
    {
        err = clGetDeviceInfo (devices[0],
            CL_DEVICE_MEM_BASE_ADDR_ALIGN ,
            sizeof(cl_uint),
            alignment,
            NULL);

        *alignment/=8; //in bytes
        printf("OpenCL data alignment is %d bytes.\n", *alignment);
    }

    g_cmd_queue = clCreateCommandQueue(g_context, devices[0], 0, NULL);
    if (g_cmd_queue == (cl_command_queue)0)
    {
        Cleanup_OpenCL();
        return -1;
    }

    char *sources = ReadSources(program_source);	//read program .cl source file
	if( NULL == sources )
	{
        printf("ERROR: Failed to read sources into memory...\n");
        Cleanup_OpenCL();
        return -1;
    }

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

    g_kernel = clCreateKernel(g_program, "BitonicSort", NULL);
    if (g_kernel == (cl_kernel)0)
    {
        printf("ERROR: Failed to create kernel...\n");
        Cleanup_OpenCL();
        free(sources);
        return -1;
    }
    free(sources);

    return 0; // success...
}

void generateInput(cl_int* inputArray, size_t arraySize)
{
    const size_t minElement = 0;
    const size_t maxElement = arraySize + 1;

    srand(12345);

    // random initialization of input
    for (size_t i = 0; i < arraySize; ++i)
    {
        inputArray[i] = (cl_int)((float) (maxElement - minElement) * (rand() / (float) RAND_MAX));
    }
}

void ExecuteSortReference(cl_int* inputArray, cl_int arraySize, cl_bool sortAscending)
{
    cl_int numStages = 0;
    cl_uint temp;

    cl_int stage;
    cl_int passOfStage;

    for (temp = arraySize; temp > 1; temp >>= 1)
    {
        ++numStages;
    }

    for (stage = 0; stage < numStages; ++stage)
    {
        int dirMask = 1 << stage;

        for (passOfStage = 0; passOfStage < stage + 1; ++passOfStage)
        {
            const cl_uint shift = stage - passOfStage;
            const cl_uint distance = 1 << shift;
            const cl_uint lmask = distance - 1;

            for(int g_id=0; g_id < arraySize >> 1; g_id++)
            {
                cl_uint leftId = (( g_id >> shift ) << (shift + 1)) + (g_id & lmask);
                cl_uint rightId = leftId + distance;

                cl_uint left  = inputArray[leftId];
                cl_uint right = inputArray[rightId];

                cl_uint greater;
                cl_uint lesser;

                if(left > right)
                {
                    greater = left;
                    lesser  = right;
                }
                else
                {
                    greater = right;
                    lesser  = left;
                }

                cl_bool dir = sortAscending;
                if( ( g_id & dirMask) == dirMask )
                    dir = !dir;

                if(dir)
                {
                    inputArray[leftId]  = lesser;
                    inputArray[rightId] = greater;
                }
                else
                {
                    inputArray[leftId]  = greater;
                    inputArray[rightId] = lesser;
                }
            }
        }
    }
}

bool ExecuteSortKernel(cl_int* inputArray, cl_int arraySize, cl_uint sortAscending)
{
    cl_int err = CL_SUCCESS;
    cl_int numStages = 0;
    cl_uint temp;

    cl_int stage;
    cl_int passOfStage;

    //create OpenCL buffer using input array memory
    g_inputBuffer = clCreateBuffer(g_context, CL_MEM_USE_HOST_PTR, sizeof(cl_int) * arraySize, inputArray, NULL);

    if (g_inputBuffer == (cl_mem)0)
    {
        printf("ERROR: Failed to create input data Buffer\n");
        return false;
    }

    for (temp = arraySize; temp > 2; temp >>= 1)
    {
        numStages++;
    }

    err  = clSetKernelArg(g_kernel, 0, sizeof(cl_mem), (void *) &g_inputBuffer);
    err |= clSetKernelArg(g_kernel, 3, sizeof(cl_uint), (void *) &sortAscending);
    if (err != CL_SUCCESS)
    {
        printf("ERROR: Failed to set input kernel arguments\n");
        return false;
    }

    QueryPerformanceCounter(&g_PerformanceCountNDRangeStart);
    for (stage = 0; stage < numStages; stage++)
    {
        if ( CL_SUCCESS != clSetKernelArg(g_kernel, 1, sizeof(cl_uint), (void *) &stage) )
            return false;

        for (passOfStage = stage; passOfStage >= 0; passOfStage--)
        {
            if ( CL_SUCCESS != clSetKernelArg(g_kernel, 2, sizeof(cl_uint), (void *) &passOfStage) )
                return false;

            // set work-item dimensions
            size_t gsz = arraySize / (2*4);
            size_t global_work_size[1] = { passOfStage ? gsz : gsz << 1 };	//number of quad items in input array
            size_t local_work_size[1]= { 128 };					//valid WG sizes are 1:1024

            // execute kernel
            if (CL_SUCCESS != clEnqueueNDRangeKernel(g_cmd_queue, g_kernel, 1, NULL, global_work_size, local_work_size, 0, NULL, NULL))
            {
                printf("ERROR: Failed to execute sorting kernel\n");
                return false;
            }
        }
    }

    err = clFinish(g_cmd_queue);
    QueryPerformanceCounter(&g_PerformanceCountNDRangeStop);

    void* tmp_ptr = NULL;
    tmp_ptr = clEnqueueMapBuffer(g_cmd_queue, g_inputBuffer, true, CL_MAP_READ, 0, sizeof(cl_int) * arraySize , 0, NULL, NULL, NULL);
	if(tmp_ptr!=inputArray)
	{
		printf("ERROR: clEnqueueMapBuffer failed to return original pointer\n");
		return false;
	}


    err = clFinish(g_cmd_queue);

    clEnqueueUnmapMemObject(g_cmd_queue, g_inputBuffer, tmp_ptr, 0, NULL, NULL);

    clReleaseMemObject(g_inputBuffer); g_inputBuffer = NULL;

    return true;
}

void Usage()
{
    printf("Usage: BitonicSort.exe [--h] [-s <arraySize>] [-d]\n");
    printf("  where, --h prints this message\n");
    printf("    <arraySize> is input/output array size\n");
    printf("    -d performs descending sort (default is ascending)\n");
    printf("    -g run on Processor Graphics\n");
    exit(-1);
}

int _tmain(int argc, _TCHAR* argv[])
{
    cl_uint dev_alignment = 128;
    cl_bool sortAscending = true;
    cl_int arraySize = (1 << 20);

    int argn = 1;
    while (argn < argc)
    {
        if (_tcscmp(argv[argn], _T("--h")) == 0)
        {
            Usage();
        }
        else if (_tcscmp(argv[argn], _T("-s")) == 0)
        {
            if(++argn==argc)
                Usage();
            arraySize = _ttoi(argv[argn]);
            argn++;
        }
        else if (_tcscmp(argv[argn], _T("-d")) == 0)
        {
            sortAscending = false;
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

    //validate user input parameters, if any
    {
        cl_uint sz_log_b = 0;

        if( arraySize < 1024 )
        {
            printf("Input size should be no less than 1024!\n");
            return -1;
        }

        for (int temp = arraySize; temp>1; temp >>= 1)
            sz_log_b++;

        if( arraySize & ((1 << sz_log_b)-1)  )
        {
            printf("Input size should be (2^N)*4!\n");
            return -1;
        }
    }

    printf("Initializing OpenCL runtime...\n");

    //initialize Open CL objects (context, queue, etc.)
    if( 0 != Setup_OpenCL("BitonicSort.cl", &dev_alignment) )
        return -1;

    printf("Sort order is %s\n", sortAscending ? "ascending" : "descending" );
    printf("Input size is %d items\n", arraySize);
    cl_int* inputArray = (cl_int*)_aligned_malloc(sizeof(cl_int) * arraySize, dev_alignment);
    cl_int* refArray = (cl_int*)_aligned_malloc(sizeof(cl_int) * arraySize, dev_alignment);

    //random input
    generateInput(inputArray, arraySize);
    memcpy(refArray, inputArray, sizeof(cl_int) * arraySize);

    //sort input array in a given direction
    printf("Executing OpenCL kernel...\n");
    ExecuteSortKernel(inputArray, arraySize, sortAscending);

    printf("Executing reference...\n");
    ExecuteSortReference(refArray, arraySize, sortAscending);

    bool result = true;
    printf("Performing verification...\n");
    for (cl_int i = 0; i < arraySize; i++)
    {
        if (inputArray[i] != refArray[i])
        {
            result = false;
            break;
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

    printf("Releasing resources...\n");
    _aligned_free( refArray );
    _aligned_free( inputArray );

    Cleanup_OpenCL();

    return 0;
}

