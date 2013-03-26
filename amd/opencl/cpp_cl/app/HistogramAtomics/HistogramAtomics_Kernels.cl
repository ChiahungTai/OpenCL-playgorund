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

#define NBINS        256
#define BITS_PER_PIX 8


#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable

#define MIN(a,b) ((a) < (b)) ? (a) : (b)
#define MAX(a,b) ((a) > (b)) ? (a) : (b)

#define NBANKS 16

// histogramKernel work group size must be integer multiple of 256

__kernel __attribute__((reqd_work_group_size(256,1,1)))
void histogramKernel(__global uint4 *Image,
                     __global uint  *Histogram,
                              uint  n4VectorsPerThread)
{
    __local uint subhists[NBANKS * NBINS];
    
    uint tid     = get_global_id(0);
    uint ltid    = get_local_id(0);
    uint Stride  = get_global_size(0);

    uint i, idx;
    uint4 temp, temp2;
	
    const uint shft = (uint) BITS_PER_PIX;
    const uint msk =  (uint) (NBINS-1);
    uint offset = (uint) ltid % (uint) (NBANKS);

    uint lmem_items = NBANKS * NBINS;
    uint lmem_items_per_thread;
    uint lmem_max_threads;

    // parallel LDS clear
    
    // first, calculate threads per item, at least 1:
   lmem_max_threads = MIN( 1, get_local_size(0) / lmem_items );	
    // but no more than we have items:
    lmem_max_threads = MAX( 1, lmem_max_threads / lmem_items );
    // calculate threads total:
    lmem_max_threads = lmem_items / lmem_max_threads;
    // but no more than LDS banks:
    lmem_max_threads = MIN( get_local_size(0), lmem_max_threads );

    lmem_items_per_thread = lmem_items / lmem_max_threads;

    // now, clear LDS
    __local uint4 *p = (__local uint4 *) subhists;
    
    if( ltid < lmem_max_threads )
    {
       for(i=0, idx=ltid; i<lmem_items_per_thread/4; i++, idx+=lmem_max_threads)
       {
          p[idx] = 0;
       }
    }

    barrier( CLK_LOCAL_MEM_FENCE );
    
    // read & scatter phase
    
    for( i=0; i<n4VectorsPerThread; i++)
    {
       temp = Image[tid + i * Stride];
       temp2 = (temp & msk) * (uint4) NBANKS + offset;

        atom_inc( subhists + temp2.x );
        atom_inc( subhists + temp2.y );
        atom_inc( subhists + temp2.z );
        atom_inc( subhists + temp2.w );

       temp = temp >> shft;
       temp2 = (temp & msk) * (uint4) NBANKS + offset;

        atom_inc( subhists + temp2.x );
        atom_inc( subhists + temp2.y );
        atom_inc( subhists + temp2.z );
        atom_inc( subhists + temp2.w );

       temp = temp >> shft;
       temp2 = (temp & msk) * (uint4) NBANKS + offset;
       
        atom_inc( subhists + temp2.x );
        atom_inc( subhists + temp2.y );
        atom_inc( subhists + temp2.z );
        atom_inc( subhists + temp2.w );

       temp = temp >> shft;
       temp2 = (temp & msk) * (uint4) NBANKS + offset;
       
        atom_inc( subhists + temp2.x );
        atom_inc( subhists + temp2.y );
        atom_inc( subhists + temp2.z );
        atom_inc( subhists + temp2.w );
    }

    barrier( CLK_LOCAL_MEM_FENCE );

    // reduce __local banks to single histogram per work-group
    
    if( ltid < NBINS )
    {
       uint bin = 0;

       for( i=0; i<NBANKS; i++ )
       {
          bin += subhists[ (ltid * NBANKS) + i ];
       }

       Histogram[ (get_group_id(0) * NBINS) + ltid ] = bin;
    }
}

__kernel void reduceKernel( __global uint *Histogram, uint nSubHists )
{
    uint tid = get_global_id(0);
    uint bin = 0;
      
    // Reduce work-group histograms into single histogram,
    // one thread for each bin.
 

	 for( int i=0; i < nSubHists; i = i + 4)
       bin += Histogram[ i * NBINS + tid ] + Histogram[ i * NBINS + tid + NBINS ] + 
	   Histogram[ i * NBINS + tid + 2 * NBINS ] + Histogram[ i * NBINS + tid + 3 * NBINS];

    Histogram[ tid ] = bin;
}
