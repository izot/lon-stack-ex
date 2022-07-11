/*
 * LonLinkIzoTRNIEthLink.h
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
 */

#ifndef LONLINKIZOTRNIETHLINK_H_
#define LONLINKIZOTRNIETHLINK_H_

#include "LonLinkIzoT.h"
#include "IzoTDevSocketMaps.h"
#include <sys/socket.h>
#include <linux/if.h>

class LonLinkIzoTRNIEthLink: public LonLinkIzoT
{
public:
	LonLinkIzoTRNIEthLink(int ipManagementOptions);
	virtual ~LonLinkIzoTRNIEthLink();
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
    static LonLinkIzoTRNIEthLink* getInstance();
protected:
    virtual LtSts driverOpen(const char* pName);
    virtual void driverClose();
    virtual LtSts driverRead(void *pData, short len);
    virtual LtSts driverWrite(void *pData, short len);
    virtual void sendAnnouncement(const uint8_t*, uint8_t);
    void dumpData(const char *title, const void *pData, short len);

    // The socket index of the last socket that was read.  Sockets are read
    // in round robin fashion
    int m_lastSocketRead;
private:
    int m_lonSocket;
    int m_sendSocket;
    int  m_ipManagementOptions;     // Bitmap of LONLINK_IZOT_MGMNT_OPTION_* options
    char m_name[IFNAMSIZ];
    uint8_t m_lastMsg[MAX_ETH_PACKET];
    uint8_t m_lastMsgSize;

    // protocol Analyzer callback routine
    void (*_paFn)(char*, int);

    static LonLinkIzoTRNIEthLink *m_instance;
};

#endif /* LONLINKIZOTRNILTLINK_H_ */
