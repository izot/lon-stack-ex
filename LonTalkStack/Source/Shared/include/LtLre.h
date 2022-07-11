#ifndef LTLRE_H
#define LTLRE_H

//
// LtLre.h
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

typedef enum
{
	LT_NODE,
	LT_ROUTER_SIDE,
	LT_LONTALK_PORT,
	LT_IP_SOCKET,
} LreClientType;

class LtLreClientOwner
{
private:
	int			m_nPacketNumber;

protected:
	friend class LtLre;
	int getPacketNumber() { return m_nPacketNumber; }
	void setPacketNumber(int packetNumber) { m_nPacketNumber = packetNumber; }

public:
	LtLreClientOwner()
	{
		m_nPacketNumber = -1;
	}
};


#if PRODUCT_IS(ILON) || PRODUCT_IS(IZOT)

// Global functions to control the routing of "internal" packets to port clients
// Use 'int' instead of 'boolean' to allow accessing from shell
extern int sendInternalPacketsToPort;
extern int xmitInternalPacketsOnPort;

#endif

class LtLreClient : public LtEventClient, public LtEventServerBase, public LtLreClientOwner
{
protected: 
	LtChannel*	m_pChannel;
	LreClientType m_clientType;
    boolean		m_bNodeType;

public:
	LtLreClient(LreClientType clientType, LtChannel *pChannel) : m_pChannel(pChannel), m_clientType(clientType) 
    {
        m_bNodeType = m_clientType == LT_NODE || m_clientType == LT_ROUTER_SIDE;
    }

    virtual ~LtLreClient() {}

	// Each client is notified when a packet needs to be
	// processed.  It is up to the client whether the
	// packet is processed directly or via another task.
	// If the packet is processed directly, that processing
	// must be short and can not suspend or block the 
	// current task. 
	virtual void processPacket(boolean bPriority, LtPktInfo *pPkt) {}

	// Every client is notified when a packet needs to be
	// count (for stats purposes).  
	virtual void updateStats(const byte* pApdu, bool isUnackd, bool domainMatch, bool gotThisMsg) {}

	// Each client has a Logical Channel identity.  Logical Channels are subsets of
	// LonTalk channels as defined by LTIP Proxies.  LonTalk Channels are defined
	// by LTIP Routers and LTIP-LTIP Routers.  Note that the LtChannel points to the
	// owning LonTalk Channel.
	LtLogicalChannel* getLogicalChannel() 
	{
		return m_pChannel;
	}

    LtLonTalkChannel* getLonTalkChannel()
    {
        return m_pChannel->getLonTalk();
    }

	// Every client has an ID.  This ID is used to enforce the rule that no client
	// will get a packet from another client with the same ID.  The default for
	// this method is to use the channel object as the ID.  This is appropriate
	// for IP sockets and LonTalk ports where packets shouldn't be rerouted onto
	// the same channel.  For nodes and router sides, this ID should be something
	// like the layer 4 object (this allows a single stack to have multiple 
	// clients without packets being routed back to itself).
	virtual void* getId()
	{
		// IP socket or LonTalk port appropriate default
		return (void*) m_pChannel;
	}

	// Each client has an owner.  For node and router side clients, the owner is the
	// layer 4 object.  For IP sockets and LonTalk ports, the owner is unique per
	// client (perhaps the client itself.  The owner is used to enforce the rule that
	// a packet only routed once to an owner.  That is, if the owner has multiple clients,
	// at most one client should get the packet.  
	virtual LtLreClientOwner* getOwner()
	{
		// IP socket or LonTalk port appropriate default
		return this;
	}

	// Each client has a client type.  Not used by engine.  Might be
	// used by other clients.
	LreClientType getClientType()
	{
		return m_clientType;
	}

	// Controls interchannel routing.  If false, no packets are routed
	// cross channel.  This handles the exceptional case of a 
	// LT_ROUTER_SIDE client which must not route to the far side
	// channel.
	virtual boolean routeInterchannel()
	{
		return m_clientType != LT_ROUTER_SIDE;
	}

	// Returns true if the client is in a one to many owner
	// to client relationship.  Such clients register
	// the same unique id multiple times, once per configured
	// domain (and once on no domain).
	virtual boolean getIsMulti()
	{
		return false;
	}

	// Returns false if the client doesn't want packets that already have a
	// specific client.  For example, the LonTalk port won't get subnet/node
	// packets destined for an internal subnet/node address.  However, a repeater
	// will get these packets (mainly for debugging).
	virtual boolean routeMultiple()
	{
#if PRODUCT_IS(ILON)
		// Allow overriding the default behavior (mostly for debugging)
		if (xmitInternalPacketsOnPort)
		{
			return true;
		}
#endif
		return m_clientType != LT_LONTALK_PORT;
	}

	// Used to start up or stop the client.  true is returned if the function
	// succeeded.
	virtual boolean start() 
	{ 
		return true; 
	}
	virtual boolean stop() 
	{ 
		return true; 
	}

    // Controls whether source subnet/node filtering is used
    // before routing to this client.  This is mainly for 
    // LonTalk compatibility.  The rule is that if a destination
    // client returns true, then if the source address is received
    // by a client with the same id (getId()), then the packet
    // is not delivered.  This is really only intended for
    // non-router clients.  Using it for routers has not been
    // analyzed completely (might reduce traffic for rare cases 
    // involving redundant routers?  then again, might cause lost
    // packets?).
    virtual boolean sourceAddressFilter()
    {
        return m_bNodeType;
    }

	// The following methods are queried by the LRE to get routing
	// configuration.  If the data returned by these methods changes,
	// the LRE must be notified via an event so that it can rebuild
	// its routing tables.

	// Used to get the routes associated with a client determined by
	// IP or LonTalk routing messages.  Function returns true if it succeeds, 
	// false if no more routes.  Index is expected to be 0..1 to cover
	// two domains.
	virtual boolean getRoute(int& index, LtRoutingMap *pRoute)
	{
		return false;
	}

	// Function returns address specified by index, 0..N-1.  Returns true if
	// successful, else false.  For stack clients, index will typically be
	// 0 or 0..1 to allow one or two domains.  For IP clients, index might
	// be large since an IP device might have any number of stacks attached
	// to the IP channel.
	virtual boolean getAddress(int& index, LtDomain *d, LtSubnetNode *s, LtGroups *g)
	{
		return false;
	}

	// Function used to specify an individual group address for a client.  More
	// efficient than above format for specifying individual group address clients.
	virtual boolean getAddress(int& index, LtDomain *d, int& group)
	{
		return false;
	}

	// Function returns the unique ID(s) associated with this client.  false means
	// there is none.
	virtual boolean getAddress(int& index, LtUniqueId *u)
	{
		return false;
	}

	// Used to get the route(s) to be advertised to the rest of the world. 
	// Only supported by the Router Clients.
	virtual boolean getExternalRoute(int& index, LtRoutingMap *pRoute, int* pRoutingSubnet)
	{
		return false;
	}

	// Controls whether client wants all broadcasts.  This is useful
	// to handle the case where the client is an unconfigured node or
	// a router which proxies for an unconfigured node.  Unconfigured
	// nodes need all broadcasts, regardless of domain.  
	virtual boolean getNeedAllBroadcasts()
	{
		return false;
	}

	// Controls whether a client wants to have a valid CRC associated with
	// the packet.  Currently, LonTalk Port Clients always regenerate the
	// CRC and stack clients ignore the CRC.  However, IP clients demand
	// that the CRC be valid.  If the packet comes from a stack client and/or
	// has had it contents tweaks by the router algorithm, regeneration of 
	// the CRC is necessary.  Note that regenerating the CRC is not cheap
	// so it is only done when necessary (i.e., packet originated from a 
	// stack or had its source subnet modified).
	virtual boolean needValidCrc()
	{
		return m_clientType == LT_IP_SOCKET;
	}

    // Controls whether client wants all layer 2 packets, even if they are invalid
    virtual boolean getNeedAllLayer2Packets()
    {
        return false;
    }

	// Indicates whether the "owner" pointer can be safely cast to an LtDeviceStack.
	// This to get around requiring RTTI to safely do a dynamic cast to the derived LtDeviceStack object.
	virtual boolean ownerIsLtDeviceStack()
	{
		return false;
	}

    virtual void setErrorLog(int error) 
    {
    }

};

class LtLreServer : public LtEventClient, public LtEventServer
{
private:
	static LtTypedVector<LtLreServer> m_vecLre;

public:
    LtLreServer();
	virtual ~LtLreServer();

	// Use to enumerate LREs.  
	// "bFirst" must be true initially, called function clears it.
	// called function cycles "nIndex".
	// Returns true if found, else false.
	static boolean getLre(boolean& bFirst, int& nIndex, LtLreServer** ppResult);

	// Use to enumerate LRE clients for this LRE.
	// "bFirst" must be true initially, called function clears it.
	// called function cycles "nIndex".
	// Returns true if found, else false.
	virtual boolean getLreClient(boolean& bFirst, int& index, LtLreClient **pClient) = 0;

	// Each client invokes this function to inform the router
	// of its existence.  
	virtual void registerClient(LtLreClient *c, boolean bSync = false) = 0;

	// This method deregisters a routing client
	virtual void deregisterClient(LtLreClient *c) = 0;	

	// This method deregisters all clients for a given owner.
	virtual void deregisterOwner(LtLreClientOwner* ownerId) = 0;

	// This method places the packet onto an input queue
	// for the router task and schedules the router task.  
	virtual void routePacket(boolean bPriority,
							 LtLreClient *pClient,
							 LtPktInfo *pPkt) = 0;
	
	// This is used to get/set to set the offline state of the
	// router.  Multiple clients may write this state so it is
	// counted.  Thus, if a client sets offline twice it must
	// set online twice.  When offline, packets are only 
	// forwarded amongst clients on the same logical channel.
	// This method is typically used by the LT_ROUTER_SIDE client.
	virtual boolean getOffline() = 0;
	virtual void setOffline(boolean bOffline, LtLonTalkChannel* pChannel=null) = 0;

	// This method is used to get/set full LonTalk verbose mode. 
	// In this mode, the router delivers local stack messages onto 
	// the channel even if it is not necessary.  This may be useful
	// for debugging to allow LonTalk communications between local
	// stacks to be viewed from the LonManager Protocol Analyzer.
	// This capability could be turned on via a configuration
	// property, for example.
	virtual boolean getVerboseMode() = 0;
	virtual void setVerboseMode(boolean verbose) = 0;

	// Use these routines to start/stop routing.  Not to be confused with
	// online/offline.  Stop returns only once all packets have been
	// flushed from the input queue and all clients have "stopped".  The
	// LRE invokes the "start" or "stop" method on every registered client.
	// true is returned if the function was successful.
	virtual boolean start() = 0;
	virtual boolean stop() = 0;

	// Used to get the route(s) to be advertised to the rest of the world. 
	// Only needed for learning routers.
	virtual boolean getExternalRoute(LtDomain& domain, LtLonTalkChannel* pChannel, LtSubnets* pSubnets) = 0;

	// Event registration
	virtual void registerEventClient(LtEventClient* pClient) = 0;

	// Event deregistration
	virtual void deregisterEventClient(LtEventClient* pClient) = 0;

	// Get latest LRE error log
	virtual int getErrorLog() = 0;

	// Creates an instance of a derived class of this base class.
	static LtLreServer* create();

    virtual LtLreClient* getLreUniqueIdClient(LtUniqueId* pUniqueId) = 0;

};

#endif
