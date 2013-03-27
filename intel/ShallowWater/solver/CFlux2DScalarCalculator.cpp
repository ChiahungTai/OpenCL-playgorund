/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFlux2DScalarCalculator.h
 *
 * @brief This file implements a non-optimized calculator for shallow water simulation based on a flux splitting approach
 */

#include "CFlux2DScalarCalculator.h"

/**
 * @fn CalcU_Neg_MoreCritical()
 *
 * @brief This method calculates the flow component related with a NEGATIVE water bar speed
 *        U component that has an absolute value that is MORE than the wave speed in the node
 */
void CFlux2DScalarCalculator::CalcU_Neg_MoreCritical()
{
    /* ******************************************************************** *\
                                   Formulas used

                                    fH = dp[Hu]
                                    fU = dp[H*(0.5gH + u^2)]
                                    fV = dp[Huv]
                                    Ru = 0.5g(Hp+H)(hp-h)
    \* ******************************************************************** */

    // Now load new values for dp
    float H_  = inPtr.pH[1];
    float u_  = inPtr.pU[1];
    float v_  = inPtr.pV[1];
    float Hb_ = pHb[1];

    // First do the calculations with the center (H, u,v) point in the grid
    float tmpFlowH = -cur.H * cur.u;
    float tmpFlowU = -cur.H * (halfGravity * cur.H + cur.u * cur.u);
    float tmpFlowV = -cur.H * cur.u * cur.v;

    // Calculate the positive portion of the water flow  
    tmpFlowH += H_ * u_;
    tmpFlowU += H_ * (halfGravity * H_ + u_ * u_);
    tmpFlowV += H_ * u_ * v_;
    // In the final formula, Ru is taken with a minus
    tmpFlowU -= halfGravity * (H_ + cur.H) * (Hb_ - curHb);

    // For the final step, write the result to the flow output
    flow.H += tmpFlowH * tau_div_deltaw;
    flow.u += tmpFlowU * tau_div_deltaw;
    flow.v += tmpFlowV * tau_div_deltaw;
}

/**
 * @fn CalcU_Pos_MoreCritical()
 *
 * @brief This method calculates the flow component related with a POSITIVE water bar speed U component 
 *        that has an absolute value MORE than the wave speed in the node
 */
void CFlux2DScalarCalculator::CalcU_Pos_MoreCritical()
{
    /* ******************************************************************** *\
                                   Formulas used

                                    fH = dm[Hu]
                                    fU = dm[H*(0.5gH + u^2)]
                                    fV = dm[Huv]
                                    Ru = 0.5g(H+Hm)(h-hm)
    \* ******************************************************************** */

    // Load values for the left node
    float H_  = inPtr.pH[-1];
    float u_  = inPtr.pU[-1];
    float v_  = inPtr.pV[-1];
    float Hb_ = pHb[-1];

    // First do the calculations with the center (H, u,v) point in the grid
    float tmpFlowH = cur.H * cur.u;
    float tmpFlowU = cur.H * (halfGravity * cur.H + cur.u * cur.u);
    float tmpFlowV = cur.H * cur.u * cur.v;

    // Calculate the positive portion of the water flow  
    tmpFlowH -= H_ * u_;
    tmpFlowU -= H_ * (halfGravity * H_ + u_ * u_);
    tmpFlowV -= H_ * u_ * v_;

    // In the final formula, Ru is taken with a minus
    tmpFlowU -= halfGravity * (cur.H + H_) * (curHb - Hb_);
    
    // Finally write to the output flows
    flow.H += tmpFlowH * tau_div_deltaw;
    flow.u += tmpFlowU * tau_div_deltaw;
    flow.v += tmpFlowV * tau_div_deltaw;
}

/**
 * @fn CalcU_Neg_LessCritical()
 *
 * @brief This method calculates the flow component related with a NEGATIVE water bar speed U component 
 *        that has an absolute value LESS than the wave speed in the node
 */
void CFlux2DScalarCalculator::CalcU_Neg_LessCritical()
{
    /* ******************************************************************** *\
                                   Formulas used

                fH = 0.25 * ( dm[H(u+c)]        + dp[H(3u-c)] )
                fU = 0.25 * ( dm[H(u^2+2uc+gH)] + dp[H(3u^2-2uc+gH)]
                fV = 0.25 * ( dm[H(u+c)v]       + dp[H(3u-c)v] )
                Rh = 0.25 * ( -dm[h*sqrt(gh)]   + dp[h*sqrt(gh)] )
                Ru = 0.25 * ( (H+Hm)(h-hm)      + (Hp+H)(hp-h)  ) * g
    \* ******************************************************************** */

    // First do the calculations with the center (H, u,v) point in the grid
    float tmpFlowH = 2.0f * cur.H * (curSpeed - cur.u);
    float tmpFlowU = 2.0f * cur.H * (2.0f * curSpeed - cur.u) * cur.u;
    float tmpFlowV = tmpFlowH * cur.v;
    tmpFlowH -= 2.0f * curHbSqrt;

    // Load values for the left node
    float H_  = inPtr.pH[-1];
    float u_  = inPtr.pU[-1];
    float v_  = inPtr.pV[-1];
    float Hb_ = pHb[-1];
    float HbSqrt_ = pHbSqrt[-1];
    float speed_ = pSpeed[-1];

    // The part of the flow from the left node
    tmpFlowH -= H_ * (u_ + speed_);
    tmpFlowU -= H_ * (u_ * u_ + 2.0f * u_ * speed_ + gravity * H_);
    tmpFlowV -= H_ * (u_ + speed_) * v_;
    tmpFlowH += HbSqrt_;
    // In the final formula, Ru is taken with a minus
    tmpFlowU -= gravity * (cur.H + H_) * (curHb - Hb_); 

    // Load new values for the right node
    H_  = inPtr.pH[1];
    u_  = inPtr.pU[1];
    v_  = inPtr.pV[1];
    Hb_ = pHb[1];
    HbSqrt_ = pHbSqrt[1];
    speed_ = pSpeed[1]; 
    
    // The part of the flow from the right node index
    tmpFlowH += H_ * (3.0f * u_ - speed_);
    tmpFlowU += H_ * (3.0f * u_ * u_ - 2.0f * u_ * speed_ + gravity * H_);
    tmpFlowV += H_ * (3.0f * u_ - speed_) * v_;
    tmpFlowH += HbSqrt_;
    // In the final formula, Ru is taken with a minus
    tmpFlowU -= gravity * (H_ + cur.H) * (Hb_ - curHb);

    // For the final step, write the result to the flow outputs
    flow.H += tmpFlowH * tau_div_deltaw_quarter;
    flow.u += tmpFlowU * tau_div_deltaw_quarter;
    flow.v += tmpFlowV * tau_div_deltaw_quarter;
}

/**
 * @fn CalcU_Pos_LessCritical()
 *
 * @brief This method calculates the flow component related with a POSITIVE water bar speed U component 
 *        that has absolute value LESS than the wave speed in the node
 */
void CFlux2DScalarCalculator::CalcU_Pos_LessCritical()
{
    /* ******************************************************************** *\
                                   Formulas used

                fH = 0.25 * ( dm[H(3u-c)]        + dp[H(u+c)] )
                fU = 0.25 * ( dm[H(3u^2+2uc+gH)] + dp[H(u^2-2uc+gH)]
                fV = 0.25 * ( dm[H(3u+c)v]       + dp[H(u-c)v] )
                Rh = 0.25 * ( -dm[h*sqrt(gh)]    + dp[h*sqrt(gh)] )
                Ru = 0.25 * ( (H+Hm)(h-hm)       + (Hp+H)(hp-h)  ) * g
    \* ******************************************************************** */

    // First do the calculations with the center (H, u,v) point in the grid
    float tmpFlowH = 2.0f * cur.H * (curSpeed + cur.u);
    float tmpFlowU = 2.0f * cur.H * (2.0f * curSpeed + cur.u) * cur.u;
    float tmpFlowV = tmpFlowH * cur.v;
    tmpFlowH -= 2.0f * curHbSqrt;

    // Now load new values for dm
    float H_  = inPtr.pH[-1];
    float u_  = inPtr.pU[-1];
    float v_  = inPtr.pV[-1];
    float Hb_ = pHb[-1];
    float HbSqrt_ = pHbSqrt[-1];
    float speed_ = pSpeed[-1]; 

    // The part of the flow from the negative index
    tmpFlowH -= H_ * (3.0f * u_ + speed_);
    tmpFlowU -= H_ * (3.0f * u_ * u_ + 2.0f * u_ * speed_ + gravity * H_);
    tmpFlowV -= H_ * (3.0f * u_ + speed_) * v_;
    tmpFlowH += HbSqrt_;
    // In the final formula, Ru is taken with a minus
    tmpFlowU -= gravity * (cur.H + H_) * (curHb - Hb_); 

    // Now load new values for dp
    H_  = inPtr.pH[1];
    u_  = inPtr.pU[1];
    v_  = inPtr.pV[1];
    Hb_ = pHb[1];
    HbSqrt_ = pHbSqrt[1];
    speed_ = pSpeed[1]; 
    
    // The part of the flow from the negative index
    tmpFlowH += H_ * (u_ - speed_);
    tmpFlowU += H_ * (u_ * u_ - 2.0f * u_ * speed_ + gravity * H_);
    tmpFlowV += H_ * (u_ - speed_) * v_;
    tmpFlowH += HbSqrt_;
    // In the final formula, Ru is taken with a minus
    tmpFlowU -= gravity * (H_ + cur.H) * (Hb_ - curHb); 

    // For the final step, write the result to the flow outputs
    flow.H += tmpFlowH * tau_div_deltaw_quarter;
    flow.u += tmpFlowU * tau_div_deltaw_quarter;
    flow.v += tmpFlowV * tau_div_deltaw_quarter;
}

/**
 * @note V versions of the calculations are made from the corresponding U versions
 *       by looking into the formulas and replacing "U" with "V" and vise versa.
 *       U and V flows are also exchanged. +-1 offsets are replaced with stride.
 *       Rh, Ru, Rv (they are named Q values in all documents) remain unchanged
 */

/**
 * @fn CalcV_Neg_MoreCritical()
 *
 * @brief This method calculates the flow component related with a NEGATIVE water bar speed V component 
 *        that has an absolute value MORE than the wave speed in the node
 */
void CFlux2DScalarCalculator::CalcV_Neg_MoreCritical()
{
    // load values for the down node
    float H_  = inPtrDn.pH[0];
    float u_  = inPtrDn.pU[0];
    float v_  = inPtrDn.pV[0];
    float Hb_ = pHbDn[0];
    
    // First do the calculations with the center (H, u,v) point in the grid
    float tmpFlowH = -cur.H * cur.v;
    float tmpFlowU = -cur.H * cur.v * cur.u;
    float tmpFlowV = -cur.H * (halfGravity * cur.H + cur.v * cur.v);

    // Calculate the positive portion of the water flow  
    tmpFlowH += H_ * v_;
    tmpFlowU += H_ * v_ * u_;
    tmpFlowV += H_ * (halfGravity * H_ + v_ * v_);

    // In the final formula, Ru is taken with a minus
    tmpFlowV -= halfGravity * (H_ + cur.H) * (Hb_ - curHb);

    // For the final step, write the results to the flow outputs
    flow.H += tmpFlowH * tau_div_deltah;
    flow.u += tmpFlowU * tau_div_deltah;
    flow.v += tmpFlowV * tau_div_deltah;
}

/**
 * @fn CalcV_Pos_MoreCritical()
 *
 * @brief  This method calculates the flow component related with a POSITIVE water bar speed V component 
 *         that has an absolute value MORE than the wave speed in the node
 */
void CFlux2DScalarCalculator::CalcV_Pos_MoreCritical()
{
    // Load values for the up node
    float H_  = inPtrUp.pH[0];
    float u_  = inPtrUp.pU[0];
    float v_  = inPtrUp.pV[0];
    float Hb_ = pHbUp[0];

    // First do the calculations with the center (H, u,v) point in the grid
    float tmpFlowH = cur.H * cur.v;
    float tmpFlowU = cur.H * cur.v * cur.u;
    float tmpFlowV = cur.H * (halfGravity * cur.H + cur.v * cur.v);

    // Calculate the positive portion of the water flow  
    tmpFlowH -= H_ * v_;
    tmpFlowU -= H_ * v_ * u_;
    tmpFlowV -= H_ * (halfGravity * H_ + v_ * v_);

    // In the final formula, Ru is taken with a minus
    tmpFlowV -= halfGravity * (cur.H + H_) * (curHb - Hb_);

    // For the final step, write the results to the flow outputs
    flow.H += tmpFlowH * tau_div_deltah;
    flow.u += tmpFlowU * tau_div_deltah;
    flow.v += tmpFlowV * tau_div_deltah;
}

/**
 * @fn CalcV_Neg_LessCritical()
 *
 * @brief  This method calculates the flow component related with a NEGATIVE water bar speed V component 
 *         that has an absolute value LESS than the wave speed in the node
 */
void CFlux2DScalarCalculator::CalcV_Neg_LessCritical()
{
    // First do the calculations with the center (H, u,v) point in the grid
    float tmpFlowH = 2.0f * cur.H * (curSpeed - cur.v);
    float tmpFlowU = tmpFlowH * cur.u;
    float tmpFlowV = 2.0f * cur.H * (2.0f * curSpeed - cur.v) * cur.v;
    tmpFlowH -= 2.0f * curHbSqrt;

    // Now load new values for dm
    float H_  = inPtrUp.pH[0];
    float u_  = inPtrUp.pU[0];
    float v_  = inPtrUp.pV[0];
    float Hb_ = pHbUp[0];
    float speed_ = *pSpeedUp; 

    // The part of the flow from a negative index
    tmpFlowH -= H_ * (v_ + speed_);
    tmpFlowU -= H_ * (v_ + speed_) * u_;
    tmpFlowV -= H_ * (v_ * v_ + 2.0f * v_ * speed_ + gravity * H_);
    tmpFlowH += pHbSqrtUp[0];
    // In the final formula, Ru is taken with a minus
    tmpFlowV -= gravity * (cur.H + H_) * (curHb - Hb_); 

    // Now load new values for dp
    H_  = inPtrDn.pH[0];
    u_  = inPtrDn.pU[0];
    v_  = inPtrDn.pV[0];
    Hb_ = pHbDn[0];
    speed_ = *pSpeedDn; 

    // The part of the flow from a negative index
    tmpFlowH += H_ * (3.0f * v_ - speed_);
    tmpFlowU += H_ * (3.0f * v_ - speed_) * u_;
    tmpFlowV += H_ * (3.0f * v_ * v_ - 2.0f * v_ * speed_ + gravity * H_);
    tmpFlowH += pHbSqrtDn[0];

    // In the final formula, Ru is taken with a minus
    tmpFlowV -= gravity * (H_ + cur.H) * (Hb_ - curHb); 

    // Final step, write the results to the flow outputs
    flow.H += tmpFlowH * tau_div_deltah_quarter;
    flow.u += tmpFlowU * tau_div_deltah_quarter;
    flow.v += tmpFlowV * tau_div_deltah_quarter;
}

/**
 * @fn CalcV_Pos_LessCritical()
 *
 * @brief  This method calculates the flow component related with a POSITIVE water bar speed V component 
 * that has an absolute value LESS than the wave speed in the node
 */
void CFlux2DScalarCalculator::CalcV_Pos_LessCritical()
{
    // First do the calculations with the center (H, u,v) point in the grid
    float tmpFlowH = 2.0f * cur.H * (curSpeed + cur.v);
    float tmpFlowU = tmpFlowH * cur.u;
    float tmpFlowV = 2.0f * cur.H * (2.0f * curSpeed + cur.v) * cur.v;
    tmpFlowH -= 2.0f * curHbSqrt;

    // Now load new values for dm
    float H_  = inPtrUp.pH[0];
    float u_  = inPtrUp.pU[0];
    float v_  = inPtrUp.pV[0];
    float Hb_ = pHbUp[0];

    float speed_ = *pSpeedUp; 
    // The part of the flow from a negative index
    tmpFlowH -= H_ * (3.0f * v_ + speed_);
    tmpFlowU -= H_ * (3.0f * v_ + speed_) * u_;
    tmpFlowV -= H_ * (3.0f * v_ * v_ + 2.0f * v_ * speed_ + gravity * H_);
    tmpFlowH += pHbSqrtUp[0];
    // In the final formula, Ru is taken with a minus
    tmpFlowV -= gravity * (cur.H + H_) * (curHb - Hb_); 

    // Now load new values for dp
    H_  = inPtrDn.pH[0];
    u_  = inPtrDn.pU[0];
    v_  = inPtrDn.pV[0];
    Hb_ = pHbDn[0];
    speed_ = *pSpeedDn; 

    // The part of the flow from a negative index
    tmpFlowH += H_ * (v_ - speed_);
    tmpFlowU += H_ * (v_ - speed_) * u_;
    tmpFlowV += H_ * (v_ * v_ - 2.0f * v_ * speed_ + gravity * H_);
    tmpFlowH += pHbSqrtDn[0];
    // In the final formula, Ru is taken with a minus
    tmpFlowV -= gravity * (H_ + cur.H) * (Hb_ - curHb); 

    // Final step, write the results to the flow outputs
    flow.H += tmpFlowH * tau_div_deltah_quarter;
    flow.u += tmpFlowU * tau_div_deltah_quarter;
    flow.v += tmpFlowV * tau_div_deltah_quarter;
}
// End of File