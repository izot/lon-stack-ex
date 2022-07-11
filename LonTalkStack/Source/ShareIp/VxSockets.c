/***************************************************************
 *  Filename: VxSockets.c
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
 *  Description:  Implementation of VxWorks portable socket layer.
 *
 *	Fremont B. - ported from VxWinSockets.c May 1999
 *
 ****************************************************************/

/*
 * $Log: /Dev/ShareIp/VxSockets.c $
 *
 * 12    11/09/06 11:45a Fremont
 * change port type to unsigned short in vxsSetPort
 *
 * 11    8/01/06 4:26p Fremont
 * fix warnings
 *
 * 10    12/08/05 2:11p Fremont
 * Fix INADDR_xxx macro conflicts by replacing them with vxsINADDR_xxx
 * macros
 *
 * 9     7/19/05 11:44p Fremont
 * Add vxsMakeDottedAddr() for a portable equivalent to inet_ntoa()
 *
 * 8     6/13/02 9:47a Fremont
 * Add local addr funcs
 *
 * 6     10/08/99 5:14p Fremont
 * add select function
 *
 * 5     9/08/99 6:09p Fremont
 * copy setsocketopt defs from Cisco include file
 *
 * 4     9/08/99 3:11p Darrelld
 * Support for TOS bits
 *
 * 3     7/02/99 8:45a Darrelld
 * Target specific files
 *
 * 2     6/07/99 5:44p Fremont
 * Remove vxlayer.h
 *
 * 1     6/07/99 5:21p Fremont
 * Initial port
 *
 */

#include	<vxWorks.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#ifdef linux
#include    <sys/socket.h>
#include    <errno.h>
#include    <netdb.h>
#include 	<unistd.h>
#include 	<netinet/in.h>
#include 	<arpa/inet.h>
#include 	<net/if.h>
#include 	<sys/ioctl.h>
#include 	<net/if_arp.h>
#include 	<sys/types.h>
#include 	<ifaddrs.h>
#include    <netpacket/packet.h>
#include    <net/ethernet.h>
#else
#include    <socket.h>
#include	<netinet\in.h>
#include	<sockLib.h>
#include	<hostLib.h>
#include	<inetLib.h>
#include	<ioLib.h>
#include	<selectLib.h>
#endif

#include 	<signal.h>
#include	<assert.h>

#include	"VxSockets.h"


//////////////////////////////////////////////////////////////////////////
// Local data definitions
//


//////////////////////////////////////////////////////////////////////////
// Local data structures
//

// Socket address structure
//
typedef struct sockaddr_in SOCKADDR_IN;
#ifdef linux
typedef struct sockaddr SOCKADDR;
#endif
typedef struct _VXSOCKADDR
{
	union
	{
		SOCKADDR_IN		sad_in;
		SOCKADDR		sad;
    } U;
} VXSOCKADDRS;



//////////////////////////////////////////////////////////////////////////
// Local routines
//


//////////////////////////////////////////////////////////////////////////
// Global Routines
//
#ifdef linux
int hostGetByName(char *name)
{
	int inaddr = ERROR;
	struct hostent *pHostent = gethostbyname(name);
	if (pHostent)
		inaddr = *((ULONG*)pHostent->h_addr_list[0]);
	return inaddr;
}
#endif

//
// vxsGetHostName
//
// Return name of local host
//
STATUS	vxsGetHostName( LPSTR pszNameRtn, int nMaxLen )
{
	return gethostname( pszNameRtn, nMaxLen );
}

// Return IP addr of local host
int vxsGetLocalAddr(void)
{
	char name[100];
	int	host = ERROR;

	if (vxsGetHostName(name, 100) == OK)
	{
#if linux
		struct hostent *pHostent = gethostbyname(name);
		if (pHostent)
			host = *((ULONG*)pHostent->h_addr_list[0]);
#else
		host = hostGetByName(name);
#endif
	}
	return(host);
}

// Return IP addr of local host as a dotted string
STATUS vxsGetLocalAddrDotted(char *buf)
{
	struct in_addr inAddr;
	STATUS sts = ERROR;

	inAddr.s_addr = vxsGetLocalAddr();
	if (inAddr.s_addr != ERROR)
	{
#ifdef linux
		inet_ntop(AF_INET, &inAddr, buf, INET_ADDRSTRLEN);
#else
		inet_ntoa_b(inAddr, buf);
#endif
		sts = OK;
	}
	return(sts);
}


// Make a dotted notation (x.x.x.x) string from an address
// Buffer should be at least INET_ADDR_LEN bytes long
void vxsMakeDottedAddr(char *buf, ULONG inAddr)
{
	struct in_addr addrStruct;

	addrStruct.s_addr = ntohl(inAddr);  // need to convert the address from network byte order to host byte order

#ifdef linux
		inet_ntop(AF_INET, &addrStruct, buf, INET_ADDRSTRLEN);
#else
		inet_ntoa_b(addrStruct, buf);
#endif
}

//
// Get and free socket address structures
//
VXSOCKADDR	vxsGetSockaddr()
{
	VXSOCKADDR		psad;

	psad = malloc( sizeof(VXSOCKADDRS) );
	// Woops, maybe we didn't get one
	if ( psad != NULL)
	{	memset( psad, 0, sizeof(VXSOCKADDRS) );
		psad->U.sad_in.sin_family = AF_INET;
        // The address remains to be filled in
	}
	return psad;
}

void		vxsFreeSockaddr( VXSOCKADDR psad )
{
	// be defensive about being passed a NULL
	if ( psad != NULL)
	{
        free( psad );
	}
}

// Get address and port from a sockaddr structure
ULONG		vxsAddrGetAddr( VXSOCKADDR psad )
{
	return ntohl(psad->U.sad_in.sin_addr.s_addr);
}

word		vxsAddrGetPort( VXSOCKADDR psad )
{
	return ntohs(psad->U.sad_in.sin_port );
}

static VXSOCKET createSocket(int domain, int type, int protocol) {
	VXSOCKET	sock = 0;

	sock = socket( domain, type, protocol );
    if (sock == ERROR)
    {
        sock = INVALID_SOCKET;
    }

	return sock;
}

// Create a socket, only supply a type
VXSOCKET	vxsSocket( int type )
{
	VXSOCKET	sock = 0;

	sock = socket( AF_INET, type, IPPROTO_UDP );

    if (sock == ERROR)
    {
        sock = INVALID_SOCKET;
    }

	return sock;
}
#if 0
VXSOCKET vxsRawSocket(int type) {
#ifdef linux
	return createSocket(AF_PACKET, type, htons(ETH_P_ALL));
#else
	return vxsSocket(type);
#endif
}
#endif
VXSOCKET vxsRawSocket(int type, int proto) {
#ifdef linux
	return createSocket(AF_PACKET, type, htons(proto));
#else
	return vxsSocket(type);
#endif
}

//
// Close an open socket
//
STATUS		vxsShutdownSocket( VXSOCKET sock, int how )
{
	return shutdown( sock, how );
}

//
// Close an open socket
//
STATUS		vxsCloseSocket( VXSOCKET sock )
{
	return close( sock );
}

//
// vxsInetAddr
//
// return ULONG of an inet address from a dotted string
//
ULONG		vxsInetAddr( LPSTR pszDottedStr )
{
	return ntohl(inet_addr( pszDottedStr ));
}


//
// SocketAddr from dotted string
//
VXSOCKADDR	vxsAddrDotted( const char* pszDottedStr )
{
	VXSOCKADDR	psad = NULL;
	ULONG		inaddr;

	// Get binary form of dotted string
	inaddr = inet_addr( pszDottedStr );
	if ( inaddr != vxsINADDR_NONE )
	{
	    psad = vxsGetSockaddr();
	    if ( psad != NULL )
	    {
	        psad->U.sad_in.sin_addr.s_addr = inaddr;
	        psad->U.sad_in.sin_family = AF_INET;
	        // port remains to be filled in
	    }
	}

	return psad;
}

// This will work for IPV4 not IPV6
VXSOCKADDR	vxsAddrValue( ULONG inaddr )
{
	VXSOCKADDR psad;

	psad = vxsGetSockaddr();
	if ( psad != NULL )
	{
		psad->U.sad_in.sin_addr.s_addr = htonl(inaddr);
		psad->U.sad_in.sin_family = AF_INET;
		// port remains to be filled in
	}

	return psad;
}

// SocketAddr from name string
// Uses gethostbyname
VXSOCKADDR	vxsAddrName( LPSTR pszName )
{
	VXSOCKADDR	psad = NULL;
    int         inaddr = ERROR;

	inaddr = hostGetByName(pszName);

	if (inaddr != ERROR)
    {
		psad = vxsGetSockaddr();
		if ( psad != NULL )
		{
		    psad->U.sad_in.sin_addr.s_addr = (ULONG)inaddr;
		    psad->U.sad_in.sin_family = AF_INET;
		    // port remains to be filled in
		}
	}

	return psad;
}

// set the port number in a sockaddr structure
void		vxsSetPort( VXSOCKADDR psad, unsigned short port )
{

	psad->U.sad_in.sin_port = htons(port);
}


// Get name from a socket address

// maxNameSize must be MAXHOSTNAMELEN + 1 or larger
// DON"T ASSUME THIS VALUE, VXWORKS.H RULES
//#ifndef MAXHOSTNAMELEN
//#define MAXHOSTNAMELEN 16
//#endif // MAXHOSTNAMELEN

STATUS		vxsAddrGetName( VXSOCKADDR psad, LPSTR pszName, int maxNameSize )
{
	STATUS      sts = ERROR;

	if ( maxNameSize > MAXHOSTNAMELEN )
    {
		// Get the address from the sockaddr passed in.
#ifdef linux
		struct hostent *host = gethostbyaddr((char *)&psad->U.sad_in.sin_addr.s_addr, 4, AF_INET);
		strncpy(pszName, host->h_name, maxNameSize - 1);
		pszName[maxNameSize - 1] = 0;
#else
	    int  		inaddr;
		inaddr = (int)psad->U.sad_in.sin_addr.s_addr;
        sts = hostGetByAddr( inaddr, pszName );
#endif
    }

	return sts;
}


// Bind socket to an address
STATUS		vxsBind( VXSOCKET sock, VXSOCKADDR psad )
{
	return bind( sock, &psad->U.sad, sizeof(psad->U.sad) );
}

// Perform a listen
STATUS		vxsListen( VXSOCKET sock, int backlog )
{
	return listen( sock, backlog );
}


// Accept from a listen
VXSOCKET	vxsAccept( VXSOCKET sock, VXSOCKADDR psad )
{
#ifdef linux
	socklen_t	addrLen = sizeof(psad->U.sad);
#else
	int	addrLen = sizeof(psad->U.sad);
#endif
	return accept( sock, &psad->U.sad, &addrLen );
}

// Connect
STATUS		vxsConnect( VXSOCKET sock, VXSOCKADDR psad )
{
#ifdef linux
	socklen_t	addrLen = sizeof(psad->U.sad);
#else
	int	addrLen = sizeof(psad->U.sad);
#endif

	return connect( sock, &psad->U.sad, addrLen );
}

// Send a UDP frame
int			vxsSendTo( VXSOCKET sock, LPSTR buf, int bufLen, int flags, VXSOCKADDR psad )
{
#ifdef linux
	socklen_t	addrLen = sizeof(psad->U.sad);
#else
	int	addrLen = sizeof(psad->U.sad);
#endif

	return sendto( sock, buf, bufLen, flags, &psad->U.sad, addrLen );
}

// Send on a stream
int			vxsSend( VXSOCKET sock, LPSTR buf, int bufLen, int flags )
{
	return send( sock, buf, bufLen, flags );
}

// Receive a datagram
int			vxsRecvFrom( VXSOCKET sock, char* buf, int bufLen, int flags, VXSOCKADDR psad )
{
#ifdef linux
	socklen_t	addrLen = sizeof(psad->U.sad);
#else
	int	addrLen = sizeof(psad->U.sad);
#endif

	return recvfrom( sock, buf, bufLen, flags, &psad->U.sad, &addrLen );
}

// Receive from stream
int			vxsRecv( VXSOCKET sock, char* buf, int bufLen, int flags )
{
	return recv( sock, buf, bufLen, flags );
}

/* Definitions from Cisco's include file in.h */
#undef IPPROTO_IP
#undef IP_TOS
#define	IPPROTO_IP		0
#define	IP_TOS			3
// set socket options
int			vxsSetTosBits( VXSOCKET sock, int tosBits )
{
	return setsockopt( sock, IPPROTO_IP, IP_TOS, (char*)&tosBits, sizeof(int) );
}


// Select to wait on a read fd
int         vxsSelectRead( VXSOCKET sock, struct timeval *timeout)
{
	return vxsSelectAnyRead(&sock, 1, timeout);
}

// Select to wait on a read fd for any of several sockets
int         vxsSelectAnyRead( VXSOCKET *sockets, int numSockets, struct timeval *timeout)
{
    int i;
    int numSet = 0;
	int         result = -1;
    fd_set      readFds;
    VXSOCKET    fdMax = -1;

    FD_ZERO (&readFds);
    for (i = 0; i < numSockets; i++)
    {
    	VXSOCKET sock = *sockets++;

    	if (sock != INVALID_SOCKET)
    	{
    		FD_SET (sock, &readFds);
     		numSet++;
     		// highest file descriptor number, need it for the select function
            if (sock > fdMax)
            	fdMax = sock;
    	}
    }
    if (numSet)
    {
        result = select(fdMax+1, &readFds, NULL, NULL, timeout);
		if ( result == -1 )
		{
			// Commented out the following line.  It always happens on exit
			// perror("Select to wait on a read");
        }
    }

    return result;
}

// returns errno type value, 0 on success
int GetInterfaceInfo(int sockfd, char *name, int *outMtu, int *outMetric)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));

	if( outMetric )
	{
		if (ioctl(sockfd, SIOCGIFMETRIC, (caddr_t)&ifr) < 0)
			return errno;

		*outMetric = ifr.ifr_metric;
	}

	if( outMtu )
	{
		if (ioctl(sockfd, SIOCGIFMTU, (caddr_t)&ifr) < 0)
			return errno;

		*outMtu = ifr.ifr_mtu;
	}

	return 0;
}

int vxBindToInterface( VXSOCKET socket, const char* ifname, const int protocol ) {
	struct ifreq ifr = { 0 };
	size_t ifnamelen = strlen(ifname);
	memcpy(ifr.ifr_name, ifname, ifnamelen);
	if (ioctl(socket, SIOCGIFINDEX, &ifr) < 0) {
		perror("ioctl");
		return errno;
	}

	struct sockaddr_ll addr = { 0 };
	addr.sll_family = AF_PACKET;
	addr.sll_ifindex = ifr.ifr_ifindex;
	if (protocol)
		addr.sll_protocol = protocol;

	if (bind(socket, (struct sockaddr*) &addr, sizeof(struct sockaddr_ll)) < 0) {
		return errno;
	}
	return 0;
}

int vxSetPromicuous( VXSOCKET socket, const char* ifname ) {
	struct ifreq ifr = { 0 };
	size_t ifnamelen = strlen(ifname);
	memcpy(ifr.ifr_name, ifname, ifnamelen);
	if (ioctl(socket, SIOCGIFINDEX, &ifr) < 0) {
		return errno;
	}

    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifr.ifr_ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    if (setsockopt(socket, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0) {
        perror("setsockopt");
    }
	return 0;
}

void vxsGetSelfIpAddr( char* szStr )
{
	struct ifreq *ifrp;
	int sockfd;
	int total, remaining, current;
	struct ifconf ifc;
	struct sockaddr_in *addr;
	char buf[sizeof(struct ifreq)*32];
	int err;
	int metric,mtu;

	// default if we get nothing
	strcpy(szStr, "127.0.0.1");

	// we need a socket to call ioctl() on
	sockfd = socket(PF_INET,SOCK_DGRAM,0);
	if( sockfd < 0 ) {
		perror("cannot create UDP socket");
		return;
	}

	ifc.ifc_len = sizeof( buf );
	ifc.ifc_buf = (caddr_t)buf;

	if (ioctl(sockfd, SIOCGIFCONF, (caddr_t)&ifc) < 0) {
    	perror("ioctl (SIOCGIFCONF)");
		return;
	}

	remaining = total = ifc.ifc_len;
	ifrp = ifc.ifc_req;
	while( remaining )
	{
		if( ifrp->ifr_addr.sa_family == AF_INET )
		{
			err = GetInterfaceInfo(sockfd, ifrp->ifr_name,&mtu,&metric);
			if( err )
			{
				errno = err;
				perror("ioctl");
				continue;
			}
			addr = (struct sockaddr_in *)&(ifrp->ifr_addr);
			if (strcmp(szStr, "127.0.0.1") != 0)
			{
				strcpy(szStr, inet_ntoa(addr->sin_addr));
				break;
			}
		}

		current = sizeof(ifrp->ifr_addr) + IFNAMSIZ;
		ifrp = (struct ifreq *)( ((char *)ifrp)+current );
		remaining -= current;
	}

	close(sockfd);
}

// set multicast loop socket option
int	vxsSetMulticastLoop( VXSOCKET sock, char byteOption )
{
    // According to the documentation, this call should take an 'int' option.
    // However, it only accepts a single byte option.
    int result = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&byteOption, sizeof(byteOption));
    if (result < 0)
        perror("cannot set multicast loop socket option");
    return result; 
}

// set multicast time-to-live value option
int	vxsSetMulticastTTL( VXSOCKET sock, char byteOption )
{
    // According to the documentation, this call should take an 'int' option.
    // However, it only accepts a single byte option.

    // Sets the time-to-live value of outgoing multicast packets for this socket.  
    // It is very important for multicast packets to set the smallest TTL possible.
    // The default is 1 which means that multicast packets don't leave the local
    // network unless the user program explicitly requests it.
    int result = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&byteOption, sizeof(byteOption));
    if (result < 0)
        perror("cannot set multicast TTL soeckt option");
    return result; 
}

// allow multiple sockets to use the same PORT number 
int	vxsAllowUsingSamePort( VXSOCKET sock )
{
    int nOption = 1;
    int result = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&nOption, sizeof(nOption));
    if (result < 0)
        perror("cannot set option to allow multiple sockets to use the same PORT number");
    return result; 
}

// Join the multicast group
int vxsJoinMulticast( VXSOCKET sock, ULONG multicastAddr, ULONG interfaceAddr)
{
	return vxsUpdateMulticast(true, sock, multicastAddr, interfaceAddr);
}

// Join or drop the multicast group
int vxsUpdateMulticast(int add, VXSOCKET sock, ULONG multicastAddr, ULONG interfaceAddr)
{
    // Special self-installed multicast channel -- enable multicast on this socket
    struct ip_mreq ipMreq;
    int result;

    // Join the multicast group
    ipMreq.imr_multiaddr.s_addr = htonl(multicastAddr);  // IP multicast group address 
    ipMreq.imr_interface.s_addr = htonl(interfaceAddr);  // IP address of local interface
    if (add)
        // Set the option so that all multicast traffic generated in this socket will be output from the interface chosen
        // This is needed otherwise it will fail to join the multicast group if using static IP network interface
        setsockopt(sock,IPPROTO_IP, IP_MULTICAST_IF, &ipMreq.imr_interface, sizeof(struct in_addr));
    result = setsockopt(sock, IPPROTO_IP, add ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP,
    		(char *)&ipMreq, sizeof(ipMreq));

 #if 0
    // If we disable the loopback option, all other apps running in the same system will not receive any message
    // from us.  So leave it as loopback = enabled 
    if (result >= 0)
    {
        // Don't loopback outbound multicast messages (default is to loopback)
        char byteOption = 0;
        vxsSetMulticastLoop(sock, byteOption);
    }
#endif

    if (result < 0)
        perror(add ? "cannot join multicast group" : "cannot drop multicast group");
    return result;
}

// Get the current name for the specified socket
VXSOCKADDR	vxsGetSockName( VXSOCKET sock )
{
    VXSOCKADDR	psad = vxsGetSockaddr();
 
    if ( psad )
    {
        // obtain the address/port of the socket
        socklen_t addrLen=sizeof(psad->U.sad);
        memset(&psad->U.sad,0,addrLen);
        if (getsockname(sock, (struct sockaddr *)&psad->U.sad, &addrLen) < 0)
        {
            perror("cannot get sock name");
            vxsFreeSockaddr(psad);
            psad = NULL;
            
        }
    }
    return psad;
}

// Get the address and port number assigned to the socket
STATUS vxsGetSockAddressAndPort( VXSOCKET sock,  ULONG *inAddr, USHORT *inPort)
{
    STATUS		sts = ERROR;
    VXSOCKADDR	psad = vxsGetSockName(sock);
    
    if (psad)
    {
        *inPort = vxsAddrGetPort(psad);
        *inAddr = vxsAddrGetAddr(psad);
        vxsFreeSockaddr( psad );
        sts = OK;
    }
    else
    {
        *inPort = 0;
        *inAddr = 0;
    }
     return sts;
}

// Select to wait on a read fd for any of several sockets
// This function returns the first valid network interface address found in the local system.
// It also returns the number of valid network interface IP4 addresses found in the local system (not including the local loopback)
STATUS getFirstLocalInterfaceAddress(ULONG *interfaceIpAddr, int *AddressCount)
{
    struct ifaddrs *ifAddrStruct = NULL, *ifa = NULL;
    STATUS sts = ERROR;
    int addrCount = 0;

    // get the list of structures describing the network interfaces of the local system
    if (getifaddrs(&ifAddrStruct) == -1)
    {
    	perror("getifaddrs");
    	return sts;
    }

    // Walk through linked list, maintaining head pointer so we can free list later
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }

       	if (ifa->ifa_addr->sa_family == AF_INET)
    	{
    		// it is IP4
     		char mask[INET_ADDRSTRLEN];
            void* mask_ptr = &((struct sockaddr_in*) ifa->ifa_netmask)->sin_addr;

            inet_ntop(AF_INET, mask_ptr, mask, INET_ADDRSTRLEN);	// find the mask

            if (strcmp(mask, "255.0.0.0"))
            {
                // is a valid IP4 Address
                if (addrCount == 0)
                {
                	*interfaceIpAddr =  ntohl(((struct sockaddr_in *) ifa->ifa_addr)->sin_addr.s_addr);
                	sts = OK;
                }
                addrCount++;
            }
        }
    }
    if (ifAddrStruct != NULL)
    	// Free the data which dynamically allocated by the getifaddrs()
    	freeifaddrs(ifAddrStruct);
    *AddressCount = addrCount;
    return sts;
}

#ifdef NOTNOW
STATUS getFirstAvailUnicastAddress(const char *interfaceName, VXSOCKADDR *pSocketAddr)
{
    struct ifaddrs *ifAddrStruct = NULL, *ifa = NULL;
    STATUS sts = ERROR;
    int addrCount = 0;
    VXSOCKADDR socketAddr;

    *pSocketAddr = NULL;

    // get the list of structures describing the network interfaces of the local system
    if (getifaddrs(&ifAddrStruct) == -1)
    {
    	perror("getifaddrs");
    	return sts;
    }

    // Walk through linked list, maintaining head pointer so we can free list later
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }

       	if ((ifa->ifa_addr->sa_family == AF_INET) &&
       			()!strncmp(ipIfName, ifa->ifa_name, sizeof(ifa->ifa_name))))
		{
            *interfaceIpAddr =  ntohl(((struct sockaddr_in *) ifa->ifa_addr)->sin_addr.s_addr);
                	sts = OK;
                }
                addrCount++;
            }
        }
    }
    if (ifAddrStruct != NULL)
    	// Free the data which dynamically allocated by the getifaddrs()
    	freeifaddrs(ifAddrStruct);
    *AddressCount = addrCount;
    return sts;
}
#endif

// Checks if the given IP4 address is one of the valid local system network interface IP address
STATUS validInterfaceAddress(ULONG ipAddress, boolean *valid)
{
    struct ifaddrs *ifAddrStruct = NULL, *ifa = NULL;

    *valid = false;

    // get the list of structures describing the network interfaces of the local system
    if (getifaddrs(&ifAddrStruct) == -1)
    {
    	perror("getifaddrs");
    	return ERROR;
    }

    // Walk through linked list, maintaining head pointer so we can free list later
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }

     	if (ifa->ifa_addr->sa_family == AF_INET)
    	{
    		// it is IP4
     		char mask[INET_ADDRSTRLEN];
            void* mask_ptr = &((struct sockaddr_in*) ifa->ifa_netmask)->sin_addr;
            inet_ntop(AF_INET, mask_ptr, mask, INET_ADDRSTRLEN);

            if (strcmp(mask, "255.0.0.0"))
            {
                // is a valid IP4 Address
                if (ipAddress == ntohl(((struct sockaddr_in *) ifa->ifa_addr)->sin_addr.s_addr))
                {
                	// Found it
                	*valid = true;
                    break;
                }
            }
        }
    }
    if (ifAddrStruct != NULL)
    	// Free the data which dynamically allocated by the getifaddrs()
    	freeifaddrs(ifAddrStruct);
    return OK;
}



// end

