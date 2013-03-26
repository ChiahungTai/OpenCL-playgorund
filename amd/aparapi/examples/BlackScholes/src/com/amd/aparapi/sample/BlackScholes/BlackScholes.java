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

package com.amd.aparapi.sample.BlackScholes;

import com.amd.AparapiUtil.*;
import com.amd.AparapiUtil.AparapiUtil.CmdArgsEnum;

import java.io.IOException;

import com.amd.aparapi.Kernel;
import com.amd.aparapi.Range;

/**
 ******************************************************************************* 
 * @file <BlackScholes.java>
 * 
 * @brief Demonstrates the implementation of Black-Scholes functionality by using 
 *        kernel class and overriding "run" method of it.
 ******************************************************************************/

/**
 * Class with the control code and an internal class that extends kernel class
 * 
 */
public class BlackScholes {

	AparapiUtil bsAparapiUtils;

	BlackScholesKernel blackscholesKernel;

	Range range;

	AparapiUtil.Option num_samples;

	final float maxPrecisionError = (float)0.01;

	/**
	 *  Maximum number of samples that are printed if quiet is not specified 
	 */
	final int maxSamplesPrint = 16;

	/**
	 * Method to compare floats of different precision. Values multiplied with
	 * multiplierFactor, converted to int and compared
	 * 
	 * @param array1
	 *            Input 1
	 * @param array2
	 *            Input 2
	 * @return false if mismatch, true otherwise
	 */
	boolean compareFloatArray(float[] array1, float[] array2) {
		float diffArrays;
		for (int i = 0; i < array1.length; i++) {
			diffArrays = Math.abs(array1[i] - array2[i]);
			if (diffArrays > maxPrecisionError) {
				return false;
			}
		}
		return true;
	}

	/**
	 * Method to crate AparapiUtil object and add a "samples" in command line
	 * options.
	 * 
	 * @return SDK_APARAPI_SUCCESS on success
	 * @throws IOException
	 */
	public AparapiUtil.ReturnStatus initialize() throws IOException {

		bsAparapiUtils = new AparapiUtil();
		AparapiUtil.Option num_samples_temp = new AparapiUtil.Option();

		num_samples_temp._sVersion = "x";
		num_samples_temp._lVersion = "samples";
		num_samples_temp._description = "Number of samples to be calculated";
		num_samples_temp._type = CmdArgsEnum.CA_ARG_STRING;
		num_samples_temp._value = "262144"; // 256*1024
		num_samples = num_samples_temp;
		bsAparapiUtils.AddOption(num_samples_temp);
		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
	}

	/**
	 * Used AparapiUtils to read all command line arguments
	 * 
	 * @param _args
	 *            Input arguments
	 * @return SDK_APARAPI_SUCCESS on success, SDK_APARAPI_FAILURE in case of
	 *         any errors
	 */
	public AparapiUtil.ReturnStatus parseCommandLine(String[] _args) {

		/* Parse input arguments and exit after printing usage */
		if (bsAparapiUtils.parse(_args) != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			bsAparapiUtils.help();
			return AparapiUtil.ReturnStatus.SDK_APARAPI_FAILURE;
		}
        if(Integer.parseInt(num_samples._value) < 1) {
            System.out.println("Error, num_samples cannot be 0 or negative. Exiting..\n");
    		System.exit(0);
        	
        }
		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;

	}

	/**
	 * Create "BlackScholes" object that in turns allocate various arrays. Also
	 * sets the specified device for execution
	 * 
	 * @return SDK_APARAPI_SUCCESS on success
	 */
	public AparapiUtil.ReturnStatus setup() {
		int size = Integer.parseInt(num_samples._value);
		range = Range.create(size);
		blackscholesKernel = new BlackScholesKernel(size);
		/* No output on console in quiet mode */
		if (Boolean.parseBoolean(bsAparapiUtils.quiet._value) != true) {
			System.out.println("Number of samples = " + size);
			System.out.println("Iterations = "
					+ Integer.parseInt(bsAparapiUtils.iterations._value));
		}

		/*
		 * Specify the device (CPU/GPU) if it is not same as mentioned in the
		 * command line option
		 */
		if (bsAparapiUtils.deviceType._value != "") {
			if (!bsAparapiUtils.deviceType._value.equals(blackscholesKernel
					.getExecutionMode().name())) {
				blackscholesKernel.setExecutionMode(Kernel.EXECUTION_MODE
						.valueOf(bsAparapiUtils.deviceType._value));
			}
		}
		bsAparapiUtils.createTimer();
		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
	}

	/**
	 * Method to execute BlackScholes algorithm over range of input. Prints out
	 * results if quiet mode not selected
	 * 
	 * @return SDK_APARAPI_SUCCESS on success
	 */
	public AparapiUtil.ReturnStatus runBlackScholes() {

		/* Loop over for number of iterations specified */

		bsAparapiUtils.startTimer();
		/* Work is performed here */
		blackscholesKernel.execute(range,
				Integer.parseInt((bsAparapiUtils.iterations._value)));
		bsAparapiUtils.stopTimer();

		if (Boolean.parseBoolean(bsAparapiUtils.quiet._value) != true) {
			blackscholesKernel.showResults(Math.min(maxSamplesPrint,
					Integer.parseInt(num_samples._value)));
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
		if ((Boolean.parseBoolean(bsAparapiUtils.verify._value) == true)) {
			int size = Integer.parseInt(num_samples._value);
			float refPut[] = new float[size];
			float refCall[] = new float[size];
			blackscholesKernel.refRun(refPut, refCall);
			if ((compareFloatArray(blackscholesKernel.put, refPut) == true)
					&& (compareFloatArray(blackscholesKernel.call, refCall) == true)) {
				System.out.println("Verification Passed!");
				return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
			} else {
				System.out.println("Verification Passed!");
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
	public AparapiUtil.ReturnStatus printStats() {
		/* Print out timing statistics */
		if (Boolean.parseBoolean(bsAparapiUtils.timing._value) == true) {
			System.out.format("\n%s : %d ms", "Total Execution time",
					bsAparapiUtils.getTotalTime());
			System.out
					.format("\n%s : %d ms",
							"Average Execution Time (including "
									+ "Bytecode to OpenCL Conversion time)",
							bsAparapiUtils.getAverageTime()
									/ Integer
											.parseInt((bsAparapiUtils.iterations._value)));
			System.out.format("\n%s : %d ms\n",
					"Bytecode to OpenCL Coversion time",
					blackscholesKernel.getConversionTime());
		}
		return AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS;
	}
	/**
	 * Entry point for the sample
	 * 
	 * @param _args
	 *            Input arguments
	 * @throws ClassNotFoundException
	 * @throws InstantiationException
	 * @throws IllegalAccessException
	 * @throws IOException
	 */
	public static void main(String[] _args) throws ClassNotFoundException,
			InstantiationException, IllegalAccessException, IOException {

		/* Create an object to BlackScholes class */
		BlackScholes objBlackscholes = new BlackScholes();

		/* Read input image, create view */
		if (objBlackscholes.initialize() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Initialization Error");
			System.exit(0);
		}

		/* Parse command line options */
		if (objBlackscholes.parseCommandLine(_args) != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Command line parsing error");
			System.exit(0);
		}

		/* Setup like setting right execute target */
		if (objBlackscholes.setup() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Setup Error");
			System.exit(0);
		}

		/* run BlackScholes on entire range of input */
		if (objBlackscholes.runBlackScholes() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Error in running BlackScholes");
			System.exit(0);
		}

		/* Verify with reference implementation if option mentioned */
		if (objBlackscholes.verifyResults() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Result verfication error");
			System.exit(0);
		}

		/* Print performance statistics */
		if (objBlackscholes.printStats() != AparapiUtil.ReturnStatus.SDK_APARAPI_SUCCESS) {
			System.out.println("Error in printing stats");
			System.exit(0);
		}
		System.out.println("Run Complete");
	}
}
