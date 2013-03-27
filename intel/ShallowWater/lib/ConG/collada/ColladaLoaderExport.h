#pragma once

#include "SceneGraphBase.h"

namespace collada
{

	/// Load scene from specified COLLADA file. This function may be used either to create a new scene 
	/// or to merge the file contents with an existing scene.
	extern "C" void OpenColladaFile(const TCHAR *szFileName, EditableScene *pResult);

} // end of collada namespace

// end of file