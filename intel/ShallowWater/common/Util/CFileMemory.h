/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFileMemory.h
 *
 * @brief Contains the water surface data read from the (write in) file routines declaration. 
 */

#pragma once

#include "macros.h"
#include "util/memory.h"


/**
 * @class CFileMemory
 *
 * @brief This class is used to pack object data into one single memory block
 */
class CFileMemory : 
    public CFile
{
public:

    /**
     * @fn CFileMemory()
     *
     * @brief Class constructor, initializes the current index to zero
     */
    CFileMemory();

    /**
     * @fn Write(void *p, int len)
     *
     * @brief Writes a single memory block
     *
     * @param p - pointer to the memory block that will be written
     *
     * @param len - size of the memory block in bytes
     */
    virtual void Write(void *p, int len);

    /**
     * @fn Read(void *p, int len)
     *
     * @brief Reads a single memory block
     *
     * @param p - pointer to the memory block that will be filled by data
     *
     * @param len - The size of the memory block in bytes
     */
    virtual void Read(void *p, int len);

    /**
     * @fn GetSize()
     *
     * @brief Gets the data size of stored data
     *
     * @return - The data size
     */
    int GetSize()
    {
        return m_data.Size();
    }

    /**
     * @fn GetData()
     *
     * @brief Gets a direct pointer to the stored data
     *
     * @return - A pointer to the start of the stored data
     */
    char* GetData()
    {
        return &m_data[0];
    }

    /**
     * @fn SetData(char* pData, int len)
     *
     * @brief Fills the internal buffer with the given data
     *
     * @param pData - A pointer to the external data that will be copied into the internal buffer
     *
     * @param len - The number of bytes that will be copied
     */
    void SetData(char* pData, int len);


private:

    /**
     * @var m_curIndex
     *
     * @brief The current index into the local data array
     */
    int                 m_curIndex; 

    /**
     * @var m_data
     *
     * @brief The array in which to store the data
     */
    DynamicArray<char>  m_data;
};

