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


__kernel void copy1DFastPath(__global float *input, __global float *output)
{
	int gid = get_global_id(0);
	output[gid]=input[gid];
	return;
}

#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

__kernel void copy1DCompletePath(__global float *input, __global float *output)
{
	int gid = get_global_id(0);
	if(gid < 0)
		atom_add((__global int*)output, 1);
	output[gid]=input[gid];
	return;
}

#define WIDTH 1024
#define DATA_TYPE float
#define A(y,x) A[(y)* WIDTH +(x)]
#define C(y,x) C[(y)* WIDTH +(x)]
__kernel void copy2Dfloat(__global DATA_TYPE *A, __global DATA_TYPE *C)
{
	int idx = get_global_id(0);
	int idy = get_global_id(1);
	C(idy,idx) = A(idy,idx);
}


#define DATA_TYPE float4
__kernel void copy2Dfloat4(__global DATA_TYPE *A, __global DATA_TYPE *C)
{
	int idx = get_global_id(0);
	int idy = get_global_id(1);
	C(idy,idx) = A(idy,idx);
}

__kernel void copy1Dfloat4(__global float4 *input, __global float4 *output)
{
	int gid = get_global_id(0);
	output[gid]=input[gid];
	return;
}

//Shift by 16
__kernel void NoCoal(__global float* input, __global float* output)
{
	int gid = get_global_id(0) - 1;
	if((get_local_id(0) & 0xf) == 0)
		gid += 16;
	output[gid] = input[gid];
}

//Messed up access pattern
__kernel void Split(__global float* input, __global float* output)
{
	int gid = get_global_id(0);
	if((gid & 0x1) == 0)
	{
		gid = (gid & !63) + 62 - get_local_id(0);
	}
	output[gid] = input[gid];
}



/*
LDS has 32 banks, every bank is 4 byte wide, this kernel showed the bandwidth 
that the adjacent 32 threads in one wave access the same bank.
Every thread reads 128 elements.
*/

#define NUM_BANKS 32
#define PER_THREAD_READ 128
__kernel void localBankConflicts( __local float* share, __global float* output)
{
	int gid = get_global_id(0);
	int lid = get_local_id(0);
	int resulta = 0;
	int resultb = 0;
	for(int i =0; i < PER_THREAD_READ; i+=2 )
	{
		resulta += share[lid * NUM_BANKS + i];
		resultb += share[lid * NUM_BANKS + i + 1];
	}
	output[gid] = resulta + resultb;
}

/*
This kernel showed the bandwidth that  the adjacent 32 threads in one wave access 32 banks.
This can avoid local bank conflict.
*/
__kernel void noLocalBankConflicts( __local float* share, __global float* output)
{
	int gid = get_global_id(0);
	int lid = get_local_id(0);
	int resulta = 0;
	int resultb = 0;
	for(int i =0; i < PER_THREAD_READ; i+=2 )
	{
		resulta += share[i + lid];
		resultb += share[i + 1 + lid];
	}
	output[gid] = resulta + resultb;
}