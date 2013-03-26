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

package com.amd.aparapi.sample.Convolution;

import com.amd.AparapiUtil.*;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.Graphics;
import java.awt.GridLayout;
import java.awt.Toolkit;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;
import java.io.File;
import java.io.IOException;
import java.util.Arrays;

import javax.imageio.ImageIO;
import javax.swing.JComponent;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.WindowConstants;

import com.amd.aparapi.Kernel;

/**
 ******************************************************************************* 
 * @file <Convolution.java>
 * 
 * @brief Implements image manipulation using a convolution filter. The original 
 * 		  image along with NONE, BLUR and EMBOSS filtered image is displayed on 
 * 		  a 2x2 grid. Display window remains is closed if "verify" or "quiet"
 * 		  option is seelected  
 ******************************************************************************/

/**
 * This class demonstrates the usage of APARAPI for applying convolution filter
 * on an image
 */
public class Convolution {

	/** 
	 * NONE has filter parameters that leaves the image as it is 
	 */
	private static final ConvolutionFilter NONE = new ConvolutionFilter(0f, 0f,
			0f, 0f, 1f, 0f, 0f, 0f, 0f, 0);

	/**
	 * BLUR has filter parameters to blur the image, basically a low pass filter
	 */
	private static final ConvolutionFilter BLUR = new ConvolutionFilter(1f, 1f,
			1f, 1f, 1f, 1f, 1f, 1f, 1f, 0);

	/**
	 * EMBOSS has filter parameters to get embossing affect on image (raised
	 * image)
	 */
	private static final ConvolutionFilter EMBOSS = new ConvolutionFilter(-2f,
			-1f, 0f, -1f, 1f, 1f, 0f, 1f, 2f, 0);

	/**
	 *  convAparapiUtils is used for using utilities defined in aparapi_utils 
	 */
	private AparapiUtil convAparapiUtils;

	/**
	 * Used for displaying the test image after applying convolution filters on
	 * it
	 */
	JFrame frame;

	/**
	 *  Used for reading test image 
	 */
	BufferedImage testCard;

	/**
	 * Instance of the class that extends Aparapi "kernel" class and defines
	 * what is to be done
	 */
	ConvolutionKernel convolutionKernel;

	/**
	 *  Input image width rounded off to PAD 
	 */
	int widthPadded;

	/**
	 *  Input image height rounded off to PAD 
	 */
	int heightPadded;

	/**
	 *  Input image, padded width and height 
	 */
	BufferedImage inputImage;

	/**
	 *  Output of convolution when NONE filter is applied 
	 */
	BufferedImage outputImageNone;

	/** 
	 * Output of convolution when NONE filter is applied 
	 */
	BufferedImage outputImageBlur;

	/**
	 *  Output of convolution when NONE filter is applied 
	 */
	BufferedImage outputImageEmboss;

	/**
	 *  Output of reference implementation 
	 */
	BufferedImage refOutputImage;

	/**
	 *  Component to display original image 
	 */
	JComponent viewerOrig;

	/**
	 *  Component to display NONE filtered image 
	 */
	JComponent viewerNone;

	/**
	 *  Component to display BLUR filtered image 
	 */
	JComponent viewerBlur;

	/**
	 *  Component to display EMBOSS filtered image 
	 */
	JComponent viewerEmboss;

	/**
	 *  Height of image label 
	 */
	final int labelHeight = 15;

	/**
	 *  Width of each image displayed is half the screen 
	 */
	final int displayImageWidth = Toolkit.getDefaultToolkit().getScreenSize().width / 2;

	/**
	 * Height of each image displayed is half the screen after deducting for 2
	 * labelHeight
	 */
	final int displayImageHeight = (Toolkit.getDefaultToolkit().getScreenSize().height - 2 * labelHeight) / 2;

	/**
	 * Create array of convolution filters, will iterate over them for specified
	 * "iterations"
	 */
	ConvolutionFilter filters[];

	/**
	 *  Image padded to multiple of 1024 for width and height 
	 */
	public static final int PAD = 1024;

	/**
	 * Method to calculate number that need to be added to input value to make
	 * it multiple of "PAD"
	 * 
	 * @param value
	 *            Input value, may not be multiple of PAD
	 * @return Number that if added to "value" will make it multiple of PAD
	 */
	public static int padValue(int value) {
		if ((value % PAD) == 0) {
			return 0;
		} else {
			return (PAD - (value % PAD));
		}
	}

	/**
	 * Method to make "value" multiple of PAD
	 * 
	 * @param value
	 *            Input value, may not be multiple of PAD
	 * @return Padded value
	 */
	public static int padTo(int value) {
		return (value + padValue(value));
	}

	/**
	 * 4 images are displayed in a 2x2 grid, this method sets up each image for
	 * display along with the specified label
	 * 
	 * @param imageBuffer
	 *            Input buffer, can be original, NONE, BLUR or EMBOSS buffer
	 * @param labelDisplay
	 *            Label to be displayed for the specified image buffer
	 * @param convPanel
	 *            Panel that would have all 4 images
	 * 
	 * @return
	 */
	@SuppressWarnings("serial")
	public JComponent setupImageDisplay(BufferedImage imageBuffer,
			String labelDisplay, JPanel convPanel) {
		/* Local final BufferedImage that is needed to create the JComponent */
		final BufferedImage finalImageBuffer = imageBuffer;
		JComponent viewerImage = new JComponent() {
			@Override
			public void paintComponent(Graphics g) {
				g.drawImage(finalImageBuffer, 0, 0, displayImageWidth,
						displayImageHeight, 0, 0, widthPadded, heightPadded,
						this);
			}
		};
		/* Set the default size and add to the frames content pane */
		viewerImage.setPreferredSize(new Dimension(displayImageWidth,
				displayImageHeight));

		JLabel imageLabel = new JLabel(labelDisplay);
		/*
		 * Label has same width as image so that FlowLayout doesn't mess around
		 * for different sizes
		 */
		imageLabel.setPreferredSize(new Dimension(displayImageWidth,
				labelHeight));
		imageLabel.setHorizontalAlignment(JLabel.CENTER);
		JPanel imagePanel = new JPanel(new FlowLayout());
		imagePanel.add(imageLabel);
		imagePanel.add(viewerImage);

		convPanel.add(imagePanel);
		return viewerImage;
	}

	/**
	 * Read the input image, pads and creates view of input image
	 * 
	 * @return SDK_APARAPI_SUCCESS on success
	 * @throws IOException
	 */
	public AparapiUtil.ReturnStatus initialize() throws IOException {
		frame = new JFrame("Convolution");

		/* Image taken from testcard.jpg */
		testCard = ImageIO.read(new File("testcard.jpg"));

		int imageHeight = testCard.getHeight();

		int imageWidth = testCard.getWidth();

		/* width made multiple of PAD */
		widthPadded = padTo(imageWidth);

		/* height made multiple of PAD */
		heightPadded = padTo(imageHeight);

		/* Create image of RGB type */
		inputImage = new BufferedImage(widthPadded, heightPadded,
				BufferedImage.TYPE_INT_RGB);

		inputImage.getGraphics().drawImage(testCard, padValue(imageWidth) / 2,
				padValue(imageHeight) / 2, null);

		/* Output of APARAPI implemented convolution filter NONE */
		outputImageNone = new BufferedImage(widthPadded, heightPadded,
				BufferedImage.TYPE_INT_RGB);

		/* Output of APARAPI implemented convolution filter BLUR */
		outputImageBlur = new BufferedImage(widthPadded, heightPadded,
				BufferedImage.TYPE_INT_RGB);

		/* Output of APARAPI implemented convolution filter EMBOSS */
		outputImageEmboss = new BufferedImage(widthPadded, heightPadded,
				BufferedImage.TYPE_INT_RGB);

		/* Output of reference implementation */
		refOutputImage = new BufferedImage(widthPadded, heightPadded,
				BufferedImage.TYPE_INT_RGB);

		convolutionKernel = new ConvolutionKernel(widthPadded, heightPadded,
				inputImage);

		/* Create a component for viewing the off screen image */
		JPanel convPanel = new JPanel(new GridLayout(2, 2, 0, 0));

		viewerOrig = setupImageDisplay(inputImage, "Original Image", convPanel);
		viewerNone = setupImageDisplay(outputImageNone, "Filter Applied: NONE",
				convPanel);
		viewerBlur = setupImageDisplay(outputImageBlur, "Filter Applied: BLUR",
				convPanel);
		viewerEmboss = setupImageDisplay(outputImageEmboss,
				"Filter Applied: EMBOSS", convPanel);
		frame.add(convPanel);
		frame.setVisible(true);
		frame.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
		convAparapiUtils = new AparapiUtil();
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
		if (convAparapiUtils.parse(_args) != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			convAparapiUtils.help();
			return AparapiUtil.ReturnStatus.SDK_APARAPI_FAILURE;
		}

		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;

	}

	/**
	 * Declares various convolution filters and sets the right device that is to
	 * be used for execution
	 * 
	 * @return SDK_APARAPI_SUCCESS on success
	 */
	public AparapiUtil.ReturnStatus setup() {
		/*
		 * Create array of convolution filters, will iterate over them for
		 * specified "iterations"
		 */
		filters = new ConvolutionFilter[] { NONE, BLUR, EMBOSS, };
		/* Swing housekeeping */
		frame.pack();
		frame.setVisible(false);
		frame.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);

		/* No output on console in quiet mode */
		if (Boolean.parseBoolean(convAparapiUtils.quiet._value) != true) {
			System.out.println("image width,height=" + widthPadded + ","
					+ heightPadded);
		}

		/*
		 * Specify the device (CPU/GPU) if it is not same as mentioned in the
		 * command line option
		 */
		if (convAparapiUtils.deviceType._value != "") {
			if (!convAparapiUtils.deviceType._value.equals(convolutionKernel
					.getExecutionMode().name())) {
				convolutionKernel.setExecutionMode(Kernel.EXECUTION_MODE
						.valueOf(convAparapiUtils.deviceType._value));
			}
		}
		convAparapiUtils.createTimer();
		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
	}

	/**
	 * Method to do convolution over various filters for specified iterations
	 * 
	 * @return SDK_APARAPI_SUCCESS on success
	 * @throws InterruptedException
	 */
	public AparapiUtil.ReturnStatus runConvolution() {

		/* Loop over for number of iterations specified */
		for (int i = 0; i < Integer
				.parseInt(convAparapiUtils.iterations._value); i++) {

			/* Go over various filter types */
			convAparapiUtils.startTimer();
			/* Work is performed here */
			convolutionKernel.convolve(NONE, outputImageNone);
			convolutionKernel.convolve(BLUR, outputImageBlur);
			convolutionKernel.convolve(EMBOSS, outputImageEmboss);
			convAparapiUtils.stopTimer();
			/**
			 * Request a repaint of the viewer (causes paintComponent(Graphics)
			 * to be called later not synchronous
			 */
			viewerOrig.repaint();
			viewerNone.repaint();
			viewerBlur.repaint();
			viewerEmboss.repaint();
			if (i == 0)
				frame.setVisible(true);
		}

		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
	}

	/**
	 * If verify option is specified then the result is compared against a
	 * reference implementation
	 * 
	 * @return SDK_APARAPI_SUCCESS on success
	 */
	public AparapiUtil.ReturnStatus verifyResults() {
		/*
		 * If "verify" is set then compare the last output with CPU
		 * implementation
		 */
		if ((Boolean.parseBoolean(convAparapiUtils.verify._value) == true)) {
			convolutionKernel
					.referenceConvolution(((DataBufferInt) refOutputImage
							.getRaster().getDataBuffer()).getData());
			if (Arrays.equals(((DataBufferInt) refOutputImage.getRaster()
					.getDataBuffer()).getData(),
					((DataBufferInt) outputImageEmboss.getRaster()
							.getDataBuffer()).getData())) {
				System.out.println("Verification Passed!");
				return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
			} else {
				System.out.println("Verification Failed!");
				return AparapiUtil.ReturnStatus.SDK_APARAPI_FAILURE;
			}

		}
		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
	}

	/**
	 * Method to print the statistics like timing information
	 * 
	 * @return SDK_APARAPI_SUCCESS
	 */
	public AparapiUtil.ReturnStatus printStats() throws InterruptedException {
		/* Print out timing statistics */
		if (Boolean.parseBoolean(convAparapiUtils.timing._value) == true) {
			System.out.format("\n%s : %d ms", "Total Execution time",
					convAparapiUtils.getTotalTime());
			System.out.format("\n%s : %d ms",
					"Bytecode to OpenCL Conversion time",
					convolutionKernel.getConversionTime());
			System.out
					.format("\n%s : %d ms\n",
							"Average Execution Time (excluding Conversion time if iterations are more than 1)",
							convAparapiUtils.getAverageTime());
		}
		/* Close the display window if quiet or verify option is selected*/ 
		if ((Boolean.parseBoolean(convAparapiUtils.quiet._value) == true)
				|| (Boolean.parseBoolean(convAparapiUtils.verify._value) == true)) {
			/* Ensure filtered image is visible even if iterations are too few */
			Thread.sleep(10000);
			frame.setVisible(false);
			frame.dispose();
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

		/* Create an object to Convolution class */
		Convolution objConvolution = new Convolution();

		/* Read input image, create view */
		if (objConvolution.initialize() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Initialization Error");
			System.exit(0);
		}

		/* Parse command line options */
		if (objConvolution.parseCommandLine(_args) != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Command line parsing error");
			System.exit(0);
		}

		/* Setup like setting right execute target */
		if (objConvolution.setup() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Setup Error");
			System.exit(0);
		}

		/* Perform convolution and display */
		if (objConvolution.runConvolution() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Error in running convolution");
			System.exit(0);
		}

		/* Verify with reference implementation if option mentioned */
		if (objConvolution.verifyResults() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Result verfication error");
			System.exit(0);
		}

		/* Print performance statistics */
		if (objConvolution.printStats() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Error in printing stats");
			System.exit(0);
		}

		System.out.println("Run Complete");
	}
}
