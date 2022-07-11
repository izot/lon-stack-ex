/***************************************************************
 *  Filename: LonLinkIzoT.cpp
 *
 * Copyright © 2022 Dialog Semiconductor
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in 
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *  Description:  Implementation of the LonLinkIzoT class.
 *		This is the IzoT derivation of the LonLink class
 *
 ****************************************************************/
#include "LtaDefine.h"
#if FEATURE_INCLUDED(IZOT)

#ifdef WIN32
#include <windows.h>
#endif
#include <string.h>
#include <assert.h>

#include "LonTalk.h"
#include "LonLinkIzoT.h"
#include "IzoTLsIpMapping.h"

#include "VxSockets.h"
#include "LtCUtil.h"
#include "vxlTarget.h"

extern "C"
{
#include "ipv6_ls_to_udp.h"
}

/*
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
*/

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   LonLinkIzoT
//  Summary:
//      The baseclass for a LonLink supporting an "IzoT" (LS/IP) network interface.  
//      Derived classes may support routers or application devices.
//
///////////////////////////////////////////////////////////////////////////////

/******************************************************************************
  LonLinkIzoT constructor
  Parameters:
    numEntries:             The initial size of the socket map.
    socketResize:           The number of entries to add when the socket map needs
                            to grow.
*****************************************************************************/
LonLinkIzoT::LonLinkIzoT(int numSockets, int socketResize) : m_lock(LonLinkIzoTLock::create()), m_sockets(numSockets, socketResize, m_lock)
{
    m_isOpen = false;
    m_wdTimer = NULL;
    m_agingTimer = wdCreate();
    assert( m_agingTimer != NULL );
    m_agingTimerEnabled = false;
    m_bufferConfiguration.setNetworkInputBuffers(255, 2);
    m_bufferConfiguration.setNetworkOutputBuffers(255, 2, 2);  
    m_pLsIpMapHead = NULL;
    m_agingInterval = LON_LINK_IZOT_DEFUALT_AGING_INTERVAL;  // REMINDER:  This should be configurable.
    startAgingTimer();
}

LonLinkIzoT::~LonLinkIzoT()
{
	stopReceiveTask();

    driverClose();
    wdCancel( m_agingTimer );
    wdDelete( m_agingTimer );

	if ( m_wdTimer )
	{
        // Delete the retry timer
		wdCancel( m_wdTimer );
		wdDelete( m_wdTimer );
		m_wdTimer = NULL;
	}

    // Delete the LS/IP mapping table
    IzoTLsIpMappingSubnetInfo *pNext;
    for (IzoTLsIpMappingSubnetInfo *p = m_pLsIpMapHead; p != NULL; p = pNext)
    {
        pNext = p->getNext();
        delete p;
    }

    // Delete the sockets. Need to do this explicity rather than relying on the destructor
    // because it has to be done before deleting the lock
    m_sockets.closeAllAndDelete();
    semDelete(m_lock);
}

///////////////////////////////////////////////////////////////////////////
// LonLink methods 
///////////////////////////////////////////////////////////////////////////
void LonLinkIzoT::driverClose()
{
    wdCancel( m_agingTimer );
    LonLinkIzoTLock lock(m_lock);
    m_agingTimerEnabled = false;
    m_sockets.closeAll();
    m_isOpen = false;
}

LtSts LonLinkIzoT::driverRegisterEvent()
{
    // Nothing needed for this, but we need to define it because LonLink defines this as a pure virtual function
    return LTSTS_OK;
}

// Wait for any socket to have data
void LonLinkIzoT::driverReceiveEvent()
{
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;      // Wake up periodically just in case

    // Get a reference to the sockets so that we can pass the array to vxsSelectAnyRead.
    // Note that if new sockets are added while we are waiting, LonLinkIzoT will
    // release its reference to the sockets contained in m_sockets, but since we
    // have a reference to the map it won't get deleted until we release it.
    LonLinkIzoTSocketsRef *pSocketRef = m_sockets.getSocketRef();

    // Wait for any socket activity or a timeout.
    vxsSelectAnyRead( pSocketRef->getSockets(), pSocketRef->getNumEntries(), &timeout);

    // Release the socket reference, since we are done with it.
    pSocketRef->release();
}

//
// lonLinkIzoTTimer
//
// C entry point transmitTimerRoutine 
//
static int lonLinkIzoTTimer( int a1 )
{
	LonLinkIzoT*	pLink = (LonLinkIzoT*) a1;
	pLink->transmitTimerRoutine();
	return 0;
}

//
// transmitTimerRoutine
//
// Try repeatedly to transmit something if we get a transmit full condition.
//
void	LonLinkIzoT::transmitTimerRoutine()
{
	LLPktQue*	pPkt;
	LtQue*		pItem;
	LtSts		sts = LTSTS_OK;

	lock();

	while ( m_qTransmit.removeHead( &pItem ) )
	{
		pPkt = (LLPktQue*)pItem;

		sts = tryTransmit( pPkt->m_refId, pPkt->m_pktFlags, pPkt->m_pData, pPkt->m_nDataLength );
		if ( sts == LTSTS_QUEUEFULL )
		{
			m_qTransmit.insertHead( pPkt );
			break;
		}
		else
		{
			// Done with this packet - return it to the owner.
			unlock();
			m_pNet->packetComplete( pPkt->m_refId, sts );
			lock();
		}

		freeLLPkt(pPkt);
	}

	m_bDelayedRetransmitPending = false;

	if ( sts == LTSTS_QUEUEFULL || !m_qTransmit.isEmpty() )
	{	startDelayedRetransmit();
	}

	unlock();
}

//
// startDelayedRetransmit
//
// Start the timer to try again on a transmit
//
void	LonLinkIzoT::startDelayedRetransmit()
{
	STATUS	vxSts;
	if ( ! m_bDelayedRetransmitPending )
	{
		if ( m_wdTimer == NULL )
		{
			m_wdTimer = wdCreate();
			assert( m_wdTimer != NULL );
		}
		vxSts = wdStart( m_wdTimer, 20, lonLinkIzoTTimer, (int)this );
		m_bDelayedRetransmitPending = true;
	}
}

void	LonLinkIzoT::wakeReceiveTask()
{
    if (m_bExitReceiveTask)
    {
        driverClose();  // Close the driver so that the reciever wakes up.
    }
}

// STUBS

LtSts LonLinkIzoT::getTransceiverRegister(int n)
{
    return LtLinkBase::getTransceiverRegister(n);
}

// void  LonLinkIzoT::reset() {}
LtSts LonLinkIzoT::setCommParams(const LtCommParams& commParams)
{
    m_bActive = true;
	m_commParams = commParams;
	return LTSTS_OK;
}
LtSts LonLinkIzoT::getCommParams(LtCommParams& commParams)
{
	commParams = m_commParams;
	return LTSTS_OK;
}

LtSts LonLinkIzoT::getNetworkBuffers(LtReadOnlyData& readOnlyData)
{
    readOnlyData.setNetworkInputBuffers(m_bufferConfiguration.getNetInBufSize(), m_bufferConfiguration.getNumNetInBufs());
    readOnlyData.setNetworkOutputBuffers(m_bufferConfiguration.getNetOutBufSize(), m_bufferConfiguration.getNumNetOutBufs(0), m_bufferConfiguration.getNumNetOutBufs(1));    
	return LTSTS_OK;
}

LtSts LonLinkIzoT::setNetworkBuffers(LtReadOnlyData& readOnlyData)
{
    m_bufferConfiguration.setNetworkInputBuffers(readOnlyData.getNetInBufSize(), readOnlyData.getNumNetInBufs());
    m_bufferConfiguration.setNetworkOutputBuffers(readOnlyData.getNetOutBufSize(), readOnlyData.getNumNetOutBufs(0), readOnlyData.getNumNetOutBufs(1));    
	return LTSTS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// LS/IP Mapping 
///////////////////////////////////////////////////////////////////////////////

/******************************************************************************
  Method:  findLsIpMapSubnetInfo
   
  Summary:
    Find the IzoTLsIpMappingSubnetInfo object for a given LS domain.  Optionally
    create one if necessary.

  Parameters:
    pDomainId:   The LS domain ID.

  Return:
    A pointer to the IzoTLsIpMappingSubnetInfo object.
*****************************************************************************/
IzoTLsIpMappingSubnetInfo *LonLinkIzoT::findLsIpMapSubnetInfo(const uint8_t *pDomainId, int domainIdLen, bool createIfNotFound)
{
    IzoTLsIpMappingSubnetInfo *p;
    
    byte id[LT_DOMAIN_LENGTH];
    memcpy(id, pDomainId, domainIdLen);
    if (domainIdLen == 3)
    {
        id[2] = 0;
    }

    LtDomain domain(id, domainIdLen);
    for (p = m_pLsIpMapHead; p != NULL; p = p->getNext())
    {
        if (domain == p->GetDomain())
        {
            break;
        }
    }

    if (p == NULL && createIfNotFound)
    {
        p = new IzoTLsIpMappingSubnetInfo(domain);
        p->link(m_pLsIpMapHead);
        m_pLsIpMapHead = p;
    }
    return p;
}

///////////////////////////////////////////////////////////////////////////
// Arbitrary IP Address Aging
///////////////////////////////////////////////////////////////////////////

// Process aging timeout, to handle arbitrary IP address aging
void	LonLinkIzoT::agingTimerRoutine(void)
{
    LonLinkIzoTLock lock(m_lock);

    if (m_agingTimerEnabled)
    {
        IzoTLsIpMappingSubnetInfo *p;
    
        // vxlReportEvent("Aging Timer Expired...\n");
        for (p = m_pLsIpMapHead; p != NULL; p = p->getNext())
        {
            p->agingTimerExpired();
        }
        startAgingTimer();
    }
}

// C entry point for agingTimerRoutine 
static int lonLinkIzoTAgingTimer( int a1 )
{
    LonLinkIzoT*	pLink = (LonLinkIzoT*) a1;
	pLink->agingTimerRoutine();
	return 0;
}


// Start the aging timer
void	LonLinkIzoT::startAgingTimer(void)
{
    // Cancel the timer before taking the lock.  Otherwise
    // we could take the lock, then the timer routine wait on
    // the lock, and any timer updates would deadlock.
    wdCancel(m_agingTimer);  
    LonLinkIzoTLock lock(m_lock);

    if (m_agingInterval)
    {
	    STATUS	vxSts;
        m_agingTimerEnabled = true;
	    vxSts = wdStart( m_agingTimer, msToTicksX(m_agingInterval), lonLinkIzoTAgingTimer, (int)this );
    }
}

// Update the aging interval
void LonLinkIzoT::updateAgingInterval(ULONG agingInterval)
{
    LonLinkIzoTLock lock(m_lock);

    agingInterval = agingInterval/2;  // Check at half the frequency, and age it if it fails two times
    if (agingInterval != m_agingInterval)
    {
        m_agingInterval = agingInterval;
        startAgingTimer();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// The following callback functions are called by the LS to UDP mapping layer.
//
///////////////////////////////////////////////////////////////////////////////

/******************************************************************************
  Function:  getArbitraryDestAddress
   
  Summary:
    Retrieve arbitrary IP address information for a given destination address.  

  Parameters:
    pDomainId:              The LS domain ID.
    domainIdLen:            The length (in bytes) of the LS domain ID
    subnetId:               The LS destination subnet ID
    nodeId:                 The LS destination node ID
    ipv1AddrFmt:            The LS/IP address format
    pDestIpAddress:         Pointer to a buffer to receive the destination IP
                            address to be used.
    pEnclosedDest:          Pointer to a buffer to receive additional LS
                            destination address information enclosed in the
                            PDU, if any.
  Return: 
    The length of the additional enclosed destination address information
*****************************************************************************/
uint8_t LonLinkIzoT::getArbitraryDestAddress(const uint8_t *pDomainId, uint8_t domainLen, 
                                             uint8_t subnetId, uint8_t nodeId, uint8_t ipv1AddrFmt,
                                             uint8_t *pDestIpAddress, uint8_t *pEnclosedDest)
{
    uint8_t useDerivedAddress = false;
    void *pArbitraryIpAddr = null;

    LonLinkIzoTLock lock(m_lock);

    // Find the map for this domain.  If it doesn't exist, we don't know anything
    // about the destination.
    IzoTLsIpMappingSubnetInfo *p = findLsIpMapSubnetInfo(pDomainId, domainLen, false);
    if (p != NULL)
    {
        if (p->getLsDerivedIpAddr(subnetId, nodeId & 0x7f))
        {
            useDerivedAddress = true;
        }
        else
        {
            pArbitraryIpAddr = p->getArbitraryIpAddr(subnetId, nodeId & 0x7f);
            if (pArbitraryIpAddr)
            {
                // REMINDER IPV6 support...
                // Copy the arbitrary address as the actual destination IP address
               memcpy(pDestIpAddress, pArbitraryIpAddr, IPV4_ADDRESS_LEN);
            }
        }
    }

    if (!useDerivedAddress)
    {
        // Not using a derived address, so we need to include the destination LS
        // address in the payload.
        pEnclosedDest[0] = subnetId;
        pEnclosedDest[1] = nodeId & 0x7f;
        if (ipv1AddrFmt == IPV6_LSUDP_NPDU_ADDR_FMT_GROUP_RESP)
        {
            pEnclosedDest[1] |= 0x80;
        }

        if (pArbitraryIpAddr == NULL)
        {
            // It's not an arbitrary address, so we need to use the subnet broadcast 
            // address.
            ipv6_gen_ls_mc_addr(LT_AF_BROADCAST, 
#if UIP_CONF_IPV6
                                pDomainId, domainLen, 
#endif
                                subnetId, pDestIpAddress);

        }
    }

    return !useDerivedAddress;
}

/******************************************************************************
  Function:  setArbitraryAddressMapping
   
  Summary:
    Inform the LS/IP mapping layers that a given LS address uses an
    arbitrary IP address.  

  Parameters:
    pArbitraryIpAddr:       The arbitrary IP address to use when addressing
                            the LS device.
    pDomainId:              The LS domain ID.
    domainIdLen:            The length (in bytes) of the LS domain ID
    subnetId:               The LS subnet ID
    nodeId:                 The LS node ID

*****************************************************************************/
void LonLinkIzoT::setArbitraryAddressMapping(const uint8_t *pArbitraryIpAddr, 
                                              const uint8_t *pDomainId, uint8_t domainLen, 
                                              uint8_t subnetId, uint8_t nodeId)
{
    LonLinkIzoTLock lock(m_lock);
    IzoTLsIpMappingSubnetInfo *p = findLsIpMapSubnetInfo(pDomainId, domainLen, true);
    if (p != NULL)
    {
        p->setArbitraryIpAddr(subnetId, nodeId, pArbitraryIpAddr);
    }
}

/******************************************************************************
  Function:  setDerivedAddressMapping
   
  Summary:
    Inform the LS/IP mapping layers that a given LS address uses an
    LS derived IP address.  

  Parameters:
    pDomainId:              The LS domain ID.
    domainIdLen:            The length (in bytes) of the LS domain ID
    subnetId:               The LS subnet ID
    nodeId:                 The LS node ID

*****************************************************************************/
void LonLinkIzoT::setDerivedAddressMapping(const uint8_t *pDomainId, uint8_t domainLen, 
                                           uint8_t subnetId, uint8_t nodeId)
{
    LonLinkIzoTLock lock(m_lock);
    IzoTLsIpMappingSubnetInfo *p = findLsIpMapSubnetInfo(pDomainId, domainLen, true);
    if (p != NULL)
    {
        p->setLsDerivedIpAddr(subnetId, nodeId, true);
    }
}

/******************************************************************************
  Function:  setDerivedSubnetsMapping
   
  Summary:
    This callback is used by the ls to udp translation layers when an 
    SubnetsAddrMapping message is received.

  Parameters:
    pDomainId:              The LS domain ID.
    domainIdLen:            The length (in bytes) of the LS domain ID
    set:                    True to set the derived mapping entries, clear to
                            clear the dervived mapping entries.
    pSubneteMap:            Pointer to a bit map of subnets to set or clear.

*****************************************************************************/
void LonLinkIzoT::setDerivedSubnetsMapping(const uint8_t *pDomainId, uint8_t domainLen, 
                              uint8_t set, const uint8_t *pSubnets)
{
    LonLinkIzoTLock lock(m_lock);
    IzoTLsIpMappingSubnetInfo *p = findLsIpMapSubnetInfo(pDomainId, domainLen, true);
    if (p != NULL)
    {
        p->setLsDerivedIpSubnets(set, pSubnets);
    }
}

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   LonLinkIzoTSocketsRef
//  Summary:
//      This utilty class is used to maintain a refernce to a map of sockets.
//      This allows a socket map to grow (by reallocating a new map and copying)
//      even if the map is in use (for example, the reader thread is waiting
//      on a socket event.
//
///////////////////////////////////////////////////////////////////////////////

    /******************************************************************************
      Method:  LonLinkIzoTSocketsRef
       
      Summary:
        LonLinkIzoTSocketsRef constructor. Allocates numEntries sockets, and
        optionally copies sockets from another socket reference.

      Parameters:
        numEntries: The number of entries in the socket array
        lock:       A lock to protect this data.
        p:          Optional pointer to a LonLinkIzoTSocketsRef.  If non-null
                    the sockets contained in p will be copied to the new
                    LonLinkIzoTSocketsRef.

    *****************************************************************************/
LonLinkIzoTSocketsRef::LonLinkIzoTSocketsRef(int numEntries, SEM_ID lock, LonLinkIzoTSocketsRef *p)
{
    m_numEntries = numEntries;
    m_lock = lock;
    m_sockets = new VXSOCKET[numEntries];   // Allocate the socket map

    int i = 0;
    if (p != NULL)
    {
        // Supplied a reference.  Copy the sockets in that one to this one.
        while (i < p->getNumEntries())
        {
            m_sockets[i++] = p->getSocket(i);
        }
    }

    while(i < m_numEntries)
    {
        // Set the remaining sockets to INVALID_SOCKET
	    m_sockets[i++] = INVALID_SOCKET;
    }

    // Set refrence count to 0.
    m_useCount = 0;
}


LonLinkIzoTSocketsRef::~LonLinkIzoTSocketsRef()
{
    delete[] m_sockets;
}

// Get the socket at the specified index, or return INVALID_SOCKET
VXSOCKET LonLinkIzoTSocketsRef::getSocket(int index)
{
    if (index < m_numEntries)
    {
        return m_sockets[index];
    }
    return INVALID_SOCKET;
}

// Set the socket at the specified index.  Index must be < m_numEntries;
void LonLinkIzoTSocketsRef::setSocket(int index, VXSOCKET socket)
{
    if (index < m_numEntries)
    {
        m_sockets[index] = socket;
    }
}

// Add a reference to the socket map. 
void LonLinkIzoTSocketsRef::addRef(void)
{
    LonLinkIzoTLock lock(m_lock);
    m_useCount++;
}

// Release the socket reference and delete it if 0.
void LonLinkIzoTSocketsRef::release(void)
{
    {
        LonLinkIzoTLock lock(m_lock);
        m_useCount--;
    }

    if (m_useCount == 0)
    {
        delete this;
    }
 }

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   LonLinkIzoTSockets
//  Summary:
//      This utilty class is used to maintain a map of sockets. The number of
//      entries can grow dynamically.
//
///////////////////////////////////////////////////////////////////////////////

/******************************************************************************
  Method:  LonLinkIzoTSockets
   
  Summary:
    LonLinkIzoTSockets constructor. Allocates a LonLinkIzoTSocketsRef with
    numEntries, and adds a reference to it.

  Parameters:
    numEntries:             The initial size of the socket map.
    reallocSize:            The number of entries to add when the socket map needs
                            to grow.
    lock:                   A lock to protect this data.

*****************************************************************************/
LonLinkIzoTSockets::LonLinkIzoTSockets(int numEntries, int reallocSize, SEM_ID lock)
{
    m_reallocSize = reallocSize;
    m_lock = lock;
    m_sockets = new LonLinkIzoTSocketsRef(numEntries, lock);

    // We have a refernce to the sockets - so do an addref.  Will be released
    // when the socket map is resized or on destruction.
    m_sockets->addRef();
}

LonLinkIzoTSockets::~LonLinkIzoTSockets()
{
    closeAllAndDelete();
}

// Get the number of sockets in the map.
int LonLinkIzoTSockets::getNumSockets()
{
    LonLinkIzoTLock lock(m_lock);
    return m_sockets->getNumEntries();
}

// Get the socket at the index or INVALID_SOCKET
VXSOCKET LonLinkIzoTSockets::getSocket(int index)
{
    LonLinkIzoTLock lock(m_lock);
    return m_sockets->getSocket(index);
}

// Access sockets.  This adds a refernce count, so the caller must release
// the reference when they are done with it.
LonLinkIzoTSocketsRef *LonLinkIzoTSockets::getSocketRef(void)
{
    LonLinkIzoTLock lock(m_lock);
    m_sockets->addRef();
    return m_sockets;
}

// Add a socket to the map, growing the map as necessary.  Return the index
// of the socket in the map.
int LonLinkIzoTSockets::addSocket(VXSOCKET socket)
{
    LonLinkIzoTLock lock(m_lock);   // Lock it
    int index;

    // See if there is a free socket.
    for (index = 0; index < m_sockets->getNumEntries() && m_sockets->getSocket(index) != INVALID_SOCKET; index++)
    {        
    }
    if (index == m_sockets->getNumEntries())
    {
        // No free sockets.  Need to create a new, larger socket map.  

        // Get a new larger socket map and copy the entries from the current map.
        LonLinkIzoTSocketsRef *pNewSockets = new LonLinkIzoTSocketsRef(m_sockets->getNumEntries()+m_reallocSize, m_lock, m_sockets);

        // Add a reference to the newly created socket map
        pNewSockets->addRef();
        // Release the old socket map. Note that the receive task may still be using it, in which case
        // it won't be deleted until it releases its reference.
        m_sockets->release();

        // Switch to the new map.
        m_sockets = pNewSockets;
    }

    // At this point, index is an available socket index, so set the socket value.
    m_sockets->setSocket(index, socket);
    return index;
}

// Close the socket at the socket index.
void LonLinkIzoTSockets::closeSocket(int index)
{
    LonLinkIzoTLock lock(m_lock);  // Lock
    if (index < m_sockets->getNumEntries() && m_sockets->getSocket(index) != INVALID_SOCKET)
    {
    	vxlReportEvent("Close socket[%d] = %d\n", index, m_sockets->getSocket(index));
        vxsCloseSocket(m_sockets->getSocket(index));
        m_sockets->setSocket(index, INVALID_SOCKET);
    }
}

// Close all sockets.
void LonLinkIzoTSockets::closeAll(void)
{
    LonLinkIzoTLock lock(m_lock);   // Lock
    for (int i = 0; i < m_sockets->getNumEntries(); i++)
    {
        closeSocket(i);
    }
}

// Close all sockets in preparation for deleting this object.  This must be
// the last method called on this object until its destructor.
void LonLinkIzoTSockets::closeAllAndDelete(void)
{
    if (m_sockets != NULL)
    {
        // Close all the sockets
        closeAll(); 

        // Release the socket map
        m_sockets->release();

        // Mark it as deleted.
        m_sockets = NULL;
    }
    // Can't use the lock anymore.
    m_lock = NULL;
}

//////////////////////////////////////////////////////////////////////////////
//
//       C Interfaces for Arbitrary IP Mapping Callback Functions
//
//////////////////////////////////////////////////////////////////////////////

/******************************************************************************
  Function:  ipv6_send_announcement
   
  Summary:
    This callback is used to send an announcement message.  

  Parameters:
    lsSenderHandle: A handle to the callback object that implements the send 
                    function.
    ltV0msg:        The announcement message, in LTV0 format.
    msgLen:         The message length.

*****************************************************************************/
void ipv6_send_announcement(void *lsMappingHandle, const uint8_t *ltV0msg, uint8_t msgLen)
{
    ((LonLinkIzoT *)lsMappingHandle)->sendAnnouncement(ltV0msg, msgLen);
}

/******************************************************************************
  Function:  ipv6_get_arbitrary_source_address
   
  Summary:
    This callback is used to retrieve arbitrary IP address information 
    for a given source address.  

  Parameters:
    lsMappingHandle:        A handle used for LS mapping 
    pSourceIpAddress:       On input, a pointer the desired (LS derived) source 
                            IP address.  If this IP address cannot be used, 
                            pSourceIpAddress will be updated with the arbitrary
                            IP address to be used instead.
    pDomainId:              The LS domain ID.
    domainIdLen:            The length (in bytes) of the LS domain ID
    pEnclosedSource:        Pointer to a buffer to receive the necessary LS
                            source addressing information (in V1 format) to be 
                            added to the UDP payload, if any
  Return: 
    The length of the additional enclosed source address information

*****************************************************************************/
uint8_t ipv6_get_arbitrary_source_address(void *lsMappingHandle,
                                          uint8_t *pSourceIpAddress, 
                                          const uint8_t *pDomainId, int domainIdLen,
                                          uint8_t *pEnclosedSource)
{
    return ((LonLinkIzoT *)lsMappingHandle)->
        getArbitrarySourceAddress(pSourceIpAddress, pDomainId, domainIdLen, pEnclosedSource);
}

/******************************************************************************
  Function:  ipv6_get_arbitrary_dest_address
   
  Summary:
    This callback is used to used by the ls to udp translation layers to retrieve 
    arbitrary IP address information for a given destination address.  

  Parameters:
    lsMappingHandle:        A handle used for LS mapping 
    pDomainId:              The LS domain ID.
    domainIdLen:            The length (in bytes) of the LS domain ID
    subnetId:               The LS destination subnet ID
    nodeId:                 The LS destination node ID
    ipv1AddrFmt:            The LS/IP address format
    pDestIpAddress:         Pointer to a buffer to receive the destination IP
                            address to be used.
    pEnclosedDest:          Pointer to a buffer to receive additional LS
                            destination address information enclosed in the
                            PDU, if any.
  Return: 
    The length of the additional enclosed destination address information
*****************************************************************************/
uint8_t ipv6_get_arbitrary_dest_address(void *lsMappingHandle,
                                        const uint8_t *pDomainId, uint8_t domainLen, 
                                        uint8_t subnetId, uint8_t nodeId, uint8_t ipv1AddrFmt,
                                        uint8_t *pDestIpAddress, uint8_t *pEnclosedDest)
{
    return ((LonLinkIzoT *)lsMappingHandle)->
        getArbitraryDestAddress(pDomainId, domainLen, subnetId, nodeId, ipv1AddrFmt,
                                pDestIpAddress, pEnclosedDest);
}

/******************************************************************************
  Function:  ipv6_set_arbitrary_address_mapping
   
  Summary:
    This callback is used by the ls to udp translation layers to 
    inform the LS/IP mapping layers that a given LS address uses an
    arbitrary IP address.  

  Parameters:
    lsMappingHandle:        A handle used for LS mapping 
    pArbitraryIpAddr:       The arbitrary IP address to use when addressing
                            the LS device.
    pDomainId:              The LS domain ID.
    domainIdLen:            The length (in bytes) of the LS domain ID
    subnetId:               The LS subnet ID
    nodeId:                 The LS node ID

*****************************************************************************/
void ipv6_set_arbitrary_address_mapping(void *lsMappingHandle, const uint8_t *pArbitraryIpAddr, 
                                         const uint8_t *pDomainId, uint8_t domainLen, 
                                         uint8_t subnetId, uint8_t nodeId)
{
    ((LonLinkIzoT *)lsMappingHandle)->setArbitraryAddressMapping(pArbitraryIpAddr, pDomainId, domainLen, subnetId, nodeId);
}

/******************************************************************************
  Function:  ipv6_set_derived_address_mapping
   
  Summary:
    This callback is used by the ls to udp translation layers to 
    inform the LS/IP mapping layers that a given LS address uses an
    LS derived IP address.  

  Parameters:
    lsMappingHandle:        A handle used for LS mapping 
    pDomainId:              The LS domain ID.
    domainIdLen:            The length (in bytes) of the LS domain ID
    subnetId:               The LS subnet ID
    nodeId:                 The LS node ID

*****************************************************************************/
void ipv6_set_derived_address_mapping(void *lsMappingHandle, 
                                      const uint8_t *pDomainId, uint8_t domainLen, 
                                      uint8_t subnetId, uint8_t nodeId)
{
    ((LonLinkIzoT *)lsMappingHandle)->setDerivedAddressMapping(pDomainId, domainLen, subnetId, nodeId);
}

/******************************************************************************
  Function:  ipv6_set_derived_subnets_mapping
   
  Summary:
    This callback is used by the ls to udp translation layers when an 
    SubnetsAddrMapping message is received.

  Parameters:
    lsMappingHandle:        A handle used for LS mapping 
    pDomainId:              The LS domain ID.
    domainIdLen:            The length (in bytes) of the LS domain ID
    set:                    True to set the derived mapping entries, clear to
                            clear the dervived mapping entries.
    pSubneteMap:            Pointer to a bit map of subnets to set or clear.

*****************************************************************************/
void ipv6_set_derived_subnets_mapping(void *lsMappingHandle, 
                                      const uint8_t *pDomainId, uint8_t domainLen, 
                                      uint8_t set, const uint8_t *pSubnets)
{
    ((LonLinkIzoT *)lsMappingHandle)->setDerivedSubnetsMapping(pDomainId, domainLen, set, pSubnets);
}

/******************************************************************************
  Function:  ipv6_is_unicast_address_supported
   
  Summary:
    This callback is used by the ls to udp translation layers to 
    determmine whether or not the specified IP address can be used by this
    device as a source address.

  Parameters:
    lsMappingHandle:        A handle used for LS mapping 
    ipAddress:              The LS domain ID.

*****************************************************************************/
uint8_t ipv6_is_unicast_address_supported(void *lsMappingHandle, const uint8_t *ipAddress)
{
    return ((LonLinkIzoT *)lsMappingHandle)->isUnicastAddressSupported(ipAddress);
}

#endif
