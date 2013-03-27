/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file MessageListener.h

#include "SceneGraphBase.h"
#include "RefCounted.h"

namespace collada
{
	//
	// user-defined log output
	//

	/// Defines log message severity classes
	enum MessageSeverity
	{
		MSG_SEVERITY_ERROR,      ///< Describes a nonrecoverable or difficult to recover error.
		MSG_SEVERITY_WARNING,    ///< Describes non-critical error that still may impact visual or functionality.
		MSG_SEVERITY_INFO,       ///< Provides some additional information that may be interesting for user.
		MSG_SEVERITY_DEBUG,      ///< Provides some additional information for developer.
		//---------------------
		MSG_SEVERITY_COUNT,
	};

	/// Application provides this interface to receive messages from the framework during its operation.
	struct IMessageListener : public RefCounted
	{
		/// Output a message with specified severity class
		virtual void Message(MessageSeverity severity, const char *szMessage) = 0;
	};

	void CreateMessageListenerStd(IMessageListener **ppResult);

} // end of namespace collada
// end of file
