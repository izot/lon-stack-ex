//
// LtChannel.cpp
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

#ifdef WIN32
#include <windows.h>
#endif
#include "LtRouter.h"

#if FEATURE_INCLUDED(LONLINK)
#include "LtLinkDefault.h"
#endif

#if FEATURE_INCLUDED(IP852)
#include <LtIpMaster.h>
#endif
#include <tickLib.h>
#include "LtMip.h"
#include "VxLayer.h"

#include "LtFailSafeFile.h"
#if FEATURE_INCLUDED(IZOT)
#include "LonLinkIzoTDev.h"
#include "LonLinkIzoTLtLink.h"
#endif
#include "LtDomainConfiguration.h"
#include "LtMisc.h"
#include "LtAddressConfiguration.h"
#include "LtOutgoingAddress.h"
#include "LtNetworkVariable.h"
#include "LtNetworkVariableConfiguration.h"
#include "LtPlatform.h"

#if FEATURE_INCLUDED(L5MIP)
LtErrorType VXLCDECL lonInit(void *const szPort, void** ppInitBlock, int ldvHandle);
void VXLCDECL lonClose(void** ppInitBlock);
#endif

LtChannelVector LtLogicalChannel::m_channels;

boolean LtLogicalChannel::getLogicalChannel(boolean& bFirst, int& nIndex, LtLogicalChannel **ppResult)
{
	LtVectorPos pos;
	boolean result;
	if (!bFirst)
	{
		pos = LtVectorPos(nIndex);
	}
	result = m_channels.getElement(pos, ppResult);
	bFirst = false;
	nIndex = pos.getIndex();
	return result;
}

// 
// Create a channel with an LRE of NULL to create a channel that
// is not connected to a router.  If a channel is connected to a 
// router, create the LRE explicitly and specify it to each of the
// two channels.  Or create the first channel with no LRE and then
// use that channel's LRE (getLre()) when creating the subsequent
// channel(s).  Note there might be multiple channels in the case
// of a router/proxy combination or in an N-way router.
//
LtLogicalChannel::LtLogicalChannel(boolean bIpPort, LtLonTalkChannel* pLonTalk, LtLreServer* pLreServer)
	: m_pLonTalk(pLonTalk), m_bIpChannel(bIpPort) 
{ 
	m_startError = LT_NO_ERROR;

	if (m_pLonTalk == NULL)
	{
	    m_pLonTalk = new LtLonTalkChannel();
		m_bCreated = true;
	}
	else
	{
		m_bCreated = false; 
	}
	m_channels.addElement(this);
	m_bCreatedServer = false;
	m_nClientsPending = 0;

	if (pLreServer == null)
	{
		m_bCreatedServer = true;
		pLreServer = LtLreServer::create();
		pLreServer->start();
	}
	// Only call base method - derived method may do more.
	LtLogicalChannel::setLre(pLreServer);

	m_xcvrId = 30;	// default is custom
}

LtLogicalChannel::~LtLogicalChannel()
{
	if (m_bCreatedServer)
	{	// EPR 18932. Stop the server before deleting the channel - otherwise the engine
		// may reference deleted items.
		m_pLreServer->stop();
	}
	if (m_bCreated)
	{
		delete m_pLonTalk;
	}
	if (m_bCreatedServer)
	{
		delete m_pLreServer;
		m_pLreServer = null;
	}
	m_channels.removeElement(this);
}

LtLtLogicalChannel *LtLogicalChannel::getLtLtLogicalChannel(void)
{
    return m_bIpChannel ? NULL : (LtLtLogicalChannel *)this;
}

LtLreServer* LtLogicalChannel::getLre()
{
	return m_pLreServer;
}

void LtLogicalChannel::setLre(LtLreServer* pLre)
{
	m_pLreServer = pLre;
}

// register stack clients that know the routing info
void LtLogicalChannel::registerStackClient( LtLreClient* pClient )
{
	boolean bNotify;

	lock();
	if ( ! m_vStackClients.isElement( pClient ) )
	{	m_vStackClients.addElement( pClient );
	}
	bNotify = m_nClientsPending <= 1;
	if (m_nClientsPending) m_nClientsPending--;
	unlock();

	if (bNotify)
	{
		// We don't notify until all the clients which were created
		// have registered.  This avoids notifications while still
		// in a state of flux.
		notify();
	}
}

// deregister a stack client
void LtLogicalChannel::deregisterStackClient( LtLreClient* pClient )
{
	lock();
	m_vStackClients.removeElement( pClient );
	unlock();
	notify();
}

// enumerate the stack clients
// call lock() before and unlock() after a string of
// calls to the following to preserve integrity
boolean LtLogicalChannel::enumStackClients(LtVectorPos& vectorPos, LtLreClient** ppValue)
{
	LtObject* pObject;
	boolean bResult;
	bResult = m_vStackClients.getElement( vectorPos, &pObject);
	*ppValue = (LtLreClient*) pObject;
	return bResult;
}

LtLtLogicalChannel::LtLtLogicalChannel(const char* szName, LtLreServer* pLre, LtLink* pLink, boolean bForceL2) : LtLogicalChannel(false, null, pLre)
{
#ifdef WIN32
    m_openTimeout = 10000;
    m_responseTimeout = 1000;
    m_openRetries = 0;

    m_pRegParms = null;
#endif
    init(szName, pLink, bForceL2);
}

#if FEATURE_INCLUDED(IZOT)
LtLtLogicalChannel::LtLtLogicalChannel(const char* szName, LtLreServer* pLre, LtLink* pLink, boolean bForceL2, boolean bIsIzoTDevChannel) : LtLogicalChannel(false, null, pLre)
{
#ifdef WIN32
    m_openTimeout = 10000;
    m_responseTimeout = 1000;
    m_openRetries = 0;

    m_pRegParms = null;
#endif
    init(szName, pLink, bForceL2, bIsIzoTDevChannel);
}
#endif

LtLtLogicalChannel::LtLtLogicalChannel(const char* szName, int maxChannelPackets, int receiveQueueDepth, int transmitQueueDepth) : LtLogicalChannel(false, null, null)
{
#ifdef WIN32
    m_openTimeout = 10000;
    m_responseTimeout = 1000;
    m_openRetries = 0;

    m_pRegParms = null;
#endif
    init(szName, null, false, false, maxChannelPackets, receiveQueueDepth, transmitQueueDepth);
}

#ifdef WIN32
LtLtLogicalChannel::LtLtLogicalChannel(const char* szName, LtLtLogicalChannelRegParms *pRegParms) 
                                       : LtLogicalChannel(false, null, null)
{
    m_pRegParms = pRegParms;
    pRegParms->getInterfaceTimeouts(m_openTimeout, m_responseTimeout, m_openRetries);
    init(szName, null, false, false);
}
#endif

#if FEATURE_INCLUDED(IZOT)

// This constructor is used to create an LonLinkIzoTDev channel
LtLtLogicalChannel::LtLtLogicalChannel(const char* szIzoTName, const char *szIpIfName, int ipManagementOptions
#ifdef WIN32
									   , LtLtLogicalChannelRegParms *pRegParms
#endif
									   ) : LtLogicalChannel(false, null, null)
{
	HANDLE hWnd = 0;
    int tag = 0;

#ifdef WIN32
    m_pRegParms = pRegParms;

    if (pRegParms)
    {
        pRegParms->getLdvEventRegistrationParameters(hWnd, tag);
    }
#endif

    LtLink* pLink = new LonLinkIzoTDev(szIzoTName, szIpIfName, ipManagementOptions, hWnd, tag);

    init(szIzoTName, pLink, true, true);
}

#ifndef WIN32
// This constructor is used to create an LonLinkIzoTLtLink channel
LtLtLogicalChannel::LtLtLogicalChannel(const char* szName, int ipManagementOptions) : LtLogicalChannel(false, null, null)
{
    HANDLE hWnd = 0;
    int tag = 0;

    LtLink* pLink = new LonLinkIzoTLtLink();
    init(szName, pLink, true, true);
}
#endif
#endif

void LtLtLogicalChannel::init(const char* szName, LtLink* pLink, 
		boolean bForceL2, boolean bIsIzoTDevChannel, int maxChannelPackets, int receiveQueueDepth, 
        int transmitQueueDepth) 
{
	LtErrorType err = LT_NO_ERROR;
#if FEATURE_INCLUDED(LONLINK)    
	int xcvrId;
#endif
	int ldvHandle = -1;

    m_szPort = NULL;

	m_pAllocator = null;
	m_pLtPortClient = null;
	m_pInitBlock = null;
	m_pLonLink = null;

#ifdef WIN32
    m_bConnectionEstablished = false;
#endif

    m_bIsIzoTDevChannel = bIsIzoTDevChannel;
#if FEATURE_INCLUDED(LONLINK)    
	if (pLink == null)
	{
		// Create a LonTalk link for this channel.
        if (m_bIsIzoTDevChannel)
        {
		    err = LT_INVALID_PARAMETER;
        }
        else
        {
#if FEATURE_INCLUDED(IZOT) && !defined(WIN32)
			err = LT_INVALID_PARAMETER;
#else
		    m_pLonLink = (LtLinkDefault*) LtLinkDefault::createInstance();
#endif
        }
	}
	else
	{
		m_pLonLink = pLink;
	}

    {   // Fill in the name
		char szPort[300];
	    if (szName == null)
	    {
		    if (m_pLonLink->enumInterfaces( 0, szPort, sizeof(szPort) ))
		    {
			    szName = szPort;
		    }
            else
            {
                szName = "";
            }
	    }
        m_szPort = new char[strlen(szName) + 1];
		strcpy(m_szPort, szName);
    }

#ifdef WIN32
    m_openTime = tickGet();
#endif
    xcvrId = -1;
    if (err == LT_NO_ERROR)
    {
	    if (bForceL2)
	    {
		    m_minLayer = 2;
		    m_maxLayer = 2;
		    m_bNsa = false;
	    }
	    else
	    {
		    // Determine the nature of the interface.
		    err = determineMipType(this, m_minLayer, m_maxLayer, m_bNsa, xcvrId, ldvHandle);
	    }
	    if (xcvrId == -1)
	    {
		    // Get it from the link
		    xcvrId = m_pLonLink->getStandardTransceiverId();
	    }
    }

	setXcvrId(xcvrId);
	setDisplayName("LonTalk");

	if (err == LT_NO_ERROR && m_szPort[0] != 0)
	{
		LtCommParams	cp;

		// Read comm params so that link layer can initialize its
		// cached copy.
		m_pLonLink->getCommParams(cp);

		if (isLayer5Interface())
		{
			// We don't need the "link" anymore.
			((LtLinkDefault*) m_pLonLink)->destroyInstance();
			m_pLonLink = null;

#if FEATURE_INCLUDED(L5MIP)
			err = lonInit(m_szPort, &m_pInitBlock, ldvHandle);
#endif
		}
		else
		{
			// Now create the LonTalk port client for the server
			// Create an allocator
			m_pAllocator = new LtpcPktAllocator();

			// Note that the allocator count must allow for allocation
			// of buffers to the LONC receive queues and must also allow
			// for cloning of messages in the engine (both deep and 
			// shallow).  

			// Number of packets allows for aggregation on all clients
			// but the dominating factor is line speed so allow for a high speed
			// line.  Number includes priority and non-priority queues.
			m_pAllocator->init(MAX_LINK_PKT_SIZE, 20, 10, maxChannelPackets);
			// Need more message refs than buffers to allow for cloning.
			m_pAllocator->initMsgRefs(20, 10, maxChannelPackets*2 );

			// Create and set up LonTalk Port Client
			m_pLtPortClient = new LtIpPortClient(this);
			m_pLtPortClient->setName(m_szPort);
			m_pLtPortClient->registerLink(*m_pLonLink);

#ifdef WIN32
            // Get LDVX event registration information to be passed to the open routine.  
            if (ldvHandle != -1)
            {
                m_pLtPortClient->registerLdvHandle(ldvHandle);
            }
#endif

			m_pLtPortClient->setAllocator(m_pAllocator);
			// Currently this is set to a fairly large number mainly because the layer2 test
			// requires that a bunch of packets be sent at one time.
			m_pLtPortClient->setTransmitQueueDepth(transmitQueueDepth);
			m_pLtPortClient->setReceiveQueueDepth(receiveQueueDepth);
		}
	}
	setStartError(err);
	// Since base class initialization doesn't call my derived method, I'll do it now explicitly.
	setLre(getLre());
#endif
}

LtLtLogicalChannel::~LtLtLogicalChannel()
{
#if FEATURE_INCLUDED(L5MIP)
	lonClose(&m_pInitBlock);
#endif
	getLre()->deregisterClient(m_pLtPortClient);
	if (m_pAllocator != NULL)
		delete m_pAllocator;
	if (m_pLtPortClient != NULL)
		delete m_pLtPortClient;
#if FEATURE_INCLUDED(LONLINK)
	if (m_pLonLink)
	{
		((LtLinkDefault*) m_pLonLink)->destroyInstance();
	}
#endif
	if (m_szPort != NULL)
		delete m_szPort;
}

boolean LtLtLogicalChannel::getConnectionTerminated(void)
{
#ifdef WIN32
    if (m_pRegParms != NULL)
    {
        return m_pRegParms->getConnectionTerminated();
    }
#endif
    return FALSE;
}

void LtLtLogicalChannel::setLre(LtLreServer* pServer)
{
	if (m_pLtPortClient)
	{
		m_pLtPortClient->setServer(pServer);
		pServer->registerClient(m_pLtPortClient, true);
	}
	LtLogicalChannel::setLre(pServer);
}

LtErrorType LtLtLogicalChannel::getStartError()
{
	return LtLogicalChannel::getStartError();
}

void LtLtLogicalChannel::getL5Control(void** ppInitBlock)
{
	*ppInitBlock = m_pInitBlock;
}

#ifdef WIN32

void LtLtLogicalChannel::connectionEstablished()
{
    m_bConnectionEstablished = true;
}
boolean LtLtLogicalChannel::openPending()
{
    return !m_bConnectionEstablished && ((m_openTime+((ULONG)m_openTimeout)) > tickGet());
}

void LtLtLogicalChannel::getInterfaceTimeouts(int &openTimeout, int &responseTimeout, int &openRetries)
{
    openTimeout = m_openTimeout;
    responseTimeout = m_responseTimeout;
    openRetries = m_openRetries;
}

boolean LtLtLogicalChannel::getRegBufferSizes(int &maxSicbData)
{
    return m_pRegParms ? m_pRegParms->getRegBufferSizes(maxSicbData) : false;
}

boolean LtLtLogicalChannel::getRegAdvancedTxTickler(int &advancedTxTickler)
{
    return m_pRegParms ? m_pRegParms->getRegAdvancedTxTickler(advancedTxTickler) : false;
}

boolean LtLtLogicalChannel::getRegNmVersion(int &nmVersion, int &nmCapabilities)
{
    return m_pRegParms ? m_pRegParms->getRegNmVersion(nmVersion, nmCapabilities) : false;
}

void LtLtLogicalChannel::getRegSupportsEncryption(boolean &supportsEncryption)
{
    if (m_pRegParms)
    {
        m_pRegParms->getRegSupportsEncryption(supportsEncryption);
    }
}

void LtLtLogicalChannel::getLdvEventRegistrationParameters(HANDLE &hWnd, int &tag)
{
    hWnd = NULL;
    tag = 0;
    if (m_pRegParms)
    {
        m_pRegParms->getLdvEventRegistrationParameters(hWnd, tag);
    }
}

#endif

#if FEATURE_INCLUDED(IZOT)
LonLinkIzoT *LtLtLogicalChannel::getLonLinkIzoTDev()
{
    return m_bIsIzoTDevChannel ? (LonLinkIzoT *)m_pLonLink : NULL;
}
#endif

LtErrorType LtLtLogicalChannel::setUnicastAddress(int stackIndex, int domainIndex, int subnetNodeIndex, LtDomainConfiguration* pDc)
{
    LtErrorType err = LT_NO_ERROR;

#if FEATURE_INCLUDED(IZOT)
    // If the link exists, is open, and its an IzoT link, set the unicast address.
    // Otherewise do nothing (and fail silently).
    if (m_pLonLink != NULL && m_pLonLink->isOpen())
    {
        LonLinkIzoT *pLink = getLonLinkIzoTDev();
        if (pLink != NULL)
        {
            LtSubnetNode subnetNode;

            byte id[LT_DOMAIN_LENGTH];  
            if (pDc->getSubnetNode(subnetNodeIndex, subnetNode) == LT_NO_ERROR)
            {
                pDc->getDomain().getAll(id);
                if (subnetNode.getNode() != 255)
                {
                    if (pLink->setUnicastAddress(stackIndex, domainIndex, subnetNodeIndex, id, pDc->getDomain().getLength(), 
                                                 subnetNode.getSubnet(), subnetNode.getNode()) != LTSTS_OK)
                    {
                        err = LT_INVALID_IPADDRESS;
                    }
                }
            }
        }
    }
#endif
    return err;
}

LtErrorType LtLtLogicalChannel::updateGroupMembership(int stackIndex, int domainIndex, LtGroups &groups)
{
    LtErrorType err = LT_NO_ERROR;

#if FEATURE_INCLUDED(IZOT)
    // If the link exists, is open, and its an IzoT link, set the unicast address.
    // Otherewise do nothing (and fail silently).
    if (m_pLonLink != NULL && m_pLonLink->isOpen())
    {
        LonLinkIzoT *pLink = getLonLinkIzoTDev();
        if (pLink != NULL)
        {
            if (pLink->updateGroupMembership(stackIndex, domainIndex, groups) != LTSTS_OK)
            {
                err = LT_INVALID_IPADDRESS;
            }
        }
    }
#endif
    return err;
}

void LtLtLogicalChannel::setLsAddrMappingConfig(int stackIndex, 
                                                ULONG lsAddrMappingAnnounceFreq, 
                                                WORD lsAddrMappingAnnounceThrottle,
                                                ULONG lsAddrMappingAgeLimit)
{
#if FEATURE_INCLUDED(IZOT)
    // If the link exists, is open, and its an IzoT link, set the unicast address.
    // Otherewise do nothing (and fail silently).
    if (m_pLonLink != NULL && m_pLonLink->isOpen())
    {
        LonLinkIzoT *pLink = getLonLinkIzoTDev();
        if (pLink != NULL)
        {
            pLink->setLsAddrMappingConfig(stackIndex, lsAddrMappingAnnounceFreq, 
                                          lsAddrMappingAnnounceThrottle, 
                                          lsAddrMappingAgeLimit);
        }
    }
#endif
}

#if FEATURE_INCLUDED(IZOT)
// Query the IP address that this device would use when sending a message with
// the specified source address
int LtLtLogicalChannel::queryIpAddr(LtDomain &domain, byte subnetId, byte nodeId, byte *ipAddress)
{
    if (m_pLonLink != NULL && m_pLonLink->isOpen())
    {
        LonLinkIzoT *pLink = getLonLinkIzoTDev();
        if (pLink != NULL)
        {
            return pLink->queryIpAddr(domain, subnetId, nodeId, ipAddress);
        }
    }
    return 0;
}
#endif

void LtLtLogicalChannel::deregisterStack(int stackIndex)
{
#if FEATURE_INCLUDED(IZOT)
    // If the link exists, is open, and its an IzoT link, set the unicast address.
    // Otherewise do nothing (and fail silently).
    if (m_pLonLink != NULL && m_pLonLink->isOpen())
    {
        LonLinkIzoT *pLink = getLonLinkIzoTDev();
        if (pLink != NULL)
        {
            pLink->deregisterStack(stackIndex);
        }
    }
#endif
}

void LtLogicalChannel::stackClientCreated()
{
	m_nClientsPending++;
}

void LtLogicalChannel::allStacksCreated()
{
	if (m_nClientsPending == 0)
	{
		notify();
	}
}

//
// Call this to clean up resources allocated globally
//
void ltShutdown()
{
	vxlShutdown();
    #if PERSISTENCE_TYPE_IS(STANDARD)
	LtFailSafeFile::Shutdown();
    #endif
    LtNetworkVariableConfigurationBase::ltShutdown();
    LtPlatform::ltShutdown();
}
