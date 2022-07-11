//
// LonTalk Enhanced Proxy library
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
// Internal definitions
//
//  PS - proxy source - initiates transaction chain.
//  PR - proxy repeater - forwards proxy message.
//  PA - proxy agent - sends normal message to target.
//  PT - proxy target - terminates proxy chain.
//

#ifndef PROXYINT_H
#define PROXYINT_H

#include "sicb.h"

struct Sicb;

// Proxy message code
#define PROXY_COMMAND  0x4D

#define PROXY_SUCCESS	0x4D
#define PROXY_FAILURE	0x4C	

//
// If the application wishes the PROXY layer to buffer responses for it, then it 
// must call "enable_response_buffering" during initialization and it must call
// "set_buffered_respons" for each response sent that is greater than 2 bytes in 
// length (counting the code).  Saving responses of length less than 2 is benign.

//
// Returns 1 if the message was handled.  If so, the
// message is not to be processed further by the caller.
// Invoked by the MIP.
//
extern  int _proxy_handler(void);

//
// Does the bulk of the proxy processing.
//
extern  void handle_proxy(struct Sicb* pSicb, int tag, int* pCnt);

// 
// Returns pointer to priority or non-priority buffer, NULL if none.
//
extern  struct Sicb* proxy_out_alloc(boolean pri);

// Following the proxy message code are N subnet/node definitions
typedef struct ProxyNeuronIdAddress
{
    unsigned char     subnet;           // routing subnet
    unsigned char     nid[6];         // Neuron ID
} ProxyNeuronIdAddress;

typedef struct ProxyNeuronIdAddressCompact
{
    // Routing subnet of 0 is used.
    unsigned char     nid[6];         // Neuron ID
} ProxyNeuronIdAddressCompact;

typedef struct ProxySubnetNodeAddress
{
	unsigned char subnet;
	unsigned char node : 7;
	unsigned char path : 1;
} ProxySubnetNodeAddress;

typedef struct ProxySubnetNodeAddressCompact
{
    // Subnet used is that of agent
    unsigned char     node : 7;       // Node
    unsigned char     path : 1;
} ProxySubnetNodeAddressCompact;

typedef struct ProxyGroupAddress
{
    unsigned char     group;
    unsigned char     size;
} ProxyGroupAddress;

typedef struct ProxyGroupAddressCompact
{
    // Size of 0 is used (for unackd and unackd/rpt)
    unsigned char     group;
} ProxyGroupAddressCompact;

typedef struct ProxyBroadcastAddress
{
    unsigned char     subnet;
    unsigned char     backlog;
} ProxyBroadcastAddress;

// Note that this is defined as a union.  However, the actual
// size of data shipped is the minimum size needed to convey
// the enclosed structure.
typedef union ProxyTargetAddress
{
    ProxyNeuronIdAddress nid;
    ProxyNeuronIdAddressCompact nidc;
    ProxySubnetNodeAddress sn;
    ProxySubnetNodeAddressCompact snc;
    ProxyGroupAddress gp;
    ProxyGroupAddressCompact gpc;
    ProxyBroadcastAddress bc;
} ProxyTargetAddress;

typedef struct ProxyHeader
{
    unsigned char     count : 4;      // repeater count
    unsigned char     uniform_by_src  : 1;  // 1 => uniform addressing by source subnet
                                      // subnet
	unsigned char	  all_agents : 1; // 1 => all repeaters are agents too.
	unsigned char	  long_timer : 1; // 1 => repeaters use long timers 
									  // (effectively makes timer field 5 bits).
    unsigned char     uniform_by_dest : 1;	// 1 => uniform addressing by destination subnet
} ProxyHeader;

typedef struct ProxyTxCtrl
{
    unsigned char     timer : 4;     // retry timer used by
                                     // repeater
    unsigned char     retry : 4;     // retry count used by
                                     // repeater
} ProxyTxCtrl;

typedef enum
{
    PROXY_AGENT_MODE_NORMAL = 0,
    PROXY_AGENT_MODE_ZERO_SYNC = 1,
    PROXY_AGENT_MODE_ALTKEY = 2,
    PROXY_AGENT_MODE_ATTENUATE = 3
} ProxySicbMode;

//
//
// This structure differs from the normal SICB in the following ways:
// 1. No auth field is needed - this is inherited.
// 2. No tag field is needed - we correlate using the rcvtx index.
// 3. No length field is needed - this comes from the message length.
//
typedef struct ProxySicb
{
    // The following fields are used by the agent to talk
    // to the target.
	unsigned char	mode	: 2;		/* ProxySicbMode */
	unsigned char	service	: 2;		/* Service Type */
	unsigned char	path	: 1;		/* normal or alt path */
	unsigned char	type	: 3;		/* ProxyAddressType */
    ProxyTxCtrl       txctrl;
} ProxySicb;

#define AUTH_STD	0
#define AUTH_OMA	1

typedef struct ProxyAuthKey
{
    // if "altkey" is zero
	unsigned char     type:2;		  // AUTH_STD
	unsigned char     mbz:6;
    unsigned char     key[6];		  // 48 bit key
} ProxyAuthKey;

typedef struct ProxyOmaKey
{
    // if "altkey" is one
	unsigned char     type:2;		  // AUTH_OMA
	unsigned char     mbz:6;
    unsigned char     key[12];		  // 96 bit key
} ProxyOmaKey;

#define MAX_PROXY_DATA 102

typedef struct ProxyTargetApdu
{
    unsigned char     code;
    unsigned char     data[MAX_PROXY_DATA];      // message data where
    // length is determined by length of remainder of packet.
} ProxyTargetApdu;

typedef enum
{
    PX_GROUP = 0,
    PX_SUBNET_NODE = 1,
    PX_NEURON_ID = 2,
    PX_BROADCAST = 3,
    PX_GROUP_COMPACT = 4,
    PX_SUBNET_NODE_COMPACT_SRC = 5,
    PX_NEURON_ID_COMPACT = 6,
	PX_SUBNET_NODE_COMPACT_DEST = 7,

	PX_ADDRESS_TYPES 
} ProxyAddressType;

//
// Simplify subnet node address setup
//
#ifndef _NEURON_C
#define SNODE pSicb->addr.out.snode
#define NID   pSicb->addr.out.nrnid
#define BCAST pSicb->addr.out.bcast
#define GROUP pSicb->addr.out.group
#define PROXY pSicb->addr.out.proxy
#else
#define SNODE msg_out.dest_addr.snode
#define NID   msg_out.dest_addr.nrnid
#define BCAST msg_out.dest_addr.bcast
#define GROUP msg_out.dest_addr.group
#define PROXY msg_out.dest_addr.proxy
#endif

#if 0
typedef struct
{
    unsigned char key[6];
} AuthKey;

typedef struct
{
	int nsa;		
    AuthKey keys[2];
} AltAuthKeys;
#endif

extern void _sicb_send(int len);
extern void _clear_apps_flags(int mask);
extern int msg_max_data;
extern void _sys_flag_set(int f);
extern void _sys_flag_clear(int f);
extern boolean _get_auth_std(void);
extern int get_power_line_frequency(void);
extern int _last_retry_extra_time;

//
// Send an SICB
//
//extern system far void proxy_out(SicbByte* pSicb, int byte0, int byte1, int code, const unsigned char* p, int len);

// 
// Returns pointer to priority or non-priority buffer, NULL if none.
//
extern Sicb* proxy_out_alloc(boolean pri);

// 
// Reallocates input buffer as output buffer.
//
extern void proxy_out_realloc(Sicb* pSicb);

//
// Frees a proxy completion code or response
//
//MS extern system far void proxy_ccresp_free(Sicb* pSicb);

// Must define all system far routines as extern before use.
extern void repeater_as_agent(Sicb* pSicb, int txcpos);
extern void proxy_response(int tag, int code, const unsigned char* p, int len);

#endif
