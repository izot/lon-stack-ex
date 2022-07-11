#ifndef LTCONFIGURATIONENTITY_H
#define LTCONFIGURATIONENTITY_H
//
// LtConfigurationEntity.h
//
// Copyright © 2022 Dialog Semiconductor
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in 
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// This file contains a definition of the base class for the various
// forms of stack configuration information.  This includes domain,
// address, NV, alias and monitor set configuration.  The node itself
// also implements this interface.
//

//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtConfigurationEntity.h#1 $
//

#if FEATURE_INCLUDED(MONITOR_SETS)
#include "LtDescription.h"
#endif

class LtConfigurationEntity 
{
public:
	virtual ~LtConfigurationEntity() {}
	// Enumerate is only required method.
	virtual LtErrorType enumerate(int index, boolean authenticated, LtApdu &response) = 0;

	// Operations on a specific element or element range.
	virtual LtErrorType initialize(int fromIndex, int toIndex, byte* pData, int len, int domainIndex) { return LT_NOT_IMPLEMENTED; }
	virtual LtErrorType update(int index, byte* pData, int len) { return LT_NOT_IMPLEMENTED; }
	virtual LtErrorType remove(int fromIndex, int toIndex) { return LT_NOT_IMPLEMENTED; }
	// By default, create is same as an update.
	virtual LtErrorType create(int index, byte* pData, int len) 
	{ 
		return update(index, pData, len);
	}

	// Catch all
	virtual LtErrorType resourceSpecificCommand(int cmd, int index, byte* pData, int len, boolean authenticated, LtApdu &response) { return LT_NOT_IMPLEMENTED; }

	// Limit checker - certain commands may have unique limits.
	virtual LtErrorType checkLimits(int cmd, byte* pData, int len);

	// Designates use of primary configuration file.  If persistence
	// is in something other than the primary file (IMGx.dat), then
	// functions above must handle persistence themselves.  Otherwise
	// the caller does.
	virtual boolean affectsNetworkImage() { return true; }
};

#endif
