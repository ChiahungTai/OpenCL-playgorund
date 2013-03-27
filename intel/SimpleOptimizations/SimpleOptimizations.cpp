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

//for perf. counters
#include <Windows.h>

//we want to use POSIX functions
#pragma warning( push )
#pragma warning( disable : 4996 )

// OpenCL specific
cl_context	g_context = NULL;
cl_command_queue g_cmd_queue = NULL;
cl_program	g_program = NULL;
cl_kernel	g_kernel = NULL;
cl_kernel	g_kernel4 = NULL;
cl_uint     g_min_align = 0;
cl_device_id g_device_ID =0;

// Host arrays
cl_float* g_pfInput = NULL;
cl_float* g_pfRegularOutput = NULL;
cl_float* g_pfOCLOutput = NULL;
int g_szTask = 16*1024*1024;


//	global and local sizes of global and local worksizes
size_t g_szGlobalWork = 32768;
size_t g_szLocalWork = 16;

//	cl_mem objects used as parameters for kernels
cl_mem g_pfInputBuffer = NULL;
cl_mem g_pfOCLOutputBuffer = NULL;

bool g_bUseRelaxedMath = false;
bool g_bUseHostPtr = false;
bool g_bAutoGroupSize = false;
bool g_bEnableProfiling = false;
bool g_bWarming = false;
bool g_bGather4 = false;

bool g_bRunOnPG = false;

LARGE_INTEGER g_PerfFrequency;
LARGE_INTEGER g_PerformanceCountNDRangeStart;
LARGE_INTEGER g_PerformanceCountNDRangeStop;
LARGE_INTEGER g_PerformanceCountReadStart;
LARGE_INTEGER g_PerformanceCountReadStop;
cl_double g_NDRangeTime;



void Cleanup()
{
    //release g_kernel, g_program, and memory objects
    if( g_pfInputBuffer ) {clReleaseMemObject( g_pfInputBuffer ); g_pfInputBuffer = NULL;}
    if( g_pfOCLOutputBuffer ) {clReleaseMemObject( g_pfOCLOutputBuffer ); g_pfOCLOutputBuffer = NULL;}
    if( g_kernel ) {clReleaseKernel( g_kernel );  g_kernel = NULL;}
    if( g_kernel4 ) {clReleaseKernel( g_kernel4 );  g_kernel4 = NULL;}
    if( g_program ) {clReleaseProgram( g_program );  g_program = NULL;}
    if( g_cmd_queue ) {clReleaseCommandQueue( g_cmd_queue );  g_cmd_queue = NULL;}
    if( g_context ) {clReleaseContext( g_context );  g_context = NULL;}

    //host memory
    if(g_pfInput) {_aligned_free( g_pfInput ); g_pfInput = NULL;}
    if(g_pfRegularOutput) {_aligned_free( g_pfRegularOutput ); g_pfRegularOutput = NULL;}
    if(g_pfOCLOutput) {_aligned_free( g_pfOCLOutput ); g_pfOCLOutput = NULL;}

}

bool Setup_OpenCL( const char *program_source )
{
    cl_device_id devices[16];
    size_t cb;
    cl_uint size_ret = 0;
    cl_int err;
    int num_cores;
    char device_name[128] = {0};

	static const char buildOpts[] = "-cl-fast-relaxed-math";

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
        return false;
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
        return false;

    // get the list of CPU devices associated with context
    err = clGetContextInfo(g_context, CL_CONTEXT_DEVICES, 0, NULL, &cb);
    clGetContextInfo(g_context, CL_CONTEXT_DEVICES, cb, devices, NULL);
	g_cmd_queue = clCreateCommandQueue(g_context, devices[0], g_bEnableProfiling ? CL_QUEUE_PROFILING_ENABLE: 0, NULL);
    if (g_cmd_queue == (cl_command_queue)0)
    {
        Cleanup();
        return false;
    }

    char *sources = ReadSources(program_source);	//read program .cl source file
    g_program = clCreateProgramWithSource(g_context, 1, (const char**)&sources, NULL, NULL);
    if (g_program == (cl_program)0)
    {
        printf("ERROR: Failed to create Program with source...\n");
        Cleanup();
        free(sources);
        return false;
    }

	err = clBuildProgram(g_program, 0, NULL, g_bUseRelaxedMath ? buildOpts :NULL, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        printf("ERROR: Failed to build program...\n");
        BuildFailLog(g_program, devices[0]);
        Cleanup();
        free(sources);
        return false;
    }

    g_kernel = clCreateKernel(g_program, "SimpleKernel", NULL);
    if (g_kernel == (cl_kernel)0)
    {
        printf("ERROR: Failed to create kernel...\n");
        Cleanup();
        free(sources);
        return false;
    }
    g_kernel4 = clCreateKernel(g_program, "SimpleKernel4", NULL);
    if (g_kernel4 == (cl_kernel)0)
    {
        printf("ERROR: Failed to create second kernel...\n");
        Cleanup();
        free(sources);
        return false;
    }
    free(sources);

    // use first device ID
    g_device_ID = devices[0];
    err = clGetDeviceInfo(g_device_ID, CL_DEVICE_NAME, 128, device_name, NULL);
    if (err!=CL_SUCCESS)
    {
        printf("ERROR: Failed to get device information (device name)...\n");
        Cleanup();
        return false;
    }
    printf("Using device %s...\n", device_name);

    err = clGetDeviceInfo(g_device_ID, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &num_cores, NULL);
    if (err!=CL_SUCCESS)
    {
        printf("ERROR: Failed to get device information (max compute units)...\n");
        Cleanup();
        return false;
    }
    printf("Using %d compute units...\n", num_cores);


    err = clGetDeviceInfo(g_device_ID, CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(cl_uint), &g_min_align, NULL);
    if (err!=CL_SUCCESS)
    {
        printf("ERROR: Failed to get device information (max memory base address align size)...\n");
        Cleanup();
        return false;
    }
    g_min_align /= 8; //in bytes
    printf("Buffer alignment required for zero-copying is %d bytes (CL_DEVICE_MEM_BASE_ADDR_ALIGN)\n\n", g_min_align);

    return true; // success...
}

void ExecuteNative()
{
    for (int i = 0; i < g_szTask ; ++i)
    {
        g_pfRegularOutput[i] = 1.0f/(sqrt(fabs(g_pfInput[i])));
    }
}

bool ExecuteKernel()
{
    cl_int err = CL_SUCCESS;
    cl_event perf_event;

    const cl_mem_flags INFlags  = (g_bUseHostPtr ? CL_MEM_USE_HOST_PTR: CL_MEM_COPY_HOST_PTR) | CL_MEM_READ_ONLY; 
    const cl_mem_flags OUTFlags = (g_bUseHostPtr ? CL_MEM_USE_HOST_PTR: CL_MEM_COPY_HOST_PTR) | CL_MEM_READ_WRITE;

    // allocate buffers
    g_pfInputBuffer = clCreateBuffer(g_context, INFlags, sizeof(cl_float) * g_szTask , g_pfInput, NULL);
    if (g_pfInputBuffer == (cl_mem)0)
    {
        printf("ERROR: Failed to create g_pfInputBuffer...\n");
        return false;
    }
    g_pfOCLOutputBuffer = clCreateBuffer(g_context, OUTFlags, sizeof(cl_float) * g_szTask , g_pfOCLOutput, NULL);
    if (g_pfOCLOutputBuffer == (cl_mem)0)
    {
        printf("ERROR: Failed to create g_pfOCLOutputBuffer...\n");
        return false;
    }

	cl_kernel	kernel = g_kernel;
    size_t globalWorkSize[1];
    size_t localWorkSize[1];
    globalWorkSize[0] = g_szGlobalWork;
	if(g_bGather4)
	{
	    globalWorkSize[0]/=4; //since proccesing in quadruples
		kernel = g_kernel4;
	}

    //Set kernel arguments
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *) &g_pfInputBuffer);
    if (err != CL_SUCCESS)
    {
        printf("ERROR: Failed to set input g_kernel arguments...\n");
        return false;
    }
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *) &g_pfOCLOutputBuffer);
    if (err != CL_SUCCESS)
    {
        printf("ERROR: Failed to set input g_kernel arguments...\n");
        return false;
    }

    localWorkSize[0] = g_szLocalWork;
    printf("Original global work size %lu\n", globalWorkSize[0]);
    printf("Original local work size %lu\n", localWorkSize[0]);
	if(g_bAutoGroupSize)
	{
		printf("Run-time determines optimal workgroup size\n\n");
	}
	size_t  workGroupSizeMaximum;
	err = clGetKernelWorkGroupInfo(g_kernel, g_device_ID, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), (void *)&workGroupSizeMaximum, NULL);
	printf("Maximum workgroup size for this kernel  %lu\n\n",workGroupSizeMaximum );

	if(g_bWarming)
	{
		printf("Warming up OpenCL execution...");
		err= clEnqueueNDRangeKernel(g_cmd_queue, kernel, 1, NULL, globalWorkSize, g_bAutoGroupSize? NULL:localWorkSize, 0, NULL, NULL);
		clFinish(g_cmd_queue);
		printf("Done\n");
	}

    printf("Executing OpenCL kernel...");
    QueryPerformanceCounter(&g_PerformanceCountNDRangeStart);
	// execute kernel, pls notice g_bAutoGroupSize
	err= clEnqueueNDRangeKernel(g_cmd_queue, kernel, 1, NULL, globalWorkSize, g_bAutoGroupSize? NULL:localWorkSize, 0, NULL, &perf_event);
	if (err != CL_SUCCESS)
    {
        printf("ERROR: Failed to execute kernel...\n");
        return false;
    }
    err = clWaitForEvents(1, &perf_event);
    if (err != CL_SUCCESS)
    {
        printf("ERROR: Failed to clWaitForEvents...\n");
        return false;
    }
    QueryPerformanceCounter(&g_PerformanceCountNDRangeStop);
    printf("Done\n");

    if(g_bEnableProfiling)
    {    
        cl_ulong start = 0;
        cl_ulong end = 0;

        //notice that pure HW execution time is END-START
		err = clGetEventProfilingInfo(perf_event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
        if (err != CL_SUCCESS)
        {
            printf("ERROR: Failed to get clGetEventProfilingInfo CL_PROFILING_COMMAND_START...\n");
            return false;
        }
        err = clGetEventProfilingInfo(perf_event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
        if (err != CL_SUCCESS)
        {
            printf("ERROR: Failed to get clGetEventProfilingInfo CL_PROFILING_COMMAND_END...\n");
            return false;
        }
        g_NDRangeTime = (cl_double)(end - start)*(cl_double)(1e-06);
    }

    void* tmp_ptr = NULL;
    QueryPerformanceCounter(&g_PerformanceCountReadStart);
	if(g_bUseHostPtr)
    {
        tmp_ptr = clEnqueueMapBuffer(g_cmd_queue, g_pfOCLOutputBuffer, true, CL_MAP_READ, 0, sizeof(cl_float) * g_szTask , 0, NULL, NULL, NULL);
        if(tmp_ptr!=g_pfOCLOutput)
        {
            printf("ERROR: clEnqueueMapBuffer failed to return original pointer\n");
            return false;
        }
    }
    else
    {
        err = clEnqueueReadBuffer(g_cmd_queue, g_pfOCLOutputBuffer, CL_TRUE, 0, sizeof(cl_float) * g_szTask , g_pfOCLOutput, 0, NULL, NULL);
        if (err != CL_SUCCESS)
        {
            printf("ERROR: Failed to clEnqueueReadBuffer...\n");
            return false;
        }
    }
    clFinish(g_cmd_queue);
	QueryPerformanceCounter(&g_PerformanceCountReadStop);

    clEnqueueUnmapMemObject(g_cmd_queue, g_pfOCLOutputBuffer, tmp_ptr, 0, NULL, NULL);
	clReleaseMemObject(g_pfInputBuffer); g_pfInputBuffer = NULL;
    clReleaseMemObject(g_pfOCLOutputBuffer); g_pfOCLOutputBuffer = NULL;

    return true;
}

static cl_float randomFloat(const cl_float fMinValue, const cl_float fMaxValue)
{
    if (fMinValue == fMaxValue)
    {
        return fMaxValue;
    }
    cl_float tmp = (cl_float)(rand() % (int)(fMaxValue - fMinValue));
    tmp = (0 == tmp) ? 1.0f : tmp;
    return fMinValue + ((cl_float)(rand() % (int)(fMaxValue - fMinValue))) / tmp;
}

void Usage()
{
    printf("Usage: SimpleOptimizations.exe [--h] [-t <TaskSize>][-l <GroupSize>] [-r] [-p] [-a] [-w] [-v] [-g]\n");
    printf("  where, --h prints this message\n");
    printf("    <TaskSize> is task size (also global size)\n");
    printf("    <GroupSize> is work group size (aka local size)\n");
    printf("    -r relaxed math enabled\n");
    printf("    -p host pointers/buffer-mapping enabled\n");
    printf("    -a auto-selected work group size enbaled, [-l] 'local size' option will be ignored in this case\n");
    printf("    -f OpenCL profiling will be enabled\n");
    printf("    -w additional \"warming\" kernel run enabled \n");
    printf("    -v \"gather4\" kernel version\n");
    printf("    -g run on Processor Graphics\n");
    exit(-1);
}

// main execution routine - perform simple math on float vectors
int _tmain(int argc, _TCHAR* argv[])
{
    //parse command line
    int argn = 1;
    while (argn < argc)
    {
        if (_tcscmp(argv[argn], _T("--h")) == 0)
        {
            Usage();
        }
        else if (_tcscmp(argv[argn], _T("-t")) == 0)
        {
            if(++argn==argc)
                Usage();
            g_szTask = _ttoi(argv[argn]);
            argn++;
        }
        else if (_tcscmp(argv[argn], _T("-l")) == 0)
        {
            if(++argn==argc)
                Usage();
            g_szLocalWork = _ttoi(argv[argn]);
            argn++;
        }
        else if (_tcscmp(argv[argn], _T("-w")) == 0)
        {
            g_bWarming= true;
            argn++;
        }
        else if (_tcscmp(argv[argn], _T("-v")) == 0)
        {
            g_bGather4 = true;
            argn++;
        }
		else if (_tcscmp(argv[argn], _T("-r")) == 0)
        {
            g_bUseRelaxedMath = true;
            argn++;
        }
        else if (_tcscmp(argv[argn], _T("-p")) == 0)
        {
            g_bUseHostPtr = true;
            argn++;
        }
        else if (_tcscmp(argv[argn], _T("-a")) == 0)
        {
            g_bAutoGroupSize = true;
            argn++;
        }
        else if (_tcscmp(argv[argn], _T("-f")) == 0)
        {
            g_bEnableProfiling = true;
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

    //initialize Open CL objects (context, queue, etc.)
    if( Setup_OpenCL("SimpleOptimizations.cl")!=true )
        return -1;


    g_szGlobalWork = g_szTask;
    if(g_szGlobalWork%g_szLocalWork!=0 && !g_bAutoGroupSize)
    {
        printf("Global or local work size is incorect.\n");
        printf("g_szGlobalWork / g_szLocalWork remainder is = %lu. This value must be 0\n", g_szGlobalWork%g_szLocalWork);
        Cleanup();
        return -1;
    }

    g_pfInput = (cl_float*)_aligned_malloc(sizeof(cl_float) * g_szTask , g_min_align);
    g_pfRegularOutput = (cl_float*)_aligned_malloc(sizeof(cl_float) * g_szTask , g_min_align);
    g_pfOCLOutput = (cl_float*)_aligned_malloc(sizeof(cl_float) * g_szTask , g_min_align);
    if(g_pfInput == NULL || g_pfRegularOutput == NULL || g_pfOCLOutput == NULL)
    {
        printf("Host memory allocation error.\n");
        Cleanup();
        return -1;
    }


    //	set input array to random legal values
    srand(2011);
    for (int i = 0; i < g_szTask ; i += 1)
    {
        g_pfInput[i] = randomFloat(-255.0f, 255.0f);
    }

    //retrieve perf. counter frequency
    QueryPerformanceFrequency(&g_PerfFrequency);

    //do simple math
    if(ExecuteKernel()!=true)
    {
        printf("Failed executing OpenCL kernel...\n");
        Cleanup();
        return -1;
    }

    printf("Executing reference...");
    ExecuteNative();
    printf("Done\n\n");


    printf("NDRange perf. counter time %f ms.\n", 
        1000.0f*(float)(g_PerformanceCountNDRangeStop.QuadPart - g_PerformanceCountNDRangeStart.QuadPart)/(float)g_PerfFrequency.QuadPart);
    if(g_bEnableProfiling)    
        printf("NDRange event profiling time %f ms.\n\n", g_NDRangeTime);
    printf("Read buffer perf. counter time %f ms.\n", 
        1000.0f*(float)(g_PerformanceCountReadStop.QuadPart - g_PerformanceCountReadStart.QuadPart)/(float)g_PerfFrequency.QuadPart);


    //Do verification
    printf("Performing verification...\n");
    bool result = true;
    for(cl_int i = 0; i < g_szTask ; i++)	
    {
        //Compare the data
        if( fabsf(g_pfOCLOutput[i] - g_pfRegularOutput[i]) > 0.01 )	
        {
            printf("Error at location %d,  outputArray = %f, refArray = %f \n", i, g_pfOCLOutput[i], g_pfRegularOutput[i]);
            result = false;
        }
    }
	printf("%s", !result ?"ERROR: Verification failed.\n":"Verification succeeded.\n");
    
    Cleanup();
    return 0;
}

#pragma warning( pop )
