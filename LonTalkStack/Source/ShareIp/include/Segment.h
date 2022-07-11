#ifndef _LTIPSEGMENT_H
#define _LTIPSEGMENT_H

/***************************************************************
 *  Filename: Segment.h
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
 *  Description:  Header file for LonTalk IP Segment Packet formats.
 *
 *	DJ Duffy May 1999
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/ShareIp/include/Segment.h#1 $
//
/*
 * $Log: /Dev/ShareIp/include/Segment.h $
 *
 * 18    9/26/03 4:59p Fremont
 * EPR 30215 - reduce max seg payload to allow for max overhead
 *
 * 17    9/19/03 3:04p Fremont
 * add functions for authentication and extended header support, tweak
 * literals for UDP packet size and segment size
 *
 * 16    9/03/03 5:28p Fremont
 * First pass at using extended packet headers. For EPR 29618. NOT
 * FINISHED!
 *
 * 15    6/25/03 3:41p Fremont
 * fix segment pkt size virtual functions
 *
 * 14    6/10/03 5:36p Fremont
 * changes for EIA0852 auth
 *
 * 13    5/28/03 2:09p Iphan
 * IKP05202003: added support for checking any pending request before
 * sending new request.
 *
 * 12    5/16/03 3:32p Iphan
 * Modified TIMEOUT_RETRANS from 1 second to 900ms (slightly less than 1
 * second which is the upper layer timeout value.)
 *
 * 11    5/14/03 6:55p Iphan
 * EPRS FIXED:
 * IKP05132003: modify sixty to thirty seconds times out even if activity
 * IKP05122003: added checking against TIMEOUT_RETRANS before timing out
 * on segment receive and retransmiting the same request again
 *
 * 10    3/13/00 5:28p Darrelld
 * Segmentation work
 *
 * 9     2/29/00 4:51p Darrelld
 * Segmentation fixes
 *
 * 8     2/25/00 5:46p Darrelld
 * Fixes for PCLIPS operation
 *
 * 6     12/10/99 4:04p Darrelld
 * MFC based segment base classes
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
 * 1     5/10/99 2:25p Darrelld
 * RFC Segment Packet header file
*/

// Get the base class for the segmentor class
// Define SEGSUPPORTX for the config server version
#ifdef SEGSUPPORTX
#include <SegSupportX.h>
#else
#include <SegSupport.h>
#endif // SEGSUPPORTX
#include <LtMD5.h>

extern "C" ULONG tickGet();

// Max size of a UPD packet payload .
// This is not the total UPD packet size, as it does not include the IP header.
// NOTE: to debug segmentation, it is possible to decrease this size so that
// packets will segment more quickly. A reasonable number is 200.
// Only do this temporarily!
// You must also change LtIpSeg::MAX_PAYLOAD (below) -- see comments there.
#define UDP_MAX_PKT_LEN 548


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// LtIpSegment
//
// the segment packet itself
//
class LtIpSegment : public LtIpPktBase
{
public:
	ULONG	dateTime;				// datetime of packet inside
	byte	flags;					// Valid or final
	word	requestId;				// Request id from sender
	byte	segmentId;				// Segment number 0 - n

	boolean	bParsed;				// segment has been parsed
	byte*	payload;				// pointer to payload of this segment
	int		payloadSize;			// size of payload for this segment

	enum {
		FLAG_VALID		= 0x80,		// this segment is valid
		FLAG_FINAL		= 0x40,		// this is last segment of packet
		ADDED_SIZE		= sizeof(ULONG)+sizeof(word)+(2*sizeof(byte)),
// NOTE: to debug segmentation, it is possible to decrease this size so that
// packets will segment more quickly. A reasonable number is 100.
// Only do this temporarily!
// You must also change UDP_MAX_PKT_LEN (above) -- see comments there.
// The normal value is calculated as: maxUdp - (pktHdr + extHdr + segPktData + authDigest)
// The max overhead is (20 + 12 + 8 + 16) = 56, giving 548 - 59 = 492 max payload
		MAX_PAYLOAD		= 492,		// size of the maximum payload size
		JUNK			= 0
	};

	LtIpSegment()
	{	zap();
	}

	void	zap()
	{
		packetType = PKTTYPE_SEGMENT;
		dateTime = 0;
		flags = 0;
		requestId = 0;
		segmentId = 0;
		bParsed = false;
		payload = NULL;
		payloadSize = 0;
	}
	virtual				~LtIpSegment() {};
	boolean				isParsed()
	{	return bParsed;
	}
	boolean				isFinal()
	{	return 0 != (flags & FLAG_FINAL);
	}
	boolean				isValid()
	{	return LtIpPktHeader::isValid() && ( 0!= (flags & FLAG_VALID) );
	}
	virtual boolean		parse( byte* pStart, boolean bRewrite = true );
	virtual byte*		build( byte* pStart );
	virtual int commonFixedSize() { return(hdrSize() + ADDED_SIZE); }
	virtual int commonDefinedSize()
		{ return  commonFixedSize()+payloadSize; }

	int	size() { return  definedPktSize(); }
	void				dump( );

};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Segment Request class
//
// This class maintains a list of segment descriptors to process incoming
// or outgoing segments.
//

class LtIpSegReq : public LtObject
{
public:
	enum {
		SEG_SIZE		= LtIpSegment::MAX_PAYLOAD,		// max bytes in a segment
		MAX_SEGMENTS	= 40,		// max segments in a whole message
		MAX_TOTAL_SEG_PAYLOAD	= (SEG_SIZE * MAX_SEGMENTS),
		TIMEOUT_RETRANS = (1*900),	// retransmit timer when we have the ball or req seg timeout
		TIMEOUT_QUIET	= (10*1000),	// ten seconds with no activity times out
		TIMEOUT_BUSY	= (30*1000),	// IKP05132003: thirty seconds times out even if activity
		JUNK			= 0
	};
protected:
	LtIpRequest	m_orgRequest;			// original request that created us
	byte		m_nType;				// PKTTYPE_ code of the request
	word		m_nReqId;				// request id of this request
	boolean		m_bIpFromPkt;			// ip addr/port from LtPktInfo
	ULONG		m_ipAddr;				// ip address and port
	word		m_port;
	boolean		m_bOutbound;			// we have the data to send
	boolean		m_bHasBall;			// we have the ball, which means we need to make
										// sure, if we are inbound, that we keep sending
										// requests until we get all data
										// or if we are outbound, that at least one segment
										// is requested from us.
	int			m_nSegments;			// number of segments in payload
	ULONG		m_tickStarted;			// tick count for starting process
	ULONG		m_tickLast;				// tick count last time we received message
	ULONG		m_tickSent;				// tick count when we last sent something
	int			m_nHighestSent;			// highest segment sent - with a sendall
	ULONG		m_dateTime;				// datetime of packet stream
	boolean		m_bComplete;			// received packet is complete
	boolean		m_bPayloadDelivered;	// we have returned the data
	byte*		m_pPayload;				// assembled payload
	int			m_nPayloadSize;			// size of assembled payload
	boolean		m_bWeOwnPayload;		// if we own it, then we can free it
	LtIpSegment	m_aSegs[MAX_SEGMENTS];	// list of segments if request
	LtQue		m_qSegs;				// queue of LtPktInfo's of request
	boolean		m_bUseExtHdrs;			// use extended packet headers
	ULONG		m_ipAddrLocal;			// Data for extended headers
	ULONG		m_natIpAddr;			//   same
	word		m_ipPortLocal;			//	 same


	void		freePayload();			// free the payload if we own it
	void		emptyQueue();			// release all waiting segments
	void		zapSegs();				// zap all the segments we have, and release all packets
										// we had a dateTime change
	boolean		checkComplete();		// check to see if inbound segments are complete

public:
				LtIpSegReq();
	virtual		~LtIpSegReq();

	word		getRequestId()
	{	return m_nReqId;
	}
	ULONG		getDatetime()
	{	return m_dateTime;
	}
	// set the direction and whether we have the ball
	void		setDirectionAndBall( boolean bOutbound, boolean bHasBall )
	{	m_bOutbound = bOutbound;
		m_bHasBall = bHasBall;
	}
	boolean		isOutbound()
	{	return m_bOutbound;
	}
	boolean		hasBall()
	{	return m_bHasBall;
	}
	void		dropBall()
	{	m_bHasBall = false;
	}
	void		setUseExtHdrs(boolean bExtHdrs)
	{
		m_bUseExtHdrs = bExtHdrs;
	}
	void setExtHdrData(ULONG ipAddr, ULONG natAddr, word ipPort, boolean bExtHdrs)
	{
		m_ipAddrLocal = ipAddr;
		m_natIpAddr = natAddr;
		m_ipPortLocal = ipPort;
		m_bUseExtHdrs = bExtHdrs;
	}
	void setPktExtHdrData(LtIpPktHeader *pHdr)
	{
		pHdr->extHdrLocalIpAddr = m_ipAddrLocal;
		pHdr->extHdrNatIpAddr = m_natIpAddr;
		pHdr->extHdrIpPort = m_ipPortLocal;
		pHdr->bHasExtHdr = m_bUseExtHdrs;
		pHdr->extndHdrSize = 0;	// This will be modified (if appropriate) in build()
	}
	int packetSize()
	{
		int hdrSize = m_bUseExtHdrs ? LtIpPktHeader::EXT_HDR_SIZE :
										LtIpPktHeader::STD_HDR_SIZE;
		return (hdrSize + LtIpSegment::ADDED_SIZE + SEG_SIZE);
	}

	// for received packets, pass a segment in. Get a true if the packet was saved
	// and the request owns it, or false if it was not kept.
	// If not kept, then caller must release.
	// If kept, caller must not release. It will be released when
	// request times out.
	boolean		segmentArrived( LtPktInfo* pPkt, LtIpSegment& segPkt );

	// IKP05122003: has it been TIMEOUT_RETRANS since the start of this segment processing?
	boolean     hasReqRetransmissionTimerTimedOut( ULONG nTicksNow )
	{
		boolean bReturn = false;
		ULONG	nQuietTime = nTicksNow - m_tickSent;	// when was the last request sent?
		if (nQuietTime > (ULONG)msToTicksX(TIMEOUT_RETRANS))
		{
			// yes, we need to retransmit the segment or request for the segment
			bReturn = true;
		}
		return bReturn;
	}
	// IKP05122003: reset the retransmission timer to the current tick count
	void updateSentTimer()
	{
		m_tickSent = tickGet();	// tick count for starting process
	}
	boolean		isTimedOut( ULONG nTicksNow );
	boolean		isComplete()
	{	return m_bComplete;
	}
	byte*		getPayload()
	{	return m_pPayload;
	}
	int			getPayloadSize()
	{	return m_nPayloadSize;
	}
	int			getNumSegments()
	{	return m_nSegments;
	}
	void		takePayload()
	{	m_pPayload = NULL;
		m_nPayloadSize = 0;
		m_bPayloadDelivered = true;
	}
	// IKP05202003: added support for checking of any pending requests
	boolean		isPayloadDelivered()
	{
		return m_bPayloadDelivered;
	}
	/*
	void		setDone()
	{	m_bPayloadDelivered = true;
	}
	*/
	// for outgoing packets, destination set manually or from Packets
	void		setIpAddrPort( boolean bFromPkt )
	{	m_bIpFromPkt = bFromPkt;
	}
	void		setIpAddrPort( ULONG ipAddr, word port )
	{	m_ipAddr = ipAddr;
		m_port = port;
	}

	void		setRequest( LtIpRequest& reqPkt )
	{	m_orgRequest = reqPkt;
		setType( reqPkt.packetType );
		m_nReqId = reqPkt.requestId;
	}

	// used to purge outstanding requests if the data goes stale
	void		setType( int nPktType )
	{	m_nType = nPktType;
	}
	int			getType()
	{	return m_nType;
	}
	boolean		matchType( int nPktType )
	{	return m_nType == nPktType ;
	}
	boolean		match( ULONG ipAddr, word port, word reqId )
	{
		boolean bIpMatch = ipAddr!= 0 ? ( ipAddr == m_ipAddr && port == m_port ) : true;
		return ( bIpMatch && reqId == m_nReqId );
	}
	boolean		matchTypeIp( int nPktType, ULONG ipAddr, word port )
	{
		boolean bIpMatch = ipAddr!= 0 ? ( ipAddr == m_ipAddr && port == m_port ) : true;
		boolean bTypeMatch = nPktType ? ( m_nType == nPktType ) : true;
		return ( bTypeMatch && bIpMatch );
	}

	ULONG		getIpAddr()
	{	return m_ipAddr;
	}
	word		getPort()
	{	return m_port;
	}
	// bOwn = false - caller owns data, callee must copy it.
	//        true  - callee owns data. No copy required.
	boolean		buildSegments( byte* pData, int nSize,
						word reqId, boolean bOwn=false );
	boolean		getSegment( int nSegment, LtIpSegment& segPkt );
	void		setHighestSent( int nSeg )
	{	if ( nSeg > m_nHighestSent ) m_nHighestSent = nSeg;
	}

	// get request for inbound transaction, if it has been long enough since
	// the otherside sent us anything
	boolean getInboundRequest( ULONG nTickNow, LtIpRequest& reqPkt );
	// get segment to send for outbound request, if we have the ball
	//
	boolean getOutboundSegment( ULONG nTickNow, LtIpSegment& segPkt );

};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// LtIpSegmentor
//
// high level class maintains a list of segment requiests until they are satisfied
// or times them out.
//

class LtIpSegReq;
class LtPktInfo;
class CIpLink;
//typedef LtTypedVVector<LtIpSegReq> LtSegReqV;
typedef LtTypedVector<LtIpSegReq> LtSegReqV;

class LtIpSegmentor : public LtIpSegBase
{
protected:
	CIpLink*		m_pLink;				// pointer to link object to send packet
	LtPktAllocator*	m_pAlloc;				// allocator to get buffers for sending
											// cannot use for receiving since packets may be large
	word			m_nNextReqId;			// The next request Id to use for sending
	LtSegReqV		m_vReqs;				// vector of outstanding requests
	LtPktInfo*		m_pSentPkt;				// testing - the packet we built to send if no link
	LtMD5			m_md5;					// seat of the secret...
	boolean			m_bActive;				// Must be active to do anything
	boolean			m_bUseExtHdrs;			// Use extended packet headers
	ULONG			m_ipAddrLocal;			// Data for extended headers
	ULONG			m_natIpAddr;			//   same
	word			m_ipPortLocal;			//	 same

public:
	enum {
		TIMEOUT_TIMERMS	= (1*1000),		// timer tick rate in milliseconds
	};
	LtIpSegmentor();
	virtual ~LtIpSegmentor();
	void	setLink( CIpLink* pLink )
	{	m_pLink = pLink;
	}
	void	setAllocator( LtPktAllocator* pAlloc )
	{	m_pAlloc = pAlloc;
	}
	void	setActive( boolean bActive )
	{	m_bActive = bActive;
	}
	boolean	isActive()
	{	return m_bActive;
	}

	// must call setSecret first, then we operate locally
	void	setSecret( byte* pSecret )
	{	m_md5.setSecret( pSecret );
	}
	void getSecret(byte* pSecret)
	{
		m_md5.getSecret(pSecret);
	}
	void	setAuthenticating( boolean bAuth )
	{	m_md5.setAuthenticating( bAuth );
	}
	boolean isAuthenticating()
	{
		return m_md5.isAuthenticating();
	}
	void	setEIA852Auth(boolean bEnable)
	{
		m_md5.setEIA852Auth(bEnable);
	}
	boolean isEIA852Auth()
	{
		return m_md5.isEIA852Auth();
	}
	void setExtHdrData(ULONG ipAddr, ULONG natAddr, word ipPort, boolean bExtHdrs)
	{
		m_ipAddrLocal = ipAddr;
		m_natIpAddr = natAddr;
		m_ipPortLocal = ipPort;
		m_bUseExtHdrs = bExtHdrs;
	}

	// **** OUTBOUND SEGMENTS
	// to send segments out, we create a request with  buildSegments
	// which sends the first or all depending.
	// then call segmentRequest for each segment request message that comes in.
	// we are answering a request
	// HasBall means that this is an unsolicited outbound request.
	// If we have the ball, then we need to re-transmit until the otherside
	// responds with a message, either a request for more, or an ack.
	boolean	buildSegments( LtIpRequest& reqPkt, byte* pData, int nSize,
						ULONG ipAddr, word port, boolean bHasBall );
	boolean	segmentRequest( LtIpRequest& reqPkt, ULONG ipAddress, word port );
	// when data goes stale, then we can purge the outgoing requests
	// when a segment request comes in from the requestor, then we will start
	// another request anyway.
	// purge a specific request when its satisfied, one way or another.
	void	purgeRequests( int nPktType, ULONG ipAddr = 0, word port = 0 );
	// purge all requests, no matter what type
	void	purgeAllRequests();

	// IKP05202003: added support for checking of any pending requests
	// return true if segmentor is currently handling this particular request
	boolean	hasPendingRequest( int nPktType, ULONG ipAddr, word port );

	int		getRequestCount()
	{
		return m_vReqs.getCount();
	}

	// **** INBOUND SEGMENTS
	// to build request for receiving, create a LtIpRequest or subclass and fill it out.
	// then call createRequest to create a request. This creates the message and sends
	// it out, or if a request already exists that matches it, then it sends a request
	// for the next segment we don't have. Timer up to caller, but messages sent
	// by segment manager.
	boolean	newInboundRequest( LtIpRequest& reqPkt, ULONG ipAddress, word port );
	// As each segment packet arrives, call receivedPacket. When request is satisfied by
	// last segment packet, you get the assembled payload in the ppPktRtn and return true.
	boolean	receivedPacket( LtPktInfo* pPkt, LtPktInfo** ppPktRtn, ULONG ipAddr, word port );

	// for testing only
	boolean	dumpSegments( word requestId );
	void	dump( boolean bSegments = false );

	// if no link is set, then packets are set to be picked up by polling.
	// You own it now.
	LtPktInfo*	getSentPacket()
	{	LtPktInfo*	pPkt = m_pSentPkt;
		m_pSentPkt = NULL;
		return pPkt;
	}

protected:

	// create a new request with a new request id
	LtIpSegReq*	newRequest( LtIpRequest& reqPkt, ULONG ipAddress, word port );
	// find a request in our list that matches everything
	LtIpSegReq* findRequest( int reqType, ULONG ipAddress, word port );

	boolean	sendSegment( LtIpSegReq* pReq, int nSeg, boolean bRemaining =false );
	boolean	sendPacket( byte* pData, int nLen, ULONG ipAddr, word port );
	boolean doTimeout( ULONG msTick );

};


#endif // _LTIPSEGMENT_H
