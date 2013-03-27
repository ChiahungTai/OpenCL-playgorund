
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
 *	Create a euclidean distance "matrix" (unpacked into a long vector) for given ND points
 *	Also, calculate difference between euclidean distance "matrices" to get projection error
 *
 */

float nddistance( 
	__global const float* point1, 
	__local const float* point2, 
	
	const unsigned int dims 
	)
{
	float distance = 0;
	float tmp;

	// calculate error between i,j pair (can this be vectorized?)
	for(int d=0; d<dims; d++)
	{
		tmp = point1[d] - point2[d];
		distance += tmp*tmp;
	}

	return distance;
}

__kernel
void relative_distances (
	// in/out buffers
    __global const float* ndpoints,
	__global float* distance_sum,

	// local memory
	__local float* cachedpts,

	// int parameters
	const unsigned int dims,			// number of dimensions
	const unsigned int numpts			// number of points
	)
{
    unsigned int tid = get_local_id(0);
    unsigned int gid = get_global_id(0);
    unsigned int localSize = get_local_size(0);

	unsigned int index = tid*dims;

    // Number of tiles we need to iterate
    unsigned int numTiles = numpts / localSize;

	// src point index (remove dimensions)
	unsigned int src = gid / dims;
	unsigned int dst;

	// get output index for optimal EDM
	unsigned int buffer_index = (unsigned int)(src*(numpts - 0.5f) - numpts - (float)(src*src)*0.5f) - 1;

	// index for cache
	// TODO: start at src, not at 0
	unsigned int pt_index;
    for(int i = 0; i < numTiles; ++i)
    {
        // load one tile into local memory
		pt_index = i*dims*localSize + index;
        cachedpts[index] = ndpoints[pt_index];

        barrier(CLK_LOCAL_MEM_FENCE);

		// test against each point in tile
        for(int j = 0; j < localSize; j+=dims)
		{
			dst = pt_index + j;
			dst /= dims; // dst point index (remove dimensions)

			if(src > dst)
			{
				// calculate error between i,j pair
				distance_sum[buffer_index + dst] = nddistance( &ndpoints[index], &cachedpts[j], dims );
			}
        }

        barrier(CLK_LOCAL_MEM_FENCE);
    }
}

__kernel
void float_difference(
	__global float* lhs,
	__global float* rhs,

	__global float* out
	)
{
	unsigned int gid = get_global_id(0);
	float diff = lhs[gid] - rhs[gid];
	out[gid] = diff*diff;
}
