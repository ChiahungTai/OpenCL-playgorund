/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */


/// @file SGImage.cpp

#include "stdafx.h"

#include "SGImage.h"

///////////////////////////////////////////////////////////////////////////////

ImageImpl::ImageImpl(Scene *s)
  : BaseImpl(s)
{
}

const TCHAR* ImageImpl::GetFileName() const
{
	return m_fileName.c_str();
}

void ImageImpl::SetFileName(const TCHAR *fileName)
{
	m_fileName = fileName;
}

// end of file
