/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file WinUtils.h
///
/// @brief Contains definition of helper functions specific to Win32 applications

#pragma once

// forward declaration
namespace collada
{
	struct INodeControlView;
}


LRESULT HandleCameraMovement(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                             float sceneScale, collada::INodeControlView *ctrl);


// end of file
