/***************************************************************
 *  Filename: LtLreIpClient.cpp
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
 *  Description:  LreClient for IP links
 *
 *	DJ Duffy Feb 1999
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/ShareIp/LtLreIpClient.cpp#2 $
//
/*
 * $Log: /Dev/ShareIp/LtLreIpClient.cpp $
 *
 * 69    8/01/07 5:41p Fremont
 * EPR 43171 - create mechanism so that LonScanner can see internal
 * packets on the iLON. Must set the flag 'sendInternalPacketsToPort'
 *
 * 68    6/09/05 6:07p Fremont
 * EPRS FIXED: 37362 - IP-852 channel uses too much memory. Point all
 * packet allocations to the "master" object, and don't allocate any
 * buffers for the slaves.
 *
 * 67    5/20/05 3:25p Fremont
 * EPRS FIXED: 37164 - all IP-852 packets are marked as having bad CRC
 *
 * 66    5/18/05 5:49p Fremont
 * EPR 37089: Don't send bad packets, check CRC on incomming packets
 *
 * 65    12/09/04 6:23p Fremont
 * Change conditional compilation for self installed multicast
 *
 * 64    11/23/04 5:39p Fremont
 * EPRS FIXED: 35208 - Bombardier special - self-installed multicast IP
 * channel mode
 *
 * 63    11/05/04 3:43p Fremont
 * init some values
 *
 * 62    2/25/04 6:23p Iphan
 * Added an EOL at the end of the file to avoid compilation warning under
 * T2.2.1
 *
 * 61    10/01/03 6:33p Fremont
 * EPR 30338 - tunneled LonTalk packets were not being marked as secure
 * for EIA-852
 *
 * 60    9/19/03 11:47p Iphan
 * Rolled back changes of literal name from version 58
 *
 * 59    9/19/03 6:11p Iphan
 * EPRS FIXED:
 * 29442 - Unable to set aggregation time to greater than 60ms
 * 29441 - Routing is shut down if channel bandwidth is set to a low
 * number
 *
 * 58    9/19/03 3:09p Fremont
 * change literal name
 *
 * 57    9/03/03 5:36p Fremont
 * Implement extended packet header for EPR 29618.
 *
 * 56    6/10/03 5:35p Fremont
 * changes for EIA0852 auth
 *
 * 55    6/04/03 8:03p Iphan
 * IKP06042003: Fixed statistics shown in console & ConfigServer
 * IKP06042003: Support for LT & LTIP statistics in System Info
 *
 * 54    6/14/02 5:59p Fremont
 * remove warnings
 *
 * 53    11/06/01 9:25a Glen
 * Need control of zero crossing synchronization and attenuation for
 * LonTalk Validator.  Added these flags to sendPacket().  This propagated
 * to lots of places.  Also added control options to LtMsgOut.
 *
 * 51    12/20/99 11:22a Darrelld
 * Remove pktsOwned counter
 *
 * 50    12/20/99 11:14a Darrelld
 * Clamp statistics
 *
 * 49    12/20/99 6:47a Darrelld
 * Don't print negative pktsowned
 *
 * 48    12/17/99 10:04a Darrelld
 * Don't print negative pktsOwned
 *
 * 47    12/15/99 3:09p Darrelld
 * EPRS FIXED:  15825 Fix reordering
 *
 * 46    12/13/99 2:50p Darrelld
 * Avoid packets hanging out when aggregation is turned off
 *
 * 45    12/13/99 2:06p Darrelld
 * Use unsigned math with ticks
 *
 * 44    12/09/99 12:13p Darrelld
 * EPR 15860 - i.Lon fails as receiver when traffic is not aggregated.
 *
 * 43    12/08/99 3:03p Darrelld
 * Add some crumbs
 *
 * 42    11/16/99 11:21a Darrelld
 * EPRS FIXED: 15662, 15433 BW limit causes too much delay.
 * Also fix aggregation to send first packet immediately to reduce delay.
 *
 * 41    11/12/99 3:45p Darrelld
 * Updates for segment support
 *
 * 40    11/12/99 11:07a Darrelld
 * Changed vxlReportPrintf to vxlPrintf
 * Prettied up stats output
 *
 * 39    11/12/99 9:44a Darrelld
 * Use vxlReportPrintf for rtrstat output
 * Make synchronous mode work
 *
 * 38    11/02/99 3:54p Darrelld
 * Fix authentication for aggregated packets too. ;-)
 *
 * 37    11/02/99 11:13a Darrelld
 * Fix authentication, provide tracing when receiving authentication
 * failures
 *
 * 36    10/21/99 5:12p Darrelld
 * Protect against out of sequence session numbers
 *
 * 35    10/07/99 12:17p Darrelld
 * Report authorization failures in statistics
 *
 * 34    10/07/99 12:07p Glen
 * Allocation failure breakpoint
 *
 * 33    9/08/99 2:21p Darrelld
 * Session number is now a ULONG
 *
 * 32    9/07/99 4:53p Darrelld
 * Debug info
 *
 * 31    8/18/99 5:03p Darrelld
 * Fix bug in packet aggregation
 *
 * 30    8/16/99 4:35p Darrelld
 * Fix timeDiff compare for stale packets
 *
 * 29    8/13/99 1:05p Darrelld
 * Correctly process stale packets
 *
 * 28    8/12/99 9:25a Darrelld
 * Add stats for process pkt and route pkt
 *
 * 27    8/04/99 9:31a Darrelld
 * Debug for an assert
 *
 * 26    8/02/99 1:23p Darrelld
 * Add trace in processPacket
 *
 * 25    7/30/99 3:56p Darrelld
 * Remove assert in sendRFCtoMaster
 *
 * 24    7/30/99 8:59a Darrelld
 * Cleanup and streamline
 *
 * 23    7/28/99 4:47p Darrelld
 * Fix packet reordering and starvation
 *
 * 22    7/27/99 1:52p Darrelld
 * Fix packet reordering
 *
 * 21    7/26/99 5:18p Darrelld
 * Debugging of IP router features
 *
 * 20    7/20/99 5:26p Darrelld
 * Updates for reordering test
 *
 * 19    7/14/99 11:02a Darrelld
 * fix for proper getSession with master
 *
 * 18    7/08/99 1:11p Darrelld
 * Fix deadlock problem
 *
 * 17    7/06/99 5:24p Darrelld
 * Lock nesting debugging
 *
 * 16    7/02/99 8:45a Darrelld
 * Target specific files
 *
 * 15    6/30/99 4:41p Darrelld
 * Handle statistics properly
 *
 * 14    5/07/99 11:05a Darrelld
 * Upgrade for RFC packet formats
 *
 * 13    5/03/99 4:04p Darrelld
 * Add sending aggregated packets as they fill up when BW limiting is not
 * enabled.
 *
 * 12    4/22/99 5:02p Darrelld
 * Testing of routers
 *
 * 11    4/16/99 3:04p Darrelld
 * Aggregation and BW limiting works
 *
 * 10    4/13/99 4:57p Darrelld
 * Enhance for Aggregation and BW Limiting and test
 *
 * 9     3/30/99 5:10p Darrelld
 * intermediate checkin
 *
 * 8     3/17/99 3:43p Darrelld
 * Enhanced tests and fixed memory leaks
 *
 * 7     3/15/99 5:04p Darrelld
 * Intermediate checkin
 *
 * 6     3/12/99 4:47p Darrelld
 * intermediate checkin
 *
 * 5     3/11/99 5:01p Darrelld
 * intermediate checkin
 *
 * 4     3/03/99 4:54p Darrelld
 * intermediate checkin
 *
 * 3     2/22/99 10:56a Darrelld
 * intermediate check-in
 *
 * 2     2/09/99 5:05p Darrelld
 * Initial creation
 *
 * 1     2/02/99 9:16a Darrelld
 *
 */

//#include "stdafx.h"

#include <LtRouter.h>
//#include "LtMsg.h"
#include <LtLinkBase.h>
#include <LtIpPackets.h>
#include <LtPktReorderQue.h>
#include <LtIpMaster.h>
#include <LtLreIpClient.h>
#include <vxlTarget.h>
#include <IpLink.h>
#include <tickLib.h>
#include <LtMD5.h>
//#include <LtCUtil.h>
#include "SelfInstallMulticast.h"


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// Module data
//

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// LtLreIpClient implementation
//

//
// constructor
//
LtLreIpClient::LtLreIpClient(LreClientType clientType, LtChannel *pChannel, LtIpBase* pMasterLtIpBase)
	: LtLreClient(clientType, pChannel), LtIpBase(pMasterLtIpBase), /* not the master */
	m_qWaitingMsgs(true)
{
	assert( m_clientType == LT_IP_SOCKET );

	m_routerType				= LT_REPEATER;	// start as repeater, master will tell us
	m_pMaster					= NULL;
	m_pServer					= NULL;
	m_nSequence					= 0;
	m_ipAddress					= 0;			// no address known yet
	m_ipPort					= 0;
	m_ppktChanRoute				= NULL;
	m_nWaitingBytes				= 0;			// no waiting bytes yet
	m_nTickLastOneSend			= 0;			// when did we last send a single packet
	m_bSeenPacket				= false;		// we have not yet seen a packet from peer
	m_nPeerSession				= 0;
	m_nPeerSessionPrevious		= 0;
	m_nPeerNextSeq				= 0;
	m_ticksAtReorder			= 0;			// no packet time for waiting packets

	clearStats();

	m_nTestReorderEvery			= 0;			// test data for reordering test
	m_nTestReorderBy			= 0;
	m_pTestReorderPkt			= NULL;
	m_nTestSequence				= 0;
}

//
// destructor
//
LtLreIpClient::~LtLreIpClient()
{
	// stop us and all our base classes if needed
	stop();
	// release the waiting packet if any
	if ( m_pTestReorderPkt )
	{	m_pTestReorderPkt->release();
	}
	// master removes itself before doing this to avoid recursion
	if ( m_pMaster )
	{	m_pMaster->clientBye( this );
	}
}

// The server or Lre or Router engine routes packets for a living
// we forward all packets to her and she sends packets to us for sending out
// to our "peer" router on the channel.
// Just always believe what she says.
// Also we don't send stuff, or expect to receive stuff, from her unless
// she has called start. Until she says stop.
void	LtLreIpClient::setServer( LtLreServer* pServer )
{
	m_pServer = pServer;
	if ( pServer )
	{	pServer->registerClient( this );
	}
	// we are about to come on-line as a repeater.
}

//
// setRouting
//
// Set the routing information for this client
// Called by the master to update the Channel Routing
// information. Two kinds of things change with this packet.
//
// 1 The channel routing data that the LRE is excited about
// 2 The IP information to tell how to get to this device.
//
// This object does not own the channel routing data and
// does not destruct it, it just serves it up to the server
// when asked.
//
void	LtLreIpClient::setRouting( byte* pPktChanRoute )
{
	LtIpChanRouting	chr;
	boolean			bOk = false;

	lock();
	bOk = chr.parse( pPktChanRoute, false );
	// its been parsed before, so this is corruption of data
	assert( bOk );
	// THIS IS NOT A MEMORY LEAK
	// REMEMBER THE MASTER OWNS THESE PACKETS AND WILL CLEAN THIS UP
	// ON RETURN FROM HERE.
	m_ppktChanRoute = pPktChanRoute;
	m_pktChanRoute = chr;
	setRouterType((LtRouterType)chr.routerType );
	// if the destination address has changed for us, then
	// stop and start the link.
	if ( chr.ipUcAddress != m_ipAddress || chr.ipUcPort != m_ipPort )
	{
		boolean bActive = isActive();
		if ( bActive )
		{	stop();
		}
		m_ipAddress = chr.ipUcAddress;
		m_ipPort = chr.ipUcPort;
		// addresses in link set by master
		m_pLink->open("Ignored");
		start();
	}
	// notify the server that we have a new update on the way.
	unlock();
	notify();
}


// For the following calls from the LRE, there is a RACE to be considered.
// Say, for example, that the LRE is busy processing a sequence of calls
// to the client to obtain it's routing information. These calls are
// not atomic, and the client has no way of knowing when they start and
// when they stop or which calls constitute a "consistent set".
// If among these calls, a packet arrives which gives the client
// a new set of routing tables, then the LRE might obtain an inconsistent
// set of routing information.
// For the moment we assume that this is ok. The client and master deal
// with this race as follows:
//
// RFC packet arrives in the master. Master parses it and updates its
// database immediately. Under a lock on itself only of course.
//
// The master then sends the client the new information and the client
// updates itself under a lock and notifies the server of the change.
//
// If there is a RACE then the server will receive another notification
// and will start the update process again and receive a consistent
// set of data, someday. Who knows where packets go in the meantime.
// The assumption is that this is ok since it rapidly settles down.
//
// A related issue is with respect to deregisterClient on the server during
// an update. Since deregisterClient is synchronous, it blocks until the
// update process if any is complete.
// During an update process, the client is "locked" during each call.
// A stop call makes the client - inactive. and when it's inactive, it
// responds with false to all routing requests.
// The server may or may not call stop() during deregister. The client
// doesnt require it, but its not a problem if it occurs.
//


//
// getRoute
//
// Lre asks us for our routing info
// This is the list of domains from the Channel routing packet
// we are holding, if any.
//
boolean LtLreIpClient::getRoute(int& nIndex, LtRoutingMap *pRoute)
{
	boolean	bOk = false;
	lock();
	int		nDomains = m_pktChanRoute.domainBytes / LtIpDomain::SIZE;
#if 0 // test
	vxlReportEvent("LtLreIpClient::getRoute( %d ) routing packet %s domBytes %d nDoms %d\n",
				nIndex,
				m_pktChanRoute.isValid()?"Valid":"Not Valid",
				m_pktChanRoute.domainBytes, nDomains );
#endif // test
	while ( m_pktChanRoute.isValid() && nIndex < nDomains )
	{
		byte*		pDomain;
		LtIpDomain	ipdom;
		// So now let's convert the data from one form to another.
		pDomain = m_pktChanRoute.pDomains + (nIndex*LtIpDomain::SIZE);
		ipdom.parse( pDomain );
		// This looks a little cumbersom to build a LtRoutingMap object
		// from a bag of bytes lying around in rfc messages.
		// Got to create some intermediate objects and then set them.
		// The problem is that all the members of LtRoutingMap assume
		// that you have the classes that make it up rather than
		// raw data. Sorry, but RFC messages don't contain these classes.
		// Note that the only scheme that works causes the data to be copied
		// twice.
#if 0	// a simpler way to do it, but it won't work
		// These methods would cause one copy rather than two.
		// But all the parts of the pRoute are private so this won't work.
		pRoute->m_domain.set( ipdom.domainBytes, ipdom.domainLength );
		pRoute->m_subnets.set( ipdom.subnetMask, 0, LtIpDomain::MASKLEN );
		pRoute->m_groups.set( ipdom.groupMask, 0, LtIpDomain::MASKLEN );
#endif	// a simpler way
		pRoute->setRouterType( m_routerType );
		// Gee. Clever, we can use a temporary object with a constructor here
		LtDomain dom(ipdom.domainBytes, ipdom.domainLength);
		pRoute->setDomain( dom );
		// but no suitable constructors for these objects are available.
		LtSubnets	sub;
		LtGroups	grp;
		sub.set( ipdom.subnetMask, 0, LtIpDomain::MASKLEN );
		grp.set( ipdom.groupMask, 0, LtIpDomain::MASKLEN );
		pRoute->setSubnets( sub );
		pRoute->setGroups( grp );
		nIndex++;
		bOk = true;
#if 0 // test
		vxlReportEvent("LtLreIpClient::getRoute - type %d", pRoute->getRouterType() );
		pRoute->getDomain().dump();
		vxlReportEvent("\n");
#endif // test
		break;
	}
	unlock();
#if 0 // test
	if ( bOk )
	{	vxlReportEvent("LtLreIpClient::getRoute - return true index %d\n", nIndex );
	}
	else
	{	vxlReportEvent("LtLreIpClient::getRoute - return false index %d\n", nIndex );
	}
#endif // test
	return	bOk;
}

//
// getAddress
//
// Lre asks us for our addressing info
// This the list of subnet, node addresses, with their attendant
// domains from the Channel routing packet we are holding, if any.
//
boolean LtLreIpClient::getAddress(int& nIndex, LtDomain* pDom,
									LtSubnetNode* pS, LtGroups* pG)
{
	boolean	bOk = false;
	lock();
	int		nDomains = m_pktChanRoute.domainBytes / LtIpDomain::SIZE;
	int		nSnns = m_pktChanRoute.subnetNodeBytes / LtIpSubnetNode::SIZE;
	int		nDomIdx;
	byte*	p;
	byte*	pDomain;
	while ( m_pktChanRoute.isValid() && nIndex < nSnns )
	{
		LtIpDomain		ipdom;
		LtIpSubnetNode	snn;

		p = m_pktChanRoute.pSubnetNodes + (nIndex*LtIpSubnetNode::SIZE );
		snn.parse( p );
		nDomIdx = snn.domainIndex;
		if ( nDomIdx >= nDomains )
		{
			vxlReportEvent("IpClient::getAddress - malformed chan routing SNN domain index out of range %d\n",
						nDomIdx );
			break;
		}
		*pS = LtSubnetNode( snn.subnet, snn.node );

		// So now let's convert the data from one form to another.
		pDomain = m_pktChanRoute.pDomains + (nDomIdx*LtIpDomain::SIZE);
		ipdom.parse( pDomain );

		LtDomain dom(ipdom.domainBytes, ipdom.domainLength);
		*pDom = dom;
		LtGroups grps;
		*pG = grps;
		nIndex++;
		bOk = true;
		break;
	}
	unlock();
	return	bOk;
}

//
// getAddress
//
// Lre asks us for our addressing info
// This is the list of NeuronIds from our Channel Routing packet
//
boolean LtLreIpClient::getAddress(int& nIndex, LtUniqueId* pU)
{
	boolean	bOk = false;
	lock();
	int		nUids = m_pktChanRoute.neuronIdBytes / LtIpNeuronId::SIZE;
	byte*	p;
	while ( m_pktChanRoute.isValid() && nIndex < nUids )
	{
		// Compute the address of the appropriate unique Id
		// note that this assumes that the RFC message format is
		// exactly the NeuronId and nothing else.
		p = m_pktChanRoute.pNeuronIds + (nIndex*LtIpNeuronId::SIZE );

		*pU = LtUniqueId( p );
		nIndex++;
		bOk = true;
		break;
	}
	unlock();
	return	bOk;
}


//
// start
//
// Start the client operation by checking that all is well
// with the link and then queueing receives to the link.
//
boolean		LtLreIpClient::start()
{
	boolean		bOk = false;
	lock();
	// Maybe more to do later, but for now, just call base class
	bOk = LtIpBase::start();
	unlock();
	return bOk;
}


//
// stop
//
// Stop the link with a reset
//
boolean		LtLreIpClient::stop()
{
	boolean		bOk = false;
	LtPktInfo*	pPkt;
	LtQue*		pItem;
	lock();

	// discard all items in the aggregation queue
	while ( m_qWaitingMsgs.removeTail( &pItem ) )
	{	pPkt = (LtPktInfo*)pItem;
		pPkt->release();
	}
	m_nWaitingBytes = 0;
	// discard all items in reorder queue
	m_qReorderQue.discard();

	// Maybe more to do later, but for now, just call base classes
	bOk = LtLreClient::stop();	// this doesnt do much
	bOk = LtIpBase::stop();		// shut down the link and recover buffers

	unlock();
	return bOk;
}


//
// sendMore
//
// send more packets if we can. Return true if we need to be called back here
// later.
// If bFullPktsOnly, then only send full packets. That is, stop sending when the
// m_nWaitingBytes is less than UDP_MAX_PKT_LEN
//
boolean LtLreIpClient::sendMore( boolean bFullPktsOnly )
{
	boolean		bReturn = true;
	LtPktInfo*	pPkt;
	LtPktInfo*	pPkt2 = NULL;
	LtQue*		pItem;
	UINT		nAgg = 0;
	lock();
	ULONG		timestamp = m_pMaster->getTimestamp();
	ULONG		timeDiff;

	m_stats.m_nPktsMaxQueDpth = max( m_stats.m_nPktsMaxQueDpth, (ULONG)m_qWaitingMsgs.getCount() );
	while ( TRUE )
	{
		if (m_qWaitingMsgs.removeHead( &pItem ) == false)
		{
			bReturn = false;
			break;
		}
		pPkt = (LtPktInfo*)pItem;
		pPkt->setCrumb("LtLreIpClient::sendMore - packet removeHead" );
		m_nWaitingBytes -= pPkt->getDataSize() +
							LtIpPktHeader::hdrSize(m_pMaster->useExtPktHdrs());
		m_nWaitingBytes = max(0,m_nWaitingBytes); // limit to zero

		timeDiff = timestamp - pPkt->getTimestamp();
		if ( m_pMaster->isStaleSending( timestamp, pPkt->getTimestamp() ) )
		{
			pPkt->setCrumb("LtLreIpClient::sendMore - packet released stale" );
			pPkt->release();
			countClamp(m_stats.m_nPktsStale);
		}
		else if ( !aggregatePacket( &pPkt2, pPkt ) )
		{
			// if we couldnt send the packet, then
			// put it back on the queue
			pPkt->setCrumb("LtLreIpClient::sendMore - packet requeued" );
			m_qWaitingMsgs.insertHead( pItem );
			m_nWaitingBytes += pPkt->getDataSize() +
								LtIpPktHeader::hdrSize(m_pMaster->useExtPktHdrs());
			// if we've been working on a packet, then this one wouldn't fit
			// so send it and keep trying to aggregate packets
			if ( pPkt2 )
			{
				m_stats.m_nPktsAggMax = max( m_stats.m_nPktsAggMax, nAgg );
				countClamp(m_stats.m_nPktsSent);
				addClamp( m_stats.m_nBytesSent,  pPkt2->getDataSize() );
				sendPacket( pPkt2, false );
				pPkt2 = NULL;
				nAgg = 0;
				// if we are sending full packets only, and we have less than
				// a full packet left, then don't send the remainder.
				if ( bFullPktsOnly && ( m_nWaitingBytes < UDP_MAX_PKT_LEN ) )
				{
					bReturn = false;
					break;
				}
			}
			else
			{
				break;
			}
		}
		else
		{	nAgg++;
		}

	}
	if ( pPkt2 )
	{	// keep track of bytes sent and maximum aggregated packets
		addClamp(m_stats.m_nBytesSent, pPkt2->getDataSize() );
		m_stats.m_nPktsAggMax = max( m_stats.m_nPktsAggMax, nAgg );
		countClamp(m_stats.m_nPktsSent);
		sendPacket( pPkt2, false );
		pPkt2 = NULL;
	}
	unlock();
	return bReturn;
}

//
// aggregatePacket
//
// Build a packet into another packet. Including adding a data header.
// If the pointer at ppPktAgg is NULL, then a new packet is created.
// This routine is actually used to encapsulate every packet whether we are
// aggregateing or not.
//
boolean LtLreIpClient::aggregatePacket( LtPktInfo** ppPktAgg, LtPktInfo* pPkt )
{
	LtPktInfo*		pPkt2 = *ppPktAgg;
	LtIpPktBase		pktH;
	boolean			bOk = false;
	int				nSize;
	int				nOffset;
	int				nRemSize;
	byte*			p;
	byte*			p1;
	byte*			p2;
	byte*			p3;
	int				nAuthSize = m_pMaster->isAuthenticating()? LtMD5::LTMD5_DIGEST_LEN : 0;

	nSize = pPkt->getDataSize();
	if ( !m_pMaster->okToSend( nSize + LtIpPktHeader::hdrSize(m_pMaster->useExtPktHdrs()) ) )
	{	return bOk;
	}
	if ( pPkt2 == NULL )
	{
		// Get a packet to aggregate into
		// Set the data pointer
		pPkt2 = m_pAlloc->allocPacket();
		// allocation failure, just bug out
		if ( pPkt2 == NULL )
		{	return bOk;
		}
		*ppPktAgg = pPkt2;
		pPkt2->setMessageData( pPkt2->getBlock(), 1, pPkt2 );;
		nRemSize = pPkt2->getBlockSize();
		nOffset = 0;
	}
	else
	{	nRemSize = pPkt2->getBlockSize() - pPkt2->getDataSize();
		nOffset = pPkt2->getDataSize();
	}
	// we might not have been able to get a packet
	while ( pPkt2 )
	{
		nSize = pPkt->getDataSize();
		if ( (nSize + LtIpPktHeader::hdrSize(m_pMaster->useExtPktHdrs()) + nAuthSize) > nRemSize )
		{	break;
		}
		p = pPkt->getDataPtr();
		p1 = pPkt2->getBlock();
		p2 = p1 + nOffset;
		if ( nOffset != 0 )
		{	countClamp(m_stats.m_nPktsAggregated);
		}

		// test code
#if 1
		{
			ULONG		pktSeq = pPkt->getSequence();
			int			seqDiff = pktSeq - m_nTestSequence;
			if ( seqDiff < 0 )
			{	vxlReportEvent("aggregatePacket - sending bad sequence %d last %d\n",
								pktSeq, m_nTestSequence );
			}
			m_nTestSequence = pktSeq;
		}
#endif // test code

		m_pMaster->setPktExtHdrData(&pktH);
		pktH.packetSize = nSize + LtIpPktHeader::hdrSize(m_pMaster->useExtPktHdrs());
		pktH.packetType = PKTTYPE_DATA;
		pktH.session = m_pMaster->getSession();
		// be sure and use the sequence and timestamp from
		// packet, since these were valid when packet was first seen here.
		pktH.sequence = pPkt->getSequence();
		if ( m_pMaster->useTimestamp() )
		{	pktH.timestamp = pPkt->getTimestamp();
		}
		else
		{	pktH.timestamp = 0;
		}
		pktH.vendorCode = VENDOR_STANDARD;
		// iLON 1000 set data packets to version 0!!!
		pktH.version = m_pMaster->backwardCompatibleChan() ? VERSION_ZERO : VERSION_ONE;
		p3 = pktH.build( p2 );
		memcpy( p3, p, nSize );
		pPkt2->setMessageNoRef( p1, (p3+nSize+nAuthSize) - p1 );
		p3 += nSize;
		// Now build the authentication code if we should
		if ( nAuthSize )
		{
			// Set secure bit for EIA-852
			if (m_pMaster->isEIA852Auth())
				pktH.markSecure(p2);
			// build the digest and store at the end of the packet
			LtMD5::digest( m_pMaster->getAuthenticSecret(),
							LtIpMaster::AUTHENTIC_SECRET_SIZE,
							p2, pktH.packetSize, p3, m_pMaster->isEIA852Auth());

		}
		// tell the client that we are done with the packet
		pPkt->setCrumb("LtLreIpClient::aggregatePacket - packet released" );
		pPkt->release();
		bOk = true;
		break;
	}
	return bOk;
}


//
// processPacket
//
// Packet sent to us by the LreEngine. We need to send it
// to the link.
//
static ULONG	nLastTick = 0;
#define DEADTIME 10000

void LtLreIpClient::processPacket(boolean bPriority, LtPktInfo *pPkt)
{
	LtPktInfo*		pPkt2 = NULL;
	boolean			bSendPacket = false;
	boolean			bOk;
	LtSts			sts;
	LtQue*			pItem;

#ifdef SUPPRESS_INTERNAL_TARGET_MSGS_ON_IP_PORT
	// This code is what would need to be done if we chose to suppress sending messages
	// on the IP "port" that have a single internal target. This is done on the LON-side
	// physical port to exclude some internal traffic from going out on the network.
	// The IP side currently does not follow this behavior, relying on the router and the
	// IP-852 mechanisms to limit network traffic. If we want to implement the above
	// behavior, activate the following code, plus change the routine routeMultiple() in
	// LtLre.h to include the IP port as well as the LON port.
	// This would only be for the iLON.
#if PRODUCT_IS(ILON)
	if (pPkt->getRouteMultipleNoXmit() && !pPkt->getRouteMultiple()) // These should be mutually exclusive...
	{
		// Don't transmit the packet.
		// This is just to allow the iLON to send it to the LSPA on the LON port side.
		pPkt->release();
		return;
	}
#endif
#endif

	// if we have a link registered
	// then send the packet to the link to be sent
	// m_bLreClientActive
	countClamp(m_stats.m_nPktsProcessed);
	if ( m_bActive && pPkt->getIsValidL2L3Packet())
	{
		lock();
#if 1 // test
		ULONG	nTickNow = tickGet();
		ULONG	nMsDead;
		nMsDead = nTickNow -nLastTick;
		nMsDead = ticksToMs( nMsDead );
		if ( nMsDead > DEADTIME )
		{
			vxlReportEvent("IpClient::processPacket - packet arrived\n");
		}
		nLastTick = nTickNow;
#endif // test
		pPkt->setSequence( m_nSequence++ );
		pPkt->setTimestamp( m_pMaster->getTimestamp() );
		do	// provide escape to allow sending first packet in aggregation
		{
			if ( m_pMaster->getClientMustQueue() )
			{
				// are we bandwidth limiting?
				boolean		bBWLimit = m_pMaster->getBWLimit( NULL );
				UINT		nMsAgg = 0;

				m_pMaster->getAggregate( (int*)&nMsAgg );
				// if no messages waiting, and it has been a long time since we queued
				// a message.
				if ( !bBWLimit && m_qWaitingMsgs.getCount() == 0)
				{
					UINT	nTickNow = tickGet();
					UINT	nTicksSince = nTickNow - m_nTickLastOneSend;
					UINT	nMsSince = ticksToMs( nTicksSince );
					if ( nMsSince > (UINT)nMsAgg )
					{
						m_nTickLastOneSend = nTickNow;
						bSendPacket = true;
						pPkt->setCrumb("LtLreIpClient::processPacket first packet sent");
						break;
					}
				}
				pPkt->setCrumb("LtLreIpClient::processPacket queued");
				m_qWaitingMsgs.insertTail( pPkt );
				m_nWaitingBytes += pPkt->getDataSize() +
								LtIpPktHeader::hdrSize(m_pMaster->useExtPktHdrs());

				// if we aren't bandwidth limiting, and we have more than 548 waiting bytes
				// then let'er rip and send out the messages until we have fewer than 548 bytes
				// waiting.
				// The timer will go off and drain the queue at that point.

				if ( !bBWLimit )
				{
					if ( m_nWaitingBytes > UDP_MAX_PKT_LEN )
					{	sendMore( true );
					}
				}
				else
				{
					// limit queue depth in each client
					// master is in charge of the client queue depth.
					int		ncqd = m_pMaster->getClientQueueDepth();
					while ( m_qWaitingMsgs.getCount() > ncqd )
					{
						m_qWaitingMsgs.removeHead( &pItem );
						pPkt2 = (LtPktInfo*)pItem;
						pPkt2->setCrumb("LtLreIpClient::processPacket released queue depth");
						m_nWaitingBytes -= pPkt2->getDataSize() +
									LtIpPktHeader::hdrSize(m_pMaster->useExtPktHdrs());
						m_nWaitingBytes = max(0,m_nWaitingBytes); // limit to zero

						pPkt2->release();
					}
				}
			}
			else
			{	bSendPacket = true;
			}
		} while ( false );

		if ( bSendPacket )// packet not sent the other way )
		{
			// if we are supposed to send this packet, then send out packets ahead of it
			// if any.
			if ( m_qWaitingMsgs.getCount() )
			{	sendMore();
			}
			// client not queueing packets
			// So we "aggregate" and send each packet
			bOk = aggregatePacket( &pPkt2, pPkt );
			if ( bOk )
			{
				// test reordering
				if ( m_pMaster->m_bTReorder )
				{	// if we have a waiting packet, then count down the waiting number of
					// packets before we send it.
					if ( m_pTestReorderPkt )
					{	m_nTestReorderBy--;
						if ( m_nTestReorderBy <= 0 )
						{	// after enough packets have gone by, then send the waiting packet
							sts = sendPacket( m_pTestReorderPkt, bPriority );
							m_pTestReorderPkt = NULL;
						}
					}
					else
					{	// if no waiting packet, then count down until we are supposed
						// to mess with a packet
						m_nTestReorderEvery--;
						if ( m_nTestReorderEvery <= 0 )
						{	// delete the packet or just reorder it
							if ( m_pMaster->m_bTReorderDelete )
							{	pPkt2->release();
							}
							else
							{	m_pTestReorderPkt = pPkt2;
								m_nTestReorderBy = m_pMaster->m_nTReorderBy;
							}
							// regardless, the packet is no longer around to be sent
							pPkt2 = NULL;
							// reset the "every" counter now
							if ( m_pMaster->m_bTReorderRandom )
							{	m_nTestReorderEvery = rand() % m_pMaster->m_nTReorderEvery;
							}
							else
							{	m_nTestReorderEvery = m_pMaster->m_nTReorderEvery;
							}
						}
					}
				} // test reordering
				// if we have a packet to send, then send it
				if ( pPkt2 )
				{
					addClamp(m_stats.m_nBytesSent, pPkt2->getDataSize());
					countClamp(m_stats.m_nPktsSent);
					sts = sendPacket( pPkt2, bPriority );
				}
			}
			else
			{	// woops. We couldnt get a secondary packet, so just drop the packet
				pPkt->setCrumb("LtLreIpClient::processPacket - packet dropped" );
				pPkt->release();
				countClamp(m_stats.m_nPktsDropped);
			}
		}
		unlock();
	}
	else
	{
		// If not a valid packet, log different stuff
		if (!pPkt->getIsValidL2L3Packet())
		{
			pPkt->setCrumb("LtLreIpClient::processPacket - invalid packet dropped" );
			countClamp(m_stats.m_nPktsDropped);
		}
		else
		{
			// if we are shutdown, then just release the packet as if
			// it had been sent
			pPkt->setCrumb("LtLreIpClient::processPacket - packet released inactive" );
		}
		pPkt->release();
	}
}

//
//  sendRFCtoMaster
//
// We received an RFC message on a client socket.
// This means another client is sending us RFC messages.
// We copy the message to another packet allocated from the master's pool,
// then include the src IP address and port, then allow the master
// to process the packet and return the response.
// Frankly, this is a hairy wart on an otherwise reasonable protocol.
//
void	LtLreIpClient::sendRFCtoMaster( byte* pData, int nSize,
									   ULONG ipSrcAddr, word ipSrcPort )
{
	LtPktInfo*		pPkt;

	pPkt = m_pMaster->allocPacket();
	// if the master is out of packets, then just discard the rfc message
	if ( pPkt )
	{
		// cant use copyMessage because it's only part of the pktInfo that we want
		/// bad stuff
		//assert( pPkt->getBlockSize() >= nSize );
		if ( pPkt->getBlockSize() < nSize )
		{
			vxlReportEvent("LtLreIpClient::sendRFCtoMaster - packet too small blkSize %d pktSize %d\n",
					pPkt->getBlockSize(), nSize );
		}
		else
		{
			memcpy( pPkt->getBlock(), pData, nSize );
			pPkt->setMessageData( pPkt->getBlock(), nSize, pPkt );
			pPkt->setIpSrcAddr( ipSrcAddr );
			pPkt->setIpSrcPort( ipSrcPort );
			m_pMaster->RfcFromClient( pPkt );
			countClamp(m_stats.m_nPktsRfcIn);
		}
	}
	else
	{	// missed packets are those that we could not process due
		// to a buffering problem
		countClamp(m_stats.m_nPktsMissed);
	}
}

//
// packetReceived
//
// Packet received from the link. We need to send it to the
// engine.
// Base class takes care of requeueing packets to the link.
// If we are authenticating packets, then skip over the authentication codes
// after we authenticate packets. If any fail, then bail out on the whole packet.
//
void LtLreIpClient::packetReceived(	boolean bPriority,
						LtPktInfo*	pPkt,
						LtSts sts)
{
	if ( m_bActive && (sts == LTSTS_OK ) )
	{
		lock();
		// decapsulate the packet and then pass it on to the server
		LtIpPktHeader	phd;
		int				nSize = pPkt->getDataSize();
		byte*			pData = pPkt->getDataPtr();
		byte*			pDataSt = pData;
		byte*			pEnd = pData + nSize;
		boolean			bRouted = false;
		LtPktInfo*		pPkt2;
		int				nAuthSize = m_pMaster->isAuthenticating()?LtMD5::LTMD5_DIGEST_LEN:0;
		boolean			bOk;
		int				nDataLen;
		int				nLtPayloadLen;
		byte*			pLtData;

		// count packets and bytes received
		addClamp(m_stats.m_nBytesReceived, nSize);
		countClamp(m_stats.m_nPktsReceived);

		while ( pData < pEnd )
		{
			if ( !phd.parse( pData ) )
			{	// invalid packets are those that don't parse
				countClamp(m_stats.m_nPktsInvalid);
				break;
			}

			if ( m_pMaster->isAuthenticating() )
			{	// authenticate the RFC packet
				nDataLen = MAX( nAuthSize, phd.packetSize );
				bOk = LtMD5::checkDigest( m_pMaster->getAuthenticSecret(),
											LtIpMaster::AUTHENTIC_SECRET_SIZE,
											pData, nDataLen , &pData[nDataLen],
											m_pMaster->isEIA852Auth());
				if ( !bOk )
				{	// authorization failures are packets that don't
					// pass authentication. High counts indicate attempt
					// to break in, or some device with a bad secret.

					// test authentication
					byte*	p = &pData[nDataLen];
					byte	d[LtMD5::LTMD5_DIGEST_LEN];

					LtMD5::digest( m_pMaster->getAuthenticSecret(),
											LtIpMaster::AUTHENTIC_SECRET_SIZE,
											pData, nDataLen , (byte*)&d,
											m_pMaster->isEIA852Auth());

					vxlReportEvent("Client Auth Failure: authSize %d frame size %d offset %d dataLen %d EIA-852 %s\n"
				"        Received   %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n"
				"        Correct    %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
									nAuthSize, nSize, pData - pPkt->getDataPtr(), nDataLen,
									(m_pMaster->isEIA852Auth() ? "T" : "F"),
									p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],
									p[8],p[9],p[10],p[11],p[12],p[13],p[14],p[15],
									d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7],
									d[8],d[9],d[10],d[11],d[12],d[13],d[14],d[15]

									);
					// end of test


					m_pMaster->countAuthFailures();
					break;
				}
			}


			if ( phd.packetType == PKTTYPE_DATA )
			{
				if ( m_pServer )
				{	// if this is the one and only packet in this buffer
					// then send it on to the server directly.
					// otherwise, build a new packet and deaggregate the packet.
					//
					pLtData = phd.pRemaining;	// start of LonTalk packet
					if ( (pData == pDataSt) && ( (pData+phd.packetSize+nAuthSize) == pEnd ) )
					{	pPkt2 = pPkt;
						pPkt2->setMessageNoRef( pLtData, phd.packetSize - phd.hdrSize());
						bRouted = true;
					}
					else
					{	pPkt2 = m_pMaster->allocMsgRef();
						if ( pPkt2 == NULL )
						{
							countClamp(m_stats.m_nAllocFailures);
							break;		// bad news - lost a packet due to allocator empty
						}
						pPkt2->setMessageData( pLtData, phd.packetSize - phd.hdrSize(), pPkt );
						pPkt2->initPacket();
					}

					// We must check the CRC for each LonTalk packet and mark failures
					boolean isValidPacket = true;
					byte    l2PacketType = L2_PKT_TYPE_INCOMING;
					nLtPayloadLen = pPkt2->getDataSize();	// should really limit to buffer size...
					if (nLtPayloadLen < 2)	// minimum bytes for CRC. Could check len < 8 (min valid LT pkt)
					{
						vxlReportEvent("LT packet received on IP channel with invalid length (%d)!\n", pPkt2->getDataSize());
						isValidPacket = FALSE;
						l2PacketType = L2_PKT_TYPE_PACKET_TOO_SHORT;
					}
					else
					{
						byte crchi = pLtData[nLtPayloadLen-2];
						byte crclo = pLtData[nLtPayloadLen-1];
						LtCRC16(pLtData, nLtPayloadLen-2);
						if ((pLtData[nLtPayloadLen-2] != crchi) ||
							(pLtData[nLtPayloadLen-1] != crclo))
						{
							vxlReportEvent("LT packet received on IP channel with invalid CRC!!!!!\n");
							isValidPacket = FALSE;
							l2PacketType = L2_PKT_TYPE_CRC;
							// restore the data back to the (bad) original. No need to if they match.
							pLtData[nLtPayloadLen-2] = crchi;
							pLtData[nLtPayloadLen-1] = crclo;
						}
					}
					pPkt2->setIncomingSicbData(isValidPacket, l2PacketType);

					// our own private routePacket routine takes care of
					// packet reordering.
					routePacket( bPriority, phd, pPkt2 );
					if ( bRouted ) break;
				}
			}
			else
			{	// send any parsed, but non-data packets to the master
				// to handle
				sendRFCtoMaster( pData, phd.packetSize,
								pPkt->getIpSrcAddr(), pPkt->getIpSrcPort() );
			}
			pData += phd.packetSize + nAuthSize;
			//assert( pData <= pEnd );
			if ( pData > pEnd )
			{	vxlReportEvent("IpClient::packetReceived - ERROR pData 0x%08x > pEnd 0x%08x\n",
								pData, pEnd );
				phd.dump();
				break;
			}
		}
		// if we didnt route the packet directly, then release
		// the master now
		if ( ! bRouted )
		{	pPkt->release();
		}
		unlock();
	}
	else
	{	pPkt->release();
	}
}

//
// routePacket
//
// route the packet and take care of any reordering issues
//
void	LtLreIpClient::routePacket( boolean bPriority, LtIpPktHeader& phd, LtPktInfo* pPkt )
{
	int		seqDiff;
	int		timeDiff;
	int		nChanTimeout = 0;
	word	sChanTimeout = 0;
	boolean bStale = false;

#ifdef WIN32X
	pPkt->parsePktLayers2and3();
	vxlReportEvent("Route packet [%d]\n", pPkt->getData()[10]);
#endif
	pPkt->setCrumb("LtLreIpClient::routePacket before checks");

	if ( m_pMaster->getChannelTimeout( &sChanTimeout ) )
	{	nChanTimeout = sChanTimeout;
	}
	// get the stale time of the packet.
	if ( phd.timestamp == 0 || nChanTimeout == 0 )
	{}
	else
	{	timeDiff = phd.getTimestamp() - phd.timestamp;
		if ( timeDiff < 0 )	timeDiff = -timeDiff;
		if ( timeDiff > nChanTimeout )
		{	bStale = true;
		}
	}
	if ( !m_bSeenPacket ||
		phd.session != m_nPeerSession
		)
	{	// don't allow stale packets to change our perceived session id.  A packet
		// is considered stale if it fails the time stamp check or if it is from
		// a previous session.  This allows for the case where the originator is
		// changing sessions (due to a move and change) and packets get out of
		// order.
		if ( bStale )
		{
			pPkt->setCrumb("LtLreIpClient::routePacket release stale");
			pPkt->release();
			countClamp(m_stats.m_nPktsStale);
		}
		else
		{
			// comment out the following line to pass packets without reordering
			// and not drop out of order either
#if INCLUDE_SELF_INSTALLED_MULTICAST
			// Note comment above -- for self-installed multicast mode,
			// skip reordering and ignore "stale" sessions -- just route all packets
			if (!m_pMaster->selfInstalledMcastMode())
#endif
			{
				m_bSeenPacket = true;
				m_nPeerSessionPrevious = m_nPeerSession;
			}
			m_nPeerSession = phd.session;
			m_nPeerNextSeq = (phd.sequence) + 1;
			m_qReorderQue.discard();
			// we sent a packet to the engine
			countClamp(m_stats.m_nPktsRouted );
			pPkt->setCrumb("LtLreIpClient::routePacket to server");
			m_pServer->routePacket( bPriority, this, pPkt );
		}
	}
	else
	{
		if ( phd.sequence == m_nPeerNextSeq )
		{	m_nPeerNextSeq++;
			//if packet in sequence is stale, then toss it out now to avoid
			// messing up the sequence number space. We already know that
			// the session id is the same.
			if ( bStale )
			{
				pPkt->setCrumb("LtLreIpClient::routePacket release stale seqNo");
				pPkt->release();
				countClamp(m_stats.m_nPktsStale);
			}
			else
			{
				// we sent a packet to the engine
				countClamp(m_stats.m_nPktsRouted);
				pPkt->setCrumb("LtLreIpClient::routePacket route to server 2");
				m_pServer->routePacket( bPriority, this, pPkt );
				// route any waiting packets that match the sequence number from
				// the reorder queue
				routeWaitingPackets( (ULONG)-1 );
			}
		}
		else
		{
			seqDiff = phd.sequence - m_nPeerNextSeq;
			// if ( seqDiff < 0 )
			// -100 check is to avoid having a huge gap in expected sequence 
			// numbers in the event that an incorrect sequence number is 
			// received.
			if ( seqDiff < 0 && seqDiff > -100 )
			{
				pPkt->setCrumb("LtLreIpClient::routePacket duplicate");

				pPkt->release();
				// count old packets as duplicates
				countClamp(m_stats.m_nPktsDuplicates);
			}
			else
			{	// if we are reordering packets, then escrow the packets
				// someday, there might be another case. For a proxy, we might
				// just pass packets in the wrong order, and reorder them at
				// the end. But since we are only building an end - Ip <-> LonTalk
				// router now, then we must never fail to reorder the packets
				// for LonTalk correctness
				if ( m_pMaster->isReordering() )
				{
					if ( bStale )
					{
						pPkt->setCrumb("LtLreIpClient::routePacket isReorder, stale");

						pPkt->release();
						countClamp(m_stats.m_nPktsStale);
					}
					else
					{
						pPkt->setCrumb("LtLreIpClient::routePacket onReorder Que");

						m_qReorderQue.insert( pPkt, phd );
						// if this is the first packet we have stored here then
						// set the tick count for proper delay
						if ( m_ticksAtReorder == 0 )
						{	m_ticksAtReorder = tickGet();
						}
					}
				}
				else
				{	// if not, then update the sequence number to skip the missing
					// packet and move on
					addClamp(m_stats.m_nPktsLost, seqDiff);
					m_nPeerNextSeq = phd.sequence + 1;
					if ( bStale )
					{
						pPkt->setCrumb("LtLreIpClient::routePacket stale SkipSequence");

						pPkt->release();
						countClamp(m_stats.m_nPktsStale);
					}
					else
					{
						// we sent a packet to the engine
						countClamp(m_stats.m_nPktsRouted);
						pPkt->setCrumb("LtLreIpClient::routePacket to server SkipSequence");
						m_pServer->routePacket( bPriority, this, pPkt );
					}
					// route any waiting packets that match the sequence number from
					// the reorder queue
					if ( !m_qReorderQue.lockedIsEmpty() )
					{	routeWaitingPackets( (ULONG)-1 );
					}
				}
			}
		}
	}
}


//
// routeWaitingPackets
//
// route any packets waiting on the sequence number to become correct
//
void	LtLreIpClient::routeWaitingPackets( ULONG nEscrowTick )
{
	// if there are no packets, and timer went off, then
	// just forget we had a timer and bug out.
	if ( m_qReorderQue.lockedIsEmpty() )
	{	m_ticksAtReorder = 0;
		return;
	}

	// if we have packets waiting, and we weren't called with -1
	// then check the time to see if we should ship them out.
	// but if ticksAtReorder is zero or the caller passed -1, then
	// just ship the packets out.
	if ( m_ticksAtReorder != 0 && nEscrowTick != (ULONG)-1 )
	{
		ULONG	nMs = nEscrowTick - m_ticksAtReorder;
		nMs = ticksToMs( nMs );
		if ( nMs < m_pMaster->getEscrowTime() )
		{	return;
		}
	}
	// we have passed all waiting packets along now
	m_ticksAtReorder = 0;

	LtPktInfo*		pPkt;
	ULONG			timeNow;
	ULONG			delta = 0;
	word			wdelta;
	boolean			bRemoveStale = m_pMaster->getChannelTimeout( &wdelta );
	int				nPkts = 0;
	ULONG			nSeq;

	if ( bRemoveStale && wdelta )
	{
		delta = wdelta;
		timeNow = m_pMaster->getTimestamp();
		nPkts = m_qReorderQue.discardStale( timeNow, delta );
		addClamp(m_stats.m_nPktsDropped, nPkts);
	}
	// the assumption is that dropped and reordered packets are very infrequent
	// So, when the escrow timer goes off, we drain the escrow queue
	// drain the escrow queue
	do
	{
		// setup to fetch first item in queue, if any
		nSeq = m_nPeerNextSeq;
		m_qReorderQue.sequenceOfFirst( m_nPeerNextSeq );
		nSeq = m_nPeerNextSeq - nSeq;
		addClamp(m_stats.m_nPktsLost, nSeq);
		pPkt = m_qReorderQue.removeSequence( m_nPeerNextSeq );
		if ( pPkt )
		{
			// we sent a packet to the engine
			countClamp(m_stats.m_nPktsRouted);
			pPkt->setCrumb("LtLreIpClient::routeWaitingPackets to server");
			m_pServer->routePacket( false, this, pPkt );
			countClamp(m_stats.m_nPktsReordered);
			m_nPeerNextSeq++;
		}
	} while ( pPkt );
}



//
// freeMessage
//
// We are given opportunity to dispose of a message
//
boolean LtLreIpClient::freeMessage(LtPktInfo* pPkt)
{
	boolean result = true;
	lock();
	LtSts sts;

	boolean bPriority = pPkt->getPriority();

	pPkt->setMessageNoRef(pPkt->getBlock(), pPkt->getBlockSize());
	pPkt->initPacket();

	sts = m_pLink->queueReceive( pPkt, bPriority, pPkt->getFlags(), pPkt->getBlock(),
			  					 pPkt->getBlockSize() );

	if (sts != LTSTS_PENDING)
	{
		// Expected queue is full.  Try the other.
		bPriority = !bPriority;
		sts = m_pLink->queueReceive( pPkt, bPriority, pPkt->getFlags(), pPkt->getBlock(),
				  					 pPkt->getBlockSize() );

		if (sts != LTSTS_PENDING)
		{
			// Driver is full.  This can occur if a reset occurs while
			// buffers are outstanding.
			result = false;
		}
	}

	unlock();

	return result;
}

//
// reportStatus
//
// report our current status
//
void LtLreIpClient::reportStatus()
{
	LtIpAddressStr		ias;
	lock();

	ias.setIaddr( m_ipAddress );
	vxlPrintf(
		"Client Statistics for IP address: %s (%d) - %s\n",
		ias.getString(), m_ipPort, isActive()?"Active":"Not Active" );
	if ( isActive() )
	{
		reportStats();
		vxlPrintf(
			 //      1234567890     1234567890     1234567890     1234567890
			"        Waiting msgs%5d\n",
						m_qWaitingMsgs.getCount() );

	}
	// now report status of the link
	if ( m_pLink )
	{	((CIpLink*)m_pLink)->reportStatus();
	}
	else
	{	vxlPrintf("LtLreIpClient::reportStatus - pLink is NULL\n");
	}
	unlock();

}

//
// clearStats
//
void	LtLreIpClient::clearStats()
{
	m_stats.clear();
}

//
// reportStats
//
void	LtLreIpClient::reportStats()
{
	vxlPrintf(
					//      1234567890     1234567890     1234567890     1234567890
				   "        Sent       %6d Received   %6d Pkts owned %6d\n"
				   "        Aggregated %6d Aggreg max %6d Stale pkt  %6d\n"
				   "        Max Qdepth %6d Rfc pkts   %6d Waitnbytes %6d\n"
				   "        PktsReorder%6d PktsLost   %6d AllocFail  %6d\n",
				   m_stats.m_nPktsSent, m_stats.m_nPktsReceived, m_qWaitingMsgs.getCount(),
				   m_stats.m_nPktsAggregated, m_stats.m_nPktsAggMax, m_stats.m_nPktsStale,
				   m_stats.m_nPktsMaxQueDpth, m_stats.m_nPktsRfcIn, m_nWaitingBytes,
				   m_stats.m_nPktsReordered, m_stats.m_nPktsLost, m_stats.m_nAllocFailures );
	vxlPrintf(
					//      1234567890     1234567890     1234567890     1234567890
				   "        IP->LT     %6d LT->IP     %6d\n",
				   m_stats.m_nPktsRouted, m_stats.m_nPktsProcessed );
	ULONG		nTickNow = tickGet();
	while ( m_stats.m_nLastTick != 0 )
	{
		ULONG	nSec = (nTickNow - m_stats.m_nLastTick) / sysClkRateGet();
		if ( nSec < 2 ) break;
		m_stats.getLast( nSec );
		vxlPrintf(
					//      1234567890     1234567890     1234567890     1234567890
				   "        Pkt/s send %6d Pkt/s recv %6d Seconds %9d\n",
				   m_stats.m_nLastPktSent, m_stats.m_nLastPktRecv, nSec );

		break;
	}
	m_stats.setLast( nTickNow );
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// LtLreIpMcastClient implementation
//

//
// constructor
//
LtLreIpMcastClient::LtLreIpMcastClient(LreClientType clientType, LtChannel *pChannel, LtIpBase* pMasterLtIpBase)
	: LtLreIpClient(clientType, pChannel, pMasterLtIpBase)
{
    m_ipMcastAddress = 0;
}

//
// destructor
//
LtLreIpMcastClient::~LtLreIpMcastClient()
{
	// master removes itself before doing this to avoid recursion
	if ( m_pMaster )
	{	
        m_pMaster->clientMcastBye( this );
	}
}

// end
