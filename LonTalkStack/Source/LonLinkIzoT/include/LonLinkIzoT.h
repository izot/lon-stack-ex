/***************************************************************
 *  Filename: LonLinkIzoT.h
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
 *  Description:  Header file for the IzoT LonLink class
 *
 ****************************************************************/

#if !defined(LONLINKIZOT_H)
#define LONLINKIZOT_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "LonLink.h"
#include "VxSockets.h"
#include <assert.h>
#include "LtDomain.h"
#include "LtRouteMap.h"

#ifndef WIN32
#include <stdint.h>
#include <stdio.h>
#endif

#define MAX_ETH_PACKET 1500
#define MAX_UDP_PACKET MAX_LPDU_SIZE

#define LON_LINK_IZOT_DEFUALT_AGING_INTERVAL (5*60*1000) // 5 minutes

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   LonLinkIzoTLock
//  Summary:
//      This utilty class is used to lock a critical section.  
//      It unlocks automatically upon destruction.
//
///////////////////////////////////////////////////////////////////////////////
class LonLinkIzoTLock
{
public:
    LonLinkIzoTLock(SEM_ID lockId, bool lockIt = true) 
    { 
        m_lock = lockId; 
        if (lockIt)
        {
            lock();
        }
    }
    ~LonLinkIzoTLock() { unlock(); }

    void lock(void) 
    {
        semTake(m_lock, -1 /* not used for mutex */);
        m_locked = true;
    }

    void unlock(void) 
    {
        if (m_locked)
        {
            m_locked = false;
            semGive(m_lock);
        }
    }
    static SEM_ID create(void) { return semMCreatez(SEM_Q_FIFO); }
private:
    bool   m_locked;
    SEM_ID m_lock;
};

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
class LonLinkIzoTSocketsRef
{
public:
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
    LonLinkIzoTSocketsRef(int numEntries, SEM_ID lock, LonLinkIzoTSocketsRef *p = NULL);
    ~LonLinkIzoTSocketsRef();

        // Get the socket at the specified index, or return INVALID_SOCKET
    VXSOCKET getSocket(int index);
        // Set the socket at the specified index.  Index must be < m_numEntries;
    void setSocket(int index, VXSOCKET socket);

        // Get pointer to the socket map.
    VXSOCKET *getSockets(void) { return m_sockets; }

        // Get the number of entries in the socket map.
    int getNumEntries(void)    { return m_numEntries; }

        // Add a reference to the socket map. 
     void addRef(void);

        // Release the socket reference.  This may delete this object.
     void release(void);

private:
    int          m_numEntries;  // Number of entries in the socket array
    VXSOCKET    *m_sockets;     // The soket array
    int          m_useCount;    // Number of users of the array.
    SEM_ID       m_lock;        // Lock used to manage the socket arra
};


///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   LonLinkIzoTSockets
//  Summary:
//      This utilty class is used to maintain a map of sockets. The number of
//      entries can grow dynamically.
//
///////////////////////////////////////////////////////////////////////////////
class LonLinkIzoTSockets
{
public:

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
    LonLinkIzoTSockets(int numEntries, int reallocSize, SEM_ID lock);
    ~LonLinkIzoTSockets();

        // Get the number of sockets in the map.
    int getNumSockets();

        // Get the socket at the index or INVALID_SOCKET
    VXSOCKET getSocket(int index);

        // Access sockets.  This adds a refernce count, so the caller must release
        // the reference when they are done with it.
    LonLinkIzoTSocketsRef *getSocketRef(void);

        // Add a socket to the map, growing the map as necessary.  Return the index
        // of the socket in the map.
    int addSocket(VXSOCKET socket);

        // Close the socket at the socket index.
    void closeSocket(int index);

        // Close all sockets.
    void closeAll(void);

        // Close all sockets in preparation for deleting this object.  This must be
        // the last method called on this object until its destructor.
    void closeAllAndDelete(void);

private:
    int                    m_reallocSize;           // Number of entries to add when the socket array grows
	LonLinkIzoTSocketsRef *m_sockets;				// My IP-C sockets
    SEM_ID                 m_lock;                  // The lock protecting the uses of these sockets.
};

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   LonLinkIzoT
//  Summary:
//      The baseclass for a LonLink supporting an "IzoT" (LS/IP) network interface.  
//      Derived classes may support routers or application devices.
//
///////////////////////////////////////////////////////////////////////////////

class LonLinkIzoT : public LonLink
{
public:

    /******************************************************************************
      LonLinkIzoT constructor
      Parameters:
        numEntries:             The initial size of the socket map.
        socketResize:           The number of entries to add when the socket map needs
                                to grow.
    *****************************************************************************/
 	LonLinkIzoT(int numSockets = 4, int socketResize = 4);
	~LonLinkIzoT();

	// Retransmit timer routines
	void transmitTimerRoutine();

    // Process aging timeout, to handle arbitrary IP address aging
	void agingTimerRoutine(void);

    // Update the aging interval
    void updateAgingInterval(ULONG agingInterval);

    ///////////////////////////////////////////////////////////////////////////////
    //
    // The following callback functions are called by the LS to UDP mapping layer.
    //
    ///////////////////////////////////////////////////////////////////////////////

    /******************************************************************************
      Method:  sendAnnouncement for the LS <-> UDP mapping layer
       
      Summary:
        Send an announcement message.  

      Parameters:
        ltV0msg:        The announcement message, in LTV0 format.
        msgLen:         The message length.

    *****************************************************************************/
    virtual void sendAnnouncement(const uint8_t *ltV0msg, uint8_t msgLen) = 0;

    /******************************************************************************
      Method:  getArbitrarySourceAddress
       
      Summary:
        Retrieve arbitrary IP address information for a given source address.  

        The default implementation is for all sources to use LS derived IP
        addresses.

      Parameters:
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
    virtual uint8_t getArbitrarySourceAddress(uint8_t *pSourceIpAddress, 
                                           const uint8_t *pDomainId, int domainIdLen,
                                           uint8_t *pEnclosedSource)
    {
        return 0;
    }

    /******************************************************************************
      Method:  getArbitraryDestAddress
       
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
    virtual uint8_t getArbitraryDestAddress(const uint8_t *pDomainId, uint8_t domainLen, 
                                            uint8_t subnetId, uint8_t nodeId, uint8_t ipv1AddrFmt,
                                            uint8_t *pDestIpAddress, uint8_t *pEnclosedDest);

    /******************************************************************************
      Method:  setArbitraryAddressMapping
       
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
    virtual void setArbitraryAddressMapping(const uint8_t *pArbitraryIpAddr, 
                                            const uint8_t *pDomainId, uint8_t domainLen, 
                                            uint8_t subnetId, uint8_t nodeId);

    /******************************************************************************
      Method:  setDerivedAddressMapping
       
      Summary:
        Inform the LS/IP mapping layers that a given LS address uses an
        LS derived IP address.  

      Parameters:
        pDomainId:              The LS domain ID.
        domainIdLen:            The length (in bytes) of the LS domain ID
        subnetId:               The LS subnet ID
        nodeId:                 The LS node ID

    *****************************************************************************/
    virtual void setDerivedAddressMapping(const uint8_t *pDomainId, uint8_t domainLen, 
                                          uint8_t subnetId, uint8_t nodeId);

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
    void setDerivedSubnetsMapping(const uint8_t *pDomainId, uint8_t domainLen, 
                                  uint8_t set, const uint8_t *pSubnets);

    /******************************************************************************
      Method:  isUnicastAddressSupported
       
      Summary:
        This callback is used by the ls to udp translation layers to 
        determmine whether or not the specified IP address can be used by this
        device as a source address.

        The default implementation indicates all sources are supported, and is 
        appropriate for a router.

      Parameters:
        ipAddress: The ip address in question.

    *****************************************************************************/
    virtual bool isUnicastAddressSupported(const uint8_t *ipAddress) { return true; }

    virtual LtSts setUnicastAddress(int stackIndex, int domainIndex, int subnetNodeIndex,
                            byte *domainId, int domainLen, byte subnetId, byte nodeId) = 0;
    virtual void deregisterStack(int stackIndex) = 0;
    virtual LtSts updateGroupMembership(int stackIndex, int domainIndex, LtGroups &groups) = 0;
    virtual int queryIpAddr(LtDomain &domain, byte subnetId, byte nodeId, byte *ipAddress) = 0;
    virtual void setLsAddrMappingConfig(int stackIndex,
                                ULONG lsAddrMappingAnnounceFreq,
                                WORD lsAddrMappingAnnounceThrottle,
                                ULONG lsAddrMappingAgeLimit) = 0;

protected:

    ///////////////////////////////////////////////////////////////////////////
    // LonLink methods and variables
    ///////////////////////////////////////////////////////////////////////////

	virtual boolean		isOpen()
	{	
        return m_isOpen;
	}

    static LonLinkIzoT *_instance;

    int             m_isOpen;

	// Platform-specific driver function overrides
	virtual void driverClose();
	virtual LtSts driverRegisterEvent();
	virtual void driverReceiveEvent();
	virtual void startDelayedRetransmit();
	
    // Need to override these to fake out the rest of the system - because
    // there is no neuron.
	virtual LtSts getTransceiverRegister(int n);
	virtual LtSts setCommParams(const LtCommParams& commParams);
	virtual LtSts getCommParams(LtCommParams& commParams);
	virtual LtSts getNetworkBuffers(LtReadOnlyData& readOnlyData);
	virtual LtSts setNetworkBuffers(LtReadOnlyData& readOnlyData);
    virtual void wakeReceiveTask();

	WDOG_ID				m_wdTimer;			// timer for transmits


   ///////////////////////////////////////////////////////////////////////////
    // Locking
    ///////////////////////////////////////////////////////////////////////////
    SEM_ID m_lock;                  // A lock used to protect this class and its 
                                    // data structures.  Also used by derived 
                                    // classes.


    ///////////////////////////////////////////////////////////////////////////////
    // Socket management
    ///////////////////////////////////////////////////////////////////////////////
    LonLinkIzoTSockets m_sockets;

    ///////////////////////////////////////////////////////////////////////////////
    // LS/IP Mapping 
    ///////////////////////////////////////////////////////////////////////////////

    /******************************************************************************
      Method:  findLsIpMapSubnetInfo
       
      Summary:
        Find the IzoTLsIpMappingSubnetInfo object for a given LS domain.  Optionaly
        create one if necessary.

      Parameters:
        pDomainId:   The LS domain ID.

      Return:
        A pointer to the IzoTLsIpMappingSubnetInfo object.
    *****************************************************************************/
    class IzoTLsIpMappingSubnetInfo *findLsIpMapSubnetInfo(
        const uint8_t *pDomainId, int domainIdLen, bool createIfNotFound);

        // List of IzoTLsIpMappingSubnetInfo objects used to determine whether a
        // specified LS address corresponds to a derived IP address or an
        // arbitrary IP address.
    class IzoTLsIpMappingSubnetInfo *m_pLsIpMapHead;

    ///////////////////////////////////////////////////////////////////////////
    // Arbitrary IP Address Aging
    ///////////////////////////////////////////////////////////////////////////

 	WDOG_ID				m_agingTimer;		    // timer for aging arbitrary address assignments
    int                 m_agingInterval;        // Aging interval
    int                 m_agingTimerEnabled;    // True if aging is enabled
    
        // Start the aging timer
    void startAgingTimer(void);
};


#endif	// LONLINKIZOT_H
