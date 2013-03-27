/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFlux2DSSECalculator.cpp
 *
 * @brief Implements a Streaming SIMD Extension verion of the calculator
 */

#include <math.h>
#include "Flux/CFlux2DGrid.h"
#include "CFlux2DSSECalculator.h"

namespace
{
    /**
     * @fn CalculateSqrtZeroSafe()
     *
     * @brief Calculates the square root of a value
     *
     * @param vValue - The value for which to find the square root
     *
     * @return - The square root of the value
     */
    __forceinline __m128 CalculateSqrtZeroSafe(__m128 vValue)
    {
        return _mm_sqrt_ps(vValue);
    }

    /**
     * @fn LoadLeft()
     *
     * @brief This method loads the left neighbor of a home address
     *
     * @param pAddr - The home address
     *
     * vCurValue - The value of the home address
     *
     * @return - The left neighbor
     */
    __forceinline __m128 LoadLeft(float const *pAddr, __m128 vCurValue)
    {
        __m128 vTemp = vCurValue;
        vTemp = _mm_loadh_pi(vTemp, (__m64 const*)(pAddr - 2));
        vTemp = _mm_shuffle_ps(vTemp, vCurValue, 0x93); // 10-01-00-11
        return vTemp;
    }

    /**
     * @fn LoadRight()
     *
     * @brief This method loads the right neighbor of a home address
     *
     * @param pAddr - The home address
     *
     * vCurValue - The value of the home address
     *
     * @return - The right neighbor
     */
    __forceinline __m128 LoadRight(float const *pAddr, __m128 vCurValue)
    {
        __m128 vTemp = vCurValue;
        vTemp = _mm_loadl_pi(vTemp, (__m64 const*)(pAddr + 4));
        return _mm_shuffle_ps(vCurValue, vTemp, 0x39); // 00-11-10-01
    }
}

/**
 * @fn CheckNeighbourNodes()
 *
 * @brief Checks the neighborhood of a node for wet and dry nodes
 *
 * @param vHC - Height of the water
 *
 * @param - Write Mask
 *
 * @return FALSE if all neighbor nodes are dry, TRUE otherwise
 */
inline bool CFlux2DSSECalculator::CheckNeighbourNodes(__m128 vHC, __m128 vMask) const
{
    __m128 vZero = sse_zero;
    // Check the left neighbor
    __m128 vHL = LoadLeft(inPtr.pH, vHC);
    // Where the height is above zero, OR with write mask
    vMask = _mm_or_ps(vMask, _mm_cmpgt_ps(vHL, vZero));
    // Check the right neighbor
    __m128 vHR = LoadRight(inPtr.pH, vHC);
    // Where the height is above zero, OR with write mask
    vMask = _mm_or_ps(vMask, _mm_cmpgt_ps(vHR, vZero));
    // Check the lower neighbor
    __m128 vHU = _mm_load_ps(inPtrDn.pH);
    // Where the height is above zero, OR with write mask
    vMask = _mm_or_ps(vMask, _mm_cmpgt_ps(vHU, vZero));
    // Load the upper neighbor
    __m128 vHD = _mm_load_ps(inPtrUp.pH);
    // Where the height is above zero, OR with write mask
    vMask = _mm_or_ps(vMask, _mm_cmpgt_ps(vHD, vZero));
    // Convert to an integer
    int mask = _mm_movemask_ps(vMask);
    // If any bit in the mask is set, then somewhere, the the water was higher than the zero
    // Return FALSE if all nodes are dry 
    return mask != 0;
}

/**
 * @fn ProcessState()
 *
 * @brief Process the current node and neighbors
 *
 * @return TRUE if all the current vector has all elements greater than zero (all dry)
 *         FALSE if at least one element is wet
 */
inline bool CFlux2DSSECalculator::ProcessState()
{
    // Load height
    vCur.H = _mm_load_ps(inPtr.pH);
    // Check the height, setting bits in the mask where the element is greater than zero
    __m128 vMask = _mm_cmpgt_ps(vCur.H, sse_zero);
    // Convert the mask into an integer to use programatically
    int mask = _mm_movemask_ps(vMask);

    // If no element is greater than zero (all wet)
    if (mask == 0)
    {
        if (!CheckNeighbourNodes(vCur.H, vMask))
        {
            // If all neighbors are dry, use streaming SIMD extensions for calculation
            Process4NodesSSE_Inactive();
        }
        else // At least one neighbor is wet
        {
            // We cannot use SIMD, fallback to using the scalar version
            Process4NodesScalar();
        }
        return false;
    } 
    else if (mask != 0xF)
    {
        // If some neighbors are wet and some are dry, we cannot use SIMD 
        // extensions and so fallback to the scalar version
        Process4NodesScalar();
        return false;
    }
    return true;
}

/**
 * @fn BeginSSE()
 *
 * @brief Checks pointer for alignment, loads parameters and values, sets flows to zero
 */
inline void CFlux2DSSECalculator::BeginSSE()
{
    // Check pointers are aligned
    assert(IsPtrAligned16(inPtr.pH));
    assert(IsPtrAligned16(inPtr.pU));
    assert(IsPtrAligned16(inPtr.pV));
    assert(IsPtrAligned16(pHb));
    assert(IsPtrAligned16(pHbSqrt));
    // Load the rest (The height was already loaded in ProcessState())
    vCur.u = _mm_load_ps(inPtr.pU);
    vCur.v = _mm_load_ps(inPtr.pV);
    vCurHb = _mm_load_ps(pHb);
    vCurHbSqrt = _mm_load_ps(pHbSqrt);
    // Calculate the critical speed
    vCurSpeed = _mm_load_ps(pSpeed); 
    vMinusCurSpeed = _mm_sub_ps(sse_zero, vCurSpeed);
    // Set flows to zero
    vFlow.H = sse_zero;
    vFlow.u = sse_zero;
    vFlow.v = sse_zero;
}

/**
 * @fn BeginSSE_Prefetch()
 *
 * @brief Checks pointer for alignment, loads parameters and values, sets flows to zero
 *        pre-fetches nodes for further operations
 */
inline void CFlux2DSSECalculator::BeginSSE_Prefetch()
{
    // Check pointers are aligned
    assert(IsPtrAligned32(inPtr.pH));
    assert(IsPtrAligned32(inPtr.pU));
    assert(IsPtrAligned32(inPtr.pV));
    assert(IsPtrAligned32(pHb));
    assert(IsPtrAligned32(pHbSqrt));
    // Load the rest (the height was already loaded in ProcessState())
    int const offset = 8;
    vCur.u = _mm_load_ps(inPtr.pU);
    _mm_prefetch((char const *)(inPtrDn.pU + offset), _MM_HINT_T0);
    vCur.v = _mm_load_ps(inPtr.pV);
    _mm_prefetch((char const *)(inPtrDn.pV + offset), _MM_HINT_T0);
    vCurHb = _mm_load_ps(pHb);
    _mm_prefetch((char const *)(pHbDn + offset), _MM_HINT_T0);
    vCurHbSqrt = _mm_load_ps(pHbSqrt);
    _mm_prefetch((char const *)(pHbSqrtDn + offset), _MM_HINT_T0);
    // Calculate the critical speed
    vCurSpeed = _mm_load_ps(pSpeed); 
    vMinusCurSpeed = _mm_sub_ps(sse_zero, vCurSpeed);
    // Set flows to zero
    vFlow.H = sse_zero;
    vFlow.u = sse_zero;
    vFlow.v = sse_zero;
}

/**
 * @fn EndSSE()
 *
 * @brief Completes SSE calculations
 */
inline void CFlux2DSSECalculator::EndSSE()
{
    /* ********************************************************************************** *\
                  NOTE: Flows have already been multiplied by corresponding values
    \* ********************************************************************************** */

    // Calculate new H value
    __m128 vHnew = vCur.H - vFlow.H;

    // Define a mask to blend values in the future
    __m128 vMask = _mm_cmpgt_ps(vHnew, sse_zero); 
    int mask = _mm_movemask_ps(vMask);

    // If at least one element of the new H is greater than zero
    if (mask != 0)
    {
        // If all elements of the new H are greater than zero
        if (mask == 0xF)
        {
            // Store the new value
            _mm_store_ps(outPtr.pH, vHnew);

            // Calculate new (u,v) values
            __m128 vSq   = vTau_mul_Cf * CalculateSqrtZeroSafe(vCur.u * vCur.u + vCur.v * vCur.v);
            __m128 vH_sq = vCur.H - vSq;
            __m128 vRcpHnew = rcp(_mm_max_ps(vHnew, vHmin));

            // U speed
            __m128 vSpeed = vRcpHnew * (vH_sq * vCur.u - vFlow.u);
            CorrectSpeed(vSpeed);
            _mm_store_ps(outPtr.pU, vSpeed);

            // V speed
            vSpeed = vRcpHnew * (vH_sq * vCur.v - vFlow.v);
            CorrectSpeed(vSpeed);
            _mm_store_ps(outPtr.pV, vSpeed);
        }
        else // There is a mixture, some elements are less than zero, some are greater
        {
            // Calculate and store a new value
            __m128 vHnewCorr = _mm_blendv_ps(vHnew, sse_zero, vMask);
            _mm_store_ps(outPtr.pH, vHnewCorr);

            // Calculate new (u,v) values
            __m128 vSq   = vTau_mul_Cf * CalculateSqrtZeroSafe(vCur.u * vCur.u + vCur.v * vCur.v);
            __m128 vH_sq = vCur.H - vSq;
            __m128 vRcpHnew = rcp(_mm_max_ps(vHnew, vHmin));

            // U speed
            __m128 vSpeed = vRcpHnew * (vH_sq * vCur.u - vFlow.u);
            CorrectSpeed(vSpeed);
            vSpeed = _mm_blendv_ps(vSpeed, sse_zero, vMask);
            _mm_store_ps(outPtr.pU, vSpeed);

            // V speed
            vSpeed = vRcpHnew * (vH_sq * vCur.v - vFlow.v);
            CorrectSpeed(vSpeed);
            vSpeed = _mm_blendv_ps(vSpeed, sse_zero, vMask);
            _mm_store_ps(outPtr.pV, vSpeed);
        }
    }
    else // No element of the new H is greater than zero
    {
        // Just store zeros
        _mm_store_ps(outPtr.pH, sse_zero);
        _mm_store_ps(outPtr.pU, sse_zero);
        _mm_store_ps(outPtr.pV, sse_zero);
    }
}

/**
 * @fn ParseSpeedValue()
 *
 * @brief Ensures that the speed value is large enough to not cause division errors or overflow
 *
 * @param speed - The speed value to parse
 *
 * @return 
 */
inline int CFlux2DSSECalculator::ParseSpeedValue(__m128 speed)
{
    int mask;
    // Set bits in the mask where the element of speed is less than zero
    mask = _mm_movemask_ps(_mm_cmplt_ps(speed, sse_zero));

    // If all four components of the speed are greater than or equal to zero
    if (mask == 0x0) 
    { 
        // Compare with the current speed setting bits where the new speed is less than the current speed
        mask = _mm_movemask_ps(_mm_cmplt_ps(speed, vCurSpeed));
        //If all four components of the speed are greater than or equal to the current speed
        if (mask == 0x0)
        {
            return CFlux2DGrid::SPEED_POS_MORECRITICAL;
        } 
        // If all four components were less than the current speed
        else if (mask == 0xF)
        {
            return CFlux2DGrid::SPEED_POS_LESSCRITICAL;
        } 
        else 
        {
            // We have a mixture of faster and slower elements when compared to the current speed
            return -1;
        }
    }
    // If all four components of the speed are less than zero
    else if (mask == 0xF)
    { 
        // Compare with the inverse of the current speed setting bits in the mask when the speed
        // is less than the inverse of the current speed
        mask = _mm_movemask_ps(_mm_cmplt_ps(speed, vMinusCurSpeed));
        // If all four components are greater than or equal to zero
        if (mask == 0x0) 
        {
            return CFlux2DGrid::SPEED_NEG_LESSCRITICAL;
        } 
        // If no component was greater than or equal to zero
        else if (mask == 0xF)
        {
            return CFlux2DGrid::SPEED_NEG_MORECRITICAL;
        } 
        else
        {
            // We have a mixture, some components were faster, some slower
            return -1;
        }
    }
    else
    {
        // We have a mixture, some components were greater than or equal to zero, some were not.
        return -1;
    }
}

/**
 * @fn CalcU_Neg_MoreCritical_SSE()
 *
 * @brief This method calculates the flow component related with a NEGATIVE water bar speed
 *        U component that has an absolute value that is MORE than the wave speed in the node
 */
inline void CFlux2DSSECalculator::CalcU_Neg_MoreCritical_SSE()
{
    /* ************************************************************************************* *\
                    ATTENTION: tempFlow signs are inverted for optimization!
    /* ************************************************************************************* */

    // First do the calculations with the center (H, u,v) point in the grid
    __m128 vHu = vCur.H * vCur.u;
    __m128 vTmpFlowH = vHu;
    __m128 vTmpFlowU = vCur.H * (vHalfGravity * vCur.H + vCur.u * vCur.u);
    __m128 vTmpFlowV = vHu * vCur.v;

    // Now load new values for dp
    __m128 vH_  = LoadRight(inPtr.pH, vCur.H);
    __m128 vU_  = LoadRight(inPtr.pU, vCur.u);
    __m128 vV_  = LoadRight(inPtr.pV, vCur.v);
    __m128 vHb_ = LoadRight(pHb, vCurHb);

    // Calculate the positive portion of the water flow vector 
    vHu = vH_ * vU_;
    vTmpFlowH -= vHu;
    vTmpFlowU -= vH_ * (vHalfGravity * vH_ + vU_ * vU_);
    vTmpFlowV -= vHu * vV_;

    // In the final formula, Ru is taken with a minus
    vTmpFlowU += vHalfGravity * (vH_ + vCur.H) * (vHb_ - vCurHb);
    // For the final step, write the results to the flow outputs
    vFlow.H -= vTmpFlowH * vTau_div_deltaw;
    vFlow.u -= vTmpFlowU * vTau_div_deltaw;
    vFlow.v -= vTmpFlowV * vTau_div_deltaw;
}

/**
 * @fn CalcU_Pos_MoreCritical_SSE()
 *
 * @brief This method calculates the flow component related with a POSITIVE water bar speed U component 
 *        that has an absolute value MORE than the wave speed in the node
 */
inline void CFlux2DSSECalculator::CalcU_Pos_MoreCritical_SSE()
{
    // First do the calculations with the center (H, u,v) point in the grid
    __m128 vHu = vCur.H * vCur.u;
    __m128 vTmpFlowH = vHu;
    __m128 vTmpFlowU = vCur.H * (vHalfGravity * vCur.H + vCur.u * vCur.u);
    __m128 vTmpFlowV = vHu * vCur.v;

    // Now load new values for dp
    __m128 vH_  = LoadLeft(inPtr.pH, vCur.H);
    __m128 vU_  = LoadLeft(inPtr.pU, vCur.u);
    __m128 vV_  = LoadLeft(inPtr.pV, vCur.v);
    __m128 vHb_ = LoadLeft(pHb, vCurHb);

    // Calculate the positive portion of the water flow vector 
    vHu = vH_ * vU_;
    vTmpFlowH -= vHu;
    vTmpFlowU -= vH_ * (vHalfGravity * vH_ + vU_ * vU_);
    vTmpFlowV -= vHu * vV_;

    // In the final formula, Ru is taken with a minus
    vTmpFlowU -= vHalfGravity * (vCur.H + vH_) * (vCurHb - vHb_);
    
    // For the final step, write the results to the flow outputs
    vFlow.H += vTmpFlowH * vTau_div_deltaw;
    vFlow.u += vTmpFlowU * vTau_div_deltaw;
    vFlow.v += vTmpFlowV * vTau_div_deltaw;
}

inline void CFlux2DSSECalculator::CalcU_Neg_LessCritical_SSE()
{

    /* ***************************************** Mathematical Equivalent *************************************** *\
                                    tmpFlowH = 2.0f * cur.H * (curSpeed - cur.u);
                                    tmpFlowU = 2.0f * cur.H * (2.0f * curSpeed - cur.u) * cur.u;
                                    tmpFlowV = tmpFlowH * cur.v - 2.0f * curHbSqrt;
    /* ***************************************END Mathematical Equivalent ************************************** */

    // First do the calculations with the center (H, u,v) point in the grid
    __m128 v2CurH = vCur.H + vCur.H;
    __m128 v2CurH_Speed = v2CurH * vCurSpeed;
    __m128 v2CurH_CurU  = v2CurH * vCur.u;

    __m128 vTmpFlowH = v2CurH_Speed - v2CurH_CurU;
    __m128 vTmpFlowU = (vTmpFlowH + v2CurH_Speed) * vCur.u;
    __m128 vTmpFlowV = vTmpFlowH * vCur.v;
    vTmpFlowH -= vCurHbSqrt; 
    vTmpFlowH -= vCurHbSqrt;

    // Now load new values for dm
    __m128 vH_ = LoadLeft(inPtr.pH, vCur.H);
    __m128 vU_ = LoadLeft(inPtr.pU, vCur.u);
    __m128 vV_ = LoadLeft(inPtr.pV, vCur.v);
    __m128 vHb_ = LoadLeft(pHb, vCurHb);
    __m128 vSpeed_ = LoadLeft(pSpeed, vCurSpeed); 
    
    /* ***************************************** Mathematical Equivalent *************************************** *\
            tmpFlowH += - H_ * (u_ + speed_) + pHbSqrt[-1];
            tmpFlowU += - H_ * (u_ * u_ + 2.0f * u_ * speed_ + gravity * H_) - gravity * (cur.H + H_) * (curHb - Hb_);
            tmpFlowV += - H_ * (u_ + speed_) * v_;
    /* ***************************************END Mathematical Equivalent ************************************** */

    // The part of the flow from a negative index
    __m128 vUpS   = vU_ + vSpeed_;
    __m128 vH_UpS = vH_ * vUpS;
    vTmpFlowH -= vH_UpS;
    vTmpFlowH += LoadLeft(pHbSqrt, vCurHbSqrt);
    vTmpFlowV -= vH_UpS * vV_;
    vTmpFlowU -= ((vUpS + vSpeed_) * vU_ + (vGravity) * vH_) * vH_;
    vTmpFlowU -= (vGravity) * (vCur.H + vH_) * (vCurHb - vHb_);

    // Now load new values for dp
    vH_ = LoadRight(inPtr.pH, vCur.H);
    vU_ = LoadRight(inPtr.pU, vCur.u);
    vV_ = LoadRight(inPtr.pV, vCur.v);
    vHb_ = LoadRight(pHb, vCurHb);
    vSpeed_ = LoadRight(pSpeed, vCurSpeed); 

    /* ***************************************** Mathematical Equivalent *************************************** *\
        tmpFlowH += H_ * (3.0f * u_ - speed_) + pHbSqrt[1];
        tmpFlowU += H_ * (3.0f * u_ * u_ - 2.0f * u_ * speed_ + gravity * H_) - gravity * (H_ + cur.H) * (Hb_ - curHb);
        tmpFlowV += H_ * (3.0f * u_ - speed_) * v_;
    /* ***************************************END Mathematical Equivalent ************************************** */

    // The part of the flow from a positive index
    __m128 v3UmS   = sse_three * vU_ - vSpeed_;
    __m128 vH_3UmS = vH_ * v3UmS;
    vTmpFlowH += vH_3UmS;
    vTmpFlowH += LoadRight(pHbSqrt, vCurHbSqrt);
    vTmpFlowV += vH_3UmS * vV_;
    vTmpFlowU += ((v3UmS - vSpeed_) * vU_ + (vGravity) * vH_) * vH_;
    vTmpFlowU -= (vGravity) * (vH_ + vCur.H) * (vHb_ - vCurHb);
    
    // For the final step, write the results to the flow outputs
    vFlow.H += vTmpFlowH * vTau_div_deltaw_quarter;
    vFlow.u += vTmpFlowU * vTau_div_deltaw_quarter;
    vFlow.v += vTmpFlowV * vTau_div_deltaw_quarter;
}

/**
 * @fn CalcU_Pos_LessCritical_SSE()
 *
 * @brief This method calculates the flow component related with a POSITIVE water bar speed U component 
 *        that has absolute value LESS than the wave speed in the node
 */
inline void CFlux2DSSECalculator::CalcU_Pos_LessCritical_SSE()
{

    /* ***************************************** Mathematical Equivalent *************************************** *\
                            tmpFlowH = 2.0f * cur.H * (curSpeed + cur.u) - 2.0f * curHbSqrt;
                            tmpFlowU = 2.0f * cur.H * (2.0f * curSpeed + cur.u) * cur.u;
                            tmpFlowV = tmpFlowH * cur.v;
    /* ***************************************END Mathematical Equivalent ************************************** */

    // First do the calculations with the center (H, u,v) point in the grid
    __m128 v2CurH = vCur.H + vCur.H;
    __m128 v2CurH_Speed = v2CurH * vCurSpeed;
    __m128 v2CurH_CurU  = v2CurH * vCur.u;

    __m128 vTmpFlowH = v2CurH_Speed + v2CurH_CurU;
    __m128 vTmpFlowU = (vTmpFlowH + v2CurH_Speed) * vCur.u;
    __m128 vTmpFlowV = vTmpFlowH * vCur.v;
    vTmpFlowH -= vCurHbSqrt; 
    vTmpFlowH -= vCurHbSqrt;

    // Now load new values for dm
    __m128 vH_ = LoadLeft(inPtr.pH, vCur.H);
    __m128 vU_ = LoadLeft(inPtr.pU, vCur.u);
    __m128 vV_ = LoadLeft(inPtr.pV, vCur.v);
    __m128 vHb_ = LoadLeft(pHb, vCurHb);
    __m128 vSpeed_ = LoadLeft(pSpeed, vCurSpeed); 

    /* ***************************************** Mathematical Equivalent *************************************** *\
                tmpFlowH += - H_ * (3.0f * u_ + speed_) + pHbSqrt[-1];
                tmpFlowU += - H_ * (3.0f * u_ * u_ + 2.0f * u_ * speed_ + gravity * H_);
                tmpFlowV += - H_ * (3.0f * u_ + speed_) * v_ - gravity * (cur.H + H_) * (curHb - Hb_);
    /* ***************************************END Mathematical Equivalent ************************************** */

    // The part of the flow from a negative index
    __m128 v3UpS   = sse_three * vU_ + vSpeed_;
    __m128 vH_3UpS = vH_ * v3UpS;
    vTmpFlowH -= vH_3UpS;
    vTmpFlowH += LoadLeft(pHbSqrt, vCurHbSqrt);
    vTmpFlowV -= vH_3UpS * vV_;
    vTmpFlowU -= ((v3UpS + vSpeed_) * vU_ + (vGravity) * vH_) * vH_;
    vTmpFlowU -= (vGravity) * (vCur.H + vH_) * (vCurHb - vHb_);

    // Now load new values for dp
    vH_ = LoadRight(inPtr.pH, vCur.H);
    vU_ = LoadRight(inPtr.pU, vCur.u);
    vV_ = LoadRight(inPtr.pV, vCur.v);
    vHb_ = LoadRight(pHb, vCurHb);
    vSpeed_ = LoadRight(pSpeed, vCurSpeed);

    /* ***************************************** Mathematical Equivalent *************************************** *\
        tmpFlowH += H_ * (u_ - speed_) + pHbSqrt[1];
        tmpFlowU += H_ * (u_ * u_ - 2.0f * u_ * speed_ + gravity * H_) - gravity * (H_ + cur.H) * (Hb_ - curHb);
        tmpFlowV += H_ * (u_ - speed_) * v_;
    /* ***************************************END Mathematical Equivalent ************************************** */

    // The part of the flow from a positive index
    __m128 vUmS   = vU_ - vSpeed_;
    __m128 vH_UmS = vH_ * vUmS;
    vTmpFlowH += vH_UmS;
    vTmpFlowH += LoadRight(pHbSqrt, vCurHbSqrt);
    vTmpFlowV += vH_UmS * vV_;
    vTmpFlowU += ((vUmS - vSpeed_) * vU_ + (vGravity) * vH_) * vH_;
    vTmpFlowU -= (vGravity) * (vH_ + vCur.H) * (vHb_ - vCurHb);
    
    // For the final step, write the results to the flow outputs
    vFlow.H += vTmpFlowH * vTau_div_deltaw_quarter;
    vFlow.u += vTmpFlowU * vTau_div_deltaw_quarter;
    vFlow.v += vTmpFlowV * vTau_div_deltaw_quarter;
}

/**
 * @fn CalcV_Neg_MoreCritical_SSE()
 *
 * @brief This method calculates the flow component related with a NEGATIVE water bar speed V component 
 *        that has an absolute value MORE than the wave speed in the node
 */
inline void CFlux2DSSECalculator::CalcV_Neg_MoreCritical_SSE()
{
    /* ************************************************************************************* *\
                    ATTENTION: tempFlow signs are inverted for optimization!
    /* ************************************************************************************* */

    // First do the calculations with the center (H, u,v) point in the grid
    __m128 vHv = vCur.H * vCur.v;
    __m128 vTmpFlowH = vHv;
    __m128 vTmpFlowU = vHv * vCur.u;
    __m128 vTmpFlowV = vCur.H * (vHalfGravity * vCur.H + vCur.v * vCur.v);

    // Now load new values for dp
    __m128 vH_  = _mm_load_ps(inPtrDn.pH);
    __m128 vU_  = _mm_load_ps(inPtrDn.pU);
    __m128 vV_  = _mm_load_ps(inPtrDn.pV);
    __m128 vHb_ = _mm_load_ps(pHbDn);
    
    // Calculate the positive portion of the water flow vector 
    vHv = vH_ * vV_;
    vTmpFlowH -= vHv;
    vTmpFlowU -= vHv * vU_;
    vTmpFlowV -= vH_ * (vHalfGravity * vH_ + vV_ * vV_);

    // In the final formula, Ru is taken with a minus
    vTmpFlowV += vHalfGravity * (vH_ + vCur.H) * (vHb_ - vCurHb);
    
    // For the final step, write the results to the flow outputs
    vFlow.H -= vTmpFlowH * vTau_div_deltah;
    vFlow.u -= vTmpFlowU * vTau_div_deltah;
    vFlow.v -= vTmpFlowV * vTau_div_deltah;
}

/**
 * @fn CalcV_Pos_MoreCritical_SSE()
 *
 * @brief  This method calculates the flow component related with a POSITIVE water bar speed V component 
 *         that has an absolute value MORE than the wave speed in the node
 */
inline void CFlux2DSSECalculator::CalcV_Pos_MoreCritical_SSE()
{
    // First do the calculations with the center (H, u,v) point in the grid
    __m128 vHv = vCur.H * vCur.v;
    __m128 vTmpFlowH = vHv;
    __m128 vTmpFlowU = vHv * vCur.u;
    __m128 vTmpFlowV = vCur.H * (vHalfGravity * vCur.H + vCur.v * vCur.v);

    // Now load new values for dp
    __m128 vH_  = _mm_loadu_ps(inPtr.pH - 1);
    __m128 vU_  = _mm_loadu_ps(inPtr.pU - 1);
    __m128 vV_  = _mm_loadu_ps(inPtr.pV - 1);
    __m128 vHb_ = _mm_loadu_ps(pHb - 1);
    
    // Calculate the positive portion of the water flow vector 
    vHv = vH_ * vV_;
    vTmpFlowH -= vHv;
    vTmpFlowU -= vHv * vU_;
    vTmpFlowV -= vH_ * (vHalfGravity * vH_ + vV_ * vV_);

    // In the final formula, Ru is taken with a minus
    vTmpFlowV -= vHalfGravity * (vCur.H + vH_) * (vCurHb - vHb_);
    
    // For the final step, write the results to the flow outputs
    vFlow.H += vTmpFlowH * vTau_div_deltah;
    vFlow.u += vTmpFlowU * vTau_div_deltah;
    vFlow.v += vTmpFlowV * vTau_div_deltah;
}

/**
 * @fn CalcV_Neg_LessCritical_SSE()
 *
 * @brief  This method calculates the flow component related with a NEGATIVE water bar speed V component 
 *         that has an absolute value LESS than the wave speed in the node
 */
inline void CFlux2DSSECalculator::CalcV_Neg_LessCritical_SSE()
{

    /* ***************************************** Mathematical Equivalent *************************************** *\
                        tmpFlowH = 2.0f * cur.H * (curSpeed - cur.v) - 2.0f * curHbSqrt;
                        tmpFlowU = tmpFlowH * cur.u;
                        tmpFlowV = 2.0f * cur.H * (2.0f * curSpeed - cur.v) * cur.v;
    /* ***************************************END Mathematical Equivalent ************************************** */

    // First do the calculations with the center (H, u,v) point in the grid
    __m128 v2CurH = vCur.H + vCur.H;
    __m128 v2CurH_Speed = v2CurH * vCurSpeed;
    __m128 v2CurH_CurV  = v2CurH * vCur.v;
    __m128 vTmpFlowH = v2CurH_Speed - v2CurH_CurV;
    __m128 vTmpFlowU = vTmpFlowH * vCur.u;
    __m128 vTmpFlowV = (vTmpFlowH + v2CurH_Speed) * vCur.v;
    vTmpFlowH -= vCurHbSqrt; 
    vTmpFlowH -= vCurHbSqrt;

    // Now load new values for dm
    __m128 vH_ = _mm_load_ps(inPtrUp.pH);
    __m128 vU_ = _mm_load_ps(inPtrUp.pU);
    __m128 vV_ = _mm_load_ps(inPtrUp.pV);
    __m128 vHb_ = _mm_load_ps(pHbUp);
    __m128 vSpeed_ = _mm_load_ps(pSpeedUp); 

    /* ***************************************** Mathematical Equivalent *************************************** *\
    // tmpFlowH += - H_ * (v_ + speed_) + pHbSqrt[-stride] + pHbSqrt[-stride];
    // tmpFlowU += - H_ * (v_ + speed_) * u_;
    // tmpFlowV += - H_ * (v_ * v_ + 2.0f * v_ * speed_ + gravity * H_) - gravity * (cur.H + H_) * (curHb - Hb_);
    /* ***************************************END Mathematical Equivalent ************************************** */

    // The part of the flow from a negative index
    __m128 vVpS   = vV_ + vSpeed_;
    __m128 vH_VpS = vH_ * vVpS;
    vTmpFlowH -= vH_VpS;
    vTmpFlowH += _mm_load_ps(pHbSqrtUp);
    vTmpFlowU -= vH_VpS * vU_;
    vTmpFlowV -= ((vVpS + vSpeed_) * vV_ + (vGravity) * vH_) * vH_;
    vTmpFlowV -= (vGravity) * (vCur.H + vH_) * (vCurHb - vHb_);

    // Now load new values for dp
    vH_ = _mm_load_ps(inPtrDn.pH);
    vU_ = _mm_load_ps(inPtrDn.pU);
    vV_ = _mm_load_ps(inPtrDn.pV);
    vHb_ = _mm_load_ps(pHbDn);
    vSpeed_ = _mm_load_ps(pSpeedDn); 

    /* ***************************************** Mathematical Equivalent *************************************** *\
    tmpFlowH += H_ * (3.0f * v_ - speed_) + pHbSqrt[stride];
    tmpFlowU += H_ * (3.0f * v_ - speed_) * u_;
    tmpFlowV += H_ * (3.0f * v_ * v_ - 2.0f * v_ * speed_ + gravity * H_) - gravity * (H_ + cur.H) * (Hb_ - curHb);
    /* ***************************************END Mathematical Equivalent ************************************** */

    // The part of the flow from a positive index
    __m128 v3VmS   = sse_three * vV_ - vSpeed_;
    __m128 vH_3VmS = vH_ * v3VmS;
    vTmpFlowH += vH_3VmS;
    vTmpFlowH += _mm_load_ps(pHbSqrtDn);
    vTmpFlowU += vH_3VmS * vU_;
    vTmpFlowV += ((v3VmS - vSpeed_) * vV_ + (vGravity) * vH_) * vH_;
    vTmpFlowV -= (vGravity) * (vH_ + vCur.H) * (vHb_ - vCurHb);

    // For the final step, write the results to the flow outputs
    vFlow.H += vTmpFlowH * vTau_div_deltah_quarter;
    vFlow.u += vTmpFlowU * vTau_div_deltah_quarter;
    vFlow.v += vTmpFlowV * vTau_div_deltah_quarter;
}

/**
 * @fn CalcV_Pos_LessCritical_SSE()
 *
 * @brief  This method calculates the flow component related with a POSITIVE water bar speed V component 
 * that has an absolute value LESS than the wave speed in the node
 */
inline void CFlux2DSSECalculator::CalcV_Pos_LessCritical_SSE()
{

    /* ***************************************** Mathematical Equivalent *************************************** *\
                            tmpFlowH = 2.0f * cur.H * (curSpeed + cur.v) - 2.0f * curHbSqrt;
                            tmpFlowU = tmpFlowH * cur.u;
                            tmpFlowV = 2.0f * cur.H * (2.0f * curSpeed + cur.v) * cur.v;
    /* ***************************************END Mathematical Equivalent ************************************** */

    // First do the calculations with the center (H, u,v) point in the grid
    __m128 v2CurH = vCur.H + vCur.H;
    __m128 v2CurH_Speed = v2CurH * vCurSpeed;
    __m128 v2CurH_CurV  = v2CurH * vCur.v;
    __m128 vTmpFlowH = v2CurH_Speed + v2CurH_CurV;
    __m128 vTmpFlowU = vTmpFlowH * vCur.u;
    __m128 vTmpFlowV = (vTmpFlowH + v2CurH_Speed) * vCur.v;
    vTmpFlowH -= vCurHbSqrt; 
    vTmpFlowH -= vCurHbSqrt;

    // Now load new values for dm
    __m128 vH_ = _mm_load_ps(inPtrUp.pH);
    __m128 vU_ = _mm_load_ps(inPtrUp.pU);
    __m128 vV_ = _mm_load_ps(inPtrUp.pV);
    __m128 vHb_ = _mm_load_ps(pHbUp);
    __m128 vSpeed_ = _mm_load_ps(pSpeedUp); 

    /* ***************************************** Mathematical Equivalent *************************************** *\
    tmpFlowH += - H_ * (3.0f * v_ + speed_) + pHbSqrt[-stride];
    tmpFlowU += - H_ * (3.0f * v_ + speed_) * u_;
    tmpFlowV += - H_ * (3.0f * v_ * v_ + 2.0f * v_ * speed_ + gravity * H_) - gravity * (cur.H + H_) * (curHb - Hb_);
    /* ***************************************END Mathematical Equivalent ************************************** */

    // The part of the flow from a negative index
    __m128 v3VpS   = sse_three * vV_ + vSpeed_;
    __m128 vH_3VpS = vH_ * v3VpS;
    vTmpFlowH -= vH_3VpS;
    vTmpFlowH += _mm_load_ps(pHbSqrtUp);
    vTmpFlowU -= vH_3VpS * vU_;
    vTmpFlowV -= ((v3VpS + vSpeed_) * vV_ + (vGravity) * vH_) * vH_;
    vTmpFlowV -= (vGravity) * (vCur.H + vH_) * (vCurHb - vHb_);

    // Now load new values for dp
    vH_ = _mm_load_ps(inPtrDn.pH);
    vU_ = _mm_load_ps(inPtrDn.pU);
    vV_ = _mm_load_ps(inPtrDn.pV);
    vHb_ = _mm_load_ps(pHbDn);
    vSpeed_ = _mm_load_ps(pSpeedDn); 

    /* ***************************************** Mathematical Equivalent *************************************** *\
        tmpFlowH += H_ * (v_ - speed_) + pHbSqrt[stride];
        tmpFlowU += H_ * (v_ - speed_) * u_;
        tmpFlowV += H_ * (v_ * v_ - 2.0f * v_ * speed_ + gravity * H_) - gravity * (H_ + cur.H) * (Hb_ - curHb);
    /* ***************************************END Mathematical Equivalent ************************************** */

    // The part of the flow from a positive index
    __m128 vVmS   = vV_ - vSpeed_;
    __m128 vH_VmS = vH_ * vVmS;
    vTmpFlowH += vH_VmS;
    vTmpFlowH += _mm_load_ps(pHbSqrtDn);
    vTmpFlowU += vH_VmS * vU_;
    vTmpFlowV += ((vVmS - vSpeed_) * vV_ + vGravity * vH_) * vH_;
    vTmpFlowV -= vGravity * (vH_ + vCur.H) * (vHb_ - vCurHb);

    // For the final step, write the results to the flow outputs
    vFlow.H += vTmpFlowH * vTau_div_deltah_quarter;
    vFlow.u += vTmpFlowU * vTau_div_deltah_quarter;
    vFlow.v += vTmpFlowV * vTau_div_deltah_quarter;
}

/**
 * @fn Process4NodesSSE_WOBegin()
 *
 * @brief Processes four nodes using SSE if possible. If neighboring nodes
 *        are multi-directional, then we fall back to using the scalar method
 */
void CFlux2DSSECalculator::Process4NodesSSE_WOBegin()
{
    // Analize speed U, a flag that indicates direction.
    int speedDirectionU = ParseSpeedValue(vCur.u);
    // If the speed is less than zero, we must fall back to the scalar method
    if (speedDirectionU < 0)
    {
        Process4NodesScalar();
        return;
    }

    // Analize speed V
    int speedDirectionV = ParseSpeedValue(vCur.v);
    // If the speed is less than zero, we must fall back to the scalar method
    if (speedDirectionV < 0)
    {
        Process4NodesScalar();
        return;
    }

    // speedDirectionU is greater than or equal to zero
    switch (speedDirectionU)
    {
        case CFlux2DGrid::SPEED_NEG_MORECRITICAL: 
        {
            CalcU_Neg_MoreCritical_SSE(); 
            break;
        }
        case CFlux2DGrid::SPEED_NEG_LESSCRITICAL: 
        {
            CalcU_Neg_LessCritical_SSE(); 
            break;
        }
        case CFlux2DGrid::SPEED_POS_LESSCRITICAL: 
        {    
            CalcU_Pos_LessCritical_SSE(); 
            break;
        }
        case CFlux2DGrid::SPEED_POS_MORECRITICAL: 
        {
            CalcU_Pos_MoreCritical_SSE(); 
            break;
        }
    }

    // speedDirectionV is greater than or equal to zero
    switch (speedDirectionV)
    {
        case CFlux2DGrid::SPEED_NEG_MORECRITICAL:
        {  
            CalcV_Neg_MoreCritical_SSE(); 
            break;
        }
        case CFlux2DGrid::SPEED_NEG_LESSCRITICAL: 
        {
            CalcV_Neg_LessCritical_SSE(); 
            break;
        }
        case CFlux2DGrid::SPEED_POS_LESSCRITICAL: 
        {
            CalcV_Pos_LessCritical_SSE(); 
            break;
        }
        case CFlux2DGrid::SPEED_POS_MORECRITICAL: 
        {
            CalcV_Pos_MoreCritical_SSE(); 
            break;
        }
    }

    EndSSE();
    Advance(4);
}

/**
 * @fn Process4NodesSSE_Inactive()
 *
 * @brief Processes an inactive node, writes zero vectors to the output
 */
void CFlux2DSSECalculator::Process4NodesSSE_Inactive()
{
    _mm_store_ps(outPtr.pH, sse_zero);
    _mm_store_ps(outPtr.pU, sse_zero);
    _mm_store_ps(outPtr.pV, sse_zero);
    Advance(4);
}

/**
 * @fn ProcessTile()
 *
 * @briefd Processes a Tile of a grid
 *
 * @param pG - grid tile that will be processed
 *
 * @param x0 - The x coordinate of the left-top corner of the tile that will be processed
 *
 * @param y0 - The y coordinate of the left-top corner of the tile that will be processed
 *
 * @param dimW - The x size of the tile that will be processed
 *
 * @param dimH - The y size of the tile that will be processed
 */
void CFlux2DSSECalculator::ProcessTile(CFlux2DGrid* pG, int x0, int y0, int dimW, int dimH)
{
    float*  pH = NULL;
    int     i;

    // Initialize constants
    tau_div_deltaw  = m_CalcParams.tau * pG->GetRcpGridStepW();
    tau_div_deltah  = m_CalcParams.tau * pG->GetRcpGridStepH();
    tau_div_deltaw_quarter  = tau_div_deltaw * 0.25f;
    tau_div_deltah_quarter  = tau_div_deltah * 0.25f;
    // Vectorize the contsants
    vTau_div_deltaw  = _mm_set_ps1(tau_div_deltaw);
    vTau_div_deltah  = _mm_set_ps1(tau_div_deltah);
    vTau_div_deltaw_quarter = vTau_div_deltaw * sse_quarter;
    vTau_div_deltah_quarter = vTau_div_deltah * sse_quarter;

    // Get line pointers for the for the first three lines
    pSpeedUp = m_SpeedCacheMatrix.GetLinePtr(0);
    pSpeed = m_SpeedCacheMatrix.GetLinePtr(1);
    pSpeedDn = m_SpeedCacheMatrix.GetLinePtr(2);

    // Calculate the -1 line of speed to be used in the future
    pH = pG->GetCurrentSurface().m_heightMap.GetLinePtr(y0-1)+x0;
    for(i=-1;i<dimW+1;++i)
    {
        pSpeedUp[i] = sqrtf(gravity*pH[i]);
    }

    // Calculate the 0 line of speed to be used in the future
    pH = pG->GetCurrentSurface().m_heightMap.GetLinePtr(y0)+x0;
    for(i=-1;i<dimW+1;++i)
    {
        pSpeed[i] = sqrtf(gravity*pH[i]);
    }

    // Start loop, for every line
    for (int y = 0; y < dimH; y++)
    {

        // Initialize pointers to the first elements in the line
        InitPointersForLine(pG,x0,y0,y);

        // Calculate the +1 line of speed to be used in future
        for(i=-1;i<dimW+1;++i)
        {
            pSpeedDn[i] = sqrtf(gravity*inPtrDn.pH[i]);
        }

        // For every node in the line
        for (int x = 0; x < dimW / 8; x++)
        {
            // If all the nodes are dry
            if (ProcessState())
            {
                // Pre-fetch four nodes
                BeginSSE_Prefetch();
                Process4NodesSSE_WOBegin();
            }

            if (ProcessState())
            {
                // Process four nodes
                BeginSSE();
                Process4NodesSSE_WOBegin();
            }
        }
    }// The next line
}// End of ProcessTile()
