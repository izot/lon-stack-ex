//
// LtMonitorPoint.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtMonitorPoint.h#1 $
//
#ifndef _LTMONITORPOINT_H
#define _LTMONITORPOINT_H

#include "LtMonitorSetDefs.h"

class LtMonitorSetTable;
class NdMonitorSet;

#define LT_MONITOR_POINT_SIZE	16

#define MCP_IS_MESSAGE_POINT 0x01
typedef byte McpOptions;

class LTA_EXTERNAL_CLASS NdMonitorPoint : public NdMonitorControl
{
    friend class LtMonitorSetTable;
	friend class LtMonitorSet;

public:
	NdMonitorPoint() : NdMonitorControl(true) {}
    virtual ~NdMonitorPoint() {};

    LtMsIndex          getMonitorSetIndex() {return m_monitorSetIndex; }
    LtMpIndex          getIndex() { return NdCollectionElement::getIndex(); }
    LtMtNvIndex        getNvOrMtIndex() { return m_nvOrMtIndex; }
    boolean            getIsMsg() { return getOptions() & MCP_IS_MESSAGE_POINT; }
	LtDescription&	   getDescription() { return NdMonitorControl::getDescription(); }
	McpOptions		   getOptions() { return (McpOptions) NdMonitorControl::getOptions(); }

        // NV only
    LtMpUpdateOptions  getUpdateOptions() { return m_updateOptions; }
    LtMpPollInterval   getPollInterval() { return m_pollInterval; }
    LtMpNotifyOptions  getNotifyOptions() { return m_notifyOptions; }
    LtMpNotifyInterval getNotifyInterval() { return m_notifyInterval; }
        // Message only...
    byte                getFilterCode() { return m_filterCode; }
    LtMpFilterOptions   getFilterOptions() { return m_filterOptions; }

	LT_SERIALIZABLE;

    int toLonTalk(byte* data, int nVersion = 1);
    int fromLonTalk(byte* data, int length, int nVersion = 1);

private:
    LtMsIndex           m_monitorSetIndex;
    LtMtNvIndex         m_nvOrMtIndex;

    // NV specific options
    LtMpUpdateOptions   m_updateOptions;
    LtMpPollInterval    m_pollInterval;
    LtMpNotifyOptions   m_notifyOptions;
    LtMpNotifyInterval  m_notifyInterval;

    // Msg specific options
    byte                m_filterCode;
    LtMpFilterOptions   m_filterOptions;
};

class LTA_EXTERNAL_CLASS LtMonitorPoint : public NdMonitorPoint
{
public:
	LtMonitorPoint() {}
	virtual ~LtMonitorPoint() {}

};

#endif
