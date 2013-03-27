/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file TgaImage.cpp
///
/// @brief Contains implementation of the TgaImage class

#include "stdafx.h"
#include "TgaImage.h"

namespace
{
	struct AutoHandle
	{
		HANDLE h;
		AutoHandle() : h(NULL) {}
		~AutoHandle()
		{
			if( NULL != h && INVALID_HANDLE_VALUE != h )
			{
				CloseHandle(h);
			}
		}
	private:
		AutoHandle(const AutoHandle&);
		AutoHandle& operator = (const AutoHandle&);
	};

	struct AutoMapViewOfFile
	{
		void *ptr;
		AutoMapViewOfFile() : ptr(NULL) {}
		~AutoMapViewOfFile()
		{
			if( ptr )
			{
				UnmapViewOfFile(ptr);
			}
		}

	private:
		AutoMapViewOfFile(const AutoHandle&);
		AutoMapViewOfFile& operator = (const AutoHandle&);
	};

	std::string GetLastErrorMessage()
	{
		LPVOID lpMsgBuf = NULL;
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, GetLastError(), 0, (LPSTR) &lpMsgBuf, 0, NULL );
		std::string result((LPCSTR) lpMsgBuf);
		LocalFree(lpMsgBuf);
		return result;
	}
}

///////////////////////////////////////////////////////////////////////////////

TgaImage::TgaImage(const TCHAR *fileName, size_t extraBytes)
  : _height(-1)
  , _width(-1)
  , _bpp(-1)
{
	AutoHandle file;
	AutoHandle map;

	file.h = ::CreateFile(fileName,
		FILE_READ_DATA,           // dwDesiredAccess
		FILE_SHARE_READ,          // dwShareMode
		NULL,                     // lpSecurityAttributes
		OPEN_EXISTING,            // dwCreationDisposition
		0,                        // dwFlagsAndAttributes
		NULL);                    // hTemplateFile

	if( INVALID_HANDLE_VALUE == file.h )
	{
		throw std::runtime_error(GetLastErrorMessage());
	}

	DWORD size = GetFileSize(file.h, NULL);
	if( INVALID_FILE_SIZE == size )
	{
		throw std::runtime_error(GetLastErrorMessage());
	}

	map.h = CreateFileMapping(file.h, NULL, PAGE_READONLY, 0, 0, NULL);
	if( NULL == map.h )
	{
		throw std::runtime_error(GetLastErrorMessage());
	}

	AutoMapViewOfFile data;
	data.ptr = MapViewOfFile(map.h, FILE_MAP_READ, 0, 0, 0);
	if( NULL == data.ptr )
	{
		throw std::runtime_error(GetLastErrorMessage());
	}

	Load(data.ptr, size);
}

TgaImage::~TgaImage()
{
}

void TgaImage::Load(const void *data, unsigned long size)
{
	static const unsigned char signatureUncompressed[12] = {0,0,2, 0,0,0,0,0,0,0,0,0};
	static const unsigned char signatureCompressed[12]   = {0,0,10,0,0,0,0,0,0,0,0,0};

	struct Header
	{
		unsigned char signature[12];
		unsigned char header[6]; // First 6 useful bytes from the header
		unsigned char data[1];
	};

	if( size < sizeof(Header) )
	{
		throw std::runtime_error("corrupted TGA image");
	}

	const Header &h = *(const Header *) data;
	unsigned long dataSize = size - offsetof(Header, data);

	_width  = h.header[1] * 256 + h.header[0];
	_height = h.header[3] * 256 + h.header[2];
	_bpp    = h.header[4];

	if( _width <= 0 || _height <= 0 || (_bpp != 24 && _bpp != 32) )
	{
		throw std::runtime_error("unsupported size or bpp");
	}

	long bytesPerPixel = _bpp / 8;
	unsigned long imageSize = bytesPerPixel * _width * _height;

	// determine file type
	if( 0 == memcmp(signatureUncompressed, h.signature, 12) )
	{
		// the file is uncompressed TGA
		if( dataSize < imageSize )
		{
			throw std::runtime_error("corrupted TGA image");
		}
		_data.assign(h.data, h.data + imageSize);
	}
	else if( 0 == memcmp(signatureCompressed, h.signature, 12) )
	{
		// the file is compressed TGA
		unsigned long pixelcount   = _height * _width;
		unsigned long currentpixel = 0;
		unsigned long currentbyte  = 0;
		_data.reserve(imageSize);
		do
		{
			if( currentbyte >= dataSize )
			{
				throw std::runtime_error("corrupted TGA image");
			}
			const unsigned char &chunkheader = h.data[currentbyte++];

			if( chunkheader < 128 )    // If the header is < 128, it means the that is the number 
			{                          // of RAW color packets minus 1 that follow the header
				// Read RAW color values
				int pcount = chunkheader + 1;
				currentpixel += pcount;
				if( pixelcount < currentpixel || dataSize < currentbyte + pcount * bytesPerPixel )
				{
					throw std::runtime_error("corrupted TGA image");
				}
				_data.insert(_data.end(), h.data + currentbyte, h.data + currentbyte + pcount * bytesPerPixel);
				currentbyte += pcount * bytesPerPixel;
			}
			else // chunkheader >= 128 RLE data, next color repeated chunkheader - 127 times
			{
				int pcount = chunkheader - 127;  // get rid of the ID bit
				currentpixel += pcount;
				if( pixelcount < currentpixel || dataSize < currentbyte + bytesPerPixel  )
				{
					throw std::runtime_error("corrupted TGA image");
				}
				const unsigned char *colorbuffer = h.data + currentbyte;
				currentbyte += bytesPerPixel;
				for( int counter = 0; counter < pcount; ++counter )
				{
					_data.insert(_data.end(), colorbuffer, colorbuffer + bytesPerPixel);
				}
			}
		}
		while( currentpixel < pixelcount );
	}
	else
	{
		throw std::runtime_error("unsupported TGA signature");
	}

	// swap R <-> G
	// TGA stores color value as RBG(A) but we need RGB(A)
	for( unsigned long cswap = 0; cswap < imageSize; cswap += bytesPerPixel )
	{
		std::swap(_data[cswap], _data[cswap + 2]);
	}

	// flip vertical because we need inverse rows order
	unsigned long len = _width * (_bpp >> 3);
	for( unsigned long y = 0; y < _height >> 1; y++ )
	{
		std::swap_ranges(_data.begin() + y * len, _data.begin() + y * len + len,
			_data.begin() + (_height - y - 1) * len);
	}

	// do padding for 24 images to always have 32 bits per pixel
	if( 24 == _bpp )
	{
		std::vector<char> padded(_width * _height * 4);
		std::vector<char>::iterator dst = padded.begin();
		std::vector<char>::const_iterator src = _data.begin();
		while( src != _data.end() )
		{
			*(dst++) = *(src++);
			*(dst++) = *(src++);
			*(dst++) = *(src++);
			*(dst++) = 0;     // padding
		}
		_data.swap(padded);
		_bpp = 32;
	}
}


const void* TgaImage::GetData() const
{
	assert(!_data.empty());
	return &_data[0];
}

unsigned long TgaImage::GetBpp() const
{
	assert(-1 != _bpp);
	return _bpp;
}

unsigned long TgaImage::GetWidth() const
{
	assert(-1 != _width);
	return _width;
}

unsigned long TgaImage::GetHeight() const
{
	assert(-1 != _height);
	return _height;
}

// end of file
