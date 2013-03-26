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

package com.amd.aparapi.sample.Mandel;

import com.amd.AparapiUtil.*;
import com.amd.AparapiUtil.AparapiUtil.CmdArgsEnum;

import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Point;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;

import javax.swing.JComponent;
import javax.swing.JFrame;
import javax.swing.WindowConstants;

import java.util.Random;

import com.amd.aparapi.Device;
import com.amd.aparapi.OpenCL;
import com.amd.aparapi.OpenCLDevice;
import com.amd.aparapi.OpenCLPlatform;
import com.amd.aparapi.Range;

/**
 ******************************************************************************* 
 * @file <mandel.java>
 * 
 * @brief An example Aparapi application which displays a view of the Mandelbrot
 *        set and lets the user zoom in to a particular point. When the user
 *        clicks on the view, this example application will zoom in to the
 *        clicked point and zoom out there after. If quiet or verify option is 
 *        specified on command line, the zoom in point is generated randomly. 
 *        On GPU, additional computing units will offer a better viewing 
 *        experience. On the other hand on CPU, this example application might 
 *        suffer with sub-optimal frame refresh rate as compared to GPU.
 ******************************************************************************/

@OpenCL.Resource("com/amd/aparapi/sample/Mandel/Mandel.cl")
interface MandelBrot extends OpenCL<MandelBrot> {
	MandelBrot createMandleBrot(//
			Range range,//
			@Arg("scale") float scale, //
			@Arg("offsetx") float offsetx, //
			@Arg("offsety") float offsety, //
			@GlobalWriteOnly("rgb") int[] rgb

	);
}

public class Mandel {

	/**
	 * User selected zoom-in point on the Mandelbrot view.
	 */
	public static volatile Point to = null;

	final Object userClickDoorBell = new Object();

	public static MandelBrot mandelBrot = null;

	public static MandelBrot gpuMandelBrot = null;

	public static MandelBrot javaMandelBrot = null;

	public static MandelBrot javaMandelBrotMultiThread = null;
	public AparapiUtil mandelAparapiUtils;
	JFrame frame = new JFrame("MandelBrot");
	JComponent viewer;
	/**
	 * Width of Mandelbrot view
	 */
	final int width = 768;

	/**
	 *  Height of Mandelbrot view. 
	 */
	final int height = 768;
	final int frames = 128;
	BufferedImage image;
	/**
	 * Used to make sure image buffered is not being written into while it is
	 * being
	 */
	Object framePaintedDoorBell;
	/**
	 * scale of MadelBrot image, lower scale indicates zoomed in view. Negative
	 * value would get a mirror view
	 */
	float scale;
	/**
	 *  horizontal offset from where image needs to be created 
	 */
	float offsetx;
	/**
	 *  vertical offset from where image needs to be created 
	 */
	float offsety;
	float defaultScale;
	/**
	 *  used to create a Range over size of image 
	 */
	Range range;
	/**
	 *  buffer to create mandelbrot image
	 */
	int[] imageRgb;
	/**
	 *  Total frames created
	 */
	int totalFramesCreated = 0;
	AparapiUtil.Option exec_platform = new AparapiUtil.Option();
	/**
	 * Due to precision difference, there will be some mismatches when CPU
	 * reference is against GPU output. This is the minimum percentage of pixels
	 * that need to match for verify to succeed
	 */
	final int expectedMatchPercent = 99;

	/**
	 * Method to run Java single threaded implementation and use it as a
	 * reference
	 * 
	 * @param refImageRgb
	 *            reference image
	 * @return SDK_APARAPI_SUCCESS on success
	 */
	public AparapiUtil.ReturnStatus refMandel(int[] refImageRgb) {
		JavaMandelBrot refJavaST = new JavaMandelBrot();
		refJavaST.createMandleBrot(range, scale, offsetx, offsety, refImageRgb);
		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
	}

	/**
	 * Adds additonal command line option supported, creates image buffer,
	 * creates frame that would be displayed and does some swing house keeping
	 * 
	 * @return SDK_APARAPI_SUCCESS on success
	 */
	@SuppressWarnings("serial")
	public AparapiUtil.ReturnStatus initialize() {

		mandelAparapiUtils = new AparapiUtil();
		AparapiUtil.Option exec_platform_temp = new AparapiUtil.Option();

		/*
		 * Add additional option called "exec_plat" and remove "device" options
		 * because the kernel can be run not just of GPU or CPU
		 */
		exec_platform_temp._sVersion = "x";
		exec_platform_temp._lVersion = "exec_plat";
		exec_platform_temp._description = "Execution platform [CPU|GPU|JAVAST|JAVAMT]";
		exec_platform_temp._type = CmdArgsEnum.CA_ARG_STRING;
		exec_platform_temp._value = "GPU";
		exec_platform = exec_platform_temp;
		mandelAparapiUtils.AddOption(exec_platform_temp);

		AparapiUtil.Option device_option_temp = new AparapiUtil.Option();
		device_option_temp._lVersion = "device";
		mandelAparapiUtils.DeleteOption(device_option_temp);
		device_option_temp._lVersion = "iterations";
		mandelAparapiUtils.DeleteOption(device_option_temp);

		frame = new JFrame("MandelBrot");

		/* Image for Mandelbrot view. */
		image = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);

		framePaintedDoorBell = new Object();
		viewer = new JComponent() {
			@Override
			public void paintComponent(Graphics g) {

				g.drawImage(image, 0, 0, width, height, this);
				synchronized (framePaintedDoorBell) {
					framePaintedDoorBell.notify();
				}
			}
		};
		/* Mouse listener which reads the user clicked zoom-in point on the
		   Mandelbrot view */
		viewer.addMouseListener(new MouseAdapter() {
			@Override
			public void mouseClicked(MouseEvent e) {
				to = e.getPoint();
				synchronized (userClickDoorBell) {
					userClickDoorBell.notify();
				}
			}
		});
		/* Set the size of JComponent which displays Mandelbrot image */
		viewer.setPreferredSize(new Dimension(width, height));

		// Swing housework to create the frame
		frame.getContentPane().add(viewer);
		frame.pack();
		frame.setLocationRelativeTo(null);
		frame.addWindowListener(new WindowAdapter() {

			// Shows code to Add Window Listener
			@Override
			public void windowClosing(WindowEvent e) {
				/* Print performance statistics */
				if (printStats() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
					System.out.println("Error in printing stats");
					System.exit(0);
				}
				System.exit(0);
			}
		});
		frame.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
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
		if (mandelAparapiUtils.parse(_args) != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			mandelAparapiUtils.help();
			return AparapiUtil.ReturnStatus.SDK_APARAPI_FAILURE;
		}
		if (!((exec_platform._value.equals("CPU"))
				|| (exec_platform._value.equals("GPU"))
				|| (exec_platform._value.equals("JAVAST")) || (exec_platform._value
					.equals("JAVAMT")))) {
			System.out
			.println("Error, Invalid execution platform. Exiting..\n");
			System.exit(0);
		}
		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;

	}

	/**
	 * This method selects an appropriate device and displays the first
	 * mandelbrot image
	 * 
	 * @return SDK_APARAPI_SUCCESS on success
	 * @throws InterruptedException
	 */
	public AparapiUtil.ReturnStatus setup() throws InterruptedException {

		frame.setVisible(true);

		/*
		 * Extract the underlying RGB buffer from the image. Pass this to the
		 * kernel so it operates directly on the RGB buffer of the image
		 */
		imageRgb = ((DataBufferInt) image.getRaster().getDataBuffer())
				.getData();

		scale = .0f;
		offsetx = .0f;
		offsety = .0f;
		Device device = null;
		OpenCLPlatform platform = null;
		for (OpenCLPlatform p : OpenCLPlatform.getPlatforms()) {
			if (p.getVendor().equals("Advanced Micro Devices, Inc.")) {
				platform = p;
				break;
			}
		}
		for (OpenCLDevice d : platform.getDevices()) {
			if (exec_platform._value.equals("GPU")) {
				if (d.getType().equals(Device.TYPE.GPU)) {
					device = d;
					break;
				}
			} else {
				if (d.getType().equals(Device.TYPE.CPU)) {
					device = d;
					break;
				}
			}
		}
		if (device == null) {
			System.out.println("Device not found");
			System.exit(0);
		}

		if (device instanceof OpenCLDevice) {
			OpenCLDevice openclDevice = (OpenCLDevice) device;

			/* print put the device memory size if not in quiet mode */
			if (Boolean.parseBoolean(mandelAparapiUtils.quiet._value) != true) {
				System.out.println("max memory = "
						+ openclDevice.getGlobalMemSize());
				System.out.println("max mem alloc size = "
						+ openclDevice.getMaxMemAllocSize());
			}
			gpuMandelBrot = openclDevice.bind(MandelBrot.class);
		}
		/* Select the right platform */
		if ((exec_platform._value.equals("GPU"))
				|| (exec_platform._value.equals("CPU"))) {
			mandelBrot = gpuMandelBrot;
		} else if (exec_platform._value.equals("JAVAST")) {
			javaMandelBrot = new JavaMandelBrot();
			mandelBrot = javaMandelBrot;
		} else {
			javaMandelBrotMultiThread = new JavaMandelBrotMultiThread();
			mandelBrot = javaMandelBrotMultiThread;
		}
		defaultScale = 3f;
		scale = defaultScale;
		offsetx = -1f;
		offsety = 0f;
		range = device.createRange2D(width, height);

		/* Create the first image */
		mandelBrot.createMandleBrot(range, scale, offsetx, offsety, imageRgb);
		viewer.repaint();

		mandelAparapiUtils.createTimer();
		if ((Boolean.parseBoolean(mandelAparapiUtils.quiet._value) == true)
				|| (Boolean.parseBoolean(mandelAparapiUtils.verify._value) == true)) {
			Thread.sleep(2000);
		}
		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
	}

	/**
	 * Method to generate mandelbrot images as it zooms in and out by modifying
	 * value of scale. The point of zoom in is generated randomly if "quiet" or 
	 * "verify" option is selected and taken from user click otherwise.
	 * 
	 * @return SDK_APARAPI_SUCCESS on success
	 */
	public AparapiUtil.ReturnStatus runMandel() {
		Random randomGenerator = new Random();
		Boolean waitForUser = true;
		int tox_int = randomGenerator.nextInt(width);
		int toy_int = randomGenerator.nextInt(height);
		float tox = (float) (tox_int - width / 2) / width * scale;
		float toy = (float) (toy_int - height / 2) / height * scale;
		float x;
		float y;

		/* Don't loop if quiet or verify option is selected*/
		if ((Boolean.parseBoolean(mandelAparapiUtils.quiet._value) == true)
				|| (Boolean.parseBoolean(mandelAparapiUtils.verify._value) == true)) {
			waitForUser = false;
		}
		do {

			if (waitForUser) {
				/* Wait for user input to select zoom in point*/
				while (to == null) {
					synchronized (userClickDoorBell) {
						try {
							userClickDoorBell.wait();
						} catch (InterruptedException ie) {
							ie.getStackTrace();
						}
					}
				}
				tox = (float) (to.x - width / 2) / width * scale;
				toy = (float) (to.y - height / 2) / height * scale;
			}
			x = -1f;
			y = 0f;

			/* This is how many frames we will display as we zoom in and out */
			for (int sign = -1; sign < 2; sign += 2) {
				totalFramesCreated += (frames - 4);
				for (int i = 0; i < frames - 4; i++) {
					scale = scale + sign * defaultScale / frames;
					x = x - sign * (tox / frames);
					y = y - sign * (toy / frames);
					offsetx = x;
					offsety = y;
					mandelAparapiUtils.startTimer();
					mandelBrot.createMandleBrot(range, scale, offsetx, offsety,
							imageRgb);
					mandelAparapiUtils.stopTimer();
					viewer.repaint();
					/*
					 * There will wait on framePaintedDoorBell.wait() till
					 * repaint is complete
					 */
					synchronized (framePaintedDoorBell) {
						try {
							framePaintedDoorBell.wait();
						} catch (InterruptedException ie) {
							ie.getStackTrace();
						}
					}
				}
			}
			// Reset zoom-in point.
			to = null;
		} while (waitForUser);

		frame.setVisible(false);
		frame.dispose();
		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
	}

	/**
	 * If verify option is specified then the result is compared against a
	 * JAVAST implementation. Due to precision difference, there will be some
	 * mismatches when CPU reference is against GPU output. If the percentage of
	 * matching pixels is more than "expectedMatchPercent" then it is considered
	 * as matching.
	 * 
	 * @return SDK_APARAPI_SUCCESS on success
	 */

	public AparapiUtil.ReturnStatus verifyResults() {
		int errorCount = 0;
		int totalPixels = width * height;
		if ((Boolean.parseBoolean(mandelAparapiUtils.verify._value) == true)) {
			BufferedImage refImage = new BufferedImage(width, height,
					BufferedImage.TYPE_INT_RGB);
			if (refMandel(((DataBufferInt) refImage.getRaster().getDataBuffer())
					.getData()) != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
				return AparapiUtil.ReturnStatus.SDK_APARAPI_FAILURE;
			}
			/* Calculate number of differences */
			for (int i = 0; i < totalPixels; i++) {
				if (((DataBufferInt) refImage.getRaster().getDataBuffer())
						.getData()[i] != ((DataBufferInt) image.getRaster()
						.getDataBuffer()).getData()[i]) {
					errorCount += 1;
				}
			}
			/*
			 * errorCount can be greater than 0 only if selected platform is GPU
			 * due to precision difference
			 */
			if ((errorCount > 0)
					&& exec_platform._value.equals("GPU")
					&& (((totalPixels - errorCount) * 100 / totalPixels) < expectedMatchPercent)) {
				System.out.println("Verification Failed!");
				return AparapiUtil.ReturnStatus.SDK_APARAPI_FAILURE;
			}
			System.out.println("Verification Passed!");
			return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
		}
		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
	}

	/**
	 * Method to print the statistics like timing information
	 * 
	 * @return SDK_APARAPI_SUCCESS
	 */
	public AparapiUtil.ReturnStatus printStats() {
		/* Print out timing statistics */
		if (Boolean.parseBoolean(mandelAparapiUtils.timing._value) == true) {
			System.out.println("\nFPS:" + (totalFramesCreated*1000)
					/ (mandelAparapiUtils.getTotalTime()));
			System.out.format("%s : %d ms", "Total Execution time",
					mandelAparapiUtils.getTotalTime());
			System.out.format("\n%s : %d ms\n", "Average Execution Time",
					mandelAparapiUtils.getAverageTime());
		}
		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
	}

	/**
	 * Entry point for the sample
	 * 
	 * @param _args
	 *            Input arguments
	 * @throws InterruptedException
	 */
	public static void main(String[] _args) throws InterruptedException {
		Mandel objMandel = new Mandel();

		/* Creates image buffer and frame that would be used for display */
		if (objMandel.initialize() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Initialization Error");
			System.exit(0);
		}

		/* Parse command line options */
		if (objMandel.parseCommandLine(_args) != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Command line parsing error");
			System.exit(0);
		}

		/* Setup like setting right execute target */
		if (objMandel.setup() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Setup Error");
			System.exit(0);
		}

		/* Generate frames and display */
		if (objMandel.runMandel() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Error in running mandel");
			System.exit(0);
		}

		/* Verify with reference implementation if option mentioned */
		if (objMandel.verifyResults() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Result verfication error");
			System.exit(0);
		}

		/* Print performance statistics */
		if (objMandel.printStats() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Error in printing stats");
			System.exit(0);
		}

		System.out.println("Run Complete");
	}
}
