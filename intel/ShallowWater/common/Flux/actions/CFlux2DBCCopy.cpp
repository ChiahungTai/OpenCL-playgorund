/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFile2DBCCopy.cpp
 *
 * @brief Contains the basic boundary conditions (BC) implementation. 
 */

#include "Flux/actions/CFlux2DBCCopy.h"
#include "physics_3D_fluid_sim_common.h"
#include "Flux/CFlux2DGrid.h"


CFlux2DBCCopy::CFlux2DBCCopy(CFile &file)
{
    file.Restore(m_CopyPlace);
}

void CFlux2DBCCopy::SaveData(CFile &file) const
{
    file.Save(m_CopyPlace);
}

/**
 * @fn Run(pG, pars, globals )
 *
 * @brief This method runs the copy process
 *
 * @note This function implements the free boundary condition. The function
 * calculates the u and v velocity components according to the new water level
 * on the boundary.
 *
 * @param pG - The grid to be modified
 *
 * @param pars - Address of the current simulation step parameters
 *
 * @param globals - Address of the global simulation parameters
 */
void CFlux2DBCCopy::Run( CFlux2DGrid* pG, 
    CFlux2DCalculatorPars const &pars, 
    CFlux2DGlobalParameters const &globals )
{
    // Initialize working variables;
    int i,num=0;
    int x0=0,y0=0,dx=0,dy=0,SrcDX=0,SrcDY=0;
    DynamicMatrix<float>* pH = &pG->GetDestSurface().m_heightMap;

    DynamicMatrix<float>* pU = &pG->GetDestSurface().m_uMap;
    DynamicMatrix<float>* pV = &pG->GetDestSurface().m_vMap;
    DynamicMatrix<float>* pB = &pG->GetBottom().m_heightMap;

    // Depending on the active border, compute the source DX and DY
    switch(m_CopyPlace)
    {
    case TOP:
        num = pH->Width();
        x0 = 0;
        y0 = -1;
        dx = 1;
        dy = 0;
        SrcDX = 0;
        SrcDY = 1;
        break;
    case BOTTOM:
        num = pH->Width();
        x0 = 0;
        y0 = pH->Height();
        dx = 1;
        dy = 0;
        SrcDX = 0;
        SrcDY = -1;
        break;
    case LEFT:
        num = pH->Height();
        x0 = -1;
        y0 = 0;
        dx = 0;
        dy = 1;
        SrcDX = 1;
        SrcDY = 0;
        break;
    case RIGHT:
        num = pH->Height();
        x0 = pH->Width();
        y0 = 0;
        dx = 0;
        dy = 1;
        SrcDX = -1;
        SrcDY = 0;
        break;
    }


    // If we have a border with a value
    for(i=0;i<num;++i)
    {
        // Initialize working variables
        int x = x0+i*dx;
        int y = y0+i*dy;
        int xs = x + SrcDX;
        int ys = y + SrcDY;

        // Compute the height
        float Hval = pH->At(xs,ys);
        if (Hval > 0.0f)
        {
            pH->At(x,y) = __max(0.0f, (Hval - pB->At(xs,ys)) + pB->At(x,y));
        }
        else
        {
            pH->At(x,y) = 0.0f;
        }
        // Handle speed
        pU->At(x,y) = pU->At(xs,ys);
        pV->At(x,y) = pV->At(xs,ys);
    }
}
// End of File