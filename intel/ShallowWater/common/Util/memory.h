/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file memory.h
 *
 * @brief Implements memory managment routines 
 */
#pragma once

#include <windows.h>

#ifdef _WIN32
typedef __int8                  int8_t;
typedef unsigned __int8         uint8_t;
typedef __int16                 int16_t;
typedef unsigned __int16        uint16_t;
typedef __int32                 int32_t;
typedef unsigned __int32        uint32_t;
typedef __int64                 int64_t;
typedef unsigned __int64        uint64_t;
#elif defined(_LINUX)
#include <stdint.h>
#include <unistd.h>
#else
#include <sys/types.h>
#endif


#include "assert.h"
#include <stdio.h>
#include "CommonDataStruct.h"
#include "malloc.h"

/**
 * @def CACHELINE_SIZE
 *
 * @brief Size of cache line to align with it
 */
#define CACHELINE_SIZE 128 
//64

/**
 * @def SIMD_WIDTH
 *
 * @brief The number of elements in one vector.
 *
 * @note This value is used to allocate memory that is valid for 
 * read and write operations by whole vectors.
 */
#define SIMD_WIDTH 16  

/**
 * @struct UseOwnMemFunctionsTag
 *
 * @brief A structure that is used to redefine a new operator
 */
struct UseOwnMemFunctionsTag
{

};

extern const UseOwnMemFunctionsTag UseOwnMem;

/**
 * @fn AllocateMemory(size_t in_size, int align)
 *
 * @brief - Allocates memory 
 *
 * @param in_size - The size of requested memory block in bytes
 *
 * @param align - The number of bytes that are used for align
 *
 * @return - A pointer to the allocated memory or NULL in case of a failure.
 */
inline void* AllocateMemory(size_t in_size, int align)
{
    char* ptr = NULL;
    assert(in_size > 0);

    ptr = (char*)_mm_malloc(in_size, align);

    return (void*)ptr;
}

/**
 * @fn ReleaseMemory( void* in_pPtr)
 *
 * @brief - Releases the memory allocated by the AllocateMemory function
 *
 * @param in_pPtr - A pointer to the memory to be released.
 *
 * @note This pointer is allocated in and returned by AllocateMemory()
 */
inline void ReleaseMemory( void* in_pPtr)
{
    assert(in_pPtr != NULL);
    if(in_pPtr != NULL)
    {
        _mm_free(in_pPtr);
    }
}

/**
 * @fn operator new (size_t in_size, UseOwnMemFunctionsTag)
 *
 * @brief - A re-defined (overloaded) new operator
 *
 * @param in_size - The size of the memory to allocate
 */
inline void* operator new (size_t in_size, UseOwnMemFunctionsTag)
{
    return AllocateMemory(in_size,128);//AllocateMemory(in_size,64);
}

/**
 * @fn operator new [] (size_t in_size, UseOwnMemFunctionsTag)
 *
 * @brief - A re-defined new[] operator
 *
 * @param in_size - The size of the memory to allocate
 */
inline void* operator new [] (size_t in_size, UseOwnMemFunctionsTag)
{
    return AllocateMemory(in_size,128);//AllocateMemory(in_size,64);
}

/**
 * @fn operator delete (void* in_pPtr, UseOwnMemFunctionsTag)
 *
 * @brief delete is a re-defined delete operator to avoid error compilation.
 *
 * @note         An object allocated by the new operator should be released by
 *               ReleaseMemory (see below)
 *
 * @param in_pPtr - A pointer to the object to be deleted
 */
inline void operator delete (void* in_pPtr, UseOwnMemFunctionsTag)
{
    ReleaseMemory(in_pPtr);
}

/**
 * @fn operator delete [] (void* in_pPtr, UseOwnMemFunctionsTag)
 *
 * @brief delete[] is a re-defined delete[] operator to avoid error compilation
 *
 * @note           An object allocated by the new operator should be released by
 *                 ReleaseMemory (see below)
 *
 * @param in_pPtr - A pointer to the object to be deleted
 */
inline void operator delete [] (void* in_pPtr, UseOwnMemFunctionsTag)
{
    ReleaseMemory(in_pPtr);
}

/**
 * @fn DestroyObject(T* in_pPtr)
 *
 * @brief DestroyObject() is a function used to delete objects allocated by the
 *                        re-defined new operator
 *
 * @param in_pPtr - A pointer to the object to be deleted
 */
template <class T>
inline void DestroyObject(T* in_pPtr)
{
    assert(in_pPtr!=NULL);
    if(in_pPtr!=NULL)
    {
        in_pPtr->~T();
        ReleaseMemory(in_pPtr);
    }
}

/**
 * @fn DestroyArray(T* in_pPtr, size_t in_count)
 *
 * @brief DestroyArray() is a function used to delete an array of objects that were
 *                         allocated by re-defined new[] operator
 *
 * @param in_pPtr - A pointer to the object array to be destroyed
 *
 * @param in_count - The number of elements in the array
 */
template <class T>
inline void DestroyArray(T* in_pPtr, size_t in_count)
{
    assert(in_pPtr!=NULL);
    if(in_pPtr!=NULL)
    {
        for(uint32_t i = 0; i<in_count; i++)
        {
            (&in_pPtr[i])->~T();
        }
        ReleaseMemory(in_pPtr);
    }
}


/*
 * @class Cfile
 *
 * @brief - A class that define the interface for packing and unpacking objects from an array of bytes
 */
class CFile
{
public:

    /**
    * @fn !CFile() 
    *
    * @brief A virtual destructor to make all destructors virtual
    */
    virtual ~CFile() 
    {
    
    }

    /**
     * @fn Write(void *p, int len)
     *
     * @brief - Writes raw data
     *
     * @param p - A pointer to the data that will be written
     *
     * @param len - The number of bytes that will be written
     */
    virtual void Write(void *p, int len) = 0;

    /**
     * @fn Read(void *p, int len)
     *
     * @brief - Reads raw data
     *
     * @param p - A pointer to memory location that will be filled with the data
     *
     * @param len - The number of bytes that will be read
     */
    virtual void Read(void *p, int len) = 0;

    /**
     * @fn Save(T const & val)
     *
     * @brief - Writes a structure
     *
     * @param val - Reference to the structure that will be written 
     */
    template <typename T>
    void Save(T const & val) 
    { 
        Write((void*)&val, sizeof(T)); 
    }

    /**
     * @fn Restore(T& val) 
     *
     * @brief - Reads a structure
     *
     * @param val - A reference to structure that will be filled by the read data
     */
    template <typename T>
    void Restore(T& val) 
    { 
        Read(&val, sizeof(T)); 
    }

    /**
     * @fn Save(char const *string)
     *
     * @brief - This overloaded version is used to write a null terminated string
     *
     * @param string - The null terminated string
     */
    void Save(char const *string);

    /**
     * @fn Restore(char *string, int maxChars)
     *
     * @brief - This overloaded version is used to read a string
     *
     * @param string - A pointer to the data that will be filled by the string read
     *
     * @param maxChars - The maximum number of chars that can be stored in the string
     */
    void Restore(char *string, int maxChars);

};


/**
 * @class DynamicArray
 *
 * @brief Defines a dynamically extensible array
 */
template <typename T>
class DynamicArray
{
public:

    /**
     * @var m_size
     *
     * @brief  The number of elements occupied in the array
     */
    int     m_size;

    /**
     * @var m_pPointer
     *
     * @brief  A pointer to the data to store elements of the array
     */
    T*      m_pPointer;

    /**
     * @var m_capacity
     *
     * @brief  The number of elements that can contain currently allocated memory block
     */
    int     m_capacity;

    /**
     * @fn DynamicArray()
     *
     * @brief Class constructor. Initializes an empty dynamic array
     */
    DynamicArray()
    {
        m_size = 0;
        m_pPointer = NULL;
        m_capacity = 0;
    }

    /**
     * @fn SetCapacity(int in_capacity)
     *
     *@brief Reallocates internal memory if needed to be able to store the given number of elements
     *
     * @param in_capacity - The number of element that the array can contain
     */
    void SetCapacity(int in_capacity)
    {
        Reallocate(in_capacity);
    }
    
    /**
     * @fn Set(T& val)
     *
     * @brief Sets all elements in the array as one
     *
     * @param val - The value that will be used to set all elements in the array
     */
    void Set(T& val)
    {
        for(int i = 0; i<m_size; i++)
        {
            m_pPointer[i] = val;
        }
    }

    /**
     * @fn SetSize(int in_size)
     *
     * @brief Sets the size of the array. Reallocates if needed
     *
     * @param in_size - The new size for the array 
     */
    void SetSize(int in_size)
    {
        if(in_size>m_capacity)Reallocate(in_size);
        m_size = in_size;
    }

    /**
     * @fn Append()
     *
     * @brief Adds one element into the tail of the array and returns the reference for the new element
     *
     * @return - The reference to the newly added element
     */
    T& Append()
    {
        if(m_size >= m_capacity)
        {
            int newCap = m_capacity << 1;
            if(newCap<(m_size+1))newCap=m_size+1;
            if(newCap<4)newCap=4;
            Reallocate(newCap);
        }
        return m_pPointer[m_size++];
    }

    /**
     * @fn Append(T in_element)
     *
     * @brief Adds one element into the tail of the array
     *
     * @param in_element - The element that will be added to the array
     */
    void Append(T in_element)
    {
        Append() = in_element;
    }

    /**
     * @fn AppendArray(int ElementNum, T* pData)
     *
     * @brief Add one or more elements to the tail of the array and initialize if required
     *
     * @param ElementNum - The number of elements that will be added to the end of the array
     *
     * @param pData - A pointer to an array of elements that will be used to initialize the newly added elements
     *
     * @note pData could be NULL to skip initialization
     */
    T* AppendArray(int ElementNum, T* pData)
    {
        T* pRes = NULL;
        int newSize = m_size+ElementNum;
        if(newSize > m_capacity)
        {
            int newCap = m_capacity << 1;
            if(newCap<newSize)newCap=newSize;
            if(newCap<4)newCap=4;
            Reallocate(newCap);
        }
        pRes = m_pPointer + m_size;
        m_size = newSize;
        if(pData)
        {
            memcpy(pRes,pData,sizeof(T)*ElementNum);
        }
        return pRes;
    }

    /**
     * @fn operator[](int in_index)
     *
     * @brief Gets the element referenced by its index
     *
     * @param in_index - The index of the required element
     */
    inline T& operator[](int in_index) const
    {
        assert(m_pPointer != 0);
        assert(in_index < m_size);
        return m_pPointer[in_index];
    }

    /**
     * @fn Size()
     *
     * @brief  Gets the number of elements in the array
     *
     * @return - The number of elements in the array
     */
    int Size() const
    {
        return m_size;
    }

    /**
     * @fn Clear()
     *
     * @brief Sets the size of the array to 0
     *
     * @note Sets the used object counter to 0
     */
    void Clear()
    {
        m_size = 0;
    }

    /**
     * @fn ~DynamicArray()
     *
     * @brief Class destructor, releases resources occupied by the array, frees memory 
     */
    ~DynamicArray()
    {
        Reallocate(0);
    }

    /**
     * @fn operator=(const DynamicArray& src)
     *
     * @brief Used to copy one array to another
     *
     * @param src - A reference to the source array
     *
     * @return - A reference to the destination array
     */
    DynamicArray& operator=(const DynamicArray& src)
    {
        if(&src == this) return *this;
        int i,size = src.Size();
        SetSize(size);
        for(i=0;i<size;++i)
        {
            m_pPointer[i] = src[i];
        }
        return *this;
    }

protected:

    /**
     * @fn Reallocate(int in_capacity)
     *
     * @brief Reallocates the array to be able keep more elements
     *
     * @param in_capacity - The number of elements that can be saved in the reallocated array
     */
    void Reallocate(int in_capacity)
    {
        T*  newPointer = in_capacity>0?new (UseOwnMem) T[in_capacity]:NULL;
        //T*  newPointer = in_capacity>0?new T[in_capacity]:NULL; -> deprecated
        int newSize = m_size;
        if(newSize>in_capacity)newSize=in_capacity;
        if(m_pPointer != NULL)
        {
            for(int i = 0; i<newSize; i++)
            {
                newPointer[i] = m_pPointer[i];
            }
            if(m_pPointer && m_capacity>0) DestroyArray(m_pPointer,m_capacity);
            //if(m_pPointer && m_capacity>0) delete m_pPointer; -> deprecated
        }
        m_pPointer = newPointer;
        m_capacity = in_capacity;
        m_size = newSize;
    }
}; // DynamicArray

/**
 * @class DynamicMatrix
 *
 * @brief The dynamicMatrix class is used to store a different matrix that could expand or shrink
 *
 * @note For example, the matrix could be a map of water bar height, of a map of water bar speed.
 * This matrix is not designed for complex elements that have a constructor and destructor
*/
template <typename T>
class DynamicMatrix
{
public:

    /**
     * @var m_WidthMax
     *
     * @brief The maximum allowed width for the currently allocated buffer
     */
    int         m_WidthMax; 

    /**
     * @var m_HeightMax
     *
     * @brief The maximum allowed height for the currently allocated buffer
     */
    int         m_HeightMax;

    /**
     * @var m_Width
     *
     * @brief The currently used width
     */
    int         m_Width;

    /**
     * @var m_Height
     *
     * @brief The currently used height
     */
    int         m_Height;

    /**
     * @var m_pPointer
     *
     * @brief A pointer to the allocated data
     */
    char*       m_pPointer;

    /**
     * @var m_DataSize
     *
     * @brief The size of the allocated data
     */
    int         m_DataSize;

    /**
     * @var m_Offset
     *
     * @brief The offset in bytes from m_pPointer to the map element with 0,0 coordinates
     */
    int         m_Offset;

    /**
     * @var m_WidthStep
     *
     * @brief The number of bytes between the (x,y) and (x,y+1) elements
     */
    int         m_WidthStep;

    /**
     * @var m_BorderSize
     *
     * @brief The border size
     */
    int         m_BorderSize;

    /**
     * @fn Width()
     *
     * @brief Returns the current width of the matrix
     *
     * @return - The number of columns in the matrix
     */
    int Width() const
    {
        return m_Width;
    }
    /**
     * @fn Height()
     *
     * @brief Returns the current height of the matrix
     *
     * @return - The number of rows in the matrix
     */
    int Height() const
    {
        return m_Height;
    }

    /**
     * @fn Border()
     *
     * @brief Returns the size of the border
     *
     * @return - The width of the border for the matrix
     */
    int Border() const
    {
        return m_BorderSize;
    }

    /**
     * @fn GetLinePtr(int y)
     *
     * @brief Used to get a pointer to a specific line in the matrix
     *
     * @param y - The number of the line
     *
     * @return - A pointer to the specified line
     */
    T* GetLinePtr(int y)
    {
        assert(m_pPointer);
        assert(y>=-m_BorderSize && y<m_Height+m_BorderSize);
        return ((T*)(m_pPointer + m_Offset + y*m_WidthStep));
    }

    /**
     * @fn At(int x, int y)
     *
     * @brief Gets a pointer to the X element in line Y
     *
     * @param x - The element number
     *
     * @param y - The line number
     *
     * @return - Pointer to the X element in the Y line
     */
    T& At(int x, int y)
    {
        assert(m_pPointer);
        assert(x>=-m_BorderSize && x<m_Width+m_BorderSize);
        return GetLinePtr(y)[x];
    }

    /**
     * @fn At(Point2D32i p)
     *
     * @brief Gets a pointer to the element represented by a point
     *
     * @param p - The point
     *
     * @return - Pointer to the element represented by the point
     */
    inline T& At(Point2D32i p)
    {
        return At(p.x, p.y);
    }

    /**
     * @fn GetVal(float x, float y)
     *
     * @brief Gets the value represented by the x and y coordinates into the matrix
     *
     * @param x - The X coordinate into the matrix
     *
     * @param y - The Y coordinate into the matrix
     *
     * @return - The value at the location
     */
    inline T GetVal(float x, float y)
    {
        float* pL0;
        float* pL1;
        float   xf = x+m_BorderSize;
        float   yf = y+m_BorderSize;
        int     xi = (int)(xf);
        int     yi = (int)(yf);
        float   xr = xf-xi;
        float   yr = yf-yi;
        xi -= m_BorderSize;
        yi -= m_BorderSize;
        // Make sure we have data
        assert(m_pPointer);
        // Ensure that the coordinate is within the matrix
        assert(x>=-m_BorderSize && x<m_Width+m_BorderSize);
        assert(y>=-m_BorderSize && y<m_Height+m_BorderSize);
        // Get the line pointer to the line and the next line above
        pL0 = GetLinePtr(yi);
        pL1 = GetLinePtr(yi+1);
        return 
            (1-yr)*((1-xr)*pL0[xi]+xr*pL0[xi+1]) + 
            (yr)  *((1-xr)*pL1[xi]+xr*pL1[xi+1]);
    }

    /**
     * @fn DynamicMatrix(int W=-1, int H=-1, int BorderSize = 1)
     *
     * @brief Class constructor, initializes the matrix
     *
     * @param W - The width of the matrix
     *
     * @param H - The height of the matrix
     *
     * @param BorderSize - The border size around the matrix.
     */
    DynamicMatrix(int W=-1, int H=-1, int BorderSize = 1)
    {
        m_BorderSize = BorderSize;
        m_Width = 0;
        m_WidthMax = 0;
        m_Height = 0;
        m_HeightMax = 0;
        m_pPointer = NULL;
        m_Offset = 0;
        m_WidthStep = 0;
        if(W>0 && H>0)
        {
            SetSize(W,H,BorderSize);
        }
    }

    /**
     * @fn Set(T val)
     *
     * @brief Sets all elements in the matrix as one to the specified value
     *
     * @param val - The value that will be used to initialize the matrix
     */
    void Set(T val)
    {
        int x,y;
        for(y=-m_BorderSize;y<m_Height+m_BorderSize;y++)
        {
            T* ptr = GetLinePtr(y);
            for(x=-m_BorderSize;x<m_Width+m_BorderSize;++x)
            {
                ptr[x] = val;
            }
        }
    }

    /**
     * @fn SetSize(int Width, int Height, int BorderSize = -1)
     *
     * @brief Sets a new size of the Matrix
     *
     * @param Width - The new width for the matrix
     *
     * @param Height - The new height for the matrix
     *
     * @param BorderSize - The new border size around the matrix
     */
    void SetSize(int Width, int Height, int BorderSize = -1)
    {
        if(BorderSize<0)BorderSize = m_BorderSize;
        if(Width>m_WidthMax || Height>m_HeightMax || BorderSize!=m_BorderSize)Reallocate(Width,Height,BorderSize);
        m_Width = Width;
        m_Height = Height;
    };

    /**
     * @fn ~DynamicMatrix()
     *
     * @brief Class destructor, frees memory allocated by the matrix
     */
    ~DynamicMatrix()
    {
        Free();
    };

    /**
     * @fn operator=(const DynamicMatrix& src)
     *
     * @brief Overloaded copy operator
     *
     * @param src - The matrix to copy
     */
    DynamicMatrix& operator=(const DynamicMatrix& src)
    {
        int x,y;
        if(&src == this) return *this;
        Reallocate(src.Width(), src.Height());
        for(y=0;y<m_Height;++y)
        {
            T* pSrc = src.GetLinePtr(y);
            T* pDst = GetLinePtr(y);
            for(x=0;x<m_Width;++x)pDst[x] = pSrc[x];
        }
        return *this;
    }

    /**
     * @fn DynamicMatrix(const DynamicMatrix& src)
     *
     * @brief Overloaded copy constructor for the matrix
     *
     * @param src - The source data for the new matrix
     */
    DynamicMatrix(const DynamicMatrix& src)
    {
        int x,y;
        if(&src == this) return *this;
        Reallocate(src.Width(), src.Height());
        for(y=0;y<m_Height;++y)
        {
            T* pSrc = src.GetLinePtr(y);
            T* pDst = GetLinePtr(y);
            for(x=0;x<m_Width;++x)pDst[x] = pSrc[x];
        }
        return *this;
    }

    /**
     * @fn Save(CFile &file)
     *
     * @brief Used to save the matrix into a CFile 
     *
     * @param file - CFile interface for data storage
     */
    void Save(CFile &file) const
    {
        file.Save(m_BorderSize);
        file.Save(m_DataSize);
        file.Save(m_WidthStep);
        file.Save(m_Width);
        file.Save(m_Height);
        file.Save(m_Offset);
        file.Save(m_WidthMax);
        file.Save(m_HeightMax);
        file.Write(m_pPointer, m_DataSize);
    }

    /**
     * @fn Restore(CFile &file)
     *
     * @brief Use to restore the matrix from a CFile interface
     *
     * @param file -The CFile interface used to restore the matrix parameters and data
     */
    void Restore(CFile &file)
    {
        Free();
        file.Restore(m_BorderSize);
        file.Restore(m_DataSize);
        file.Restore(m_WidthStep);
        file.Restore(m_Width);
        file.Restore(m_Height);
        file.Restore(m_Offset);
        file.Restore(m_WidthMax);
        file.Restore(m_HeightMax);

        m_pPointer = (char*)_mm_malloc(m_DataSize,CACHELINE_SIZE);
        if(!m_pPointer)
        {
            printf("Could not allocate %d bytes \n",m_DataSize);
            assert(0);
            m_pPointer = 0;
            m_DataSize = 0;
            m_Offset = 0;
            m_WidthStep = 0;
            return;
        };
        file.Read(m_pPointer, m_DataSize);
    }


protected:

    /**
     * @fn Free()
     *
     * @brief Used to free all allocated data 
     */
    void Free()
    {
        if(m_pPointer)
        {
            _mm_free(m_pPointer);
        }
        m_pPointer = NULL;
        m_DataSize = 0;
        m_Width = 0;
        m_Height = 0;
        m_WidthMax = 0;
        m_HeightMax = 0;
        m_WidthStep = 0;
    }

    /**
     * @fn Reallocate(int Width, int Height, int BorderSize)
     *
     * @brief Used to reallocate data for a new matrix size
     *
     * @param Width - The new width of the matrix
     *
     * @param Height - The new height of the matrix
     *
     * @param BorderSize - The new border size around the matrix
     */
    void Reallocate(int Width, int Height, int BorderSize)
    {
        // Release all memory
        Free();

        // Check height and width
        if(Width<=0 || Height<=0) return;

        // Initialize offset
        m_Offset = 0;

        {
            // If there is a border then calculate how many cache lines the border occupies
            int CashLineNumForBorder = (int)(BorderSize*(int)sizeof(T)+CACHELINE_SIZE-1)/(CACHELINE_SIZE);
            // Update the offset
            m_Offset += CashLineNumForBorder * CACHELINE_SIZE;
        }

        // Compute the width step
        m_WidthStep = (Width+BorderSize+SIMD_WIDTH)*sizeof(T) + m_Offset;
        m_WidthStep = (m_WidthStep+CACHELINE_SIZE-1)&(~(CACHELINE_SIZE-1));

        // Compute the data size
        m_DataSize = m_WidthStep*(Height+2*BorderSize);

        // Allocate the memory
        m_pPointer = (char*)_mm_malloc(m_DataSize,CACHELINE_SIZE);
        if(!m_pPointer)
        {
            printf("Could not allocate %d bytes of memory\n",m_DataSize);
            assert(0);
            m_pPointer = 0;
            m_DataSize = 0;
            m_Offset = 0;
            m_WidthStep = 0;
            return;
        };

        // Shift the offset to the 0,0 element
        m_Offset += BorderSize * m_WidthStep;
        m_WidthMax = m_Width = Width;
        m_HeightMax = m_Height = Height;
        m_BorderSize = BorderSize;
    }
}; // DynamicMatrix






