/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

#include "fluidsimhost.h"

int _tmain(int argc, TCHAR *argv[])
{
#if defined( DEBUG ) || defined( _DEBUG )
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // The app() function creates a #CFluidSimHost object on the stack and then calls its default constructor.
    CFluidSimHost app(NULL);

    // Call the #CFluidSimHost::Run() function to pass control to the class.
    // The exit code of the #CFluidSimHost::Run() function is used as the application return value.
    return app.Run();
}


/* end of file */
