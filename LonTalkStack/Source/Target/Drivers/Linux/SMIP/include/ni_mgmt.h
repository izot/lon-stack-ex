//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Target/Drivers/Linux/SMIP/include/ni_mgmt.h#1 $
//

#ifndef _NI_MGMT_H
#define _NI_MGMT_H

/****************************************************************************
 *  Filename:     ni_mgmt.h
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
 *  Description:  This include file contains LonTalk network management
 *                codes and messages.  These definitions are a subset
 *                of the LonTalk network management structures.  The
 *                types have been changed where necessary because of the
 *                difference in representation between the Neuron and
 *                MS-DOS.  If you aren't using DOS you may need to modify
 *                these structures.
 *
 ***************************************************************************/

#include "Max.h"
#include <stddef.h>
#include "nsplatform.h"
#include "lon.h"

#pragma pack(1)		//_NSS_PACKING

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

/****** LOCALIZED DEFINITIONS ********/
#define DOMAIN_ID_LEN 6
#define AUTH_KEY_LEN 6

#define LON_APP_OFFLINE				0x3f

#define NULL_IDX  (-1)                /* unused address table index */

#define OFFSET_SHORT_READ_ONLY_DATA_STRUCT \
    ((Byte)(offsetof (read_only_data_reference, nv_count_rwp)))


/* message codes for network management and diagnostic classes of messages. */
typedef enum _NM_message_code
{
    ND_query_status                 = 0x51,
    ND_proxy                        = 0x52,
    ND_clear_status                 = 0x53,
    ND_query_xcvr                   = 0x54,
	ND_signal_strength				= 0x57,
	ND_phase						= 0x58,

	NM_expanded						= 0x60,
    NM_query_id                     = 0x61,
    NM_respond_to_query             = 0x62,
    NM_update_domain                = 0x63,
    NM_leave_domain                 = 0x64,
    NM_update_key                   = 0x65,
    NM_update_address               = 0x66,
    NM_query_addr                   = 0x67,
    NM_query_nv_cnfg                = 0x68,
    NM_update_group_addr            = 0x69,
    NM_query_domain                 = 0x6A,
    NM_update_nv_cnfg               = 0x6B,
    NM_set_node_mode                = 0x6C,
    NM_read_memory                  = 0x6D,
    NM_write_memory                 = 0x6E,
    NM_checksum_recalc              = 0x6F,
    NM_wink                         = 0x70,
	NM_ecs							= NM_wink,
    NM_install                      = NM_wink,
    NM_app_cmd                      = NM_wink,
    NM_memory_refresh               = 0x71,
    NM_query_snvt                   = 0x72,
    NM_nv_fetch                     = 0x73,
    NM_device_esc                   = 0x7D,
    NM_router_esc                   = 0x7E,
    NM_service_pin                  = 0x7F,
} _NM_message_code;
typedef NetEnum(_NM_message_code) NM_message_code;

typedef enum
{
	NM_ecs_init						= 0x20
} _NM_ecs_code;
typedef NetEnum(_NM_ecs_code) NM_ecs_code;

typedef enum
{
	NM_ecs_address					= 0x03,
	NM_ecs_nv_config				= 0x05,
} _NM_ecs_resource;
typedef NetEnum(_NM_ecs_resource) NM_ecs_resource;

typedef enum _NM_message_code_expanded
{
	NMX_query_version				= 0x01,
	NMX_update_domain				= 0x07,
	NMX_query_domain				= 0x08,
	NMX_query_key					= 0x09,
	NMX_update_key					= 0x0A,
	NMX_update_prt					= 0x0B,
	NMX_query_prt					= 0x0C,
	NMX_update_prit					= 0x0D,
	NMX_query_prit					= 0x0E,
	NMX_init_config					= 0x0F,
} _NM_message_code_expanded;
typedef NetEnum(_NM_message_code_expanded) NM_message_code_expanded;

#define NM_REQUEST(code)	( (code)>=0x50 && (code)<=0x7f )
#define NM_EXPANDED(code)	( (code)==0x50 || (code)==0x60 )
#define NM_SUCCESS(code)	( ((code)&0x1f) | 0x20 )
// For expanded commands, we consider the response valid if the response matches but the sub response does not
// exist because older devices that don't support expanded commands will nack this way.
#define NM_VALID_RESPONSE(code, code1, resp, resp1, len) \
							( (((code)&0x1f) == ((resp)&0x1f)) && \
			   			      (!NM_EXPANDED( code ) || (len)==1 || (code1)==(resp1)) )
#define SET_NMX_CODE(p, sc) { (p)->code = NM_expanded; (p)->subcode = (sc); }

/* success and failure response codes for network management and diagnostic classes
   of messages.  The values overlap between the two classes; but their uses are
   context-dependent */
typedef enum _NM_response_code
{
    /* success codes for network diagnostic commands */
    ND_query_status_success         = 0x31,
    ND_proxy_success                = 0x32,
    ND_clear_status_success         = 0x33,
    ND_query_xcvr_success           = 0x34,

    /* success codes for network management commands */
    NM_expanded_success             = 0x20,
    NM_query_id_success             = 0x21,
    NM_respond_to_query_success     = 0x22,
    NM_update_domain_success        = 0x23,
    NM_leave_domain_success         = 0x24,
    NM_update_key_success           = 0x25,
    NM_update_addr_success          = 0x26,
    NM_query_addr_success           = 0x27,
    NM_query_nv_cnfg_success        = 0x28,
    NM_update_group_addr_success    = 0x29,
    NM_query_domain_success         = 0x2A,
    NM_update_nv_cnfg_success       = 0x2B,
    NM_set_node_mode_success        = 0x2C,
    NM_read_memory_success          = 0x2D,
    NM_write_memory_success         = 0x2E,
    NM_checksum_recalc_success      = 0x2F,
    NM_wink_success                 = 0x30,
    NM_install_success              = NM_wink_success,
    NM_app_cmd_success              = NM_wink_success,
    NM_memory_refresh_success       = 0x31,
    NM_query_snvt_success           = 0x32,
    NM_nv_fetch_success             = 0x33,
    NM_device_esc_success           = 0x3D,
    NM_router_esc_success           = 0x3E,
    NM_service_pin_success          = 0x3F, /* doesn't really exist */

    /* failure codes for network diagnostic commands */
    ND_query_status_fail            = 0x11,
    ND_proxy_fail                   = 0x12,
    ND_clear_status_fail            = 0x13,
    ND_query_xcvr_fail              = 0x14,

    /* failure codes for network management commands */
	NM_expanded_fail				= 0x00,
    NM_query_id_fail                = 0x01,
    NM_respond_to_query_fail        = 0x02,
    NM_update_domain_fail           = 0x03,
    NM_leave_domain_fail            = 0x04,
    NM_update_key_fail              = 0x05,
    NM_update_addr_fail             = 0x06,
    NM_query_add_failr              = 0x07,
    NM_query_nv_cnfg_fail           = 0x08,
    NM_update_group_addr_fail       = 0x09,
    NM_query_domain_fail            = 0x0A,
    NM_update_nv_cnfg_fail          = 0x0B,
    NM_set_node_mode_fail           = 0x0C,
    NM_read_memory_fail             = 0x0D,
    NM_write_memory_fail            = 0x0E,
    NM_checksum_recalc_fail         = 0x0F,
    NM_wink_fail                    = 0x10,
    NM_install_fail                 = NM_wink_fail,
    NM_app_fail                     = NM_wink_fail,
    NM_memory_refresh_fail          = 0x11,
    NM_query_snvt_fail              = 0x12,
    NM_nv_fetch_fail                = 0x13,
    NM_device_esc_fail              = 0x1D,
    NM_router_esc_fail              = 0x1E,
    NM_service_pin_fail             = 0x1F,  /* doesn't really exist */
} _NM_response_code;
typedef NetEnum(_NM_response_code) NM_response_code;

typedef enum
{
	QUERY_ID_UNCONFIGURED,
	QUERY_ID_SELECTED,
	QUERY_ID_SELECTED_UNCONFIGURED
} _QueryIdSelector;
typedef NetEnum(_QueryIdSelector) QueryIdSelector;

typedef struct
{
	Byte			code;
	QueryIdSelector selector;
} NM_query_id_request;

typedef struct
{
	Byte			code;
	QueryIdSelector selector;
	Byte			mode;
	USHORT			offset;
	Byte			len;
	Byte			data[1];
} NM_query_id_request_conditional;

typedef struct
{
	Byte code;
	Byte on;		// 1 => Respond to query is on, 0 => Respond to query is off
} NM_respond_to_query_request;

typedef enum _nm_checksum_recalc {
	/* 1 */		BOTH_CS = 1,
	/* 4 */		CNFG_CS = 4,
} _nm_checksum_recalc;
typedef NetEnum(_nm_checksum_recalc) nm_checksum_recalc;

typedef struct
{
	Byte code;
	nm_checksum_recalc option;
} NM_checksum_recalc_request;

/* aliases for backward compatibility */
#define NM_ESCAPE_CODE NM_device_esc
#define ND_query_status_succ    ND_query_status_success
#define ND_clear_status_succ    ND_clear_status_success
#define NM_update_nv_cnfg_succ  NM_update_nv_cnfg_success
#define NM_query_nv_cnfg_succ   NM_query_nv_cnfg_success
#define NM_set_node_mode_succ   NM_set_node_mode_success
#define NM_query_snvt_succ      NM_query_snvt_success
#define NM_NV_fetch             NM_nv_fetch
#define NM_NV_fetch_succ        NM_nv_fetch_success
#define NM_NV_fetch_fail        NM_nv_fetch_fail


#if 0 // NVs not required
/* NM_query_nv_cnfg */
typedef struct
{
   Byte      code;
   nv_struct nv_cnfg;
} NM_query_nv_cnfg_response;
#endif // NVs not required
/* NM_set_node_mode */
typedef enum
{
   APPL_OFFLINE = 0,         /* Soft offline state */
   APPL_ONLINE,
   APPL_RESET,
   CHANGE_STATE,

   APPL_UNDEFINED = 0xff	// Can be used as a do-nothing type ping message
} _nm_node_mode;
typedef NetEnum(_nm_node_mode) nm_node_mode;

/*
   The neuron "state" reported by the query status command contains a 3 bit
   state value stored in the node's EEPROM, plus an online bit and an
   offline/bypass mode bit.  In general the offline and offline/bypass mode
   bits are useful only if the 3 bit neuron state state is "configured".
   However, for debugging purposes it might be useful to know if the node was
   taken offline prior to having its state changed - the offline bit indicates
   this (although if the node is reset after the state change the offline bit
   will not be set).

   The following literals and macros can be used to interpret the node state.

*/

#define OFFLINE_BIT 0x8
#define NODE_STATE_MASK 0x7

/* return the 3 bit neuron state stored in EEPROM. */
#define NEURON_STATE(state) ((state)&NODE_STATE_MASK)

/* return nm_node_state - literals defined below */
#define NODE_STATE(state) ((NEURON_STATE(state) == CNFG_ONLINE) \
    ? (state) : NEURON_STATE(state))

/* Return 0 if the state indicates offline, non-zero otherwise. */
#define NODE_STATE_OFFLINE(state) ((state) & OFFLINE_BIT)


/* This enumeration is used with
        NM_set_node_mode_request  and
        ND_query_status_response.


   This ENUM contains the documented values of the neuron state. To convert
   the state value returned by the ND_query_status command to one of these
   values, use the macro NODE_STATE() defined above.  To see if the offline
   bit is set, use the macro NODE_STATE_OFFLINE().
    */

typedef struct
{
   Byte code;
   Byte mode;          /* Interpret with 'nm_node_mode'        */
   Byte node_state;    /* Optional field if mode==CHANGE_STATE */
                  /* Interpret with 'nm_node_state'       */
} NM_set_node_mode_request;

/* NM_write_memory */
typedef enum
{
    ABSOLUTE_MEM_MODE  = 0,         /* Address is absolute neuron mem address */
    READ_ONLY_RELATIVE = 1,         /* Address is offset from beginning of
                                       read-only memory structures. */
    CONFIG_RELATIVE    = 2,         /* Address is offset from beginning of
                                       config data structures. */
    STATS_RELATIVE     = 3,         /* Address is offset from beginning of
                                       statistics data structures NM_stats_struct. */
    MEM_MODE_RESERVED_A= 4          /* Reserved for Echelon internal use only */
} _NmMemMode;
typedef NetEnum(_NmMemMode) NmMemMode;

typedef enum
{
   NO_ACTION      = 0,
   BOTH_CS_RECALC = 1,
   DELTA_CS_RECALC= 3,
   CNFG_CS_RECALC = 4,
   ONLY_RESET     = 8,
   BOTH_CS_RECALC_RESET = 9,
   CNFG_CS_RECALC_RESET = 12
} _NmMemForm;
typedef NetEnum(_NmMemForm) NmMemForm;

typedef struct
{
   Byte    code;
   Byte    mode;
   Byte    offset_hi;
   Byte    offset_lo;
   Byte    count;
   Byte    form;          /* followed by the data */
   Byte	   data[1];
} NM_write_memory_request;

/* this is an alternative declaration for NM_write_memory_request that
   resembles the declaration in the neuron-c include file */
typedef struct
{
   Byte    code;
   Byte    mode;
   NetWord offset;
   Byte    count;
   Byte    form;          /* followed by the data */
} NM_write_memory_request_nc;


/* NM_query_snvt */
/* Partial list of SNVT type index values */
typedef enum
{
   SNVT_str_asc  = 36,
   SNVT_lev_cont = 21,
   SNVT_lev_disc = 22,
   SNVT_count_f  = 51
} SNVT_t;

typedef struct
{
    Byte      code;
    NetWord   offset;     /* big-endian 16-bits */
    Byte      count;
} NM_query_snvt_request;

typedef struct
{
    Byte		id[ DOMAIN_ID_LEN ];
    Byte		subnet;
	BITS2	   (node,			7,
				must_be_one,	1) /* this bit must be set to 1 */
	BITS4      (len,			3,	/* Domain len (0, 1, 3, 6) */
				oma,			2,	/* 1=> open media authentication */
				mbz,			2,
				invalid,		1)  /* 1=> domain entry is invalid */
} NM_dmn_id;

typedef struct
{
	NM_dmn_id	id;
    Byte		key[ AUTH_KEY_LEN ];
} NM_domain;

/* NM_update_domain */
typedef struct
{
    Byte        code;
    Byte        domain_index;
	NM_domain	dmn;
} NM_update_domain_request;

/* NM_query_domain */
typedef struct
{
    Byte        code;
    Byte        domain_index;
} NM_query_domain_request;

typedef struct
{
    Byte        code;
	NM_domain	dmn;
} NM_query_domain_response;

/* NM_leave_domain */
typedef struct
{
    Byte        code;
    Byte        domain_index;
} NM_leave_domain_request;

/* NM_update_address */
typedef struct
{
	Byte		code;
	Byte		address_index;
	Byte		address[5];		// TBD - define this.
} NM_update_address_request;

/* NM_read_memory */
typedef struct
{
    Byte        code;
    Byte        mode;               /* enum nm_mem_mode */
	Byte		offset_hi;
	Byte		offset_lo;
    Byte        count;
} NM_read_memory_request;

/* NM_device_esc
 * The LONTalk network management commands for device escape all begin
 * with the following 3-byte sequence:
 *      <Device Escape Code> <Device Code> <Optional Command code>
 */
#define DEVICE_ESC_SEQ_LEN          3

/*
 * Header structure for all messages using the device escape code.
 */
typedef struct
{
    Byte     device_code;
    Byte     device_command;
    /* command-specific data follows */
} NM_device_request_header;

typedef struct
{
    Byte        code;           /* NM_device_esc */
    NM_device_request_header devReqHdr;
} NM_device_request;

/*
 * Device Escape Product Query
 */
#define PRODUCT_QUERY_PRODUCT_CODE 1
#define PRODUCT_QUERY_COMMAND 1

/* values for response field "product" */
#define MIP_PRODUCT_CODE    2
#define NS_PRODUCT_CODE     3
#define NS_OVERRIDE_CODE      5

/* values for NS_PRODUCT_CODE response field "model" */
#define NSS_10_MODEL_CODE 1     /* returned by NSS-10 and NSI-10 firmware */
#define LNS_VNI_MODEL_CODE 2    /* returned by NSS/NSI using VNI */

typedef struct
{
    Byte        code;           /* NM response code */
    Byte        product;
    Byte        model;          /* product specific values */
    Byte        version;        /* product/model specific values */
    Byte        product_specific; /* product specific interpretation */
	BITS2      (transceiver_ID,		5,
				XID_reserved,		3);
} NM_product_query_response;

/* NMX_query_version */
typedef struct
{
	Byte		code;
	Byte		subcode;
} NMX_query_version_request;

// Literals for capabilities2
#define LT_EXP_CAP_OMA						    	0x01
#define LT_EXP_CAP_PROXY						    0x02
#define LT_EXP_CAP_PHASE_DETECTION				    0x04
#define LT_EXP_CAP_BI_DIRECTIONAL_SIGNAL_STRENGTH	0x08
#define LT_EXP_CAP_INIT_CONFIG						0x10

typedef struct
{
	Byte		code;
	Byte		subcode;
	Byte		version;
	Byte		capabilities1;
	Byte		capabilities2;
} NMX_query_version_response;

/* NMX_update_domain */
typedef struct
{
    Byte        code;
	Byte		subcode;
    Byte        domain_index;
	NM_dmn_id	dmn;
} NMX_update_domain_request;

/* NMX_query_domain */
typedef struct
{
    Byte        code;
	Byte		subcode;
    Byte        domain_index;
} NMX_query_domain_request;

typedef struct
{
    Byte        code;
	Byte		subcode;
	NM_dmn_id	dmn;
} NMX_query_domain_response;

/* NMX_update_key */
typedef struct
{
	Byte		code;
	Byte		subcode;
	Byte		inc;				/* 1=> key is an increment */
	Byte		key[12];
} NMX_update_key_request;

/* NMX_query_key */
typedef struct
{
	Byte		code;
	Byte		subcode;
} NMX_query_key_request;

typedef struct 
{
	Byte		code;
	Byte		subcode;
	Byte		key[12];
} NMX_query_key_response;

/* NMX_init_config */
#define MAX_NV_AUTH_LEN				32
typedef struct
{
	Byte		code;
	Byte		subcode;
	Byte		nvAuthLen;
	Byte		nvAuth[1];			// Supports up to MAX_NV_AUTH_LEN*8 NVs on a device.
} NMX_init_config_request;

typedef struct
{
	BITS3      (max_wait,		3,
				nm_auth,		1,
				rcv_timer,		4);
} nmMsgParms;

/* NEURON CHIP model numbers returned by the Query Status command response. */
/* Note: all 256 values are potentially valid, and new models may not be in this list. */
/* Values 128 and above  are defined as LonTalk protocol implementations */
/* not hosted on a Neuron Chip. */
typedef enum _nm_model_code
{
        /* 0 */         NEURON_3150_CODE   =  0,
        /* 8 */         NEURON_3120_CODE   =  8,
        /* 9 */         NEURON_3120E1_CODE =  9,
        /* 10 */        NEURON_3120E2_CODE = 10,
        /* 11 */        NEURON_3120E3_CODE = 11,
        /* 12 */        NEURON_3120A20_CODE = 12,
        /* 13 */        NEURON_3120E5_CODE = 13,
        /* 128 */       NON_NEURON_GENERIC_CODE = 128,
        /* 129 */       NON_NEURON_PENTAGON_CODE = 129,
        /* 130 */       NON_NEURON_MIPS_CODE = 130,
} _nm_model_code;

typedef NetEnum(_nm_model_code) nm_model_code;

/* STUFF ADDED HERE BECAUSE OF INCOMPATIBLE HEADER FILES
 */

/* This enumeration is used with
        NM_set_node_mode_request  and
        ND_query_status_response.


   This ENUM contains the documented values of the neuron state. To convert
   the state value returned by the ND_query_status command to one of these
   values, use the macro NODE_STATE() defined above.  To see if the offline
   bit is set, use the macro NODE_STATE_OFFLINE().
    */

typedef enum _nm_node_state {
   STATE_INVALID        = 0,    /* invalid or echelon use only          */
   STATE_INVALID_1      = 1,    /* equivalent to STATE_INVALID          */
   APPL_UNCNFG          = 2,    /* has application, unconfigured        */
   NO_APPL_UNCNFG       = 3,    /* applicationless, unconfigured        */
   CONFIGURED           = 4,    /* configured		                    */
   STATE_INVALID_5      = 5,    /* equivalent to STATE_INVALID          */
   HARD_OFFLINE         = 6,    /* hard offline                         */
   NO_APPL_CNFG		    = 7,    /* appless but configured		        */
} _nm_node_state;

typedef NetEnum(_nm_node_state) NmNodeState;


/* This enumeration is used with Network Diagnostic
   response error_log field in the command - Query Status Response */
typedef enum _nm_error {
#ifndef NO_ERROR                    /* NO_ERROR is also defined in Windows */
        /*   0 */       NO_ERROR = 0,
#endif
        /* 129 */       BAD_EVENT =
#ifdef COMPILE_8                    /* special casting for neuron C compiler */
                         (signed char)
#endif
                                       129,
        /* 130 */       NV_LENGTH_MISMATCH,
        /* 131 */       NV_MSG_TOO_SHORT,
        /* 132 */       EEPROM_WRITE_FAIL,
        /* 133 */       BAD_ADDRESS_TYPE,
        /* 134 */       PREEMPTION_MODE_TIMEOUT,
        /* 135 */       ALREADY_PREEMPTED,
        /* 136 */       SYNC_NV_UPDATE_LOST,
        /* 137 */       INVALID_RESP_ALLOC,
        /* 138 */       INVALID_DOMAIN,
        /* 139 */       READ_PAST_END_OF_MSG,
        /* 140 */       WRITE_PAST_END_OF_MSG,
        /* 141 */       INVALID_ADDR_TABLE_INDEX,
        /* 142 */       INCOMPLETE_MSG,
        /* 143 */       NV_UPDATE_ON_OUTPUT_NV,
        /* 144 */       NO_MSG_AVAIL,
        /* 145 */       ILLEGAL_SEND,
        /* 146 */       UNKNOWN_PDU,
        /* 147 */       INVALID_NV_INDEX,
        /* 148 */       DIVIDE_BY_ZERO,
        /* 149 */       INVALID_APPL_ERROR,
        /* 150 */       MEMORY_ALLOC_FAILURE,
        /* 151 */       WRITE_PAST_END_OF_NET_BUFFER,
        /* 152 */       APPL_CS_ERROR,
        /* 153 */       CNFG_CS_ERROR,
        /* 154 */       INVALID_XCVR_REG_ADDR,
        /* 155 */       XCVR_REG_TIMEOUT,
        /* 156 */       WRITE_PAST_END_OF_APPL_BUFFER,
        /* 157 */       IO_READY,
        /* 158 */       SELF_TEST_FAILED,
        /* 159 */       SUBNET_ROUTER,
        /* 160 */       AUTHENTICATION_MISMATCH,
        /* 161 */       SELF_INST_SEMAPHORE_SET,
        /* 162 */       READ_WRITE_SEMAPHORE_SET,
        /* 163 */       APPL_SIGNATURE_BAD,
        /* 164 */       ROUTER_FIRMWARE_VERSION_MISMATCH,
} _nm_error;
typedef NetEnum(_nm_error) nm_error;

typedef struct {
	BITS4		   (node_state,		3,
					offline,		1,
					rsvd,			3,
					bypass,			1)
} NmNodeStatus;
 
typedef struct ND_query_status_response {
    NetWord         xmit_errors;
    NetWord         transaction_timeouts;
    NetWord         rcv_transaction_full;
    NetWord         lost_msgs;
    NetWord         missed_msgs;
    Byte            reset_cause;        /* use resetCause... macros to eval */
	NmNodeStatus	status;
    Byte            version_number;
    nm_error        error_log;
    nm_model_code   model_number;
} ND_query_status_response;


// NM_read_memory response for STATS_RELATIVE addressing mode
typedef struct NM_stats_struct
{
	NetWord transmission_errors;
	NetWord transmit_tx_failures;
	NetWord receive_tx_full;
	NetWord lost_messages;
	NetWord missed_messages;
	NetWord layer2_received;
	NetWord layer3_received;
	NetWord layer3_transmitted;
	NetWord transmit_tx_retries;
	NetWord backlog_overflows;
	NetWord late_acknowledgements;
	NetWord collisions;
} NM_stats_struct;


// Query ID response
typedef struct {
	Byte	nid[6];
	Byte	pid[8];
} NM_query_id_response;

#define PLT_MARGIN_STEP_SIZE	3		// 3 dB per margin step
#define PLT_MARGIN_MIN			0		// Min encoded value
#define PLT_MARGIN_MAX			15		// Max encoded value

#define PLT_STRENGTH_STEP_SIZE  6		// 6 dB per strength step
#define PLT_STRENGTH_MIN		0		// Min encoded value
#define PLT_STRENGTH_MAX		15		// Max encoded value.

// Margin values range from -6 dB to 39 dB in 3 dB steps
// Strength values range from -84 dB to 6 dB in 6 dB steps
#define PLT_MARGIN_39dB			15
#define PLT_MARGIN_15dB		 7
#define PLT_MARGIN_9dB		 5
#define PLT_MARGIN_6dB		 4
#define PLT_MARGIN_3dB		 3
#define PLT_MARGIN_NEG3dB	 1
#define PLT_MARGIN_NEG6dB	 0
#define PLT_MARGIN_0dB			2
#define PLT_MARGIN_THRESHOLD PLT_MARGIN_15dB		// Agent must have at least this much margin to be considered a quality agent
#define PLT_MARGIN_REPLACE_MIN	PLT_MARGIN_6dB
#define PLT_STRENGTH_60dB	 4
#define PLT_STRENGTH_30dB	 9

// These define an "unknown" margin/strength
#define PLT_MARGIN_UNKNOWN		PLT_MARGIN_MAX
#define PLT_STRENGTH_UNKNOWN	PLT_STRENGTH_MIN

// These fudge factor come from Phil Sutterlin based on empirical data and gut feel.
#define PLT_NI_MARGIN_PENALTY -6		// This number is applied to the network interface when it has a version number
										// of 3 (i.e., the PLT-22 interface).
#define PLT_PHASE_MARGIN_PENALTY -6		// This penalty is applied when an agent is on a different phase than the target.

typedef struct
{
	BITS2          (strength,		4,
					margin,			4)
} FreqQuality;

typedef struct
{
	FreqQuality		pri;				// Primary frequency quality
	FreqQuality		sec;				// Secondary frequency quality
} SignalQuality;

// A composite form of the margin and signal strength for the target and the agent!
typedef USHORT SignalQualityMeasure;

typedef struct 
{
	BITS3          (version,		4,
					rsvd0,			1,
					threshold,		3)
	BITS1		   (snavg,			8)
	BITS5          (bndinuse,		1,	// Set if last packet tripped BNDINUSE
					parity,			1,	// Set if last packet had a parity error
					pktflg,			1,	// Set if received one and only one packet since last read of status reg 3.
					secondary,		1,	// 1 => Last packet on secondary (0 => primary in case it wasn't obvious).
					rsvd1,			4)
    SignalQuality	sigQual;			// Signal quality
	BITS2		   (unused,			6,
					format,			2)	// See FrequencyFormat
	Byte			unused2;
} PLTRegisters;

typedef struct ND_signal_strength_response {
	PLTRegisters	targetReg;
	PLTRegisters	sourceReg;
} ND_signal_strength_response;

/* macros for evaluating the reset cause byte.  Example usage:
 *
 *      status_struct status;
 *      ....
 *      if (resetCauseWDT(status.reset_cause))
 *      {
 *          ....
 *      }
 *      ...
 */
#define resetCauseCleared(cause)  (cause == 0)
#define resetCausePowerUp(cause)  ((cause & 1)    == 1)
#define resetCauseExternal(cause) ((cause & 3)    == 2)
#define resetCauseWDT(cause)      ((cause & 0xf)  == 0xc)
#define resetCauseSoftware(cause) ((cause & 0x1f) == 0x14)

typedef struct
{
	Byte			phase;	// Phase/60 + 1 (0 => unknown)
	Byte			freq;	// 50 or 60 hz
	Byte			raw;	// Raw I/O reading (for debug)
	signed char		certainty; // Closer to 0, the better -7 to +7
} ND_phase_response;

#define NUM_COMM_PARAMS 7

typedef struct xcvr_status_struct
{
    Byte            xcvr_params[NUM_COMM_PARAMS];
} xcvr_status_struct;


typedef NetWord NeuronMemAddr;              /* neuron memory address */
typedef struct EncodedBuffers
{
   BITS2   (app_buf_size_out,4,
			app_buf_size_in, 4)
   BITS2   (net_buf_size_out,4,
		    net_buf_size_in, 4)
    Byte    priority_buf_counts;
    Byte    app_buf_counts;
    Byte    net_buf_counts;
} EncodedBuffers;

typedef struct read_only_data_reference
{
    Byte    neuron_id [LON_NEURON_ID_LEN];
    Byte    model_num;
    Byte    minor_model_num;
    NeuronMemAddr                   nv_fixed;   /* ptr nv_fixed_struct */
    Byte    nv_count_rwp;                       /* NV count && read write
                                                   protect */
    NeuronMemAddr                   snvt;       /* ptr snvt_struct */
    Byte    id_string [LON_PROG_ID_LEN];
    Byte    state_byte;
    Byte    address_count;
    Byte    rcvtx_count;
    EncodedBuffers buffers;
    Byte    reserved1 [6];
    Byte    tx_by_address;
    Byte    alias_count;
    Byte    msg_tag_count;                      /* also contains capability_info */
    Byte    reserved2 [3];
} read_only_data_reference;

typedef struct short_read_only_data_struct
{
    BITS3  (nv_count,			6,
			unused,				1,
			read_write_protect, 1)
    NeuronMemAddr                  snvt; /* ptr snvt_struct */
    Byte    id_string [LON_PROG_ID_LEN];
    BITS4  (unused2,			5,       /* resv0 */
			explicit_addr,      1,
			two_domains,	    1,
			NV_processing_off,  1);

    BITS2  (unused3,			4,
			address_count,		4);
} short_read_only_data_struct;

typedef struct
{
	Byte	type;
	Byte	info[4];				  // TBD - to be supplied
} address_struct;

typedef struct nv_struct
{
    BITS3  (nv_selector_hi,		6,
			nv_direction,		1,    /* 1 => output */
			nv_priority,		1);

    Byte    nv_selector_lo;

    BITS4  (nv_addr_index,		4,    /* Address table index */
			nv_auth,			1,
			nv_service,			2,
			nv_turnaround,		1);
} nv_struct;

/* Structure used by host application to store network variables */
//#if 0 // NVs not required
typedef enum
{
   NV_IN = 0,
   NV_OUT = 1
} nv_direction;

#if 0 // NVs not required
typedef struct
{                                      /* structure to define NVs */
   int          size;                  /* number of Bytes */
   nv_direction direction;             /* input or output */
   const char*  name;                  /* name of variable */
   void ( * print_func )(Byte *);      /* routine to print value */
   void ( * read_func )(Byte *);       /* routine to read value */
   Byte         data[MAX_NETVAR_DATA]; /* actual storage for value */
} network_variable;
#endif

typedef struct
{
   Byte         code;
   Byte         index;
} short_index_struct;

typedef struct
{
   short_index_struct short_index;
   nv_struct          nv_cnfg_data;
} short_config_data_struct;


#define HOST_NV_INDEX_ESCAPE 255
typedef struct
{
    Byte            code;
    Byte            index_escape;   /* Must be 255 (HOST_NV_INDEX_ESCAPE) */
    NetWord         index;
} long_index_struct;


typedef struct
{
   long_index_struct index;
   nv_struct         nv_cnfg_data;
} long_config_data_struct;

typedef struct alias_struct
{
    nv_struct   alias_nv;
    Byte        alias_primary;
} alias_struct;

typedef struct
{
   short_index_struct short_index;
   alias_struct       alias_cnfg_data;
} short_alias_config_struct;


typedef struct
{
   long_index_struct long_index;
   alias_struct      alias_cnfg_data;
   NetWord           host_primary_index;
} long_alias_config_struct;

/* NM_update_nv_cnfg */
typedef union NM_update_nv_cnfg_request
{
   short_config_data_struct  short_config_index;
   long_config_data_struct   long_config_index;
   short_alias_config_struct short_alias_config_index;
   long_alias_config_struct  long_alias_config_index;
} NM_update_nv_cnfg_request;

/* NM_query_nv_cnfg */
typedef union NM_query_nv_cnfg_request
{
   short_index_struct   short_index;
   long_index_struct long_index;
} NM_query_nv_cnfg_request;
typedef struct
{
   nv_struct nv_cnfg;
} NM_query_nv_cnfg_response;

/*  NM_nv_fetch */
typedef NM_query_nv_cnfg_request NM_nv_fetch_request;

typedef union NM_nv_fetch_response
{
   short_index_struct   short_index;
   long_index_struct    long_index;   /* followed by data */
} NM_nv_fetch_response;

//#endif // NVs not required

typedef struct
{
	Byte	code;
	Byte	subcode;
	Byte	resource;
	Word	fromKey;
	Word	toKey;
} NM_ecs_init_request;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#pragma pack()

#endif
