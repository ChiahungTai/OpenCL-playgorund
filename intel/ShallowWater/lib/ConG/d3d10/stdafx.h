/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file stdafx.h


#include <atlcomcli.h> // for CComPtr support
#include <comutil.h>   // for _bstr_t support


#if defined(DEBUG) || defined(_DEBUG)
#ifndef V
#define V(x)           { hr = (x); if( FAILED(hr) ) { assert(0); } }
#endif
#ifndef V_RETURN
#define V_RETURN(x)    { hr = (x); if( FAILED(hr) ) { assert(0); return hr; } }
#endif
#else
#ifndef V
#define V(x)           { hr = (x); }
#endif
#ifndef V_RETURN
#define V_RETURN(x)    { hr = (x); if( FAILED(hr) ) { return hr; } }
#endif
#endif

#include <d3dx10.h>
#include <d3d10.h>

#include <cassert>
#include <queue>
#include <stack>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <algorithm>
#include <iostream>



//
// Memory tracking allocation
//

#if defined(_DEBUG) && !defined(_AFX_NO_DEBUG_CRT)

void * __cdecl operator new[](size_t);
void* __cdecl operator new[](size_t nSize, LPCSTR lpszFileName, int nLine);
void* __cdecl operator new(size_t nSize, LPCSTR lpszFileName, int nLine);

void __cdecl operator delete[](void *);
void __cdecl operator delete[](void* p, LPCSTR lpszFileName, int nLine);
void __cdecl operator delete(void* p, LPCSTR lpszFileName, int nLine);

#define new new(__FILE__, __LINE__)

#endif

// end of file
