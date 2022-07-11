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

#ifndef _LTMONITORPOINTTABLE_H
#define _LTMONITORPOINTTABLE_H

//
// LtMonitorPointTable.h
//
// CopyRight 1999 Echelon Corporation.  All Rights Reserved.
//

//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtMonitorPointTable.h#1 $
//

#include "LtMonitorControlTable.h"
#include "LtMonitorPoint.h"

// This class defines a collection of monitor sets and thier configuration.
class LtMonitorPointTable: public LtMonitorControlTable
{
public:
    LtMonitorPointTable() : LtMonitorControlTable("MonPoint") {}
	virtual ~LtMonitorPointTable() {}

	virtual LtMonitorControl* createInstance()
	{
		return new LtMonitorPoint();
	}

	LtMonitorPoint* get(LtMpIndex index, boolean bValidOnly = LT_MC_ALL) 
	{
		return (LtMonitorPoint*) LtMonitorControlTable::get(index, bValidOnly);
	}

	LtMonitorPoint* getNext(LtMpIndex &index, boolean bValidOnly = LT_MC_ALL)
	{
		return (LtMonitorPoint*) LtMonitorControlTable::getNext(index, bValidOnly);
	}

	LtErrorType checkLimits(int cmd, byte* pData, int len);
};

#endif
