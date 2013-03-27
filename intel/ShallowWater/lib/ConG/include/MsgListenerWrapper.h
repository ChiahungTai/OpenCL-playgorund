/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file MsgListenerWrapper.h

#pragma once

#include "AutoPtr.h"
#include "MessageListener.h"
#include <sstream>

class MsgListenerWrapper
{
	class StreamHelper
	{
	public:
		StreamHelper(collada::IMessageListener *ls, collada::MessageSeverity sev, std::string mod)
		  : m_ls(ls)
		  , m_sev(sev)
		  , m_mod(mod)
		{
		}

		StreamHelper(const StreamHelper &other)
		  : m_ls(other.m_ls)
		  , m_sev(other.m_sev)
		  , m_mod(other.m_mod)
		{
		}

		~StreamHelper()
		{
			if( m_ls && !m_buf.str().empty() )
			{
				if( m_mod.empty() )
				{
					m_ls->Message(m_sev, m_buf.str().c_str());
				}
				else
				{
					m_mod += ": ";
					m_mod += m_buf.str();
					m_ls->Message(m_sev, m_mod.c_str());
				}
			}
		}

		StreamHelper& operator << (const char *sz)
		{
			m_buf << (sz ? sz : "<null>");
			return *this;
		}

		StreamHelper& operator << (char *sz)
		{
			m_buf << (sz ? sz : "<null>");
			return *this;
		}

		StreamHelper& operator << (const wchar_t *sz)
		{
			m_buf << (sz ? sz : L"<null>");
			return *this;
		}

		StreamHelper& operator << (wchar_t *sz)
		{
			m_buf << (sz ? sz : L"<null>");
			return *this;
		}

		template <class T>
		StreamHelper& operator << (const T &arg)
		{
			m_buf << arg;
			return *this;
		}

	private:
		StreamHelper& operator = (const StreamHelper&);
		collada::IMessageListener *m_ls;
		collada::MessageSeverity m_sev;
		std::string m_mod;
		std::stringstream m_buf;
	};

	collada::AutoPtr<collada::IMessageListener> m_ls;
	std::string m_moduleName;

public:
	MsgListenerWrapper(collada::IMessageListener *ls, const char *moduleName = NULL)
	  : m_ls(ls)
	  , m_moduleName(moduleName ? moduleName : "")
	{
	}
	collada::IMessageListener* GetMessageListener() const
	{
		return m_ls;
	}
	StreamHelper operator() (collada::MessageSeverity sev)
	{
		return StreamHelper(m_ls, sev, m_moduleName);
	}
};

// end of file
