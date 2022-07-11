/*
 * LonLinkIzoTRNIEthLink.cpp
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
 *  Created on: Oct 25, 2013
 *      Author: kblomseth
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h>

#include <netpacket/packet.h>
#include <netinet/ip.h>
#include <linux/if_ether.h>
#include <linux/udp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include "include/LonLinkIzoTRNIEthLink.h"
#include "IzoTNiConfig.h"
#include "ipv6_ls_to_udp.h"
#include "time.h"
#include "vxlTarget.h"  // for vxlReportXXXX
#include "LtMip.h"

#ifdef WIN32
#include <windows.h>
#else
#include <arpa/inet.h>
#define SOCKET_ERROR    -1  // In Win32, this is defined in WinSock.h
#endif

#include "VxSockets.h"
#include "LtCUtil.h"

LonLinkIzoTRNIEthLink* LonLinkIzoTRNIEthLink::m_instance = NULL;

LonLinkIzoTRNIEthLink* LonLinkIzoTRNIEthLink::getInstance()
{
	return m_instance;
}

LonLinkIzoTRNIEthLink::LonLinkIzoTRNIEthLink(int ipManagementOptions): LonLinkIzoT(1)
{
    m_lonSocket = -1;
    m_sendSocket = -1;
    m_instance = this;
    m_lastSocketRead = 0;
    m_ipManagementOptions = ipManagementOptions;
    m_lastMsgSize = 0;
    memset(m_lastMsg, 0, sizeof(m_lastMsg));
    _paFn = NULL;
}

LonLinkIzoTRNIEthLink::~LonLinkIzoTRNIEthLink()
{
    _paFn = NULL;
}

LtSts LonLinkIzoTRNIEthLink::driverOpen(const char* pName)
{
    LtSts   sts = LTSTS_ERROR;

    //0x800 = IP
	if((m_lonSocket = vxsRawSocket(SOCK_DGRAM, ETH_P_IP)) == -1)
	{
	    perror("socket");
	    fprintf(stderr,"driverOpen: failed to open socket for receiving packets\n");
	    return sts;
	}
    vxBindToInterface(m_lonSocket, pName, htons(ETH_P_ALL));

    m_sockets.addSocket(m_lonSocket);

#ifdef SEND_SOCKET
    // Now setup the socket for sending
    if ((m_sendSocket = vxsSocket(SOCK_RAW) == -1))
    {
        perror("socket");
        fprintf(stderr,"driverOpen: failed to open socket for sending packets\n");
        return sts;
    }

    struct ifreq ifr = {0};
    strcpy(ifr.ifr_name, pName);
    int on = 1;

    if (setsockopt(m_sendSocket, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
    {
        perror("ip hdr incl");
        fprintf(stderr,"driverOpen: failed to open socket for sending packets\n");
        return sts;
    }
    if (setsockopt(m_sendSocket, SOL_SOCKET, SO_BINDTODEVICE, (void*)&ifr, sizeof(ifr)) < 0)
    {
        perror("bind to device");
        fprintf(stderr,"driverOpen: failed to open socket for sending packets\n");
        return sts;
    }

    m_sockets.addSocket(m_sendSocket);
#else
    m_sendSocket = m_lonSocket;
#endif


    sts = LTSTS_OK;

    strcpy(m_name, pName);
    vxlReportEvent("DriverOpen: use auto IpIfName - '%s'\n", pName);
 	m_bLinkOpen = true;
	m_isOpen = 1;
	return LTSTS_OK;
}

void LonLinkIzoTRNIEthLink::driverClose()
{
    LonLinkIzoT::driverClose(); // Close the base IzoT driver
}

// Process the LS/IP UDP packet and convert it to LTVx
LtSts LonLinkIzoTRNIEthLink::driverRead(void *pData, short len)
{
	LtSts	sts = LTSTS_ERROR;
    char            msgBuffer[MAX_UDP_PACKET];   // [MAX_ETH_PACKET];
    int             msgBufferLen = 0;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    if (vxsSelectRead(m_lonSocket, &tv))
    {
        // An LS/IP UDP message has arrived.  Now we need to process it.
        struct sockaddr_ll inaddr;
        socklen_t addrlen = sizeof(inaddr);

        msgBufferLen = recvfrom(m_lonSocket, msgBuffer, sizeof(msgBuffer), 0,(struct sockaddr*)&inaddr, &addrlen);
        if (m_lastMsgSize && msgBufferLen > 0)
        {
            if (!memcmp(m_lastMsg, msgBuffer, m_lastMsgSize))
                // It's the msg originated by us.  Don't read it
                msgBufferLen = 0;
        }

       // REMINDER: Need to addresses for IPV6

       LonLinkIzoTLock lock(m_lock);   // LOCK
       if (msgBufferLen && msgBufferLen != SOCKET_ERROR && msgBufferLen < MAX_UDP_PACKET)
       {
            struct iphdr    *pIpHdr = (struct iphdr*)msgBuffer;
            struct udphdr   *pUdpHdr = (struct udphdr*)(msgBuffer + sizeof(struct iphdr));
            uint8_t         *pUdpPayload = (uint8_t*)(msgBuffer + sizeof(struct iphdr) + sizeof(struct udphdr));
            int             udpPayloadLen = ntohs(pUdpHdr->len) - sizeof(struct udphdr);
            ULONG           destAddress = pIpHdr->daddr;
            ULONG           sourceAddress = pIpHdr->saddr;

            if (pIpHdr->protocol == IPPROTO_UDP && (pUdpHdr->dest == htons(gLsUdpPort)))
            {
                uint8_t ltVxNpdu[MAX_UDP_PACKET+30];  // Assumes that the LVT0 packet at most 30 bytes bigger than the UDP packet.
                uint16_t ltVxLen;
                memset(ltVxNpdu, 0xaa, sizeof(ltVxNpdu));
#ifdef _DEBUG
                char dst[50], src[50];
                VXSOCKADDR vxsSrc = vxsAddrValue(ntohl(pIpHdr->saddr));
                VXSOCKADDR vxsDest = vxsAddrValue(ntohl(pIpHdr->daddr));
                vxsMakeDottedAddr(src, vxsAddrGetAddr(vxsSrc));
                vxsMakeDottedAddr(dst, vxsAddrGetAddr(vxsDest));
                fprintf(stderr,"vxsRecvFrom src= %s dest=%s udpPayloadLen=%d\n",
                            src, dst, udpPayloadLen);
                vxsFreeSockaddr(vxsSrc);
                vxsFreeSockaddr(vxsDest);
#endif
                // convert the LS/IP UDP packet to LTV0 or LTV2
                ipv6_convert_ls_udp_to_ltvx(0, (uint8_t *)pUdpPayload, udpPayloadLen,
                        (uint8_t *)&sourceAddress, htons(pUdpHdr->source),
                        (uint8_t *)&destAddress, gLsUdpPort,
                        ltVxNpdu, &ltVxLen, static_cast<LonLinkIzoT*>(this));

                // Make sure packet buf has room for 2 byte SICB header plus 2 byte CRC
                if (len < ltVxLen+4)
                {
                    // packet too big.
                    vxlReportEvent("LonLinkIzoTRNIEthLink::driverRead ERROR: LS/UDP packet is too big to fit in ltvx buffer, LtVx len = %d, buffer = %d\n", ltVxLen+4, len);
                    sts = LTSTS_ERROR;
                }
                else if (ltVxLen == 0)
                {
                    vxlReportEvent("LonLinkIzoTRNIEthLink::driverRead ERROR: invalid LS/UDP packet, source\n");
                    dumpData("LS/UDP packet", (uint8_t *)pUdpPayload, udpPayloadLen);
                }
                else
                {
                    uint8_t *pPacketBuf = (uint8_t *)pData;

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
                    if (m_ipManagementOptions & LONLINK_IZOT_MGMNT_OPTION_TRACE_MSGS)
                    {
                        dumpData("LonLinkIzoTRNIEthLink::driverRead received UDP packet, SICB:", pPacketBuf, ltVxLen+dataOffset);
                    }
                    sts = LTSTS_OK;
                }
            }
            else
            {
                // Nothing to read.  LonLink expects LTSTS_ERROR for that.
                sts = LTSTS_ERROR;
            }
        }
     }
    return sts;
}

uint16_t
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

LtSts LonLinkIzoTRNIEthLink::driverWrite(void *pData, short len)
{
	LtSts	sts = LTSTS_ERROR;
	char *pPacket = (char *)pData;
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
	        uint8_t msgBuffer[MAX_ETH_PACKET];
	        struct iphdr *pIpHdr = (struct iphdr *)msgBuffer;
	        struct udphdr *pUdpHdr = (struct udphdr *)(msgBuffer + sizeof(struct iphdr));
	        uint8_t *pUdpPayload = msgBuffer + sizeof(struct iphdr) + sizeof(struct udphdr);
	        uint16_t destPort;
	        uint16_t sourcePort;

	        memset(msgBuffer, 0, MAX_ETH_PACKET);
	        memcpy(pUdpPayload, pNpdu, len);

	        // Convert the LTV0 or LTV2 packet (in place) to an LS/IP UDP.  Returns the source and destination
	        // IP addresses to use.
	        uint16_t npduLen = ipv6_convert_ltvx_to_ls_udp(pUdpPayload, len, sourceAddr,
	                &sourcePort, destAddr, &destPort, static_cast<LonLinkIzoT*>(this));

	        if (npduLen != 0)
	        {
	            // Translation succeeded

	            LonLinkIzoTLock lock(m_lock);

	            int msglen = sizeof(struct iphdr) + sizeof(struct udphdr) + npduLen;

	            // Setup the message header
                pIpHdr->ihl = 5;
                pIpHdr->version = 4;
                pIpHdr->tot_len = htons(msglen);;
                pIpHdr->id = htons(54321);
                pIpHdr->ttl = 64; // hops
                pIpHdr->protocol = 17; // UDP
                pIpHdr->saddr = *((uint32_t*)sourceAddr);
                pIpHdr->daddr = *((uint32_t*)destAddr);
                pIpHdr->check = checksum((uint16_t*)msgBuffer, 20);

                pUdpHdr->source = htons(sourcePort);
                pUdpHdr->dest = htons(destPort);
                pUdpHdr->len = htons(sizeof(struct udphdr) + npduLen);
                pUdpHdr->check = 0;

                VXSOCKADDR vxsDest = vxsAddrValue(ntohl(pIpHdr->daddr));

                memcpy(m_lastMsg, msgBuffer, msglen);
                m_lastMsgSize = msglen;
	            if (m_ipManagementOptions & LONLINK_IZOT_MGMNT_OPTION_TRACE_MSGS)
	            {
	                char buf[200];
	                char dst[50], src[50];
	                VXSOCKADDR vxsSrc = vxsAddrValue(ntohl(pIpHdr->saddr));

	                vxsMakeDottedAddr(src, vxsAddrGetAddr(vxsSrc));
	                vxsMakeDottedAddr(dst, vxsAddrGetAddr(vxsDest));
	                sprintf(buf, "LonLinkIzoTRNIEthLink::driverWrite, send UDP from %s to %s",
	                        src, dst);
	                dumpData(buf, pUdpPayload, npduLen);
	                vxsFreeSockaddr(vxsSrc);
	            }
	            // Finally, send the LS/IP UDP packet
	            int nBytes = vxsSendTo( m_sendSocket, (LPSTR)msgBuffer, msglen, 0, vxsDest );
	            vxsFreeSockaddr( vxsDest );
	            if ( nBytes == msglen )
	            {
	                sts = LTSTS_OK;
	            }
	        }
	        else
	        {
	            vxlReportUrgent("ipv6_convert_ltvx_to_ls_udp failed!\n");
	            dumpData("ipv6_convert_ltvx_to_ls_udp failed!: **", pData, len);
	        }
	    }
	    else
	    {
	        dumpData("DriverWrite: **", pData, len);
	    }
	}

	return(sts);
}

void LonLinkIzoTRNIEthLink::sendAnnouncement(const uint8_t *ltVxmsg, uint8_t msgLen) {
}

LtSts LonLinkIzoTRNIEthLink::setUnicastAddress(int stackIndex, int domainIndex, int subnetNodeIndex,
                            byte *domainId, int domainLen, byte subnetId, byte nodeId) {
    return LTSTS_OK;
}

void LonLinkIzoTRNIEthLink::deregisterStack(int stackIndex)
{
}

LtSts LonLinkIzoTRNIEthLink::updateGroupMembership(int stackIndex, int domainIndex, LtGroups &groups)
{
    return LTSTS_OK;
}

int LonLinkIzoTRNIEthLink::queryIpAddr(LtDomain &domain, byte subnetId, byte nodeId, byte *ipAddress)
{
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifreq));
    ifr.ifr_addr.sa_family = AF_INET;
    memcpy(ifr.ifr_name, m_name, strlen(m_name));
    ioctl(m_lonSocket, SIOCGIFADDR, &ifr);
    ipAddress[0] = (((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr) & 0xff;
    ipAddress[1] = ((((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr)>>8) & 0xff;
    ipAddress[2] = ((((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr)>>16) & 0xff;
    ipAddress[3] = ((((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr)>>24) & 0xff;
    return 4;
}

void LonLinkIzoTRNIEthLink::setLsAddrMappingConfig(int stackIndex,
                                ULONG lsAddrMappingAnnounceFreq,
                                WORD lsAddrMappingAnnounceThrottle,
                                ULONG lsAddrMappingAgeLimit)
{
}

void LonLinkIzoTRNIEthLink::sendToProtocolAnalyser(byte *pData, bool crcIncluded)
{
    if  (_paFn)
    {
        short len =  pData[1];
        if (!crcIncluded)
        {
            // Add two crc bytes because the Protocol Analyzer expects it
            // Zero out the CRC bytes
            pData[len+2] = 0x00;
            pData[len+3] = 0x00;
            // Increment length field inside SICB
            pData[1] = len + 2;
            // Increment length of buffer sent to LSPA.mod
            len += 2;
        }
        (*_paFn)((char*)pData, len + 2);
        if (!crcIncluded)
        {
            // Decrement length field inside SICB back to original
            pData[1] = len - 2;
        }
    }
}

void LonLinkIzoTRNIEthLink::registerProtocolAnalyzerCallback(void (*fn)(char*, int))
{
    _paFn = fn;
}

void LonLinkIzoTRNIEthLink::unregisterProtocolAnalyzerCallback()
{
    _paFn = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Debugging
///////////////////////////////////////////////////////////////////////////////

// Trace data
void LonLinkIzoTRNIEthLink::dumpData(const char *title, const void *pData, short len)
{
    // Print time stamp
    char *pAsctimeStr;
    time_t dateTime;
    time(&dateTime);  // Get time in seconds
    pAsctimeStr = asctime(localtime(&dateTime));  // get local time as string
    // Remove the \n
    char *pos = strchr(pAsctimeStr, '\n');
    *pos = '\0';
    vxlReportEvent("[%s]", pAsctimeStr);
    vxlReportEvent("%s (len=%d) ", title, len);
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
    else
        vxlReportEvent("\n");
}
