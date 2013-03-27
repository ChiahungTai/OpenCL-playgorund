/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CSceneVisual.h
 *
 * @brief Implements a class containing the high level visualization 
 * routines implementation and declaration for the scene visualization in the shader
 */

#pragma once

#include <d3d10.h>
#include "flux/CFlux2DScene.h"
#include "Utils/vismodes.h"
#include "CVisual3DCommonData.h"
#include "CVisual3DInterface.h"

class CSceneVisual
{
public:
    enum 
    { 
        VIS_SIMPLE = 0,
        VIS_GATHERED,
        VIS_LAST,
    };

    /**
     * @fn CSceneVisual()
     *
     * @brief Class constructor
     *
     * @param rScene - The scene
     *
     * @param rCommonData - Common data for visuals
     */
    CSceneVisual(CFlux2DScene &rScene, CVisual3DCommonData &rCommonData);

    /**
     * @fn ~CSceneVisual()
     *
     * @brief Class destructor
     */
    ~CSceneVisual();

    /**
     * @fn Update()
     *
     * @brief Updates the current visual mode of the current visual engine 
     *
     * @param dt - A new visual mode index
     */
    void Update(float dt);

    /**
     * @fn Render()
     *
     * @brief Renders the bottom and surface
     *
     * @param rWorldViewProj - the world view projection matrix
     *
     * @param rComPos - The camera position
     */
    void Render(D3DXMATRIX const &rWorldViewProj, D3DXVECTOR3 const &rCamPos);

    /**
     * @fn GetVisualModeName()
     *
     * @brief Gets the visual mode name for a specific mode from the array of visual modes
     *
     * @param i - Index into the array of visual modes
     *
     * @return - The name of the visual mode
     */
    wchar_t const *GetVisualModeName(int i) 
    { 
        if(i>=0 && i < m_VisualModes.Size())
        {
            return m_VisualModes[i]->GetName(); 
        }
        return NULL;
    }

    /**
     * @fn GetName()
     *
     * @brief Gets the visual name for a specific visual from the array of visuals
     *
     * @param i - Index into the array of visuals
     *
     * @return - The name of the visual
     */
    wchar_t const *GetName(int i)
    {
        if(i>=0 && i<m_Visuals.Size())
        {
            return m_Visuals[i]->Name;
        }
        return NULL;
    }

    /**
     * @fn SetMode()
     *
     * @brief Sets the visual index for a specific visual in the array of visuals
     *
     * @param i - Index into the array of visuals
     */
    void SetMode(int i)
    {
        if(i>=0 && i < m_Visuals.Size())
        {
            if(m_Visuals[i]->VisualModeIndex == -1)
            {
                SetVisualIndex(m_Visuals[i]->VisualEngineIndex);
            }
            else
            {
                SetVisualIndex(m_Visuals[i]->VisualEngineIndex);
                SetVisualMode(m_Visuals[i]->VisualModeIndex);

            }
        }
    }

    /**
     * @fn SetVisualMode()
     *
     * @brief Sets the visual mode index
     *
     * @param i - The new visual mode index
     */
    void SetVisualMode(int i) 
    {
        if(i>=0 && i < m_VisualModes.Size())
        {
            m_VisualModeIndex = i; 
        }
    }

    /**
     * @fn SetWireframe()
     *
     * @brief Sets the wireframe flag
     *
     * @param bWireFrame - TRUE for wireframe output, FALSE otherwise
     */
    void SetWireframe(bool bWireframe) 
    { 
        m_bWireframe = bWireframe; 
    }

    /**
     * @fn SetVisualIndex()
     *
     * @brief Sets the visual engine index
     *
     * @param visIndex - The new visual index
     */
    void SetVisualIndex(int visIndex) 
    { 
        m_VisualEngineIndex = visIndex; 
    }

    /**
     * @fn GetVisualIndex()
     *
     * @brief Gets the visual engine index
     *
     * @return - The current visual index
     */
    int  GetVisualIndex() const 
    { 
        return m_VisualEngineIndex; 
    }

    /**
     * @fn GetVisualName()
     *
     * @brief Gets the visual engine nane
     *
     * @return - The current visual engine name
     */
    wchar_t const * GetVisualName(int visIndex) const 
    { 
        if(visIndex<0 || visIndex>=m_VisualEngines.Size()) return NULL;
        return m_VisualEngines[visIndex]->GetName();

    }


    /**
     * @fn GatheredVisual()
     *
     * @brief Validates the index and casts the appropriate visual engine if valid.
     *
     * @return - Pointer to the gathered visual if successful
     *           NULL if not
     */
    class CVisual3DGathered* GatheredVisual(); // Could return NULL (in future)

private:

    CFlux2DScene&          m_scene;             // reference to visualized scene
    CVisual3DCommonData&   m_commonData;        // reference to common data
    bool                   m_bWireframe;        // flag to draw wireframe

    /**
     * @struct Visual 
     *
     * @brief A structure to store visual parameters, name, engine index and visual mode index
     */
    struct Visual
    {
        Visual(const wchar_t* in_name, int in_veIndex, int in_vmIndex)
        {
            Name = new wchar_t[wcslen(in_name) + 1];
            Name[wcslen(in_name)] = (wchar_t)0;
            memcpy(Name, in_name, sizeof(wchar_t) * wcslen(in_name));
            VisualEngineIndex = in_veIndex;
            VisualModeIndex = in_vmIndex;
        }
        wchar_t*     Name;
        int         VisualEngineIndex;
        int         VisualModeIndex;

    };

    DynamicArray<Visual*> m_Visuals;

    /**
     * @fn AddVisualMode()
     *
     * @brief Adds a visual mode to the array
     *
     * @param in_pVm - The new visual mode to add
     */
    void AddVisualMode(VisMode* in_pVm)
    {
        // If the current visual engine can be removed, then replace it
        if(TryToRemoveEngine(m_VisualEngineIndex))
        {
            m_Visuals[m_Visuals.Size()-1] = new Visual(in_pVm->GetName(), m_VisualEngineIndex, m_VisualModes.Size());
        }
        else // However, if it cannot be removed, add the new visual mode to the array
        {
            m_Visuals.Append(new Visual(in_pVm->GetName(), m_VisualEngineIndex, m_VisualModes.Size()));
        }
        m_VisualModes.Append(in_pVm);
    }

    /**
     * @fn TryToRemoveEngine()
     *
     * @brief Tries to remove engine from the visuals array. Only modes for this engine will be displaced in GUI.
     *
     * @param in_VEIndex - The index into the m_VisualEngines array
     *
     * @return TRUE if the engine can be removed, FALSE otherwise
     */
    bool TryToRemoveEngine(int in_VEIndex)
    {
        for(int i = 0; i<m_VisualEngines.Size(); i++)
        {
            if ((m_Visuals[i]->VisualEngineIndex==in_VEIndex) && (m_Visuals[i]->VisualModeIndex == -1))
            {
                // Remove the engine from visuals
                Visual* lastVis = m_Visuals[m_Visuals.Size() - 1];
                Visual* visToDelete = m_Visuals[i];
                m_Visuals[i] = lastVis;
                delete[] visToDelete->Name;
                delete visToDelete;
                return true;
            }
        }
        return false;
    }

    /**
     * @fn RemoveEngine()
     *
     * @brief Remove engine from the visuals array. 
     *
     * @param in_VEIndex - The index into the m_VisualEngines array.
     *
     */    
    void RemoveEngine(int in_VEIndex)
    {
        delete[] m_Visuals[in_VEIndex]->Name;
        delete m_Visuals[in_VEIndex];
    }

    /**
     * @fn AddVisualEngine()
     *
     * @brief Adds a visual engine to the array
     *
     * @parem in_pVe - The visual engine to add
     *
     * @return - Index to the new visual engine
     */
    int AddVisualEngine(CVisual3DInterface*  in_pVe)
    {
        m_Visuals.Append(new Visual(in_pVe->GetName(), m_VisualEngines.Size(), -1));
        m_VisualEngines.Append(in_pVe);
        return m_VisualEngines.Size()-1;
    }

    // A list of visualization engines
    DynamicArray<CVisual3DInterface*>  m_VisualEngines;
    int                                m_VisualEngineIndex;             // Index of the current visualization engine
    int                                m_VisualEngineGatheredIndex;     // Index of the gathered visualization engine

    // A list of color calculators for each vertex
    DynamicArray<VisMode*>  m_VisualModes;
    int                     m_VisualModeIndex;                          // The current visual mode.

    // Effect states
    ID3D10RasterizerState *m_pStateBase;
    ID3D10RasterizerState *m_pStateWire;
};
// End of File