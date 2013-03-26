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

import com.amd.AparapiUtil.*;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.Graphics;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;
import java.io.IOException;
import java.util.Arrays;
import java.util.List;

import javax.swing.JComponent;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.WindowConstants;

import com.amd.aparapi.Kernel;
import com.amd.aparapi.ProfileInfo;
import java.awt.Toolkit;

/**
 ******************************************************************************* 
 * @file <Life.java>
 * 
 * @brief Implements the game of Life. 
 * 
 * An example Aparapi application which demonstrates Conways 'Game Of Life'.
 * 
 ******************************************************************************/

/**
 * This class demonstrates the usage of APARAPI for the algorithm on the game of
 * life on an image
 */
public class Life {

	/**
	 * LifeKernel represents the data parallel algorithm describing by Conway's
	 * game of life.
	 * 
	 * http://en.wikipedia.org/wiki/Conway's_Game_of_Life
	 * 
	 * We examine the state of each pixel and its 8 neighbors and apply the
	 * following rules.
	 * 
	 * if pixel is dead (off) and number of neighbors == 3 { pixel is turned on
	 * } else if pixel is alive (on) and number of neighbors is neither 2 or 3
	 * pixel is turned off }
	 * 
	 * We use an image buffer which is 2*width*height the size of screen and we
	 * use fromBase and toBase to track which half of the buffer is being
	 * mutated for each pass. We basically copy from getGlobalId()+fromBase to
	 * getGlobalId()+toBase;
	 * 
	 * 
	 * Prior to each pass the values of fromBase and toBase are swapped.
	 * 
	 */

	private AparapiUtil lifeAparapiUtils;

	/**
	 * Used for displaying the output image
	 */
	JFrame frame;

	/**
	 * Instance of the class that extends Aparapi "kernel" class and defines
	 * what is to be done
	 */
	LifeKernel lifeKernel;

	/**
	 * Input image
	 */
	BufferedImage image;

	/**
	 * Output of APARAPI implemented algorithm
	 */
	BufferedImage outputImage;

	/**
	 * Output of reference implementation
	 */
	BufferedImage refOutputImage;

	JComponent viewer;
	JLabel generationsPerSecond;

	JPanel controlPanel;

	final int width = Toolkit.getDefaultToolkit().getScreenSize().width - 128;
	final int height = Toolkit.getDefaultToolkit().getScreenSize().height - 128;

	int totalFramesGenerated = 0;
	/**
	 * Read the input image, pads and creates view of input image
	 * 
	 * @return SDK_APARAPI_SUCCESS on success
	 * @throws IOException
	 */
	@SuppressWarnings("serial")
	public AparapiUtil.ReturnStatus initialize() throws IOException {

		frame = new JFrame("Game of Life");

		// Buffer is twice the size as the screen. We will alternate between
		// mutating data from top to bottom
		// and bottom to top in alternate generation passses. The LifeKernel
		// will track which pass is which
		// final BufferedImage
		image = new BufferedImage(width, height * 2, BufferedImage.TYPE_INT_RGB);

		lifeKernel = new LifeKernel(width, height, image);

		// Create a component for viewing the offscreen image
		viewer = new JComponent() {
			@Override
			public void paintComponent(Graphics g) {
				if (lifeKernel.isExplicit()) {
					lifeKernel.get(lifeKernel.imageData); // We only pull the
															// imageData when we
															// intend to use it.
					List<ProfileInfo> profileInfo = lifeKernel.getProfileInfo();
					if (profileInfo != null) {
						for (ProfileInfo p : profileInfo) {
							System.out
									.print(" " + p.getType() + " "
											+ p.getLabel() + " "
											+ (p.getStart() / 1000) + " .. "
											+ (p.getEnd() / 1000) + " "
											+ (p.getEnd() - p.getStart())
											/ 1000 + "us");
						}
					}
				}
				// We copy one half of the offscreen buffer to the viewer, we
				// copy the half that we just mutated.
				if (lifeKernel.fromBase == 0) {
					g.drawImage(image, 0, 0, width, height, 0, 0, width,
							height, this);
				} else {
					g.drawImage(image, 0, 0, width, height, 0, height, width,
							2 * height, this);
				}
			}
		};

		/* Instantiate Aparapi utilities */
		lifeAparapiUtils = new AparapiUtil();

		// Set number of iterations by default to 1000
		lifeAparapiUtils.iterations._value = "1000";

		// Setup Display
		controlPanel = new JPanel(new FlowLayout());
		frame.getContentPane().add(controlPanel, BorderLayout.SOUTH);

		// Set the default size and add to the frames content pane
		viewer.setPreferredSize(new Dimension(width, height));
		frame.getContentPane().add(viewer);

		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
	}

	/**
	 * Used aparapi_uitls to read all command line arguments
	 * 
	 * @param _args
	 *            Input arguments
	 * @return SDK_APARAPI_SUCCESS on success, SDK_APARAPI_FAILURE in case of
	 *         any errors
	 */
	public AparapiUtil.ReturnStatus parseCommandLine(String[] _args) {

		/* Parse input arguments and exit after printing usage */
		if (lifeAparapiUtils.parse(_args) != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			lifeAparapiUtils.help();
			return AparapiUtil.ReturnStatus.SDK_APARAPI_FAILURE;
		}

		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;

	}

	/**
	 * Sets the right device that is to be used for execution
	 * 
	 * @return SDK_APARAPI_SUCCESS on success
	 */
	public AparapiUtil.ReturnStatus setup() {
		/* Swing housekeeping */
		frame.pack();
		frame.setVisible(true);
		frame.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
		if (Boolean.parseBoolean(lifeAparapiUtils.quiet._value) != true) {
			System.out.println("width = " + width);
			System.out.println("height = " + height);
		}
		long start = System.currentTimeMillis();
		long generations = 0;
		try {
			Thread.sleep(10);
			viewer.repaint();
		} catch (InterruptedException e1) {
			e1.printStackTrace();
		}

		generations++;
		long now = System.currentTimeMillis();
		if (now - start > 1000) {
			generationsPerSecond.setText(String.format("%5.2f",
					(generations * 1000.0) / (now - start)));
			start = now;
			generations = 0;
		}

		/*
		 * Specify the device (CPU/GPU) if it is not same as mentioned in the
		 * command line option
		 */
		if (lifeAparapiUtils.deviceType._value != "") {
			if (!lifeAparapiUtils.deviceType._value.equals(lifeKernel
					.getExecutionMode().name())) {
				lifeKernel.setExecutionMode(Kernel.EXECUTION_MODE
						.valueOf(lifeAparapiUtils.deviceType._value));
			}
		}

		// Set up labels for display
		controlPanel.add(new JLabel("Execution Mode: " + lifeKernel.getExecutionMode().toString()));
		controlPanel.add(new JLabel("          Generations/Second="));
		generationsPerSecond = new JLabel("0.00");
		controlPanel.add(generationsPerSecond);

		// In verify mode, set iterations to 10
		if (lifeAparapiUtils.verify._value != "") {
			lifeAparapiUtils.iterations._value = "10";
			System.out.println("In --verify mode, setting iterations to: "
					+ lifeAparapiUtils.iterations._value);
		}

		lifeAparapiUtils.createTimer();
		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
	}

	/**
	 * Method to invoke the Life Kernel
	 * 
	 * @return SDK_APARAPI_SUCCESS on success
	 * @throws InterruptedException
	 */
	public AparapiUtil.ReturnStatus runLife() throws InterruptedException {
		/* Loop over for number of iterations specified */

		long generations = 0;
		long start = System.currentTimeMillis();

		// Invoke the Life algorithm over the iterations
		for (int i = 0; i < Integer
				.parseInt(lifeAparapiUtils.iterations._value); i++) {

			/* Start Timer for stats */
			lifeAparapiUtils.startTimer();

			lifeKernel.nextGeneration(); // Work is performed here

			/* Stop timer */
			lifeAparapiUtils.stopTimer();

			generations++;
			long now = System.currentTimeMillis();
			if ((now - start > 1000) || (i == Integer
					.parseInt(lifeAparapiUtils.iterations._value)-1)) {
				generationsPerSecond.setText(String.format("%5.2f",
						(generations * 1000.0) / (now - start)));
				start = now;
				totalFramesGenerated += generations; 
				generations = 0;
			}

			/**
			 * Request a repaint of the viewer (causes paintComponent(Graphics)
			 * to be called later not synchronous
			 */
			viewer.repaint();
			if (i == 0)
				frame.setVisible(true);
		}

		Thread.sleep(1000);
		frame.setVisible(false);
		frame.dispose();
		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
	}

	/**
	 * If verify option is specified then the result is compared against a
	 * reference implementation
	 * 
	 * @return SDK_APARAPI_SUCCESS on success
	 */

	public AparapiUtil.ReturnStatus verifyResults() {

		if ((Boolean.parseBoolean(lifeAparapiUtils.verify._value) == true)) {

			outputImage = new BufferedImage(width, height * 2,
					BufferedImage.TYPE_INT_RGB);

			// Call Reference code for verification
			lifeKernel.referencelife(outputImage,
					Integer.parseInt(lifeAparapiUtils.iterations._value));

			if (Arrays.equals(((DataBufferInt) outputImage.getRaster()
					.getDataBuffer()).getData(), ((DataBufferInt) image
					.getRaster().getDataBuffer()).getData())) {
				System.out.println("Verification Passed!");
			} else {
				System.out.println("Verification Failed!");
			}
		}

		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
	}

	/**
	 * Method to print the statistics like timing information
	 * 
	 * @return SDK_APARAPI_SUCCESS
	 */
	public AparapiUtil.ReturnStatus printStats() {
		/* Print out timing statistics if not quiet mode */
		System.out.println(totalFramesGenerated+"\n");
		if (Boolean.parseBoolean(lifeAparapiUtils.timing._value) == true) {
			System.out.println("\nFPS:" + (totalFramesGenerated*1000)
					/ (lifeAparapiUtils.getTotalTime()));
			System.out.format("%s : %d ms\n", "Total Execution time",
					lifeAparapiUtils.getTotalTime());
			System.out.format("%s : %d ms\n", "Average Execution Time",
					lifeAparapiUtils.getAverageTime());
			System.out.format("%s : %d ms\n\n",
					"Bytecode to OpenCL Coversion time",
					lifeKernel.getConversionTime());

		}
		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
	}

	/**
	 * Entry point for the sample
	 * 
	 * @param _args
	 *            Input arguments
	 * @throws IOException
	 * @throws InterruptedException
	 */
	public static void main(String[] _args) throws IOException,
			InterruptedException {

		/* Create an object to Life class */
		Life objLife = new Life();

		/* Setup display image, create view */
		if (objLife.initialize() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Initialization Error");
			return;
		}

		/* Parse command line options */
		if (objLife.parseCommandLine(_args) != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Command line parsing error");
			return;
		}

		/* Setup like setting right execute target */
		if (objLife.setup() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Setup Error");
			return;
		}

		/* Perform Life and display */
		if (objLife.runLife() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Error in running life");
			return;
		}

		/* Verify with reference implementation if option mentioned */
		if (objLife.verifyResults() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Result verfication error");
			return;
		}

		/* Print performance statistics */
		if (objLife.printStats() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Error in printing stats");
			return;
		}

		System.out.println("Run Complete");
	}
}
