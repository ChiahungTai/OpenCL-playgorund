/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file Processor.h

#include "RefCounted.h"

namespace collada
{
	/// processor may have several outputs
	struct Processor : public RefCountedCyclic
	{
		virtual bool DoProcess() = 0; ///< @return true when dirty state changes

		/// processor become dirty when one or more inputs become dirty
		struct IDirtyListener
		{
			virtual void OnDirty(Processor *which) = 0;
		};
		virtual void AddListener(IDirtyListener *ls) = 0;
		virtual void RemoveListener(IDirtyListener *ls) = 0;
	};

} // end of namespace collada

// end of file
