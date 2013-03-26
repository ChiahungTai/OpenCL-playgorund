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
 * Quasi Random Sequence
 * Output size : n_dimensions * n_vectors
 * Input size: n_dimensions * n_directions 
 * shared buffer size : n_directions
 * Number of blocks : n_dimensions
 * First, all the direction numbers for a dimension are cached into
 * shared memory. Then each thread writes a single vector value by
 * using all the direction numbers from the shared memory.
 *
 */


#define N_DIRECTIONS_IN 8


__kernel void QuasiRandomSequence(__global  float4* output,
                                  __global  uint4* input,
					    		  __local uint4* shared)
{
	uint global_id = get_global_id(0);
	uint local_id = get_local_id(0);
	uint group_id = get_group_id(0);


	uint4 temp = 0;
	uint factor = local_id*4;
	uint4 vlid = (uint4)(factor, factor + 1, factor + 2, factor + 3);
	float divisor = (float)pow((float)2, (float)32);


	for(int i=0; i < N_DIRECTIONS_IN; i++)
	{
		shared[i] = input[group_id * N_DIRECTIONS_IN + i];
	}
	
	
	for(int k=0; k < 3 ; k++)
	{

		uint fK = k*4;
		uint4 vK = (uint4)(fK, fK + 1, fK + 2, fK + 3);

		/*The following line of bitwise operation is equal to
		uint4 mask = (uint4)(pow((float)2, (float)vK.x), pow((float)2, (float)vK.y), pow((float)2, (float)vK.z), pow((float)2, (float)vK.w));*/

		uint4 mask = (uint4) ((1 << vK.x) , 1<< vK.y, 1<< vK.z, 1 << vK.w );
		

		temp.x ^= (((vlid.x & mask.x)>>vK.x) * shared[k].x) ^ 			    (((vlid.x & mask.y)>>vK.y) * shared[k].y) ^ 			    (((vlid.x & mask.z)>>vK.z) * shared[k].z) ^ 			    (((vlid.x & mask.w)>>vK.w) * shared[k].w);

		temp.y ^= (((vlid.y & mask.x)>>vK.x) * shared[k].x) ^ 			    (((vlid.y & mask.y)>>vK.y) * shared[k].y) ^ 			    (((vlid.y & mask.z)>>vK.z) * shared[k].z) ^ 			    (((vlid.y & mask.w)>>vK.w) * shared[k].w);

		temp.z ^= (((vlid.z & mask.x)>>vK.x) * shared[k].x) ^ 			    (((vlid.z & mask.y)>>vK.y) * shared[k].y) ^ 			    (((vlid.z & mask.z)>>vK.z) * shared[k].z) ^ 			    (((vlid.z & mask.w)>>vK.w) * shared[k].w);

		temp.w ^= (((vlid.w & mask.x)>>vK.x) * shared[k].x) ^ 			    (((vlid.w & mask.y)>>vK.y) * shared[k].y) ^ 			    (((vlid.w & mask.z)>>vK.z) * shared[k].z) ^ 			    (((vlid.w & mask.w)>>vK.w) * shared[k].w);

	}

	  output[global_id] = convert_float4(temp) / divisor;
}

