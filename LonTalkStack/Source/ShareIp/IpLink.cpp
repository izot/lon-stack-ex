/***************************************************************
 *  Filename: IpLink.cpp
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
 *  Description:  IP Link class implementation
 *
 *	DJ Duffy Nov 1998
 *
 ****************************************************************/

/*
 * $Log: /Dev/ShareIp/IpLink.cpp $
 *
 * 48    8/01/07 5:50p Fremont
 * EPR 45753 - clearing the network statistics of an internal device
 * cleared the console linkstats display, which was intended to show the
 * external packet statistics. In standalone mode on the iLON, this
 * happened automatically on any internal device commission. Solution:
 * keep a shadow shadow copy of the statistics that were actually shared,
 * increment them in the driver, and read/clear the shadow copies when
 * accessing the internal device.
 *
 * 47    12/08/05 2:11p Fremont
 * Fix INADDR_xxx macro conflicts by replacing them with vxsINADDR_xxx
 * macros
 *
 * 46    6/09/05 6:07p Fremont
 * EPRS FIXED: 37362 - IP-852 channel uses too much memory. Point all
 * packet allocations to the "master" object, and don't allocate any
 * buffers for the slaves.
 *
 * 45    12/09/04 6:23p Fremont
 * Change conditional compilation for self installed multicast
 *
 * 44    11/23/04 5:39p Fremont
 * EPRS FIXED: 35208 - Bombardier special - self-installed multicast IP
 * channel mode
 *
 * 43    9/09/03 2:54p Fremont
 * add time-to-live for alternate IP port, fix logic with device ID
 * data/req packets
 *
 * 42    9/03/03 11:20p Fremont
 * rewrite determineReceiveClient() again...
 *
 * 41    9/03/03 5:31p Fremont
 * rewrite determineReceiveClient(), use extended packet headers
 *
 * 40    8/28/03 9:24a Fremont
 * don't include ilon.h under Windows
 *
 * 39    8/27/03 12:33p Fremont
 * try various techniques to determine the source port of a message if it
 * is unknown, since it may have been switched by a firewall/NAT
 *
 * 38    7/22/03 3:55p Fremont
 * don't bind to INADDR_ANY
 *
 * 37    3/25/03 11:38a Fremont
 * include port num in slave hash table
 *
 * 36    12/11/02 6:16p Fremont
 * fix warning
 *
 * 35    12/02/02 2:13p Fremont
 * Change receive task for startup and shutdown
 *
 * 34    6/14/02 5:59p Fremont
 * remove warnings
 *
 * 33    11/07/01 10:32a Fremont
 * rename m_flags to m_pktFlags to avoid conflict with stupid Tornado
 * incude file (mbuf.h)
 *
 * 32    11/06/01 9:25a Glen
 * Need control of zero crossing synchronization and attenuation for
 * LonTalk Validator.  Added these flags to sendPacket().  This propagated
 * to lots of places.  Also added control options to LtMsgOut.
 *
 * 31    9/24/01 5:57p Fremont
 * fix T2 warning
 *
 * 30    11/15/00 3:39p Bobw
 * Delete useless and dangerous stamenent m_socket = 0 - meant to do that
 * when I moved the m_socket = ERROR.
 *
 * 29    10/24/00 11:25a Bobw
 * Dont ever set socket to 0 - that is considered a bad value
 *
 * 28    10/20/00 11:23a Bobw
 * Prevent deadlock when closing IP channel
 *
 * 27    10/10/00 2:16p Darrelld
 * Fix thread rundown
 *
 * 26    5/01/00 2:38p Darrelld
 * Use the real source IP address, rather than INADDR_ANY
 *
 * 25    3/13/00 5:26p Darrelld
 * Fix locking problems exposed by segments
 *
 * 23    12/08/99 12:19p Darrelld
 * Fix assert in receive task
 *
 * 22    8/17/99 4:15p Darrelld
 * Fix receive packet size
 *
 * 21    8/09/99 12:08p Darrelld
 * Fix for shutdown under traffic
 *
 * 20    8/05/99 5:23p Darrelld
 * Correct handling of zero ipaddr and port
 *
 * 19    7/30/99 8:59a Darrelld
 * Cleanup and streamline
 *
 * 18    7/28/99 4:45p Darrelld
 * Correct master dispatch
 *
 * 17    7/27/99 1:53p Darrelld
 * Remove trace code
 *
 * 16    7/08/99 1:11p Darrelld
 * Fix deadlock problem
 *
 * 15    7/07/99 5:06p Darrelld
 * Lock testing
 *
 * 14    7/07/99 3:51p Darrelld
 * Clean up reporting
 *
 * 13    7/07/99 2:39p Darrelld
 * Lock nesting debugging
 *
 * 12    7/07/99 9:46a Darrelld
 * Fix lock stack check
 *
 * 11    7/06/99 5:24p Darrelld
 * Lock nesting debugging
 *
 * 10    7/02/99 8:45a Darrelld
 * Target specific files
 *
 * 9     6/22/99 5:00p Darrelld
 * Support for master and slave links
 *
 * 8     4/22/99 5:02p Darrelld
 * Testing of routers
 *
 * 7     4/20/99 4:12p Darrelld
 * add "confidential" statement
 *
 * 6     4/13/99 4:58p Darrelld
 * Enhance for Aggregation and BW Limiting and test
 *
 * 5     4/02/99 5:21p Darrelld
 * Multi-Router Testing in Memory
 *
 * 4     3/17/99 3:43p Darrelld
 * Enhanced tests and fixed memory leaks
 *
 * 3     3/15/99 5:04p Darrelld
 * Intermediate checkin
 *
 * 2     3/05/99 2:32p Darrelld
 * Enhance for Ipaddresses and ports
 *
 * 1     2/22/99 9:20a Darrelld
 * moved here from LTIPtest
 *
 * 5     12/15/98 4:33p Darrelld
 * Reorganize base classes to make LtLink pure and virtual as the driven
 * snow.
 *
 * 4     12/03/98 12:58p Darrelld
 * Base level 1. All functions tested and work as advertised.
 *
 * 3     12/02/98 4:57p Darrelld
 * Check-in updates
 *
 * 2     12/02/98 1:47p Darrelld
 * LonLink, TgLink and IpLink work, Event Log works.
 *
 * 1     12/02/98 9:08a Darrelld
 *
 * 1     11/30/98 4:57p Darrelld
 * LTIP Test application
 *
 */
//
//////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <LtRouter.h>
//#include <VxWorks.h>
//#include <VxLayer.h>
#include <VxSockets.h>
//#include <taskLib.h>
//#include <LtCUtil.h>
#include <IpLink.h>
#include <LtIpPackets.h>
#include <LtNetworkBase.h>
#include <vxlTarget.h>
#include "LtIpMaster.h"
#include "LtIpPackets.h"
#include "LtIpEchPackets.h"
#ifdef __VXWORKS__
#include "echelon/ilon.h"
#endif
#include "SelfInstallMulticast.h"

#if INCLUDE_SELF_INSTALLED_MULTICAST
#ifdef linux
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#elif __VXWORKS__
#include "inetLib.h"
#include "sockLib.h"
#else

#endif
#endif  // if INCLUDE_SELF_INSTALLED_MULTICAST

#define LTPKTMAXLEN 249

#if 0 // enable to debug deadlocks
void	CIpLink::xLock( char* pszFile, int nLine )
{	lock();
	int i = ++m_nLockDepth;
	if ( i >= LOCKDEPTHMAX )
	{	i = LOCKDEPTHMAX-1;
		m_nLockDepth = i;
		vxlReportEvent("Link xLock - lock depth overflow at %s %d\n", pszFile, nLine);
	}
	m_pszLockFile[i] = pszFile;
	m_nLockLine[i] = nLine;
}

void	CIpLink::xUnlock( char* pszFile, int nLine )
{	unlock();
	int i = --m_nLockDepth;
	if ( i<0 )
	{	vxlReportEvent("Link xUnlock - lock depth underflow at %s %d\n", pszFile, nLine);
		i = 0;
		m_nLockDepth = 0;
	}
	m_pszLockFile[i] = NULL;
	m_nLockLine[i] = 0;
}

void	CIpLink::xReportLockStack()
{

	int		i;
	int		j = 0;

	for ( i=0; i< LOCKDEPTHMAX && i < m_nLockDepth ; i++ )
	{
		vxlReportEvent("Link LockStack [%d] %s; %d\n", i, m_pszLockFile[i], m_nLockLine[i] );
		j++;
	}
	if ( j == 0 )
	{	vxlReportEvent("Link LockStack empty\n");
	}
}

#define lock() xLock( __FILE__, __LINE__ )
#define unlock() xUnlock( __FILE__, __LINE__ )
#endif // debug deadlocks

#define DEBUGReportEvent(a)
//#define DEBUGReportEvent vxlReportEvent

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIpLink::CIpLink()
{
	m_bIpActive			= false;
	m_socket			= INVALID_SOCKET;
    m_sendSocket        = INVALID_SOCKET;   // use to send the msg in multicast
	m_bNoHeader			= false;
	m_ipDstAddr			= 0;	// of our partner
	m_ipSrcAddr			= 0;	// of ourselves
	m_ipDstPort			= 0;	// our partner
	m_ipSrcPort			= 0;	// ourselves
	m_ipDstPortAlt		= 0;	// alternate port for our partner (for NAT)
	m_altPortTTL		= 0;	// time-to-live for alternate port
	m_ipDstSockAddr		= NULL;	// no socket address for the destination
    m_ipRcvSockAddr     = NULL;	// no socket address for receive
	m_pMasterLink		= NULL;	// we are master until told otherwise
	m_nPktsPerSlave		= 20;	// twenty packets per slave
	m_pIpNet			= NULL;	// enhanced network object
	m_pLtIpMaster		= NULL;	// owner LtIpMaster object
	m_linkStats.m_shadowed = false; // Don't shadow the IP-side link stats

    m_ipMCastSenderPort = 0;            // multicast sender port
	m_selfInstalledMcastAddr = 0;       // Multicast group IP address
	m_pSelfInstalledMcastClient = NULL; // The other client for multicast
	m_selfInstalledMcastHops = 1;       

	m_nLockDepth = 0;
	memset( m_pszLockFile, 0, sizeof(m_pszLockFile) );
	memset( m_nLockLine, 0, sizeof(m_nLockLine) );
}

//
//	Destructor
//
//	make sure the socket is closed and the queues are drained
// althrough if queues are not drained by now, then we may crash
// because the network object may already be gone.
//
CIpLink::~CIpLink()
{
	// do not lock first
	close();
}

//
//	enumInterfaces
//
//	Specify only one interface, the local host name.
//
boolean CIpLink::enumInterfaces( int idx, LPSTR pNameRtn, int nMaxSize )
{
	STATUS	sts;
	boolean	bOk = false;
	if ( idx == 0 )
	{
		sts = vxsGetHostName( pNameRtn, nMaxSize );
		bOk = sts == OK;
	}
	return bOk;
}

//
// bindSocket
//
// Create a socket and bind it to us
//

int CIpLink::bindSocket( int type )
{
	VXSOCKADDR		psad1 = NULL;
	STATUS			vxsts = OK;
	VXSOCKET		sock = INVALID_SOCKET;

	do
	{
		// bind to our local address
		// use an explicit address if we have one
		if (selfInstalledMcastMode())
		{
			// Special self-installed multicast channel, must accept incoming multicast messages
			psad1 = vxsAddrValue( vxsINADDR_ANY );
		}
		else
		if ( m_ipSrcAddr )
		{
			psad1 = vxsAddrValue( m_ipSrcAddr );
		}
		else
		{	// Don't do this, it could cause protocol failures
			// psad1 = vxsAddrValue( vxsINADDR_ANY );
			break;
		}

		vxsSetPort( psad1, m_ipSrcPort );

		sock = vxsSocket( type );
		if ( sock == INVALID_SOCKET )
		{
			vxlReportEvent("CIpLink::bindSocket - INVALID_SOCKET Addr = %ld Port = %d\n", m_ipSrcAddr, m_ipSrcPort);
			break;
		}

        if (selfInstalledMcastMode())
		{
            // allow multiple sockets to use the same PORT number 
            vxsts = vxsAllowUsingSamePort(sock);
        }

		if ( vxsts == OK )
        {
		    // Try bind
		    vxsts = vxsBind( sock, psad1 );
       }
		if ( vxsts != OK )
		{
            vxsCloseSocket( sock );
			sock = INVALID_SOCKET;
			vxlReportEvent("CIpLink::bindSocket - InvalidSocket sock = %d vxsBind = %d Addr = %ld Port = %d\n", sock, vxsts, m_ipSrcAddr, m_ipSrcPort);
 		}
		else if (selfInstalledMcastMode())
		{
			// Special multicast channel -- enable multicast on this socket
            vxsts = vxsJoinMulticast( sock, m_selfInstalledMcastAddr, m_ipSrcAddr );
            if ( vxsts == OK )
            {
                // set multicast time-to-live value option
                if (vxsSetMulticastTTL( sock, m_selfInstalledMcastHops ) < 0)
                    vxsts = ERROR;
            }
            if ( vxsts != OK )
		    {
                vxsCloseSocket( sock );
			    sock = INVALID_SOCKET;
		    }
		}
	} while (false);

	vxsFreeSockaddr( psad1 );

	return sock;
}


//
// createSocket
//
// Create a socket and return the default socket address and port
//
int CIpLink::createSocket( int type )
{
	VXSOCKET		sock = INVALID_SOCKET;

    sock = vxsSocket( type );
	return sock;
}

//
// getSendSocketToUse
//
// Get the socket number to be used for sending
//
int CIpLink::getSendSocketToUse()
{
    int sockToUse;
    if (selfInstalledMcastMode()) 
    {
        // multicast app has its own socket for sending
        sockToUse = isMaster() ? m_sendSocket : m_pMasterLink->masterSendSocket();
    }
    else
    {
        sockToUse = isMaster() ? m_socket : m_pMasterLink->masterSocket();   
    }
     return sockToUse;
}

//
//	open
//
//	Open the socket to the specified host name for UPD use.
//
LtSts CIpLink::open( const char* pName )
{
	LtSts			sts = LTSTS_OPENFAILURE;
	VXSOCKADDR		psad = NULL;
	int				nErrors = 0;
	VXSOCKET		sock = INVALID_SOCKET;
    VXSOCKET		sock2 = INVALID_SOCKET;
	boolean			bRegisterSlave = false;

	DEBUGReportEvent("CIpLink::open - close any open link\n");
	// must not call with this link locked
	close();
	clearStatistics();

	DEBUGReportEvent("CIpLink::open - lock link object\n");
	lock();
	do
	{
		// if we are the master, then we own the socket
		if ( isMaster() )
		{
			DEBUGReportEvent("CIpLink::open - master get a socket\n");
			// get a socket bound to us
			sock = bindSocket( VXSOCK_DGRAM );
			if ( sock == INVALID_SOCKET )
			{
				DEBUGReportEvent("CIpLink::open - bind socket error\n");
				nErrors++;
				break;
			}

	        if (selfInstalledMcastMode())
            {
                // If we're running multicast, the sender has its own socket.   
                sock2 = createSocket( VXSOCK_DGRAM );;
                if ( sock2 == INVALID_SOCKET )
			    {
                	DEBUGReportEvent("CIpLink::open - create socket error for mc\n");
                	nErrors++;
				    break;
			    }
            }
			DEBUGReportEvent("CIpLink::open - master start receivetask\n");
			startReceiveTask();
		}
		else
		{	// register ourselves with the master to receive stuff for us
			assert( m_ipDstAddr != 0 && m_ipDstPort != 0 );
			bRegisterSlave = true;
		}
		// Address struct for the destination
		DEBUGReportEvent("CIpLink::open - get address for destination\n");
		m_ipDstSockAddr = vxsAddrValue( m_ipDstAddr );
		vxsSetPort( m_ipDstSockAddr, m_ipDstPort );
		// empty sock address to use for receive
		m_ipRcvSockAddr = vxsGetSockaddr();

	} while (false);

	vxsFreeSockaddr( psad );

	if ( nErrors == 0 )
	{	
        m_socket = sock;
        m_sendSocket = sock2;
		m_bIpActive = true;
		DEBUGReportEvent("CIpLink::open - success\n");
		sts = LTSTS_OK;
	}
	else
	{
		DEBUGReportEvent("CIpLink::open - failed\n");
        if (sock != INVALID_SOCKET)
		vxsCloseSocket( sock );
        if (sock2 != INVALID_SOCKET)
            vxsCloseSocket( sock2 );
	}
	DEBUGReportEvent("CIpLink::open - unlock link object\n");
	unlock();
	// now that we are unlocked, we can call the master to lock us
	// the receive task locks the master and then the slave, so we can't
	// be locked and then call the master.
	//
	if ( bRegisterSlave )
	{
		DEBUGReportEvent("CIpLink::open - slave register slave\n");
		m_pMasterLink->registerSlave( this, m_ipDstAddr, m_ipDstPort );
	}
	return sts;
}

//
//	close
//
//	stop the receive task, drain the queues and close the socket.
//
LtSts CIpLink::close()
{
	lock();
	if ( isOpen() )
	{

		if ( isMaster() )
		{
			int	holdSocket = m_socket;
            int senderMcastSocket = m_sendSocket;
			m_socket = INVALID_SOCKET;
            m_sendSocket = INVALID_SOCKET;
			// give everyone time to be out of  the socket layer
			taskDelay( msToTicksX( 200 ) );

			// Calling the shutdown before closing socket.  This fixed the problem in Linux environment when the recvr() task
			// currently blocked to abort early
			vxsShutdownSocket(holdSocket, 2);   // 2 = SHUT_RDWR (Linux) or SD_BOTH (Win32));
			vxsCloseSocket( holdSocket );

            if (selfInstalledMcastMode() && (senderMcastSocket != INVALID_SOCKET))
            {
			    vxsShutdownSocket(senderMcastSocket, 2);   // 2 = SHUT_RDWR (Linux) or SD_BOTH (Win32));
    			vxsCloseSocket( senderMcastSocket );
            }

			// Must unlock to avoid deadlock, since the Receive task may
			// be waiting on this lock.
			unlock();
			stopReceiveTask();
			lock();
			drainReceiveQueue( m_qReceiveP );
			drainReceiveQueue( m_qReceive );
			//drainTransmitQueue( m_qTransmit );
		}
		else
		{
			// if we are the slave and we are open, then we need to deregister
			// first and must not have the client locked when we do that.
			// this avoids deadlock with the master receive task
			unlock();
			m_pMasterLink->deregisterSlave( this, m_ipDstAddr, m_ipDstPort );
			lock();
		}
		vxsFreeSockaddr( m_ipDstSockAddr );
		m_ipDstSockAddr = NULL;
		vxsFreeSockaddr( m_ipRcvSockAddr );
		m_ipRcvSockAddr = NULL;

		m_bActive = false;
		// don't reset statistics
		//reset();
		m_bIpActive = false;
	}
	unlock();
	return LTSTS_OK;
}

//
// sendPacketTo
//
// Send to explicit destination
//
LtSts CIpLink::sendPacketTo( void* refId, ULONG ipAddr, word port, byte* pData, int nLen )
{
	VXSOCKADDR	psad = NULL;
	LtSts		sts = LTSTS_ERROR;
	int			nBytes;
	int			sockToUse;


	// if we are open
	while ( isOpen() )
	{
        sockToUse = getSendSocketToUse();   // Mcast app has its own socket for sending

		// this covers the case of no configuration server id
		// just drop the message
		if ( ipAddr == 0 || port == 0 || sockToUse == 0 )
		{	break;
		}
        psad = vxsAddrValue( ipAddr );
		if ( psad == NULL )
		{	break;
		}
		vxsSetPort( psad, port );

		nBytes = vxsSendTo( sockToUse, (LPSTR)pData, nLen, 0, psad );
		vxsFreeSockaddr( psad );
		if ( nBytes == nLen )
		{	sts = LTSTS_OK;
		}
		break;
	}

	if ( m_pNet )
	{
		m_linkStats.m_nTransmittedPackets++;
		m_pNet->packetComplete( refId, sts );
	}

	return sts;
}


//
//	sendPacket
//
//	Send a packet on the socket, to the default.
//
LtSts CIpLink::sendPacket(void* referenceId,
				int nPrioritySlot,
				byte flags,
				byte* pData,
				int nDataLength,
				boolean bPriority)
{
	LtSts	sts = LTSTS_INVALIDSTATE;
	int		bytes = 0;
	int		nNewSize;
	byte*	pNewData;
	int		sockToUse;

	if ( isOpen() )
	{
		nNewSize = nDataLength;
		pNewData = pData;
        sockToUse = getSendSocketToUse();   // Mcast app has its own socket for sending
    
		// don't send any messages to the zero ipAddr or port
		if ( m_ipDstSockAddr != NULL &&  m_ipDstAddr != 0 && m_ipDstPort != 0 && sockToUse )
		{
			bytes = vxsSendTo( sockToUse, (LPSTR)pNewData, nNewSize, 0, m_ipDstSockAddr );
		}

		if ( bytes == nNewSize )
		{	sts = LTSTS_OK;
			dumpPacket("IpLink - sendPacket OK", pNewData, nNewSize, 
                m_ipSrcAddr, vxsAddrGetAddr(m_ipDstSockAddr));
		}
		else
		{	sts = LTSTS_ERROR;
			m_linkStats.m_nTransmissionErrors++;
			dumpPacket("IpLink - sendPacket ERROR", pNewData, nNewSize,
                 m_ipSrcAddr, vxsAddrGetAddr(m_ipDstSockAddr));
		}
	}

	if ( m_pNet )
	{
		m_linkStats.m_nTransmittedPackets++;
		m_pNet->packetComplete( referenceId, sts );
	}
	// immediate data return
	return LTSTS_OK;
}


//
// receiveNoNet
//
// Receive synchronously if there is no Net object
//
int		CIpLink::receiveNoNet( ULONG* pipSrc, ULONG* pipSrcPort, byte* pData, int nMaxLen )
{
	int			nBytes = 0;
	VXSOCKADDR	psa = vxsGetSockaddr();
	int			sockToUse;

	*pipSrc = 0;
	*pipSrcPort = 0;

	if ( isOpen() && psa != NULL )
	{
		if ( isMaster() )
		{	sockToUse = m_socket;
		}
		else
		{	sockToUse = m_pMasterLink->masterSocket();
		}
		// Peek first so we don't block if there's no data waiting
		// this doesn't actually work on NT apparently, but we don't depend on it
		nBytes = vxsRecvFrom( sockToUse, (LPSTR)pData, nMaxLen, VXSOCK_MSG_PEEK, psa);
		if ( nBytes != 0 )
		{	// Data waiting, so receive it
			nBytes = vxsRecvFrom( sockToUse, (LPSTR)pData, nMaxLen, 0, psa);
			dumpPacket("IpLink - receiveNoNet", pData, nBytes,
                vxsAddrGetAddr(m_ipRcvSockAddr), m_ipSrcAddr);
		}
	}

	// if we got a packet, then return the source address and port
	if ( nBytes && psa )
	{
		*pipSrc = vxsAddrGetAddr( psa );
		*pipSrcPort = vxsAddrGetPort( psa );
	}
	if ( psa ) vxsFreeSockaddr( psa );
	return nBytes;

}


//
//	queueReceive
//
//	queue a receive buffer for the receive task
//
LtSts CIpLink::queueReceive( void* referenceId,
					boolean bPriority,
					byte flags,
					byte* pData,
					int nMaxLength)
{
	LtSts	sts = LTSTS_INVALIDSTATE;
	LLPktQue*	pPkt;

	if (m_pMasterLink != NULL)
	{
		return m_pMasterLink->queueReceive(referenceId, bPriority, flags, pData, nMaxLength);
	}

	lock();
#ifdef TESTSEGS

	// search the queue to see if the item we are going to queue
	// is already on the queue
	LtQue*		pQue = &m_qReceiveP;
	pQue->lock();
	LtQue*		pItem = pQue->m_pNxt;
	LtPktInfo*	pPktInfo = (LtPktInfo*)referenceId;
	LtQue*		pNewItem = (LtQue*)pPktInfo;
	while ( pItem != pQue )
	{
		pPkt = (LLPktQue*)pItem;
		if ( pPkt->m_refId == pNewItem )
		{	vxlReportEvent("CIpLink::queueReceive - Priority duplicate item 0x%08x\n", pNewItem );
			return LTSTS_PENDING;	// It's already here, so this is less disruptive.
		}
		pItem = pItem->m_pNxt;
	}
	pQue->unlock();

	pQue = &m_qReceive;
	pQue->lock();
	pItem = pQue->m_pNxt;
	while ( pItem != pQue )
	{
		pPkt = (LLPktQue*)pItem;
		if ( pPkt->m_refId == pNewItem )
		{	vxlReportEvent("CIpLink::queueReceive - duplicate item 0x%08x\n", pNewItem );
			return LTSTS_PENDING;	// It's already here, so this is less disruptive.
		}
		pItem = pItem->m_pNxt;
	}
	pQue->unlock();

#endif // TESTSEGS
	if ( m_nReceiveQueueDepth < m_qReceive.getCount() )
	{
		sts = LTSTS_QUEUEFULL;
	}
	else
	{
		pPkt = getLLPkt();
		pPkt->m_refId			= referenceId;
		pPkt->m_pktFlags		= flags;
		pPkt->m_bPriority		= bPriority;
		pPkt->m_pData			= pData;
		pPkt->m_nDataLength		= nMaxLength;
		if ( bPriority )
		{	m_qReceiveP.insertTail( pPkt );
		}
		else
		{	m_qReceive.insertTail( pPkt );
		}
		sts = LTSTS_PENDING;
	}
	unlock();
	return sts;
}

//
//	reset
//
//	but unlike our parent, we remain active
//
void CIpLink::reset()
{
	LtLinkBase::reset();
	m_bActive = true;
}

//
//	registerSlave
//
//	Put a slave link in the hash table.
//
void	CIpLink::registerSlave( CIpLink* pLink, ULONG ipAddr, USHORT ipPort )
{
	LtHipKey*	pKey = new LtHipKey(ipAddr, ipPort);

	lock();
	m_htSlaves.set( pKey, pLink );
	unlock();
}


//
//	deregisterSlave
//
//	Remove a slave link from the slave table
//
void	CIpLink::deregisterSlave( CIpLink* pLink, ULONG ipAddr, USHORT ipPort )
{
	LtHipKey	Key(ipAddr, ipPort);
	CIpLink*	pRecLink;
	lock();
	pRecLink = m_htSlaves.removeKey( &Key );
    if (pRecLink)
	    assert( pRecLink == pLink );
	unlock();
}

//
// receivePacket
//
// Process a packet for this link.
// Called for the master or a slave from the master receive task
//
void CIpLink::receivePacket( byte* data, int nPktSize, VXSOCKADDR rcvSockAddr, USHORT ipPortOverride )
{
	boolean		bOpen = isOpen();
	int			nSize;
	LLPktQue*	pPkt;
	LtQue*		pItem;
	LtSts		sts;
	boolean		bPrior = false;
	byte*		pData;
	byte*		pDataIn;
	boolean		bGotOne;
	USHORT		ipPort;

	do
	{
		// don't need to lock
		//lock();
		if ( bOpen )
		{
			if ( nPktSize == 0 )
			{	// ignore errors
				break;
			}
		}
		else
		{	break;	// not open
		}
		// set the priority bit in the packet.
		m_linkStats.m_nReceivedPackets++;
		// Figure out priority setting for packet
		bPrior = false;
		if ( bPrior )
		{	m_linkStats.m_nReceivedPriorityPackets++;
		}
		//if ( bPrior?!m_qReceiveP.isEmpty() : !m_qReceive.isEmpty() )
		do
		{
			sts = LTSTS_OK;
			// No priority receive...
			// Use the master link's buffers
			if (m_pMasterLink == NULL)
			{
				bGotOne = m_qReceive.removeHead( &pItem );
			}
			else
			{
				bGotOne = m_pMasterLink->m_qReceive.removeHead( &pItem );
			}

			/*
			if ( bPrior )
			{	bGotOne = m_qReceiveP.removeHead( &pItem );
			}
			else
			{	bGotOne = m_qReceive.removeHead( &pItem );
			}
			*/
			if ( !bGotOne ) break;
			pPkt = (LLPktQue*)pItem;
			pData = pPkt->m_pData;

			nSize = nPktSize;
			pDataIn = data;

			if ( nSize > pPkt->m_nDataLength )
			{
				// packet is too big for the waiting receive buffer
				// so count a missed packet and then put the receive
				// buffer back on the head of the queue to be used again.
				sts = LTSTS_OVERRUN;
				m_linkStats.m_nMissedPackets++;

				// Give buffer back to the master link
				if (m_pMasterLink == NULL)
				{
					m_qReceive.insertHead( pItem );
				}
				else
				{
					m_pMasterLink->m_qReceive.insertHead( pItem );
				}
				/*
				if ( bPrior )
				{	m_qReceiveP.insertHead( pItem );
				}
				else
				{	m_qReceive.insertHead( pItem );
				}
				*/
				break;
			}
			else if ( pPkt && m_pNet )
			{
				memcpy( pData, pDataIn, nSize );

				// at this point we need to decide which slave client gets the packet
				// So look through all registered clients and if it's none of them, then
				// process it ourselves.
				//
				//unlock();
				// If a port override was supplied, use that
				if (ipPortOverride != 0)
					ipPort = ipPortOverride;
				else
					ipPort = vxsAddrGetPort( rcvSockAddr );
				// Use the IP sensitive return point
				m_pIpNet->packetReceived( pPkt->m_refId, nSize, pPkt->m_bPriority,
										vxsAddrGetAddr( rcvSockAddr ), ipPort, sts );
				//lock();
				freeLLPkt( pPkt );
			}
			pPkt = NULL;
			pItem = NULL;
		} while (false);
		if ( !bGotOne )
		{	// we didn't have a buffer for this packet
			m_linkStats.m_nMissedPackets++;
		}
	} while(false);
	//unlock();
}

//
// receiveTask
//
// The actual task code is in context of our class
// read an ip packet from the socket, and return the data
// if we have a waiting receive.
//

void CIpLink::receiveTask()
{
	byte		data[MAX_UDP_FRAME_SIZE];	// allow largest size - symbol someday
	int			nPktSize;
	boolean		bOpen;
	boolean		bNotOpenReported = false;
	CIpLink*	pLink;		// pointer to the link to receive message, us or a slave
	USHORT		ipPortOverride = 0;
    boolean     bMcastLoopBackPkt = false;

	// Make sure the spawning task has released the lock
	lock();
	unlock();

	while(true)
	{
		bOpen = isOpen();
		if ( m_bExitReceiveTask )
		{	break;
		}
		//DEBUGReportEvent("CIpLink::receiveTask - unlock link object\n");

		// if we dont have a net, then just hang out here forever.
		// we might be in use without a network object, and thats
		// ok for testing.
		while ( m_pNet == NULL )
		{	taskDelay( msToTicksX(100) );
			//DEBUGReportEvent("CIpLink::receiveTask - waiting on a net object\n");
		}


		if ( bOpen )
		{
			// The call to close() will invalidate the socket before it has a chance
			// to set m_bExitReceiveTask, so bail out in that case
			if (m_socket == INVALID_SOCKET)
			{
				break;
			}

			nPktSize = vxsRecvFrom( m_socket, (LPSTR)data, MAX_UDP_FRAME_SIZE, 0, m_ipRcvSockAddr);
			if ( m_bExitReceiveTask )
			{	break;
			}
			dumpPacket("IpLink - receiveTask", data, nPktSize, 
                            vxsAddrGetAddr(m_ipRcvSockAddr), m_ipSrcAddr );
			bNotOpenReported = false;
		}
		else
		{
			if ( !bNotOpenReported )
			{
				vxlReportEvent("IpLink - receiveTask - Link Not Open\n");
				bNotOpenReported = true;
			}
			taskDelay( msToTicksX(100) );
			nPktSize = 0;
		}
		if ( nPktSize == ERROR )
		{
			vxlReportEvent("IpLink - socket error on RecvFrom\n");
			continue;	// Start back at top, to not process this error.
		}

		if ( m_bExitReceiveTask )
		{	break;
		}
		//DEBUGReportEvent("CIpLink::receiveTask - lock link object\n");
		lock();
		// the master is now locked, get the source address of the packet
		// and the port too.

		pLink = determineReceiveClient(data, nPktSize, ipPortOverride, bMcastLoopBackPkt);

		// unlock to allow segmentation to free buffers
		// to us during the receivePacket call.
		unlock();
        if (!bMcastLoopBackPkt)
        {
		    if ( pLink == NULL )
		    {	
                pLink = this;	// we are NOT locked
			    DEBUGReportEvent("CIpLink::receiveTask - for master\n");
            }
            else
            {
                DEBUGReportEvent("CIpLink::receiveTask - for a slave\n");
            }
            // Don't lock the link object since that will cause deadlocks.
		    //pLink->lock();
		    pLink->receivePacket( data, nPktSize, m_ipRcvSockAddr, ipPortOverride);
    	    //pLink->unlock();
		    //DEBUGReportEvent("CIpLink::receiveTask - back from receivePacket\n");
        }
	}
	// Report that we have exited
	m_tidReceive = ERROR;
}

// Determine which link client to use for this receive message
CIpLink* CIpLink::determineReceiveClient(byte* pPkt, int nPktSize, USHORT& ipPortOverride, boolean& bMcastLoopBackPkt)
{
	ULONG		ipAddr;
	word		ipPort;
	LtHipKey	key;
	CIpLink*	pLink;		// pointer to the link to receive message: us or a slave
	CIpLink*	pLinkTemp;
	LtIpPktHeader pktHdr;
	boolean		hasDups;
	boolean		gotDevId = false;

	pLink = NULL;
	ipPortOverride = 0;
    bMcastLoopBackPkt = false; 

	ipAddr = vxsAddrGetAddr( m_ipRcvSockAddr );
	ipPort = vxsAddrGetPort( m_ipRcvSockAddr );
	// if the the addr and port match, then it's the one "bound"
	// to the master, so process it directly here.
	// Else, look it up in the table.
	if (selfInstalledMcastMode())
	{
        ULONG ipMCastSenderAddr;    // in this case the socket InAddress is zero(use default interface)
		pLink = NULL;
        // check if the message comes from ours (loopback message)
        if (m_ipMCastSenderPort == 0)
            vxsGetSockAddressAndPort( m_sendSocket, &ipMCastSenderAddr, &m_ipMCastSenderPort );
        if ((m_ipSrcAddr == ipAddr) && (m_ipMCastSenderPort == ipPort))	
        {
            bMcastLoopBackPkt = true;
            vxlReportEvent("CIpLink::receiveTask - **** Message rejected!! it's a loopback message Port=%u\n", m_ipMCastSenderPort );
        }

		if (!bMcastLoopBackPkt && pktHdr.parse(pPkt, false) && (pktHdr.packetType == PKTTYPE_DATA))
		{
			if (m_pSelfInstalledMcastClient == NULL)
			{
				LtHashTablePos pos;
				LtHipKey key;

				pos.reset();
				m_htSlaves.getElement(pos, (LtHipKey**)NULL, &m_pSelfInstalledMcastClient);
			}
            pLink = m_pSelfInstalledMcastClient;	// All received data packets go to this client
    	}
	}
	else
	if ( ipAddr == m_ipDstAddr && ipPort == m_ipDstPort )
	{
		pLink = NULL;
		m_ipDstPortAlt = 0;	// forget any alternate port, we got the real one
		m_altPortTTL = 0;
	}
	else
	{
		// Try searching with the port
		key.setValue( ipAddr, ipPort );
		//DEBUGReportEvent("CIpLink::receiveTask - look up in hash\n");
		pLink = m_htSlaves.get(&key);

		if (pLink != NULL)
		{
			// We don't save alt ports for slaves, but make sure it is zero.
			pLink->m_ipDstPortAlt = 0;	// forget any alternate port, we got the real one
			pLink->m_altPortTTL = 0;
		}
		// If the hash lookup fails, we must get more sophisticated in our
		// search. It could be that the port was switched by NAT (or some
		// other type of firewall), but this may or may not matter.
		// Start by parsing the packet header.
		else if (pktHdr.parse(pPkt, false))
		{
			// Try peeking at the packet, to see if we
			// can figure out something from it.
			if (pktHdr.bHasExtHdr)
			{
				// Contains extended header with source port
				ipPortOverride = pktHdr.extHdrIpPort;
				gotDevId = true;
				// We still don't have the link; we'll look it up later
			}
			// Try checking the alternate CS port.
			else if ((pktHdr.packetType != PKTTYPE_DATA) &&
					(m_ipDstAddr == ipAddr) &&
					(m_ipDstPortAlt == ipPort) && // This is the CS port
					(time(NULL) < m_altPortTTL))	// the alt port is not stale
			{
				pLink = this;
				// Override the port with the real one we know about.
				ipPortOverride = m_ipDstPort;
				m_altPortTTL = time(NULL) + (60 * 5);	// Give it 5 more minutes to live
			}
			else
			{
				// Search without using the port.
				key.setValue(ipAddr, 0);
				pLinkTemp = m_htSlaves.get(&key);
				// If this fails just bail out
				if (pLinkTemp != NULL)
				{
					// OK, there is at least one client with this IP addr.
					// If it is the only one, and it is a data packet, we can use it.
					hasDups = m_htSlaves.hasDuplicates(&key);
					if ((pktHdr.packetType == PKTTYPE_DATA) && !hasDups)
					{
						pLink = pLinkTemp;
						// Override the port with the real one we know about.
						ipPortOverride = pLink->m_ipDstPort;
					}
				}
			}

			if ((pLink == NULL) && (ipPortOverride == 0))
			{
				// Still haven't found it. Try some more packet types
				if (pktHdr.vendorCode == VENDOR_ECHELON)
				{
					// Look for the special device ID packets
					// These are send by the diagnostic routine
					if (pktHdr.packetType == PKTTYPE_ECHDEVID)
					{
						LtIpEchDeviceId devId;

						if (devId.parse(pPkt, FALSE))
						{
							ipPortOverride = devId.ipPort;
							gotDevId = true;
						}
					}
					else if (pktHdr.packetType == PKTTYPE_ECHDEVIDREQ)
					{
						LtIpReqEchDeviceId devIdReq;

						if (devIdReq.parse(pPkt, FALSE))
						{
							ipPortOverride = devIdReq.ipPort;
							gotDevId = true;
						}
					}
				}
				// Do some housekeeping
				if ((m_altPortTTL != 0) && (time(NULL) >= m_altPortTTL))
				{
					// the alt port is stale, clear it
					pLink->m_ipDstPortAlt = 0;
					pLink->m_altPortTTL = 0;
				}
			}

			if ((pLink == NULL) && (ipPortOverride != 0))
			{
				// We found the real port, see if we can find
				// the proper link now.
				if (ipAddr == m_ipDstAddr &&
					ipPortOverride == m_ipDstPort)
				{
					// This is the CS
					pLink = this;	// Indicate we have found it
					// We got the client, remember the alt port
					m_ipDstPortAlt = ipPort;
					m_altPortTTL = time(NULL) + (60 * 5);	// Give it 5 minutes to live
				}
				else
				{
					// Try looking up the slave again
					key.setValue( ipAddr, ipPortOverride );
					pLink = m_htSlaves.get(&key);
				}
			}

			// If all else fails, tell the LtIpMaster to send some
			// diagnostic packets.
			// Do this only to identify the sender of a config packet.
			// Don't if the packet had device ID in it.
			if ((pLink == NULL) && (m_pLtIpMaster != NULL) &&
				(pktHdr.packetType != PKTTYPE_DATA) && !gotDevId)
			{
				m_pLtIpMaster->requestUnknownPortDiagnostic(ipAddr, ipPort);
			}
		}
	}

	return pLink;
}

LtLinkBase*	CIpLink::copyInstance()
{
    LtLinkBase* link = new CIpLink();
	((CIpLink*)link)->m_bIpActive   = m_bIpActive;
	((CIpLink*)link)->m_socket      = m_socket;
    ((CIpLink*)link)->m_sendSocket  = m_sendSocket; 
	((CIpLink*)link)->m_bNoHeader   = m_bNoHeader;
	((CIpLink*)link)->m_ipDstAddr   = m_ipDstAddr;	// of our partner
	((CIpLink*)link)->m_ipSrcAddr   = m_ipSrcAddr;	// of ourselves
	((CIpLink*)link)->m_ipDstPort   = m_ipDstPort;	// our partner
	((CIpLink*)link)->m_ipSrcPort   = m_ipSrcPort;	// ourselves
	((CIpLink*)link)->m_ipDstPortAlt    = m_ipDstPortAlt;	// alternate port for our partner (for NAT)
	((CIpLink*)link)->m_altPortTTL      = m_altPortTTL;	// time-to-live for alternate port

    ((CIpLink*)link)->m_ipMCastSenderPort = m_ipMCastSenderPort;
	((CIpLink*)link)->m_selfInstalledMcastAddr = m_selfInstalledMcastAddr;
	((CIpLink*)link)->m_pSelfInstalledMcastClient = m_pSelfInstalledMcastClient;
	((CIpLink*)link)->m_selfInstalledMcastHops = m_selfInstalledMcastHops;

	((CIpLink*)link)->m_nLockDepth = m_nLockDepth;
    return link;
}


//
// dumpPacket
//
// add ip address to the decoration for the dump
//
void	CIpLink::dumpPacket( const char* tag, byte* pData, int nLen, ULONG ipSrcAddr, ULONG ipDstAddr)
{
	LtIpAddressStr		ias1;
	LtIpAddressStr		ias2;
	char				cTag[256];
    if (ipSrcAddr == 0)
        ipSrcAddr = m_ipSrcAddr;    
    if (ipDstAddr == 0)
        ipDstAddr = m_ipDstAddr;
	ias1.setIaddr( ipSrcAddr );
	ias2.setIaddr( ipDstAddr );
	sprintf( cTag, "%s Src %s:%d Dst %s:%d", tag, ias1.getString(), m_ipSrcPort,
								ias2.getString(), m_ipDstPort);
	LtLinkBase::dumpPacket( cTag, pData, nLen );
}

//
// reportStatus
//
// report status and addresses for this link
//
void	CIpLink::reportStatus()
{
	LtIpAddressStr		ias1;
	LtIpAddressStr		ias2;
	lock();
	ias1.setIaddr( m_ipSrcAddr );
	ias2.setIaddr( m_ipDstAddr );

	vxlReportEvent("CIpLink::reportStatus - %s Src %s %d Dst %s %d\n",
		isOpen()?"Open":"Shut", ias1.getString(), m_ipSrcPort,
		ias2.getString(), m_ipDstPort );
	unlock();
}


// LtHipTable Methods

// See if there are multiple entries with the same IP address
boolean LtHipTable::hasDuplicates(LtHipKey* key)
{
	LtHipKey dupKey = *key;
	LtHashRecord *pHashRecFirst;
	LtHashRecord *pHashRecOther;
	boolean hasDups = false;

	// Just to be sure, zero out the port in a local copy
	dupKey.ipPortValue = 0;
	// Find the first match
	pHashRecFirst = findRec(&dupKey, NULL);
	if (pHashRecFirst != NULL)
	{
		// See if there are any more that match
		pHashRecOther = pHashRecFirst;
		while ((pHashRecOther = pHashRecOther->getNext()) != NULL)
		{
			if (dupKey == *(LtHipKey*)pHashRecOther->getKey())
			{
				hasDups = true;
				break;
			}
		}
	}

	return hasDups;
}

// Return the next CIpLink that matches the (partial) key.
// If the position object is uninitialized, get the first one.
CIpLink* LtHipTable::GetNextDup(LtHipKey* key, LtHashTablePos *pos)
{
	LtHipKey keyCopy = *key;
	CIpLink* pLink = NULL;
	LtHashRecord* pRec;

	// Just to be sure, zero out the port in a local copy
	keyCopy.ipPortValue = 0;
	if (pos->getIndex() == -1)
	{
		// Get the first one
		pos->setIndex(getIndex(&keyCopy));
		pos->setRec(findRec(&keyCopy, NULL));	// This finds the first one
		if (pos->getRec() != NULL)
		{
			pLink = (CIpLink*)(pos->getRec()->getValue());
		}
	}
	else
	{
		// Get subsequent ones
		pRec = pos->getRec();
		while (pRec != NULL)
		{
			pRec = pRec->getNext();
			if ((pRec != NULL) && (keyCopy == *(LtHipKey*)pRec->getKey()))
			{
				pLink = (CIpLink*)(pRec->getValue());
				break;
			}
		}
		pos->setRec(pRec);
	}
	return pLink;
}

// end
