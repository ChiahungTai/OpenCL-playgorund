/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file stdafx.cpp

#include "stdafx.h"


//
// Memory tracking allocation
//

#if defined(_DEBUG) && !defined(_AFX_NO_DEBUG_CRT)
#undef new
void* __cdecl operator new(size_t nSize, LPCSTR lpszFileName, int nLine)
{
    return ::operator new(nSize, _NORMAL_BLOCK, lpszFileName, nLine);
}
void* __cdecl operator new[](size_t nSize, LPCSTR lpszFileName, int nLine)
{
    return ::operator new[](nSize, _NORMAL_BLOCK, lpszFileName, nLine);
}
void __cdecl operator delete(void* pData, LPCSTR /* lpszFileName */, int /* nLine */)
{
    ::operator delete(pData);
}
void __cdecl operator delete[](void* pData, LPCSTR /* lpszFileName */, int /* nLine */)
{
    ::operator delete(pData);
}
#endif

// end of file
