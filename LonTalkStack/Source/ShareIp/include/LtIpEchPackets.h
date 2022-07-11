#ifndef _LTIPECHPACKETS_H
#define _LTIPECHPACKETS_H

/***************************************************************
 *  Filename: LtIpEchPackets.h
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
 *  Description:  Header file for Echelon private LonTalk IP 
 *					Packet formats.
 *
 *	DJ Duffy July 1999
 *
 ****************************************************************/

/*
 * $Log: /Dev/ShareIp/include/LtIpEchPackets.h $
 * 
 * 19    9/18/07 3:56p Fremont
 * 46427 - Rename product to "i.LON SmartServer". This is too long for the
 * product name field in the packet, so we need to terminate the truncated
 * name.
 * 
 * 18    11/05/04 3:35p Fremont
 * iLON 100 PPP router demo
 * 
 * 17    9/03/03 5:36p Fremont
 * Implement extended packet header for EPR 29618.
 * 
 * 16    6/20/03 2:30p Fremont
 * add field to EchMode pkt for local config state
 * 
 * 15    6/03/03 7:17p Fremont
 * more EIA-852 packet transformations, general cleanup
 * 
 * 14    6/02/03 10:36a Fremont
 * move NAT IP addr from Ech Config pkt to Ech Mode pkt
 * 
 * 13    5/27/03 2:34p Fremont
 * Rename field
 * 
 * 12    5/09/03 1:45p Fremont
 * comments
 * 
 * 11    3/25/03 1:11p Fremont
 * adjusted size calculations/literals
 * 
 * 10    3/25/03 12:42p Fremont
 * add Echelon extended channel routing request packet, remove config
 * server conditional compilation
 * 
 * 9     3/04/03 3:43p Fremont
 * updated config and version pkts to ver 2, add mode pkt
 * 
 * 8     2/21/03 11:51a Fremont
 * transform code compiled for a specific platform to used generic
 * functions from LtIpPlatform
 * 
 * 7     2/11/03 11:15a Fremont
 * Use proper version macros
 * 
 * 6     3/24/00 9:26a Darrelld
 * Fix version  for correct decimal operation
 * 
 * 5     3/17/00 5:09p Darrelld
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
 * 
 */

#include <LtIpPackets.h>
#include <LtIpPlatform.h>

enum
{
	PKTTYPE_TIMESYNCHREQ		= 0xf1,		// request a time synch
	PKTTYPE_TIMESYNCHRSP		= 0xf2,		// respond with a time synch
	PKTTYPE_ECHCONFIG			= 0xf3,		// echelon private configuration
	PKTTYPE_ECHCONFIGREQ		= 0xf4,		// request current configuration-LtIpRequest format
	PKTTYPE_ECHCONTROL			= 0xf5,		// control packet
	PKTTYPE_ECHVERSION			= 0xf6,		// version packet
	PKTTYPE_ECHVERSREQ			= 0xf7,		// version request packet
	PKTTYPE_ECHMODE				= 0xf8,		// mode packet (for backward compatibilty)
	PKTTYPE_ECHMODEREQ			= 0xf9,		// mode request packet
	PKTTYPE_ECHDEVID			= 0xfa,		// device ID packet
	PKTTYPE_ECHDEVIDREQ			= 0xfb,		// device ID request packet
};




///////////////////////////////////////////////////////////////////////////////
// TimeSynch packet
// 
// This packet is used by the configuration manager to determine the
// UTC time that the node thinks it is.
//
class LtIpTimeSynch : public LtIpPktBase
{
public:
	enum
	{	ECHV1_ADDED_SIZE = (4*sizeof(ULONG)),
		ECHV2_ADDED_SIZE = ECHV1_ADDED_SIZE,					// new V2 data size (same)
		MAX_PKT_SIZE = MAX_HDR_SIZE + ECHV2_ADDED_SIZE,
	};
	
	ULONG			dateTime;			// seconds since 1 Jan 1900 - of request
	ULONG			dateTimeRsp;		// of response
	ULONG			timeStampReq;		// Requesting timestamp
	ULONG			timeStampRsp;		// Response timestamp
	// - end direct copy

	LtIpTimeSynch()
	{									// PKTTYPE_TIMESYNCHREQ, PKTTYPE_TIMESYNCHRSP
		vendorCode = VENDOR_ECHELON;	// Vendor private for Echelon
		packetType = 0;					// set packet type manually
		dateTime = 0;
		dateTimeRsp = 0;
		timeStampReq = 0;
		timeStampRsp = 0;
	}
	virtual ~LtIpTimeSynch()
	{
		// nothing to do
	}

#if ECHV1_ADDED_SIZE != ECHV2_ADDED_SIZE
#error Must provide overrides for different packet sizes
#endif
	virtual int commonFixedSize() { return(hdrSize() + ECHV1_ADDED_SIZE); }

	// parse a packet, given the address of a parsed packet header
	boolean		parse( byte* pStartPoint, boolean bRewrite = true  );
	// build a packet of this type if it's all filled in
	// returns pointer beyond the packet built.
	byte*		build( byte* pStartPoint );

	void dump();

};

///////////////////////////////////////////////////////////////////////////////
// Echelon Config packet
// 
// This packet is used by the configuration manager to set parameters
// in iLon devices.
//
class LtIpEchConfig : public LtIpPktBase
{
public:
	enum
	{
		ECHCFG_AGGREGATE		= 1,		// enable aggregation
		ECHCFG_BWLIMIT			= 2,		// enable bandwidth limit
		ECHCFG_NOREORDER		= 4,		// disable packet reordering on receive
		ECHCFG_AUTHENTICATE		= 8,		// authenticate packets
		ECHCFG_USETOS			= 0x10,		// use TOS bits for type of service
		ECHCFG_USETZ			= 0x20,		// use Timezone string
		ECHCFG_TZSIZE			= 128,		// size of timezone string
		ECHV1_ADDED_SIZE = (6*sizeof(ULONG)) + ECHCFG_TZSIZE, // old V1 size
		ECHV2_ADDED_SIZE = ECHV1_ADDED_SIZE,	// new V2 data size (add... nothing? )
		MAX_PKT_SIZE = MAX_HDR_SIZE + ECHV2_ADDED_SIZE,
	};
	
	ULONG			dateTime;			// seconds since 1 Jan 1900 - of request or response
	ULONG			flags;				// flag bits
	ULONG			aggTimerMs;			// aggregation timer in milliseconds
	ULONG			bwLimitKbPerSec;	// bandwidth limitation in KB per sec
	ULONG			escrowTimerMs;		// receive reorder escrow timer in ms
										// must be less than channel timeout
										// good value is about 1/10 channel timeout
										// zero implies the same as the aggregation timer
	ULONG			TosBits;			// TOS bits for sockets
	char			szTimeZone[ECHCFG_TZSIZE]; // timezone string

	// - end direct copy
	boolean			bAggregate;			// aggregate
	boolean			bBwLimit;			// limit bandwidth
	boolean			bNoReorder;			// disable reordering
	boolean			bAuthenticate;		// authenticate packets
	boolean			bUseTosBits;		// use TOS bits for sockets
	boolean			bUseTZ;				// use Timezone

	LtIpEchConfig()
	{									// PKTTYPE_TIMESYNCHREQ, PKTTYPE_TIMESYNCHRSP
		vendorCode	= VENDOR_ECHELON;	// Vendor private for Echelon
		packetType	= PKTTYPE_ECHCONFIG;
		dateTime	= 0;
		flags		= 0;
		aggTimerMs	= 0;
		bwLimitKbPerSec = 0;
		escrowTimerMs = 0;
		TosBits = 0;
		szTimeZone[0] = 0;

		bAggregate			= false;		// aggregate
		bBwLimit			= false;		// limit bandwidth
		bNoReorder			= false;		// disable reordering
		bAuthenticate		= false;		// authenticate packets
		bUseTosBits			= false;		// use TOS bits for sockets
		bUseTZ				= false;		// use TimeZone

	}
	virtual ~LtIpEchConfig()
	{
		// nothing to do
	}

#if ECHV1_ADDED_SIZE != ECHV2_ADDED_SIZE
#error Must provide overrides for different packet sizes
#endif
	virtual int commonFixedSize() { return(hdrSize() + ECHV1_ADDED_SIZE); }

	// parse a packet, given the address of a parsed packet header
	boolean		parse( byte* pStartPoint, boolean bRewrite = false  );
	// build a packet of this type if it's all filled in
	// returns pointer beyond the packet built.
	byte*		build( byte* pStartPoint );

	void dump();

};

///////////////////////////////////////////////////////////////////////////////
// Echelon Config Request packet
// 
// This packet is used by the configuration manager to request parameters
// from iLon devices.
//
class LtIpReqEchConfig : public LtIpRequest
{
public:
	LtIpReqEchConfig()
	{
		packetType = PKTTYPE_ECHCONFIGREQ;
		vendorCode = VENDOR_ECHELON;
	}
};

///////////////////////////////////////////////////////////////////////////////
// Echelon Control packet
// 
// This packet is used by the configuration manager to control the device
//
class LtIpEchControl : public LtIpPktBase
{
public:
	enum
	{	ECHV1_ADDED_SIZE = (2*sizeof(ULONG)),		// old V1 data size
		ECHV2_ADDED_SIZE = ECHV1_ADDED_SIZE,	// new V2 data size (same)
		MAX_PKT_SIZE = MAX_HDR_SIZE + ECHV2_ADDED_SIZE,
		ECHCON_REBOOT			= 1,		// Reboot the device
		ECHCON_WEBSTOP			= 2,		// Stop web server
		ECHCON_WEBSTART			= 4,		// Start web server
	};
	
	ULONG			dateTime;			// seconds since 1 Jan 1900 - of request
	ULONG			flags;				// flag bits

	// - end direct copy
	boolean			bReboot;			// Reboot the device
	boolean			bStopWeb;			// Stop web server
	boolean			bStartWeb;			// Start web server

	LtIpEchControl()
	{
		vendorCode	= VENDOR_ECHELON;	// Vendor private for Echelon
		packetType	= PKTTYPE_ECHCONTROL;
		dateTime	= 0;
		flags		= 0;
		bReboot			= false;		// aggregate
		bStopWeb		= false;		// disable reordering
		bStartWeb		= false;		// authenticate packets

	}
	virtual ~LtIpEchControl()
	{
		// nothing to do
	}

#if ECHV1_ADDED_SIZE != ECHV2_ADDED_SIZE
#error Must provide overrides for different packet sizes
#endif
	virtual int commonFixedSize() { return(hdrSize() + ECHV1_ADDED_SIZE); }

	// parse a packet, given the address of a parsed packet header
	boolean		parse( byte* pStartPoint, boolean bRewrite = false  );
	// build a packet of this type if it's all filled in
	// returns pointer beyond the packet built.
	byte*		build( byte* pStartPoint );

	void dump();

};

///////////////////////////////////////////////////////////////////////////////
// Echelon Version packet
// 
// This packet is used to convey the version of the software.
//
class LtIpEchVersion : public LtIpPktBase
{
public:
	enum
	{
		ECHVRS_PRODUCTSIZE = 16,		// size of product string
		ECHV1_ADDED_SIZE = (sizeof(ULONG) + ECHVRS_PRODUCTSIZE + 3*sizeof(word)), // old V1 size
		ECHV2_ADDED_SIZE = ECHV1_ADDED_SIZE + sizeof(word),		// new V2 size (add supported version)
		MAX_PKT_SIZE = MAX_HDR_SIZE + ECHV2_ADDED_SIZE,
	};
	ULONG			dateTime;			// seconds since 1 Jan 1900 - WHAT IS THIS FOR?
	char			szProduct[ECHVRS_PRODUCTSIZE]; // product name - i.LON 1000 etc.
	word			nMajorVersion;
	word			nMinorVersion;
	word			nBuildNumber;
	word			echVersionSupported;

	// - end direct copy

	LtIpEchVersion()
	{
		vendorCode	= VENDOR_ECHELON;	// Vendor private for Echelon
		packetType	= PKTTYPE_ECHVERSION;
		dateTime	= 0;	// What is this used for in this packet?
		const char*		pName = getSoftwareProductStr(); // PRODUCT_NAME;
		for ( int i=0; i< ECHVRS_PRODUCTSIZE; i++ )
		{	szProduct[i] = pName[i];
			if ( pName[i] == 0 ) break;
		}
		szProduct[ECHVRS_PRODUCTSIZE-1] = 0; // terminate
		// Use the macros for decimal numbers
		int major, minor, build;
		getSoftwareVersionNum(&major, &minor, &build);
		nMajorVersion = major; // VER_MAJOR_D;
		nMinorVersion = minor; // VER_MINOR_D;
		nBuildNumber = build; // VER_BUILD_D;
		echVersionSupported = LTIP_ECH_VER2;

	}
	virtual ~LtIpEchVersion()
	{
		// nothing to do
	}

	virtual	int echV1FixedSize() { return(hdrSize() + ECHV1_ADDED_SIZE); }	// size of packet under V1 implementation
	virtual int echV2FixedSize() { return(hdrSize() + ECHV2_ADDED_SIZE); }	// size of packet under V2 implementation
	virtual	int echV1DefinedSize() { return(echV1FixedSize()); }	// size of packet under V1 implementation
	virtual int echV2DefinedSize() { return(echV2FixedSize()); }	// size of packet under V2 implementation

	// parse a packet, given the address of a parsed packet header
	boolean		parse( byte* pStartPoint, boolean bRewrite = false  );
	// build a packet of this type if it's all filled in
	// returns pointer beyond the packet built.
	byte*		build( byte* pStartPoint );

	void dump();

};

///////////////////////////////////////////////////////////////////////////////
// Echelon Version Request packet
// 
// This packet is used by the configuration manager to request parameters
// from iLon devices.
//
class LtIpReqEchVersion : public LtIpRequest
{
public:
	LtIpReqEchVersion()
	{
		packetType = PKTTYPE_ECHVERSREQ;
		vendorCode = VENDOR_ECHELON;
	}
};

///////////////////////////////////////////////////////////////////////////////
// Echelon Mode packet
// 
// This packet is used to convey various operating modes 
// of the software.
//
class LtIpEchMode : public LtIpPktBase
{
public:
	enum
	{
		ECHV2_ADDED_SIZE = (sizeof(word)+sizeof(ULONG)+(2*sizeof(byte))),		// V2 size
		ECHV1_ADDED_SIZE = ECHV2_ADDED_SIZE,					// V1 size, just use V2 size
		MAX_PKT_SIZE = MAX_HDR_SIZE + ECHV2_ADDED_SIZE,
	};

	ULONG			natIpAddr;			// NAT IP address (or zero)
	word			echVersionInUse;	// The Echelon implementation version to use/being used
	byte			strictEia852;
	byte			usingLocalCnfg;		// running with local XML config

	// - end direct copy

	LtIpEchMode()
	{
		vendorCode	= VENDOR_ECHELON;	// Vendor private for Echelon
		packetType	= PKTTYPE_ECHMODE;

		natIpAddr = 0;
		echVersionInUse = LTIP_ECH_VER_UNKNOWN;
		strictEia852 = 0;
		usingLocalCnfg = FALSE;
	}
	virtual ~LtIpEchMode()
	{
		// nothing to do
	}

#if ECHV1_ADDED_SIZE != ECHV2_ADDED_SIZE
#error Must provide overrides for different packet sizes
#endif
	virtual int commonFixedSize() { return(hdrSize() + ECHV2_ADDED_SIZE); }

	// parse a packet, given the address of a parsed packet header
	boolean		parse( byte* pStartPoint, boolean bRewrite = false  );
	// build a packet of this type if it's all filled in
	// returns pointer beyond the packet built.
	byte*		build( byte* pStartPoint );

	void dump();

};

///////////////////////////////////////////////////////////////////////////////
// Echelon Mode Request packet
// 
// This packet is used by the configuration manager to request the operating mode
// from iLon devices.
//
class LtIpReqEchMode : public LtIpRequest
{
public:
	LtIpReqEchMode()
	{
		packetType = PKTTYPE_ECHMODEREQ;
		vendorCode = VENDOR_ECHELON;
	}
};


///////////////////////////////////////////////////////////////////////////////
// Echelon Channel Routing Request packet
// 
// This packet is an extension of the standard request packet
// It includes any number of IP address/port pairs.
// Used only for requesting channel routing packets.
//

// *** NOTE *** The LIST part of this packet is currently
// NOT IMPLEMENTED by either the iLON or the config server!
// Only the single port for the single IP address is implemented.
// Set the list size to zero.

class LtIpReqEchChnRouting: public LtIpReqChnRouting
{
public:
	enum
	{	ADDED_SIZE = (2 * 2),
		TOTAL_ADDED_SIZE = (LtIpRequest::ADDED_SIZE + ADDED_SIZE),
		MAX_PKT_SIZE = (MAX_HDR_SIZE + TOTAL_ADDED_SIZE),
	};

	word			listSize;			// number of addresses in list
	word			port;				// single port if list size == 0
	
	// - end direct copy

	byte*			pUcAddresses;		// LtIpAddPort

	LtIpReqEchChnRouting()
	{
		// packetType is standard: PKTTYPE_REQCHNROUTING
		vendorCode = VENDOR_ECHELON;
		listSize = 0;
		port = 0;
		pUcAddresses = NULL;
	}
	
	virtual int commonFixedSize() { return(LtIpReqChnRouting::commonFixedSize() + ADDED_SIZE); }
	virtual int commonDefinedSize() { return(commonFixedSize() + listSize*LtIpAddPort::SIZE); }
	
	int	size() { return definedPktSize(); }

	// parse a packet, given the address of a packet header
	boolean		parse( byte* pStartPoint, boolean bRewrite = true );
	// build a packet of this type if it's all filled in
	// returns pointer beyond the packet built.
	byte*		build( byte* pStartPoint );

	void dump();
};

#ifndef LT_UNIQUE_ID_LEN
#define LT_UNIQUE_ID_LEN 6
#endif
///////////////////////////////////////////////////////////////////////////////
// Echelon Device ID packet
// 
// This packet is used to inform a remote device or CS of the identity of the
// sender of the packet. This is needed when firewalls switch the source port.
//
class LtIpEchDeviceId: public LtIpPktBase
{
public:
	enum
	{
#ifdef ILON_100_ROUTER_DEMO
// Extra identification added for dynamic IP support in demo
		ECHV2_ADDED_SIZE = ((3 * sizeof(ULONG)) + sizeof(USHORT) + LT_UNIQUE_ID_LEN), // V2 size
#else
		ECHV2_ADDED_SIZE = (2 * sizeof(ULONG)) + (sizeof(USHORT)), // V2 size
#endif
		ECHV1_ADDED_SIZE = ECHV2_ADDED_SIZE,					// V1 size, just use V2 size
		MAX_PKT_SIZE = MAX_HDR_SIZE + ECHV2_ADDED_SIZE,
	};

	ULONG			localIpAddr;		// local IP addr of sender
	ULONG			natIpAddr;			// NAT IP addr of sender (0 if none)
	USHORT			ipPort;				// IP port of sender
#ifdef ILON_100_ROUTER_DEMO
	byte			ltUniqueId[LT_UNIQUE_ID_LEN];	// First LUID in the package on the IP channel
	ULONG			prevLocalIpAddr;	// previous local IP addr of sender
#endif

	// - end direct copy

	LtIpEchDeviceId()
	{
		packetType = PKTTYPE_ECHDEVID;
		vendorCode = VENDOR_ECHELON;
		localIpAddr = 0;
		natIpAddr = 0;
		ipPort = 0;
#ifdef ILON_100_ROUTER_DEMO
		memset(ltUniqueId, 0, sizeof(ltUniqueId));
		prevLocalIpAddr = 0;
#endif
	}
	virtual ~LtIpEchDeviceId()
	{
		// nothing to do
	}

	virtual int commonFixedSize() { return(hdrSize() + ECHV2_ADDED_SIZE); }

#ifdef ILON_100_ROUTER_DEMO
	int echV2_0FixedSize() { return echV2FixedSize() - sizeof(ULONG); }
#endif

	// parse a packet, given the address of a parsed packet header
	boolean		parse( byte* pStartPoint, boolean bRewrite = false  );
	// build a packet of this type if it's all filled in
	// returns pointer beyond the packet built.
	byte*		build( byte* pStartPoint );

	void dump();

};

///////////////////////////////////////////////////////////////////////////////
// Echelon Device ID Request packet
// 
// This packet is an extension of the standard request packet
// It includes the senders ID info, allowing the receiver to
// uniquely identify the sender of the request.
// Used only for requesting a device ID packet.
//

class LtIpReqEchDeviceId: public LtIpRequest
{
public:
	enum
#ifdef ILON_100_ROUTER_DEMO
	{	ADDED_SIZE = ((3 * sizeof(ULONG)) + (sizeof(USHORT))),
#else
	{	ADDED_SIZE = ((2 * sizeof(ULONG)) + (sizeof(USHORT))),
#endif
		TOTAL_ADDED_SIZE = (LtIpRequest::ADDED_SIZE + ADDED_SIZE),
		MAX_PKT_SIZE = (MAX_HDR_SIZE + TOTAL_ADDED_SIZE),
	};

	ULONG			localIpAddr;		// local IP addr of sender
	ULONG			natIpAddr;			// NAT IP addr of sender (0 if none)
	USHORT			ipPort;				// IP port of sender
#ifdef ILON_100_ROUTER_DEMO
	ULONG			prevLocalIpAddr;	// previous local IP addr of sender
#endif
	
	// - end direct copy


	LtIpReqEchDeviceId()
	{
		packetType = PKTTYPE_ECHDEVIDREQ;
		vendorCode = VENDOR_ECHELON;
		localIpAddr = 0;
		natIpAddr = 0;
		ipPort = 0;
#ifdef ILON_100_ROUTER_DEMO
		prevLocalIpAddr = 0;
#endif
	}
	
	virtual int commonFixedSize() { return(LtIpRequest::commonFixedSize() + ADDED_SIZE); }
	virtual int commonDefinedSize() { return(commonFixedSize()); }
	
#ifdef ILON_100_ROUTER_DEMO
	int echV2_0FixedSize() { return echV2FixedSize() - sizeof(ULONG); }
#endif

	int	size() { return definedPktSize(); }

	// parse a packet, given the address of a packet header
	boolean		parse( byte* pStartPoint, boolean bRewrite = true );
	// build a packet of this type if it's all filled in
	// returns pointer beyond the packet built.
	byte*		build( byte* pStartPoint );

	void dump();
};


#endif // _LTIPECHPACKETS_H
