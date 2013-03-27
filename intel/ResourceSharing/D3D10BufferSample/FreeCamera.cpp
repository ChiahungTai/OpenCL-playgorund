// Copyright (c) 2009-2011 Intel Corporation
// All rights reserved.
// 
// WARRANTY DISCLAIMER
// 
// THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
// MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Intel Corporation is the author of the Materials, and requests that all
// problem reports or change requests be submitted to it directly


#include "FreeCamera.h"


CFreeCamera::CFreeCamera(void)
{
	fAspectRatio = 1;
	vEye = D3DXVECTOR3(0.0f,0.0f,-1000.0f);
	vAt = D3DXVECTOR3(0,0,0);
	vUp = D3DXVECTOR3(0,1,0);
	vRot = D3DXVECTOR2(0,0);
	D3DXMatrixLookAtLH(&mView, &vEye, &vAt, &vUp);
	D3DXMatrixPerspectiveFovLH(&mProj,FOV,fAspectRatio, F_NEAR, F_FAR);
}

CFreeCamera::CFreeCamera(D3DXVECTOR3 i_vEye)
{
	fAspectRatio = 1;
	vEye = i_vEye;
	vAt = D3DXVECTOR3(0,0,0);
	vUp = D3DXVECTOR3(0,1,0);
	vRot = D3DXVECTOR2(0,0);
	D3DXMatrixLookAtLH(&mView, &vEye, &vAt, &vUp);
	D3DXMatrixPerspectiveFovLH(&mProj,FOV,fAspectRatio, F_NEAR, F_FAR);
}

D3DXMATRIX CFreeCamera::getViewMatrix(void)
{
	return mView;
}

D3DXMATRIX CFreeCamera::getProjMatrix(void)
{
	return mProj;
}

D3DXVECTOR3 CFreeCamera::getEye(void)
{
	return vEye;
}

D3DXVECTOR3 CFreeCamera::getLocalAt(void)
{
	return vAt - vEye;
}

D3DXVECTOR2 CFreeCamera::getRot(void)
{
	return vRot;
}

void CFreeCamera::translateGlobal(D3DXVECTOR3 vTrans)
{
	vEye = vEye + vTrans;
	vAt = vAt + vTrans;
	D3DXMatrixLookAtLH(&mView, &vEye, &vAt, &vUp);
}

void CFreeCamera::translateLocal(D3DXVECTOR3 vTrans)
{
	D3DXVECTOR3 vRightTrans,vUpTrans,vAtTrans;
	D3DXVECTOR3 v3VecAt = vAt - vEye;
	D3DXVec3Cross( &vRightTrans, &v3VecAt, &vUp );

	D3DXVec3Normalize( &vRightTrans, &vRightTrans );
	D3DXVec3Normalize( &vUpTrans, &vUp );
	D3DXVec3Normalize( &vAtTrans, &v3VecAt );

	D3DXVec3Scale( &vRightTrans, &vRightTrans, vTrans.x );
	D3DXVec3Scale( &vUpTrans, &vUpTrans, vTrans.y );
	D3DXVec3Scale( &vAtTrans, &vAtTrans, vTrans.z );

	D3DXVECTOR3 vTotalTrans = vRightTrans + vUpTrans + vAtTrans;
	vEye = vEye + vTotalTrans;
	vAt = vAt + vTotalTrans;

	D3DXMatrixLookAtLH(&mView, &vEye, &vAt, &vUp);
}

void CFreeCamera::rotate(float rx, float ry)
{
	vRot.x += rx;
	vRot.y -= ry;
	D3DXVECTOR3 vRight;
	D3DXVECTOR3 v3VecAt = vAt - vEye;
	D3DXVec3Cross( &vRight, &v3VecAt, &vUp );

	D3DXMATRIX mYaw, mPitch;
	D3DXMatrixRotationAxis(&mYaw, &vUp, rx);
	D3DXMatrixRotationAxis(&mPitch, &vRight, -ry); // positive for inverted control
	
	D3DXVec3TransformCoord(&v3VecAt, &v3VecAt, &mYaw);
	D3DXVec3TransformCoord(&vUp, &vUp, &mPitch);
	D3DXVec3TransformCoord(&v3VecAt, &v3VecAt, &mPitch);

	vAt = v3VecAt + vEye;
	D3DXVec3Normalize( &vUp, &vUp );

	D3DXMatrixLookAtLH(&mView, &vEye, &vAt, &vUp);
}

CFreeCamera::~CFreeCamera(void)
{
}
