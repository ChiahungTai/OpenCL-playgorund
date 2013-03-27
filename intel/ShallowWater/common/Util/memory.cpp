/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */


/**
 * @file memory.cpp
 *
 * @brief Implements overloaded save and restore functions
 */

#include "memory.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

const UseOwnMemFunctionsTag UseOwnMem = UseOwnMemFunctionsTag();

/**
 * @fn Save(char const *string) 
 *
 * @brief  - This overloaded version is used to write a null terminated string
 *
 * @param string - The null terminated string
 */
void CFile::Save(char const *string)
{
    int len = (int)strlen(string);
    Save(len);
    Write((void*)string, (int)len);
}

/**
 * @fn Restore(char *string, int maxChars)
 *
 * @brief - This overloaded version is used to read a string
 *
 * @param string - A pointer to the data that will be filled by the string read
 *
 * @param maxChars - The maximum number of chars that can be stored in the string
 */
void CFile::Restore(char *string, int maxChars)
{
    int len;
    Restore(len);
    assert(len < (maxChars-1));
    int len2Read = len<(maxChars-1)?len:(maxChars-1);
    Read((void*)string, len2Read);
    string[len2Read] = '\0';
}

// End of file
