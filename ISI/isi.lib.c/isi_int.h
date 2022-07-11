// Filename:  isi_int h  
//
// Copyright © 2013-2022 Dialog Semiconductor
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
// Description: internal header file for the ISI implementation
//
#ifndef __ISIINT_H__
#	define	__ISIINT_H__

//  pull-in public ISI stuff:
#ifndef ISI_DEBUG_IMPLEMENT_LIBRARY
#define ISI_IMPLEMENT_LIBRARY 
#endif  //  ISI_DEBUG_IMPLEMENT_LIBRARY

#include "LtaDefine.h"
#include "IsiApi.h"
#include <string.h>

#ifdef  __cplusplus
extern "C" {
#endif

//  define ISI_COMPACT for the small version (will be FULL of COMPACT is not defined)
#ifdef IsiCompactAuto
#define ISI_COMPACT
#endif
#ifdef IsiCompactManual
#define ISI_COMPACT
#endif

#ifdef ISI_COMPACT
#ifdef IsiCompactManual
#	define ISI_SUPPORT_MANUAL_CONNECTIONS
#else
#	define ISI_SUPPORT_AUTOMATIC_CONNECTIONS
#endif
//#	define ISI_SUPPORT_HEARTBEATS
//#	define ISI_SUPPORT_CONNECTION_REMOVAL
//#	define ISI_SUPPORT_ALIAS
//#	define ISI_SUPPORT_DIAGNOSTICS             // enable IsiUpdateDiagnostist
//#	define ISI_SUPPORT_TIMG
//#	define ISI_SUPPORT_DADAS
//# define ISI_SUPPORT_TURNAROUNDS
#else  // not compact (aka FULL):
#   define ISI_SUPPORT_HEARTBEATS
#   define ISI_SUPPORT_CONNECTION_REMOVAL
#   define ISI_SUPPORT_ALIAS
#   define ISI_SUPPORT_DIAGNOSTICS             // enable IsiUpdateDiagnostist
#   define ISI_SUPPORT_TIMG
#   define ISI_SUPPORT_MANUAL_CONNECTIONS
#   define ISI_SUPPORT_AUTOMATIC_CONNECTIONS
#   define ISI_SUPPORT_DADAS
#   define ISI_SUPPORT_TURNAROUNDS
#   define ISI_SUPPORT_CONTROLLED_CONNECTIONS
#endif  //  ISI_COMPACT

//  common defines
#define ISI_PROTOCOL_VERSION        3u
#define ISI_IMPLEMENTATION_VERSION  4u
#define ISI_DEFAULT_DEVICECOUNT     32u
#define ISI_MINIMUM_DEVICECOUNT     4u
#define ISI_MAX_CONNECTION_COUNT    256
#define ISI_WIDTH_PER_CONNTAB       4u			// Note: certain code paths assume this value (look for "ASSUMES")
#define ISI_SELECTOR_MASK           0x2FFFul
#define ISI_MESSAGE_CODE            0x3Du
#define ISI_PRIMARY_DOMAIN_INDEX    0u			// Note: certain code paths assume this value (look for "ASSUMES")
#define ISI_SECONDARY_DOMAIN_INDEX  1u
#define ISI_SECONDARY_SUBNET_ID     1u
#define ISI_SECONDARY_NODE_ID       1u
#define ISI_NOT_ACCEPTABLE         255u			// Note: certain code paths assume this value (look for "ASSUMES")
#define ISI_ADPU_OFFSET             11u
#define ISI_MAX_ADDRESS_TABLE_SIZE	254u        // was 15u
#define ISI_MAX_ALIAS_COUNT         254u
#define ISI_MAX_NV_COUNT            254u
#define	ISI_ALIAS_UNUSED			0xFFFFu
#define ISI_NO_ADDRESS              0xFFFFu
#define ISI_T_ACQ                   (5ul*60ul*ISI_TICKS_PER_SECOND)
#define ISI_T_CSMR_PAUSE            15  //  these are seconds minimum hesitation after DIDCF
#define ISI_T_CSMR                  (60u*ISI_TICKS_PER_SECOND)
#define ISI_T_AUTO                  (30u*ISI_TICKS_PER_SECOND)
#define ISI_T_TIMG                  (60u*ISI_TICKS_PER_SECOND)
#define ISI_T_ENROLL                ISI_T_ACQ
#define ISI_T_CSMO                  (5u * ISI_TICKS_PER_SECOND)
#define ISI_T_CSME					ISI_T_CSMO

#define ISI_T_RM                    (5u*ISI_TICKS_PER_SECOND)
#define ISI_DIDRQ_RETRIES           20u
#define ISI_DIDRQ_PAUSE             (5u*ISI_T_RM)
#define ISI_T_COLL                  ((3ul*ISI_TICKS_PER_SECOND)/2u)
#define ISI_T_CF                    (ISI_T_ACQ/5u)
#define ISI_DIDRM_RETRIES           3
#define ISI_T_QDR                   (1+ISI_TICKS_PER_SECOND)
#define ISI_T_UDR                   (2+ISI_TICKS_PER_SECOND)

#define ISI_NV_UPDATE_RETRIES		3u
#define ISI_SUBNET_BUCKET_SIZE      64u
#define ISI_SUBNET_START_TPFT       64u
#define ISI_SUBNET_START_PL20       128u
#define ISI_SUBNET_START_OTHER		192u
#define	ISI_MESSAGE_HEADROOM		4u		// see approve.c

#define LONTALK_SERVICE_PIN_MESSAGE 0x7Fu
#define LONTALK_WINK_MESSAGE        0x70u
#define LONTALK_QUERY_DOMAIN_MESSAGE 0x6A
#define LONTALK_QUERY_DOMAIN_SUCCESS 0x2A
#define LONTALK_QUERY_DOMAIN_FAILURE 0x0A

#define LONTALK_UPDATE_DOMAIN_MESSAGE 0x63
#define LONTALK_UPDATE_DOMAIN_SUCCESS 0x23
#define LONTALK_UPDATE_DOMAIN_FAILURE 0x03

#define ISI_WINK_REPEATS            3
#define ISI_QUERY_DOMAIN_RETRIES    3
#define ISI_UPDATE_DOMAIN_RETRIES   3
#define ISI_NVHB_REPEATS            1
#define ISI_CTR_RETRIES             3
#define ISI_RDC_RETRIES             3

#define NEURON_ID_LEN               LON_UNIQUE_ID_LENGTH

#define MAX_DOMAINS      2          /* Maximum # of domains allowed.         */
#define MAX_CONNECTION_TBL_ENTRIES  255 
#define NUM_ADDR_TBL_ENTRIES    5   /* # of entries in addr tbl    */
#define NV_TABLE_SIZE          40   /* Check management tool for any restriction on maximum size */
#define NUM_ADDR_TBL_SIZE
#define ISI_MESSAGE_TAG         0x0F

#define DOMAIN_ID_LEN    6
#define AUTH_KEY_LEN     6
#define ID_STR_LEN       8

#ifndef WIN32

/*------------------------------------------------------------------------------
Section: Macro Definitions
------------------------------------------------------------------------------*/
#define MIN(x,y) (((x)<(y))?(x):(y))
#define MAX(x,y) (((x)>(y))?(x):(y))

#define max(a, b) MAX(a, b)
#define min(a, b) MIN(a, b)

#endif
// The following are the enumerations that indicate the amount of initialization work
// required. 
typedef	enum
{
    isiReboot = 0,
	isiReset,
	isiRestart
} 	IsiBootType;

// The following are possible reasons for a node losing its persistent data
typedef enum
{
	LT_CORRUPTION				= 0x00,		// Image checksum invalid.
	LT_PROGRAM_ID_CHANGE		= 0x01,		// Program ID changed
	LT_SIGNATURE_MISMATCH		= 0x02,		// Image signature mismtach.  Could be corruption
											// or change to the persistent data format.
	LT_PROGRAM_ATTRIBUTE_CHANGE = 0x03,		// Number of NVs, aliases, address or domain
											// entries changed.
	LT_PERSISTENT_WRITE_FAILURE = 0x04,		// Could not write the persistence file.
	LT_NO_PERSISTENCE			= 0x05,		// No persistence found.
	LT_RESET_DURING_UPDATE		= 0x06,		// Reset or power cycle occurred while
											// configuration changes were in progress.
	LT_VERSION_NOT_SUPPORTED	= 0x07,		// Version number not supported

	LT_PERSISTENCE_OK			= -1
} LtPersistenceLossReason;



//  ISI variables fall into two sections, persistent ones, and not persistent
//  ones. Both are held in one structure each. The structures are defined here
//  (and implemented in vars.c. The defaults are also set there)
typedef struct {
#ifdef  ISI_SUPPORT_TIMG
    LonByte    Devices;           // the latest device count
#endif  //  ISI_SUPPORT_TIMG
    LonByte    Nuid;              // non-unique device ID
    LonBits16 Serial;          // running serial number of CIDs
	// The next field is an enumeration that indicates the amount of initialization work
	// required. The field is initialized to "isiReset", because the Neuron Exporter already
	// gives us cleared-out NV, alias, and address tables. For a normal application download,
	// this significantly reduces the time the node is not responsive - making NSS happy.
	// The ReturnToFactoryDefaults routine resets this field to isiReboot, thereby causing an
	// entire reinitialization of all tables - the right thing to do when transitioning back from
	// a managed to a self-installed network. Finally, when the initialization routine has done
	// its job one way or another, it changes this field to isiRestart so that a subsequent reset
	// only causes normal reset operations, but no wiping out of system tables, etc.
	IsiBootType BootType;
	LonByte	RepeatCount;	// used for NV updates (i.e. the address table); only 1,2, or 3 are supported.
} IsiPersist;

extern IsiPersist _isiPersist;
extern IsiType gIsiType;
extern IsiFlags gIsiFlags;
extern unsigned gConnectionsTableSz;
extern unsigned gDIDLen;
extern LonDomainId gDomainID;
extern unsigned gRepeatCount;
// extern LonNvEcsConfig nvConfigTable[NV_TABLE_SIZE];
extern unsigned char globalExtended;
extern unsigned char gIsiDerivableAddr;  // flag to indicate that the IP address is derivabled or not

//  Following is the structure for volatile (RAM) data.
typedef enum {
    isiChannelTpFt    = 0x04,
    isiChannelPl20A   = 0x0F,
    isiChannelPl20C   = 0x10,
    isiChannelPl20N   = 0x11,
    isiChannelIp852   = 0x9A,
    isiChannelIzotIp  = 0x00
} IsiChannelType;

typedef struct {
    unsigned    RepeatTimer;    // encoded (and pre-shifted into normal address table location)
    unsigned    TransmitTimer;  // encoded
    unsigned    GroupRcvTimer;  // encoded
    unsigned    NonGroupTimer;  // encoded
    unsigned    BaseSubnet;
    unsigned    TicksPerSlot;   // width of ISI broadcast slots in ticks. Must be >> SpreadingInterval
    unsigned    SpreadingInterval;  //  width of the spreading interval. Must be >= 2 x JitterInterval. Note JitterInterval is fixed at +- 1 (see period.c)
} IsiTransport;

typedef enum {
    isiPeriodicTypeDrum = 0,  // isiPeriodicTypeDRUM *must* be zero
    isiPeriodicTypeCsmr,
    isiPeriodicTypeCsmi,
    isiPeriodicTypeNvHb,
    isiPeriodicTypeApplication,
    isiPeriodicTypeTimg,
    //  insert new mode before this comment. Make sure there can only by a total of 8 modes, starting with
    //  isiPeriodicTypeDrum (0) and ending with isiPeriodicTypeXYZ (7). For more modes, Ticks.c must be
    //  restructured to meet the requirement that at least every 8th periodic message must be a DRUM message.
    isiPeriodicTypes
} _IsiPeriodicType;

typedef struct {
    unsigned    drumPause;   // use _IsiPeriodicType
	enum {
		slotCsmr, slotCsmi, slotNvHb, slotAppl, slotTimg
	}			slotUsage;
	unsigned	lastConnectionIdx;
} _IsiPeriodic;

// Note that these enumerations are powers of 2 so that they can be checked using bit operations.
// If there are more than 8 non-zero states required, then additional state bytes need to be defined.
typedef enum {
    isiStateNormal			= 0x00,	// make sure this remains zero
    // enrollment states:
    isiStateInviting		= 0x01u,	// about to become a host, not heard of anybody back yet
    isiStatePlannedParty	= 0x02u,	// about to become a host, have at least one guest
    isiStateInvited			= 0x04u,	// have been invited but not accepted yet
    isiStateAccepted		= 0x08u,	// have been invited and accepted invitation
    // device and domain acquisition states:
    isiStateAwaitDidrx      = 0x10u, // ISI-DA: wait for DIDRM, ISI-DAS: wait for DIDRQ
    isiStateAwaitConfirm    = 0x20u, // ISI-DA: wait for DIDCF, ISI-DAS: wait for IsiStartDeviceAcquisition()
    isiStateCollect         = 0x40u, // ISI-DA: collect DIDRM, ISI-DAS: collect service pin 1 and 2
    isiStateAwaitQdr        = (int)0x80u, // ISI-DAS only: await query domain response
    isiStatePause           = isiStateAwaitQdr  // ISI-DA: Wait before issueing new DIDRM
} IsiState;

#ifdef  ISI_SUPPORT_DADAS
//  DAS devices support an extended set of state flags - note these only exist on DAS (implemented in msgdas.c)
typedef enum {
    isiDasNormal                = 0x00,    // no special state modifier values
    isiDasAutoDeviceAcquisition = 0x01,    // automatic device acquisition following a domain fetch process; see msgdas.c for details.
    isiDasFetchDomain           = 0x02,    // regular DAS state bits refer to a normal FetchDomain process (obtain remote domain ID)
    isiDasFetchDevice_Query     = 0x04,    // regular DAS state bits refer to a FetchDevice process (set remote domain ID): obtain remote domain id
    isiDasFetchDevice_Confirm   = 0x08,    // regular DAS state bits refer to a FetchDevice process (set remote domain ID): await positive response to assigning remote ID
    isiDasAwaitDidrx            = 0x10,
    isiDasAwaitConfirm          = 0x20,
    isiDasCollect               = 0x40,
    isiDasAwaitQdr              = (int)0x80u
} IsiDasExtendedStates;
extern IsiDasExtendedStates isiDasExtState;
#endif  //  ISI_SUPPORT_DADAS

#define HOST_STATES			(isiStateInviting|isiStatePlannedParty)
#define GUEST_STATES		(isiStateInvited|isiStateAccepted)
#define CONNECTION_STATES	(HOST_STATES | GUEST_STATES)
#define ACQUISITION_STATES  (isiStateAwaitDidrx | isiStateAwaitConfirm | isiStateCollect | isiStateAwaitQdr)

typedef struct {
    LonBool         Running;
    IsiFlags        Flags;      // public enumeration IsiFlags plus ISI_FLAG_* macros
    IsiState        State;
    IsiChannelType  ChannelType;    // channel type, used to determine subnet bucket and IsiTransport pointer
    IsiTransport    Transport;    // current transport parameter record
    unsigned long   Wait;       // number of ticks Tick*() is currently waiting before anything happens
    unsigned long   Startup;    // number of ticks since start, stops at 0xFFFF. Used to determine Tcsmr and Tauto events
    unsigned long   Timeout;    // number of ticks, counting down from some timeout. 1==due, 0=off
    unsigned        ShortTimer;
	unsigned		Group;		// the group ID for a pending connection. re-used during domain/device acquisition.
    unsigned        Spreading;  // tick counter used for spreading
    _IsiPeriodic    Periodic;
    unsigned        ConnectionTableSize;
    unsigned        pendingConnection;
    unsigned long   specialDrum;  // if the broadcaster starts too late, we might issue a single DRUM before the broadcaster actually starts
} IsiVolatile;
extern IsiVolatile _isiVolatile;
extern IsiMessage isi_out;							// the global ISI message buffer
extern const unsigned _isiMessageLengthTable[];		//	 see approve.c for details

extern void _IsiInitialize(IsiFlags Flags);
extern unsigned _IsiRand(unsigned maximum, unsigned offset);
extern void	_IsiVerifyDomainsS(void);
extern unsigned _IsiAllocSubnet(void);
unsigned _GetIsiSubnet(void);
extern unsigned _IsiAllocNode(void);
unsigned _GetIsiNode(void);
extern LonBool _IsiEnableAddrMgmt(void);
extern LonBool _GetDomainIdFromAddr(LonByte* pDom, unsigned int* didLen);
#ifdef  ISI_SUPPORT_TIMG
    extern unsigned long _IsiAllocSlot(unsigned Devices);
    extern unsigned long _IsiGetPeriod(unsigned Devices);
#else
    extern unsigned long _IsiAllocSlot(void);
    extern unsigned long _IsiGetPeriod(void);
#endif  //  ISI_SUPPORT_TIMG
extern void _IsiSelectTransportProps(IsiChannelType Channel);
extern void _IsiConditionalDiagnostics(IsiDiagnostic Event, unsigned Parameter);
extern LonBool _IsiSetDomain(unsigned Idx, unsigned Len, const LonByte* pId, unsigned Subnet, unsigned Node);

#if PRODUCT_IS(IZOT) 
extern void _IsiBroadcast(IsiMessageCode code, unsigned domain, unsigned repeats);
#else
// Macro used for building up the IsiBroadcast parameter.  ASSUMES: IsiMessageLength <= 31 && Repeats <= 3
// 15-June-05, BG: Note the IsiMessageLength now includes the size of the IsiMessageHeader structure; the assembly routine that propagates the
// message no longer accounts for that. This is changed so that we can use a single lookup table to determine the correct length for each
// message both outbound and inbound (_isiMessageLengthTable, see approve.c).
#define _IsiBroadcast(Code, Domain, Repeats) _IsiBroadcastAsm(((Domain)<<7) | ((Repeats)<<5) | (Code))
//extern void _IsiBroadcast(IsiMessageCode Code, unsigned Domain, unsigned Repeats);
extern void _IsiBroadcastAsm(unsigned Parms);
#endif


#ifdef  ISI_SUPPORT_TURNAROUNDS
extern LonBool _IsiHaveAtLeastOneOutputNv(unsigned Assembly);
#endif  //  ISI_SUPPORT_TURNAROUNDS

//
// These macros are used for defining a forwarder and forwardee routine for certain APIs.
// The forwarder and forwardee have the same code.  At most one will be linked into any given
// application.  If the default version of the API is used, the forwarder is linked in.  If
// the app defines its own version of the API, it can then call the forwardee from that API
// and only the forwardee is linked in.  If the application version chooses not to call the
// forwardee, then neither is linked in.
//
#ifdef FORWARDER
#define FWD(fwder,fwdee) fwder
#else
#define FWD(fwder,fwdee) fwdee
#endif

#ifdef	ISI_SUPPORT_TIMG
#define PersistDevices _isiPersist.Devices
#else
#define PersistDevices
#endif

//
// The following is an alternate form of the address structure.  This isn't likely to change
// any time soon.  It is defined to reduce bitfield overhead in certain paths.
//
#define ADDR_GROUP_MASK	0x80	// MSB of typeSize => group
typedef struct {
	LonByte	typeSize;
	LonByte	member;
	LonByte	timer1;
	LonByte	timer2;
	LonByte	group;
} address_struct_alt;

// Typedefs for the read_only_data_struct:  (from access.h)
typedef struct nv_fixed_struct {
    LonByte    nv_sync     : 1;
    LonByte                : 2;
    LonByte    nv_length   : 5;
    void *      nv_address;
} nv_fixed_struct;

// Network management response structures (from netmgmt.h)
typedef struct NM_query_domain_response {
    LonByte	    id[ DOMAIN_ID_LEN ];
    LonByte	    subnet;
    LonByte	    must_be_one    : 1;
    LonByte	    node	   : 7;
    LonByte	    len;
    LonByte	    key[ AUTH_KEY_LEN ];
} NM_query_domain_response;

typedef struct NM_update_domain_request {
    LonByte	    domain_index;
    LonByte	    id[ DOMAIN_ID_LEN ];
    LonByte	    subnet;
    LonByte	    must_be_one : 1;	// this bit must be set to 1
    LonByte	    node	: 7;
    LonByte	    len;
    LonByte	    key[ AUTH_KEY_LEN ];
} NM_update_domain_request;

typedef struct NM_service_pin_msg
{
    LonByte	    neuron_id[ NEURON_ID_LEN ];
    LonByte	    id_string[ ID_STR_LEN ];
} NM_service_pin_msg;

/* NM_query_domain */
typedef struct NM_query_domain_request 
{
    LonByte        code;
    LonByte        domain_index;
} NM_query_domain_request;

#ifdef MSC
#pragma pack(1)
#endif

/***************************** Start ni_msg.h *****************************/

/*
 ****************************************************************************
 * Application buffer structures for sending and receiving messages to and
 * from a network interface.  The 'ExpAppBuffer' and 'ImpAppBuffer'
 * structures define the application buffer structures with and without
 * explicit addressing.  These structures have up to four parts:
 *
 *   Network Interface Command (NI_Hdr)                        (2 Bytes)
 *   Message Header (MsgHdr)                                   (3 Bytes)
 *   Network Address (ExplicitAddr)                            (11 Bytes)
 *   Data (MsgData)                                            (varies)
 *
 * Network Interface Command (NI_Hdr):
 *
 *   The network interface command is always present.  It contains the
 *   network interface command and queue specifier.  This is the only
 *   field required for local network interface commands such as niRESET.
 *
 * Message Header (MsgHdr: union of NetVarHdr and ExpMsgHdr):
 *
 *   This field is present if the buffer is a data transfer or a completion
 *   event.  The message header describes the type of LONTALK message
 *   contained in the data field.
 *
 *   NetVarHdr is used if the message is a network variable message and
 *   network interface selection is enabled.
 *
 *   ExpMsgHdr is used if the message is an explicit message, or a network
 *   variable message and host selection is enabled (this is the default
 *   for the SLTA).
 *
 * Network Address (ExplicitAddr:  SendAddrDtl, RcvAddrDtl, or RespAddrDtl)
 *
 *   This field is present if the message is a data transfer or completion
 *   event, and explicit addressing is enabled.  The network address
 *   specifies the destination address for downlink application buffers,
 *   or the source address for uplink application buffers.  Explicit
 *   addressing is the default for the SLTA.
 *
 *   SendAddrDtl is used for outgoing messages or NV updates.
 *
 *   RcvAddrDtl is used  for incoming messages or unsolicited NV updates.
 *   RespAddrDtl is used for incoming responses or NV updates solicited
 *   by a poll.
 *
 * Data (MsgData: union of UnprocessedNV, ProcessedNV, and ExplicitMsg)
 *
 *   This field is present if the message is a data transfer or completion
 *   event.
 *
 *   If the message is a completion event, then the first two Bytes of the
 *   data are included.  This provides the NV index, the NV selector, or the
 *   message code as appropriate.
 *
 *   UnprocessedNV is used if the message is a network variable update, and
 *   host selection is enabled. It consists of a two-Byte header followed by
 *   the NV data.
 *   ProcessedNV is used if the message is a network variable update, and
 *   network interface selection is enabled. It consists of a two-Byte header
 *   followed by the NV data.
 *
 *   ExplicitMsg is used if the message is an explicit message.  It consists
 *   of a one-Byte code field followed by the message data.
 *
 * Note - the fields defined here are for a little-endian (Intel-style)
 * host processor, such as the 80x86 processors used in PC compatibles.
 * Bit fields are allocated right-to-left within a Byte.
 * For a big-endian (Motorola-style) host, bit fields are typically
 * allocated left-to-right.  For this type of processor, reverse
 * the bit fields within each Byte.  Compare the NEURON C include files
 * ADDRDEFS.H and MSG_ADDR.H, which are defined for the big-endian NEURON
 * CHIP.
 */

/*
 ****************************************************************************
 * Network Interface Command data structure.  This is the application-layer
 * header used for all messages to and from a LONWORKS network interface.
 ****************************************************************************
 */

/* Literals for the 'cmd.q.queue' nibble of NI_Hdr. */

typedef enum {
    niTQ          =  2,             /* Transaction queue                    */
    niTQ_P        =  3,             /* Priority transaction queue           */
    niNTQ         =  4,             /* Non-transaction queue                */
    niNTQ_P       =  5,             /* Priority non-transaction queue       */
    niRESPONSE    =  6,             /* Response msg & completion event queue*/
    niINCOMING    =  8              /* Received message queue               */
} NI_Queue;

/* Literals for the 'cmd.q.q_cmd' nibble of NI_Hdr. */

//typedef enum {
//    niCOMM           = 1,           /* Data transfer to/from network        */
//    niNETMGMT        = 2            /* Local network management/diagnostics */
//} NI_QueueCmd;

/* Literals for the 'cmd.noq' Byte of NI_Hdr. */

typedef enum {
    niNULL           = 0x00,
    niTIMEOUT        = 0x30,        /* Not used                             */
    niCRC            = 0x40,        /* Not used                             */
    niRESET          = 0x50,
    niFLUSH_COMPLETE = 0x60,        /* Uplink                               */
    niFLUSH_CANCEL   = 0x60,        /* Downlink                             */
    niONLINE         = 0x70,
    niOFFLINE        = 0x80,
    niFLUSH          = 0x90,
    niFLUSH_IGN      = 0xA0,
    niSLEEP          = 0xB0,
    niACK            = 0xC0,
    niNACK           = 0xC1,        /* SLTA only                            */
    niSSTATUS        = 0xE0,
    niPUPXOFF        = 0xE1,
    niPUPXON         = 0xE2,
    niPTRHROTL       = 0xE4,        /* Not used                             */
    niIRQENA         = 0xE5,
//    niSERVICE        = 0xE6,
    niTXID           = 0xE8,
    niSLTAPLS        = 0xEA,
    niDRV_CMD        = 0xF0,        /* Not used                             */
} NI_NoQueueCmd;

#define niWAIT_TIME 5               // Timeout for network interface

/*
 * Header for network interface messages.  The header is a union of
 * two command formats: the 'q' format is used for the niCOMM and
 * niNETMGMT commands that require a queue specification; the 'noq'
 * format is used for all other network interface commands.
 * Both formats have a length specification where:
 *
 *      length = header (3) + address field (11 if present) + data field
 *
 * WARNING:  The fields shown in this structure do NOT reflect the actual
 * structure required by the network interface.  Depending on the network
 * interface, the network driver may change the order of the data and add
 * additional fields to change the application-layer header to a link-layer
 * header.  See the description of the link-layer header in Chapter 2 of the
 * Host Application Programmer's Guide.
 */
typedef union {
	LON_STRUCT_BEGIN(q) {
#ifdef BITF_LITTLE_ENDIAN
        LonUbits8 queue  :4;     /* Network interface message queue      */
                                 /* Use value of type 'NI_Queue'         */
        LonUbits8 q_cmd  :4;     /* Network interface command with queue */
                                 /* Use value of type 'NI_QueueCmd'      */
#else
        LonUbits8 q_cmd  :4;     /* Network interface command with queue */
                                 /* Use value of type 'NI_QueueCmd'      */
        LonUbits8 queue  :4;     /* Network interface message queue      */
                                 /* Use value of type 'NI_Queue'         */
#endif
        LonByte length;          /* Length of the buffer to follow       */
    } LON_STRUCT_END(q);         /* Queue option                         */
    LON_STRUCT_BEGIN(noq) {
        LonByte     cmd;         /* Network interface command w/o queue  */
                                 /* Use value of type 'NI_NoQueueCmd'    */
        LonByte     length;      /* Length of the buffer to follow       */
    } LON_STRUCT_END(noq);		 /* No queue option                      */
} NI_Hdr;

/*
 ****************************************************************************
 * Message Header structure for sending and receiving explicit
 * messages and network variables which are not processed by the
 * network interface (host selection enabled).
 ****************************************************************************
 */

typedef enum {
   ACKD            = 0,
   UNACKD_RPT      = 1,
   UNACKD          = 2,
   REQUEST         = 3
} ServiceType;

/* Literals for 'cmpl_code' fields of ExpMsgHdr and NetVarHdr. */

typedef enum {
    MSG_NOT_COMPL  = 0,         /* Not a completion event              */
    MSG_SUCCEEDS   = 1,         /* Successful completion event         */
    MSG_FAILS      = 2          /* Failed completion event             */
} ComplType;

/* Explicit message and Unprocessed NV Application Buffer. */
typedef LON_STRUCT_BEGIN(ExpMsgHdr) {
#ifdef BITF_LITTLE_ENDIAN
    LonUbits8   tag       :4;   /* Message tag for implicit addressing  */
                                /* Magic cookie for explicit addressing */
    LonUbits8   auth      :1;   /* 1 => Authenticated                   */
    LonUbits8   st        :2;   /* Service Type - see 'ServiceType'     */
    LonUbits8   msg_type  :1;   /* 0 => explicit message                */
                                /*      or unprocessed NV               */
/*--------------------------------------------------------------------------*/
    LonUbits8   response  :1;   /* 1 => Response, 0 => Other            */
    LonUbits8   pool      :1;   /* 0 => Outgoing                        */
    LonUbits8   alt_path  :1;   /* 1 => Use path specified in 'path'    */
                                /* 0 => Use default path                */
    LonUbits8   addr_mode :1;   /* 1 => Explicit addressing,            */
                                /* 0 => Implicit (outgoing only)        */
                                /* 1 => Duplicate (incoming only)       */
    LonUbits8   cmpl_code :2;   /* Completion Code - see 'ComplType'    */
    LonUbits8   path      :1;   /* 1 => Use alternate path,             */
                                /* 0 => Use primary path                */
                                /*      (if 'alt_path' is set)          */
    LonUbits8   priority  :1;   /* 1 => Priority message                */
/*--------------------------------------------------------------------------*/
    LonByte   length;           /* Length of msg or NV to follow        */
                                /* not including any explicit address   */
                                /* field, includes code Byte or         */
                                /* selector Bytes                       */
#else
    LonUbits8   msg_type  :1;   /* 0 => explicit message                */
                                /*      or unprocessed NV               */
    LonUbits8   st        :2;   /* Service Type - see 'ServiceType'     */
    LonUbits8   auth      :1;   /* 1 => Authenticated                   */
    LonUbits8   tag       :4;   /* Message tag for implicit addressing  */
                                /* Magic cookie for explicit addressing */
/*--------------------------------------------------------------------------*/
    LonUbits8   priority  :1;   /* 1 => Priority message                */
    LonUbits8   path      :1;   /* 1 => Use alternate path,             */
                                /* 0 => Use primary path                */
                                /*      (if 'alt_path' is set)          */
    LonUbits8   cmpl_code :2;   /* Completion Code - see 'ComplType'    */
    LonUbits8   addr_mode :1;   /* 1 => Explicit addressing,            */
                                /* 0 => Implicit                        */
                                /* Outgoing buffers only                */
    LonUbits8   alt_path  :1;   /* 1 => Use path specified in 'path'    */
                                /* 0 => Use default path                */
    LonUbits8   pool      :1;   /* 0 => Outgoing                        */
    LonUbits8   response  :1;   /* 1 => Response, 0 => Other            */
    
/*--------------------------------------------------------------------------*/
    LonByte   length;           /* Length of msg or NV to follow        */
                                /* not including any explicit address   */
                                /* field, includes code Byte or         */
                                /* selector Bytes                       */
#endif
} LON_STRUCT_END(ExpMsgHdr);

/*
 ****************************************************************************
 * Message Header structure for sending and receiving network variables
 * that are processed by the network interface (network interface
 * selection enabled).
 ****************************************************************************
 */
typedef LON_STRUCT_BEGIN(NetVarHdr) {
#ifdef BITF_LITTLE_ENDIAN
    LonUbits8   tag       :4;   /* Magic cookie for correlating         */
                                /* responses and completion events      */
    LonUbits8   rsvd0     :2;
    LonUbits8   poll      :1;   /* 1 => Poll, 0 => Other                */
    LonUbits8   msg_type  :1;   /* 1 => Processed network variable      */
/*--------------------------------------------------------------------------*/
    LonUbits8   response  :1;   /* 1 => Poll response, 0 => Other       */
    LonUbits8   pool      :1;   /* 0 => Outgoing                        */
    LonUbits8   trnarnd   :1;   /* 1 => Turnaround Poll, 0 => Other     */
    LonUbits8   addr_mode :1;   /* 1 => Explicit addressing,            */
                                /* 0 => Implicit addressing             */
    LonUbits8   cmpl_code :2;   /* Completion Code - see above          */
    LonUbits8   path      :1;   /* 1 => Used alternate path             */
                                /* 0 => Used primary path               */
                                /*      (incoming only)                 */
    LonUbits8   priority  :1;   /* 1 => Priority msg (incoming only)    */
/*--------------------------------------------------------------------------*/
    LonByte   length;           /* Length of network variable to follow */
                                /* not including any explicit address   */
                                /* not including index and rsvd0 Byte   */
#else
    LonUbits8   msg_type  :1;   /* 1 => Processed network variable      */
    LonUbits8   poll      :1;   /* 1 => Poll, 0 => Other                */
    LonUbits8   rsvd0     :2;
    LonUbits8   tag       :4;   /* Magic cookie for correlating         */
                                /* responses and completion events      */
/*--------------------------------------------------------------------------*/
    LonUbits8   priority  :1;   /* 1 => Priority msg (incoming only)    */
    LonUbits8   path      :1;   /* 1 => Used alternate path             */
                                /* 0 => Used primary path               */
                                /*      (incoming only)                 */
    LonUbits8   cmpl_code :2;   /* Completion Code - see above          */
    LonUbits8   addr_mode :1;   /* 1 => Explicit addressing,            */
                                /* 0 => Implicit addressing             */
    LonUbits8   trnarnd   :1;   /* 1 => Turnaround Poll, 0 => Other     */
    LonUbits8   pool      :1;   /* 0 => Outgoing                        */
    LonUbits8   response  :1;   /* 1 => Poll response, 0 => Other       */
/*--------------------------------------------------------------------------*/
    LonByte   length;           /* Length of network variable to follow */
                                /* not including any explicit address   */
                                /* not including index and rsvd0 Byte   */
#endif
} LON_STRUCT_END(NetVarHdr);


/* Union of all message headers. */

typedef union {
    ExpMsgHdr  exp;
    NetVarHdr  pnv;
    LonByte    cmd[1];       /* For immediate commands with parameters */
} MsgHdr;

/*
 ****************************************************************************
 * Network Address structures for sending messages with explicit addressing
 * enabled.
 ****************************************************************************
 */

/* Literals for 'type' field of destination addresses for outgoing messages. */

typedef enum {
    UNASSIGNED     = 0,
    SUBNET_NODE    = 1,
    NEURON_ID      = 2,
    BROADCAST      = 3,
    // The following are not real destination address types
    // They are interpreted in ni_send_msg_wait()
    SNODE_FARSIDE  = SUBNET_NODE | 0x40,    // router far side by S/N
    NRNID_FARSIDE  = NEURON_ID   | 0x40,    // router far side by NID
    BCAST_FARSIDE  = BROADCAST   | 0x40,    // router far side by bcast
    IMPLICIT       = 126,    /* use address table, not explicit address */
    LOCAL          = 127,    /* network interface node */
    GROUP_0        = 128,
} AddrType;

/* Group address structure.  Use for multicast destination addresses. */
typedef LON_STRUCT_BEGIN(SendGroup) {
#ifdef BITF_LITTLE_ENDIAN
    LonUbits8   size      :7;   /* Group size (0 => huge group)         */
    LonUbits8   type      :1;   /* 1 => Group                           */

    LonUbits8   rsvd0     :7;
    LonUbits8   domain    :1;   /* Domain index                         */

    LonUbits8   retry     :4;   /* Retry count                          */
    LonUbits8   rpt_timer :4;   /* Retry repeat timer                   */

    LonUbits8   tx_timer  :4;   /* Transmit timer index                 */
    LonUbits8   rcv_timer :4;   /* Receive timer index                  */

    LonByte       group;        /* Group ID                             */
#else
    LonUbits8   type      :1;   /* 1 => Group                           */
    LonUbits8   size      :7;   /* Group size (0 => huge group)         */
    
    LonUbits8   domain    :1;   /* Domain index                         */
    LonUbits8   rsvd0     :7;
    
    LonUbits8   rpt_timer :4;   /* Retry repeat timer                   */
    LonUbits8   retry     :4;   /* Retry count                          */
    
    LonUbits8   rcv_timer :4;   /* Receive timer index                  */
    LonUbits8   tx_timer  :4;   /* Transmit timer index                 */
    
    LonByte       group;        /* Group ID                             */
#endif
} LON_STRUCT_END(SendGroup);

/* Subnet/node ID address.  Use for a unicast destination address. */
typedef LON_STRUCT_BEGIN(SendSnode) {
#ifdef BITF_LITTLE_ENDIAN
    LonByte   type;             /* SUBNET_NODE                          */

    LonUbits8   node      :7;   /* Node number                          */
    LonUbits8   domain    :1;   /* Domain index                         */

    LonUbits8   retry     :4;   /* Retry count                          */
    LonUbits8   rpt_timer :4;   /* Retry repeat timer                   */

    LonUbits8   tx_timer  :4;   /* Transmit timer index                 */
    LonUbits8   rsvd1     :4;

    LonByte   subnet;           /* Subnet ID                            */
#else
    LonByte   type;             /* SUBNET_NODE                          */
    
    LonUbits8   domain    :1;   /* Domain index                         */
    LonUbits8   node      :7;   /* Node number                          */

    LonUbits8   rpt_timer :4;   /* Retry repeat timer                   */
    LonUbits8   retry     :4;   /* Retry count                          */
    
    LonUbits8   rsvd1     :4;
    LonUbits8   tx_timer  :4;   /* Transmit timer index                 */
    
    LonByte   subnet;           /* Subnet ID                            */
#endif
} LON_STRUCT_END(SendSnode);

/* 48-bit NEURON ID destination address. */
typedef LON_STRUCT_BEGIN(SendNrnid) {
#ifdef BITF_LITTLE_ENDIAN
    LonByte     type;             /* NEURON_ID                            */
    LonUbits8   rsvd0      :7;
    LonUbits8   domain     :1;  /* Domain index                         */
    LonUbits8   retry      :4;  /* Retry count                          */
    LonUbits8   rpt_timer  :4;  /* Retry repeat timer                   */
    LonUbits8   tx_timer   :4;  /* Transmit timer index                 */
    LonUbits8   rsvd2      :4;
    LonByte     subnet;           /* Subnet ID, 0 => pass all routers     */
    LonByte     nid[ NEURON_ID_LEN ]; /* NEURON ID                          */
#else
    LonByte     type;             /* NEURON_ID                            */
    LonUbits8   domain     :1;  /* Domain index                         */
    LonUbits8   rsvd0      :7;
    LonUbits8   rpt_timer  :4;  /* Retry repeat timer                   */
    LonUbits8   retry      :4;  /* Retry count                          */
    LonUbits8   rsvd2      :4;
    LonUbits8   tx_timer   :4;  /* Transmit timer index                 */
    LonByte     subnet;           /* Subnet ID, 0 => pass all routers     */
    LonByte     nid[ NEURON_ID_LEN ]; /* NEURON ID                          */
#endif
} LON_STRUCT_END(SendNrnid);

/* Broadcast destination address. */
typedef LON_STRUCT_BEGIN(SendBcast) {
#ifdef BITF_LITTLE_ENDIAN
    LonByte   type;             /* BROADCAST                            */

    LonUbits8   backlog     :6; /* Backlog                              */
    LonUbits8   rsvd0       :1;
    LonUbits8   domain      :1; /* Domain index                         */

    LonUbits8   retry       :4; /* Retry count                          */
    LonUbits8   rpt_timer   :4; /* Retry repeat timer                   */

    LonUbits8   tx_timer    :4; /* Transmit timer index                 */
    LonUbits8   rsvd2       :4;

    LonByte   subnet;           /* Subnet ID, 0 => domain-wide          */
#else
    LonByte   type;             /* BROADCAST                            */
    
    LonUbits8   domain      :1; /* Domain index                         */
    LonUbits8   rsvd0       :1;
    LonUbits8   backlog     :6; /* Backlog                              */
    
    LonUbits8   rpt_timer   :4; /* Retry repeat timer                   */
    LonUbits8   retry       :4; /* Retry count                          */
    
    LonUbits8   rsvd2       :4;
    LonUbits8   tx_timer    :4; /* Transmit timer index                 */
    
    LonByte   subnet;           /* Subnet ID, 0 => domain-wide          */
#endif
} LON_STRUCT_END(SendBcast);

/* Address format to clear an address table entry.         */
typedef LON_STRUCT_BEGIN(SendUnassigned) {
    LonByte   type;             /* UNASSIGNED */
} LON_STRUCT_END(SendUnassigned);

typedef LON_STRUCT_BEGIN(SendLocalNi) {
    LonByte   type;             /* LOCAL */
} LON_STRUCT_END(SendLocalNi);

typedef LON_STRUCT_BEGIN(SendImplicit) {
   LonByte    type;             /* IMPLICIT */
   LonByte    msg_tag;          /* address table entry number */
} LON_STRUCT_END(SendImplicit);

/* Union of all destination addresses. */

typedef union {
    SendUnassigned ua;
    SendGroup      gp;
    SendSnode      sn;
    SendBcast      bc;
    SendNrnid      id;
    SendLocalNi    lc;
    SendImplicit   im;
} SendAddrDtl;

/*
 ****************************************************************************
 * Network Address structures for receiving messages with explicit
 * addressing enabled.
 ****************************************************************************
 */

/* Received subnet/node ID destination address.  Used for unicast messages. */
typedef LON_STRUCT_BEGIN(RcvSnode) {
    LonByte       subnet;
#ifdef BITF_LITTLE_ENDIAN
    LonUbits8       node   :7;
    LonUbits8              :1;
#else
    LonUbits8              :1;
    LonUbits8       node   :7;
#endif
} LON_STRUCT_END(RcvSnode);

/* Received 48-bit NEURON ID destination address. */
typedef LON_STRUCT_BEGIN(RcvNrnid) {
    LonByte   subnet;
    LonByte   nid[NEURON_ID_LEN];
} LON_STRUCT_END(RcvNrnid);

/* Union of all received destination addresses. */

typedef union {
    LonByte    gp;                  /* Group ID for multicast destination   */
    RcvSnode   sn;                  /* Subnet/node ID for unicast           */
    RcvNrnid   id;                  /* 48-bit NEURON ID destination address */
    LonByte    subnet;              /* Subnet ID for broadcast destination  */
                                    /* 0 => domain-wide */
} RcvDestAddr;

/* Source address of received message.  Identifies */
/* network address of node sending the message.    */
typedef LON_STRUCT_BEGIN(RcvSrcAddr) {
    LonByte   subnet;
#ifdef BITF_LITTLE_ENDIAN
    LonUbits8   node    :7;
    LonUbits8           :1;
#else
    LonUbits8           :1;
    LonUbits8   node    :7;
#endif
} LON_STRUCT_END(RcvSrcAddr);

/* Literals for the 'format' field of RcvAddrDtl. */

typedef enum {
    ADDR_RCV_BCAST  = 0,
    ADDR_RCV_GROUP  = 1,
    ADDR_RCV_SNODE  = 2,
    ADDR_RCV_NRNID  = 3
} RcvDstAddrFormat;

/* Address field of incoming message. */
typedef LON_STRUCT_BEGIN(RcvAddrDtl) {
#ifdef BITF_LITTLE_ENDIAN
    LonUbits8    format      :6;/* Destination address type             */
                                /* See 'RcvDstAddrFormat'               */
    LonUbits8    flex_domain :1;/* 1 => broadcast to unconfigured node  */
    LonUbits8    domain      :1;/* Domain table index                   */
#else
    LonUbits8    domain      :1;/* Domain table index                   */
    LonUbits8    flex_domain :1;/* 1 => broadcast to unconfigured node  */
    LonUbits8    format      :6;/* Destination address type             */
                                /* See 'RcvDstAddrFormat'               */
#endif
    RcvSrcAddr  source;         /* Source address of incoming message   */
    RcvDestAddr dest;           /* Destination address of incoming msg  */
} LON_STRUCT_END(RcvAddrDtl);

/*
 ****************************************************************************
 * Network Address structures for receiving responses with explicit
 * addressing enabled.
 ****************************************************************************
 */

/* Source address of response message. */
typedef LON_STRUCT_BEGIN(RespSrcAddr) {
    LonByte   subnet;
#ifdef BITF_LITTLE_ENDIAN
    LonUbits8   node     :7;
    LonUbits8   is_snode :1;   /* 0 => Group response,                 */
                               /* 1 => snode response                  */
#else
    LonUbits8   is_snode :1;   /* 0 => Group response,                 */
                               /* 1 => snode response                  */
    LonUbits8   node     :7;
#endif
} LON_STRUCT_END(RespSrcAddr);

/* Destination of response to unicast request. */
typedef LON_STRUCT_BEGIN(RespSnode) {
    LonByte   subnet;
#ifdef BITF_LITTLE_ENDIAN
    LonUbits8   node     :7;
    LonUbits8            :1;
#else
    LonUbits8            :1;
    LonUbits8   node     :7;
#endif
} LON_STRUCT_END(RespSnode);

/* Destination of response to multicast request. */
typedef LON_STRUCT_BEGIN(RespGroup) {
#ifdef BITF_LITTLE_ENDIAN
    LonByte   subnet;
    LonUbits8   node     :7;
    LonUbits8            :1;
    LonByte   group;
    LonUbits8   member   :6;
    LonUbits8            :2;
#else
    LonByte   subnet;
    LonUbits8            :1;
    LonUbits8   node     :7;
    LonByte   group;
    LonUbits8            :2;
    LonUbits8   member   :6;
#endif
} LON_STRUCT_END(RespGroup);

/* Union of all response destination addresses. */

typedef union {
    RespSnode  sn;
    RespGroup  gp;
} RespDestAddr;

/* Address field of incoming response. */
typedef LON_STRUCT_BEGIN(RespAddrDtl) {
#ifdef BITF_LITTLE_ENDIAN
    LonUbits8                 :6;
    LonUbits8     flex_domain :1; /* 1=> Broadcast to unconfigured node   */
    LonUbits8     domain      :1; /* Domain table index                   */
#else
    LonUbits8     domain      :1; /* Domain table index                   */
    LonUbits8     flex_domain :1; /* 1=> Broadcast to unconfigured node   */
    LonUbits8                 :6;
#endif
    RespSrcAddr  source;            /* Source address of incoming response  */
    RespDestAddr dest;              /* Destination address of incoming resp */
} LON_STRUCT_END(RespAddrDtl);

/* Explicit address field if explicit addressing is enabled. */

typedef union {
    RcvAddrDtl  rcv;
    SendAddrDtl snd;
    RespAddrDtl rsp;
} ExplicitAddr;

/*
 ****************************************************************************
 * Data field structures for explicit messages and network variables.
 ****************************************************************************
 */

/*
 * MAX_NETMSG_DATA specifies the maximum size of the data portion of an
 * application buffer.  MAX_NETVAR_DATA specifies the maximum size of the
 * data portion of a network variable update.  The values specified here
 * are the absolute maximums, based on the LONTALK protocol. Actual limits
 * are based on the buffer sizes defined on the attached NEURON CHIP.
 */

#define MAX_NETMSG_DATA 228
#define MAX_NETVAR_DATA 31

/* Data field for network variables (host selection enabled). */
typedef LON_STRUCT_BEGIN(UnprocessedNV) {
#ifdef BITF_LITTLE_ENDIAN
    LonUbits8   NV_selector_hi :6;
    LonUbits8   direction      :1;     /* 1 => output NV, 0 => input NV      */
    LonUbits8   must_be_one    :1;     /* Must be set to 1 for NV            */
#else
    LonUbits8   must_be_one    :1;     /* Must be set to 1 for NV            */
    LonUbits8   direction      :1;     /* 1 => output NV, 0 => input NV      */
    LonUbits8   NV_selector_hi :6;
#endif
    LonByte     NV_selector_lo;
    LonByte     data[MAX_NETVAR_DATA]; /* Network variable data            */
} LON_STRUCT_END(UnprocessedNV);

/* Data field for network variables (network interface selection enabled).  */
typedef LON_STRUCT_BEGIN(ProcessedNV) {
    LonByte    index;                 /* Index into NV configuration table  */
    LonByte    rsvd0;
    LonByte    data[MAX_NETVAR_DATA]; /* Network variable data            */
} LON_STRUCT_END(ProcessedNV);

/* Data field for explicit messages. */
typedef LON_STRUCT_BEGIN(ExplicitMsg) {
    unsigned char       code;                  /* Message code                       */
    unsigned char       data[MAX_NETMSG_DATA]; /* Message data                     */
} LON_STRUCT_END(ExplicitMsg);

/* Union of all data fields. */

typedef union {
    UnprocessedNV unv;
    ProcessedNV   pnv;
    ExplicitMsg   exp;
} MsgData;

/*
 ****************************************************************************
 * Message buffer types.
 ****************************************************************************
 */

/* Application buffer when using explicit addressing. */
typedef LON_STRUCT_BEGIN(ExpAppBuffer) {
    NI_Hdr       ni_hdr;            /* Network interface header             */
    MsgHdr       msg_hdr;           /* Message header                       */
    ExplicitAddr addr;              /* Network address                      */
    MsgData      data;              /* Message data                         */
} LON_STRUCT_END(ExpAppBuffer);

/* Application buffer when not using explicit addressing. */
typedef LON_STRUCT_BEGIN(ImpAppBuffer) {
    NI_Hdr       ni_hdr;            /* Network interface header             */
    MsgHdr       msg_hdr;           /* Message header                       */
    MsgData      data;              /* Message data                         */
} LON_STRUCT_END(ImpAppBuffer);

/*
 ****************************************************************************
 * Network interface error codes.
 ****************************************************************************
 */

/*
 * Types: NICode
 * Return values for message sending functions.
 */
typedef enum {
    NI_OK = 0,
    NI_NO_DEVICE,
    NI_DRIVER_NOT_OPEN,
    NI_DRIVER_NOT_INIT,
    NI_DRIVER_NOT_RESET,
    NI_DRIVER_ERROR,
    NI_NO_RESPONSES,
    NI_RESET_FAILS,
    NI_TIMEOUT,
    NI_UPLINK_CMD,
    NI_INTERNAL_ERR,
    NI_FILE_OPEN_ERR,
    NI_NO_COMPLETION,
    NI_NO_COMP_DATA,
    NI_NUM_ERRS
} NICode;

typedef enum {
    MF_OK,
    MF_NO_ANSWER,
    MF_RESPONSE_ERROR,
} MsgResult;

/***************************** End ni_msg.h *****************************/


//
// The following macros are used when accessing data that is normally on the Neuron but could be remote
// in the event of a MIP with host selection.
//
#define NvCount			_nv_count()
#define AliasCount		_alias_count()

//
// Used for convenient access to selector bytes
//
typedef union {
	LonUbits16  Long;
	LonByte Byte[2];
} SelectorType;

/*
 *  Typedef: IsiCid2
 *  Structure representing the rev 2 format of the unique connection ID.  
 *  See the IsiCid typedef in IsiTypes.h for the rev1 format of the unique 
 *  connection ID. 
 *  In this rev2, the uniqueID is constructed based on the 6-byte neuronID 
 *  (it's no longer in the compressed form) and leaving the Serial Number with 1-byte.
 *  It's currently used internally during the creation of the new unique CID.
 *
 */
typedef LON_STRUCT_BEGIN(IsiCid2) 
{
    LonUniqueId UniqueId;    // host's unique ID (copy from Neuron ID)
    LonByte     SerialNumber;
} LON_STRUCT_END(IsiCid2);

/*
 *  Typedef: IsiUniqueCid
 *  Structure representing the union of the rev 1 and rev 2 format of the unique connection ID.
 *  It's currently used internally as a convenient access to the connection ID.   
 */
typedef LON_UNION_BEGIN(IsiUniqueCid) 
{        
    IsiCid rev1Cid;
    IsiCid2 rev2Cid;
} LON_UNION_END(IsiUniqueCid);


//
// MACRO for common setup of NV structure
//
#define _IsiBind(nv, Address, Selector,turnAround) \
	LON_SET_UNSIGNED_WORD(nv.AddressIndex, Address);     \
	LON_SET_ATTRIBUTE(nv,LON_NV_ECS_SELHIGH,high_byte(LON_GET_UNSIGNED_WORD(Selector)));	\
	nv.SelectorLow = low_byte(LON_GET_UNSIGNED_WORD(Selector));		\
    LON_SET_ATTRIBUTE(nv,LON_NV_ECS_TURNAROUND,turnAround);        \
    LON_SET_ATTRIBUTE(nv,LON_NV_ECS_SERVICE,LonServiceRepeated);
// End _IsiBind


// extern void _IsiBind(LonNvEcsConfig* pNv, unsigned Address, LonWord Selector, LonByte turnAround);
extern unsigned get_nv_type (unsigned index);
extern LonByte high_byte (LonUbits16 a);
extern LonByte low_byte (LonUbits16 a);
extern LonWord make_long(LonByte low_byte, LonByte high_byte);
extern void watchdog_update(void);
extern IsiApiError update_config_data(const LonConfigData *config_data1);

extern LonReadOnlyData read_only_data;
extern LonConfigData config_data;
extern LonNvEcsConfig nv_config;
extern LonAliasEcsConfig alias_config;

extern LonDomain domainTable[MAX_DOMAINS];  // retrieve using LonQueryDomainConfig(const unsigned index, LonDomain* const pDomain);
extern LonAddress addrTable;    // [NUM_ADDR_TBL_ENTRIES];    // retrieve using LonQueryAddressConfig(const unsigned index, LonAddress* const pAddress);
extern unsigned int getRandom(void);
extern const LonAddress* access_address	(int index);
extern const LonDomain* access_domain(int index);
extern const unsigned get_nv_length(const unsigned index);
extern LonByte* get_nv_value(const unsigned index);
extern IsiApiError set_node_mode(unsigned mode, unsigned state);

//
// Used to check for valid data pointer.
// Note it is sufficient to just check the high byte of the pointer since the first
// page of Neuron memory is always constant system image.
//
#define VALID_DATA_PTR(p)	(p != NULL)      


//  Incoming / Outgoing messages:
extern LonBool IsiApproveMsg(const LonByte code, const LonByte* msgIn, const unsigned dataLength);
extern LonBool IsiApproveMsgDas(const LonByte code, const LonByte* msgIn, const unsigned dataLength);

// These API are supported but not exposed.  Engine handles all incoming
// message and request approval and processing

extern LonBool IsiProcessMsgS(const LonByte code, const IsiMessage* msgIn, const unsigned dataLength);
extern LonBool IsiProcessMsgDa(const LonByte code, const IsiMessage* msgIn, const unsigned dataLength);
extern LonBool IsiProcessMsgDas(const LonByte code, const IsiMessage* msgIn, const unsigned dataLength);
extern LonBool IsiProcessMsg(IsiType Type, const LonByte code, const IsiMessage* msgIn, const unsigned dataLength);
extern LonBool IsiProcessResponse(IsiType Type, const LonByte code, const LonByte* const pData, const unsigned dataLength);

extern void IsiMsgDeliver(const IsiMessage* pMsg, unsigned Length, LonSendAddress* pDestination);
extern void IsiMsgSend(const IsiMessage* pMsg, unsigned IsiMessageLength, LonServiceType Service, LonSendAddress* pDestination);
extern void IsiStartDa(IsiFlags Flags);
extern void IsiStartDas(IsiFlags Flags);
extern void IsiStartS(IsiFlags Flags);
extern void IsiTickS(void);
extern void IsiTickDa(void);
extern void IsiTickDas(void);
extern const LonAliasEcsConfig* IsiGetAlias(unsigned Index);        // forwarder to access_alias
extern const LonNvEcsConfig* IsiGetNv(unsigned Index);              // forwarder to nv_access
extern IsiApiError IsiSetAlias(LonAliasEcsConfig* pAlias, unsigned Index);         // forwarder to update_alias
extern void IsiSetNv(LonNvEcsConfig* pNv, unsigned Index);          // forwarder to update_nv
extern void IsiSetPrimaryDid(const LonByte* pDid, unsigned len);
extern void IsiSetRepeatCount(unsigned Count);
extern unsigned IsiGetNextAssembly(const IsiCsmoData* pCsmoData, LonBool Auto, unsigned Assembly);  // forwarder
    extern unsigned isiGetNextAssembly(const IsiCsmoData* pCsmoData, LonBool Auto, unsigned Assembly);  // forwardee
extern unsigned IsiGetNextNvIndex(unsigned Assembly, unsigned Offset, unsigned PreviousIndex);  // forwarder
extern unsigned isiGetNextNvIndex(unsigned Assembly, unsigned Offset, unsigned PreviousIndex);  // forwardee
extern unsigned isiGetNvIndex(unsigned Assembly, unsigned Offset, unsigned PreviousIndex);  // forwardee
extern unsigned isiGetWidth(unsigned Assembly);                     // forwardee
extern void isiCreateCsmo(unsigned Assembly, IsiCsmoData* const pCsmoData); // forwardee
extern unsigned isiGetAssembly(const IsiCsmoData* pCsmoData, LonBool Auto, unsigned Assembly); // forwardee
extern const LonByte* IsiGetNvValue(unsigned NvIndex, unsigned* pLength);
extern unsigned IsiGetConnectionTableSize(void);
extern const IsiConnection* IsiGetConnection(unsigned Index);
extern void IsiSetConnection(const IsiConnection* pConnection, unsigned Index);
extern void _IsiInitConnectionTable();
extern const LonByte* IsiGetPrimaryDid(LonByte* pLength);      // OVERRIDING MIGHT BREAK INTEROPERABILITY!
extern unsigned IsiGetRepeatCount(void);
IsiApiError IsiSetDomain(const LonDomain* pDomain, unsigned index);   // forwarder to update_domain
extern void IsiAcquireDomain(LonBool SharedServicePin);
extern void IsiStartDeviceAcquisition(void);
extern void IsiCancelAcquisition(void);     // on ISI-DAS, use IsiCancelAcquisitionDas
extern void IsiCancelAcquisitionDas(void);  // only for ISI-DAS. On ISI-S or ISI-DA, use IsiCancelAcquisition
// The following internal API has been added with ISI 3.03, and is used from the pre-defined IsiMsgDeliver 
// function defined below. The isiPrepareSicb function prepares the pending outgoing message for sending. 
// This function is called from the IsiMsgDeliver function rather than from the ISI core in order to supply
// this improvements to all ISI implementations. 
// Do not call this function other than from the IsiMsgDeliver() function implemented below.
extern void isiPrepareSicb(void);

unsigned isiGetPrimaryGroup(unsigned Assembly); 
LonBool isiQueryHeartbeat(unsigned nv);
void isiUpdateDiagnostics(IsiDiagnostic Event, LonByte Parameter);
void isiUpdateUserInterface(IsiEvent Event, LonByte Parameter);
extern LonBool isiCreatePeriodicMsg(void);

// Use IsiGetFreeAliasCount() to obtain the number of unused aliases. Function is unavailable
// with IsiCompactManual and IsiCompactAuto.lib. 
extern unsigned IsiGetFreeAliasCount(void);  // N/A for Pilon
extern LonBool IsiIsConfiguredOnline(void);

// The following ISI function has been added with ISI 3.01.01, and must be called after the device resets,
// and prior to calling any other ISI function.
// extern void IsiPreStart(void); // N/A for Pilon 
void _IsiSetConnectionTableSize(unsigned sz);

extern LonSendAddress* _IsiGetMsgOutAddrPtr(void);
extern IsiMessage* _IsiGetMsgInDataPtr(void);
extern unsigned _IsiGetMsgInDataLen(void);
extern unsigned _IsiGetMsgInCode(void);
extern void* _IsiGetRespInDataPtr(void);

extern void _IsiReceiveDrumS(const IsiMessage* msgIn);
extern void _IsiReceiveDrumDas(const IsiMessage* msgIn);
extern void _IsiReceiveTimg(unsigned DeviceEstimate, unsigned ChannelType);
extern void _IsiReceiveCsmi(void);

extern void _IsiSendDrum(void);
extern void _IsiSendTimg(unsigned DeviceCount);

extern void _IsiInitDeviceCountEstimation(void);	// DAS only
extern unsigned _IsiGetCurrentDeviceEst(void);      // DAS only
extern void _IsiDecrementLiveCounters(void);        // DAS only

extern void _IsiUpdateUiNormal(void);				// Same as IsiUpdateUserInterface(isiNormal, ISI_NO_ASSEMBLY)
extern void _IsiUpdateUi(IsiEvent Event);			// Same as IsiUpdateUserInterface(Event, ISI_NO_ASSEMBLY)
extern void _IsiUpdateUiAndState(IsiState State, IsiEvent Event, int Parameter);
extern void _IsiUpdateUiAndStateTimeout(unsigned long Timeout, IsiState State, IsiEvent Event, int Parameter);
extern void _IsiUpdateUiAndStateEnroll(IsiState State, IsiEvent Event, int Parameter);

extern LonBool _IsiCreateCid(void);
extern LonWord _IsiGetSelectors(unsigned Width);
extern void _IsiSendCsmi(const IsiConnection* pConnection);
extern void _IsiSendCsmr(unsigned Index, const IsiConnection* pConnection);
extern LonBool _IsiSendNvHb(void);

extern void _IsiResolveSelectorConflict(unsigned Conflict);
extern void _IsiReplaceSelectors(unsigned Assembly, LonWord Old, LonWord New, unsigned Count);
extern LonBool _IsiInSelectorRange(LonWord RangeStart, unsigned Count, LonWord Selector);
extern void _IsiCreateCsmi(const IsiConnection* pConnection, IsiCsmi* pCsmi);
extern unsigned _IsiIsGroupAcceptable(unsigned Group, LonBool Join);
extern LonBool _IsiApproveCsmo(const IsiCsmo* pCsmo, LonBool Auto, unsigned host, unsigned member);
extern void _IsiReceiveCsmo(LonBool Auto, LonBool ShortForm, const IsiCsmo* pCsmo);
extern void _IsiReceivePtrCsmo(IsiCsmo* pCsmo, unsigned localHostAssembly, LonBool Auto, LonBool ShortForm, LonBool IsLocalTurnaroundCsmo);
extern void _IsiReceivePtrCsmi(const IsiCsmi * pCsmi);
extern LonBool _IsiBecomeHost(unsigned Assembly, LonBool Auto, const IsiCsmoData* pData);
extern unsigned _IsiFindLocalNvOfType(const IsiCsmoData* pCsmoData, unsigned NextAssembly);
extern void _IsiResendCsmo(void);
extern void _IsiSendCsmx(void);
extern void _IsiSendPCsmx(const IsiConnection* pConnection);
extern void _IsiReceiveCsmx(const IsiCsmx* pCsmx);
extern void _IsiImplementEnrollment(LonBool Extend, LonByte Assembly);
extern void _IsiAcceptEnrollment(LonBool Extend, unsigned Assembly);
extern void _IsiSendCsme(void);
extern void _IsiReceiveCsme(const IsiCsme* pCsme);
extern void _IsiSendCsmX(const IsiConnection* pConnection, IsiMessageCode Code, unsigned Repeats);
extern void _IsiMakeEnrollment(LonBool bExtend, unsigned Assembly);
extern void _IsiReceiveCsmc(const IsiCsmc* pCsmc);
extern void _IsiReceiveCsmd(const IsiCsmd* pCsmd);
extern void _IsiRemoveConnection(IsiState RequiredState, unsigned Assembly, LonBool Global);
extern void _IsiRemovePtrConnection(const IsiConnection* pConnection, unsigned assembly);
extern void _IsiSweepAddressTable(void);
extern void _IsiSendCsmex(LonBool RequireHost, IsiMessageCode Code, unsigned Repeats);
extern LonBool _IsiNextConditionalConnection(unsigned* pIndex, IsiConnection* pConn, unsigned Assembly, IsiConnectionState State);
extern LonBool _IsiNextConnection(unsigned index, IsiConnection* pConn);
extern void _IsiClearConnection(IsiConnection* pConn, unsigned Index);

extern unsigned long _IsiMaskSelector(unsigned long Selector);
extern LonWord _IsiIncrementSelector(LonWord Selector);
extern LonWord _IsiAddSelector(LonWord Selector, unsigned Increment);

extern unsigned _nv_count(void);
extern unsigned int _alias_count(void);
extern unsigned int _address_table_count(void);
extern IsiApiError service_pin_msg_send (void);
extern IsiApiError update_address(const LonAddress* address, int index);
extern IsiApiError update_domain_address(const LonDomain* pDomain, int index, int nonCloneValue, LonBool bUpdateID);
extern void node_reset();
extern IsiApiError retrieve_status(LonStatus* status);
extern void update_nv(const LonNvEcsConfig * nv_entry, unsigned index);

//  interal functions used by device and domain acquisistion:
extern void _IsiAcquireDomain(void);    //  see acqdom.c
extern void _IsiSendDidrm(IsiMessageCode code); // see snddidrm.c
extern IsiDidrq    lastDidrq;           // see msgdas.c
extern unsigned long   _isiTcsmr;       //  see Tcsmr.c
extern void _IsiTcsmr(void);            // see Tcsmr.c
extern void _IsiSendIsi(LonServiceType serviceType, const void* pOut, unsigned len, LonSendAddress* pDestination);
extern void _IsiSend(unsigned LonTalkCode, LonServiceType serviceType, const void* pOut, unsigned len, const LonSendAddress* pDestination);
// _IsiNidDestination requires ShiftedDomainIndex, i.e. 0 for the primary domain, but 0x80 (1 << 7) for the secondary. We can pass this as a constant and
// safe some code by not having to shift in the _IsiNidDestination routine.
extern void _IsiNidDestination(LonSendAddress* pDestination, unsigned ShiftedDomainIndex, unsigned repeats, const LonByte* pNid);
extern LonBool _IsiRespArrives(void);       //  see resparr.ns
extern LonBool _IsiIsHeartbeatCandidate(unsigned nvIndex);
extern LonBool _IsiPropagateNvHb(const LonNvEcsConfig* pNv, const LonByte* pNvData, unsigned length);
extern IsiType _IsiGetCurrentType();
extern void _IsiSetCurrentType(IsiType type);
extern void _IsiSetSubnet(unsigned subnetId);
extern void _IsiSetNode(unsigned nodeId);


extern const LonBool isiFilterMsgArrived(const LonReceiveAddress* const pAddress,
                          const LonCorrelator correlator,
                          const LonBool priority,
                          const LonServiceType serviceType,
                          const LonBool authenticated,
                          const LonByte code,
                          const LonByte* const pData, const unsigned dataLength);

extern const LonBool isiFilterResponseArrived(const LonResponseAddress* const pAddress, const unsigned tag, 
                        const LonByte code, const LonByte* const pData, const unsigned dataLength);

extern LtPersistenceLossReason readPersistence(LonNvdSegmentType type, LonByte** pImage, int* imageLength, int* nVersion);
extern IsiApiError initializeData(IsiBootType bootType);
extern void _IsiAPIDebug(const char * fmt, ...);
extern void _IsiAPIDump(const char* prefix, void* data, unsigned bytes, const char* postfix);
extern void _IsiSetAppSignature(unsigned signature);
extern LtPersistenceLossReason restorePersistentData(LonNvdSegmentType type);
extern void savePersistentData(LonNvdSegmentType type);
extern void DumpConnectionTable();
extern void DumpIsiPersistData();
extern void _IsiSetDeviceCount(LonByte number);
extern LonByte _IsiGetDasDeviceCountEst(void);
extern void _IsiSetDasDeviceCountEst(LonByte number);

#define ISI_CALLBACK_EXEC(str)                  _IsiAPIDebug("%s = executing registered callback vector\n", str)
#define ISI_CALLBACK_EXCEPTION(str)             _IsiAPIDebug("%s = ** an exception occurred when executing the callback vector **\n", str)
#define ISI_CALLBACK_NOT_REGISTERED(str)        _IsiAPIDebug("%s = callback vector not registered\n", str)
#define ISI_CALLBACK_NOT_REGISTERED_DEF(str)    _IsiAPIDebug("%s = callback vector not registered, executing default\n", str)
#define ISI_CALLBACK_REGISTER(str, sts)         _IsiAPIDebug("%sRegistrar = %d\n", str, sts)

// Controlled enrollment
extern LonBool IsiControlledEnrollmentMsg(const LonByte code, const IsiMessage* msgIn, const unsigned dataLength, const LonCorrelator correlator);
extern LonBool IsiProcessCtrlEnrollmentRequest(const LonByte code, const IsiMessage* msgIn, const unsigned dataLength, const LonCorrelator correlator);
extern void _IsiRequestConnectionTable(const LonUniqueId* pUniqueId, LonByte index, unsigned hostAssembly, unsigned memberAssembly); 

#ifdef  __cplusplus
}
#endif
#endif	//	!defined __ISIINT_H__
