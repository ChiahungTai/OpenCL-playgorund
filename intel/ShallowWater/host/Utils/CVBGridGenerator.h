/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CVBGridGenerator.h
 *
 * @brief Defines structures, classes, variables, and functions to manage grids
 */

#pragma once

#include <d3d10.h>
#include <d3dx10.h>
#include "Util/CRegion.h"
#include "macros.h"

/**
 * @struct CVBContentGenerator
 *
 * @brief Defines common virtual functions for manageing content
 */
struct CVBContentGenerator
{
    /**
     * @fn GetSize()
     *
     * @brief Gets the size of the content
     *
     * @note A virtual function 
     */
    virtual int GetSize() const = 0;

    /**
     * @fn Generate(pData)
     *
     * @brief Gets the size of the content
     *
     * @note A virtual function
     *
     * @param pData - Output - A buffer with the generated content 
     */
    virtual void Generate(void *pData) const = 0;
};

/**
 * @struct CVBGridGenerator
 *
 * @brief Child class specific to the generation of grids
 */
template <class T>
struct CVBGridGenerator : 
    public CVBContentGenerator
{
    /**
     * @fn CVBGridGenerator( int dimW, int dimH, T const &gen )
     *
     * @brief Class constructor
     *
     * @param dimW - The cell line address
     *
     * @param dimH - The cell on the line
     *
     * @param gen - The input generator 
     */
    CVBGridGenerator( int dimW, int dimH, T const &gen ) 
        : m_dimW(dimW), 
        m_dimH(dimH), 
        m_gen(gen)
    {

    }

    /**
     * @fn GetSize()
     *
     * @brief Gets the size of the grid
     *
     * @return The size of the grid in bytes
     */
    virtual int GetSize() const 
    { 
        return m_dimW * m_dimH * sizeof(T::VERTEX); 
    }

    /**
     * @fn Generate(pData)
     *
     * @brief Generats a grid
     *
     * @param pData - Vertex output buffer
     */
    virtual void Generate(void *pData) const
    {
        T::VERTEX *pV = static_cast<T::VERTEX*>(pData);
        // For every line in the buffer
        for (int y = 0; y < m_dimH; y++)
        {
            // For every pixel in the line
            for (int x = 0; x < m_dimW; x++, pV++)
            {
                // call generator to generate vertex for (x,y)
                m_gen(x, y, *pV);
            }
        }
    }
private:

    /** 
     * @var m_dimW 
     *
     * @brief - The width of the grid
     */
    int      m_dimW;

    /**
     * @var m_dimH
     *
     * @brief Height of the grid
     */
    int      m_dimH;

    /**
     * @var m_gen
     *
     * @brief The generator
     */
    T const &m_gen;
    NON_COPYABLE(CVBGridGenerator)
};

/**
 * @struct CVBRegionGenerator
 *
 * @brief Defines functions and variables used for regions
 */
struct CVBRegionGenerator
{
    /**
     * @def VERTEX
     *
     * @brief A D3DXVECTOR2 object
     */
    typedef D3DXVECTOR2 VERTEX;

    /**
     * @fn CVBRegionGenerator(int dimW, int dimH, CRegion const &reg)
     *
     * @brief Class constructor Abstacts the region
     *
     * @param dimW - The number of horizontal cells in the region
     *
     * @param dimH - The number of vertical cells in the region
     *
     * @param reg - The region
     */
    CVBRegionGenerator(int dimW, int dimH, CRegion const &reg);

    /**
     * @fn Operator()(int x, int y, D3DXVECTOR2 &vtx)
     *
     * @brief Extracts a vertex from the region
     *
     * @param x - The horizontal cell address
     *
     * @param y - The vertical cell address
     *
     * @param vtx - Output - the vertext
     */
    void operator()(int x, int y, D3DXVECTOR2 &vtx) const;

    /**
     * @var m_start
     *
     * @brief The starting address of the region
     */
    D3DXVECTOR2 m_start;

    /**
     * @var m_step
     *
     * @brief A vector used to store the horizontal and vertical steps between cells
     */
    D3DXVECTOR2 m_step;
};


/**
 * @fn CSetDevice(ID3D10Device *pDev)
 *
 * @brief Sets the output display device
 *
 * @param pDev - Pointer to the ID3D10 Device
 */
void CSetDevice(ID3D10Device *pDev);

/**
 * @fn *CGetDevice()
 *
 * @brief Retrieves a pointer to the output display device
 *
 * @return -  - Pointer to the ID3D10 Device
 */
ID3D10Device *CGetDevice();

/**
 * @fn LoadEffect(wchar_t const *name, D3D10_SHADER_MACRO* pMacro = NULL)
 *
 * @brief Loads the named effect and shader macro
 *
 * @param name - Name of the effect
 *
 * @param pMacro - The effect macro
 *
 * @return - Pointer to the loaded effect
 */
ID3D10Effect* LoadEffect(wchar_t const *name, D3D10_SHADER_MACRO* pMacro = NULL);

/**
 * @fn CreateIndexBufferImmutable(int numIndices, int *pNumIndices)
 *
 * @brief Creates an immutable index buffer
 *
 * @param numIndices - The number of indices to create in the buffer
 *
 * @return - pNumIndices - An array of numIndices indices  (pNumIndices[numIndices])
 */
ID3D10Buffer* CreateIndexBufferImmutable(int numIndices, int *pNumIndices);

/**
 * @fn CreateGridIndexBuffer(int dimW, int dimH, int *pNumIndices = NULL)
 *
 * @brief Creates an index buffer for a grid
 *
 * @param dimW - The width of the grid
 *
 * @param dimH - The height of the grid
 *
 * @param pNumIndices - Ouput - The number of indices created
 *
 * @return - A pointer to the created index buffer
 */
ID3D10Buffer* CreateGridIndexBuffer(int dimW, int dimH, int *pNumIndices = NULL);

/**
 * @fn CreateVertexBuffer(int size, D3D10_USAGE usage)
 *
 * @brief Creates a vertext buffer
 *
 * @param size - The byte-width of the buffer
 *
 * @param usage - The buffer's use
 *
 * @return - A pointer to the created vertext buffer
 */
ID3D10Buffer* CreateVertexBuffer(int size, D3D10_USAGE usage);

/**
 * @fn CreateVertexBufferImmutable(CVBContentGenerator const &gen)
 *
 * @brief Creates an immutable vertext buffer.
 *
 * @param gen - Address of a content generator object
 *
 * @return - Pointer to the created immutable vertex buffer
 */
ID3D10Buffer* CreateVertexBufferImmutable(CVBContentGenerator const &gen);

/**
 * @fn LoadShaderResourceView(wchar_t const *name)
 *
 * @brief Loads the named shader resource view
 *
 * @param name - The name of the shader resource view to load
 *
 * @return - Pointer to the shader resource view
 */
ID3D10ShaderResourceView* LoadShaderResourceView(wchar_t const *name);

/**
 * @fn DXFormatSize(DXGI_FORMAT format)
 *
 * @brief Obtains the size of the buffer data
 *
 * @param format - The enumerated format type
 *
 * @return - The byte size of the data
 */
int DXFormatSize(DXGI_FORMAT format);

/**
 * @fn DropResources()
 *
 * @brief Drops all shader resources from the output device
 */
void DropResources();
