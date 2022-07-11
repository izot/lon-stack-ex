//
// LtMonitorControlTable.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtMonitorSetTable.h#1 $
//
#ifndef _LTMONITORSETTABLE_H
#define _LTMONITORSETTABLE_H

#include "LtMonitorControlTable.h"
#include "LtMonitorSet.h"

class LtMonitorPointTable;

// This class defines a collection of monitor sets and their configuration.
class LtMonitorSetTable: public LtMonitorControlTable
{
public:
    LtMonitorSetTable() : LtMonitorControlTable("MonSet") {}
    virtual ~LtMonitorSetTable();

	virtual LtMonitorControl* createInstance()
	{
		return new LtMonitorSet(m_pMpTable);
	}

	LtMonitorSet* get(LtMsIndex index, boolean bValidOnly = LT_MC_ALL) 
	{
		return (LtMonitorSet*) LtMonitorControlTable::get(index, bValidOnly);
	}

	LtMonitorSet* getNext(LtMsIndex &index, boolean bValidOnly = LT_MC_ALL)
	{
		return (LtMonitorSet*) LtMonitorControlTable::getNext(index, bValidOnly);
	}

	void setMpTable(LtMonitorPointTable* pMpTable) { m_pMpTable = pMpTable; }

	LtErrorType checkLimits(int cmd, byte* pData, int len);

private:
	LtMonitorPointTable* m_pMpTable;
};

#endif
