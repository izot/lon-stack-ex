/***************************************************************
 *  Filename: LtIpMaster.cpp
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
 *  Description:  implementation of the LtIpMaster class.
 *
 *	DJ Duffy Feb 1999
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/ShareIp/LtIpMaster.cpp#2 $
//

#include <LtRouter.h>
#include <LtIpMaster.h>
#include <LtLreIpClient.h>
#include <LtStack.h>
#include <LtStackInternal.h>
#include <LtRouterApp.h>
#include <tickLib.h>
#include "LtIpPersist.h"
#include <IpLink.h>
#include <sysLib.h>
#include <vxlTarget.h>
#include <LtIpEchPackets.h>
#include <LtMD5.h>
#include <iLonSntp.h>
#include <LtIpPlatform.h>
#include "LtIpXmlConfig.h"

int	LtIpMasterTimeout( int a1, ... );
int LtIpBwAggTask( int a1, ... );


// Problems getting these in C++ with vxworks 6.2
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#if defined(ILON_100_ROUTER_DEMO) || INCLUDE_SELF_INSTALLED_MULTICAST
#ifdef linux
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#elif __VXWORKS__
#include "inetLib.h"
#else
#endif

#ifndef WIN32

#if defined(ILON_100_ROUTER_DEMO)
#include "ifLib.h"
extern "C"
{
extern STATUS ifAddrGet (char *interfaceName, char *interfaceAddress);
}; // extern "C"
#endif // ILON_100_ROUTER_DEMO

#if defined(__VXWORKS__)
#include "echelon\ilon.h"
#endif
#endif // WIN32
#endif // ILON_100_ROUTER_DEMO || INCLUDE_SELF_INSTALLED_MULTICAST


#if defined(_DEBUG) && !defined(LTA_EXPORTS)
// FB: this causes link problems if not linked with MFC
#if 0
void* __cdecl operator new(size_t nSize, char const* lpszFileName, int nLine);
void __cdecl operator delete(void* p, char const* lpszFileName, int nLine);
#define DEBUG_NEW new(THIS_FILE, __LINE__)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

// define the following to test sweep and handling of our channel routing packet
//#define TESTSWEEP 1

// define the following to test segmented packets
//#define TESTSEGS 1


/////////////////////////////////////////////////////////////////////////////
LtIpConnectState::eLTIP_CONNECT_STATE LtIpConnectState::GetState()
{
	return (LtIpMaster::getMaster())->getConnectState();
}


/************************************************************************

  The Ip Master performs the following functions:

  Communicates with the Config Server to obtain all RFC data. This includes
  Config data for this device	- Device Config Request / Response
  Channel Membership			- List of other devices on this "channel"
  Channel Routing				- list of routing packets for the other
								  routers on this channel
  SendList						- does not request and ignores if sent

  Communiates with the Router FarSide to obtain current Routing data for
  this router and inform the ConfigServer.

  Creates an LRE client for each channel member, other than ourselves,
  and connects it to the server.

  Satisfies all routing data requests for all these clients. These requests
  are forwarded to this object from all its clients.

 ************************************************************************/

// #define DEBUG_LOCKS must be defined in the header file or project

#ifdef DEBUG_LOCKS // test for lock deadlocks

void	LtIpMaster::xLock( char* pszFile, int nLine )
{	lock();
	int i = ++m_nLockDepth;
	if ( i >= LOCKDEPTHMAX )
	{	i = LOCKDEPTHMAX-1;
		m_nLockDepth = i;
		vxlReportEvent("xLock - lock depth overflow at %s %d\n", pszFile, nLine);
	}
	m_pszLockFile[i] = pszFile;
	m_nLockLine[i] = nLine;
}

void	LtIpMaster::xUnlock( char* pszFile, int nLine )
{	unlock();
	int i = --m_nLockDepth;
	if ( i<0 )
	{	vxlReportEvent("xUnlock - lock depth underflow at %s %d\n", pszFile, nLine);
		i = 0;
		m_nLockDepth = 0;
	}
	m_pszLockFile[i] = NULL;
	m_nLockLine[i] = 0;
}

void	LtIpMaster::xReportLockStack()
{

	int		i;
	int		j = 0;

	for ( i=0; i< LOCKDEPTHMAX && i <= m_nLockDepth ; i++ )
	{
		vxlReportEvent("LockStack [%d] %s; %d\n", i, m_pszLockFile[i], m_nLockLine[i] );
		j++;
	}
	if ( j == 0 )
	{	vxlReportEvent("LockStack empty\n");
	}
}

#define lock() xLock( __FILE__, __LINE__ )
#define unlock() xUnlock( __FILE__, __LINE__ )
#endif // test for lock deadlocks

//////////////////////////////////////////////////////////////////////
// LOCAL DATA
//////////////////////////////////////////////////////////////////////

// static pointer to the master object
static	LtIpMaster*		theMaster = NULL;

const boolean LtIpMaster::bIncludeLinkStatistics = true;
const boolean LtIpMaster::bNoLinkStatistics = false;

FUNCPTR LtIpXmlConfigBootstrapCallbackInstall = NULL;
FUNCPTR LtIpXmlConfigBootstrapCallbackUninstall = NULL;

//////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////

//
// maximizeNodeType
//
// make sure the node type we are using is the maximum of
// any we have seen according to LonTalk semantics
//
static int maximizeNodeType( LtRouterType rt, int nt )
{
	int		nrt = (int) rt;
	int		nnrt;
	if ( nt <= 3 && nrt <= 3 )
	{	nnrt = MAX( nrt, nt );
	}
	else
	{	nnrt = nrt;
	}
	return	nnrt;
}
#if 0 // not now used
#undef FREEANDCLEAR
#define FREEANDCLEAR( a) TSTFreeAndClear( a ); a = NULL;

static void TSTFreeAndClear( void* pFoo )
{
	int*	pBar = (int*)pFoo;
	if ( pFoo )
	{
		vxlReportEvent( "TSTFreeAndClear - 0x%08x -> 0x%08x\n", pFoo, *pBar );
		if ( *pBar != 0xFDFDFDFD )
		{	::free( pFoo );
		}
	}
}
#endif // not active

//////////////////////////////////////////////////////////////////////
// Static methods
//////////////////////////////////////////////////////////////////////

//
//
// getTimestamp
//
// return the milliseconds of absolute time. Allow for wrap around
// and to make this faster, just get the full time every 1000 ticks.
// During the 1000 ticks, correct the time by adjusting for the tick count.
//
ULONG	LtIpMaster::getTimestamp(boolean bSyncCheck)
{
	return LtIpPktHeader::getTimestamp(bSyncCheck);
}

//
// getDatetime
//
// return the datetime to use for the configuration packet we are
// about to create
//
ULONG	LtIpMaster::getDatetime(boolean writePersist)
{
	ULONG	dtNow;

	dtNow = LtIpPktHeader::getDateTime();
	if ( dtNow <= m_lastDatetime )
	{
		dtNow = m_lastDatetime + 1;
	}
	if (writePersist)
	{
		m_lastDatetime = dtNow;
		doWriteNvRam();
	}
	return dtNow;
}

//
// getSession
//
// The session is saved in persistent storage and retrieved
// It is stable until next time we boot.
// The LTMasterObject needs to update it's value in stable storage each time we boot.
//
ULONG	LtIpMaster::getSession()
{
	return m_nSession;
}


//
// newSession
//
// Create a new session number.  This needs to be done any time communication
// is being established with a target.  This includes after rebooting or when
// a target is initially added to the member list.
//
void	LtIpMaster::newSession()
{
	m_nSession++;
	doWriteNvRam();
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

LtIpMaster::LtIpMaster( ULONG ipAddress, word ipPort ) :
m_ipAddrLocal(ipAddress),
m_ipPortLocal(ipPort),
m_qRFCin(true)	// this is a head
{
	byte defAuthKey[AUTHENTIC_SECRET_SIZE];

#ifndef LTA_EXPORTS
	// don't assert if we are running in the VNISTACK.DLL
	assert( theMaster == NULL );
#endif // LTA_EXPORTS
	theMaster = this;

	m_pServer			= NULL;
	m_pChannel			= NULL;
	m_pLonLink			= NULL;
	m_natIpAddr			= 0;
	m_ipAddrCfgServer	= 0;
	m_ipAddrNtpServer	= 0;
	m_ipAddrNtpServer2	= 0;
	m_ipPortCfgServer	= 0;
	m_ipPortNtpServer	= 0;
	m_ipPortNtpServer2	= 0;

	m_updateCsHostName  = false;
	memset(m_lastKnownCsHost, 0, sizeof(m_lastKnownCsHost));
	m_lastKnownCsPort	= 0;

	m_nAggregateMs		= 64;
	m_bAggregate		= false;
	m_bBWLimit			= false;
	m_nBWLimitKBPerSec	= 1000;
	m_nChannelTimeout	= 1000;		// default to one second
	m_bCheckStale		= true;
	m_bReorderPackets	= true;
	m_nEscrowTimeMs		= 200;
	m_bAuthenticate		= false;
	memset( m_aAuthenticSecret, 0, sizeof(m_aAuthenticSecret) );
	m_nBWLastClient		= 0;

	m_nTOSbits			= 0;					// type of service bits
	m_bUseTOSbits		= false;				// whether to use them

	// these items are in NVRAM
	m_nSession			= 0;		// session number
	m_lastDatetime		= 0;		// last date time of a config packet
#ifdef ILON_100_ROUTER_DEMO
	m_prevIpAddrLocal	= 0;
#endif
	m_tidWorker			= 0;
	m_tidBwAggTask		= 0;
	m_tidCheckStuck		= 0;
	m_tidWritePersist	= 0;
	m_mqWritePersist	= msgQCreate( 20, 8, MSG_Q_FIFO );
	m_mStaticConfig		= false;
	m_bTaskExit			= false;
	m_semWork			= semBCreate( SEM_Q_FIFO, SEM_EMPTY );
	//vxlReportEvent("LtIpMaster - work semaphore 0x%08x\n", m_semWork );
	//vxlReportEvent("LtIpMaster - lock semaphore 0x%08x\n", m_lock );
	m_tPending			= wdCreate();
	m_tRequest			= wdCreate();

	m_bStopping			= false; // not stopping
	m_nWorkMask			= 0;
	m_nTimerMask		= 0;
	m_nReqtMask			= 0;
	m_nReqtCount		= 0;
	memset( m_baReqtCR, 0, sizeof(m_baReqtCR) );
	m_bDataValid		= false;
	m_nDataIncarnation	= 0;
	m_nMembers			= 0;
	m_hasSharedIpAddrs	= false;
	m_nOurIndex			= -1;
	m_nIndex			= 0;
	m_pktChanMembers	= NULL;
	m_nPktCMSize		= 0;
	m_dtPktCM			= 0;	// no date time either
	m_pktDevRegister	= NULL;
	m_pktOurChanRouting	= NULL;
	m_nPktOCRSize		= 0;
	m_wantsAllPackets	= false;

    m_selfInstalledMcastAddr = 0;
	m_actualIpAddrLocal = m_ipAddrLocal;
    m_selfInstalledMcastHops = 1;
    m_apMcastClient = 0;

    memset( m_apdMembers, 0, sizeof(m_apdMembers) );
	memset( m_pktChanRouting, 0, sizeof(m_pktChanRouting) );
	memset( m_apClients, 0, sizeof(m_apClients) );
	memset( m_acName, 0, sizeof(m_acName) );

	m_runningEchProtocolVer = LTIP_ECH_VER2;
	m_bStrictEia852		= true;
	m_tickLastSendChanRouting = 0;		// Last time we sent channel routing packet

	m_dtStatisticsReset = LtIpPktHeader::getDateTime();

	m_cnfgServerType = CS_TYPE_UNKNOWN;
	m_cnfgSrvrTypeChkCount = 0;
	m_CSmsgRcvd = 0;
	m_CSmsgRcvdCheckpoint = 0;

	m_loadXmlConfig_Callback = NULL;
	m_dumpXmlConfig_Callback = NULL;
	if (LtIpXmlConfigBootstrapCallbackInstall != NULL)
	{
		LtIpXmlConfigBootstrapCallbackInstall(this);
	}

	m_unknownSrcPortDiag.ipAddr = 0;
	m_unknownSrcPortDiag.ipPort = 0;
	m_unknownSrcPortDiag.timestamp = 0;
	m_unknownSrcPortDiag.throttleCount = 0;

	clearCounts();

	// establish the allocator for the segmentor objects
	// the allocator is built by the IPBase class.
	m_segOut.setAllocator( m_pAlloc );
	m_segIn.setAllocator( m_pAlloc );
	m_segOut.setActive( true );
	m_segIn.setActive( true );
	setSegmentorExtHdrData();
	setMembersEIA852Auth();
	memset(defAuthKey, 0, sizeof(defAuthKey));	// set default auth key of all zero
	setAuthenticSecret(defAuthKey);

	// set the names of persistence in case noone else does
	setIndex( 0 );

	// set the reordering data to OFF
	m_bTReorder			= false;
	m_bTReorderRandom	= false;
	m_bTReorderDelete	= false;
	m_nTReorderEvery	= 0;
	m_nTReorderBy		= 0;
    m_bPersistenceRead = false;

#ifdef DEBUG_LOCKS
	m_nLockDepth = 0;
	memset( m_pszLockFile, 0, sizeof(m_pszLockFile) );
	memset( m_nLockLine, 0, sizeof(m_nLockLine) );
#endif // DEBUG_LOCKS
}

//
// getMaster
//
// return address of the master
//
LtIpMaster* LtIpMaster::getMaster()
{	return theMaster;
}

//
// deletePersistence
//
// delete all persistence files
//
void LtIpMaster::deletePersistence()
{
	LtIpPersist		pst;
	long			junk = 0;

	// delete the persistence file
	pst.deleteFile( m_cPersistName );

	// write a bogus NV ram contents
	LtNvRam::set( m_szNvRamKey, (byte*)&junk, sizeof(junk), FALSE );

}


//
// setLocalIpAddr
//
// Get the local ip address
//
#ifdef WIN32
	// on Windows only, validate the IP address we have as a valid address on this node
	// special routine in VxWinSockets.c for just this purpose
	extern "C" boolean	vxsValidateIPAddress( LPSTR pszName, ULONG ipAddr );
#endif // WIN32
boolean	LtIpMaster::setLocalIpAddr()
{

	STATUS			vxsts;
	LtIpAddressStr	ips;
	VXSOCKADDR		psad = NULL;
	char			szHostName[32];
#ifndef WIN32
	ULONG			tempIpAddrLocal;
	int             addressCount = 0;
	boolean         validLocalAddr = false;
#endif
	boolean			bOk = false;

	// If IP address was not set by our creator, then default
	// to the machine's IP address for its own host name.
	if ( m_ipAddrLocal == 0 )
	{
		do
		{
#ifdef ILON_100_ROUTER_DEMO
#ifndef WIN32
			char pppAddrStr[25];
			if (sysLwipRouterUsesPPP())
			{
				if (ifAddrGet("ppp1", pppAddrStr) != ERROR)
				{
					m_ipAddrLocal = vxsInetAddr(pppAddrStr);
					vxlReportEvent("Getting local IP address from PPP\n");
					bOk = true;
				}
			}
			else
#endif
#endif // ILON_100_ROUTER_DEMO
			{
#ifndef WIN32
				vxsts = getFirstLocalInterfaceAddress(&tempIpAddrLocal, &addressCount);
				if ( vxsts != OK )
				{	vxlReportEvent("LtIpMaster::setLocalIpAddr - getFirstInterfaceAddress failed\n");
					break;
				}
				if (addressCount == 1)
				{
					// only one local IP interface - use it!
					m_ipAddrLocal = tempIpAddrLocal;
				}
				else
#endif
				{
					// avoid need to look up our IP address
					vxsts = vxsGetHostName( szHostName, 32 );
					if ( vxsts != OK )
					{	vxlReportEvent("LtIpMaster::setLocalIpAddr - vxsGetHostName failed\n");
						break;
					}
					vxlReportEvent("LtIpMaster::setLocalIpAddr - vxsGetHostName %s\n", szHostName);
					psad = vxsAddrName( szHostName );
					if ( psad == NULL )
					{	vxlReportEvent("LtIpMaster::setLocalIpAddr - vxsAddrName failed\n");
						break;
					}
					m_ipAddrLocal = vxsAddrGetAddr( psad );
					vxsFreeSockaddr( psad );
#ifndef WIN32
					// Check to make sure that the host addr is valid
					vxsts = validInterfaceAddress(m_ipAddrLocal, &validLocalAddr);
                    if ( vxsts != OK )
					{	vxlReportEvent("LtIpMaster::setLocalIpAddr - validInterfaceAddress failed\n");
						break;
					}
					if (!validLocalAddr)
					{	vxlReportEvent("LtIpMaster::setLocalIpAddr - address is not valid\n");
						break;
					}
#endif
				}
				bOk = true;
			}
			ips.setIaddr( m_ipAddrLocal );
			vxlReportEvent("LtIpMaster::setLocalIpAddr - local IP address %s\n", ips.getString() );

		} while (false);
	}
	else
	{	bOk = true;
	}
#ifdef WIN32
	// on Windows only, validate the IP address we have as a valid address on this node
	// special routine in VxWinSockets.c for just this purpose
	do
	{
		bOk = false;
		vxsts = vxsGetHostName( szHostName, 32 );
		if ( vxsts != OK )
		{	vxlReportEvent("LtIpMaster::setLocalIpAddr - vxsGetHostName failed\n");
			setChannelError( LT_INVALID_IPADDRESS );
			break;
		}
		if ( !vxsValidateIPAddress( szHostName, m_ipAddrLocal ) )
		{
			ips.setIaddr( m_ipAddrLocal );
			vxlReportUrgent("LtIpMaster::setLocalIpAddr - local IP address %s is INVALID\n",
							ips.getString() );
			setChannelError( LT_INVALID_IPADDRESS );
			break;
		}
		bOk = true;
	} while (FALSE );

#endif // WIN32
	return bOk;
}

#ifdef ILON_100_ROUTER_DEMO
void LtIpMaster::clearLocalIpAddr()
{
	m_ipAddrLocal = 0;
}
#endif // ILON_100_ROUTER_DEMO

ULONG LtIpMaster::getExternalIpAddr()
{
	ULONG ipAddr;

    if (m_natIpAddr == 0)
		ipAddr = m_ipAddrLocal;	// Normal value
	else
		ipAddr = m_natIpAddr;	// NAT value

	return(ipAddr);
}

LtIpMaster::~LtIpMaster()
{
	//lock();

	if (LtIpXmlConfigBootstrapCallbackUninstall != NULL)
	{
		LtIpXmlConfigBootstrapCallbackUninstall(this);
	}

#ifdef TESTSEGS
	vxlReportEvent("~LtIpMaster - deleteWorkerTask %d\n", tickGet() );
#endif // TESTSEGS

	// clean up all worker related stuff
	//unlock();	// unlock to allow tasks to finish
	deleteWorkerTask();
#ifdef TESTSEGS
	vxlReportEvent("~LtIpMaster - waiting on lock %d\n", tickGet() );
#endif // TESTSEGS
	lock();

	removeAllClients();
    removeMcastClient();

#ifdef TESTSEGS
	vxlReportEvent("~LtIpMaster - clients removed %d\n", tickGet() );
#endif // TESTSEGS
	cleanPackets();
#ifdef TESTSEGS
	vxlReportEvent("~LtIpMaster - packets cleaned %d\n", tickGet() );
#endif // TESTSEGS

	if ( m_pChannel )
	{
		// remove ourselves from the channel as an event client
		m_pChannel->deregisterEventClient( this );
	}
	unlock();
#ifdef TESTSEGS
	vxlReportEvent("~LtIpMaster - lock released %d\n", tickGet() );
#endif // TESTSEGS

	msgQDelete(m_mqWritePersist);

	// the allocator and link are taken care of by our base class
	theMaster = NULL;

#ifdef WIN32
    SntpServerList::remove(m_nIndex);
#endif
}

// Set the init object from whence all other information flows
void	LtIpMaster::setChannel( LtLogicalChannel* pChannel )
{
	//  find our server
	m_pChannel = pChannel;
	m_pServer = m_pChannel->getLre();
}


//
// setLocalPort
//
// the following is a local override for the port.
// static function
//
boolean	LtIpMaster::setLocalPort( word ipPort )
{
	boolean				bOk = false;
	boolean				bChange;
	LtIpDevRegister		dvr;
	int					nBytes = 0;
	word				nOldPort;

	lock();
	do
	{
		if (ipPort == m_ipPortLocal) break;	// do nothing if it is the same
		// build a new device registration packet, then
		// parse it again, and then
		// set the new servers, which stops and restarts the link.
		// if there was a change, write the persistence to remember it
		// save the old port, and then set the new one to build the message
		nOldPort = m_ipPortLocal;
		m_ipPortLocal = ipPort;
		nBytes = buildDevRegister( true );
		if ( nBytes == 0 ) break;
		bOk = dvr.parse( m_pktDevRegister, false );
		if ( !bOk ) break;
		// put the old port back so the compare will fail in
		// setNewServers function.
		m_ipPortLocal = nOldPort;
		bChange = setNewServers( dvr );
		if ( bChange )
		{	startWorkerTask( WORK_WritePersist | WORK_SendDevRegister);
		}
	} while ( false );

	unlock();

	generatePropertyChangeHostEvent(); // for iLON

	return bOk;
}

void LtIpMaster::setNatAddress(ULONG natIpAddr)
{
	if (natIpAddr != m_natIpAddr)
	{
		lock();
		m_natIpAddr	= natIpAddr;
		setSegmentorExtHdrData();
		buildDevRegister(false);
		startWorkerTask( WORK_SendDevRegister | WORK_WritePersist | WORK_SendChanRouting );
		unlock();
	}
}

void LtIpMaster::setStrictEia852(boolean strictEia852)
{
	if (strictEia852 != m_bStrictEia852)
	{
		lock();
		m_bStrictEia852 = strictEia852;
		setSegmentorExtHdrData();
		buildDevRegister(false);
		startWorkerTask( WORK_SendDevRegister | WORK_WritePersist | WORK_SendChanRouting );
		unlock();
	}
}

void LtIpMaster::setEchProtocolVersion(LtIpEchProtocolVersion ver)
{
	if (ver != m_runningEchProtocolVer)
	{
		lock();
		m_runningEchProtocolVer = ver;
		setMembersEIA852Auth();
		setSegmentorExtHdrData();
		buildDevRegister(false);
		startWorkerTask( WORK_SendDevRegister | WORK_WritePersist | WORK_SendChanRouting );
		unlock();
	}
}

//
// setConfigServer
//
// the following is a local override for the config server's address.
//
boolean	LtIpMaster::setConfigServer( ULONG ipAddr, word ipPort, boolean updateLastKnownHost )
{
	boolean				bOk = false;
	boolean				bChange;
	LtIpDevRegister		dvr;
	int					nBytes = 0;
    ULONG               nOldIpAddr;
	word				nOldPort;

	lock();
	do
	{	// build a new device registration packet, then
		// parse it again, and then
		// set the new servers, which stops and restarts the link.
		// if there was a change, write the persistence to remember it
		// save the old port, and then set the new one to build the message
		nOldIpAddr = m_ipAddrCfgServer;
        nOldPort = m_ipPortCfgServer;
        m_ipAddrCfgServer = ipAddr;
        m_ipPortCfgServer = ipPort;
		nBytes = buildDevRegister( true );
		if ( nBytes == 0 ) break;
		bOk = dvr.parse( m_pktDevRegister, false );
		if ( !bOk ) break;
		// put the old port back so the compare will fail in
		// setNewServers function.
		m_ipAddrCfgServer = nOldIpAddr;
		m_ipPortCfgServer = nOldPort;
		bChange = setNewServers( dvr );
		if ( bChange )
		{
			if (updateLastKnownHost)
			{
				m_updateCsHostName = true;
			}
			startWorkerTask( WORK_WritePersist);
			// Can't use startWorkerTask() to request the CS type
			startRequestInfo( REQT_DEVRESPONSE | REQT_CSTYPE, true );
		}
	} while ( false );

	unlock();
	return bOk;
}

// Overloaded version of above, with string for host name/ip addr
boolean	LtIpMaster::setConfigServer( char* pszServer, word ipPort )
{
    ULONG ipAddr;
	boolean bOk = false;
	boolean bHostChanged = false;

	if (strcmp(pszServer, m_lastKnownCsHost) != 0)
		bHostChanged = true;
	strncpy(m_lastKnownCsHost, pszServer, min(sizeof(m_lastKnownCsHost), strlen(pszServer)+1));
	m_lastKnownCsHost[sizeof(m_lastKnownCsHost)-1] = 0;
	m_lastKnownCsPort = ipPort;

	ipAddr = vxsInetAddr(pszServer);
	if ((STATUS)ipAddr != ERROR)
	{
		if ((ipAddr == m_ipAddrCfgServer) && bHostChanged)
		{
			// Force writing the persistent data for the host string
			startWorkerTask( WORK_WritePersist);
		}
		bOk = setConfigServer(ipAddr, ipPort, false);
	}
	else
	{
		ipAddr = hostGetByName(pszServer);
		if ((STATUS)ipAddr != ERROR)
		{
			if ((ipAddr == m_ipAddrCfgServer) && bHostChanged)
			{
				// Force writing the persistent data for the host string
				startWorkerTask( WORK_WritePersist);
			}
			bOk = setConfigServer(ipAddr, ipPort, false);
		}
	}

	return bOk;
}

//
// startConfigServerCheck
//
// the following starts up a "config server communication" check. Optionally
// sets the config server address.  Then sends a request for DEV RESPONSE from
// the config server.  The calling app can check to see if its complete by calling
// getConfigServerCheckComplete periodically.
//

boolean LtIpMaster::startConfigServerCheck( ULONG csIpAddr, word csIpPort )
{
    boolean started = TRUE;
	int requestType = REQT_DEVRESPONSE;

    if (csIpAddr != 0)
    {
        started = setConfigServer(csIpAddr, csIpPort);
		requestType |= REQT_CSTYPE;	// check what type of CS this is
    }
    if (started)
    {
        startRequestInfo( requestType, FALSE );
    }
    return(started);
}

//
// getConfigServerCheckComplete
//
// Return TRUE when if the check is complete (have communicated with the VNI server,
// FALSE otherwise.  Also return the current CS IP and port.
//

boolean LtIpMaster::getConfigServerCheckComplete(ULONG &csIpAddr, word &csIpPort)
{
	getConfigServer(csIpAddr, csIpPort);
		// Initiated a request for a DEVRESPONSE.
		// Complete if it is no longer pending.
	boolean complete = (m_nReqtMask & REQT_DEVRESPONSE) || csIpAddr == 0 || csIpPort == 0 ? FALSE : TRUE;
	return(complete);
}

// Extended version with diagnostic logic
CsCommTestSts LtIpMaster::startConfigServerCheckEx(ULONG newAddr, word newPort)
{
	char lastHost[64];
	word lastPort;
	ULONG lastAddr;
	word realPort;
	ULONG realAddr;

	m_CsCommTestSts = CsCommTest::Incomplete;

	getLastKnownConfigServer(lastHost, lastPort);
	getConfigServer(realAddr, realPort);
	if (newAddr != 0)
	{
		// Use the supplied values
		startConfigServerCheck(newAddr, newPort);
	}
	else if (realAddr != 0)
	{
		// Use configured values
		startConfigServerCheck(0, 0);
	}
	else
	{
		// Try the "last known" values again
		lastAddr = vxsInetAddr(lastHost);
		if ((lastHost[0] == 0) || (lastAddr == 0))
		{
			m_CsCommTestSts = CsCommTest::NoAddr;
		}
		else
		{
			if (((STATUS)lastAddr == ERROR))
			{
				lastAddr = hostGetByName(lastHost);
			}
			if ((STATUS)lastAddr == ERROR)
			{
				// DNS failure
				m_CsCommTestSts = CsCommTest::DnsFailed;
			}
			else
			{
				startConfigServerCheck(lastAddr, lastPort);
			}
		}
	}

	return m_CsCommTestSts;
}

// Extended version with diagnostic logic
CsCommTestSts LtIpMaster::getConfigServerCheckCompleteEx(ULONG &csIpAddr, word &csIpPort)
{
	ULONG realAddr;
	word realPort;

	if (m_CsCommTestSts == CsCommTest::Incomplete)
	{
		if (getConfigServerCheckComplete(csIpAddr, csIpPort))
		{
			m_CsCommTestSts = CsCommTest::OkActive;
		}
		else
		{
			getConfigServer(realAddr, realPort);
			if (realAddr == 0)
			{
				// It completed, but the CS zeroed the address
				m_CsCommTestSts = CsCommTest::OkNotActive;
			}
		}
	}

	return m_CsCommTestSts;
}

// Do the exteded CS tests above and return result
CsCommTestSts testConfigServerComm(int waitSecs)
{
	LtIpMaster* pMaster = LtIpMaster::getMaster();
	CsCommTestSts sts;
	ULONG ipAddr;
	word ipPort;

	// Constrain: 0 <= waitSecs <= 60
	waitSecs = max(0, waitSecs);
	waitSecs = min(60, waitSecs);
	sts = pMaster->startConfigServerCheckEx(0, 0);
	if (sts == CsCommTest::Incomplete)
	{
		do
		{
			sts = pMaster->getConfigServerCheckCompleteEx(ipAddr, ipPort);
			if (sts == CsCommTest::Incomplete)
			{
				taskDelay(sysClkRateGet());
			}
			else
			{
				break;
			}
		}
		while (waitSecs-- > 0);
	}

	if (sts == CsCommTest::Incomplete)
	{
		sts = CsCommTest::Failed;
	}

	return sts;
}

#define WAIT_FOR_PENDING_INTERFACE_UPDATES_SLEEP_INTERVAL 10
#define WAIT_FOR_PENDING_INTERFACE_UPDATES_TIMEOUT 10000

// Presumably this is fairly rare, so the "busy loop" is probably acceptable.
LtErrorType LtIpMaster::waitForPendingInterfaceUpdates(void)
{
	ULONG startTime = tickGet();

	while (!m_bStopping && m_nReqtMask != 0 &&
		   (tickGet() - startTime) < WAIT_FOR_PENDING_INTERFACE_UPDATES_TIMEOUT)
	{
		taskDelay(msToTicksX(WAIT_FOR_PENDING_INTERFACE_UPDATES_SLEEP_INTERVAL));
	}

	if (m_nReqtMask != 0)
	{
		return LT_LOCAL_MSG_FAILURE;
	}

	return LT_NO_ERROR;
}

//
// removeAllClients
//
// delete all our clients on the way out
//
void LtIpMaster::removeAllClients()
{
	int		i;

	for ( i=0; i<MAX_MEMBERS; i++ )
	{
		if ( m_apClients[i] )
		{	removeClient( i );
		}
	}
}

//
// cleanPackets
//
// clean out all the packets we are saving
//
void LtIpMaster::cleanPackets()
{
	int		i;
	m_bDataValid = false;

	FREEANDCLEAR( m_pktChanMembers );
	m_nPktCMSize = 0;
	m_dtPktCM = 0;
	FREEANDCLEAR( m_pktDevRegister );
	FREEANDCLEAR( m_pktOurChanRouting );
	m_nPktOCRSize = 0;
	for ( i=0; i< MAX_MEMBERS; i++ )
	{
		FREEANDCLEAR(m_pktChanRouting[i] );
	}
}


// LtEventClient methods
// we are an event client to receive notification from the far-side
// about routing changes.

void LtIpMaster::eventNotification(LtEventServer* pServer)
{
	if (pServer == (LtEventServer*)m_pChannel)
	{
		lreClientChange();
	}
	// If we don't yet have a worker task, then we can create one now
	if ( !m_bTaskExit && m_tidWorker == 0 )
	{	startWorkerTask( 0 );
	}
	// get the new farside routing information
	// make an RFC Channel Routing packet, and ship it to the Config Server
	// and save it locally.
	// but don't do any of this stuff really soon since we expect more
	// configuration changes.
#if TESTSWEEP
	vxlPrintf( "eventNotification - received event\n");
#endif // TESTSWEEP
	startTimedWork( WORK_SendChanRouting, CR_HOLDDOWN_MS );
}

//
// lreClientChange
// some change to some client somewhere
//
void	LtIpMaster::lreClientChange()
{
	// just register again with all clients
	registerWithLreClients();
}

//
// registerWithLreClients
//
// register with any LreClients on the same channel
//
void	LtIpMaster::registerWithLreClients()
{
	// Connect with the local clients
	// and make us an event Client to receive routing updates
	LtLreClient*	pClient;
    LtVectorPos     pos;
	if ( m_pChannel )
	{
		lockChannel();
		while ( m_pChannel->enumStackClients( pos, &pClient ) )
		{
			if ( pClient )
			{	// deregister first to allow idempotency
				pClient->deregisterEventClient( this );
				pClient->registerEventClient( this );
			}
		}
		unlockChannel();
	}
}

boolean LtIpMaster::setSelfInstalledMcastAddr(ULONG mcastAddr)
{
	boolean bOk = false;
#if INCLUDE_SELF_INSTALLED_MULTICAST
	m_selfInstalledMcastAddr = mcastAddr;
	bOk = true;
#endif	// INCLUDE_SELF_INSTALLED_MULTICAST
	return bOk;
}


//
// start
//
// Start the client operation by checking that all is well
// with the link and then queueing receives to the link.
//
boolean		LtIpMaster::start()
{
	boolean			bOk = false;
	LtSts			sts;
	LtIpAddressStr	ias1;
	LtIpAddressStr	ias2;
	LtIpAddressStr	ias3;
	LtIpAddressStr	ias4;
	boolean			bXmlConfig;

	lock();

	do
	{
		vxlReportEvent("LtIpMaster::start - index %d ipAddr %s port %d\n",
						m_nIndex, ias1.getString( m_ipAddrLocal ), m_ipPortLocal );
		if (m_natIpAddr != 0)
		{
			vxlReportEvent("LtIpMaster::start - External NAT ipAddr %s\n",
						ias1.getString( m_natIpAddr) );
		}
		// already active??
		if ( isActive() ) break;
		// set our own local ip address
		bOk = setLocalIpAddr();
		// dont start if the IP address or port are zero
		if ( !bOk )
		{
			vxlReportEvent("LtIpMaster::start - Problem in setLocalIpAddr\n");
			break;
		}
		if ( m_ipAddrLocal == 0 || m_ipPortLocal == 0 )
		{
			vxlReportEvent("LtIpMaster::start - IPAddress or port are zero\n");
			setChannelError( LT_INVALID_IPADDRESS );
			break;
		}
#if INCLUDE_SELF_INSTALLED_MULTICAST
		// Save the real value for later. m_ipAddrLocal will be switched to match the config
		m_actualIpAddrLocal = m_ipAddrLocal;
#endif

		// restore all the persistant data synchronously
		// since we need the information

		// First try loading it from from an XML file (manual configuration)
		bXmlConfig = loadXmlConfig();

		//doReadPersist();
		// *** server is not ready for clients that we will create yet. ***
		// do not run the sntp client when we are on windows
		// get and bump the session id on the NVRam along with the latest
		// datetime for building configuration packets

		// Get the server information from "NVRAM" data.
		// If we read the config from XML, only load selected data.
		doReadNvRam( !bXmlConfig );

		// Bump the session number and save it to "nvram".
		newSession();

// EPANG TODO - don't support SNTP yet in Linux iLON
#if defined(__VXWORKS__) || defined(WIN32)
		// servers have been read in from NVram, now we can set them
		iLonSetTimeServersFromIP852(m_ipAddrNtpServer, m_ipPortNtpServer,
								  m_ipAddrNtpServer2, m_ipPortNtpServer2);
#endif

		if ( m_pLink == NULL )
		{
			vxlReportEvent("LtIpMaster::start - m_pLink == NULL\n");
			setChannelError( LT_NO_LINK );
			break;
		}

		if (selfInstalledMcastMode())
		{
            ((CIpLink*)m_pLink)->setSrcIp( m_actualIpAddrLocal, m_ipPortLocal );
            ((CIpLink*)m_pLink)->setDstIp( m_selfInstalledMcastAddr, m_ipPortLocal ); // same with the local interface       
            ((CIpLink*)m_pLink)->setSelfInstalledMcastAddr( m_selfInstalledMcastAddr );
            ((CIpLink*)m_pLink)->setSelfInstalledMcastHops( m_selfInstalledMcastHops );        
        }
        else
        {
		    ((CIpLink*)m_pLink)->setSrcIp( m_ipAddrLocal, m_ipPortLocal );
            ((CIpLink*)m_pLink)->setDstIp( m_ipAddrCfgServer, m_ipPortCfgServer );
        }

		((CIpLink*)m_pLink)->setNoHeader( true );
		((CIpLink*)m_pLink)->setLtIpMaster(this);

		// setup the segmentors with a link object to send
		m_segOut.setLink( (CIpLink*)m_pLink );
		m_segIn.setLink( (CIpLink*)m_pLink );

		vxlReportEvent( "LtIpMaster::start - Local %s %d Cfg %s %d\n"
						"                    Time %s %d %s %d\n",
						ias1.getString(m_ipAddrLocal), m_ipPortLocal,
						ias2.getString(m_ipAddrCfgServer), m_ipPortCfgServer,
						ias3.getString(m_ipAddrNtpServer), m_ipPortNtpServer,
						ias4.getString(m_ipAddrNtpServer2), m_ipPortNtpServer2
						);

  		sts = m_pLink->open("Ignored");
		if ( sts != LTSTS_OK )
		{
			vxlReportEvent("LtIpMaster::start - cant open the link\n");
			setChannelError( LT_CANT_OPEN_IP_LINK );
			bOk = false;
			break;
		}
  
		// Call the base class to start the link
		bOk = LtIpBase::start();
		if ( !bOk )
		{
			vxlReportEvent("LtIpMaster::start - cant start the link\n");
			setChannelError( LT_CANT_OPEN_IP_LINK );
			break;
		}
		
        if (selfInstalledMcastMode())
            // Create the one and only one MULTICAST Ip client
            createMcastClientRouting();

		// set so we will read persistence before writing any but
		// don't start the process until server triggers us with notification
		m_nWorkMask |= WORK_ReadPersist;

		// Request for Device Config Response but don't automatically request Channel Membership
		// Let the Channel Membership request be based on CHM datetime in the Device Config Response
		// startRequestInfo( REQT_DEVRESPONSE | REQT_CHANMEMBERS, true );
		startRequestInfo( REQT_DEVRESPONSE, true );

	} while ( false );

	unlock();
	return bOk;
}


//
// stop
//
// Stop the link with a reset
//
boolean		LtIpMaster::stop()
{
	boolean		bOk = false;

	m_bStopping = true;
#ifdef TESTSEGS
	vxlReportEvent("LtIpMaster::stop - acquiring lock %d\n", tickGet() );
#endif // TESTSEGS
	lock();
#ifdef TESTSEGS
	vxlReportEvent("LtIpMaster::stop - acquired lock, deregister clients%d\n", tickGet() );
#endif // TESTSEGS

	// Disconnect with the local clients so we do not receive routing updates
	LtLreClient*	pClient;
    LtVectorPos     pos;
	if ( m_pChannel )
	{
		lockChannel();
		while ( m_pChannel->enumStackClients( pos, &pClient ) )
		{
			if ( pClient )
			{	pClient->deregisterEventClient( this );
			}
		}
		unlockChannel();
	}
#ifdef TESTSEGS
	vxlReportEvent("LtIpMaster::stop - clients deregistered %d\n", tickGet() );
#endif // TESTSEGS
	unlock();
	bOk = stopBase();
	return bOk;
}

//
// stopBase
//
// Special care needed because the segments need to be run down
// carefully to avoid deadlock.
//
boolean	LtIpMaster::stopBase()
{
	boolean		bOk = false;
	// must be unlocked here to allow other things to finish
	// that want access to the IpBase
#ifdef TESTSEGS
	vxlReportEvent("LtIpMaster::stopBase - purge segment requests %d\n", tickGet() );
#endif // TESTSEGS
	m_bStopping = true;
	// stop segments so we shut down quickly
	m_segOut.setActive( false );
	m_segIn.setActive( false );

	// purge all outstanding segment requests so we clean up in a hurry
	m_segIn.purgeAllRequests();
	m_segOut.purgeAllRequests();
#ifdef TESTSEGS
	vxlReportEvent("LtIpMaster::stopBase - base stop %d\n", tickGet() );
#endif // TESTSEGS
	bOk = LtIpBase::stop();
#ifdef TESTSEGS
	vxlReportEvent("LtIpMaster::stopBase - base stop done%d\n", tickGet() );
#endif // TESTSEGS
	m_bStopping = false;
	return bOk;
}

//
// RfcFromClient
//
// RFC message was received from a client
// This can only be a "request" for information from some other node
// so we must honor it.
// But if its configuration information from some other node, then discard it.
// Checks are made in the worker task.
// Note that this packet is one of ours, not a client packet. We have
// already done the copy to a master packet in the client routine.
//
void LtIpMaster::RfcFromClient( LtPktInfo* pPkt )
{
	packetReceived( false, pPkt, LTSTS_OK );
}

//
// packetReceived
//
// Packet received from the link. We need to process it.
//
void LtIpMaster::packetReceived(	boolean bPriority,
						LtPktInfo*	pPkt,
						LtSts sts)
{
	// if we are stopping, then ignore input to avoid deadlock
	// in segmentation, or anywhere else for that matter
	if ( !m_bStopping && sts == LTSTS_OK )
	{
		// lock not strictly required here since queues are all locked
		//lock();
		countPacketsReceived();

		// on inbound segments, we process the segments and queue the final message
		// for processing. Avoid queuing the worker thread until we have a complete
		// segment.

		LtIpSegment		seg;
		LtPktInfo*		pPktDone = NULL;
		byte*			pData = pPkt->getDataPtr();
		int				nSize= 0;
		boolean			bOk = false;
		do
		{
			if ( seg.parse( pPkt->getDataPtr(), false ) )
			{
				nSize = seg.packetSize;
				// authenticate a segment packet before passing it on
				if ( isAuthenticating() )
				{
					if (backwardCompatibleChan() || seg.isMarkedSecure(pData))
						bOk = m_md5.checkDigestWithSecret( pData, nSize, &pData[nSize] );
					if ( !bOk )
					{	countAuthFailures();
						break; // just release the packet
					}
				}
				if ( seg.flags & LtIpSegment::FLAG_FINAL )
				{ // this is the last packet of the big test packet
					vxlReportEvent("Master::packetReceived - last segment arrived %d\n", seg.segmentId );
				}
				// we have a segment packet arriving.
				// pPkt is always gone if return true.
				if ( m_segIn.receivedPacket( pPkt, &pPktDone, pPkt->getIpSrcAddr(), pPkt->getIpSrcPort() ) )
				{
					if ( pPktDone )
					{
#ifdef TESTSEGS // segmentTest
					ULONG		ipAddr = pPkt->getIpSrcAddr();
					word		ipPort = pPkt->getIpSrcPort();
					LtIpAddressStr	ias;
					vxlReportEvent( "Master::packetReceived - segmented packet complt %d bytes from %s: %d\n",
									pPktDone->getDataSize(), ias.getString( ipAddr ), ipPort );
#endif // segment test
						pPktDone->setIpSrcAddr( pPkt->getIpSrcAddr() );
						pPktDone->setIpSrcPort( pPkt->getIpSrcPort() );
					}
#ifdef TESTSEGS // segmentTest
					else
					{
					ULONG		ipAddr = pPkt->getIpSrcAddr();
					word		ipPort = pPkt->getIpSrcPort();
					LtIpAddressStr	ias;
					vxlReportEvent( "Master::packetReceived - segmented packet reqId %4d seg %2d from %s: %d\n",
									seg.requestId, seg.segmentId, ias.getString( ipAddr ), ipPort );
					}
#endif // segment test

					pPkt = pPktDone;
				}
				else
				{	// packet has been dealt with by segmentation, forget it existed.
					pPkt = NULL;
				}
			}
			// if we have a packet, then it must be for the worker task
			// else, we escrowed the segment packet
			if ( pPkt )
			{
				// Note: no longer need to process reboot requests here.
				// The master will not deadlock.

				// RFC packet received from the Config server
				// so we need to process it and notify any clients affected.
				boolean bEmpty = m_qRFCin.insertHead( pPkt );
				if ( bEmpty )
				{
					startWorkerTask( 0 );		// if the queue was empty
				}
				pPkt = NULL;
			}
			//unlock();
		} while ( false );
	} // if

	if ( pPkt )
	{	pPkt->release();
	}
}

//
// startTimedWork
//
// start a timer to trigger work when we need to
//
void	LtIpMaster::startTimedWork( int nWorkBits, int nMsTimer )
{
	STATUS	sts;
	lock();
	m_nTimerMask |= nWorkBits;
	sts = wdCancel( m_tPending );
	sts = wdStart( m_tPending, msToTicksX( nMsTimer ), LtIpMasterTimeout, (int)this );
	unlock();
}

//
// cancelTimedWork
//
// cancel any outstanding timed work
//
void	LtIpMaster::cancelTimedWork()
{
	lock();
	wdCancel( m_tPending );
	m_nTimerMask = 0;
	unlock();
}




//
// LtIpMasterTimeout
//
// Friend function that serves as the timer function
//
int	LtIpMasterTimeout( int a1, ... )
{

	LtIpMaster*	pMaster = (LtIpMaster*)a1;
	pMaster->workerTimeout();

	return 0;
}

//
// workerTimeout
//
// Timeout function for worker task
//
void	LtIpMaster::workerTimeout()
{
	// note that this is called in "interrupt" state on the target, so you can't take
	// a lock here.
	startWorkerTask( m_nTimerMask, true );
	m_nTimerMask = 0;
}

//
// LtIpRequestTimeout
//
// Friend function that serves as a timer function
//
int	LtIpRequestTimeout( int a1, ... )
{

	LtIpMaster*	pMaster = (LtIpMaster*)a1;
	pMaster->doRequestTimeout();

	return 0;
}
// Secondary timeout routine so we can access member variables
void	LtIpMaster::doRequestTimeout()
{
	// we can't just do the doRequestInfo member here
	// it needs to take a lock, and on VxWorks, the timer triggers
	// at "interrupt" level. This means that the interrupt will be blocked if
	// the lock is taken, and this causes deadlock.
	// So we need to trigger the work task to do the request info.
	if (m_nReqtMask & REQT_CSTYPE)
	{
		m_cnfgSrvrTypeChkCount++;
	}
	startWorkerTask( WORK_RequestInfo, true );
}



//
// sendNewPacket
// sendNewPacketTo
//
// send a byte string. Copy it to a LtPktInfo and send it on the link.
//
boolean	LtIpMaster::sendNewPacket( byte* pData, int nLength, LtIpRequest* pReqPkt )
{
	return sendNewPacketTo( pData, nLength, 0, 0, pReqPkt );
}

boolean	LtIpMaster::sendNewPacketTo( byte* pData, int nLength,
									ULONG ipAddr, word port, LtIpRequest* pReqPkt )
{
	LtPktInfo*	pPkt;
	LtSts		sts;
	boolean		bOk = false;
	int			nAuthSize = m_bAuthenticate? LtMD5::LTMD5_DIGEST_LEN : 0;
	if ( isActive() )
	{
		if ( nLength > UDP_MAX_PKT_LEN )
		{	// need to build segmented message, since it is too large
			// we do not authenticate the payload, but only the individual segments
			do
			{
				if( pReqPkt == NULL )
				{	vxlReportEvent("LtIpMaster::sendNewPacketTo - Segments - NULL reqPkt\n" );
					break;
				}
				if ( ipAddr == 0 )
				{	ipAddr = m_ipAddrCfgServer;
					port = m_ipPortCfgServer;
				}
				if ( ipAddr == 0 || port == 0 )
				{	vxlReportEvent("LtIpMaster::sendNewPacketTo - invalid ipAddr/port 0x%08x %d\n",
									ipAddr, port );
					break;
				}
				// outbound request does not have ball
				boolean		bHasBall = false;
				LtIpRequest	req;

				if ( pReqPkt == NULL )
				{
					// if no request was passed in, then we are sending unsolicited packet
					// so make a foney request and bHasBall is true.
					bHasBall = true;
					req.packetType = PKTTYPE_REQCHNROUTING;
					req.requestId = 100; // not in range 1000 + since that's other channel routing packets
					req.reason = REQUEST_ALL;
					pReqPkt = &req;
				}

				bOk = m_segOut.buildSegments( *pReqPkt, pData, nLength,	ipAddr, port, bHasBall );

				if ( !bOk )
				{	vxlReportEvent("LtIpMaster::sendNewPacketTo - Segment build failed len %d ip 0x%08x %d\n",
									nLength, ipAddr, port );
				}
			} while ( false );
		}
		else
		{
			pPkt = m_pAlloc->allocPacket();
			if ( pPkt )
			{
				byte*	pNewData = pPkt->getBlock();

				//assert( (nLength + LtMD5::DIGEST_LEN ) <= pPkt->getBlockSize() );
				if ( (nLength + LtMD5::LTMD5_DIGEST_LEN ) <= pPkt->getBlockSize() )
				{
					memcpy( pNewData, pData, nLength );
					pPkt->setMessageData( pNewData, nLength+nAuthSize, pPkt );
					pPkt->setIpSrcAddr( ipAddr );
					pPkt->setIpSrcPort( port );
					if ( m_bAuthenticate )
					{	// Create the authentic digest for an RFC packet
						if (isEIA852Auth())
						{
							LtIpPktBase pktBase;

							pktBase.markSecure(pNewData);
						}
						m_md5.digestWithSecret( pNewData, nLength, &pNewData[nLength] );

					}
					sts = sendPacket( pPkt, false );
					if ( sts == LTSTS_OK )
					{	bOk = true;
					}
				}
				else
				{	vxlReportEvent("LtIpMaster::sendNewPacketTo - invalid pkt size %d blkSize %d\n",
								(nLength + LtMD5::LTMD5_DIGEST_LEN ), pPkt->getBlockSize() );
				}
			}
		}
		countPacketsSent();	// IKP06042003: update pkt sent statistics
	}
	return bOk;
}



//
// LtIpMasterTask
//
// Friend function that serves as the "task" entrypoint
//
int	LtIpMasterTask( int a1, ... )
{

	LtIpMaster*	pMaster = (LtIpMaster*)a1;
	pMaster->workerTask();

	return 0;
}

//
// LtIpWritePersistTask
//
// Friend function that writes persistence
//
int	LtIpWritePersistTask( int a1, ... )
{

	LtIpMaster*	pMaster = (LtIpMaster*)a1;
	pMaster->writePersistTask();

	return 0;
}

void LtIpMaster::writePersistTask()
{
	LtIpPersist			pst;
	LtIpWritePersistMsg	msgTemp;
	LtIpWritePersistMsg	msgLast;
	STATUS				sts;
	boolean				bOk;
	ULONG				nTicksNow;
	ULONG				nTicksStart;
	int					wait;
	boolean				bExit = false;


	while ( !bExit )
	{
		msgLast.pData = NULL;
		msgLast.nSize = 0;
		wait = WAIT_FOREVER;	// Initially, don't wake up
		nTicksStart = 0;
		while ( !bExit )
		{
			sts = msgQReceive( m_mqWritePersist, (char*)&msgTemp, sizeof(msgTemp), wait);
			if ( sts != sizeof(msgTemp) )
			{	break;
			}
			if (nTicksStart == 0)
			{
				nTicksStart = tickGet();
			}
			if (msgTemp.pData != NULL)
			{
			    if ( msgLast.pData )
			    {
                    ::free( msgLast.pData );
                    msgLast.pData = NULL;
			    }
				msgLast = msgTemp;
				wait = msToTicksX(1000);	// Try again, then wake up and do the write if no more received
			}
			else
			{
				// An empty message is now the signal to exit. Don't use m_bTaskExit
				// We will still write out any pending message before exiting
				bExit = true;
				break;
			}
			// If we've waited long enough, go ahead and flush out the changes
			nTicksNow = tickGet();
			if (ticksToMs( nTicksNow - nTicksStart ) > 5000)
			{
				break;
			}
		}
		if ( msgLast.pData )
		{
			nTicksStart = tickGet();
			bOk = pst.writeFile( m_cPersistName, msgLast.pData, msgLast.nSize );
			nTicksNow = tickGet();
			nTicksNow = ticksToMs( nTicksNow - nTicksStart );
			if ( !bOk )
			{
				vxlReportUrgent("Router Persistence: Unable to write persistent data block\n");
			}
			else
			{
				vxlReportEvent("IpMaster::writePersistTask - Successful write of %d bytes %d ms\n",
					msgLast.nSize, nTicksNow );
			}
			::free( msgLast.pData );
			msgLast.pData = NULL;
		}
	} // while
	m_tidWritePersist = 0;
	vxlReportEvent("writePersistTask - exit\n");
}


//
// LtIpCheckStuckTask
//
// Friend function that checks to see if we are stuck
//
int	LtIpCheckStuckTask( int a1, ... )
{

	LtIpMaster*	pMaster = (LtIpMaster*)a1;
	pMaster->checkStuck();

	return 0;
}

//
// checkStuck
//
// as this is writtem below, it doesn't actually work.
// It determines whether the master is stuck, but it doesn't
// recover from that case gracefully.
// Another way to do the check would be to have the lock / unlock
// pair bump a counter. Lock would bump the counter if it succeeded.
// unlock doesn't do anything actually.
// Then this task could wake up every second and look for a change in the
// lock counter. No change, then the master object is stuck. There is
// some problem with the logic below which causes a failure when
// the master object gets stuck.
//
void LtIpMaster::checkStuck()
{
#ifdef DEBUG_LOCKS
	STATUS		sts = 0;
	boolean		bStuck = false;
	boolean		bUnlock = true;
	int			nStuckMonitor = 60;
	int			nStuckReport = 60;

	while ( ! m_bTaskExit )
	{
		taskDelay( msToTicksX( 1000 ) );
		if ( m_bTaskExit ) break;

		sts = semTake( m_lock, msToTicksX( 500 ) );
		if ( sts == ERROR && ! bStuck )
		{	vxlReportUrgent("checkStuck - master object lock is stuck\n");
			bStuck = true;
			bUnlock = false;
			xReportLockStack();
			((CIpLink*)m_pLink)->xReportLockStack();
		}
		else if ( bStuck )
		{	vxlReportUrgent("checkStuck - master object lock is not stuck\n");
			bStuck = false;
			bUnlock = true;
		}
		else
		{	bUnlock = true;
		}
		if ( bUnlock )
		{	semGive( m_lock );
		}
		nStuckReport--;
		if ( nStuckReport <= 0 )
		{
			vxlReportUrgent("checkStuck - monitoring continuing\n");
			nStuckReport = nStuckMonitor;
		}
	}
#endif // DEBUG_LOCKS
	m_tidCheckStuck = 0;	// we are gone now
}



//
// startWorkerTask
//
// Start the worker task if necessary, and then trigger
// it to run.
// This is called from timer completions. On VxWorks this is "interrupt"
// code. So avoid locking unless we are starting the work task.
//
void	LtIpMaster::startWorkerTask( int nWorkBits, boolean bIntLvl )
{
	if ( m_tidWorker == 0 && !bIntLvl )
	{
		vxlReportEvent("LtIpMaster::startWorkerTask - start tasks\n");
		lock();
		if ( !m_bTaskExit && m_tidWorker == 0 )
		{
			m_tidWorker = taskSpawn( "LtIpMaster", LTIP_MASTER_TASK_PRIORITY, 0, 32*1024,
								LtIpMasterTask, (int)this, 2, 3, 4, 5, 6, 7, 8, 9, 0 );
			assert(m_tidWorker );
			m_tidWritePersist = taskSpawn( "LtIpWrtPrst", LTIP_MASTER_TASK_PRIORITY+1, 0, 16*1024,
								LtIpWritePersistTask, (int)this, 2, 3, 4, 5, 6, 7, 8, 9, 0 );
			assert(m_tidWritePersist );
#ifdef DEBUG_LOCKS
			m_tidCheckStuck = taskSpawn( "LtIpCheckStuck", LTIP_CHECKSTUCK_TASK_PRIORITY, 0, 16*1024,
								LtIpCheckStuckTask, (int)this, 2, 3, 4, 5, 6, 7, 8, 9, 0 );
			assert(m_tidCheckStuck );
#endif // DEBUG_LOCKS
			m_tidBwAggTask = taskSpawn( "LtIpBwAgg", LTIP_BWAGG_TASK_PRIORITY, 0, 16*1024,
								LtIpBwAggTask, (int)this, 2, 3, 4, 5, 6, 7, 8, 9, 0 );
			assert(m_tidBwAggTask );

		}
		unlock();
		vxlReportEvent("LtIpMaster::startWorkerTask - tasks started\n");
	}
	// Don't signal any new tasks if we are shutting down
	if (!m_bTaskExit)
	{
		m_nWorkMask |= nWorkBits;
	}
	semGive( m_semWork );
}

//
// workerTask
//
// Worker task for the master. Functions include:
// Read persistent data, create clients.
// Update our information from the Config Manager
// Send new client information to the config manager
// Write persistent data
//
// All these functions might block, so we do them in another task to avoid
// slowing the rest of the system down.
//
void	LtIpMaster::workerTask()
{
	int				nDataIncarnation;
	boolean			bOk;

	while ( !m_bTaskExit )
	{
		semTake( m_semWork, WAIT_FOREVER );
		if ( m_bTaskExit ) break;
		lock();

		// we are fairly stupid about sending messages
		// if we arent using TCP, then we should retry a few times to talk to the
		// config server. To keep things simple, dont do anything else while we are
		// trying to talk to the config server.
		// Just send a message, start a timer and wait on expire or a message to arrive.
		//


		nDataIncarnation = m_nDataIncarnation;

		if ( m_nWorkMask & WORK_ReadPersist )
		{	m_nWorkMask &= ~WORK_ReadPersist;
			unlock();
			bOk = doReadPersist();

			lock();
            m_bPersistenceRead = true;
		}

		while ( !m_qRFCin.lockedIsEmpty() )
		{
			doRFCin();
		}
		// uses doRequestInfo now
		if ( m_nWorkMask & WORK_SendDevRegister )
		{
			m_nWorkMask &= ~WORK_SendDevRegister;
			//doSendDevRegister( false );
			startRequestInfo( REQT_DEVRESPONSE, true );
		}

		if ( m_nWorkMask & WORK_SendChanRouting )
		{
			m_nWorkMask &= ~WORK_SendChanRouting;
			// Send one, if we get a new one, and set up timing if
			// we need to.
			if ( doSendChanRouting( 0, 0, NULL, false, true ) )
			{	startRequestInfo( REQT_SENDCHANROUTING );
			}
		}

		// write the data if we were told to, or if the data was updated
		if ( ( m_nWorkMask & WORK_WritePersist )
			|| (nDataIncarnation != m_nDataIncarnation )
			)
		{
			m_nWorkMask &= ~WORK_WritePersist;
			// Don't unlock - we want the persistent data set to be coherent.
			// Unlocking not needed because doWritePersist() doesn't actually write the data,
			// it just collects it and sends it to another task.
			//unlock();
			doWritePersist();
			//lock();
		}
		// triggered by a special timer
		if (  m_nWorkMask & WORK_RequestInfo )
		{
			m_nWorkMask &= ~WORK_RequestInfo;
			doRequestInfo();
		}
		m_ConnectLedObSubject.Notify();	// update Connect LED

		unlock();
	} // while
	m_tidWorker = 0; // we are history
}


//
// deleteWorkerTask
//
// delete the task, clean up the semaphore and empty the work queue(s)
// don't call this with object locked
//
void	LtIpMaster::deleteWorkerTask()
{
	LtIpWritePersistMsg	persistMsg;

	persistMsg.pData = NULL;
	persistMsg.nSize = 0;

	// the safe way to delete tasks, wait on the tasks to exit on their own.
	// The problem with taskDelete is that tasks may be holding locks when
	// they go down and then you are stuck.
	m_bTaskExit = true;

	// If there is a pending persistent data write, force it out without relying
	// on the workTask,
	if ((m_nWorkMask & WORK_WritePersist) || (m_nTimerMask & WORK_WritePersist))
	{
		// Lock to guarentee consistency
		lock();
		doWritePersist();
		unlock();
	}
	// Send a message to the persist task to shut it down
	msgQSend( m_mqWritePersist, (char*)&persistMsg, sizeof(persistMsg), NO_WAIT, MSG_PRI_NORMAL );

	startWorkerTask( 0 );
	while ( m_tidWorker || m_tidCheckStuck || m_tidBwAggTask || m_tidWritePersist )
	{	taskDelay( msToTicksX(50) );
	}

	if ( m_semWork )
	{
		semDelete( m_semWork );
		m_semWork = 0;
	}
	wdDelete( m_tPending );
	wdDelete( m_tRequest );
	m_tRequest = 0;
	m_tPending = 0;
	// discard any waiting RFC messages from the queue
	LtQue*	pLtQue = NULL;
	LtPktInfo*	pItem;
	while ( m_qRFCin.removeTail( (LtQue**)&pLtQue ) )
	{
		pItem = (LtPktInfo*)pLtQue;
		pItem->release();
	}
}

//
// setIndex
//
// set the identity index and adjust the name of the nvRam key and persist name
//
void	LtIpMaster::setIndex( int nIdx )
{
	m_nIndex = nIdx;
	sprintf( m_szNvRamKey, "%s%d", NVRAM_KEY, m_nIndex );
	if ( m_nIndex )
	{	sprintf( m_cPersistName, "routerConfig%d", m_nIndex );
	}
	else
	{	strcpy(m_cPersistName, "routerConfig" );
	}
}


//
// doReadNvRam
//
// read lastdatetime and session from nv ram
// MARSHA TODO: Do we need to save the MULTICAST address to nv ram?
void	LtIpMaster::doReadNvRam( boolean bSetServers )
{
	MasterNvRam		ram;
	int				nBytes;
	boolean			bOk = true;

	// set a random session id just in case we can't get one
	m_nSession = getTimestamp(false);

	nBytes = LtNvRam::get( m_szNvRamKey, (byte*)&ram, sizeof(ram), FALSE );
	do
	{
		if ( nBytes != sizeof(ram) ) bOk = false;
		if ( ram.magic != (int)MAGIC_NUMBER || ram.version != (int)NVRAM_DATA_VERSION_1 ) bOk = false;
		if ( ram.length != sizeof(ram) ) bOk = false;
		if ( !bOk )
		{
			if (nBytes == 0)
				vxlReportUrgent( "Router Persistence - non-volatile data missing. Initializing defaults.\n" );
			else
				vxlReportUrgent( "Router Persistence - non-volatile data discarded. Wrong version or corrupted.\n" );
			break;
		}
		m_lastDatetime = MAX( m_lastDatetime, ram.dateTime );
		m_nSession			= ram.nSession;			// session number
		if ( m_ipAddrLocal != ram.ipAddrLocal )
		{
			LtIpAddressStr	ias;
			LtIpAddressStr	ias2;
#if CLEAR_NVDATA_ON_IP_CHANGE	// the old way...
#ifdef ILON_PLATFORM
			// For the iLON, keep the local port value, since it is the one
			// thing not configured by the config server.
			// Don't do this for LNS or the iLON simulator, because they
			// always pass the correct port in to the constructor.
			if (bSetServers)
				m_ipPortLocal = ram.ipPortLocal;		// local ip port
#endif
			vxlReportUrgent( "Router Persistence - non-volatile data discarded due to local IP address change\n"
					"             IP address is %s was %s \n",
					ias.getString( m_ipAddrLocal ),
					ias2.getString( ram.ipAddrLocal ) );
			break;
#else	// CLEAR_NVDATA_ON_IP_CHANGE
#ifdef ILON_100_ROUTER_DEMO
			// New logic - just remember the address change
			m_prevIpAddrLocal = ram.ipAddrLocal;
#endif	// ILON_100_ROUTER_DEMO

			if (!selfInstalledMcastMode())
			{
			    vxlReportUrgent( "Router Persistence - local IP address change. IP address is %s, was %s \n",
					ias.getString( m_ipAddrLocal ),
					ias2.getString( ram.ipAddrLocal ) );
			}
#endif	// CLEAR_NVDATA_ON_IP_CHANGE
		}
		if ( bSetServers )
		{
			//m_ipAddrLocal			= ram.ipAddrLocal;		// local IP address
			if (selfInstalledMcastMode())
			{
				m_ipAddrCfgServer		= 0;	// no config server in this mode
			}
			else
			{
			    m_ipAddrCfgServer		= ram.ipAddrCfgServer;	// config server address
			}
			m_ipAddrNtpServer		= ram.ipAddrNtpServer;	// time server address
			m_ipAddrNtpServer2		= ram.ipAddrNtpServer2;	// time server address
#ifdef ILON_PLATFORM
			// For the iLON, get the local port from "NVRAM"
			// Don't do this for LNS or the iLON simulator, because they
			// always pass the correct port in to the constructor.
			m_ipPortLocal			= ram.ipPortLocal;		// local ip port
#endif
            m_ipPortCfgServer		= ram.ipPortCfgServer;	// config server port
			m_ipPortNtpServer		= ram.ipPortNtpServer;	// time server port
			m_ipPortNtpServer2		= ram.ipPortNtpServer2;	// time server port
		}
	} while (false);
}

//
// doWriteNvRam
//
// write lastDatetime and session to nv ram
// also save all the server stuff, since when we restart, we get nvram first, and
// other stuff later on.
//
void	LtIpMaster::doWriteNvRam()
{
	MasterNvRam		ram;

	ram.magic				= MAGIC_NUMBER;				// magic number
	ram.version				= NVRAM_DATA_VERSION_1;		// version of the file
	ram.length				= sizeof(ram);				// length of this data
	ram.dateTime			= m_lastDatetime;			// dateTime saved
	ram.nSession			= getSession();				// session number
#ifdef ILON_100_ROUTER_DEMO
	// Save the previous local address until we know we don't need it
	ram.ipAddrLocal			= (m_prevIpAddrLocal == 0) ? m_ipAddrLocal : m_prevIpAddrLocal;		// local IP address
#else
	ram.ipAddrLocal			= m_ipAddrLocal;		// local IP address
#endif
	ram.ipAddrCfgServer		= m_ipAddrCfgServer;	// config server address
	ram.ipAddrNtpServer		= m_ipAddrNtpServer;	// time server address
	ram.ipAddrNtpServer2	= m_ipAddrNtpServer2;	// time server address
	ram.ipPortLocal			= m_ipPortLocal;		// local ip port
	ram.ipPortCfgServer		= m_ipPortCfgServer;	// config server port
	ram.ipPortNtpServer		= m_ipPortNtpServer;	// time server port
	ram.ipPortNtpServer2	= m_ipPortNtpServer2;	// time server port

	LtNvRam::set( m_szNvRamKey, (byte*)&ram, sizeof(ram), FALSE );
}


//
// doReadPersist
//
// Read a persistent configuration from version 1 and convert to version 2
//
boolean	LtIpMaster::getPersistV1(MasterDataV1& masV1, byte* pData, int& size)
{
	boolean				bOk = false;

	if ((masV1.version == (int)MASTER_DATA_VERSION_1) && (masV1.length >= (int)sizeof(MasterDataV1)))
	{
		size = sizeof(masV1);
		memcpy(&masV1, pData, sizeof(masV1));
		bOk = true;
	}

	return bOk;
}

boolean	LtIpMaster::getPersistV2(MasterDataV2& masV2, byte* pData, int& size)
{
	boolean				bOk = false;

	if (masV2.version < (int)MASTER_DATA_VERSION_2)
	{
		bOk = getPersistV1(*((MasterDataV1*)&masV2), pData, size);
		if (bOk)
		{
			// Set defaults for the new fields
			masV2.bHasSharedIpAddrs = false;
			masV2.runningEchProtocolVer = LTIP_ECH_VER1;
			masV2.natIpAddr = 0;
			masV2.bStrictEia852 = false;
		}
	}
	else if ((masV2.version >= (int)MASTER_DATA_VERSION_2) && (masV2.length >= (int)sizeof(MasterDataV2)))
	{
		size = sizeof(masV2);
		memcpy(&masV2, pData, sizeof(masV2));
		bOk = true;
	}

	return bOk;
}

boolean	LtIpMaster::getPersistV3(MasterDataV3& masV3, byte* pData, int& size)
{
	boolean				bOk = false;

	if (masV3.version < (int)MASTER_DATA_VERSION_3)
	{
		bOk = getPersistV2(*((MasterDataV2*)&masV3), pData, size);
		if (bOk)
		{
			masV3.lastKnownCsPort = masV3.ipPortCfgServer;
			vxsMakeDottedAddr(masV3.lastKnownCsHost, masV3.ipAddrCfgServer);
		}
	}
	else if ((masV3.version >= (int)MASTER_DATA_VERSION_3) && (masV3.length >= (int)sizeof(MasterDataV3)))
	{
		size = sizeof(masV3);
		memcpy(&masV3, pData, sizeof(masV3));
		bOk = true;
	}

	return bOk;
}

//
// doReadPersist
//
// Read a persistent configuration if we can find one
//
boolean	LtIpMaster::doReadPersist()
{
	LtIpPersist			pst;
	LtIpAddPortDate		apd[MAX_MEMBERS];
	int					nBytes = 0;
	byte*				pData = NULL;
	byte*				p;
	byte*				pend;
	byte*				p2;
	int					i;
	int					idx;
	boolean				bOk = false;
	MasterDataV3		mas;
	int					masSize = 0;	// init for warning
	LtIpAddressStr		ias;
	LtIpAddressStr		ias2;

	LtIpChanMembers	chm;
	LtIpChanRouting	chr;
	LtIpDevRegister dvr;

	// temporary storage for data as we read it in
	int				nMembers = 0;
	byte*			pktChanMembers = NULL;
	byte*			pktChanRouting[MAX_MEMBERS];
	byte*			pktDevRegister = NULL;

	memset( pktChanRouting, 0, sizeof(pktChanRouting) );

	// leave any current data alone until we restore the persistent data
	// completely.
	do
	{	vxlReportEvent("IpMaster::doReadPersist - starting\n");
		if ( !pst.readFile( m_cPersistName, &pData, &nBytes ) )
		{
			vxlReportEvent("IpMaster::doReadPersist - Unable to read persistent data\n");
			break;
		}
		p = pData;
		pend = pData+nBytes;

		memcpy( &mas, p, sizeof(mas) );
		if ( mas.magic == (int)MAGIC_NUMBER && nBytes == mas.length)
		{
			bOk = getPersistV3(mas, pData, masSize);
		}

		if (!bOk)
		{	vxlReportEvent( "LtIpMaster::doReadPersist - invalid persistance file\n" );
			break;
		}
		bOk = false;

		// restore the master control data
		// the caller is responsible for making sure that we
		// are synchronized when all this stuff changes.
		// m_ipAddrLocal		= mas.ipAddrLocal; // this may change behind our back
		// allow override by init before we get started
#if 0 // these are stored in NV ram as well, so use those values not these
		m_ipAddrCfgServer	= mas.ipAddrCfgServer;
		m_ipAddrNtpServer	= mas.ipAddrNtpServer;
		m_ipAddrNtpServer2	= mas.ipAddrNtpServer2;
		m_ipPortLocal		= mas.ipPortLocal;
		m_ipPortCfgServer	= mas.ipPortCfgServer;
		m_ipPortNtpServer	= mas.ipPortNtpServer;
		m_ipPortNtpServer2	= mas.ipPortNtpServer2;
#endif // stored in nvram
		m_bBWLimit			= mas.bBwLimit;
		m_nBWLimitKBPerSec	= mas.nBwLimitKbPerSec;
		m_bAggregate		= mas.bAggregate;
		m_nAggregateMs		= mas.nAggregateMs;
		m_nChannelTimeout	= mas.nChannelTimeout;
		m_bCheckStale		= mas.bCheckStale;
		m_bReorderPackets	= mas.bReorderPackets;
		m_nEscrowTimeMs		= mas.nEscrowTimeMs;
		m_bUseTOSbits		= mas.bUseTosBits;
		m_nTOSbits			= mas.nTOSbits;
		setAuthentication(mas.bAuthenticate, false);
		setAuthenticSecret(mas.aAuthenticSecret, false);
		memcpy( m_acName, mas.cName, MAXNAMELEN );
		m_hasSharedIpAddrs	= mas.bHasSharedIpAddrs;

		m_runningEchProtocolVer = (LtIpEchProtocolVersion)mas.runningEchProtocolVer;
		m_natIpAddr			= mas.natIpAddr;
		m_bStrictEia852		= mas.bStrictEia852;
		// All this needs to be propogated to the segmentors
		setMembersEIA852Auth();
		setSegmentorExtHdrData();
		m_lastKnownCsPort	= mas.lastKnownCsPort;
		memcpy(m_lastKnownCsHost, mas.lastKnownCsHost, sizeof(m_lastKnownCsHost));
		// Add additional nonvol items here

		m_acName[MAXNAMELEN-1] = 0;

#ifdef ILON_100_ROUTER_DEMO
		if ( m_ipPortLocal != mas.ipPortLocal ) // ignore address change
#else
		if ( (( m_ipAddrLocal != mas.ipAddrLocal ) || ( m_ipPortLocal != mas.ipPortLocal )) &&
			!selfInstalledMcastMode())
#endif	// ILON_100_ROUTER_DEMO
		{
			vxlReportUrgent(	"Router Persistence - discarded due to local IP address/port  change\n"
								"             IP address(port) is %s (%d) was %s (%d)\n",
							ias.getString( m_ipAddrLocal ), m_ipPortLocal,
							ias2.getString( mas.ipAddrLocal ), mas.ipPortLocal );
			break;
		}
		if ( m_bUseTOSbits )
		{
			VXSOCKET	sock;
			CIpLink*	pLink = (CIpLink*)m_pLink;
			int			nErr;
			sock = pLink->masterSocket();
			nErr = vxsSetTosBits( sock, m_nTOSbits );
			if ( 0 != nErr )
			{	vxlReportUrgent("Router Persistence - error %d setting TOS bits to %d\n",
								nErr, m_nTOSbits );
			}
		}

		// Skip past MasterData record
		p += masSize;

		if ( mas.bHaveDevRegister )
		{
			bOk = dvr.parse( p, false );
			if ( !bOk )
			{	vxlReportEvent("doReadPersist -Error parsing device registration packet\n");
				break;
			}
			bOk = makeNewPacket( p, dvr.packetSize, &pktDevRegister );
			if ( !bOk )
			{	vxlReportEvent("doReadPersist -Unable to allocate device registration\n");
				break;
			}
			p += dvr.packetSize;
			if ( p >= pend ) break;
		}
		if ( mas.bHaveMembers )
		{
			bOk = chm.parse( p, false );
			if ( !bOk )
			{	vxlReportEvent("doReadPersist -Error parsing channel members packet\n");
				break;
			}
			bOk = makeNewPacket( p, chm.packetSize, &pktChanMembers );
			m_nPktCMSize = chm.packetSize;
			m_dtPktCM = chm.dateTime;
			if ( !bOk )
			{	vxlReportEvent("doReadPersist -Unable to allocate chan members %d\n", m_nPktCMSize);
				break;
			}
			p += chm.packetSize;

			// parse chan members to array
			p2 = chm.pUcAddresses;
			nMembers = chm.listSize;
			for ( i=0; i<nMembers; i++ )
			{
				p2 = apd[i].parse( p2 );
			}
		}

		for ( i=0; i< mas.nChanRouting; i++ )
		{
			bOk = true;
			if ( p >= pend ) break;
			bOk = chr.parse( p, false );
			if ( !bOk )
			{	vxlReportEvent("doReadPersist -Error parsing channel routing packet\n");
				break;
			}
			// if someone passed us a channel routing with zero address
			// toss it out
			if ( chr.ipUcAddress == 0 )
			{
				p += chr.packetSize;
				vxlReportEvent(	"LtIpMaster::doReadPersist - discard channel routing for 0.0.0.0\n" );
				continue;
			}
			idx = findChannelMember( chr.ipUcAddress, chr.ipUcPort, apd );
			if ( idx == -1 )
			{	// extra channel routing packet - discard
				//assert( false );
				//continue;	// not sure how this happened
				ias.setIaddr( chr.ipUcAddress );
				vxlReportEvent(	"LtIpMaster::doReadPersist - extra channel routing packet for %s\n"
								"                          - persistent data discarded\n",
								ias.getString() );
				bOk = false;
				break;
			}
			if ( pktChanRouting[idx] )
			{	// we already have one for this mode
				//assert(false );
				FREEANDCLEAR(pktChanRouting[idx]);
			}
			ias.setIaddr( chr.ipUcAddress );
			vxlReportEvent("IpMaster::doReadPersist - Recovered Chan Routing %d for %s:%d\n",
							idx, ias.getString(), chr.ipUcPort );

			bOk = makeNewPacket( p, chr.packetSize, &pktChanRouting[idx] );
			if ( !bOk )
			{	vxlReportEvent("doReadPersist -Unable to allocate chan routing %d\n", chr.packetSize);
				bOk = false;
				break;
			}
			// if this is our channel routing packet, then save it here and
			// remember its size
			// Our address could be local or from a NAT box
			if ((chr.ipUcAddress == getExternalIpAddr()) && (chr.ipUcPort == m_ipPortLocal))
			{	m_nPktOCRSize = chr.packetSize;
				m_pktOurChanRouting = pktChanRouting[idx];
			}
			else if (chrPktHasLocalUid(chr))
			{
				// This looks like ours, but doesn't properly match.
				// It may be a leftover from a local port change - delete it
				FREEANDCLEAR(pktChanRouting[idx]);
			}

			p += chr.packetSize;
		}
		if ( ! bOk )
		{	break;
		}


		vxlReportEvent("doReadPersist -Persistent data restored\n");
	} while ( false );

	if ( pData )
	{	free( pData );
	}
	// if we obtained a new set of data, then make it the real set
	if ( bOk )
	{
		lock();
		m_pktDevRegister = pktDevRegister;
		vxlReportEvent("IpMaster::doReadPersist - setNewMembers %d\n", nMembers );
		// store the new channel members packet we have
		m_pktChanMembers = pktChanMembers;
		if ( pktChanMembers )
		{
			bOk = chm.parse( pktChanMembers, false );
			if ( !bOk )
			{	vxlReportEvent("IpMaster::doReadPersist - ERROR reparse of ChanMembers\n" );
			}
			else
			{	setNewMembers( chm );
			}
			for ( i=0; i<MAX_MEMBERS; i++ )
			{
				if ( pktChanRouting[i] )
				{
					vxlReportEvent("IpMaster::doReadPersist - updateClientRouting %d\n", i );
					updateClientRouting( i, pktChanRouting[i] );
				}
			}
		}
		m_bDataValid = true;
		m_nDataIncarnation++;
		unlock();
		// Now we need to notify the world of a change in our data
	}
	else
	{
		// clean up our temporary storage
		FREEANDCLEAR(pktDevRegister);
		FREEANDCLEAR( pktChanMembers );
		for ( i=0; i<MAX_MEMBERS; i++ )
		{
			FREEANDCLEAR( pktChanRouting[i] );
		}
	}
	return bOk;
}


//
// doWritePersist
//
// store the data in the flash
// Assume that the data cannot change out from under us while we are storing
// it. This assumption is controlled because any routine that can change the
// data is called in the same worker task that we are called from, so until we
// complete, any RFC messages that are arriving are being queued for us.
//
boolean	LtIpMaster::doWritePersist()
{
	//LtIpPersist		pst;
	ULONG			nTickStart = tickGet();
	ULONG			nTickEnd;
	int				nBytes = 0;
	byte*			pData = NULL;
	int				i;
	int				nSizes[MAX_MEMBERS];
	boolean			bOk;
	byte*			p;
	int				nChanRouting = 0;
	boolean			bChanRout[MAX_MEMBERS];
	byte*			pChanRout[MAX_MEMBERS];

	MasterDataV3	mas;
	LtIpChanMembers	chm;
	LtIpChanRouting	chr;
	LtIpDevRegister dvr;
	memset( nSizes, 0, sizeof(nSizes) );
	memset( &mas, 0, sizeof(mas) );
	memset( bChanRout, 0, sizeof(bChanRout) );
	memset( pChanRout, 0, sizeof(pChanRout) );

	// write server addresses into NV ram as well, since they need to
	// aggree.
	doWriteNvRam();
	do
	{
		mas.magic				= MAGIC_NUMBER;
		mas.version				= MASTER_DATA_VERSION_3;
		mas.length				= 0;
		mas.dateTime			= m_lastDatetime;
		mas.nSession			= getSession();
#ifdef ILON_100_ROUTER_DEMO
		mas.ipAddrLocal			= (m_prevIpAddrLocal == 0) ? m_ipAddrLocal : m_prevIpAddrLocal;
#else
		mas.ipAddrLocal			= m_ipAddrLocal;
#endif
		mas.ipAddrCfgServer		= m_ipAddrCfgServer;
		mas.ipAddrNtpServer		= m_ipAddrNtpServer;
		mas.ipAddrNtpServer2	= m_ipAddrNtpServer2;
		mas.ipPortLocal			= m_ipPortLocal;
		mas.ipPortCfgServer		= m_ipPortCfgServer;
		mas.ipPortNtpServer		= m_ipPortNtpServer;
		mas.ipPortNtpServer2	= m_ipPortNtpServer2;
		mas.bHaveDevRegister	= true;
		mas.bHaveMembers		= true;
		mas.bBwLimit			= m_bBWLimit;
		mas.nBwLimitKbPerSec	= m_nBWLimitKBPerSec;
		mas.bAggregate			= m_bAggregate;
		mas.nAggregateMs		= m_nAggregateMs;
		mas.nChannelTimeout		= m_nChannelTimeout;
		mas.bCheckStale			= m_bCheckStale;
		mas.bReorderPackets		= m_bReorderPackets;
		mas.nEscrowTimeMs		= m_nEscrowTimeMs;
		mas.bUseTosBits			= m_bUseTOSbits;
		mas.nTOSbits			= m_nTOSbits;
		mas.bAuthenticate		= m_bAuthenticate;
		memcpy( mas.aAuthenticSecret, m_aAuthenticSecret, AUTHENTIC_SECRET_SIZE );
		memcpy( mas.cName, m_acName, MAXNAMELEN );
		mas.bHasSharedIpAddrs	= m_hasSharedIpAddrs;
		mas.runningEchProtocolVer = (int)m_runningEchProtocolVer;
		mas.natIpAddr			= m_natIpAddr;
		mas.bStrictEia852		= m_bStrictEia852;
		// This logic is moved here so that a potential DNS lookup won't delay response to CS.
		if (m_updateCsHostName)
		{
			ULONG csIpAddr;
			// If the CS addr got set directly to a real address, update "last known" info (if changed)
			if (m_ipAddrCfgServer != 0)
			{
				m_lastKnownCsPort = m_ipPortCfgServer;
				csIpAddr = hostGetByName(m_lastKnownCsHost);
				if (((STATUS)csIpAddr == ERROR) || (csIpAddr != m_ipAddrCfgServer))
				{
					// The "last known" CS host name/addr doesn't match (or we can't tell), so set it
					vxsMakeDottedAddr(m_lastKnownCsHost, m_ipAddrCfgServer);
				}
			}
			m_updateCsHostName = false;
		}
		mas.lastKnownCsPort		= m_lastKnownCsPort; // Keep here, may get changed above
		memcpy(mas.lastKnownCsHost, m_lastKnownCsHost, sizeof(mas.lastKnownCsHost));
		// Add additional nonvol items here

		nBytes = sizeof(mas);

		if ( m_pktDevRegister == NULL ||
			!dvr.parse( m_pktDevRegister, false )
			)
		{	mas.bHaveDevRegister	= false;
			vxlReportEvent("IpMaster::doWritePersist - No dev register pkt\n");
		}
		else
		{	nBytes += dvr.packetSize;
		}

		if ( m_pktChanMembers == NULL ||
			!chm.parse( m_pktChanMembers, false )
			)
		{
			mas.bHaveMembers		= false;
			vxlReportEvent("IpMaster::doWritePersist - No chan members pkt\n");
		}
		else
		{	nBytes += chm.packetSize;
		}

		bOk = true;
		for ( i=0; i<m_nMembers; i++ )
		{
			p = NULL;

			if ( m_pktChanRouting[i] )
			{	p = m_pktChanRouting[i];
			}
			else if ( m_nOurIndex == i && m_pktOurChanRouting )
			{
#if TESTSWEEP
				vxlPrintf("doWritePersist - writing our channel routing %d 0x%08x\n",
								m_nOurIndex, m_pktOurChanRouting );
#endif // TESTSWEEP
				p = m_pktOurChanRouting;
			}
			if ( p )
			{
				bOk = chr.parse( p, false );
				if ( bOk )
				{
					nChanRouting++;
					bChanRout[i] = true;
					pChanRout[i] = p;
					nSizes[i] = chr.packetSize;
					nBytes += chr.packetSize;
				}
			}
		}
		mas.nChanRouting = nChanRouting;
		mas.length = nBytes;
#if 0 // debug
		vxlReportEvent("IpMaster::doWritePersist - alloc block %d bytes %d members %d chrPkts\n",
						nBytes, m_nMembers, nChanRouting );
#endif // debug
		pData = (byte*)malloc( nBytes );
		if ( pData == NULL )
		{
			vxlReportEvent("IpMaster::doWritePersist -Unable to allocate persistent data block\n");
			assert( pData );
			break;
		}

		p = pData;
		memcpy( p, &mas, sizeof(mas) );
		p += sizeof(mas);

		if ( mas.bHaveDevRegister )
		{	memcpy( p, m_pktDevRegister, dvr.packetSize );
			p += dvr.packetSize;
		}
		if ( mas.bHaveMembers )
		{	memcpy( p, m_pktChanMembers, chm.packetSize );
			p += chm.packetSize;
		}
		for ( i=0; i<m_nMembers; i++ )
		{
			if ( bChanRout[i] )
			{
				memcpy( p, pChanRout[i], nSizes[i] );
				p += nSizes[i];
			}
		}
		assert( p == ( pData+nBytes ) );
#if 0
		bOk = pst.writeFile( m_cPersistName, pData, nBytes );
		if ( !bOk )
		{
			vxlReportEvent("IpMaster::doWritePersist -Unable to write persistent data block\n");
			break;
		}
#endif
		LtIpWritePersistMsg	msg;
		LtIpWritePersistMsg	msg2;
		int			nTry = 1;
		STATUS		sts;
		msg.pData = pData;
		msg.nSize = nBytes;
		// write a message to the persistence task with block to write
		msg.pData = pData;
		msg.nSize = nBytes;
		do
		{	// send without waiting
			sts = msgQSend( m_mqWritePersist, (char*)&msg, sizeof(msg), NO_WAIT, MSG_PRI_NORMAL );
			if ( sts == ERROR )
			{	// woops, the MQ is full, so read a message out and toss it
				sts = msgQReceive( m_mqWritePersist, (char*)&msg2, sizeof(msg), NO_WAIT );
				if ( sts != ERROR )
				{	::free( msg2.pData );
				}
				// but only try it once
				if ( --nTry == 0 )continue;
				// No more tries, just bug out and toss the data
				break;
			}
			// we succeeded, so forget the data, it's gone.
			pData = NULL;
			break;
		} while ( true );



		nTickEnd = tickGet();
		nTickStart = ticksToMs( nTickEnd - nTickStart);
		vxlReportEvent("IpMaster::doWritePersist -Persistent write queued. %d ms\n", nTickStart);
	} while ( false );
	if ( pData )
	{	free( pData );
	}

	return bOk;
}

#define XML_CONFIG_NAME "config.xml"
#define XML_CONFIG_DUMP_DIR "XmlDump"

// Read the IpMaster config from an XML file
boolean LtIpMaster::loadXmlConfig()
{
	boolean bOk = false;

	char path[MAX_PATH];
	if (m_loadXmlConfig_Callback != NULL)
	{
		LtIpPersist::makePath(path, XML_CONFIG_NAME, m_nIndex);
		bOk = m_loadXmlConfig_Callback(this, path);
	}
	return bOk;
}
// Dump the IpMaster config to an XML file
boolean LtIpMaster::dumpXmlConfig()
{
	boolean bOk = false;

	if (m_dumpXmlConfig_Callback != NULL)
	{
		char path[MAX_PATH];

		// Make sure the dump folder exists
		LtPlatform::getPersistPath( path, MAX_PATH );
		strcat(path, XML_CONFIG_DUMP_DIR);
		LtIpMakeDir(path);

		LtIpPersist::makePath(path, XML_CONFIG_NAME, m_nIndex, XML_CONFIG_DUMP_DIR);
		bOk = m_dumpXmlConfig_Callback(this, path);
	}
	return bOk;
}

LtIpConnectState::eLTIP_CONNECT_STATE LtIpMaster::getConnectState()
{
	LtIpConnectState::eLTIP_CONNECT_STATE state;
	BOOL bConfigOutOfDate = FALSE;

	// Mark as out-of-date for any standard packet request
	if (m_nReqtMask & (REQT_DEVRESPONSE	| REQT_CHANMEMBERS | REQT_CHANROUTING | REQT_SENDCHANROUTING))
	{
		bConfigOutOfDate = TRUE;
	}

	if (bConfigOutOfDate)
		state = LtIpConnectState::CONFIG_OUT_OF_DATE;
	else if ((m_nMembers >= 1) && (m_ipAddrCfgServer != 0))
		state = LtIpConnectState::ACTIVE_MEMBER;
	else
		state = LtIpConnectState::NOT_ACTIVE_MEMBER;

	return state;
};

//
// makeNewPacket
//
// Make a new RFC packet from an old one. Just copy to the heap from
// the message actually.
//
boolean	LtIpMaster::makeNewPacket( byte* pOld, int nBytes, byte** ppNew )
{
	byte*		pNew = (byte*)malloc(nBytes);
	boolean		bOk = true;
	*ppNew	= pNew;
	if ( pNew == NULL )
	{	bOk = false;
	}
	else
	{
		memcpy( pNew, pOld, nBytes);
	}
	return bOk;
}

//
// removeClient
//
// Remove a current client by index.
// Deregister it from the Lre
// close it down and delete the object
//
// called under lock
//
void LtIpMaster::removeClient( int idx )
{
	LtLreIpClient*		pClient;

	pClient = m_apClients[idx];
	// if we never received a channel routing packet for this client, then
	// there is no client object either. So don't get too upset.
	if ( pClient )
	{
		m_apClients[idx] = NULL;
		// stop the client so that its doesn't route any more packets
		pClient->stop();
		// at this point the client will return failure for all routing
		// data calls from the server, so that when we call deregister
		// the server will dry up quickly.
		// tell the engine to forget about the client
		// the server may be calling the client during this next call, but
		// when it returns, the client is completely out of the severs kenn.
		if ( m_pServer )
		{	m_pServer->deregisterClient( pClient );
		}
		// remove the client object permanently.
		// clear master to avoid recursion
		pClient->setMaster( NULL );
		delete pClient;
	}
	// discard the channel routing packet as well
	FREEANDCLEAR( m_pktChanRouting[idx] );
	m_baReqtCR[idx] = false;
}

void LtIpMaster::removeMcastClient()
{
    LtLreIpClient*		pClient;

    pClient = m_apMcastClient;
    if (pClient)
    {
		m_apMcastClient = NULL;
        // stop the MULTICAST client so that its doesn't route any more packets
		pClient->stop();
		// at this point the client will return failure for all routing
		// data calls from the server, so that when we call deregister
		// the server will dry up quickly.
		// tell the engine to forget about the client
		// the server may be calling the client during this next call, but
		// when it returns, the client is completely out of the severs.
		if ( m_pServer )
		{	m_pServer->deregisterClient( pClient );
		}
		// remove the client object permanently.
		// clear master to avoid recursion
		pClient->setMaster( NULL );
		delete pClient;
    }
}

void	LtIpMaster::clientBye( LtLreIpClient* pClient )
{
	lock();
	int			idx;
	boolean		bOk = false;

	for ( idx=0; idx<MAX_MEMBERS; idx++ )
	{
		if ( m_apClients[idx] == pClient )
		{
			bOk = true;
			break;
		}
	}
	assert( bOk );
	m_apClients[idx] = NULL;
	// discard the channel routing packet as well
	FREEANDCLEAR( m_pktChanRouting[idx] );
	m_baReqtCR[idx] = false;
	unlock();
}

void	LtIpMaster::clientMcastBye( LtLreIpMcastClient* pClient )
{
    if ( m_apMcastClient == pClient)
        m_apMcastClient = NULL;
}



// getDuplicateChanMember
//
// Check to see if the channel membership packet contains any duplicates,
// and if so, return the first one found.
// This is an illegal configuration - ip address/port is used as a key in the
// channel membership tables, and if we ever have two members with the
// same address/port, bad things happen (EPR 18718).  So if there are duplicates
// just toss the whole packet.
// The first version required IP addresses to be unique.
boolean	LtIpMaster::getDuplicateChanMember( LtIpChanMembers& chm, ULONG& ipa, word& port, boolean& sharedIpAddrs )
{
	int				nMembers;					// members in current membership
	int				i;
	int				j;
	byte*			p;
	LtIpAddPortDate	apdMembers[MAX_MEMBERS];	// parsed data for current members
	boolean			duplicateMember = FALSE;

	nMembers = chm.listSize;
	p = chm.pUcAddresses;
	sharedIpAddrs = FALSE;
	for ( i=0; i<nMembers && !duplicateMember; i++ )
	{
		p = apdMembers[i].parse( p );
		for (j = 0; j < i && !duplicateMember; j++)
		{
			if (apdMembers[j].ipAddress == apdMembers[i].ipAddress)
			{
				// Duplicate IP address. Is this OK?
				if ((apdMembers[j].ipPort == apdMembers[i].ipPort) ||
					backwardCompatibleChan())
				{
					// No, bad channel definition
					duplicateMember = TRUE;
					ipa	= apdMembers[j].ipAddress;
					port = apdMembers[j].ipPort;
				}
				else
				{
					// It's OK, just remember this
					sharedIpAddrs = TRUE;
				}
			}
		}
	}
	return(duplicateMember);
}
//
// setNewMembers
//
// We just received a new channel membership packet
// So it's parsed and valid.
// Now align our membership data with this new list and
// remove any clients that are out of the new list.
// Then ???
//
// called under lock on this object
//
void	LtIpMaster::setNewMembers( LtIpChanMembers& chm )
{
	int				nMembers;					// members in current membership
	int				i;
	byte*			p;
	ULONG			ipAddr;
	USHORT			ipPort;
	int				idx;
	int				iOurNewIndex;
	LtIpAddPortDate	apdMembers[MAX_MEMBERS];	// parsed data for current members
	LtLreIpClient*	apClients[MAX_MEMBERS];
	boolean			bDeleteClients[MAX_MEMBERS];	// clients to delete later
	byte*			pktChanRouting[MAX_MEMBERS];	// The channel routing packets in member order
	boolean			bNeedCRP[MAX_MEMBERS];			// need the packet because what we have is old
	LtIpChanRouting	chr;							// channel routing packet parser
	boolean			bOk;

	memset( pktChanRouting, 0, sizeof(pktChanRouting) );
	memset( bDeleteClients, 1, sizeof(bDeleteClients) );
	memset( apClients, 0, sizeof(apClients) );
	memset( bNeedCRP, 0, sizeof( bNeedCRP ) );
	memset( apdMembers, 0, sizeof( apdMembers ) );

	iOurNewIndex = -1;
	nMembers = chm.listSize;
	p = chm.pUcAddresses;
	for ( i=0; i<nMembers; i++ )
	{
		p = apdMembers[i].parse( p );
		// Set the index of ourselves so sweepClients has something to compare
		// Our address could be local or from a NAT box
#ifdef ILON_100_ROUTER_DEMO
		if (((apdMembers[i].ipAddress == getExternalIpAddr()) ||
				((m_prevIpAddrLocal != 0) && (apdMembers[i].ipAddress == m_prevIpAddrLocal))) &&
			(apdMembers[i].ipPort == m_ipPortLocal))
#else
		if ((apdMembers[i].ipAddress == getExternalIpAddr()) && (apdMembers[i].ipPort == m_ipPortLocal))
#endif
		{	iOurNewIndex = i;
		}
	}

	//
	// build new lists of clients and mark list of deleted clients
	//
	for ( i=0; i<nMembers; i++ )
	{
		ipAddr = apdMembers[i].ipAddress;
		ipPort = apdMembers[i].ipPort;
		idx = findChannelMember( ipAddr, ipPort );
		if ( idx < 0 )
		{	// client has been added to the list
			// nothing to do now, we'll get its channel routing packet soon
		}
		else
		{
			if ( i == iOurNewIndex )
			{
				if (m_pktOurChanRouting == NULL)
				{
					// REMINDER - This may indicate a race condition when starting up a PPP link
					bOk = false;
				}
				else
				{
					bOk = chr.parse( m_pktOurChanRouting, false );
					assert( bOk );
				}
				if ( bOk && (chr.dateTime != apdMembers[i].dateTime) )
				{
					// CS does not have the most current CR packet
					bNeedCRP[i] = true;	// this will trigger sending our own CR
				}
			}
			else
			{
				apClients[i]		= m_apClients[idx];
				pktChanRouting[i]	= m_pktChanRouting[idx];
				// found the client - maybe moved
				// so we arent going to delete it
				bDeleteClients[idx] = false;
				// check the datetime of the packet we have, vs
				// the datetime reported in the channel membership packet we just got.
				// that is, if we have a channel routing packet for this member yet.
				if ( pktChanRouting[i] )
				{
					bOk = chr.parse( pktChanRouting[i], false );
					assert( bOk );
					if ( !bOk || (chr.dateTime != apdMembers[i].dateTime) )
					{
						bNeedCRP[i] = true;
					}
				}
			}
		}
	}

	//
	// remove the missing clients from the old list
	//
	// scan the old list, which might be longer or shorter
	// if the client was not found in the new list, then its hanging out
	// with delete still set. So remove it now.
	//
	for ( i=0; i<m_nMembers; i++ )
	{
		if ( bDeleteClients[i] )
		{
			LtIpAddressStr	ias;
			ias.setIaddr( m_apdMembers[i].ipAddress );
			if (i != m_nOurIndex)	// delete except ours (NOTE:use current, not new index)
			{
				vxlReportEvent("LtIpMaster::setNewMembers - removing member for %s:%d\n",
							ias.getString(), m_apdMembers[i].ipPort);
				removeClient( i );
			}
		}
	}


	// we might want to tell the clients they all have new indeces now.
	// update the master stuff from the new data
	//
	m_nMembers	= nMembers;			// number of new members
	m_nOurIndex = iOurNewIndex;		// update our own current index to the new index
	// make sure we have no stale members in the list
	memset( m_apdMembers, 0, sizeof(m_apdMembers) );
	memset( m_pktChanRouting, 0, sizeof(m_pktChanRouting) );
	memset( m_apClients, 0, sizeof(m_apClients) );
	int		nNeedAnyCRPs = 0;
	for ( i=0; i<nMembers; i++ )
	{
		m_apdMembers[i]			= apdMembers[i];
		m_pktChanRouting[i]		= pktChanRouting[i];
		if ( i == m_nOurIndex )
		{
			if (bNeedCRP[i])
			{
				// CS has different CR datetime or we need to create one and send it to CS
				startWorkerTask( WORK_SendChanRouting );
			}
			m_baReqtCR[i] = false;
		}
		else if ( pktChanRouting[i] == NULL )
		{
			m_baReqtCR[i] = true;
		}
		else
		{
			// report we need the packet if there is a new date for that packet.
			m_baReqtCR[i] = bNeedCRP[i];
		}
		if ( m_baReqtCR[i] )
		{
			nNeedAnyCRPs++;
		}
		m_apClients[i]			= apClients[i];
	}
	if ( nNeedAnyCRPs )
	{
		// we just got a message, so do this right away.
		startRequestInfo( REQT_CHANROUTING, true );

		// There are new members.  Let's start a new session for the benefit
		// of these new members.  We do this here rather than as channel
		// routing is received to minimize the number of times this is done.
		// This is minimized for two reasons.  First, NV ram is updated (an
		// expensive operation) and, more importantly, changing sessions opens
		// windows for packets getting out of order.  We currently handle
		// a single session switch but would not handle a sequence as follows:
		// session 3, session 4, session 5, session 3 (session 3 would be
		// detected as a new session).  We could easily detect this if the
		// RFC specified that sessions were incremented but it only says that
		// they must differ.
		newSession();
	}

	// wheew. We are done. We have a new database and
	// now we can await any new channel routing packets for new clients
	// all old clients are now destroyed.

} // setNewMembers


//
// updateClientRouting
//
// Update channel routing information for a client.
// If there is no client object yet, then create one and set the routing
// if we have an object, then just update the routing.
// Do this in such a way that any on-going activity in the client
// is not disrupted. Back-to-back updates could be happening.
//
// *** CALL UNDER LOCK ***
//
void	LtIpMaster::updateClientRouting( int i, byte* pPktChanRouting )
{
	LtLreIpClient*	pClient = NULL;
	CIpLink*		pLink;
	LtIpAddressStr	ias;

	lock();
	ias.setIaddr( m_apdMembers[i].ipAddress );
	// Our address here could be local or from a NAT box
#ifdef ILON_100_ROUTER_DEMO
	if (((m_apdMembers[i].ipAddress == getExternalIpAddr()) ||
				((m_prevIpAddrLocal != 0) && (m_apdMembers[i].ipAddress == m_prevIpAddrLocal))) &&
		(m_apdMembers[i].ipPort == m_ipPortLocal))
#else
	if ((m_apdMembers[i].ipAddress == getExternalIpAddr()) && (m_apdMembers[i].ipPort == m_ipPortLocal))
#endif
	{
		m_nOurIndex = i;
		if ( m_pktOurChanRouting != pPktChanRouting )
		{
			FREEANDCLEAR( m_pktOurChanRouting );
			m_pktOurChanRouting = pPktChanRouting;
		}
		vxlReportEvent("LtIpMaster::updateClientRouting - store our own chanrtng pkt. ipAddr %s\n"
					   "              our index %d 0x%08x\n",
					ias.getString(), m_nOurIndex, m_pktOurChanRouting
					);
	}
	else if ( m_apClients[i] == NULL )
	{
		vxlReportEvent("LtIpMaster::updateClientRouting - creating member for %s:%d\n",
					ias.getString(), m_apdMembers[i].ipPort
					);
		pClient = new LtLreIpClient( LT_IP_SOCKET, m_pChannel, this);
		assert(pClient);
		m_apClients[i] = pClient;
		pClient->setMaster( this );
		pClient->setServer( m_pServer );
		// now we need to create a new link for the client
		pLink = (CIpLink*)m_pLink;
		// make a new link object for the new client
		pLink = (CIpLink*)pLink->cloneInstance();
#if 0
        // Don't do anything special for multicast
		if (selfInstalledMcastMode())
		{
			pLink->setSrcIp( m_actualIpAddrLocal, m_ipPortLocal );
			pLink->setDstIp( m_selfInstalledMcastAddr, m_ipPortLocal );  // use the MULTICAST port instead of the m_apdMembers[i].ipPort
			pLink->setSelfInstalledMcastAddr(m_selfInstalledMcastAddr);
			// Don't need to set hop count for slaves; for outbound socket only
		}
		else
#endif
		{
		    pLink->setSrcIp( m_ipAddrLocal, m_ipPortLocal );
		    pLink->setDstIp( m_apdMembers[i].ipAddress, m_apdMembers[i].ipPort );
		}
		pClient->registerLink( *pLink );
		// tell this link that it is a slave link to our link
		pLink->setMasterLink( (CIpLink*)m_pLink );
	}
	else // if ( m_apClients[i] != NULL )
	{
		pClient = m_apClients[i];
	}
	if ( pClient )
	{
		// tell client about new routing packet first
		pClient->setRouting( pPktChanRouting );

		if (selfInstalledMcastMode())
		{
			// Tell the LtLreIpClient about the multi-cast address -- for stats display only
			pClient->setIpAddr(m_selfInstalledMcastAddr);
		}

		// now client processes the update itself and informs the server
		// that it has a new update to process
		// now we can safely update our own database and delete the old packet
		FREEANDCLEAR(m_pktChanRouting[i]);
		m_pktChanRouting[i] = pPktChanRouting;
		m_baReqtCR[i] = false;
	}

	unlock();
} // updateClientRouting

void	LtIpMaster::createMcastClientRouting()
{
    // Create the one and only one MULTICAST Ip client
    if  (selfInstalledMcastMode() &&(m_apMcastClient == 0))
    {
        LtLreIpMcastClient* pClient = new LtLreIpMcastClient( LT_IP_SOCKET, m_pChannel, this);
		assert(pClient);
        m_apMcastClient = pClient;
        pClient->setIpMcastAddress(m_selfInstalledMcastAddr);
    	pClient->setMaster( this );
        pClient->setActive(LtIpBase::isActive());
	    pClient->setServer( m_pServer );
		// now we need to create a new link for the client
	    CIpLink* pLink = (CIpLink*)m_pLink;
		// make a new link object for the new client
		pLink = (CIpLink*)pLink->copyInstance();        // cloneInstance();
		pClient->registerLink( *pLink );
		// tell this link that it is a slave link to our link
		pLink->setMasterLink( (CIpLink*)m_pLink );
        pLink->open("ignored");
    }
}

//
// findChannelMember
//
// Find a channel member by IP address in the list and return the index
// to its channel routing packet in the list
//
int	LtIpMaster::findChannelMember( ULONG ipAddress, USHORT ipPort, LtIpAddPortDate* apdi)
{
	int					idx = -1;
	int					i;
	LtIpAddPortDate*	apd = apdi;

	if ( apd == NULL )
	{	apd = m_apdMembers;
	}
	for ( i=0; i<MAX_MEMBERS; i++ )
	{
		// Don't consider backward compatible mode here.
		// Force port to match, to force proper updates if a client's port is changed.
		if ( (apd[i].ipAddress == ipAddress) && (apd[i].ipPort == ipPort) )
		{
			idx = i;
			break;
		}
	}

	return idx;
}

//
// orSubnetsAndGroups
//
// OR the subnet and group masks between two maps
//
void	LtIpMaster::orSubnetsAndGroups( LtRoutingMap& src, LtRoutingMap& dst )
{
	int		i;
	byte	mask[32];
	byte	mask2[32];
	src.getSubnets().get( mask, 0, 32 );
	dst.getSubnets().get( mask2, 0, 32 );
	for ( i=0; i<32; i++ )
	{	mask2[i] |= mask[i];
	}
	dst.getSubnets().set( mask2, 0, 32 );

	src.getGroups().get( mask, 0, 32 );
	dst.getGroups().get( mask2, 0, 32 );
	for ( i=0; i<32; i++ )
	{	mask2[i] |= mask[i];
	}
	dst.getGroups().set( mask2, 0, 32 );
}


//
// sweepLocalClients
//
// create a new channel routing packet from local client information and
// compare with the one we have.
// If the packet is different, then emit it to the configuration server
//
boolean	LtIpMaster::sweepLocalClients()
{
	ULONG				nTickStart = tickGet();
	ULONG				nTickEnd;
	LtIpChanRouting		chro;		// old packet we already have
	LtIpChanRouting		chrn;		// new packet we just built
	LtRoutingMap*		pMaps = NULL;		// array of new domains
	LtIpSubnetNode*		pNodes = NULL;		// array of new subnet-nodes
	LtUniqueId*			pUids = NULL;		// array of new neuronIds
	byte*				pUids2 = NULL;		// building message
	byte*				pDoms = NULL;		// building message
	byte*				pChrn = NULL;		// new message
	LtLreClient*		pClient;
	int					idx = 0;
	int					i;
	int					j;
	int					nMapIdx = 0;
	int					nNodeIdx = 0;
	int					nUidIdx = 0;
	LtRoutingMap		map;
	LtUniqueId			uid;
	LtDomain			dom;
	LtSubnetNode		snn;
	LtGroups			grps;
	LtSubnets			sns;
	int					nNeedsBroadcasts = 0;
	int					nRouterType = LTROUTER_CONFIGURED;
	LtIpNeuronId		uid2;
	LtIpDomain			dom2;
	LtIpSubnetNode		snn2;
	byte*				p;
	byte*				p2;
	boolean				bOk;
	boolean				bNewBuilt = false;
	LtVectorPos			pos;
	boolean				bChanLocked = false;

	pMaps = new LtRoutingMap[MAX_DOMAINS];
	pNodes = new LtIpSubnetNode[MAX_NODES];
	pUids = new LtUniqueId[MAX_UIDS];

	if ( pMaps == NULL || pNodes == NULL || pUids == NULL )
	{
		vxlReportUrgent("Router - pMaps/pNodes/pUids malloc failed\n");
		goto ERROR_EXIT;
	}

	// lock the channel against the comings and goings of clients
	// remember that we did that so we can unlock it as early as possible.
	assert( m_pChannel );
	lockChannel();
	bChanLocked = true;
	while ( m_pChannel->enumStackClients( pos, &pClient ) )
	{
		// get the unique ID first so that we can correlate it to SNNs
		idx = 0;
		while ( pClient->getAddress( idx, &uid ) )
		{
			// must be unique and there must be only 1
			assert(idx==1);
			for ( i=0; i<nUidIdx; i++ )
			{
				if ( uid == pUids[i] )
				{	assert(false);		// must be unique
				}
			}
			if ( nUidIdx >= MAX_UIDS )
			{	countTooMany();
				vxlReportUrgent("Router - too many UIDs: %d\n", nUidIdx );
				goto ERROR_EXIT;
			}

			pUids[nUidIdx] = uid;
			nUidIdx++;
		}

		// scan the client as a node first
		idx = 0;
		while ( pClient->getAddress( idx, &dom, &snn, &grps ) )
		{
			for ( i=0; i<nMapIdx; i++ )
			{
				if ( dom == pMaps[i].getDomain() )
				{	break;
				}
			}
			// May not find it if this is a node
			if ( i == nMapIdx )
			{
				if ( nMapIdx >= MAX_DOMAINS )
				{	countTooMany();
					vxlReportUrgent("Router - too many domains: %d\n", nMapIdx );
					goto ERROR_EXIT;
				}
				// Not here, we've got to create it
				pMaps[i].setDomain( dom );
				pMaps[i].setGroups( grps );
				// Explicitly do not merge the SUBNETs into the mask
				// since we dont "Route To" any subnet that we find
				// this way. But only the explicit node that we find here.
				// use LT_INVALID_ROUTER_TYPE to imply a node
				pMaps[i].setRouterType( LT_INVALID_ROUTER_TYPE );
				nMapIdx++;
			}
			if ( nNodeIdx >= MAX_NODES )
			{	countTooMany();
				vxlReportUrgent("Router - too many nodes: %d\n", nNodeIdx );
				goto ERROR_EXIT;
			}

			// fill in our subnet Node structure for RFC message
			pNodes[nNodeIdx].subnet			= snn.getSubnet();
			pNodes[nNodeIdx].node			= snn.getNode();
			pNodes[nNodeIdx].domainIndex	= nMapIdx-1;
			pNodes[nNodeIdx].neuronIdIndex	= nUidIdx-1;
			nNodeIdx++;
		}

		idx = 0;
		while ( pClient->getExternalRoute( idx, &map, NULL ) )
		{
			// merge domains
			for ( i=0; i<nMapIdx; i++ )
			{
				if ( map.getDomain() == pMaps[i].getDomain() )
				{
					orSubnetsAndGroups( map, pMaps[i] );
					nRouterType = maximizeNodeType( map.getRouterType(), nRouterType );
					break;
				}
			}
			if ( i == nMapIdx )
			{
				if ( nMapIdx >= MAX_DOMAINS )
				{	countTooMany();
					vxlReportUrgent("Router - too many domains: %d\n", nMapIdx );
					goto ERROR_EXIT;
				}
				pMaps[nMapIdx] = map;
				nMapIdx ++ ;
			}
		}
		// Does any client need all broadcasts?
		nNeedsBroadcasts |=
			pClient->getNeedAllBroadcasts()? LTROUTER_ALLBROADCAST : 0;
	}

	// Check for protocol analyzer mode
	if (protocolAnalyzerMode())
	{
		// The only way to do this is to pretend the "router" type is a repeater
		nRouterType = LTROUTER_REPEATER;
	}
#if !PRODUCT_IS(VNISTACK)
	// Don't generate an empty channel routing packet for the all products (except LNS) if we already have one.
	// This happens on shutdown. LNS needs to do this, because client apps can come and go.
	if ((nUidIdx == 0) && (m_pktOurChanRouting != NULL))
	{
#if TESTSWEEP
		vxlPrintf("sweep - No Uids, don't create new CHR\n");
#endif // TESTSWEEP
		goto ERROR_EXIT;
	}
#endif

	// If we dont have a channel routing packet yet, then
	// we always want to build one.
	if ( m_nOurIndex == -1 || m_pktOurChanRouting == NULL )
	{
#if TESTSWEEP
		vxlPrintf("sweep - new channel routing packet. our index %d our packet 0x%08x\n",
			m_nOurIndex, m_pktOurChanRouting );
#endif // TESTSWEEP
		goto DIFFERENT;
	}
	p = m_pktOurChanRouting;
	if ( !chro.parse( p, false )  )
	{	assert(false);
		goto ERROR_EXIT;
	}
	if ( ( (chro.lonTalkFlags & LTROUTER_ALLBROADCAST) != nNeedsBroadcasts ) ||
		 ( !(chro.lonTalkFlags & SUPPORTS_EIA_AUTH) != backwardCompatibleChan() ) ||
		 ( chro.domainBytes != LtIpDomain::SIZE * nMapIdx ) ||
		 ( chro.neuronIdBytes != LtIpNeuronId::SIZE * nUidIdx ) ||
		 ( chro.subnetNodeBytes != LtIpSubnetNode::SIZE * nNodeIdx ) ||
		 ( chro.routerType != nRouterType )
		 )
	{
#if TESTSWEEP
		vxlPrintf("sweep - lontalk flags was %0x is %0x\n",
			chro.lonTalkFlags, (nNeedsBroadcasts |
						(!backwardCompatibleChan() ? SUPPORTS_EIA_AUTH : 0)));
		vxlPrintf("sweep - domain bytes was %d is %d\n",
						chro.domainBytes, LtIpDomain::SIZE * nMapIdx );
		vxlPrintf("sweep - uid bytes was %d is %d\n",
						chro.neuronIdBytes, LtIpNeuronId::SIZE * nUidIdx );
		vxlPrintf("sweep - subnet node bytes was %d is %d\n",
						chro.subnetNodeBytes, LtIpSubnetNode::SIZE * nNodeIdx );
		vxlPrintf("sweep - router type was %d is %d\n",
						 chro.routerType, nRouterType );
#endif // TESTSWEEP
		goto DIFFERENT;
	}
	// if our ipaddress or uc port changes, then generate a new packet
	// Our address could be local or from a NAT box
	if ( ( chro.ipUcAddress != getExternalIpAddr() ) ||
		 ( chro.ipUcPort != m_ipPortLocal )
		)
	{
#if TESTSWEEP
		vxlPrintf("sweep - ip Add was %d is %d port was %d is %d\n",
					chro.ipUcAddress, getExternalIpAddr(),
					chro.ipUcPort, m_ipPortLocal );
#endif // TESTSWEEP

		goto DIFFERENT;
	}
	// compare UIDs to make sure they are the same. Order might be different
	p = chro.pNeuronIds;
	for ( i=0; i<nUidIdx; i++ )
	{
		p = uid2.parse( p );
		uid.set( uid2.id );
		bOk = false;
		for ( j=0; j<nUidIdx; j++ )
		{
			if ( pUids[j] == uid )
			{	bOk = true;
				break;
			}
		}
		if ( ! bOk )
		{
#if TESTSWEEP
			vxlPrintf("sweep - neuron IDs changed\n" );
#endif // TESTSWEEP
			goto DIFFERENT;
		}
	}
	// Compare domains to make sure they are the same. Order might be different.
	p = chro.pDomains;
	for ( i=0; i<nMapIdx; i++ )
	{
		p = dom2.parse( p );
		dom.setDomain( dom2.domainLength, dom2.domainBytes );
		bOk = false;
		for ( j=0; j<nMapIdx; j++ )
		{
			if ( pMaps[j].getDomain() == dom )
			{	bOk = true;
				break;
			}
		}
		if ( ! bOk )
		{
#if TESTSWEEP
			byte	 domb[10];
			dom.getData( (byte*)&domb );
			vxlPrintf("sweep - domain codes or lengths changed\n"
							"        old len %d  0x%02x%02x%02x%02x%02x%02x\n",
							dom.getLength(), domb[0],domb[1],domb[2],domb[3],domb[4],domb[5]
							);
#endif // TESTSWEEP
			goto DIFFERENT;
		}
		// Ok - we found the same domain in the list, now compare the
		// subnet and group masks for equal.
		sns.set( dom2.subnetMask, 0, LtIpDomain::MASKLEN );
		grps.set( dom2.groupMask, 0, LtIpDomain::MASKLEN );
		if ( pMaps[j].getSubnets() == sns &&
			 pMaps[j].getGroups() == grps
			 )
		{}
		else
		{
#if TESTSWEEP
			vxlPrintf("sweep - subnet or group masks changed\n" );
#endif // TESTSWEEP
			goto DIFFERENT;
		}
	}

	// Compare subnet-nodes to make sure they are the same. Order might be different.
	p = chro.pSubnetNodes;
	for ( i=0; i<nNodeIdx; i++ )
	{
		// for each of the old subnet/nodes
		p = snn2.parse( p );
		bOk = false;
		// look through the new subnet nodes til we find one that matches
		for ( j=0; j<nNodeIdx; j++ )
		{
			if ( pNodes[j].subnet == snn2.subnet &&
				 pNodes[j].node == snn2.node)
			{
				// now pull out the old domain information
				p2 = chro.pDomains+(snn2.domainIndex * LtIpDomain::SIZE );
				dom2.parse( p2 );
				map.getDomain().set( dom2.domainBytes, dom2.domainLength );
				map.getSubnets().set( dom2.subnetMask, 0, LtIpDomain::MASKLEN );
				map.getGroups().set( dom2.groupMask, 0, LtIpDomain::MASKLEN );
				// now compare with the new map we built
				// but be sure and ignore the router type field
				map.setRouterType( pMaps[pNodes[j].domainIndex].getRouterType() );
				if ( map == pMaps[pNodes[j].domainIndex] )
				{}
				else
				{	continue;	// keep looking for another
				}
				p2 = chro.pNeuronIds + (snn2.neuronIdIndex * LtIpNeuronId::SIZE );
				uid.set( p2 );
				if ( uid == pUids[pNodes[j].neuronIdIndex] )
				{	bOk = true;
					break;
				}
				// nope, gotta keep looking incase somebody else matches
			}
		}
		if ( ! bOk )
		{
#if TESTSWEEP
			vxlPrintf("sweep - subnet node addresses changed\n" );
#endif // TESTSWEEP

			goto DIFFERENT;
		}
	}
	// duplicate code
	// if our address or port changes, must build a new packet
	// Our address could be local or from a NAT box
	if ( chro.ipUcAddress != getExternalIpAddr() || chro.ipUcPort != m_ipPortLocal )
	{
#if TESTSWEEP
		vxlPrintf("sweep - redundant check on ip address and port\n" );
#endif // TESTSWEEP

		goto DIFFERENT;
	}

#if TESTSWEEP
	vxlPrintf("sweep - No differences found\n");
#endif // TESTSWEEP
	// Couldnt find anything different, so it all must be the same
	goto ERROR_EXIT;

DIFFERENT:
#if TESTSWEEP
	vxlPrintf("sweep - Found differences\n");
#endif // TESTSWEEP
	if ( bChanLocked )
	{	unlockChannel();
		bChanLocked = false;
	}
	// the new packet is different, so build it and send it
	chrn.dateTime			= getDatetime();
	chrn.ipFlags			= LTIPPROTOCOL_UDP;
	chrn.lonTalkFlags		= nNeedsBroadcasts | (!backwardCompatibleChan() ? SUPPORTS_EIA_AUTH : 0);
	chrn.routerType			= nRouterType;
	chrn.ipNodeType			= LTNODE_LTIPROUTER;	// IKP10212003: fix EPR 30778
    chrn.ipMcPort			= m_selfInstalledMcastAddr ? m_ipPortLocal : 0;    // if MULTICAST, use the same port with local
	chrn.ipMcAddress		= m_selfInstalledMcastAddr;
	chrn.ipUcAddress		= getExternalIpAddr();	// local or NAT box
	chrn.ipUcPort			= m_ipPortLocal;
	chrn.neuronIdBytes		= nUidIdx * LtIpNeuronId::SIZE;
	chrn.domainBytes		= nMapIdx * LtIpDomain::SIZE;
	chrn.subnetNodeBytes	= nNodeIdx * LtIpSubnetNode::SIZE;
	chrn.pSubnetNodes		= (byte*)pNodes;

	// convert the uids and domains from internal to external form
	pUids2 = (byte*)malloc( LtIpNeuronId::SIZE * nUidIdx );
	pDoms  = (byte*)malloc( LtIpDomain::SIZE * nMapIdx );
	if ( pUids2 == NULL || pDoms == NULL )
	{
		vxlReportUrgent("Router - pUids or pDoms malloc failed\n");
		goto ERROR_EXIT;
	}
	chrn.pDomains = pDoms;
	chrn.pNeuronIds = pUids2;
	p = pUids2;
	for ( i=0; i<nUidIdx; i++ )
	{
		//uid.get( uid2.id );
        pUids[i].get( uid2.id );
		p = uid2.build( p );
	}
	p = pDoms;
	for ( i=0; i<nMapIdx; i++ )
	{
		pMaps[i].getDomain().getData( dom2.domainBytes );
		dom2.domainLength = pMaps[i].getDomain().getLength();
		pMaps[i].getSubnets().get(dom2.subnetMask, 0, LtIpDomain::MASKLEN );
		pMaps[i].getGroups() .get(dom2.groupMask, 0, LtIpDomain::MASKLEN );
		p = dom2.build( p );
	}
	// ask the size of the new packet, and then allocate it
	// then build it and replace the packet that we have
	setPktExtHdrData(&chrn);
	j = chrn.size();
	pChrn = (byte*)malloc( j );
	p = pChrn;
	p = chrn.build( p );
	assert(pChrn+j == p);
	FREEANDCLEAR( m_pktOurChanRouting );
	m_nPktOCRSize = j;
	m_pktOurChanRouting = pChrn;
	bNewBuilt = true;
	vxlReportEvent("LtIpMaster::sweepLocalClients[%d] - Created new channel routing packet 0x%08x\n",
					m_nIndex, pChrn );
	pChrn = NULL;	// dont free the good packet we just built
	// make sure we write out our packet
	startWorkerTask( WORK_WritePersist );
#if 0 // dump packet we just built
	vxlReportUrgent("sweepLocalClients - new channel routing packet\n");
	chrn.dump();
#endif // new channel routing packet

ERROR_EXIT:
	if ( bChanLocked )
	{	unlockChannel();
		bChanLocked = false;
	}
	if ( !bNewBuilt )
	{
		vxlReportEvent("LtIpMaster::sweepLocalClients[%d] - No new channel routing packet\n",
					m_nIndex );
	}
	// delete all the temporary storage
	DELETEANDCLEAR(pMaps);
	DELETEANDCLEAR(pNodes);
	DELETEANDCLEAR(pUids);
	FREEANDCLEAR(pUids2);
	FREEANDCLEAR(pDoms);
	FREEANDCLEAR(pChrn);

	if ( m_pktOurChanRouting == NULL )
	{
		vxlReportEvent("LtIpMaster::sweepLocalClients[%d] - No channel routing packet built\n",
					m_nIndex );
	}

	nTickEnd = tickGet();
	nTickStart = ticksToMs(nTickEnd - nTickStart);
	vxlReportEvent("LtIpMaster::sweepLocalClients - took %d ms\n", nTickStart );

	return bNewBuilt;
} // sweepLocalClients


//
// doRFCin
//
// Read and parse an RFC packet
//
void	LtIpMaster::doRFCin()
{
	int				nBytes = 0;
	byte*			pData = NULL;
	byte*			pNewPacket;
	int				i;
	boolean			bOk = false;
	boolean			bFromCnfgSrvr = false;
	byte*			p;
	LtIpAddressStr	ias;
	LtIpAddressStr	ias2;
	ULONG			ipSrcAddr;
	word			ipSrcPort;
	LtIpPktBase		pkt;
	LtIpChanMembers	chm;
	LtIpChanRouting	chr;
	LtIpDevRegister dvr;
	LtIpTimeSynch	tms;
	LtIpEchConfig	ecfg;
	LtIpResponse	rsp;
	LtIpEchControl	ctl;
	LtIpEchMode     mode;
	LtIpEchVersion  echVer;
	LtPktInfo*		pPkt = NULL;
	LtQue*			pQue = NULL;

	ias.setIaddr( m_ipAddrLocal );
	do
	{
		if ( !m_qRFCin.removeTail( &pQue ) )
		{	break;
		}
		pPkt = (LtPktInfo*)pQue;
		p = pData = pPkt->getDataPtr();
		nBytes = pPkt->getDataSize();

		bOk = pkt.parse( p, false );
		if ( !bOk )
		{	countInvalidPackets();
			break;
		}

		if ( m_bAuthenticate && !pPkt->ignoreAuthentication() )
		{	// authenticate the RFC packet
			int	nDataLen = MAX( LtMD5::LTMD5_DIGEST_LEN, nBytes - LtMD5::LTMD5_DIGEST_LEN );

			bOk = false;
			// Theoretically, the following condition should be met, but
			// if we are switching modes, it may not be, so ignore it
			//if (backwardCompatibleChan() || pkt.isMarkedSecure(pData))
			{
				bOk = m_md5.checkDigestWithSecret( pData, nDataLen , &pData[nDataLen] );
				if (!bOk)
				{
					// Try the other auth algorithm, and allow it
					bOk = m_md5.checkDigestWithSecret( pData, nDataLen , &pData[nDataLen], true );
					if (bOk)
						countAltAuthUsed();
				}
			}
			if ( !bOk )
			{	countAuthFailures();
			    // Allow certain requests anyway
				switch (pkt.packetType)
				{
					case PKTTYPE_REQDEVCONFIG :			// request device configuration
					case PKTTYPE_ECHCONFIGREQ :			// request echelon private config
					case PKTTYPE_ECHVERSREQ :			// request version number
					case PKTTYPE_ECHMODEREQ :			// request mode
					case PKTTYPE_ECHDEVIDREQ :			// request device ID
						bOk = true;
						break;
					default:
						break;
				}
				// Bail out if not approved above
				if (!bOk)
					break;
			}
			nBytes = nDataLen;
		}

		// Reject foreign vendor packets
		if ((pkt.vendorCode != VENDOR_STANDARD) && (pkt.vendorCode != VENDOR_ECHELON))
			break;

		// Before processing each message, check for an "unknown source port"
		// diagnostic request
		unknownPortDiagnostic();

		// is the packet from the Configuration manager?
		// IKP08292003: If m_ipAddrCfgServer is NULL, assume pkt is from CfgServer
		// we only accept certain types of packets from the configuration manager.
		//
		ipSrcAddr = pPkt->getIpSrcAddr();
		ias2.setIaddr( ipSrcAddr );
		ipSrcPort = pPkt->getIpSrcPort();
		if ((m_ipAddrCfgServer == 0) ||
			( ipSrcAddr == m_ipAddrCfgServer && ipSrcPort == m_ipPortCfgServer ) ||
			// try match against extended header info
			(pkt.bHasExtHdr &&
				((pkt.extHdrNatIpAddr == m_ipAddrCfgServer) || (pkt.extHdrLocalIpAddr == m_ipAddrCfgServer)) &&
				(pkt.extHdrIpPort == m_ipPortCfgServer)))
		{	bFromCnfgSrvr = true;
			// we heard from the server, so expect it
			// to respond quickly
			//m_nReqtCount = 0;

			// count the message
			m_CSmsgRcvd++;
		}
		else
		{	bFromCnfgSrvr = false;
		}
		// dump the packet
		//pkt.dumpPacket( "doRFCin", p, pkt.packetSize, true );

		switch ( pkt.packetType )
		{
		case PKTTYPE_DEVCONFIGURE:

			// we allow these packets from a node other than
			// the config server so a new config server can show up.
			// One of these and must be ok for us to use.
			// UcPort, McAddr, McPort can be set by this packet
			// and a bunch of other stuff too.
			p = pData;
			bOk = dvr.parse( p, false );
			vxlReportEvent("LtIpMaster::doRFCin %s- Device configure\n",
							ias.getString() );
			//dvr.dump();

			if ( bOk )
			{
				byte *pNewPkt;

				// Assuming we are accepting this packet and will update ourselves
				// with any changes, save a copy of the packet with its date/time
				// so we will remain in sync with the config server.
				pNewPkt = (byte*)malloc(dvr.packetSize);
				if (pNewPkt)
				{
					memcpy(pNewPkt, p, dvr.packetSize);
					FREEANDCLEAR( m_pktDevRegister );
					m_pktDevRegister = pNewPkt;
				}

				// validate the packet for only some things changed
				doSendAck( ACK_OK, 0, ipSrcAddr, ipSrcPort );
				boolean	bNewName = false;
				// save the name if required

				// Note: nameLen includes zero terminator
				bNewName = ( dvr.nameLen <= EIA852_MAXNAMELEN+1 ) &&
							( dvr.nameLen > 0) &&
							( 0 != memcmp( m_acName, dvr.pName, dvr.nameLen ) );
				if ( bNewName )
				{	memcpy( m_acName, dvr.pName, dvr.nameLen );
					m_acName[dvr.nameLen-1] = 0;
				}
				//
				// setNewServers shuts down the link
				// but we are holding a packet so it won't shut down
				//
				if ( pPkt )
				{	pPkt->release();
					pPkt = NULL;
				}

				// The configuration or Ntp server may have changed
				boolean bNewServers = setNewServers( dvr );
#ifdef ILON_100_ROUTER_DEMO
				if ( bNewServers || bNewName || (m_prevIpAddrLocal != 0))
#else
				if ( bNewServers || bNewName )
#endif
				{	// schedule us to write new persistance with the
					// updated server information
#ifdef ILON_100_ROUTER_DEMO
					m_prevIpAddrLocal = 0;	// Assume the CS got our new address, so save it
#endif
					startWorkerTask( WORK_WritePersist );
				}
				// This means the only thing that we need to do when we get
				// the packet, is to check the date of the Channel Membership
				// list vs ours and set the request bit
				// so forget we requested the device response if
				// we even did so, then check the dates
				cancelRequest( REQT_DEVRESPONSE );
				if ( m_dtPktCM == 0 || m_dtPktCM < dvr.chanMemDatetime ||
					m_pktChanMembers == NULL
					)
				{
					// IKP04212003: Send our channel routing packet to the config server
					// always send it regardless of whether a new one is built
					// there's no request, so just make up a request id
					if ( doSendChanRouting( 0, 0, NULL, true, true ) )
					{	startRequestInfo( REQT_SENDCHANROUTING );
					}

					// IKP04212003: Hopefully when this request comes in, config server has the
					// information needed to build the Channel Membership pkt with CR timestamp
					// sent earlier in the Channel Routing Pkt.
					startRequestInfo( REQT_CHANMEMBERS, true );
				}
			}
			break;

		case PKTTYPE_CHNMEMBERS:
			// must only come from the Configure Manager, if not ignore
			if ( !bFromCnfgSrvr )
			{
				ias2.setIaddr( ipSrcAddr );
				vxlReportEvent("LtIpMaster::doRFCin - %s from %s channel members ignored\n",
								ias.getString(), ias2.getString() );
				bOk = false; // Don't bump the data incarnation
				break;
			}
			// one of these, and always believe it if it's later than
			// the one we have.
			p = pData;
			bOk = chm.parse( p, false );
			if ( bOk && ( chm.listSize > MAX_MEMBERS ) )
			{
				vxlReportUrgent("LtIpMaster::doRFCin - %s too many members %d in channel\n",
								ias.getString(), chm.listSize );
				bOk = false;
			}

			boolean hasSharedIpAddrs;
			if (bOk)
			{
				ULONG duplicateIpAddr = 0;
				word duplicatePort = 0;
				boolean foundDuplicate = getDuplicateChanMember( chm, duplicateIpAddr, duplicatePort, hasSharedIpAddrs);
				if (foundDuplicate)
				{
					LtIpAddressStr	iasDup;
					iasDup.setIaddr( duplicateIpAddr );

					vxlReportUrgent("LtIpMaster::doRFCin - %s member %s:%d is a duplicate\n",
									ias.getString(), iasDup.getString(), duplicatePort);
					bOk = false;
				}
			}
			if ( bOk && ( m_dtPktCM < chm.dateTime ) )
			{
				// validate the dateTime of the packet
				// validate the packet for only some things changed
				vxlReportEvent("LtIpMaster::doRFCin - %s channel members\n",
								ias.getString() );
				chm.dump();

				bOk = makeNewPacket( pData, chm.packetSize, &pNewPacket );
				if ( bOk )
				{
					lock();
					FREEANDCLEAR(m_pktChanMembers);
					m_pktChanMembers = pNewPacket;
					m_nPktCMSize = chm.packetSize;
					m_dtPktCM = chm.dateTime;
					m_hasSharedIpAddrs = hasSharedIpAddrs;
					m_nDataIncarnation++;
					// All hell might just have broken loose
					// There might be new members that we dont have channel
					// routing packets for yet.
					// There might be members missing that we should tear down.
					setNewMembers( chm );
					m_bDataValid = true;
					cancelRequest( REQT_CHANMEMBERS );
					unlock();
				}
			}
			else if ( bOk && ( m_dtPktCM == chm.dateTime ) )
			{
				lock();
				cancelRequest( REQT_CHANMEMBERS );
				vxlReportEvent("LtIpMaster::doRFCin - %s up to date channel members ignored\n",
								ias.getString() );
				// we just got a message, so the delay is zero.
				startRequestInfo( REQT_CHANROUTING, true );
				unlock();
			}
			else if ( bOk )
			{
				lock();
				cancelRequest( REQT_CHANMEMBERS );
				// we just got a message, so the delay is zero.
				startRequestInfo( REQT_CHANROUTING, true );

				vxlReportEvent("LtIpMaster::doRFCin - %s stale channel members ignored \n",
								ias.getString() );
				unlock();
			}
			else
			{
				vxlReportEvent("LtIpMaster::doRFCin - %s corrupt channel members ignored \n",
								ias.getString() );
				countInvalidPackets();
			}

			break;

		case PKTTYPE_CHNROUTING:
			if ( !bFromCnfgSrvr )
			{
				ias2.setIaddr( ipSrcAddr );
				vxlReportEvent("LtIpMaster::doRFCin - %s from %s channel routing ignored\n",
								ias.getString(), ias2.getString() );
				countPacketsDropped();
				bOk = false; // Don't bump the data incarnation
				break;
			}
			// as many of these as members in the Chan Membership packet.
			// Channel membership packet must arrive first, or we toss the
			// Channel routing packet out.
			p = pData;
			bOk = chr.parse( p, false );

			// log that we got a channel routing
			ias2.setIaddr( chr.ipUcAddress );
			ias.setIaddr( m_ipAddrLocal );
			vxlReportEvent("LtIpMaster::doRFCin - %s received channel routing for %s:%d\n",
						ias.getString(), ias2.getString(), chr.ipUcPort );
			if ( chr.ipUcAddress == 0 )
			{
				vxlReportEvent("LtIpMaster::doRFCin - %s discard channel routing for %s\n",
							ias.getString(), ias2.getString() );
				break;
			}
			lock();
			while ( bOk )
			{
				// validate the dateTime of the packet
				// validate the packet for only some things changed
				// Our address could be local or from a NAT box
#ifdef ILON_100_ROUTER_DEMO
                if (((chr.ipUcAddress == getExternalIpAddr()) ||
						((m_prevIpAddrLocal != 0) && (chr.ipUcAddress == m_prevIpAddrLocal))) &&
					(chr.ipUcPort == m_ipPortLocal))
#else
                if ((chr.ipUcAddress == getExternalIpAddr()) && (chr.ipUcPort == m_ipPortLocal))
#endif
				{	// its our very own packet, so don't create a client for us
					vxlReportEvent("LtIpMaster::doRFCin - %s discard our own channel routing packet\n",
									ias.getString() );
					bOk = false;
					break; // leave while
				}
				else
				{
					// There is a possibility that this may be our own CHR, but
					// we can't tell because we are behing a NAT box and we don't
					// know it yet, thus the "external" IP address doesn't match.
					// We must not create an LRE client for our own CHR. For one thing,
					// this would cause a duplicate UID client that discards Neuron ID
					// messages addressed to us (e.g. by LonMaker). Check the
					// UID in the packet to see if it matches any of our own.
					if (chrPktHasLocalUid(chr))
					{
						vxlReportEvent("LtIpMaster::doRFCin - %s discard our own channel routing packet (from UID check)\n",
										ias.getString() );
						bOk = false;
						break; // leave while
					}
				}

				// We are satisfied that it is not our own CHR, so look it
				// up in the CHM and create routing for it.
				i = findChannelMember( chr.ipUcAddress, chr.ipUcPort );
				if ( i>=0 )
				{
					// if the packet datetime is == to the datetime we want, then use it
					// otherwise discard packet. Get the datetime from the chan members packet.
					ULONG	currentDT = m_apdMembers[i].dateTime;
					if ( currentDT && m_pktChanRouting[i] )
					{	// Take the packet if it has the same datetime as the latest
						// channel members packet
						bOk = ( chr.dateTime == currentDT );
					}
					else
					{
						bOk = true;
					}

					static int iBadChrCount = 0;

					if ( bOk )
					{
						bOk = makeNewPacket( pData, chr.packetSize, &pNewPacket );
						m_nDataIncarnation++;
						// We might have a channel member waiting for this packet
						// There might be new members that we dont have channel
						// routing packets for yet.
						// There might be members missing that we should tear down.
						updateClientRouting(i, pNewPacket);
						vxlReportEvent("LtIpMaster::doRFCin - %s updated channel routing for %s:%d\n",
									ias.getString(), ias2.getString(), chr.ipUcPort );
						// doSendAck( ACK_OK, 0, ipSrcAddr, ipSrcPort ); // IKP04182003: -- removed to follow RFC
						iBadChrCount = 0;	// as long as we receive good CHR, clear iBadChrCount.
					}
					else
					{	// If we got a channel routing packet, and it was not what we wanted, then
						// continue asking for the proper one.
						//m_baReqtCR[i] = false;
						LtIpDateTimeStr dts;
						LtIpDateTimeStr dts1;
						vxlReportEvent( "LtIpMaster::doRFCin - %s ignored channel routing for %s\n"
										"             new pkt %s stored pkt %s\n",
									ias.getString(), ias2.getString(),
									dts.getString( chr.dateTime ), dts1.getString( currentDT ) );
						doSendAck( ACK_BAD_MESSAGE, 0, ipSrcAddr, ipSrcPort );

						// something is really wrong, it appears that CHR packet DateTime does not match with the
						// CHM DateTime for this device.  We should re-request CHM.
						if (++iBadChrCount > 3)
							startRequestInfo( REQT_CHANMEMBERS, true );
					}
				}
				else
				{
					// why did we get a channel routing packet that we dont
					// have in the membership list?
					countInvalidPackets();
					ias2.setIaddr( chr.ipUcAddress );
					vxlReportEvent("LtIpMaster::doRFCin - %s channel routing for %s not in channel members\n",
							ias.getString(), ias2.getString() );
					chr.dump();
					//assert(false);
					bOk = false;
				}
				// we don't cancel the request for a channel routing packet
				// we let the routine do that itself.
				// but we zero the count since we heard from the server recently.
				//m_nReqtCount = 0;
				// Check at end of this loop to see if we need to request more information
				// startWorkerTask( WORK_RequestInfo );
				break; // leave while
			} // while
			unlock();
			break;

		case PKTTYPE_SENDLIST:
			if ( !bFromCnfgSrvr ) break;
			// ignore send list packets completely
			countSendListPackets();
			bOk = false; // Don't bump the data incarnation
			break;
		case PKTTYPE_REQCHNMEMBERS :		// request channel membership
		case PKTTYPE_REQSENDLIST :			// request send list
		case PKTTYPE_REQDEVCONFIG :			// request device configuration
		case PKTTYPE_REQCHNROUTING :		// request channel routing
		case PKTTYPE_ECHCONFIGREQ :			// request echelon private config
		case PKTTYPE_REQSTATISTICS :		// request statistics
		case PKTTYPE_ECHVERSREQ :			// request version number
		case PKTTYPE_ECHMODEREQ :			// request mode
		case PKTTYPE_ECHDEVIDREQ :			// request device ID
			// these requests are valid from any node.
			// The request can only be a request for data from this node.
			// We will not respond to requests for data about any node
			// but ourselves.
			// Process the packet, but do not release it since we do that here.
			doRFCRequest( pkt.packetType, pPkt );
			bOk = false; // Don't bump the data incarnation
			break;

		case PKTTYPE_RESPONSE :				// response to one of our packets
			// If the response is for a request of a channel routing packet.
			// then we must have not gotten the packet. So clear our request
			// for that packet.
			p = pData;
			bOk = rsp.parse( p, false );
			if ( bOk )
			{
				if ( rsp.requestId >= 1000 )
				{	// it's a response for our request for a channel routing packet
					// so the only reason we could get an ack rather than a packet
					// is that the packet is not available.
					// so stop requesting it.
					//
					int		idx = rsp.requestId - 1000;
					if ( idx >= 0 && idx <= m_nMembers )
					{	m_baReqtCR[idx] = false;
					}
				}
				else if ( m_nReqtMask & REQT_SENDCHANROUTING )
				{	// we received an ack for our channel routing packet, so cancel the request
					// to send it out.
					vxlReportEvent( "LtIpMaster::doRfcIn - Ack received for Channel Routing\n");
					cancelRequest( REQT_SENDCHANROUTING );
				}
			}
			bOk = false; // Don't bump the data incarnation
			break;

		case PKTTYPE_TIMESYNCHREQ:
			p = pData;
			bOk = tms.parse( p, false );
			if ( bOk )
			{
				byte	buf[ LtIpTimeSynch::MAX_PKT_SIZE +4];
				tms.packetType = PKTTYPE_TIMESYNCHRSP;
				tms.dateTimeRsp = getDatetime(false);
				// Get time stamp without regard to time sync status.
				tms.timeStampRsp = getTimestamp(false);
				setPktExtHdrData(&tms);
				tms.build( buf );
				sendNewPacketTo( buf, tms.packetSize, ipSrcAddr, ipSrcPort );
			}
			else
			{	countInvalidPackets();
			}
			bOk = false; // Don't bump the data incarnation
			break;
		case PKTTYPE_TIMESYNCHRSP:
			countInvalidPackets();
			bOk = false; // Don't bump the data incarnation
			break;

		case PKTTYPE_ECHCONFIG :
			// if this message is not from the Config Manager, then ignore it
			// since they could mess us up, like turning on Authentication.
			if ( !bFromCnfgSrvr ) break;
			p = pData;
			bOk = ecfg.parse( p, false );
			if ( bOk )
			{
				boolean		bChanged = false;
				boolean		bTOSChanged = false;

				if ((m_bAggregate		!= ecfg.bAggregate) ||
					(m_nAggregateMs		!= ecfg.aggTimerMs)	||
					(m_bBWLimit			!= ecfg.bBwLimit) ||
					(m_nBWLimitKBPerSec	!= ecfg.bwLimitKbPerSec) ||
					(m_bReorderPackets	!= !ecfg.bNoReorder) ||
					(m_nEscrowTimeMs	!= ecfg.escrowTimerMs) ||
					(m_bAuthenticate	!= ecfg.bAuthenticate) ||
					(m_bUseTOSbits		!= ecfg.bUseTosBits) ||
					(m_nTOSbits			!= ecfg.TosBits))
				{
					bChanged = true;
				}

				// Special check for actually setting the TOS bits in the stack
				if ((m_bUseTOSbits != ecfg.bUseTosBits) ||
					(ecfg.bUseTosBits && (m_nTOSbits != ecfg.TosBits)))
				{
					bTOSChanged = true;
				}

				if (bChanged)
				{
					m_bAggregate			= ecfg.bAggregate;
					m_nAggregateMs			= ecfg.aggTimerMs;
					m_bBWLimit				= ecfg.bBwLimit;
					m_nBWLimitKBPerSec		= ecfg.bwLimitKbPerSec;
					m_bReorderPackets		= !ecfg.bNoReorder;
					m_nEscrowTimeMs			= ecfg.escrowTimerMs;
					m_bUseTOSbits			= ecfg.bUseTosBits;
					m_nTOSbits				= ecfg.TosBits;
					setAuthentication(ecfg.bAuthenticate, false);

					// set the bit so we will write persistence soon
					startWorkerTask( WORK_WritePersist );
				}
				doSendAck( ACK_OK, 0, ipSrcAddr, ipSrcPort );

				// If the timezone was sent to us, then set it.
				// This is not saved in the normal nonvol data,
				// but is written directly to the platform support code.
				// Only do this if necessary, since it is expensive,
				// and do it after the ACK is sent.
				if ( ecfg.bUseTZ )
				{
					char tz[LtIpEchConfig::ECHCFG_TZSIZE];
					LtGetTimeZone(tz);
					if (strcmp(tz, ecfg.szTimeZone))
					{
						LtSetTimeZone( ecfg.szTimeZone );
					}
				}

				vxlReportEvent("doRFCin - Echelon Config Aggrega %s %5d ms   Reorder %s %5d ms\n"
								"                        BwLimit %s %5d kb/s TOSbits %s 0x%08x\n"
								"                        Authenticate %s\n",
					m_bAggregate?"T":"F", m_nAggregateMs,
					m_bReorderPackets?"T":"F", m_nEscrowTimeMs,
					m_bBWLimit?"T":"F", m_nBWLimitKBPerSec,
					m_bUseTOSbits?"T":"F", m_nTOSbits,
					m_bAuthenticate?"T":"F");
				if (bTOSChanged)
				{
					VXSOCKET	sock;
					CIpLink*	pLink = (CIpLink*)m_pLink;
					int			nErr;
					int			TOSbits = m_bUseTOSbits ? m_nTOSbits : 0;
					sock = pLink->masterSocket();
					nErr = vxsSetTosBits( sock, TOSbits );
					if ( 0 != nErr )
					{	vxlReportUrgent("Router - error %d setting TOS bits to 0x%08x\n",
										nErr, TOSbits );
					}
					else
					{	vxlReportEvent("Router - setting TOS bits to 0x%08x\n",
												TOSbits );
					}
				}
			}
			else
			{	countInvalidPackets();
			}
			bOk = false; // Don't bump the data incarnation
			break;
		case PKTTYPE_ECHCONTROL :
			// if this message is not from the Config Manager, then ignore it
			// since they could mess us up, like rebooting.
			if ( !bFromCnfgSrvr ) break;
			p = pData;
			bOk = ctl.parse( p, false );
			if ( bOk )
			{
				int ack = ACK_CANT_COMPLY;

				if ( ctl.bReboot )
				{	// reboot ourselves
					if (LtIpRebootAllowed())
						ack = ACK_OK;
				}
				else if ( ctl.bStopWeb )
				{
					if (LtIpStopWebServer())
						ack = ACK_OK;
				}
				else if ( ctl.bStartWeb )
				{
					if (LtIpStartWebServer())
						ack = ACK_OK;
				}
				doSendAck( ack, 0, ipSrcAddr, ipSrcPort );
				if (ctl.bReboot && LtIpRebootAllowed())
				{
					taskDelay(sysClkRateGet());
					LtIpReboot();
				}
			}
			bOk = false;
			break;
		case PKTTYPE_ECHMODE:
			// if this message is not from the Config Manager, then ignore it
			if ( !bFromCnfgSrvr ) break;

			p = pData;
			bOk = mode.parse( p, false );
			if ( bOk )
			{
				if ((mode.echVersionInUse != m_runningEchProtocolVer) &&
					((mode.echVersionInUse == LTIP_ECH_VER1) ||
					 (mode.echVersionInUse == LTIP_ECH_VER2)))
				{
					setEchProtocolVersion((LtIpEchProtocolVersion)mode.echVersionInUse);
					vxlReportEvent("LtIpMaster::doRfcIn - setting protocol mode to %d\n",
												mode.echVersionInUse);
				}
				if (mode.natIpAddr != m_natIpAddr)
				{
					// This is equivalent to a local address change.
					// We must update all the packets
					setNatAddress(mode.natIpAddr);
					vxlReportEvent("LtIpMaster::doRfcIn - setting NAT IP address to %d\n",
												ias.getString(mode.natIpAddr));
				}
				if (mode.strictEia852 != m_bStrictEia852)
				{
					setStrictEia852(mode.strictEia852);
					vxlReportEvent("LtIpMaster::doRfcIn - setting strict EIA-852 mode to %d\n",
												mode.strictEia852);
				}
				// ignore local config setting

				doSendAck( ACK_OK, 0, ipSrcAddr, ipSrcPort );

			}
			bOk = false;	// why???
			break;
		case PKTTYPE_ECHVERSION:
			// if this message is not from the Config Manager, then ignore it
			if ( !bFromCnfgSrvr ) break;
			p = pData;
			bOk = echVer.parse( p, false );
			if ( bOk )
			{
				// Probably a redundant check
				if (echVer.echVersionSupported >= LTIP_ECH_VER2)
				{
					setCnfgServerType(CS_TYPE_ECH_V2);
				}
			}
			bOk = false;	// why???
			break;
		default:
			// invalid packet, just ignore it after counting
			countInvalidPackets();
			bOk = false; // Don't bump the data incarnation
		}


	} while ( false );

	if ( pPkt )
	{
		pPkt->release();
	}
}


//
// setNewServers
//
// We got a new DeviceConfiguration message from someone.
// They are allowed to change the config server address we have
// as well as the NTP server address we have.
//
// This routine is also called on startup to setup things from
// the persistance.
//
boolean	LtIpMaster::setNewServers( LtIpDevRegister& dvr )
{
	CIpLink*	pLink = (CIpLink*)m_pLink;
	boolean		bOk = false;
	boolean		bChanges = false;
	LtSts		sts;
	boolean		bCsChanged = dvr.ipAddressCS != m_ipAddrCfgServer ||
							dvr.ipPortCS != m_ipPortCfgServer;
	boolean		bLocalPortChanged = dvr.ipUnicastPort != m_ipPortLocal;


	if ( bCsChanged )
	{
		// we had major changes in our configuration
		// esp, a new config server or config server port
		// so forget we have a channel membership packet
		// so we make sure we request one again
		lock();
		FREEANDCLEAR(m_pktChanMembers);
		m_nPktCMSize = 0;
		m_dtPktCM = 0;
		unlock();
		setCnfgServerType(CS_TYPE_UNKNOWN);
	}
	// if the address has changed since last time
	if ( bCsChanged || bLocalPortChanged )
	{
		do
		{
			// shut down the link in the baseclass
			// must empty m_qRFCin too
			// discard any waiting RFC messages from the queue
			LtPktInfo*	pMsg;
			LtQue*		pItem;
			while ( m_qRFCin.removeTail( &pItem ) )
			{
				pMsg = (LtPktInfo*)pItem;
				pMsg->release();
			}
			// stop the segments and the base class together
			stopBase();
			m_ipAddrCfgServer	= dvr.ipAddressCS;
			m_ipPortCfgServer	= dvr.ipPortCS;
			if (m_ipAddrCfgServer != 0)
			{
				// Trigger a translation of these on save of persist data
				m_updateCsHostName = true;
			}
			m_ipPortLocal		= dvr.ipUnicastPort;
			pLink->setDstIp( m_ipAddrCfgServer, m_ipPortCfgServer );
			pLink->setSrcIp( m_ipAddrLocal, m_ipPortLocal );
			sts = pLink->open("Ignored");
			if ( sts != LTSTS_OK ) break;
			// Call the base class to start the link
			bOk = LtIpBase::start();
			// start segments
			m_segOut.setActive( true );
			m_segIn.setActive( true );

			if ( !bOk ) break;
		} while (false);
		if ( !bOk )
		{
			LtIpAddressStr	ias;
			vxlReportUrgent("Router - unable to restart link to config server\n"
				            "                at %s %d\n",
							ias.getString(m_ipAddrCfgServer), m_ipPortCfgServer );
		}
		bChanges = true;
	}
	// if the address has changed since last time
	if ( dvr.ipAddressTS != m_ipAddrNtpServer ||
		 dvr.ipPortTS != m_ipPortNtpServer ||
		dvr.ipAddressTS2 != m_ipAddrNtpServer2 ||
		 dvr.ipPortTS2 != m_ipPortNtpServer2
		 )
	{
		// we need to stop the SNTP client
		m_ipAddrNtpServer = dvr.ipAddressTS;
		m_ipPortNtpServer = dvr.ipPortTS;
		m_ipAddrNtpServer2 = dvr.ipAddressTS2;
		m_ipPortNtpServer2 = dvr.ipPortTS2;
// EPANG TODO - don't support SNTP yet in Linux iLON
#if defined(__VXWORKS__) || defined(WIN32)
		// now we need to restart the SNTP client
		iLonSetTimeServersFromIP852(m_ipAddrNtpServer, m_ipPortNtpServer,
								  m_ipAddrNtpServer2, m_ipPortNtpServer2);
#endif

		bChanges = true;
	}
	// update the channel timeout
	if ( m_nChannelTimeout != dvr.channelTimeout )
	{	m_nChannelTimeout = dvr.channelTimeout;
		bChanges = true;
	}
	return bChanges;
}



//
// doRFCRequest
//
// respond to a request by a node for our information.
// pPkt contains the requestors address / port
// Reply directly and do not release the pPkt.
//
void	LtIpMaster::doRFCRequest( int pktType, LtPktInfo* pPkt )
{
	byte*			p;
	byte*			pData;
	int				nBytes;
	int				i;
	boolean			bOk;
	boolean			bFromCnfgSrvr;

	LtIpRequest		req;
	LtIpEchConfig	ecfg;
	ULONG			ipSrcAddr = 0;	// initialize to fix warning
	word			ipSrcPort = 0;	// initialize to fix warning
	ULONG			ipSrcAddrOrg;
	word			ipSrcPortOrg;
	boolean			bSendNak = false;	// bad request
	int				nNakType = 0;		// type of NAK
										// largest packet we have...
	byte			buf[LtIpEchConfig::MAX_PKT_SIZE+4]; // largest fixed size request...
	LtIpAddressStr	ias;
	LtIpAddressStr	ias1;
	USHORT			reqPort;

	do
	{
		p = pData = pPkt->getDataPtr();
		nBytes = pPkt->getDataSize();
		bOk = req.parse( p, false );
		if ( !bOk )
		{
			nNakType = ACK_BAD_MESSAGE;
			bSendNak = true;
			countInvalidPackets();
			break;
		}
		// is the packet from the Configuration manager?
		// we only accept certain types of packets from the configuration manager.
		// (currently none)
		ipSrcAddrOrg = ipSrcAddr = pPkt->getIpSrcAddr();
		ipSrcPortOrg = ipSrcPort = pPkt->getIpSrcPort();
		if ( ipSrcAddr == m_ipAddrCfgServer &&
			 ipSrcPort == m_ipPortCfgServer
			 )
		{	bFromCnfgSrvr = true;
			ipSrcAddr = 0;	// send via the default mechanism
			ipSrcPort = 0;
		}
		else
		{	bFromCnfgSrvr = false;
		}

		switch ( pktType )
		{
		case PKTTYPE_REQCHNMEMBERS:
			// Send the one we have if we have one.
			if ( m_pktChanMembers == NULL ||
				!sendSavedChanMemberPkt(m_pktChanMembers, ipSrcAddr, ipSrcPort, &req))
			{
				nNakType = ACK_DEVICE_REFUSED;
				bSendNak = true;
			}
			break;

		case PKTTYPE_REQCHNROUTING:
			reqPort = 0;
			if (req.vendorCode == VENDOR_ECHELON)
			{
				// The Echelon extended form of this packet contains
				// the port number
				LtIpReqEchChnRouting echReq;
				// Grab the single port
				if (echReq.parse(p, false))	// don't mess with the original pkt data
					reqPort = echReq.port;
			}
			// allow request from anyone for debugging - sigh...
			vxlReportEvent("doRFCReq - request Channel routing for %s:%d from %s:%d\n",
							ias.getString( req.ipUcAddress ), reqPort,
							ias1.getString( ipSrcAddrOrg ), ipSrcPortOrg);
			// check to see if segment was handled by segmentor
			if ( m_segOut.segmentRequest( req, ipSrcAddrOrg, ipSrcPortOrg ) )
			{	// handled by segmentor
				break;
			}

			if ( req.ipUcAddress == 0 )
			{	vxlReportEvent("Zero requested IP address\n");
				break;
			}

			// Note: we may not be able to check the port here because it does
			// not exist in a standard channel routing request packet.
			// Assume that if it is the same IP address, it must be for
			// us, since we only listen on our own port
			// Our address could be local or from a NAT box, and the request
			// could be for either one, so check both
			if (((req.ipUcAddress == getExternalIpAddr()) ||
					(req.ipUcAddress == m_ipAddrLocal)) &&
				((reqPort == 0) || (reqPort == m_ipPortLocal)))
			{
				doSendChanRouting( ipSrcAddr, ipSrcPort, &req, true, true );
				// do not repeat the sending here
				break;
			}
			// not our own, so search for the member packet
			// and send it if we have it.
			// but to get the size, we've got to parse it.
			// This request should almost never happen anyway.
			bSendNak = true;
			nNakType = ACK_DEVICE_REFUSED;
			for ( i=0; i<m_nMembers; i++ )
			{
				// Match the port, too, if we have it
				if (
					( m_apdMembers[i].ipAddress == req.ipUcAddress ) &&
					((reqPort == 0) || (m_apdMembers[i].ipPort == reqPort)) &&
					m_pktChanRouting[i]
					)
				{
					if (sendSavedChanRoutingPkt(m_pktChanRouting[i],
												ipSrcAddr, ipSrcPort, &req))
					{
						vxlReportEvent("doRFCReq - send chan routing for %s to %s:%d\n",
										ias.getString(req.ipUcAddress),
										ias1.getString( ipSrcAddr ), ipSrcPort );
						bSendNak = false;
					}
					break;
				}
			}
			if (i >= m_nMembers)
				// Debug: just a place to put a breakpoint if we don't find the dev
				i = m_nMembers;

			break;

		case PKTTYPE_REQDEVCONFIG:
			// send our own device config packet
			// never larger than a single frame, so doesn't need a request id actually
			doSendDevRegister( true, ipSrcAddr, ipSrcPort );
			break;

		case PKTTYPE_REQSENDLIST:
			// we don't support this packet.
			// Send a NAK
			nNakType = ACK_NOT_SUPPORTED;
			bSendNak = true;
			break;
			// request echelon private configuration
			// must be echelon vendor code requesting
		case PKTTYPE_ECHCONFIGREQ :
			if ( req.vendorCode != VENDOR_ECHELON )
			{	break;
			}
			ecfg.dateTime			= getDatetime();
			ecfg.aggTimerMs			= m_nAggregateMs;
			ecfg.bwLimitKbPerSec	= m_nBWLimitKBPerSec;
			ecfg.escrowTimerMs		= m_nEscrowTimeMs;
			ecfg.TosBits			= m_nTOSbits;
			ecfg.bAggregate			= m_bAggregate;
			ecfg.bBwLimit			= m_bBWLimit;
			ecfg.bNoReorder			= !m_bReorderPackets;
			ecfg.bUseTosBits		= m_bUseTOSbits;
			ecfg.bAuthenticate		= m_bAuthenticate;
			// get the timezone string
			// if we got one, then set for we are returning it
			LtGetTimeZone( ecfg.szTimeZone );
			if ( strlen( ecfg.szTimeZone ) )
			{	ecfg.bUseTZ = true;
			}
			setPktExtHdrData(&ecfg);
			ecfg.build( buf );
			sendNewPacketTo( buf, ecfg.packetSize, ipSrcAddr, ipSrcPort, &req );
			break;
		case PKTTYPE_REQSTATISTICS :
			nBytes = doRfcBuildStats( buf, req.reason );
			sendNewPacketTo( buf, nBytes, ipSrcAddr, ipSrcPort, &req );
			break;
		case PKTTYPE_ECHVERSREQ :
			{	// report our build and version number
				LtIpEchVersion	vrs;

				vrs.dateTime = getDatetime(false);
				setPktExtHdrData(&vrs);
				vrs.build( buf );
				sendNewPacketTo( buf, vrs.packetSize, ipSrcAddr, ipSrcPort, &req );
			}
			break;
		case PKTTYPE_ECHMODEREQ:
			{	// report our operational mode (Echelon protocol version)
				LtIpEchMode mode;

				mode.natIpAddr = m_natIpAddr;
				mode.echVersionInUse = m_runningEchProtocolVer;
				mode.strictEia852 = m_bStrictEia852;
				mode.usingLocalCnfg = m_mStaticConfig;
				setPktExtHdrData(&mode);
				mode.build( buf );
				sendNewPacketTo( buf, mode.packetSize, ipSrcAddr, ipSrcPort, &req );
				vxlReportEvent("doRFCReq - sending Ech Mode: %d\n", m_runningEchProtocolVer);
			}
			break;
		case PKTTYPE_ECHDEVIDREQ:
			{	// report our device ID info
				LtIpEchDeviceId devId;
#ifdef ILON_100_ROUTER_DEMO
				LtIpDevRegister devReg;
#endif

				devId.localIpAddr = m_ipAddrLocal;
				devId.natIpAddr = m_natIpAddr;
				devId.ipPort = m_ipPortLocal;
#ifdef ILON_100_ROUTER_DEMO
				if (m_pktDevRegister == NULL)
				{
					buildDevRegister(false);
				}
				if (m_pktDevRegister != NULL)
				{
					devReg.parse(m_pktDevRegister, false);
					if (devReg.neuronIdBytes >= 6)
					{
						memcpy(devId.ltUniqueId, devReg.pNeuronIds, sizeof(devId.ltUniqueId));
					}
				}
				devId.prevLocalIpAddr = m_prevIpAddrLocal;
#endif // ILON_100_ROUTER_DEMO
				setPktExtHdrData(&devId);
				devId.build( buf );
				sendNewPacketTo( buf, devId.packetSize, ipSrcAddr, ipSrcPort, &req );
				vxlReportEvent("doRFCReq - sending Ech Device ID pkt\n");
			}
			break;
		}
	} while (false);

	// on some error, send the NAK packet with the code
	if ( bSendNak )
	{
		LtIpResponse	nak;
		byte			nakPkt[LtIpResponse::MAX_PKT_SIZE + 4];

		nak.type = nNakType;
		// assure that we NAK the request
		nak.requestId = req.requestId;
		nak.segmentId = 0;

		setPktExtHdrData(&nak);
		nak.build( nakPkt );
		sendNewPacketTo( nakPkt, nak.packetSize, ipSrcAddr, ipSrcPort );
	}

}

//
// doRfcBuildStats
//
// Build a statistics message to be sent to a requestor.
//
int		LtIpMaster::doRfcBuildStats( byte* buf, int reason )
{
	LtIpPktStats	stats;
	LtLinkStats		linkStats;
	LtLreIpStats	clientStats;
	LtIpStats		masterStats;
	boolean			bZeroStats = 0 != (reason & REQUEST_MOVE);
	ULONG			dtNow = LtIpPktHeader::getDateTime();
	ULONG			nSecondsSinceReset;

	getCounts( masterStats );
	getClientCounts( clientStats );

	nSecondsSinceReset = dtNow - m_dtStatisticsReset;
	stats.stats[LtStats_secondsSinceReset] = nSecondsSinceReset;
	stats.stats[LtStats_dateTimeReset] = m_dtStatisticsReset;
	stats.stats[LtStats_members] = m_nMembers;


	if ( m_pLonLink )
	{
		m_pLonLink->getStatistics( linkStats );
		if ( bZeroStats )
		{	m_pLonLink->clearStatistics();
		}
		stats.stats[LtStats_LtPktsReceived] = linkStats.m_nReceivedPackets;
		stats.stats[LtStats_LtPktsSent] = linkStats.m_nTransmittedPackets;
	}

	stats.stats[LtStats_IpPktsSent] = clientStats.m_nPktsProcessed;
	stats.stats[LtStats_IpBytesSent] = clientStats.m_nBytesSent;
	stats.stats[LtStats_IpPktsReceived] = clientStats.m_nPktsRouted;
	stats.stats[LtStats_IpBytesReceived] = clientStats.m_nBytesReceived;
	stats.stats[LtStats_IpPktsDataSent] = clientStats.m_nPktsSent;
	stats.stats[LtStats_IpPktsDataReceived] = clientStats.m_nPktsReceived;
	stats.stats[LtStats_LtPktsStaleDropped] = clientStats.m_nPktsStale;

	// IKP06042003: assume we are going to send this packet, so add by one
	stats.stats[LtStats_RfcPktsSent] = masterStats.nPacketsSent + 1;
	stats.stats[LtStats_RfcPktsReceived] = masterStats.nPacketsReceived;

	// for Version 2 Stat packet, fill in the average UDP pkts sent & recvd
	if (!backwardCompatibleChan())
	{
		if (nSecondsSinceReset > 0)
		{
			stats.stats[LtV2Stats_AvgUdpPktsPerSecSent] =
				clientStats.m_nPktsProcessed/nSecondsSinceReset;
			stats.stats[LtV2Stats_AvgUdpPktsPerSecRcvd] =
				clientStats.m_nPktsRouted/nSecondsSinceReset;
		}
	}

	if ( bZeroStats )
	{
		setLastStatisticsResetTime( dtNow );

		// clear master statistics
		clearCounts( bIncludeLinkStatistics );

		// clear cleints statistics
		clearClientCounts( bIncludeLinkStatistics );
	}
	setPktExtHdrData(&stats);
	stats.build( buf );

	return stats.packetSize;
}

//
// doRequestInfo
//
// Request different packets from the server.
// We may request a channel membership packet or
// the channel routing packets.
// The m_nReqtMask has bits in it for what we are requesting.
// As packets come in, then the bits of this mask are cleared.
// If we finish this routine and there are more bits in the request mask,
// then we trigger the routine to run again after 1 second.
// After 10 seconds we lengthen the timer to 30 seconds incase
// the server is not around.
//
void LtIpMaster::doRequestInfo()
{

	LtIpRequest		req;
	int				i;
	ULONG			ipAddr = 0;
	USHORT			ipPort = 0;
	LtIpAddressStr	ias;
	LtIpAddressStr	ias2;
	// REMINDER - 600: this does not include any space for the
	// optional address/port list -- not currently implemented.
	byte			reqPkt[LtIpReqEchChnRouting::MAX_PKT_SIZE + 4];
	byte*			p;
	int				nReqsSent = 0;

	// if we are shutting down, then just bug out
	// don't cause any activity that could keep us around here longer.
	if ( m_bStopping )
	{	return;
	}
	do
	{
	ias.setIaddr( m_ipAddrLocal );
	req.dateTime = getDatetime(false);
	req.sinceDateTime = 0;
	req.requestId = 0;	// for all except channel routing packets

	// don't make requests if we don't have a config server 
 	if (( m_ipAddrCfgServer == 0 || m_ipPortCfgServer == 0 ))   
	{
		m_nReqtMask = 0;
		m_ConnectLedObSubject.Notify();
		break;
	}
	if ( m_nReqtMask & REQT_DEVRESPONSE )
	{
		vxlReportEvent("LtIpMaster::doRequestInfo - for %s - call doSendDevRegister()\n",
						ias.getString() );
#ifdef ILON_100_ROUTER_DEMO
		if (m_prevIpAddrLocal != 0)
		{
			// First send a device ID pkt with the IP address change
			LtIpEchDeviceId devId;
			LtIpDevRegister devReg;

			devId.localIpAddr = m_ipAddrLocal;
			devId.natIpAddr = m_natIpAddr;
			devId.ipPort = m_ipPortLocal;
			if (m_pktDevRegister == NULL)
			{
				buildDevRegister(false);
			}
			if (m_pktDevRegister != NULL)
			{
				devReg.parse(m_pktDevRegister, false);
				if (devReg.neuronIdBytes >= 6)
				{
					memcpy(devId.ltUniqueId, devReg.pNeuronIds, sizeof(devId.ltUniqueId));
				}
			}
			devId.prevLocalIpAddr = m_prevIpAddrLocal;
			setPktExtHdrData(&devId);
			devId.build( reqPkt );
			sendNewPacket( reqPkt, devId.packetSize );
			vxlReportEvent("Sending Ech Device ID pkt for IP address change\n");
		}
#endif // ILON_100_ROUTER_DEMO
		doSendDevRegister( false );
	}
	else
	{
		if (m_nReqtMask & REQT_CSTYPE)
		{
			req.packetType = PKTTYPE_ECHVERSREQ;
			setPktExtHdrData(&req);
			p = req.build( reqPkt );
			assert( (p-reqPkt) < (int)sizeof( reqPkt ) );
			sendNewPacket( reqPkt, req.packetSize);
			// Try a few time to get the CS type before
			// letting these other packets go through
			if ((m_cnfgServerType == CS_TYPE_UNKNOWN) &&
				(m_cnfgSrvrTypeChkCount < 3))
				break;
			// If we have tried a number of times and received no answer,
			// but have got other messages from the config server since
			// we started asking, assume it is not an Echelon V2 CS
			if ((m_cnfgSrvrTypeChkCount >= 5) &&
				(m_CSmsgRcvdCheckpoint != m_CSmsgRcvd))
			{
				setCnfgServerType(CS_TYPE_OTHER);
			}
		}
		if ( m_nReqtMask & REQT_CHANMEMBERS )
		{
			// IKP05202003: Incoming channel membership packet may get segmented and handled by
			// the segmentor code.  if the segmentor is currently in the middle of handling segments
			// for this request type, then don't request anymore until the request is done or timedout.
			if (! m_segIn.hasPendingRequest(PKTTYPE_REQCHNMEMBERS, m_ipAddrCfgServer, m_ipPortCfgServer))
			{
				vxlReportEvent("LtIpMaster::doRequestInfo - for %s Request Chan Members\n",
							ias.getString() );

				req.packetType = PKTTYPE_REQCHNMEMBERS;
				setPktExtHdrData(&req);
				p = req.build( reqPkt );
				assert( (p-reqPkt) < (int)sizeof( reqPkt ) );
				sendNewPacket( reqPkt, req.packetSize);
			}
		}
		else if ( m_nReqtMask & REQT_CHANROUTING )
		{
			LtIpReqEchChnRouting reqEx;	// Echelon extended request packet
			LtIpRequest* pChanRtReq;

			req.packetType = PKTTYPE_REQCHNROUTING;
			// look for first channel routing packet we don't have
			// Actually we need a separate array for this since we could have
			// a packet, but need to request a new one if the date changed.
			boolean firstTry = TRUE;
			boolean useExtPkt = FALSE;
			for ( i=0; i< m_nMembers; i++ )
			{
				if ( m_baReqtCR[i] )
				{
					ipAddr = m_apdMembers[i].ipAddress;
					ipPort = m_apdMembers[i].ipPort;
					// never ask for our own channel routing packet
					// Our address could be local or from a NAT box
					if ((ipAddr != getExternalIpAddr()) || (ipPort != m_ipPortLocal))
					{	// request all the channel routing packets we need all at once
						// since we don't know which ones the server has now
						ias2.setIaddr( ipAddr );
						vxlReportEvent("LtIpMaster::doRequestInfo - for %s Request Chan Routing for %s(%d)\n",
								ias.getString(), ias2.getString(), ipPort );
						req.ipUcAddress = ipAddr;
						// set the request id so that we can tell it was
						// for a channel routing packet.
						req.requestId = 1000 + i;
						req.reason = REQUEST_ALL;

						if (firstTry)
						{
							// If we don't know the config server type,
							// make sure we try to find out
							if (!(m_nReqtMask & REQT_CSTYPE) &&
								!backwardCompatibleChan() &&
								(m_hasSharedIpAddrs ||
									(m_natIpAddr != 0)) &&	// ports matter for these
								((m_cnfgServerType == CS_TYPE_UNKNOWN) ||
									(m_cnfgServerType == CS_TYPE_OTHER)))
							{
								m_nReqtMask |= REQT_CSTYPE;
								if (m_CSmsgRcvdCheckpoint == 0)
									m_CSmsgRcvdCheckpoint = m_CSmsgRcvd;
							}

							// Decide which packet to use
							if (!backwardCompatibleChan() &&
								(m_hasSharedIpAddrs ||
									(m_natIpAddr != 0)) &&	// ports matter for these
								((m_cnfgServerType == CS_TYPE_ECH_V2) ||
									((m_cnfgServerType == CS_TYPE_UNKNOWN) &&
									(m_cnfgSrvrTypeChkCount < 3))))
							{
								useExtPkt = TRUE;
							}
						}

						if (useExtPkt)
						{
							// Use (or try) the Echelon extended packet
							reqEx.ipUcAddress = req.ipUcAddress;
							reqEx.port = ipPort;
							reqEx.requestId = req.requestId;
							reqEx.reason = req.reason;
							setPktExtHdrData(&reqEx);
							p = reqEx.build( reqPkt );
							pChanRtReq = &reqEx;
						}
						else
						{
							// Use standard packet
							setPktExtHdrData(&req);
							p = req.build( reqPkt );
							assert( (p-reqPkt) < (int)sizeof( reqPkt ) );
							pChanRtReq = &req;
						}
						sendNewPacket( reqPkt, pChanRtReq->size() );
						if (++nReqsSent > 50) // do 50 at a time
							break;
					}
					ipAddr = 0;
				}
			}
			// if we have them all, then don't request any
			if ( nReqsSent == 0 )
			{	m_nReqtMask = m_nReqtMask & ~REQT_CHANROUTING;
			}
		}

		// should we send our own channel routing?
		if ( m_nReqtMask & REQT_SENDCHANROUTING )
		{
			// send our channel routing packet to the config server
			// always send it regardless of whether a new one is built
			// there's no request, so just make up a request id
			doSendChanRouting( 0,0, NULL, TRUE, false );
			// no need to reset the bit here since it's set.
		}
	}
	} while ( false );

	// if there are bits in the mask, then we need to call ourselves again
	//
	if ( m_nReqtMask )
	{
		m_nReqtCount++;
		// just restart the timer for us
		startRequestInfo( 0 );
	}
}

void LtIpMaster::setCnfgServerType(int type)
{
	m_cnfgServerType = type;
	if (type != CS_TYPE_UNKNOWN)
	{
		cancelRequest(REQT_CSTYPE);
		m_cnfgSrvrTypeChkCount = 0;
		m_CSmsgRcvdCheckpoint = 0;
	}
}

//
// startRequestInfo
//
// start the timer for a request
//
void LtIpMaster::startRequestInfo( int nReqBits, boolean bQuick )
{
	int				nDelayMs = 500;
	STATUS			sts;

	if ( bQuick )
	{	m_nReqtCount = 0;
		nDelayMs = 500;
		// make sure we do it now, if we are already in the work task
		m_nWorkMask |= WORK_RequestInfo;
	}
	else
	{
		if ( m_nReqtCount > 0 && m_nReqtCount < 10 )
		{	nDelayMs = 1000;
		}
		if ( m_nReqtCount > 10 )
		{
			nDelayMs = 30000;
		}
	}
	m_nReqtMask |= nReqBits;
	//if (m_nReqtMask)
		m_ConnectLedObSubject.Notify();	// update Connect LED

	sts = wdCancel( m_tRequest );
	sts = wdStart( m_tRequest, msToTicksX( nDelayMs ), LtIpRequestTimeout, (int)this );
	assert( sts == OK );
}

//
// doSendChanRouting
//
BOOL LtIpMaster::doSendChanRouting( int ipAddr, word ipPort, LtIpRequest* pReqPkt,
								   boolean bAlways, boolean bIgnoreTime )
{
	boolean			bOk;
	boolean			bNewPacket = false;
	LtIpAddressStr	ias;
	int				ipAddrToUse = ipAddr;
	word			nThePort = ipPort;
	const char*		pResult;
	ULONG			nTicksNow = tickGet();
	ULONG			nTicksSince = nTicksNow - m_tickLastSendChanRouting;
	LtIpRequest		reqPkt;

	nTicksSince = ticksToMs( nTicksSince );

	do
	{
		if ( !bIgnoreTime && ( nTicksSince < MIN_RESEND_MS ) )
		{
			vxlReportEvent("doSendChanRouting - skip sending. Not enough time\n");
			break;
		}


		if ( ipAddrToUse )
		{
			ias.setIaddr( ipAddrToUse );
		}
		else
		{
            // Use MULTICAST address if runnning in MULTICAST mode
            if (selfInstalledMcastMode())
        	{
                ipAddrToUse = m_selfInstalledMcastAddr;
                nThePort = m_ipPortLocal;
            }
            else
            {
			    ipAddrToUse = m_ipAddrCfgServer;
			    nThePort = m_ipPortCfgServer;
            }
			ias.setIaddr( ipAddrToUse );
		}
		if ( ipAddrToUse == 0 )
		{
			vxlReportEvent( "doSendChanRouting - nowhere to send channel routing packet to %s %d\n",
							ias.getString(), nThePort );
			break;
		}

		// if we are always supposed to send it, and we already have it,
		// then don't build a new packet, since this can cause a loop under some
		// circumstances with the config server
		//
		// check for a new packet and remember whether we created one
		bNewPacket = sweepLocalClients();

		if ( bNewPacket || bAlways )
		{
			if ( m_pktOurChanRouting && m_nPktOCRSize )
			{
				bOk = sendSavedChanRoutingPkt(m_pktOurChanRouting, ipAddr, ipPort, pReqPkt);

				pResult = bOk?"OK":"FAILED";
				vxlReportEvent( "doSendChanRouting - send %s chanRoutng packet to %s %d %s reqId %d\n",
								bAlways?"":"new",ias.getString(), nThePort, pResult,
								pReqPkt?pReqPkt->requestId:-1 );
				// if we created a new packet, then clear the count to try hard to send it
				//startRequestInfo( REQT_SENDCHANROUTING );
			}
			else
			{
				vxlReportEvent( "doSendChanRouting - no channel routing packet for %s %d\n",
								ias.getString(), nThePort );
			}
		}
	} while ( false );

	return bNewPacket;
}

//
// doSendAck
//
// send an acknowledgement
//
void	LtIpMaster::doSendAck( int code, word reqId, int ipAddr, word ipPort )
{
	LtIpResponse	rsp;
	byte			buf[LtIpResponse::MAX_PKT_SIZE+4];

	rsp.dateTime = getDatetime(false);
	rsp.type = code;
	rsp.requestId = reqId;
	rsp.segmentId = 0;
	setPktExtHdrData(&rsp);
	rsp.build( buf );

	// send the packet to the requestor whoever it is
	sendNewPacketTo( buf, rsp.packetSize, ipAddr, ipPort );
}


//
// buildDevRegister
//
// return bytes in new packet
//
int LtIpMaster::buildDevRegister( boolean bDevResponse )
{
	ULONG				dateTime;
	LtIpDevRegister		dvrNew;
	LtIpDevRegister		dvrCurrent;
	LtIpDevRegister		dvrTest;
	LtIpChanMembers		chm;
	int					nBytes;
	byte*				p;
	char				chName[40];
	LtIpAddressStr		ias;
	byte*				ppkt;
	boolean				curPktOk;
	boolean				bParsedOk;
	// int					nNeedsBroadcasts = 0;	// IKP10212003: this fix did not make it to release 1.0

	// get router farside unique id and pass in this message
	LtUniqueId			uid;
	int					nUidx = 0;
	int					nUids = 0;
	// max uids we will put in the packet
	// more would make the packet longer than the 548 maximum
#define DEVREG_MAXUIDS 70
	byte				uidList[6*DEVREG_MAXUIDS];
	byte*				pUids = NULL;

	// Connect with the local clients
	// and make us an event Client to receive routing updates
	LtLreClient*	pClient;
    LtVectorPos     pos;

	// parse the channel members packet for the datetime
	if ( m_pktChanMembers )
	{
		chm.parse( m_pktChanMembers, false );
	}
	lockChannel();
	while ( m_pChannel->enumStackClients( pos, &pClient ) )
	{
		if ( pClient )
		{	// Get the one unique ID from this client
			// and add it to the end of the list
			pClient->getAddress( nUids, &uid );
			pUids = uid.getData();
			memcpy( &uidList[nUidx*6], pUids, 6 );
			nUidx++;
			// if we got too many, then just bug out
			if ( nUidx >= DEVREG_MAXUIDS ) break;
		}
	}
    unlockChannel();

	nUids = nUidx;
	pUids = uidList;

	dateTime = getDatetime(false);	// don't write this persistently yet

	// are we responding to another node?
	if ( bDevResponse )
	{	dvrNew.packetType = PKTTYPE_DEVCONFIGURE;
	}
	else
	{	dvrNew.packetType = PKTTYPE_DEVREGISTER;
	}
	//dvrNew.dateTime			= dateTime;	// set this below
	dvrNew.ipFlags				= LTIPPROTOCOL_UDP;
	// Check for protocol analyzer mode
	if (protocolAnalyzerMode())
	{
		// The only way to do this is to pretend the "router" type is a repeater
	   dvrNew.routerType = LTROUTER_REPEATER;
	}

	dvrNew.lonTalkFlags			= !backwardCompatibleChan() ? SUPPORTS_EIA_AUTH : 0;

	dvrNew.ipNodeType			= LTNODE_LTIPROUTER;
	dvrNew.mcAddressCount		= 0;
	dvrNew.neuronIdBytes		= 6 * nUids;
	dvrNew.ipUnicastPort		= m_ipPortLocal;
	dvrNew.ipUcAddress			= getExternalIpAddr();	// local or NAT
	dvrNew.chanMemDatetime		= chm.dateTime;
	dvrNew.pMcAddresses			= NULL; // none specified
	dvrNew.pNeuronIds			= pUids;
	dvrNew.ipAddressCS			= m_ipAddrCfgServer;
	dvrNew.ipPortCS				= m_ipPortCfgServer;
	dvrNew.ipAddressTS			= m_ipAddrNtpServer;
	dvrNew.ipPortTS				= m_ipPortNtpServer;
	dvrNew.ipAddressTS2			= m_ipAddrNtpServer2;
	dvrNew.ipPortTS2			= m_ipPortNtpServer2;
	dvrNew.channelTimeout		= m_nChannelTimeout;	
	if ( 0 == strlen(m_acName) )
	{
		sprintf( chName, "%s - %s(%d)", getSoftwareProductStr(),
						ias.getString(dvrNew.ipUcAddress), dvrNew.ipUnicastPort );
		dvrNew.nameLen			= min(strlen(chName),EIA852_MAXNAMELEN);
		dvrNew.nameLen++;	// The length must include the zero terminator
		dvrNew.pName			= (byte*)chName;
	}
	else
	{
		dvrNew.nameLen			= min(strlen(m_acName),EIA852_MAXNAMELEN);
		dvrNew.nameLen++;		// The length must include the zero terminator
		dvrNew.pName			= (byte*)m_acName;
	}

	lock();
	// See if we already have one. If so, create the new one with the same date/time
	// to see if they are the same. If so, don't change anything. Otherwise,
	// rebuild the new packet with a new date/time stamp, and replace the old one.
	if (m_pktDevRegister && dvrCurrent.parse(m_pktDevRegister))
	{
		dvrNew.dateTime = dvrCurrent.dateTime;
		curPktOk = true;
		// Guarentee that a new packet gets a newer date/time
		if (dateTime == dvrCurrent.dateTime)
			dateTime++;
	}
	else
	{
		dvrNew.dateTime = dateTime;
		curPktOk = false;
	}

	setPktExtHdrData(&dvrNew);
	nBytes = dvrNew.size();
	ppkt = (byte*)malloc(nBytes);
	p = dvrNew.build( ppkt );
	assert ( ppkt+nBytes == p );

	bParsedOk = dvrTest.parse( ppkt, false );
	if ( !bParsedOk )
	{	vxlReportEvent("buildDevRegister - error creating devRegister packet\n");
	}

	if (bParsedOk)
	{
		// If the new packet built and parsed OK, and it doesn't match
		// the old one, but we built with old date/time, rebuild with
		// the new date/time
		if (curPktOk &&	(dvrNew.dateTime != dateTime) &&
			(memcmp((m_pktDevRegister+dvrCurrent.hdrSize()),
					(ppkt+dvrTest.hdrSize()), (nBytes-dvrTest.hdrSize())) != 0))
		{
			// need to get the date/time again, this time saving it persistenly
			dvrNew.dateTime = getDatetime();
			dvrNew.build( ppkt );

		}
		// Free the old packet and save the new one, even if they are the same.
		// We have to free one of them anyway, and the logic is simpler this way.
		FREEANDCLEAR( m_pktDevRegister );
		m_pktDevRegister = ppkt;
	}
	unlock();
	return nBytes;
}
//
// doSendDevRegister
//
void LtIpMaster::doSendDevRegister( boolean bDevResponse, int ipAddr, word ipPort )
{
	int	nBytes;

	nBytes = buildDevRegister( bDevResponse );
	if ( nBytes )
	{
		// send the packet to the requestor whoever it is
		sendNewPacketTo( m_pktDevRegister, nBytes, ipAddr, ipPort );
	}
}

static void masterReportLinkStats( const char* tag, LtLinkStats& stats )
{
	vxlPrintf(
		"%s\n"
		//       1234567890     1234567890     1234567890
		"        Trans pkts %6d Trans errs %6d Collisions %6d\n"
		"        Recvd pkts %6d RecvPrPkts %6d MissedPkts %6d\n\n",
		tag,
		stats.m_nTransmittedPackets, stats.m_nTransmissionErrors, stats.m_nCollisions,
		stats.m_nReceivedPackets, stats.m_nReceivedPriorityPackets, stats.m_nMissedPackets );
}


//
// reportStatus
//
// report the status of the master and all links
//
void	LtIpMaster::reportStatus( boolean bAllMembers, boolean bLinksToo )
{
	int					i;
	LtLreIpClient*		pClient;
	CIpLink*			pIpLink;
	LtLinkStats			linkStats;
	LtIpAddressStr		ias;
	ULONG				nTickNow = tickGet();
#ifdef DEBUG_LOCKS
	// this needs a rework so that we don't lock-up if the master object
	// gets stuck. If we have the problem again, we'll fix it.
	xReportLockStack();
#endif // DEBUG_LOCKS
	lock();

	if (selfInstalledMcastMode())
	{
		// Use the real value -- m_ipAddrLocal is spoofed
		ias.setIaddr( m_actualIpAddrLocal );
	}
	else
	{
	    ias.setIaddr( m_ipAddrLocal );
	}
	vxlPrintf("Master Statistics for %s (%d) with %d members\n",
				ias.getString(), m_ipPortLocal, m_nMembers );
	vxlPrintf(
   			      //      1234567890     1234567890     1234567890     1234567890
				 "        Pkts sent  %6d Pkts recv  %6d Pkts dropd %6d Pkts missd %6d\n"
			     "        Bytes sent %6d Bytes recv %6d Pkt rcverr %6d Inval pkts %6d\n"
			     "        Auth fails %6d Alt Auth   %6d Too many   %6d \n",
				  m_stats.nPacketsSent, m_stats.nPacketsReceived, m_stats.nPacketsDropped, m_stats.nPacketsMissed,
				  m_stats.nBytesSent, m_stats.nBytesReceived, m_stats.nPacketReceiveErrors, m_stats.nInvalidPackets,
				  m_stats.nAuthFailures, m_stats.nAltAuthUsed, m_stats.nTooMany
				 );

	while ( m_stats.nLastTick != 0 )
	{
		ULONG	nSec = (nTickNow - m_stats.nLastTick) / sysClkRateGet();
		if ( nSec < 2 )
		{
			vxlPrintf(
				 "        Packet/data rates omitted - time interval too small\n");
			break;
		}
		m_stats.getLast( nSec );
		vxlPrintf(
   			      //      1234567890     1234567890     1234567890     1234567890
				 "        Pkt/s send %6d Pkt/s recv %6d Byt/s send %6d Byt/s recv %6d\n",
				  m_stats.nLastPktSent, m_stats.nLastPktRecv,
				  m_stats.nLastBytesSent, m_stats.nLastBytesRecv  );

		break;
	}
	m_stats.setLast( nTickNow );

	if ( bLinksToo )
	{
		pIpLink = (CIpLink*)getLink();
		if ( pIpLink )
		{	pIpLink->getStatistics( linkStats );
			masterReportLinkStats( "Master IP Link Statistics", linkStats );
		}
	}

	if (bAllMembers)
	{
		for ( i=0; i<m_nMembers; i++ )
		{
			pClient = m_apClients[i];
			if ( pClient )
			{	pClient->reportStatus();
				if ( bLinksToo )
				{
					pIpLink = (CIpLink*)pClient->getLink();
					if ( pIpLink )
					{	pIpLink->getStatistics( linkStats );
						masterReportLinkStats( "Client IP Link Statistics", linkStats );
					}
				}

			}
		}
	}
	vxlPrintf("End of Statistics\n" );
	unlock();
}

//
// getClientMustQueue
//
// Called by client to see if a message must be queued
//
boolean	LtIpMaster::getClientMustQueue()
{
	boolean	bMustQue =  m_bAggregate || m_bBWLimit;
	return bMustQue;
}


//
// okToSend
//
// Check ok to send a message
//
boolean LtIpMaster::okToSend( ULONG nSize )
{
	boolean		bOk = true;
	//lock();	// cannot lock the master since this is called
	// from the client and the master may be locked by a timer
	// task waiting on the client
	if ( m_bBWLimit )
	{
		if ( nSize > m_nBWBytesThisSlot )
		{
			bOk = false;
		}
		else
		{	// this math is only approximate since there is no lock
			m_nBWBytesThisSlot -= nSize;
		}
	}
	// unlock(); // cannot lock
	return bOk;
}

//
// LtIpBwAggTask
//
// bandwidth control and aggregation task
//
int LtIpBwAggTask( int a1, ... )
{
	LtIpMaster*	pMaster = (LtIpMaster*)a1;
	pMaster->doBwAgg();
	return 0;
}

//
// doBwAgg
//
// Task that sits and processes bandwidth and aggregate requests
//
void	LtIpMaster::doBwAgg()
{
	LtLreIpClient*	pClient;
	int				i;
	ULONG			nTimerMs = 500;
	int				nLC;
	int				nDiv;
	ULONG			nCurrentTick = tickGet();
	ULONG			nLastEscrowTick = nCurrentTick;
	ULONG			nLastAggregateTick = nCurrentTick;
	ULONG			nEscrowMs;
	ULONG			nAggregateMs = 0;	// init for warning
	boolean			bDumpEscrow;

#if 0 // report tests nTimerMs computation
	ULONG			nReportTick = nCurrentTick;
	ULONG			nTickNow;
	ULONG			nTickDiff;
#define REPORTMS 10000
#endif // report

	m_nBWBytesThisSlot = 0;	// IKP09182003: EPR 29441

	while ( !m_bTaskExit )
	{
		nTimerMs = 200;
		//nTimerMs = m_bAggregate? MIN( m_nAggregateMs, nTimerMs ) : nTimerMs;
		// use the residual aggregate timer if we have one rather than only
		// using that timer if we have aggregation enabled.
		// but only if either aggregation or BWLimiting is enabled.
		// This, in effect, allows the agg timer to become the BWLimit timer if
		// aggregation is off.
		nTimerMs = (m_bAggregate||m_bBWLimit ) && m_nAggregateMs?
									MIN( m_nAggregateMs, nTimerMs ) : nTimerMs;
		nTimerMs = m_bReorderPackets? MIN( m_nEscrowTimeMs, nTimerMs ) : nTimerMs;
		nTimerMs = m_bBWLimit? MIN( BWLIMIT_SLOTMS, nTimerMs ) : nTimerMs;

#if 0 // report tests nTimerMs computation
		nTickNow = tickGet();
		nTickDiff = nTickNow - nReportTick;
		nTickDiff = ticksToMs( nTickDiff );
		if ( nTickDiff > REPORTMS )
		{
			nReportTick = nTickNow;
			vxlReportEvent("LtIpMaster::doBwAgg - nTimerMs %d ticks %d\n",
							nTimerMs, msToTicksX( nTimerMs ) );
		}
#endif // report

		// set the bytes per slot based on the width of the slot
		if (nTimerMs == 0) nTimerMs = 200;	// Protect from failures
		nDiv = 1000 / nTimerMs;
		if ( nDiv == 0 )
		{	nDiv = 1;
		}
		m_nBWBytesPerSlot = (m_nBWLimitKBPerSec*1024) / nDiv;
		taskDelay( msToTicksX(nTimerMs ) );
		if ( m_bTaskExit ) break;
		lock();
		if ( m_bBWLimit || m_bAggregate || m_bReorderPackets )
		{
			if ( m_nMembers )
			{
				nLC = (++m_nBWLastClient) % m_nMembers;
			}
			else
			{	nLC = 0;
			}
			//
			// compute whether we should dump escrow packets on this time around
			//
			// boolean bDumpEscrow = it's been long enough since the last time we did
			// remember that we did dump escrow this time
			//
			nCurrentTick = tickGet();
			bDumpEscrow = false;
			if ( m_bReorderPackets )
			{
				nEscrowMs = nCurrentTick - nLastEscrowTick;
				nEscrowMs = ticksToMs( nEscrowMs );
				if ( nEscrowMs  >= m_nEscrowTimeMs )
				{	bDumpEscrow = true;
					nLastEscrowTick = nCurrentTick;
				}
			}
			else
			{	nLastEscrowTick = nCurrentTick;
			}

			// IKP09182003: EPR 29441
			// add # of unused bytes from previous slot to the current slot
			// this way, even if the bandwidth set to very low #, given enough
			// time slots, eventually it is able to send out some packets.
			// Make sure to peg this number at its max bandwidth
			m_nBWBytesThisSlot += m_nBWBytesPerSlot;
			m_nBWBytesThisSlot = MIN(m_nBWBytesThisSlot, (m_nBWLimitKBPerSec * 1024));

			// IKP09192003: EPR 29442
			// separate the aggregate from reordering processsing timer
			if (m_bAggregate)
			{
				nAggregateMs = nCurrentTick - nLastAggregateTick;
				nAggregateMs = ticksToMs( nAggregateMs );
			}

			for ( i=0; i< m_nMembers; i++ )
			{
				if ( m_apClients[nLC] )
				{
					pClient = m_apClients[nLC];
					if ( bDumpEscrow )
					{
						pClient->routeWaitingPackets( nCurrentTick );
					}

					// IKP09192003: EPR 29442
					// separate the aggregate from reordering processsing timer
					if (m_bAggregate)
					{
						// send the packet only if aggregation timer is up
						if ( nAggregateMs >= m_nAggregateMs)
						{
							pClient->sendMore();
							nLastAggregateTick = nCurrentTick;
						}
					}
					else
						pClient->sendMore();
				}
				nLC++;
				nLC = nLC % m_nMembers;
			}
		} // if
		unlock();
	} // while
	m_tidBwAggTask = 0;
}


// statistics
void	LtIpMaster::getClientCounts( LtLreIpStats& stats )
{
	int				i;
	LtLreIpClient*	pClient;
	LtLreIpStats	st;
	LtLreIpStats	st2;
	LtLreIpStats	cst;

	memset( &st, 0, sizeof(st) );
	memset( &st2, 0, sizeof(st2) );

	lock();
	for ( i=0; i< m_nMembers; i++ )
	{
		pClient = m_apClients[i];
		if ( pClient )
		{	pClient->getStats( cst );
			st2 = st2 + cst;
			st += cst;
		}
	}
	unlock();
	stats = st2;
}


// IKP06042003: added support for clearing link statistics
void	LtIpMaster::clearClientCounts( boolean bIncludeLink )
{
	int				i;
	LtLreIpClient*	pClient;
	CIpLink*		pIpLink;

	lock();
	for ( i=0; i< m_nMembers; i++ )
	{
		pClient = m_apClients[i];
		if ( pClient )
		{
			pClient->clearStats();

			// IKP06042003: clear the link statistics if necessary
			if (bIncludeLink)
			{
				pIpLink = (CIpLink*)pClient->getLink();
				if ( pIpLink )
					pIpLink->clearStatistics();
			}
		}
	}
	unlock();
}

// Send a saved copy of a channel member packet, but build a current header
boolean LtIpMaster::sendSavedChanMemberPkt(byte* pSavedPkt, int ipSrcAddr,
										   int ipSrcPort, LtIpRequest *pReq)
{
	LtIpChanMembers chmCur;
	LtIpChanMembers chmCopy;
	int				newSize;
	byte*			pNewPkt;
	boolean			bOk = false;

	// Must rebuild the packet, because the header should be our own
	bOk = chmCur.parse(pSavedPkt, false);
	if (bOk)
	{
		// Copy all the member data. We only need the stuff that applies to the header
		chmCopy = chmCur;
		// Adjust for any difference in header size/content
		chmCopy.timestamp = getTimestamp();
		setPktExtHdrData(&chmCopy);
		newSize = chmCur.packetSize - chmCur.hdrSize() + chmCopy.hdrSize();
		pNewPkt = (byte*)malloc(newSize);
		if (pNewPkt != NULL)
		{
			// Build only the header
			chmCopy.LtIpPktBase::build(pNewPkt);
			// Copy the remaining data
			memcpy((pNewPkt + chmCopy.hdrSize()), (pSavedPkt + chmCur.hdrSize()),
					(chmCur.packetSize - chmCur.hdrSize()));
			// Explicitly set the packet size
			chmCopy.buildPacketSize(pNewPkt + newSize);

			sendNewPacketTo( pNewPkt, newSize,
							ipSrcAddr, ipSrcPort, pReq );
			free(pNewPkt);
			bOk = true;
		}
	}
	return bOk;
}

// Send a saved copy of a channel routing packet, but build a current header
boolean LtIpMaster::sendSavedChanRoutingPkt(byte* pSavedPkt, int ipSrcAddr,
										   int ipSrcPort, LtIpRequest *pReq)
{
	LtIpChanRouting chrCur;
	LtIpChanRouting chrCopy;
	int				newSize;
	byte*			pNewPkt;
	boolean			bOk = false;
	LtIpRequest		reqPkt;

	// Must rebuild the packet, because the header should be our own
	bOk = chrCur.parse(pSavedPkt, false);
	if (bOk)
	{
		// Copy all the member data. We only need the stuff that applies to the header
		chrCopy = chrCur;
		// Adjust for any difference in header size/content
		chrCopy.timestamp = getTimestamp();
		setPktExtHdrData(&chrCopy);
		newSize = chrCur.packetSize - chrCur.hdrSize() + chrCopy.hdrSize();
		pNewPkt = (byte*)malloc(newSize);
		if (pNewPkt != NULL)
		{
			// Build only the header
			chrCopy.LtIpPktBase::build(pNewPkt);
			// Copy the remaining data
			memcpy((pNewPkt + chrCopy.hdrSize()), (pSavedPkt + chrCur.hdrSize()),
					(chrCur.packetSize - chrCur.hdrSize()));
			// Explicitly set the packet size
			chrCopy.buildPacketSize(pNewPkt + newSize);

			if ( pReq == NULL && newSize > UDP_MAX_PKT_LEN )
			{
				// If we have no request passed in, we need to make a request
				// for segmentation
				reqPkt.packetType = PKTTYPE_REQCHNROUTING;
				// we use reqIds in range of 1000 ++ to request channel routing packets
				reqPkt.requestId = 123;
				// use our own packet rather than nothing so segmentation has something
				// to work with.
				pReq = &reqPkt;

			}
			sendNewPacketTo( pNewPkt, newSize,
							ipSrcAddr, ipSrcPort, pReq );
			free(pNewPkt);
			bOk = true;
		}
	}
	return bOk;
}

// There is a possibility that a given CHR pkt may be our own, but
// we can't tell because we are behing a NAT box and we don't
// know it yet, thus the "external" IP address doesn't match.
// Another possibility is a local port change.
// We must not create an LRE client for our own CHR. For one thing,
// this would cause a duplicate UID client that discards Neuron ID
// messages addressed to us (e.g. by LonMaker). Check the
// UID in the packet to see if it matches any of our own.
boolean LtIpMaster::chrPktHasLocalUid(LtIpChanRouting& chr)
{
	LtPlatform platform;
	boolean hasLocalUid = false;
	byte *pUids;
	int nUids;
	int pktUidIdx;

	// Look at all the UIDs in the packet
	nUids = chr.neuronIdBytes / LtIpNeuronId::SIZE;
	pUids = chr.pNeuronIds;
	for (pktUidIdx = 0; (pktUidIdx < nUids) && !hasLocalUid; pktUidIdx++)
	{
		LtIpNeuronId pktUid;

		pUids = pktUid.parse(pUids);
#ifdef ILON_PLATFORM
		// Compare against all the UIDs owned by this platform.
		// NOTE: on the iLON the UID count currently is defined by
		// pncLtUniqueIdListC_getCount(), thought that should
		// be replaced, and doesn't even exist on Windows.
		int localUidIdx;
		for (localUidIdx = 0; localUidIdx < 16; localUidIdx++)
		{
			LtUniqueId platformUid;
			platform.setIndex(localUidIdx);
			if (platform.getUniqueId(&platformUid))
			{
				if (LtUniqueId(pktUid.id) == platformUid)
				{
					hasLocalUid = true;
					break;
				}
			}
		}
#else
		// EPR 30947
		// In LNS, there is not pre-defined set of unique IDs.  Instead look to
		// see if the unique ID is already registed in this routing engine.  If its
		// a remote client, the clients ID will be m_pChannel, and we should go ahead
		// and process this.  However, if the client has a different id, it must be a
		// local stack.

		// Note that even if the unique ID is not registered to a local stack now,
		// it may be at some later time.  However, that's OK, because when it is
		// registered it will replace the remote entry that is created due to this
		// channel routing packet.
		LtUniqueId uid(pktUid.id);
		LtLreClient* pUniqueIdClient = m_pServer->getLreUniqueIdClient(&uid);
		if (pUniqueIdClient != NULL && pUniqueIdClient->getId() != m_pChannel)
		{   // Client already exits in this LRE but its not mine.  Must be local.
			hasLocalUid = true;
			break;
		}
#endif
	}
	return(hasLocalUid);
}

// With NAT, we may have the source port of a message switched on us.
// The link client can call this to request the master to take some
// action to try to fix the problem or determine the actual sender.
// This should only be necessary if there are multiple clients, or a
// client and the CS, behind the same NAT box.
void LtIpMaster::requestUnknownPortDiagnostic(ULONG ipAddr, USHORT ipPort)
{
	m_unknownSrcPortDiag.lock();
	if (m_unknownSrcPortDiag.timestamp != time(NULL))
	{
		m_unknownSrcPortDiag.timestamp = time(NULL);
		m_unknownSrcPortDiag.throttleCount = 0;
	}
	if ((m_unknownSrcPortDiag.ipAddr == 0) &&
		(m_unknownSrcPortDiag.throttleCount++ < 5))	// allow 5 per second
	{
		m_unknownSrcPortDiag.ipAddr = ipAddr;
		m_unknownSrcPortDiag.ipPort = ipPort;
	}
	m_unknownSrcPortDiag.unlock();
}

// Execute the algorithm to try to fix or workaround the unknown port problem (above).
void LtIpMaster::unknownPortDiagnostic()
{
	LtIpReqEchDeviceId	req;
	byte			reqPkt[LtIpReqEchDeviceId::MAX_PKT_SIZE + 4];
	ULONG ipAddr;
	USHORT ipPort;

	if (m_unknownSrcPortDiag.ipAddr != 0)
	{
		m_unknownSrcPortDiag.lock();
		ipAddr = m_unknownSrcPortDiag.ipAddr;
		ipPort = m_unknownSrcPortDiag.ipPort;
		// We are done with this, clear it for another use
		m_unknownSrcPortDiag.ipAddr = 0;
		m_unknownSrcPortDiag.ipPort = 0;
		m_unknownSrcPortDiag.unlock();

		// Send an ID request to the unknown addr/port.
		req.dateTime = getDatetime(false);
		req.sinceDateTime = 0;
		req.requestId = 0;

		// Fill in our own ID info. This will break the logjam if both sides
		// are having their source port switched
		req.localIpAddr = m_ipAddrLocal;
		req.natIpAddr = m_natIpAddr;
		req.ipPort = m_ipPortLocal;
		setPktExtHdrData(&req);
		req.build(reqPkt);

		sendNewPacketTo(reqPkt, req.packetSize, ipAddr, ipPort);
	}
}

// On the iLON, notify the host app of property certain changes
void LtIpMaster::generatePropertyChangeHostEvent()
{
// EPANG TODO - don't support SNTP yet in Linux iLON
#if defined(ILON_PLATFORM) && defined(__VXWORKS__)
	// Notify host app of change
    LtVectorPos     pos;
	LtLreClient*	pClient;
	int				nUids = 0;
	LtUniqueId		uid;

	if (m_bDataValid && (m_pChannel != NULL) &&
		m_pChannel->enumStackClients(pos, &pClient) &&
		(pClient != NULL) &&
		pClient->getAddress(nUids, &uid))
	{
		callDevicePropEventCallback(uid.getData());
	}
#endif
}
// A debug routine
// For 'which'
// 0 = master object
// 1 = m_md5
// 2 = segOut
// 3 = segIn
void LtIpMaster::getAuthParams(int which, boolean* pEnabled, byte* pKey, boolean* pEia852)
{
	switch (which)
	{
	case 1:
		*pEnabled = m_md5.isAuthenticating();
		m_md5.getSecret(pKey);
		*pEia852 = m_md5.isEIA852Auth();
		break;

	case 2:
		*pEnabled = m_segOut.isAuthenticating();
		m_segOut.getSecret(pKey);
		*pEia852 = m_segOut.isEIA852Auth();
		break;

	case 3:
		*pEnabled = m_segIn.isAuthenticating();
		m_segIn.getSecret(pKey);
		*pEia852 = m_segIn.isEIA852Auth();
		break;

	default:	// 0 or other
		*pEnabled = isAuthenticating();
		memcpy(pKey, getAuthenticSecret(), AUTHENTIC_SECRET_SIZE);
		*pEia852 = isEIA852Auth();
		break;
	}
}

// Debug for link allocator
void LtIpMaster::showLinkAlloc()
{
	m_pAlloc->showAllocator();
}
extern "C" void showIpMasterLinkAlloc()
{
	LtIpMaster::getMaster()->showLinkAlloc();
}

#if defined(WIN32)
// On Windows, we maintain a list of SNTP severs for each active IP852 master, and use the
// last one that has been updated.

// Let IP-852 config server set SNTP servers
void LtIpMaster::iLonSetTimeServersFromIP852(ULONG newIpAddr1, USHORT newIpPort1, 
							                 ULONG newIpAddr2, USHORT newIpPort2)
{
    SntpServerList::update(m_nIndex, newIpAddr1, newIpPort1,
                           newIpAddr2, newIpPort2);
}

SEM_ID SntpServerList::m_semSntpServerList = NULL; 
SntpServerList *SntpServerList::m_pSntpServerList = NULL;

SntpServerList::SntpServerList(int ipMasterId, ULONG ipAddr1, USHORT ipPort1, 
                               ULONG ipAddr2, USHORT ipPort2)
{
    m_pNext = NULL;
    m_ipMasterId = ipMasterId;
    m_ipAddr1 = ipAddr1;
    m_ipPort1 = ipPort1;
    m_ipAddr2 = ipAddr2;
    m_ipPort2 = ipPort2;
}

SntpServerList::~SntpServerList()
{
    remove(m_ipMasterId);
}

void SntpServerList::update(int ipMasterId, ULONG ipAddr1, USHORT ipPort1, 
                            ULONG ipAddr2, USHORT ipPort2)
{
    boolean updated = false;
    if (ipAddr1 == 0 && ipAddr1 == 0)
    {
        updated = true;
        SntpServerList::remove(ipMasterId, false);
    }
    else
    {
        lock();
        SntpServerList *p = find(ipMasterId);
        if (p == NULL ||
            p->m_ipAddr1 != ipAddr1 || p->m_ipPort1 != ipPort1 ||
            p->m_ipAddr2 != ipAddr2 || p->m_ipPort2 != ipPort2)
        {
            remove(ipMasterId, false);
            SntpServerList *p = new SntpServerList(ipMasterId, ipAddr1, ipPort1, 
                                                   ipAddr2, ipPort2);
            p->m_pNext = m_pSntpServerList;
            m_pSntpServerList = p;
            updated = true;
        }
        unlock();
    }
    if (updated)
    {
        updateSntp();
    }
}

boolean SntpServerList::remove(int ipMasterId, boolean update)
{
    boolean removed = false;
    lock();
    SntpServerList *p = NULL;
    SntpServerList *pPrev = NULL;
    for (SntpServerList *p = m_pSntpServerList; !removed && p != NULL; pPrev = p, p = p->m_pNext)
    {
        if (p->m_ipMasterId == ipMasterId)
        {
            if (pPrev == NULL)
            {
                m_pSntpServerList = p->m_pNext;
            }
            else
            {
                pPrev->m_pNext = p->m_pNext;
            }
            delete p;
            removed = true;
        }
    }
    unlock();
    if (removed && update)
    {
        updateSntp();
    }
    return removed;
}

SntpServerList *SntpServerList::find(int ipMasterId)
{
    lock();
    SntpServerList *p = NULL;
    for (p = m_pSntpServerList; p != NULL; p = p->m_pNext)
    {
        if (p->m_ipMasterId == ipMasterId)
        {
            break; 
        }
    }
    unlock();
    return p;
}

boolean SntpServerList::getCurrentSntpConfig(ULONG &ipAddr1, USHORT &ipPort1, 
                                             ULONG &ipAddr2, USHORT &ipPort2)
{
    lock();
    boolean found = false;
    if (m_pSntpServerList != NULL)
    {
        ipAddr1 = m_pSntpServerList->m_ipAddr1;
        ipPort1 = m_pSntpServerList->m_ipPort1;
        ipAddr2 = m_pSntpServerList->m_ipAddr2;
        ipPort2 = m_pSntpServerList->m_ipPort2;
        found = true;
    }
    else
    {
        ipAddr1 = 0;
        ipPort1 = 0;
        ipAddr2 = 0;
        ipPort2 = 0;
    }
    unlock();
    return found;
}

void SntpServerList::updateSntp()
{
    bool updatedSntp = false;

    ULONG newIpAddr1;
    USHORT newIpPort1;
    ULONG newIpAddr2;
    USHORT newIpPort2;
    
    boolean sntpEnabled = getCurrentSntpConfig(newIpAddr1, newIpPort1, newIpAddr2, newIpPort2);
    ULONG oldIpAddr;
    USHORT oldIpPort;
    iLonSntpGetServer(0, &oldIpAddr, &oldIpPort);
    if (newIpAddr1 != oldIpAddr || newIpPort1 != oldIpPort)
    {
        updatedSntp = true;
        iLonSntpSetServer(0, newIpAddr1, newIpPort1, false);
    }

    iLonSntpGetServer(1, &oldIpAddr, &oldIpPort);
    if (newIpAddr2 != oldIpAddr || newIpPort2 != oldIpPort)
    {
        updatedSntp = true;
        iLonSntpSetServer(1, newIpAddr2, newIpPort2, false);
    }

    if (!sntpEnabled)
    {
         ilonSntpResetOffset();
    }
    if (updatedSntp)
    {
        iLonSntpUpdateNow();
    }
 }

void SntpServerList::shutdown()
{
    // If m_semSntpServerList is NULL, we don't have anything to do.
    if (m_semSntpServerList != NULL)
    {
        if (lock())
        {
            while (m_pSntpServerList)
            {
                SntpServerList *p = m_pSntpServerList;
                m_pSntpServerList = p->m_pNext;
                delete p;
            }
            unlock();
            semDelete(m_semSntpServerList);
            m_semSntpServerList = NULL;
        }
    }
}

boolean SntpServerList::lock()
{
    if (m_semSntpServerList == NULL)
    {
        m_semSntpServerList = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
    }
    return semTake(m_semSntpServerList, WAIT_FOREVER) == OK; 
}

void SntpServerList::unlock()
{
    semGive(m_semSntpServerList);
}

#endif

// end
