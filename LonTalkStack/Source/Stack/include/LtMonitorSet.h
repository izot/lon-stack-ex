//
// LtMonitorSet.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtMonitorSet.h#1 $
//
#ifndef _LTMONITORSET_H
#define _LTMONITORSET_H

#include "LtMonitorPoint.h"

#define LT_MONITOR_SET_SIZE 3		// Size of LonTalk data

#define MCS_IS_PERSISTENT   0x01
typedef byte McsOptions;

#ifdef WIN32
// Explicitly instantiate this template class so we can attach the
// __declspec() attribute to it for the DLL implementation.
// The Tornado compiler (GNU) cannot handle this.
template class LTA_EXTERNAL_CLASS NdCollection<NdMonitorPoint>;
#endif

class LtMonitorPointTable;

class LTA_EXTERNAL_CLASS NdMonitorSet : public NdMonitorControl
{
    friend class LtMonitorSetTable;

public:
	NdMonitorSet(LtMonitorPointTable* pMpTable) : NdMonitorControl(false), m_pMpTable(pMpTable) {}
    virtual ~NdMonitorSet();
	void removeAllRelatedItems();

	LtMonitorPoint *getMonitorPoint(LtMpIndex mpIndex, boolean bValidOnly = LT_MC_VALID_DESCRIPTION_ONLY);
    LtMonitorPoint *getNextMonitorPoint(LtMpIndex mpIndex, boolean bValidOnly = LT_MC_VALID_DESCRIPTION_ONLY);
    LtMsIndex       getIndex() { return NdCollectionElement::getIndex(); }
	LtDescription&  getDescription() { return NdMonitorControl::getDescription(); }
	McsOptions		getOptions() { return (McsOptions) NdMonitorControl::getOptions(); }

private:
	LtMonitorPointTable* m_pMpTable;
};

class LTA_EXTERNAL_CLASS LtMonitorSet : public NdMonitorSet
{
public:
	LtMonitorSet(LtMonitorPointTable* pTable) : NdMonitorSet(pTable) {}
	virtual ~LtMonitorSet() {}

    int toLonTalk(byte* data, int nVersion = 1);
    int fromLonTalk(byte* data, int length, int nVersion = 1);
};

#endif
