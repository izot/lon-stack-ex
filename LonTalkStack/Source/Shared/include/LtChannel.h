#ifndef LT_CHANNEL_H
#define LT_CHANNEL_H
//
// LtChannel.h
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
//

// get our lockable base class
#include <VxClass.h>
#include "LtEvents.h"
#include "LtEventServerBase.h"

class LtLreServer;

LTA_EXTERNAL_FN void ltShutdown(void);

// This class represents a LonTalk channel.  LonTalk channels are defined
// by LonTalkIP-LonTalkIP routers.
class LTA_EXTERNAL_CLASS LtLonTalkChannel
{
private:
public:
};

// For backward compatibility, equate LtChannel to LtLogicalChannel.
// Remove after porting all source.
#define LtChannel LtLogicalChannel
class LtLogicalChannel;
class LtLreClient;

#ifdef WIN32
// Explicitly instantiate this template class so we can attach the
// __declspec() attribute to it for the DLL implementation.
// The Tornado compiler (GNU) cannot handle this.
template class LTA_EXTERNAL_CLASS LtTypedVector<LtObject>;
#endif

class LTA_EXTERNAL_CLASS LtObjectVector : public LtTypedVector<LtObject>
{
};

#ifdef WIN32
// Explicitly instantiate this template class so we can attach the
// __declspec() attribute to it for the DLL implementation.
// The Tornado compiler (GNU) cannot handle this.
template class LTA_EXTERNAL_CLASS LtTypedVector<LtLogicalChannel>;
#endif

class LTA_EXTERNAL_CLASS LtChannelVector : public LtTypedVector<LtLogicalChannel>
{
};

// This class represents a LonTalk logical channel.  A LonTalk channel can
// be separated into multiple logical channels by LonTalkIP Proxies.
// Each logical channel is constructed with a reference to its parent
// LonTalk channel.
class LTA_EXTERNAL_CLASS LtLogicalChannel : public LtObject, public VxcLock, public LtEventServerBase
{
private:
	LtErrorType								m_startError;

	static LtChannelVector					m_channels;
    LtLonTalkChannel*						m_pLonTalk;
	boolean									m_bCreated;
	boolean									m_bCreatedServer;

	int										m_nClientsPending;
	boolean									m_bIpChannel;
	boolean									m_bLayer5Intf;
	int										m_xcvrId;

	LtLreServer*							m_pLreServer;

	char								    m_szDisplayName[16];
	// These clients have the RIGHT STUFF for the master object to
	// request routing information.
	// must use blind vector here to avoid circular object references
	// members below are still typesafe.
	LtObjectVector							m_vStackClients;

public:
	// Each channel in the system has a channel object.  Each LRE
	// client is associated with a channel.
	// Get channels.  First call specifies bFirst == true.  Callee sets to
	// false and cycles index.
	static boolean getLogicalChannel(boolean& bFirst, int& nIndex, LtLogicalChannel **ppResult);

    LtLogicalChannel(boolean bIpPort, LtLonTalkChannel* pLonTalk=NULL, LtLreServer* pLreServer=NULL);
	virtual ~LtLogicalChannel();

    LtLonTalkChannel* getLonTalk()  { return m_pLonTalk; }

    boolean getIpChannel()	{ return m_bIpChannel; }
    class LtLtLogicalChannel *getLtLtLogicalChannel(void);

	LtLreServer*	getLre();
	virtual void setLre(LtLreServer* pLre);

	void setDisplayName(const char* szName) { strcpy(m_szDisplayName, szName); }
	char* getDisplayName() { return m_szDisplayName; }

	// register stack clients that know the routing info
	void registerStackClient( LtLreClient* pClient );
	// deregister a stack client
	void deregisterStackClient( LtLreClient* pClient );
	// enumerate the stack clients
	// call lock() before and unlock() after a string of
	// calls to the following to preserve integrity
	boolean enumStackClients(LtVectorPos& vectorPos, LtLreClient** ppValue);
	void stackClientCreated();
	void allStacksCreated();

	virtual boolean isLayer5Interface() { return false; }

	int getXcvrId() { return m_xcvrId; }
	void setXcvrId(int xcvrId) { m_xcvrId = xcvrId; }

	void setStartError(LtErrorType err) { m_startError = err; }
	virtual LtErrorType getStartError() { return m_startError; }

	virtual LtErrorType waitForPendingInterfaceUpdates(void) { return LT_NO_ERROR; }

#if FEATURE_INCLUDED(IZOT)
    virtual boolean getIsIzoTChannel(void) { return FALSE; }
    virtual class LonLinkIzoT *getLonLinkIzoTDev() { return NULL; }
#endif
};

class LtLreServer;
class LtIpPortClient;
class LtpcPktAllocator;
class LtLink;

#ifdef WIN32
class LtLtLogicalChannelRegParms
{
public:
    LtLtLogicalChannelRegParms() {};
    virtual ~LtLtLogicalChannelRegParms() {};

    virtual void getInterfaceTimeouts(int &openTimeout, int &responseTimeout, int &openRetries) = 0;

    // The following access routines retrieve information from the device registry, if
    // it is available, and returns TRUE, otherwise returns FALSE.
    virtual boolean getRegBufferSizes(int &maxSicbData) = 0;
    virtual boolean getRegAdvancedTxTickler(int &advancedTxTickler) = 0;
    virtual boolean getRegNmVersion(int &nmVersion, int &nmCapabilities) = 0;
    virtual void getRegSupportsEncryption(boolean &supportsEncryption) = 0;
    virtual void getLdvEventRegistrationParameters(HANDLE &hWnd, int &tag) = 0;
    virtual boolean getConnectionTerminated(void) = 0;
};
#endif

class LTA_EXTERNAL_CLASS LtLtLogicalChannel : public LtLogicalChannel
{
private:
	char			   *m_szPort;
	int					m_minLayer;
	int					m_maxLayer;
	boolean				m_bNsa;
	LtIpPortClient*		m_pLtPortClient;
	LtpcPktAllocator*	m_pAllocator;
	LtLink*				m_pLonLink;

	// Layer 5 MIP port control
	void*				m_pInitBlock;

    // Note that the allocator count (maxChannelPackets) must allow for allocation
	// of buffers to the LONC receive queues and must also allow
	// for cloning of messages in the engine (both deep and 
	// shallow).  

	// Number of packets allows for aggregation on all clients
	// but the dominating factor is line speed so allow for a high speed
	// line.  Number includes priority and non-priority queues.

    // The default queue deptsh are set to a fairly large number mainly because 
    // the layer2 test requires that a bunch of packets be sent at one time.
    void init(const char* szName, LtLink* pLink, boolean bForceL2, 
              boolean isIzoTChannel = false,
              int maxChannelPackets = 240, 
              int receiveQueueDepth = 60, 
              int transmitQueueDepth = 100); 

#ifdef WIN32
    boolean m_bConnectionEstablished;
    ULONG m_openTime;
    int m_openTimeout;
    int m_responseTimeout;
    int m_openRetries;
    
    LtLtLogicalChannelRegParms *m_pRegParms;
#endif
    boolean m_bIsIzoTDevChannel;

public:
	// To open a specific channel, specify the port name.  Generally, it is
	// not necessary to specify a link object so let this default to null.
	LtLtLogicalChannel(const char* szName = null, LtLreServer* pLre = null, LtLink* pLink = null, boolean bForceL2=false);
	LtLtLogicalChannel(const char* szName, int maxChannelPackets, int receiveQueueDepth, int transmitQueueDepth);
	~LtLtLogicalChannel();
	const char* getName() { return m_szPort; }
	int getMinLayer() { return m_minLayer; }
	int getMaxLayer() { return m_maxLayer; }
	boolean getNsaMip() { return m_bNsa; }
	void setLre(LtLreServer* pLre);
	LtLink* getLonLink() { return m_pLonLink; }
	LtIpPortClient* getLtpc() { return m_pLtPortClient; }
	boolean isLayer5Interface() { return getMaxLayer() >= 4; }
	LtErrorType getStartError();
	void getL5Control(void** pInitBlock);
    boolean getConnectionTerminated(void);
    boolean getIsIzoTChannel(void) { return m_bIsIzoTDevChannel; }
#if FEATURE_INCLUDED(IZOT)
    class LonLinkIzoT *getLonLinkIzoTDev();

    // To open an LonLinkIzoTDev channel
	LtLtLogicalChannel(const char* szIzoTName, const char *szIpIfName, int ipManagementOptions
#ifdef WIN32
		, LtLtLogicalChannelRegParms *pRegParms = null
#endif
		);
	LtLtLogicalChannel(const char* szName, LtLreServer* pLre, LtLink* pLink, boolean bForceL2, boolean bIsIzoTDevChannel);
#ifndef WIN32
	LtLtLogicalChannel(const char* szName, int ipManagementOptions);
#endif
#endif

#ifdef WIN32
    LtLtLogicalChannel(const char* szName, LtLtLogicalChannelRegParms *pRegParms);
 	void connectionEstablished();
    boolean openPending();

    void getInterfaceTimeouts(int &openTimeout, int &responseTimeout, int &openRetries);

    // The following access routines retrieve information from the device registry, if
    // it is available, and returns TRUE, otherwise returns FALSE.
    boolean getRegBufferSizes(int &maxSicbData);
    boolean getRegAdvancedTxTickler(int &advancedTxTickler);
    boolean getRegNmVersion(int &nmVersion, int &nmCapabilities);
    void getRegSupportsEncryption(boolean &supportsEncryption);
    void getLdvEventRegistrationParameters(HANDLE &hWnd, int &tag);

#endif
    LtErrorType setUnicastAddress(int stackIndex, int domainIndex, int subnetNodeIndex, 
                                  LtDomainConfiguration* pDc);
    LtErrorType updateGroupMembership(int stackIndex, int domainIndex, class LtGroups &groups);
    void setLsAddrMappingConfig(int stackIndex, ULONG lsAddrMappingAnnounceFreq, 
                                WORD lsAddrMappingAnnounceThrottle, ULONG lsAddrMappingAgeLimit);

#if FEATURE_INCLUDED(IZOT)
        // Query the IP address that this device would use when sending a message with
        // the specified source address
    int queryIpAddr(LtDomain &domain, byte subnetId, byte nodeId, byte *ipAddress);
#endif

    void deregisterStack(int stackIndex);
};


#endif

