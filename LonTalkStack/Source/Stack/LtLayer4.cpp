//
// LtLayer4.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtLayer4.cpp#5 $
//

#include "LtaDefine.h"

#if FEATURE_INCLUDED(STANDALONE_MGMT)
#include "StdHelper.h"
#include "SnmInterface.h"
#endif
#include "LtStackInternal.h"
#include "LtMip.h"
#include "LonLink.h"
#if FEATURE_INCLUDED(STANDALONE_MGMT)
#include "LtProxyHistory.h"
#endif
#include "VxLayer.h"

//
// Tasks
//

// Convert static members for task entries to globals to make vxworks task 
// display more readable because of the shorter (unqualified) names

//int	VXLCDECL LtLayer4::startOutput( int stack, ... )
extern "C" int VXLCDECL L4OutputTask( int stack, ... )
{
	LtLayer4* pStack = (LtLayer4*) stack;
	pStack->processOutgoing();
	return 0;
}

//int	VXLCDECL LtLayer4::startInput( int stack, ... )
extern "C" int VXLCDECL L4InputTask( int stack, ... )
{
	LtLayer4* pStack = (LtLayer4*) stack;
	pStack->processIncoming();
	return 0;
}

//int	VXLCDECL LtLayer4::startTimeouts( int stack, ... )
extern "C" int VXLCDECL L4TimerTask( int stack, ... )
{
	LtLayer4* pStack = (LtLayer4*) stack;
	pStack->processTimeouts();
	return 0;
}

//
// Methods
//
LtLayer4::LtLayer4(int nRxTx, int nTxTx, 
                   int maxOutputPackets, 
                   int maxPriorityOutputPackets)
{
	m_pTxsTx = new LtTypedTxs<LtTransmitTx>(nTxTx);
	m_pTxsRx = new LtTypedTxs<LtReceiveTx>(nRxTx);
	m_nTxActive = 0;
	m_nTxLimit = TX_INITIAL_LIMIT;
	m_bLimitHit = false;
	m_bException = false;
    m_bReceivedSomething = false;
	m_bLastAdjustmentPositive = false;
    m_bUseLsEnhancedMode = false;
    m_bUseLsEnhacedModeOnly = false;

	m_nTxCount = 0;
	m_nUpperLimit = TX_LIMIT_MAX;
	memset(m_txStats, 0, sizeof(m_txStats));

	m_msgTimeouts = msgQCreate(50, sizeof(LtTx*), MSG_Q_FIFO);
	m_msgTxOut = msgQCreate(100, sizeof(LtMsgOutgoing), MSG_Q_FIFO);
	m_msgRxOut = msgQCreate(20, sizeof(LtMsgOutgoing), MSG_Q_FIFO);
	m_throttledMsgOut = msgQCreate(200, sizeof(LtMsgOutgoing), MSG_Q_FIFO);
	m_unthrottledMsgOut = msgQCreate(200, sizeof(LtMsgOutgoing), MSG_Q_FIFO);

	m_semMsg = semBCreate(SEM_Q_FIFO, SEM_EMPTY);

	m_semTx = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);

	// Start out in the flush state
	m_bCommIgnore = true;
	m_bReceiveAllBroadcasts = false;

	for (int j=0; j<LT_PRIORITY_TYPES; j++)
	{
        int maxPackets = (j) ? maxPriorityOutputPackets : maxOutputPackets;

        // We don't need many buffers to start, but if there are a lot of channels we
		// might need to end up adding a bunch to handle broadcasts (e.g. service pin).
		m_pktAlloc[j].init(LT_PKT_INFO_BUFFER_SIZE, 4, 10, maxPackets);

		// Make sure we have more message refs than bufs.  Message refs
		// are needed for cloning and we want to make sure we have more
		// message refs than bufs in case all bufs are in queue somewhere
		// and cloning needs to occur.

		m_pktAlloc[j].initMsgRefs(4, 10, 2*maxPackets);
	}

	m_bResetRequested = false;

	// Default life time for a transaction id tracking.  The total number
	// of targets that we can talk to is a function of the number of transmit
	// transactions and the lifetime.  Note that this life time is only used
	// for non-unique ID transactions.  Unique ID transactions use a fixed life time
	// of 8 seconds.
	m_txIdLifetime = 3000;

	m_taskIdInput = vxlTaskSpawn("L4Input", 
                              LT_L4INPUT_TASK_PRIORITY, 0, 
                              LT_L4INPUT_TASK_STACK_SIZE, L4InputTask, 
	  					      (int)this, 0,0,0,0, 0,0,0,0,0);
	// Back-up on the output side can be problematic so give output thread
	// higher priority over input thread (see notifyOutgoing).
	m_taskIdOutput = vxlTaskSpawn("L4Output", 
                                LT_L4OUTPUT_TASK_PRIORITY, 0, 
                                LT_L4OUTPUT_TASK_STACK_SIZE, L4OutputTask, 
	  					       (int)this, 0,0,0,0, 0,0,0,0,0);
	m_taskIdTimeouts = vxlTaskSpawn("L4Timer", 
                                 LT_L4TIMER_TASK_PRIORITY, 0, 
                                 LT_L4TIMER_TASK_STACK_SIZE, 
                                 L4TimerTask,
	  					         (int)this, 0,0,0,0, 0,0,0,0,0);

	registerTask(m_taskIdInput, NULL, NULL);
	registerTask(m_taskIdOutput, NULL, m_semMsg);
	registerTask(m_taskIdTimeouts, m_msgTimeouts, NULL);

    m_bOmaSupported = true;

    m_bNmVersionOveridden = FALSE;
    m_nmVersionOverride = 0;
    m_nmCapOveride = 0;
}

LtLayer4::~LtLayer4()
{
	shutdown();

	delete m_pTxsTx;
	delete m_pTxsRx;

	msgQDelete(m_msgTimeouts);
	msgQDelete(m_msgTxOut);
	msgQDelete(m_msgRxOut);
	msgQDelete(m_throttledMsgOut);
	msgQDelete(m_unthrottledMsgOut);
	m_msgTimeouts = null;
	semDelete(m_semTx);
	semDelete(m_semMsg);
	m_txSources.removeAllElements(true);
}

void LtLayer4::shutdown() 
{ 
	waitForTasksToShutdown();

	// Clean up incoming packets
    LtMsgRef* pMsgRef;
	while (m_pkts.receive(&pMsgRef, NO_WAIT) == OK) ((LtPktInfo*) pMsgRef)->release();

    // Clean up outgoing messages
	LtMsgOutgoing msg;
	while (msgQReceive(m_msgTxOut, (char*) &msg, sizeof(msg), NO_WAIT) == sizeof(msg) ||
		   msgQReceive(m_msgRxOut, (char*) &msg, sizeof(msg), NO_WAIT) == sizeof(msg) ||
		   msgQReceive(m_throttledMsgOut, (char*) &msg, sizeof(msg), NO_WAIT) == sizeof(msg) ||
		   msgQReceive(m_unthrottledMsgOut, (char*) &msg, sizeof(msg), NO_WAIT) == sizeof(msg))
	{
		if (msg.getType() == LT_APDU_MSG)
		{
	        LtApduOut* pApdu = (LtApduOut*) msg.getObject();
			delete pApdu;
		}
	}

	// Wait for packet buffers to come back from the engine or other clients
	for (int j=0; j<LT_PRIORITY_TYPES; j++)
	{
		while (!m_pktAlloc[j].allItemsReturned())
		{
			taskDelay(msToTicks(50));
		}
	}
}

void LtLayer4::notifyExpiration(LtTx* pTx)
{
	// Check for existence of queue before proceeding.  During shutdown, queue can be deleted
	// before timers are all deleted.
	if (m_msgTimeouts)
	{
		// Save in local since passing &pTx directly crashes in release mode!?!?!
		LtTx* pTxSave = pTx;

		// Called by ISR.  Must use NO_WAIT.
		// Set expiration occurred flag before sending message.  Necessary on Windows
		// platform where this is not an ISR and thus other task may run.  Thus, we
		// clear the flag on failure.  There should not be a window where the flag
		// is examined while this routine executes.
		pTx->setExpirationOccurred(true);
		if (msgQSend(m_msgTimeouts, (char*) &pTxSave, sizeof(pTxSave), NO_WAIT, MSG_PRI_NORMAL) == ERROR)
		{
			pTx->setExpirationOccurred(false);
			// No more room in the queue!  Let's run the timer
			// again so that we try to queue again later.
			pTx->startTimer(msToTicks(20));
		}
	}
}

void LtLayer4::notifyOutgoing(LtMsgOutType type, LtTx* pTx)
{
    LtMsgOutgoing msg(type, pTx);
	MSG_Q_ID mid = m_msgTxOut;

	if (!pTx->getOnTxQ())
	{
		if (pTx->getReceive())
		{
			mid = m_msgRxOut;
		}

		if (msgQSend(mid, (char*) &msg, sizeof(msg), NO_WAIT, MSG_PRI_NORMAL) == ERROR)
		{
			if (!pTx->getReceive())
			{
				printf("notifyOutgoing: m_msgTxOut full\n");
			}
		}
		else
		{
			pTx->setOnTxQ(true);
		}
		semGive(m_semMsg);
	}
}

void LtLayer4::processIncoming()
{
	// Process next incoming message.  
    while (!taskShutdown())
    {
        LtMsgRef* pMsgRef;
		// We don't "wait forever" because we need to periodically wake up to check for shutdown
	    if (m_pkts.receive(&pMsgRef, 250) == OK)
	    {
            LtPktInfo *pPkt = (LtPktInfo*) pMsgRef;
            semTake(m_semTx, WAIT_FOREVER);
#ifdef __ECHELON_TRACE
			printf("Receive service = %d\n", pPkt->getServiceType());
#endif
			LtErrorType err = receive(pPkt);
			if (err != LT_NO_ERROR)
			{
                getStack()->errorLog(err);
            }
			pPkt->release();
            semGive(m_semTx);
        }
	}
}

LtPktInfo* LtLayer4::allocatePacket(boolean bPriority)
{
	// This function keeps trying to allocate a packet buffer until
	// it succeeds.
	LtPktInfo* pPkt;
	while ((pPkt = m_pktAlloc[bPriority].allocPacket()) == null)
	{
		semGive(m_semTx);
		taskDelay(msToTicks(20));
		semTake(m_semTx, WAIT_FOREVER);
	}
	pPkt->setMessageData(pPkt->getBlock(), pPkt->getBlockSize(), pPkt);
    pPkt->setVersion(LT_L2_PKT_VER_LS_LEGACY_MODE);
    pPkt->setAuth(false);
    pPkt->setAltPath(false);
    pPkt->setPriority(bPriority);
    pPkt->setDeltaBacklog(0);
	return pPkt;
}

void LtLayer4::send(LtApduOut* pApdu, boolean wait, boolean throttle)
{
    LtMsgOutgoing msg(LT_APDU_MSG, pApdu);
    
    // EPR 19057 Allow some messages to be sent by the application that are not
    // subject to throttling.
    MSG_Q_ID *pMsgQue = (throttle) ? &m_throttledMsgOut : &m_unthrottledMsgOut;

    if (msgQSend(*pMsgQue, (char*) &msg, sizeof(msg), wait ? WAIT_FOREVER : NO_WAIT, MSG_PRI_NORMAL) == ERROR)
	{
        getStack()->completionEvent(pApdu, false);
	}
	else
	{
		semGive(m_semMsg);
	}
}

void LtLayer4::setResetRequested()
{
	LtMsgOutgoing msg(LT_RESET_MSG, null);
	// This message really is just used to force the outgoing thread to be 
	// scheduled.
	m_bResetRequested = true;
	msgQSend(m_msgTxOut, (char*) &msg, sizeof(msg), WAIT_FOREVER, MSG_PRI_URGENT);
	semGive(m_semMsg);
}

void LtLayer4::processOutgoing()
{
	// Process next outgoing message.
	while (!taskShutdown())
	{
        LtMsgOutgoing msg;
		if (msgQReceive(m_msgTxOut, (char*) &msg, sizeof(msg), NO_WAIT) != sizeof(msg) &&
			msgQReceive(m_msgRxOut, (char*) &msg, sizeof(msg), NO_WAIT) != sizeof(msg) &&
			msgQReceive(m_unthrottledMsgOut, (char*) &msg, sizeof(msg), NO_WAIT) != sizeof(msg) &&
			(deferTx() ||
			 msgQReceive(m_throttledMsgOut, (char*) &msg, sizeof(msg), NO_WAIT) != sizeof(msg)))
		{
			semTake(m_semMsg, WAIT_FOREVER);
		}
		else
		{
			semTake(m_semTx, WAIT_FOREVER);

			if (m_bResetRequested)
			{
				m_bResetRequested = false;
				getStack()->localReset();
			}

            LtPktInfo* pPkt = null;
			LtErrorType err = LT_NO_ERROR;

            switch (msg.getType())
            {
                case LT_TX_MSG:
                {
                    LtTx* pTx = (LtTx*) msg.getObject();
					pTx->setOnTxQ(false);
                    pPkt = allocatePacket(pTx->getPriority());
				    err = handleTxSend(pTx, pPkt);
                    break;
                }
                case LT_APDU_MSG:
                {
                    LtApduOut* pApdu = (LtApduOut*) msg.getObject();
                    pPkt = allocatePacket(pApdu->getPriority());
                    if (pApdu->getCode() == LT_SERVICE_PIN)
                    {
                        // Service pin messages always go out using legacy mode
                        pPkt->setVersion(LT_L2_PKT_VER_LS_LEGACY_MODE);
                    }
#ifdef __ECHELON_TRACE
					printf("Send APDU code = %02X (%d)\n", pApdu->getCode(), pApdu->getCode());
#endif
		            err = construct(pApdu, pPkt);
                    break;
                }
				case LT_RESET_MSG:
				{
				}
			}
			if (err != LT_NO_ERROR)
			{
	            getStack()->errorLog(err);
				pPkt->release();
			}
			else if (pPkt != null)
			{
				if (pPkt->getServiceType() == LT_UNKNOWN)
				{
					// No packet data - turnaround perhaps?
					pPkt->release();
				}
				else
				{
#ifdef __ECHELON_TRACE
					printf("sendPacket, service type=%d\n", pPkt->getServiceType());
#endif
					sendPacket(pPkt);
				}
            }

			// When flush is implemented, need to check here for completion of
			// TXs and outgoing messages if flushPending and then report it to
			// the app via noOutgoingActivity.

			semGive(m_semTx);
		}
	}
}

void LtLayer4::processTimeouts()
{
	while (!taskShutdown())
	{
		LtTx* pTx;
		// Timeouts are delivered via message queues. 
		if (msgQReceive(m_msgTimeouts, (char*) &pTx, sizeof(pTx), WAIT_FOREVER) == sizeof(pTx))
		{
			semTake(m_semTx, WAIT_FOREVER);
			expiration(pTx);
			semGive(m_semTx);
		}
	}
}

void LtLayer4::queuePacket(LtPktInfo* pPkt)
{
	m_pkts.send(pPkt);
}

LtErrorType LtLayer4::receive(LtPktInfo* pPkt)
{
	LtErrorType err = LT_NO_ERROR;
	boolean bReceive = true;

	getStack()->getNetworkStats()->bump(LT_LAYER3_RECEIVE);
	if (getCommIgnore())
	{
		bReceive = false;
	}
	else if (getStack()->isLayer2())
	{
		// Layer 2 MIPs send the packet directly to the app.
		LtApduIn* pApdu = getStack()->getApdu();
		if (pApdu != null) 
		{
			byte* pData;
			int nLen = pPkt->getMessageData(&pData);
			pApdu->setCode(0);
			pApdu->setData(pData, 0, nLen);
			pApdu->setLayer2Info(pPkt);
			getStack()->receive(pApdu);
		}
		bReceive = false;
	}
	// Engine doesn't track unconfigured/configured state for each client
	// so if a configured address client gets a message when unconfigured,
	// we pitch it here.
	else if (getStack()->unconfigured())
	{
		if (pPkt->getAddressFormat() != LT_AF_UNIQUE_ID &&
			pPkt->getAddressFormat() != LT_AF_BROADCAST)
		{
			bReceive = false;
		}
	}
	else
	{
		// Configured - toss broadcasts received on the catchall client. 
		if (pPkt->getAddressFormat() == LT_AF_BROADCAST)
		{
			LtUniqueIdClient* pClient = (LtUniqueIdClient*) pPkt->getDestClient();
			if (pClient->getIndex() == FLEX_DOMAIN_INDEX)
			{
				// Exception - allow broadcasts if receive all broadcasts is true
				bReceive = getReceiveAllBroadcasts();
			}
		}
	}

    if (bReceive && m_bUseLsEnhacedModeOnly && pPkt->getVersion() == LT_L2_PKT_VER_LS_LEGACY_MODE)
    {
        // We are emulating a device that only supports enhanced mode.  Toss the packet.  Unless its a
        // service pin message
        if (pPkt->getAddressFormat() != LT_AF_BROADCAST || pPkt->getServiceType() != LT_UNACKD ||
            *pPkt->getData() != LT_SERVICE_PIN)
        {
            bReceive = false;
        }
    }

	if (bReceive)
	{
		switch(pPkt->getServiceType())
		{
			case LT_UNACKD:
			{
				LtApduIn* pApdu = getStack()->getApdu();
				if (pApdu != null) 
				{
					LtRefId rid;
					pApdu->init(getStack(), pPkt, rid);
					err = getStack()->LtLayer6::receive(pApdu, null);
				}
				break;
			}
			case LT_UNACKD_RPT:
			case LT_ACKD:
			case LT_REMINDER_ACKD:
			case LT_REMINDER_REQUEST:
			case LT_REMMSG_ACKD:
			case LT_REMMSG_REQUEST:
			case LT_REQUEST:
				err = receiveMsg(pPkt);
				break;
			case LT_RESPONSE:
			case LT_ACK:
				err = receiveAck(pPkt);
				break;
			case LT_CHALLENGE_OMA:
                if (!omaSupported())
                {
                    break;
                }
                // Fall through if OMA is supported
			case LT_CHALLENGE:
				err = receiveChallenge(pPkt);
				break;
			case LT_REPLY_OMA:
                if (!omaSupported())
                {
                    break;
                }
                // Fall through if OMA is supported
			case LT_REPLY:
				err = receiveReply(pPkt);
				break;
			default:
				break;	// nothing
		}
	}
	return err;
}

LtErrorType LtLayer4::receiveChallenge(LtPktInfo* pPkt) 
{
	LtErrorType err = LT_NO_ERROR;
    // Validate that the challenge corresponds to a valid
    // outgoing transaction.
	if (pPkt->getAddressFormat() != LT_AF_SUBNET_NODE && pPkt->getAddressFormat() != LT_AF_GROUP_ACK)
	{
		// Address format is assumed by getTxSource in getTransmitTx.
		err = LT_INVALID_PARAMETER;
	}
	else
	{
		flipAuthAddress(pPkt);
		LtTransmitTx* pTx = getTransmitTx(pPkt);
		if (pTx == null || !pTx->active()) 
		{
			registerTransactionEvent(LT_TX_STAT_LATE_ACK);
			getStack()->getNetworkStats()->bump(LT_LATE_ACKS);
		}
		else 
		{
			byte* pKey = NULL;
            boolean isOma = pPkt->getServiceType() == LT_CHALLENGE_OMA;
			LtDomainConfiguration* pDomainConfiguration = NULL;

			// Access of transaction means extend timer (i.e., challenges received
			// extends expiration).
			pTx->setExpiration();

			LtApduOut* pApduOut = (LtApduOut*) pTx->getApdu();
			if (getDomainConfiguration(pApduOut->getDomainIndex(), &pDomainConfiguration) == LT_NO_ERROR)
			{
				pKey = pDomainConfiguration->getKey();
			}
			if (pApduOut->getOverride()->getOptions().overrideAuthKey())
			{
				pKey = pApduOut->getOverride()->getAuthKey();
			}
			if (pKey != NULL)
			{
				LtPktInfo* pReply = allocatePacket(pPkt->getPriority());
				if (pReply)
				{
					byte challenge[LT_CHALLENGE_LENGTH];
					pPkt->getChallengeReply(challenge);
					*(LtPktData*)pReply = *(LtPktData*)pPkt;

                    // Normally, reply to challenge using subnet node addressing.  However, if the 
                    // source node of the challeng is 0, this must be flex domain authentication,
                    // so the original neuron ID should be used.  Note that by the time we get here,
                    // the source and dest of been switched, so this test is for the destination node,
                    // even though its really for the source node of the original challenge.
                    if ((pReply->getDestNode() == 0) && 
                        (pTx->getAddressFormat() == LT_AF_UNIQUE_ID))
                    {   
                        pReply->setAddressFormat(LT_AF_UNIQUE_ID);
                        pReply->setUniqueId(pTx->getUniqueId());
                    }
                    else
                    {
					    pReply->setAddressFormat(LT_AF_SUBNET_NODE);
                    }
					pTx->encrypt(challenge, pDomainConfiguration, pKey, isOma);
					pReply->setChallengeReply(challenge);
                    pReply->setServiceType(isOma ? LT_REPLY_OMA : LT_REPLY);
					sendPacket(pReply);
				}
			}
		}
    }
	return err;
}

void LtLayer4::sendPacket(LtPktInfo* pPkt)
{
	LtLreClient* pSourceClient = getStack()->getMainClient();
	// The main client might not be initialized yet!  Dump the packet in this case.
	if (pSourceClient == null)
	{
		pPkt->release();
	}
	else
	{
		LtDeviceStack* pStack = getStack();
        LtErrorType errorType;
		pStack->getNetworkStats()->bump(LT_LAYER3_TRANSMIT);
        errorType = pPkt->buildPkt(pStack->isLayer2());
		if (errorType == LT_NO_ERROR)
        {
		    pStack->getLre()->routePacket(pPkt->getPriority(), pSourceClient, pPkt);
        }
        else
        {
            pStack->putErrorLog(errorType);
            pPkt->release();
        }
	}
}

void LtLayer4::halt() 
{
	setCommIgnore(true);
}

void LtLayer4::resume() 
{
	setCommIgnore(false);
}

//
// Public Member Functions
//


void LtLayer4::resetTx()
{
	// During normal resets, we don't reset the tx records.  At a minimum, we don't 
	// want to lose the tx tracking on the transmit side nor lose duplicate detection
	// on the receive side.  However, we may want to implement a special clean up mode
	// where all outstanding txs are forced to fail immediately.  This is for future
	// consideration.
    m_pTxsRx->reset();
    m_pTxsTx->reset();
}

void LtLayer4::reset() 
{
	LtVectorPos pos;
	LtTxSource* pTxSource;

	while (m_txSources.getElement(pos, &pTxSource))
	{
		pTxSource->initTxIds();
	}
}

void LtLayer4::terminate() 
{
}

LtErrorType LtLayer4::handleTxSend(LtTx* pTx, LtPktInfo* pPkt)
{
	LtErrorType err = LT_NO_ERROR;
    if (pTx->getReceive())
	{
        LtReceiveTx* pRtx = (LtReceiveTx*) pTx;
        err = sendReceiveTx(pRtx, pPkt);
    }
	else
	{
        LtTransmitTx* pTtx = (LtTransmitTx*) pTx;
        err = sendTransmitTx(pTtx, pPkt);
    }
	return err;
}

void LtLayer4::prepareForSend(LtTx* pTx) 
{
	notifyOutgoing(LT_TX_MSG, pTx);
}

LtTransmitTx* LtLayer4::getTransmitTx(LtPktInfo* pPkt) 
{
    // We can either map by tx id (if the transaction has
    // ambiguous addressing) or by tx key.  Do the former
    // first since it is more efficient.
	LtTransmitTx* pTx = null;
	LtTxSource* pTxSource = getTxSource(pPkt);

	if (pTxSource != null)
	{
		pTx = pTxSource->m_pTxById[LtMisc::getPriorityType(pPkt->getPriority())][pPkt->getTxNumber()];
		if (pTx == null || !pTx->ambiguousAddress()) 
		{
			pTx = m_pTxsTx->get(pPkt);
			if (pTx != null && pTx->getTxNumber() != pPkt->getTxNumber())
			{
				pTx = null;
			}
		}
	}
    return pTx;
}

LtErrorType LtLayer4::constructResponse(LtApduOut* pApdu, LtPktInfo* pPkt) 
{
	LtErrorType err = LT_NO_MESSAGE;
    LtTx* pTx = refToTx(pApdu->getRefId());
	if (pTx != null) 
	{
		if (pTx->valid()) 
		{
	        if (pApdu->getTurnaround())
		    {
				// We don't have a way to send turnaround null responses so just don't send
				// it.
				if (!pApdu->getNullResponse())
				{
					LtTransmitTx* pTxTx = (LtTransmitTx*) pTx;
					err = buildMessage2(pTxTx, pApdu, pPkt);
				}
			}
			else
			{
				LtReceiveTx* pRxTx = (LtReceiveTx*) pTx;
				err = pRxTx->registerResponse(pApdu->getCodeAndData(), pApdu->getLength(), 
                                              pApdu->getNullResponse(), pApdu->getRespondOnFlexDomain());
				if (err == LT_NO_ERROR)
				{
					err = sendReceiveTx(pRxTx, pPkt);
				}
			}
		} 
	}
	delete pApdu;
	return err;
}


LtReceiveTx* LtLayer4::getReceiveTx(LtPktInfo* pPkt) 
{
    LtReceiveTx* pTx = m_pTxsRx->get(pPkt);
    if (pTx != null) 
    {
        // Check for duplicate
        if (!pTx->active() || pTx->getTxNumber() != pPkt->getTxNumber())
        {
			pTx->setTxNumber(pPkt->getTxNumber());
            pTx->init(pPkt);
        } 
        else if (pPkt->getEnclPdu() != LT_AUTHPDU &&
				 pPkt->getEnclPdu() != pTx->getEnclPdu())
		{
			// New message looks like dup but is of different PDU type!  Treat
			// as non-duplicate.
			pTx->init(pPkt);
		}
		else
        {
			// For Neuron ID addressing, restart the receive timer.  This is done because
			// we don't know what the true receive timer is for Neuron ID since it is fixed
			// at 8 seconds.  By restarting it, we emulate the worst case which is that the
			// last of N retries is the one received.  The following prompted this change:
			// We want to reject the first NM message after a reset with pending updates.  
			// However, if the NM is retrying at a slow rate with lots of retries, the first
			// message might not get rejected due to duplicate detection failure.
			if (pPkt->getAddressFormat() == LT_AF_UNIQUE_ID)
			{
				pTx->restartTimer();
			}
            pTx->markAsDuplicate();
        }
    } 
    else
    {
        // Create a record with this key. 
        pTx = m_pTxsRx->alloc(pPkt);
		if (pTx != null)
		{
			pTx->init(pPkt);
		}
    } 
    if (pTx != null && pPkt->getAltPath()) 
    {
        pTx->setAltPath(true);
    }
    return pTx;
}

int LtLayer4::nextTxId(int id) 
{
    return (id >= sizeOfTxSpace() ? 1 : (id + 1));
}

LtErrorType LtLayer4::getNextTx(LtTransmitTx** ppTx, LtApduOut* pApdu, LtPktInfo* pPkt) 
{
	LtErrorType err = LT_NO_ERROR;
	LtTxSource* pTxSource = getTxSource(pApdu);
	LtTransmitTx* pTx = null;
	boolean bPriority = pApdu->getPriority();
	int txId = 0;	// init to remove warning

	if (pTxSource == null)
	{
		err = LT_INVALID_PARAMETER;
	}

	if (err == LT_NO_ERROR)
	{
		// Given the packet address, create a key to find the transmit tx associated
		// with this address.  If none, create one and add it.  
		pTx = *ppTx;

		if (pApdu->isUnsafe())
		{
			if (pApdu->getServiceType() == LT_ACKD ||
				pApdu->getServiceType() == LT_REQUEST)
			{
				pTxSource->m_lastUnsafeOperation = System::currentTimeMillis();
			}
		}
	}

    if (pTx == null) 
    {
        pTx = m_pTxsTx->get(pPkt);
        if (pTx != null && pTx->inUse()) 
        {
            // User is trying to send another message to a destination which has an
            // in-use transaction.  In this case we queue up the PDU to be sent 
            // at some future time (i.e., when the transaction completes).
            pTx->addPending(pApdu);
			err = LT_MESSAGE_BLOCKED;
        }
    }

	if (err == LT_NO_ERROR)
	{
		txId = pTxSource->m_curTxId[LtMisc::getPriorityType(bPriority)];
		long currentTime = System::currentTimeMillis();
		bool validTxIdFound = false;
		boolean newTx = false;
		int minWait = LT_MAX_TIMEOUT;

        if (txId > sizeOfTxSpace())
        {
            // This can happen right after changing from enhanced to legacy mode.
            txId = 1;
        }

		if (pTx == null) 
		{
			// Never been used before (or timed out)
			pTx = m_pTxsTx->alloc(pPkt);
			newTx = true;
		} 
		else
		{
			pTx->setEnclPdu(pPkt->getEnclPdu());
			pTx->setServiceType(pPkt->getServiceType());
            pTx->setAltPath(pApdu->getAlternatePath());
            if (pApdu->getAddressType() == LT_AT_UNIQUE_ID)
            {   // Usually the TX already has destination addressing (since that's what
                // we matched on).  However, unique ID addressing matches even if the
                // destination subnet does not - so better fill it in.
		        pTx->setDestSubnet(pApdu->getSubnet());
            }
		}

		if (pTx == null)
		{
			// We could improve this.  Need to put item back onto queue? 
			// Howabout if we only dequeue new outgoing transactions if there is
			// an available transaction?  Implies if there are none that all messages, 
			// both tx and non-tx back up.  Not so great.
			
			// For now, generate a failure completion event.
			// Dump tx data stats for grins
			// m_pTxsTx->stats();
	        getStack()->completionEvent(pApdu, false);
			err = LT_NO_RESOURCES;
		}
		else
		{
			int possibleTxId = 0;

			pTx->init(pApdu);
			pTx->setTxSource(pTxSource);
            pTx->setUseEnhancedMode(m_bUseLsEnhancedMode);

#if INCLUDE_STANDALONE_MGMT
			bool inOverride = proxyHistory.getTxId(pApdu, txId, validTxIdFound, minWait);
#else
            bool inOverride = false;
#endif                        

			for (int i = 0; !inOverride && i < sizeOfTxSpace(); i++) 
			{
				// Go through the transaction space looking for a usable 
				// txid.  If none are usable, use that with the minimum wait time.
				if (newTx || pTx->txIdOk(txId))
				{
					// Transaction id not same as that in target so OK to use.
                    // Don't ever track txids for turnarounds since it doesn't apply.
					if (pTxSource->m_lastUnsafeOperation == 0 ||
						currentTime - pTxSource->m_lastUnsafeOperation > 6000 ||
                        pApdu->getTurnaround())
					{
						validTxIdFound = true;
						break;
					}
                    else
					{
 						// A potentially unsafe operation is pending so check for tx conflicts.
						// Basically, if a Unique ID or broadcast message was sent with an ackd
						// or req service type recently, then a response/ack from said message
						// may be ambiguous.  So, only use the tx id if there is little chance
						// that such a late response might be outstanding.  This can substantially
						// reduce performance if long tx timers are being used.
						// A performance improvement here would be to only throttle when the
						// potential unsafe operation was to a different node.  Currently don't
						// have this information.  Could do this easily by storing tx pointer
						// rather than tx id.
                        if (pTxSource->m_txIdExpiration[LtMisc::getPriorityType(bPriority)][txId].pTx == pTx)
                        {
                            // This txID was last used with the same address - so it is safe to reuse it.
							validTxIdFound = true;
							break;                    
                        }
                        else
                        {
						    int expiration = (int) 
                                (pTxSource->m_txIdExpiration[LtMisc::getPriorityType(bPriority)][txId].expiration - currentTime);

						    if (expiration <= 0) {
							    validTxIdFound = true;
							    break;                    
						    } 
						    else if (expiration < minWait) 
						    {
							    minWait = expiration;
							    possibleTxId = txId;
						    }
                        }
					}
				}
				txId = nextTxId(txId);
			}
		
			if (!validTxIdFound) 
			{
				pTx->defer(pApdu, minWait);
				txId = possibleTxId;
				err = LT_MESSAGE_DEFERRED;
			}
		}
	}

    if (err == LT_NO_ERROR)
    {
	    if (!pApdu->getTurnaroundOnly())
	    {
		    pTxSource->m_curTxId[LtMisc::getPriorityType(bPriority)] = nextTxId(txId);
	    }
        startTx();
	    pTx->setApdu(pApdu);
	    pTx->setTxNumber(txId);
    }

	*ppTx = pTx;
	return err;
}

void LtLayer4::setTransactionId(LtApduOut* pApdu, LtPktInfo* pPkt, LtTransmitTx* pTx) 
{
	int txId = pTx->getTxNumber();
    pPkt->setTxNumber(txId);
    // No need to time unackd_rpt or unbound messages.
    if (pApdu->getServiceType() != LT_UNACKD_RPT &&
        pApdu->isBound())
    {
		LtTxSource* pTxSource = pTx->getTxSource();
        pTxSource->m_txIdExpiration[LtMisc::getPriorityType(pApdu->getPriority())][txId].expiration = 
			System::currentTimeMillis() +
            (unsigned int) (pApdu->getTxTimer() * TX_WAIT_MUL / TX_WAIT_DIV);
        pTxSource->m_txIdExpiration[LtMisc::getPriorityType(pApdu->getPriority())][txId].pTx = pTx;
		pTxSource->m_pTxById[LtMisc::getPriorityType(pApdu->getPriority())][txId] = pTx;
    }
}

LtErrorType LtLayer4::sendReceiveTx(LtReceiveTx* pTx, LtPktInfo* pPkt) 
{
	LtErrorType err = LT_NO_ERROR;
	// It might be faster to copy the whole LtPktData from the tx to the pkt but
	// need to take care about fields which shouldn't be copied (e.g., backlog, auth).
	pPkt->setDomain(pTx->getDomain());
	pPkt->setPriority(pTx->getPriority());
	pPkt->setAltPath(pTx->getAltPath());
    pPkt->setServiceType(pTx->getPendingType());
	pPkt->setTxNumber(pTx->getTxNumber());
    pPkt->setUseLsEnhancedMode(pTx->getUseEnhancedMode());
	pPkt->setOrigFmt(pTx->getOrigFmt());
    pPkt->setAddressFormat((pTx->getAddressFormat()==LT_AF_GROUP) ? LT_AF_GROUP_ACK : LT_AF_SUBNET_NODE);
	pPkt->setDestMember(pTx->getDestMember());
	pPkt->setDestGroup(pTx->getDestGroup());
	
	// Set the destination to be the original source subnet/node.
	pPkt->setDestSubnet(pTx->getSourceNode().getSubnet());
	pPkt->setDestNode(pTx->getSourceNode().getNode());

	// We either want to use the flex domain, the default source/node of the incoming domain 
    // or, for subnet/node that on which we received the message (to accommodate alternate 
    // address).
	int nDomainIndex = pTx->getDomainIndex();
    if (nDomainIndex == FLEX_DOMAIN_INDEX)
    {
		pPkt->getSourceNode().set(0, 0);
    }
	else if (pTx->getOrigFmt() == LT_AF_SUBNET_NODE)
	{
		pPkt->getSourceNode().set(pTx->getDestSubnet(), pTx->getDestNode());
	}
	else
	{
		LtDomainConfiguration *pDom;
		err = getDomainConfiguration(nDomainIndex, &pDom);
        pPkt->getSourceNode() = pDom->getSubnetNode();
	}

    switch (pPkt->getServiceType())
    {
    case LT_CHALLENGE:
    case LT_CHALLENGE_OMA:
        // Build and send challenge.
        byte challenge[LT_CHALLENGE_LENGTH];
        pTx->getChallenge(challenge);
        pPkt->setChallengeReply(challenge);
        break;
    case LT_RESPONSE:
	{
		byte* pResponse;
		int nLen = pTx->getResponse(pResponse);
		pPkt->setData(pResponse, nLen);
        break;
	}
    case LT_ACK:
        break;
    default:
        err = LT_INVALID_PARAMETER;
    }
	return err;
}

LtErrorType LtLayer4::buildMessage2(LtTransmitTx* pTx, LtApduOut* pApdu, LtPktInfo* pPkt) 
{
	LtErrorType err = LT_NO_ERROR;
    boolean bound = pApdu->isBoundExternally();
    LtServiceType serviceType = pApdu->getServiceType();
    // Generate success completion event if unbound or unackd
    boolean completion = !bound || serviceType == LT_UNACKD;

    if (pTx != NULL)
    {
        pPkt->setUseLsEnhancedMode(pTx->getUseEnhancedMode() || m_bUseLsEnhacedModeOnly);
    }
    else
    {
        pPkt->setUseLsEnhancedMode(m_bUseLsEnhancedMode);
    }
    if (pApdu->getTurnaround()) 
    {
        // Try to deliver the message to the app.  Note that turnaround messages do 
		// not get throttled.  There is no retry mechanism for turnarounds so we can't
		// afford to "lose" messages.  Flow control for node itself is assumed to be 
		// the node's problem.
        LtApduIn* pApduIn = getStack()->getApdu(pApdu->getResponse(), true);
        if (pTx != null) 
		{
            // Always generate completion via tx.
            completion = false;
            if (!pApdu->getResponse())
            {
                pTx->setTurnaround(true);
            }
        }
        if (pApduIn != null) 
		{
			LtApduOut* pRefApdu = pApdu;
            if (pTx != null) 
			{
                if (!pApdu->isRequest())
                {
                    // Clear turnaround to allow tx completion to occur and to
                    // prohibit further retries.
                    pApdu->setTurnaround(false);
                    pTx->setTurnaround(false);
					pRefApdu = pTx->getApdu();
                }
				// Response uses the ref id of the original request while the echoed 
				// request uses the TX ref id so we can look up the tx when the response is returned.
			    pApduIn->setRefId(pApdu->getResponse()?pRefApdu->getRefId():pTx->getRefId());
            }
            // Can deliver message
			// Always mark local messages as authenticated so that they 
			// are never denied.
			pApduIn->setAuthenticated(true);
            pApduIn->setTurnaroundAddress();
            pApduIn->setServiceType(serviceType);
            pApduIn->setTurnaround(true);
            pApduIn->setCodeAndData(pApdu);
			pApduIn->setDomainConfiguration(pRefApdu->getDomainConfiguration());
            err = getStack()->LtLayer6::receive(pApduIn, pRefApdu);
        }
    }

    if (!bound) 
    {
		// Nothing to send.  Mark packet accordingly
		pPkt->setServiceType(LT_UNKNOWN);
	}
    
    if (pTx != null && (completion || pTx->isComplete(false))) 
    {
        // Deliver success completion event and terminate tx. This occurs
        // following last unackd rpt message or for unbound messages or
        // for turnaround completion.
        completeTx(pTx, true);
    } 
    else if (completion) 
    {
        getStack()->completionEvent(pApdu, true);
    }
	return err;
}

LtErrorType LtLayer4::buildMessage(LtTransmitTx* pTx, LtApduOut* pApdu, LtPktInfo* pPkt) 
{
    if (!pTx->buildReminderMsg(pApdu, pPkt)) 
    {
        pTx->send(pApdu, pPkt);
        pPkt->setAuth(pApdu->getAuthenticated());
    } 
    else 
    {
        // Send message next
        pTx->setPendingType(pTx->isSession() ? LT_REQUEST : LT_ACKD);
        prepareForSend(pTx);
    }
    pTx->altPath(pPkt);
    setTransactionId(pApdu, pPkt, pTx);
    LtErrorType err = buildMessage2(pTx, pApdu, pPkt);
	// Don't mess with transactions that have already been retired.
	if (pTx->active() && !pTx->getDeletionRequired())
	{
		pTx->setExpiration();
	}
	return err;
}

LtTxSource* LtLayer4::getTxSource(LtPktInfo* pPkt)
{
	// This is used for mapping acks, responses and challenges to a 
	// source based on domain index and subnet/node index.
	LtTxSource* pTxSource = null;
	// Assumes ack address format is subnet/node.  Must be validated by caller.
	LtSubnetNodeClient* pClient = (LtSubnetNodeClient*) pPkt->getDestClient();
	int domainIndex = pClient->getIndex();
	LtDomainConfiguration* pDom = (LtDomainConfiguration*) pClient;
	int subnetNodeIndex;
	// Warning: this method must be called only after flipping has occurred (see receiveAck).
	if (pDom->getSubnetNodeIndex(pPkt->getSourceNode(), subnetNodeIndex) == LT_NO_ERROR)
	{
		pTxSource = m_txSources.elementAt(TX_SOURCE_INDEX(domainIndex, subnetNodeIndex));
	}

	return pTxSource;
}

LtTxSource*	LtLayer4::getTxSource(LtApduOut* pApduOut)
{
	int index = pApduOut->getDomainIndex();
	if (index == LT_FLEX_DOMAIN || index > 65535)
	{
		// Just use the first entry
		index = 0;
	}
	else
	{
		index = TX_SOURCE_INDEX(index, pApduOut->getSubnetNodeIndex());
	}
	LtTxSource* pTxSource = m_txSources.elementAt(index);

	if (pTxSource == null)
	{
		pTxSource = new LtTxSource();
		m_txSources.addElementAt(index, pTxSource);
	}
	return pTxSource;
}

LtErrorType LtLayer4::construct(LtApduOut* pApdu, LtPktInfo* pPkt) 
{
	LtErrorType err = LT_NO_ERROR;
    pPkt->setServiceType(pApdu->getServiceType());

	int flags = 0;
	if (pApdu->getAttenuate())
	{
		flags |= MI_ATTENUATE;
	}
	if (pApdu->getZeroSync())
	{
		flags |= MI_ZEROSYNC;
	}
	pPkt->setFlags(flags);

    // Construct outgoing message.
	if (getStack()->isLayer2() &&
		pApdu->getAddressType() != LT_AT_LOCAL)
	{
		// Sending a layer 2, non local message.  Building message simply entails
		// copying the APDU data into the message.  We add two to the length to
		// emulate a CRC.  However, it is obviously not a valid CRC.  The assumption
		// is the engine expects this and just strips it.
		err = pPkt->setData(pApdu->getData(), pApdu->getDataLength()+2);
        getStack()->completionEvent(pApdu, true);
	}
    else if (pApdu->getResponse())
    {
        err = constructResponse(pApdu, pPkt);
    } 
    else 
    {
		// Set the path specification
		if (pApdu->getAlternatePath() != LT_DEFAULT_PATH)
		{
			pPkt->setAltPath(pApdu->getAlternatePath() == LT_ALT_PATH);
		}

		// Convert implicit address to explicit.
        if (pApdu->getAddressIndex() != LT_EXPLICIT_ADDRESS)
        {
            LtAddressConfiguration ac;
            err = getStack()->getAddressConfiguration(pApdu->getAddressIndex(), &ac);
			if (err == LT_NO_ERROR)
			{
				pApdu->setAddressIndex(LT_EXPLICIT_ADDRESS);
				pApdu->copy(ac);
				// Now that address has been set, apply any overrides.  Overrides are ignored
				// with explicit addressing.
				pApdu->applyOverride(false);
			}
        }

		pPkt->setEnclPdu(pApdu->getServiceType() == LT_REQUEST ? LT_SPDU : LT_TPDU);

		if (pApdu->isBound())
		{
			// Convert address domain index to domain configuration if not already
			// established.
			if (!pApdu->getDomainConfiguration().getDomain().inUse() &&
				!getStack()->unconfigured()) 
			{
				LtDomainConfiguration* dc;
				err = getDomainConfiguration(pApdu->getDomainIndex(), &dc);
				if (err == LT_NO_ERROR)
				{
					pApdu->setDomainConfiguration(*dc);
				}
			}
			if (!pApdu->getDomainConfiguration().getDomain().inUse() &&
				pApdu->isBoundExternally())
			{
				// Don't log an error for responses on empty domain (for example,
				// can happen with a leave domain request).
				err = pApdu->getResponse() ? LT_INVALID_PARAMETER : LT_INVALID_DOMAIN;
			}
		}

		if (err == LT_NO_ERROR)
		{
			// Set packet address fields
			pPkt->setDomain(pApdu->getDomainConfiguration().getDomain());
			LtSubnetNode sn;
			int subnetNodeIndex = pApdu->getSubnetNodeIndex();
			if (subnetNodeIndex == -1)
			{
				subnetNodeIndex = 0;
				if (pApdu->isUnsafe())
				{
					// Eventually if we have more than one alternate address we want to be able to 
					// load balance use of alternate addresses, particularly for unique IDs.  Perhaps
					// we could just take the unique ID, sum the bytes and divide by number of alt addresses.
					subnetNodeIndex = 1;
				}
			}

			// Increment here and then pre-decrement in loop.
			subnetNodeIndex++;
			do
			{
				err = pApdu->getDomainConfiguration().getSubnetNode(--subnetNodeIndex, sn);
			}
			while (err == LT_NO_ERROR && !sn.inUse());

			pApdu->setSubnetNodeIndex(subnetNodeIndex);
			pPkt->setSubnetNode(sn);
		}
		
		if (err == LT_NO_ERROR)
		{
			switch (pApdu->getAddressType())
			{
			case LT_AT_UNIQUE_ID:
				pPkt->setAddressFormat(LT_AF_UNIQUE_ID);
				pPkt->setUniqueId(pApdu->getDestinationUniqueId());
				pPkt->setDestSubnet(pApdu->getSubnet());
				break;
			case LT_AT_SUBNET_NODE:
				pPkt->setAddressFormat(LT_AF_SUBNET_NODE);
				pPkt->setDestSubnet(pApdu->getSubnet());
				pPkt->setDestNode(pApdu->getDestId());
				// We allow certain packets that are known to be used to locate duplicate subnet/node
				// addresses to be routed to both specific clients and certain general clients.  By default, once 
				// a specific client is found, packets are not routed to general clients that designate
				// that they don't want such packets (such as the LonTalk port).  This avoids having "in the box"
				// traffic flooding the LonTalk channel except when necessary for dupliate address detection.
				if (pApdu->getCode() == LT_QUERY_ID || 
					pApdu->getCode() == LT_RESPOND_TO_QUERY ||
					(pApdu->getCode() == LT_NODE_MODE && pApdu->getData(0) == LT_MODE_CHANGE_STATE))
				{
					pPkt->setRouteMultiple(TRUE);
				}
				break;
			case LT_AT_BROADCAST:
			case LT_AT_BROADCAST_GROUP:
				pPkt->setAddressFormat(LT_AF_BROADCAST);
				pPkt->setDestSubnet(pApdu->getSubnet());
				break;
			case LT_AT_GROUP:
				pPkt->setAddressFormat(LT_AF_GROUP);
				pPkt->setDestGroup(pApdu->getGroup());
				break;
			case LT_AT_LOCAL:
				pApdu->setTurnaround(true);
				pPkt->setAddressFormat(LT_AF_TURNAROUND);
				break;
			case LT_AT_TURNAROUND_ONLY:
				// This can happen for turnaround NVs.
				pPkt->setAddressFormat(LT_AF_TURNAROUND);
				break;
			default:
				// Illegal address type
				err = LT_BAD_ADDRESS_TYPE;
				break;
			}
		}

		if (err != LT_NO_ERROR)
		{
			// Record failure and continue so that turnaround stuff
			// can be processed and completion events generated.
			pApdu->setAnyTxFailure(true);
			pApdu->setAddressType(LT_AT_UNBOUND);
			pPkt->setAddressFormat(LT_AF_TURNAROUND);
		}

#if INCLUDE_STANDALONE_MGMT
		Snm::ResponseClient responseClient((LtLayer6*)getStack());

		// Try sending the message through the DCI (but not service pin messages)
		if ((pApdu->getCode() != LT_SERVICE_PIN) && Snm::Interface::GetSnmInstance()->SendMessage(pApdu, &responseClient))
		{
			// The DCI will be sending this message so just mark the packet to be freed.
			pPkt->setServiceType(LT_UNKNOWN);
		}
		else
#endif        
        if (pApdu->getServiceType() == LT_UNACKD) 
		{
			err = pPkt->setData(pApdu->getCodeAndData(), pApdu->getLength());
            if (err == LT_NO_ERROR)
            {
			    err = buildMessage2(null, pApdu, pPkt);
            }
		}
		else
		{
			LtTransmitTx* pTx = null;
			err = getNextTx(&pTx, pApdu, pPkt);
			if (err == LT_NO_ERROR)
			{
				err = buildMessage(pTx, pApdu, pPkt);
			}
		}
    }

	return err;
}

LtErrorType LtLayer4::sendTransmitTx(LtTransmitTx* pTx, LtPktInfo* pPkt)
{
	LtErrorType err = LT_NO_ERROR;
    LtApduOut* pApdu = pTx->getApdu();

    if (pApdu != null) 
	{
		if (!pTx->active())
		{
			pTx->setServiceType(pApdu->getServiceType());
			pTx->setEnclPdu(pApdu->getServiceType() == LT_REQUEST ? LT_SPDU : LT_TPDU);
		}
        *(LtPktData*)pPkt = *(LtPktData*)pTx;
        if (!pTx->active())
        {
            // Send as an original message
            err = getNextTx(&pTx, pApdu, pPkt);
        }
		if (err == LT_NO_ERROR)
		{
			err = buildMessage(pTx, pApdu, pPkt);
		}
    }
    else
    {
		err = LT_INVALID_PARAMETER;
    }
    return err;
}

void LtLayer4::completeTx(LtTransmitTx* pTx, boolean success) 
{
    int pri = pTx->getPriorityType();
    bool deferred = pTx->getState() == TX_DEFERRED;
	LtApduOut* pApdu = pTx->getApdu();
    if (pTx->isUnackdRpt()) 
    {
        success = true;
    }
    else if (success)
    {
		registerTransactionEvent(LT_TX_STAT_RETRY, pTx->getRetryCount());
    }

#if FEATURE_INCLUDED(STANDALONE_MGMT)
	// Mark this TX as complete in the proxy history
	proxyHistory.updateTxId(success?true:false);
#endif

    pTx->complete();
    pTx->getTxSource()->m_pTxById[pri][pTx->getTxNumber()] = null;
    // If another message is pending, prepare to send it.
    if (pTx->pendingPdu()) 
    {
        prepareForSend(pTx);
    } 
    else 
    {
        if (pApdu != NULL && pApdu->getAddressType() == LT_AT_UNIQUE_ID)
        {   // recieve timer for unique ID messages is 8 seconds.
            pTx->setDeleteTimer(8000);
        }
        else
        {
            pTx->setDeleteTimer(m_txIdLifetime);
        }
    }
    getStack()->completionEvent(pApdu, success);
    if (!deferred)
    {
	    endTx();
    }
}

LtTx* LtLayer4::refToTx(LtRefId& ref)
{
    LtTx* pTx = null;
    if (ref.getType() == LT_REF_TX)
    {
        pTx = m_pTxsTx->get(ref.getIndex());
    }
    else if (ref.getType() == LT_REF_RX)
    {
        pTx = m_pTxsRx->get(ref.getIndex()); 
    }
	// Validate it
	if (pTx != null && !pTx->getRefId().matches(ref))
	{
		pTx = null;
	}
	return pTx;
}

void LtLayer4::expiration(LtTx* pTx)
{
	if (!pTx->ignoreExpiration())
	{
		pTx->expiration();
		if (pTx->getDeletionRequired()) 
		{
			m_pTxsTx->release((LtTransmitTx*) pTx);
		} 
		else 
		{
			if (!pTx->getReceive())
			{
				LtTransmitTx* pTxTx = (LtTransmitTx*) pTx;
				// 
				if (pTxTx->isComplete(true)) 
				{
					completeTx(pTxTx, true);
				} 
				else if (pTxTx->resendNeeded()) 
				{
					// Bump retry count except for unackd rpt
					if (!pTxTx->isUnackdRpt()) 
					{
						getStack()->getNetworkStats()->bump(LT_RETRIES);
                        pTxTx->bumpRetryCount();
					}
					prepareForSend(pTx);
				} 
				else if (pTxTx->expired()) 
				{
					registerTransactionEvent(LT_TX_STAT_FAILURE);
					getStack()->getNetworkStats()->bump(LT_TRANSMIT_FAILURES);
					completeTx(pTxTx, false);
				} 
				else if (pTxTx->deferred()) 
				{
					prepareForSend(pTxTx);
				}
			}
			if (!pTx->inUse()) 
			{
				// Delete record after a while.
				if (pTx->getReceive())
				{
					m_pTxsRx->release((LtReceiveTx*) pTx);
				}
				else
				{
					pTx->setDeleteTimer(m_txIdLifetime);
				}
			}
		}
	}
}

LtErrorType LtLayer4::receiveAck(LtPktInfo* pPkt) 
{
	LtErrorType err = LT_NO_ERROR;
	boolean bResponse = pPkt->getServiceType() == LT_RESPONSE;
	LtApduIn* pApdu = null;

    m_bReceivedSomething = true;    // We've recieved an ack or a response. This means that
                                    // we can rely on late acks/retries for throttling, not
                                    // missing responses/acks.

	if (pPkt->getAddressFormat() != LT_AF_SUBNET_NODE && pPkt->getAddressFormat() != LT_AF_GROUP_ACK)
	{
		err = LT_INVALID_PARAMETER;
	}
	else
	{
		flipAckAddress(pPkt);

		if (bResponse)
		{
			pApdu = getStack()->getApdu(true);
		}

		LtTransmitTx* pTx = getTransmitTx(pPkt);

		if (pTx == null || !pTx->active()) 
		{
			registerTransactionEvent(LT_TX_STAT_LATE_ACK);
			getStack()->getNetworkStats()->bump(LT_LATE_ACKS);
		} 
		else if ((!bResponse || pApdu != null) &&
				 pTx->receivedValidAck(pPkt)) 
		{
			// If response, send up to application
			if (bResponse)
			{
				LtApduOut* pApduOut = pTx->getApdu();

				// Flip address back for proper setting of incoming address (yuck!)
				flipAckAddress(pPkt);

				pApdu->init(getStack(), pPkt, pApduOut->getRefId());
				pApdu->setProxy(pApduOut->getProxy());

				// Generally we don't care a lick about the response contents.  However, the bi-directional
				// query transceiver status command requires that we modify the response to insert the
				// transceiver register values associated with the last received message (hopefully this
				// response though not guaranteed).  In this case, we get the register values and augment
				// the response to contain them.
				if (pApduOut->getCode() == LT_BIDIR_XCVR_STATUS)
				{
					getStack()->getNetworkManager()->fetchXcvrStatus(*pApdu, *pApdu, LT_NUM_REGS);
				}
				err = getStack()->LtLayer6::receive(pApdu, pTx->getApdu());
				// APDU delivered, null it out to prevent deletion below
				pApdu = null;
			}
			if (err == LT_NO_ERROR)
			{
				if (pTx->isComplete(false)) 
				{
					// Tx is complete
					completeTx(pTx, true);
				} 
				else 
				{
					// Ack means extend retry interval (since
					// each group ack extends timeout).
					pTx->setExpiration();
				}
			}
		}
	}

	delete pApdu;

	return err;
}

LtErrorType LtLayer4::receiveReply(LtPktInfo* pPkt)
{
	LtErrorType err = LT_NO_ERROR;
    flipAuthAddress(pPkt);

    // Now, make sure there is a transaction which is being
    // authenticated to map into.
    LtReceiveTx* pTx = m_pTxsRx->get(pPkt);
    if (pTx != null && pTx->authenticating()) 
    {
        // Validate reply
        LtApduIn* pApdu = pTx->getApdu();
        LtDomainConfiguration* pDomCnfg;
        boolean bProcessReply = FALSE;

        if (pTx->getDomainIndex() == FLEX_DOMAIN_INDEX)
        {   // Process reply using flex domain
            if (getFlexAuthDomain(&pDomCnfg) == LT_NO_ERROR)
            {
                bProcessReply = TRUE;
            }
        }
        else if (getDomainConfiguration(pTx->getDomainIndex(), &pDomCnfg) == LT_NO_ERROR)
        {
            bProcessReply = TRUE;
        }

        if (bProcessReply)
        {
			byte challenge[LT_CHALLENGE_LENGTH];
			byte reply[LT_CHALLENGE_LENGTH];
			pTx->getChallenge(challenge);
			pTx->encrypt(challenge, pDomCnfg, pDomCnfg->getKey(), pPkt->getServiceType() == LT_REPLY_OMA);
			pTx->setApdu(null);
			pPkt->getChallengeReply(reply);
			if (memcmp(challenge, reply, sizeof(reply)) == 0)
			{
				pApdu->setAuthenticated(true);
			}
			err = deliver(pTx, pApdu);
		}
    }
	return err;
}

void LtLayer4::flipAckAddress(LtPktInfo* pPkt)
{
    LtSubnetNode sn(pPkt->getDestSubnet(), pPkt->getDestNode());
    pPkt->setDestSubnet(pPkt->getSourceNode().getSubnet());
    pPkt->setDestNode(pPkt->getSourceNode().getNode());
    pPkt->setSubnetNode(sn);
}

void LtLayer4::flipAuthAddress(LtPktInfo* pPkt)
{
    pPkt->setAddressFormat(pPkt->getOrigFmt());
	if ((pPkt->getServiceType() == LT_CHALLENGE) || 
        (pPkt->getServiceType() == LT_CHALLENGE_OMA))
	{
		flipAckAddress(pPkt);
	}
	else
	{
		switch (pPkt->getAddressFormat())
		{
		    case LT_AF_BROADCAST:
			    pPkt->setDestSubnet(pPkt->getOrigAddress());
			    break;
		    case LT_AF_UNIQUE_ID:
		    {
			    LtUniqueId* pUid = (LtUniqueId*) ((LtUniqueIdClient*) getStack()->getMainClient());
			    pPkt->setUniqueId(*pUid);
			    break;
		    }
			default:
				break;	// nothing
		}
    }
}

void LtLayer4::sendChallenge(LtReceiveTx* pTx, LtPktInfo* pPkt, boolean bUseOma)
{
	// Check for resend state in case of non acking group
	if (pTx->resendNeeded())
	{
        pTx->setPendingType(bUseOma ? LT_CHALLENGE_OMA : LT_CHALLENGE);
		pTx->setChallenge();
		prepareForSend(pTx);
	}
}

LtErrorType LtLayer4::deliver(LtReceiveTx* pTx, LtApduIn* pApdu)
{
    if (!pTx->isSession() &&
        pApdu->getServiceType() != LT_UNACKD_RPT)
    {
        // Schedule ack
        pTx->setPendingType(LT_ACK);
		if (pTx->resendNeeded())
		{
			prepareForSend(pTx);
		}
    }
    pTx->deliver();
    return getStack()->LtLayer6::receive(pApdu, null);
}

LtErrorType LtLayer4::receiveMsg(LtPktInfo* pPkt) 
{
	LtErrorType err = LT_NO_ERROR;

    // Attempt to find a matching receive TX.  If it
    // doesn't match, a new tx is created.
    LtReceiveTx* pTx = getReceiveTx(pPkt);
    
    if (pTx == null) 
    {
        getStack()->getNetworkStats()->bump(LT_RCVTX_FULL);
    } 
    else 
    {
        LtServiceType serviceType = pPkt->getServiceType();
        boolean dup = pTx->isDuplicate();
        if (ANY_REMINDER(serviceType))
        {
            LtAckMap ackMap(pPkt);
            if (dup && ackMap.getReceived(pTx->getDestMember())) 
            {
                pTx->done();
            }
        }
        if (PLAIN_REMINDER(serviceType))
        {
            // Nothing more to do in case of plain reminder.
        } 
        else if (dup) 
        {
            // This is a duplicate.  May need to resend ack/challenge.
            if (pTx->resendNeeded()) 
            {
                prepareForSend(pTx);
            }
        } 
        else 
        {
            // Not a duplicate; first attempt to allocate 
            // an LtApdu.  If none is available, we don't 
            // ack.
            LtApduIn* pApdu = getStack()->getApdu();
            if (pApdu != null) 
            {
                pApdu->init(getStack(), pPkt, pTx->getRefId());
                boolean bSendChallenge = FALSE;
                boolean bUseOma = FALSE;

                if (pPkt->getAuth())
                {
                    if (pApdu->getDomainConfiguration().isFlexDomain())
                    {   // Honor authentication on flex domain only if we are not unconfigured.
                        LtDomainConfiguration* pFlexDomain;
                        
                        if ((getFlexAuthDomain(&pFlexDomain) == LT_NO_ERROR) && 
                             !getStack()->getNetworkImage()->unconfigured() &&
                             omaSupported())
                        {
                            bSendChallenge = TRUE;
                            bUseOma = pFlexDomain->getUseOma();
                        }
                    }
                    else
                    {
                        LtDomainConfiguration* pDomainConfiguration;

		                if (getDomainConfiguration(pTx->getDomainIndex(), &pDomainConfiguration) == LT_NO_ERROR)
		                {
                            bUseOma = omaSupported() && pDomainConfiguration->getUseOma();
                        }
                        bSendChallenge = TRUE;
                    }
                }
                if (bSendChallenge)
                {
                    // Authentication was requested.  Prepare challenge.
                    pTx->setApdu(pApdu);
					sendChallenge(pTx, pPkt, bUseOma);
                } 
                else 
                {
					err = deliver(pTx, pApdu);
                }
            }
        }
    }
	return err;
}

void LtLayer4::setTxIdLifetime(int duration)
{
	m_txIdLifetime = duration;
}

LtErrorType LtLayer4::getDomainConfiguration(int nIndex, LtDomainConfiguration** ppDc)
{
	return getStack()->getNetworkImage()->domainTable.get(nIndex, ppDc);
}

LtErrorType LtLayer4::getConfigurationData(byte* pData, int offset, int length)
{
	getStack()->getNetworkImage()->configData.toLonTalk(pData, offset, length);
	return LT_NO_ERROR;
}

LtErrorType LtLayer4::updateConfigurationData(byte* pData, int offset, int length)
{
	getStack()->getNetworkImage()->configData.fromLonTalk(pData, offset, length, false);
	return LT_NO_ERROR;
}

LtErrorType LtLayer4::getDomainConfiguration(int nIndex, LtDomainConfiguration* pDc)
{
	LtDomainConfiguration* pLocalDc;
	LtErrorType err = getStack()->getNetworkImage()->domainTable.get(nIndex, &pLocalDc);
	if (err == LT_NO_ERROR)
	{
		*pDc = *pLocalDc;
	}
	return err;
}

LtErrorType LtLayer4::getFlexAuthDomain(LtDomainConfiguration** ppDc)
{
    return getStack()->getNetworkImage()->domainTable.getFlexAuthDomain(ppDc);
}

LtErrorType LtLayer4::updateSubnetNode(int domainIndex, int subnetNodeIndex, 
								       const LtSubnetNode &subnetNode)
{
	LtDomainConfiguration* pDc;
	LtErrorType err = getDomainConfiguration(domainIndex, &pDc);
	if (err == LT_NO_ERROR)
	{
		LtSubnetNodeClient* pClient = (LtSubnetNodeClient*) pDc;
		err = pDc->updateSubnetNode(subnetNodeIndex, subnetNode);
		if (err == LT_NO_ERROR)
		{
            if (domainIndex == 0 && !m_bUseLsEnhacedModeOnly)
            {
                m_bUseLsEnhancedMode = pDc->getUseLsEnhancedMode();
            }
            LtLtLogicalChannel* pLtChannel = getStack()->getChannel()->getLtLtLogicalChannel();
            if (pLtChannel != NULL)
            {
                err = pLtChannel->setUnicastAddress(getStack()->getIndex(), domainIndex, subnetNodeIndex, pDc);
            }
        }
		if (err == LT_NO_ERROR)
		{
			pClient->notify();
		}
	}
	return err;
}

LtErrorType LtLayer4::updateDomainConfiguration(int nIndex, LtDomainConfiguration* pDomain, boolean bStore, boolean bRestore)
{
	LtDomainConfiguration *pDc;
	LtErrorType err = getDomainConfiguration(nIndex, &pDc);
	if (err == LT_NO_ERROR)
	{
		LtSubnetNodeClient* pClient = (LtSubnetNodeClient*) pDc;
		getStack()->getNetworkImage()->domainTable.set(nIndex, pDomain);

        if (nIndex == 0 && !m_bUseLsEnhacedModeOnly)
        {
            m_bUseLsEnhancedMode = pDc->getUseLsEnhancedMode();
        }

        LtLtLogicalChannel* pLtChannel = getStack()->getChannel()->getLtLtLogicalChannel();
        if (pLtChannel != NULL)
        {
            err = pLtChannel->setUnicastAddress(getStack()->getIndex(), nIndex, 0, pDomain);
        }

		if (!bRestore)	// Don't send notifications when restoring config via deserialization (it's done later)
		{
			pClient->notify();
		}

		if (bStore)
		{
			getStack()->getNetworkImage()->store();
		}
	}
	return err;
}

LtErrorType LtLayer4::getAddressConfiguration(int nIndex, LtAddressConfiguration* pAc)
{
	LtErrorType err = getStack()->getNetworkImage()->addressTable.get(nIndex, pAc);
	return err;
}

boolean LtLayer4::notifySubnetNodeClient(LtAddressConfiguration* pAc)
{
	boolean bNotified = false;
	// If a change occurred in an input group, let the client know to re-fetch
	// the routing information.
	if (pAc->getAddressType() == LT_AT_GROUP &&
		pAc->getRestrictions() != LT_GRP_OUTPUT_ONLY)
	{
		LtDomainConfiguration* pDc;
		if (getDomainConfiguration(pAc->getDomainIndex(), &pDc) == LT_NO_ERROR)
		{
			LtSubnetNodeClient* pClient = (LtSubnetNodeClient*) pDc;
			pClient->notify();
			bNotified = true;
		}
	}
	return bNotified;
}

LtErrorType LtLayer4::updateAddressConfiguration(int nIndex, LtAddressConfiguration* pAddress, boolean bRestore)
{
	LtAddressConfiguration oldAc;
	LtErrorType err = getStack()->getNetworkImage()->addressTable.get(nIndex, &oldAc);
	if (err == LT_NO_ERROR)
	{
		getStack()->getNetworkImage()->addressTable.set(nIndex, *pAddress);

#if FEATURE_INCLUDED(IZOT)
        // Check for group membership change.
        if ((oldAc.getAddressType() == LT_AT_GROUP || pAddress->getAddressType() == LT_AT_GROUP))
        {
            LtLtLogicalChannel* pLtChannel = getStack()->getChannel()->getLtLtLogicalChannel();
            if (pLtChannel != NULL && pLtChannel->getIsIzoTChannel())
            {
                LtGroups groups;
                getStack()->getNetworkImage()->addressTable.getGroups(pAddress->getDomainIndex(), groups);
                err = pLtChannel->updateGroupMembership(getStack()->getIndex(), pAddress->getDomainIndex(), groups);
                if (err == LT_NO_ERROR && pAddress->getDomainIndex() != oldAc.getDomainIndex())
                {
                    getStack()->getNetworkImage()->addressTable.getGroups(pAddress->getDomainIndex(), groups);
                    err = pLtChannel->updateGroupMembership(getStack()->getIndex(), pAddress->getDomainIndex(), groups);
                }
            }
        }
#endif

		// Notify the subnet/node client.  If the domain changed, notify
		// both! Don't send notifications when restoring config via deserialization (it's done later)
		if (err == LT_NO_ERROR && !bRestore && !oldAc.equals(*pAddress))
		{
            if (!bRestore)
            {
			    // If no notification occurs or the domain changed, do the new
			    // domain.  Covers case where old address is not a group but
			    // new one is.
			    if (!notifySubnetNodeClient(&oldAc) ||
				    oldAc.getDomainIndex() != pAddress->getDomainIndex())
			    {
				    notifySubnetNodeClient(pAddress);
			    }
            }
		}
	}
	return err;
}

LtErrorType LtLayer4::changeState(int newState, boolean bClearKeys)
{
	LtErrorType err = LT_NO_ERROR;

	boolean bNotifyLre = getStack()->getNetworkImage()->configurationChange(newState);

	err = getStack()->LonTalkNode::changeState(newState, bClearKeys);

	if (err == LT_NO_ERROR)
	{
		getStack()->setLreStateInfo();
		if (bNotifyLre)
		{
			// Since main client specifies whether we receive all broadcasts or
			// not, update it only.  We don't mess with all the other clients
			// on a state change.  Instead, we toss messages in the unconfigured
			// state in the stack. 
			getStack()->getLre()->eventNotification(getStack()->getMainClient());
		}
	}
	return err;
}

LtErrorType LtLayer4::getReadOnlyData(byte* readOnlyData)
{
	int len = getStack()->getReadOnly()->getLength();
    byte* data;
	getStack()->getReadOnly()->toLonTalk(0, len, &data);
    memcpy(readOnlyData, data, len);
	delete data;
    return LT_NO_ERROR;
}

void LtLayer4::setErrorLog(int err)
{
	getStack()->LonTalkNode::setErrorLog(err);
}

LtErrorType LtLayer4::initProgram(LtProgramId &pid)
{
	getStack()->getReadOnly()->setProgramId(pid);
	return LT_NO_ERROR;
}

void LtLayer4::sendServicePinMessage()
{
	getStack()->LonTalkStack::sendServicePinMessage();
}

int LtLayer4::getXcvrId()
{
	return getStack()->LonTalkStack::getXcvrId();
}

void LtLayer4::setXcvrId(int id)
{
	getStack()->LonTalkStack::setXcvrId(id);
}

//
// deferTx
//
// This function is used to determine whether we can generate more output.
// We don't want to generate too much network activity lest we flood the
// network and start causing excessive retries.  Worse yet, we can end up
// with a situation where lots of packets are queued up in the source and
// thus start timing out even before packets are delivered on the channel.
// We could try to have a feedback loop which indicates that packets are
// backing up at the interface but this requires "reaching around" to the
// driver which is a kludge and doesn't address the lag time between 
// queuing something for the engine and congesting at the interface.  Also,
// there are a fair number of buffers in the driver and the interface itself
// so we don't have a good way to know congestion is occurring until it
// may already be too late.
//
// So, we instead use feedback at the transaction layer in the form of
// late acks, retries and failures to determine how to throttle.
//
// Insert formula here...
//
// The function returns true if output should be throttled.  It should
// be called for new transactions only, not for retries.
//
boolean LtLayer4::deferTx(void)
{
	boolean bDefer = m_nTxActive >= m_nTxLimit;
	if (bDefer)
	{
		m_bLimitHit = true;
	}
	return bDefer;
}

void LtLayer4::startTx(void)
{
	++m_nTxActive;
}

void LtLayer4::endTx(void)
{
	m_nTxActive--;
	if (m_bLimitHit)
	{
		// Prevented dequeues so signal output thread
		semGive(m_semMsg);
	}

	if (++m_nTxCount%TX_WINDOW_SIZE == 0)
	{
        if (!m_bReceivedSomething && m_txStats[LT_TX_STAT_FAILURE] != 0)
        {   // Normally we don't throttle based on transaction failures,
            // but we've been receiving nothing, and have lots of losses. 
            // This could be due to complete saturation.
            m_bException = TRUE;

        }
		// Up the transaction limit as long as something was deferred
		// and nothing bad happened.  
		if (!m_bException)
		{
			if (m_bLimitHit && m_nTxLimit < m_nUpperLimit)
			{
				m_bLastAdjustmentPositive = true;
				m_nTxLimit++;
			}
		}
		else
		{
			unsigned int delta = 0;

			if (m_txStats[LT_TX_STAT_LATE_ACK] > 2)
			{
				// We hit a transaction limit which produces late acks.  Don't
				// try to go past this for a while to avoid trashing around this
				// set point.  Of course, one too aggressive tx timer will result
				// in a potentially overly severe upper limit.  So, throttling
				// will only work well if timers are set well.
				if (m_bLastAdjustmentPositive)
				{
					m_nUpperLimit = m_nTxLimit-1;
				}
				if (m_txStats[LT_TX_STAT_LATE_ACK] > TX_WINDOW_SIZE*.1)
				{
					delta = 2;
				}
				else
				{
					delta = 1;
				}
			}
			else 
			{
				// Drop the tx limit if retries exceed 10%.
				// We don't look at retries if we had late acks since late acks
				// imply retries.
				if (m_txStats[LT_TX_STAT_RETRY] > 2)
				{
					if (m_txStats[LT_TX_STAT_RETRY] > TX_WINDOW_SIZE*.1)
					{
						delta = 2;
					}
					else
					{
						delta = 1;
					}
				}
			}

			// A failure without retries or late acks in other transactions
			// is assumed to be a dead node.  So, if no delta so far, ignore
			// failures.  Otherwise, assume they are indicative of traffic problems
			// and reduce transaction limit.
            // An exception if nothing is being recieved at all - this could mean total
            // network melt-down - so reduce the counts even when there are no retries or
            // late acks.
			if (delta || !m_bReceivedSomething)
			{
				if (m_txStats[LT_TX_STAT_FAILURE]>TX_WINDOW_SIZE*.5)
				{
					delta += 2;
				}

				if (delta <= m_nTxLimit)
				{
					m_bLastAdjustmentPositive = false;
					m_nTxLimit -= delta;
					if (m_nTxLimit < TX_INITIAL_LIMIT)
					{
						m_nTxLimit = TX_INITIAL_LIMIT;
					}
				}
				else
				{
					m_nTxLimit = TX_INITIAL_LIMIT;
				}
			}
		}

		// Clear the stats
		m_bLimitHit = false;
		m_bException = false;
        m_bReceivedSomething = false;
		memset(m_txStats, 0, sizeof(m_txStats));
	}

	if (m_nTxCount == TX_WINDOW_SIZE*25)
	{
		m_nUpperLimit = TX_LIMIT_MAX;
		m_nTxCount = 0;
	}
}

void LtLayer4::registerTransactionEvent(LtTransactionStatistic stat, int count)
{
	if (stat < NUM_LT_TX_STATS)
	{
		m_txStats[(int)stat] += count;

        if (stat != LT_TX_STAT_FAILURE)
        {
            if (count != 0)
            {  // If any kind of exception occurred other than TX failure, we don't up the limit.
		        m_bException = true;
            }
        }
	}
}

void LtLayer4::getNmVersion(int &nmVersion, int &nmCapabilities) 
{
    nmVersion = 2;
    nmCapabilities = LT_EXP_CAP_INIT_CONFIG |
                     LT_EXP_CAP_UPDATE_NV_INDEX |
                     LT_EXP_CAP_LS_MODE_COMPATIBILITY_OR_ENHANCED;

#if FEATURE_INCLUDED(IZOT)   
    nmCapabilities |= LT_EXP_CAP_LSIP_ADDR_MAPPING_ANNOUNCEMENTS;
#endif

#if !PRODUCT_IS(FTXL)
    nmCapabilities |= LT_EXP_CAP_BI_DIRECTIONAL_SIGNAL_STRENGTH | 
                      LT_EXP_CAP_PHASE_DETECTION;
#endif

#if FEATURE_INCLUDED(LTEP)
	nmCapabilities |= LT_EXP_CAP_PROXY;
#endif

    if (omaSupported())
    {
        nmCapabilities |= LT_EXP_CAP_OMA;
    }

	// If the device is serving in SNM mode, then it must not respond that it has PROXY capability.  Otherwise,
	// the DCI would attempt to repeat with this device.
#if FEATURE_INCLUDED(STANDALONE_MGMT)
	if (Snm::Interface::GetSnmInstance()->IsActive())
	{
		nmCapabilities &= ~LT_EXP_CAP_PROXY;
	}
#endif    

    if (m_bNmVersionOveridden)
    {
        // NM version and capabilities have been overridden.  Report the minimum
        // of what is actually implemented and the override.
        if (m_nmVersionOverride < nmVersion)
        {
            nmVersion = m_nmVersionOverride;
        }
        if ((m_nmCapOveride & LT_EXP_CAP_LS_MODE_MASK) == LT_EXP_CAP_LS_MODE_ENHANCED_ONLY &&
            (nmCapabilities & LT_EXP_CAP_LS_MODE_MASK) >= LT_EXP_CAP_LS_MODE_COMPATIBILITY_OR_ENHANCED)
        {
            // The stack supports both enhanced mode and compatibiltiy mode. 
            // The override says to support enhaced mode only.  We can do that.
            nmCapabilities |= LT_EXP_CAP_LS_MODE_ENHANCED_ONLY;  
        }
        nmCapabilities = nmCapabilities & m_nmCapOveride;
    }
}

LtErrorType LtLayer4::disableOma()
{
    m_bOmaSupported = false;
    return LT_NO_ERROR;
}

LtErrorType LtLayer4::setNmVersionOverride(int nmVersion, int nmCapabilities)
{
    m_bNmVersionOveridden = TRUE;
    m_nmVersionOverride = nmVersion;
    m_nmCapOveride = nmCapabilities;

    if ((m_nmCapOveride & LT_EXP_CAP_LS_MODE_MASK) == LT_EXP_CAP_LS_MODE_ENHANCED_ONLY)
    {
        // Emulate a device that doesn't support compatibity mode.
        m_bUseLsEnhacedModeOnly = TRUE;
    }

    return LT_NO_ERROR;
}


boolean LtLayer4::getTransmitTxStats(int &nMax, int &nFree, int &nPendingFree, 
                                     int &nMaxAllocated, int &nInstantiated, int &searchRatio,
                                     int &txAllocationFailures)
{
    m_pTxsTx->getStats(nMax, nFree, nPendingFree, nMaxAllocated, nInstantiated, searchRatio, txAllocationFailures);
    return true;
}

boolean LtLayer4::getReceiveTxStats(int &nMax, int &nFree, int &nPendingFree,  
                                    int &nMaxAllocated, int &nInstantiated, int &searchRatio,
                                    int &txAllocationFailures)  
{
    m_pTxsRx->getStats(nMax, nFree, nPendingFree, nMaxAllocated, nInstantiated, searchRatio, txAllocationFailures);
    return true;
}

void LtLayer4::clearTransmitTxStats(void)
{
    m_pTxsTx->clearStats();
}

void LtLayer4::clearReceiveTxStats(void)  
{
    m_pTxsRx->clearStats();
}

LtErrorType LtLayer4::waitForPendingInterfaceUpdates(void)
{
	return getStack()->getChannel()->waitForPendingInterfaceUpdates();
}



