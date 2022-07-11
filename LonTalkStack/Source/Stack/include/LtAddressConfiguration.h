#ifndef LTADDRESSCONFIGURATION_H
#define LTADDRESSCONFIGURATION_H
//
// LtAddressConfiguration.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtAddressConfiguration.h#1 $
//

#define LT_ADDRESS_STORE_SIZE   7

// Group Restrictions
#define LT_GRP_NORMAL		0
#define LT_GRP_OUTPUT_ONLY	1
#define LT_GRP_INPUT_NO_ACK 2

class LTA_EXTERNAL_CLASS LtAddressConfiguration : public LtObject
{
	DefineInt(Index)
	DefineInt(Subnet)
	DefineInt(DomainIndex)

	// Group/subnet fields
	DefineInt(DestId)

	// Group only fields
	DefineInt(Size)
	DefineInt(Member)
	DefineInt(Restrictions)

	// Broadcast only fields
	DefineInt(Backlog)

private:
    int m_nTxTimer;
    int m_nRptTimer;
    int m_nRcvTimer;
    int m_nRetry;
    int m_nAddressType;
    int m_nMaxResponses; // Broadcast/group only
	int m_nTxTimerDeltaLast;	// Used to extend the last retry timer duration - in milliseconds
    void init();

protected:
    friend class LtStackBlob;
    LtAddressConfiguration(LtBlob &blob)
    {
        package(&blob);
    }
    void package(LtBlob *pBlob);

protected:

public:
	static int getStoreSize() { return LT_ADDRESS_STORE_SIZE; }

    boolean equals(LtAddressConfiguration& cmp);
    LtAddressConfiguration();
    LtAddressConfiguration(int index);
    void copy(LtAddressConfiguration& ac);
    LtAddressConfiguration(int type, int domainIndex, int id);
	virtual ~LtAddressConfiguration() {}
	inline int getGroup() { return getDestId(); }
    boolean isBound();
	boolean isBoundExternally();
	boolean inUse() { return m_nAddressType != LT_AT_UNBOUND; }
    void setGroup(int group) { setDestId(group); }
    int toLonTalk(byte* pData, int version);
    LtErrorType fromLonTalk(byte data[], int& len, int version);

    int getAddressType() { return m_nAddressType; }
    void setAddressType(int type) { m_nAddressType = type; }
	boolean isUnsafe() { return m_nAddressType == LT_AT_UNIQUE_ID ||
								m_nAddressType == LT_AT_BROADCAST ||
								m_nAddressType == LT_AT_BROADCAST_GROUP; }

    int getTxTimer() { return m_nTxTimer; }
    void setTxTimer(int value) { m_nTxTimer = min(value,65535); }

	int getTxTimerDeltaLast() { return m_nTxTimerDeltaLast; }
	void setTxTimerDeltaLast(int value) { m_nTxTimerDeltaLast = min(value, 65535); }

    int getRptTimer() { return m_nRptTimer; }
    void setRptTimer(int value) { m_nRptTimer = min(value,65535); }

    int getRcvTimer() { return m_nRcvTimer; }
    void setRcvTimer(int value) { m_nRcvTimer = min(value,65535); }

    int getRetry() { return m_nRetry; }
    void setRetry(int value) { m_nRetry = min(value,15); }

    int getMaxResponses() { return m_nMaxResponses; }
    void setMaxResponses(int value) { m_nMaxResponses = value; }
};

#endif
