/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CSceneVisual.cpp
 *
 * @brief implements a class to manage an array of visuals
 */

#include <DXUT.h>
#include "CSceneVisual.h"
#include "CVisual3DSimple.h"
#include "CVisual3DGathered.h"


#include "common/physics_3D_fluid_sim_common.h"

float const g_visualizationGridStep = 2.0f;
int const   g_minMaxDimention = 256;//128;//512;//1024;//256

/**
 * @fn CSceneVisual()
 *
 * @brief Class constructor
 *
 * @param rScene - The scene
 *
 * @param rCommonData - Common data for visuals
 */
CSceneVisual::CSceneVisual(CFlux2DScene &rScene, CVisual3DCommonData &rCommonData)
    : m_scene(rScene),
    m_commonData(rCommonData),
    m_bWireframe(false),
    m_VisualEngineGatheredIndex(0),
    m_VisualEngineIndex(0),
    m_VisualModeIndex(0)
{
    // Create a simple engine

    AddVisualEngine(new CVisual3DSimple(rScene, m_commonData));

    // Determine the resolution, first, get the region
    CRegion const &r = rScene.GetRegion();
    // Compute the horizontal and vertical lengths
    float lenX = r.x2 - r.x1;
    float lenY = r.y2 - r.y1;

    int   dimX, dimY;

    // If the the region is square or it is longer horizontally than vertically
    if (lenX >= lenY)
    {
        // Compute a dimension factor by dividing the horizontal length by the number of grid steps
        int dim = int(lenX / g_visualizationGridStep);
        // If the dimension is less than the minMax value, use the minMax value
        if (dim < g_minMaxDimention)
        {
            dimX = g_minMaxDimention;
        }
        else // However, if the factor is equal to or greater than the minMax value
        {
            // Get the next highest power of 2 to use as the factor
            dimX = NextPowOf2(dim);
        }
        // The Y dimension is the next highest power of 2 after taking the aspect ratio into account
        dimY = NextPowOf2( int( float(dimX) * lenY / lenX ) );
    }
    else // The vertical component is larger than the horizontal component
    {
        // Divide the vertical component by the number of grid steps to get a factor
        int dim = int(lenY / g_visualizationGridStep);
        // And test the result against the minMax value
        if (dim < g_minMaxDimention)
        {
            // If less than the minMax value, use the MinMax value
            dimY = g_minMaxDimention;
        }
        else // If the factor is equal to or greater than the minMax value
        {
            // Get the next highest power of two to use as the factor
            dimY = NextPowOf2(dim);
        }
        // The X dimension is the next hightest power of 2 after taking the aspect ration into account
        dimX = NextPowOf2( int( float(dimY) * lenX / lenY ) );
    }

    // Create "gathered"
    CVisual3DGathered::MacroDefinition();
    m_VisualEngineGatheredIndex = AddVisualEngine(new CVisual3DGathered(rScene, m_commonData, dimX, dimY));

    

    // Fill the visualization modes List
    AddVisualMode(new VisModeLevelColored(2.0f, VisColor(0.15f, 0.9f, 1.35f)));
    AddVisualMode(new VisModeSpeed(2.0f));
    AddVisualMode(new VisModeHeightColored(2.0f, VisColor(0.52f, 0.86f, 0.99f), VisColor(0.2f, 0.31f, 0.68f), VisColor(0.97f, 0.91f, 0.6f)));
    AddVisualMode(new VisModeCritical);
    AddVisualMode(new VisModeFroude);
    m_VisualEngineIndex = m_VisualEngineGatheredIndex;

    // Create state object and initialize
    D3D10_RASTERIZER_DESC state;
    state.FillMode = D3D10_FILL_SOLID;
    state.CullMode = D3D10_CULL_NONE;
    state.FrontCounterClockwise = true;
    state.DepthBias = 0;
    state.DepthBiasClamp = 0.0f;
    state.SlopeScaledDepthBias = 0.0f;
    state.DepthClipEnable = true;
    state.ScissorEnable = false;
    state.MultisampleEnable = false;
    state.AntialiasedLineEnable = false;
    // Create a rasterizer state for FILL SOLID
    HRESULT hr = DXUTGetD3D10Device()->CreateRasterizerState(&state, &m_pStateBase);
    assert(hr == S_OK);

    // Create a rasterizer state for wireframe
    state.FillMode = D3D10_FILL_WIREFRAME;
    hr = DXUTGetD3D10Device()->CreateRasterizerState(&state, &m_pStateWire);
    assert(hr == S_OK);
}

/**
 * @fn ~CSceneVisual()
 *
 * @brief Class destructor
 */
CSceneVisual::~CSceneVisual()
{
    int i;
    // Release the two states
    SAFE_RELEASE(m_pStateBase);
    SAFE_RELEASE(m_pStateWire);
    // Delete visual engines
    for(i = 0; i< m_VisualModes.Size()+ m_VisualEngines.Size() - 1; i++)
    {
        RemoveEngine(i);
    }
    for(i = 0; i<m_VisualEngines.Size(); i++)
    {
        SAFE_DELETE(m_VisualEngines[i]);
    }
    // And delete visualization modes
    for(i = 0; i<m_VisualModes.Size(); i++)
    {
        SAFE_DELETE(m_VisualModes[i]);
    }
}

/**
 * @fn Update()
 *
 * @brief Updates the current visual mode of the current visual engine 
 *
 * @param dt - A new visual mode index
 */
void CSceneVisual::Update(float dt)
{
    m_VisualEngines[m_VisualEngineIndex]->Update(m_VisualModes[m_VisualModeIndex], dt);
}

/**
 * @fn Render()
 *
 * @brief Renders the bottom and surface
 *
 * @param rWorldViewProj - the world view projection matrix
 *
 * @param rComPos - The camera position
 */
void CSceneVisual::Render(D3DXMATRIX const &rWorldViewProj, D3DXVECTOR3 const &rCamPos)
{
    // Declare a working visual engine and initialize it
    CVisual3DInterface* pVisEngine = m_VisualEngines[m_VisualEngineIndex];
    // Render the bottom
    pVisEngine->RenderBottom(rWorldViewProj);
    // If wireframe is enabled
    if (m_bWireframe)
    {
        // Set the render state
        DXUTGetD3D10Device()->RSSetState(m_pStateWire);
    }

    // Render the surface
    pVisEngine->RenderSurface(rWorldViewProj, rCamPos);
    // If wireframe is enabled
    if (m_bWireframe)
    {
        // Set the render state
        DXUTGetD3D10Device()->RSSetState(m_pStateBase);
    }
}

/**
 * @fn GatheredVisual()
 *
 * @brief Validates the index and casts the appropriate visual engine if valid.
 *
 * @return - Pointer to the gathered visual if successful
 *           NULL if not
 */
CVisual3DGathered* CSceneVisual::GatheredVisual()
{
    if(m_VisualEngineGatheredIndex<0||m_VisualEngineGatheredIndex>=m_VisualEngines.Size())
    {
        return NULL;
    }

    return static_cast<CVisual3DGathered*>(m_VisualEngines[m_VisualEngineGatheredIndex]);
}

