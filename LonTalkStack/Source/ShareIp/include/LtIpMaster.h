#if !defined(AFX_LTIPMASTER_H__68FEF4F1_C68E_11D2_A837_00104B9F34CA__INCLUDED_)
#define AFX_LTIPMASTER_H__68FEF4F1_C68E_11D2_A837_00104B9F34CA__INCLUDED_
/***************************************************************
 *  Filename: LtIpMaster.h
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
 *  Description:  interface for the LtIpMaster class.
 *
 *	DJ Duffy Feb 1999
 *
 ****************************************************************/

/*
 * $Log: /Dev/ShareIp/include/LtIpMaster.h $
 *
 * 89    9/20/07 6:00p Fremont
 * EPR 46461 - event mechanism for device property change notification
 *
 * 88    8/23/05 12:02p Fremont
 * Don't write out persistent data when setting auth is there is no change
 *
 * 87    7/21/05 11:05a Fremont
 * Moved config server comm test status from LtIpMaster.h to LtIpChannel.h
 * to decouple LNS exports from LtIpMaster.h
 *
 * 86    7/20/05 3:48p Bobw
 * EPR 36316
 * Return an error when setting secret key before persistence files are
 * read (since reading persistence files will overwrite the key).
 *
 * Also fix persistent write task to ensure that closing the stack will
 * write any pending data before exit.
 *
 * 85    7/20/05 12:19a Fremont
 * EPRs 37379 and 37566 -- make Web or console configured config server
 * address stick, add support for Web validate function for config server
 *
 * 84    5/23/05 5:38p Fremont
 * Tweak the link observer code
 *
 * 83    5/06/05 4:04p Fremont
 * Limit persistent writes by throttling and by not saving every fetch of
 * a date/time
 *
 * 82    12/09/04 6:23p Fremont
 * Change conditional compilation for self installed multicast
 *
 * 81    12/09/04 5:14p Fremont
 * EPR 35367 - reboot after local port change confuses LT stack
 *
 * 80    11/23/04 5:39p Fremont
 * EPRS FIXED: 35208 - Bombardier special - self-installed multicast IP
 * channel mode
 *
 * 79    11/05/04 3:53p Fremont
 * add protocol analyzer mode to IP Master
 *
 * 77    5/21/04 2:01p Bobw
 * EPR 18520
 * Add methods to wait for L5 or LONTAlk IP resync
 *
 * 76    1/19/04 3:24p Fremont
 * Name change for LON link pointer in IP master
 *
 * 75    10/09/03 2:14p Fremont
 * change access methods for channel mode vars and NAT addr
 *
 * 74    10/01/03 5:08p Fremont
 * fix command line statistics reporting control
 *
 * 73    9/22/03 1:38p Iphan
 * EPRS FIXED:
 * Change the max number of devices per channel from 255 to 256
 *
 * 72    9/19/03 3:11p Fremont
 * adjust functions for auth, extended headers, protocol version
 * Moved/renamed UDP packet size literal
 *
 * 71    9/16/03 5:34p Fremont
 * add new statistic for alternate auth, add auth diagnostic
 *
 * 70    9/09/03 6:47p Fremont
 * add routines for sending saved packets
 *
 * 69    9/06/03 4:03p Fremont
 * Add EIA852 name length literal
 *
 * 68    9/03/03 11:22p Fremont
 * change unknown port diag
 *
 * 67    9/03/03 6:58p Fremont
 * reset extended pkt header length to zero
 *
 * 66    9/03/03 5:36p Fremont
 * Implement extended packet header for EPR 29618.
 *
 * 65    8/27/03 12:35p Fremont
 * support for CIpLink class to help determine a source port that may have
 * been switched by a firewall/NAT box
 *
 * 64    8/19/03 3:00p Fremont
 * move persistent data definitions here from LtIpMaster.cpp
 *
 * 63    8/06/03 8:11p Fremont
 * remove XML config object pointer
 *
 * 62    8/05/03 5:21p Fremont
 * regenerate device reg pkt is NAT IP addr changes, compare local and NAT
 * IP addrs for CHR requests
 *
 * 61    7/10/03 6:34p Fremont
 * add callback for xml config dump
 *
 * 60    6/20/03 2:31p Fremont
 * more stuff for local XML config
 *
 * 59    6/19/03 4:07p Iphan
 * Adding support for device Connected/Configured LED
 *
 * 58    6/16/03 4:33p Fremont
 * rename the YAF stuff to WritePersist
 *
 * 57    6/10/03 5:35p Fremont
 * changes for EIA0852 auth
 *
 * 56    6/05/03 4:00p Fremont
 * Callbacks for local XML config loading, fix static member init
 *
 * 55    6/05/03 2:10p Iphan
 * iLON600: Fixed router statistic info
 *
 * 54    6/04/03 6:30p Iphan
 * IKP06042003: Fixed statistics shown in console & ConfigServer
 * IKP06042003: Support for LT & LTIP statistics in System Info
 *
 * 53    6/03/03 7:16p Fremont
 * statistics cleanup
 *
 * 52    6/02/03 6:00p Fremont
 * Changes for NAT support
 *
 * 51    5/02/03 3:30p Iphan
 * IKP05022003: Increased MAX_MEMBERS from 40 to 255
 *
 * 50    4/15/03 6:54p Fremont
 * track shared IP addresses, save that and backward compat mode to
 * persist data
 *
 * 49    3/27/03 2:34p Fremont
 * make setConfigServer() public so rtrSetConfigServer() can use it
 *
 * 48    3/25/03 12:43p Fremont
 * add config server type determination, add port to findChannelMember
 *
 * 47    3/04/03 3:47p Fremont
 * beginnings of version 2 and backward compatible support, change
 * duplicate member detection, add mode packet
 *
 * 46    11/03/00 9:05a Bobw
 * EPR 18718
 * If config server sends channel membership packets with duplicate
 * entries (devices with the same IP address), reject it - otherwise we
 * end up with duplicate entries referencing the same object, and all
 * sorts of bad things happen.
 *
 * 45    11/01/00 1:07p Bobw
 * EPR 18689
 * Expose interfaces to allow a client to verify communication with the
 * config server, and to optionally set the config server's address
 * locally.
 *
 * 44    4/26/00 2:53p Darrelld
 * Report invalid IP address properly
 *
 * 43    3/27/00 1:16p Darrelld
 * Fix channel deadlock
 *
 * 42    3/23/00 1:31p Darrelld
 * Errors returned from master start
 *
 * 41    3/16/00 3:38p Darrelld
 * Fix max sizes
 *
 * 40    3/14/00 9:57a Darrelld
 * Remove ($)Log
 *
 * 39    3/13/00 5:28p Darrelld
 * Segmentation work
 *
 * 38    2/25/00 5:46p Darrelld
 * Fixes for PCLIPS operation
 *
 * 37    2/23/00 9:09a Darrelld
 * LNS 3.0 merge
 *
 * 36    1/17/00 10:07a Glen
 * New initialization paradigm - Now you should be able to simply create a
 * LtLtLogicalChannel or LtIpLogicalChannel and create a stack.  Channel
 * create and delete take care of creating master for IP side, port client
 * for LT side and LRE in either case
 *
 * 44    1/11/00 5:00p Darrelld
 * Fix handling of Channel Routing packet
 *
 * 43    12/22/99 4:45p Darrelld
 * Reduce max members to 40, and enforce it.
 *
 * 42    12/21/99 5:01p Darrelld
 * Speed up requests
 *
 * 41    12/21/99 1:53p Darrelld
 * Add timing and enhance doRequestInfo
 *
 * 40    12/21/99 9:32a Darrelld
 * Add the YAFT to write persistence
 *
 * 39    12/20/99 3:13p Darrelld
 * Fix clamping... Again
 *
 * 38    12/20/99 11:45a Darrelld
 * Fix clamping
 *
 * 37    12/20/99 10:48a Darrelld
 * Clamp statistics
 *
 * 36    11/16/99 11:21a Darrelld
 * EPRS FIXED: 15662, 15433 BW limit causes too much delay.
 * Also fix aggregation to send first packet immediately to reduce delay.
 *
 * 35    11/12/99 3:44p Darrelld
 * Updates for segment support
 *
 * 34    10/22/99 1:13p Darrelld
 * Don't use zero time stamp for certain cases
 *
 * 33    10/21/99 5:11p Darrelld
 * Change session number on move and change
 *
 * 32    10/08/99 10:42a Darrelld
 * Increase MAX_DOMAINS and fix checking
 *
 * 31    10/06/99 4:08p Darrelld
 * Better handling of channel routing and reporting link statistics
 *
 * 30    9/08/99 2:22p Darrelld
 * Session number is now a ULONG
 * Add statistics packet
 *
 * 29    9/02/99 1:39p Darrelld
 * Add deletePersistence
 *
 * 28    8/13/99 1:05p Darrelld
 * Initialize session id
 * bug in sweepLocalClients
 * process NAK for channel routing packets
 *
 * 27    8/05/99 5:24p Darrelld
 * update of config server and local port
 * updates for target router control
 *
 * 26    8/03/99 2:47p Darrelld
 * Fix startup issues
 *
 * 25    8/02/99 4:57p Darrelld
 * Fixes to start master
 *
 * 24    8/02/99 1:51p Darrelld
 * Two NTP servers, and start the pncSntpClient
 *
 * 23    7/30/99 8:59a Darrelld
 * Cleanup and streamline
 *
 * 22    7/27/99 1:50p Darrelld
 * Fix locks and allow soft task exit
 *
 * 21    7/26/99 5:18p Darrelld
 * Debugging of IP router features
 *
 * 20    7/20/99 5:26p Darrelld
 * Updates for reordering test
 *
 * 19    7/14/99 11:01a Darrelld
 * Enhance for persistance and nvram support
 *
 * 18    7/06/99 5:23p Darrelld
 * Lock nesting debugging
 *
 * 17    7/06/99 1:05p Darrelld
 * Clean up tasks and timers
 *
 * 16    7/02/99 8:45a Darrelld
 * Target specific files
 *
 * 15    6/30/99 4:41p Darrelld
 * Handle statistics properly
 *
 * 14    5/14/99 2:44p Darrelld
 *
 * 13    5/07/99 3:30p Darrelld
 * More enhancement to RFC handling
 *
 * 12    5/06/99 5:09p Darrelld
 * Enhancments to RFC packets
 *
 * 11    5/03/99 4:04p Darrelld
 * Add sending aggregated packets as they fill up when BW limiting is not
 * enabled.
 *
 * 10    4/16/99 3:04p Darrelld
 * Aggregation and BW limiting works
 *
 * 8     4/02/99 5:21p Darrelld
 * Multi-Router Testing in Memory
 *
 * 7     3/30/99 5:10p Darrelld
 * intermediate checkin
 *
 * 6     3/15/99 5:03p Darrelld
 * Enhance for update and check status. Fix memory leaks.
 *
 * 5     3/11/99 5:01p Darrelld
 * intermediate checkin
 *
 * 4     3/05/99 2:37p Darrelld
 * Intermediate check-in
 *
 * 3     3/03/99 4:54p Darrelld
 * intermediate checkin
 *
 * 2     3/01/99 5:08p Darrelld
 * Intermediate checkin
 *
 * 1     2/22/99 9:25a Darrelld
 *
 *
 */

#include <LtIpBase.h>
#include <LtIpPackets.h>
#include <LtInit.h>
#include "Segment.h"
#include <LtMD5.h>
#include <IpLink.h>
#include <Observer.h>
// EPANG TODO - Linux iLON doesn't have a LonLinkObserver for now
#if defined(__VXWORKS__)
# include <LonLinkObserver.h>
#endif
#include "SelfInstallMulticast.h"
#include <LtIpChannel.h>


// forward or external decleration
class LtLreIpClient;
class LtLreIpMcastClient;
class LtIpConnectState;
class LtIpMaster;

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// helper macros

#define FREEANDCLEAR(a) if ( a != NULL ) { ::free(a); a = NULL; }
#define DELETEANDCLEAR(a) if ( a != NULL ) { delete[] a; a = NULL; }


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// Statistics
//
struct LtIpStats
{
	UINT	nPacketsSent;
	UINT	nPacketsReceived;
	UINT	nBytesSent;
	UINT	nBytesReceived;
	UINT	nPacketsDropped;
	UINT	nPacketsMissed;
	UINT	nPacketReceiveErrors;
	UINT	nSendListPackets;
	UINT	nInvalidPackets;
	UINT	nTooMany;
	UINT	nAuthFailures;
	UINT	nAltAuthUsed;
	UINT	nLastTick;
	UINT	nLastPktSent;
	UINT	nLastPktRecv;
	UINT	nLastBytesSent;
	UINT	nLastBytesRecv;

	void setLast( UINT nLT )
	{
		nLastTick = nLT;
		nLastPktSent = nPacketsSent;
		nLastPktRecv = nPacketsReceived;
		nLastBytesSent = nBytesSent;
		nLastBytesRecv = nBytesReceived;
	}

	void getLast( UINT nSec )
	{
		nLastPktSent = (nPacketsSent - nLastPktSent)/nSec;
		nLastPktRecv = (nPacketsReceived - nLastPktRecv)/nSec;
		nLastBytesSent = (nBytesSent - nLastBytesSent)/nSec;
		nLastBytesRecv = (nBytesReceived - nLastBytesRecv)/nSec;
	}

	void clear()
	{
		nLastTick = 0;
		nBytesSent = 0;
		nBytesReceived = 0;
		nPacketsSent = 0;
		nPacketsReceived = 0;
		nPacketsDropped = 0;
		nPacketsMissed = 0;
		nPacketReceiveErrors = 0;
		nSendListPackets = 0;
		nInvalidPackets = 0;
		nTooMany = 0;
		nAuthFailures = 0;
		nAltAuthUsed = 0;
	};
};

struct LtLreIpStats;

struct LtIpWritePersistMsg
{
	byte*	pData;
	ULONG	nSize;
};

//////////////////////////////////////////////////////////////////////////////////////
class LtIpConnectState : public Subject
{
	public:
		typedef enum { NOT_ACTIVE_MEMBER = 0, CONFIG_OUT_OF_DATE, ACTIVE_MEMBER } eLTIP_CONNECT_STATE;
		LtIpConnectState()
		{
// EPANG TODO - Linux iLON and simulators don't have a LonLinkObserver for now
#if defined(__VXWORKS__)
			LonLinkObserver::getInstance(this);	// attaches subject to observer
#endif
		}
		virtual ~LtIpConnectState(){}
		eLTIP_CONNECT_STATE GetState();
};


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// IP Master Class
//
// To debug deadlocks, there is support available via a symbol
// #define DEBUG_LOCKS // must be defined in the header file or project

// forward declaration
struct MasterDataV1;
struct MasterDataV2;
struct MasterDataV3;

// Prototype for CS comm test. Calls extended tests in the LtIpMaster
CsCommTestSts testConfigServerComm(int waitSecs);

class LtIpMaster : public LtIpBase, public LtEventClient
{
	friend class LtIpMasterXmlConfig;

public:
	LtIpMaster( ULONG ipAddress = 0, word ipPort = 1628 );
	virtual ~LtIpMaster();

	// Set the channel objects from whence all other information flows
	void	setChannel( LtLogicalChannel* pChannel );
	// set an error in the channel during startup
	void	setChannelError( LtErrorType err )
	{	if ( m_pChannel )
		{	m_pChannel->setStartError( err );
		}
	}
	void	lockChannel()
	{	lock();	// always lock us first
		if ( m_pChannel )
		{	m_pChannel->lock();
		}
	}
	void	unlockChannel()
	{
		if ( m_pChannel )
		{	m_pChannel->unlock();
		}
		unlock();	// always unlock ourselves last
	}

	inline void setWorkMask( int a_iWorkMask )
	{
		lock();
		m_nWorkMask |= a_iWorkMask;
		unlock();
	}

	inline void clearWorkMask( int a_iWorkMask )
	{

		lock();
		m_nWorkMask &= ~a_iWorkMask;
		unlock();
	}

	inline void setTimerMask( int a_iTimerMask )
	{
		lock();
		m_nTimerMask |= a_iTimerMask;
		unlock();
	}

	inline void clearTimerMask( int a_iTimerMask )
	{

		lock();
		m_nTimerMask &= ~a_iTimerMask;
		unlock();
	}

	inline void setReqtMask( int a_iReqtMask )
	{
		lock();
		m_nReqtMask |= a_iReqtMask;
		unlock();
	}

	inline void clearReqtMask( int a_iReqtMask )
	{

		lock();
		m_nReqtMask &= ~a_iReqtMask;
		unlock();
	}

	// Set pointer to corresponding native LON link for stats (only for a router)
	void	setLonLink( LtLink* pLonLink) { m_pLonLink = pLonLink; }

	// Set the config server's address locally.
    boolean	setConfigServer( ULONG ipAddr, word ipPort, boolean updateLastKnownHost=true );
    boolean	setConfigServer( char* pszHost, word ipPort ); // This is the prefered one

	// the following is a local override for the port
	boolean	setLocalPort( word ipPort );
	word	getLocalPort()
	{	return m_ipPortLocal;
	}
    ULONG getLocalIpAddress() { return m_ipAddrLocal; }

	void	setNatAddress(ULONG natIpAddr);
	void	getNatAddress(ULONG& natIpAddr) { natIpAddr = m_natIpAddr; }
	void	getConfigServer( ULONG& ipAddr, word& ipPort )
	{
		ipAddr = m_ipAddrCfgServer;
		ipPort = m_ipPortCfgServer;
	}
	// must pass in a buffer of at least 64 chars
	void	getLastKnownConfigServer( char* pHost, word& ipPort )
	{
		strcpy(pHost, m_lastKnownCsHost);
		ipPort = m_lastKnownCsPort;
	}


    boolean startConfigServerCheck( ULONG csIpAddr, word csIpPort );
    boolean getConfigServerCheckComplete(ULONG &csIpAddr, word &csIpPort);
	CsCommTestSts startConfigServerCheckEx(ULONG csIpAddr, word csIpPort);
	CsCommTestSts getConfigServerCheckCompleteEx(ULONG &csIpAddr, word &csIpPort);

	LtErrorType waitForPendingInterfaceUpdates(void);

	void	getTimeServers( ULONG& ipAddr, word& ipPort,
							ULONG& ipAddr2, word& ipPort2  )
	{
		ipAddr = m_ipAddrNtpServer;
		ipPort = m_ipPortNtpServer;
		ipAddr2 = m_ipAddrNtpServer2;
		ipPort2 = m_ipPortNtpServer2;
	}

	// get the instance of the master. Can be NULL
	static LtIpMaster*	getMaster();
	// remove all persistence files
	void deletePersistence();

	void	setIndex( int nIdx );
	// client is departing
	friend class LtLreIpClient;

	void	clientBye( LtLreIpClient* pClient );
    void	clientMcastBye( LtLreIpMcastClient* pClient );

	// LtEventClient methods
	// we are an event client to receive notification from the far-side
	// about routing changes.

	virtual void eventNotification(LtEventServer* pServer);


	// Manage the link on start and stop
	virtual boolean		start();
	void				lreClientChange();
			void		registerWithLreClients();
	//		boolean		createClients();
	virtual boolean		stop();
	virtual boolean		stopBase();

	virtual void packetReceived( boolean bPriority,
								LtPktInfo* pPkt, LtSts sts );


	// statistics
	void	getClientCounts( LtLreIpStats& stats );

	// IKP06042003: added support for clearing link statistics
	const   static boolean bIncludeLinkStatistics;
	const   static boolean bNoLinkStatistics;

	void	clearClientCounts( boolean bIncludeLink = bNoLinkStatistics );

	void	getCounts( LtIpStats& stats )
	{	stats = m_stats;
	}

	// IKP06042003: added support for clearing link statistics
	void clearCounts( boolean bIncludeLink = bNoLinkStatistics )
	{
		CIpLink*	pIpLink;

		m_stats.clear();

		// IKP06042003: clear the link statistics if necessary
		if (bIncludeLink)
		{
			pIpLink = (CIpLink*)getLink();
			if ( pIpLink )
				pIpLink->clearStatistics();
		}
	}

	void	countClamp( UINT& cnt )
	{	if ( cnt != STATS_OVERFLOW ) cnt++;
	}
	UINT	addClamp( UINT& cnt, UINT add )
	{
		if ( cnt != STATS_OVERFLOW )
		{
			if ( ( cnt + add ) >= cnt ) cnt += add;
			else	cnt = STATS_OVERFLOW;
			if ( cnt == (UINT)STATS_UNSUPPORTED ) cnt = STATS_OVERFLOW;
		}
		return cnt;
	}
	void    countPacketsSent()
	{	countClamp( m_stats.nPacketsSent ); }
	void    countPacketsReceived()
	{	countClamp( m_stats.nPacketsReceived ); }
	void    countPacketsDropped()
	{	countClamp( m_stats.nPacketsDropped ); }
	void    countPacketsMissed()
	{	countClamp( m_stats.nPacketsMissed ); }
	void    countPacketReceiveErrors()
	{	countClamp( m_stats.nPacketReceiveErrors ); }

	void    countSendListPackets()
	{	countClamp( m_stats.nSendListPackets ); }
	void    countInvalidPackets()
	{	countClamp( m_stats.nInvalidPackets ); }
	void	countTooMany()
	{	countClamp( m_stats.nTooMany ); }
	void	countAuthFailures()
	{	countClamp( m_stats.nAuthFailures ); }
	void	countAltAuthUsed()
	{	countClamp( m_stats.nAltAuthUsed); }
	void	countBytesSent( UINT nBytes )
	{	addClamp( m_stats.nBytesSent, nBytes );
	}
	void	countBytesReceived( UINT nBytes )
	{	addClamp( m_stats.nBytesReceived, nBytes );
	}


	// report status for testing
	virtual void	reportStatus( boolean bAllMembers = TRUE, boolean bLinksToo = FALSE );

	boolean			getAggregate( int* pnMsDelay )
	{	if ( pnMsDelay ) *pnMsDelay = m_nAggregateMs;
		return m_bAggregate;
	}
	boolean			getClientMustQueue();
	// eventually the master will discuss this with the LtInit object
	// and it will be a more intelligent number
	int				getClientQueueDepth()
	{	return 100;
	}

	boolean			getBWLimit( int* pnKBPerSec )
	{	if ( pnKBPerSec ) *pnKBPerSec = m_nBWLimitKBPerSec;
		return m_bBWLimit;
	}
	// called by a client that wishes to send
	boolean			okToSend( ULONG nSize );


	boolean			getReorderPackets()
	{	return m_bReorderPackets;
	}
	boolean			isReordering()
	{	return m_bReorderPackets;
	}

	ULONG			getEscrowTime()
	{	return m_nEscrowTimeMs;
	}

	boolean			getChannelTimeout( word* pReturn )
	{	if ( pReturn ) *pReturn = m_nChannelTimeout;
		return m_bCheckStale;
	}
	// should we use a timestamp
	boolean			useTimestamp()
	{	return m_nChannelTimeout != 0 &&
				( m_ipAddrNtpServer || m_ipAddrNtpServer2 );
	}
	// timeout for packet gets stale waiting on aggregation
	// one half the channel timeout value
	boolean			isStaleSending( ULONG timeStampNow, ULONG timeStampPkt )
	{
		boolean		bStale = false;
		if ( m_nChannelTimeout && m_bCheckStale )
		{	bStale = ( timeStampNow - timeStampPkt ) > ((ULONG)m_nChannelTimeout/2);
		}
		return bStale;
	}
	// timeout for stale packet that's inbound
	boolean			isStale( ULONG timeStampNow, ULONG timeStampPkt )
	{	boolean		bStale = false;
		if ( m_bCheckStale && m_nChannelTimeout )
		{	bStale = ( timeStampNow - timeStampPkt ) > ((ULONG)m_nChannelTimeout);
		}
		return bStale;
	}

	// data to build packets
	ULONG			getSession();
	void			newSession();

	static ULONG	getTimestamp(boolean bSyncCheck=true);
	ULONG			getDatetime(boolean writePersist=true);
	void			setAuthentication( boolean bEnable, boolean writePersist=true )
	{	// set the first few unconditionally, but check the main value for a change
		m_md5.setAuthenticating(bEnable);
		m_segOut.setAuthenticating(bEnable);
		m_segIn.setAuthenticating(bEnable);
		if (m_bAuthenticate != bEnable)
		{
			m_bAuthenticate = bEnable;
			if (writePersist)
			{
				// write out new values
				startTimedWork( WORK_WritePersist, 16 );
			}
		}
		generatePropertyChangeHostEvent();	// For iLON
	}
	boolean			isAuthenticating()
	{	return m_bAuthenticate;
	}
	void			setMembersEIA852Auth()
	{
		m_md5.setEIA852Auth(isEIA852Auth());
		m_segOut.setEIA852Auth(isEIA852Auth());
		m_segIn.setEIA852Auth(isEIA852Auth());
	}
	boolean			isEIA852Auth()
	{
		return(!backwardCompatibleChan());
	}

	enum { AUTHENTIC_SECRET_SIZE = 16,
			MAXNAMELEN = 130,
			EIA852_MAXNAMELEN = 128,
	};

	void			setAuthenticSecret( byte* pSecret, boolean writePersist=true )
	{	memcpy( &m_aAuthenticSecret, pSecret,  AUTHENTIC_SECRET_SIZE );
		m_md5.setSecret(pSecret);
		m_segOut.setSecret(pSecret);
		m_segIn.setSecret(pSecret);
		if (writePersist)
		{
			startTimedWork( WORK_WritePersist, 16 );
		}
		generatePropertyChangeHostEvent();	// For iLON
	}
	byte*			getAuthenticSecret()
	{	return m_aAuthenticSecret;
	}

    boolean getPersitenceRead()
    {
        return m_bPersistenceRead;
    }

	// channel routing packet is not acked
	boolean unackedChannelRouting()
	{	return ( 0 != (m_nReqtMask & REQT_SENDCHANROUTING) ) && (m_nReqtCount>2);
	}

	boolean backwardCompatibleChan()
	{
		return(m_runningEchProtocolVer == LTIP_ECH_VER1);
	}
	void setEchProtocolVersion(LtIpEchProtocolVersion ver);
	void getEchProtocolVersion(LtIpEchProtocolVersion& ver) { ver = m_runningEchProtocolVer; }
	boolean useExtPktHdrs()
	{
		return (!backwardCompatibleChan() && !m_bStrictEia852);
	}
	void setPktExtHdrData(LtIpPktHeader *pHdr)
	{
		pHdr->extHdrLocalIpAddr = m_ipAddrLocal;
		pHdr->extHdrNatIpAddr = m_natIpAddr;
		pHdr->extHdrIpPort = m_ipPortLocal;
		pHdr->bHasExtHdr = useExtPktHdrs();
		pHdr->extndHdrSize = 0;	// This will be modified (if appropriate) in build()
	}
	void setSegmentorExtHdrData()
	{
		m_segOut.setExtHdrData(m_ipAddrLocal, m_natIpAddr, m_ipPortLocal, useExtPktHdrs());
		m_segIn.setExtHdrData(m_ipAddrLocal, m_natIpAddr, m_ipPortLocal, useExtPktHdrs());
	}
	void setStrictEia852(boolean strictEia852);
	void getStrictEia852(boolean& strictEia852) { strictEia852 = m_bStrictEia852; }

	ULONG getLastStatisticsResetTime()
	{
		return m_dtStatisticsReset;
	}

	void setLastStatisticsResetTime(ULONG dtNow)
	{
		m_dtStatisticsReset = dtNow;
	}

	// Support for manual/local config via XML file
	void installXmlCallbacks(FUNCPTR loadConfig_Callback, FUNCPTR dumpConfig_Callback)
	{
		m_loadXmlConfig_Callback = loadConfig_Callback;
		m_dumpXmlConfig_Callback = dumpConfig_Callback;
	}
	boolean loadXmlConfig();
	boolean dumpXmlConfig();

	LtIpConnectState::eLTIP_CONNECT_STATE getConnectState();

	void requestUnknownPortDiagnostic(ULONG ipAddr, USHORT ipPort);
	void getAuthParams(int which, boolean* pEnabled, byte* pKey, boolean* pEia852);

	void setProtocolAnalyzerMode(boolean enable)
	{
		m_wantsAllPackets = enable;
		eventNotification(NULL);
	}
	boolean protocolAnalyzerMode() { return m_wantsAllPackets; }
    boolean selfInstalledMcastMode() { return (m_selfInstalledMcastAddr != 0); }
	boolean setSelfInstalledMcastAddr(ULONG mcastAddr);
	void setSelfInstalledMcastHops(int hops) { m_selfInstalledMcastHops = hops; }
	void showLinkAlloc();

protected:
	// data
#ifdef WIN32	// on the PC, use larger limits
	enum
	{
		MAX_DOMAINS	= 100,		// max domains we can handle now
		MAX_NODES		= 200,		// max nodes we handle
		MAX_UIDS		= 100			// max uids we handle
	};
#else // on i.LON, use smaller limits now
	enum
	{
		MAX_DOMAINS	= 10,			// max domains we can handle now
		MAX_NODES		= 20,			// max nodes we handle
		MAX_UIDS		= 20			// max uids we handle
	};
#endif // WIN32

	enum
	{
		MAX_MEMBERS		= 256,		// IKP05022003: Increased from 40 to 256
		RETRY_COUNT		= 3,			// Max times to retry before giving up
		RETRY_MS		= 2000,			// milliseconds per retry
		MIN_RESEND_MS	= 500,		// minimum resent time for a packet
		CR_HOLDDOWN_MS	= 100,	// hold down time for channel routing.
		JUNK
	};


	LtLreServer*		m_pServer;		// The Lre Engine
	LtLogicalChannel*	m_pChannel;		// IP Channel
	LtLink*				m_pLonLink;		// LON Channel Link (for stats)

	ULONG				m_ipAddrCfgServer;	// save just because
	word				m_ipPortCfgServer;
	char				m_lastKnownCsHost[64];	// Last known CS set locally or from CS. Name or addr.
	word				m_lastKnownCsPort;
	boolean				m_updateCsHostName; // CS addr set by CS, sync "last known" CS
	ULONG				m_ipAddrLocal;		// save to tell clients
	ULONG				m_natIpAddr;		// NAT IP address
	word				m_ipPortLocal;
	ULONG				m_ipAddrNtpServer;	// Ntp server ip address
	word				m_ipPortNtpServer;	// Ntp server ip port
	ULONG				m_ipAddrNtpServer2;	// Ntp server ip address
	word				m_ipPortNtpServer2;	// Ntp server ip port
	word				m_nChannelTimeout;	// 0-1500 MS channel timeout
	boolean				m_bCheckStale;		// check packets timed out
	boolean				m_bReorderPackets;	// reorder inbound packets
	ULONG				m_nEscrowTimeMs;	// packet reorder escrow timer
	boolean				m_bAuthenticate;	// authenticate all packets
											 // authentication secret
	byte				m_aAuthenticSecret[AUTHENTIC_SECRET_SIZE];
	LtMD5				m_md5;				// just an object to call with

	// these items are in NVRAM
	ULONG				m_nSession;			// session number
	ULONG				m_lastDatetime;		// last date time of a config packet
	char				m_szNvRamKey[16];	// qualified key for nv ram

	LtIpEchProtocolVersion	m_runningEchProtocolVer;	// active Echelon protocol ver for chan
	boolean				m_bStrictEia852;	// strict EIA-852 compliance
#ifdef ILON_100_ROUTER_DEMO
	ULONG				m_prevIpAddrLocal;	// local IP addr last time we ran, if changed
#endif
	boolean				m_wantsAllPackets;	// Promiscuous mode for protocol analyzers
    // Multicast address
	ULONG				m_selfInstalledMcastAddr; // Multicast addr for special self-installed channels
	ULONG				m_actualIpAddrLocal;	  // The real value -- m_ipAddrLocal will be spoofed
	int					m_selfInstalledMcastHops; // Max hops (TTL) for multicast messages

	LtIpStats			m_stats;		// statistics

	int					m_tidWorker;	// worker task id
	int					m_tidCheckStuck;// check stuck task
	boolean				m_bTaskExit;	// true for task should exit
	SEM_ID				m_semWork;		// worker task synch
	WDOG_ID				m_tPending;		// timeout id
	WDOG_ID				m_tRequest;		// request timer id
	LtQue				m_qRFCin;		// rfc messages received
	int					m_tidWritePersist;	// Task to write persistence
	MSG_Q_ID			m_mqWritePersist;	// messsage queue for above task
	boolean				m_mStaticConfig;	// Configured via static XML file

	boolean				m_bStopping;	// true if stopping to prevent access
										// to segments during stop process
	// the segmentor objects handle segmentation of RFC messages
	LtIpSegmentor		m_segOut;		// outbound segmentor
	LtIpSegmentor		m_segIn;		// inbound segmentor

	enum {
		WORK_ReadPersist		= 1,	// read persistent data
		WORK_WritePersist		= 2,	// write persistent data
		WORK_SendChanRouting	= 4,	// send channel routing
		WORK_SendDevRegister	= 8,	// send device registration
		WORK_RequestInfo		= 16,	// Request something
		WORK_SetLink			= 32,	// set a link parameter
		WORK_END				= 0,
		REQT_DEVRESPONSE		= 1,	// Request a device response
		REQT_CHANMEMBERS		= 2,	// Request a channel routing packet
		REQT_CHANROUTING		= 4,	// Request channel membership packet
		REQT_SENDCHANROUTING	= 8,	// Request to send a channel routing packet
		REQT_CSTYPE				= 0x10, // Request the config server's type
		REQT_END				= 0
	};

	int					m_nReqtCount;		// tells us how long to delay
	int					m_nReqtMask;		// request mask
	boolean				m_baReqtCR[MAX_MEMBERS];	// mask of packets we need to
													// request
	int					m_nReqtCRipAddr;	// ipAddr of channel routing requested
	int					m_nWorkMask;		// mask to work on
	int					m_nTimerMask;		// mask to work on when timer expires
	char				m_cPersistName[32];
	// control information for sending packets
	boolean				m_bUseTCP;
	byte*				m_pPktPending;
	int					m_nPktPending;
	int					m_nRetransmitCount;
	ULONG				m_tickLastSendChanRouting;	// ticks when we last sent channel routing

	boolean				m_bDataValid;
	int					m_nDataIncarnation;

	int					m_nMembers;				// members in current membership
	boolean				m_hasSharedIpAddrs;		// some member IP addresses are the same (but different ports)
	int					m_nIndex;				// test index
	int					m_nOurIndex;			// our index in the member table

	byte*			m_pktChanMembers;			// the membership packet
	int				m_nPktCMSize;				// size of this packet
	ULONG			m_dtPktCM;					// date-time of this packet
	byte*			m_pktDevRegister;			// our own device registration packet
	byte*			m_pktOurChanRouting;		// our own channel routing packet
	int				m_nPktOCRSize;				// size of this packet
	LtIpAddPortDate	m_apdMembers[MAX_MEMBERS];	// parsed data for current members
	byte*			m_pktChanRouting[MAX_MEMBERS];	// The channel routing packets in member order
	LtLreIpClient*	m_apClients[MAX_MEMBERS];	// array of client addresses
	LtLreIpMcastClient*	m_apMcastClient;        // MUTLICAST client address
	char			m_acName[MAXNAMELEN];		// LonMark RFC name

	ULONG			m_dtStatisticsReset;		// datetime of last statistics reset

	enum {
		CS_TYPE_UNKNOWN,
		CS_TYPE_ECH_V2,
		CS_TYPE_OTHER,
	};
	int				m_cnfgServerType;			// is the CS is Echelon's Ver2?
	int				m_cnfgSrvrTypeChkCount;		// count of attempts to determine above var
	ULONG			m_CSmsgRcvd;				
	ULONG			m_CSmsgRcvdCheckpoint;		// Used for CS type check
	CsCommTestSts	m_CsCommTestSts;

	// Support for manual/local config via XML file
	// Should change this once parser is part of core baseline for iLON 100
	FUNCPTR			m_loadXmlConfig_Callback;	// Look for/install local XML config
	FUNCPTR			m_dumpXmlConfig_Callback;	// dump local XML config

	LtIpConnectState m_ConnectLedObSubject;

	class UnknowSrcPortDiag	: public VxcLock
	{
	public:
		ULONG			ipAddr;
		USHORT			ipPort;
		time_t			timestamp;
		int				throttleCount;
	};
	UnknowSrcPortDiag m_unknownSrcPortDiag;

#ifdef DEBUG_LOCKS
	// keep track of lock owner
	#define LOCKDEPTHMAX 20
	char*			m_pszLockFile[LOCKDEPTHMAX];
	int				m_nLockLine[LOCKDEPTHMAX];
	int				m_nLockDepth;
	void	xLock( char* pszFile, int nLine );
	void	xUnlock( char* pszFile, int nLine );
public:
	void	xReportLockStack();
#endif // DEBUG_LOCKS

protected:
	// find and set our local ip address
	boolean	setLocalIpAddr();
#ifdef ILON_100_ROUTER_DEMO
	void	clearLocalIpAddr();
	friend	int rtrIpMasterPPPStartupTask();
#endif
	friend	int	LtIpMasterTask( int a1, ... );
	friend	int	LtIpWritePersistTask( int a1, ... );
	void	writePersistTask();	// persistence writer
	friend	int LtIpMasterTimeout( int a1, ... );
	friend	int LtIpRequestTimeout( int a1, ... );

	friend	int LtIpCheckStuckTask( int a1, ... );
	void	checkStuck();

	friend	int LtIpBwAggTask( int a1, ... );
	void	doBwAgg();

	void	startWorkerTask( int nWorkBits, boolean bIntLvl=false );
	void	workerTask();
	void	deleteWorkerTask();
	void	workerTimeout();
	void	startTimedWork( int nWorkBits, int nMsTimer );
	void	cancelTimedWork();
	void	doRequestTimeout();

	boolean	sendNewPacket( byte* pData, int nLength, LtIpRequest* pReqPkt = NULL );
	boolean	sendNewPacketTo( byte* pData, int nLength,
							ULONG ipAddr, word port, LtIpRequest* pReqPkt = NULL );

	// read and write our flash resident persistence
	boolean	getPersistV1(MasterDataV1& masV1, byte* pData, int& size);
	boolean	getPersistV2(MasterDataV2& masV2, byte* pData, int& size);
	boolean	getPersistV3(MasterDataV3& masV3, byte* pData, int& size);
	boolean	doReadPersist();
	boolean	doWritePersist();
	// read and write our NV ram resident persistence
	void	doReadNvRam( boolean bSetServers = false );
	void	doWriteNvRam();

	int		buildDevRegister( boolean bDevResponse );
	void	doSendDevRegister( boolean bDevResponse,
								int ipAddr = 0, word ipPort = 0 );
	BOOL	doSendChanRouting( int ipAddr = 0, word ipPort = 0,
								LtIpRequest* pReqPkt = 0,
								boolean bAlways=FALSE,
								boolean bIgnoreTime = FALSE );
	void	doSendAck( int code, word reqId, int ipAddr = 0, word ipPort = 0 );
	void	doRequestInfo();
	// we received a response to a packet
	// So clear the request for the packet and set the count to zero
	// so retransmits occur often
	void	cancelRequest( int nReqBits )
	{
		clearReqtMask( nReqBits );
		m_nReqtCount = 0;
		if ( m_nReqtMask )
		{	startRequestInfo( 0, true );
		}
		else
			m_ConnectLedObSubject.Notify();	// update Connect LED

	}
	void	startRequestInfo( int nReqBits, boolean bQuick = false );

	void	doRFCin();
	boolean	setNewServers( LtIpDevRegister& dvr );
	void	doRFCRequest( int nPktType, LtPktInfo* pPkt );
	int		doRfcBuildStats( byte* buf, int reason );
	boolean	makeNewPacket( byte* pOld, int nBytes, byte** ppNew );
	void	cleanPackets();
	void	setCnfgServerType(int type);

	void	removeAllClients();
	void	removeClient( int idx );
    void    removeMcastClient();        // remobe MULTICAST IP client
	boolean	getDuplicateChanMember( LtIpChanMembers& chm, ULONG& ipa, word& port, boolean& sharedIp );
	void	setNewMembers( LtIpChanMembers& chm );
	void	updateClientRouting( int i, byte* pPktChanRouting );
    void	createMcastClientRouting();
	int		findChannelMember( ULONG ipAddress, USHORT ipPort, LtIpAddPortDate apd[MAX_MEMBERS]=NULL );
	void	orSubnetsAndGroups( LtRoutingMap& src, LtRoutingMap& dst );
	boolean	sweepLocalClients();
	ULONG	getExternalIpAddr();
	boolean sendSavedChanMemberPkt(byte* pSavedPkt, int ipSrcAddr, int ipSrcPort, LtIpRequest *pReq);
	boolean sendSavedChanRoutingPkt(byte* pSavedPkt, int ipSrcAddr, int ipSrcPort, LtIpRequest *pReq);
	boolean chrPktHasLocalUid(LtIpChanRouting& chr);
	void	unknownPortDiagnostic();
	void	generatePropertyChangeHostEvent();

	// allocate a packet from the master
	LtPktInfo*	allocPacket()
	{
		return m_pAlloc->allocPacket();
	}
	LtPktInfo*	allocMsgRef()
	{
		return (LtPktInfo*)m_pAlloc->allocMsgRef();
	}
	// a client tells the master about an RFC message that arrived and needs
	// processing.
	void RfcFromClient( LtPktInfo* pPkt );

	// aggregation and bandwidth limiting client services
	int				m_tidBwAggTask;				// task id for bw agg task
	boolean			m_bAggregate;
	ULONG			m_nAggregateMs;
	boolean			m_bBWLimit;
	ULONG			m_nBWLimitKBPerSec;			// in KB/Sec
	ULONG			m_nBWBytesPerSlot;			// slot is 1/5 sec
	ULONG			m_nBWBytesThisSlot;			// bytes sent this slot time
	int				m_nBWLastClient;			// last client checked
	ULONG				m_nBWTimerLastTicks;		// last timer ticks we had
	enum {
		BWLIMIT_SLOTS		= 30,
		BWLIMIT_SLOTMS		= (1000/BWLIMIT_SLOTS),
		STALETIME_DEFAULT	= 2000				// default to two seconds stale time
												// before packet leaves the box
	};
	ULONG			m_nTOSbits;					// type of service bits
	boolean			m_bUseTOSbits;				// whether to use them

public:
	void	setTestReordering( boolean bReorder, boolean bRandom,
								boolean bDelete, int nEvery, int nBy )
	{
		m_bTReorder			= bReorder;
		m_bTReorderRandom	= bRandom;
		m_bTReorderDelete	= bDelete;
		m_nTReorderEvery		= nEvery;
		m_nTReorderBy		= nBy;
	}

protected:
	boolean		m_bTReorder;
	boolean		m_bTReorderRandom;
	boolean		m_bTReorderDelete;
	int			m_nTReorderEvery;
	int			m_nTReorderBy;
    boolean     m_bPersistenceRead;

private:
#if defined(WIN32)
    // Let IP-852 config server set SNTP servers
    void iLonSetTimeServersFromIP852(ULONG newIpAddr1, USHORT newIpPort1, 
								     ULONG newIpAddr2, USHORT newIpPort2);
#endif

};

// Persistent data structures

// change version for MasterData
#define MASTER_DATA_VERSION_1 7
#define MASTER_DATA_VERSION_2 8
#define MASTER_DATA_VERSION_3 9
#define MAGIC_NUMBER	0xbabeface

// change version for MasterNvRam change
#define NVRAM_DATA_VERSION_1 7
#define NVRAM_KEY "LtMasterKey"

// definition of persistent data fields for different versions
#define MASTER_DATA_V1_FIELDS											\
	int		magic;				/* magic number */						\
	int		version;			/* version of the file */			   	\
	int		length;				/* length of entire persist file */	   	\
	ULONG	dateTime;			/* dateTime saved */				   	\
	ULONG	nSession;			/* session number */				   	\
	ULONG	ipAddrLocal;		/* local IP address */				   	\
	ULONG	ipAddrCfgServer;	/* config server address */			   	\
	ULONG	ipAddrNtpServer;	/* time server address */			   	\
	ULONG	ipAddrNtpServer2;	/* time server address */			 	\
	word	ipPortLocal;		/* local ip port */					 	\
	word	ipPortCfgServer;	/* config server port */				\
	word	ipPortNtpServer;	/* time server port */					\
	word	ipPortNtpServer2;	/* time server port */					\
	boolean	bHaveDevRegister;	/* have a device register packet */		\
	boolean	bHaveMembers;		/* have a membership packet */			\
	int		nChanRouting;		/* number of channel routing packets */	\
	boolean	bBwLimit;			/* limit bandwidth */					\
	int		nBwLimitKbPerSec;	/* KB per second limit */				\
	boolean	bAggregate;			/* aggregate packets */					\
	int		nAggregateMs;		/* ms for aggregation */				\
	word	nChannelTimeout;	/* ms 0 - 1500 */						\
	boolean	bCheckStale;		/* check for stale packets */			\
	boolean	bReorderPackets;	/* reorder inbound packets */			\
	ULONG	nEscrowTimeMs;		/* reorder escrow timer in ms */		\
	boolean	bUseTosBits;		/* use TOS bits on link */				\
	ULONG	nTOSbits;			/* tos bits to use */					\
	boolean	bAuthenticate;		/* authenticate packets */				\
	byte	aAuthenticSecret[LtIpMaster::AUTHENTIC_SECRET_SIZE ];		\
	char	cName[LtIpMaster::MAXNAMELEN];	/* name of the device - set/read by CS */

// persistant data header
struct MasterDataV1
{
	MASTER_DATA_V1_FIELDS
};

#define MASTER_DATA_V2_FIELDS											\
	boolean bHasSharedIpAddrs;	/* some member IP addrs are the same */	\
	int		runningEchProtocolVer; /* backward compatible mode */		\
	ULONG	natIpAddr;			/* NAT IP addr */						\
	boolean bStrictEia852;		/* eforce strict EIA-852 */

struct MasterDataV2
{
	MASTER_DATA_V1_FIELDS
	MASTER_DATA_V2_FIELDS
};

#define MASTER_DATA_V3_FIELDS															\
	word	lastKnownCsPort;	/* last known/configured config server port */		\
	char	lastKnownCsHost[64];	/* last known/configured config server address */

struct MasterDataV3
{
	MASTER_DATA_V1_FIELDS
	MASTER_DATA_V2_FIELDS
	MASTER_DATA_V3_FIELDS
};

struct MasterNvRam
{
	int		magic;				// magic number
	int		version;			// version of the file
	int		length;				// length of this data
	ULONG	dateTime;			// dateTime saved
	ULONG	nSession;			// session number
	ULONG	ipAddrLocal;		// local IP address
	ULONG	ipAddrCfgServer;	// config server address
	ULONG	ipAddrNtpServer;	// time server address
	ULONG	ipAddrNtpServer2;	// time server address
	word	ipPortLocal;		// local ip port
	word	ipPortCfgServer;	// config server port
	word	ipPortNtpServer;	// time server port
	word	ipPortNtpServer2;	// time server port
};

#if defined(WIN32)
// On Windows, we maintain a list of SNTP severs for each active IP852 master, and use the
// last one that has been updated.

class SntpServerList
{
public:
    SntpServerList(int ipMasterId, ULONG ipAddr1, USHORT ipPort1, 
                  ULONG ipAddr2, USHORT ipPort2);
    ~SntpServerList();

    static void update(int ipMasterId, ULONG ipAddr1, USHORT ipPort1, 
                       ULONG ipAddr2, USHORT ipPort2);
    static boolean remove(int ipMasterId, boolean update = true);

    static boolean getCurrentSntpConfig(ULONG &ipAddr1, USHORT &ipPort1, 
                                        ULONG &ipAddr2, USHORT &ipPort2);

    static SntpServerList *find(int ipMasterId);
    static void shutdown();
private:
    SntpServerList *m_pNext;
    int             m_ipMasterId;
    ULONG           m_ipAddr1;
    USHORT          m_ipPort1;
    ULONG           m_ipAddr2;
    USHORT          m_ipPort2;

    static boolean lock();
    static void unlock();
    static void updateSntp();

    static SEM_ID     m_semSntpServerList; 
    static class SntpServerList *m_pSntpServerList;
};
#endif

#endif // !defined(AFX_LTIPMASTER_H__68FEF4F1_C68E_11D2_A837_00104B9F34CA__INCLUDED_)
