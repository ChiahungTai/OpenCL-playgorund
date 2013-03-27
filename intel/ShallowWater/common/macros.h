/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file macros.h
 *
 * @desc Defines a number of macros used for calculations and check number alignment
 */

#ifndef _MACROS_H
#define _MACROS_H

// Forced to be the first include, a workaround for _CRTDBG_MAP_ALLOC causing error
// stdlib.h(488) : error C2059: syntax error : 'constant'
#include <stdlib.h> 
#include <assert.h>


//standard
//#include <stdio.h>

// engine
#include "udef.h"

/**
 * @def NON_COPYABLE(name)
 *
 * @brief Use this macro in the class to avoid warning that the copy constructor and operator= is not defined.
 */
#define NON_COPYABLE(name) name(const name&); name& operator=(const name&);

/**
 * @def DIM(a)
 *
 * @brief A macro to calculate dimensions of the array a[]
 */
#ifndef DIM
#define DIM(a) (sizeof((a)) / sizeof(*(a)))
#endif

/**
 * @fn NextPowOf2(int nNumber)
 *
 * @brief   Calculates the next power of two number, the input number should be 16 bit.
 *          Example: 25 -> 32, 64 -> 64.
 *
 * @param nNumber - The input number
 *
 * @return - The next power of two number folloing the input number
 *
 * @see NextPowOf2Log()
 */
inline int NextPowOf2(int nNumber)
{
    assert(nNumber <= 0xFFFF);   // fails => add one more line
    
    // Start example number         // 0 1000 0000 0000 0001
    nNumber--;                      // 0 1000 0000 0000 0000
    nNumber |= nNumber >> 1;        // 0 1100 0000 0000 0000
    nNumber |= nNumber >> 2;        // 0 1111 0000 0000 0000
    nNumber |= nNumber >> 4;        // 0 1111 1111 0000 0000
    nNumber |= nNumber >> 8;        // 0 1111 1111 1111 1111

    return nNumber + 1;
}

/**
 * @fn NextPowOf2Log(int nNumber)
 *
 * @brief Gets the exponent of the next power of two number greater than nNumber
 *
 * @param nNumber - The input number
 *
 * @return - The exponent of the next power of two number
 */
inline int NextPowOf2Log(int nNumber)
{
    assert(nNumber <= 0xFFFF);
    float rFloat;
    int nExponent;
    int n(nNumber);
    
    // Start example number    // 0 1000 0000 0000 0001
    n--;                       // 0 1000 0000 0000 0000
    n |= n >> 1;               // 0 1100 0000 0000 0000
    n |= n >> 2;               // 0 1111 0000 0000 0000
    n |= n >> 4;               // 0 1111 1111 0000 0000
    n |= n >> 8;               // 0 1111 1111 1111 1111
    rFloat = (float)(n + 1);   // 1 0000 0000 0000 0000

    nExponent = (((*(int *)&rFloat) & 0x7F800000) >> 23) - 127;
    assert((1 << nExponent) == (n + 1));

    return nExponent;
}

/**
 * @fn IsPtrAligned16(T * p)
 *
 * @brief Check to see if a pointer aligned to 16 bytes
 *
 * @param p - The pointer to check the alignment of
 *
 * @return - TRUE if pointer is aligned and FALSE otherwise
 */
template <typename T>
inline bool IsPtrAligned16(T * p)
{
    return (*((int*)&p) & 0xF) == 0;
}

/**
 * @fn IsPtrAligned32(T * p)
 *
 * @brief Check to see if a pointer aligned to 32 bytes
 *
 * @param p - The pointer to check the alignment of
 *
 * @return - TRUE if pointer is aligned and FALSE otherwise
 */
template <typename T>
inline bool IsPtrAligned32(T * p)
{
    return (*((int*)&p) & 0x1F) == 0;
}

#ifndef _WIN32
/**
 * @def __min
 * @def __max
 *
 * @brief Provides __min and __max definition for a unix compiler
 */
#ifndef __min
#define __min(a, b)    ( ((a) < (b)) ? (a) : (b) )
#endif
#ifndef __max
#define __max(a, b)    ( ((a) > (b)) ? (a) : (b) )
#endif

#endif    

#endif // _MACROS_H
