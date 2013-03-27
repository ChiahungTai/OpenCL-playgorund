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

//we want to use POSIX functions
#pragma warning( push )
#pragma warning( disable : 4996 )

#define INPUT_IMAGE "My5_TM.rgb"
#define BLOCK_DIM 16

//to enable kernel version with per pixel processing
#define PER_PIXEL


cl_mem		g_inputBuffer = NULL;
cl_mem		g_outputBuffer = NULL;
cl_mem		g_HDRDataBuffer = NULL;
cl_context	g_context = NULL;
cl_command_queue g_cmd_queue = NULL;
cl_program	g_program = NULL;
cl_kernel	g_kernel = NULL;
cl_uint 	g_globalWorkSize = 0;
cl_int      g_min_align = 0;

bool g_bRunOnPG = false;

//for perf. counters
#include <Windows.h>

LARGE_INTEGER g_PerfFrequency;
LARGE_INTEGER g_PerformanceCountNDRangeStart;
LARGE_INTEGER g_PerformanceCountNDRangeStop;


void Cleanup_OpenCL()
{
    //release g_kernel, g_program, and memory objects
    if( g_inputBuffer ) {clReleaseMemObject( g_inputBuffer ); g_inputBuffer = NULL;}
    if( g_outputBuffer ) {clReleaseMemObject( g_outputBuffer );  g_outputBuffer = NULL;}
    if( g_HDRDataBuffer ) {clReleaseMemObject( g_HDRDataBuffer );  g_HDRDataBuffer = NULL;}
    if( g_kernel ) {clReleaseKernel( g_kernel );  g_kernel = NULL;}
    if( g_program ) {clReleaseProgram( g_program );  g_program = NULL;}
    if( g_cmd_queue ) {clReleaseCommandQueue( g_cmd_queue );  g_cmd_queue = NULL;}
    if( g_context ) {clReleaseContext( g_context );  g_context = NULL;}
}

int Setup_OpenCL( const char *program_source )
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
        return -1;
    }
    free(sources);

    // retrieve platform information

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
    iMemSize = iWidth*iHeight*4*sizeof(cl_float); 

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

    // Extended dynamic range 
    for(int i = 0; i < iWidth*iHeight*4; i++)
    {
        inputArray[i] = 5.0f*inputArray[i];
    }


    // HDR-image hight & weight
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
            cl_uint uiTmp[4];
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
    SaveImageAsBMP( outUIntBuf, iWidth, iHeight, "ToneMappingInput.bmp");
    free(outUIntBuf);

    return inputArray;
}

//declaration of native function
void EvaluateRaw(float* inputArray, float* outputArray, CHDRData *pData, int arrayWidth, int iRow);

void ExecuteToneMappingReference(cl_float* inputArray, cl_float* outputArray, CHDRData *pData, cl_uint arrayWidth, cl_uint arrayHeight)
{
    //image height loop
    for(cl_uint j = 0; j < arrayHeight;j++)
    {
        EvaluateRaw(inputArray, outputArray, pData, arrayWidth, j);
    }
}

bool ExecuteToneMappingKernel(cl_float* inputArray, cl_float* outputArray, CHDRData *pData, cl_uint arrayWidth, cl_uint arrayHeight)
{
    cl_int err = CL_SUCCESS;


    // allocate the buffer
    g_inputBuffer = clCreateBuffer(g_context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(cl_float) * 4 * arrayWidth * arrayHeight, inputArray, NULL);

    if (g_inputBuffer == (cl_mem)0)
    {
        printf("ERROR: Failed to create Input Buffer...\n");
        return false;
    }

    g_outputBuffer = clCreateBuffer(g_context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, sizeof(cl_float) * 4 * arrayWidth * arrayHeight, outputArray, NULL);

    if (g_outputBuffer == (cl_mem)0)
    {
        printf("ERROR: Failed to create Output Buffer...\n");
        return false;
    }

    g_HDRDataBuffer = clCreateBuffer(g_context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(CHDRData), pData, NULL);

    if (g_HDRDataBuffer == (cl_mem)0)
    {
        printf("ERROR: Failed to create Output Buffer...\n");
        return false;
    }


    err  = clSetKernelArg(g_kernel, 0, sizeof(cl_mem), (void *) &g_inputBuffer);
    err  |= clSetKernelArg(g_kernel, 1, sizeof(cl_mem), (void *) &g_outputBuffer);
    err  |= clSetKernelArg(g_kernel, 2, sizeof(cl_mem), (void *) &g_HDRDataBuffer);
    err  |= clSetKernelArg(g_kernel, 3, sizeof(cl_int), (void *) &arrayWidth);

    if (err != CL_SUCCESS)
    {
        printf("ERROR: Failed to set input g_kernel arguments...\n");
        return false;
    }

#ifdef PER_PIXEL
	cl_uint workDim = 2;
#else
	cl_uint workDim = 1;
#endif


    size_t globalWorkSize[2];
    size_t localWorkSize[2]= { BLOCK_DIM, BLOCK_DIM };
    globalWorkSize[0] = g_globalWorkSize;
	globalWorkSize[1] = arrayWidth;

    printf("Original global work size %d\n", globalWorkSize[0]);
    printf("Original local work size %d\n", localWorkSize[0]);

    QueryPerformanceCounter(&g_PerformanceCountNDRangeStart);
    // execute kernel
    if (CL_SUCCESS != clEnqueueNDRangeKernel(g_cmd_queue, g_kernel, workDim, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL))
    {
        printf("ERROR: Failed to run kernel...\n");
        return false;
    }
    err = clFinish(g_cmd_queue);
    QueryPerformanceCounter(&g_PerformanceCountNDRangeStop);

    void* tmp_ptr = NULL;
    tmp_ptr = clEnqueueMapBuffer(g_cmd_queue, g_outputBuffer, true, CL_MAP_READ, 0, sizeof(cl_float) *  4 * arrayWidth * arrayHeight , 0, NULL, NULL, NULL);
	if(tmp_ptr!=outputArray)
	{
		printf("ERROR: clEnqueueMapBuffer failed to return original pointer\n");
		return false;
	}

    err = clFinish(g_cmd_queue);

    clEnqueueUnmapMemObject(g_cmd_queue, g_outputBuffer, tmp_ptr, 0, NULL, NULL);

    clReleaseMemObject(g_inputBuffer); g_inputBuffer = NULL;
    clReleaseMemObject(g_outputBuffer); g_outputBuffer = NULL;
    clReleaseMemObject(g_HDRDataBuffer); g_HDRDataBuffer = NULL;

    return true;
}


//calculate FStops value parameter from the arguments
float resetFStopsParameter( float powKLow, float kHigh )
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
    printf("Usage: ToneMapping.exe [--h] [-g]\n");
    printf("  where, --h prints this message\n");
    printf("    -g run on Processor Graphics\n");
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
    HDRData.fFStops = resetFStopsParameter(HDRData.fPowKLow, kHigh);
    printf("resetFStopsParameter result = %f\n", HDRData.fFStops);

    HDRData.fFStopsInv = 1.0f/HDRData.fFStops;

    
    //initialize Open CL objects (context, queue, etc.)
    if( 0 != Setup_OpenCL("ToneMapping.cl") )
        return -1;

    
    //fill input frame
    cl_float* inputArray = 0;
    //read input image
    inputArray = readInput(&arrayWidth, &arrayHeight);
    if(inputArray==0)
        return -1;


    printf("Input size is %d X %d\n", arrayWidth, arrayHeight);
    cl_float* outputArray = (cl_float*)_aligned_malloc(sizeof(cl_float) * 4 * arrayWidth * arrayHeight, g_min_align);
    cl_float* refArray = (cl_float*)_aligned_malloc(sizeof(cl_float) * 4 * arrayWidth * arrayHeight, g_min_align);


    //do tone mapping
    printf("Executing OpenCL kernel...\n");
    ExecuteToneMappingKernel(inputArray, outputArray, &HDRData, arrayWidth, arrayHeight);

    printf("Executing reference...\n");
    ExecuteToneMappingReference(inputArray, refArray, &HDRData, arrayWidth, arrayHeight);

    //save results in bitmap files

    float fTmpFVal = 0.0f;
    cl_uint* outUIntBuf=0;
    outUIntBuf = (cl_uint*)malloc(arrayWidth*arrayHeight*sizeof(cl_uint));
    if(!outUIntBuf)
    {
        printf("Failed to allocate memory for output BMP image!\n");
        return -1;
    }
    for(cl_uint y = 0; y < arrayHeight; y++)
    {
        for(cl_uint x = 0; x < arrayWidth; x++)
        {
            cl_uint uiTmp[4];
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
    SaveImageAsBMP( outUIntBuf, arrayWidth, arrayHeight, "ToneMappingOutput.bmp");

    for(cl_uint y = 0; y < arrayHeight; y++)
    {
        for(cl_uint x = 0; x < arrayWidth; x++)
        {
            cl_uint uiTmp[4];
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
    SaveImageAsBMP( outUIntBuf, arrayWidth, arrayHeight, "ToneMappingOutputReference.bmp");
    free(outUIntBuf);

    //Do verification
    printf("Performing verification...\n");
    bool result = true;
    for(cl_uint i = 0; i < arrayWidth*arrayHeight*4; i++)	
    {
        //Compare the data
        if( fabsf(outputArray[i] - refArray[i]) > 0.01 )	
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

#pragma warning( pop )
