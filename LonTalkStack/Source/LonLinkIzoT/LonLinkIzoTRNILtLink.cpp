/*
 * LonLinkIzoTRNILtLink.cpp
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
#include "include/LonLinkIzoTRNILtLink.h"
#include "LtMip.h"
#include "ipv6_ls_to_udp.h"
#include "time.h"

#ifdef WIN32
#include <windows.h>
#else
#include <arpa/inet.h>
#define SOCKET_ERROR    -1  // In Win32, this is defined in WinSock.h
#endif

#include "VxSockets.h"
#include "LtCUtil.h"

LonLinkIzoTRNILtLink* LonLinkIzoTRNILtLink::m_instance = NULL;

LonLinkIzoTRNILtLink* LonLinkIzoTRNILtLink::getInstance()
{
	return m_instance;
}

LonLinkIzoTRNILtLink::LonLinkIzoTRNILtLink(): LonLinkIzoT(1)
{
	_paFn = NULL;
	m_lonSocket = -1;
	m_sendSocket = -1;

	m_instance = this;
	m_lastSocketRead = 0;
}

LonLinkIzoTRNILtLink::~LonLinkIzoTRNILtLink()
{
	_paFn = NULL;
}

LtSts LonLinkIzoTRNILtLink::driverOpen(const char* pName)
{
    LtSts   sts = LTSTS_ERROR;

    if((m_lonSocket = vxsRawSocket(SOCK_RAW, ETH_P_ALL)) == -1)
    {
        perror("socket");
        fprintf(stderr,"driverOpen: failed to open socket for receiving packets\n");
        return sts;
    }
    if (vxBindToInterface(m_lonSocket, pName, 0) != 0)
    {
        fprintf(stderr,"driverOpen: failed to bind to %s\n", pName);
        return sts;
    }
    m_sockets.addSocket(m_lonSocket);

    // The kernel module expects to have the message sends from 0x8950 protocol
    if((m_sendSocket = vxsRawSocket(SOCK_RAW, 0x8950)) == -1)
    {
        perror("socket");
        fprintf(stderr,"driverOpen: failed to open socket for sending packets\n");
        return sts;
    }
    vxBindToInterface(m_sendSocket, pName, 0);
    m_sockets.addSocket(m_sendSocket);
    sts = LTSTS_OK;
    strcpy(m_name, pName);
    m_bLinkOpen = true;
    m_isOpen = 1;
    return sts;
}

void LonLinkIzoTRNILtLink::driverClose()
{
    LonLinkIzoT::driverClose(); // Close the base IzoT driver
}

LtSts LonLinkIzoTRNILtLink::driverRead(void *pData, short len)
{
	LtSts	sts = LTSTS_ERROR;
    char            msgBuffer[MAX_UDP_PACKET];   // [MAX_ETH_PACKET];
    int             msgBufferLen = 0;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    if (vxsSelectRead(m_lonSocket, &tv))
    {
        struct sockaddr_ll inaddr;
        socklen_t addrlen = sizeof(inaddr);

        msgBufferLen = recvfrom(m_lonSocket, msgBuffer, sizeof(msgBuffer), 0,(struct sockaddr*)&inaddr, &addrlen);
#ifdef _DEBUG
        struct sockaddr_ll* sourceAddr = &inaddr;
        fprintf(stderr, "msg len %i PktTYpe=%d Proto=%x\n",
                msgBufferLen, sourceAddr->sll_pkttype, sourceAddr->sll_protocol);
#endif
        dumpData("driverRead1", (uint8_t *)msgBuffer, msgBufferLen);
        if (msgBufferLen && msgBufferLen != SOCKET_ERROR)
        {
            uint8_t *pPacket = (uint8_t *)msgBuffer;
            if (pPacket[0] == L2_PKT_TYPE_INCOMING)
            {
                // message from the FT devices.  Already in L2 packet format
                if (len < msgBufferLen)
                {
                    // packet too big.
                    fprintf(stderr,
                            "LonLinkIzoTDev::driverRead ERROR: L2 packet is too big to fit in ltvx buffer, LtVx len = %d, buffer = %d\n",
                            msgBufferLen, len);
                    sts = LTSTS_ERROR;
                }
                else
                {
                    memcpy(pData, msgBuffer, msgBufferLen);
                    LtL2Sicb *pMsg = (LtL2Sicb*)pData;
                    uint8_t * pPdu = &pMsg->data[1];

                    if (IPV6_LT_IS_VER_LS_ENCAPSULATED_IP(pPdu[IPV6_LTVX_NPDU_IDX_TYPE]))
                    {
                        fprintf(stderr, "driverRead error is IPV6\n");
                        return LTSTS_ERROR;
                    }
                    if (!(pMsg->cmd == MI_COMM || pMsg->cmd == MI_NETMGMT))
                    {
                        fprintf(stderr, "driverRead error is MI_COMM or MI_NETMGMT\n");
                        dumpData("driverRead", (uint8_t *)pData, msgBufferLen);
                        return LTSTS_ERROR;
                    }
                    sts = LTSTS_OK;
                }
            }
            else if (pPacket[0] == 0x12 || pPacket[0] == 0x13)
            {
                int dataOffset = 2;
                int packetLen = msgBufferLen;

                if (pPacket[1] == L2_PKT_LEN_EXTENDED)
                {
                    // Use extended length
                    dataOffset += 2;
                }

                if (packetLen > dataOffset)
                {
                    unsigned char *pNpdu = (unsigned char *)pPacket + dataOffset;
                    packetLen =  packetLen-dataOffset;  // Skip SICB header

                    LonLinkIzoTLock lock(m_lock);   // LOCK

                    uint8_t *pPacketBuf = (uint8_t *)pData;

                    // Make sure packet buf has room for 2 byte SICB header plus 2 byte CRC
                    if (len < packetLen+4)
                    {
                        // packet too big.
                        fprintf(stderr,
                            "LonLinkIzoTDev::driverRead ERROR: L2 packet is too big to fit in ltvx buffer, LtVx len = %d, buffer = %d\n",
                            packetLen+4, len);
                        sts = LTSTS_ERROR;
                    }
                    else if (packetLen == 0)
                    {
                        fprintf(stderr, "LonLinkIzoTDev::driverRead ERROR: invalid L2 packet, source\n");
                        dumpData("L2 packet", (uint8_t *)msgBuffer, msgBufferLen);
                    }
                    else
                    {
                        // Calculate the CRC
                        int dataOffset = 2;
                        LtCRC16(pNpdu, packetLen);
                        packetLen += 2;   // Adjust len to include CRC

                        // Set the SICB header
                        pPacketBuf[0] = L2_PKT_TYPE_INCOMING;
                        if (packetLen >= L2_PKT_LEN_EXTENDED)
                        {
                            pPacketBuf[1] = L2_PKT_LEN_EXTENDED;
                            memcpy(&pPacketBuf[2], &packetLen, 2);
                            dataOffset += 2;
                        }
                        else
                        {
                            pPacketBuf[1] = (uint8_t)packetLen;
                        }

                        // Set the pdu
                        memcpy(pPacketBuf+dataOffset, pNpdu, packetLen);
                        dumpData("driverRead2", (uint8_t *)pPacketBuf, packetLen + dataOffset);
                        sts = LTSTS_OK;
                    }
                }
            }
        }
        else
        {
            // Nothing to read.  LonLink expects LTSTS_ERROR for that.
            sts = LTSTS_ERROR;
        }
    }

    return sts;
}


LtSts LonLinkIzoTRNILtLink::driverWrite(void *pData, short len)
{
	LtSts	sts = LTSTS_ERROR;
	char *pPacket = (char *)pData;
    int nBytes = write( m_sendSocket, (char*)pPacket, len);

    if ( nBytes == len )
        sts = LTSTS_OK;

    if (sts == LTSTS_OK)
         dumpData("driverWrite", (uint8_t *)pPacket, nBytes);

	return(sts);
}

void LonLinkIzoTRNILtLink::sendAnnouncement(const uint8_t *ltVxmsg, uint8_t msgLen) {
}

LtSts LonLinkIzoTRNILtLink::setUnicastAddress(int stackIndex, int domainIndex, int subnetNodeIndex,
                            byte *domainId, int domainLen, byte subnetId, byte nodeId) {
    return LTSTS_OK;
}

void LonLinkIzoTRNILtLink::deregisterStack(int stackIndex)
{
}

LtSts LonLinkIzoTRNILtLink::updateGroupMembership(int stackIndex, int domainIndex, LtGroups &groups)
{
    return LTSTS_OK;
}

int LonLinkIzoTRNILtLink::queryIpAddr(LtDomain &domain, byte subnetId, byte nodeId, byte *ipAddress)
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

void LonLinkIzoTRNILtLink::setLsAddrMappingConfig(int stackIndex,
                                ULONG lsAddrMappingAnnounceFreq,
                                WORD lsAddrMappingAnnounceThrottle,
                                ULONG lsAddrMappingAgeLimit)
{
}

void LonLinkIzoTRNILtLink::sendToProtocolAnalyser(byte *pData, bool crcIncluded)
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

void LonLinkIzoTRNILtLink::registerProtocolAnalyzerCallback(void (*fn)(char*, int))
{
	_paFn = fn;
}

void LonLinkIzoTRNILtLink::unregisterProtocolAnalyzerCallback()
{
	_paFn = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Debugging
///////////////////////////////////////////////////////////////////////////////

// Trace data
void LonLinkIzoTRNILtLink::dumpData(const char *title, const void *pData, short len)
{
#ifdef _DEBUG
	// Print time stamp
	char *pAsctimeStr;
	time_t dateTime;
	time(&dateTime);  // Get time in seconds
	pAsctimeStr = asctime(localtime(&dateTime));  // get local time as string
	// Remove the \n
	char *pos = strchr(pAsctimeStr, '\n');
	*pos = '\0';
	fprintf(stderr, "[%s]", pAsctimeStr);
	fprintf(stderr, "%s (len=%d) ", title, len);
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
            fprintf(stderr,"%s\n", buf);
            s = buf;
        }
    }
    if (s != buf)
    {
        *s = 0;
        fprintf(stderr,"%s\n", buf);
    }
    else
        fprintf(stderr,"\n");
#endif
}
