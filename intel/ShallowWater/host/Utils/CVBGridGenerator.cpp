/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CVBGridGenerator.cpp
 *
 * @brief Implements structures, classes, variables, and functions to manage grids
 */

#include <DXUT.h>
#include "macros.h"
#include "CVBGridGenerator.h"

#include <malloc.h>

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
CVBRegionGenerator::CVBRegionGenerator(int dimW, int dimH, CRegion const &reg)
{
    m_start.x = reg.x1;
    m_start.y = reg.y1;
    m_step.x = (reg.x2 - reg.x1) / float(dimW);
    m_step.y = (reg.y2 - reg.y1) / float(dimH);
}

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
void CVBRegionGenerator::operator()(int x, int y, VERTEX &vtx) const
{
    vtx.x = m_start.x + m_step.x * float(x);
    vtx.y = m_start.y + m_step.y * float(y);
}

/**
 * @fn LoadEffect(wchar_t const *name, D3D10_SHADER_MACRO* pMacro)
 *
 * @brief Loads the named effect and shader macro
 *
 * @param name - Name of the effect
 *
 * @param pMacro - The effect macro
 *
 * @return - Pointer to the loaded effect
 */
ID3D10Effect* LoadEffect(wchar_t const *name, D3D10_SHADER_MACRO* pMacro)
{
    ID3D10Effect *pEffect;

    // Make the shader name
    wchar_t shaderName[100];
    wcscpy_s(shaderName, 100, L"Shaders/");
    wcscat_s(shaderName, 100, name);

    DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;

#if defined( DEBUG ) || defined( _DEBUG )
    dwShaderFlags |= D3D10_SHADER_DEBUG;
#endif

    ID3D10Blob *pErrors;
    HRESULT hr = D3DX10CreateEffectFromFile( shaderName, pMacro, NULL, "fx_4_0", dwShaderFlags, 0, DXUTGetD3D10Device(), NULL, NULL, &pEffect, &pErrors, NULL );
    if (hr != S_OK)
    {
        if (pErrors != NULL)
        {
            OutputDebugStringA((char *)pErrors->GetBufferPointer());
            pErrors->Release();
        }
        return NULL;
    }

    return pEffect;
}

/**
 * @fn CreateIndexBufferImmutable(int numIndices, int *pNumIndices)
 *
 * @brief Creates an immutable index buffer
 *
 * @param numIndices - The number of indices to create in the buffer
 *
 * @return - pNumIndices - An array of numIndices indices  (pNumIndices[numIndices])
 */
ID3D10Buffer* CreateIndexBufferImmutable(int numIndices, int *pIndices)
{
    ID3D10Buffer *pIBuff;

    // Declare and initialize the initialization data
    D3D10_SUBRESOURCE_DATA initData;
    initData.pSysMem          = pIndices;
    initData.SysMemPitch      = 0;
    initData.SysMemSlicePitch = 0;

    // Prepare the buffer description
    CD3D10_BUFFER_DESC desc(numIndices * sizeof(int), D3D10_BIND_INDEX_BUFFER, D3D10_USAGE_IMMUTABLE);
    // Create the buffer
    HRESULT hr = DXUTGetD3D10Device()->CreateBuffer(&desc, &initData, &pIBuff);
    // Validate the error return code
    assert(hr == S_OK);
    hr;

    // Return the index buffer
    return pIBuff;
}

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
ID3D10Buffer* CreateGridIndexBuffer(int dimW, int dimH, int *pNumIndices)
{
    ID3D10Buffer *pIBuff;

    // Compute the number of indices, 6 per cell
    int numIndices = 2 * 3 * (dimW - 1) * (dimH - 1);
    // Validate the output variable
    if (pNumIndices != NULL)
    {
        // If valid, set the number of indices to be created
        *pNumIndices = numIndices;
    }

    // Alloicate memory
    int *pMem = (int*)_mm_malloc(numIndices * sizeof(int), 16);

    // Prepare to create the buffer. Declare a working pointer.
    int *pIB = pMem;
    // The initial offset is zero
    int offs = 0;

    // For every horizontal line of cells
    for (int y = 0; y < (dimH-1); y++, offs++)
    {
        // For every horizontal cell in the line
        for (int x = 0; x < (dimW-1); x++, offs++)
        {
            // Write the indices into the buffer
            pIB[0] = offs;
            pIB[2] = offs + 1;
            pIB[1] = offs + dimW + 1;
            pIB[3] = offs;
            pIB[5] = offs + dimW + 1;
            pIB[4] = offs + dimW;
            // Increment the buffer pointer
            pIB += 6;
        }
    }

    pIBuff = CreateIndexBufferImmutable(numIndices, pMem);

    // Free the initialization data
    _mm_free(pMem);

    // Return the created buffer
    return pIBuff;
}


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
ID3D10Buffer* CreateVertexBuffer(int size, D3D10_USAGE usage)
{
    ID3D10Buffer *pVBuff;
    D3D10_BUFFER_DESC desc;
    desc.Usage            = usage;
    desc.ByteWidth        = size;
    desc.BindFlags        = D3D10_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags   = usage == D3D10_USAGE_DYNAMIC ? D3D10_CPU_ACCESS_WRITE : 0;
    desc.MiscFlags        = 0;
    HRESULT hr = DXUTGetD3D10Device()->CreateBuffer(&desc, NULL, &pVBuff);
    assert(hr == S_OK);
    hr;

    return pVBuff;
}

/**
 * @fn CreateVertexBufferImmutable(CVBContentGenerator const &gen)
 *
 * @brief Creates an immutable vertext buffer.
 *
 * @param gen - Address of a content generator object
 *
 * @return - Pointer to the created immutable vertex buffer
 */
ID3D10Buffer* CreateVertexBufferImmutable(CVBContentGenerator const &gen)
{
    int size = gen.GetSize();

    // Declare and initizlize initialization data
    D3D10_SUBRESOURCE_DATA initData;
    void *pMem = _mm_malloc(size, 16);
    initData.pSysMem = pMem;
    initData.SysMemPitch      = 0;
    initData.SysMemSlicePitch = 0;

    // Generate the contents
    gen.Generate(pMem);

    // Declare a vertex buffer
    ID3D10Buffer *pVBuff;
    // Create and initialize a buffer description
    CD3D10_BUFFER_DESC desc(size, D3D10_BIND_VERTEX_BUFFER, D3D10_USAGE_IMMUTABLE);
    // Create the buffer
    HRESULT hr = DXUTGetD3D10Device()->CreateBuffer(&desc, &initData, &pVBuff);
    // Validate the return code
    assert(hr == S_OK);
    hr;

    // Free working memory
    _mm_free(pMem);

    // Return the pointer to the vertext buffer
    return pVBuff;
}

/**
 * @fn LoadShaderResourceView(wchar_t const *name)
 *
 * @brief Loads the named shader resource view
 *
 * @param name - The name of the shader resource view to load
 *
 * @return - Pointer to the shader resource view
 */
ID3D10ShaderResourceView* LoadShaderResourceView(wchar_t const *name)
{
/*	D3DX10_IMAGE_LOAD_INFO loadInfo;
	loadInfo.Width = D3DX10_DEFAULT;
	loadInfo.Height = D3DX10_DEFAULT;
	loadInfo.Depth = D3DX10_DEFAULT;
	loadInfo.FirstMipLevel = D3DX10_DEFAULT;
	loadInfo.MipLevels = D3DX10_DEFAULT;
	loadInfo.Usage = D3D10_USAGE_IMMUTABLE;
	loadInfo.BindFlags = D3D10_BIND_SHADER_RESOURCE;
	loadInfo.CpuAccessFlags = 0;
	loadInfo.MiscFlags = 0;
	loadInfo.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	loadInfo.Filter = D3DX10_FILTER_NONE | D3DX10_FILTER_SRGB;
	loadInfo.MipFilter = D3DX10_DEFAULT;
	loadInfo.pSrcInfo = NULL;
*/
    ID3D10ShaderResourceView *pRV;
    HRESULT hr = D3DX10CreateShaderResourceViewFromFile( DXUTGetD3D10Device(), name, /*&loadInfo*/NULL, NULL, &pRV, NULL );
    if (hr != S_OK)
    {
        return NULL;
    }
    return pRV;
}

/**
 * @fn DXFormatSize(DXGI_FORMAT format)
 *
 * @brief Obtains the size of the buffer data
 *
 * @param format - The enumerated format type
 *
 * @return - The byte size of the data
 */
int DXFormatSize(DXGI_FORMAT format)
{
    if (DXGI_FORMAT_R32G32B32A32_TYPELESS <= format && format <= DXGI_FORMAT_R32G32B32A32_SINT)
    {
        return 16;
    }
    if (DXGI_FORMAT_R32G32B32_TYPELESS <= format && format <= DXGI_FORMAT_R32G32B32_SINT)
    {
        return 12;
    }
    if (DXGI_FORMAT_R16G16B16A16_TYPELESS <= format && format <= DXGI_FORMAT_X32_TYPELESS_G8X24_UINT)
    {
        return 8;
    }
    if (DXGI_FORMAT_R10G10B10A2_TYPELESS <= format && format <= DXGI_FORMAT_X24_TYPELESS_G8_UINT)
    {
        return 4;
    }
    if (DXGI_FORMAT_R8G8_TYPELESS <= format && format <= DXGI_FORMAT_R16_SINT)
    {
        return 2;
    }
    if (DXGI_FORMAT_R8_TYPELESS <= format && format <= DXGI_FORMAT_A8_UNORM)
    {
        return 1;
    }
    // Skip the rest
    return 0;
}

/**
 * @fn DropResources()
 *
 * @brief Drops all shader resources from the output device
 */
void DropResources()
{
    void* arr[4] = { NULL, NULL, NULL, NULL };
    DXUTGetD3D10Device()->PSSetShaderResources(0, 4, (ID3D10ShaderResourceView**)arr);
    DXUTGetD3D10Device()->VSSetShaderResources(0, 4, (ID3D10ShaderResourceView**)arr);
}
