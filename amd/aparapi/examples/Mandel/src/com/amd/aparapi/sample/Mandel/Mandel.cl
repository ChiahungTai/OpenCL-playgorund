/**********************************************************************
Copyright 2012 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/
#define MAX_ITERATIONS 64

__constant const int pallette[]={
   -65536,
   -59392,
   -53248,
   -112640,
   -106752,
   -166144,
   -160256,
   -219904,
   -279552,
   -339200,
   -399104,
   -985344,
   -2624000,
   -4197376,
   -5770496,
   -7343872,
   -8851712,
   -10425088,
   -11932928,
   -13375232,
   -14817792,
   -16260096,
   -16719602,
   -16720349,
   -16721097,
   -16721846,
   -16722595,
   -16723345,
   -16724351,
   -16725102,
   -16726110,
   -16727119,
   -16728129,
   -16733509,
   -16738889,
   -16744269,
   -16749138,
   -16754006,
   -16758619,
   -16762976,
   -16767077,
   -16771178,
   -16774767,
   -16514932,
   -15662970,
   -14942079,
   -14221189,
   -13631371,
   -13107088,
   -12648342,
   -12320669,
   -11992995,
   -11796393,
   -11665328,
   -11993019,
   -12386248,
   -12845011,
   -13303773,
   -13762534,
   -14286830,
   -14745588,
   -15269881,
   -15728637,
   -16252927, 
   0
};

#define WIDTH get_global_size(0)
#define HEIGHT get_global_size(1)
#define X get_global_id(0)
#define Y get_global_id(1)

__kernel void createMandleBrot(
      float scale, 
      float offsetx, 
      float offsety, 
      __global int *rgb 
      ){
   float x = ((((float)(X) * scale) - ((scale / 2.0f) * (float)WIDTH)) / (float)WIDTH) + offsetx;
   float y = ((((float)(Y) * scale) - ((scale / 2.0f) * (float)HEIGHT)) / (float)HEIGHT) + offsety;
   float zx = x;
   float zy = y;
   float new_zx = 0.0f;
   int count = 0;
   for (; count<MAX_ITERATIONS && ((zx * zx) + (zy * zy))<8.0f; count++){
      new_zx = ((zx * zx) - (zy * zy)) + x;
      zy = ((2.0f * zx) * zy) + y;
      zx = new_zx;
   }
   rgb[X + Y*WIDTH]  = pallette[count];
}

