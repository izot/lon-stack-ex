#ifndef _LTLAYER4_H
#define _LTLAYER4_H

//
// LtLayer4.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtLayer4.h#2 $
//

#include "LtLayer4Intf.h"

typedef enum
{
    LT_TX_STAT_LATE_ACK,
    LT_TX_STAT_RETRY,
    LT_TX_STAT_FAILURE,

    NUM_LT_TX_STATS
} LtTransactionStatistic;

#define TX_WINDOW_SIZE      50
#define TX_INITIAL_LIMIT    1
#define TX_LIMIT_MAX        100000

typedef enum
{
    LT_TX_MSG,
    LT_APDU_MSG,
	LT_RESET_MSG,
} LtMsgOutType;

class LtMsgOutgoing
{
private:
    LtMsgOutType    m_type;
    void*           m_pObject;
public:
    LtMsgOutgoing() {}
    LtMsgOutgoing(LtMsgOutType type, void* pObject) : m_type(type), m_pObject(pObject) {}
    LtMsgOutType getType() { return m_type; }
    void* getObject() { return m_pObject; }
};

// Add one to allow for flex domain at index 0.
#define TX_SOURCE_INDEX(a, b) (a*NUM_ADDRESSES_PER_DOMAIN + b + 1)


// Allocate extra space in LtPktInfo so that it can hold a message with
// the maximum APDU plus the maximum address space, even though this may
// exceed the MAX_LINK_PKT_SIZE.  Note that a LtApdu can contain a L2
// APDU, so it supports up to MAX_LPDU_SIZE (rather than MAX_NPDU_SIZE).
// This limit is regardless of whether its an L2 packet or not, so we might
// get an L6 packet with up to MAX_NPDU_SIZE, and we will need to add 
// room for network headers to that
// 
// Need to allow for:
//   1 byte layer 2 header
//   1 byte layer 3 header
//   1 byte source subnet
//   1 byte source node
//   1 byte destination address type
//   2 byte CRC
//  + 6 byte domain
//  + worst case address = 6 byte neuron ID + 1 byte subnet
//  + up to 2 byte TPDU/SPDU/AUTHPDU header (1 for compatiblity mode, and 2 for enhanced mode)
// = 22
#define LT_PKT_INFO_BUFFER_SIZE (MAX_LPDU_SIZE+22)

class LtLayer4 : public LtLayer4Intf, public LtTaskOwner
{
private:
    LtDeviceStack*								m_pStack;
	LtTypedTxs<LtTransmitTx>*                   m_pTxsTx;
	LtTypedTxs<LtReceiveTx>*        		    m_pTxsRx;
	LtTxSourceVector							m_txSources;

	int				m_taskIdInput;
    int             m_taskIdOutput;
	int				m_taskIdTimeouts;
	MSG_Q_ID		m_msgTimeouts;
	MSG_Q_ID		m_msgTxOut;
	MSG_Q_ID		m_msgRxOut;
	MSG_Q_ID		m_throttledMsgOut;
    MSG_Q_ID		m_unthrottledMsgOut;

	SEM_ID			m_semMsg;
	SEM_ID			m_semTx;
	boolean			m_bResetRequested;
	int				m_txIdLifetime;
	boolean			m_bCommIgnore;
	boolean			m_bReceiveAllBroadcasts;
    boolean         m_bUseLsEnhancedMode;
    boolean         m_bUseLsEnhacedModeOnly;  // Emulate a device that only supports enhanced mode.

    // Used to reduce reported capabilities for node simulation.
    boolean         m_bNmVersionOveridden;
    int             m_nmVersionOverride;
    int             m_nmCapOveride;

	LtRefQue m_pkts;
    
    LtPktAllocator  m_pktAlloc[LT_PRIORITY_TYPES];

	LtTxSource*	getTxSource(LtApduOut* pApduOut);
	LtTxSource* getTxSource(LtPktInfo* pPktInfo);

    boolean	        m_nTxActive;
	boolean         m_bLimitHit;
	boolean         m_bException;
	boolean         m_bLastAdjustmentPositive;
    boolean         m_bReceivedSomething;
    boolean         m_bOmaSupported;
    unsigned int    m_nTxLimit;
    unsigned int    m_nTxCount;
	unsigned int    m_nUpperLimit;
    unsigned int    m_txStats[NUM_LT_TX_STATS];

protected:
	boolean notifySubnetNodeClient(LtAddressConfiguration* pAc);

public:
    LtLayer4(int nRxTx, int nTxTx, int maxOutputPackets, int maxPriorityOutputPackets);
    ~LtLayer4();

    void halt();
    void resume();

	void shutdown();

    LtErrorType receiveMsg(LtPktInfo* pPkt);
    LtErrorType receiveChallenge(LtPktInfo* pPkt);
    void sendReply(LtPktInfo* pPkt);
    void receiveReminder();
    void sendReminder(int pduType);

    void send(LtApduOut* pApdu, boolean wait = true, boolean throttle = true);
    void notifyOutgoing(LtMsgOutType type, LtTx* pTx);
	void notifyExpiration(LtTx* pTx);
	void processIncoming();
	void processOutgoing();
	void processTimeouts();

	void queuePacket(LtPktInfo* pPkt);
    void sendPacket(LtPktInfo* pPkt);

	// Convert static members for task entries to globals to make vxworks task 
	// display more readable because of the shorter (unqualified) names
	//static int	VXLCDECL startOutput( int stack, ... );
	//static int	VXLCDECL startInput( int stack, ... );
	//static int	VXLCDECL startTimeouts( int stack, ... );

    void reset();
	void resetTx();
    void terminate();
    LtErrorType receive(LtPktInfo* pPkt);

    LtDeviceStack* getStack() { return m_pStack; }
    void setStack(LtDeviceStack* pStack) 
	{ 
		m_pStack = pStack; 
		m_pTxsTx->setOwner(this);
		m_pTxsRx->setOwner(this);
	}

	void setTxIdLifetime(int duration);

    LtErrorType handleTxSend(LtTx* pTx, LtPktInfo* pPkt);
    void prepareForSend(LtTx* pTx);
    LtTransmitTx* getTransmitTx(LtPktInfo* pPkt);
    LtErrorType constructResponse(LtApduOut* pApdu, LtPktInfo* pPkt);
	LtErrorType deliver(LtReceiveTx* pTx, LtApduIn* pApdu);
    LtReceiveTx* getReceiveTx(LtPktInfo* pPkt);
    int nextTxId(int id);
    LtErrorType getNextTx(LtTransmitTx** ppTx, LtApduOut* pApdu, LtPktInfo* pPkt);
    void setTransactionId(LtApduOut* pApdu, LtPktInfo* pPkt, LtTransmitTx* pTx);
    LtErrorType sendReceiveTx(LtReceiveTx* pTx, LtPktInfo* pPkt);
    LtErrorType buildMessage(LtTransmitTx* pTx, LtApduOut* pApdu, LtPktInfo* pPkt);
    LtErrorType buildMessage2(LtTransmitTx* pTx, LtApduOut* pApdu, LtPktInfo* pPkt);
    LtErrorType construct(LtApduOut* pApdu, LtPktInfo* pPkt);
    LtErrorType sendTransmitTx(LtTransmitTx* pTx, LtPktInfo* pPkt);
    void completeTx(LtTransmitTx* pTx, boolean success);
    void run();
    void expirationCheck();
    LtErrorType receiveAck(LtPktInfo* pPkt);
    LtErrorType receiveReply(LtPktInfo* pPkt);
    void sendChallenge(LtReceiveTx* pTx, LtPktInfo* pPkt, boolean bUseOma);
    void expiration(LtTx* pTx);
    void flipAuthAddress(LtPktInfo* pPkt);
	void flipAckAddress(LtPktInfo* pPkt);
	void setResetRequested();
	void setErrorLog(int err);

    LtTx* refToTx(LtRefId& ref);
    LtPktInfo* allocatePacket(boolean bPriority);

	boolean getCommIgnore() { return m_bCommIgnore; }
	void setCommIgnore(boolean value) { m_bCommIgnore = value; }

	int getXcvrId();
	void setXcvrId(int id);

	void dumpTx();

    void getNmVersion(int &nmVersion, int &nmCapabilities);
        // Used to reduce reported capabilities for node simulation.
    LtErrorType setNmVersionOverride(int nmVersion, int nmCapabilities);

    LtErrorType disableOma();
    boolean omaSupported() { return m_bOmaSupported; }

    LtErrorType getDomainConfiguration(int nIndex, LtDomainConfiguration** ppDc);
    LtErrorType getFlexAuthDomain(LtDomainConfiguration** ppDc);

	LtErrorType getConfigurationData(byte* pData, int offset, int length);
	LtErrorType updateConfigurationData(byte* pData, int offset, int length);
	LtErrorType getDomainConfiguration(int nIndex, LtDomainConfiguration* pDc);
	LtErrorType updateSubnetNode(int domainIndex, int subnetNodeIndex, const LtSubnetNode &subnetNode);
    LtErrorType updateDomainConfiguration(int nIndex, LtDomainConfiguration* pDomain, boolean bStore = false, boolean bRestore = false);
    LtErrorType getAddressConfiguration(int nIndex, LtAddressConfiguration* pAc);
    LtErrorType updateAddressConfiguration(int nIndex, LtAddressConfiguration* pAddress, boolean bRestore = false);
	LtErrorType changeState(int newState, boolean bClearKeys);
  
    LtErrorType getReadOnlyData(byte* readOnlyData);
	LtErrorType initProgram(LtProgramId &pid);
	void sendServicePinMessage();
	void setReceiveAllBroadcasts(boolean bValue) { m_bReceiveAllBroadcasts = bValue; }
	boolean getReceiveAllBroadcasts() { return m_bReceiveAllBroadcasts; }

    boolean deferTx(void);
    void startTx(void);
    void endTx(void);
    void registerTransactionEvent(LtTransactionStatistic stat, int count=1);

    boolean getTransmitTxStats(int &nMax, int &nFree, int &nPendingFree, 
                               int &nMaxAllocated, int &nInstantiated, 
                               int &searchRatio, int &txAllocationFailures);

    boolean getReceiveTxStats(int &nMax, int &nFree, int &nPendingFree, 
                              int &nMaxAllocated, int &nInstantiated, 
                              int &searchRatio, int &txAllocationFailures);

    void clearTransmitTxStats(void);
    void clearReceiveTxStats(void); 

    int sizeOfTxSpace(void) { return m_bUseLsEnhancedMode ? LS_ENHANCED_MODE_MAX_TX_ID : LS_LEGACY_MODE_MAX_TX_ID; }
	LtErrorType waitForPendingInterfaceUpdates(void);
};

#endif
