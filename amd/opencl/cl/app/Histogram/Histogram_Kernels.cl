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
 * On invocation of kernel blackScholes, each work thread calculates
 * thread-histogram bin and finally all thread-histograms merged into 
 * block-histogram bin. Outside the kernel, All block-histograms merged
 * into final histogram
 */

#define LINEAR_MEM_ACCESS
#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable 

#define BIN_SIZE 256

/**
 * @brief   Calculates block-histogram bin whose bin size is 256
 * @param   data  input data pointer
 * @param   sharedArray shared array for thread-histogram bins
 * @param   binResult block-histogram array
 */
__kernel
void histogram256(__global const uint4* data,
                  __local uchar* sharedArray,
                  __global uint* binResult)
{
    size_t localId = get_local_id(0);
    size_t globalId = get_global_id(0);
    size_t groupId = get_group_id(0);
    size_t groupSize = get_local_size(0);
    int offSet1 = localId & 31;    
    int offSet2 = 4 * offSet1;      //which element to access in one bank.
    int offSet3 = localId >> 5;     //bank number
    /* initialize shared array to zero */
	__local uchar4 * input = (__local uchar4*)sharedArray;
    for(int i = 0; i < 64; ++i)
        input[groupSize * i + localId] = 0;

    barrier(CLK_LOCAL_MEM_FENCE);


    /* calculate thread-histograms */
	/*uint4 value = data[groupId * groupSize *64  + localId];*/
    for(int i = 0; i < 64; i++)
    {
#ifdef LINEAR_MEM_ACCESS
       uint4 value =  data[groupId * groupSize * BIN_SIZE/4 + i * groupSize + localId];
#else
       uint4  value = data[globalId + i*4096]

#endif // LINEAR_MEM_ACCESS
	   sharedArray[value.s0 * 128 + offSet2 + offSet3]++;
	   sharedArray[value.s1 * 128 + offSet2 + offSet3]++;
	   sharedArray[value.s2 * 128 + offSet2 + offSet3]++;
	   sharedArray[value.s3 * 128 + offSet2 + offSet3]++;
    }
    barrier(CLK_LOCAL_MEM_FENCE); 
    
    /* merge all thread-histograms into block-histogram */

	for(int i = 0; i < BIN_SIZE / groupSize; ++i)
    {
        int passNumber = BIN_SIZE / 2 * 32 * i +  localId * 32 ;
		uchar4 binCount = 0;
		uint result= 0;
        for(int j = 0; j < 32; ++j)
		{
			int bankNum = (j + offSet1) & 31;   // this is bank number
            binCount += input[passNumber  +bankNum];
		}
        result = binCount.s0 + binCount.s1 + binCount.s2 + binCount.s3;
        binResult[groupId * BIN_SIZE + groupSize * i + localId ] = result;
	}
}


