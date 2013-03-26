/**********************************************************************
Copyright ©2012 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

/**
*******************************************************************************
* @file <AmpEngine.cpp>
* 
* @brief demonstrates the usage of the the DX11 - AMP interop 
******************************************************************************/

#include "AmpEngine.hpp"


/**************************************************************************
* Animate controls parameter
**************************************************************************/
#define ANGLE     (3.1416f/2000)
#define FLOAT_MIN (0.00001f)

/**************************************************************************
* Vertices data
**************************************************************************/
XMFLOAT2 vertices[nVerticesNum] =
{
    XMFLOAT2(-0.5f,  -0.5f),
    XMFLOAT2( 0.5f,   0.5f),
    XMFLOAT2(-0.5f,   0.5f),
    XMFLOAT2( 0.5f,  -0.5f),
};

AMPEngine *AMPEngine::pAMPEngine = NULL;

/**
*******************************************************************************
* @fn getInstance
* @brief Get instance of AMP Engine
*
* @return Pointer to AMPEngine obj
******************************************************************************/
AMPEngine* AMPEngine::getInstance()
{
    return pAMPEngine;
}

/**
*******************************************************************************
* @fn ampEngineFactory
* @brief Creation factory of AMP Engine
*
* @param  Pointer of D3D11 Device
*
* @return NOERROR on success Positive if expected and E_FAIL on failure
******************************************************************************/
int AMPEngine::ampEngineFactory(ID3D11Device** pD3D11Device) 
{    
    if(NULL == pAMPEngine)
    {
        pAMPEngine = new AMPEngine(*pD3D11Device);
        if(NULL == pAMPEngine)
        {
            return E_FAIL;
        }
    }

    return NOERROR;
}

/**
*******************************************************************************
* @fn setup
* @brief Set accelerator and create array buffer,
*        also get D3D11 buffer pointer from array buffer
*
@ param  address of D3D11 buffer pointer
*
* @return NOERROR on success Positive if expected and E_FAIL on failure
******************************************************************************/
int AMPEngine::setup(ID3D11Buffer **ppVertexBuffer)
{
    if(nDataNum > 0)
    {
        RETURN_IF_FAIL(setDefaultAccelarator());
        RETURN_IF_FAIL(createArrayBuffer());
    }
    else
    {
        return E_FAIL;
    }

    RETURN_IF_FAIL(getDxBuffer(reinterpret_cast<void**>(ppVertexBuffer)));
        
    return S_OK;
}

/**
*******************************************************************************
* @fn setDefaultAccelarator
* @brief set default accelarator
*
* @return NOERROR on success Positive if expected and E_FAIL on failure
******************************************************************************/
int AMPEngine::setDefaultAccelarator(void)
{
    std::vector<accelerator> allAccl = accelerator::get_all(); 

    deviceAccl = allAccl[deviceId];

    accelerator::set_default(deviceAccl.device_path);
    std::wcout << L"Accelerator used : " << deviceAccl.get_description() << std::endl;    

    if (deviceAccl == accelerator(accelerator::direct3d_ref))
    {
        std::cout << "WARNING!! Running on very slow emulator!" << std::endl;
    }

    return NOERROR;
}

/**
*******************************************************************************
* @fn createArrayBuffer
* @brief Create array buffer
*
* @return NOERROR on success Positive if expected and E_FAIL on failure
******************************************************************************/
int AMPEngine::createArrayBuffer(void) 
{
    if(NULL == pData)
    {
        pData = new array<XMFLOAT2, 1>(nDataNum, vertices, acclView);
        if(NULL == pData)
        {
            return E_FAIL;
        }
    }
    return NOERROR;
}

/**
*******************************************************************************
* @fn getDxBuffer
* @brief Get D3D buffer from D3D accelerator_view interface of array data.
*
* @param address of a pointer,it's for output
*
* @return NOERROR on success Positive if expected and E_FAIL on failure
******************************************************************************/
int AMPEngine::getDxBuffer( void** d3dbuffer) const
{
    return concurrency::direct3d::get_buffer(*pData)->QueryInterface(__uuidof(ID3D11Buffer), (LPVOID*)d3dbuffer);
}

/**
*******************************************************************************
* @fn calculateDataByGPU
* @brief Run compute shader to acquire data
*
* @return void
******************************************************************************/
void AMPEngine::calculateDataByGPU(void)
{
    array<XMFLOAT2, 1>& arrayData = *pData;

    // Transform the vertex data on the accelerator which is associated with the array data.
    parallel_for_each(pData->extent, [=, &arrayData] (index<1> idx) restrict(amp)
    {
        // Compute new pos data
        XMFLOAT2 pos     = arrayData[idx];
        arrayData[idx].x = pos.x * cos(ANGLE) - pos.y * sin(ANGLE);
        arrayData[idx].y = pos.x * sin(ANGLE) + pos.y * cos(ANGLE);
    });
}

/**
*******************************************************************************
* @fn verifyResults
* @brief Verifies the results obtained with reference implementation
*
* @return NOERROR on success Positive if expected and E_FAIL on failure
******************************************************************************/
int AMPEngine::verifyResults(void)
{
    std::vector<XMFLOAT2> vSrc(nDataNum);
    std::vector<XMFLOAT2> vArrayTest(nDataNum);
    std::vector<XMFLOAT2> vCpuRef(nDataNum);

    // Save source data for GPU and CPU loop calculation
    copy(*pData, vSrc.begin());

    calculateDataByGPU();

    // Save GPU test result
    copy(*pData, vArrayTest.begin());

    // Do CPU loop calculation
    for (int i = 0; i < nDataNum; ++i)
    {
        vCpuRef[i].x = vSrc[i].x * cos(ANGLE) - vSrc[i].y * sin(ANGLE);
        vCpuRef[i].y = vSrc[i].x * sin(ANGLE) + vSrc[i].y * cos(ANGLE);
    }

    // Verification
    int passed = NOERROR;
    unsigned totalMismatches = 0;

    for (int i = 0; i < nDataNum; ++i)
    {
        if ((abs(vArrayTest[i].x - vCpuRef[i].x) > FLOAT_MIN)
            || (abs(vArrayTest[i].y - vCpuRef[i].y) > FLOAT_MIN)
            )
        {
            if (!totalMismatches)
            {
                printf("First mismatch found vTest[%d] = %d, vRef[%d] = %d\n",
                        i, vArrayTest[i], i, vCpuRef[i]);
                passed = E_FAIL;
            }
            totalMismatches++;
        }
    }
    
    if (totalMismatches)
    {
        cout << "Total number of mismatches found = " << totalMismatches << std::endl;
    }
    else
    {
        cout << "Verification Passed" << std::endl;
    }
    
    return passed;
}

