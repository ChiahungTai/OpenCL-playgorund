/**********************************************************************
Copyright ?012 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

?Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
?Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/
#include "SDKBitMap.hpp"
static const short bitMapID = 19778;

namespace streamsdk
{
void
SDKBitMap::releaseResources(void)
{
    if (pixels_ != NULL) {
        delete[] pixels_;
    }

    if (colors_ != NULL) {
        delete[] colors_;
    }

    pixels_    = NULL;
    colors_    = NULL;
    isLoaded_  = false;
}


SDKBitMap& SDKBitMap::operator=(const SDKBitMap& rhs)
{
    if (this == &rhs) {
        return *this;
    }

    // Copy header
    id         = rhs.id;
    size       = rhs.size;
    reserved1  = rhs.reserved1;
    reserved2  = rhs.reserved2;
    offset     = rhs.offset;

    // Copy header info
    sizeInfo       = rhs.sizeInfo;
    width          = rhs.width;
    height         = rhs.height;
    planes         = rhs.planes;
    bitsPerPixel   = rhs.bitsPerPixel;
    compression    = rhs.compression;
    imageSize      = rhs.imageSize;
    xPelsPerMeter  = rhs.xPelsPerMeter;
    yPelsPerMeter  = rhs.yPelsPerMeter;
    clrUsed        = rhs.clrUsed;
    clrImportant   = rhs.clrImportant;

    numColors_ = rhs.numColors_;
    isLoaded_  = rhs.isLoaded_;

    pixels_    = NULL;
    colors_    = NULL;
    if (isLoaded_) {
        if (rhs.colors_ != NULL) {
            colors_ = new ColorPalette[numColors_];
            if (colors_ == NULL) {
                isLoaded_ = false;
                return *this;
            }
            memcpy(colors_, rhs.colors_, numColors_ * sizeof(ColorPalette));
         }

        pixels_ = new uchar4[width * height];
        if (pixels_ == NULL) {
            delete[] colors_;
            colors_   = NULL;
            isLoaded_ = false;
            return *this;
        }
        memcpy(pixels_, rhs.pixels_, width * height * sizeof(uchar4));
    }

    return *this;
}


bool SDKBitMap::write(const char * filename, int width, int height, unsigned int *ptr)
{
    // Open BMP file
    FILE * fd = fopen(filename, "wb");

    int alignSize  = width * 4;
    alignSize ^= 0x03;
    alignSize ++;
    alignSize &= 0x03;

    int rowLength = width * 4 + alignSize;

    // Opened OK
    if (fd != NULL) 
    {
        BitMapHeader *bitMapHeader = new BitMapHeader;
        bitMapHeader->id = bitMapID;
        bitMapHeader->offset = sizeof(BitMapHeader) + sizeof(BitMapInfoHeader);
        bitMapHeader->reserved1 = 0x0000;
        bitMapHeader->reserved2 = 0x0000;
        bitMapHeader->size = sizeof(BitMapHeader) + sizeof(BitMapInfoHeader) + rowLength * height;
        // Write header
        fwrite(bitMapHeader, sizeof(BitMapHeader), 1, fd);

        // Failed to write header
        if (ferror(fd)) 
        {
            fclose(fd);
            return false;
        }

        BitMapInfoHeader *bitMapInfoHeader = new BitMapInfoHeader;
        bitMapInfoHeader->bitsPerPixel = 32;
        bitMapInfoHeader->clrImportant = 0;
        bitMapInfoHeader->clrUsed = 0;
        bitMapInfoHeader->compression = 0;
        bitMapInfoHeader->height = height;
        bitMapInfoHeader->imageSize = rowLength * height;
        bitMapInfoHeader->planes = 1;
        bitMapInfoHeader->sizeInfo = sizeof(BitMapInfoHeader);
        bitMapInfoHeader->width = width; 
        bitMapInfoHeader->xPelsPerMeter = 0;
        bitMapInfoHeader->yPelsPerMeter = 0;

        // Write map info header
        fwrite(bitMapInfoHeader, sizeof(BitMapInfoHeader), 1, fd);

        // Failed to write map info header
        if (ferror(fd)) 
        {
            fclose(fd);
            return false;
        }    
        unsigned char buffer[4];
        int x, y;

        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++, ptr++)
            {
                if( 4 != fwrite(ptr, 1, 4, fd)) 
                {
                    fclose(fd);
                    return false;
                }
            }
            memset( buffer, 0x00, 4 );

            fwrite( buffer, 1, alignSize, fd );
        }

        fclose( fd );
        return true;
    }

    return false;
}

int 
SDKBitMap::getNumChannels()
{
    if (isLoaded_) 
    {
        return bitsPerPixel / 8;
    }
    else 
    {
        return SDK_FAILURE;
    }
}

void
SDKBitMap::load(const char * filename)
{
    // Release any existing resources
    releaseResources();

    // Open BMP file
    FILE * fd = fopen(filename, "rb");
    // FILE *fd; 
    //fopen_s(&fd, filename, "rb");

    // Opened OK
    if (fd != NULL) {
        // Read header
        fread((BitMapHeader *)this, sizeof(BitMapHeader), 1, fd);

        // Failed to read header
        if (ferror(fd)) {
            fclose(fd);
            return;
        }

        // Confirm that we have a bitmap file
        if (id != bitMapID) {
            fclose(fd);
            return;
        }

        // Read map info header
        fread((BitMapInfoHeader *)this, sizeof(BitMapInfoHeader), 1, fd);

        // Failed to read map info header
        if (ferror(fd)) {
            fclose(fd);
            return;
        }

        // No support for compressed images
        if (compression) {
            fclose(fd);
            return;
        }

        // Support only 8 or 24 bits images
        if (bitsPerPixel < 8) {
            fclose(fd);
            return;
        }

        // Store number of colors
        numColors_ = 1 << bitsPerPixel;

        //load the palate for 8 bits per pixel
        if(bitsPerPixel == 8) {
            colors_ = new ColorPalette[numColors_];
            if (colors_ == NULL) {
                fclose(fd);
                return;
            }
            fread(
                (char *)colors_,
                numColors_ * sizeof(ColorPalette),
                1,
                fd);

            // Failed to read colors
            if (ferror(fd)) {
                fclose(fd);
                return;
            }
        }

        // Allocate buffer to hold all pixels
        unsigned int sizeBuffer = size - offset;
        unsigned char * tmpPixels = new unsigned char[sizeBuffer];

        if (tmpPixels == NULL) {
            delete colors_;
            colors_ = NULL;
            fclose(fd);
            return;
        }

        // Read pixels from file, including any padding
        fread(tmpPixels, sizeBuffer * sizeof(unsigned char), 1, fd);

        // Failed to read pixel data
        if (ferror(fd)) {
            delete colors_;
            colors_ = NULL;
            delete tmpPixels;
            fclose(fd);
            return;
        }

        // Allocate image
        pixels_ = new uchar4[width * height];
        if (pixels_ == NULL) {
            delete colors_;
            colors_ = NULL;
            delete tmpPixels;
            fclose(fd);
            return;
        }
        // Set image, including w component (white)
        memset(pixels_, 0xff, width * height * sizeof(uchar4));

        unsigned int index = 0;
        for(int y = 0; y < height; y++) {
            for(int x = 0; x < width; x++) {
                // Read RGB values
                if (bitsPerPixel == 8) {
                    pixels_[(y * width + x)] = colors_[tmpPixels[index++]];
                }
                else { // 24 bit
                    pixels_[(y * width + x)].z = tmpPixels[index++];
                    pixels_[(y * width + x)].y = tmpPixels[index++];
                    pixels_[(y * width + x)].x = tmpPixels[index++];
                }
            }

            // Handle padding
            for(int x = 0; x < (4 - (3 * width) % 4) % 4; x++) {
                index++;
            }
        }

        // Loaded file so we can close the file.
        fclose(fd);
        delete[] tmpPixels;

        // Loaded file so record this fact
        isLoaded_  = true;
    }
}

int
SDKBitMap::colorIndex(uchar4 color)
{
    for (int i = 0; i < numColors_; i++) {
        if (colors_[i].x == color.x &&
            colors_[i].y == color.y &&
            colors_[i].z == color.z &&
            colors_[i].w == color.w) {
            return i;
        }
    }

    return SDK_SUCCESS;
}

bool
SDKBitMap::write(const char * filename)
{
    if (!isLoaded_) {
        return false;
    }

    // Open BMP file
    FILE * fd = fopen(filename, "wb");
    //FILE * fd;
    //fopen_s(&fd, filename, "wb");


    // Opened OK
    if (fd != NULL) {
        // Write header
        fwrite((BitMapHeader *)this, sizeof(BitMapHeader), 1, fd);

        // Failed to write header
        if (ferror(fd)) {
            fclose(fd);
            return false;
        }

        // Write map info header
        fwrite((BitMapInfoHeader *)this, sizeof(BitMapInfoHeader), 1, fd);

        // Failed to write map info header
        if (ferror(fd)) {
            fclose(fd);
            return false;
        }

        // Write palate for 8 bits per pixel
        if(bitsPerPixel == 8) {
            fwrite(
                (char *)colors_,
                numColors_ * sizeof(ColorPalette),
                1,
                fd);

            // Failed to write colors
            if (ferror(fd)) {
                fclose(fd);
                return false;
            }
        }

        for(int y = 0; y < height; y++) {
            for(int x = 0; x < width; x++) {
                // Read RGB values
                if (bitsPerPixel == 8) {
                    fputc(
                        colorIndex(
                            pixels_[(y * width + x)]),
                            fd);
                }
                else { // 24 bit
                    fputc(pixels_[(y * width + x)].z, fd);
                    fputc(pixels_[(y * width + x)].y, fd);
                    fputc(pixels_[(y * width + x)].x, fd);

                    if (ferror(fd)) {
                        fclose(fd);
                        return false;
                    }
                }
            }

            // Add padding
            for(int x = 0; x < (4 - (3 * width) % 4) % 4; x++) {
                fputc(0, fd);
            }
        }

        return true;
    }

    return false;
}
} //streamsdk
