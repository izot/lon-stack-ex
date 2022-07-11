#ifndef _LTREADONLYDATA_H
#define _LTREADONLYDATA_H

//
// LtReadOnlyData.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtReadOnlyData.h#1 $
//

#include "LonTalk.h"

#define LT_ROD_UNIQUEID_OFFSET	0
#define LT_ROD_MODEL_OFFSET		6
#define LT_ROD_PROGRAMID_OFFSET   13
#define LT_ROD_STATE_OFFSET		21
#define LT_ROD_DOMAIN_OFFSET 	    21 // Byte Offset of number of domains bit in read only data
#define LT_ROD_ADDRESS_OFFSET		22
#define LT_ROD_MTAG_OFFSET		37

class LtNetworkImage;

#define LT_READ_ONLY_DATA_BUFFER_OFFSET 0x18
#define LT_READ_ONLY_DATA_BUFFER_NUM_BYTES  5 // size of buffer info in read only data.

class LTA_EXTERNAL_CLASS LtReadOnlyData {

private:
    // Offsets

    LtUniqueId uniqueId;
    LtProgramId programId;
    LtNetworkImage* m_pNetworkImage;

	byte lonTalk[41];

	static const byte lonTalkInit[41];
    void setProgramId();
	void setUniqueId();
    void setBuffers();

    byte m_netOutBufSize;
    byte m_netInBufSize;
    byte m_appOutBufSize;
    byte m_appInBufSize;
    byte m_numAppOutBufs[2];    // 0 non-priority, 1, priority
    byte m_numAppInBufs;
    byte m_numNetOutBufs[2];    // 0 non-priority, 1, priority
    byte m_numNetInBufs;

    static const byte sizeTable[16];
    static const byte countTable[16];

	boolean m_pendingUpdate;

    LtErrorType getEncodedCount(byte count, byte &encodedValue);
    LtErrorType getEncodedSize(byte size, byte &encodedValue);
    LtErrorType setLonTalkBuffers(void);

protected:

public:
	LtReadOnlyData() { m_pNetworkImage = null; 	m_pendingUpdate = FALSE; }
    LtReadOnlyData(class LtDeviceStack* pStack, int dc, int ac, int mt);
    LtUniqueId* getUniqueId();
    int getModel();
    LtProgramId* getProgramId();
    void setProgramId(LtProgramId& programId);
    int getLength();
	byte* getData() { return lonTalk; }
	LtErrorType fromLonTalk(int offset, int length, byte* pData);
	LtErrorType toLonTalk(int offset, int length, byte** ppData);

    void setNetworkBuffers(const LtReadOnlyData &source);
    const boolean netBuffersMatch(const LtReadOnlyData &ro);

    void setBuffers(const LtReadOnlyData &source);
    const boolean buffersMatch(const LtReadOnlyData &ro);

    int getBufferSize();

    byte getNetOutBufSize() { return m_netOutBufSize; }
    byte getNetInBufSize()  { return m_netInBufSize; }
    byte getAppOutBufSize() { return m_appOutBufSize; }
    byte getAppInBufSize()  { return m_appInBufSize; }
    byte getNumAppOutBufs(boolean bPriority) { return m_numAppOutBufs[bPriority]; }
    byte getNumAppInBufs() { return m_numAppInBufs; }
    byte getNumNetOutBufs(boolean bPriority) { return m_numNetOutBufs[bPriority]; }
    byte getNumNetInBufs() { return m_numNetInBufs; }

    LtErrorType setNetworkInputBuffers(byte netInBufSize, byte numNetInBufs);
    LtErrorType setNetworkOutputBuffers(byte netOutBufSize, byte numNetOutBufs, byte numNetPriorityOutBufs);
    LtErrorType setAppInputBuffers(byte appInBufSize, byte numAppInBufs);
    LtErrorType setAppOutputBuffers(byte appOutBufSize, byte numAppOutBufs, byte numAppPriorityOutBufs);

	boolean getPendingUpdate() { return m_pendingUpdate; }
	void setPendingUpdate(boolean pendingUpdate) { m_pendingUpdate = pendingUpdate; }

    /* Return the largest count that can be encoded that is <= count */
    static int getApproximateCount(byte count);

};

#endif

