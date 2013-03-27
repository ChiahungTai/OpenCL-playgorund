/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file udef.h
 *
 * @brief Used to define data types, common events, and other functions
 */

#pragma once

#ifndef _UDef_H_
#define _UDef_H_

// Data type definitions
typedef __int64             llong;
typedef unsigned __int64    ullong;
typedef unsigned __int64    uint64;
typedef unsigned __int32    uint32;
typedef unsigned __int16    uint16;

typedef unsigned short int  ushort;
typedef unsigned long       ulong;
typedef unsigned int        uint;
typedef unsigned char       uchar;
typedef unsigned char       byte;

// Declarations for compatibility with WIN32
#ifndef _WIN32
typedef  long               lparam;
typedef  unsigned long      wparam;
typedef  long               longptr;
#else
typedef __w64 long          lparam;
typedef __w64 unsigned long wparam;
typedef __w64 long          longptr;
#endif

typedef    void*            handle;
typedef const char*         lpcstr;
typedef const wchar_t*      lpcwstr;
typedef char*               lpstr;
typedef wchar_t*            lpwstr;

#define LODWORD(l)           ((ulong)((ullong)(l) & 0xffffffff))
#define HIDWORD(l)           ((ulong)((ullong)(l) >> 32))
#define MAKEULLONG(hi, lo)   (ullong)((ullong(hi) << 32) | ullong(ulong(lo)))
#define ABS(val)             (((val) < 0)? (-(val)) : (val) )

/**
 * @def max(a,b)
 *
 * @brief Finds the maximum of the two values
 */
#ifndef max
#define max(a,b)             (((a) > (b)) ? (a) : (b))
#endif

/**
 * @def min(a,b)
 *
 * @brief Finds the minimum of the two values
 */
#ifndef min
#define min(a,b)             (((a) < (b)) ? (a) : (b))
#endif

/**
 * @def ZEROSTRUCT(_struct)
 *
 * @brief Zero's out the structure, all elements set to zero
 *
 * @param _struct - The structure to zero out
 */
#ifndef ZEROSTRUCT
#define ZEROSTRUCT(_struct) memset(&_struct, 0, sizeof(_struct))
#endif

#define SETMAXFLOAT(MF) *((long*)(&MF))= (0x7f800000)
#define SETMINFLOAT(MF) *((long*)(&MF))= (0xff800000)

/**
 * @def RADTODEG
 *
 * @brief - 180.0 / PI, used to convert radians to degrees
 */
#define        RADTODEG            57.2957795130823208768f 

/**
 * @def DEGTORAD
 *
 * @brief - PI / 180.0, used to convert degrees to radians
 */
#define        DEGTORAD            0.01745329251994329577f 

/**
 * @def M_PI
 *
 * @brief The value of Pi to 20 places
 */
#ifndef        M_PI                
#define        M_PI                3.14159265358979323846f
#endif

/**
 * @def FLT_MAX
 *
 * @brief Maximum floating point value
 */
#ifndef FLT_MAX
    #define FLT_MAX                3.402823466e+38F
#endif

// Memory management macros for buffers and arrays
#define _RELEASE_(p) { if(p) { (p)->Release(); (p)=0; };};
#define _SAFE_RELEASE_(p) { if(p) { (p)->Release(); };};
#define _DELETE_(p)  { if(p) { delete (p);     (p)=0; };};
#define _DELETE_ARRAY_(p)  { if(p) { delete[] (p);     (p)=0; };};
#define _SAFE_DELETE_(p)  { if(p) { delete (p);}};
#define _DESTROY_(p)  { if(p) { (p)->Destroy();    (p)=0; };};
#define _ADDREF_(p)    if(p)p->AddRef()

#define PAS __stdcall

#ifdef IS_EXPORTS
#define IS_API __declspec(dllexport) 
#else
 #ifdef IS_IMPORTS
  #define IS_API __declspec(dllimport) 
 #else 
  #define IS_API 
 #endif
#endif

#ifndef INLINE
#define INLINE __inline
#endif

#ifndef FORCEINLINE
#define FORCEINLINE __forceinline
#endif


#endif

// End of file