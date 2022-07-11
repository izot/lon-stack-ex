/***************************************************************
 *  Filename: LtIpEchPackets.cpp
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
 *  Description:  implementation file for LonTalk IP Packet formats.
 *
 *	DJ Duffy Feb 1999
 *
 ****************************************************************/

/*
 * $Log: /Dev/ShareIP/LtIpEchPackets.cpp $
 *
 * 18    11/05/04 3:35p Fremont
 * iLON 100 PPP router demo
 *
 * 17    9/03/03 5:33p Fremont
 * Implement extended packet header for EPR 29618.
 *
 * 16    6/20/03 2:30p Fremont
 * add field to EchMode pkt for local config state
 *
 * 15    6/10/03 5:33p Fremont
 * move from LtIpPktHeader to LtIpPktBase
 *
 * 14    6/03/03 7:17p Fremont
 * more EIA-852 packet transformations, general cleanup
 *
 * 13    6/02/03 10:36a Fremont
 * move NAT IP addr from Ech Config pkt to Ech Mode pkt
 *
 * 12    5/27/03 2:34p Fremont
 * Rename field
 *
 * 11    5/09/03 1:46p Fremont
 * fix version packet parsing and dumping
 *
 * 10    4/08/03 11:56a Fremont
 * packet size and parsing changes
 *
 * 9     3/25/03 12:42p Fremont
 * add Echelon extended channel routing request packet
 *
 * 8     3/18/03 4:35p Fremont
 * unused var
 *
 * 7     3/04/03 3:43p Fremont
 * updated config and version pkts to ver 2, add mode pkt
 *
 * 6     6/14/02 5:59p Fremont
 * remove warnings
 *
 * 5     3/17/00 5:08p Darrelld
 * Add version packet
 *
 * 3     9/16/99 4:48p Darrelld
 * Add timezone to config packet
 *
 * 2     9/09/99 11:46a Darrelld
 * Add echelon control packet
 *
 * 1     7/21/99 3:09p Darrelld
 *
 * 19    7/20/99 5:26p Darrelld
 * Updates for reordering test
 *
 * 18    7/14/99 9:32a Darrelld
 * Support primary and secondary time servers
 *
 * 17    7/02/99 8:45a Darrelld
 * Target specific files
 *
 * 16    6/22/99 9:47a Darrelld
 * Correct processing of packet size and update packet type codes
 *
 * 15    5/14/99 2:44p Darrelld
 *
 * 14    5/13/99 4:56p Darrelld
 * Segment packet support in progress
 *
 * 13    5/07/99 11:01a Darrelld
 * Upgrade for latest RFC packet formats
 *
 * 12    5/06/99 5:09p Darrelld
 * Enhancments to RFC packets
 *
 * 11    4/23/99 5:22p Darrelld
 * Router testing
 *
 * 10    4/22/99 5:02p Darrelld
 * Testing of routers
 *
 * 9     4/20/99 4:12p Darrelld
 * add "confidential" statement
 *
 * 8     4/13/99 4:58p Darrelld
 * Enhance for Aggregation and BW Limiting and test
 *
 * 7     4/02/99 2:20p Darrelld
 * Defensive about terminated name in dump
 *
 * 6     3/15/99 10:37a Darrelld
 * Add dumpPacket to dump a decoded packet
 *
 * 5     3/01/99 5:08p Darrelld
 * Intermediate checkin
 *
 * 4     2/26/99 4:52p Darrelld
 * Intermediate check-in
 *
 * 3     2/25/99 5:38p Darrelld
 * Intermediate check-in
 *
 * 2     2/09/99 5:05p Darrelld
 * Packet formats and parsing
 *
 * 1     2/03/99 10:39a Darrelld
 *
 *
 *
 */
#include <VxWorks.h>
#include <tickLib.h>
#include <sysLib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <VxlTypes.h>
#include <LtIpPackets.h>
#include <LtIpEchPackets.h>
#include <vxlTarget.h>




///////////////////////////////////////////////////////////////////////////////
// Time Synch packet
//
// This packet is used by a configuration server to obtain the
// timestamp of a member to determine it's clock synchronization with
// the timeserver.
//
// class LtIpTimeSynch : public LtIpPktBase
//
//
// parse
//
// parse a packet.
//
boolean		LtIpTimeSynch::parse( byte* pStart, boolean bRewrite )
{
	boolean		bOk = false;

	boolean bHeaderOk = LtIpPktBase::parse( pStart, bRewrite );
	byte*			p = pRemaining;

	if ( bHeaderOk && (packetSize >= fixedPktSize()) &&
		( packetType == PKTTYPE_TIMESYNCHREQ ||
		  packetType == PKTTYPE_TIMESYNCHRSP
		  )
		)
	{
		PTOHL(p, dateTime );
		PTOHL(p, dateTimeRsp );
		PTOHL(p, timeStampReq );
		PTOHL(p, timeStampRsp );
		bOk = true;
	}
	return bOk;
}

//
// build
//
// build a packet of this type if it's all filled in
// returns pointer beyond the packet built.
//
byte*		LtIpTimeSynch::build( byte* pStartPoint )
{
	byte*			p = pStartPoint;

	p = LtIpPktBase::build( p );
	PTONL(p, dateTime );
	PTONL(p, dateTimeRsp );
	PTONL(p, timeStampReq );
	PTONL(p, timeStampRsp );

	buildPacketSize( p );
	return p;
}

//
// dump
//
// dump the packet for debugging
//
void LtIpTimeSynch::dump()
{
	LtIpDateTimeStr	dts1;
	LtIpDateTimeStr	dts2;

	LtIpPktBase::dump();
	vxlReportEvent( "LtIpTimeSynch::dump - dateTime Req %s dateTime Rsp %s\n"
					"                      timeStampReq %d timeStampRsp %d\n",
					dts1.getString(dateTime), dts2.getString(dateTimeRsp),
					timeStampReq, timeStampRsp );
}

///////////////////////////////////////////////////////////////////////////////
// Echelon Config packet
//
// This packet is used by a configuration server to configure
// devices with Echelon specific parameters.
//
// class LtIpEchConfig : public LtIpPktBase
//
//
// parse
//
// parse a packet.
//
boolean		LtIpEchConfig::parse( byte* pStart, boolean bRewrite )
{
	boolean		bOk = false;
	int			i;

	boolean bHeaderOk = LtIpPktBase::parse( pStart, bRewrite );
	byte*			p = pRemaining;

	if ( bHeaderOk && (packetSize >= fixedPktSize()) &&
		( packetType == PKTTYPE_ECHCONFIG )
		)
	{
		PTOHL(p, dateTime );
		PTOHL(p, flags );
		PTOHL(p, aggTimerMs );
		PTOHL(p, bwLimitKbPerSec );
		PTOHL(p, escrowTimerMs );
		PTOHL(p, TosBits );
		for ( i=0; i< ECHCFG_TZSIZE; i++ )
		{	PTOHB( p, szTimeZone[i] );
		}
		if (packetSize >= echV2FixedSize())
		{
			// parse the version 2 fields
			setEchVersion(LTIP_ECH_VER2);	// if it isn't already...
			// Currently nothing here
		}
		bAggregate = 0 != ( flags & ECHCFG_AGGREGATE );
		bBwLimit = 0 != ( flags & ECHCFG_BWLIMIT );
		bNoReorder = 0 != ( flags & ECHCFG_NOREORDER );
		bAuthenticate = 0 != ( flags & ECHCFG_AUTHENTICATE );
		bUseTosBits = 0 != ( flags & ECHCFG_USETOS );
		bUseTZ = 0 != ( flags & ECHCFG_USETZ );
		bOk = true;
	}
	return bOk;
}

//
// build
//
// build a packet of this type if it's all filled in
// returns pointer beyond the packet built.
//
byte*		LtIpEchConfig::build( byte* pStartPoint )
{
	byte*			p = pStartPoint;
	int				i;

	p = LtIpPktBase::build( p );

	flags |= bAggregate ? ECHCFG_AGGREGATE : 0;
	flags |= bBwLimit ? ECHCFG_BWLIMIT : 0;
	flags |= bNoReorder ? ECHCFG_NOREORDER : 0;
	flags |= bAuthenticate ? ECHCFG_AUTHENTICATE : 0;
	flags |= bUseTosBits ? ECHCFG_USETOS : 0;
	flags |= bUseTZ ? ECHCFG_USETZ : 0;

	PTONL(p, dateTime );
	PTONL(p, flags );
	PTONL(p, aggTimerMs );
	PTONL(p, bwLimitKbPerSec );
	PTONL(p, escrowTimerMs );
	PTONL(p, TosBits );
	for ( i=0; i< ECHCFG_TZSIZE; i++ )
	{	PTONB( p, szTimeZone[i] );
	}
	if (pktIsEchVer2())
	{
		// Add fields for version 2 packet
		// Currently nothing here
	}

	buildPacketSize( p );
	return p;
}

//
// dump
//
// dump the packet for debugging
//
void LtIpEchConfig::dump()
{
	LtIpDateTimeStr	dts1;

	LtIpPktBase::dump();
	vxlReportEvent( "LtIpEchConfig::dump - dateTime Req %s\n"
					"                      aggregate %s bwLimit %s NoReorder %s Authenticate %s UseTos %s\n"
					"                      aggTimerMs %d bwLimitKbPerSec %d escrowTimerMs %d TosBits 0x%08x\n",
					dts1.getString(dateTime),
					bAggregate?"T":"F", bBwLimit?"T":"F", bNoReorder?"T":"F", bAuthenticate?"T":"F", bUseTosBits?"T":"F",
					aggTimerMs, bwLimitKbPerSec, escrowTimerMs, TosBits );
	// assure a terminator if none
	szTimeZone[ ECHCFG_TZSIZE-1 ] = 0;
	vxlReportEvent( "       Timezone %s\n", szTimeZone);
}

///////////////////////////////////////////////////////////////////////////////
// Echelon Control packet
//
// This packet is used by a configuration server to configure
// devices with Echelon specific parameters.
//
// class LtIpEchControl : public LtIpPktBase
//
//
// parse
//
// parse a packet.
//
boolean		LtIpEchControl::parse( byte* pStart, boolean bRewrite )
{
	boolean		bOk = false;

	boolean bHeaderOk = LtIpPktBase::parse( pStart, bRewrite );
	byte*			p = pRemaining;

	if ( bHeaderOk && (packetSize >= fixedPktSize()) &&
		( packetType == PKTTYPE_ECHCONTROL)
		)
	{
		PTOHL(p, dateTime );
		PTOHL(p, flags );
		bReboot = 0 != ( flags & ECHCON_REBOOT );
		bStopWeb = 0 != ( flags & ECHCON_WEBSTOP );
		bStartWeb = 0 != ( flags & ECHCON_WEBSTART );
		bOk = true;
	}
	return bOk;
}

//
// build
//
// build a packet of this type if it's all filled in
// returns pointer beyond the packet built.
//
byte*		LtIpEchControl::build( byte* pStartPoint )
{
	byte*			p = pStartPoint;

	p = LtIpPktBase::build( p );

	flags |= bReboot ? ECHCON_REBOOT : 0;
	flags |= bStopWeb ? ECHCON_WEBSTOP : 0;
	flags |= bStartWeb ? ECHCON_WEBSTART : 0;

	PTONL(p, dateTime );
	PTONL(p, flags );

	buildPacketSize( p );
	return p;
}

//
// dump
//
// dump the packet for debugging
//
void LtIpEchControl::dump()
{
	LtIpDateTimeStr	dts1;

	LtIpPktBase::dump();
	vxlReportEvent( "LtIpEchControl::dump - dateTime %s\n"
					"                      reboot %s WebStop %s WebStart %s \n",
					dts1.getString(dateTime),
					bReboot?"T":"F", bStopWeb?"T":"F", bStartWeb?"T":"F");
}


///////////////////////////////////////////////////////////////////////////////
// Echelon Version packet
//
// This packet is used to convey the version of the software.
//
// class LtIpEchVersion : public LtIpPktBase
//
//
// parse
//
// parse a packet.
//
boolean		LtIpEchVersion::parse( byte* pStart, boolean bRewrite )
{
	boolean		bOk = false;

	boolean bHeaderOk = LtIpPktBase::parse( pStart, bRewrite );
	byte*			p = pRemaining;

	if ( bHeaderOk && (packetSize >= echV1FixedSize()) &&
		( packetType == PKTTYPE_ECHVERSION )
		)
	{
		PTOHL(p, dateTime );
		PTOHBN( ECHVRS_PRODUCTSIZE, p, szProduct );
		PTOHS(p, nMajorVersion );
		PTOHS(p, nMinorVersion );
		PTOHS(p, nBuildNumber );
		if (packetSize >= echV2FixedSize())
		{
			// parse the version 2 fields
			setEchVersion(LTIP_ECH_VER2);	// if it isn't already...
			PTOHS(p, echVersionSupported );
		}
		else
		{
			// set the version 2 fields
			setEchVersion(LTIP_ECH_VER1);	// if it isn't already...
			echVersionSupported = LTIP_ECH_VER1;
		}
		bOk	= true;
	}
	return bOk;
}

//
// build
//
// build a packet of this type if it's all filled in
// returns pointer beyond the packet built.
//
byte*		LtIpEchVersion::build( byte* pStartPoint )
{
	byte*			p = pStartPoint;

	p = LtIpPktBase::build( p );

	PTONL(p, dateTime );
	PTONBN( ECHVRS_PRODUCTSIZE, p, szProduct );
	PTONS(p, nMajorVersion );
	PTONS(p, nMinorVersion );
	PTONS(p, nBuildNumber );
	if (pktIsEchVer2())
	{
		PTONS(p, echVersionSupported );
	}

	buildPacketSize( p );
	return p;
}

//
// dump
//
// dump the packet for debugging
//
void LtIpEchVersion::dump()
{
	LtIpDateTimeStr	dts1;
	char v2Buf[100] = "";

	LtIpPktBase::dump();
	// assure a terminator if none
	szProduct[ECHVRS_PRODUCTSIZE-1] = 0;

	if (pktIsEchVer2())
	{
		sprintf(v2Buf, "                      Echelon protcol version %d\n", echVersionSupported);
	}

	vxlReportEvent( "LtIpEchVersion::dump - dateTime Req %s\n"
					"                      Product Name %s\n"
					"                      Version Number %d.%d%d.%d\n%s",
					dts1.getString(dateTime),
					szProduct,
					nMajorVersion, nMinorVersion%10, nMinorVersion/10, nBuildNumber, v2Buf );
}

///////////////////////////////////////////////////////////////////////////////
// Echelon Mode packet
//
// This packet is used to convey the get/set the operating mode of the software.
//
//
// parse
//
// parse a packet.
//
boolean		LtIpEchMode::parse( byte* pStart, boolean bRewrite )
{
	boolean		bOk = false;

	boolean bHeaderOk = LtIpPktBase::parse( pStart, bRewrite );
	byte*			p = pRemaining;

	if ( bHeaderOk && (packetSize >= fixedPktSize()) &&
		( packetType == PKTTYPE_ECHMODE)
		)
	{
		PTOHL(p, natIpAddr );
		PTOHS(p, echVersionInUse );
		PTOHB(p, strictEia852);
		PTOHB(p, usingLocalCnfg);
		bOk	= true;
	}
	return bOk;
}

//
// build
//
// build a packet of this type if it's all filled in
// returns pointer beyond the packet built.
//
byte*		LtIpEchMode::build( byte* pStartPoint )
{
	byte*			p = pStartPoint;

	p = LtIpPktBase::build( p );

	PTONL(p, natIpAddr );
	PTONS(p, echVersionInUse );
	PTONB(p, strictEia852);
	PTONB(p, usingLocalCnfg);

	buildPacketSize( p );
	return p;
}

//
// dump
//
// dump the packet for debugging
//
void LtIpEchMode::dump()
{
	LtIpAddressStr	ias;

	LtIpPktBase::dump();

	vxlReportEvent( "LtIpEchMode::dump - NAT IP address %s  Echelon version in use: %d\n"
					"                      Strict EIA-852: %s  Local config: %s\n",
					ias.getString(natIpAddr), echVersionInUse,
					strictEia852?"T":"F", usingLocalCnfg?"T":"F");
}

///////////////////////////////////////////////////////////////////////////////
// Echelon extended channel routing request packet
//
// This packet is an extension of the standard request packet
// It includes any number of IP address/port pairs.
// Used only for requesting channel routing packets.
//

// *** NOTE *** The LIST part of this packet is currently
// NOT IMPLEMENTED by either the iLON or the config server!
// Only the single port for the single IP address is implemented.
// Set the list size to zero.

// parse
//
// parse a packet.
//
boolean		LtIpReqEchChnRouting::parse( byte* pStart, boolean bRewrite )
{
	boolean		bOk = false;
	int				i;
	byte*			p2;
	byte*			p3;
	LtIpAddPort		iap;

	boolean		bHeaderOk = LtIpReqChnRouting::parse( pStart, bRewrite );

	// These rely on the above parse() call.
	// Because this packet is built on top of a regular request,
	// the current pointer must be beyond the end of a standard pkt.
	byte*		p = pRemaining + LtIpReqChnRouting::ADDED_SIZE;

	if ( bHeaderOk && (packetSize >= fixedPktSize()) &&
		( packetType == PKTTYPE_REQCHNROUTING)
		)
	{
		PTOHS(p, listSize );
		PTOHS(p, port );

		// Now that we know the fixed data, check to make sure
		// there is enough data to parse the dynamic data
		if (packetSize >= definedPktSize())
		{
			pUcAddresses = p;
			p2 = p;
			p3 = p;
			for ( i=0; i< listSize; i++ )
			{
				p2 = iap.parse( p2 );
				if ( bRewrite ) p3 = iap.store( p3 );
			}

			bOk = true;
		}
	}

	return bOk;
}

//
// build
//
// build a packet of this type if it's all filled in
// returns pointer beyond the packet built.
//
byte*		LtIpReqEchChnRouting::build( byte* pStartPoint )
{
	byte*			p = pStartPoint;
	int				i;
	byte*			p2;
	byte*			p3;
	LtIpAddPort		iap;
	LtIpReqChnRouting stdReq;

	// Make a copy of this extended request in a standard request
	// and build that. Otherwise, the packet size will appear to
	// be wrong when the standard build routine is called.
	// Then copy back anything set by that build() call.
	stdReq = *this;
	p = stdReq.build( p );
	*(dynamic_cast<LtIpReqChnRouting *>(this)) = stdReq;

	PTONS(p, listSize );
	PTONS(p, port );

	p2 = (byte*)pUcAddresses;
	p3 = p;
	pUcAddresses = p3;
	for ( i=0; i< listSize; i++ )
	{
		p2 = iap.fetch( p2 );
		p3 = iap.build( p3 );
	}

	buildPacketSize( p3 );
	return p3;
}

//
// dump
//
// dump the packet for debugging
//
void LtIpReqEchChnRouting::dump()
{
	LtIpReqChnRouting::dump();

	vxlReportEvent( "LtIpReqEchChnRouting::dump - requesting %d routing packets\n",
					listSize);
}


///////////////////////////////////////////////////////////////////////////////
// Echelon Device ID packet
//
// This packet is used to inform a remote device or CS of the identity of the
// sender of the packet. This is needed when firewalls switch the source port.
//
boolean LtIpEchDeviceId::parse(byte* pStart, boolean bRewrite)
{
	boolean		bOk = false;

	boolean bHeaderOk = LtIpPktBase::parse( pStart, bRewrite );
	byte*			p = pRemaining;

#ifdef ILON_100_ROUTER_DEMO
	if ( bHeaderOk && (packetSize >= echV2_0FixedSize()) &&
#else
	if ( bHeaderOk && (packetSize >= fixedPktSize()) &&
#endif
		( packetType == PKTTYPE_ECHDEVID)
		)
	{
		PTOHL(p, localIpAddr );
		PTOHL(p, natIpAddr );
		PTOHS(p, ipPort );
#ifdef ILON_100_ROUTER_DEMO
		if (packetSize >= echV2FixedSize())
		{
			PTOHBN((int)sizeof(ltUniqueId), p, ltUniqueId);
			PTOHL(p, prevLocalIpAddr );
		}
		else
		{
			prevLocalIpAddr = 0;
		}
#endif
		bOk	= true;
	}
	return bOk;
}

byte* LtIpEchDeviceId::build( byte* pStartPoint )
{
	byte*			p = pStartPoint;

	p = LtIpPktBase::build( p );

	PTONL(p, localIpAddr );
	PTONL(p, natIpAddr );
	PTONS(p, ipPort );
#ifdef ILON_100_ROUTER_DEMO
	PTONBN((int)sizeof(ltUniqueId), p, ltUniqueId);
	PTONL(p, prevLocalIpAddr );
#endif

	buildPacketSize( p );
	return p;
}

void LtIpEchDeviceId::dump()
{
	LtIpAddressStr	ias1;
	LtIpAddressStr	ias2;

	LtIpPktBase::dump();

	vxlReportEvent( "LtIpEchDeviceId::dump - Local IP address: %s\n"
					"                      NAT IP address %s IP port: %d\n",
					ias1.getString(localIpAddr), ias2.getString(natIpAddr),
					ipPort);
}


///////////////////////////////////////////////////////////////////////////////
// Echelon Device ID Request packet
//
// This packet is an extension of the standard request packet
// It includes the senders ID info, allowing the receiver to
// uniquely identify the sender of the request.
// Used only for requesting a device ID packet.
//

boolean LtIpReqEchDeviceId::parse(byte* pStart, boolean bRewrite)
{
	boolean		bOk = false;

	boolean		bHeaderOk = LtIpRequest::parse( pStart, bRewrite );

	// These rely on the above parse() call.
	// Because this packet is built on top of a regular request,
	// the current pointer must be beyond the end of a standard pkt.
	byte*		p = pRemaining + LtIpReqEchDeviceId::ADDED_SIZE;

#ifdef ILON_100_ROUTER_DEMO
	if ( bHeaderOk && (packetSize >= echV2_0FixedSize()) &&
#else
	if ( bHeaderOk && (packetSize >= fixedPktSize()) &&
#endif
		( packetType == PKTTYPE_ECHDEVIDREQ)
		)
	{
		PTOHL(p, localIpAddr );
		PTOHL(p, natIpAddr );
		PTOHS(p, ipPort );
#ifdef ILON_100_ROUTER_DEMO
		if (packetSize >= echV2FixedSize())
		{
			PTOHL(p, prevLocalIpAddr );
		}
		else
		{
			prevLocalIpAddr = 0;
		}
#endif
		bOk = true;
	}

	return bOk;
}

byte* LtIpReqEchDeviceId::build( byte* pStartPoint )
{
	byte*			p = pStartPoint;
	LtIpRequest		stdReq;

	// Make a copy of this extended request in a standard request
	// and build that. Otherwise, the packet size will appear to
	// be wrong when the standard build routine is called.
	// Then copy back anything set by that build() call.
	stdReq = *this;
	p = stdReq.build( p );
	*(dynamic_cast<LtIpRequest*>(this)) = stdReq;

	PTONL(p, localIpAddr );
	PTONL(p, natIpAddr );
	PTONS(p, ipPort );
#ifdef ILON_100_ROUTER_DEMO
	PTONL(p, prevLocalIpAddr );
#endif

	buildPacketSize( p );
	return p;
}

void LtIpReqEchDeviceId::dump()
{
	LtIpAddressStr	ias1;
	LtIpAddressStr	ias2;

	LtIpRequest::dump();

	vxlReportEvent( "LtIpReqEchDeviceId::dump - Local IP address: %s\n"
					"                      NAT IP address %s IP port: %d\n",
					ias1.getString(localIpAddr), ias2.getString(natIpAddr),
					ipPort);
}

