/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file MsgListenerStdImpl.cpp
///
/// @brief Contains the MsgPrinterPrintf class implementation

#include "stdafx.h"

#include "MessageListener.h"
#include "RefCountedImpl.h"

#include <cstdio>

using namespace collada;

namespace
{
	class MsgPrinterStd : public RefCountedImpl<IMessageListener>
	{
	public:
		// IMessageListener
		virtual void Message(collada::MessageSeverity severity, const char *szMessage)
		{
			static const char s[][16] = {"[ERROR]", "[WARNING]", "[INFO]", "[DEBUG]"};
			if( severity <= collada::MSG_SEVERITY_DEBUG )
			{
				printf("%s %s\n", s[severity], szMessage);
			}
		}
	};
}

namespace collada
{
	void CreateMessageListenerStd(IMessageListener **ppResult)
	{
		AutoPtr<IMessageListener> result(new MsgPrinterStd());
		DetachRawPtr(result, ppResult);
	}
}

// end of file
