/***************************************************************
 *  Filename: LonLinkIzoTDev.cpp
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
 *  Description:  Implementation of the LonLinkIzoTDev class.
 *                This class is intended for use by one or more
 *                application devices.  The class uses UDP sockets 
 *                with dedicated addresses, and therefore cannot be 
 *                used for layer2 functions such as routing.
 *
 ****************************************************************/

#include "LtaDefine.h"
#if FEATURE_INCLUDED(IZOT)
#ifdef WIN32
#include <windows.h>
#else
#include <stdlib.h>
#include <arpa/inet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <sys/ioctl.h>
#include <net/if.h>
#define SOCKET_ERROR    -1  // In Win32, this is defined in WinSock.h
#define DEFAULT_ETH_ADAPTER "eth1"
#define SOCKET_ERROR    -1  // In Win32, this is defined in WinSock.h
#endif

#include <string.h>
#include <assert.h>

#include "LonTalk.h"
#include "LonLinkIzoTDev.h"
#include "IzoTNiConfig.h"
#include "VxSockets.h"
#include "LtCUtil.h"
#include "vxlTarget.h"
#include "stdio.h"

#ifdef WIN32
#include "ldv32.h"
#endif

extern "C"
{
#include "ipv6_ls_to_udp.h"
}

#ifndef WIN32
static uint16_t
checksum (uint16_t *addr, int len)
{
	int nleft = len;
	int sum = 0;
	uint16_t *w = addr;
	uint16_t answer = 0;

	while (nleft > 1) {
		sum += *w++;
		nleft -= sizeof (uint16_t);
	}

	if (nleft == 1) {
		*(uint8_t *) (&answer) = *(uint8_t *) w;
		sum += answer;
	}

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	answer = ~sum;
	return (answer);
}
#endif

LonLinkIzoTDev* LonLinkIzoTDev::m_instance = NULL;
LonLinkIzoTDev* LonLinkIzoTDev::getInstance()
{
	return m_instance;
}

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   LonLinkIzoTDev
//  Summary:
//      A LonLink for use by application devices using an "IzoT" (LS/IP) 
//      network interface.  
//
///////////////////////////////////////////////////////////////////////////////

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
LonLinkIzoTDev::LonLinkIzoTDev(const char* szIzoTName, const char *szIpIfName, int ipManagementOptions
                                , HANDLE connEventHWnd, int connEventTag  /* Connection event parameters */
                                ) : LonLinkIzoT(), m_unicastAddresses(m_lock), m_devConfigMap(m_lock)
{
    m_isOpen = false;
    m_lastSocketRead = 0;

    m_rebindTimeout = LON_LINK_IZOT_DEV_REBIND_TIMEOUT_MIN;
    m_szIzoTName = NULL;
    m_szIpIfName = NULL;

    if (szIzoTName != NULL)
    {
        m_szIzoTName = new char[strlen(szIzoTName)+1];
        strcpy(m_szIzoTName, szIzoTName);
    }
    if (szIpIfName)
    {
        m_szIpIfName = new char[strlen(szIpIfName)+1];
        strcpy(m_szIpIfName, szIpIfName);
    }
    m_ipManagementOptions = ipManagementOptions;

    m_rebindTimer = wdCreate();
    assert( m_rebindTimer != NULL );
    m_bDelayedRebindPending = false;
    m_bRebindTimerEnabled = false;
    m_bInRebindTimerRoutine = false;

#ifdef WIN32
    m_connEventHWnd = connEventHWnd;
    m_connEventTag = connEventTag;
#endif  

    m_announceTimer = wdCreate();
    assert( m_announceTimer != NULL );
    m_announceTimerEnabled = false;
    m_announceInterval = m_agingInterval;
    m_announceThrottle = LON_LINK_IZOT_DEV_ANNOUNCEMENT_THROTTLE;   
    m_announceIndex = 0;
    m_announceDelay = 0;

#ifdef WIN32    
    // Delete the unicast registery.  We will add to it as stacks  register
    IzoTDeleteUnicastReg(szIzoTName);
#endif

    m_ProtocolAnalyserFn = NULL;
    m_instance = this;
}

LonLinkIzoTDev::~LonLinkIzoTDev()
{
	wdCancel( m_rebindTimer );
	wdDelete( m_rebindTimer );
	wdCancel( m_announceTimer );
	wdDelete( m_announceTimer );

    // Delete the names
    delete[] m_szIzoTName;
    delete[] m_szIpIfName;

    m_ProtocolAnalyserFn = NULL;
}

int opensocket(void) {
#ifdef WIN32
    return vxsSocket(VXSOCK_DGRAM);
#else 
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("raw socket");
    }
    int on = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
        perror("ip hdr incl");
    }

	if (setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on)) < 0) {
		perror("pkt info");
	}
    return sockfd;
#endif
}

///////////////////////////////////////////////////////////////////////////
// LonLink methods
///////////////////////////////////////////////////////////////////////////
LtSts LonLinkIzoTDev::driverOpen(const char* pName)
{
	LtSts sts = LTSTS_OPENFAILURE;
    int socketIndex;

#ifndef WIN32
    // Open up a socket and bind to INADDR_ANY to use for receiving 
    // multicast messages. 
    // NOTE: For non-Windows systems, when binding a socket for multicast use, 
    // we should always bind to INADDR_ANY.
    // You only specify the multicast interface in the ip_mreq or ip_mreqn struct.
    // Failing to bind to INADDR_ANY breaks multicast
    VXSOCKET socketInAddrAny = opensocket();
    if (socketInAddrAny != INVALID_SOCKET)
    {
        socketIndex = m_sockets.addSocket(socketInAddrAny); 
        VXSOCKADDR inAddrAny = NULL;
        VXSOCKADDR *pInAddrAny = &inAddrAny;     
        if (IzoTBindToAddress(socketInAddrAny, vxsINADDR_ANY, gLsUdpPort, &pInAddrAny))
        {
            m_unicastAddresses.set(socketIndex, inAddrAny);
            
            char szIpAddress[100];
            vxsMakeDottedAddr(szIpAddress, vxsAddrGetAddr(inAddrAny));
            vxlReportEvent("DriverOpen Bind socketIndex=%d to '%s'\n", socketIndex, szIpAddress);

            if (updateLsSubnetMembership() == LTSTS_OK)
            {
                m_unicastAddresses.setIsBound(socketIndex, true);
                sts = LTSTS_OK;
            }
            vxsFreeSockaddr(inAddrAny);
        }    
    } 
    if (sts != LTSTS_OK)
        return (sts);  // Failed on open up a socket and bind to INADDR_ANY to use for receiving
    
    sts = LTSTS_OPENFAILURE;   // re-initialize   
#endif   
    
    // Open up a socket and bind to available address to use for receiving 
    // multicast messages and for use as an arbitrary IP address.
    VXSOCKET socket = opensocket();
    if (socket != INVALID_SOCKET)
    {
        socketIndex = m_sockets.addSocket(socket); 

        int nMaxTry = 1;
        if (m_szIpIfName == NULL)
        {
            // The caller doesn't specify the IP interface
        	// Use any unicast address available
        	m_szIpIfName = new char[strlen(ETH_ADAPTER_NAME)+1];
        	strcpy(m_szIpIfName, ETH_ADAPTER_NAME);
        	// Try to find any unicast address from Local Area Connection first then
        	// Wireless Connection if Local Area Connection is not available
        	nMaxTry = 2;
        	vxlReportEvent("DriverOpen: use any address - '%s'\n", m_szIpIfName);
        }
        while (nMaxTry--)
        {
            VXSOCKADDR localAddr = NULL;
            if (IzoTBindToAvailUnicastAddress(socket, m_szIpIfName, &localAddr))
            {
            	m_unicastAddresses.set(socketIndex, localAddr);
            	if (updateLsSubnetMembership() == LTSTS_OK)
            	{
            		m_unicastAddresses.setIsBound(socketIndex, true);
            		char szIpAddress[100];
            		vxsMakeDottedAddr(szIpAddress, vxsAddrGetAddr(localAddr));
            		vxlReportEvent("DriverOpen Bind socketIndex=%d to '%s'\n", socketIndex, szIpAddress);
            		sts = LTSTS_OK;
            		m_isOpen = TRUE;
            		m_bRebindTimerEnabled = m_ipManagementOptions & LONLINK_IZOT_MGMNT_OPTION_RETRY_BINDING ? true : false;

            	}
                vxsFreeSockaddr(localAddr);
                break;
            }
            else if (nMaxTry)
  	        {
      	        // Try to use a Wireless Connection
                delete[] m_szIpIfName;
                m_szIpIfName = new char[strlen(WIFI_ADAPTER_NAME)+1];
                strcpy(m_szIpIfName, WIFI_ADAPTER_NAME);
                vxlReportEvent("DriverOpen: use a different interface - '%s'\n", m_szIpIfName);
  	        }
        }
    }
    return(sts);
}

void LonLinkIzoTDev::driverClose()
{
    m_unicastAddresses.close(); // Free all the unicast addresses.
    m_devConfigMap.close();     // Free all the device configuration info
    LonLinkIzoT::driverClose(); // Close the base IzoT driver
}

// Read a LS/IP UDP packet and convert it to LTVx
LtSts LonLinkIzoTDev::driverRead(void *pData, short len)
{
	LtSts	sts = LTSTS_ERROR;
    int socketIndex = IZOT_NULL_SOCKET_INDEX;

    // Figure out which socket to read next.  Sockets with data are read round-robin
	VXSOCKET socket = SelectSocketToRead(&socketIndex);

    if ((socket != INVALID_SOCKET) && (socketIndex != IZOT_NULL_SOCKET_INDEX))
    {
        // An LS/IP UDP message has arrived on the socket indexed by socketIndex.
        // Now we need to process it.

#ifndef WIN32
        char            msgBuffer[MAX_ETH_PACKET];
        struct iphdr	*pIpHdr = (struct iphdr*)msgBuffer;
        struct udphdr	*pUdpHdr = (struct udphdr*)(msgBuffer + sizeof(struct iphdr));
        uint8_t 		*udpPayload = (uint8_t*)(msgBuffer + sizeof(struct iphdr) + sizeof(struct udphdr));
#else
        char            udpPayload[MAX_UDP_PACKET];
#endif
        VXSOCKADDR      sourceAddr;   // Source address of last read
        int             udpPayloadLen;

        // allocate a buffer for the source address
        sourceAddr = vxsGetSockaddr();  

#ifndef WIN32
        // Read the message and the source address.  Too bad we can't get the dest address.
        vxsRecvFrom(socket, msgBuffer, sizeof(msgBuffer), 0, sourceAddr);

		if (pIpHdr->protocol != IPPROTO_UDP || (pUdpHdr->dest != htons(IPV6_LS_UDP_PORT)))
            return LTSTS_ERROR;
#else
        udpPayloadLen = vxsRecvFrom(socket, udpPayload, sizeof(udpPayload), 0, sourceAddr);
#endif

        // DEBUG
#ifdef WIN32
        uint8_t destAddress[IZOT_MAX_IP_ADDR_SIZE];
#else
        char src[50];
        char revcStr[50], destStr[50];
        udpPayloadLen = ntohs(pUdpHdr->len) - sizeof(struct udphdr);
        ULONG destAddress, recvDestAddress = pIpHdr->daddr;
        //ULONG sourceAddress = pIpHdr->saddr;
#endif

        LonLinkIzoTLock lock(m_lock);   // LOCK

        // REMINDER: Need to addresses for IPV6

#ifdef WIN32
        // ipv6_convert_ls_udp_to_ltvx needs the destination address for subnet/node or broadcast
        // addressing (but not neuron ID or group addressing).
        // Unfortunately we can't get the dest address from sockets.  So we
        // just assume it was addressed using the unicast address bound to this socket. 
        m_unicastAddresses.getIpAddress(socketIndex, destAddress, sizeof(destAddress));
#else
        m_unicastAddresses.getIpAddress(socketIndex, (uint8_t*)&destAddress, sizeof(destAddress));

        // Get the source IP address as a string, for debugging purposes
        vxsMakeDottedAddr(src, vxsAddrGetAddr(sourceAddr));
        vxsMakeDottedAddr(revcStr, ntohl(recvDestAddress));
        vxsMakeDottedAddr(destStr, ntohl(destAddress));

        vxlReportEvent("vxsRecvFrom sockets(%d)=%d source= %s dest=%s rec=%s dest2=%s udpPayloadLen=%d\n",
                socketIndex, socket, src, m_unicastAddresses.getSzIpAddress(socketIndex),
                revcStr, destStr, udpPayloadLen);
#endif
        if (udpPayloadLen && udpPayloadLen != SOCKET_ERROR && udpPayloadLen < MAX_UDP_PACKET)
        {
#ifndef WIN32
            if (((((byte *)&recvDestAddress)[0]) & 0xF0) == 0xE0)
            {
                // multicast message.
                if (socketIndex != LON_LINK_IZOT_DEV_MC_SOCKET_INDEX)
                {
                    sts = LTSTS_ERROR;
                    vxsFreeSockaddr(sourceAddr);
                    vxlReportEvent("LonLinkIzoTDev::driverRead multicast msg in unicast socket\n");
                    return(sts);
                 }
            }
            else
            {
                // unicast message
                if (socketIndex == LON_LINK_IZOT_DEV_MC_SOCKET_INDEX)
                {
                    sts = LTSTS_ERROR;
                    vxsFreeSockaddr(sourceAddr);
                    vxlReportEvent("LonLinkIzoTDev::driverRead unicast msg in multicast socket\n");
                    return(sts);
                }
                if (destAddress != recvDestAddress)
                {
                    sts = LTSTS_ERROR;
                    vxsFreeSockaddr(sourceAddr);
                    vxlReportEvent("LonLinkIzoTDev::driverRead Unicast Address Mismatch\n");
                    return(sts);
                }
            }
#endif
            // Now we need to  fixup the address if necessary, based on addressing mode
            switch (udpPayload[1] & IPV6_LSUDP_NPDU_MASK_ADDRFMT)
            {
                case IPV6_LSUDP_NPDU_ADDR_FMT_DOMAIN_BROADCAST:
                case IPV6_LSUDP_NPDU_ADDR_FMT_BROADCAST_NEURON_ID:
                case IPV6_LSUDP_NPDU_ADDR_FMT_SUBNET_BROADCAST:
#ifdef WIN32
                    ipv6_gen_ls_mc_addr(IPV6_LS_MC_ADDR_TYPE_BROADCAST, 0, destAddress);
#else
                    ipv6_gen_ls_mc_addr(IPV6_LS_MC_ADDR_TYPE_BROADCAST, 0, (uint8_t*)&destAddress);
#endif
                    break;

                case  IPV6_LSUDP_NPDU_ADDR_FMT_GROUP:
#ifdef WIN32
                    ipv6_gen_ls_mc_addr(IPV6_LS_MC_ADDR_TYPE_GROUP, 0, destAddress);
#else
                    ipv6_gen_ls_mc_addr(IPV6_LS_MC_ADDR_TYPE_GROUP, 0, (uint8_t*)&destAddress);
#endif
                    break;

            }                    

            // Get the source address in network order.
            ULONG sourceAddress = htonl(vxsAddrGetAddr(sourceAddr));

            uint8_t ltVxNpdu[MAX_UDP_PACKET+30];  // Assumes that the LVT0 packet at most 30 bytes bigger than the UDP packet.
            uint16_t ltVxLen;
            memset(ltVxNpdu, 0xaa, sizeof(ltVxNpdu));

            // convert the LS/IP UDP packet to LTV0 or LTV2
            ipv6_convert_ls_udp_to_ltvx(0, (uint8_t *)udpPayload, udpPayloadLen,
#ifdef WIN32
                                        (uint8_t *)&sourceAddress, vxsAddrGetPort(sourceAddr),
                                        (uint8_t *)&destAddress, gLsUdpPort,
#else
                                        (uint8_t *)&sourceAddress, htons(pUdpHdr->source),
                                        (uint8_t *)&destAddress, htons(pUdpHdr->dest),
#endif
                                        ltVxNpdu, &ltVxLen, static_cast<LonLinkIzoT*>(this));

            uint8_t *pPacketBuf = (uint8_t *)pData; 
            // Make sure packet buf has room for 2 byte SICB header plus 2 byte CRC
            if (len < ltVxLen+4)
            {
                // packet too big.  
            	vxlReportEvent("LonLinkIzoTDev::driverRead ERROR: LS/UDP packet is too big to fit in ltvx buffer, LtVx len = %d, buffer = %d\n", ltVxLen+4, len);
                sts = LTSTS_ERROR;
            }
            else if (ltVxLen == 0)
            {
            	vxlReportEvent("LonLinkIzoTDev::driverRead ERROR: invalid LS/UDP packet, source\n");
            	dumpData("LS/UDP packet", (uint8_t *)udpPayload, udpPayloadLen);
            }
            else
            {
                // Calculate the CRC
                int dataOffset = 2;
                LtCRC16(ltVxNpdu, ltVxLen);
                ltVxLen += 2;   // Adjust len to include CRC

                // Set the SICB header
                pPacketBuf[0] = L2_PKT_TYPE_INCOMING;
                if (ltVxLen >= L2_PKT_LEN_EXTENDED)
                {
                    pPacketBuf[1] = L2_PKT_LEN_EXTENDED;
                    memcpy(&pPacketBuf[2], &ltVxLen, 2);
                    dataOffset += 2;
                }
                else
                {
                    pPacketBuf[1] = (uint8_t)ltVxLen;
                }

                // Set the pdu
                memcpy(pPacketBuf+dataOffset, ltVxNpdu, ltVxLen);
                //if (m_ipManagementOptions & LONLINK_IZOT_MGMNT_OPTION_TRACE_MSGS)
                {
                    dumpData("LonLinkIzoTDev::driverRead received UDP packet, SICB:", pPacketBuf, ltVxLen+dataOffset);
                }
                sts = LTSTS_OK;
            }
        }
        else
        {
            // Nothing to read.  LonLink expects LTSTS_ERROR for that.
            sts = LTSTS_ERROR;
        }
        vxsFreeSockaddr(sourceAddr);
    }
	return(sts);
}

// Convert the LTV0 or LTV2 packet into an LS/IP UDP packet and send it.
LtSts LonLinkIzoTDev::driverWrite(void *pData, short len)
{
	LtSts	sts = LTSTS_ERROR;
    uint8_t *pPacket = (uint8_t *)pData;
    int dataOffset = 2;

    if (pPacket[1] == L2_PKT_LEN_EXTENDED)
    {
        dataOffset += 2;
    }

    if ((pPacket[0] == 0x12 || pPacket[0] == 0x13) && len > dataOffset)
    {
        // Looks like a valid packet to be sent out the network.
        unsigned char *pNpdu = (unsigned char *)pPacket + dataOffset;
        len =  len-dataOffset;  // Skip SICB header

        // Make sure its LTV0 or LTV1
        if (IPV6_LT_IS_VER_LS_LEGACY_MODE(pNpdu[1]) || IPV6_LT_IS_VER_LS_ENHANCED_MODE(pNpdu[1]))
        {
            uint8_t sourceAddr[IPV4_ADDRESS_LEN];
            uint8_t destAddr[IPV4_ADDRESS_LEN];
#ifndef WIN32
            uint8_t msgBuffer[MAX_ETH_PACKET];
			memset(msgBuffer, 0, MAX_ETH_PACKET);
            struct iphdr *pIpHdr = (struct iphdr *)msgBuffer;
            struct udphdr *pUdpHdr = (struct udphdr *)(msgBuffer + sizeof(struct iphdr));
            uint8_t *lsUdpPayload = msgBuffer + sizeof(struct iphdr) + sizeof(struct udphdr);
#else
            uint8_t lsUdpPayload[MAX_LPDU_SIZE+50]; // Assumes that udp payload is no more than 50 bytes bigger than NPDU
#endif
            uint16_t destPort;            
            uint16_t sourcePort;            
            memcpy(lsUdpPayload, pNpdu, len);

            // Translate to LS/IP UDP packet.
            if (m_ipManagementOptions & LONLINK_IZOT_MGMNT_OPTION_TRACE_MSGS)
            {
                dumpData("LonLinkIzoTDev::driverWrite LtVx msg:", pData, len+2);
            }

            // Convert the LTV0 or LTV2 packet (in place) to an LS/IP UDP.  Returns the source and destination
            // IP addresses to use.
            uint16_t npduLen = ipv6_convert_ltvx_to_ls_udp(lsUdpPayload, len, sourceAddr, &sourcePort, 
                                                           destAddr, &destPort, static_cast<LonLinkIzoT*>(this));
                
            if (npduLen != 0)
            {
                int socketIndex;
                // Translation succeeded

                // Find the socket to use to send the message.  Note that ipv6_convert_ltvx_to_ls_udp should
                // find the appropriate source IP address, which might be LS derived or might be arbitrary,
                // but in any case should be bound.
                socketIndex = SelectSourceSocket(sourceAddr);

                if (socketIndex != IZOT_NULL_SOCKET_INDEX)
                {
                    VXSOCKADDR vxsDest;

                    // Yuk, all this conversion!
		            vxsDest = vxsAddrValue(htonl(*((ULONG *)destAddr)));

                    if (vxsDest != NULL)
                    {
		                vxsSetPort( vxsDest, destPort );

                        char dst[50];
                        vxsMakeDottedAddr(dst, vxsAddrGetAddr(vxsDest));

                        LonLinkIzoTLock lock(m_lock);

                        if (m_ipManagementOptions & LONLINK_IZOT_MGMNT_OPTION_TRACE_MSGS)
                        {
                            char buf[200];

                            sprintf(buf, "LonLinkIzoTDev::driverWrite, send UDP socket[%d]=%d from %s to %s",
                                socketIndex, m_sockets.getSocket(socketIndex), 
                                m_unicastAddresses.getSzIpAddress(socketIndex), dst);
                            dumpData(buf, lsUdpPayload, npduLen);
                        }
                        else
                        {
                            char src[50];
                            ULONG ipSenderAddr;
                            USHORT ipSenderPort;

                            vxsGetSockAddressAndPort(m_sockets.getSocket(socketIndex),
                                                     		&ipSenderAddr, &ipSenderPort);
                            vxsMakeDottedAddr(src, ntohl(ipSenderAddr));
                            vxlReportEvent("vxsSendTo socket[%d]=%d from %s to %s\n",
                                socketIndex, m_sockets.getSocket(socketIndex), 
                                src, dst);
                        }

#ifndef WIN32
                        int msglen = sizeof(struct iphdr) + sizeof(struct udphdr) + npduLen;

                        pIpHdr->ihl = 5;
                        pIpHdr->version = 4;
                        pIpHdr->tot_len = htons(msglen);
                        pIpHdr->id = 0;
                        pIpHdr->ttl = 64; // hops
                        pIpHdr->protocol = 17; // UDP
                        pIpHdr->saddr = 0;
                        pIpHdr->daddr = *((uint32_t*)destAddr);
                        pIpHdr->check = 0;

                        pUdpHdr->source = htons(sourcePort);
                        pUdpHdr->dest = htons(destPort);
                        pUdpHdr->len = htons(sizeof(struct udphdr) + npduLen);
                        pUdpHdr->check = 0;

                        pIpHdr->check = checksum((uint16_t*)msgBuffer, 20);
#endif
                        // Finally, send the LS/IP UDP packet
#ifdef WIN32
		                int nBytes = vxsSendTo( m_sockets.getSocket(socketIndex), (LPSTR)lsUdpPayload, npduLen, 0, vxsDest );
#else
		                int nBytes = vxsSendTo( m_sockets.getSocket(socketIndex), (LPSTR)msgBuffer, msglen, 0, vxsDest);
#endif
		                vxsFreeSockaddr( vxsDest );
		                if ( nBytes == npduLen )
		                {
                            sts = LTSTS_OK;
		                }
                    }
                    else
                    {
                        dumpData("vxsAddrValue failed for dest", vxsDest, sizeof(vxsDest));
                    }
                }
                else
                {
                    dumpData("LonLinkIzoTDev::driverWrite failed to find socket for source address: ",  sourceAddr, sizeof(sourceAddr));
                }
            }
            else
            {
                vxlReportUrgent("ipv6_convert_ltvx_to_ls_udp failed!\n");
            }
        }
    }
	return(sts);
}


///////////////////////////////////////////////////////////////////////////////
// Socket management
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
LtSts LonLinkIzoTDev::setUnicastAddress(int stackIndex, int domainIndex, int subnetNodeIndex, 
                                        byte *domainId, int domainLen, byte subnetId, byte nodeId)
{
	LtSts	sts = LTSTS_ERROR;
    char    szUnicastAddress[100];

    LonLinkIzoTLock lock(m_lock);   // LOCK

    uint8_t unicastAddress[IPV4_ADDRESS_LEN];

    // Generate the LS derived IP address for this domain/subnet/node ID.
    ipv6_gen_ls_subnet_node_addr(domainId, (uint8_t)domainLen, subnetId, nodeId, unicastAddress);

    vxlReportEvent("LonLinkIzoTDev::setUnicastAddress stackIndex = %d domainIndex = %d  subnetNodeIndex = %d\n  domLen=%d subnetId=%d nodeId=%d\n",
		stackIndex, domainIndex, subnetNodeIndex, domainLen, subnetId, nodeId);

    // Find the subnet/node config for this stackIndex/domainIndex/subnetIndex.  
    // Returns NULL if not yet set.
    IzoTDevSubnetNodeConfig *pSubnetNodeConfig = m_devConfigMap.find(stackIndex, domainIndex, subnetNodeIndex);

    // The old socket index used by this stack/domain/subnet config. Updated later.
    int oldSocketIndex = IZOT_NULL_SOCKET_INDEX;

    // Bind to this address only if this is not an empty address.
    if ((domainLen <= 6) && (subnetId != 0) && ((nodeId&0x7f)!=0))
    {
        // Create a string form the unicast IP address
        ULONG addr = ntohl(*((ULONG*)unicastAddress));
        vxsMakeDottedAddr(szUnicastAddress, addr);

        // See if we already have that addresss
        int newSocketIndex = m_unicastAddresses.find(unicastAddress, IZOT_MAX_IP_ADDR_SIZE);

        IzotUnicastAddress *pNewUcastAddr = m_unicastAddresses.get(newSocketIndex);

        if (pSubnetNodeConfig == NULL)
        {
            // No config for this stack/domain/subnetNode.  Create it.
            pSubnetNodeConfig = m_devConfigMap.add(stackIndex, domainIndex, subnetNodeIndex);
        }

        // Get the socket index used by the old config (if any).
        oldSocketIndex = pSubnetNodeConfig->getSocketIndex();

        if (pNewUcastAddr == NULL)
        {
            // Address doesn't exist yet.  Need to add it.
            VXSOCKET socket = opensocket();
            if (socket != INVALID_SOCKET)
            {
                // Get a new socket
                newSocketIndex = m_sockets.addSocket(socket);          
                // Set the unicast address to the new unicast address.  Note that this
                // increments the use count (to 1).
                m_unicastAddresses.set(newSocketIndex, unicastAddress, IPV4_ADDRESS_LEN);
                pNewUcastAddr = m_unicastAddresses.get(newSocketIndex);
            }
        }
        else 
        {
            // Address exists.  Just increment the use count since its being used again.
            pNewUcastAddr->increment();
        }

        // Update the socket index used by this object
        pSubnetNodeConfig->setSocketIndex(newSocketIndex);

        lock.unlock();  // UNLOCK before binding,  because this could take a bit of time

        // Bind the socket to the new address.
        sts = bindUnicastAddress(newSocketIndex, false);
    }
    else
    {
        // The device has no address.  Remove if there was one before.
        if (pSubnetNodeConfig != NULL)
        {
            // get the oldSocketIndex so that it can be released below.
            oldSocketIndex = pSubnetNodeConfig->getSocketIndex();
            // Remove the config data for this subnetNodeIndex
            m_devConfigMap.remove(stackIndex, domainIndex, subnetNodeIndex);
        }
        // Don't need to bind...
        sts = LTSTS_OK;
    }

    if (oldSocketIndex != IZOT_NULL_SOCKET_INDEX)
    {
        // Release the reference to the old address.  Note that if the old and new addresses are the
        // same this just restores the use count.
        releaseUnicastAddress(oldSocketIndex);
    }

    return sts;
}

 /******************************************************************************
  method:  deregisterStack
   
  Summary:
    Deregister the stack from the link, and close the appropriate sockets,
    and leave the appropriate multicast groups.

  Parameters:
    stackIndex:         The device stacks appIndex
*****************************************************************************/
void LonLinkIzoTDev::deregisterStack(int stackIndex)
{
    LonLinkIzoTLock lock(m_lock);
    m_devConfigMap.remove(stackIndex);
}

    // Bind the socket at socketIndex to the unicastAddresses address att
    // socketIndex.  rebind is true if we are retrying this binding operation.
LtSts LonLinkIzoTDev::bindUnicastAddress(int socketIndex, bool rebind)
{
	LtSts sts = LTSTS_OPENFAILURE;

	vxlReportEvent("LonLinkIzoTDev::bindUnicastAddress\n");

    if (m_unicastAddresses.getIsBound(socketIndex))
    {
        // Its already bound.  Don't need to rebind it.
#ifdef WIN32
        if (!rebind && socketIndex == 0)
        {
            // We opened this as the "any socket", so we never added it to
            // the registry, but it turns out we need this one specifically
            // - so add it now.
            LonLinkIzoTLock lock(m_lock);
            IzoTSetUnicastReg(m_szIzoTName, m_unicastAddresses.getSzIpAddress(socketIndex));
        }
#endif
        sts = LTSTS_OK;
    }
    else
    {
        // Try to bind it 

        VXSOCKET socket = m_sockets.getSocket(socketIndex);
        if (socket != INVALID_SOCKET)
        {
            char szIpAddr[IZOT_MAX_SZ_IP_ADDR_SIZE];

            // Get the IP addresss as a string
            m_unicastAddresses.getSzIpAddress(socketIndex, szIpAddr, sizeof(szIpAddr));

            // Attempt to bind to it.
            bool bound = IzoTSetUnicastAddress(m_szIzoTName, m_ipManagementOptions, 
                                               socket, m_szIpIfName, szIpAddr);

    	    vxlReportEvent("setUnicastAddress Idx=%d Socket=%d Addr=%s\n",
    			            socketIndex, socket, szIpAddr);

            if (bound)
            {
                sts = LTSTS_OK;
            }
            else
            {
        	    vxlReportEvent("LonLinkIzoTDev::bindUnicastAddress - ERROR\n");
                if (!rebind)
                {
                    // Failed, so we will use an arbitrary IP address instead.  Announce that
                    // fact.
                    sendBroadcastAnnouncement(socketIndex);
                }
            }

            // Set the isBound flag based on success or failure.
            m_unicastAddresses.setIsBound(socketIndex, sts == LTSTS_OK);
            
            sts = LTSTS_OK;  // Ignore binding errors.  We can always fall back on an arbitrary address.
        }

        if (m_bRebindTimerEnabled)
        {
            // See if there are any binding errors to be retried...
            // REMINDER:  Do we really even need this rebinding stuff?  Leave it in for
            // now and see if we ever turn it on.
            bool bindError = false;
            for (int i = LON_LINK_IZOT_DEV_FIRST_SEND_SOCKET_INDEX; !bindError && i < m_sockets.getNumSockets(); i++)
            {
                bindError = m_unicastAddresses.getRebind(i);
            }
#ifdef WIN32
            // Post connection status after each bind attempt
            PostMessage((HWND)m_connEventHWnd, bindError ? LDVX_WM_DETACHED : LDVX_WM_ATTACHED, 0, m_connEventTag);
#endif
            if (bindError)
            {
                // Need to rebind, so schedule it.
                if (!rebind)
                {
                    // This was the inital bind - so reset timeout
                    m_rebindTimeout = LON_LINK_IZOT_DEV_REBIND_TIMEOUT_MIN;
                }
                startDelayedRebinding();
            }
        }
    }

    // Update the subnet multicast membership
    if (sts == LTSTS_OK)
        sts = updateLsSubnetMembership();

    return sts;
}

// Release the unicast address at the associated socketIndex
void LonLinkIzoTDev::releaseUnicastAddress(int socketIndex)
{
    LonLinkIzoTLock lock(m_lock);

    // Find the unicast address for this socket.
    IzotUnicastAddress *pUnicastAddress = m_unicastAddresses.get(socketIndex);
    if (pUnicastAddress != NULL)
    {
        // It exists, so decrement the use count
        if (pUnicastAddress->decrement() == 0)
        {
            // Its zero, so close the socket and recompute the subnet membership
            m_sockets.closeSocket(socketIndex);
            updateLsSubnetMembership();
        }
    }
}

// Find the next socket to read, and return the socket and the
// socket index.  If no socket has any available data, return INVALID_SOCKET 
VXSOCKET LonLinkIzoTDev::SelectSocketToRead(int *socketIndex)
{
    int i;
    struct timeval timeout;
    
    // Set timeout to 0.  We don't want to wait
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    *socketIndex = -1;
    int numSockets = m_sockets.getNumSockets();

    // Check for every possible socket.
    for (i = 0; i < numSockets; i++)
    {
        // compute the socket index using round robin selection
        m_lastSocketRead = (m_lastSocketRead + 1) % numSockets;

        if (m_sockets.getSocket(m_lastSocketRead) != INVALID_SOCKET)
        {
            // This is a valid socket.  See if there is  anything to read.
            int ret = vxsSelectRead(m_sockets.getSocket(m_lastSocketRead), &timeout);
            if (ret == 1)
	        {
                // There is something to read.  Return the socket index
                // and the socket
            	*socketIndex = m_lastSocketRead;
                return m_sockets.getSocket(m_lastSocketRead);
            }
        }
    }

    // Nothing to read.
    return INVALID_SOCKET;
}

// Return the socketIndex corresponding to the specified pSourceAddress.
// Return INVALID_SOCKET if there is no socket successfully bound to that
// address.
int LonLinkIzoTDev::SelectSourceSocket(const uint8_t *pSourceAddress)
{
    // Find the socket
    int socketIndex = m_unicastAddresses.find(pSourceAddress, IPV4_ADDRESS_LEN);

#ifndef WIN32
    // For non Windows system, socket index 0 is only used for receiving the multicast mssg
    // which is bound to INADDR_ANY
    if (socketIndex == LON_LINK_IZOT_DEV_MC_SOCKET_INDEX)
    {
     	socketIndex = LON_LINK_IZOT_DEV_FIRST_SEND_SOCKET_INDEX;
    }
#endif
    // Make sure it's bound
    if (!m_unicastAddresses.getIsBound(socketIndex))
    {
        // Since its not bound its no good to us.
        socketIndex = IZOT_NULL_SOCKET_INDEX;
    }
    return socketIndex;
}

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
LtSts LonLinkIzoTDev::updateGroupMembership(int stackIndex, int domainIndex, LtGroups &groups)
{
	LtSts	sts = LTSTS_OK;

    LonLinkIzoTLock lock(m_lock);   // LOCK

    // Find the domain config, if it exists.
    IzoTDevDomainConfig *pDomainConfig = m_devConfigMap.find(stackIndex, domainIndex);
    if (pDomainConfig == NULL)
    {
        // Doesn't exist, so we need to create it.
        pDomainConfig = m_devConfigMap.add(stackIndex, domainIndex);
    }

    // See if the group membership of the domain has changed.
    if (!(pDomainConfig->getGroups() == groups))
    {
        // Update the group membership of the domain.
        pDomainConfig->getGroups() = groups;

        // Recompute the group membership of all devices using the link.
        sts = updateLsGroupMembership();
    }
    return sts;
}

// Update subnet or group multicast membership based on the old and new
// subnet or group membership.
LtSts LonLinkIzoTDev::updateMcMembership(int type, LtRouteMap &currentMap, LtRouteMap newMap)
{
    LtSts sts = LTSTS_OK;

    // See if the membership has changed.  Note the == operator is defined, but not !=
    if (!(currentMap == newMap))
    {
        // A change in membership.  See what has been added and/or what has been removed.
        int limit = max(currentMap.getMax(), newMap.getMax());
        for (int id = 0; id <= limit; id++)
        {
            // Has this ID been added or removed?
            if (currentMap.get(id) != newMap.get(id))
            {
                // Its either been added or removed.  It was added if currentMap.get(id) == 0
                LtSts tempSts = updateMcAddress(currentMap.get(id) == 0, type, id);
                if (tempSts != LTSTS_OK)
                {
                    // Remember the error.
                    sts = tempSts;
                }
                else
                {
                    // Update the current status.
                    currentMap.set(id, newMap.get(id) != 0);
                }
            }
        }
    }
    return sts;
}

// Rebuild LS group membership for the link by scanning the group membership
// of all the domains of all the devices using the link.
LtSts  LonLinkIzoTDev::updateLsGroupMembership(void)
{
    LtSts sts = LTSTS_OK;

    LonLinkIzoTLock lock(m_lock);

    LtGroups newGroups;  // Initially empty 

    // Recompute group membership by scanning the domain config of each device
    // and adding groups

    // For each device
    for (IzoTDevConfig *pDev = m_devConfigMap.findFirst(); pDev != NULL; pDev = pDev->next())
    {
        // For each domain within the device
        for (IzoTDevDomainConfig *pDomain = pDev->getDomains().findFirst(); pDomain != NULL; pDomain = pDomain->next())
        {
            // For each group in the 
            for (int groupId = 0; groupId <= pDomain->getGroups().getMax(); groupId++)
            {
                if (pDomain->getGroups().get(groupId))
                {
                    // Add the group.
                    newGroups.set(groupId);
                }
            }
        }
    }

    // Was there a change in group membership?
    if (!(newGroups == m_groups))
    {
        // Yes, update the corresponding MC membership.
        sts = updateMcMembership(IPV6_LS_MC_ADDR_TYPE_GROUP, m_groups, newGroups);
    }

    return sts;
}

// Rebuild LS subnet membership for the link by scanning the subnet membership
// of all the domains of all the devices using the link.
LtSts  LonLinkIzoTDev::updateLsSubnetMembership(void)
{
    LtSts sts = LTSTS_OK;
    LtSubnets newSubnets;   // initially empty

    newSubnets.set(0); // Always belong to the broadcast subnet.

    LonLinkIzoTLock lock(m_lock);
    // Recompute subnet membership;

    // Recompute subnet membership by scanning the subnet/node config of each device
    // and adding any subnets found

    // For each device
    for (IzoTDevConfig *pDev = m_devConfigMap.findFirst(); pDev != NULL; pDev = pDev->next())
    {
        // For each domain on the device
        for (IzoTDevDomainConfig *pDomain = pDev->getDomains().findFirst(); pDomain != NULL; pDomain = pDomain->next())
        {
            // For each subent/node in the domain
            for (IzoTDevSubnetNodeConfig *pSubnetNode = pDomain->getSubnetNodes().findFirst(); pSubnetNode != NULL; pSubnetNode = pSubnetNode->next())
            {
                // Get the unicast address
                const byte *pIpAddr = m_unicastAddresses.getIpAddress(pSubnetNode->getSocketIndex());
                if (pIpAddr != NULL)
                {
                    // Extract the LS subnet ID from the unicast address and set its use newSubnets 
                    newSubnets.set(pIpAddr[IPV6_LSIP_UCADDR_OFF_SUBNET]);
                }
            }
        }
    }

    // Was there a change in subnet membership?
    if (!(newSubnets == m_subnets))
    {
        // Yes, update the corresponding MC membership.
        sts = updateMcMembership(IPV6_LS_MC_ADDR_TYPE_BROADCAST, m_subnets, newSubnets);
    }

    return sts;
}

// Add or remove the multicast address for a specfic group or subnet.
// REMINDER:  For IPV6 support this needs to take the domain ID into account, so
// this needs to be done for each unique domainID.
LtSts LonLinkIzoTDev::updateMcAddress(bool add, byte type, byte subnetOrGroup)
{
	LtSts	sts = LTSTS_ERROR;

    // Generate the multicast address for this subnet or group.
    uint8_t mcAddr[IPV4_ADDRESS_LEN];
    ipv6_gen_ls_mc_addr(type, subnetOrGroup, mcAddr);

    // Get the dotted address from the multicast address
    char mcastAddr[50];
    vxsMakeDottedAddr(mcastAddr, htonl(*((ULONG *)mcAddr)));

    // Find the socket we use to receive multicast
    VXSOCKET socket = m_sockets.getSocket(LON_LINK_IZOT_DEV_MC_SOCKET_INDEX);

    // Add or remove the multicast membership.
    if (vxsUpdateMulticast(add, socket, htonl(*((ULONG *)mcAddr)),
    		ntohl(m_unicastAddresses.getIpv4Address(LON_LINK_IZOT_DEV_FIRST_SEND_SOCKET_INDEX))) == OK)
    {
        sts = LTSTS_OK;
        if (add)
        {
            // set multicast time-to-live value option
            if (vxsSetMulticastTTL( socket, IPV6_MC_TTL_FOR_IPV4) < 0)
            {
        	    sts = LTSTS_ERROR;
         	    vxlReportEvent("Error on setting TTL on  Multicast Address: %s\n", mcastAddr);
            }
        }
        if (sts == LTSTS_OK)
        {
            vxlReportEvent("%s Multicast Address: %s\n", add ? "Joined" : "Left", mcastAddr); 
        }
    }
    else
    {
        vxlReportEvent("Failed to %s Multicast Address: %s\n", add ? "join" : "leave", mcastAddr); 
    }
    return sts;
}

///////////////////////////////////////////////////////////////////////////////
// Rebinding 
///////////////////////////////////////////////////////////////////////////////

// C interface to rebindTimerRoutine
int lonLinkIzoTRebindTimer( int a1 )
{
	vxlReportEvent("lonLinkIzoTRebindTimer\n");
	LonLinkIzoTDev*	pLink = (LonLinkIzoTDev*) a1;
	pLink->rebindTimerRoutine();
	return 0;
}

// Attempt to rebind if a binding failure occured.  Enabled only if the
// ipManagementOptions LONLINK_IZOT_MGMNT_OPTION_RETRY_BINDING option is set.
void LonLinkIzoTDev::rebindTimerRoutine()
{
    bool bindError = false;

    LonLinkIzoTLock lock(m_lock);   // LOCK 

    m_bInRebindTimerRoutine = TRUE;
    vxlReportEvent("LonLinkIzoTDev::rebindTimerRoutine\n");
    // For each socket, see if it currently needs to be rebound.
    for (int i = 0; m_bRebindTimerEnabled && i < m_sockets.getNumSockets(); i++)
    {
        if (m_unicastAddresses.getRebind(i))
        {
            // Its allocated but not bound - try to bind again...
            lock.unlock();  // Binding can take some time - give up the lock.
            bindUnicastAddress(i, true);
            lock.lock();
            if (m_unicastAddresses.getRebind(i))
            {
                // Still not bound

                // Failed again
                const char *pSzIpAddress = m_unicastAddresses.getSzIpAddress(i);
            	vxlReportEvent("rebindTimerRoutine: Bind Err ipAddress = %s\n",
                    (pSzIpAddress == NULL) ? "NULL" : pSzIpAddress);

                // Note that there was a binding error, so we can try again later
                bindError = true;
            }
        }
    }


    // Indicate that we are no longer rebinding
    m_bDelayedRebindPending = false;
    if (bindError)
    {
        // Start the timer again.
        startDelayedRebinding();
    }
    m_bInRebindTimerRoutine = FALSE;
}

// Start the rebinding timer.
void	LonLinkIzoTDev::startDelayedRebinding()
{
    // Only start the rebind timer if the link supports it.
    if (m_bRebindTimerEnabled)
    {
	    vxlReportEvent("LonLinkIzoTDev::startDelayedRebinding\n");

        // Cancel the timer before taking the lock.  Otherwise
        // we could take the lock, then the timer routine wait on
        // the lock, and any timer updates would deadlock.
        wdCancel(m_rebindTimer);  
        LonLinkIzoTLock lock(m_lock);  // LOCK 

        // Only start it if the timer hasn't already been started
	    if ( ! m_bDelayedRebindPending )
	    {
            // Start the timer
		    wdStart( m_rebindTimer, m_rebindTimeout, lonLinkIzoTRebindTimer, (int)this);

            m_rebindTimeout = m_rebindTimeout*2;    // geometric backoff
            if (m_rebindTimeout > LON_LINK_IZOT_DEV_REBIND_TIMEOUT_MAX)
            {
                // Exceeded its max.
                m_rebindTimeout = LON_LINK_IZOT_DEV_REBIND_TIMEOUT_MAX;
            }

            // Indicate that the timer has started.
		    m_bDelayedRebindPending = true;
	    }
    }
}

///////////////////////////////////////////////////////////////////////////
// Arbitrary IP Address Announcements
///////////////////////////////////////////////////////////////////////////

// Process an announce timeout.
void	LonLinkIzoTDev::announceTimerRoutine(void)
{
    LonLinkIzoTLock lock(m_lock);  // LOCK

    // Only announce if the timer has been enabled.
    if (m_announceTimerEnabled)
    {
        // Scan through all unicast entries to find unbound entries
        while (m_announceIndex < m_unicastAddresses.getNumEntries())
        {
            if (m_unicastAddresses.getRebind(m_announceIndex))
            {
                // Its allocated but not bound. That means that the LS address 
                // that should be useing this socket cant, and has to resort
                // to using an arbitrary IP address.  Announce that fact.
                sendBroadcastAnnouncement(m_announceIndex++);

                // Break, because we don't want to flood the network with
                // announcements.
                break;
            }
            m_announceIndex++;
        }

        if (m_announceIndex >= m_unicastAddresses.getNumEntries())
        {
            // Done with all the announcements for now
            m_announceIndex = 0;
        }

        // Start the timer again. The time is based on whether we are done or have just made an announcement.
        startAnnounceTimer();
    }
}

// C interface to announceTimerRoutine
static int lonLinkDevIzoTAnnounceTimer( int a1 )
{
    LonLinkIzoTDev*	pLink = (LonLinkIzoTDev*) a1;
	pLink->announceTimerRoutine();
	return 0;
}

// Start the announce timer.  If we are in the middle of announcements set timer to
// m_announceThrottle, otherwise set to m_announceInterval adjusted for any delay
// caused by throttling previous announcements (m_announceDelay).
void	LonLinkIzoTDev::startAnnounceTimer()
{
    // Cancel the timer before taking the lock.  Otherwise
    // we could take the lock, then the timer routine wait on
    // the lock, and any timer updates would deadlock.
    wdCancel(m_announceTimer);  

    LonLinkIzoTLock lock(m_lock);   // LOCK

    if (m_announceInterval)
    {
        m_announceTimerEnabled = true;  // Indicate that the timer is enabled

        ULONG interval = m_announceInterval;  // The default interval
        if (m_announceIndex == 0)
        {
            // Starting over. Adjust the interval to take into account any delay we may have had
            // because we sent out multiple announcements.
            if (interval > m_announceDelay)
            {
                interval -= m_announceDelay;
            }
            else
            {
                // Hmm, we seem to have spent a lot of time doing announcements.  Just
                // set to the throttle value and start over again.
                interval = m_announceThrottle;
            }

            // No more delays after this.
            m_announceDelay = 0;
        }
        else
        {
            // We are between announcements - just set the delay long enough to throttle them.
            interval = m_announceThrottle;

            // And keep track of how long we are spending so we don't drift too much (though this
            // doesn't need to be super accurate).
            m_announceDelay += m_announceThrottle;
        }
            
	    wdStart( m_announceTimer, msToTicksX(interval), lonLinkDevIzoTAnnounceTimer, (int)this );
    }
}

// Send a broadcast announcement for the specified socket.
void LonLinkIzoTDev::sendBroadcastAnnouncement(int socketIndex)
{
    LonLinkIzoTLock lock(m_lock);
    if (socketIndex < m_unicastAddresses.getNumEntries())
    {
        // Socket is valid.  
        vxlReportEvent("Send announcement for arbitrary address %s\n...", m_unicastAddresses.getSzIpAddress(socketIndex));

        // This formats the announcement message and calls back to do the send.
        ipv6_send_multicast_announcement(this, m_unicastAddresses.getIpAddress(socketIndex));
    }
}

/******************************************************************************
  method:  setLsAddrMappingConfig
   
  Summary:
    Update the timers used for announcements and aging.

  Parameters:
    lsAddrMappingAnnounceFreq:      Frequency to make announcements
    lsAddrMappingAnnounceThrottle:  Minimum time between consecutive announcements
    lsAddrMappingAgeLimit:          Expiration period for arbtrary address mapping
*****************************************************************************/
void LonLinkIzoTDev::setLsAddrMappingConfig(int stackIndex, ULONG lsAddrMappingAnnounceFreq, 
                                            WORD lsAddrMappingAnnounceThrottle, 
                                            ULONG lsAddrMappingAgeLimit)
{
    LonLinkIzoTLock lock(m_lock);   // LOCK

    // Find the dev config, if it exists.
    IzoTDevConfig *pDevConfig = m_devConfigMap.find(stackIndex);
    if (pDevConfig == NULL)
    {
        // Doesn't exist, so we need to create it.
        pDevConfig = m_devConfigMap.add(stackIndex);
    }
    pDevConfig->setLsAddrMappingConfig(lsAddrMappingAnnounceFreq, 
                                       lsAddrMappingAnnounceThrottle, 
                                       lsAddrMappingAgeLimit);
    lock.unlock();  // Have to unlock before updating the mapping config to prevent
                    // possible deadlock with timer routines.
    updateLsAddrMappingConfig();
}

// Update the LS address mapping configuration for the link based on the configuration
// of each device.
void LonLinkIzoTDev::updateLsAddrMappingConfig(void)
{
    bool update = false;
    LonLinkIzoTLock lock(m_lock);

    ULONG newAnnounceTimer = 0;     /* disabled */
    WORD  newAnnounceThrottle = 0;  /* disabled */
    ULONG newAgeLimit = 0;          /* disabled */
    // Recompute timers by scanning the timer config of each device

    // For each device
    for (IzoTDevConfig *pDev = m_devConfigMap.findFirst(); pDev != NULL; pDev = pDev->next())
    {
        if (pDev->getLsAddrMappingAnnounceFreq() != 0)
        {
            if (newAnnounceTimer == 0 ||
                newAnnounceTimer > pDev->getLsAddrMappingAnnounceFreq())
            {
                newAnnounceTimer = pDev->getLsAddrMappingAnnounceFreq();
            }
        }
        if (pDev->getLsAddrMappingAgeLimit() != 0)
        {
            if (newAgeLimit == 0 ||
                newAgeLimit > pDev->getLsAddrMappingAgeLimit())
            {
                newAgeLimit = pDev->getLsAddrMappingAgeLimit();
            }
        }
        if (pDev->getLsAddrMappingAnnounceThrottle() != 0)
        {
            if (newAnnounceThrottle == 0 ||
                newAnnounceThrottle > pDev->getLsAddrMappingAnnounceThrottle())
            {
                newAnnounceThrottle = pDev->getLsAddrMappingAnnounceThrottle();
            }
        }

    }

    if (newAnnounceThrottle == 0)
    {
        // Need to have some kind of throttling!
        m_announceThrottle = LON_LINK_IZOT_DEV_ANNOUNCEMENT_THROTTLE;
    }
    else
    {
        m_announceThrottle = newAnnounceThrottle;
    }
    if (newAnnounceTimer != m_announceInterval)
    {
        m_announceInterval = newAnnounceTimer;
        m_announceTimerEnabled = FALSE;
        m_announceIndex = 0;
        m_announceDelay = 0;
        update = true;
    }
    lock.unlock();  // Have to unlock before updating the mapping config to prevent
                    // possible deadlock with timer routines.
    if (update)
    {
        startAnnounceTimer();
    }
    updateAgingInterval(newAgeLimit);
}

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
void LonLinkIzoTDev::sendAnnouncement(const uint8_t *ltVxmsg, uint8_t msgLen)
{
    uint8_t *pMsg = new uint8_t[msgLen+2];
    pMsg[0] = 0x12;
    pMsg[1] = msgLen;
    memcpy(&pMsg[2], ltVxmsg, msgLen);

    if (m_ipManagementOptions & LONLINK_IZOT_MGMNT_OPTION_TRACE_MSGS)
    {
        dumpData("sendAnnouncement", pMsg, msgLen+2);
    }

    driverWrite(pMsg, msgLen+2);
    delete[] pMsg;
}

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
                            source addressing information (in LS/UDP format) to be 
                            added to the UDP payload, if any
  Return: 
    The length of the additional enclosed source address information

    *****************************************************************************/
uint8_t LonLinkIzoTDev::getArbitrarySourceAddress(uint8_t *pSourceIpAddress, 
                                                  const uint8_t *pDomainId, int domainIdLen,
                                                  uint8_t *pEnclosedSource)
{
    uint8_t enclosedSourceLen = 0;
    LonLinkIzoTLock lock(m_lock);

    int socketIndex = IZOT_NULL_SOCKET_INDEX;
#if UIP_CONF_IPV6
    if (domainIdLen == 6)
#else
    if (domainIdLen <= 1 || (domainIdLen == 3 && pDomainId[2] == 0))
#endif
    {
        // See if we have a bound socket for this source IP address.
        socketIndex = SelectSourceSocket(pSourceIpAddress);

        if (socketIndex == IZOT_NULL_SOCKET_INDEX)
        {
            // Can't use this address.  See if we can find a domain match at least!
            uint8_t domainIpPrefix[IZOT_MAX_IP_ADDR_SIZE];
            ipv6_gen_ls_subnet_node_addr(pDomainId, domainIdLen, 1, 1, domainIpPrefix);
            for (int i = LON_LINK_IZOT_DEV_FIRST_SEND_SOCKET_INDEX; i < m_sockets.getNumSockets(); i++)
            { 
                if (m_unicastAddresses.getIsBound(i))
                {
                    const byte *pSourceAddr = m_unicastAddresses.getIpAddress(i);
                    if (memcmp(pSourceAddr, domainIpPrefix, IPV6_LSIP_IPADDR_DOMAIN_LEN) == 0)
                    {
                        // This matches the domain, so its a good arbitrary IP address.
                        socketIndex = i;
                        enclosedSourceLen = IPV6_LSUDP_NPDU_OFF_ARB_SOURCE_NODE+1;
                        break;
                    }
                }
            }
        }
    }

    if (socketIndex == IZOT_NULL_SOCKET_INDEX)
    {
        // No socket found.  Just pick one. 
        for (socketIndex = LON_LINK_IZOT_DEV_MC_SOCKET_INDEX;
        		!m_unicastAddresses.getIsBound(socketIndex) && socketIndex < m_sockets.getNumSockets(); socketIndex++)
        {
        }

        if (socketIndex !=  m_sockets.getNumSockets())
        {
            // Need to include the domain plus the source subnet/node
            enclosedSourceLen = IPV6_LSUDP_NPDU_OFF_ARB_SOURCE_DM + domainIdLen;
            int encodedDomainLen = 0;
            switch (domainIdLen)
            {
            case 1: encodedDomainLen = 1; break;
            case 3: encodedDomainLen = 2; break;
            case 6: encodedDomainLen = 3; break;
            }
            pEnclosedSource[IPV6_LSUDP_NPDU_OFF_ARB_SOURCE_DMLEN] = encodedDomainLen;
            memcpy(&pEnclosedSource[IPV6_LSUDP_NPDU_OFF_ARB_SOURCE_DM], pDomainId, domainIdLen);
        }
    }

    if (enclosedSourceLen != 0)
    {
        // Using an arbitrary address. Include the source subnet/node
        pEnclosedSource[IPV6_LSUDP_NPDU_OFF_ARB_SOURCE_SUBNET] = pSourceIpAddress[IPV6_LSIP_UCADDR_OFF_SUBNET];
        pEnclosedSource[IPV6_LSUDP_NPDU_OFF_ARB_SOURCE_NODE] = pSourceIpAddress[IPV6_LSIP_UCADDR_OFF_NODE] & 0x7f;
        if (enclosedSourceLen > (IPV6_LSUDP_NPDU_OFF_ARB_SOURCE_NODE+1))
        {
            // Domain is included, so set the flag
            pEnclosedSource[IPV6_LSUDP_NPDU_OFF_ARB_SOURCE_DMFLAG] |= IPV6_LSUDP_NPDU_MASK_ARB_SOURCE_DMFLG;
        }

        // Copy the arbitrary source IP address
        memcpy(pSourceIpAddress, m_unicastAddresses.getIpAddress(socketIndex), IPV4_ADDRESS_LEN);
    }

    return enclosedSourceLen;
}

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
bool LonLinkIzoTDev::isUnicastAddressSupported(const uint8_t *ipAddress)
{
    return SelectSourceSocket(ipAddress) != IZOT_NULL_SOCKET_INDEX;
}

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
int LonLinkIzoTDev::queryIpAddr(LtDomain &domain, byte subnetId, byte nodeId, byte *ipAddress)
{
    byte domainId[LT_DOMAIN_LENGTH];
    domain.getData(domainId);
    ipv6_gen_ls_subnet_node_addr(domainId, domain.getLength(), subnetId, nodeId, ipAddress);
    if (SelectSourceSocket(ipAddress) == IZOT_NULL_SOCKET_INDEX)
    {
        // No socket found.  Just pick one. 
        int socketIndex;
        memset(ipAddress, 0, IPV4_ADDRESS_LEN); 
        for (socketIndex = LON_LINK_IZOT_DEV_FIRST_SEND_SOCKET_INDEX; socketIndex < m_sockets.getNumSockets(); socketIndex++)
        {
            if (m_unicastAddresses.getIsBound(socketIndex))
            {
                m_unicastAddresses.getIpAddress(socketIndex, ipAddress, IPV4_ADDRESS_LEN);
                break;
            }
        }
    }
    return IPV4_ADDRESS_LEN;
}

void
LonLinkIzoTDev::sendToProtocolAnalyser(byte *pData, bool crcIncluded)
{
	if  (m_ProtocolAnalyserFn)
	{
		short len =  pData[1];
		if (!crcIncluded)
		{
			// Add two crc bytes because the Protocol Analyser expects it
			// Zero out the CRC bytes
			pData[len+2] = 0x00;
			pData[len+3] = 0x00;
			// Increment length field inside SICB
			pData[1] = len + 2;
			// Increment length of buffer sent to LSPA.mod
			len += 2;
		}
		(*m_ProtocolAnalyserFn)((char*)pData, len + 2);
		if (!crcIncluded)
		{
			// Decrement length field inside SICB back to original
			pData[1] = len - 2;
		}
	}
}

 /******************************************************************************
   Method:  registerProtocolAnalyzerCallback

   Summary:
        This routine is used by the application to register the callback routine
        to hook protocol analyzer data sink.

   Parameters:
        fn:     The callback routine, passing the data packet and the length

   Return:   None

*****************************************************************************/
void LonLinkIzoTDev::registerProtocolAnalyzerCallback(void (*fn)(char*, int))
{
	m_ProtocolAnalyserFn = fn;
}

 /******************************************************************************
  	Method:  unregisterProtocolAnalyzerCallback

    Summary:
        This routine is used by the application to un-register the callback routine
        to hook protocol analyzer data sink.

    Parameters:
        None

    Return:   None

 *****************************************************************************/
void LonLinkIzoTDev::unregisterProtocolAnalyzerCallback()
{
	m_ProtocolAnalyserFn = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Debugging 
///////////////////////////////////////////////////////////////////////////////

// Trace data
void LonLinkIzoTDev::dumpData(const char *title, const void *pData, short len)
{
    vxlReportEvent("%s", title);
    const byte *p = (byte *)pData;
    char buf[1024];
    char *s = buf;
    for (int i = 0; i < len; i++)
    {
        sprintf(s, "%.2x", *p++);
        s += 2;
        if (i % 32 == 31)
        {
            *s = 0;
            vxlReportEvent("%s\n", buf);
            s = buf;
        }
    }
    if (s != buf)
    {
        *s = 0;
        vxlReportEvent("%s\n", buf);
    }
}
#endif
