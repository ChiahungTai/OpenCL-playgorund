/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file sseoperators.h
 *
 * @brief Defines operators for Streaming SIMD Extensions (SSE)
 */

#pragma once

#include <xmmintrin.h>
#include <emmintrin.h>

#ifndef __INTEL_COMPILER
  #pragma warning(disable : 4995)
#endif

/**
 * @fn inline __m128 castsi128_ps(__m128i in)
 *
 * @brief Casts a 128-bit integer vector to a 128-bit floating point vector
 *
 * @param in - Input signed 128-bit integer vector
 *
 * @return - __m128 floating point vector
 */
inline __m128 castsi128_ps(__m128i in)
{
	union {
		__m128i i;
		__m128	f;
	} tmp;
	tmp.i = in;
	return tmp.f;
}


/**
 * @fn inline  __m128i castps_si128(__m128 in)
 *
 * @brief Casts a 128-bit floating point vector to a 128-bit signed integer vector
 *
 * @param in - The floating point vector
 *
 * @return - 128-bit signed integer vector
 */
inline  __m128i castps_si128(__m128 in)
{
	union {
		__m128i i;
		__m128	f;
	} tmp;
	tmp.f = in;
	return tmp.i;
}

#ifndef __INTELL_COMPILER

// If we are not using the Intel compiler, we need to define some operations

/**
 * @def _mm_blendv_ps(v1, v2, f)
 *
 * @brief Blends floating point vectors
 *
 * @note Takes the shared elements of the first and third vectors and
 *       combines the result with the elements of the second vector not
 *       shared with the third vector
 *
 * @param v1 - The first vector
 *
 * @param v2 - The second vector
 *
 * @param f - The blending vector
 */
#define _mm_blendv_ps(v1, v2, f) _mm_or_ps(_mm_and_ps(f, v1), _mm_andnot_ps(f, v2))

/**
 * @def _mm_blendv_epi8(v1, v2, f)
 *
 * @brief Blends signed 128-bit integer vectors
 *
 * @note Takes the shared elements of the first and third vectors and
 *       combines the result with the elements of the second vector not
 *       shared with the third vector
 *
 * @param v1 - The first vector
 *
 * @param v2 - The second vector
 *
 * @param f - The blending vector
 */
#define _mm_blendv_epi8(v1, v2, f) _mm_or_si128(_mm_and_si128(f, v1), _mm_andnot_si128(f, v2))

#else

#include <smmintrin.h>

#endif

/**
 * @def sse_zero
 *
 * @brief Sets a vector to zero
 */
#define sse_zero _mm_setzero_ps()

/**
 * @var sse_one
 *
 * @brief Floating point vector with all values set to one (1.0)
 */
static const __m128		sse_one        =  _mm_set_ps1(1.0f);

/**
 * @var sse_two
 *
 * @brief Floating point vector with all values set to two (2.0)
 */
static const __m128 	sse_two        =	_mm_set_ps1(2.0f);

/**
 * @var sse_three
 *
 * @brief Floating point vector with all values set to three (2.0)
 */
static const __m128 	sse_three      =	_mm_set_ps1(3.0f);

/**
 * @var sse_minushalf
 *
 * @brief Floating point vector with all values set to negative one half (-0.5)
 */
static const __m128 	sse_minushalf  =	_mm_set_ps1(-0.5f);

/**
 * @var sse_half
 *
 * @brief Floating point vector with all values set to one half (0.5)
 */
static const __m128 	sse_half       =	_mm_set_ps1(0.5f);

/**
 * @var sse_quarter
 *
 * @brief Floating point vector with all values set to one quarter (0.25)
 */
static const __m128 	sse_quarter    =	_mm_set_ps1(0.25f);

/**
 * @var sse_rsqrt_eps
 *
 * @brief Floating point vector with all values set to 1.0e-30
 */
static const __m128 	sse_rsqrt_eps  =	_mm_set_ps1(1.0e-30f);


/**
 * @var sse_true
 *
 * @brief Floating point vector with all values set to TRUE
 */
static const __m128		sse_true     = _mm_cmpeq_ps(sse_one, sse_one);


/**
 * @var sse_sign
 *
 * @brief Floating point vector with all values set to 0x7FFFFFFF
 */
static const __m128		sse_sign     = castsi128_ps(_mm_set1_epi32(0x7fffffff));

/**
 * @var mask255
 *
 * @brief Floating point vector with all values set to 255
 */
static const __m128   mask255      = _mm_set_ps1(255);


/**
 * @var sse_mask
 *
 * @brief Floating point vector with all values set to 0x80000000
 *
 * @note Each value in this vector is one greater than in the sse_sign vector
 */
static const __m128		sse_mask     = castsi128_ps(_mm_set1_epi32(0x80000000));

/* ****************************** DEPRECATED CODE *************************** *\
    static const __m128		sse_full     = castsi128_ps(_mm_set1_epi32(0xFFFFFFFF));
\* ****************************** END DEPRECATED CODE *********************** */

/* ************************************************************************** *\
                Operator definitions for SIMD instructions
\* ************************************************************************** */

/**
 * @fn operator == (__m128 a, __m128 b)
 *
 * @brief Compares two floating point vectors for equality
 *
 * @note First computes for equality in the four single-precision,
 *       floating-point values of a and b such that:
 *       r0 = (a0 == b0) ? 0xffffffff : 0x0
 *       r1 = (a1 == b1) ? 0xffffffff : 0x0
 *       r2 = (a2 == b2) ? 0xffffffff : 0x0
 *       r3 = (a3 == b3) ? 0xffffffff : 0x0
 *       Then constructs a 4-bit mask from the most significant bits of
 *       the four single-precision, floating-point values
 *
 * @param a - The first floating point vector
 *
 * @param b - The second floating point vector
 *
 * @return - A 4-bit mask indicating the equality of the 4 elements of the vector
 *           where a set bit indicates equality of the matching elements
 */
inline int operator == (__m128 a, __m128 b)
{
	return _mm_movemask_ps(_mm_cmpeq_ps(a, b));
}


/**
 * @fn operator != (__m128 a, __m128 b)
 *
 * @brief Compares two floating point vectors for inequality
 *
 * @note First computes for inequality in the four single-precision,
 *       floating-point values of a and b such that:
 *       r0 = (a0 != b0) ? 0xffffffff : 0x0
 *       r1 = (a1 != b1) ? 0xffffffff : 0x0
 *       r2 = (a2 != b2) ? 0xffffffff : 0x0
 *       r3 = (a3 != b3) ? 0xffffffff : 0x0
 *       Then constructs a 4-bit mask from the most significant bits of
 *       the four single-precision, floating-point values
 *
 * @param a - The first floating point vector
 *
 * @param b - The second floating point vector
 *
 * @return - A 4-bit mask indicating the inequality of the 4 elements of the vector
 *           where a set bit indicates inequality of the matching elements
 */
inline int operator != (__m128 a, __m128 b)
{
	return _mm_movemask_ps(_mm_cmpneq_ps(a, b));
}


/**
 * @fn operator < (__m128 a, __m128 b)
 *
 * @brief Performs a less than comparison operation on two floating point vectors
 *
 * @note First computes for less than of the four single-precision,
 *       floating-point values of a and b such that:
 *       r0 = (a0 < b0) ? 0xffffffff : 0x0
 *       r1 = (a1 < b1) ? 0xffffffff : 0x0
 *       r2 = (a2 < b2) ? 0xffffffff : 0x0
 *       r3 = (a3 < b3) ? 0xffffffff : 0x0
 *       Then constructs a 4-bit mask from the most significant bits of
 *       the four single-precision, floating-point values
 *
 * @param a - The first floating point vector
 *
 * @param b - The second floating point vector
 *
 * @return - A 4-bit mask indicating the state of the 4 elements of the vector
 *           where a set bit indicates that the value of an element in the first vector
 *           is less than the value of the same element in the second vector.
 */
inline int operator < (__m128 a, __m128 b)
{
	return _mm_movemask_ps(_mm_cmplt_ps(a, b));
}


/**
 * @fn operator <= (__m128 a, __m128 b)
 *
 * @brief Performs a less than or equal to operation for two floating point vectors
 *
 * @note First computes for less than or equal to for the four single-precision,
 *       floating-point values of a and b such that:
 *       r0 = (a0 <= b0) ? 0xffffffff : 0x0
 *       r1 = (a1 <= b1) ? 0xffffffff : 0x0
 *       r2 = (a2 <= b2) ? 0xffffffff : 0x0
 *       r3 = (a3 <= b3) ? 0xffffffff : 0x0
 *       Then constructs a 4-bit mask from the most significant bits of
 *       the four single-precision, floating-point values
 *
 * @param a - The first floating point vector
 *
 * @param b - The second floating point vector
 *
 * @return - A 4-bit mask indicating the state of the 4 elements of the vector
 *           where a set bit indicates that the value of an element in the first vector
 *           is less than or equal to the value of the same element in the second vector.
 */
inline int operator <= (__m128 a, __m128 b)
{
	return _mm_movemask_ps(_mm_cmple_ps(a, b));
}


/**
 * @fn operator > (__m128 a, __m128 b)
 *
 * @brief Performs a greater than operator for two floating point vectors
 *
 * @note First computes for greater than of the four single-precision,
 *       floating-point values of a and b such that:
 *       r0 = (a0 > b0) ? 0xffffffff : 0x0
 *       r1 = (a1 > b1) ? 0xffffffff : 0x0
 *       r2 = (a2 > b2) ? 0xffffffff : 0x0
 *       r3 = (a3 > b3) ? 0xffffffff : 0x0
 *       Then constructs a 4-bit mask from the most significant bits of
 *       the four single-precision, floating-point values
 *
 * @param a - The first floating point vector
 *
 * @param b - The second floating point vector
 *
 * @return - A 4-bit mask indicating the state of the 4 elements of the vector
 *           where a set bit indicates that the value of an element in the first vector
 *           is greater than the value of the same element in the second vector.
 */
inline int operator > (__m128 a, __m128 b)
{
	return _mm_movemask_ps(_mm_cmpgt_ps(a, b));
}


/**
 * @fn operator >= (__m128 a, __m128 b)
 *
 * @brief Performs a greater than or equal to operation on two floating point vectors
 *
 * @note First computes the greater than or equal state of the four single-precision,
 *       floating-point values of a and b such that:
 *       r0 = (a0 >= b0) ? 0xffffffff : 0x0
 *       r1 = (a1 >= b1) ? 0xffffffff : 0x0
 *       r2 = (a2 >= b2) ? 0xffffffff : 0x0
 *       r3 = (a3 >= b3) ? 0xffffffff : 0x0
 *       Then constructs a 4-bit mask from the most significant bits of the four
 *       single-precision, floating-point values
 *
 * @param a - The first floating point vector
 *
 * @param b - The second floating point vector
 *
 * @return - A 4-bit mask indicating the state of the 4 elements of the vector
 *           where a set bit indicates that the value of an element in the first vector
 *           is greater than or equal to the value of the same element in the second vector.
 */
inline int operator >= (__m128 a, __m128 b)
{
	return _mm_movemask_ps(_mm_cmpge_ps(a, b));
}


/**
 * @fn operator & (__m128 a, __m128 b)
 *
 * @brief Performs a bit-wize AND of two floating point vectors
 *
 * @note Computes the bitwise AND of the four single-precision,
 *       floating-point values of a and b such that:
 *       r0 = a0 & b0
 *       r1 = a1 & b1
 *       r2 = a2 & b2
 *       r3 = a3 & b3
 *
 * @param a - The first floating point vector
 *
 * @param b - The second floating point vector
 *
 * @return - A floating point vector with the bit-wize AND combined vectors
 */
inline __m128 operator & (__m128 a, __m128 b)
{
	return _mm_and_ps(a, b);
}


/**
 * @fn operator | (__m128 a, __m128 b)
 *
 * @brief Performs a bit-wize OR of two floating point vectors
 *
 * @note Computes the bitwise OR of the four single-precision,
 *       floating-point values of a and b such that:
 *       r0 = a0 | b0
 *       r1 = a1 | b1
 *       r2 = a2 | b2
 *       r3 = a3 | b3
 *
 * @param a - The first floating point vector
 *
 * @param b - The second floating point vector
 *
 * @return - A floating point vector with the bit-wize OR combined vectors
 */
inline __m128 operator | (__m128 a, __m128 b)
{
	return _mm_or_ps(a, b);
}


/**
 * @fn operator + (__m128 a, __m128 b)
 *
 * @brief Performs an addition two floating point vectors
 *
 * @note Adds the four single-precision, floating-point values of a and b such that:
 *       r0 = a0 + b0
 *       r1 = a1 + b1
 *       r2 = a2 + b2
 *       r3 = a3 + b3
 *
 * @param a - The first floating point vector
 *
 * @param b - The second floating point vector
 *
 * @return - A floating point vector with the sum of the two vectors
 */
inline __m128 operator + (__m128 a, __m128 b)
{
	return _mm_add_ps(a, b);
}


/**
 * @fn operator - (__m128 a, __m128 b)
 *
 * @brief Subtracts the second floating point vector from the first floating point vector
 *
 * @note Subtracts the four single-precision, floating-point values of a and b such that:
 *       r0 = a0 - b0
 *       r1 = a1 - b1
 *       r2 = a2 - b2
 *       r3 = a3 - b3
 *
 * @param a - The first floating point vector
 *
 * @param b - The second floating point vector
 *
 * @return - A floating point vector with the difference between the two vectors
 */
inline __m128 operator - (__m128 a, __m128 b)
{
	return _mm_sub_ps(a, b);
}


/**
 * @fn operator * (__m128 a, __m128 b)
 *
 * @brief Multiplies two floating point vectors
 *
 * @note Multiplies the four single-precision, floating-point values of a and b such that:
 *       r0 = a0 * b0
 *       r1 = a1 * b1
 *       r2 = a2 * b2
 *       r3 = a3 * b3
 *
 * @param a - The first floating point vector
 *
 * @param b - The second floating point vector
 *
 * @return - A floating point vector with the product of the two vectors
 */
inline __m128 operator * (__m128 a, __m128 b)
{
	return _mm_mul_ps(a, b);
}


/**
 * @fn operator / (__m128 a, __m128 b)
 *
 * @brief Divides the first floating point vector by the second floating point vector
 *
 * @note Divides the four single-precision, floating-point values of a and b such that:
 *       r0 = a0 / b0
 *       r1 = a1 / b1
 *       r2 = a2 / b2
 *       r3 = a3 / b3
 *
 * @param a - The first floating point vector
 *
 * @param b - The second floating point vector
 *
 * @return - A floating point vector with the quotient of the two vectors
 */
inline __m128 operator / (__m128 a, __m128 b)
{
	return _mm_div_ps(a, b);
}


/**
 * @fn operator &= (__m128 a, __m128 b)
 *
 * @brief Performs a bit-wize AND of two floating point vectors,
 *        Alters the value of the first floating point vector
 *
 * @note Computes the bitwise AND of the four single-precision,
 *       floating-point values of a and b such that:
 *       a0 = a0 & b0
 *       a1 = a1 & b1
 *       a2 = a2 & b2
 *       a3 = a3 & b3
 *
 * @param a - The first floating point vector
 *
 * @param b - The second floating point vector
 *
 * @return - A floating point vector with the bit-wize AND combined vectors
 */
inline const __m128& operator &= (__m128& a, __m128 b)
{
	a = _mm_and_ps(a, b);
	return a;
}


/**
 * @fn operator |= (__m128 a, __m128 b)
 *
 * @brief Performs a bit-wize OR of two floating point vectors,
 *        Alters the value of the first floating point vector
 *
 * @note Computes the bitwise OR of the four single-precision,
 *       floating-point values of a and b such that:
 *       a0 = a0 | b0
 *       a1 = a1 | b1
 *       a2 = a2 | b2
 *       a3 = a3 | b3
 *
 * @param a - The first floating point vector
 *
 * @param b - The second floating point vector
 *
 * @return - A floating point vector with the bit-wize OR combined vectors
 */
inline const __m128& operator |= (__m128& a, __m128 b)
{
	a = _mm_or_ps(a, b);
	return a;
}


/**
 * @fn operator += (__m128 a, __m128 b)
 *
 * @brief Performs an addition two floating point vectors,
 *        Alters the value of the first floating point vector
 *
 * @note Adds the four single-precision, floating-point values of a and b such that:
 *       a0 = a0 + b0
 *       a1 = a1 + b1
 *       a2 = a2 + b2
 *       a3 = a3 + b3
 *
 * @param a - The first floating point vector
 *
 * @param b - The second floating point vector
 *
 * @return - A floating point vector with the sum of the two vectors
 */
inline const __m128& operator += (__m128& a, __m128 b)
{
	a = _mm_add_ps(a, b);
	return a;
}


/**
 * @fn operator -= (__m128 a, __m128 b)
 *
 * @brief Subtracts the second floating point vector from the first floating point vector,
 *        Alters the value of the first floating point vector
 *
 * @note Subtracts the four single-precision, floating-point values of a and b such that:
 *       a0 = a0 - b0
 *       a1 = a1 - b1
 *       a2 = a2 - b2
 *       a3 = a3 - b3
 *
 * @param a - The first floating point vector
 *
 * @param b - The second floating point vector
 *
 * @return - A floating point vector with the difference between the two vectors
 */
inline const __m128& operator -= (__m128& a, __m128 b)
{
	a = _mm_sub_ps(a, b);
	return a;
}


/**
 * @fn operator *= (__m128 a, __m128 b)
 *
 * @brief Multiplies two floating point vectors,
 *        Alters the value of the first floating point vector
 *
 * @note Multiplies the four single-precision, floating-point values of a and b such that:
 *       a0 = a0 * b0
 *       a1 = a1 * b1
 *       a2 = a2 * b2
 *       a3 = a3 * b3
 *
 * @param a - The first floating point vector
 *
 * @param b - The second floating point vector
 *
 * @return - A floating point vector with the product of the two vectors
 */
inline const __m128& operator *= (__m128& a, __m128 b)
{
	a = _mm_mul_ps(a, b);
	return a;
}


/**
 * @fn abs(__m128 a)
 *
 * @brief Finds the absolute value of a floating point vector
 *
 * @note Computes the bitwise AND of the four single-precision,
 *       floating-point values of a and the pre-defined sse_sign
 *       vector whose values are all 0X7FFFFFFF so that the sign
 *       bit is always zero such that:
 *       r0 = a0 & 0X7FFFFFFF
 *       r1 = a1 & 0X7FFFFFFF
 *       r2 = a2 & 0X7FFFFFFF
 *       r3 = a3 & 0X7FFFFFFF
 *
 * @param a - The floating point vector
 *
 * @return - A floating point vector with absolute values
 */
inline __m128 abs(__m128 a)
{
	return _mm_and_ps(a, sse_sign);
}


/**
 * @fn rcp(__m128 a)
 *
 * @brief Finds the reciprocal of a floating point vector
 
 * @note Computes the approximations of reciprocals of the four single-precision,
 *       floating-point values of a such that:
 *       b0 = recip(a0)
 *       b1 = recip(a1)
 *       b2 = recip(a2)
 *       b3 = recip(a3)
 *       Then adjusts the vector such that r = b(2 - ab). Since ab = 1, we have
 *       r = 2b - b = b. 
 *
 * @param a - The floating point vector
 *
 * @return The reciprocal vector
 */
inline __m128 rcp(__m128 a)
{
	__m128 rcpa = _mm_rcp_ps(a);
	return ((rcpa+rcpa)-((a*rcpa)*rcpa));
}
