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
********************************************************************************
* @file <ParReduction.cpp>                                              *
*                                                                              *
* @brief This is a C++ AMP introductory sample that implements a parallel      *
*   reduction algorithm.                                                       *
********************************************************************************
*/

#include <iostream>
#include <amp.h>
#include <stdlib.h>

using namespace concurrency;

const int TILE_SIZE		= 64;						/* Number of elements per tile */
const int NUM_TILES     = 512;						/* Total number of tiles */
const int GLOBAL_SIZE   = TILE_SIZE * NUM_TILES;    /* Total number of elements */

/********************************************************************************
* Execution of program begins from here                                         *
********************************************************************************/
void main()
{
	/****************************************************************************
    * Initialize the input data                                                 *
    *****************************************************************************/
	std::vector<int> data(GLOBAL_SIZE);
	for (int count = 0; count < GLOBAL_SIZE; count++)
		data[count] = count;
	/****************************************************************************
    * Create an array_view object to wrap around the input data                 *
    *****************************************************************************/
	array_view	<int, 1>	inputData(GLOBAL_SIZE, data);

	/****************************************************************************
	* Create the storage for the partial sums from the parallel reduction       *
	*****************************************************************************/
	std::vector<int> pSum(NUM_TILES, 0);
	array_view <int, 1> pOutput(NUM_TILES,pSum);

	parallel_for_each(inputData.extent.tile<TILE_SIZE>(),[=](tiled_index<TILE_SIZE> t_idx) restrict(amp)
	{
		/****************************************************************************
		* Each thread in a tile will copy one value from the input array into the   *
		* tile_static array                                                         *
		*****************************************************************************/
		tile_static int tileValues[TILE_SIZE];
		tileValues[t_idx.local[0]] = inputData[t_idx.global[0]];

		/****************************************************************************
		* synchronize all the threads in a tile                                     *
		*****************************************************************************/
		t_idx.barrier.wait();

		/****************************************************************************
		* Compute the partial sum in parallel. Each iteration reduce the number of  *
		* "working" thread by half.                                                 *
		*****************************************************************************/
		for (int i = TILE_SIZE/2; i >= 1; i = i/2)
		{
			if(t_idx.local[0] < i)
				tileValues[t_idx.local[0]] =  tileValues[t_idx.local[0]] + tileValues[t_idx.local[0] + i];

			t_idx.barrier.wait_with_tile_static_memory_fence();
		}

		/****************************************************************************
		* Store the partial sum of this tile into the output array_view             *
		*****************************************************************************/
		if(t_idx.local[0] == 0)
			pOutput[t_idx.tile[0]] = tileValues[0];
	});
	/****************************************************************************
	* Copy the partial sums to the CPU and add them together                    *
	*****************************************************************************/
	pOutput.synchronize();
	int sum = 0;
	for (int i = 0; i < NUM_TILES; i++)
		sum += pSum[i];
	std::cout << "The parallel reduction result is: " << sum << std::endl;

	/****************************************************************************
    * Compute the reduction in serial on the CPU                                *
    *****************************************************************************/
	int sSum = 0;
	for (int i = 0; i < GLOBAL_SIZE; i++)
		sSum += data[i];
	std::cout << "The serial reduction result is: " << sSum << std::endl << std::endl;

	/****************************************************************************
    * Compare the serial result with that from the previous parallel algorithm	*
    *****************************************************************************/
	if (sum == sSum)
		std::cout << "The two results are equal!" << std::endl;
	else
		std::cout << "The two results are not equal!" << std::endl;
}