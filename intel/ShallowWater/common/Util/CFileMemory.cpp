/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFileMemory.cpp
 *
 * @brief Contains the water surface data read from the (write in) file routines implementation. 
 */

#include <stdio.h>
#include <string.h>
#include "CFileMemory.h"

/**
 * @fn CFileMemory()
 *
 * @brief Class constructor, initializes the current index to zero
 */
CFileMemory::CFileMemory()
{
    m_curIndex = 0;
}

/**
 * @fn Write(void *p, int len)
 *
 * @brief Writes a single memory block
 *
 * @param p - pointer to the memory block that will be written
 *
 * @param len - size of the memory block in bytes
 */
void CFileMemory::Write(void *p, int len)
{
    m_data.AppendArray(len,(char*)p);
}

/**
 * @fn Read(void *p, int len)
 *
 * @brief Reads a single memory block
 *
 * @param p - pointer to the memory block that will be filled by data
 *
 * @param len - The size of the memory block in bytes
 */
void CFileMemory::Read(void *p, int len)
{
    assert(m_curIndex + len <= m_data.Size());
    memcpy(p, &m_data[m_curIndex], len);
    m_curIndex += len;
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
void CFileMemory::SetData(char* pData, int len)
{
    m_data.Clear();
    m_data.AppendArray(len,pData);
    m_curIndex = 0;
}
