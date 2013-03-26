/**********************************************************************
Copyright ©2012 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/


/**    
******************************************************************************** 
* @file < Intro2StaticCpp_Kernels.cl>                            
*                                         
* @brief This file contains the kernel that calculate the area of any input regtangle.
*          
********************************************************************************
*/

/**
* Class Rect
* Class Rect describes rectangles with lengths and widths.
*/
template <typename T>
class Rect
{
    T myLength; /**< length of the rectangle */
    T myWidth; /**< width of the rectangle */
public:
    Rect () {
    }
    Rect (T a, T b)
    {
        myLength = a;
        myWidth = b;
    }
    T get_length()
    {
        return myLength;
    }
    T get_width()
    {
        return myWidth;
    }
};

/**************************************************************************  
* Kernel template, which also defines two explicit instants of the kernel 
* template, AreaCalculation_float and AreaCalculation_int. AreaCalculation_float 
* and AreaCalculation_int are the externel names for the two kernel template 
* instants that must be used as parameters when calling to AreaCalculationh. 
**************************************************************************/  


template <typename T> 
kernel void AreaCalculation(__global Rect <T>* InputClass, __global T* myDevArea )
{  	
    size_t gid = get_global_id(0);
    myDevArea[gid] = InputClass[gid].get_length()* InputClass[gid].get_width();
}

template __attribute__((mangled_name(AreaCalculation_float))) kernel void 
    AreaCalculation(__global Rect<float>* InputClass, __global float* myDevArea);

template __attribute__((mangled_name(AreaCalculation_int))) kernel void 
    AreaCalculation(__global Rect<int>* InputClass, __global int* myDevArea);



