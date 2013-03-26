/**********************************************************************
Copyright 2012 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

package com.amd.aparapi.sample.Convolution;

import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;

import com.amd.aparapi.Kernel;
import com.amd.aparapi.Range;

/**
 ******************************************************************************* 
 * @file <ConvolutionKernel.java>
 * 
 * @brief Implements the core functionality including the kernel i.e. core 
 *	  algorithm. Called by "Convolution" to do all the work.
 ******************************************************************************/

/**
 * Class that extends the "Kernel" class (defined in aparapi.jar). This Class
 * overrides "Kernel.run()", run() defines what gets executed for each input
 * value in the image
 */
public class ConvolutionKernel extends Kernel {

    private final float[] filter = new float[9];

    private final int[] inputData;

    private int[] outputData;

    private final int width;

    private final int height;

    private int offset;

    /**
     * ConvolutionKernel constructor, sets the image parameters
     * 
     * @param _width
     *            Width of the image, rounded to PAD value
     * @param _height
     *            Height of the image, rounded to PAD value
     * @param _inputImage
     * @param _outputImage
     */
    public ConvolutionKernel(int _width, int _height,
            BufferedImage _inputImage) {
        inputData = ((DataBufferInt) _inputImage.getRaster().getDataBuffer())
                .getData();
        width = _width;
        height = _height;

    }

    /**
     * This method is reference CPU implementation for convolution. It is used
     * only if verify option is used.
     * 
     * @param refOutput
     *            Output buffer where post convolution output image is stored
     */
    public void referenceConvolution(int[] refOutput) {
        int x, y;
        for (x = 0; x < height; x++) {
            for (y = 0; y < width; y++) {
                if (x > 1 && x < (width - 1) && y > 1 && y < (height - 1)) {
                    int result = 0;
                    /**
                     * We handle each color separately using rgbshift as an 8
                     * bit mask for red, green, blue
                     */
                    for (int rgbShift = 0; rgbShift < 24; rgbShift += 8) {
                        int channelAccum = 0;
                        float accum = 0;

                        /* 3x3 filter being applied */
                        for (int count = 0; count < 9; count++) {
                            int dx = (count % 3) - 1; // 0,1,2 -> -1,0,1
                            int dy = (count / 3) - 1; // 0,1,2 -> -1,0,1

                            int rgb = (inputData[((y + dy) * width) + (x + dx)]);
                            int channelValue = ((rgb >> rgbShift) & 0xff);
                            /**
                             * Keep sum of all weights, will be used for
                             * normalization
                             */
                            accum += filter[count];

                            /* filter weights applied and accumulated */
                            channelAccum += channelValue * filter[count++];

                        }
                        /* Normalization */
                        channelAccum /= accum;
                        channelAccum += offset;
                        /* Pixel value clamp */
                        channelAccum = max(0, min(channelAccum, 0xff));
                        result |= (channelAccum << rgbShift);
                    }
                    refOutput[y * width + x] = result;
                }
            }
        }
    }

    /**
     * "run()" defines the operations that need to done for each input parameter
     */
    public void run() {

        /* x is pixel location in the row */
        int x = getGlobalId(0);

        /* y is pixel location in the column */
        int y = getGlobalId(1);

        /* w is picture width */
        int w = getGlobalSize(0);

        /* h is picture height */
        int h = getGlobalSize(1);

        /**
         * A pixel is filtered only if it is not at the image edge i.e. valid
         * pixels are available all around it
         */
        if (x > 1 && x < (w - 1) && y > 1 && y < (h - 1)) {

            int result = 0;
            /**
             * We handle each color separately using rgb shift as an 8 bit mask
             * for red, green and blue
             */
            for (int rgbShift = 0; rgbShift < 24; rgbShift += 8) {
                int channelAccum = 0;
                float accum = 0;

                /* 3x3 filter being applied */
                for (int count = 0; count < 9; count++) {
                    int dx = (count % 3) - 1; // 0,1,2 -> -1,0,1
                    int dy = (count / 3) - 1; // 0,1,2 -> -1,0,1

                    int rgb = (inputData[((y + dy) * w) + (x + dx)]);
                    int channelValue = ((rgb >> rgbShift) & 0xff);
                    /**
                     * Keep sum of all weights, will be used for normalization
                     */
                    accum += filter[count];
                    /* filter weights applied and accumulated */
                    channelAccum += channelValue * filter[count++];

                }
                /* Normalization */
                channelAccum /= accum;
                channelAccum += offset;
                /* Pixel value clamp */
                channelAccum = max(0, min(channelAccum, 0xff));
                result |= (channelAccum << rgbShift);
            }
            outputData[y * w + x] = result;
        }
    }

    /**
     * This method mainly calls "execute" with appropriate values which in turns
     * causes the overridden Kernel.run() method to be invoked once for each
     * value in the range
     * 
     * @param _filter
     *            Object of ConvolutionFilter, have appropriate weights and
     *            offset
     */
    public void convolve(ConvolutionFilter _filter, BufferedImage _outputImage) {

    	outputData = ((DataBufferInt) _outputImage.getRaster().getDataBuffer())
                .getData();
        System.arraycopy(_filter.weights, 0, filter, 0, _filter.weights.length);
        offset = _filter.offset;
        /*
         * "kernel" method that explicitly enqueued before the kernel is
         * executed
         */
        put(filter);
        execute(Range.create2D(width, height, 8, 8));
        /**
         * "kernel" method that enqueues request to get this buffer from GPU
         * blocks till until "outputData" is available
         */
        get(outputData);
    }
}
