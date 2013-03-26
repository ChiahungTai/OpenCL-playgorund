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

import com.amd.aparapi.Kernel;

/**
 ******************************************************************************* 
 * @file <BlackScholesKernel.java>
 * 
 * @brief Class that overrides Aparapi-kernel class. 
 ******************************************************************************/

/**
 * On invocation of kernel
 * blackScholes, each work thread calculates call price and put price values for
 * given stock price, option strike price, time to expiration date, risk free
 * interest and volatility factor.
 */
public class BlackScholesKernel extends Kernel {

    final float S_LOWER_LIMIT = 10.0f;

    final float S_UPPER_LIMIT = 100.0f;

    final float K_LOWER_LIMIT = 10.0f;

    final float K_UPPER_LIMIT = 100.0f;

    final float T_LOWER_LIMIT = 1.0f;

    final float T_UPPER_LIMIT = 10.0f;

    final float R_LOWER_LIMIT = 0.01f;

    final float R_UPPER_LIMIT = 0.05f;

    final float SIGMA_LOWER_LIMIT = 0.01f;

    final float SIGMA_UPPER_LIMIT = 0.10f;

    /**
     * @brief Abromowitz Stegun approximation for PHI (Cumulative Normal
     *        Distribution Function)
     * @param X
     *            Input value
     * @return Calculated CND of X
     */
    float phi(float X) {
        final float c1 = 0.319381530f;
        final float c2 = -0.356563782f;
        final float c3 = 1.781477937f;
        final float c4 = -1.821255978f;
        final float c5 = 1.330274429f;

        final float zero = 0.0f;
        final float one = 1.0f;
        final float two = 2.0f;
        final float temp4 = 0.2316419f;

        final float oneBySqrt2pi = 0.398942280f;

        float absX = abs(X);
        float t = one / (one + temp4 * absX);

        float y = one - oneBySqrt2pi * exp(-X * X / two) * t
                * (c1 + t * (c2 + t * (c3 + t * (c4 + t * c5))));

        float result = (X < zero) ? (one - y) : y;

        return result;
    }

    /**
     * @brief Calculates the call and put prices by using Black Scholes model
     *        for an input value.
     */
    @Override
    public void run() {
        float d1, d2;
        float phiD1, phiD2;
        float sigmaSqrtT;
        float KexpMinusRT;

        int gid = getGlobalId();
        float two = 2.0f;
        float inRand = randArray[gid];
        float S = S_LOWER_LIMIT * inRand + S_UPPER_LIMIT * (1.0f - inRand);
        float K = K_LOWER_LIMIT * inRand + K_UPPER_LIMIT * (1.0f - inRand);
        float T = T_LOWER_LIMIT * inRand + T_UPPER_LIMIT * (1.0f - inRand);
        float R = R_LOWER_LIMIT * inRand + R_UPPER_LIMIT * (1.0f - inRand);
        float sigmaVal = SIGMA_LOWER_LIMIT * inRand + SIGMA_UPPER_LIMIT
                * (1.0f - inRand);

        sigmaSqrtT = sigmaVal * sqrt(T);

        d1 = (log(S / K) + (R + sigmaVal * sigmaVal / two) * T) / sigmaSqrtT;
        d2 = d1 - sigmaSqrtT;

        KexpMinusRT = K * exp(-R * T);

        phiD1 = phi(d1);
        phiD2 = phi(d2);

        call[gid] = S * phiD1 - KexpMinusRT * phiD2;

        phiD1 = phi(-d1);
        phiD2 = phi(-d2);

        put[gid] = KexpMinusRT * phiD2 - S * phiD1;
    }

    /**
     * Reference implementation, without using any Aparapi functionality
     * 
     * @param refPut
     *            Array to store reference Put values
     * @param refCall
     *            Array to store reference Call values
     */
    public void refRun(float refPut[], float refCall[]) {
        for (int i = 0; i < size; i++) {
            float d1, d2;
            float phiD1, phiD2;
            float sigmaSqrtT;
            float KexpMinusRT;

            float two = 2.0f;
            float inRand = randArray[i];
            float S = S_LOWER_LIMIT * inRand + S_UPPER_LIMIT * (1.0f - inRand);
            float K = K_LOWER_LIMIT * inRand + K_UPPER_LIMIT * (1.0f - inRand);
            float T = T_LOWER_LIMIT * inRand + T_UPPER_LIMIT * (1.0f - inRand);
            float R = R_LOWER_LIMIT * inRand + R_UPPER_LIMIT * (1.0f - inRand);
            float sigmaVal = SIGMA_LOWER_LIMIT * inRand + SIGMA_UPPER_LIMIT
                    * (1.0f - inRand);

            sigmaSqrtT = sigmaVal * sqrt(T);

            d1 = (log(S / K) + (R + sigmaVal * sigmaVal / two) * T)
                    / sigmaSqrtT;
            d2 = d1 - sigmaSqrtT;

            KexpMinusRT = K * exp(-R * T);

            phiD1 = phi(d1);
            phiD2 = phi(d2);

            refCall[i] = S * phiD1 - KexpMinusRT * phiD2;

            phiD1 = phi(-d1);
            phiD2 = phi(-d2);

            refPut[i] = KexpMinusRT * phiD2 - S * phiD1;

        }
    }

    private float randArray[];

    public float put[];

    public float call[];

    public int size;

    /**
     * Constructor for BlackScholes class, create arrays for specified number of
     * samples
     * 
     * @param num_samples
     */
    public BlackScholesKernel(int num_samples) {
        size = num_samples;
        randArray = new float[size];
        call = new float[size];
        put = new float[size];
        size = num_samples;
        for (int i = 0; i < size; i++) {
            randArray[i] = i * 1.0f / size;
        }
    }

    /**
     * Method to print out an array
     * 
     * @param ary
     *            Input array
     * @param name
     *            Name to be printed
     */
    public void showArray(float ary[], String name, int printLength) {
        String line;
        line = name + ": ";
        for (int i = 0; i < printLength; i++) {
            if (i > 0)
                line += ", ";
            line += ary[i];
        }
        System.out.println(line);
    }

    /**
     * Method to print out call and put values that were calculated, uses
     * showArray internally
     */
    public void showResults(int printLength) {
        showArray(call, "Call Prices", printLength);
        showArray(put, "Put  Prices", printLength);
    }
}
