//
// LtMonitorControl.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtMonitorControl.h#1 $
//
#ifndef _LTMONITORCONTROL_H
#define _LTMONITORCONTROL_H

#include "LtDescription.h"

class LTA_EXTERNAL_CLASS NdMonitorControl : public NdCollectionElement
{
public:
	NdMonitorControl(boolean isMonitorPoint) : m_bIsMonitorPoint(isMonitorPoint) 
	{
		m_bAppNotifiedOfCreate = false;
	}

	LtDescription&	getDescription() { return m_description; }
	byte getOptions() { return m_options; }
	void setOptions(byte options) { m_options = options; }

protected:
	friend class LtMonitorControlTable;
	void setAppNotifiedOfCreate() { m_bAppNotifiedOfCreate = true; }
	boolean getAppNotifiedOfCreate() { return m_bAppNotifiedOfCreate; }
	boolean getIsMonitorPoint() { return m_bIsMonitorPoint; }

    virtual int toLonTalk(byte* data, int nVersion = 1) = 0;
    virtual int fromLonTalk(byte* data, int length, int nVersion = 1) = 0;
	virtual void removeAllRelatedItems() {}

	LT_SERIALIZABLE_VIRTUAL;

private:
	boolean m_bAppNotifiedOfCreate;
	LtDescription m_description;
	byte m_options;
	boolean m_bIsMonitorPoint;
};

typedef NdMonitorControl LtMonitorControl;

#endif
