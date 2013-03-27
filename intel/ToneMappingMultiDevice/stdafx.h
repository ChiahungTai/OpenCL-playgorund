// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <memory.h>
#include <math.h>
#include <emmintrin.h>
#include "CL\cl.h"
#include <assert.h>
#include "utils.h"

//for perf. counters
#include <Windows.h>


typedef struct 
{
    float fPowKLow;                             //fPowKLow = pow( 2.0f, kLow)
    float fPowKHigh;                            //fPowKHigh = pow( 2.0f, kHigh)
    float fPow35;                               //fPow35 = pow( 2.0f, 3.5f)
    float fFStops;                              //F stops
    float fFStopsInv;                           //Invesrse fFStops value
    float fPowExposure;                         //fPowExposure = pow( 2.0f, exposure +  2.47393f )
    float fGamma;                               //Gamma correction parameter
    float fPowGamma;                            //Scale factor
    float fDefog;                               //Defog value
} CHDRData;

// TODO: reference additional headers your program requires here
