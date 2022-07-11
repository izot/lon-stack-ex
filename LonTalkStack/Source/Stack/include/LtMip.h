#ifndef LTMIP_H
#define LTMIP_H
//
// LtMip.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtMip.h#1 $
//

#include "LtaDefine.h"
#include "LonTalk.h"

#define LT_MIP_DOMAIN_COUNT	2

//
// MIP commands
//
#define MI_MISC				0
#define MI_COMM				1
#define MI_NETMGMT			2
#define MI_RESET			5
#define MI_FLUSH_COMPLETE	6	// uplink
#define MI_FLUSH_CANCEL		6	// downlink
#define MI_ONLINE			7
#define MI_OFFLINE			8
#define MI_FLUSH			9
#define MI_FLUSH_IGNORE		10
#define MI_ESCAPE			14
// Full byte commands
#define MI_UNLOCK_INITIATE  0x51
#define MI_UNLOCK_CHALLANGE 0x52
#define MI_UNLOCK_REPLY     0x53

#define MI_ACK				0xC0
#define MI_NACK				0xC1

//
// Miscellaneous commands (queue values)
//
#define MI_OPERATION		0x0D	// AKA USOP in ShortStack

//
// Escape commands (queue values)
//
#define MI_SSTATUS			0		// Extended status

//
// MI_OPERATION values
//
#define MI_OP_SIGNAL		8		// Get/return signal strength info


//
// MIP queues
//
#define MI_TQ				2
#define MI_TQP				3
#define MI_NTQ				4
#define MI_NTQP				5
#define MI_RESPONSE			6
#define MI_INCOMING			8

#define MI_ESCAPE_SERVICE	6
#define MI_ESCAPE_TXID		8

#define LT_MIP_BUF_SIZE	260

// Layer 2 MI_COMM flags
#define MI_ZEROSYNC			0x08
#define MI_ATTENUATE		0x04

#define MI_L2_FLAGS			(MI_ZEROSYNC | MI_ATTENUATE)

#define MI_DONT_TRANSMIT    0x80	// Steal a bit to tell the driver not to actually send the packet.
									// This is to allow sending "internal-only" packets to the LSPA,
									// which is done in the driver on the iLON.

// MI_ACK/MI_NACK flags.  An MI_ACK and MI_NACK messsages contain
// a single data byte following the 2 byte header.  This data byte is a bitmask
// indicating which types of network buffers are available:
#define MI_NP_NETWORK_BUFFER_AVAIL 0x01     // Non-priority network buffer
#define MI_P_NETWORK_BUFFER_AVAIL  0x02     // Priority network buffer

#if defined(WIN32) || defined(unix)
#pragma pack(1)
    #ifndef BITF_LITTLE_ENDIAN
        #define	BITF_LITTLE_ENDIAN
    #endif

#elif PRODUCT_IS(FTXL)

    #ifndef BITF_LITTLE_ENDIAN
        #define	BITF_LITTLE_ENDIAN
    #endif

#else // Tornado 
/*
	Bitfield Packing:

	As long as the bit fields are declared as unsigned char (or char?),
	the fields will be packed. If a larger type (int) is used, the fields
	will be unpacked unless the following special construct is used:

    typedef struct { 
		int a : 1;
		int b : 1;
	} __attribute__ ((packed)) x

    
	Bitfield Ordering:

    The Tornado compiler for MIPS orders bitfields according to the
	processor endian. Since we use big endian, the bitfields must be
	ordered big endian, which is the reverse of Windows.
*/
#ifndef BITF_BIG_ENDIAN
#define	BITF_BIG_ENDIAN
#endif

#endif

// Address type control bits
#define LT_OVERRIDE		0x40	// Authentication key override
#define LT_LONGTIME		0x10	// Long timers
#define LT_ZERO_SYNC	0x08	// Zero crossing synchronization
#define LT_ATTENUATE	0x04	// Attenuate outgoing packet

#define LT_TYPEMASK		(LT_OVERRIDE | LT_LONGTIME | LT_ZERO_SYNC | LT_ATTENUATE)

typedef struct
{
	// Assumed to be same as packed form of LtAddressConfiguration.
	byte type;
	byte filler[10];
} LtSicbAddrOut;

typedef struct
{
#ifdef BITF_BIG_ENDIAN
	unsigned char sub : 8;
	unsigned char mbo : 1;	
	unsigned char nod : 7;
#else
	unsigned char sub : 8;
	unsigned char nod : 7;
	unsigned char mbo : 1;	
#endif
} LtSicbAddrInSubnetNode;

typedef struct
{
#ifdef BITF_BIG_ENDIAN
	unsigned char sub : 8;
	unsigned char mbo : 1;
	unsigned char nod : 7;
	unsigned char grp : 8;
	unsigned char mem : 8;
#else
	unsigned char sub : 8;
	unsigned char nod : 7;
	unsigned char mbo : 1;
	unsigned char grp : 8;
	unsigned char mem : 8;
#endif
} LtSicbAddrInGroupAck;

typedef struct
{
	unsigned char sub : 8;
	byte uid[6];
} LtSicbAddrInUniqueId;

typedef struct
{
#ifdef BITF_BIG_ENDIAN
	unsigned char dmn : 1;
	unsigned char flex : 1; // 1 if this is the flex domain
	unsigned char fmt : 6;
	unsigned char sub : 8;
	unsigned char grp : 1; // 0 => group ack
	unsigned char nod : 7;
#else
	unsigned char fmt : 6;
	unsigned char flex : 1; // 1 if this is the flex domain
	unsigned char dmn : 1;
	unsigned char sub : 8;
	unsigned char nod : 7;
	unsigned char grp : 1; // 0 => group ack
#endif
	union
	{
		byte grp;
		LtSicbAddrInSubnetNode sn;
		LtSicbAddrInGroupAck ga;
		LtSicbAddrInUniqueId uid;
	} addr;
} LtSicbAddrIn;

typedef struct
{
#ifdef BITF_BIG_ENDIAN
	unsigned char cmd : 4;
	unsigned char que : 4;

	unsigned char len : 8;

	unsigned char mbz : 1;
	unsigned char svc : 2;
	unsigned char auth: 1;
	unsigned char tag : 4;

	unsigned char pri : 1;
	unsigned char pth : 1;
	unsigned char fail: 1;
	unsigned char succ: 1;
	unsigned char exp : 1;
	unsigned char pths: 1;
	unsigned char mbz1: 1;
	unsigned char rsp : 1;

	unsigned char dlen: 8;
#else
	unsigned char que : 4;
	unsigned char cmd : 4;

	unsigned char len : 8;

	unsigned char tag : 4;
	unsigned char auth: 1;
	unsigned char svc : 2;
	unsigned char mbz : 1;

	unsigned char rsp : 1;
	unsigned char mbz1: 1;
	unsigned char pths: 1;
	unsigned char exp : 1;
	unsigned char succ: 1;
	unsigned char fail: 1;
	unsigned char pth : 1;
	unsigned char pri : 1;

	unsigned char dlen: 8;
#endif

#if !PRODUCT_IS(FTXL) || defined(WIN32)
	union
	{
		LtSicbAddrIn in;
		LtSicbAddrOut out;
	} addr;
#endif
	byte     data[1];
} LtSicb;

// These define the traditional SICB size (SICB_NEURON_SIZE) and the size used internally by eVNI
// (SICB_SIZE).  SICB_SIZE includes signal strength information which is added to the end of the 
// SICB but is not reflected in the overall length.  
#define SICB_NEURON_SIZE(s)	((s).len + 2)
#define SICB_SIZE(s)		(SICB_NEURON_SIZE(s) + sizeof(LtOpSignal))

//
// LtL2Sicb
//
// This is the format of the layer 2 frames sent to/from a L2 MIP.  
// For incoming frames, we also get a timing
// value relative to the zero crossing used for phase measurement.
// For outgoing frames, the field is used to indicate whether we do 
// zero crossing synchronization or not.
//
typedef struct
{
#ifdef BITF_BIG_ENDIAN
	unsigned char cmd : 4;
	unsigned char que : 4;
	unsigned char len : 8;
#else
	unsigned char que : 4;
	unsigned char cmd : 4;
	unsigned char len : 8;
#endif
#ifndef WIN32
	// On the PC when we use L2 VNI, we don't have access to this extra field.
	unsigned char zerocross;
#endif
	byte     data[255];
} LtL2Sicb;

// LDVX commands - (SICB).
typedef  struct
{
    byte command;   // Must be LDVX_NICMD_CODE
    byte length;    // Length of data + 1
    byte subCommand; // Of type LDVX_NICMD

// Data, if any, follows
}  LdvXNiCmdHeader;

// Misc and Escape commands
typedef struct
{
	byte command;
	byte length;
	byte subCommand;
	byte data[1];
} LdvMisc;

typedef struct
{
#ifdef BITF_BIG_ENDIAN
	unsigned char cmd    : 4;
	unsigned char subcmd : 4;

	unsigned char len    : 8;

	unsigned char data   : 8;
#else
	unsigned char subcmd : 4;
	unsigned char cmd    : 4;

	unsigned char len    : 8;

	unsigned char data   : 8;
#endif
} LtLocal;
#define LT_LOCAL_DATA_OFFSET 2

// Note that in some implementations, Reg4 and Reg5 are the same.  It is assumed only the register indicated
// as valid by Reg3 is worth looking at.
typedef struct
{
	byte		operation;		// Which operations?
	byte		status;			// 1 => valid;
	byte		reg1;			// The first register (Reg3 of PLC)
	byte		reg2;			// The second register (Reg4 of PLC)
	byte		reg3;			// The third register (Reg5 of PLC)
} OpSignal;

// Form of the data sent to the application
typedef struct
{
	BYTE		valid;			// 1=>valid
	BYTE		oaoo;			// 1=>one and only one
	BYTE		secondary;		// 1=>secondary
	BYTE		margin;			// Encoded margin
	BYTE		strength;		// Encoded signal
} LtOpSignal;

typedef struct 
{
	byte		mode;
	byte		addresshi;
	byte		addresslo;
	byte		len;
	byte		flags;
	byte	    data[LT_MIP_BUF_SIZE];
} LtWriteMemory;

typedef struct
{
	byte		mode;
	byte		addresshi;
	byte		addresslo;
	byte		len;
} LtReadMemory;

typedef struct
{
	byte		index;
	byte		address[5];
} LtUpdateAddress;

typedef struct
{
	byte		cmd;
	byte		state;
} LtChangeState;

typedef struct
{
	byte		nsa;
	byte		key[LT_MIP_DOMAIN_COUNT][LT_CLASSIC_DOMAIN_KEY_LENGTH];
} LtNsaData;

typedef struct 
{
    unsigned short  xmit_errors;
    unsigned short  transaction_timeouts;
    unsigned short  rcv_transaction_full;
    unsigned short  lost_msgs;
    unsigned short  missed_msgs;
    byte            reset_cause;           
    byte            node_state;
    byte            version_number;
    byte            error_log;
    byte            model_number;
} LtStatusRespData;


#if defined(WIN32) || defined(unix)
#pragma pack()
#endif

#ifdef __cplusplus
extern "C"
{
#endif
	LtErrorType determineMipType(class LtLtLogicalChannel *pChannel, int &minLayer, int &maxLayer, boolean &bNsaMip, int &xcvrId, int &ldvHandle);

#ifdef __cplusplus
}
#endif

#endif
