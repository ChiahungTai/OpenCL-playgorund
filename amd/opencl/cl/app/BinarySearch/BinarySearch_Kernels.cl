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

/**
 * One instance of this kernel call is a thread.
 * Each thread finds out the segment in which it should look for the element.
 * After that, it checks if the element is between the lower bound and upper bound
 * of its segment. If yes, then this segment becomes the total searchspace for the next pass.
 *
 * To achieve this, it writes the lower bound and upper bound to the output array.
 * In case the element at the left end (lower bound) matches the element we are looking for,
 * That is marked in the output and we no longer need to look any further.
 */
__kernel void
binarySearch(        __global uint4 * outputArray,
             __const __global uint2  * sortedArray, 
             const   unsigned int findMe)
{
    unsigned int tid = get_global_id(0);

    /* Then we find the elements  for this thread */
    uint2 element = sortedArray[tid];

    /* If the element to be found does not lie between them, then nothing left to do in this thread */
    if( (element.x > findMe) || (element.y < findMe)) 
    {
        return;
    }
    else
    {
        /* However, if the element does lie between the lower and upper bounds of this thread's searchspace
         * we need to narrow down the search further in this search space 
         */
 
        /* The search space for this thread is marked in the output as being the total search space for the next pass */
        outputArray[0].x = tid;
        outputArray[0].w = 1;

    }
}


__kernel void
binarySearch_mulkeys(__global int *keys,
                    __global uint *input,
                    const unsigned int numKeys,
                    __global int *output)
{
    int gid = get_global_id(0);
    int lBound = gid * 256;
    int uBound = lBound + 255;
    //int uBound
    

    for(int i = 0; i < numKeys; i++)
    {
        
        if(keys[i] >= input[lBound] && keys[i] <=input[uBound])
            output[i]=lBound;       
        
    }
    
}


__kernel void
binarySearch_mulkeysConcurrent(__global uint *keys,
                    __global uint *input,
                    const unsigned int inputSize, // number of input elements
                    const unsigned int numSubdivisions,
                    __global int *output)
{
    int lBound = (get_global_id(0) % numSubdivisions) * (inputSize / numSubdivisions);
    int uBound = lBound + inputSize / numSubdivisions;
    int myKey = keys[get_global_id(0) / numSubdivisions];
    int mid;
    while(uBound >= lBound)
    {
        mid = (lBound + uBound) / 2;
        if(input[mid] == myKey)
        {
            output[get_global_id(0) / numSubdivisions] = mid;
            return;
        }
        else if(input[mid] > myKey)
            uBound = mid - 1;
        else
            lBound = mid + 1;
    }
}
