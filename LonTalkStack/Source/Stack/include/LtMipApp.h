#ifndef _LTMIPAPP_H
#define _LTMIPAPP_H

//
// LtMipApp.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtMipApp.h#4 $
//

#include "ltaBase.h"
#include "LtStackInternal.h"
#include "LtMip.h"

// MIP APP version
#define MIPAPP_VERSION	1

typedef unsigned short NetWord;

typedef enum _NSSMipMode
{
    NSS_ENGINE_MODE,        /* Neuron contains an engine (non-NSI) manager:
                               Block host from sending NM write commands,
                               route incoming nm-escape and service pin
                               to neuron */

    NSS_PRIVILEGED_MODE,    /* Reserved used.
                               Allow host to send NM write commands,
                               route incoming nm-escape and service pin
                               to neuron */

    NSS_ACTS_AS_A_MIP,      /* Neuron is a standalone NSI or regular MIP:
                               Allow host to send NM write commands,
                               route incoming nm-escape to neuron, but
                               pass service pin to host */

    NSS_TRANSPARENT_MODE,   /* NSS engine resides on host:
                               Allow host to send NM write commands,
                               route incoming nm-escape and service pin
                               to host */

    NSS_NSI_MODE,           /* Neuron is a standalone NSI:
                               Block host from sending NM write commands,
                               route incoming nm-escape to neuron, but
                               pass service pin to host */

    NSS_PROXY_MODE,         /* SLTA use only:  The NSI acts as a proxy for the
                                disconnected NSS engine and respond to QueryNSS */

    /* add new modes above this line.  Max is 14. */
    NSS_MAX_MIP_MODES       /* if mode >= this, reset the current mode. */
} _NSSMipMode;

typedef struct NmNsMipEevars
{
    NetWord     pNsiStaticAttributes;
    NetWord     pNsiConfigAttributes;
    NetWord     pNsiRamAttributes;
    byte        flags;                      /* MV_xxx */
    byte		mipDeviceType;              /* MIPDEV_UNKNOWN in other release */
    byte        unused2;
    byte        unused3;
    byte        sizeofMipEevars;            /* = SIZEOF_MIP_EEVARS */
    byte        version;                    /* major, minor */
    byte        irqBits;
    byte        xcvrId;
} NmNsMipEevars;

#define MAXIMUM_L5MIP_RECEIVE_TRANSACTIONS 16
#define MAXIMUM_L5MIP_SICB_TAGS 16

typedef	struct
{
	struct LtSicb;
	byte data[MAX_APDU_SIZE-1];
} LtSicbFull;

typedef struct
{
	LtMsgIn*		m_pMsgIn;
	unsigned int	m_instance;
} LtMsgInLog;

class LtMipTag : public LtMsgTag
{
private:
	struct
	{
		LtSicb sicbHdrCode; // SICB Header
		char sicbCcData;  /* Completion events return the code 
				     		 and 1 byte of  data (for NV correlation).
				     		 LtSicb already has space for 1 byte.
				     		 Added space for one more */
	} sicb;

public:
	LtMipTag(int index, LtSicb* pSicb) : LtMsgTag(false, index)
	{
		memcpy(&sicb, pSicb, sizeof(sicb));
	}
	LtSicb* getSicb() { return &sicb.sicbHdrCode; }
};

class LtMipApp : public LtaBase
{
private:
	VxcSignal	m_signal;
    int			m_tidEvents;
	boolean		m_bInitialFlushState;
	LtNsaData	m_nsa;
	int			m_instance;
	LtMsgInLog	m_msgs[MAXIMUM_L5MIP_SICB_TAGS];
	LtSicb		m_winksicb;

	// NSI stub support
	int			m_currentNssMipMode;
	byte		m_myNssData[50];

	OpSignal	m_opSignal;			// Buffers signal strength data for most recent incoming SICB
	
protected:
	bool 		isBroadCastMessage;
	bool 		isNetworkVariableMessage;
	bool 		isApplicationMessage;

public:
	LtMipApp(int appIndex, LtLogicalChannel* pChannel, const char *persistencePath = null, int nAddressTableCount = 15);
	virtual ~LtMipApp();

	// The MIP application is designed to accept an SICB stream and to deliver an SICB stream.  Therefore,
	// its interface is very simple.  It consists of a send routine and receive routine.
	//

	// Call the start function first after creating the MIP application.
	void startMip(void);

	// Defined by the application for receipt of uplink SICBs.  The SICB contents must be copied by the 
	// function since it will be freed immediately upon return.  This function is called from a unique
	// thread context so it is OK to suspend (e.g. to send the SICB via a socket).
	virtual void receive(LtSicb* pSicb, OpSignal* pSsi=NULL) {};

	// Used by the application to send SICBs downlink.  The SICB contents are copied by the called function.
	// The SICB memory must be freed by the caller.  This routine can return LT_NO_RESOURCES if the 
	// maximum number of allowed outgoing SICBs has been hit.  It is up to the caller to suspend and retry
	// in this case.
	LtErrorType send(LtSicb* pSicb);

	// ******* Internal methods ******** //

	LtErrorType sendMsg(LtSicb* pSicb);
	LtErrorType sendNm(LtSicb* pSicb);
	void incomingSicb(int queue, int tag, LtApduIn* pApdu);
	void receiveSicb(LtSicb* pSicb, OpSignal* pSsi=NULL);

	LtErrorType handleNetworkManagement(LtApduIn* pApdu);
	void applicationEventIsPending(void);
	void run();
	void reportCommand(int command, int subcommand = 0, int len = 0, int d0 = 0, int d1 = 0);
	void reportCommandData(int command, int subcommand, int len, byte* pData);

    void flushCompletes();
    void reset();
	void sendServicePinMessage(void);

	// The following are the stack related callbacks and application functions.
	boolean isMip() { return true; }

	LtProgramId* getProgramId() { return NULL; }
	void wink() {}
	void offline() {}
	void online() {}
	void initializing() {}
    byte* getSiData(int* pLength) {*pLength = 0; return null; }

    void msgArrives(LtMsgIn* msg);
    void msgCompletes(LtMsgTag* tag, boolean success);
    void respArrives(LtMsgTag* tag, LtRespIn* response);

	// Support for tag to message mapping
	int getTag(LtMsgIn* pMsgIn);
	LtMsgIn* getMsgIn(int tag);

	// We don't support NV callbacks because we only support layer 5 MIP mode.  Layer 6 MIP
	// mode could be added if there ever is a reason for it.

    void servicePinPushed() {}
    void wakeupOccurred() {}
    boolean readMemory(int address, byte* data, int len) { return false; }
    boolean writeMemory(int address, byte* data, int len) { return false; }
	void persistenceLost(LtPersistenceLossReason reason) {}

	static int 	s_maxL5MIPTxTransactions;
	static int 	s_maxL5MIPRxTransactions;
};

#endif

