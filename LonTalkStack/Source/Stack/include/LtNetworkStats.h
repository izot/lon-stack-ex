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

#ifndef _LTNETWORKSTATS_H
#define _LTNETWORKSTATS_H

//
// LtNetworkStats.h
//
// CopyRight 1998 Echelon Corporation.  All Rights Reserved.
//				** ECHELON CONFIDENTIAL **
//

//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtNetworkStats.h#1 $
//

#define LT_TRANSMISSION_ERRORS   0
#define LT_TRANSMIT_FAILURES   1
#define LT_RCVTX_FULL   2
#define LT_LOST_MESSAGES   3
#define LT_MISSED_MESSAGES   4
#define LT_LAYER2_RECEIVE   5
#define LT_LAYER3_RECEIVE   6
#define LT_LAYER3_TRANSMIT   7
#define LT_RETRIES   8
#define LT_BACKLOG_OVERFLOW   9
#define LT_LATE_ACKS   10
#define LT_COLLISIONS   11
#define LT_EEPROM_LOCK   12
#define LT_NUM_STATS   13

class LtNetworkStats 
{

private:
	LtLreServer* m_pLre;

    LtLink* drv;
    
    int netStats[LT_NUM_STATS];

protected:

public:
    LtNetworkStats(LtLink* d, LtLreServer* pLre);
    void bump(int index);
    int get(int index);
    boolean getEepromLock();
    void setEepromLock(boolean l);
    void reset();
	void refresh();
    LtErrorType validate(int offset, int length);
    LtErrorType fromLonTalk(byte data[], int offset, int length);
    LtErrorType toLonTalk(byte** ppData, int offset, int length);
 };

#endif
