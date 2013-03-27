/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SceneReaderDae.h

#include "SceneGraphBase.h"

namespace collada
{
	/// Merges content of .dae file with the existing scene
	void ReadSceneDae(const TCHAR *szFileName, EditableScene *pResult);
}


// end of file
