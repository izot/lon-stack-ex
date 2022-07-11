#ifndef _VXSOCKETS_H
#define _VXSOCKETS_H
/***************************************************************
 *  Filename: VxSockets.h
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
 *  Description:  Definitions for VxWorks portable socket layer.
 *
 *	DJ Duffy Oct 1998
 *
 ****************************************************************/

/*
 * $Log: /Dev/ShareIp/include/VxSockets.h $
 *
 * 14    11/09/06 8:28a Glen
 * Changed port to be unsigned
 *
 * 13    12/08/05 2:11p Fremont
 * Fix INADDR_xxx macro conflicts by replacing them with vxsINADDR_xxx
 * macros
 *
 * 12    7/19/05 11:44p Fremont
 * Add vxsMakeDottedAddr() for a portable equivalent to inet_ntoa()
 *
 * 11    7/05/05 5:30p Fremont
 * Expose hostGeyByName as an external API call
 *
 * 10    4/07/04 4:38p Rjain
 * Added new VxLayer Dll.
 *
 * 9     5/29/02 4:54p Fremont
 * Added some functions for EDC modules
 *
 * 7     10/08/99 5:15p Fremont
 * add select function
 *
 * 6     9/08/99 3:11p Darrelld
 * Support for TOS bits
 *
 * 5     6/14/99 4:51p Darrelld
 * Add RecvFrom flags
 *
 * 4     6/14/99 4:36p Darrelld
 * Added vxsAddrGetAddr and vxsAddrGetPort
 *
 * 3     12/02/98 12:50p Darrelld
 * Fix vxsFreeSockaddr parameter
 *
 * 4     10/27/98 4:49p Darrelld
 * Time client / server work
 *
 * 3     10/23/98 5:05p Darrelld
 * Enhancements and socket testing
 *
 * 2     10/22/98 9:13a Darrelld
 * initial checkin
 *
 * 1     10/20/98 4:34p Darrelld
 * Vx Layer Socket Interface
 *
 */


//
// A socket interface which is host endian and platform neutral
// It doesn't support everything. Only enough to get our job done
//
// avoid include of winsock.h here
//

// BOOL, LPSTR etc.
#include	<VxlTypes.h>
#include    "VxLayerDll.h" // VXLAYER_API

#ifdef __cplusplus
extern "C" {
#endif

// Same as VxWorks, but strongly typed.
typedef int VXSOCKET;

typedef struct _VXSOCKADDR* VXSOCKADDR;

// MAX SOCKETS WE SUPPORT
// Ignored on VxWorks actually
#define VXMAXSOCKETS	256

//#ifndef WIN32  
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (VXSOCKET)(~0)
#endif // INVALID_SOCKET

//
// Only these are supported
//#define SOCK_STREAM     1               /* stream socket */
//#define SOCK_DGRAM      2               /* datagram socket */
//

//

#define VXSOCK_STREAM	1
#define	VXSOCK_DGRAM	2

												/* flags for vxsRecvFrom */
#define VXSOCK_MSG_OOB         0x1             /* process out-of-band data */
#define VXSOCK_MSG_PEEK        0x2             /* peek at incoming message */
#define VXSOCK_MSG_DONTROUTE   0x4             /* send without using routing tables */

// Define alternatives to the INADDR_XXX macros to eliminate header file conflicts
#define vxsINADDR_ANY              (ULONG)0x00000000
#define vxsINADDR_LOOPBACK         (ULONG)0x7f000001
#define vxsINADDR_BROADCAST        (ULONG)0xffffffff
#define vxsINADDR_NONE             0xffffffff


// Create a socket, only supply a type
VXLAYER_API VXSOCKET	vxsSocket( int type );

VXLAYER_API VXSOCKET vxsRawSocket(int type, int proto);

// Close the socket
VXLAYER_API STATUS		vxsCloseSocket( VXSOCKET sock );
VXLAYER_API STATUS		vxsShutdownSocket( VXSOCKET sock, int how );

// Get and free socket addresses
VXLAYER_API VXSOCKADDR	vxsGetSockaddr();
VXLAYER_API void		vxsFreeSockaddr( VXSOCKADDR psad );

// SocketAddr from dotted string
VXSOCKADDR	vxsAddrDotted( const char* pszDottedStr );

VXLAYER_API ULONG		vxsInetAddr( LPSTR pszDottedStr );

// Return name of the local system
VXLAYER_API STATUS		vxsGetHostName( LPSTR pszNameRtn, int nMaxLen );

// Return IP addr of local host
int vxsGetLocalAddr(void);

// Return IP addr of local host as a dotted string
STATUS vxsGetLocalAddrDotted(char *buf);

// Make a dotted string from an address
VXLAYER_API void vxsMakeDottedAddr(char *buf, ULONG inAddr);

// This will work for IPV4 not IPV6
VXLAYER_API VXSOCKADDR	vxsAddrValue( ULONG inetAddr );

// SocketAddr from name string
// Uses hostGetByName
VXLAYER_API VXSOCKADDR	vxsAddrName( LPSTR pszName );

// VxWorks interface replacement
VXLAYER_API int hostGetByName(char *name);

// set the port number in a sockaddr structure
VXLAYER_API void		vxsSetPort( VXSOCKADDR psad, unsigned short port );

// Get name from a socket address
// Uses hostGetByAddr
// maxNameSize must be MAXHOSTNAMELEN + 1 or larger
// DON"T ASSUME THIS VALUE, VXWORKS.H RULES
#ifndef MAXHOSTNAMELEN
//#define MAXHOSTNAMELEN 16
// somebody said this could be a "fully qualified domain name"
// so be better allow plenty of space
#define MAXHOSTNAMELEN 64
#endif // MAXHOSTNAMELEN

STATUS		vxsAddrGetName( VXSOCKADDR sa, LPSTR pszName, int maxNameSize );

// Get address and port from a sockaddr structure
VXLAYER_API ULONG		vxsAddrGetAddr( VXSOCKADDR sa );
VXLAYER_API word		vxsAddrGetPort( VXSOCKADDR sa );

// Bind socket to an address
VXLAYER_API STATUS		vxsBind( VXSOCKET s, VXSOCKADDR sa );

// Perform a listen
STATUS		vxsListen( VXSOCKET s, int backlog );


// Accept from a listen
VXSOCKET	vxsAccept( VXSOCKET s, VXSOCKADDR sa );

// Connect
STATUS		vxsConnect( VXSOCKET s, VXSOCKADDR sa );

// Send a UDP frame
VXLAYER_API int			vxsSendTo( VXSOCKET s, LPSTR buf, int bufLen, int flags, VXSOCKADDR sa );

// Send on a stream
int			vxsSend( VXSOCKET s, LPSTR buf, int bufLen, int flags );

// Receive a datagram
VXLAYER_API int			vxsRecvFrom( VXSOCKET s, char* buf, int bufLen, int flags, VXSOCKADDR psa );

// Receive from TCP
int			vxsRecv( VXSOCKET s, char* buf, int bufLen, int flags );

// set socket options
VXLAYER_API int			vxsSetTosBits( VXSOCKET s, int tosBits );

// Select to wait on a read fd
VXLAYER_API int         vxsSelectRead( VXSOCKET s, struct timeval *timeout);


VXLAYER_API int vxsSelectAnyRead( VXSOCKET *sockets, int numSockets, struct timeval *timeout);

VXLAYER_API void vxsGetSelfIpAddr( char* szStr );

// Join the multicast group
VXLAYER_API
STATUS	vxsUpdateMulticast(int add,  VXSOCKET sock, ULONG multicastAddr, ULONG interfaceAddr);

// Join the multicast group
VXLAYER_API
STATUS	vxsJoinMulticast(VXSOCKET sock, ULONG multicastAddr, ULONG interfaceAddr);

// set multicast time-to-live value option
VXLAYER_API
int	vxsSetMulticastTTL( VXSOCKET sock, char byteOption );

// allow multiple sockets to use the same PORT number 
VXLAYER_API
int	vxsAllowUsingSamePort( VXSOCKET sock );

// set multicast loop socket option
VXLAYER_API
int	vxsSetMulticastLoop( VXSOCKET sock, char byteOption );

// Get the address and port of the socket
VXLAYER_API
VXSOCKADDR	vxsGetSockName( VXSOCKET sock );

VXLAYER_API
STATUS vxsGetSockAddressAndPort( VXSOCKET sock,  ULONG *inAddr, USHORT *inPort);

VXLAYER_API
STATUS getFirstLocalInterfaceAddress(ULONG *interfaceIpAddr, int *AddressCount);

VXLAYER_API
STATUS validInterfaceAddress(ULONG ipAddress, boolean *valid);

VXLAYER_API
int vxBindToInterface( VXSOCKET socket, const char *ifName, const int protocol );

VXLAYER_API
int vxSetPromicuous( VXSOCKET socket, const char *ifName );

#ifdef __cplusplus
}
#endif

#endif // _VXSOCKETS_H
