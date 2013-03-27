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


/*
 *	Projects a buffer of N dimensional points into a buffer of 3 dimensional points
 *	Uses star coordinate projection (projection matrix) 
 *
 */


float4 project_point( 
	const __global float* point, 
	const __global float4* transaxes, 
	
	const unsigned int dims )
{
	float4 projpoint = {0,0,0,0};
	for(unsigned int i=0; i<dims; i++)
	{
		projpoint += point[i] * transaxes[i];
	}
	return projpoint;
}

__kernel 
void project_all_points( 
	// in/out buffers
    __global const float* ndpoints,
	__global const float4* transaxes,
	__global float4* projpoints,

	// int parameters
	const unsigned int dims			// number of dimensions
	)
{
    unsigned int gid = get_global_id(0);
	unsigned int currpt = gid*dims;

	projpoints[gid] = project_point( &ndpoints[currpt], transaxes, dims );
}