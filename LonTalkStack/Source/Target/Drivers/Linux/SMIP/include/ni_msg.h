/* NI_MSG.H -- LON host application message handler.
 *
 * Copyright © 1993-2022 Dialog Semiconductor
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
 * Date last modified: 11/17/94
 *
 * Message definitions for the LON network interface protocol.
 */
#ifdef SUPPORT_PRAGMA_ONCE
#pragma once
#endif

#ifndef __NI_MSG_H
#define __NI_MSG_H


#include "Ha_Comn.h"

/*
 ****************************************************************************
 * Application buffer structures for sending and receiving messages to and
 * from a network interface.  The 'ExpAppBuffer' and 'ImpAppBuffer'
 * structures define the application buffer structures with and without
 * explicit addressing.  These structures have up to four parts:
 *
 *   Network Interface Command (NI_Hdr)                        (2 bytes)
 *   Message Header (MsgHdr)                                   (3 bytes)
 *   Network Address (ExplicitAddr)                            (11 bytes)
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
 *
 *   RespAddrDtl is used for incoming responses or NV updates solicited
 *   by a poll.
 *
 * Data (MsgData: union of UnprocessedNV, ProcessedNV, and ExplicitMsg)
 *
 *   This field is present if the message is a data transfer or completion
 *   event.
 *
 *   If the message is a completion event, then the first two bytes of the
 *   data are included.  This provides the NV index, the NV selector, or the
 *   message code as appropriate.
 *
 *   UnprocessedNV is used if the message is a network variable update, and
 *   host selection is enabled. It consists of a two-byte header followed by
 *   the NV data.
 *
 *   ProcessedNV is used if the message is a network variable update, and
 *   network interface selection is enabled. It consists of a two-byte header
 *   followed by the NV data.
 *
 *   ExplicitMsg is used if the message is an explicit message.  It consists
 *   of a one-byte code field followed by the message data.
 *
 * Note - the fields defined here are for a little-endian (Intel-style)
 * host processor, such as the 80x86 processors used in PC compatibles.
 * Bit fields are allocated right-to-left within a byte.
 * For a big-endian (Motorola-style) host, bit fields are typically
 * allocated left-to-right.  For this type of processor, reverse
 * the bit fields within each byte.  Compare the NEURON C include files
 * ADDRDEFS.H and MSG_ADDR.H, which are defined for the big-endian NEURON
 * CHIP.
 ****************************************************************************
 */

/*
 ****************************************************************************
 * Network Interface Command data structure.  This is the application-layer
 * header used for all messages to and from a LONWORKS network interface.
 ****************************************************************************
 */

// Literals for the 'cmd.q.queue' nibble of NI_Hdr.

typedef enum {
    niTQ          =  2,             // Transaction queue
    niTQ_P        =  3,             // Priority transaction queue
    niNTQ         =  4,             // Non-transaction queue
    niNTQ_P       =  5,             // Priority non-transaction queue
    niRESPONSE    =  6,             // Response msg & completion event queue
    niINCOMING    =  8              // Received message queue
} NI_Queue;

// Literals for the 'cmd.q.q_cmd' nibble of NI_Hdr.

typedef enum {
    niCOMM           = 1,           // Data transfer to/from network
    niNETMGMT        = 2,            // Local network management/diagnostics
    niRESET          = 5,
    niFLUSH_CANCEL   = 6,
    niFLUSH_COMPLETE = 6,
    niONLINE         = 7,
    niOFFLINE        = 8,
    niFLUSH          = 9,
    niFLUSH_IGN      = 10,
	niMODE			 = 13,
	niESCAPE		 = 14

} NI_QueueCmd;

typedef enum {
	niSSTATUS		 = 0,
	niSERVICE		 = 6,
	niTXID			 = 8,
} NI_EscapeCmd;

typedef enum {
	niMODE_L5		 = 0,
	niMODE_L2		 = 1,
} NI_ModeCmd;

typedef enum {
	niINITIATE		 = 1,
	niCHALLENGE		 = 2,
	niREPLY		 = 3,	
} NI_ResetCmd;


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
#pragma pack(1)		//_DCX_PACKING

typedef union {
    struct {
		BITS2 (queue, 4,        // Network interface message queue
                                // Use value of type 'NI_Queue'
			   q_cmd, 4)        // Network interface command with queue
                                // Use value of type 'NI_QueueCmd'
        Byte length;            // Length of the buffer to follow
    } q;                        // Queue option
    struct {
        Byte     cmd;           // Network interface command w/o queue
                                // Use value of type 'NI_NoQueueCmd'
        Byte     length;        // Length of the buffer to follow
    } noq;                      // No queue option
} NI_Hdr;

/*
 ****************************************************************************
 * Message Header structure for sending and receiving explicit
 * messages and network variables which are not processed by the
 * network interface (host selection enabled).
 ****************************************************************************
 */

// Literals for 'st' fields of ExpMsgHdr and NetVarHdr.

typedef enum {
   ACKD            = 0,
   UNACKD_RPT      = 1,
   UNACKD          = 2,
   REQUEST         = 3
} ServiceType;

// Literals for 'cmpl_code' fields of ExpMsgHdr and NetVarHdr.

typedef enum {
    MSG_NOT_COMPL  = 0,             // Not a completion event
    MSG_SUCCEEDS   = 1,             // Successful completion event
    MSG_FAILS      = 2              // Failed completion event
} ComplType;

// Explicit message and Unprocessed NV Application Buffer.

typedef struct {
	BITS4 (tag,			4,      // Message tag for implicit addressing
                                // Magic cookie for explicit addressing
		   auth,		1,      // 1 => Authenticated
		   st,			2,      // Service Type - see 'ServiceType'
		   msg_type,	1)      // 0 => explicit message
                                //      or unprocessed NV
//--------------------------------------------------------------------------
	BITS7 (response,    1,      // 1 => Response, 0 => Other
		   pool,        1,      // 0 => Outgoing
		   alt_path,    1,      // 1 => Use path specified in 'path'
                                // 0 => Use default path
           addr_mode,   1,      // 1 => Explicit addressing,
                                // 0 => Implicit
                                // Outgoing buffers only
           cmpl_code,   2,      // Completion Code - see 'ComplType'
           path,        1,      // 1 => Use alternate path,
                                // 0 => Use primary path
                                //      (if 'alt_path' is set)
           priority,    1)      // 1 => Priority message
//--------------------------------------------------------------------------
    Byte   length;              // Length of msg or NV to follow
                                // not including any explicit address
                                // field, includes code byte or
                                // selector bytes
} ExpMsgHdr;

/*
 ****************************************************************************
 * Message Header structure for sending and receiving network variables
 * that are processed by the network interface (network interface
 * selection enabled).
 ****************************************************************************
 */

typedef struct {
	BITS4 (tag,			4,      // Magic cookie for correlating
                                // responses and completion events
		   rsvd0,		2,
		   poll,		1,      // 1 => Poll, 0 => Other
           msg_type,    1)      // 1 => Processed network variable
//--------------------------------------------------------------------------
    BITS7 (response,    1,      // 1 => Poll response, 0 => Other
		   pool,        1,      // 0 => Outgoing
           trnarnd,     1,      // 1 => Turnaround Poll, 0 => Other
           addr_mode,   1,      // 1 => Explicit addressing,
                                // 0 => Implicit addressing
           cmpl_code,   2,      // Completion Code - see above
           path,        1,      // 1 => Used alternate path
                                // 0 => Used primary path
                                //      (incoming only)
           priority,    1)      // 1 => Priority msg (incoming only)
//--------------------------------------------------------------------------
    Byte   length;              // Length of network variable to follow
                                // not including any explicit address
                                // not including index and rsvd0 byte
} NetVarHdr;

// Union of all message headers.

typedef union {
    ExpMsgHdr  exp;
    NetVarHdr  pnv;
} MsgHdr;

/*
 ****************************************************************************
 * Network Address structures for sending messages with explicit addressing
 * enabled.
 ****************************************************************************
 */

// Literals for 'type' field of destination addresses for outgoing messages.

typedef enum {
    UNASSIGNED     = 0,
    SUBNET_NODE    = 1,
    NEURON_ID      = 2,
    BROADCAST      = 3,
    LOCAL_INTFC    = 127,    // network interface node
} AddrType;

//
// Masks for the type field
//
#define TYPE_KEY_OVERRIDE	0x40
#define TYPE_BCGROUP		0x20	// Broadcast group
#define TYPE_LONGTIME		0x10
#define TYPE_SYNCZERO		0x08
#define TYPE_ATTENUATE		0x04

// Group address structure.  Use for multicast destination addresses.

typedef struct {
	BITS2 (size,		7,      // Group size (0 => huge group)
		   type,		1)      // 1 => Group
	BITS2 (rsvd0,		7,
		   domain,		1)      // Domain index
	BITS2 (retry,		4,      // Retry count
           rpt_timer,   4)      // Retry repeat timer
	BITS2 (tx_timer,    4,      // Transmit timer index
		   rsvd1,		4) 
    Byte   group;               // Group ID
} SendGroup;

// Subnet/node ID address.  Use for a unicast destination address.

typedef struct {
	BITS7  (type,		2,		// SUBNET_NODE
			atten,      1,		// Attenuate signal
			synczero,   1,		// Zero sync the signal
			longtime,   1,		// Use long timers
	        rsvd,		1,		
			altkey,		1,		// Alternate auth key
			mbz,		1)		// Must be zero
	BITS2  (node,		7,      // Node number
			domain,		1)      // Domain index
	BITS2  (retry,		4,		// Retry count
			rpt_timer,	4) 		// Retry repeat timer
	BITS2  (tx_timer,	4,		// Transmit timer index
			rsvd1,		4) 
    Byte   subnet;              // Subnet ID
} SendSnode;

// 48-bit NEURON ID destination address.

#define NEURON_ID_LEN 6

typedef struct {
	BITS7  (type,		2,		// NEURON_ID
			atten,      1,		// Attenuate signal
			synczero,   1,		// Zero sync the signal
			longtime,   1,		// Use long timers
	        rsvd,		1,		
			altkey,		1,		// Alternate auth key
			mbz,		1)		// Must be zero
	BITS2  (rsvd0,		7,      // Node number
			domain,		1)      // Domain index
	BITS2  (retry,		4,		// Retry count
			rpt_timer,	4) 		// Retry repeat timer
	BITS2  (tx_timer,	4,		// Transmit timer index
			rsvd2,		4)
    Byte   subnet;                  // Subnet ID, 0 => pass all routers
    Byte   nid[ NEURON_ID_LEN ];    // NEURON ID
} SendNrnid;

// Broadcast destination address.

typedef struct {
    Byte   type;                // BROADCAST

	BITS3  (backlog,    6,		// Backlog
		    rsvd0,		1,      // Node number
			domain,		1)      // Domain index
	BITS2  (retry,		4,		// Retry count
			rpt_timer,	4) 		// Retry repeat timer
	BITS2  (tx_timer,	4,		// Transmit timer index
            all_agents, 4)		// Note that this is not actually an SICB field.  It is just used to convey
								// this broadcast option to the lower layers.  The field is not used by 
								// the Neuron.
    Byte   subnet;              // Subnet ID, 0 => domain-wide
} SendBcast;

// Address format to clear an address table entry.
// Sets the first 2 bytes of the address table entry to 0.

typedef struct {
    Byte   type;                // UNASSIGNED
} SendUnassigned;

typedef struct {
    Byte   type;                // LOCAL
} SendLocal;

// Union of all destination addresses.

typedef union {
    SendUnassigned ua;
    SendGroup      gp;
    SendSnode      sn;
    SendBcast      bc;
    SendNrnid      id;
    SendLocal      lc;
} SendAddrDtl;

/*
 ****************************************************************************
 * Network Address structures for receiving messages with explicit
 * addressing enabled.
 ****************************************************************************
 */

// Received subnet/node ID destination address.  Used for unicast messages.

typedef struct {
    Byte       subnet;
	BITS2     (node,		7,
			   rsvd,		1)
} RcvSnode;

// Received 48-bit NEURON ID destination address.

typedef struct {
    Byte   subnet;
    Byte   nid[ NEURON_ID_LEN ];
} RcvNrnid;

// Union of all received destination addresses.

typedef union {
    Byte       gp;                  // Group ID for multicast destination
    RcvSnode   sn;                  // Subnet/node ID for unicast
    RcvNrnid   id;                  // 48-bit NEURON ID destination address
    Byte       subnet;              // Subnet ID for broadcast destination
                                    // 0 => domain-wide
} RcvDestAddr;

// Source address of received message.  Identifies
// network address of node sending the message.

typedef struct {
    Byte   subnet;
	BITS2 (node,	7,
		   rsvd,   1)
} RcvSrcAddr;

// Literals for the 'format' field of RcvAddrDtl.

typedef enum {
    ADDR_RCV_BCAST  = 0,
    ADDR_RCV_GROUP  = 1,
    ADDR_RCV_SNODE  = 2,
    ADDR_RCV_NRNID  = 3
} RcvDstAddrFormat;

// Address field of incoming message.
typedef struct {
	BITS3	   (format,			6,	// Destination address type
									// See 'RcvDstAddrFormat'
				flex_domain,	1,	// 1 => broadcast to unconfigured node
				domain,			1)  // Domain table index
    RcvSrcAddr  source;             // Source address of incoming message
    RcvDestAddr dest;               // Destination address of incoming msg
} RcvAddrDtl;

/*
 ****************************************************************************
 * Network Address structures for receiving responses with explicit
 * addressing enabled.
 ****************************************************************************
 */

// Source address of response message.

typedef struct {
    Byte   subnet;
	BITS2 (node,		7,
		   is_snode,	1) 	   // 0 => Group response,
                               // 1 => S/N response
} RespSrcAddr;

// Destination of response to unicast request.

typedef struct {
    Byte   subnet;
	BITS2 (node,		7,
		   rsvd,		1)
} RespSnode;

// Destination of response to multicast request.

typedef struct {
    Byte   subnet;
	BITS2 (node,		7,
		   rsvd0,		1)
    Byte   group;
	BITS2 (member,		6,
		   rsvd1,		2)
} RespGroup;

// Union of all response destination addresses.

typedef union {
    RespSnode  sn;
    RespGroup  gp;
} RespDestAddr;

// Address field of incoming response.

typedef struct {
	BITS3 (rsvd,			6,
		   flex_domain,		1,		// 1 => response from node out of domain
		   domain,			1)		// Domain table index
    RespSrcAddr  source;            // Source address of incoming response
    RespDestAddr dest;              // Destination address of incoming resp
} RespAddrDtl;

// Explicit address field if explicit addressing is enabled.

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

// Data field for network variables (host selection enabled).

typedef struct {
	BITS3 (NV_selector_hi,	6,
		   direction,		1,      // 1 => output NV, 0 => input NV
		   must_be_one,		1)		// Must be set to 1 for NV
    Byte   NV_selector_lo;
    Byte   data[ MAX_NETVAR_DATA ]; // Network variable data
} UnprocessedNV;

// Data field for network variables (network interface selection enabled).

typedef struct {
    Byte       index;                   // Index into NV configuration table
    Byte       rsvd0;
    Byte       data[ MAX_NETVAR_DATA ]; // Network variable data
} ProcessedNV;

// Data field for explicit messages.

typedef struct {
    Byte       code;                    // Message code
    Byte       data[ MAX_NETMSG_DATA ]; // Message data
} ExplicitMsg;

// Union of all data fields.

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

// Application buffer when net intfc uses explicit address format.

typedef struct {
    NI_Hdr       ni_hdr;            // Network interface header
    MsgHdr       msg_hdr;           // Message header
    ExplicitAddr addr;              // Network address
    MsgData      data;              // Message data
} ExpAppBuffer;

// Application buffer when net intfc does not use explicit address format

typedef struct {
    NI_Hdr       ni_hdr;            // Network interface header
    MsgHdr       msg_hdr;           // Message header
    MsgData      data;              // Message data
} ImpAppBuffer;

/*
 ****************************************************************************
 * Network interface error codes.
 ****************************************************************************
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

   // Add new error codes above here:
    NI_NUM_ERRS
} NICode;

#pragma pack()

#endif		// __NI_MSG_H
