/***************************************************************
 *  Filename: LonLinkIzoTDev.h
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
 *  Description:  Header file for the LonLinkIzoTDev used for
 *                devices that connect to the network via
 *                LS/IP (the IzoT protocol). 
 *
 *                This class is intended for use by one or more
 *                application devices.  The class uses UDP sockets 
 *                with dedicated addresses, and therefore cannot be 
 *                used for layer2 functions such as routing.
 *
 ****************************************************************/

#if !defined(LONLINKIZOTDEV_H)
#define LONLINKIZOTDEV_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "LonLinkIzoT.h"
#include "IzoTDevSocketMaps.h"

#define LON_LINK_IZOT_DEV_REBIND_TIMEOUT_MIN 5000
#define LON_LINK_IZOT_DEV_REBIND_TIMEOUT_MAX (5*60*1000)

#define LON_LINK_IZOT_DEV_MC_SOCKET_INDEX 0   // Multicast is always on socket index 0.

#ifdef WIN32
#define LON_LINK_IZOT_DEV_FIRST_SEND_SOCKET_INDEX  0
#else
// For non Windows system, socket index 0 is only used for receiving the multicast msg
// which is bound to INADDR_ANY
#define LON_LINK_IZOT_DEV_FIRST_SEND_SOCKET_INDEX  (LON_LINK_IZOT_DEV_MC_SOCKET_INDEX+1)
#endif

// Time between consecutive announcements 
#define LON_LINK_IZOT_DEV_ANNOUNCEMENT_THROTTLE 500   

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   LonLinkIzoTDev
//  Summary:
//      A LonLink for use by application devices using an "IzoT" (LS/IP) 
//      network interface.  
//
///////////////////////////////////////////////////////////////////////////////

class LonLinkIzoTDev : public LonLinkIzoT
{
    friend int lonLinkIzoTRebindTimer( int a1 );

public:
    /******************************************************************************
      LonLinkIzoTDev constructor
      Parameters:
        LonLinkIzoTDev:     The name of the IzoT network interface
        szIpIfName:         The name of the IP interface
        ipManagementOptions:    A bitmap of LONLINK_IZOT_MGMNT_OPTION_* options.
                                See IzoTNiConfig.h.
        connEventHWnd:          WIN32 windows handle used for connection events.
        connEventTag:           Tag use with connection events (WIN32 only)
    *****************************************************************************/
    LonLinkIzoTDev(const char* szIzoTName, const char *szIpIfName, int ipManagementOptions,
            HANDLE connEventHWnd=0, int connEventTag=0 /* Connection event parameters */
        );

    ~LonLinkIzoTDev();

    ///////////////////////////////////////////////////////////////////////////
    // LonLink methods
    ///////////////////////////////////////////////////////////////////////////

	virtual boolean		isOpen()
	{	
        return m_isOpen;
	}

    ///////////////////////////////////////////////////////////////////////////////
    // Socket Management
    ///////////////////////////////////////////////////////////////////////////////


    /******************************************************************************
      method:  setUnicastAddress
       
      Summary:
        Set the unicast address used by the particular stack, domain index and 
        subent/node index.  Each unicast address is keyed by the triple
        stackIndex, domainIndex, subnetNodeIndex.  This function attempts to
        bind a socket to an IP address derived from the domainID/subnetId/nodeID, and
        joins the appropriate multicast addresses used for domain and subnet broadast.

      Parameters:
        stackIndex:         The device stacks appIndex
        domainIndex:        The domain index.
        subnetNodeIndex:    The subnet/node index.  Usually 0, but the stack
                            supports alternate subnet/node IDs on a given domain
        pDomainId:          The LS domain ID.
        domainIdLen:        The length (in bytes) of the LS domain ID
        subnetId:           The LS subnet ID
        nodeId:             The LS node ID

    *****************************************************************************/
    LtSts setUnicastAddress(int stackIndex, int domainIndex, int subnetNodeIndex, 
                            byte *domainId, int domainLen, byte subnetId, byte nodeId);


     /******************************************************************************
      method:  deregisterStack
       
      Summary:
        Deregister the stack from the link, and close the appropriate sockets,
        and leave the appropriate multicast groups.

      Parameters:
        stackIndex:         The device stacks appIndex
    *****************************************************************************/
    void deregisterStack(int stackIndex);

    ///////////////////////////////////////////////////////////////////////////////
    // Multicast Membership
    ///////////////////////////////////////////////////////////////////////////////

    /******************************************************************************
      method:  updateGroupMembership
       
      Summary:
        Update the LS group membership information for the specifed stack/domain.
        This may add or remove multicast groups from the "any address" socket.

      Parameters:
        stackIndex:         The device stacks appIndex
        domainIndex:        The domain index.
        groups:             A bitmap of groups in the domain that the stack
                            belongs to.
    *****************************************************************************/
    LtSts updateGroupMembership(int stackIndex, int domainIndex, LtGroups &groups);

    ///////////////////////////////////////////////////////////////////////////////
    // Rebinding 
    ///////////////////////////////////////////////////////////////////////////////

    // Attempt to rebind if a binding failure occured.  Enabled only if the
    // ipManagementOptions LONLINK_IZOT_MGMNT_OPTION_RETRY_BINDING option is set.
    void rebindTimerRoutine();

    ///////////////////////////////////////////////////////////////////////////
    // Arbitrary IP Address Announcements
    ///////////////////////////////////////////////////////////////////////////

    // Process an announce timeout.
	void announceTimerRoutine(void);

    /******************************************************************************
      method:  setLsAddrMappingConfig
       
      Summary:
        Update the timers used for announcements and aging.

      Parameters:
        stackIndex:         The device stacks appIndex
        lsAddrMappingAnnounceFreq:  Frequency to make announcements
        lsAddrMappingAnnounceThrottle:  Minimum time between consecutive announcements
        lsAddrMappingAgeLimit:      Expiration period for arbtrary address mapping
    *****************************************************************************/
    void setLsAddrMappingConfig(int stackIndex, 
                                ULONG lsAddrMappingAnnounceFreq, 
                                WORD lsAddrMappingAnnounceThrottle, 
                                ULONG lsAddrMappingAgeLimit);

    ///////////////////////////////////////////////////////////////////////////
    //
    // The following callback functions are called by the LS to UDP mapping layer.
    //
    ///////////////////////////////////////////////////////////////////////////

    /******************************************************************************
      Method:  sendAnnouncement for the LS <-> UDP mapping layer
       
      Summary:
        Send an announcement message.  

      Parameters:
        ltVxmsg:        The announcement message, in LTV0 or LTV2 format.
        msgLen:         The message length.

    *****************************************************************************/
    void sendAnnouncement(const uint8_t *ltVxmsg, uint8_t msgLen);


    /******************************************************************************
      Method:  getArbitrarySourceAddress
       
      Summary:
        Retrieve arbitrary IP address information for a given source address.  

        This implementation uses arbitrary IP addresses unless the original
        pSourceIpAddress is currently bound to a socket.

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
    uint8_t getArbitrarySourceAddress(uint8_t *pSourceIpAddress, 
                                      const uint8_t *pDomainId, int domainIdLen,
                                      uint8_t *pEnclosedSource);

     /******************************************************************************
      Method:  isUnicastAddressSupported
       
      Summary:
        This callback is used by the ls to udp translation layers to 
        determmine whether or not the specified IP address can be used by this
        device as a source address.

        This impelementation returns true if the IP address is currently bound
        to a socket.

      Parameters:
        ipAddress: The ip address in question.

    *****************************************************************************/
    bool isUnicastAddressSupported(const uint8_t *ipAddress);

    /******************************************************************************
  	  Method:  queryIpAddr
       
      Summary:
        Return the IP address that this device would use when sending a message
        with the specified source address.

        This impelementation returns true if the IP address is currently bound
        to a socket.

      Parameters:
        domain:     The source domain ID
        subnetId:   The source subnet ID
        nodeId:     The source node ID
        ipAddress:  The ip address that will be used.

      Return:   The size of the IP address in bytes.

    *****************************************************************************/
    int queryIpAddr(LtDomain &domain, byte subnetId, byte nodeId, byte *ipAddress);

    ///////////////////////////////////////////////////////////////////////////
    // Protocol Analyzer support routines
    ///////////////////////////////////////////////////////////////////////////

    /******************************************************************************
  	  Method:  registerProtocolAnalyzerCallback

      Summary:
        This routine is used by the application to register the callback routine
        to hook protocol analyzer data sink.

      Parameters:
        fn:     The callback routine, passing the data packet and the length

      Return:   None

    *****************************************************************************/
    void registerProtocolAnalyzerCallback(void (*fn)(char*, int));

    /******************************************************************************
  	  Method:  unregisterProtocolAnalyzerCallback

      Summary:
        This routine is used by the application to un-register the callback routine
        to hook protocol analyzer data sink.

      Parameters:
        None

      Return:   None

    *****************************************************************************/
   	void unregisterProtocolAnalyzerCallback();

   	static LonLinkIzoTDev* getInstance();
protected:

    ///////////////////////////////////////////////////////////////////////////
    //  LonLink methods and variables
    ///////////////////////////////////////////////////////////////////////////

	// Platform-specific driver function overrides
	virtual LtSts driverOpen(const char* pName);
	virtual void driverClose();
	virtual LtSts driverRead(void *pData, short len);
	virtual LtSts driverWrite(void *pData, short len);
	virtual void sendToProtocolAnalyser(byte* pData, bool crcIncluded = true);

    ///////////////////////////////////////////////////////////////////////////////
    // General Interface variables
    ///////////////////////////////////////////////////////////////////////////////
    
    char *m_szIzoTName;             // Name of the IzoT interface
    char *m_szIpIfName;             // Name of the IP interface
    int  m_ipManagementOptions;     // Bitmap of LONLINK_IZOT_MGMNT_OPTION_* options

    ///////////////////////////////////////////////////////////////////////////////
    // Socket management
    ///////////////////////////////////////////////////////////////////////////////

    // Bind the socket at socketIndex to the unicastAddresses address att
    // socketIndex.  rebind is true if we are retrying this binding operation.
    LtSts bindUnicastAddress(int socketIndex, bool rebind);

    // Release the unicast address at the associated socketIndex
    void releaseUnicastAddress(int socketIndex);

    // Find the next socket to read, and return the socket and the
    // socket index.  If no socket has any available data, return INVALID_SOCKET 
    VXSOCKET SelectSocketToRead(int *socketIndex);

    // Return the socketIndex corresponding to the specified pSourceAddress.
    // Return INVALID_SOCKET if there is no socket successfully bound to that
    // address.
    int SelectSourceSocket(const uint8_t *pSourceAddress);

     // Collection of unicast addresses, indexed by socket index.  The first
    // is the "any" IP address.  Others are added as necessary derived from
    // LS addresses.  Note that these addresses are created even if binding to
    // them fails. 
    IzotUnicastAddresses m_unicastAddresses;

    // A map used to find domain, subnet and node configuration, including
    // group membership and the socketIndex associated with the LS address
    IzoTDevConfigMap m_devConfigMap;

   // The socket index of the last socket that was read.  Sockets are read
    // in round robin fashon
    int m_lastSocketRead;

    ///////////////////////////////////////////////////////////////////////////////
    // Multicast Membership
    ///////////////////////////////////////////////////////////////////////////////

    // Update subnet or group multicast membership based on the old and new
    // subnet or group membership.
    LtSts updateMcMembership(int type, LtRouteMap &currentMap, LtRouteMap newMap);

    // Rebuild LS group membership for the link by scanning the group membership
    // of all the domains of all the devices using the link.
    LtSts  updateLsGroupMembership(void);

    // Rebuild LS subnet membership for the link by scanning the subnet membership
    // of all the domains of all the devices using the link.
    LtSts  updateLsSubnetMembership(void);

    // Add or remove the multicast address for a specfic group or subnet.
    LtSts updateMcAddress(bool add, byte type, byte subnetOrGroup);

    LtGroups m_groups;      // All groups devices are members of.
    LtSubnets m_subnets;    // All subnets devices are members of.

    ///////////////////////////////////////////////////////////////////////////////
    // Rebinding 
    ///////////////////////////////////////////////////////////////////////////////

    // Start the rebinding timer.
    void startDelayedRebinding();

#ifdef WIN32
    // Windows connection event parameters
    HANDLE m_connEventHWnd;     
    int m_connEventTag;
#endif
    
    WDOG_ID	m_rebindTimer;			// timer for rebinding in case of error
    int     m_rebindTimeout;        // time to wait befor rebinding. Uses exponetial backoff.  
    bool    m_bDelayedRebindPending; // Rebinding is pending.
    bool    m_bInRebindTimerRoutine; // True if rebind rebindTimerRoutine
    bool    m_bRebindTimerEnabled;  // True if rebind timer is enabled.

    ///////////////////////////////////////////////////////////////////////////
    // Arbitrary IP Address Announcements
    ///////////////////////////////////////////////////////////////////////////

    // Start the announce timer.  If we are in the middle of announcements set timer to
    // m_announceThrottle, otherwise set to m_announceInterval adjusted for any delay
    // caused by throttling previous announcements (m_announceDelay).
    void startAnnounceTimer(void);

    // Send a broadcast announcement for the specified socket.
    void sendBroadcastAnnouncement(int socketIndex);

    // Update the LS address mapping configuration for the link based on the configuration
    // of each device.
    void updateLsAddrMappingConfig(void);

	WDOG_ID				m_announceTimer;		// timer for arbitrary address announcements
    ULONG               m_announceInterval;     // announce interval.  Start announcing all arbitrary
                                                // addresses used by app devices using this link.
    ULONG               m_announceThrottle;     // time between consecutive announcements
    ULONG               m_announceDelay;        // Amount of time spent announcing due to throttle
    int                 m_announceTimerEnabled; // True if announcements are enabled
    int                 m_announceIndex;        // Index of the next (potential) announcement
    
    // Protocol Analyzer callback routine
    void (*m_ProtocolAnalyserFn)(char*, int);

    bool                m_isRNI;            // True if use for RNI on Ethernet
    static LonLinkIzoTDev *m_instance;

    ///////////////////////////////////////////////////////////////////////////////
    // Debugging 
    ///////////////////////////////////////////////////////////////////////////////

    // Trace data
    void dumpData(const char *title, const void *pData, short len);
};


#endif	// LONLINKIZOTDEV_H
