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

package com.amd.AparapiUtil;

import java.lang.Boolean;
import java.util.*;

/**
 ******************************************************************************* 
 * @file <AparapiUtils.java>
 * 
 * @brief Implements Utilities used across Aparapi samples
 ******************************************************************************/

/**
 * Class to just keep version information
 */
class VersionClass {
    private static final int SDK_VERSION_MAJOR = 2;
    private static final int SDK_VERSION_MINOR = 8;
    private static final int SDK_VERSION_BUILD = 1;
    private static final int SDK_VERSION_REVISION = 1;

    /**
     * Method to print the version number as described by above member elements
     */
    public void printSDKVersion() {
        System.out.println("AMD-APP-SDK-v" + SDK_VERSION_MAJOR + "."
                + SDK_VERSION_MINOR + " (" + SDK_VERSION_BUILD + "."
                + SDK_VERSION_REVISION + ")");
    }
}

/**
 * Main class, samples should create an object of this class type to use the
 * utilities
 */
public class AparapiUtil {
    private int _numArgs;
    /**
     *  number of arguments 
     */
    private Option _options[];
    /** 
     * struct option object 
     */
    public Option deviceType;
    public Option timing;
    public Option quiet;
    public Option verify;
    public Option iterations;
    public Option version;
    public Option help;
    public LinkedList<Long> timerLinkedList;
    public long startTime;

    /**
     * Return status used for various methods
     */
    public static enum ReturnStatus {
        SDK_APARAPI_SUCCESS, SDK_APARAPI_FAILURE
    };

    /**
     * Enum for datatype of Command Line Arguments, for example for "-t" needs
     * no value while "--iterations" needs value All values stored in String
     * irrespecitve of type
     */
    public static enum CmdArgsEnum {
        CA_ARG_STRING, CA_NO_ARGUMENT
    };

    /**
     * Option class is used to keep information for a command line argument
     * option One instance kept for each option supported
     */
    public static class Option {
        public String _sVersion;
        public String _lVersion;
        public String _description;
        public CmdArgsEnum _type;
        public String _value;

    };

    /**
     * AparapiUtils constructor allocates and initializes all default options
     */
    public AparapiUtil() {
        int defaultOptions = 7;
        _numArgs = defaultOptions;

        _options = new Option[defaultOptions];
        for (int i = 0; i < defaultOptions; i++) {
            _options[i] = new Option();
        }
        /* -h, --help option */
        _options[0]._sVersion = "h";
        _options[0]._lVersion = "help";
        _options[0]._description = "Display this information";
        _options[0]._type = CmdArgsEnum.CA_NO_ARGUMENT;
        _options[0]._value = "";
        help = _options[0];

        /* --device option */
        _options[1]._sVersion = "";
        _options[1]._lVersion = "device";
        _options[1]._description = "Execute the openCL kernel on a device [CPU|GPU]";
        _options[1]._type = CmdArgsEnum.CA_ARG_STRING;
        _options[1]._value = "";
        deviceType = _options[1];

        /* -q, --quiet option */
        _options[2]._sVersion = "q";
        _options[2]._lVersion = "quiet";
        _options[2]._description = "Quiet mode. Suppress all text output.";
        _options[2]._type = CmdArgsEnum.CA_NO_ARGUMENT;
        _options[2]._value = "";
        quiet = _options[2];

        /* -v, --verify option */
        _options[3]._sVersion = "e";
        _options[3]._lVersion = "verify";
        _options[3]._description = "Verify results against reference implementation.";
        _options[3]._type = CmdArgsEnum.CA_NO_ARGUMENT;
        _options[3]._value = "";
        verify = _options[3];

        /* -t, --timing option */
        _options[4]._sVersion = "t";
        _options[4]._lVersion = "timing";
        _options[4]._description = "Print timing.";
        _options[4]._type = CmdArgsEnum.CA_NO_ARGUMENT;
        _options[4]._value = "";
        timing = _options[4];

        /* -v, --version option */
        _options[5]._sVersion = "v";
        _options[5]._lVersion = "version";
        _options[5]._description = "AMD APP SDK version string.";
        _options[5]._type = CmdArgsEnum.CA_NO_ARGUMENT;
        _options[5]._value = "";
        version = _options[5];

        /* -i, --iterations option */
        _options[6]._sVersion = "i";
        _options[6]._lVersion = "iterations";
        _options[6]._description = "Number of iterations for kernel execution.";
        _options[6]._type = CmdArgsEnum.CA_ARG_STRING;
        _options[6]._value = "10";
        iterations = _options[6];

    }

    /**
     * Method to match user arguments to supported options. Called once for each
     * argument specified, also reads the value for the option if needed
     * 
     * @param argv
     *            String array having all user arguments
     * @param argc
     *            Current string to be read
     * @param totalArgc
     *            Total number of arguments specified by the user
     * @return Number of strings used. 1 if options doeesn't need value, 2
     *         otherwise
     */
    private int match(String argv[], int argc, int totalArgc) {
        int matched = 0;
        Boolean shortVer = true;
        String argCurr = argv[totalArgc - argc];
        argCurr = argCurr.toLowerCase();
        /* Long version of argument */
        if (argCurr.startsWith("--")) {
            shortVer = false;
            argCurr = argCurr.substring(2);
        }
        /* Short version of argument */
        else if (argCurr.startsWith("-")) {
            argCurr = argCurr.substring(1);
        }
        /* Invalid format */
        else {
            return 0;
        }
        /* Loop over all arguments to find a match */
        for (int count = 0; count < _numArgs; count++) {
            if (shortVer) {
                if (_options[count]._sVersion.equals(argCurr)) {
                    matched = 1;
                }
            } else {
                if (_options[count]._lVersion.equals(argCurr)) {
                    matched = 1;
                }
            }
            /*
             * Once a match is found then we check if follow up value is needed
             * for that argument. Read that argument if needed
             */
            if (matched == 1) {
                /*
                 * No follow up value needed for the option, for eg -t for
                 * printing timing
                 */
                if (_options[count]._type == CmdArgsEnum.CA_NO_ARGUMENT) {
                    _options[count]._value = "true";
                } else {
                    /* Check if any value is there at all or not */
                    if (argc > 1) {
                        _options[count]._value = argv[totalArgc - argc + 1].toUpperCase();
                        matched += 1;
                    } else {
                        System.err.println("Error. Missing argument for "
                                + argCurr);
                        System.exit(0);
                        return matched;
                    }
                }
                break;
            }
        }
        return matched;
    }

    /**
     * AddOption new option CmdLine Options
     * 
     * @param op
     *            Option object
     * @return SDK_APARAPI_SUCCESS on success, SDK_APARAPI_FAILURE on failure
     */
    public ReturnStatus AddOption(Option op) {
        if (op == null) {
            System.out.println("Error. Cannot add option, NULL input");
            return ReturnStatus.SDK_APARAPI_FAILURE;
        } else {
            /*
             * Keep current options in "save", allocate new option count
             * (_numArgs+1), move all previous options from "save" to new
             * _options and add new option at the end
             */
            Option[] save;
            if (_options != null)
                save = _options;
            else
                return ReturnStatus.SDK_APARAPI_FAILURE;
            _options = new Option[_numArgs + 1];
            if (_options == null) {
                System.out.println("Error. Cannot add option ");
                System.out.println(". Memory allocation error");
                return ReturnStatus.SDK_APARAPI_FAILURE;
            }
            for (int i = 0; i < _numArgs; ++i) {
                _options[i] = save[i];
            }
            _options[_numArgs] = op;
            _numArgs++;
        }

        return ReturnStatus.SDK_APARAPI_SUCCESS;
    }

    /**
     * DeleteOption Function to a delete CmdLine Option
     * 
     * @param op
     *            Option object
     * @return SDK_APARAPI_SUCCESS on success SDK_APARAPI_FAILURE on failure
     */
    public ReturnStatus DeleteOption(Option op) {
        if (op == null) {
            System.out.println("Error. Cannot delete option.");
            System.out.println("Null pointer or empty option list");
            return ReturnStatus.SDK_APARAPI_FAILURE;
        }

        /*
         * Search all current options, if match is found then it is taken out
         * and all subsequent options are moved up
         */
        for (int i = 0; i < _numArgs; i++) {
            if (op._sVersion == _options[i]._sVersion
                    || op._lVersion == _options[i]._lVersion) {
                for (int j = i; j < _numArgs - 1; j++) {
                    _options[j] = _options[j + 1];
                }
                _numArgs--;
            }
        }

        return ReturnStatus.SDK_APARAPI_SUCCESS;
    }

    /**
     * parse Function to parse the Cmdline Options
     * 
     * @param argv
     *            array of strings of CmdLine Options
     * @return SDK_APARAPI_SUCCESS on success SDK_APARAPI_FAILURE on failure
     */

    public ReturnStatus parse(String[] p_argv) {
        int matched = 0;
        int argc;
        int p_argc;
        String[] argv;

        p_argc = p_argv.length;
        argc = p_argc;
        argv = p_argv;
        /*
         * Go over all the options specified, matching them with options
         * supported. Return with SDK_APARAPI_FAILURE if match is not found for
         * any option
         */
        while (argc > 0) {
            matched = match(argv, argc, p_argc);
            if (matched < 1) {
                return ReturnStatus.SDK_APARAPI_FAILURE;
            }
            argc -= matched;
        }
        /* If -h or --help is an argument then print out the help and exit */
        if (Boolean.parseBoolean(help._value) == true) {
            help();
            System.exit(0);
        }
        /* If -v or --version is an argument then print out the version and exit */
        if (Boolean.parseBoolean(version._value) == true) {
            VersionClass versionObject = new VersionClass();
            versionObject.printSDKVersion();
            System.exit(0);
        }
        if (!(deviceType._value.equals("")) && !((deviceType._value.equals("CPU")) || deviceType._value
                .equals("GPU"))) {
            System.out.println("Error. Invalid device options. ");
            System.out.println("only CPU or GPU supported ");
            help();
            System.exit(0);
        }
		if (Integer.parseInt(iterations._value) < 1) {
			System.out
					.println("Error, iterations cannot be 0 or negative. Exiting..\n");
			System.exit(0);
		}
        	
        return ReturnStatus.SDK_APARAPI_SUCCESS;
    }

    /**
     * help method to print out usage
     */
    public void help() {
        String result = "";

        for (int count = 0; count < _numArgs; count++) {
            String line = "";

            if (_options[count]._sVersion.length() > 0) {
                line = "-" + _options[count]._sVersion + ", ";
            }

            line += "--" + _options[count]._lVersion + "\t"
                    + _options[count]._description + "\n";

            result += line;
        }

        System.out.println(result);
    }

    /**
     * Method to create link list to keep timing information
     */
    public void createTimer() {
        timerLinkedList = new LinkedList<Long>();
    }

    /**
     * Method to capture the current time as start time
     */
    public void startTimer() {
        startTime = System.nanoTime();
    }

    /**
     * This method captures the current time as end time and adds the
     * information in the timeLinkedList
     */
    public void stopTimer() {
        long endTime = System.nanoTime();
        long timeTakenms = (endTime - startTime) / 1000000;
        timerLinkedList.add((long) timeTakenms);
    }

    /**
     * This method adds up all the time measurements and returns the total value
     * 
     * @return Total Sum of all time measurements
     */
    public long getTotalTime() {
        long totalTime = 0;
        for (long timeValues : timerLinkedList) {
            totalTime += timeValues;
        }
        return totalTime;
    }

    /**
     * This method averages out all the time measurements
     * 
     * @return Average of all time measurements if number is greater than 1,
     *         first value if number is 1 and 0 if no value is captured
     */
    public long getAverageTime() {
        long totalTime = 0;
        if (timerLinkedList.size() > 1) {
            for (int i = 1; i < timerLinkedList.size(); i++) {
                totalTime += timerLinkedList.get(i);
            }
            return totalTime / (timerLinkedList.size() - 1);
        } else if (timerLinkedList.size() > 0) {
            return timerLinkedList.get(0);
        }
        return 0;

    }

}
