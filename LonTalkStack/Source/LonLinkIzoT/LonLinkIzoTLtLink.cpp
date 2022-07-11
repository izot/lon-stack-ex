/*
 * LonLinkIzoTLtLink.cpp
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
#include <linux/if_ether.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include "include/LonLinkIzoTLtLink.h"
#include "LtMip.h"
#include "ipv6_ls_to_udp.h"
#include "time.h"

LonLinkIzoTLtLink* LonLinkIzoTLtLink::m_instance = NULL;

LonLinkIzoTLtLink* LonLinkIzoTLtLink::getInstance()
{
	return m_instance;
}

LonLinkIzoTLtLink::LonLinkIzoTLtLink(): LonLinkIzoT(1)
{
	_paFn = NULL;

	m_instance = this;
}

LonLinkIzoTLtLink::~LonLinkIzoTLtLink()
{
	_paFn = NULL;
}

//int sockfd;

LtSts LonLinkIzoTLtLink::driverOpen(const char* pName)
{
    LtSts   sts = LTSTS_ERROR;

	//0x8950 = defined protocol type for lontalk in kernel module
	if((sockfd = vxsRawSocket(SOCK_RAW, 0x8950)) == -1) {
		perror("socket");
		fprintf(stderr,"driverOpen: vxsRawSocket returns error\n");
		return sts;
	}

	if (vxBindToInterface(sockfd, pName, 0) != 0)
	{
	    fprintf(stderr,"driverOpen: failed to bind to %s\n", pName);
	    return sts;
	}

	sts = LTSTS_OK;
	m_sockets.addSocket(sockfd);
	m_bLinkOpen = true;
	m_isOpen = 1;
	return sts;
}

void LonLinkIzoTLtLink::driverClose() {
	vxsCloseSocket(sockfd);
}

LtSts LonLinkIzoTLtLink::driverRead(void *pData, short len) {
	LtSts	sts = LTSTS_ERROR;
    char            msgBuffer[MAX_ETH_PACKET];
    VXSOCKADDR      sourceAddr;   // Source address of last read
    int             msgBufferLen = 0;
    LtL2Sicb *pMsg = (LtL2Sicb*)pData;
	uint8_t domain[6];
	uint8_t domainlen;
    // Block until a message comes in, and buffer the message...
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if (vxsSelectRead(sockfd, &tv)) {
    	msgBufferLen = vxsRecv(sockfd, msgBuffer, sizeof(msgBuffer), 0);
    }
    memcpy(pData, msgBuffer, msgBufferLen);
	uint8_t * pPdu = &pMsg->data[0];

	if (IPV6_LT_IS_VER_LS_ENCAPSULATED_IP(pPdu[IPV6_LTVX_NPDU_IDX_TYPE]))
	{
		fprintf(stderr, "driverRead error is IPV6\n");
		return LTSTS_ERROR;
	}
	if (msgBufferLen && !(pMsg->cmd == MI_COMM || pMsg->cmd == MI_NETMGMT))
	{
		fprintf(stderr, "driverRead error is MI_COMM or MI_NETMGMT\n");
		dumpData("driverRead", (uint8_t *)pData, msgBufferLen);
		return LTSTS_ERROR;
	}
	if (msgBufferLen)
		dumpData("driverRead", (uint8_t *)pData, msgBufferLen);
    return msgBufferLen ? LTSTS_OK : LTSTS_ERROR;
}


LtSts LonLinkIzoTLtLink::driverWrite(void *pData, short len) {
	LtSts	sts = LTSTS_ERROR;
	char *pPacket = (char *)pData;
	// LtL2Sicb *msg = (LtL2Sicb*)pPacket;
	int nBytes = write( sockfd, (char*)pPacket, len);
	//int nBytes = write( sockfd, (char*)msg->data, msg->len);
	//int nBytes = vxsSend( sockfd, (char*)msg->data, msg->len, 0);
	if ( nBytes == len )
	{
		sts = LTSTS_OK;
	}
	dumpData("driverWrite", (uint8_t *)pPacket, nBytes);
	return(sts);
}

void LonLinkIzoTLtLink::sendAnnouncement(const uint8_t *ltVxmsg, uint8_t msgLen) {
}

LtSts LonLinkIzoTLtLink::setUnicastAddress(int stackIndex, int domainIndex, int subnetNodeIndex,
                            byte *domainId, int domainLen, byte subnetId, byte nodeId) {
	return LTSTS_OK;
}

void LonLinkIzoTLtLink::deregisterStack(int stackIndex) {
}

LtSts LonLinkIzoTLtLink::updateGroupMembership(int stackIndex, int domainIndex, LtGroups &groups) {
	return LTSTS_OK;
}

int LonLinkIzoTLtLink::queryIpAddr(LtDomain &domain, byte subnetId, byte nodeId, byte *ipAddress) {
    struct ifreq ifr = { 0 };

    ifr.ifr_addr.sa_family = AF_INET;
    memcpy(ifr.ifr_name, m_name, strlen(m_name));
    ioctl(sockfd, SIOCGIFADDR, &ifr);
	ipAddress[0] = (((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr) & 0xff;
	ipAddress[1] = ((((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr)>>8) & 0xff;
	ipAddress[2] = ((((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr)>>16) & 0xff;
	ipAddress[3] = ((((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr)>>24) & 0xff;
    return 4;
}

void LonLinkIzoTLtLink::setLsAddrMappingConfig(int stackIndex,
                                ULONG lsAddrMappingAnnounceFreq,
                                WORD lsAddrMappingAnnounceThrottle,
                                ULONG lsAddrMappingAgeLimit) {
}

void LonLinkIzoTLtLink::sendToProtocolAnalyser(byte *pData, bool crcIncluded)
{
	if  (_paFn)
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
		(*_paFn)((char*)pData, len + 2);
		if (!crcIncluded)
		{
			// Decrement length field inside SICB back to original
			pData[1] = len - 2;
		}
	}
}

void LonLinkIzoTLtLink::registerProtocolAnalyzerCallback(void (*fn)(char*, int))
{
	_paFn = fn;
}

void LonLinkIzoTLtLink::unregisterProtocolAnalyzerCallback()
{
	_paFn = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Debugging
///////////////////////////////////////////////////////////////////////////////

// Trace data
void LonLinkIzoTLtLink::dumpData(const char *title, const void *pData, short len)
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
#endif
}
