/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGImage.h

#pragma once

#include "SceneGraph.h"
#include "SGImplBase.h"

using namespace collada;

///////////////////////////////////////////////////////////////////////////////

class ImageImpl : public BaseImpl<Image>
{
public:
	ImageImpl(Scene *s);

	// Image interface
	virtual const TCHAR* GetFileName() const;
	virtual void SetFileName(const TCHAR *fileName);

private:
	string_t m_fileName;
};


// end of file
