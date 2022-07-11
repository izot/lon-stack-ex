//
// LtLre.cpp
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

#include "LtRouter.h"
#include "LtMisc.h"
#include "LtPlatform.h"
#include "LtObject.h"
#include "LtVector.h"
#include "LtTaskOwner.h"
#include "LtLreInternals.h"
#include "LtStart.h"
#include "vxlTarget.h"
#include "LtStackInternal.h"

//#define __ECHELON_TRACE

//
// Tasks
//
// Convert static members for task entries to globals to make vxworks task 
// display more readable because of the shorter (unqualified) names

//int	VXLCDECL LtLre::LtLreEngine( int lre, ... )
extern "C" int VXLCDECL LreEngineTask( int lre, ... )
{
	LtLre* pLre = (LtLre*) lre;
	pLre->engine();
	return 0;
}

//int VXLCDECL LtLre::LtLreUpdate( int lre, ... )
extern "C" int VXLCDECL LreUpdateTask( int lre, ... )
{
	LtLre* pLre = (LtLre*) lre;
	pLre->update();
	return 0;
}

// Static 

LtTypedVector<LtLreServer> LtLreServer::m_vecLre;

LtLreServer* LtLreServer::create()
{
	return new LtLre();
}

boolean LtLreServer::getLre(boolean& bFirst, int& nIndex, LtLreServer** ppResult)
{
	LtVectorPos pos;
	if (!bFirst)
	{
		pos = LtVectorPos(nIndex);
	}
	bFirst = false;

	boolean result = m_vecLre.getElement(pos, ppResult);

	nIndex = pos.getIndex();

	return result;
}

//
// Classes
//
LtLreServer::LtLreServer()
{
	// Add myself to global list of LREs.  This assumes LREs are created in
	// an orderly fashion by a single thread.  If this is not a valid assumption,
	// then we'll need to add locking to this.
	m_vecLre.addElement(this);
}

LtLreServer::~LtLreServer()
{
	// Remove myself from global list of LREs.
	m_vecLre.removeElement(this);
}

LtLre::LtLre()
{
	m_semRun = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
	// Do not use a mutex for this.  It is a funky type of lock.
	m_semRoutingData = semBCreate(SEM_Q_FIFO, SEM_FULL);
	m_msgQUpdate = msgQCreate(50, sizeof(LreUpdateMsg), MSG_Q_FIFO);
	m_bUpdatesPending = false;
    m_bOffline = true;
	memset(m_states, 0, sizeof(m_states));
	m_nPacketNumber = -1;	// Start at -1 to easily test wraparound code.
	m_bRouting = false;
	m_routerType = LT_REPEATER;
	m_errorLog = LT_NO_ERROR;
	m_engineTaskId = taskSpawn("LRE_Engine", 
                               LRE_ENGINE_TASK_PRIORITY, 0, 
                               LRE_ENGINE_TASK_STACK_SIZE, LreEngineTask, 
							   (int)this, 0,0,0,0, 0,0,0,0,0);
	m_updateTaskId = taskSpawn("LRE_Update", 
                               LRE_UPDATE_TASK_PRIORITY, 0, 
                               LRE_UPDATE_TASK_STACK_SIZE, LreUpdateTask, 
								(int)this, 0,0,0,0, 0,0,0,0,0);

	registerTask(m_engineTaskId, NULL, m_semRun);
	registerTask(m_updateTaskId, m_msgQUpdate, NULL);
}

LtLre::~LtLre()
{
	syncUpdate(LRE_DELETE_CLIENT, null);

    if (!m_domains.isEmpty())
    {
		// TEMPORARY to GET AROUND AN ASSERT IN PC LIPS
        //assert(0);
    }
    for (int k = 0; k < LRE_GLOBAL_BIN_TYPES; k++)
    {
        if (!m_bin[k].isEmpty())
        {
            assert(0);
        }
    }

    m_domains.clear(true);

	waitForTasksToShutdown();
    semDelete(m_semRun);
	semDelete(m_semRoutingData);
    msgQDelete(m_msgQUpdate);
}

void LtLre::setOffline(boolean bOffline, LtLonTalkChannel* pChannel)
{
	int i;

	for (i=0; i<LT_ROUTER_SIDES; i++)
	{
		if (m_states[i].bInUse && m_states[i].pChannel == pChannel)
		{
			m_states[i].bOffline = bOffline;
			break;
		}
	}

	if (i == LT_ROUTER_SIDES)
	{
		for (i=0; i<LT_ROUTER_SIDES; i++)
		{
			if (!m_states[i].bInUse)
			{
				m_states[i].bInUse = true;
				m_states[i].pChannel = pChannel;
				m_states[i].bOffline = bOffline;
				break;
			}
		}
	}

	assert(i != LT_ROUTER_SIDES);

	m_bOffline = false;
	for (i=0; i<LT_ROUTER_SIDES; i++)
	{
		if (m_states[i].bOffline)
		{
			m_bOffline = true;
		}
	}
}

void LtLre::registerClient(LtLreClient* pClient, boolean bSync)
{
	LreUpdateMsg msg;

	// Register for events from the client
	pClient->registerEventClient((LtEventClient*)this);

	if (bSync)
	{
		syncUpdate(LRE_ADD_CLIENT, pClient);
	}
	else
	{
		msg.code = LRE_ADD_CLIENT;
		msg.data = (void*) pClient;
		msgQSend(m_msgQUpdate, (char*)&msg, sizeof(msg), WAIT_FOREVER, MSG_PRI_NORMAL);
	}

	if (m_bRouting)
	{
		// Tell the client to start
		pClient->start();
	}
}

void LtLre::waitForUpdateComplete()
{
	while (msgQNumMsgs(m_msgQUpdate))
	{
		taskDelay(msToTicks(30));
	}
}

void LtLre::syncUpdate(LreUpdateMsgCode code, void* data)
{
	LreUpdateMsg msg;
	msg.code = code;
	msg.data = data;
	msg.sem = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
	msgQSend(m_msgQUpdate, (char*)&msg, sizeof(msg), WAIT_FOREVER, MSG_PRI_NORMAL);
	semTake(msg.sem, WAIT_FOREVER);
	semDelete(msg.sem);
}

void LtLre::deregisterClient(LtLreClient* pClient)
{
	// Client can be NULL under some shutdown conditions
	if (pClient != NULL)
	{
		if (m_bRouting)
		{
			// Tell the client to stop
			pClient->stop();
		}
		syncUpdate(LRE_REMOVE_CLIENT, pClient);
	}
}

void LtLre::registerEventClient(LtEventClient* pClient)
{
    m_vClients.addElement((LtEventClient*) pClient);
}

void LtLre::deregisterEventClient(LtEventClient* pClient)
{
    m_vClients.removeElement((LtEventClient*) pClient);
}

void LtLre::notify()
{
    LtVectorPos pos;
    LtEventClient* pClient;
    while (m_vClients.getElement(pos, &pClient))
    {
        pClient->eventNotification(this);
    }
}

void LtLre::deregisterOwner(LtLreClientOwner* pOwner)
{
	syncUpdate(LRE_REMOVE_OWNER, pOwner);
}

void LtLre::updateClient(LtLreClient* pClient)
{
	// Client can be NULL under some start-up conditions
	if (pClient != NULL)
	{
		LreUpdateMsg msg;
		msg.code = LRE_UPDATE_CLIENT;
		msg.data = (void*) pClient;
		msgQSend(m_msgQUpdate, (char*)&msg, sizeof(msg), WAIT_FOREVER, MSG_PRI_NORMAL);
	}
}

LtClients* LtLre::getCatchAll(LreDomainBinType binType, LtDomain* pDomain)
{
    LtLreDomain* pLreDom = getDomainEntry(pDomain, true);
	return pLreDom->getBin(binType);
}

LtClients* LtLre::getCatchAll(LreGlobalBinType binType)
{
	return getBin(binType);
}

void LtLre::registerCatchAll(LreDomainBinType binType, LtLreClient* pClient, LtDomain* pDomain)
{
    getCatchAll(binType, pDomain)->addElement(pClient);
}

void LtLre::registerCatchAll(LreGlobalBinType binType, LtLreClient* pClient)
{
    getCatchAll(binType)->addElement(pClient);
}

LtLreDomain* LtLre::getDomainEntry(LtDomain* pDomain, boolean create)
{
	LtDomainKey key(*pDomain);
    LtLreDomain* pLreDom = m_domains.get(&key);
    if (pLreDom == null && create)
    {
        pLreDom = new LtLreDomain();
		pLreDom->setLtDomain(*pDomain);	// set the domain values in the LtLreDomain
        LtDomainKey* pKey = new LtDomainKey(*pDomain);
        m_domains.set(pKey, pLreDom);
    }
    return pLreDom;
}

void LtLre::createCriteria(LtLreClient* pClient, LtDomain* pDomain,
                        LtSubnets* pSubnets, LtGroups* pGroups)
{
    LtLreDomain *pEntry = getDomainEntry(pDomain, true);
    pEntry->set(pClient, pSubnets);
    pEntry->set(pClient, pGroups);
}

void LtLre::createCriteria(LtLreClient* pClient, LtDomain* pDomain, int group)
{
    LtLreDomain *pEntry = getDomainEntry(pDomain, true);
	if (group < LT_NUM_GROUPS)
	{
		pEntry->set(pClient, group);
	}
}

void LtLre::createCriteria(LtLreClient* pClient, LtDomain* pDomain,
                        LtSubnetNode* pAddress, LtGroups* pGroups,
						LtUniqueId* pUniqueId)
{
    LtLreDomain *pEntry = getDomainEntry(pDomain, true);
    LtSubnets sn;
    // Set special broadcast subnets (only if this client's owner
    // doesn't already get these subnets anyway).
    pEntry->setConditionally(sn, pClient, 0);
    pEntry->setConditionally(sn, pClient, pAddress->getSubnet());
    pEntry->setBroadcast(pClient, &sn);
    pEntry->set(pClient, pAddress);
    pEntry->set(pClient, pGroups);
	if (pUniqueId != null)
	{
		pEntry->set(pClient, pUniqueId);
	}
}

void LtLre::createCriteria(LtLreClient* pClient, LtUniqueId* pUniqueId)
{
	m_uniqueIds.update(pClient, pUniqueId);
}

LtLreClient* LtLre::getLreUniqueIdClient(LtUniqueId* pUniqueId)
{
    LtUniqueIdKey key(*pUniqueId);
    return(m_uniqueIds.get(&key));
}

void LtLre::update()
{
	LreUpdateMsg msg;

	while (!taskShutdown())
	{
		if (msgQReceive(m_msgQUpdate, (char*) &msg, sizeof(msg), WAIT_FOREVER) == sizeof(msg))
		{
			LtLreClient* pClient = (LtLreClient*) msg.data;

			gainUpdateAccess();

			// Don't set up for routing in a bogus environment
			if (LtStart::properEnvironment())
			{
				switch (msg.code)
				{
				case LRE_DELETE_CLIENT:
					clientFunction(LT_CLIENT_DELETE, null);
					break;
				case LRE_REMOVE_CLIENT:
					m_clients.removeElement(pClient);
					removeClient(pClient);
					break;
				case LRE_ADD_CLIENT:
					m_clients.addElement(pClient);
					addClient(pClient);
					break;
				case LRE_UPDATE_CLIENT:	
					removeClient(pClient, true);
					addClient(pClient, true);
					break;
				case LRE_REMOVE_OWNER:
					clientFunction(LT_OWNER_REMOVE, msg.data);
					deleteReferences(msg.data);
					break;
				}
			}

			releaseUpdateAccess();

			if (msg.sem != null)
			{
				semGive(msg.sem);
			}
		}
	}
}

void LtLre::gainUpdateAccess()
{
	m_bUpdatesPending = true;
    semGive(m_semRun);
	semTake(m_semRoutingData, WAIT_FOREVER);
}

void LtLre::releaseUpdateAccess()
{
    m_bUpdatesPending = false;
	semGive(m_semRoutingData);
}

void LtLre::deleteReferences(void* pRef)
{
	// A remove is performed using a brute force approach.  Examine each of the tables
	// and search for instances of the client pointer.  If found, remove them.
    m_domains.remove(pRef);
    m_uniqueIds.remove(pRef);
    for (int i = 0; i < LRE_GLOBAL_BIN_TYPES; i++)
    {
        m_bin[i].remove(pRef);
    }
}

void LtLre::removeClient(LtLreClient* pClient, boolean bUpdate)
{
	if (!bUpdate)
	{
		pClient->deregisterEventClient(this);
	}

	deleteReferences(pClient);
}

void LtLre::updateSubnetGroupForwarding(LtLreClient* pClient, LtLonTalkChannel* pChannel, LtRoutingMap& map, int nSubnet)
{
    LtLreDomain* pEntry = getDomainEntry(&map.getDomain(), true);
	// Bits sense is flipped for the forwarding map.
    pEntry->setSubnetsAndGroups(pClient, pChannel, &map.getSubnets(), &map.getGroups(), nSubnet);
}

void LtLre::addClient(LtLreClient* pClient, boolean bUpdate)
{
	int index = 0;
	LtRoutingMap map;
	LtDomain domain;
	LtGroups groups;
	LtSubnetNode node;
	LtUniqueId uid;
	boolean bDomainSpecificUniqueId = false;
	boolean bZeroLength = false;
	boolean bNonZeroLength = false;

	registerCatchAll(LRE_GLOBAL_BIN_ALL_CLIENTS, pClient);

	while (pClient->getRoute(index, &map))
	{
		LtRouterType routerType = map.getRouterType();
		if (routerType == LT_REPEATER)
		{
			registerCatchAll(LRE_GLOBAL_BIN_UNQUALIFIED, pClient);
		}
		else if (map.getDomain().isValid())
		{
			if (routerType == LT_LEARNING_ROUTER ||
				routerType == LT_CONFIGURED_ROUTER)
			{
				// Make sure we get subnet 0 messages (don't
				// assume source knows to set this).
				map.getSubnets().set(0);
			}
			createCriteria(pClient, &map.getDomain(), &map.getSubnets(), &map.getGroups());
			if (routerType == LT_BRIDGE)
			{
				registerCatchAll(LRE_DOMAIN_BIN_UNQUALIFIED, pClient, &map.getDomain());
			}
			if (map.getDomain().getLength() == 0)
			{
				bZeroLength = true;
			}
			else
			{
				bNonZeroLength = true;
			}
		}
	}

	// If the device wanted routing for at least one domain but none of the domains
	// was the zero length domain, then add to DZB bin.
	if (bNonZeroLength && !bZeroLength)
	{
		registerCatchAll(LRE_GLOBAL_BIN_DZB, pClient);
	}

    // Get external routes for purposes of source subnet validation
    index = 0;
	int routingSubnet;
    while (pClient->getExternalRoute(index, &map, &routingSubnet))
    {
		m_routerType = map.getRouterType();
		if (map.getDomain().isValid())
		{
			updateSubnetGroupForwarding(pClient, pClient->getLonTalkChannel(), map, routingSubnet);
		}
    }

	// This code assumes addresses are checked afer routes because
	// address code only creates broadcast subnets if there is
	// no route for them.  
	index = 0;
	while (pClient->getAddress(index, &domain, &node, &groups))
	{
		if (domain.isValid())
		{
			// Don't create address criteria for bridges in this 
			// domain (since they will already get the message).
			LtUniqueId* pKey = null;
			int index0 = 0;
			if (pClient->getIsMulti() && pClient->getAddress(index0, &uid))
			{
				// Local stacks register their unique IDs on a 
				// domain specific basis (in addition to generically
				// which covers flex domain cases).
				pKey = &uid;
				bDomainSpecificUniqueId = true;
			}
			createCriteria(pClient, &domain, &node, &groups, pKey);
		}
	}
	index = 0;
	while (!bDomainSpecificUniqueId && pClient->getAddress(index, &uid))
	{
		if (uid.isSet())
		{
			createCriteria(pClient, &uid);
		}
	}

	index = 0;
	int group;
	// See comment above on repeaters.
	while (pClient->getAddress(index, &domain, group))
	{
		if (domain.isValid())
		{
			createCriteria(pClient, &domain, group);
		}
	}

	if (pClient->getNeedAllBroadcasts())
	{
		// Register clients which want all broadcasts because they are
		// unconfigured.
		registerCatchAll(LRE_GLOBAL_BIN_QUALIFIED, pClient);
	}

    if (pClient->getNeedAllLayer2Packets())
    {
        registerCatchAll(LRE_GLOBAL_BIN_ALL_PACKETS, pClient);
    }
}

void LtLre::routePacket(boolean bPriority, LtLreClient* pClient, LtPktInfo* pPkt)
{
	if (m_bRouting)
	{
		pPkt->setSourceClient(pClient);
		m_pkts.send(pPkt);
		semGive(m_semRun);
	}
}

void LtLre::checkRoutingRulesForClient(LtLreClient* pDestClient, LreRouteControl& routeCtrl)
{
	if (pDestClient != null)
	{
		LtLreClient* pSourceClient = routeCtrl.getPkt()->getSourceClient();
		LtLreDomain* pEntry = routeCtrl.getDomain();
		LtLonTalkChannel* pDestChannel = pDestClient->getLonTalkChannel();
		boolean sameLtChannel = pDestChannel == routeCtrl.getSourceLtChannel();
		boolean routePacket = true;

		// Enforce the following rules:
		// 1. Packet must not be routed to another client with the same ID.
		//    This prohibits packets looping back to the same node stack
		//    or to other routers on the same logical channel.
		//	  clients on the same channel with like type.
		// 2. If offline, don't route between clients on different physical
		//    channels.  (allows proxy routing to occur if router is 
		//    offline).
		// 3. If either client specifies no interchannel routing, don't route
		//    unless target is on same physical channel.
		// 4. If the clients are on different LonTalk channels then
		//    don't route if source subnet lontalk channel matches that
		//    of destination client's lontalk channel or no cross
		//    channel routing is allowed.
		// 5. Don't route if source address resides in dest client owner.
		// 6. Channel matches or doesn't as required.
		// 7. The client hasn't already been slated to receive a packet
		//    with this packet number.  
		// 8. If the packet already has a specific destination, then don't
		//    send it to a client that doesn't want such packets (unless
		//    the "route multiple" override is set).
		if (pDestClient->getId() == pSourceClient->getId())
		{
#ifdef __ECHELON_TRACE
			printf("ID constraint");
#endif
			routePacket = false;
		}
		else if (!sameLtChannel &&
				 (getOffline() ||
 				  !pDestClient->routeInterchannel() ||
				  !pSourceClient->routeInterchannel() ||
				  !routeCtrl.getCrossChannel() ||
			      routeCtrl.getOriginalLtChannel() == pDestChannel))
		{
#ifdef __ECHELON_TRACE
			printf("Interchannel constraint (offline:%d)", getOffline());
#endif
			routePacket = false;
		}
		else if (pDestClient->sourceAddressFilter() && pEntry != null &&
				 pEntry->addressClient(routeCtrl.getPkt()->getSourceNode(), pDestClient))
		{
#ifdef __ECHELON_TRACE
			printf("Source address filter constraint");
#endif
			routePacket = false;
		}
		else if ((routeCtrl.getDiffChannelOnly() && sameLtChannel) ||
				 (routeCtrl.getSameChannelOnly() && !sameLtChannel))
		{
#ifdef __ECHELON_TRACE
			printf("Same channel constraint");
#endif
			routePacket = false;
		}
		else if (m_nPacketNumber == pDestClient->getOwner()->getPacketNumber())
		{
#ifdef __ECHELON_TRACE
			printf("Packet number match");
#endif
			routePacket = false;
		}
		// Keep this condition the last in the list, due to special iLON logic in it
		else if (routeCtrl.getSpecificTarget() && !pDestClient->routeMultiple())
		{
#ifdef __ECHELON_TRACE
			printf("Multiple client rejection");
#endif
#if PRODUCT_IS(ILON)
			// Allow normally rejected packets to be routed.
			// The logic is a little convoluted, but it keeps it an iLON-only feature.
			if (xmitInternalPacketsOnPort)
			{
				// do nothing; just let it route normally
			}
			else if (sendInternalPacketsToPort)
			{
				routeCtrl.getPkt()->setRouteMultipleNoXmit(true);
			}
			else
#endif
			{
				routePacket = false;
			}
		}

		if (!routePacket)
		{
#ifdef __ECHELON_TRACE
			printf(": can't route to %x\n", pDestClient);
#endif
		}
		else
		{
			if (pDestClient->needValidCrc())
			{
				m_bFixupCrc = true;
			}
			m_routeList.addElement(pDestClient);
			// To ensure the owner of this client gets the packet only 
			// once.  Examples of where this is needed.  
			// 1. Message addressed to bridge or repeater's subnet/node address.
			// 2. Device that has multiple instances of the same subnet/node address.
			// 3. Broadcast to an owner that has "receiveAllBroadcasts" set.
			pDestClient->getOwner()->setPacketNumber(m_nPacketNumber); 
		}
	}
}

void LtLre::checkRoutingRulesForClientList(LtClients* pClients, LreRouteControl& routeCtrl)
{
    if (pClients != null)
    {
        LtVectorPos pos;
        LtLreClient* pClient;
        while (pClients->getElement(pos, &pClient))
        {
            checkRoutingRulesForClient(pClient, routeCtrl);
        }
    }
}

//
// updateClientStats
//
// We call this for each packet in order to notify each client of the packet in order to increment
// statistics.
//
void LtLre::updateClientStats(const LtLreClientOwner* pOwner, const byte* pApdu, bool isUnackd, bool domainMatch)
{
	LtClients* pClients = getBin(LRE_GLOBAL_BIN_ALL_CLIENTS);
    if (pClients != null)
    {
        LtVectorPos pos;
        LtLreClient* pClient;
        while (pClients->getElement(pos, &pClient))
        {
        	bool gotThisMsg = pClient->getOwner()->getPacketNumber() == m_nPacketNumber;
        	// The sender of the packet should not bump its L2 receive count so exclude him.
        	if (pOwner != pClient->getOwner())
        	{
        		pClient->updateStats(pApdu, isUnackd, domainMatch, gotThisMsg);
        	}
        }
    }
}

void LtLre::routePkt(LtPktInfo* pPkt, LtLreClient* pDestClient, boolean bClone)
{
	LtPktInfo* pToRoute = pPkt;

	// If due to subnet fixup or because the packet came from a stack we need
	// to generate a valid CRC, then do so here.
	if (m_bFixupCrc)
	{
		pPkt->setCrc();
	}

	// Packet may need to be cloned (i.e., if there are multiple destinations).
	if (bClone)
	{
		pToRoute = (LtPktInfo*) pPkt->cloneMessage(/* shallow */ /* stdalloc */);
		if (pToRoute != null)
		{
			pToRoute->copyMessage(pPkt, false);
		}
	}

	if (pToRoute != null)
	{
		pToRoute->setDestClient(pDestClient);
		pDestClient->processPacket(pPkt->getPriority(), pToRoute);
	}
}

void LtLre::routePktToClientList(LtPktInfo* pPkt)
{
	LtVectorPos pos;
	LtLreClient* pDestClient;

	// If there is more than one client, then the message must be cloned.
	// Otherwise, we can just send the message as is.
	int ctClients = m_routeList.size();

	if (ctClients)
	{
		// We need to route packet so go ahead and incur cost of parsing layer 4.
		// Need by stacks and might be needed by IP clients doing routing by service
		// type, for example.
		pPkt->parsePktLayer4();

		while (m_routeList.getElement(pos, &pDestClient))
		{
			--ctClients;
			routePkt(pPkt, pDestClient, ctClients > 0);
		}
	}
	else
	{
		// No one wants this packet.
		pPkt->release();
	}
}

void LtLre::nextPacketNumber()
{
	if (++m_nPacketNumber == 0)
	{
		m_nPacketNumber = 1;
		// Clear out everyone's packet number to ensure there is no false duplicate detection.
		LtVectorPos pos;
		LtLreClient* pClient;
		while (m_clients.getElement(pos, &pClient))
		{
			pClient->getOwner()->setPacketNumber(0);
		}
	}
}

void LtLre::determineClientsAndRoute(LtPktInfo* pPkt, LtLreDomain* pEntry, boolean bSameChannelOnly, boolean bDiffChannelOnly, boolean bDomainZeroBcast)
{
	m_routeList.removeAllElements();
	m_bFixupCrc = false;
	boolean bUniqueIdRouted = false;

	LreRouteControl routeCtrl(pPkt, m_routerType, bSameChannelOnly, bDiffChannelOnly);

	if (pPkt->getIsValidL2L3Packet())
	{
		if (pEntry != null)
		{
			routeCtrl.setDomain(pEntry);
			if (pEntry->updateRoutingInfo(this, pPkt, routeCtrl))
			{
				// A change occurred (e.g., learning router subnet mask change)
				// Notify clients
				notify();
			}
		}

		LtAddressFormat format = pPkt->getAddressFormat();
		if (format == LT_AF_UNIQUE_ID)
		{
			LtUniqueIdKey key(pPkt->getUniqueId());
			// Try to find the unique ID first generically.  If not found,
			// proceed under subnet rules.  If found for a local client, 
			// see if there is a domain specific client and use that 
			// preferentially.
			LtLreClient* pClient = m_uniqueIds.get(&key);
			if (pClient != null && 
				pClient->getIsMulti() &&
				pEntry != null)
			{
				LtLreClient* pDomainSpecificClient = pEntry->get(&key);
				if (pDomainSpecificClient != null)
				{
					pClient = pDomainSpecificClient;
				}
			}
			if (pClient != null)
			{
				// Mark as routed (even if rejected for other reasons).
				bUniqueIdRouted = true;
				if (!pPkt->getRouteMultiple())
				{
					routeCtrl.setSpecificTarget();
				}
				checkRoutingRulesForClient(pClient, routeCtrl);
			}
		}

		if (pEntry != null)
		{
			LtClients* pClients = null;

			switch(format)
			{
			case LT_AF_SUBNET_NODE:
			case LT_AF_GROUP_ACK:
				pClients = pEntry->get(pPkt->getDestSubnet(), pPkt->getDestNode());
				if (pClients == NULL)
				{
					// No qualifying clients, route based on subnet only
					pClients = pEntry->get(pPkt->getDestSubnet());
				}
				else
				{
					// If we don't have a multiple target override, and the target is configured (EPR 49432), indicate that this has a specific target
					if (!pPkt->getRouteMultiple() && pEntry->configuredClients(pClients))
					{
						routeCtrl.setSpecificTarget();
					}
				}
				break;
			case LT_AF_UNIQUE_ID:
				if (!bUniqueIdRouted)
				{
					// If we already found the specific target for this unique ID,
					// don't bother forwarding it to other routers in this subnet
					// (or especially broadcasting it if the routing subnet is
					// 0 which it can often be in NSS).
					pClients = pEntry->get(pPkt->getDestSubnet());
				}
				break;
			case LT_AF_BROADCAST:
				checkRoutingRulesForClientList(pEntry->getBroadcast(pPkt->getDestSubnet()), routeCtrl);
				pClients = pEntry->get(pPkt->getDestSubnet());
				break;
			case LT_AF_GROUP:
				pClients = pEntry->getGroup(pPkt->getDestGroup());
				break;
			default:
				break;	// nothing
			}

			checkRoutingRulesForClientList(pClients, routeCtrl);

			// Also send to unqualified clients (e.g., bridges)
			checkRoutingRulesForClientList(pEntry->getBin(LRE_DOMAIN_BIN_UNQUALIFIED), routeCtrl);
		}
	    checkRoutingRulesForClientList(getBin(LRE_GLOBAL_BIN_UNQUALIFIED), routeCtrl);

        if (pPkt->getAddressFormat() == LT_AF_BROADCAST)
		{
			checkRoutingRulesForClientList(getBin(LRE_GLOBAL_BIN_QUALIFIED), routeCtrl);
		}
		if (bDomainZeroBcast)
		{
			checkRoutingRulesForClientList(getBin(LRE_GLOBAL_BIN_DZB), routeCtrl);
		}
	}
	checkRoutingRulesForClientList(getBin(LRE_GLOBAL_BIN_ALL_PACKETS), routeCtrl);

	routePktToClientList(pPkt);
}

void LtLre::beginRoutingPkt(LtPktInfo* pPkt)
{
	pPkt->parsePktLayers2and3();

	nextPacketNumber();

#ifdef __ECHELON_TRACE
	printf("Route: ");
	byte* pData;
	int nLen = pPkt->getMessageData(&pData);
	for (int i = 0; i < nLen; i++)
	{
		printf("%02x", pData[i]);
	}
	printf("\n");
#endif

	boolean bSameChannelOnly = false;
	boolean bDomainZeroBcast = false;
	LtLreDomain *pLtLreDomain = NULL;

	if (pPkt->getIsValidL2L3Packet())
	{
		// If source subnet is 0, stick in the default subnet.  
		if (pPkt->getSourceNode().getSubnet() == 0)
		{
			LtLonTalkChannel* pChannel = pPkt->getSourceClient()->getLonTalkChannel();
			if (pPkt->getDomain().getLength() == 0 &&
				pPkt->getAddressFormat() == LT_AF_BROADCAST)
			{
				bDomainZeroBcast = true;
				// For domain length 0 broadcasts, we generate packets on the domains
				// in which the router is configured, one fork per domain.
				LtHashTablePos pos;
				LtLreDomain* pEntry;
				LtDomainKey* pKey;

				if (m_routerType != LT_REPEATER)
				{
					// First, force the destination subnet to be zero.  The reason for this is
					// twofold.  
					// 1. It's what the LonTalk router does.  It does it so that if the subnet
					// happens to be non-zero, then the packet will be forwarded on the domain
					// into which the packet is placed, regardless of the specified subnet.
					// This rule is for copied packets (as done immediately below).
					// 2. It ensures the packet is forwarded to the other LTIP routers.  This
					// is unique to the LTIP router (this rule is for the non-copied packet only).
					// Note that we only change the packet data, not the packet itself. 
					// For the copied packet, this is OK since it rebuilds the header based on
					// the packet data.  For the non-copied packet, this is OK since we don't 
					// need to actually change the packet as the packet data is used for forwarding.
					// It is the far router which will actually change the destination for real.
					// Note this is probably much ado about nothing (the comment not the code)
					// since there are no known packets that are DZB with dest subnet non-zero.
					// One side effect of this is that such a broadcast gets received by nodes
					// on this channel even if they aren't the specified subnet.  This is not
					// easy to fix.  Note that all nodes on all other channels will receive the
					// message so it is not entirely unreasonable, just not exactly like a
					// true pure LonTalk channel would do.
					pPkt->setDestSubnet(0);

					while (m_domains.getElement(pos, &pKey, &pEntry))
					{
						if (pEntry->forkable())
						{
							//LtPktInfo* pCopy = (LtPktInfo*) pPkt->cloneMessage(/* deep */ /* stdalloc */);
							LtPktInfo* pCopy = (LtPktInfo*) pPkt->getAllocator()->allocMessage();
							if (pCopy != null)
							{
								byte* pData;
								int nLen = pPkt->getMessageData(&pData);
								// Put data at end of block.  Allows for message expansion.
								byte* pDest = pCopy->getBlock() + pCopy->getBlockSize() - (nLen + LONC_CRC_LENGTH);
								memcpy(pDest, pData, nLen);
								pCopy->setMessageData(pDest, nLen, pCopy);
								pCopy->copyMessage(pPkt, true);
								pCopy->domainFixup(pKey->getDomain(), pEntry->getDefaultSubnet(pChannel));
								// Note we specify false for dzb flag because the conversion makes the copy no
								// longer a dzb (maybe not zero length domain, definitely not source subnet 0).
								determineClientsAndRoute(pCopy, pEntry, false, true, false);
							}
							bSameChannelOnly = true;
						}
					}
				}
			}
			else if (m_routerType != LT_REPEATER)
			{
				LtLreDomain* pEntry = getDomainEntry(&pPkt->getDomain());
				if (pEntry != null)
				{
					//LtPktInfo* pCopy = (LtPktInfo*) pPkt->cloneMessage(/* deep */ /* stdalloc*/);
					LtPktInfo* pCopy = (LtPktInfo*) pPkt->getAllocator()->allocMessage();
					if (pCopy != null)
					{
						byte* pData;
						int nLen = pPkt->getMessageData(&pData);
						memcpy(pCopy->getBlock(), pData, nLen);
						pCopy->setMessageData(pCopy->getBlock(), nLen, pCopy);
						pCopy->copyMessage(pPkt, true);
						pCopy->subnetFixup(pEntry->getDefaultSubnet(pChannel));
						determineClientsAndRoute(pCopy, pEntry, false, true, bDomainZeroBcast);
					}
					bSameChannelOnly = true;
				}
			}
		}
		pLtLreDomain = getDomainEntry(&pPkt->getDomain());
	}
		
	// Before routing the packet (and freeing it!), save enough of the APDU (for unackd) that we can do some filtering
	// in the client based on message content.  We could clone the message but I'd prefer not to incur
	// that overhead on every packet.
	byte apdu[10];
	memcpy(apdu, pPkt->getData(), sizeof(apdu));
	bool isUnackd = pPkt->isUnackd();
    bool isValid = pPkt->getIsValidL2L3Packet() ? true : false;
	LtLreClientOwner* pOwner = pPkt->getSourceClient()->getOwner();
	
	determineClientsAndRoute(pPkt, pLtLreDomain, bSameChannelOnly, false, bDomainZeroBcast);
	
	if (isValid)
	{
		updateClientStats(pOwner, apdu, isUnackd, pLtLreDomain!=NULL);
	}
}

void LtLre::engine()
{
    LtMsgRef *pMsg;

    semTake(m_semRoutingData, WAIT_FOREVER);

	while (!taskShutdown())
	{
		// Note that this logic assumes the update thread runs at same or higher
		// priority than the engine.
		if (m_bUpdatesPending)
		{
			semGive(m_semRoutingData);
			taskDelay(1);
			semTake(m_semRoutingData, WAIT_FOREVER);
		}
		else
		{
			semTake(m_semRun, WAIT_FOREVER);
		}

		while (m_pkts.receive(&pMsg, 0) == OK)
		{
			if (m_bRouting)
			{
				// First validate that the source client is actually there (has been created and
				// registered, and has not been deleted out from under us)
				LtLreClient *pSourceClient = ((LtPktInfo*)pMsg)->getSourceClient();
				if (!m_clients.isElement(pSourceClient))
				{
					// Source client is not there, can't route this packet.
					pMsg->release();
				}
				else
				{
					beginRoutingPkt((LtPktInfo*) pMsg);
				}
			}
			else
			{
				pMsg->release();
			}
			// In heavy traffic, need to ensure we service update requests
			if (m_bUpdatesPending) break;
		}
	}

	semGive(m_semRoutingData);
}

boolean LtLre::clientFunction(LtClientFunction functionType, void* pOwner)
{
	LtVectorPos pos;
	LtLreClient* pClient;
	boolean result = true;

	while (m_clients.getElement(pos, &pClient))
	{
		if (pOwner == null || (void*)pClient->getOwner() == pOwner)
		{
			switch (functionType)
			{
			case LT_CLIENT_START:
				result = pClient->start() && result;
				break;
			case LT_CLIENT_STOP:
				result = pClient->stop() && result;
				break;
			case LT_OWNER_REMOVE:
				pClient->stop();
				pClient->deregisterEventClient(this);
				m_clients.removeElementAt(pos);
				break;
			case LT_CLIENT_DELETE:
				pClient->stop();
				removeClient(pClient);
				m_clients.removeElementAt(pos);
				delete pClient;
				break;
			}
		}
	}
	return result;
}

boolean LtLre::start()
{
	boolean result;

	// Wait for any pending registration to occur
	waitForUpdateComplete();

	// Next, start the clients
	result = clientFunction(LT_CLIENT_START, null);
	if (result)
	{
		// Start the engine!
		m_bRouting = true;
	}
	return result;
}

boolean LtLre::stop()
{
	boolean result;

	// Stop the routing
	m_bRouting = false;

	// Wait for any pending deregistration to occur
	while (msgQNumMsgs(m_msgQUpdate) != 0)
	{
		taskDelay(msToTicks(100));
	}

	// Stop the clients
	result = clientFunction(LT_CLIENT_STOP, null);

	// Wait for all packets to be gone
	while (!m_pkts.isEmpty())
	{
		taskDelay(msToTicks(100));
	}

	return result;
}

boolean LtLre::getLreClient(boolean& bFirst, int& nIndex, LtLreClient** ppClient)
{
	LtVectorPos pos;
	if (!bFirst)
	{
		pos = LtVectorPos(nIndex);
	}
	bFirst = false;

	boolean result = m_clients.getElement(pos, ppClient);

	nIndex = pos.getIndex();

	return result;
}

// Used to get the route(s) to be advertised to the rest of the world. 
// Only needed for learning routers.
boolean LtLre::getExternalRoute(LtDomain& domain, LtLonTalkChannel* pChannel, LtSubnets* pSubnets)
{
	boolean result = false;

	LtLreDomain* pEntry = getDomainEntry(&domain);
	if (pEntry != null)
	{
		LtSubnetGroupForwarding* pSubnetGroup = pEntry->getSubnetGroups(pChannel);
		if (pSubnetGroup != null)
		{
			*pSubnets = pSubnetGroup->getSubnets();
			result = true;
		}
	}
	return result;
}

void LtLre::errorLog(int err)
{
	m_errorLog = err;
}

int LtLre::getErrorLog()
{
	return m_errorLog;
}

LtLreDomain::~LtLreDomain()
{
    m_nodes.clear(true);
    m_subnets.clear(true);
    m_groups.clear(true);
    m_broadcastSubnets.clear(true);
	m_subnetGroups.clear(true);
}

void LtLreDomain::setConditionally(LtSubnets& subnets, LtLreClient* pCheckClient, int nSubnet)
{
    LtClients* pClients;

	// Set subnet bit only if client isn't already routed via subnet routing and/or 
	// subnet broadcast routing.
    if (((pClients = get(nSubnet)) == null || !pClients->ownerMatch(pCheckClient)) &&
		((pClients = getBroadcast(nSubnet)) == null || !pClients->ownerMatch(pCheckClient)))
    {
        subnets.set(nSubnet);
    }
}

LtLreClient* LtLreDomain::get(LtUniqueIdKey* pKey)
{
	return m_uniqueIds.get(pKey);
}

LtClients* LtLreDomain::get(int nSubnet)
{
    return m_subnets.elementAt(nSubnet);
}

LtClients* LtLreDomain::getGroup(int nGroup)
{
    return m_groups.elementAt(nGroup);
}

LtClients* LtLreDomain::getBroadcast(int nSubnet)
{
    return m_broadcastSubnets.elementAt(nSubnet);
}

LtClients* LtLreDomain::get(int subnet, int node)
{
    LtSubnetNodeKey sn(subnet, node);
    return m_nodes.get(&sn);
}

// See if any of the clients are configured
boolean LtLreDomain::configuredClients(LtClients *pClients)
{
    boolean configuredClients = FALSE;

    if (pClients != NULL)
    {
        LtVectorPos pos;
        LtLreClient* pClient;

        while (!configuredClients && pClients->getElement(pos, &pClient))
        {
			if (pClient->ownerIsLtDeviceStack())
			{
				// Safe cast because of above check
				LtDeviceStack *pDevStack = (LtDeviceStack*)pClient->getOwner();
				if (pDevStack != NULL)
				{  
                    configuredClients = !pDevStack->unconfigured();
				}
			}    
        }
    }  

    return configuredClients;
}

boolean LtLreDomain::addressClient(LtSubnetNode& subnetNode, LtLreClient* pClient)
{
    boolean bMatch = false;
	LtLreClient* pMatchingClient;
	LtDeviceStack* pDevStack;
	LtDomainConfiguration tgtDomainConfig;
	LtDomainConfiguration srcDomainConfig(0, *getLtDomain(), NULL, 
											subnetNode.getSubnet(), subnetNode.getNode());

	// Determine if the client owner is routed to for the specified subnet
    // node address.  
    LtClients* pClients = get(subnetNode.getSubnet(), subnetNode.getNode());
	if (pClients != NULL)
	{
		if (pClients->ownerMatch(pClient, &pMatchingClient))
		{
			bMatch = true;
			if (pMatchingClient->ownerIsLtDeviceStack())
			{
				// Safe cast because of above check
				pDevStack = (LtDeviceStack*)pMatchingClient->getOwner();
				if (pDevStack != NULL)
				{
					bMatch = pDevStack->hasMatchingSrcDomainConfiguration(srcDomainConfig);
				}
			}
		}
	}
    return bMatch;
}

boolean LtLreDomain::updateRoutingInfo(LtLre* pLre, LtPktInfo* pPkt, LreRouteControl& routeCtrl)
{
	boolean bChangeOccurred = false;

	if (routeCtrl.getRouterType() == LT_BRIDGE)
	{
		// We can route cross channel for this domain if 
		// forwarding has been set.
		routeCtrl.setCrossChannel(m_subnetGroups.getCount()!=0);
	}
	else if (routeCtrl.getRouterType() != LT_REPEATER)
	{
		LtVectorPos pos;
		LtSubnetGroupForwarding* pSubnetGroup;
		LtLonTalkChannel* pChannel = null;
		LtLonTalkChannel* pSourceChannel = routeCtrl.getSourceLtChannel();
		int nSourceSubnet = pPkt->getSourceNode().getSubnet();
		boolean bGroup = pPkt->getAddressFormat() == LT_AF_GROUP;
		int ownedCount = 0;
		int nDestIndex = bGroup ? pPkt->getDestGroup() : pPkt->getDestSubnet();
		while (m_subnetGroups.getElement(pos, &pSubnetGroup))
		{
			// If forwarding bit is not set, then this is the channel that owns the subnet.
			if (pChannel == null && !pSubnetGroup->getSubnets().get(nSourceSubnet))
			{
				pChannel = pSubnetGroup->getChannel();
			}
			// Only allow cross channel routing if destination is to be forwarded by router.  Exception - 
			// learning routers always forward groups.
			if (pSubnetGroup->getChannel() == pSourceChannel &&
				(bGroup ? (routeCtrl.getRouterType()==LT_LEARNING_ROUTER || pSubnetGroup->getGroups().get(nDestIndex)) :
						  (nDestIndex==0 || pSubnetGroup->getSubnets().get(nDestIndex))))
			{
				// In an N-way router, we would need to build a list of eligible channels
				// and then cross check against the list as clients were selected.  Not doing
				// this now for performance reasons.
				routeCtrl.setCrossChannel(true);
			}
			if (routeCtrl.getRouterType() == LT_LEARNING_ROUTER)
			{
				// For a learning router, need to clear the forwarding bit in the source 
				// channel if set.  Notify clients so that they can update rest of world.
				if (pSubnetGroup->getChannel() == pSourceChannel)
				{
					if (nSourceSubnet && pSubnetGroup->getSubnets().get(nSourceSubnet))
					{
						pSubnetGroup->getSubnets().set(nSourceSubnet, false);
						bChangeOccurred = true;
					}
				}
				if (!bGroup && pSubnetGroup->getSubnets().get(nDestIndex) == 0)
				{
					if (++ownedCount > 1)
					{
						// A subnet partition has occurred - packet not routed cross channel
						routeCtrl.setCrossChannel(false);
						if (bChangeOccurred)
						{
							pLre->errorLog(LT_SUBNET_PARTITION);
						}
					}
				}
			}
		}
		routeCtrl.setOriginalLtChannel(pChannel);
	}
	return bChangeOccurred;
}

void LtLreDomain::setSubnetsAndGroups(LtLreClient* pClient, LtLonTalkChannel* pChannel, LtSubnets* pSubnets, LtGroups* pGroups, int nDefaultSubnet)
{
	m_subnetGroups.update(pClient, pChannel, pSubnets, pGroups, nDefaultSubnet);
}

LtSubnetGroupForwarding* LtLreDomain::getSubnetGroups(LtLonTalkChannel* pChannel)
{
	return m_subnetGroups.get(pChannel);
}

LtSubnetGroupForwarding* LtSubnetGroupForwardings::get(LtLonTalkChannel* pChannel)
{
	LtVectorPos pos;
	LtSubnetGroupForwarding* pSubnetGroup;

	while (getElement(pos, &pSubnetGroup))
	{
		if (pSubnetGroup->getChannel() == pChannel)
		{
			return pSubnetGroup;
		}
	}
	return null;
}

int LtSubnetGroupForwardings::getDefaultSubnet(LtLonTalkChannel* pChannel)
{
	int subnet = 0;
	LtSubnetGroupForwarding* pSubnetGroup = get(pChannel);

	if (pSubnetGroup != null)
	{
		subnet = pSubnetGroup->getDefaultSubnet();
	}
	return subnet;
}

void LtSubnetGroupForwardings::remove(void* pRef)
{
	LtVectorPos pos;
	LtSubnetGroupForwarding* pSubnetGroup;

	while (getElement(pos, &pSubnetGroup))
	{
		if (pSubnetGroup->getRef() == pRef)
		{
			removeElementAt(pos);
			delete pSubnetGroup;
			break;
		}
	}
}

void LtSubnetGroupForwardings::update(void* pRef, LtLonTalkChannel* pChannel, LtSubnets* pSubnets, LtGroups* pGroups, int nDefaultSubnet)
{
	LtSubnetGroupForwarding* pSubnetGroup = get(pChannel);
	if (pSubnetGroup != null)
	{
		if (pSubnets != null)
		{
			pSubnetGroup->setSubnets(*pSubnets);
		}
		if (pGroups != null)
		{
			pSubnetGroup->setGroups(*pGroups);
		}
		pSubnetGroup->setDefaultSubnet(nDefaultSubnet);
	}
	else
	{
        LtSubnetGroupForwarding* pSubnetGroup = new LtSubnetGroupForwarding(pRef, pChannel, pSubnets, pGroups, nDefaultSubnet);
        addElement(pSubnetGroup);
	}
}

void LtClients::remove(void* pRef)
{
    LtVectorPos pos;
    LtLreClient* pClient;

    while (getElement(pos, &pClient))
    {
		if (pClient == pRef || (void*)pClient->getOwner() == pRef)
		{
			removeElementAt(pos);
		}
    }
}

boolean LtClients::idMatch(LtLreClient* pClient)
{
    LtVectorPos pos;
    LtLreClient* pTarget;
    boolean match = false;

    while (!match && getElement(pos, &pTarget))
    {
		match = pClient->getId() == pTarget->getId();
    }
    return match;
}

boolean LtClients::ownerMatch(LtLreClient* pClient, LtLreClient** ppMatchingClient)
{
    LtVectorPos pos;
    LtLreClient* pTarget;
    boolean match = false;

    while (!match && getElement(pos, &pTarget))
    {
		match = pClient->getOwner() == pTarget->getOwner();
		if (match && (ppMatchingClient != NULL))
		{
			*ppMatchingClient = pTarget;
		}
    }
    return match;
}

void LtClientsVector::set(LtLreClient* pClient, int nIndex)
{
    LtClients* pClients = elementAt(nIndex);
    if (pClients == null)
    {
        pClients = new LtClients();
        addElementAt(nIndex, pClients);
    }
    // Check for duplicate before adding.  A client might
    // be added in a group entry for both the routing table
    // and the address group.  We don't want to duplicate
    // packets in this case.
    if (!pClients->isElement(pClient))
    {
        pClients->addElement(pClient);
    }
}

void LtClientsVector::set(LtLreClient* pClient, LtRouteMap* pMap)
{
    for (int i = 0; i <= pMap->getMax(); i++)
    {
        if (pMap->get(i))
        {
			set(pClient, i);
        }
    }
}

void LtClientsVector::remove(void* pRef)
{
    LtVectorPos pos;
    LtClients* pClients;

    while (getElement(pos, &pClients))
    {
        if (pClients != null)
        {
            pClients->remove(pRef);
            if (pClients->isEmpty())
            {
                removeElementAt(pos);
                delete pClients;
            }
        }
    }

}

void LtLreDomain::set(LtLreClient* pClient, LtUniqueId* pUniqueId)
{
	m_uniqueIds.update(pClient, pUniqueId);
}

void LtLreDomain::set(LtLreClient* pClient, LtSubnets* pSubnets)
{
    m_subnets.set(pClient, pSubnets);
}

void LtLreDomain::set(LtLreClient* pClient, LtGroups* pGroups)
{
    m_groups.set(pClient, pGroups);
}

void LtLreDomain::setBroadcast(LtLreClient* pClient, LtSubnets* pSubnets)
{
    m_broadcastSubnets.set(pClient, pSubnets);
}

void LtLreDomain::set(LtLreClient* pClient, int group)
{
	m_groups.set(pClient, group);
}

void LtLreDomain::set(LtLreClient* pClient, LtSubnetNode* pAddress)
{
	LtSubnetNodeKey* pSn = new LtSubnetNodeKey(*pAddress);
    LtClients* pClients = m_nodes.get(pSn);
    if (pClients == null)
    {
        pClients = new LtClients();
        m_nodes.set(pSn, pClients);
    }
	else
	{
		delete pSn;
	}
    pClients->addElement(pClient);
}

void LtLreDomain::remove(void* pRef)
{
    m_nodes.remove(pRef);
    m_subnets.remove(pRef);
    m_groups.remove(pRef);
    m_broadcastSubnets.remove(pRef);
	m_uniqueIds.remove(pRef);
	m_subnetGroups.remove(pRef);
    for (int i = 0; i < LRE_DOMAIN_BIN_TYPES; i++)
    {
        m_bin[i].remove(pRef);
    }
}

boolean LtLreDomain::isEmpty()
{
    boolean bEmpty = m_nodes.isEmpty() &&
           m_subnets.isEmpty() &&
           m_groups.isEmpty() &&
           m_broadcastSubnets.isEmpty() &&
		   m_uniqueIds.isEmpty() &&
		   m_subnetGroups.getCount() == 0;
    for (int i = 0; bEmpty && i < LRE_DOMAIN_BIN_TYPES; i++)
    {
        if (!m_bin[i].isEmpty())
        {
            bEmpty = false;
            break;
        }
    }
    return bEmpty;
}

void LtNodeClients::remove(void* pClient)
{
    LtHashTablePos pos;
    LtSubnetNodeKey *pKey;
    LtClients *pClients;

    while (getElement(pos, &pKey, &pClients))
    {
        pClients->remove(pClient);
        if (pClients->isEmpty())
        {
            removeAt(pos);
            delete pClients;
        }
    }
}

void LtDomains::remove(void* pRef)
{
    LtHashTablePos pos;
    LtLreDomain* pDomain;
    while (getElement(pos, null, &pDomain))
    {
        pDomain->remove(pRef);
        if (pDomain->isEmpty())
        {
            removeAt(pos);
            delete pDomain;
        }
    }
}

void LtUniqueIdClients::remove(void* pRef)
{
    LtHashTablePos pos;
    LtLreClient* pValue;

    while (getElement(pos, null, &pValue))
    {
        if (pValue == pRef || (void*)pValue->getOwner() == pRef)
        {
            removeAt(pos);
        }
    }
}

void LtUniqueIdClients::update(LtLreClient* pClient, LtUniqueId* pKey)
{
	LtUniqueIdKey *pId = new LtUniqueIdKey(*pKey);
	if (get(pId) != null)
	{
		// Overwrite existing entry with new client
		removeKey(pId);
	}
	set(pId, pClient);
}

#if PRODUCT_IS(ILON) || PRODUCT_IS(IZOT)
// Global functions to control the routing of "internal" packets to port clients
// Use 'int' instead of 'boolean' to allow accessing from shell
int sendInternalPacketsToPort = false;
int xmitInternalPacketsOnPort = false;

#endif

