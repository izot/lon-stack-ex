/***************************************************************
 *  Filename: LtIpPackets.cpp
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
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/ShareIp/LtIpPackets.cpp#2 $
//
/*
 * $Log: /Dev/ShareIp/LtIpPackets.cpp $
 *
 * 48    5/23/06 1:31p Fremont
 * Problems finding min() and max()
 *
 * 47    11/05/04 3:36p Fremont
 * bad packet size debug code
 *
 * 46    9/03/03 11:21p Fremont
 * clear exteded hdr flag on parse
 *
 * 45    9/03/03 5:36p Fremont
 * Implement extended packet header for EPR 29618.
 *
 * 44    8/04/03 1:33p Fremont
 * EPR 29341 cant get statistics from iLON 1000
 *
 * 43    7/08/03 2:33p Fremont
 * Add intf. to get 1900-1970 time delta
 *
 * 42    6/10/03 5:35p Fremont
 * changes for EIA0852 auth, move from LtIpPktHeader to LtIpPktBase
 *
 * 41    6/03/03 7:17p Fremont
 * more EIA-852 packet transformations, general cleanup
 *
 * 40    5/09/03 1:47p Fremont
 * fix parsing of Echelon requests
 *
 * 39    3/25/03 11:42a Fremont
 * tweak request packet parsing for new extended Echelon request
 *
 * 38    3/04/03 3:44p Fremont
 * add base class for version 2 pkts
 *
 * 37    6/14/02 5:59p Fremont
 * remove warnings
 *
 * 36    6/13/02 5:50p Fremont
 * fix warnings
 *
 * 35    3/15/02 4:34p Fremont
 * rename pnc to iLon
 *
 * 34    9/25/01 10:08a Fremont
 * Add include file for T2 warning
 *
 * 33    3/24/00 9:40a Darrelld
 * Better solution to printing domain
 *
 * 32    3/24/00 9:32a Darrelld
 * Only report domain bytes we actually have
 *
 * 31    2/25/00 5:45p Darrelld
 * Fixes for PCLIPS operation
 *
 * 30    11/05/99 1:03p Darrelld
 * Fix printing of zero dates
 *
 * 29    10/25/99 7:34a Darrelld
 * Make statics class specific
 *
 * 28    10/22/99 1:12p Darrelld
 * Don't use zero time stamp for certain cases
 *
 * 27    9/09/99 8:58a Darrelld
 * Fix LtIpPktStats packet type in build
 *
 * 26    9/08/99 2:22p Darrelld
 * New Common packet header says
 * Session number is now a ULONG
 * Add statistics packet
 *
 * 25    9/03/99 11:30a Darrelld
 * Use zero for timestamp when clock is not synched.
 *
 * 24    8/26/99 1:14p Darrelld
 * Fix LtIpDevRegister::dump
 *
 * 23    8/20/99 9:41a Darrelld
 * Don't call pncSntp... on WIN32
 *
 * 22    8/19/99 4:46p Darrelld
 * Don't cache time if not synched
 *
 * 21    7/27/99 1:51p Darrelld
 * Fix mbz fields
 *
 * 20    7/21/99 3:12p Darrelld
 * Remove echelon private packets
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
#include <iLonSntp.h>
#include <sntpcLib.h>

// Problems getting these in C++ with vxworks 6.2
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// static module routines

//
/////////////////////////////////////////////////////////////////////////////


static ULONG td1970 = 0;

//
// makeTd1970
//
// make time difference between 1Jan1900 and 1Jan1970
// to correct "unix" time.
//
static void makeTd1970()
{
	// leap years are 1904,08,12,16,20,24,28,32,36,40,44,48,52,56,60,64,68
	// not 1900
	// Don't know how many leap seconds there are...
	td1970 = (70*365 + (70/4));
	td1970 *= 24*3600;
}

//
// dateTimeToStr
//
// Convert a 1900 base datetime to ascii for printing.
// just convert it to "unix" time and them convert to text
//
static	void dateTimeToStr( ULONG dateTime, LPSTR pstr, int nMaxLen )
{
	if ( td1970 == 0 )
	{
		makeTd1970();
	}
	assert( nMaxLen > 24 );
	time_t	tNow = dateTime - td1970;
	char*	pastime = NULL;
	char*	cp;

	if ( dateTime != 0 )
	{
		pastime =  ctime(&tNow);
	}
	if ( pastime )
	{
		strcpy( pstr, pastime );
		cp = strrchr( pstr, '\n' );
		if ( cp ) *cp = 0;
	}
	else
	{	sprintf(pstr, "invalid %lu", dateTime );
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Common packet header
// this is an image of the actual packet.
// the only class that is an image.
//

//
// getTimestamp
//
//
int			LtIpPktHeader::clkRate = 0;
ULONGLONG	LtIpPktHeader::msPerSec = 1000;
boolean		LtIpPktHeader::bGotClock = false;
ULONG		LtIpPktHeader::nLastTick;
ULONGLONG	LtIpPktHeader::tClkTime;
struct timespec LtIpPktHeader::tTimeSpec;

//
//
// getTimestamp
//
// return the milliseconds of absolute time. Allow for wrap around
// and to make this faster, just get the full time every 1000 ticks.
// During the 1000 ticks, correct the time by adjusting for the tick count.
//
ULONG	LtIpPktHeader::getTimestamp(boolean bSyncCheck)
{
	ULONG		nTickDiff;
	ULONG		nTickNow;
	ULONGLONG	ms;
	ULONG		nMs;

	if ( clkRate == 0 )
	{
		clkRate = sysClkRateGet();
	}
	nTickNow = tickGet();
	nTickDiff = nTickNow - nLastTick;
	// if we don't have the clock value yet, or
	// the tick value is greater then 1000 ticks, then
	// get the clock value again.
	if ( nTickDiff > MAX_CLOCK_TICKS )
	{
		OsalClockGetTime( CLOCK_REALTIME, &tTimeSpec );
		nLastTick = tickGet();
		nTickDiff = 0;
		tClkTime = ( tTimeSpec.tv_sec );
		tClkTime *= 1000;
		tClkTime += ( tTimeSpec.tv_nsec / 1000000 );
		// If we're not synchronized yet, assume the retrieved value is bogus.
		bGotClock = iLonSntpTimeSynched();
	}
	if ( !bSyncCheck || bGotClock )
	{
		// compute the milliseconds adjustment based on the tick difference
		// and add in the base milliseconds time
		ms = (nTickDiff*msPerSec)/clkRate;
		ms += tClkTime;
	}
	else
	{	// if we don't have synch, then use zero for timestamp
		ms = 0;
	}
	#ifdef WIN32
	#pragma warning( disable:4244 )
	#endif
	nMs = ms;
	#ifdef WIN32
	#pragma warning( default:4244 )
	#endif
	return nMs;
}

#if 0 // obsolete

//
// getTimestamp
//
// Temporary. This is incorrect. But what is the right answer?
//
static	ULONGLONG	clkRate = 0;
static	ULONGLONG	msPerSec = 1000;

ULONG	LtIpPktHeader::getTimestamp()
{
	if ( clkRate == 0 )
	{
		clkRate = sysClkRateGet();
	}
	ULONGLONG	tick = tickGet();
	ULONGLONG	ms = (tick*msPerSec)/clkRate;
	ULONG		nMs;
#ifdef WIN32
#pragma warning( disable:4244 )
#endif
	nMs = ms;
#ifdef WIN32
#pragma warning( default:4244 )
#endif
	return nMs;
}
#endif // obsolete

//
// getDateTime
//
// return the corrected date/time for a config packet
//
ULONG	LtIpPktHeader::getDateTime( )
{
	time_t	t1970;

	ULONG	tNow;

	// get 1970 based Unix time
	t1970 = time(NULL);
	// adjust for 1Jan1900 base
	tNow = t1970 + getTd1970();

	return tNow;
}

// Get the time delta from 1900 to 1970, in seconds
ULONG	LtIpPktHeader::getTd1970()
{
	if ( td1970 == 0 )
	{
		makeTd1970();
	}
	return td1970;
}

//
//	Constructor
//
//
//
LtIpPktHeader::LtIpPktHeader()
{
	version = VERSION_ONE;			// standard version
	versionBits = 0;
	packetType = 0;
	packetSize = 0;
	extndHdrSize = 0;
	protocolFlags =	0;		// 0 for EIA-709 (LonTalk)
	vendorCode = 0;			// standard vendor code LonMark
	session = 0;			// session id filled in now
	sequence = 0;
	timestamp = getTimestamp();	// filled in automatically
	pRemaining = NULL;
	pPacket = NULL;
	// extended header fields
	extHdrLocalIpAddr = 0;
	extHdrNatIpAddr = 0;
	extHdrIpPort = 0;
	extHdrUnused = 0;
	bHasExtHdr = false;
}

//
// Destructor
//
//
//
LtIpPktHeader::~LtIpPktHeader()
{
}

//
// parse
//
// parse the packet header
//
boolean LtIpPktHeader::parse( byte* pPkt, boolean bRewrite )
{
	byte*	p = pPkt;
	pPacket = pPkt;

	PTOHS(p, packetSize );
	PTOHB(p, version );
	versionBits = version & 0xE0; // top three bits
	version = version & 0x1F; // bottom 5 bits
	PTOHB(p, packetType );
	PTOHB(p, extndHdrSize );
	// Version 1 used a 3-byte vendor code
	// EIA852 uses one byte for protocol flags, two bytes for vendor
	// Since the protocol is 0 for EIA-709 (LonTalk), they are equivalent
	PTOHB(p, protocolFlags );
	PTOHS(p, vendorCode );
	PTOHL(p, session );
	PTOHL(p, sequence );
	PTOHL(p, timestamp );
	// extndHdrSize is a count of 4-byte values
	if (extndHdrSize == (EXT_HDR_ADD_SIZE / 4))	
	{
		// Use the extended header fields
		PTOHL(p, extHdrLocalIpAddr);
		PTOHL(p, extHdrNatIpAddr);
		PTOHS(p, extHdrIpPort);
		PTOHS(p, extHdrUnused);
		bHasExtHdr = true;
	}
	else
	{
		// Just skip any unknown extra header. (Not used by us)
		p += (extndHdrSize*4);	// extndHdrSize is a count of 4-byte values
		bHasExtHdr = false;
	}
	pRemaining = p;
	return true;
}

//
// build
//
// build a packet at a start address
//
byte*		LtIpPktHeader::build( byte* pStartPoint )
{
	byte*	p = pStartPoint;
	byte	versionByte;

	pPacket = p;
	PTONS(p, packetSize );
	versionByte = version | versionBits;
	PTONB(p, versionByte );
	PTONB(p, packetType );
	if (bHasExtHdr)
	{
		// Set the implicit extended header size.
		// extndHdrSize is a count of 4-byte values.
		extndHdrSize = EXT_HDR_ADD_SIZE / 4;
	}
	PTONB(p, extndHdrSize );
	// Version 1 used a 3-byte vendor code
	// EIA852 uses one byte for protocol flags, two bytes for vendor
	// Since the protocol is 0 for EIA-709 (LonTalk), they are equivalent
	PTONB(p, protocolFlags );
	PTONS(p, vendorCode );
	PTONL(p, session );
	PTONL(p, sequence );
	PTONL(p, timestamp );
	if (bHasExtHdr)
	{
		// Use the extended header fields
		PTONL(p, extHdrLocalIpAddr);
		PTONL(p, extHdrNatIpAddr);
		PTONS(p, extHdrIpPort);
		PTONS(p, extHdrUnused);
	}
	else
	{
		// Just skip any unknown extra header. (Not used by us)
		p += (extndHdrSize*4);	// extndHdrSize is a count of 4-byte values
	}
	return p;
}

//
// dump
//
// dump the packet for debugging
//
void LtIpPktHeader::dump()
{
	vxlReportEvent(
		"LtIpPktHeader::dump - version %d versionBits 0x%02x vendorCode %d packetType %d packetSize %d\n"
		"                      extndHdrSize %2d flags 0x%02x session %10d sequence %10d timestamp %10d \n"
		"                      pRemaining 0x%08x pPacket 0x%08x\n",
		version, versionBits, vendorCode, packetType,
		packetSize, extndHdrSize, protocolFlags, session, sequence, timestamp, pRemaining, pPacket
		);
}

// static function
int LtIpPktHeader::hdrSize(boolean bExtHdr)
{
	return(bExtHdr ? EXT_HDR_SIZE : STD_HDR_SIZE);
}

#if !defined(WIN32) && !defined(NDEBUG) && !defined(linux)
#include <sysSymTbl.h>
#include "echelon\iLon.h"
#include "iLonUtil.h"
#endif

void LtIpPktBase::buildPacketSize(byte* pend)
{
#if !defined(WIN32) && !defined(NDEBUG) && !defined(linux)
	// Debug support for incorrect packet size
	UINT raVal;
	char symBuf[MAX_SYS_SYM_LEN + 12];
	SAVE_RETURN_ADDRESS(raVal);
#endif

	LtIpPktHeader::buildPacketSize(pend);

#if !defined(WIN32) && !defined(NDEBUG) && !defined(linux)
//#if !defined(WIN32) && !defined(NDEBUG)
	if (packetSize != definedPktSize())
	{
		translateAddress(raVal, symBuf, sizeof(symBuf));
		vxlPrintf("LtIpPktBase::buildPacketSize(): wrong size. Called from %s\n", symBuf);
	}
#endif
	// make sure the actual packet size matches the definition for this packet version
	assert(packetSize == definedPktSize());
}

byte* LtIpPktBase::build(byte* pStartPoint)
{
	if (secure && !pktIsEchVer1())
		protocolFlags |= PROTFLAG_SECURITY;
	return(LtIpPktHeader::build(pStartPoint));
}

boolean	LtIpPktBase::parse(byte* pStart, boolean bRewrite)
{
	boolean ret;

	ret = LtIpPktHeader::parse(pStart, bRewrite);
	if (!pktIsEchVer1() && (protocolFlags & PROTFLAG_SECURITY))
		secure = true;
	return(ret);
}

void LtIpPktBase::dump()
{
	LtIpPktHeader::dump();
	vxlReportEvent("LtIpPktBase::dump - echPktVersion %d\n", echPktVersion);
}
//
// dumpLtData
//
// Dump the lontalk data in a packet
//
static void	dumpLtData( byte* pData, int nLen )
{
	int		i;
	char	line[256];
	char	item[16];
	strcpy( line, "        " );

	for ( i=0; i< nLen; i++ )
	{
		sprintf( item, "%02x ", pData[i] );
		strcat( line, item );
		if ( i != 0 && ((i%32)== 0) )
		{	strcat( line, "\n" );
			vxlReportEvent( line );
			strcpy( line, "        " );
		}
	}
	if ( strlen(line) )
	{	strcat( line, "\n" );
		vxlReportEvent( line );
	}
}


//
// dumpLtPacket
//
//
void LtIpPktHeader::dumpLtPacket( LPCSTR tag, byte* pData, int nLen )
{
	vxlReportEvent( "LtIpDataPacket::dump - %s\n", tag);
	dumpLtData( pData, nLen );
}


//
// dumpPacket
//
//
void	LtIpPktHeader::dumpPacket( LPCSTR tag, byte* pData, int nLen, boolean bHeaderOnly )
{
	if ( bHeaderOnly )
	{
		vxlReportEvent(
			"LtIpPktHeader::dumpPacket - headerOnly %s\n", tag );
		parse( pData, false );
		dump();
	}
	else
	{
		parse( pData, false );
		vxlReportEvent(
			"LtIpPktHeader::dumpPacket - fullPacket %s\n", tag );
		switch ( packetType )
		{
		case	PKTTYPE_DATA:			// data packet
				vxlReportEvent(
					"LtIpDataPacket::dump - \n");
			//parse( pData, false );
			dump();
			dumpLtData( pRemaining, packetSize - hdrSize() );
			break;
		case	PKTTYPE_DEVREGISTER:		// device registration packet
		case	PKTTYPE_DEVCONFIGURE:		// device configuration packet
			{
				LtIpDevRegister	dvr;
				dvr.parse( pData, false );
				dvr.dump();
				break;
			}
		case 	PKTTYPE_CHNMEMBERS:		// channel membership packet
			{
				LtIpChanMembers chm;
				chm.parse( pData, false );
				chm.dump();
				break;
			}
		case 	PKTTYPE_CHNROUTING:		// channel routing packet
			{
				LtIpChanRouting	chr;
				chr.parse( pData, false );
				chr.dump();
				break;
			}
		case 	PKTTYPE_SENDLIST:		// send list packet
			{
				LtIpSendList	snd;
				snd.parse( pData, false );
				snd.dump();
				break;
			}
		case	PKTTYPE_STATISTICS:		// statistics packet
			{
				LtIpPktStats	stats;
				stats.parse( pData, false );
				stats.dump();
				break;
			}
		case 	PKTTYPE_RESPONSE:		// response packet [ Ack ]
			{
				LtIpResponse	rsp;
				rsp.parse( pData, false );
				rsp.dump();
				break;
			}
		case 	PKTTYPE_REQCHNMEMBERS:	// request channel membership
		case 	PKTTYPE_REQSENDLIST:	// request send list
		case 	PKTTYPE_REQDEVCONFIG:	// request device configuration
		case 	PKTTYPE_REQCHNROUTING:	// request channel routing
		case	PKTTYPE_REQSTATISTICS:	// request statistics
			{
				LtIpRequest	req;
				req.parse( pData, false );
				req.dump();
				break;
			}
		default:
			{	vxlReportEvent(
					"LtIpPktHeader::dump - Invalid Packet\n" );
			parse( pData, false );
			dump();
			break;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Neuron Id
//
void LtIpNeuronId::dump()
{
	vxlReportEvent(
		"        LtIpNeuronId  0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ",
		id[0], id[1], id[2], id[3], id[4], id[5]
		);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Device registration packet
//
LtIpDevRegister::LtIpDevRegister()
{
	packetType			= PKTTYPE_DEVREGISTER;
	dateTime			= 0;
	ipFlags				= 0;
	routerType			= 0;
	lonTalkFlags		= 0;
	ipNodeType			= 0;

	mcAddressCount		= 0;
	mbz					= 0;
	channelTimeout		= 0;

	neuronIdBytes		= 0;
	ipUnicastPort		= 0;

	ipUcAddress			= 0;
	chanMemDatetime		= 0;
	sendlistDatetime	= 0;
	ipAddressCS			= 0;		// configuation server IP address
	ipAddressTS			= 0;		// NTP time server IP address
	ipAddressTS2			= 0;		// NTP time server IP address
	ipPortCS			= 0;		// Config Server IP port
	ipPortTS			= 0;		// Time Server IP port
	ipPortTS2			= 0;		// Time Server IP port
	mbz2				= 0;

	nameLen				= 0;
	pMcAddresses		= NULL;
	pNeuronIds			= NULL;
	pName				= NULL;
}
LtIpDevRegister::~LtIpDevRegister()
{
}

//
// parse
//
// parse a packet, requires that the header already be parsed.
//
boolean		LtIpDevRegister::parse( byte* pStart, boolean bRewrite )
{
	boolean		bOk = false;
	int				i;
	byte*			p2;
	byte*			p3;
	LtIpAddPort		iap;

	boolean bHeaderOk = LtIpPktBase::parse( pStart, bRewrite );

	byte*		p = pRemaining;

	if ( bHeaderOk && (packetSize >= fixedPktSize()) &&
		( packetType == PKTTYPE_DEVREGISTER ||
		  packetType == PKTTYPE_DEVCONFIGURE ))
	{
		PTOHL(p, dateTime );

		PTOHB(p, ipFlags );
		PTOHB(p, routerType );
		PTOHB(p, lonTalkFlags );
		PTOHB(p, ipNodeType );

		PTOHB(p, mcAddressCount );
		PTOHB(p, mbz );
		PTOHS(p, channelTimeout );

		PTOHS(p, neuronIdBytes );
		PTOHS(p, ipUnicastPort );

		PTOHL(p, ipUcAddress );
		PTOHL(p, chanMemDatetime );
		PTOHL(p, sendlistDatetime );

		PTOHL(p, ipAddressCS );			// configuation server IP address
		PTOHL(p, ipAddressTS );			// NTP time server IP address
		PTOHL(p, ipAddressTS2 );		// NTP time server IP address
		PTOHS(p, ipPortCS );			// Config Server IP port
		PTOHS(p, ipPortTS );			// Time Server IP port
		PTOHS(p, ipPortTS2 );			// Time Server IP port
		PTOHS(p, mbz2 );				// mbz

		// Now that we know the fixed data, check to make sure
		// there is enough data to parse the dynamic data
		if (packetSize >= definedPktSize())
		{
			pMcAddresses = p;
			p2 = p;
			p3 = p;
			for ( i=0; i< mcAddressCount; i++ )
			{
				p2 = iap.parse( p2 );
				if ( bRewrite ) p3 = iap.store( p3 );
			}

			p += mcAddressCount*LtIpAddPort::SIZE;
			pNeuronIds = p;
			p += neuronIdBytes;
			PTOHB(p, nameLen );
			pName = p;

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
byte*		LtIpDevRegister::build( byte* pStartPoint )
{
	byte*			p = pStartPoint;
	int				i;
	byte*			p2;
	byte*			p3;
	LtIpAddPort		iap;

	p = LtIpPktBase::build( p );
	PTONL(p, dateTime );

	PTONB(p, ipFlags );
	PTONB(p, routerType );
	PTONB(p, lonTalkFlags );
	PTONB(p, ipNodeType );

	PTONB(p, mcAddressCount );
	PTONB(p, mbz );
	PTONS(p, channelTimeout );

	PTONS(p, neuronIdBytes );
	PTONS(p, ipUnicastPort );

	PTONL(p, ipUcAddress );
	PTONL(p, chanMemDatetime );
	PTONL(p, sendlistDatetime );

	PTONL(p, ipAddressCS );			// configuation server IP address
	PTONL(p, ipAddressTS );			// NTP time server IP address
	PTONL(p, ipAddressTS2 );			// NTP time server IP address
	PTONS(p, ipPortCS );			// Config Server IP port
	PTONS(p, ipPortTS );			// Time Server IP port
	PTONS(p, ipPortTS2 );			// Time Server IP port
	PTONS(p, mbz2 );				// mbz

	p2 = pMcAddresses;
	p3 = p;
	pMcAddresses = p3;
	for ( i=0; i< mcAddressCount; i++ )
	{
		p2 = iap.fetch( p2 );
		p3 = iap.build( p3 );
	}
	p = p3;
	p2 = pNeuronIds;
	for ( i=0; i< neuronIdBytes; i++ )
	{
		PTONB( p, *p2 ); p2++;
	}
	pNeuronIds = p3;
	PTONB(p, nameLen );
	p3 = p;
	p2 = pName;
	for ( i=0; i< nameLen; i++ )
	{
		PTONB( p, *p2 ); p2++;
	}
	pName = p3;
	buildPacketSize( p );
	return p;
}

//
// dump
//
// dump the packet for debugging
//
void LtIpDevRegister::dump()
{
	int i;
	LtIpAddPort		iap;
	LtIpNeuronId	nid;
	byte*			p;
	LtIpDateTimeStr	dts;
	LtIpDateTimeStr	dts2;
	LtIpAddressStr	ias;
	LtIpAddressStr	ias2;
	LtIpAddressStr	ias3;
	char			name[130];
	int				nNameLen;

	LtIpPktHeader::dump();
	vxlReportEvent( "LtIpDevRegister::dump - dateTime %s ipFlags %d routerType %d ltFlags %d ipNodeType %d\n"
					"                      mcAddressCount %d mbz %d channel Timeout %d ms\n",
					dts.getString(dateTime), ipFlags, routerType, lonTalkFlags, ipNodeType,
					mcAddressCount, mbz, channelTimeout );
	vxlReportEvent( "                      neuronIdBytes %d ipUnicastPort %d ipUcAddress %s\n"
					"                      chanmem DateT %s sendList DateT %s\n",
					neuronIdBytes, ipUnicastPort, ias.getString( ipUcAddress ),
					dts.getString(chanMemDatetime), dts2.getString(sendlistDatetime) );
	vxlReportEvent( "                      Config Server         %s port %d\n"
					"                      Primary Time Server   %s port %d\n"
					"                      Secondary Time Server %s port %d\n",
					ias.getString( ipAddressCS ), ipPortCS,
					ias2.getString( ipAddressTS ), ipPortTS,
					ias3.getString( ipAddressTS2 ), ipPortTS2 );
	nNameLen = min( 128, nameLen);
	strncpy( name, (char*)pName, nNameLen );
	name[nNameLen] = 0;
	vxlReportEvent( "                      pMcAddresses 0x%08x pNeuronIds 0x%08x nameLen %d pName 0x%08x %s\n",
					pMcAddresses, pNeuronIds, nameLen, pName, name );
	p = (byte*)pMcAddresses;
	for (i=0; i<mcAddressCount; i++ )
	{
		p = iap.parse( p );
		iap.dump();
	}
	p = (byte*)pNeuronIds;
	if ( p )
	{
		for (i=0; i<neuronIdBytes; i+=LtIpNeuronId::SIZE )
		{
			p = nid.parse( p );
			nid.dump();
		}
	}
	else
	{
		vxlReportEvent( "                      Unable to dump Unique IDs\n");
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Channel Membership packet
//
LtIpChanMembers::LtIpChanMembers()
{
	dateTime = 0;
	sendListDateTime = 0;
	mbz = 0;
	listSize = 0;
	mbz10 = 0;
	pUcAddresses = NULL;
}
LtIpChanMembers::~LtIpChanMembers()
{
}

//
// parse
//
// parse a packet, requires that the header already be parsed.
//
boolean		LtIpChanMembers::parse( byte* pStart, boolean bRewrite )
{
	boolean		bOk = false;

	boolean bHeaderOk = LtIpPktBase::parse( pStart, bRewrite );

	byte*			p = pRemaining;
	int				i;
	byte*			p2;
	byte*			p3;
	LtIpAddPortDate	iap;

	if ( bHeaderOk && (packetSize >= fixedPktSize()) &&
		( packetType == PKTTYPE_CHNMEMBERS ))
	{
		PTOHL(p, dateTime );
		PTOHL(p, sendListDateTime );
		PTOHL(p,  mbz);  // this was deviceRegDateTime

		PTOHS(p, listSize );
		PTOHS(p, mbz10 );

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
byte*		LtIpChanMembers::build( byte* pStartPoint )
{
	byte*			p = pStartPoint;
	int				i;
	byte*			p2;
	byte*			p3;
	LtIpAddPortDate	iap;

	packetType = PKTTYPE_CHNMEMBERS;
	p = LtIpPktBase::build( p );
	PTONL(p, dateTime );
	PTONL(p, sendListDateTime );
	// Force MBZ field
	if (!pktIsEchVer1())
		mbz = 0;
	PTONL(p, mbz );		// this was deviceRegDateTime

	PTONS(p, listSize );
	PTONS(p, mbz10 );

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
void LtIpChanMembers::dump()
{
	int i;
	LtIpAddPortDate	iap;
	byte*			p;
	LtIpDateTimeStr dts;
	LtIpDateTimeStr dts1;

	LtIpPktHeader::dump();
	vxlReportEvent( "LtIpChanMembers::dump - dateTime %s \n"
					"                        sendListDateTime %s mbz/deviceRegDateTime %d\n"
					"                        listSize %d mbz %d\n",
					dts.getString(dateTime), dts1.getString(sendListDateTime),
					mbz, listSize, mbz10 );
	p = pUcAddresses;
	for (i=0; i<listSize; i++ )
	{
		p = iap.parse( p );
		iap.dump();
	}
}

///////////////////////////////////////////////////////////////////////////////
// IP address and port
//
//
//
void LtIpAddPort::dump()
{
	LtIpAddressStr	ias;

	vxlReportEvent( "         LtIpAddPort  ipaddress %s ipPort %d mbz %d",
					ias.getString(ipAddress), ipPort, mbz );
}

///////////////////////////////////////////////////////////////////////////////
// IP address and port with Date
//
//
//
void LtIpAddPortDate::dump()
{
	LtIpAddressStr	ias;
	LtIpDateTimeStr	dts;

	vxlReportEvent( "         LtIpAddPort  ipaddress %s ipPort %d mbz %d\n"
					"                      dateTime %s\n",
					ias.getString(ipAddress), ipPort, mbz, dts.getString(dateTime) );
}


///////////////////////////////////////////////////////////////////////////////
// IP address
//
//
//
void LtIpAddress::dump()
{
	LtIpAddressStr	ias;

	vxlReportEvent( "         LtIpAddress  ipaddress %s 0x%08x",
					ias.getString(ipAddress), ipAddress );
}



///////////////////////////////////////////////////////////////////////////////
// LtIpSubnetNode
//
//
//
void LtIpSubnetNode::dump()
{

	vxlReportEvent( "      LtIpSubnetNode  subnet %d node %d domainIdx %d neuronIdx %d",
											subnet, node, domainIndex, neuronIdIndex );

}

///////////////////////////////////////////////////////////////////////////////
// LtIpDomain
//
//
//
void LtIpDomain::dump()
{
	//byte*	sn = &subnetMask[0];
	//byte*	gm = &groupMask[0];
	byte*	dm = &domainBytes[0];
	char	line[500];
	char	line2[100];
	const char*	sep;
	int		i;
	int		j;

	line[0] = 0;
	//sep = "                       subnetMask ";
	sep = "           LtIpDomain  subnetMask ";
	strcpy(line, sep);
	for ( j=0; j < (int)sizeof(subnetMask); j++ )
	{
		sprintf( line2, "%02x ", subnetMask[j] );
		strcat( line, line2 );
	}
	vxlReportEvent( line );
	sep = "                        groupMask ";
	strcpy(line, sep);
	for ( i=0; i < (int)sizeof(groupMask); i++ )
	{
		sprintf( line2, "%02x ", groupMask[i] );
		strcat( line, line2 );
	}
	vxlReportEvent( line );

	// trim the domain to exactly what we are supposed to see.
	// Pretty strange for debugging code, but somebody wanted it.
	line[0] = 0;
	// be defensive
	int domLen = min( domainLength,6);
	for ( i=0; i<domLen; i++ )
	{
		sprintf( line2, "%02x ", dm[i] );
		strcat( line, line2 );
	}

	//              "                                  ";
	vxlReportEvent( "                           domain len %2d mbz %02x %s", domainLength, mbz, line );


}


///////////////////////////////////////////////////////////////////////////////
// LtIpChanRouting
//
//
//
//
// parse
//
// parse a packet.
//
boolean		LtIpChanRouting::parse( byte* pStart, boolean bRewrite )
{
	boolean		bOk = false;

	boolean bHeaderOk = LtIpPktBase::parse( pStart, bRewrite );

	byte*			p = pRemaining;
	byte*			p2;
	byte*			p3;
	LtIpSubnetNode	snn;

	if ( bHeaderOk && (packetSize >= fixedPktSize()) &&
		( packetType == PKTTYPE_CHNROUTING ))
	{
		PTOHL(p, dateTime );

		PTOHS(p, ipMcPort );
		PTOHS(p, ipUcPort );
		PTOHL(p, ipMcAddress );
		PTOHL(p, ipUcAddress );

		PTOHB(p, ipFlags );
		PTOHB(p, routerType );
		PTOHB(p, lonTalkFlags );
		PTOHB(p, ipNodeType );

		PTOHS(p, neuronIdBytes );
		PTOHS(p, mbz );

		PTOHS(p, subnetNodeBytes );
		PTOHS(p, domainBytes );


		// Now that we know the fixed data, check to make sure
		// there is enough data to parse the dynamic data
		if (packetSize >= definedPktSize())
		{
			pNeuronIds = p;
			// host and network order for neuronIds is the same

			p += neuronIdBytes;
			pSubnetNodes = p;

			p2 = p;
			p3 = p;
			p += subnetNodeBytes;
			// parse the bytes into host order
			while ( p2 < p )
			{
				p2 = snn.parse( p2 );
				if ( bRewrite ) p3 = snn.store( p3 );
			}

			pDomains = p;
			// host and network order for domains is the same.
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
byte*		LtIpChanRouting::build( byte* pStartPoint )
{
	byte*			p = pStartPoint;
	byte*			p2;
	byte*			p3;
	//LtIpNeuronId	nid;
	LtIpSubnetNode	snn;
	//LtIpDomain		dom;

	packetType = PKTTYPE_CHNROUTING;
	p = LtIpPktBase::build( p );
	PTONL(p, dateTime );

	PTONS(p, ipMcPort );
	PTONS(p, ipUcPort );
	PTONL(p, ipMcAddress );
	PTONL(p, ipUcAddress );

	PTONB(p, ipFlags );
	PTONB(p, routerType );
	PTONB(p, lonTalkFlags );
	PTONB(p, ipNodeType );

	PTONS(p, neuronIdBytes );
	PTONS(p, mbz );

	PTONS(p, subnetNodeBytes );
	PTONS(p, domainBytes );
	p2 = p + neuronIdBytes;
	p3 = pNeuronIds;
	pNeuronIds = p;
	while ( p < p2 )
	{
		*p++ = *p3++;		// assume no host / network order change
	}
	p2 = p + subnetNodeBytes;
	p3 = pSubnetNodes;
	pSubnetNodes = p;
	while ( p < p2 )
	{
		p3 = snn.fetch( p3 );	// fetch in host order
		p = snn.build( p );		// build into network order
	}

	p2 = p + domainBytes;
	p3 = pDomains;
	pDomains = p;
	while ( p < p2 )
	{
		*p++ = *p3++;		// assume no host / network order change
	}

	buildPacketSize( p );
	return p;
}

//
// dump
//
// dump the packet for debugging
//
void LtIpChanRouting::dump()
{
	byte*			p2;
	byte*			p3;
	LtIpNeuronId	nid;
	LtIpSubnetNode	snn;
	LtIpDomain		dom;
	LtIpDateTimeStr	dts;
	LtIpAddressStr	ips1;
	LtIpAddressStr	ips2;

	LtIpPktHeader::dump();
	vxlReportEvent( "LtIpChanRouting::dump - dateTime %s ipMcPort %d ipUcPort %d\n"
					"                        ipMcAddress %s ipUcAddress %s\n",
					dts.getString(dateTime), ipMcPort, ipUcPort,
					ips2.getString(ipMcAddress), ips1.getString(ipUcAddress) );

	vxlReportEvent( "                        ipFlags %d routerType %d ltFlags %d ipNodeType %d\n"
					"                        uidBytes %d mbz %d snnBytes %d domBytes %d\n",
					ipFlags, routerType, lonTalkFlags, ipNodeType,
					neuronIdBytes, mbz, subnetNodeBytes, domainBytes );

	p3 = pNeuronIds;
	p2 = p3 + neuronIdBytes;
	while ( p3 < p2 )
	{
		p3 = nid.parse(p3 );	// net and host order the same
		nid.dump();
	}
	p3 = pSubnetNodes;
	p2 = p3 + subnetNodeBytes;
	while ( p3 < p2 )
	{
		p3 = snn.parse( p3 );	// parse into host order
		snn.dump( );
	}

	p3 = pDomains;
	p2 = p3 + domainBytes;

	while ( p3 < p2 )
	{
		p3 = dom.parse( p3 );	// net and host order the same
		dom.dump();
	}

}



///////////////////////////////////////////////////////////////////////////////
// Response packet
//
//
//
//
// parse
//
// parse a packet.
//
boolean		LtIpResponse::parse( byte* pStart, boolean bRewrite )
{
	boolean		bOk = false;

	boolean bHeaderOk = LtIpPktBase::parse( pStart, bRewrite );
	byte*			p = pRemaining;

	if ( bHeaderOk && (packetSize >= fixedPktSize()) &&
		( packetType == PKTTYPE_RESPONSE ))
	{
		PTOHL(p, dateTime );

		PTOHB(p, type );
		PTOHS(p, requestId );
		PTOHB(p, segmentId );
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
byte*		LtIpResponse::build( byte* pStartPoint )
{
	byte*			p = pStartPoint;

	p = LtIpPktBase::build( p );
	PTONL(p, dateTime );

	PTONB(p, type );
	PTONS(p, requestId );
	PTONB(p, segmentId );

	buildPacketSize( p );
	return p;
}

//
// dump
//
// dump the packet for debugging
//
void LtIpResponse::dump()
{
	LtIpDateTimeStr	dts;

	LtIpPktHeader::dump();
	vxlReportEvent( "LtIpResponse::dump - dateTime %s type %d requestId %d segmentId %d\n",
					dts.getString(dateTime), type, requestId, segmentId );
}


///////////////////////////////////////////////////////////////////////////////
// Request packet
//
//
//
// class LtIpRequest : public LtIpPktHeader
//
//
// parse
//
// parse a packet.
//
boolean		LtIpRequest::parse( byte* pStart, boolean bRewrite )
{
	boolean		bOk = false;

	boolean bHeaderOk = LtIpPktBase::parse( pStart, bRewrite );
	byte*			p = pRemaining;

	if ( bHeaderOk && (packetSize >= fixedPktSize()) &&
		( packetType == PKTTYPE_REQCHNMEMBERS ||
		  packetType == PKTTYPE_REQSENDLIST ||
		  packetType == PKTTYPE_REQDEVCONFIG ||
		  packetType == PKTTYPE_REQCHNROUTING ||
		  packetType == PKTTYPE_REQSTATISTICS ||
		  // Echelon-specific requests
		  packetType == PKTTYPE_TIMESYNCHREQ ||
		  packetType == PKTTYPE_ECHCONFIGREQ ||
		  packetType == PKTTYPE_ECHVERSREQ ||
		  packetType == PKTTYPE_ECHMODEREQ ||
		  packetType == PKTTYPE_ECHDEVIDREQ
		  )
		)
	{
		PTOHL(p, dateTime );

		PTOHB(p, reason );
		PTOHS(p, requestId );
		PTOHB(p, segmentId );
		PTOHL(p, sinceDateTime );
		PTOHL(p, ipUcAddress );

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
byte*		LtIpRequest::build( byte* pStartPoint )
{
	byte*			p = pStartPoint;

	p = LtIpPktBase::build( p );
	PTONL(p, dateTime );

	PTONB(p, reason );
	PTONS(p, requestId );
	PTONB(p, segmentId );
	PTONL(p, sinceDateTime );
	PTONL(p, ipUcAddress );

	buildPacketSize( p );
	return p;
}

//
// dump
//
// dump the packet for debugging
//
void LtIpRequest::dump()
{
	LtIpDateTimeStr	dts1;
	LtIpDateTimeStr	dts2;
	LtIpAddressStr	ias;

	LtIpPktHeader::dump();
	vxlReportEvent( "LtIpRequest::dump - dateTime %s reason %d requestId %d segmentId %d sinceDateTime %s ipUcAddress %s\n",
					dts1.getString(dateTime), reason, requestId, segmentId, dts2.getString(sinceDateTime),
					ias.getString(ipUcAddress) );
}


///////////////////////////////////////////////////////////////////////////////
// SendList packet
//
//
//
// class LtIpSendList : public LtIpPktHeader


//
// parse
//
// parse a packet, requires that the header already be parsed.
//
boolean		LtIpSendList::parse( byte* pStart, boolean bRewrite )
{
	boolean		bOk = false;
	byte*		p2;
	byte*		p3;
	LtIpAddPort	iap;
	LtIpAddress	ipa;
	int			i;

	boolean bHeaderOk = LtIpPktBase::parse( pStart, bRewrite );

	byte*		p = pRemaining;
	byte*		pend = pPacket + packetSize;

	if ( bHeaderOk && (packetSize >= fixedPktSize()) &&
		( packetType == PKTTYPE_SENDLIST))
	{
		PTOHL(p, dateTime );

		if (pktIsEchVer1())
		{
			do	// to allow break on error
			{
					// Old format
				PTOHS(p, mcBytes );
				PTOHS(p, ucBytes );
				PTOHS(p, ipUcPort );
				PTOHS(p, mbz2 );	// two bytes


				pMcAddresses = p;
				if ( (p + mcBytes) > pend ) break; // error in packet

				p2 = p;
				p3 = p;
				p  =  p + mcBytes;
				while ( p3 < p )
				{
					p2 = iap.parse( p2 );
					if ( bRewrite ) p3 = iap.store( p3 );
				}

				pUcAddresses = p;
				if ( (p + ucBytes) > pend ) break; // error in packet

				p2 = p;
				p3 = p;
				p  =  p + ucBytes;
				while ( p3 < p )
				{
					p2 = ipa.parse( p2 );
					if ( bRewrite ) p3 = ipa.store( p3 );
				}

				bOk = true;
			} while (FALSE);
		}
		else
		{
			// New format
			PTOHB(p, ipAddrCount );
			PTOHB(p, mbz1 );	// one byte
			PTOHS(p, mbz2 );	// two bytes

			pAddresses = p;
			p2 = p;
			p3 = p;
			for (i = 0; i < ipAddrCount; i++)
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
byte*		LtIpSendList::build( byte* pStartPoint )
{
	byte*			p = pStartPoint;
	byte*			p2;
	byte*			p3;
	LtIpAddPort		iap;
	LtIpAddress		ipa;
	int				i;

	packetType = PKTTYPE_SENDLIST;
	p = LtIpPktBase::build( p );
	PTONL(p, dateTime );

	if (pktIsEchVer1())
	{
		// Old format
		PTONS(p, mcBytes );
		PTONS(p, ucBytes );
		PTONS(p, ipUcPort );
		PTONS(p, mbz2 );	// two bytes

		p2 = pMcAddresses;
		p3 = p;
		pMcAddresses = p3;
		p = p + mcBytes;
		while ( p3 < p )
		{
			p2 = iap.fetch( p2 );
			p3 = iap.build( p3 );
		}
		p2 = pUcAddresses;
		p3 = p;
		pUcAddresses = p3;
		p = p + ucBytes;
		while ( p3 < p )
		{
			p2 = ipa.fetch( p2 );
			p3 = ipa.build( p3 );
		}
	}
	else
	{
		// New format
		PTONB(p, ipAddrCount );
		PTONB(p, mbz1 );	// one byte
		PTONS(p, mbz2 );	// two bytes

		p2 = pAddresses;
		p3 = p;
		pAddresses = p3;
		for (i = 0; i < ipAddrCount; i++)
		{
			p2 = iap.fetch( p2 );
			p3 = iap.build( p3 );
		}
	}

	buildPacketSize( p3 );
	return p3;
}

//
// dump
//
// dump the packet for debugging
//
void LtIpSendList::dump()
{
	LtIpAddPort		iap;
	LtIpAddress		ipa;
	byte*			p;
	byte*			p2;
	LtIpDateTimeStr	dts;
	int				i;

	LtIpPktHeader::dump();
	if (pktIsEchVer1())
	{
		// Old format
		vxlReportEvent( "LtIpSendList::dump   - dateTime %s mcBytes %d ucBytes %d ipUcPort %d mbz %d\n",
						dts.getString(dateTime), mcBytes, ucBytes, ipUcPort, mbz2 );
		p = pMcAddresses;
		p2 = p + mcBytes;
		while ( p < p2 )
		{
			p = iap.parse( p );		// parse into host order
			iap.dump();
		}
		p = pUcAddresses;
		p2 = p + ucBytes;
		while ( p < p2 )
		{
			p = ipa.parse( p );		// parse into host order
			ipa.dump();
		}
	}
	else
	{
		// New format
		vxlReportEvent( "LtIpSendList::dump   - dateTime %s ipAddrCount %d mbz1 %d mbz2 %d\n",
						dts.getString(dateTime), ipAddrCount, mbz1, mbz2 );
		p = pAddresses;
		for (i = 0; i < ipAddrCount; i++)
		{
			p = iap.parse( p );		// parse into host order
			iap.dump();
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Statistics packet
//
//
//
// class LtIpPktStats : public LtIpPktHeader


//
// parse
//
// parse a packet.
//
boolean		LtIpPktStats::parse( byte* pStart, boolean bRewrite )
{
	boolean		bOk = false;
	int			i;

	boolean		bHeaderOk = LtIpPktBase::parse( pStart, bRewrite );

	byte*		p = pRemaining;

	// In ver 1, this packet was smaller than the standard one
	if (packetSize < echV2FixedSize())
	{
		setEchVersion(LTIP_ECH_VER1);
	}

	if ( bHeaderOk && (packetSize >= fixedPktSize()) &&
		( packetType == PKTTYPE_STATISTICS)
		)
	{
		for ( i = 0; i < LtV1Stats_Size; i++ )
		{
			PTOHL(p, stats[i] );
		}

		if (!pktIsEchVer1())
		{
			// There are two more fields for the EIA-852 packet
			for ( /* from above loop */; i < LtV2Stats_Size; i++ )
			{
				PTOHL(p, stats[i] );
			}
		}

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
byte*		LtIpPktStats::build( byte* pStartPoint )
{
	byte*			p = pStartPoint;
	int				i;

	packetType = PKTTYPE_STATISTICS;
	p = LtIpPktBase::build( p );
	for ( i=0; i< LtV1Stats_Size; i++ )
	{
		PTONL(p, stats[i] );
	}
	if (!pktIsEchVer1())
	{
		// There are two more fields for the EIA-852 packet
		for ( /* from above loop */; i < LtV2Stats_Size; i++ )
		{
			PTONL(p, stats[i] );
		}
	}
	buildPacketSize( p );
	return p;
}

//
// dump
//
// dump the packet for debugging
//
void LtIpPktStats::dump()
{
	LtIpDateTimeStr	dts;

	LtIpPktHeader::dump();
	vxlReportEvent( "LtIpPktStats::dump   - secondsSinceReset %d dateTimeReset %s members %d\n"
					"                       ...\n",
					stats[LtStats_secondsSinceReset], dts.getString(LtStats_dateTimeReset),
					stats[LtStats_members] );
}


///////////////////////////////////////////////////////////////////////////////
// LtIpAddressStr
//
// Convert LtIpAddress to and from a string.
//
// form of string is ddd.ddd.ddd.ddd
//

void	LtIpAddressStr::setString( char* pStr )
{
	assert ( strlen(pStr) < STRLEN );
	strcpy( m_string, pStr );
	m_bClean = false;
	m_iaddr = getIaddr();
}

ULONG	LtIpAddressStr::getIaddr()
{
	ULONG	nP[4] = {0,0,0,0};
	ULONG	nIad = 0;

	if ( ! m_bClean )
	{
		sscanf( m_string, "%ld.%ld.%ld.%ld",
			&nP[0],  &nP[1],  &nP[2],  &nP[3] );
		nIad = (((((nP[0] << 8)+ nP[1] ) << 8 ) + nP[2] ) << 8 ) + nP[3];
		m_iaddr = nIad;
		m_bClean = true;
	}
	return m_iaddr;
}

ULONG	LtIpAddressStr::getIaddr( char* pStr )
{
	setString( pStr );
	return getIaddr();
}

char*	LtIpAddressStr::getString()
{
	if ( !m_bClean )
	{
		ULONG	nP[4];
		ULONG	nIad = m_iaddr;
		int		i;
		for ( i=3; i>=0; i-- )
		{
			nP[i] = nIad & 0xFF;
			nIad = nIad >> 8;
		}
		sprintf( m_string, "%0ld.%0ld.%0ld.%0ld", nP[0],nP[1],nP[2],nP[3] );
		m_bClean = true;
	}
	return m_string;
}


char*	LtIpAddressStr::getString( ULONG iad )
{
	setIaddr( iad );
	return getString();
}


///////////////////////////////////////////////////////////////////////////////
// LtIpDateTimeStr
//
// Convert dateTime to a string.
//
// form of string is standard unix format of DDD MMM dd yyyy hh:mm:ss
// DDD is day of week, MMM is month, dd is day of month, yyyy is year, etc.
//
char*	LtIpDateTimeStr::getString()
{
	if ( !m_bClean )
	{
		dateTimeToStr( m_dateTime, m_string, STRLEN );
		m_bClean = true;
	}
	return m_string;
}
char*	LtIpDateTimeStr::getString( ULONG dateTime )
{
	setDateTime( dateTime );
	return getString();
}
// end

