/**********************************************************************
Copyright ©2012 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/
/*
 * For a description of the algorithm and the terms used, please see the
 * documentation for this sample.
 *
 * Each thread calculates a pixel component(rgba), by applying a filter 
 * on group of 8 neighbouring pixels in both x and y directions. 
 * Both filters are summed (vector sum) to form the final result.
 */

__constant sampler_t imageSampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_LINEAR; 

__kernel void sobel_filter(__read_only image2d_t inputImage, __write_only image2d_t outputImage)
{
	int2 coord = (int2)(get_global_id(0), get_global_id(1));

	float4 Gx = (float4)(0);
	float4 Gy = Gx;
	
	//if( coord.x >= 1 && coord.x < (get_global_size(0)-1) && coord.y >= 1 && coord.y < get_global_size(1) - 1)
	//{

		float4 i00 = convert_float4(read_imageui(inputImage, imageSampler, (int2)(coord.x - 1, coord.y + 1)));
		float4 i10 = convert_float4(read_imageui(inputImage, imageSampler, (int2)(coord.x - 0, coord.y + 1)));
		float4 i20 = convert_float4(read_imageui(inputImage, imageSampler, (int2)(coord.x + 1, coord.y + 1)));
		float4 i01 = convert_float4(read_imageui(inputImage, imageSampler, (int2)(coord.x - 1, coord.y + 0)));
		float4 i11 = convert_float4(read_imageui(inputImage, imageSampler, (int2)(coord.x - 0, coord.y + 0)));
		float4 i21 = convert_float4(read_imageui(inputImage, imageSampler, (int2)(coord.x + 1, coord.y + 0)));
		float4 i02 = convert_float4(read_imageui(inputImage, imageSampler, (int2)(coord.x - 1, coord.y - 1)));
		float4 i12 = convert_float4(read_imageui(inputImage, imageSampler, (int2)(coord.x - 0, coord.y - 1)));
		float4 i22 = convert_float4(read_imageui(inputImage, imageSampler, (int2)(coord.x + 1, coord.y - 1)));

		Gx =   i00 + (float4)(2) * i10 + i20 - i02  - (float4)(2) * i12 - i22;

		Gy =   i00 - i20  + (float4)(2)*i01 - (float4)(2)*i21 + i02  -  i22;

		Gx = native_divide(native_sqrt(Gx * Gx + Gy * Gy), (float4)(2));

		write_imageui(outputImage, coord, convert_uint4(Gx));
	//}

			
}

	

	 






	

	




	

	

	
	
