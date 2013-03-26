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


/**
 ******************************************************************************* 
 * @file <ConvolutionFilter.java>
 * 
 * @brief Defines ConvolutionFilter class, used by both Convolution and 
 *        ConvolutionKernel class for holding filter parameters for each 
 *        filter type
 ******************************************************************************/

/**
 * Class to hold filter weights for a 3x3 filter, applied to each pixel in the
 * image
 */
public class ConvolutionFilter {
    public float[] weights;

    public int offset;

    /**
     * ConvolutionFilter constructor, sets the filter weights and offset
     * 
     * @param _nw
     *            The weight used for North West pixel
     * @param _n
     *            The weight used for North pixel
     * @param _ne
     *            The weight used for North East pixel
     * @param _w
     *            The weight used for West pixel
     * @param _o
     *            The weight used for itself
     * @param _e
     *            The weight used for East pixel
     * @param _sw
     *            The weight used for South West pixel
     * @param _s
     *            The weight used for South pixel
     * @param _se
     *            The weight used for South East pixel
     * @param _offset
     *            Offset is added to pixel to normalized weighted sum
     */
    ConvolutionFilter(float _nw, float _n, float _ne, float _w, float _o,
            float _e, float _sw, float _s, float _se, int _offset) {
        weights = new float[] { _nw, _w, _ne, _w, _o, _e, _sw, _s, _se };
        offset = _offset;
    }

}
