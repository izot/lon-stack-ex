/*
 * LonLinkIzoTLtLink.h
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

#ifndef LONLINKIZOTLTLINK_H_
#define LONLINKIZOTLTLINK_H_

#include "LonLinkIzoT.h"
#include "IzoTDevSocketMaps.h"
#ifdef linux
#include <sys/socket.h>
#include <linux/if.h>
#else
#include    <windows.h>
#ifndef IFNAMSIZ
#define	IFNAMSIZ	16
#endif
#endif

class LonLinkIzoTLtLink: public LonLinkIzoT {
public:
	LonLinkIzoTLtLink();
	virtual ~LonLinkIzoTLtLink();
	virtual boolean		isOpen()
	{
        return m_isOpen;
	}
    virtual LtSts setUnicastAddress(int stackIndex, int domainIndex, int subnetNodeIndex,
                            byte *domainId, int domainLen, byte subnetId, byte nodeId);
    virtual void deregisterStack(int stackIndex);
    virtual LtSts updateGroupMembership(int stackIndex, int domainIndex, LtGroups &groups);
    virtual int queryIpAddr(LtDomain &domain, byte subnetId, byte nodeId, byte *ipAddress);
    virtual void setLsAddrMappingConfig(int stackIndex,
                                ULONG lsAddrMappingAnnounceFreq,
                                WORD lsAddrMappingAnnounceThrottle,
                                ULONG lsAddrMappingAgeLimit);
    // Support for L2 protocol analyzer
	void registerProtocolAnalyzerCallback(void (*fn)(char*, int));
	void unregisterProtocolAnalyzerCallback();
	void sendToProtocolAnalyser(byte *pData, bool crcIncluded);

  	static LonLinkIzoTLtLink* getInstance();
protected:
	virtual LtSts driverOpen(const char* pName);
	virtual void driverClose();
	virtual LtSts driverRead(void *pData, short len);
	virtual LtSts driverWrite(void *pData, short len);
	virtual void sendAnnouncement(const uint8_t*, uint8_t);
	void dumpData(const char *title, const void *pData, short len);

private:
	int sockfd;
	char m_name[IFNAMSIZ];

	// protocol Analyzer callback routine
	void (*_paFn)(char*, int);

	static LonLinkIzoTLtLink *m_instance;
};

#endif /* LONLINKIZOTLTLINK_H_ */
