/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file Win32Utils.cpp
///
/// @brief Contains implementation of helper functions declared in WinUtils.h

#include "stdafx.h"

#include <Windows.h>
#include "win32/Win32Utils.h"

#include "NodeControlPlugin.h"
using namespace collada;

static const float DOLLY_SPEED_NORMAL = 0.1f;
static const float DOLLY_SPEED_SLOW   = 0.005f;

static const float PAN_SPEED_NORMAL = 1.0f;
static const float PAN_SPEED_SLOW   = 0.1f;


LRESULT HandleCameraMovement(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, float sceneScale, INodeControlView *ctrl)
{
	static int s_xOld = 0;
	static int s_yOld = 0;

	short xPos = LOWORD(lParam);
	short yPos = HIWORD(lParam);

	switch( uMsg )
	{
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
		s_xOld = xPos;
		s_yOld = yPos;
		SetCapture(hWnd);
		break;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
		ReleaseCapture();
		break;

	case WM_MOUSEWHEEL:
		if( ctrl )
		{
			float scale = (wParam & MK_CONTROL) ? DOLLY_SPEED_SLOW : DOLLY_SPEED_NORMAL;
			ctrl->TranslateDelta(0, 0, - (float) (short) HIWORD(wParam) * sceneScale * scale / WHEEL_DELTA);
		}
		break;

	case WM_MOUSEMOVE:
		if( ctrl )
		{
			RECT rt;
			GetClientRect(hWnd, &rt);

			float dx = (float) (xPos - s_xOld) / (float) (rt.right - rt.left);
			float dy = (float) (yPos - s_yOld) / (float) (rt.bottom - rt.top);
			s_xOld = xPos;
			s_yOld = yPos;

			if( wParam & MK_LBUTTON )
			{
				const float scale = 2;
				if( wParam & MK_CONTROL )
				{
					// first person mode
					ctrl->RotateDeltaFP(-dx * scale, -dy * scale, 0);
					ctrl->RemoveRoll(NULL);
				}
				else
				{
					// model viewer mode
					ctrl->RotateDelta(-dx * scale, -dy * scale, 0);
				}
			}

			if( wParam & MK_MBUTTON )
			{
				float scale = (wParam & MK_CONTROL) ? PAN_SPEED_SLOW : PAN_SPEED_NORMAL;
				ctrl->TranslateDelta(-dx * sceneScale * scale, dy * sceneScale * scale, 0);
			}
		}
		break;
	}

	return 0;
}


// end of file
