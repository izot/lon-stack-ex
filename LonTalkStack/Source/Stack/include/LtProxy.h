//
// LtProxy.h
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

#ifndef __LTPROXY_H
#define __LTPROXY_H

#pragma pack(1)		//_DCX_PACKING

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

// FUTURE (EVNI): eVNI doesn't use bitfields as a rule but for now leaving this in.  Clean this up later?
// The reason for not using bitfields is because of the portability (or lack thereof) and also because
// the eVNI code originally came from Java which has no bitfields.
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

#pragma pack()

#endif // __LTPROXY_H
