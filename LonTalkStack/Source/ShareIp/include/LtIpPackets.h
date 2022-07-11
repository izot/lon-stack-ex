#ifndef _LTIPPACKETS_H
#define _LTIPPACKETS_H

/***************************************************************
 *  Filename: LtIpPackets.h
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
 *  Description:  Header file for LonTalk IP Packet formats.
 *
 *	DJ Duffy Feb 1999
 *
 ****************************************************************/

/*
 * $Log: /Dev/ShareIp/include/LtIpPackets.h $
 * 
 * 33    9/03/03 5:36p Fremont
 * Implement extended packet header for EPR 29618.
 * 
 * 32    8/27/03 6:26p Bobw
 * Add "fetch" and "store" methods to LtIpNeuronId and LtIpDomain and use
 * "store" instead of "build" in
 * LtIpMasterXmlConfig::buildChannelRoutingPkt for consistency.  Doesn't
 * actually make any differences, since there are no differnces between
 * host format and network format for these two objects, but seems better
 * if the caller does not need to be aware of that.
 * 
 * 31    8/05/03 3:18p Fremont
 * secure flag reversed (but worked anyway)
 * 
 * 30    7/08/03 2:33p Fremont
 * Add intf. to get 1900-1970 time delta
 * 
 * 29    6/10/03 5:35p Fremont
 * changes for EIA0852 auth
 * 
 * 28    6/03/03 7:17p Fremont
 * more EIA-852 packet transformations, general cleanup
 * 
 * 27    4/08/03 12:00p Fremont
 * provide bogus SIZE enum to hide header base class definition
 * 
 * 26    3/25/03 11:42a Fremont
 * remove special compilation for VNI, fixed some size values
 * 
 * 25    3/04/03 3:44p Fremont
 * add base class for version 2 pkts
 * 
 * 23    10/25/99 9:18a Dwf
 * Fixed assignments in struct declaration.
 * 
 * 22    10/25/99 7:35a Darrelld
 * Fix some aggregated compilation problems
 * 
 * 21    10/22/99 1:13p Darrelld
 * Don't use zero time stamp for certain cases
 * 
 * 20    9/08/99 2:22p Darrelld
 * New Common packet header says
 * Session number is now a ULONG
 * Add statistics packet
 * 
 * 19    7/27/99 1:51p Darrelld
 * Fix mbz fields
 * 
 * 18    7/21/99 3:15p Darrelld
 * Remove Echelon private packets
 * 
 * 17    7/20/99 5:26p Darrelld
 * Updates for reordering test
 * 
 * 16    7/14/99 9:32a Darrelld
 * Support primary and secondary time servers
 * 
 * 15    6/22/99 9:47a Darrelld
 * Correct processing of packet size and update packet type codes
 * 
 * 14    6/17/99 4:44p Glen
 * Moved macros to LtNetData.h
 * 
 * 13    5/14/99 2:44p Darrelld
 * 
 * 12    5/13/99 4:56p Darrelld
 * Segment packet support in progress
 * 
 * 11    5/07/99 11:01a Darrelld
 * Upgrade for latest RFC packet formats
 * 
 * 10    5/06/99 5:09p Darrelld
 * Enhancments to RFC packets
 * 
 * 9     4/23/99 5:22p Darrelld
 * Router testing
 * 
 * 8     4/20/99 4:12p Darrelld
 * add "confidential" statement
 * 
 * 7     3/15/99 10:37a Darrelld
 * Add dumpPacket to dump a decoded packet
 * 
 * 6     3/05/99 2:36p Darrelld
 * Router types to LonTalk architectural values
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
 * 1     2/02/99 12:41p Darrelld
 * 
 * 
 */

#include "LtNetData.h"
#include "limits.h"
#include <assert.h>

#define MAX_CLOCK_TICKS 120

enum
{
	PKTTYPE_DATA				= 0x01,		// data packet
	PKTTYPE_DEVREGISTER			= 0x03,		// device registration packet - from device to server
	PKTTYPE_DEVCONFIGURE		= 0x71,		// device configure packet - response from device or server
	PKTTYPE_CHNMEMBERS			= 0x04,		// channel membership packet
	PKTTYPE_CHNROUTING			= 0x08,		// channel routing packet
	PKTTYPE_SENDLIST			= 0x06,		// send list packet
	PKTTYPE_RESPONSE			= 0x07,		// response packet [ Ack ]
	PKTTYPE_REQCHNMEMBERS		= 0x64,		// request channel membership
	PKTTYPE_REQSENDLIST			= 0x66,		// request send list
	PKTTYPE_REQDEVCONFIG		= 0x63,		// request device configuration
	PKTTYPE_REQCHNROUTING		= 0x68,		// request channel routing
	PKTTYPE_REQSTATISTICS		= 0x60,		// request Statistics
	PKTTYPE_STATISTICS			= 0x70,		// Health and statistics

	PKTTYPE_SEGMENT				= 0x7f,		// segmentation packet

	PKTTYPE_MAX					= 0xff,
	VENDOR_STANDARD				= 0,
	VENDOR_ECHELON				= 1,		// vendor private for Echelon
	VERSION_ZERO				= 0,
	VERSION_ONE					= 1,

	PROTFLAG_SECURITY			= 0x20,		// security bit in protocol flags field
	PROTOCOL_FLAGS_OFFSET		= 5,		// byte offset of this field in header

	LTROUTER_CONFIGURED			= 0,		// * router mode or node type
	LTROUTER_LEARNING			= 1,		// * these are LonTalk architectural values
	LTROUTER_BRIDGE				= 2,		// *
	LTROUTER_REPEATER			= 3,		// *

	LTNODE_LTIPROUTER			= 1,		// Node type - LonTalk IP router
	LTNODE_IPNODE				= 2,
	LTNODE_IPPROXY				= 3,
	LTNODE_IPIPROUTER			= 4,

	LTIPPROTOCOL_UDP			= 1,		// ip protocol(s) supported
	LTIPPROTOCOL_TCP			= 2,
	LTIPPROTOCOL_MULTICAST		= 4,

	LTROUTER_ALLBROADCAST		= 1,		// router wants to receive all broadcasts
	SUPPORTS_EIA_AUTH			= 2,		// EIA-852 authentication supported

	REQUEST_NORMAL				= 0,		// normal request
	REQUEST_VERIFY				= 1,		// request to verify
	REQUEST_ONE					= 0,		// Request one segment
	REQUEST_ALL					= 4,		// bit 2
	REQUEST_COPY				= 0,		// Request a copy of data
	REQUEST_MOVE				= 8,		// Request a copy and clear data

	ACK_OK						= 0,		// success
	ACK_FIXED					= 1,		// cant change
	ACK_BAD_MESSAGE				= 2,		// bad message
	ACK_CANT_COMPLY				= 3,		// cant take that configuration
	ACK_DEVICE_REFUSED			= 4,		// config server says no
	ACK_NOT_SUPPORTED			= 5,		// optional feature not supported

	LTDUMMY	= 0
};

enum LtIpEchProtocolVersion
{
	LTIP_ECH_VER_UNKNOWN		= 0,		// uninitialized/unknown
	LTIP_ECH_VER1				= 1,		// Echelon's first implementation
	LTIP_ECH_VER2				= 2,		// Echelon's second implementation
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Conventions
//
//
// The following classes have a few things in common.
//
// parse		load a copy of the class with the data thats in network order
// build		store the data from an instance into a buffer in network order
// fetch		move the bytes from a buffer thats in host order into a class
// store		move the data from the class into a buffer in host order
//
// fetch and store are optional
//
// dump			write the data to vxlReportEvent
//
///////////////////////////////////////////////////////////////////////////////


// The following classes are for packet components


// Common packet header
// this is an image of the actual packet.
// the only class that is an image.
//
struct LtIpPktHeader
{
	static	int			 clkRate;
	static	ULONGLONG	 msPerSec;
	static	boolean		 bGotClock;
	static	ULONG		 nLastTick;
	static  ULONGLONG	 tClkTime;
	static  struct timespec  tTimeSpec;

	enum
	{	
		STD_HDR_SIZE = (4*5),		// size of this portion of protocol [ not structure ]
		EXT_HDR_ADD_SIZE = (4*3),	// additional size of extended portion of header
		EXT_HDR_SIZE = (STD_HDR_SIZE + EXT_HDR_ADD_SIZE),
		MAX_HDR_SIZE = EXT_HDR_SIZE,

	};
	word	packetSize;			// total bytes in packet including header
	byte	version;			// protocol version - lower 5 bits
	byte	versionBits;		// bits 5-7 of version -
								// bits 5-6 MBZ
								// bit 7 => vendor private packet follows
	byte	packetType;			// 
	byte	extndHdrSize;		// size of header - 20 - MBZ for this version
	byte	protocolFlags;		// 0 for EIA-709 (LonTalk)
	word	vendorCode;			// vendor - zero for standard
								// ** Ver1 used 3 bytes (was ULONG)
								// EIA852 uses 2 bytes, plus the previous field

	ULONG	session;			// session id
	ULONG	sequence;			// sequence number or zero
	ULONG	timestamp;			// milliseconds in wall clock time
	// - end direct copy standard header

	// Echelon extended header fields
	ULONG	extHdrLocalIpAddr;
	ULONG	extHdrNatIpAddr;
	word	extHdrIpPort;
	word	extHdrUnused;
	// - end direct copy extended header

	byte*	pRemaining;			// address of remainder of packet
	byte*	pPacket;			// address of start of packet

	boolean	bHasExtHdr;			// flag use of extended header

	LtIpPktHeader();
	virtual ~LtIpPktHeader();
	static ULONG	getTimestamp(boolean bSyncCheck=true);
	static ULONG	getDateTime();
	static ULONG	getTd1970();	// time delta from 1900 to 1970
	// Size functions
	// Size of this header
	int				hdrSize() { return (bHasExtHdr ? EXT_HDR_SIZE : STD_HDR_SIZE); }
	// Static size based on external flag
	static int		hdrSize(boolean bExtHdr);
	virtual int		size() { return hdrSize(); }
	virtual boolean parse( byte* pPacket, boolean bRewrite = true );
	virtual byte*	build( byte* pStart );
	// when building a packet we save pPacket when building the header
	// when finished, must store the correct packet size in the built packet.
	// This member does that. Pass it the address beyond the last byte of the packet.
	virtual void	buildPacketSize( byte* pend )
	{	packetSize = pend - pPacket;
		byte* pp = pPacket;
		// the packet size happens to be the first word of the packet.
		PTONS( pp, packetSize );
	}
	// quick test to see if there is a packet here at all
	// those who use this must rely on somebody to check before
	// they do this.
	virtual boolean isValid()
	{
		return pPacket != NULL;
	}

	virtual void dump();
	virtual void dumpPacket( LPCSTR tag, byte* pData, int nLen, boolean bHeaderOnly );
	virtual void dumpLtPacket( LPCSTR tag, byte* pData, int nLen );

};

///////////////////////////////////////////////////////////////////////////////
// IP address and port
// 
// 
//
struct LtIpAddPort
{
	enum
	{	SIZE = 8
	};
	ULONG	ipAddress;
	word	ipPort;
	word	mbz;

	byte* parse( byte* p )
	{
		PTOHL( p, ipAddress );
		PTOHS( p, ipPort );
		PTOHS( p, mbz );
		return p;
	}
	byte* build( byte* p )
	{	mbz = 0;
		PTONL( p, ipAddress );
		PTONS( p, ipPort );
		PTONS( p, mbz );
		return p;
	}
	byte* store( byte* p )
	{
		PTOXL( p, ipAddress );
		PTOXS( p, ipPort );
		PTOXS( p, mbz );
		return p;
	}
	byte* fetch( byte* p )
	{
		PFMXL( p, ipAddress );
		PFMXS( p, ipPort );
		PFMXS( p, mbz );
		return p;
	}

	void dump();
};

///////////////////////////////////////////////////////////////////////////////
// IP address and port with DateTime
// 
// 
//
struct LtIpAddPortDate
{
	enum
	{	SIZE = 12
	};
	ULONG	ipAddress;
	word	ipPort;
	word	mbz;
	ULONG	dateTime;

	byte* parse( byte* p )
	{
		PTOHL( p, ipAddress );
		PTOHS( p, ipPort );
		PTOHS( p, mbz );
		PTOHL( p, dateTime );
		return p;
	}
	byte* build( byte* p )
	{	mbz = 0;
		PTONL( p, ipAddress );
		PTONS( p, ipPort );
		PTONS( p, mbz );
		PTONL( p, dateTime );
		return p;
	}
	byte* store( byte* p )
	{
		PTOXL( p, ipAddress );
		PTOXS( p, ipPort );
		PTOXS( p, mbz );
		PTOXL( p, dateTime );
		return p;
	}
	byte* fetch( byte* p )
	{
		PFMXL( p, ipAddress );
		PFMXS( p, ipPort );
		PFMXS( p, mbz );
		PFMXL( p, dateTime );
		return p;
	}

	void dump();
};

///////////////////////////////////////////////////////////////////////////////
// IP address without port
// 
// 
//
struct LtIpAddress
{
	enum
	{	SIZE = 4
	};
	ULONG	ipAddress;

	byte* parse( byte* p )
	{
		PTOHL( p, ipAddress );
		return p;
	}
	byte* build( byte* p )
	{
		PTONL( p, ipAddress );
		return p;
	}
	byte* store( byte* p )
	{
		PTOXL( p, ipAddress );
		return p;
	}
	byte* fetch( byte* p )
	{
		PFMXL( p, ipAddress );
		return p;
	}

	void dump();
};



///////////////////////////////////////////////////////////////////////////////
// LonTalk Subnet Node
// 
// 
//
struct LtIpSubnetNode
{
	enum
	{	SIZE = 6
	};
	byte	subnet;				// LonTalk subnet number
	byte	node;				// LonTalk node number
	word	domainIndex;		// index into domain list
	word	neuronIdIndex;		// index into neuron ID list


	byte* parse( byte* p )
	{
		PTOHB( p, subnet );
		PTOHB( p, node );
		PTOHS( p, domainIndex );
		PTOHS( p, neuronIdIndex );
		return p;
	}
	byte* build( byte* p )
	{
		PTONB( p, subnet );
		PTONB( p, node );
		PTONS( p, domainIndex );
		PTONS( p, neuronIdIndex );

		return p;
	}
	byte* store( byte* p )
	{
		PTONB( p, subnet );
		PTONB( p, node );
		PTOXS( p, domainIndex );
		PTOXS( p, neuronIdIndex );
		return p;
	}
	byte* fetch( byte* p )
	{
		PTOHB( p, subnet );
		PTOHB( p, node );
		PFMXS( p, domainIndex );
		PFMXS( p, neuronIdIndex );
		return p;
	}

	void dump();
};

///////////////////////////////////////////////////////////////////////////////
// LonTalk Domain
// 
// Fixed size structure for domains
// There is a potential savings of 50 bytes to using counted subnet and
// group masks, but this does not materially change the limits for
// nodes which is set more stringently by the limit on the size of the
// packet size [ 64K ].
//
struct LtIpDomain
{
	enum
	{
		SIZE = 72,
		MASKLEN = 32,
		DOMLEN = 6
	};

	byte	subnetMask[MASKLEN];		// subnet zero is low order bit of byte zero
	byte	groupMask[MASKLEN];			// group zero is low order bit of byte zero
	byte	domainLength;				// 1 - 6
	byte	mbz;
	byte	domainBytes[DOMLEN];		// in lontalk network order


	byte* parse( byte* p )
	{
		PTOHBN( MASKLEN, p, subnetMask );
		PTOHBN( MASKLEN, p, groupMask );
		PTOHB( p, domainLength );
		PTOHB( p, mbz );
		PTOHBN( DOMLEN, p, domainBytes );
		return p;
	}
	byte* build( byte* p )
	{	mbz = 0;
		PTONBN( MASKLEN, p, subnetMask );
		PTONBN( MASKLEN, p, groupMask );
		PTONB( p, domainLength );
		PTONB( p, mbz );
		PTONBN( DOMLEN, p, domainBytes );

		return p;
	}

	byte* fetch( byte* p )
	{
        return parse(p);    // LtIpDomain has no host dependent data structures.
	}

	byte* store( byte* p )
	{	
        return build(p);    // LtIpDomain has no host dependent data structures.
	}
	void dump();
};


///////////////////////////////////////////////////////////////////////////////
// Neuron Id
// 
// Neuron Ids are byte strings expressed in the order of the bytes
// that appear in a LonTalk message.
//
struct LtIpNeuronId
{
	enum
	{	SIZE = 6
	};
	byte	id[SIZE];
	byte* parse( byte* p )
	{
		PTOHBN( SIZE, p, id );
		return p;
	}
	byte* build( byte* p )
	{
		PTONBN( SIZE, p, id );
		return p;
	}
    byte *fetch( byte *p )
    {
		return parse(p);  // LtIpNeuronId has no host dependent data.
    }
    
    byte *store( byte *p)
    {
		return build(p);  // LtIpNeuronId has no host dependent data.
    }
	void dump();
};


// Base class for actual packet classes
class LtIpPktBase : public LtIpPktHeader
{
protected:
	LtIpEchProtocolVersion    echPktVersion;	// Echelon's implementation version
	boolean secure;	// packet is marked for authentication

public:
	enum
	{	
		SIZEX = -100,	// Make compiles fail if they use this!
	};

	LtIpPktBase() 
	{ 
		echPktVersion = LTIP_ECH_VER2; 	// default to version 2 (latest)
		secure = false;
	}

	// These are the full defined packet size, under the two implementations
	virtual	int echV1DefinedSize()	// V1 (iLON 1000)
		{ return(commonDefinedSize()); }	// Default to common size
	virtual int echV2DefinedSize()	// V2 (EIA-852)
		{ return(commonDefinedSize()); }	// Default to common size
	// Override this if it applies and is different that the fixed size;
	// otherwise, must override commonFixedSize() or don't call this
	virtual int commonDefinedSize()
		{ return(commonFixedSize()); }	
	int eia852Size() { return(echV2DefinedSize()); }	// size of packet under EIA-852 spec (ech V2)
	// This is the total packet size, including size computed from fields in the packet
	virtual int definedPktSize() 
	{ 
		int size = (echPktVersion == LTIP_ECH_VER1) ? echV1DefinedSize() : echV2DefinedSize();
		return(size); 
	}

	// These are the fixed (static) packet size, under the two implementations
	virtual	int echV1FixedSize()	// V1 (iLON 1000)
		{ return(commonFixedSize()); }	// Default to common size
	virtual int echV2FixedSize()	// V2 (EIA-852)
		{ return(commonFixedSize()); }	// Default to common size
	// Override this if it applies; otherwise don't call it
	virtual int commonFixedSize()
		{ assert(0); return(INT_MAX); }	
	// This is the size of the fixed (static) part of the packet
	virtual int fixedPktSize() 
	{ 
		int size = (echPktVersion == LTIP_ECH_VER1) ? echV1FixedSize() : echV2FixedSize();
		return(size); 
	}
	virtual void setEchVersion(LtIpEchProtocolVersion ver) { echPktVersion = ver; }
	LtIpEchProtocolVersion getEchVersion() { return(echPktVersion); }
	boolean pktIsEchVer2() { return(echPktVersion == LTIP_ECH_VER2); }
	boolean pktIsEchVer1() { return(echPktVersion == LTIP_ECH_VER1); }
	
	// Used on outgoing	packets to set/clear security header bit  
	// before or after the packet is built, but before computing 
	// the authentication digest. 
	void markSecure(byte* pStartPoint)
	{
		secure = true;
		*(pStartPoint + PROTOCOL_FLAGS_OFFSET) |= PROTFLAG_SECURITY;
	}
	void markUnsecure(byte* pStartPoint)
	{
		secure = false;
		*(pStartPoint + PROTOCOL_FLAGS_OFFSET) &= ~PROTFLAG_SECURITY;
	}

	// Used on received packets to check header bit
	boolean isMarkedSecure(byte* pStartPoint)
	{
		secure = ((*(pStartPoint + PROTOCOL_FLAGS_OFFSET) & PROTFLAG_SECURITY) != 0);
		return(secure);
	}

	virtual byte* build( byte* pStartPoint ); // override
	virtual boolean	parse(byte* pStart, boolean bRewrite); // override
	virtual void buildPacketSize(byte* pend); // override 
	
	void dump();

};

// The following classes are for actual packets	derived from LtIpPktHeader


///////////////////////////////////////////////////////////////////////////////
// Device Registration packet
// 
// 
//
class LtIpDevRegister : public LtIpPktBase
{
public:
	enum
	{	
		ADDED_SIZE = (7*sizeof(ULONG))+(7*sizeof(word))+(6*sizeof(byte)),
	};

	ULONG			dateTime;
	// -
	byte			ipFlags;			// see LTIPPROTOCOL_UDP etc.
	byte			routerType;			// see LTROUTER_REPEATER etc.
	byte			lonTalkFlags;		// LTROUTER_ALLBROADCAST, SUPPORTS_EIA_AUTH
	byte			ipNodeType;			// type of node LTNODE_LTIPROUTER etc.
	//
	byte			mcAddressCount;		// number of mcAddresses
	byte			mbz;
	word			channelTimeout;		// milliseconds 0 - 1500 for channel timeout value
	// -
	word			neuronIdBytes;		// bytes of neuron IDs
	word			ipUnicastPort;		// port for unicast traffic
	// -
	ULONG			ipUcAddress;		// Unicast IP address
	ULONG			chanMemDatetime;	// datetime of latest channel membership packet
	ULONG			sendlistDatetime;	// datetime of latest sendlist packet
	ULONG			ipAddressCS;		// configuation server IP address
	ULONG			ipAddressTS;		// NTP time server IP address
	ULONG			ipAddressTS2;		// NTP time server IP address, secondary
	word			ipPortCS;			// Config Server IP port
	word			ipPortTS;			// Time Server IP port
	word			ipPortTS2;			// Time Server IP port, secondary
	word			mbz2;

	// - end direct copy
	byte*			pMcAddresses;		// LtIpAddPort
	byte*			pNeuronIds;			// LtIpNeuronId

	byte			nameLen;
	byte*			pName;

	LtIpDevRegister();
	virtual ~LtIpDevRegister();

	virtual int commonFixedSize() { return(hdrSize() + ADDED_SIZE); }
	virtual int commonDefinedSize() 
	{	return  commonFixedSize()+mcAddressCount*LtIpAddPort::SIZE
				+ neuronIdBytes + 1+ nameLen;
	}

	int	size() { return  definedPktSize(); }


	// parse a packet, given the address of a parsed packet header
	virtual boolean		parse( byte* pStartPoint, boolean bRewrite = true );
	// build a packet of this type if it's all filled in
	// returns pointer beyond the packet built.
	virtual byte*		build( byte* pStartPoint );

	void dump();

};


///////////////////////////////////////////////////////////////////////////////
// Channel Membership packet
// 
// 
//
class LtIpChanMembers : public LtIpPktBase
{
public:
	enum	{ ADDED_SIZE = (3*sizeof(ULONG))+(2*sizeof(word)) };
	ULONG			dateTime;			// seconds since 1 Jan 1900
	ULONG			sendListDateTime;	// send list datetime
	ULONG			mbz;				// ** Ver1 was device registration datetime
	// -
	word			listSize;			// number of addresses in list
	word			mbz10;
	// -
	// - end direct copy
	byte*			pUcAddresses;		// LtIpAddPortDate


	LtIpChanMembers();
	virtual ~LtIpChanMembers();

	virtual int commonFixedSize() { return(hdrSize() + ADDED_SIZE); }
	virtual int commonDefinedSize()
		{ return commonFixedSize() + listSize*LtIpAddPortDate::SIZE; }

	int	size() { return definedPktSize(); }

	// parse a packet, given the address of a packet header
	boolean		parse( byte* pStartPoint, boolean bRewrite = true );
	// build a packet of this type if it's all filled in
	// returns pointer beyond the packet built.
	byte*		build( byte* pStartPoint );

	void dump();

};


///////////////////////////////////////////////////////////////////////////////
// Channel Routing packet
// 
// 
//
class LtIpChanRouting : public LtIpPktBase
{
public:
	enum	{ ADDED_SIZE = (3*sizeof(ULONG))+(6*sizeof(word))+(4*sizeof(byte)) };

	ULONG			dateTime;			// seconds since 1 Jan 1900
	// -
	word			ipMcPort;			// Multicast port
	word			ipUcPort;			// unicast port
	ULONG			ipMcAddress;		// Multicast IP address
	ULONG			ipUcAddress;		// Unicast IP address
	// -
	byte			ipFlags;			// see LTIPPROTOCOL_UDP etc.
	byte			routerType;			// see LTROUTER_REPEATER etc.
	byte			lonTalkFlags;		// LTROUTER_ALLBROADCAST, SUPPORTS_EIA_AUTH
	byte			ipNodeType;			// type of node LTNODE_LTIPROUTER etc.
	// -
	word			neuronIdBytes;		// bytes of neuronIds to follow
	word			mbz;
	// -
	word			subnetNodeBytes;	// bytes of subnet Node structs to follow
	word			domainBytes;		// bytes of domain structs to follow
	// - end direct copy
	byte*			pNeuronIds;			// LtIpNeuronId
	byte*			pSubnetNodes;		// LtIpSubnetNode
	byte*			pDomains;			// LtIpDomain

	LtIpChanRouting()
	{
		packetType = PKTTYPE_CHNROUTING;
		dateTime = 0;
		ipMcPort = 0;
		ipUcPort = 0;
		ipMcAddress = 0;
		ipUcAddress = 0;
		ipFlags = 0;
		routerType = 0;
		lonTalkFlags = 0;
		ipNodeType = 0;
		neuronIdBytes = 0;
		mbz = 0;
		subnetNodeBytes = 0;
		domainBytes = 0;
		pNeuronIds = NULL;
		pSubnetNodes = NULL;
		pDomains = NULL;
	}
	virtual ~LtIpChanRouting()
	{
		// nothing to do
	}

	virtual int commonFixedSize() { return(hdrSize() + ADDED_SIZE); }
	virtual int commonDefinedSize()
		{ return commonFixedSize() + neuronIdBytes +
					subnetNodeBytes + domainBytes; }

	int	size() { return definedPktSize(); }
	

	// parse a packet, given the address of a parsed packet header
	boolean		parse( byte* pStartPoint, boolean bRewrite = true  );
	// build a packet of this type if it's all filled in
	// returns pointer beyond the packet built.
	byte*		build( byte* pStartPoint );

	void dump();

};

///////////////////////////////////////////////////////////////////////////////
// Response packet
// 
// 
//
class LtIpResponse : public LtIpPktBase
{
public:
	enum
	{	ADDED_SIZE = sizeof(ULONG)+sizeof(word)+(2*sizeof(byte)),
		MAX_PKT_SIZE = MAX_HDR_SIZE + ADDED_SIZE,
	};

	ULONG			dateTime;			// seconds since 1 Jan 1900
	// -
	byte			type;				// response code - ACK_OK etc.
	word			requestId;			// request identifier
	byte			segmentId;			// segment identifier
	// - end direct copy

	LtIpResponse()
	{
		packetType = PKTTYPE_RESPONSE;
		dateTime = 0;
		type = 0;
		requestId = 0;
		segmentId = 0;
	}
	virtual ~LtIpResponse()
	{
		// nothing to do
	}

	virtual int commonFixedSize() { return(hdrSize() + ADDED_SIZE); }

	int	size() { return definedPktSize(); }

	// parse a packet, given the address of a parsed packet header
	boolean		parse( byte* pStartPoint, boolean bRewrite = true  );
	
	// build a packet of this type if it's all filled in
	// returns pointer beyond the packet built.
	byte*		build( byte* pStartPoint );

	void dump();

};



///////////////////////////////////////////////////////////////////////////////
// Request packet
// 
// 
//
class LtIpRequest : public LtIpPktBase
{
public:
	enum
	{
		ADDED_SIZE =	(3*sizeof(ULONG))+sizeof(word)+(2*sizeof(byte)),
		MAX_PKT_SIZE = ( MAX_HDR_SIZE + ADDED_SIZE),
	};
	
	ULONG			dateTime;			// seconds since 1 Jan 1900
	// -
	byte			reason;				// reason code - REQUEST_VERIFY, REQUEST_ALL
	word			requestId;			// request identifier
	byte			segmentId;			// segment Id
	// -
	ULONG			sinceDateTime;		// Since date time
	ULONG			ipUcAddress;		// key for some types of request
	// - end direct copy

	LtIpRequest()
	{
		packetType = 0;					// set packet type manually or use derived class
		dateTime = 0;
		reason = 0;
		requestId = 0;
		segmentId = 0;
		sinceDateTime = 0;
		ipUcAddress = 0;
	}
	virtual ~LtIpRequest()
	{
		// nothing to do
	}

	void		setPacketType( int pktType )
	{	packetType = pktType;
	}
	
	virtual int commonFixedSize() { return(hdrSize() + ADDED_SIZE); }

	int	size() { return definedPktSize(); }
	
	// parse a packet, given the address of a parsed packet header
	boolean		parse( byte* pStartPoint, boolean bRewrite = true  );
	// build a packet of this type if it's all filled in
	// returns pointer beyond the packet built.
	byte*		build( byte* pStartPoint );

	void dump();

};

// subclasses to set packet type appropriately

class LtIpReqChnMembers : public LtIpRequest
{
public:
	LtIpReqChnMembers()
	{
		packetType = PKTTYPE_REQCHNMEMBERS;
	}
};

class LtIpReqChnRouting : public LtIpRequest
{
public:
	LtIpReqChnRouting()
	{
		packetType = PKTTYPE_REQCHNROUTING;
	}
};

class LtIpReqDevConfig : public LtIpRequest
{
public:
	LtIpReqDevConfig()
	{
		packetType = PKTTYPE_REQDEVCONFIG;
	}
};

class LtIpReqStats : public LtIpRequest
{
public:
	LtIpReqStats()
	{
		packetType = PKTTYPE_REQSTATISTICS;
	}
};


class LtIpReqSendList : public LtIpRequest
{
public:
	LtIpReqSendList()
	{
		packetType = PKTTYPE_REQSENDLIST;
	}
};


///////////////////////////////////////////////////////////////////////////////
// SendList packet
// 
// 
//
class LtIpSendList : public LtIpPktBase
{
public:
	enum
	{
		ECHV1_ADDED_SIZE = sizeof(ULONG)+(4*sizeof(word)),
		ECHV2_ADDED_SIZE = sizeof(ULONG)+sizeof(word)+(2*sizeof(byte)),
	};

	ULONG			dateTime;			// seconds since 1 Jan 1900
	// -
	// The following three fields are for Echelon Version 1 only!
	word			mcBytes;			// Multicast address bytes 
	word			ucBytes;			// Unicast address bytes
	word			ipUcPort;			// Unicast IP port

	// The following two fields are for Echelon Version 2 only!
	byte			ipAddrCount;		// uc and mc addr/port pairs
	byte			mbz1;				// must be zero

	word			mbz2;				// must be zero
	// - end direct copy
	byte*			pMcAddresses;		// LtIpAddPort - ** VER1 ONLY
	byte*			pUcAddresses;		// LtIpAddress - ** VER1 ONLY
	byte*			pAddresses;			// LtIpAddPort - ** VER2 ONLY

	LtIpSendList()
	{
		packetType = PKTTYPE_SENDLIST;
		dateTime = 0;
		mcBytes = 0;
		ucBytes = 0;
		ipUcPort = 0;
		ipAddrCount = 0;
		mbz1 = 0;
		mbz2 = 0;
		pMcAddresses = NULL;
		pUcAddresses = NULL;
		pAddresses = NULL;
	}
	virtual ~LtIpSendList()
	{
		// nothing to do
	}

	virtual int echV1FixedSize() { return(hdrSize() + ECHV1_ADDED_SIZE); }
	virtual int echV2FixedSize() { return(hdrSize() + ECHV2_ADDED_SIZE); }
	virtual int echV1DefinedSize() { return(echV1FixedSize() + mcBytes + ucBytes); }
	virtual int echV2DefinedSize() { return(echV2FixedSize() + ipAddrCount*LtIpAddPort::SIZE); }

	int	size() { return definedPktSize(); }

	// parse a packet, given the address of a parsed packet header
	boolean		parse( byte* pStartPoint, boolean bRewrite = true  );
	// build a packet of this type if it's all filled in
	// returns pointer beyond the packet built.
	byte*		build( byte* pStartPoint );

	void dump();

};

///////////////////////////////////////////////////////////////////////////////
// Statistics packet
// 
// Use the following symbols for get and put members.
//
enum {
	LtStats_secondsSinceReset = 0,
	LtStats_dateTimeReset,
	LtStats_members,
	LtStats_activeMembers,
	LtStats_LtPktsReceived,
	LtStats_LtPktsSelectiveDiscards,
	LtStats_LtBytesReceived,
	LtStats_LtPktsSent,
	LtStats_LtBytesSent,
	LtStats_IpPktsSent,
	LtStats_IpBytesSent,
	LtStats_IpPktsReceived,
	LtStats_IpBytesReceived,
	LtStats_IpPktsDataSent,
	LtStats_IpPktsDataReceived,
	LtStats_IpAvgAggSent,
	LtStats_IpAvgAggReceived,
	LtStats_UdpPktsSent,
	LtStats_TcpPktsSent,
	LtStats_McPktsSent,
	LtStats_LtPktsStaleDropped,
	LtStats_TcpConnectFailures,
	LtStats_TcpMembersFail,
	LtStats_RfcPktsSent,
	LtStats_RfcPktsReceived,
	LtStats_RfcConfigChanges,
	
	// Up to this point the stats are the same.
	LtStats_CommonSize,
	// The follwing two are only Ver1
	LtV1Stats_AvgUdpPktsPerSec = LtStats_CommonSize,
	LtV1Stats_AvgTcpPktsPerSec,
	LtV1Stats_Size,	// The Ver1 size
	LtStats_V1Only_Size = LtV1Stats_Size - LtStats_CommonSize,	// num unique

	// These Ver2 stats overlap the last two Ver1 stats
	LtV2Stats_AvgUdpPktsPerSecSent = LtStats_CommonSize,
	LtV2Stats_AvgUdpPktsPerSecRcvd,
	LtV2Stats_AvgTcpPktsPerSecSent,
	LtV2Stats_AvgTcpPktsPerSecRcvd,
	LtV2Stats_Size, // The Ver 2 size
	LtStats_V2Only_Size = LtV2Stats_Size - LtStats_CommonSize,	// num unique
};

// Special values
#define STATS_OVERFLOW		0xfffffffe
#define STATS_UNSUPPORTED	0xffffffff

class LtIpPktStats : public LtIpPktBase
{
public:
	enum
	{
		ECHV1_ADDED_SIZE = (LtV1Stats_Size*sizeof(ULONG)),
		ECHV2_ADDED_SIZE = (LtV2Stats_Size*sizeof(ULONG)),
	};

	ULONG		stats[LtV2Stats_Size];
	// -

	LtIpPktStats()
	{
		packetType = PKTTYPE_STATISTICS;
		memset( stats, -1, sizeof(stats) ); // STSTS_UNSUPPORTED
	}
	virtual ~LtIpPktStats()
	{
		// nothing to do
	}
	
	virtual int echV1FixedSize() { return(hdrSize() + ECHV1_ADDED_SIZE); }
	virtual int echV2FixedSize() { return(hdrSize() + ECHV2_ADDED_SIZE); }
	virtual int echV1DefinedSize() { return(echV1FixedSize()); }
	virtual int echV2DefinedSize() { return(echV2FixedSize()); }
	
	int	size() { return definedPktSize(); }

	ULONG		get( int nOffset )
	{	return stats[nOffset];
	}
	void		put( int nOffset, ULONG nValue )
	{	stats[nOffset] = nValue;
	}
	// parse a packet, given the address of a parsed packet header
	boolean		parse( byte* pStartPoint, boolean bRewrite = true  );
	// build a packet of this type if it's all filled in
	// returns pointer beyond the packet built.
	byte*		build( byte* pStartPoint );

	void dump();

};


///////////////////////////////////////////////////////////////////////////////
// LtIpAddressStr
// 
// Convert LtIpAddress to and from a string.
//
// form of string is ddd.ddd.ddd.ddd
//

class LtIpAddressStr
{
public:
	enum { STRLEN = 18 };
	boolean	m_bClean;
	char	m_string[STRLEN];
	ULONG	m_iaddr;

	LtIpAddressStr()
	{	m_bClean = false;
		m_iaddr = 0;
		m_string[0] = 0;
	}
	void	setIaddr( ULONG iad )
	{	m_iaddr = iad;
		m_bClean = false;
	}
	void	setString( char* pStr );
	ULONG	getIaddr();
	ULONG	getIaddr( char* pStr );
	char*	getString();
	char*	getString( ULONG iad );
};

///////////////////////////////////////////////////////////////////////////////
// LtIpDateTimeStr
// 
// Convert dateTime to a string.
//
// form of string is standard unix format of DDD MMM dd yyyy hh:mm:ss
// DDD is day of week, MMM is month, dd is day of month, yyyy is year, etc.
//

class LtIpDateTimeStr
{
public:
	enum { STRLEN = 32 };
	char	m_string[STRLEN];
	ULONG	m_dateTime;
	boolean	m_bClean;

	LtIpDateTimeStr()
	{
		m_string[0] = 0;
		m_dateTime = 0;
		m_bClean = false;
	}

	void	setDateTime( ULONG dateTime )
	{	m_dateTime = dateTime;
		m_bClean = false;
	}
	ULONG	getDateTime()
	{	return m_dateTime;
	}
	char*	getString();
	char*	getString( ULONG dateTime );
};

#endif // _LTIPPACKETS_H

