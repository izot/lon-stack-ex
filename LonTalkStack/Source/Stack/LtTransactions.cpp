//
// LtTransactions.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtTransactions.cpp#3 $
//

// include this to avoid 'isascii' macro collision
#include <ctype.h>
#include "LtaDefine.h"

#ifdef WIN32
#include "windows.h"
#endif

#if FEATURE_INCLUDED(STANDALONE_MGMT)
#include <StdHelper.h>
#endif

#include "LtStackInternal.h"
#include "LtNetworkManagerLit.h"

#if FEATURE_INCLUDED(STANDALONE_MGMT)
#include "SnmInterface.h"
#endif

byte LtReceiveTx::m_byLastChallenge[LT_CHALLENGE_LENGTH];

/* Comment about possible issue 
 * Since this code uses only two transaction spaces, there is the potential to
 * have a problem after a reset (since only the first tx gets txid 0).  On reset,
 * we don't zero the txid.  However, on a power cycle, need to delay if the receive
 * transactions can be longer than the power cycle time.  Currently, for JavaOS,
 * the reset time is >> the transaction time so no need to do anything. */

LtTx::LtTx(LtRefIdType nType, int nIndex) :
        m_refId(nType, nIndex, 0), m_bPendingFree(false),
        m_pApdu(null), m_nState(TX_IDLE) 
{
    m_nState = TX_IDLE;
    m_bUseLsEnhancedMode = false;
	m_ignoreCount = 0;
	m_bExpirationOccurred = false;
	m_bOnTxQ = false;
	m_wDogTimer = wdCreate();
	assert(m_wDogTimer);
}

LtTx::~LtTx()
{
	wdDelete(m_wDogTimer);
}

void LtTx::init()
{
	setExpirationOccurred(false);
	m_refId.bump();
}

boolean LtTx::operator==(LtPktData& data)
{
    boolean bMatch = false;

    switch (getAddressFormat())
    {
        case LT_AF_BROADCAST:
            bMatch = getDestSubnet() == data.getDestSubnet();
			break;
        case LT_AF_GROUP:
            bMatch = getDestGroup() == data.getDestGroup();
            break;
        case LT_AF_SUBNET_NODE:
            bMatch = getDestSubnet() == data.getDestSubnet() &&
                     getDestNode() == data.getDestNode();
            break;
        case LT_AF_UNIQUE_ID:
            bMatch = getUniqueId() == data.getUniqueId();
            break;
		case LT_AF_TURNAROUND:
			return data.getAddressFormat() == LT_AF_TURNAROUND;
		default:
			break;	// nothing
    }
    if (bMatch)
    {
        bMatch = getSourceNode() == data.getSourceNode() &&
                 getAddressFormat() == data.getAddressFormat() &&
                 getPriority() == data.getPriority() &&
                 getDomain() == data.getDomain();
    }
    return bMatch;
}

boolean LtTx::ignoreExpiration()
{ 
	boolean bResult = true;
	boolean bIgnore = m_ignoreCount != 0;

	// Regardless of TX state, make sure ignore count is decremented.
	if (bIgnore)
	{
		m_ignoreCount--;
	}

	// Ignore event if IDLE or ignore flag was set.
	if (m_nState != TX_IDLE)
	{
		setExpirationOccurred(false);
		if (!bIgnore)
		{
			bResult = false;
		}
	}
	return bResult;
}

boolean LtTx::inUse() 
{
    return m_nState != TX_IDLE && m_nState != TX_WAIT_FOR_DELETION;
}

boolean LtTx::active()
{
	return inUse();
}

int LtTx::getPriorityType() 
{
    return LtMisc::getPriorityType(getPriority());
}

void LtTx::setDelta(int delta) 
{
	// Treat -1 as special case.  Assumed to be infinite and thus we don't set timer.
	// timeToTicks would cause -1 to not be -1.
	if (delta == -1)
	{
		m_nExpirationDelta = delta;
	} 
	else
	{
		m_nExpirationDelta = msToTicks(delta);
	}
}

int LtTx::getDelta()
{
	return m_nExpirationDelta;
}

int VXLCDECL LtTx::TxTimeout(int nParam, ...)
{
	LtTx* pTx = (LtTx*) nParam;
	pTx->getOwner()->notifyExpiration(pTx);
	return 0;
}

void LtTx::stopTimer()
{
	wdCancel(m_wDogTimer);
	if (getExpirationOccurred())
	{
		// An expiration occurred before we cancelled.  Flag the tx so
		// that the expiration message is ignored.
		bumpIgnoreCount();
		setExpirationOccurred(false);
	}
}

void LtTx::startTimer(int nTicks)
{
	// Stop the timer first.  The reason for this is to guard against a previously
	// started timer from expiring just as we start this one.  The old one would 
	// then be mistaken for the new one.  An optimization here would be to only
	// stop the timer if we thought one was running already (however, in a busy
	// system this will often be the case anyway).
	stopTimer();
	setExpirationOccurred(false);
	if (nTicks != -1)
	{
		wdStart(m_wDogTimer, nTicks, TxTimeout, (int) this);
	}
}

void LtTx::setExpiration(int nState, int nDelta, int nDeltaLast) 
{
    setDelta(nDelta);
    setExpiration(nState);
	setExpirationDeltaLast(nDeltaLast);
}

void LtTx::setExpiration(int nState) 
{
    this->m_nState = nState;
    restartTimer();
}

void LtTx::restartTimer()
{
	startTimer(m_nExpirationDelta);
}

boolean LtTx::getDeletionRequired() 
{
    return m_nState == TX_WAIT_FOR_DELETION;
}

void LtTx::encrypt(byte* pValue, LtDomainConfiguration *pDomain, byte* pKey, boolean isOma)
{
    byte omaBuffer[MAX_APDU_SIZE + LT_OMA_DEST_DATA_LEN];
    int nMsgIndex = m_pApdu->getLength();
    int keyLength;
    int keyIterations;  // Number of iterations over the key bytes.  
                        // For classic keys, this is the same as
                        // the key length, but for OMA, its 1 1/2 times
                        // the key length - byte 0-11 followed by 0-5.

    const byte *pData;
    if (isOma)
    {   // Include OMA destination data with the message data.
	    getOmaDestData(omaBuffer, pDomain);
        memcpy(&omaBuffer[LT_OMA_DEST_DATA_LEN], m_pApdu->getCodeAndData(), nMsgIndex);
        pData = omaBuffer;
        nMsgIndex += LT_OMA_DEST_DATA_LEN;
        keyLength = LT_OMA_DOMAIN_KEY_LENGTH;
        keyIterations = LT_OMA_DOMAIN_KEY_LENGTH + LT_OMA_DOMAIN_KEY_LENGTH/2; // 1.5 times over the key
    }
    else
    {
        pData = m_pApdu->getCodeAndData();
        keyLength = LT_CLASSIC_DOMAIN_KEY_LENGTH;
        keyIterations = keyLength;  // Once over the entire key.
    }
    while (nMsgIndex > 0) {
        for (int i = 0; i < keyIterations; i++) 
        {
            for (int j = 7; j >= 0; j--) 
            {
                int k = (j + 1) % 8;
                byte m = 0;
                if (nMsgIndex > 0) 
                {
					m = pData[--nMsgIndex];
                }
                int temp = ~(pValue[j] + j);
                if ((pKey[i % keyLength] & (1 << (7 - j))) != 0) 
                {
                    temp = (temp << 1) + ((temp >> 7)&0x01);
                } 
                else 
                {
                    temp = -(((temp >> 1)&0x7f) + (temp << 7));
                }
                pValue[j] = pValue[k] + m + temp;
            }
        }
    }
}

// Returns the destination address data in the form used by OMA authentication
void LtTx::getOmaDestData(byte *pData, LtDomainConfiguration *pDomain)
{
	int i;

    memset(pData, 0xff, LT_OMA_DEST_DATA_LEN);
    switch (getAddressFormat())
    {
    case LT_AF_UNIQUE_ID:
        getUniqueId().getData(pData);
        break;

    case LT_AF_GROUP:
    case LT_AF_SUBNET_NODE:

        for (i = 0; i < LT_DOMAIN_LENGTH; i++)
        {   // Get the domain information from the domain table rather than 
            // the message, since otherwise we have to guess at the values
            // of some of the bytes when the domain is less than 6 bytes.  We
            // could have defined these to be 0, but that is not the way the 
            // neuron is implemented.
            *pData++ = pDomain->getDomain().getData(i);
        }
        *pData++ = getDomain().getLength();

        if (getAddressFormat() == LT_AF_SUBNET_NODE)
        {
            *pData++ = getDestSubnet();
            *pData++ = getDestNode();
        }
        else
        {   
            *pData++ = getDestGroup();
        }
        break;
	default:
		break;	// nothing
    }
}

boolean LtTx::match(LtPktData* pData)
{
    boolean bMatch = getPriority() == pData->getPriority();
	boolean bCheckDomain = true;
	LtAddressFormat format = pData->getAddressFormat();

	if (bMatch)
	{
		if (format == LT_AF_GROUP_ACK)
		{
			bMatch = getAddressFormat() == LT_AF_GROUP &&
					 getDestGroup() == pData->getDestGroup();
		}
		else
		{
			bMatch = getAddressFormat() == format;
		}
	}

	if (bMatch)
	{
		switch (format)
		{
		case LT_AF_UNIQUE_ID:
			bMatch = getUniqueId() == pData->getUniqueId();
			break;
		case LT_AF_SUBNET_NODE:
			bMatch = getDestNode() == pData->getDestNode() &&
					 getDestSubnet() == pData->getDestSubnet();
			break;
		case LT_AF_GROUP_ACK:
			break;
		case LT_AF_GROUP:
			bMatch = getDestGroup() == pData->getDestGroup();
			break;
		case LT_AF_BROADCAST:
			bMatch = getDestSubnet() == pData->getDestSubnet();
			break;
		case LT_AF_TURNAROUND:
			bCheckDomain = false;
			break;
		default:
			assert(0);
			break;
		}
	}

	if (bMatch && bCheckDomain)
	{
		bMatch = getSourceNode() == pData->getSourceNode() && getDomain() == pData->getDomain();
	}

    return bMatch;
}

//
// LtAckMap
//
int LtAckMap::getCount() 
{
    return m_nCount;
}

int LtAckMap::getMax() 
{
    return m_nMaxMember;
}

boolean LtAckMap::getReceived(int groupMember) 
{
    if (groupMember <= m_nMaxMember) 
    {
        return (m_byMap[groupMember / 8] & (1 << (groupMember % 8))) != 0;
    }
    return false;
}

boolean LtAckMap::setReceived(int nGroupMember) 
{
    boolean bOk = false;
    if (nGroupMember < NUM_MEMBERS) 
    {
        m_byMap[nGroupMember / 8] |= (1 << (nGroupMember % 8));
        m_nMaxMember = nGroupMember>m_nMaxMember ? nGroupMember : m_nMaxMember;
        m_nCount++;
        bOk = true;
    }
    return bOk;
}

void LtAckMap::setMap(LtPktInfo* pPkt)
{
    int nMapLength = ((m_nCount == 0) ? 0 : (m_nMaxMember / 8) + 1);
    pPkt->addData(m_byMap, nMapLength);
	byte mapLen = nMapLength;
    pPkt->addData(&mapLen, 1);
}

LtAckMap::LtAckMap() 
{
}

LtAckMap::LtAckMap(LtPktInfo* pPkt) 
{
    byte nLen = 0;
    pPkt->removeData(&nLen, 1);
    if (nLen > sizeof(m_byMap))
    {
        // EPR 23157
        // Corrupt message.  Don't read it it in, since this would cause a memory overwrite.
        init();
    }
    else
    {
	    pPkt->removeData(m_byMap, nLen);
	    m_nCount = nLen * 8;
        m_nMaxMember = m_nCount - 1;
    }
}

void LtAckMap::init()
{
    m_nMaxMember = -1;
    m_nCount = 0;
	memset(m_byMap, 0, sizeof(m_byMap));
}


//
// LtTransmitTx
//
LtTransmitTx::~LtTransmitTx()
{
	// Delete any APDUs that may be in queue
	LtApdu* pApdu;
	if (m_pApdusPending != NULL)
	{
		while (m_pApdusPending->receive(&pApdu, NO_WAIT) == OK)
		{
			delete pApdu;
		}
		delete m_pApdusPending;
	}
	delete getApdu();
}

void LtTransmitTx::init(LtApduOut* pApdu)
{
	LtTx::init();
    setApdu(pApdu);
    m_nRetries = pApdu->getRetry();
    m_nRetriesSent = 0;
    m_bTurnaround = false;
    m_nExpectedAcks = 1;
    m_bFirst = true;
    m_bUseCurrent = false;
    m_nAckCount = 0;
	setServiceType(pApdu->getServiceType());
    m_bUnackdRpt = getServiceType() == LT_UNACKD_RPT;
    if (m_bUnackdRpt) 
    {
        m_nExpectedAcks = 0;
    }
    else if (!pApdu->isBoundExternally())
    {
        m_nExpectedAcks = 0;
    } 
    else if (pApdu->getAddressType() == LT_AT_GROUP) 
    {
        // Expect acks from everyone but ourselves.
		m_ackMap.init();
        m_nExpectedAcks = pApdu->getSize() - 1;
    } 
    else if (pApdu->getAddressType() == LT_AT_BROADCAST_GROUP) 
    {
        m_nExpectedAcks = pApdu->getMaxResponses();
    }
    setAltPath(pApdu->getAlternatePath());
    setExpiration(TX_PROCESSING, m_bUnackdRpt ? pApdu->getRptTimer() : pApdu->getTxTimer(), pApdu->getTxTimerDeltaLast());
}

void LtTransmitTx::setTxNumber(int nTxNumber)
{
	LtApduOut* pApdu = getApdu();
	// We unfortunately must check the APDU to determine if we are changing state in a way
	// that causes the target to flip from normal mode (configured or hard offline) 
	// to flex domain mode (unconfigured or appless).  If so, we must rememeber the current
	// tx number and avoid using it in the future.
	if (pApdu && pApdu->getCode() == LT_NODE_MODE && pApdu->getData(LT_MODE_OFFSET) == LT_MODE_CHANGE_STATE)
	{
		int oldState = m_nNodeState;
		m_nNodeState = pApdu->getData(LT_STATE_OFFSET) & LT_FLEX_DOMAIN_STATE;
		if (m_nNodeState != oldState)
		{
			m_nAlternateTxId = nTxNumber;
		}
	}
	LtPktData::setTxNumber(nTxNumber);
}

LtAckMap* LtTransmitTx::getAckMap() {
    return &m_ackMap;
}

boolean LtTransmitTx::isUnackdRpt() {
    return m_bUnackdRpt;
}

boolean LtTransmitTx::resendNeeded() {
    return m_nState == TX_SEND_REMINDER_RETRY || m_nState == TX_SEND_RETRY;
}

boolean LtTransmitTx::reminderNeeded() {
    return getIsGroup() && m_nState == TX_SEND_REMINDER_RETRY && !m_bUnackdRpt;
}

boolean LtTransmitTx::expired() {
    return m_nState == TX_IDLE;
}

boolean LtTransmitTx::getTurnaround() {
    return m_bTurnaround;
}

void LtTransmitTx::setTurnaround(boolean v) {
    m_bTurnaround = v;
}

boolean LtTransmitTx::deferred() {
    boolean result = m_nState == TX_DEFERRED;
    if (result) 
    {
        if (!m_bUseCurrent && !pendingPdu()) 
        {
            m_nState = TX_IDLE;
        }
        else
        {
            m_nState = TX_UNBLOCKED;
        }
    }
    return result;
}

void LtTransmitTx::addPending(LtApdu* pApdu)
{
	if (m_pApdusPending == null)
	{
		m_pApdusPending = new LtApdus;
	}
    m_pApdusPending->send(pApdu);
}

void LtTransmitTx::defer(LtApdu* pApdu, int duration) 
{
    // Don't requeue PDU for sending if it is already 
    // the current PDU.  
    if (getApdu() == pApdu)
	{
        m_bUseCurrent = true;
    }
	else 
	{
        m_bUseCurrent = false;
        addPending(pApdu);
    }
	duration = msToTicks(duration);
    startTimer(duration);
    m_nState = TX_DEFERRED;
}

void LtTransmitTx::complete() {
    trace("complete ttx ");
    setApdu(null);
	// Stop the timer.  We could just let it time out (at which point it
	// should do nothing).  The question is, which is more efficient?  On
	// VxWorks, expect cancelling timer is more efficient.  Not sure about Win.
	stopTimer();
    m_nState = TX_IDLE;
}

void LtTransmitTx::setDeleteTimer(int duration) 
{
	// For transmission, keep tx around for a while so that we can track
	// tx id lifetimes in the receiver.
	if (getAddressFormat() == LT_AF_UNIQUE_ID && duration != -1)
	{
		duration = 8196;
	}
	setExpiration(TX_WAIT_FOR_DELETION, duration);
}

boolean LtTransmitTx::pendingPdu() 
{
	boolean result = false;
	LtApdu* pApdu;
    if (getApdu() == null && m_pApdusPending != null && 
		m_pApdusPending->receive(&pApdu, NO_WAIT) == OK)
	{
		result = true;
		setApdu(pApdu);
		m_nState = TX_UNBLOCKED;
    }
    return result;
}

int LtTransmitTx::minTxId() {
    return 1;
}

void LtTransmitTx::send(LtApduOut* pApdu, LtPktInfo *pPkt) 
{
#if FEATURE_INCLUDED(STANDALONE_MGMT)
	using namespace Snm;
#endif    
    int nDeltaBacklog;
    if (m_bUnackdRpt) 
    {
        // Backlog is retry count on first attempt, then 0
        nDeltaBacklog = m_bFirst ? m_nRetries : 0;
    // Backlog is equal to expected ack count
    } 
    else if (pPkt->getAddressFormat() == LT_AF_BROADCAST)
    {
        nDeltaBacklog = pApdu->getBacklog();
        if (nDeltaBacklog == 0)
        {
            nDeltaBacklog = 15;
        }
    } 
    else 
    {
        nDeltaBacklog = m_nExpectedAcks;
    }

#if FEATURE_INCLUDED(STANDALONE_MGMT)
	if (Interface::GetSnmInstance()->GetStandaloneMode() == Repeating)
	{
		// NOTE: In a repeating system, you want to set the network interface in
		// master/slave mode to reduce the effects of backlog creep that could
		// occur when a node sees lots of repeated packets with backlog.  Since the
		// network interface of certain masters (such as the i.lon) do not support
		// master/slave mode, we just force the backlog to 0.  This won't change
		// the backlog of the repeated frames, but it at least gives us an edge.
		// We make an exception for broadcast req/resp since these are relatively and may require
		// backlog.
		if (pPkt->getAddressFormat()!=LT_AF_BROADCAST || pPkt->getServiceType()!=LT_REQUEST)
		{
			nDeltaBacklog = 0;
		}
	}
#endif

    pPkt->setDeltaBacklog(nDeltaBacklog);
}

void LtTransmitTx::altPath(LtPktInfo* pPkt) 
{
    m_bFirst = false;
    // Resolve alternate path
    if (m_altPath == LT_DEFAULT_PATH) 
    {
        pPkt->setAltPath(false);
        if (!m_bUnackdRpt && m_nRetries < 2) 
        {
            pPkt->setAltPath(true);
        }
    }
    else
    {
        pPkt->setAltPath(m_altPath == LT_ALT_PATH);
    }
}

boolean LtTransmitTx::txIdOk(int txid)
{
	// Determines if the transaction ID is OK to use - that is, it does not
	// conflict with either the last tx ID sent of the last one sent before
	// switch config states (the alternate tx id).
	return txid != getTxNumber() &&	txid != m_nAlternateTxId;
}

void LtTransmitTx::expiration() 
{
    if (m_nState == TX_WAIT) 
	{
        if (m_nRetries-- > 0) 
		{
            m_nState = TX_SEND_REMINDER_RETRY;                
        } 
		else 
		{
            m_nState = TX_IDLE;
        }
    }
}

boolean LtTransmitTx::ambiguousAddress() {
    return getAddressFormat() == LT_AF_UNIQUE_ID ||
           getAddressFormat() == LT_AF_BROADCAST;
}

boolean LtTransmitTx::isComplete(boolean afterTimeout) 
{
    boolean complete = false;
    if (m_bUnackdRpt) 
	{
		// Only complete unackd_rpt immediately after sending message.  It would
		// be premature to terminate it on a timeout.
        complete = !afterTimeout && m_nRetries <= 0;
    } 
	else 
	{
        complete = m_nExpectedAcks == 0 && !m_bTurnaround;
        if (!complete && afterTimeout) 
		{
            // Upon timeout from broadcast group, indicate completion
            // if we received at least one response
			assert(getApdu());
            if (m_nAckCount != 0 && getApdu()->getAddressType() == LT_AT_BROADCAST_GROUP) 
			{
                complete = true;    
            }
        }
    }
    return complete;
}

boolean LtTransmitTx::receivedValidAck(LtPktInfo* pPkt)
{
    boolean bValid = false;
    if (m_nExpectedAcks != 0) {
        if (pPkt->getAddressFormat() == LT_AF_GROUP_ACK) 
        {
            int nMember = pPkt->getDestMember();
            bValid = !m_ackMap.getReceived(nMember) && 
                      m_ackMap.setReceived(nMember);
        } 
		else 
        {
            bValid = true;
        }
        if (bValid) {
            m_nExpectedAcks--;
            m_nAckCount++;
        }
    }
    return bValid;
}

boolean LtTransmitTx::buildReminderMsg(LtApdu* pApdu, LtPktInfo* pPkt) 
{
    boolean bReminderOnly = false;
    if (reminderNeeded()) 
    {
        if (m_ackMap.getMax() > 15) 
        {
            m_ackMap.setMap(pPkt);
            pPkt->setServiceType(isSession()?LT_REMINDER_REQUEST:LT_REMINDER_ACKD);
            pPkt->setDeltaBacklog(0);
            m_nState = TX_SEND_RETRY;
            bReminderOnly = true;
        } 
        else 
        {       
            pPkt->setData(pApdu->getCodeAndData(), pApdu->getLength());
            m_ackMap.setMap(pPkt);
			pPkt->setServiceType(isSession()?LT_REMMSG_REQUEST:LT_REMMSG_ACKD);
        }
    } 
    else 
    {
        pPkt->setData(pApdu->getCodeAndData(), pApdu->getLength());
    }
    return bReminderOnly;
}

boolean LtTransmitTx::active()
{
	// 
	// Transmit transactions distinguish between "in-use" and "active".  
	// "active" is "in-use" plus any transactions in the unblocked state.
	// This covers transactions that are waiting to be re-processed due
	// to a pending request from the application.
	//
	return inUse() && m_nState != TX_UNBLOCKED;
}

void LtTransmitTx::restartTimer()
{
	startTimer(m_nExpirationDelta);
	int duration = m_nExpirationDelta;
	if (m_nRetries == 0)
		duration += m_nExpirationDeltaLast;
	startTimer(duration);
}

void LtReceiveTx::init(LtPktInfo* pPkt)
{
	LtTx::init();
    m_bDuplicate = false;
    m_bAuthenticated = false;
    m_bAltPath = false;
	setApdu(null);

	setEnclPdu(pPkt->getEnclPdu());
	setPendingType(LT_UNKNOWN);
    setUseEnhancedMode(pPkt->getUseEnhancedMode());
	// Set the domain index (groups set below)
	if (getAddressFormat() != LT_AF_GROUP)
	{
		LtUniqueIdClient* pClient = (LtUniqueIdClient*) pPkt->getDestClient();
		if (getOwner()->getStack()->unconfigured())
		{
			// Domain configuration can be set to a valid domain in a subnet/node
			// client even though we're in the unconfigured state so force flex
			// domain in this case so that ack contains 0/0 source address and
			// auth can't happen.
			setDomainIndex(FLEX_DOMAIN_INDEX);
		}
		else
		{
			setDomainIndex(pClient->getIndex());
		}
	}

    switch (getAddressFormat()) 
	{
    case LT_AF_UNIQUE_ID:
	{
        setExpiration(TX_WAIT, FIXED_UNIQUE_ID_TIMER);
        break;
	}
    case LT_AF_GROUP: 
	{
		int state = TX_WAIT;
		int timeout = 1000;
		// Find the group entry
		LtSubnetNodeClient* pClient = (LtSubnetNodeClient*) pPkt->getDestClient();
		LtAddressConfiguration* pAc;
		pAc = getOwner()->getStack()->getNetworkImage()->addressTable.get(pClient->getIndex(), pPkt->getDestGroup());
		if (pAc == null)
		{
			// Did the group get deleted?
			state = TX_DONE;
		}
		else
		{
			// In the advent that we need to ack or respond, store the group number. 
			setDestMember(pAc->getMember());
			setDomainIndex(pAc->getDomainIndex());
			if (pAc->getRestrictions() == LT_GRP_INPUT_NO_ACK)
			{
				state = TX_DONE;
			}
			timeout = pAc->getRcvTimer();
		}
        setExpiration(state, timeout);
        break;
	}
    default:
        setExpiration(TX_WAIT, getOwner()->getStack()->getNetworkImage()->configData.getNonGroupReceiveTimer());
        break;
    }
}

void LtReceiveTx::deliver() 
{
	if (isSession())
	{
        m_nState = TX_PENDING;
    } 
	else 
	{
        m_nState = TX_ACK;
    }
}

void LtReceiveTx::markAsDuplicate() 
{
    // Don't mark as dup if didn't get buffer first time through
    if (m_nState != TX_WAIT) 
	{
        m_bDuplicate = true;
    }
}

boolean LtReceiveTx::isDuplicate() 
{
    boolean dup = m_bDuplicate;
    m_bDuplicate = false;
    return dup;
}

byte* LtReceiveTx::getResponse() 
{
    return m_byResponse;
}

int LtReceiveTx::getResponse(byte*& response)
{
	response = m_byResponse; 
	return m_nResponseLength; 
}

LtErrorType LtReceiveTx::registerResponse(byte response[], int length, boolean nullResponse, boolean bRespondOnFlexDomain) 
{
	LtErrorType err = LT_NO_ERROR;
    if (nullResponse) 
	{
        m_nState = TX_DONE;
		err = LT_NO_MESSAGE;
    } 
	else 
	{
        m_nState = TX_ACK;
        memcpy(m_byResponse, response, length);
		m_nResponseLength = length;
		setPendingType(LT_RESPONSE);
        if (bRespondOnFlexDomain)
        {
            setDomainIndex(FLEX_DOMAIN_INDEX);    
        }
    }
	return err;
}

void LtReceiveTx::setChallenge() {
    m_nState = TX_AUTHENTICATING;
    long time = clock();
    for (int i = 0; i < (int)sizeof(m_byChallenge); i++) {
        m_byChallenge[i] = rand() + m_byLastChallenge[i] + (byte) time;
    }
	memcpy(m_byLastChallenge, m_byChallenge, sizeof(m_byChallenge));
}

void LtReceiveTx::done() {
    m_nState = TX_DONE;
}

boolean LtReceiveTx::resendNeeded() {
    return m_nState != TX_DONE && m_nState != TX_PENDING;
}

void LtReceiveTx::expiration() {
    if (m_nState != TX_WAIT_FOR_DELETION) {
        m_nState = TX_IDLE;
    }
	// Delete incoming apdu in case it never got delivered.
	delete getApdu();
	setApdu(null);
}

boolean LtReceiveTx::authenticating() {
    return m_nState == TX_AUTHENTICATING;
}

