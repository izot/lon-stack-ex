#ifndef _LTTRANSACTIONS_H
#define _LTTRANSACTIONS_H
//
// LtTransactions.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtTransactions.h#2 $
//

/* Comment about possible issue 
 * Since this code uses only two transaction spaces, there is the potential to
 * have a problem after a reset (since only the first tx gets txid 0).  On reset,
 * we don't zero the txid.  However, on a power cycle, need to delay if the receive
 * transactions can be longer than the power cycle time.  Currently, for JavaOS,
 * the reset time is >> the transaction time so no need to do anything. */
   
#define SCHEDULING_LATENCY   20

// The maximum value of a transaction ID when using LS legacy mode
#define LS_LEGACY_MODE_MAX_TX_ID   0x000f

// The maximume value of a transaction ID when using LS enhanced mode
#define LS_ENHANCED_MODE_MAX_TX_ID 0x0fff // A 12 bit transaction space.

// Code for dealing with layer 4/5 transactions
// Multiplied by txTimer to yield lock out time on a TX id.  This 
// allows late responses to come in but not map into the wrong
// tx.
#define TX_WAIT_MUL 3
#define TX_WAIT_DIV 2


#define NUM_MEMBERS   64

class LtAckMap 
{

private:
	int     m_nMaxMember;
	int     m_nCount;
	byte    m_byMap[NUM_MEMBERS/8];

protected:

public:
	LtAckMap();
    LtAckMap(LtPktInfo* pPkt);

	void init();
    int getCount();
    int getMax();
    boolean getReceived(int groupMember);
    boolean setReceived(int groupMember);
    void setMap(LtPktInfo* pPkt);
};

#define MAX_RCVTX_DURATION   24576
#define TX_IDLE   0
#define TX_WAIT   1
#define TX_WAIT_FOR_DELETION   2
#define TX_PROCESSING   3
#define TX_USER   10

class LtLayer4;
class LtTx;
class LtTransmitTx;

class LtTxSource : public LtObject
{
public:
	LtTxSource()
	{
		for (int j=0; j<LT_PRIORITY_TYPES; j++)
		{
			for (int i=0; i<LS_ENHANCED_MODE_MAX_TX_ID+1; i++)
			{
				m_txIdExpiration[j][i].expiration = System::currentTimeMillis();
                m_txIdExpiration[j][i].pTx = NULL;
			}
		}
		memset(m_pTxById, 0, sizeof(m_pTxById));
		m_lastUnsafeOperation = 0;
		initTxIds();
	}

	void initTxIds()
	{
		memset(m_curTxId, 0, sizeof(m_curTxId));
	}

    LtTransmitTx*	m_pTxById[LT_PRIORITY_TYPES][LS_ENHANCED_MODE_MAX_TX_ID+1];

    struct {
        unsigned int  expiration;
        LtTransmitTx *pTx;  // The TX that is using this expiration. This information
                            // is used to allow reuse of an unexpired transaction if 
                            // the new destination is the same as the original.
                            // Note  that as long as a transaction is outstanding, 
                            // this entry will be the same as the corresponding entry 
                            // in m_pTxById. However, the entry in m_pTxById is set 
                            // to NULL as soon as the transaction is complete, whereas 
                            // this entry must remain until the transaction ID has expired.
                            // WARNING - this entry should be used only to compare to the
                            // pointer of a transaction, as there is no guarantee that 
                            // the contents of this pointer is still valid.
                            // Note that this entry is never cleared.  It could be cleared
                            // on expiration, but since the only use is to see if its OK
                            // to use the transaction ID, and it is always OK if the
                            // transaction has expired, there is no reason to clear it.
    } m_txIdExpiration[LT_PRIORITY_TYPES][LS_ENHANCED_MODE_MAX_TX_ID+1];
    int				m_curTxId[LT_PRIORITY_TYPES];
	unsigned int	m_lastUnsafeOperation;
};

class LtTxSourceVector : public LtTypedVector<LtTxSource>
{
};

#define LT_OMA_DEST_DATA_LEN 9

class LtTx: public LtPktData 
{
private:
	LtLayer4* 		m_pOwner;
	WDOG_ID			m_wDogTimer;
	LtRefId			m_refId;
    LtServiceType   m_pendingType;
	boolean			m_bPendingFree;
	boolean			m_bExpirationOccurred;
	int				m_ignoreCount;
	boolean			m_bOnTxQ;

    LtApdu*         m_pApdu;
protected:
    boolean         m_bUseLsEnhancedMode;
    int             m_nExpirationDelta;
    int             m_nState;
	LtApdu*			getApdu() { return m_pApdu; }

public:
    LtTx(LtRefIdType nType, int nIndex);
	~LtTx();

	void init();

    static void dumpTxData();  // TBD - remove

	void setOwner(LtLayer4* pOwner) { m_pOwner = pOwner; }

	void makeFree() { m_nState = TX_IDLE; setRefId(0); m_bPendingFree = false; }
	boolean isFree() { return m_nState == TX_IDLE; }
	boolean freePending() { return m_bPendingFree; }
	void setPendingFree() { m_bPendingFree = true; }

    boolean inUse();
	boolean getReceive() {return m_refId.getType() == LT_REF_RX; }
    int getPriorityType();
    int getDelta();
	int getState() { return m_nState; }
    void setDelta(int delta);
    void setExpiration(int state = TX_WAIT);
    void setExpiration(int state, int nDelta, int nDeltaLast=0);
	virtual void setExpirationDeltaLast(int x) {}
    virtual void setDeleteTimer(int duration) {}
	virtual boolean active();
    boolean getDeletionRequired();
    virtual void expiration() {}
	boolean valid() { return inUse(); }

    boolean operator==(LtPktData& data);

    boolean isSession() { return getEnclPdu() == LT_SPDU; }

    boolean getIsGroup() { return getAddressFormat() == LT_AF_GROUP; }

	LtLayer4* getOwner() { return m_pOwner; }
    void startTimer(int nTicks);
	void stopTimer();
	virtual void restartTimer();

	LtRefId& getRefId() { return m_refId; }
	void setRefId(int nRefId) { m_refId.setRefId(nRefId); }

    byte* getData() { return m_pApdu->getData(); }
    int getLength() { return m_pApdu->getLength(); }

    LtServiceType getPendingType() { return m_pendingType; }
    void setPendingType(LtServiceType type) { m_pendingType = type; }

    void setApdu(LtApdu* pApdu) { m_pApdu = pApdu; }

    void encrypt(byte* pValue, LtDomainConfiguration *pDomain, byte* pKey, boolean isOma);
    // Returns the destination address data in the form used by OMA authentication
    void getOmaDestData(byte *pData, LtDomainConfiguration *pDomain);

	static int VXLCDECL TxTimeout(int nParam, ...);

	boolean match(LtPktData* pData);

	boolean getExpirationOccurred()	{ return m_bExpirationOccurred; }
	void setExpirationOccurred(boolean bValue) { m_bExpirationOccurred = bValue; }

	void bumpIgnoreCount() { m_ignoreCount++; }

	boolean ignoreExpiration();

	boolean getOnTxQ() { return m_bOnTxQ; }
	void setOnTxQ(boolean bValue) { m_bOnTxQ = bValue; }

    boolean getUseEnhancedMode(void) { return m_bUseLsEnhancedMode; }
    void setUseEnhancedMode(boolean bUseLsEnhancedMode) { m_bUseLsEnhancedMode = bUseLsEnhancedMode; }

};

#define TX_SEND_REMINDER_RETRY  TX_USER
#define TX_SEND_RETRY			TX_USER+1
#define TX_DEFERRED				TX_USER+2
#define TX_UNBLOCKED			TX_USER+3

class LtTransmitTx: public LtTx 
{

private:
    boolean     m_bUnackdRpt;
	boolean     m_bGroup;
    boolean     m_bTurnaround;
    boolean     m_bFirst;
    boolean     m_bUseCurrent;
    int         m_nRetries;
    int         m_nExpectedAcks;
    int         m_nAckCount;
    LtAckMap    m_ackMap;
	LtApdus*    m_pApdusPending;
	LtTxSource* m_pTxSource;
	int			m_nAlternateTxId;
	int			m_nNodeState;
    int         m_nRetriesSent;
    int         m_altPath;
	int			m_nExpirationDeltaLast;	// Extra time for last attempt

protected:

public:
    LtTransmitTx(int i) : LtTx(LT_REF_TX, i) 
	{
		m_nAlternateTxId = -1;
		m_nNodeState = -1;
		m_pApdusPending = NULL;
		m_nExpirationDeltaLast = 0;
	}
	~LtTransmitTx();
    void init(LtApduOut* pApdu);
    LtAckMap* getAckMap();
    LtApduOut* getApdu() { return (LtApduOut*) LtTx::getApdu(); }
    boolean isUnackdRpt();
    boolean resendNeeded();
    boolean reminderNeeded();
    boolean expired();
    boolean getTurnaround();
    void setTurnaround(boolean v);
    boolean deferred();
    void addPending(LtApdu* pApdu);
    void defer(LtApdu* pApdu, int duration);
    void complete();
    boolean pendingPdu();
    int minTxId();
    void send(LtApduOut* pApdu, LtPktInfo* pPkt);
    void altPath(LtPktInfo* pPkt);
	boolean txIdOk(int txid);
    void expiration();
    boolean ambiguousAddress();
    boolean isComplete(boolean afterTimeout);
    boolean receivedValidAck(LtPktInfo* pPkt);
    boolean buildReminderMsg(LtApdu* pApdu, LtPktInfo* pPkt);
    boolean valid();
	void setDeleteTimer(int duration);
	LtTxSource* getTxSource() { return m_pTxSource; }
	void setTxSource(LtTxSource* pTxSource) { m_pTxSource = pTxSource; }
	void setTxNumber(int nTxNumber);
	boolean active();
    int getRetryCount() {return m_nRetriesSent;}
    void bumpRetryCount() {m_nRetriesSent++;}
    void setAltPath(int altPath) { m_altPath = altPath; }
	virtual void restartTimer();
	virtual void setExpirationDeltaLast(int x) { m_nExpirationDeltaLast = x; }
};

#define TX_DONE					TX_USER
#define TX_AUTHENTICATING		TX_USER+1
#define TX_PENDING				TX_USER+2
#define TX_ACK					TX_USER+3
#define TX_NOBUF				TX_USER+4
#define FIXED_UNIQUE_ID_TIMER   8192

class LtReceiveTx: public LtTx 
{

private:
    byte    m_byResponse[MAX_APDU_SIZE];
	int		m_nResponseLength;
    boolean m_bAuthenticated;  
    boolean m_bDuplicate;
    byte    m_byChallenge[LT_CHALLENGE_LENGTH];
	int		m_nDomainIndex;

    static byte m_byLastChallenge[LT_CHALLENGE_LENGTH];

protected:

public:
    LtReceiveTx(int i) : LtTx(LT_REF_RX, i) {}
    void init(LtPktInfo* pPkt);
    void deliver();
    void markAsDuplicate();
    LtApduIn* getApdu() { return (LtApduIn*) LtTx::getApdu(); }
    boolean isDuplicate();
    byte* getResponse();
    boolean valid();
	int getResponse(byte*& response);
    LtErrorType registerResponse(byte* response, int length, boolean nullResponse, boolean bRespondOnFlexDomain);
	void setChallenge();
    void getChallenge(byte* pChallenge) { memcpy(pChallenge, m_byChallenge, sizeof(m_byChallenge)); }
    void setChallenge(byte* pChallenge) { memcpy(m_byChallenge, pChallenge, sizeof(m_byChallenge)); }
    void done();
    boolean resendNeeded();
    void expiration();
    boolean authenticating();

	int getDomainIndex() { return m_nDomainIndex; }
	void setDomainIndex(int domainIndex) { m_nDomainIndex = domainIndex; }
};

#endif
