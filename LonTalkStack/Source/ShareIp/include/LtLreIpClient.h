#ifndef _LTLREIPCLIENT_H
#define _LTLREIPCLIENT_H 1
/***************************************************************
 *  Filename: LtLreIpClient.h
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
 *  Description:  Header file for LtLre IP client.
 *
 *	DJ Duffy Feb 1999
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/ShareIp/include/LtLreIpClient.h#2 $
//
/*
 * $Log: /Dev/ShareIp/include/LtLreIpClient.h $
 * 
 * 33    6/09/05 6:07p Fremont
 * EPRS FIXED: 37362 - IP-852 channel uses too much memory. Point all
 * packet allocations to the "master" object, and don't allocate any
 * buffers for the slaves.
 * 
 * 32    12/09/04 6:23p Fremont
 * Change conditional compilation for self installed multicast
 * 
 * 31    11/23/04 5:39p Fremont
 * EPRS FIXED: 35208 - Bombardier special - self-installed multicast IP
 * channel mode
 * 
 * 30    9/03/03 5:29p Fremont
 * change m_nWaitingBytes to signed int to be able to detect < 0
 * 
 * 29    6/04/03 8:04p Iphan
 * IKP06042003: Fixed statistics shown in console & ConfigServer
 * IKP06042003: Support for LT & LTIP statistics in System Info
 * 
 * 27    12/20/99 3:13p Darrelld
 * Fix clamping... Again
 * 
 * 26    12/20/99 11:45a Darrelld
 * Fix clamping
 * 
 * 25    12/20/99 11:22a Darrelld
 * Remove pktsOwned counter
 * 
 * 24    12/20/99 11:14a Darrelld
 * Clamp statistics
 * 
 * 23    12/15/99 3:09p Darrelld
 * EPRS FIXED:  15825 Fix reordering
 * 
 * 22    11/16/99 11:21a Darrelld
 * EPRS FIXED: 15662, 15433 BW limit causes too much delay.
 * Also fix aggregation to send first packet immediately to reduce delay.
 * 
 * 21    11/12/99 3:45p Darrelld
 * Updates for segment support
 * 
 * 20    11/01/99 11:46a Darrelld
 * Remove event server methods (now in base class)
 * 
 * 19    10/21/99 5:12p Darrelld
 * Protect against out of sequence session numbers
 * 
 * 18    10/07/99 12:17p Darrelld
 * Report authorization failures in statistics
 * 
 * 17    9/08/99 2:21p Darrelld
 * Session number is now a ULONG
 * 
 * 16    8/12/99 9:25a Darrelld
 * Add stats for process pkt and route pkt
 * 
 * 15    7/28/99 4:47p Darrelld
 * Fix packet reordering and starvation
 * 
 * 14    7/27/99 1:52p Darrelld
 * Fix packet reordering
 * 
 * 13    7/26/99 5:18p Darrelld
 * Debugging of IP router features
 * 
 * 12    7/20/99 5:26p Darrelld
 * Updates for reordering test
 * 
 * 11    6/30/99 4:41p Darrelld
 * Handle statistics properly
 * 
 * 10    5/07/99 11:05a Darrelld
 * Upgrade for RFC packet formats
 * 
 * 9     5/03/99 4:04p Darrelld
 * Add sending aggregated packets as they fill up when BW limiting is not
 * enabled.
 * 
 * 8     4/16/99 3:04p Darrelld
 * Aggregation and BW limiting works
 * 
 * 7     4/13/99 4:57p Darrelld
 * Enhance for Aggregation and BW Limiting and test
 * 
 * 6     3/30/99 5:10p Darrelld
 * intermediate checkin
 * 
 * 5     3/15/99 5:04p Darrelld
 * Intermediate checkin
 * 
 * 4     3/03/99 4:54p Darrelld
 * intermediate checkin
 * 
 * 3     2/22/99 10:58a Darrelld
 * Intermediate check-in
 * 
 * 2     2/09/99 5:05p Darrelld
 * Initial creation
 * 
 * 1     2/02/99 9:14a Darrelld
 * 
 * 
 */

//#include <LtRouter.h>
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// LreClientBase class
//
class LtLreServer;
class LtIpMaster;

#include <LtIpBase.h>
#include <LtIpPackets.h>
#include <LtPktReorderQue.h>
#include "SelfInstallMulticast.h"

// client stats
struct	LtLreIpStats
{
	UINT	m_nPktsSent;
	UINT	m_nBytesSent;
	UINT	m_nPktsReceived;
	UINT	m_nBytesReceived;
	UINT	m_nPktsMissed;
	UINT	m_nPktsDropped;
	UINT	m_nPktsInvalid;
	UINT	m_nPktsDuplicates;
	UINT	m_nPktsAggregated;
	UINT	m_nPktsStale;
	UINT	m_nPktsAggMax;
	UINT	m_nPktsRfcIn;
	UINT	m_nPktsMaxQueDpth;
	UINT	m_nPktsLost;
	UINT	m_nPktsReordered;
	UINT	m_nPktsRouted;			// pkts sent to engine
	UINT	m_nPktsProcessed;		// pkts received from engine
	UINT	m_nAllocFailures;		// allocation failures of msgrefs

	// IKP06042003: added support for calculating performance statistic correctly.
	UINT	m_nLastTick;
	UINT	m_nLastPktSent;
	UINT	m_nLastPktRecv;
	UINT	m_nLastBytesSent;
	UINT	m_nLastBytesRecv;

	void clear()
	{
		m_nLastTick = 0;
		m_nLastPktSent = 0;
		m_nLastPktRecv = 0;
		m_nLastBytesSent = 0;
		m_nLastBytesRecv = 0;

		m_nPktsSent = 0;
		m_nPktsReceived = 0;
		m_nBytesSent = 0;
		m_nBytesReceived = 0;
		m_nPktsMissed = 0;
		m_nPktsDropped = 0;
		m_nPktsInvalid = 0;
		m_nPktsDuplicates = 0;
		m_nPktsAggregated = 0;
		m_nPktsStale = 0;
		m_nPktsAggMax = 0;
		m_nPktsRfcIn = 0;
		m_nPktsMaxQueDpth = 0;
		m_nPktsLost = 0;
		m_nPktsReordered = 0;
		m_nPktsRouted	= 0;		// pkts sent to engine
		m_nPktsProcessed = 0;		// pkts received from engine
		m_nAllocFailures = 0;
	}
	void setLast( UINT nLT )
	{
		m_nLastTick = nLT;
		m_nLastPktSent = m_nPktsSent;
		m_nLastPktRecv = m_nPktsReceived;
		m_nLastBytesSent = m_nBytesSent;
		m_nLastBytesRecv = m_nBytesReceived;
	}

	void getLast( UINT nSec )
	{
		m_nLastPktSent = (m_nPktsSent - m_nLastPktSent)/nSec;
		m_nLastPktRecv = (m_nPktsReceived - m_nLastPktRecv)/nSec;
		m_nLastBytesSent = (m_nBytesSent - m_nLastBytesSent)/nSec;
		m_nLastBytesRecv = (m_nBytesReceived - m_nLastBytesRecv)/nSec;
	}



	//enum { COUNTMAX = 0xfffffffe };
#define COUNTMAX ((UINT)0xfffffffe)
	void	countClamp( UINT& cnt )
	{	if ( cnt != COUNTMAX ) cnt++;
	}
	UINT	addClamp( UINT& cnt, UINT add )
	{
		if ( cnt != COUNTMAX )
		{
			if ( ( cnt + add ) >= cnt ) cnt += add;
			else
				cnt = COUNTMAX;
			if ( cnt == (UINT)0xffffffff )
				cnt = COUNTMAX;
		}
		return cnt;
	}
	LtLreIpStats operator +( LtLreIpStats& s1 )
	{
		LtLreIpStats	sum;

		sum.clear();
		sum.m_nPktsSent = addClamp( m_nPktsSent, s1.m_nPktsSent );
		sum.m_nPktsReceived = addClamp( m_nPktsReceived, s1.m_nPktsReceived );
		sum.m_nBytesSent = addClamp( m_nBytesSent, s1.m_nBytesSent );
		sum.m_nBytesReceived = addClamp( m_nBytesReceived, s1.m_nBytesReceived );
		sum.m_nPktsMissed = addClamp( m_nPktsMissed, s1.m_nPktsMissed );
		sum.m_nPktsDropped = addClamp( m_nPktsDropped, s1.m_nPktsDropped );
		sum.m_nPktsInvalid = addClamp( m_nPktsInvalid, s1.m_nPktsInvalid );
		sum.m_nPktsDuplicates = addClamp( m_nPktsDuplicates, s1.m_nPktsDuplicates );
		sum.m_nPktsAggregated = addClamp( m_nPktsAggregated, s1.m_nPktsAggregated );
		sum.m_nPktsStale = addClamp( m_nPktsStale, s1.m_nPktsStale );
		sum.m_nPktsAggMax = MAX( m_nPktsAggMax, s1.m_nPktsStale );
		sum.m_nPktsRfcIn = addClamp( m_nPktsRfcIn, s1.m_nPktsRfcIn );
		sum.m_nPktsMaxQueDpth = MAX( m_nPktsMaxQueDpth, s1.m_nPktsMaxQueDpth );
		sum.m_nPktsLost = addClamp( m_nPktsLost, s1.m_nPktsLost );
		sum.m_nPktsReordered = addClamp( m_nPktsReordered, s1.m_nPktsReordered );
		sum.m_nPktsRouted = addClamp( m_nPktsRouted, s1.m_nPktsRouted );			// pkts sent to engine
		sum.m_nPktsProcessed = addClamp( m_nPktsProcessed, s1.m_nPktsProcessed );		// pkts received from engine
		sum.m_nAllocFailures = addClamp( m_nAllocFailures, s1.m_nAllocFailures );
		return sum;
	}
	LtLreIpStats operator+=( LtLreIpStats& s1 )
	{
		return operator+( s1 );
	}
};


class LtLreIpClient: public LtLreClient, public LtIpBase
{
protected:
	LtRouterType	m_routerType;
	LtLreServer*	m_pServer;
	LtIpMaster*		m_pMaster;

	byte*			m_ppktChanRoute;	// a pointer to a channel routing packet
	LtIpChanRouting	m_pktChanRoute;		// a descriptor for a channel routing packet

	ULONG			m_ipAddress;		// ip address of device to talk to
	word			m_ipPort;			// ip port to talk to 

	// RFC data packet control data
	ULONG			m_nSequence;		// increment for each packet sent.

	boolean			m_bSeenPacket;		// we have seen a packet from peer
	ULONG			m_nPeerSession;		// peer session number
	ULONG			m_nPeerSessionPrevious;	// previous peer session number
	ULONG			m_nPeerNextSeq;		// peer next sequence number

	LtPktReorderQue	m_qReorderQue;		// reorder queue of packets
	ULONG			m_ticksAtReorder;

	// reordering test data
	// these data affect packets before they are sent
	int				m_nTestReorderEvery;
	int				m_nTestReorderBy;
	LtPktInfo*		m_pTestReorderPkt;	// waiting packet
	ULONG			m_nTestSequence;	// test our sequence number

	// statistics
	LtLreIpStats	m_stats;

	LtQue			m_qWaitingMsgs;		// queue of waiting messages
	int				m_nWaitingBytes;	// bytes waiting in the queue
	UINT			m_nTickLastOneSend;	// when we last sent a single
										// packet.


public:
    LtLreIpClient(LreClientType clientType, LtChannel *pChannel, LtIpBase* pMasterLtIpBase);  
	virtual ~LtLreIpClient();

	virtual void			setRouterType( LtRouterType nRt )
	{	m_routerType = nRt;
	}
	virtual LtRouterType	getRouterType()
	{	return m_routerType;
	}
	// the master is the object that processes RFC messages and knows our
	// routes better than we do. So forward all LreServer requests for routing
	// data to the master.
	void		setMaster( LtIpMaster* pMaster )
	{	m_pMaster = pMaster;
	}
#if INCLUDE_SELF_INSTALLED_MULTICAST
	// Allow setting this to the multi-cast address -- for stats display only
	void setIpAddr(ULONG ipAddr) { m_ipAddress = ipAddr; }
#endif

	// The server or Lre or Router engine routes packets for a living
	// we forward all packets to her and she sends packets to us for sending out
	// to our "peer" router on the channel.
	// Just always believe what she says.
	// Also we don't send stuff, or expect to receive stuff, from her unless
	// she has called start. Until she says stop.
	virtual void		setServer( LtLreServer* pServer );
	// Set the routing information for this client
	// typically called by master when this object is locked
	// so that master can perform some actions atomically.
	// This object does not own the channel routing data and
	// does not destruct it, it just serves it up to the server
	// when asked.
	void		setRouting( byte* pPktChanRoute );
	// get the datetime from the current routing packet that we have
	ULONG		getRoutingDateTime()
	{	return m_pktChanRoute.dateTime;
	}


	// Lre will call us here to start and stop us
	virtual boolean		start();
	virtual boolean		stop();

	virtual void	reportStatus();
	virtual void	clearStats();
	virtual void	reportStats();
	virtual void	getStats( LtLreIpStats& st )
	{	st = m_stats;
	};

	//enum { COUNTMAX = 0xfffffffe };
	void	countClamp( UINT& cnt )
	{	if ( cnt != COUNTMAX ) cnt++;
	}
	UINT	addClamp( UINT& cnt, UINT add )
	{
		if ( cnt != COUNTMAX )
		{
			if  ( ( cnt + add ) >= cnt ) cnt += add;
			else	cnt = COUNTMAX;
			if ( cnt == (UINT)0xffffffff ) cnt = COUNTMAX;
		}
		return cnt;
	}

	// The following methods are queried by the LRE to get routing
	// configuration.  If the data returned by these methods changes,
	// the LRE must be notified via an event so that it can rebuild
	// its routing tables.

	// Used to get the routes associated with a client determined by
	// IP or LonTalk routing messages.  Function returns true if it succeeds, 
	// false if no more routes.  Index is expected to be 0..1 to cover
	// two domains.
	virtual boolean getRoute(int& index, LtRoutingMap *pRoute);

	// Function returns address specified by index, 0..N-1.  Returns true if
	// successful, else false.  For stack clients, index will typically be
	// 0 or 0..1 to allow one or two domains.  For IP clients, index might
	// be large since an IP device might have any number of stacks attached
	// to the IP channel.
	virtual boolean getAddress(int& index, LtDomain *d, LtSubnetNode *s, LtGroups *g);

	// Function returns the unique ID(s) associated with this client.  false means
	// there is none.
	virtual boolean getAddress(int& index, LtUniqueId *u);

	// Function used to specify an individual group address for a client.  More
	// efficient than above format for specifying individual group address clients.
	virtual boolean getAddress(int& index, LtDomain *d, int& group)
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
		return 0 != ( m_pktChanRoute.lonTalkFlags & LTROUTER_ALLBROADCAST ) ;
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

	// Normally, LtPktInfo is returned to allocator when released.  However,
	// client can override and do special behavior.  Return true if the message
	// was freed by the client.
	// IP Client requeues the packet to the link and returns true
	virtual boolean freeMessage(LtPktInfo* pPkt);

    void processPacket(boolean bPriority, LtPktInfo *pPkt);

	virtual void packetReceived( boolean bPriority, 
								LtPktInfo* pPkt, LtSts sts );

	// send more packets after being told to limit bandwidth by master
	boolean	sendMore( boolean bFullPktsOnly = false );
	boolean	waitingToSend()
	{	return !m_qWaitingMsgs.isEmpty();
	}
	// route packets waiting for reordering
	void	routeWaitingPackets( ULONG nEscrowTick );

protected:
	// send an RFC packet to the master for reply to the sender
	void	sendRFCtoMaster( byte* pData, int nSize, ULONG ipSrcAddr, word ipSrcPort );
	// discard stale packets waiting to be sent
	void	discardStaleWaiting();
	boolean aggregatePacket( LtPktInfo** ppPktAgg, LtPktInfo* pPkt );
	void	routePacket( boolean bPriority, LtIpPktHeader& phd, LtPktInfo* pPkt );
	void	authenticatePacket( LtPktInfo* pPkt );

};


class LtLreIpMcastClient: public LtLreIpClient
{
protected:
	ULONG			m_ipMcastAddress;	// ip multicast address 
public:
    LtLreIpMcastClient(LreClientType clientType, LtChannel *pChannel, LtIpBase* pMasterLtIpBase);  
    virtual ~LtLreIpMcastClient();

	void setIpMcastAddress(ULONG ipAddr) { m_ipMcastAddress = ipAddr; }

    boolean getNeedAllLayer2Packets()
    {
        return true;
    }
};

#endif // _LTLREIPCLIENT_H

