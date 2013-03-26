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

#ifndef SOBEL_FILTER_IMAGE_H_
#define SOBEL_FILTER_IMAGE_H_

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#include <CL/cl.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <SDKCommon.hpp>
#include <SDKApplication.hpp>
#include <SDKFile.hpp>
#include <SDKBitMap.hpp>


#define INPUT_IMAGE "SobelFilterImage_Input.bmp"
#define OUTPUT_IMAGE "SobelFilterImage_Output.bmp"

#define GROUP_SIZE 256

/**
* SobelFilterImage 
* Class implements OpenCL Sobel Filter sample using Images
* Derived from SDKSample base class
*/

class SobelFilterImage : public SDKSample
{
    cl_double setupTime;                /**< time taken to setup OpenCL resources and building kernel */
    cl_double kernelTime;               /**< time taken to run kernel and read result back */
    cl_uchar4* inputImageData;          /**< Input bitmap data to device */
    cl_uchar4* outputImageData;         /**< Output from device */

    cl::Context context;                            /**< CL context */
    std::vector<cl::Device> devices;                /**< CL device list */
    std::vector<cl::Device> device;                 /**< CL device to be used */
    std::vector<cl::Platform> platforms;            /**< list of platforms */
    cl::Image2D inputImage2D;                       /**< CL Input image2d */
    cl::Image2D outputImage2D;                      /**< CL Output image2d */
    cl::CommandQueue commandQueue;                  /**< CL command queue */
    cl::Program program;                            /**< CL program  */
    cl::Kernel kernel;                              /**< CL kernel */

    cl_uchar* verificationOutput;       /**< Output array for reference implementation */
    
    streamsdk::SDKBitMap inputBitmap;   /**< Bitmap class object */
    streamsdk::uchar4* pixelData;       /**< Pointer to image data */
    cl_uint pixelSize;                  /**< Size of a pixel in BMP format> */
    cl_uint width;                      /**< Width of image */
    cl_uint height;                     /**< Height of image */
    cl_bool byteRWSupport;
    size_t kernelWorkGroupSize;         /**< Group Size returned by kernel */
    size_t blockSizeX;                  /**< Work-group size in x-direction */
    size_t blockSizeY;                  /**< Work-group size in y-direction */
    int iterations;                     /**< Number of iterations for kernel execution */
    int imageSupport;


public:

    /**
    * Read bitmap image and allocate host memory
    * @param inputImageName name of the input file
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int readInputImage(std::string inputImageName);

    /**
    * Write to an image file
    * @param outputImageName name of the output file
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int writeOutputImage(std::string outputImageName);

    /** 
    * Constructor 
    * Initialize member variables
    * @param name name of sample (string)
    */
    SobelFilterImage(std::string name)
        : SDKSample(name),
        inputImageData(NULL),
        outputImageData(NULL),
        verificationOutput(NULL),
        byteRWSupport(true)
    {
        pixelSize = sizeof(streamsdk::uchar4);
        pixelData = NULL;
        blockSizeX = GROUP_SIZE;
        blockSizeY = 1;
        iterations = 1;
        imageSupport = 0;
    }

    /** 
    * Constructor 
    * Initialize member variables
    * @param name name of sample (const char*)
    */
    SobelFilterImage(const char* name)
        : SDKSample(name),
        inputImageData(NULL),
        outputImageData(NULL),
        verificationOutput(NULL),
        byteRWSupport(true)
    {
        pixelSize = sizeof(streamsdk::uchar4);
        pixelData = NULL;
        blockSizeX = GROUP_SIZE;
        blockSizeY = 1;
        iterations = 1;
        imageSupport = 0;
    }

    ~SobelFilterImage()
    {
    }

    /**
    * Allocate image memory and Load bitmap file
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int setupSobelFilterImage();

    /**
     * Override from SDKSample, Generate binary image of given kernel 
     * and exit application
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int genBinaryImage();

    /**
    * OpenCL related initialisations. 
    * Set up Context, Device list, Command Queue, Memory buffers
    * Build CL kernel program executable
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int setupCL();

    /**
    * Set values for kernels' arguments, enqueue calls to the kernels
    * on to the command queue, wait till end of kernel execution.
    * Get kernel start and end time if timing is enabled
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int runCLKernels();

    /**
    * Reference CPU implementation of Binomial Option
    * for performance comparison
    */
    void sobelFilterImageCPUReference();

    /**
    * Override from SDKSample. Print sample stats.
    */
    void printStats();

    /**
    * Override from SDKSample. Initialize 
    * command line parser, add custom options
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int initialize();

    /**
    * Override from SDKSample, adjust width and height 
    * of execution domain, perform all sample setup
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int setup();

    /**
    * Override from SDKSample
    * Run OpenCL Sobel Filter
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int run();

    /**
    * Override from SDKSample
    * Cleanup memory allocations
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int cleanup();

    /**
    * Override from SDKSample
    * Verify against reference implementation
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int verifyResults();
};

#endif // SOBEL_FILTER_IMAGE_H_
