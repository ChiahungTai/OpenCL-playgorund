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


__kernel void simple_invert_shared(__write_only image2d_t YOut, __read_only image2d_t YIn, __write_only image2d_t UVOut, __read_only image2d_t UVIn)
{
	const int x = get_global_id(0);		// Cuda equivalent : blockIdx.x*bw + tx
	const int y = get_global_id(1);		// Cuda equivalent : blockIdx.y*bh + ty

	float4 pixel = (float4)0;
	
	int2 coord = (int2)(x, y); 
        const sampler_t smp = CLK_FILTER_NEAREST; 

	pixel = read_imagef(YIn, smp, coord); 
	pixel =  (float4)1.0 - pixel;
	write_imagef(YOut, coord, pixel); 

	if (x%2 == 0 && y%2 == 0)
	{
		int2 coordUV = (int2)(x/2, y/2); 
		pixel = read_imagef(UVIn, smp, coordUV); 
		pixel =  (float4)1.0 - pixel;
		write_imagef(UVOut, coordUV, pixel); 
	}
}