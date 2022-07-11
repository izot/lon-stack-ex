/***************************************************************
 *  Filename: Segment.cpp
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
 *  Description:  Implementation file for LonTalk IP Segment Packet formats.
 *
 *	DJ Duffy May 1999
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/ShareIp/Segment.cpp#1 $
//
/*
 * $Log: /Dev/ShareIp/Segment.cpp $
 *
 * 26    10/01/03 6:01p Fremont
 * EPR 30335 - final segment bit not set if last seg is full
 *
 * 25    9/26/03 5:01p Fremont
 * EPR 30215 - fix segmented packet size validation
 *
 * 24    9/19/03 3:05p Fremont
 * fixes for extended header and auth support, tweaked packet size
 * literals
 *
 * 23    9/03/03 5:28p Fremont
 * First pass at using extended packet headers. For EPR 29618. NOT
 * FINISHED!
 *
 * 22    6/10/03 5:35p Fremont
 * changes for EIA0852 auth
 *
 * 21    5/28/03 2:03p Iphan
 * IKP05202003: added checking for any pending request before sending new
 * request.
 * IKP05202003: delete any old pending requests before creating a new
 * request when receiving the first response segment.
 *
 * 20    5/16/03 3:33p Iphan
 * Modified TIMEOUT_RETRANS from 1 second to 900ms (slightly less than 1
 * second which is the upper layer timeout value.)
 *
 * 19    5/14/03 6:56p Iphan
 * EPRS FIXED:
 * IKP05132003: modify sixty to thirty seconds times out even if activity
 * IKP05122003: added checking against TIMEOUT_RETRANS before timing out
 * on segment inbound request and retransmiting the same request again
 *
 * 18    5/02/03 3:16p Iphan
 * clean up comments
 *
 * 17    5/02/03 3:13p Iphan
 * EPRS FIXED:
 * IKP05022003 - fixed null pointer reference caused by pPkt set to null
 * by allocPacket().
 *
 * 16    9/24/01 11:44a Fremont
 * comment out debug new and delete
 *
 * 15    5/30/00 2:01p Darrelld
 * Fix for stale requests
 *
 * 14    3/20/00 9:52a Darrelld
 * Add setIgnoreAuthentication
 *
 * 13    3/16/00 3:48p Darrelld
 * Fix debug stuff to allow building DLL
 *
 * 12    3/15/00 5:23p Darrelld
 * Segmentation integration
 *
 * 11    3/13/00 5:28p Darrelld
 * Segmentation work
 *
 * 10    2/25/00 5:45p Darrelld
 * Fixes for PCLIPS operation
 *
 * 9     2/23/00 9:08a Darrelld
 * LNS 3.0 Merge
 *
 * 7     12/10/99 4:04p Darrelld
 * MFC based segment base classes
 *
 * 6     11/18/99 10:57a Darrelld
 * Task priority macros
 *
 * 5     11/12/99 3:45p Darrelld
 * Updates for segment support
 *
 * 4     7/02/99 8:45a Darrelld
 * Target specific files
 *
 * 3     5/14/99 2:18p Darrelld
 * Complete testing of segment support
 *
 * 2     5/13/99 4:56p Darrelld
 * Segment packet support in progress
 *
 * 1     5/10/99 2:31p Darrelld
 * RFC Segment Packet processing
*/

// define SEGSUPPORTX to get definitions for Config server version
#ifdef SEGSUPPORTX
#include <windows.h>
#include <vxlTypes.h>
#include <LtPktInfoX.h>
#include <LtMD5.h>
#include <IpLinkX.h>


#else // SEGSUPPORTX
// include files appropriate to build with full master support
#include <vxWorks.h>
#include <wdLib.h>
#include <semLib.h>
#include <tickLib.h>
#include <LtRouter.h>
#include <LtMD5.h>
#include <IpLink.h>
#endif // SEGSUPPORTX

#include <vxlTarget.h>
#include <LtIpPackets.h>
#ifdef linux
#include "include/Segment.h"
#else
#include "Segment.h"
#endif

#if defined(_DEBUG) && !defined(LTA_EXPORTS)
// FB: this causes link problems if not linked with MFC
#if 0
void* __cdecl operator new(size_t nSize, char const* lpszFileName, int nLine);
void __cdecl operator delete(void* p, char const* lpszFileName, int nLine);
#define DEBUG_NEW new(THIS_FILE, __LINE__)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// LtIpSegment
//
// the segment packet itself
//

//
// parse
// BEWARE - This parse routine is used to parse packets that are not
// segment packets to get the datetime. Dont fail if there are
// problems with bogus packets.
//
boolean		LtIpSegment::parse( byte* pStart, boolean bRewrite )
{
	LtIpPktHeader::parse( pStart, bRewrite );
	byte*	p = pRemaining;
	payloadSize = 0;
	payload = NULL;
	PTOHL(p, dateTime );
	if ( packetType == PKTTYPE_SEGMENT )
	{
		PTOHB(p, flags );
		PTOHS(p, requestId );
		PTOHB(p, segmentId );
		bParsed = true;
		payload = p;
		payloadSize = (packetSize + pStart) - payload;
	}
	return (packetType == PKTTYPE_SEGMENT) && ( payloadSize >= 0 );
}

//
// build
//
byte*		LtIpSegment::build( byte* pStart )
{
	byte*			p = pStart;

	packetType = PKTTYPE_SEGMENT;
	p = LtIpPktHeader::build( p );
	PTONL(p, dateTime );
	PTONB(p, flags );
	PTONS(p, requestId );
	PTONB(p, segmentId );
	if ( payload && payloadSize )
	{
		memcpy( p, payload, payloadSize );
		p += payloadSize;
	}
	buildPacketSize( p );
	return p;
}

//
// dump
//
void LtIpSegment::dump( )
{
	LtIpDateTimeStr	dts;

	LtIpPktHeader::dump();
	vxlReportEvent( "LtIpSegment::dump - dateTime %s flags %d requestId %d segmentId %d\n"
					"                      parsed %d payloadAddr 0x%08x payloadSize %d \n",
					dts.getString(dateTime), flags, requestId, segmentId,
					bParsed, payload, payloadSize );
}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// LtIpSegmentor
//
// high level class maintains a list of segment requiests until they are satisfied
// or times them out.
//


LtIpSegmentor::LtIpSegmentor()
{
	m_pLink				= NULL;
	m_pAlloc			= NULL;
	m_nNextReqId		= 1;
	m_pSentPkt			= NULL;
	m_bActive			= false;
	m_bUseExtHdrs		= false;
}


LtIpSegmentor::~LtIpSegmentor()
{

	// No more link pointer for quicker shutdown
	m_pLink = NULL;
	m_bActive = false;
#ifdef TESTSEGS
	vxlReportEvent("~LtIpSegmentor - purgeRequests %d\n", tickGet() );
#endif // TESTSEGS
	// purge all the requests
	purgeAllRequests();

#ifdef TESTSEGS
	vxlReportEvent("~LtIpSegmentor - purge complete %d\n", tickGet() );
#endif // TESTSEGS
}

//
// purgeAllRequests
//
// remove all requests
//
void	LtIpSegmentor::purgeAllRequests( )
{
	LtVectorPos		pos;
	LtIpSegReq*		pReq;

	lock();
#ifdef TESTSEGS
	LtIpAddressStr	ias;
	int				nCount = m_vReqs.getCount();
	vxlReportEvent("LtIpSegmentor::purgeAllRequests - reqs total %d\n",	nCount );
#endif // TESTSEGS
	// remove and delete all the request elements, if there are any
	while ( m_vReqs.getElement( pos, &pReq ) )
	{
#ifdef TESTSEGS
		vxlReportEvent("LtIpSegmentor::purgeAllRequests - purge request 0x%08x reqId %d %s %d\n",
			pReq, pReq->getRequestId(), ias.getString( pReq->getIpAddr()), tickGet() );
#endif // TESTSEGS
		m_vReqs.removeElement( pos );
		delete pReq;
	}
#ifdef TESTSEGS
	nCount = m_vReqs.getCount();
	vxlReportEvent("LtIpSegmentor::purgeAllRequests - reqs left %d\n", nCount );
#endif // TESTSEGS
	unlock();
}


//
// purgeRequest
//
// remove all requests of a given type and destination.
// Probably because we are supercedeing the data.
// note if ipAddr is zero [ the default] then all requests matching type are
// removed.
//
void	LtIpSegmentor::purgeRequests( int nPktType, ULONG ipAddr, word port )
{
	LtVectorPos		pos;
	LtIpSegReq*		pReq;

	lock();
#ifdef TESTSEGS
	LtIpAddressStr	ias;
	int				nCount = m_vReqs.getCount();
	vxlReportEvent("LtIpSegmentor::purgeRequests - reqs total %d purge type %d %s (%d) %d\n",
					nCount, nPktType, ias.getString( ipAddr), port, tickGet() );
#endif // TESTSEGS
	// remove and delete all the matching request elements, if there are any
	while ( m_vReqs.getElement( pos, &pReq ) )
	{
		if ( pReq->matchTypeIp( nPktType, ipAddr, port ) )
		{
#ifdef TESTSEGS
			vxlReportEvent("LtIpSegmentor::purgeRequests - purge request 0x%08x reqId %d %s %d\n",
				pReq, pReq->getRequestId(), ias.getString( pReq->getIpAddr()), tickGet() );
#endif // TESTSEGS

			m_vReqs.removeElement( pos );
			delete pReq;
		}
	}
#ifdef TESTSEGS
	nCount = m_vReqs.getCount();
	vxlReportEvent("LtIpSegmentor::purgeRequests - reqs left %d\n", nCount );
#endif // TESTSEGS
	unlock();
}



//
// newRequest
//
// create a new request and put it in the list
// call with lock
//
LtIpSegReq*	LtIpSegmentor::newRequest( LtIpRequest& reqPkt, ULONG ipAddress, word port )
{
	if ( !isActive() )
	{	return NULL;
	}
	LtIpSegReq*		pReq = new LtIpSegReq();
	//STATUS			sts;
#ifdef TESTSEGS
	if ( reqPkt.packetType == 0 )
	{
		vxlReportEvent("LtIpSegmentor::newRequest - request type of zero\n" );
	}
#endif // TESTSEGS

	pReq->setRequest( reqPkt );
	pReq->setIpAddrPort( ipAddress, port );
	pReq->setExtHdrData(m_ipAddrLocal, m_natIpAddr, m_ipPortLocal, m_bUseExtHdrs);
	m_vReqs.addElement( pReq );
	// if we just added our first request, then start the timer
	if ( m_vReqs.getCount() == 1 )
	{
		startTimer( TIMEOUT_TIMERMS );
	}
	return pReq;
}


//
// buildSegments
//
// Build a request structure to send data to a destination using segments, if necessary.
//
boolean	LtIpSegmentor::buildSegments( LtIpRequest& reqPkt, byte* pData, int nDataSize,
									 ULONG ipAddr, word port, boolean bHasBall )
{
	boolean			bOk = false;
	boolean			bDone = false;
	LtVectorPos		pos;
	LtIpSegReq*		pReq;
	boolean			bSendAll = 0 != (reqPkt.reason & REQUEST_ALL);
	int				nSize = nDataSize;

	if ( nSize < LtIpSegment::MAX_PAYLOAD )
	{	return bOk;
	}

	lock();
	do
	{
		// If we have this request, then satisfy it from the existing request
		while ( m_vReqs.getElement( pos, &pReq ) )
		{
			if ( pReq->match( ipAddr, port, reqPkt.requestId ) )
			{
#if 0 // always rebuild a segReq if we have a match with request id
				if ( ! pReq->matchType( reqPkt.packetType ) )
				{
					// caller is playing games with us. Reuse of a request id
					// with another packet type.
					purgeRequests( pReq->getType(), ipAddr, port );
					// so create another request with this data.
					break;
				}
				bOk = sendSegment( pReq, reqPkt.segmentId, bSendAll );
				bDone = true;
#else
				// it might be new data this time, so rebuild the segments
				purgeRequests( pReq->getType(), ipAddr, port );
				// so create another request with this data.
#endif // always rebuild
				break;
			}
		}
		// we are done, so exit and unlock on way out
		if ( bDone )
		{	break;
		}

		pReq = newRequest( reqPkt, ipAddr, port );
		if ( pReq == NULL )
		{	bOk = false;
			break;
		}
		// only outbound requests come through here. Also
		// we have the ball passed in.
		pReq->setDirectionAndBall( true, bHasBall );
		// if not active, just bug out
	#ifdef TESTSEGS
		LtIpAddressStr	ias;
		vxlReportEvent("Segs::receivedPacket - new request 0x%08x reqId %d type %d %s (%d)\n",
			pReq, reqPkt.requestId, reqPkt.packetType, ias.getString( ipAddr ), port );
	#endif // TESTSEGS

		// always take a snapshot of the data
		bOk = pReq->buildSegments( pData, nSize, reqPkt.requestId, false );
		if ( bOk )
		{	// only send all if we are supposed to and we have
			// a link to absorb them, else we are testing and need to get one
			// at a time.
			if ( bSendAll && m_pLink )
			{
				bOk = sendSegment( pReq, reqPkt.segmentId, bSendAll );
			}
			else
			{
				bOk = sendSegment( pReq, reqPkt.segmentId, false );
			}
			// we started, it may not have finished
			// but it will later on.
			bOk = true;
		}
	} while ( false ); // error control
	unlock();
	return bOk;
}

//
// segmentRequest
//
// somebody requested a segment. So send it out, or a packet saying that no such item
// exists.
// If the request is not here at all, then they get nothing.
//
boolean	LtIpSegmentor::segmentRequest( LtIpRequest& reqPkt, ULONG ipAddress, word port )
{
	boolean			bOk = false;
	LtVectorPos		pos;
	LtIpSegReq*		pReq;
	LtIpSegment		seg;
	lock();
	do
	{
		if ( ipAddress == 0 || port == 0 )
		{	break;
		}
		while ( m_vReqs.getElement( pos, &pReq ) )
		{
			if ( pReq->match( ipAddress, port, reqPkt.requestId ) )
			{
				// make sure that the request type matches original request
				// else, invalid reuse of request id for a different packet type
				if ( pReq->matchType( reqPkt.packetType ) )
				{	bOk = true;
				}
				else
				{
					LtIpAddressStr	ias;
					vxlReportUrgent("Segment - invalid type / reqId match from %s : %d\n",
							ias.getString( ipAddress ), port );
				}
				break;
			}
		}
		if ( !bOk ) break;
		// if we are outbound and we still have the ball, then we can
		// drop it now, since the other side requested at least one segment from us
		if ( pReq->isOutbound() && pReq->hasBall() )
		{	pReq->dropBall();
		}
		bOk = sendSegment( pReq, reqPkt.segmentId, reqPkt.reason | REQUEST_ALL );
	} while (false );
	unlock();
	return bOk;
}

//
// dumpSegments
//
// dump out all the segments for a request.
//
boolean	LtIpSegmentor::dumpSegments( word requestId )
{
	boolean			bOk = false;
	LtVectorPos		pos;
	LtIpSegReq*		pReq;
	LtIpSegment		seg;
	int				nSegs;
	int				i;

	lock();
	do
	{
		while ( m_vReqs.getElement( pos, &pReq ) )
		{
			if ( pReq->getRequestId() == requestId )
			{	bOk = true;
				break;
			}
		}
		if ( !bOk ) break;
		nSegs = pReq->getNumSegments();
		for ( i=0; i<nSegs; i++ )
		{	bOk = pReq->getSegment( i, seg );
			if ( !bOk ) break;
			seg.dump();
		}

	} while (false );
	unlock();
	return bOk;
}

//
// dump
//
// report on status of this segmentor
//
void	LtIpSegmentor::dump( boolean bSegments )
{
	LtVectorPos		pos;
	LtIpSegReq*		pReq;

	lock();
	vxlReportEvent("LtIpSegmentor::dump - requests %d\n",
		m_vReqs.getCount() );
	while ( m_vReqs.getElement( pos, &pReq ) )
	{
		vxlReportEvent("                      reqId %d segments %d complete %d\n",
			pReq->getRequestId(), pReq->getNumSegments(), pReq->isComplete() );
		if ( bSegments )
		{	dumpSegments( pReq->getRequestId() );
		}
	}
	unlock();
}



//
// sendSegment
//
// send a segment to the link.
// Get the segment from the request. Get a packet, build the segment
// into the buffer, and send it on the link.
// allow sending all the remaining segments is request allows it
//
boolean	LtIpSegmentor::sendSegment( LtIpSegReq* pReq, int nSeg, boolean bRemaining )
{
	LtIpSegment		segPkt;
	byte			segBuf[ UDP_MAX_PKT_LEN +10 ];
	boolean			bOk = false;
	int				nSegs = bRemaining? pReq->getNumSegments()-nSeg : 1;
	int				i;

	do
	{
		for ( i = 0; i < nSegs; i++ )
		{
			pReq->getSegment( nSeg+i, segPkt );
			segPkt.build( segBuf );
			bOk = sendPacket( segBuf, segPkt.packetSize, pReq->getIpAddr(), pReq->getPort() );
		}
	} while (false );
	return bOk;
}

//
// sendPacket
//
// build and send a packet via the link.
boolean	LtIpSegmentor::sendPacket( byte* pData, int nLen, ULONG ipAddr, word port )
{
	LtPktInfo*		pPkt;
	LtSts			sts;
	int				nBlkSize = 0;			// IKP05022003: initialized to zero
	int				nAuthSize = 0;
	boolean			bOk = false;

	do
	{
		// allow allocator and link to be optional for ease of unit testing
		if ( m_pAlloc )
		{	pPkt = m_pAlloc->allocPacket();
			if (pPkt != null)			// IKP05022003: added checking to make sure pPkt is not null
				nBlkSize = pPkt->getBlockSize();
		}
		else
		{	pPkt = new LtPktInfo();
			if (pPkt != null)			// IKP05022003: added checking to make sure pPkt is not null
			{
				pPkt->init(true);
				byte*	p = (byte*)malloc( UDP_MAX_PKT_LEN );
				pPkt->setBlock( p );
				nBlkSize = UDP_MAX_PKT_LEN;
			}
		}

		if ( pPkt == NULL ) break;

		if ( nLen > nBlkSize )
		{
			vxlReportUrgent("Segment too long for buffer\n");
			pPkt->release();
			break;
		}
		// build the authentication digest if required
		if ( m_md5.isAuthenticating() )
		{
			LtIpPktBase pktBase;

			if ( nLen > ( UDP_MAX_PKT_LEN - LtMD5::LTMD5_DIGEST_LEN ) )
			{
				vxlReportUrgent("Segment too long to authenticate\n");
				pPkt->release();
				break;	// packet too long to authenticate
			}
			// Required by EIA852. Do before digesting; it changes the header
			if (m_md5.isEIA852Auth())
				pktBase.markSecure(pData);
			m_md5.digestWithSecret( pData, nLen, &pData[ nLen] );
			nAuthSize = LtMD5::LTMD5_DIGEST_LEN;
		}
		// Copy the data the allocated block
		memcpy( pPkt->getBlock(), pData, nLen+nAuthSize );

		pPkt->setMessageData( pPkt->getBlock(), nLen+nAuthSize, pPkt );
		if ( m_pLink )
		{
			sts = m_pLink->sendPacketTo( pPkt, ipAddr, port,
									pPkt->getDataPtr(), pPkt->getDataSize() );
		}
		else
		{
			// save the packet for pickup by testor later
			// toss the previous packet out
			if ( m_pSentPkt )
			{	m_pSentPkt->release();
			}
			m_pSentPkt = pPkt;
			sts = LTSTS_OK;
		}
		if ( sts != LTSTS_OK )
		{	pPkt->release();
			break;
		}
		bOk = true;
	} while (false );
	return bOk;
}



//
// findRequest
//
// find a request that matches everything
//
LtIpSegReq* LtIpSegmentor::findRequest( int nPktType, ULONG ipAddress, word port )
{
	boolean			bOk = false;
	LtVectorPos		pos;
	LtIpSegReq*		pReq;

	// prelock required
	//lock();
	while ( m_vReqs.getElement( pos, &pReq ) )
	{
		if ( pReq->matchTypeIp( nPktType, ipAddress, port ) )
		{	bOk = true;
			break;
		}
	}
	//unlock();
	if ( !bOk ) pReq = NULL;
	return pReq;
}

//
// IKP05202003: hasPendingRequest
//
// Check if there is already pending request handled by the segmentor.
// This routine does not care about the pending request Id.
// Make sure we lock and unlock before and after finding the request.
//
boolean	LtIpSegmentor::hasPendingRequest( int nPktType, ULONG ipAddr, word port )
{
	boolean	bFound = false;
	LtVectorPos		pos;
	LtIpSegReq*		pReq;

	lock();
	while ( m_vReqs.getElement( pos, &pReq ) )
	{
		if ( pReq->matchTypeIp( nPktType, ipAddr, port ) )
		{
			// found a request matching the type, IP & port
			if (! pReq->isPayloadDelivered())
			{
				// this request is still active and not delivered
				bFound = true;
				break;
			}
		}
	}
	unlock();
	return bFound;
}


//
// newInboundRequest
//
// caller fills in all the fields of the req packet that are required.
// Thank proper design that all the request packets have all the same fields.
// This allows us to build and send the request packet out for the caller.
// create a request for inbound packets. We build a request packet to use as the request.
//
boolean	LtIpSegmentor::newInboundRequest( LtIpRequest& req, ULONG ipAddress, word port )
{
	LtIpSegReq*		pReq;	// request control object
	boolean			bOk = true;
	byte			buf[ LtIpRequest::MAX_PKT_SIZE + LtMD5::LTMD5_DIGEST_LEN +2];
	int				nAuthSize = 0;
	int				nSize = 0;

	lock();
	do
	{
		// user has filled in any other fields that are required
		pReq = findRequest( req.packetType, ipAddress, port );
		if ( pReq == NULL )
		{
			pReq = newRequest( req, ipAddress, port );
			// if not active, don't create new requests
			if ( pReq == NULL )
			{	bOk = false;
				break;
			}
		}
		req.dateTime	= req.getDateTime();
		req.requestId	= pReq->getRequestId();
		req.reason		= REQUEST_ALL;	// we can handle a stream of responses coming in
		// build the request packet byte stream
		// if we are authenticating, then build the authentication code
		// then send the packet.
		req.build( buf );
		nSize = req.packetSize;
		if ( m_md5.isAuthenticating() )
		{
			// Required by EIA852. Do before digesting; it changes the header
			if (m_md5.isEIA852Auth())
				req.markSecure(buf);
			m_md5.digestWithSecret( buf, nSize, &buf[nSize] );
			nAuthSize = LtMD5::LTMD5_DIGEST_LEN;
		}

		sendPacket( buf, nSize + nAuthSize, ipAddress, port );

	} while (false );
	unlock();
	return bOk;
}


//
// receivedPacket
//
// return false if no request matches.
// Create a new inbound request to allow unsolicited packets to arrive.
//
// return true if we have a request that matches.
// return  ppPktRtn not null as "packet" with an attached payload if a request completes.
//
// Always dispose of pPkt. Caller always knows pPkt is gone.
//
boolean	LtIpSegmentor::receivedPacket( LtPktInfo* pPkt, LtPktInfo** ppPktRtn,
									  ULONG ipAddr, word port )
{
	boolean		bOk = false;
	lock();

	LtIpSegment		seg;
	byte*			p;
	int				nSize;
	LtIpSegReq*		pReq;
	LtPktInfo*		pPkt2;
	boolean			bDone = false;
	boolean			bFound = false;
	boolean			bRelease = true;

	p = pPkt->getDataPtr();
	nSize = pPkt->getDataSize();
	*ppPktRtn = NULL;
	do
	{	// if not active, then just release packet, and bug out
		LtIpPktHeader	pktHdr;
		int				nReqType = 0;

		if ( !isActive() )
		{	break;
		}
		bOk = seg.parse( p, false );
		if ( ! bOk )
		{	break;
		}

		// IKP05202003: moved packet parsing here so we can recreate the original request
		// If this is the 1st segment, parse the payload and find out the request type
		if ( seg.segmentId == 0 )
		{
			if (!pktHdr.parse( seg.payload, false ) )
				{	break;	// parse failure on packet header
				}

			switch ( pktHdr.packetType )
			{
				case PKTTYPE_CHNMEMBERS	:
					nReqType = PKTTYPE_REQCHNMEMBERS;
					break;
				case PKTTYPE_CHNROUTING	:
					nReqType = PKTTYPE_REQCHNROUTING;
					break;
				default:
					break;
			}

			if ( nReqType == 0 )
			{
				vxlReportUrgent("LtIpSegmentor::receivedPacket - invalid packet to segment %d\n",
								pktHdr.packetType );
				break;	// invalid packet to segment
			}
		}

		// remove and delete all the request elements, if there are any
		bOk = false;	// default is create a new request and enter the packet
		LtVectorPos		pos;
		while ( m_vReqs.getElement( pos, &pReq ) )
		{
			if ( pReq->match( ipAddr, port, seg.requestId ) )
			{	// we found a matching request, default to toss it out
				// then check with request.
				bOk = pReq->segmentArrived( pPkt, seg );
				if ( bOk )
				{	bRelease = false;
					bDone = true;
				}
				bFound = true; // we found a request, so don't create a new one
				break;
			}
			else if ( seg.segmentId == 0 )	// IKP05202003: removing any old pending reqs
			{
				// only do this checking if we receive a new segment
				if (pReq->matchTypeIp(nReqType, ipAddr, port))
				{
					// found a request matching the type, IP & port
					if (! pReq->isPayloadDelivered())
					{
						// remove from the list, since we are creating a new request for it.
						m_vReqs.removeElement( pos );
						delete pReq;
					}
				}
			}
		}

		// IKP05202003: can't create the packet type if the first segment
		// we got is not segment zero. Bug out and release the packet.
		if ( !bFound && seg.segmentId == 0 )
		{	// unsolicited segmented packet Handle this case gracefully
			// create a new inbound request with the right request id, ipAddr and port
			// and wait for more segment packets. Type will not be correct. It's ok
			LtIpRequest		reqPkt;

			reqPkt.packetType = nReqType;
			reqPkt.requestId = seg.requestId;
			pReq = newRequest( reqPkt, ipAddr, port );
#ifdef TESTSEGS
			LtIpAddressStr	ias;
			vxlReportEvent("Segs::receivedPacket - new request 0x%08x reqId %d type %d %s (%d)\n",
				pReq, reqPkt.requestId, reqPkt.packetType, ias.getString( ipAddr ), port );
#endif // TESTSEGS
			bOk = pReq->segmentArrived( pPkt, seg );
			if ( bOk )
			{	bRelease = false;
				bDone = true;
				break;
			}
		}

		// we found a request that wanted the packet
		// now did this packet complete that request?
		if ( bDone )
		{
			if ( pReq->isComplete() )
			{
				p = pReq->getPayload();
				nSize = pReq->getPayloadSize();
				pReq->takePayload();
				pPkt2 = new LtPktInfo();
				pPkt2->init(true);
				pPkt2->setBlock( p );
				pPkt2->setMessageData( p, nSize, pPkt2 );
				// we do not authenticate payloads
				pPkt2->setIgnoreAuthentication( true );
				*ppPktRtn = pPkt2;
				// do not delete the request, but it is marked as having been satisfied
			}
		}

	} while ( false );

	// maybe dispose of this packet
	// packet is always gone as far as caller is concerned
	if ( bRelease )
	{	pPkt->release();
	}
	unlock();
	return bOk;
}


//
// Timeout
//
// Discard stale requests
// called by timer task with the ticks.
// return when complete. Loop is in base class.
//
boolean LtIpSegmentor::doTimeout( ULONG nTickNow )
{
	LtVectorPos		pos;
	LtIpSegReq*		pReq;
	LtIpSegment		segPkt;
	LtIpRequest		reqPkt;
	byte			segBuf[ UDP_MAX_PKT_LEN + 10 ];
	boolean			bOk;
#ifdef TESTSEGS
	LtIpAddressStr ias;
#endif // TESTSEGS
	// if not active, then just bug out and stop timer
	if ( !isActive() )
	{
#ifdef TESTSEGS
		int	nCount = m_vReqs.getCount();
		if ( nCount )
		{
			vxlReportEvent("LtIpSegmentor::doTimeout - %d reqs left inactive %d\n",
				nCount, tickGet() );
		}
		else
#endif // TESTSEGS
		return false;
	}
	// if we don't have a link pointer, then we don't have
	// anything to do. Part of clean shutdown.
	if ( m_pLink == NULL )
	{
#ifdef TESTSEGS
		vxlReportEvent("LtIpSegmentor::doTimeout - Link Null exit %d\n", tickGet() );
#endif // TESTSEGS
		return false;
	}

	lock();
	while ( m_vReqs.getElement( pos, &pReq ) )
	{
		if ( pReq->isTimedOut( nTickNow ) )
		{
#ifdef TESTSEGS
			vxlReportEvent("Segments::doTimeout - request timed out - 0x%08x reqId %d %s\n",
						pReq, pReq->getRequestId(), ias.getString( pReq->getIpAddr()) );
#endif // TESTSEGS
			m_vReqs.removeElement( pos );
			delete pReq;
		}
		else if ( pReq->isOutbound() )
		{
			// if we are outbound then
			// if we have the ball, then we need to send segment
			// zero again. This is in case the other guy didn't hear us the first time.
			// When at least one segment arrives, then there will be either a request
			// for lost segments, or an ACK for the entire request. If an Ack for the
			// the request occurs, then the request is purged from here and we wouldn't
			// be talking about doing anything.
			if ( pReq->getOutboundSegment( nTickNow, segPkt ) )
			{
#ifdef TESTSEGS
			vxlReportEvent("Segments::doTimeout - send segment - reqId %d seg %d %s\n",
						pReq->getRequestId(), segPkt.segmentId, ias.getString( pReq->getIpAddr()) );
#endif // TESTSEGS
				segPkt.build( segBuf );
				bOk = sendPacket( segBuf, segPkt.packetSize,
									pReq->getIpAddr(), pReq->getPort() );
			}
			// if we don't have the ball, or it's not time yet, do nothing.
		}
		else
		{	// If we are inbound, then we need to transmit a segment request for the
			// first segment we don't have. And always set "SEND ALL".
			// If we are inbound, then we always have the ball.
			// So if the time is right, then build a request and send it
			if ( pReq->getInboundRequest( nTickNow, reqPkt ) )
			{
#ifdef TESTSEGS
			vxlReportEvent("Segments::doTimeout - request segment - reqId %d seg %d %s\n",
						pReq->getRequestId(), reqPkt.segmentId, ias.getString( pReq->getIpAddr()) );
#endif // TESTSEGS
				reqPkt.build( segBuf );
				bOk = sendPacket( segBuf, reqPkt.packetSize,
									pReq->getIpAddr(), pReq->getPort() );
				if (bOk)
				{
					// IKP05122003: update the tickSent since we've just sent out a request
					pReq->updateSentTimer();	// assume current tick count for starting request process
				}
			}
		}
	}
	boolean bActive =  0 != m_vReqs.getCount();
	unlock();
	return bActive;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Segment Request class
//
// This class maintains a list of segment descriptors to process incoming
// or outgoing segments.
//


LtIpSegReq::LtIpSegReq() : m_qSegs(true)
{
	m_nReqId			= 0;
	m_bIpFromPkt		= false;
	m_ipAddr			= 0;
	m_port				= 0;
	m_bHasBall			= false;
	m_bOutbound			= false;
	m_nSegments			= 0;			// number of segments in payload
	m_tickStarted		= tickGet();	// tick count for starting process
	m_tickLast			= m_tickStarted; // tick count last time we received message
	m_tickSent			= 0;
	m_nHighestSent		= -1;			// highest segment sent - with a sendall
	m_dateTime			= 0;			// datetime of packet stream
	m_bComplete			= false;		// received packet is complete
	m_bPayloadDelivered	= false;		// payload has not been delivered to higher layers
	m_pPayload			= NULL;			// assembled payload
	m_nPayloadSize		= 0;			// size of assembled payload
	m_bWeOwnPayload		= false;		// if we own it, then we can free it
	m_bUseExtHdrs		= false;		// use extended packet headers
}

//
// freePayload
//
// discard the payload if we own it, then free it
//
void	LtIpSegReq::freePayload()
{
	if ( m_bWeOwnPayload && m_pPayload )
	{	free( m_pPayload );
	}
	m_bWeOwnPayload = false;
	m_pPayload = NULL;
	m_nPayloadSize = 0;
}

//
// emptyQueue
//
// release all waiting segments
//
void	LtIpSegReq::emptyQueue()
{
	LtQue*		pItem;
	LtPktInfo*	pPkt;

	while ( m_qSegs.removeHead( & pItem ) )
	{
		pPkt = (LtPktInfo*) pItem;
		pPkt->release();
	}
}


//
// destructor
//
LtIpSegReq::~LtIpSegReq()
{
	freePayload();
	emptyQueue();
}

//
// zapSegs
//
// zap all the segments and release all the packets.
//
void	LtIpSegReq::zapSegs()
{
	int			i;

	for ( i=0; i< MAX_SEGMENTS; i++ )
	{	m_aSegs[i].zap();
	}
	emptyQueue();
	freePayload();
	m_nHighestSent = -1;
	m_bComplete = false;
	m_bPayloadDelivered = false;
}


//
// segmentArrived
//
// for received packets, pass a segment in. Get a true if packet returned and
// it should be released.
//
boolean		LtIpSegReq::segmentArrived( LtPktInfo* pPkt, LtIpSegment& segPkt )
{
	boolean	bOk = false;
	do
	{
		if ( m_bPayloadDelivered ||					// we have delivered this payload before?
			!segPkt.isValid() ||
			 ( m_nSegments != 0  && (segPkt.requestId != m_nReqId ) ) ||
			 segPkt.segmentId >= MAX_SEGMENTS
			 )
		{	break;
		}
		// remember that we got a segment, so some activity
		// Either in base class or in vxWorks. - BEWARE
		m_tickLast = tickGet();
		if ( m_nSegments == 0 )
		{	m_nReqId = segPkt.requestId;
			m_dateTime = segPkt.dateTime;
		}

		if ( segPkt.dateTime > m_dateTime )
		{	m_dateTime = segPkt.dateTime;
			zapSegs();
		}
		else if ( segPkt.dateTime < m_dateTime )
		{	break;
		}
		// reject duplicate packets now that we know that we don't have
		// a date change.
		if ( m_aSegs[segPkt.segmentId].isValid() )
		{	break;
		}
		// save one copy of each segment header
		// and one copy of each segment packet in the list.
		// segment packet structure stores pointer to payload bytes in
		// packet buffer.
		m_aSegs[segPkt.segmentId] = segPkt;
		m_qSegs.insertTail( pPkt );
		m_nSegments++;

		// Check to see if all segments are in so we have a complete payload.
		checkComplete();
		// important safety tip.
		// Note that the completed packet stays around until it times out.
		// This is always done even though we are long done with it.
		// This way when packets come in for this request, we don't process the
		// duplicate packets.
		bOk = true;
	} while ( false );

	return bOk;
}

//
// checkComplete
//
// Scan the segments and see if we have them all.
// then assemble a complete payload as a contiguous memory block
//
boolean	LtIpSegReq::checkComplete()
{
	int				i;
	boolean			bOk = false;
	boolean			bBad = false;
	LtIpPktHeader	pkt;

	do
	{
		if ( ! m_aSegs[m_nSegments-1].isFinal() )
		{	break;
		}
		for ( i=0; i< m_nSegments; i++ )
		{
			if ( !m_aSegs[i].isValid() )
			{	bBad = true;
				break;
			}
		}
		if ( bBad )
		{	break;
		}
		bOk = true;
	} while ( false );

	while ( bOk )
	{
		// we have all the segments now, so assemble the payload
		// First parse the first payload so that we have the size of the
		// whole payload packet
		freePayload();
		pkt.parse( m_aSegs[0].payload );
		m_nPayloadSize = pkt.packetSize;
		if ( m_nPayloadSize < 0 || m_nPayloadSize > MAX_TOTAL_SEG_PAYLOAD )
		{	bOk = false;
			m_nPayloadSize = 0;
			break;
		}
		m_bWeOwnPayload = true;
		m_pPayload = (byte*)malloc( m_nPayloadSize );
		byte*	p = m_pPayload;
		byte*	pend = p + m_nPayloadSize;
		for ( i=0; i< m_nSegments; i++ )
		{
			// watch for invalid payload packet and other errors
			// that cause too much packet data
			if ( ( p+m_aSegs[i].payloadSize ) > pend )
			{	bOk = false;
				// serious error
				// maybe somebody trying to hack us or crash us
				// so cause a restart from begining.
				// Transfer will eventually timeout.
				zapSegs();
				break;
			}
			memcpy( p, m_aSegs[i].payload, m_aSegs[i].payloadSize );
			p += m_aSegs[i].payloadSize;
		}
		break;	// while is an error exit loop. So exit here.
	}
	m_bComplete = bOk;
	return bOk;
}


//
// isTimedOut
//
boolean		LtIpSegReq::isTimedOut( ULONG nTicksNow )
{
	boolean	bOut = false;
	ULONG		nQuietTime = nTicksNow - m_tickLast;
	ULONG		nSinceStart = nTicksNow - m_tickStarted;

	bOut = ( nQuietTime > (ULONG)msToTicksX( TIMEOUT_QUIET ) ) ||
			( nSinceStart > (ULONG)msToTicksX( TIMEOUT_BUSY ) );

	return bOut;
}

//
// buildSegments
//
// build segments to send out
//
boolean		LtIpSegReq::buildSegments( byte* pData, int nDataSize,
									  word reqId, boolean bOwn )
{
	boolean			bOk = false;
	LtIpSegment		seg;
	LtIpSegment		segX;
	int				nSeg;
	int				offset;
	int				nSize = nDataSize;

	// no matter what happens, the old payload is gone
	freePayload();
	do
	{
		if ((nSize < LtIpSegment::MAX_PAYLOAD) ||
			(nSize > (LtIpSegment::MAX_PAYLOAD*MAX_SEGMENTS)))
		{	break;
		}
		if ( bOwn )
		{
			m_pPayload = pData;
			m_nPayloadSize = nDataSize;
		}
		else
		{
			m_pPayload = (byte*)malloc( nSize + LtMD5::LTMD5_DIGEST_LEN );
			if ( m_pPayload == NULL )
			{	break;
			}
			m_nPayloadSize = nSize;
			memcpy( m_pPayload, pData, nSize );
			// if caller said we own it, but passed in a digest, then we need to free
			// after we put the parts together.
			if ( bOwn )
			{	::free( pData );
			}
		}
		m_bWeOwnPayload = true;
		m_nReqId = reqId;
		// remember all timeing starts here
		m_tickStarted = m_tickLast = m_tickSent = tickGet();
		// we are outbound, but don't change the ball.
		setDirectionAndBall( true, hasBall() );
		// parse as segment just to get the datetime
		segX.parse( pData, false );
		m_dateTime = segX.dateTime;
		nSeg = 0;
		for (offset = 0; offset < nSize; offset+=SEG_SIZE )
		{
			seg.packetType		= PKTTYPE_SEGMENT;
			// filled in by constructor
			//seg.sequence		= 0;
			//seg.timestamp		= seg.getTimestamp();
			//seg.session		= seg.getSession();
			seg.dateTime		= m_dateTime;
			seg.flags			= ((int)LtIpSegment::FLAG_VALID) |
							((int)((nSize-offset) <= SEG_SIZE)? LtIpSegment::FLAG_FINAL : 0 );
			seg.requestId		= m_nReqId;
			seg.segmentId		= nSeg;
			seg.payload			= m_pPayload + offset;
			seg.payloadSize		= min( SEG_SIZE, (nSize-offset) );
			setPktExtHdrData(&seg);

			m_aSegs[nSeg ]		= seg;
			nSeg++;
		}
		m_nSegments = nSeg;
		bOk = true;
	} while ( false );

	return bOk;
}

//
// getSegment
//
// get one of the segments we built
//
boolean		LtIpSegReq::getSegment( int nSegment, LtIpSegment& segPkt )
{
	boolean	bOk = false;
	LtIpSegment		badSeg;

	if ( m_pPayload && nSegment >= 0 && nSegment < m_nSegments )
	{
		segPkt = m_aSegs[nSegment];
		bOk = true;
		m_tickLast = tickGet();
		setHighestSent( nSegment );
	}
	else
	{
		// if invalid segment, then build a segment packet that says so
		badSeg.dateTime		= getDatetime();
		badSeg.requestId	= m_nReqId;
		badSeg.segmentId	= nSegment;
		badSeg.flags		= LtIpSegment::FLAG_FINAL;	// no valid bit
		segPkt				= badSeg;
	}

	return bOk;
}

//
// getOutboundSegment
//
// get segment zero for retransmission if we are outbound and have the ball
//
boolean LtIpSegReq::getOutboundSegment( ULONG nTickNow, LtIpSegment& segPkt )
{
	boolean		bOk = false;
	ULONG		nSinceSent = nTickNow - m_tickSent;

	// if we have the ball and it's been long enough, then send a segment
	// always segment zero
	if ( m_bHasBall && nSinceSent > TIMEOUT_RETRANS )
	{
		bOk = getSegment( 0, segPkt );
	}
	return bOk;
}

//
// getInboundRequest
//
//
boolean LtIpSegReq::getInboundRequest( ULONG nTickNow, LtIpRequest& reqPkt )
{
	int				i;
	boolean			bOk = false;
	int				nRequest = -1;
	LtIpPktHeader	pkt;

	do
	{	// if we are already complete, don't mess around
		if ( isComplete() )
		{	break;
		}
		else if (hasReqRetransmissionTimerTimedOut( nTickNow ))
		{
			// IKP05122003: TIMEOUT_RETRANS since the start of this segment processing
			// scan and look for any missing segments
			for ( i=0; i< m_nSegments; i++ )
			{
				if ( !m_aSegs[i].isValid() )
				{	nRequest = i;
					break;
				}
			}
			// If there are no gaps in list, and the last segment has
			// final bit set, we are all done, so outta here
			if ( nRequest == -1 && m_aSegs[m_nSegments-1].isFinal() )
			{	break;
			}
			// if there are no gaps, then request the next segment
			if ( nRequest == -1 )
			{	nRequest = m_nSegments;
			}
			// fill in data using original request where necessary
			reqPkt.packetType	= m_orgRequest.packetType;
			reqPkt.dateTime		= m_dateTime;
			reqPkt.reason		= REQUEST_ALL;
			reqPkt.requestId	= m_nReqId;
			reqPkt.segmentId	= nRequest;
			reqPkt.ipUcAddress	= m_orgRequest.ipUcAddress;
			reqPkt.sinceDateTime = m_orgRequest.sinceDateTime;
			bOk = true;
		}
	} while ( false );

	return bOk;
}
// end
