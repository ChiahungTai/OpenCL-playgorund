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

#ifndef _HDRTONEMAPPING_HPP__
#define _HDRTONEMAPPING_HPP__

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS

/**
* \File HDRToneMapping.hpp
* \brief declaration of the class HDRToneMapping
**/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <SDKCommon.hpp>
#include <SDKApplication.hpp>
#include <SDKFile.hpp>
#include <SDKBitMap.hpp>
#include <CL/cl.hpp>

/**
* \class HDRToneMapping
* \brief And HDRToneMaping class. This class is derived from SDKSample base class 
* This class implements tonemapping operation using OpenCL. 
* This operation is used to convert an HDR image to LDR image
* Pattanaik tone mapping operator is used to implement this.
*/

class HDRToneMapping : public SDKSample
{
public: 
    /**
    * \fn HDRToneMapping()
    * \brief A constructor
    * \param sampleName The name of the sample 
    */
    HDRToneMapping(std::string sampleName) : SDKSample(sampleName)
    {
        cPattanaik = 0.25f;
        gammaPattanaik = 0.4f;
        deltaPattanaik = 0.000002f;
        inputImageName = "input.hdr";
        outputImageName = "output.bmp";
        numChannels = 4;
        averageLuminance = 0.0f;
        iterations = 1;
        blockSizeX = 16;
        blockSizeY = 16;
        input = NULL;
        output = NULL;
        referenceOutput = NULL;
    }

    /**
    * \fn HDRToneMapping()
    * \brief A constructor
    * \param sampleName The name of the sample 
    */
    HDRToneMapping(const char* sampleName) : SDKSample(sampleName)
    {
        cPattanaik = 0.25f;
        gammaPattanaik = 0.4f;
        deltaPattanaik = 0.000002f;
        inputImageName = "input.hdr";
        outputImageName = "output.bmp";
        numChannels = 4;
        averageLuminance = 0.0f;
        iterations = 1;
        blockSizeX = 16;
        blockSizeY = 16;
        input = NULL;
        output = NULL;
        referenceOutput = NULL;
    }
    
    /**
    * \fn ~HDRToneMapping()
    * \brief A destructor
    */
    ~HDRToneMapping()
    {
        
    }

    /**
     * \fn int genBinaryImage()
     * \brief Override from SDKSample, Generate binary image of given kernel 
     * and exit application.
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int genBinaryImage();

    /**
    * \fn int setupCL()
    * \brief OpenCL related initialisations. 
    * Set up Context, Device list, Command Queue, Memory buffers
    * Build CL kernel program executable
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int setupCL();

    /**
    * \fn int cpuPattanaikReference();
    * \brief CPU reference implementation of the pattanaik tone mapping operator
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int cpuPattanaikReference();

    /**
    * \fn int runCLKernels()
    * \brief Set values for kernels' arguments, enqueue calls to the kernels
    * on to the command queue, wait till end of kernel execution.
    * Get kernel start and end time if timing is enabled
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int runCLKernels();

    /**
    * \fn void printStats()
    * \brief Override from SDKSample. Print sample stats.
    */
    void printStats();

    /**
    * \fn int initialize()
    * \brief Override from SDKSample. Initialize 
    * command line parser, add custom options
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int initialize();

    /**
    * \fn int setup()
    * \brief Override from SDKSample, adjust width and height 
    * of execution domain, perform all sample setup
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int setup();

    /**
    * \fn int run()
    * \brief Override from SDKSample
    * Run OpenCL Sobel Filter
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int run();

    /**
    * \fn int cleanup()
    * \brief Override from SDKSample
    * Cleanup memory allocations
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int cleanup();

    /**
    * \fn int verifyResults()
    * \brief Override from SDKSample
    * Verify against reference implementation
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int verifyResults();

    /**
    * \fn int readInputImage()
    * \brief Reading the input image 
    */
    int readInputImage();

private:
    std::string inputImageName;                         /**<Name of the input image*/
    std::string outputImageName;                        /**<Name of the output image*/
    cl_float cPattanaik;                                /**<C is a parameter used in Pattanaik tone mapping operator */
    cl_float gammaPattanaik;                            /**<gamma is used in pattanaik tone mapping implementation*/
    cl_float deltaPattanaik;                            /**<Delta is used in pattanaik tone mapping implementation*/
    cl_uint width;                                      /**<Width of the image */
    cl_uint height;                                     /**<Height of the image*/
    cl_uint numChannels;                                /**<Number of channels in the image */
    cl_double setupTime;                                /**< time taken to setup OpenCL resources and building kernel */
    cl_double kernelTime;                               /**< time taken to run kernel and read result back */
    cl_float *input;                                    /**< input data*/
    cl_float *output;                                   /**< output data*/
    cl_float *referenceOutput;                          /**< CPU reference output data */
    cl::Context context;                                /**< Context */
    std::vector<cl::Device> devices;                    /**< vector of devices */
    std::vector<cl::Device> device;                     /**< device to be used */
    std::vector<cl::Platform> platforms;                /**< vector of platforms */
    cl::CommandQueue commandQueue;                      /**< command queue */
    cl::Buffer inputImageBuffer;                        /**< input buffer to device */
    cl::Buffer outputImageBuffer;                       /**< output buffer from device */
    cl::Kernel kernel;                                  /**< Kernel object */
    size_t kernelWorkGroupSize;                         /**< Group Size returned by kernel */
    size_t blockSizeX;                                  /**< Work-group size in x-direction */
    size_t blockSizeY;                                  /**< Work-group size in y-direction */
    cl::Program program;                                /**< program object */
    cl_float averageLuminance;                          /**<Average luminance value of the input image */
    int iterations;                                     /**< Number of iterations for kernel execution */
    size_t       maxWorkGroupSize;                      /**< Device Specific Information */
    cl_uint         maxDimensions;
    size_t *     maxWorkItemSizes;
    cl_ulong     totalLocalMemory;
    streamsdk::SDKDeviceInfo deviceInfo;/**< Structure to store device information*/
    streamsdk::KernelWorkGroupInfo kernelInfo;/**< Structure to store kernel related info */
};

#endif 
