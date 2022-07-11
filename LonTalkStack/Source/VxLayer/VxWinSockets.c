/***************************************************************
 *  Filename: VxWinSockets.c
 *
 * Copyright © 1998-2022 Dialog Semiconductor
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
 *  Description:  Implementation of VxWorks portable socket layer.
 *					for Windows
 *
 *	DJ Duffy Oct 1998
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/VxLayer/VxWinSockets.c#5 $
//
/*
 * $Log: /Dev/VxLayer/VxWinSockets.c $
 * 
 * 21    11/09/06 8:28a Glen
 * Port should be unsigned.
 * 
 * 20    12/08/05 2:11p Fremont
 * Fix INADDR_xxx macro conflicts by replacing them with vxsINADDR_xxx
 * macros
 * 
 * 19    7/19/05 11:44p Fremont
 * Add vxsMakeDottedAddr() for a portable equivalent to inet_ntoa()
 * 
 * 18    7/05/05 5:30p Fremont
 * Expose hostGeyByName as an external API call
 * 
 * 17    4/07/04 4:38p Rjain
 * Added new VxLayer Dll. 
 * 
 * 16    3/25/03 11:36a Fremont
 * don't report read error when shutting down socket
 * 
 * 15    5/29/02 4:55p Fremont
 * Added some functions for EDC modules
 * 
 * 14    11/01/00 1:35p Bobw
 * Initialize the return value for send and fetch commands to ERROR, so
 * that an uninitialized value is not returned if the socket is closed.
 * 
 * 13    4/26/00 9:53a Darrelld
 * Add function to validate an ipAddress
 * 
 * 11    10/08/99 4:36p Fremont
 * Add select func, cleanup
 * 
 * 10    9/17/99 7:35p Bobw
 * Init win sockets in get host name
 * 
 * 9     9/08/99 3:12p Darrelld
 * Support for TOS bits - stub routine
 * 
 * 8     6/18/99 5:00p Darrelld
 * Cleanup warning
 * 
 * 7     6/18/99 2:48p Darrelld
 * vxsAddrDotted - do not do host lookup. Just use converted address
 * 
 * 6     6/18/99 1:14p Darrelld
 * All parameters are now in host order.
 * 
 * 5     6/14/99 4:36p Darrelld
 * Added vxsAddrGetAddr and vxsAddrGetPort
 * 
 * 4     11/20/98 8:58a Darrelld
 * Fix header file for tasks and add taskExit
 * 
 * 2     10/23/98 5:05p Darrelld
 * Enhancements and socket testing
 * 
 * 1     10/22/98 9:07a Darrelld
 * 
 * 
 */

#include	<windows.h>
//#include	<winsock2.h>
#include	<VxWorks.h>
#include	<VxLayer.h>
#include	<assert.h>

#include	"VxSockets.h"


// Same as VxWorks, but strongly typed.
//typedef int VXSOCKET;

//typedef struct _VXSOCKADDR* VXSOCKADDR;


//
// Only these are supported
//#define SOCK_STREAM     1               /* stream socket */
//#define SOCK_DGRAM      2               /* datagram socket */
//

//#define VXSOCK_STREAM	1
//#define	VXSOCK_DGRAM	2

//////////////////////////////////////////////////////////////////////////
// Local data definitions
//

//
// Socket control data structure
//
typedef struct _vxSock
{
	BOOL	bBusy;
	SOCKET	socket;
}	vxSock;


//
// Socket address structure
//
typedef struct _VXSOCKADDR
{
	union
	{
		SOCKADDR_IN		sad_in;
		SOCKADDR		sad;
	};
} VXSOCKADDRS;

//////////////////////////////////////////////////////////////////////////
// Local data structures
//

// Socket table
// Zero is not a valid socket number
vxSock	sockTable[VXMAXSOCKETS+1];

// socket control data
static BOOL					bSocketsInit;
static CRITICAL_SECTION		csSocketLock;

//////////////////////////////////////////////////////////////////////////
// Local routines
//

static	vxSock*	vxsSockFromIdx( int idx )
{
	if ( idx == INVALID_SOCKET )
	{	return NULL;
	}
	assert( idx != 0 );
	return &sockTable[idx];
}

static void		LockSockets()
{
	WSADATA		wsaData;
	WORD		vVersion = MAKEWORD( 2,0 );
	int			wsaSts;

	if ( !bSocketsInit )
	{
		// we assume this to be true
		//assert( INVALID_SOCKET == 0 );

		// Request version 2.0
		wsaSts = WSAStartup( vVersion, &wsaData );
		assert( wsaSts == 0 );

		// beg the question of WSACleanup for now.

		InitializeCriticalSection( &csSocketLock );
		bSocketsInit = TRUE;
	}

	EnterCriticalSection( &csSocketLock );
}

static void		UnlockSockets()
{

	LeaveCriticalSection( &csSocketLock );

}


//////////////////////////////////////////////////////////////////////////
// Global Routines
//

//
// vxsGetHostName
//
// Return name of local host
//
VXLAYER_API
STATUS	vxsGetHostName( LPSTR pszNameRtn, int nMaxLen )
{
	STATUS	sts = ERROR;

    // This assures that sockets are initialized.
    LockSockets();
    UnlockSockets();
	if ( 0 == gethostname( pszNameRtn, nMaxLen ) )
	{
		sts = OK;
	}
	return sts;
}

// Return IP addr of local host
int vxsGetLocalAddr(void)
{
	char name[100];
	int	host = ERROR;

	if (vxsGetHostName(name, 100) == OK)
	{
		host = hostGetByName(name);
	}
	return(host);
}

// Return IP addr of local host as a dotted string
STATUS vxsGetLocalAddrDotted(char *buf)
{
	struct in_addr inAddr;
	char *pStr;
	STATUS sts = ERROR;

	inAddr.s_addr = vxsGetLocalAddr();
	if (inAddr.s_addr != ERROR)
	{
		pStr = inet_ntoa(inAddr);
		strcpy(buf, pStr);
		sts = OK;
	}
	return(sts);
}

// Make a dotted string (x.x.x.x) from an address
// Buffer should be at least INET_ADDR_LEN bytes long
VXLAYER_API
void vxsMakeDottedAddr(char *buf, ULONG inAddr)
{
	char *pStr;
	struct in_addr addrStruct;

	addrStruct.s_addr = ntohl(inAddr);
	pStr = inet_ntoa(addrStruct);
	if (pStr != NULL)
	{	
		strcpy(buf, pStr);
	}
	else
	{	buf[0] = 0;
	}
}

//
// Get and free socket address structures
//
VXLAYER_API
VXSOCKADDR	vxsGetSockaddr()
{
	VXSOCKADDR		psad;

	psad = malloc( sizeof(VXSOCKADDRS) );
	// Woops, maybe we didn't get one
	if ( psad )
	{	memset( psad, 0, sizeof(VXSOCKADDRS) );
		psad->sad_in.sin_family = AF_INET;
	}
	return psad;
}

VXLAYER_API
void		vxsFreeSockaddr( VXSOCKADDR psad )
{
	// be defensive about being passed a NULL
	if ( psad )
	{	free( psad );
	}
}


// Create a socket, only supply a type
VXLAYER_API
VXSOCKET	vxsSocket( int type )
{
	int			i;
	VXSOCKET	sock = INVALID_SOCKET;
	vxSock*		pSock = NULL;

	LockSockets();
	for ( i=1; i< VXMAXSOCKETS; i++ )
	{
		if ( !sockTable[i].bBusy )
		{
			pSock = &sockTable[i];
			pSock->socket = socket( AF_INET, type, IPPROTO_UDP );
			if ( pSock->socket != INVALID_SOCKET )
			{
				sock = i;
				pSock->bBusy = TRUE;
			}
			break;
		}
	}
	UnlockSockets();
	
	return sock;
}

//
// Close an open socket
//
VXLAYER_API
STATUS		vxsCloseSocket( VXSOCKET sock )
{
	STATUS		sts = ERROR;
	int			ssts = 0;
	LockSockets();

	if ( !sockTable[ sock ].bBusy || sockTable[sock].socket != INVALID_SOCKET )
	{
		ssts = closesocket( sockTable[sock].socket );
		if ( ssts == 0 )
		{	sts = OK;
		}
	}
	sockTable[sock].bBusy = FALSE;
	sockTable[sock].socket = INVALID_SOCKET;

	UnlockSockets();
	if ( ssts )
	{	vxlReportErrorPrintf( "vxsCloseSocket - error %d 0x%08x\n", ssts, ssts );
	}
	return sts;
}

//
// shutdown an open socket
//
VXLAYER_API
STATUS		vxsShutdownSocket( VXSOCKET sock, int how )
{
    vxSock*		psock = vxsSockFromIdx(sock);
	int			wsts;
	STATUS		sts = ERROR;

	if ( psock && psock->bBusy )
    {
        wsts = shutdown( psock->socket, how );
    	if ( wsts )
		{
			wsts = WSAGetLastError();
			vxlReportErrorPrintf("vxsShutdownSocket - Error %d 0x%08x\n", wsts, wsts );
		}
		else
		{	sts = OK;
		}
	}
	return sts;
}


//
// vxsInetAddr
//
// return ULONG of an inet address from a dotted string
//
VXLAYER_API
ULONG		vxsInetAddr( LPSTR pszDottedStr )
{
	return ntohl( inet_addr( pszDottedStr ) );
}

//
// SocketAddr from dotted string
//
VXSOCKADDR	vxsAddrDotted( LPSTR pszDottedStr )
{
	VXSOCKADDR	psad = NULL;
	ULONG		inaddr;
	//HOSTENT*	pHostent;

	// Allow convenient error exits
	while (TRUE )
	{
		// Get binary form of dotted string
		inaddr = inet_addr( pszDottedStr );
		if ( inaddr == vxsINADDR_NONE )
		{	break;
		}
		psad = vxsGetSockaddr();
		if ( psad == NULL )
		{	break;
		}
#if 0	// don't do lookup of address before setting it up
		// look up the host structure for this address
		pHostent = gethostbyaddr( (LPCTSTR)&inaddr, sizeof(ULONG), AF_INET );
		if ( pHostent == NULL )
		{	break;
		}
		psad->sad_in.sin_addr.S_un.S_addr = *((ULONG*)pHostent->h_addr_list[0]);
#endif
		psad->sad_in.sin_addr.S_un.S_addr = inaddr;
		psad->sad_in.sin_family = AF_INET;
		// port remains to be filled in
	
		break;
	}

	return psad;
}

// This will work for IPV4 not IPV6
// inaddr in host order
VXLAYER_API
VXSOCKADDR	vxsAddrValue( ULONG inaddr )
{
	VXSOCKADDR	psad = NULL;
	//HOSTENT*	pHostent;

	// Allow convenient error exits
	while (TRUE )
	{
#if 0
		// look up the host structure for this address
		pHostent = gethostbyaddr( (LPCTSTR)&inaddr, sizeof(ULONG), AF_INET );
		if ( pHostent == NULL )
		{	break;
		}
#endif
		psad = vxsGetSockaddr();
		if ( psad == NULL )
		{	break;
		}
		//psad->sad_in.sin_addr.S_un.S_addr = *((ULONG*)pHostent->h_addr_list[0]);
		psad->sad_in.sin_addr.S_un.S_addr = htonl(inaddr);
		psad->sad_in.sin_family = AF_INET;
		// port remains to be filled in
	
		break;
	}

	return psad;
}

// SocketAddr from name string
// Uses gethostbyname
VXLAYER_API
VXSOCKADDR	vxsAddrName( LPSTR pszName )
{
	VXSOCKADDR	psad = NULL;
	HOSTENT*	pHostent;

	// Allow convenient error exits
	do
	{
		// look up the host structure for this address
		pHostent = gethostbyname( pszName );
		if ( pHostent == NULL )
		{	break;
		}
		psad = vxsGetSockaddr();
		if ( psad == NULL )
		{	break;
		}
		psad->sad_in.sin_addr.S_un.S_addr = *((ULONG*)pHostent->h_addr_list[0]);
		psad->sad_in.sin_family = AF_INET;
		// port remains to be filled in
	} while (FALSE);

	return psad;

}

// VxWorks interface replacement
VXLAYER_API int hostGetByName(char *name)
{
	HOSTENT*	pHostent;
	int host = ERROR;

	pHostent = gethostbyname(name);
	if (pHostent != NULL)
	{
		host = *((int*)pHostent->h_addr_list[0]);
	}
	return(host);
}

// Validate an IP address for a name string
// Uses gethostbyname
// return the address based on the index
VXLAYER_API
boolean	vxsValidateIPAddress( LPSTR pszName, ULONG ipAddr )
{
	HOSTENT*	pHostent;
	int			i;
	boolean		bFoundIt = FALSE;
	ULONG		ipHost;

	// Allow convenient error exits
	do
	{
		// look up the host structure for this address
		pHostent = gethostbyname( pszName );
		if ( pHostent == NULL )
		{	break;
		}
		for ( i=0; i<257; i++ )
		{	/* null terminated list
			 */
			if ( pHostent->h_addr_list[i] == 0 )
			{	break;
			}
			ipHost = ntohl(*((ULONG*)pHostent->h_addr_list[i]));
			if ( ipAddr == ipHost )
			{	bFoundIt = TRUE;
				break;
			}
		}
	} while (FALSE );

	return bFoundIt;

}

// set the port number in a sockaddr structure
VXLAYER_API
void		vxsSetPort( VXSOCKADDR psad, unsigned short port )
{
	psad->sad_in.sin_port = htons(port);
}

// Get address and port from a sockaddr structure
VXLAYER_API
ULONG		vxsAddrGetAddr( VXSOCKADDR psad )
{
	return ntohl(psad->sad_in.sin_addr.S_un.S_addr);
}

VXLAYER_API
word		vxsAddrGetPort( VXSOCKADDR psad )
{
	return ntohs(psad->sad_in.sin_port);
}

// Get name from a socket address
// Uses hostGetByAddr
// maxNameSize must be MAXHOSTNAMELEN + 1 or larger
// DON"T ASSUME THIS VALUE, VXWORKS.H RULES
//#ifndef MAXHOSTNAMELEN
//#define MAXHOSTNAMELEN 16
//#endif // MAXHOSTNAMELEN

STATUS		vxsAddrGetName( VXSOCKADDR psa, LPSTR pszName, int maxNameSize )
{
	STATUS		sts = ERROR;
	VXSOCKADDR	psad = NULL;
	ULONG		inaddr = vxsINADDR_NONE;
	HOSTENT*	pHostent;


	// Allow convenient error exits
	while (TRUE )
	{
		// Get the address from the sockaddr passed in.
		inaddr = psa->sad_in.sin_addr.S_un.S_addr;
		// look up the host structure for this address
		pHostent = gethostbyaddr( (LPCTSTR)&inaddr, sizeof(ULONG), AF_INET );
		if ( pHostent == NULL )
		{	break;
		}

		if ( maxNameSize < MAXHOSTNAMELEN )
		{	break;
		}
		// Let's not get too defensive in the interests of getting
		// on with it.
		assert( strlen(pHostent->h_name) <= (size_t)maxNameSize );
		strcpy( pszName, pHostent->h_name );
		sts = OK;	
		break;
	}

	return sts;
}

// Bind socket to an address
VXLAYER_API
STATUS		vxsBind( VXSOCKET s, VXSOCKADDR sa )
{
	vxSock*		psock = vxsSockFromIdx(s);
	int			wsts;
	STATUS		sts = ERROR;

	if ( psock && psock->bBusy )
	{

		wsts = bind( psock->socket, &sa->sad, sizeof(sa->sad) );
		if ( wsts )
		{
			wsts = WSAGetLastError();
			vxlReportErrorPrintf("vxsBind - Error %d 0x%08x\n", wsts, wsts );
		}
		else
		{	sts = OK;
		}
	}
	return sts;
}

// Perform a listen
STATUS		vxsListen( VXSOCKET s, int backlog )
{
	int			wsts;
	STATUS		sts = ERROR;
	vxSock*		psock = vxsSockFromIdx(s);

	if ( psock && psock->bBusy )
	{
		wsts = listen( psock->socket, backlog );
		if ( wsts == 0 )
		{	sts = OK;
		}
		else
		{
			//wsts = WSAGetLastError();
			vxlReportErrorPrintf("vxsListen - Error %d 0x%08x\n", wsts, wsts );
		}
	}
	return sts;
}


// Accept from a listen
VXSOCKET	vxsAccept( VXSOCKET s, VXSOCKADDR sa )
{
	int			wsts;
	STATUS		sts = ERROR;
	SOCKET		acs = INVALID_SOCKET;
	vxSock*		psock = vxsSockFromIdx(s);
	int			addrLen = sizeof(struct sockaddr);

	if ( psock && psock->bBusy )
	{
		acs = accept( psock->socket, &sa->sad, &addrLen );
		if ( acs != INVALID_SOCKET )
		{	sts = OK;
		}
		else
		{
			wsts = WSAGetLastError();
			vxlReportErrorPrintf("vxsAccept - Error %d 0x%08x\n", wsts, wsts );
		}
	}
	return sts;
}

// Connect
STATUS		vxsConnect( VXSOCKET s, VXSOCKADDR sa )
{
	int			wsts;
	STATUS		sts = ERROR;
	vxSock*		psock = vxsSockFromIdx(s);
	int			addrLen = sizeof(struct sockaddr);

	if ( psock && psock->bBusy )
	{
		wsts = connect( psock->socket, &sa->sad, addrLen );
		if ( wsts == 0 )
		{	sts = OK;
		}
		else
		{
			wsts = WSAGetLastError();
			vxlReportErrorPrintf("vxsConnect - Error %d 0x%08x\n", wsts, wsts );
		}
	}

	return sts;
}

// Send a UDP frame
VXLAYER_API
int			vxsSendTo( VXSOCKET s, LPSTR buf, int bufLen, int flags, VXSOCKADDR sa )
{
	int			wsts;
	vxSock*		psock = vxsSockFromIdx(s);
	int			addrLen = sizeof(struct sockaddr);
	int			bytes = ERROR;

	if ( psock && psock->bBusy )
	{
		bytes = sendto( psock->socket, buf, bufLen, flags, &sa->sad, addrLen );
		if ( bytes == SOCKET_ERROR )
		{
			bytes = ERROR;
			wsts = WSAGetLastError();
			vxlReportErrorPrintf("vxsSendTo - Error %d 0x%08x\n", wsts, wsts );
		}
	}
	return bytes;
}

// Send on a stream
int			vxsSend( VXSOCKET s, LPSTR buf, int bufLen, int flags )
{
	int			wsts;
	vxSock*		psock = vxsSockFromIdx(s);
	int			addrLen = sizeof(struct sockaddr);
	int			bytes = ERROR;

	if ( psock && psock->bBusy )
	{
		bytes = send( psock->socket, buf, bufLen, flags );
		if ( bytes == SOCKET_ERROR )
		{
			bytes = ERROR;
			wsts = WSAGetLastError();
			vxlReportErrorPrintf("vxsSend - Error %d 0x%08x\n", wsts, wsts );
		}
	}
	return bytes;
}

// Receive a datagram
VXLAYER_API
int			vxsRecvFrom( VXSOCKET s, char* buf, int bufLen, int flags, VXSOCKADDR psa )
{
	int			wsts;
	vxSock*		psock = vxsSockFromIdx(s);
	int			addrLen = sizeof(struct sockaddr);
	int			bytes = ERROR;

	if ( psock && psock->bBusy )
	{
		bytes = recvfrom( psock->socket, buf, bufLen, flags, &psa->sad, &addrLen );
		if ( bytes == SOCKET_ERROR )
		{
			bytes = ERROR;
			// If the socket is being closed, wait for that to complete,
			// and don't report the error.
			// This could also be done in other routines in this file.
			LockSockets();
			UnlockSockets();
			if (psock->bBusy)
			{
				wsts = WSAGetLastError();
				vxlReportErrorPrintf("vxsRecvFrom - Error %d 0x%08x\n", wsts, wsts );
			}
		}
	}

	return bytes;
}

// Receive from stream
int			vxsRecv( VXSOCKET s, char* buf, int bufLen, int flags )
{
	int			wsts;
	vxSock*		psock = vxsSockFromIdx(s);
	int			addrLen = sizeof(struct sockaddr);
	int			bytes = ERROR;

	if ( psock && psock->bBusy )
	{
		bytes = recv( psock->socket, buf, bufLen, flags );
		if ( bytes == SOCKET_ERROR )
		{
			bytes = ERROR;
			wsts = WSAGetLastError();
			vxlReportErrorPrintf("vxsRecv - Error %d 0x%08x\n", wsts, wsts );
		}
	}
	return bytes;
}

// set socket options
VXLAYER_API
int			vxsSetTosBits( VXSOCKET s, int tosBits )
{	// always succeed
	return 0;
}


// Select to wait on a read fd for any of several sockets
VXLAYER_API
int         vxsSelectAnyRead( VXSOCKET *sockets, int numSockets, struct timeval *timeout)
{
    int i;
    int numSet = 0;
	int         result = ERROR;
    fd_set      readFds;

    FD_ZERO (&readFds);
    for (i = 0; i < numSockets; i++)
    {        
	    vxSock*		psock = vxsSockFromIdx(*sockets++);

	    if ( psock && psock->bBusy )
	    {
            FD_SET (psock->socket, &readFds);
            numSet++;
        }
    }
    if (numSet)
    {
        result = select(FD_SETSIZE, &readFds, NULL, NULL, timeout);
		if ( result == SOCKET_ERROR )
		{
			result = ERROR;
        }
    }

    return result;
}

// Select to wait on a read fd
VXLAYER_API
int         vxsSelectRead( VXSOCKET s, struct timeval *timeout)
{
    return vxsSelectAnyRead(&s, 1, timeout);
}

// Join or drop the multicast group
VXLAYER_API
STATUS	vxsUpdateMulticast(int add, VXSOCKET sock, ULONG multicastAddr, ULONG interfaceAddr)
{
	STATUS		sts = ERROR;
	vxSock*		psock = vxsSockFromIdx(sock);
    int wsts;

    if ( psock && psock->bBusy )
	{
        // Special self-installed multicast channel -- enable multicast on this socket
	    struct ip_mreq ipMreq;
	    int result;

	    // Join the multicast group
        ipMreq.imr_multiaddr.s_addr = htonl(multicastAddr);  // IP multicast group address 
	    ipMreq.imr_interface.s_addr = htonl(interfaceAddr);  // IP address of local interface
        result = setsockopt(psock->socket, IPPROTO_IP, add ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP, (char *)&ipMreq,
				sizeof(ipMreq));
        if (result >= 0)
        {
#if 0
            if (add)
            {
                // If we disable the loopback option, the other app running in the same system will not receive any message
                // from us.  So leave it as loopback = enabled 
                char byteOption = 0;
	            // Don't loopback outbound multicast messages (default is to loopback)
                result = vxsSetMulticastLoop(sock, byteOption );
            }
#endif
        }
        else
        {
			wsts = WSAGetLastError();
			vxlReportErrorPrintf("vxsJoinMulticast - Error %d 0x%08x\n", wsts, wsts );
        }
        if (result >= 0)
            sts = OK;
    }
    return sts;
}

// Join the multicast group
VXLAYER_API
STATUS	vxsJoinMulticast(VXSOCKET sock, ULONG multicastAddr, ULONG interfaceAddr)
{
    return vxsUpdateMulticast(true, sock, multicastAddr, interfaceAddr);
}

// set multicast time-to-live value option
int	vxsSetMulticastTTL( VXSOCKET sock, char byteOption )
{
    vxSock*		psock = vxsSockFromIdx(sock);
    int wsts;
    int result = -1;

  	if ( psock && psock->bBusy )
	{
        // Sets the time-to-live value of outgoing multicast packets for this socket.  
        // It is very important for multicast packets to set the smallest TTL possible.
        // The default is 1 which means that multicast packets don't leave the local
        // network unless the user program explicitly requests it.
        result = setsockopt(psock->socket, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&byteOption, sizeof(byteOption));
        if (result < 0)
        {
            wsts = WSAGetLastError();
		    vxlReportErrorPrintf("vxsSetMulticastTTL - Error %d 0x%08x\n", wsts, wsts );
        }
    }
    return result;
}

// set multicast loop socket option
int	vxsSetMulticastLoop( VXSOCKET sock, char byteOption )
{
    vxSock*		psock = vxsSockFromIdx(sock);
    int wsts;
    int result = -1;

  	if ( psock && psock->bBusy )
    {
        // According to the documentation, this call should take an 'int' option.
        // However, it only accepts a single byte option.

	    result = setsockopt(psock->socket, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&byteOption, sizeof(byteOption));
        if (result < 0)
        {
            wsts = WSAGetLastError();
		    vxlReportErrorPrintf("vxsSetMulticastLoop - Error %d 0x%08x\n", wsts, wsts );
        }
    }
	return result; 
}

// allow multiple sockets to use the same PORT number 
int	vxsAllowUsingSamePort( VXSOCKET sock )
{
    vxSock*		psock = vxsSockFromIdx(sock);
    int wsts;
    int result = -1;

  	if ( psock && psock->bBusy )
    {
        int nOption = 1;
	    result = setsockopt(psock->socket, SOL_SOCKET, SO_REUSEADDR, (char*)&nOption, sizeof(nOption));
        if (result < 0)
        {
            wsts = WSAGetLastError();
		    vxlReportErrorPrintf("vxsAllowUsingSamePort - Error %d 0x%08x\n", wsts, wsts );
        }
    }
	return result; 
}

// Get the current name for the specified socket
VXLAYER_API
VXSOCKADDR	vxsGetSockName( VXSOCKET sock )
{
	vxSock*		psock = vxsSockFromIdx(sock);
	VXSOCKADDR	psad = NULL;
	int			wsts;

    if ( psock && psock->bBusy )
    {
        psad = vxsGetSockaddr();
	    if ( psad )
	    {
            // obtain the address/port of the socket
            int addrLen=sizeof(psad->sad_in);   // psad->sad_in);
            memset(&psad->sad_in,0,addrLen);
            if (getsockname(psock->socket, (struct sockaddr *)&psad->sad_in, &addrLen) < 0)
            {
	    	    wsts = WSAGetLastError();
    			vxlReportErrorPrintf("vxsGetSockName - Error %d 0x%08x\n", wsts, wsts );
                vxsFreeSockaddr(psad);
                psad = NULL;
            }
        }
    }
    return psad;
}

// Get the address and port number assigned to the socket
VXLAYER_API
STATUS vxsGetSockAddressAndPort( VXSOCKET sock,  ULONG *inAddr, USHORT *inPort)
{
    STATUS		sts = ERROR;
    vxSock*		psock = vxsSockFromIdx(sock);
    VXSOCKADDR	psad = NULL;

    if ( psock && psock->bBusy )
    {
        psad = vxsGetSockName(sock);
        if (psad)
        {
            *inPort = ntohs(psad->sad_in.sin_port);
            *inAddr = psad->sad_in.sin_addr.S_un.S_addr;
            vxsFreeSockaddr( psad );
            sts = OK;
        }
        else
        {
            *inPort = 0;
            *inAddr = 0;
        }
    }
    return sts;
}

// end

