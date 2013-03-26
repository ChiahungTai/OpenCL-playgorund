/**********************************************************************
Copyright 2012 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

package com.amd.aparapi.sample.Life;

import com.amd.aparapi.Kernel;

import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;

import com.amd.aparapi.Range;

/**
 ******************************************************************************* 
 * @file <LifeKernel.java>
 * 
 * @brief Implements the core functionality including the kernel i.e. core 
 *	  algorithm. Called by "Life" to do all the work.
 ******************************************************************************/


/**
 * Class that extends the "Kernel" class (defined in aparapi.jar). This Class
 * overrides "Kernel.run()", run() defines what gets executed for each input
 * value in the image
 */

public class LifeKernel extends Kernel {
	
	private static final int ALIVE = 0xffffff;

	private static final int DEAD = 0;

	public int[] imageData;

	private int width;

	private int height;

	private final Range range;

	public int fromBase;

	private int toBase;

	
	/**
     * LifeKernel constructor, sets the image parameters
     * 
     * @param _width
     *            Width of the image
     * @param _height
     *            Height of the image
     * @param _inputImage
     */
	
	public LifeKernel(int _width, int _height, BufferedImage _image) {
    
		imageData = ((DataBufferInt) _image.getRaster().getDataBuffer()).getData();
		width = _width;
		height = _height;
		range = Range.create(width * height, 256);

		fromBase = height * width;
		toBase = 0;
		setExplicit(true); // This gives us a performance boost

		/** draw a line across the image **/
		for (int i = width * (height / 2) + width / 10; i < width * (height / 2 + 1) - width / 10; i++) {
			imageData[i] = LifeKernel.ALIVE;
		}

		put(imageData); // Because we are using explicit buffer management we must put the imageData array

  }

	/**
    * @brief Implements the Life algorithm for one pixel
    */

	@Override public void run() {
		int gid = getGlobalId();
		int to = gid + toBase;
		int from = gid + fromBase;
		int x = gid % width;
		int y = gid / width;
		
		if ((x == 0 || x == width - 1 || y == 0 || y == height - 1)) {
			// This pixel is on the border of the view, just keep existing value
			imageData[to] = imageData[from];
		} else {
			// Count the number of neighbors.  We use (value&1x) to turn pixel value into either 0 or 1
			int neighbors = (imageData[from - 1] & 1) + // EAST
				  (imageData[from + 1] & 1) + // WEST
				  (imageData[from - width - 1] & 1) + // NORTHEAST                 
				  (imageData[from - width] & 1) + // NORTH
				  (imageData[from - width + 1] & 1) + // NORTHWEST
				  (imageData[from + width - 1] & 1) + // SOUTHEAST
				  (imageData[from + width] & 1) + // SOUTH
				  (imageData[from + width + 1] & 1); // SOUTHWEST

			// The game of life logic
			if (neighbors == 3 || (neighbors == 2 && imageData[from] == ALIVE)) {
			   imageData[to] = ALIVE;
			} else {
			   imageData[to] = DEAD;
			}
		}
	}
	
	
	/**
	    * @brief Swaps the top & bottom half and calls "execute"
    */

	public void nextGeneration() {
		// swap fromBase and toBase
		int swap = fromBase;
		fromBase = toBase;
		toBase = swap;

		execute(range);
	}
 
	/**
     * Reference implementation, without using any Aparapi functionality
     * 
     * @param _image
     *            image to implement the life algorithm 
     * @param iterations
     *            number of iterations to repeat over the image
     */
	
	public void referencelife( BufferedImage _image, int iterations) {
		
		int[] imageData2 = ((DataBufferInt) _image.getRaster().getDataBuffer()).getData();
		fromBase = height * width;
		toBase = 0;

		// Initialize image
		for (int i = width * (height / 2) + width / 10; i < width * (height / 2 + 1) - width / 10; i++) {
			imageData2[i] = LifeKernel.ALIVE;
		}
	 
		// Run over specified iterations
		for(int iter = 0; iter < iterations; iter++){
			
			int swap = fromBase;
			fromBase = toBase;
			toBase = swap;
			
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					int gid = y * width + x;
					int to = gid + toBase;
					int from = gid + fromBase;
					int px = gid % width;
					int py = gid / width;

					if ((px == 0 || px == width - 1 || py == 0 || py == height - 1)) {
						// This pixel is on the border of the view, just keep existing value
						imageData2[to] = imageData2[from];
					} else {
						// Count the number of neighbors.  We use (value&1x) to turn pixel value into either 0 or 1
						int neighbors = (imageData2[from - 1] & 1) + // EAST
						   (imageData2[from + 1] & 1) + // WEST
						   (imageData2[from - width - 1] & 1) + // NORTHEAST                 
						   (imageData2[from - width] & 1) + // NORTH
						   (imageData2[from - width + 1] & 1) + // NORTHWEST
						   (imageData2[from + width - 1] & 1) + // SOUTHEAST
						   (imageData2[from + width] & 1) + // SOUTH
						   (imageData2[from + width + 1] & 1); // SOUTHWEST

						// The game of life logic
						if (neighbors == 3 || (neighbors == 2 && imageData2[from] == ALIVE)) {
							imageData2[to] = ALIVE;
						} else {
							imageData2[to] = DEAD;
						}
					}
				}
			}
		}
	}
}	
