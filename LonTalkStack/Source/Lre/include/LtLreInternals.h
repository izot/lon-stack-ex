//
// LtLreInternals.h
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
    LRE_DOMAIN_BIN_UNQUALIFIED,

    LRE_DOMAIN_BIN_TYPES
} LreDomainBinType;

typedef enum
{
    LRE_GLOBAL_BIN_QUALIFIED,
    LRE_GLOBAL_BIN_UNQUALIFIED,
	LRE_GLOBAL_BIN_DZB,			// Domain zero, source subnet 0 broadcasts
    LRE_GLOBAL_BIN_ALL_PACKETS, // All packets, including invalid packets (good for PA)
    LRE_GLOBAL_BIN_ALL_CLIENTS, // A collection of all the clients

    LRE_GLOBAL_BIN_TYPES
} LreGlobalBinType;

typedef enum
{
	LRE_ADD_CLIENT,
	LRE_REMOVE_CLIENT,
	LRE_UPDATE_CLIENT,
	LRE_REMOVE_OWNER,
	LRE_DELETE_CLIENT,
} LreUpdateMsgCode;

typedef enum
{
	LRE_LONTALK_SIDE,
	LRE_IP_SIDE,

	LRE_ROUTER_SIDES
} LreRouterSides;

class LreUpdateMsg
{
public:
	LreUpdateMsg() { sem = null; }
	LreUpdateMsgCode  code;
	void*			  data;
	SEM_ID			  sem;
};

class LtLre;

//
// Define searchable forms of domain, subnet/node, uniqueid
//
class LtDomainKey : public LtHashKey 
{
private:
	LtDomain m_dom;
public:
	LtDomainKey(LtDomain& dom) : m_dom(dom) {}
	boolean operator==(LtHashKey& key) 
	{
		return m_dom == ((LtDomainKey&)key).m_dom;
	}
	LtDomain& getDomain() { return m_dom; }
	int hashCode() 
	{
		return m_dom.hashCode();
	}
};

class LtUniqueIdKey: public LtHashKey
{
private:
	LtUniqueId m_id;
public:
	LtUniqueIdKey(LtUniqueId& id) : m_id(id) {}
	int hashCode() { return m_id.hashCode(); }
	boolean operator ==(LtHashKey& key)
	{
		return m_id == ((LtUniqueIdKey&)key).m_id;
	}
};

class LtSubnetNodeKey: public LtHashKey 
{
private:
	LtSubnetNode m_sn;
public: 
	LtSubnetNodeKey(int node, int subnet) : m_sn(node, subnet) {}
	LtSubnetNodeKey(LtSubnetNode& sn) : m_sn(sn) {}
    inline int hashCode() { return m_sn.getNode() + (m_sn.getSubnet()<<16); }
    inline boolean operator==(LtHashKey& key)
    {
        return m_sn == ((LtSubnetNodeKey&)key).m_sn;
    }

};

class LtClients : public LtTypedVector<LtLreClient>
{
public:
    boolean idMatch(LtLreClient* pClient);
	boolean ownerMatch(LtLreClient* pClient, LtLreClient** ppMatchingClient = NULL);
	void remove(void* pRef);
};

class LtClientsVector : public LtTypedVector<LtClients>
{
public:
    void set(LtLreClient* pClient, LtRouteMap* pMap);
	void set(LtLreClient* pClient, int nIndex);
    void remove(void* pRef);
};

class LtNodeClients : public LtTypedHashTable<LtSubnetNodeKey, LtClients>
{
public:
    void remove(void* pClient);
};

class LtUniqueIdClients : public LtTypedHashTable<LtUniqueIdKey, LtLreClient>
{
public:
    void remove(void* pRef);
	void update(LtLreClient* pClient, LtUniqueId* pKey);
};

class LtSubnetGroupForwarding : public LtObject
{
private:
	void*			  m_pRef;
	LtLonTalkChannel* m_pChannel;
	LtSubnets		  m_subnets;
	LtGroups		  m_groups;
	int				  m_nDefaultSubnet;		
public:
	LtSubnetGroupForwarding(void* pRef, LtLonTalkChannel* pChannel, LtSubnets* pSubnets, LtGroups* pGroups, int nDefaultSubnet) :
		m_pRef(pRef), 
		m_pChannel(pChannel), 
		m_subnets(*pSubnets), 
		m_groups(*pGroups),
		m_nDefaultSubnet(nDefaultSubnet) {} 
	LtLonTalkChannel* getChannel() { return m_pChannel; }
	LtSubnets& getSubnets() { return m_subnets; }
	LtGroups& getGroups() { return m_groups; }
	void setSubnets(LtSubnets& subnets) { m_subnets = subnets; }
	void setGroups(LtGroups& groups) { m_groups = groups; }

	int getDefaultSubnet() { return m_nDefaultSubnet; }
	void setDefaultSubnet(int nSubnet) { m_nDefaultSubnet = nSubnet; }

	void* getRef() { return m_pRef; }
};

class LtSubnetGroupForwardings : public LtTypedVector<LtSubnetGroupForwarding>
{
public:
	void update(void* pRef, LtLonTalkChannel* pChannel, LtSubnets* pSubnets, LtGroups* pGroups, int nDefaultSubnet);
	LtLonTalkChannel* getChannel(int nSubnet);
	LtSubnetGroupForwarding* get(LtLonTalkChannel* pChannel);
	int getDefaultSubnet(LtLonTalkChannel* pChannel);
	void remove(void* pRef);
};

//
// For each domain, a route is maintained.  Domains are looked
// at by LtDomain.  Each route has:
class LreRouteControl;

//  1. a hashtable mapping subnet/node to clients, 
//  2. array indexed by subnet mapping to clients.
//  3. array indexed by group mapping to clients.
//
class LtLreDomain : public LtObject
{
private:
	LtDomain		m_domain;
    LtNodeClients   m_nodes;
    LtClientsVector m_subnets;
    LtClientsVector m_groups;
    LtClientsVector m_broadcastSubnets; // For broadcast only
	LtUniqueIdClients m_uniqueIds;
    LtClients       m_bin[LRE_DOMAIN_BIN_TYPES];
	LtSubnetGroupForwardings m_subnetGroups;

    void set(LtLreClient* pClient, LtRouteMap* pMap, LtClientsVector& vector);
    
public:
    ~LtLreDomain();
	void setLtDomain(LtDomain& domain) { m_domain.set(domain); }
	LtDomain* getLtDomain() { return &m_domain; }
	LtLreClient* get(LtUniqueIdKey *pKey);
    LtClients* get(int nSubnet);
    LtClients* getGroup(int nGroup);
    LtClients* getBroadcast(int nSubnet);
    LtClients* get(int subnet, int node);
    boolean configuredClients(LtClients *pClients);

	void set(LtLreClient* pClient, LtUniqueId* pUniqueId);
    void set(LtLreClient* pClient, LtSubnets* pSubnets);
    void set(LtLreClient* pClient, LtGroups* pGroups);
    void setBroadcast(LtLreClient* pClient, LtSubnets* pSubnets);
    void set(LtLreClient* pClient, LtSubnetNode* pAddress);
	void set(LtLreClient* pClient, int group);
    LtClients* getBin(LreDomainBinType binType) { return &m_bin[binType]; }
    void setConditionally(LtSubnets& subnets, LtLreClient* pCheckClient, int nSubnet);
    void remove(void* pRef);
    boolean isEmpty();
    boolean addressClient(LtSubnetNode& subnetNode, LtLreClient* pClient);
	void setSubnetsAndGroups(LtLreClient* pClient, LtLonTalkChannel* pChannel, LtSubnets* pSubnets, LtGroups* pGroups, int nDefaultSubnet);
	boolean forkable() { return m_subnetGroups.getCount() > 0; }
	int getDefaultSubnet(LtLonTalkChannel* pChannel) { return m_subnetGroups.getDefaultSubnet(pChannel); }
	LtSubnetGroupForwarding* getSubnetGroups(LtLonTalkChannel* pChannel);
	boolean updateRoutingInfo(LtLre* pLre, LtPktInfo* pPkt, LreRouteControl& routeCtrl);
};

class LtDomains : public LtTypedHashTable<LtDomainKey, LtLreDomain>
{
public:
    void remove(void* pRef);
};

class LreRouteControl
{
private:
	LtLreDomain*		m_pDomain;
	LtPktInfo*			m_pPkt;
	LtLonTalkChannel*	m_pSourceChannel;
	LtLonTalkChannel*	m_pOriginalChannel;
	boolean				m_bCrossChannel;
	boolean				m_bSame;
	boolean				m_bDiff;
	boolean				m_bSpecificTarget;
	LtRouterType		m_routerType;

public:
	LreRouteControl(LtPktInfo* pPkt, LtRouterType routerType, boolean bSame, boolean bDiff) : 
		m_pDomain(null), m_pPkt(pPkt), m_pOriginalChannel(null), m_bSame(bSame), m_bDiff(bDiff), m_routerType(routerType)
	{
		// All cross channel messages allow if repeater.  For all others, routing determined
		// by forwarding masks.
		m_bCrossChannel = routerType == LT_REPEATER;
		m_pSourceChannel = pPkt->getSourceClient()->getLonTalkChannel();
		m_bSpecificTarget = FALSE;
	}

	void setDomain(LtLreDomain* pDomain) { m_pDomain = pDomain; }
	void setOriginalLtChannel(LtLonTalkChannel* pChannel) { m_pOriginalChannel = pChannel; }
	void setCrossChannel(boolean bValue) { m_bCrossChannel = bValue; }
	void setSpecificTarget() { m_bSpecificTarget = TRUE; }

	inline LtPktInfo* getPkt() { return m_pPkt; }
	inline LtLonTalkChannel* getOriginalLtChannel() { return m_pOriginalChannel; }
	inline LtLonTalkChannel* getSourceLtChannel() { return m_pSourceChannel; }
	inline LtLreDomain* getDomain() { return m_pDomain; }
	inline boolean getSameChannelOnly() { return m_bSame; }
	inline boolean getDiffChannelOnly() { return m_bDiff; }
	inline boolean getCrossChannel() { return m_bCrossChannel; }
	inline boolean getSpecificTarget() { return m_bSpecificTarget; }
	inline LtRouterType getRouterType() { return m_routerType; }
};

typedef enum 
{
	LT_CLIENT_START,
	LT_CLIENT_STOP,
	LT_OWNER_REMOVE,
	LT_CLIENT_DELETE,
} LtClientFunction;

class LtLre;

// Allow for 2 router sides plus a non-committal side (null channel)
#define LT_ROUTER_SIDES 5

extern "C" int VXLCDECL LreEngineTask( int lre, ... );
extern "C" int VXLCDECL LreUpdateTask( int lre, ... );

class LreSideState
{
public:
	boolean bInUse;
	boolean bOffline;
	LtLonTalkChannel* pChannel;
};

class LtLre : public LtLreServer, public LtTaskOwner
{
	friend class LtLreDomain;

private:
	static LtTypedVector<LtLre> m_lres;		// Track all existing LREs

    LtClients m_bin[LRE_GLOBAL_BIN_TYPES];
    LtDomains m_domains;
    LtUniqueIdClients m_uniqueIds;
	LtClients m_routeList;
	boolean	  m_bFixupCrc;
	LtClients m_clients;

    LtRefQue m_pkts;
	int m_engineTaskId;
	int m_updateTaskId;
	SEM_ID m_semRun;
	SEM_ID m_semRoutingData;
	MSG_Q_ID m_msgQUpdate;
	boolean m_bUpdatesPending;
    int m_bOffline;
	LreSideState m_states[LT_ROUTER_SIDES];
    boolean m_bVerbose;
	boolean m_bRouting;
	int m_nPacketNumber;		// Used for duplicate detection.
	LtRouterType m_routerType;

	LtEventClientV	m_vClients;
	int m_errorLog;

	void gainUpdateAccess();
	void releaseUpdateAccess();

	void determineClientsAndRoute(LtPktInfo* pPkt, LtLreDomain* pEntry, boolean bSame, boolean bDiff, boolean bDzb);
	void routePkt(LtPktInfo* pPkt, LtLreClient* pDestClient, boolean bClone);
	void routePktToClientList(LtPktInfo* pPkt);
    void beginRoutingPkt(LtPktInfo* pPkt);
    void checkRoutingRulesForClientList(LtClients* pClients, LreRouteControl& routeCtrl);
    void checkRoutingRulesForClient(LtLreClient* pClient, LreRouteControl& routeCtrl);
    LtLreDomain* getDomainEntry(LtDomain* pDomain, boolean create=false);
    LtClients* getBin(LreGlobalBinType binType) { return &m_bin[binType]; }

    void registerCatchAll(LreDomainBinType binType, LtLreClient* pClient, LtDomain *pDomain);
    void registerCatchAll(LreGlobalBinType binType, LtLreClient* pClient);

    void createCriteria(LtLreClient* pClient, LtDomain* pDomain,
                        LtSubnets* pSubnets, LtGroups* pGroups);
    void createCriteria(LtLreClient* pClient, LtDomain* pDomain,
                        LtSubnetNode* pAddress, LtGroups* pGroups,
						LtUniqueId* pUniqueId);
    void createCriteria(LtLreClient* pClient, LtUniqueId* pUniqueId);
	void createCriteria(LtLreClient* pClient, LtDomain* pDomain, int group);

	void addClient(LtLreClient* pClient, boolean bUpdate=false);
	void removeClient(LtLreClient* pClient, boolean bUpdate=false);
    void updateClient(LtLreClient* pClient);

    LtLreClient* getLreUniqueIdClient(LtUniqueId* pUniqueId);
    void updateSubnetGroupForwarding(LtLreClient* pClient, LtLonTalkChannel* pChannel, LtRoutingMap& map, int nSubnet);

    void update();
    void engine();
	boolean clientFunction(LtClientFunction functionType, void* pOwner);
	void deleteReferences(void* pRef);
	void waitForUpdateComplete();
	void syncUpdate(LreUpdateMsgCode code, void* data);

	void nextPacketNumber();

    LtClients* getCatchAll(LreDomainBinType binType, LtDomain* pDomain);
    LtClients* getCatchAll(LreGlobalBinType binType);

	// Event notification (from LRE to its clients, e.g., router sides)
	void notify();

	// Convert static members for task entries to globals to make vxworks task 
	// display more readable because of the shorter (unqualified) names

	//static int VXLCDECL LtLreEngine( int lre, ... );
	//static int VXLCDECL LtLreUpdate( int lre, ... );
	friend int VXLCDECL LreEngineTask( int lre, ... );
	friend int VXLCDECL LreUpdateTask( int lre, ... );

	static boolean getLre(boolean& bFirst, int& nIndex, LtLreServer** ppResult);

protected:
	void errorLog(int err);

public:
    LtLre();
	~LtLre();

	// Use to enumerate LRE clients for this LRE.
	// "bFirst" must be true initially, called function clears it.
	// called function cycles "nIndex".
	// Returns true if found, else false.
	boolean getLreClient(boolean& bFirst, int& index, LtLreClient **pClient);

	// Each client invokes this function to inform the router
	// of its existence.  
	void registerClient(LtLreClient* c, boolean bSync = false);

	// This method deregisters a routing client
	void deregisterClient(LtLreClient* c);	

	// This method deregisters all clients for an owner
	void deregisterOwner(LtLreClientOwner* c);	

	// This method places the packet onto an input queue
	// for the router task and schedules the router task.  
	void routePacket(boolean bPriority,
					 LtLreClient *pClient,
					 LtPktInfo *pPkt);	
	
	// This is used to get/set to set the offline state of the
	// router. 	When offline, packets are only 
	// forwarded amongst clients on the same logical channel.
	// This method is typically used by the LT_ROUTER_SIDE client.
    boolean getOffline()
    {
        return m_bOffline;
    }

	void setOffline(boolean bOffline, LtLonTalkChannel* pChannel=null);

	// This method is used to get/set full LonTalk verbose mode. 
	// In this mode, the router delivers local stack messages onto 
	// the channel even if it is not necessary.  This may be useful
	// for debugging to allow LonTalk communications between local
	// stacks to be viewed from the LonManager Protocol Analyzer.
	// This capability could be turned on via a configuration
	// property, for example.
	boolean getVerboseMode()
	{
        return m_bVerbose;
	}
	void setVerboseMode(boolean verbose)
	{
        m_bVerbose = verbose;
	}

	void eventNotification(LtEventServer* pServer)
    {
        updateClient((LtLreClient*) pServer);
    }

	boolean start();
	boolean stop();

	// Used to get the route(s) to be advertised to the rest of the world. 
	// Only needed for learning routers.
	boolean getExternalRoute(LtDomain& domain, LtLonTalkChannel* pChannel, LtSubnets* pSubnets);

	// Event registration
	void registerEventClient(LtEventClient* pClient);

	// Event deregistration
	void deregisterEventClient(LtEventClient* pClient);

	int getErrorLog();
	
	// Used to update the network statistics on the clients
	void updateClientStats(const LtLreClientOwner* pOwner, const byte* pApdu, bool isUnackd, bool domainMatch);
};

