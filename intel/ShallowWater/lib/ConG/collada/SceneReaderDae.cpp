/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SceneReaderDae.cpp

#include "stdafx.h"

#include <windows.h>
#include <stdexcept>

#include "collada/SceneReaderDae.h"
#include "SceneGraph.h"

namespace collada
{

void ReadSceneDae(const TCHAR *szFileName, EditableScene *pResult)
{
#ifdef _M_AMD64
	wchar_t libraryName[] = L"ConG_collada64r.dll";
#else
	wchar_t libraryName[] = L"ConG_collada32r.dll";
#endif
	
	HMODULE hLib = NULL;

	typedef void (*LPDllFunction)(const TCHAR*, EditableScene*);

	LPDllFunction lpDllFunction;

	hLib = LoadLibrary(libraryName);
	if( !hLib )
	{
		wchar_t str[MAX_PATH];
		wchar_t filesDirectory[] = L".\\lib\\ConG\\bin";

		swprintf_s(str, MAX_PATH, L"%s\\%s", filesDirectory, libraryName); 
		hLib=LoadLibrary(str);
		if( !hLib )
		{
			throw std::runtime_error("collada loader library failed to load");
		}
	}

	lpDllFunction = (LPDllFunction)GetProcAddress(hLib, "OpenColladaFile");

	if (lpDllFunction)
	{
		lpDllFunction(szFileName, pResult);
	}
	else
	{
		throw std::runtime_error("failed to get proc address");
	}
}

} // end of namespace collada
// end of file
