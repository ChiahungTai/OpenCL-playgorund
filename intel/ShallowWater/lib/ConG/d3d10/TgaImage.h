/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file TgaImage.h
///
/// @brief Contains the TgaImage class definition for working with TGA file format

#pragma once

#include <vector>


class TgaImage
{
public:
	TgaImage(const TCHAR *fileName, size_t extraBytes);
	virtual ~TgaImage();

	// Image methods
	const void* GetData() const;
	unsigned long GetBpp() const; // bits per pixel
	unsigned long GetWidth() const;
	unsigned long GetHeight() const;

private:
	void Load(const void *data, unsigned long size);

	unsigned long _height;
	unsigned long _width;
	unsigned long _bpp;
	std::vector<char> _data;
};


// end of file
