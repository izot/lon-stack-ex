/***************************************************************
 *  Filename: LtLinkBase.cpp
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
 *  Description:  Stubs for LtLinkBase class
 *
 *	DJ Duffy Nov 1998
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Shared/LtLinkBase.cpp#4 $
//
/*
 * $Log: /Dev/Shared/LtLinkBase.cpp $
 * 
 * 45    8/01/07 5:50p Fremont
 * EPR 45753 - clearing the network statistics of an internal device
 * cleared the console linkstats display, which was intended to show the
 * external packet statistics. In standalone mode on the iLON, this
 * happened automatically on any internal device commission. Solution:
 * keep a shadow shadow copy of the statistics that were actually shared,
 * increment them in the driver, and read/clear the shadow copies when
 * accessing the internal device.
 * 
 * 44    5/21/07 2:23p Bobw
 * 
 * 43    3/21/07 3:52p Bobw
 * Add conditional compilation for FTXL project.  
 * 
 * 42    3/16/05 5:04p Bobw
 * EPR 36462
 * Set "m_pktFlags" to flags parameter in LtLinkBase::queueTransmit.
 * 
 * 41    1/10/05 1:49p Fremont
 * Reorgainize for platform-specific functions, add debug code
 * 
 * 40    4/28/04 6:49p Bobw
 * EPR 32593
 * Support VNI based protocol analyzer.  Pass bad packets up, and time
 * stamp them at first oportunity.  
 * 
 * 39    6/14/02 5:59p Fremont
 * remove warnings
 * 
 * 38    4/06/02 3:28p Vprashant
 * dont start timer incase of i.LON platform
 * 
 * 37    2/06/02 2:09p Fremont
 * change "service pin state" message to "service LED state"
 * 
 * 36    11/07/01 3:37p Fremont
 * Use custom values for receive task priority and stack size.
 * Call receive task wakeup func in stopReceiveTask()
 * 
 * 35    11/07/01 10:32a Fremont
 * rename m_flags to m_pktFlags to avoid conflict with stupid Tornado
 * incude file (mbuf.h)
 * 
 * 34    11/06/01 2:38p Fremont
 * Add link receive task name override
 * 
 * 33    11/06/01 9:25a Glen
 * Need control of zero crossing synchronization and attenuation for
 * LonTalk Validator.  Added these flags to sendPacket().  This propagated
 * to lots of places.  Also added control options to LtMsgOut.
 * 
 * 32    10/24/01 12:04p Fremont
 * change prototype
 * 
 * 31    10/15/01 1:47p Fremont
 * shorten task names
 * 
 * 30    9/26/01 2:51p Fremont
 * Add reminder for hang in stopReceiveTask()
 * 
 * 29    11/01/00 1:34p Bobw
 * Clear the m_bExitReceiveTask flag when starting up a link.  Otherwise,
 * if a link is started, stopped, and restarted, the recieve task will
 * just quit on the restart.
 * 
 * 28    10/10/00 2:16p Darrelld
 * Fix thread rundown
 * 
 * 27    8/11/00 8:08a Glen
 * 1.50.13
 * 
 * 25    11/18/99 10:57a Darrelld
 * Task priority macros
 * 
 * 24    8/09/99 12:08p Darrelld
 * Fix task priorities
 * 
 * 23    7/30/99 8:59a Darrelld
 * Cleanup and streamline
 * 
 * 21    7/02/99 5:32p Darrelld
 * Make lock inversion safe
 * 
 * 20    7/02/99 8:45a Darrelld
 * Target specific files
 * 
 * 19    6/18/99 2:50p Darrelld
 * Stop Receive task in destructor
 * 
 * 18    4/23/99 5:22p Darrelld
 * Router testing
 * 
 * 17    3/26/99 10:21a Glen
 * Print Service Led State
 * 
 * 16    3/15/99 10:39a Darrelld
 * Intelligent control of dumping packets
 * 
 * 15    3/11/99 5:01p Darrelld
 * intermediate checkin
 * 
 * 14    3/01/99 2:20p Glen
 * Queue receive now fails if called during reset (to mimic LONC driver)
 * 
 * 13    2/25/99 11:12a Glen
 * Fix range check
 * 
 * 12    2/22/99 9:36a Darrelld
 * Remove Windows dependencies
 * 
 * 11    2/16/99 11:09a Glen
 * Add LtVxWorks.h
 * 
 * 10    2/12/99 9:16a Darrelld
 * Fix counter check bug
 * 
 * 9     1/28/99 11:03a Darrelld
 * Support getUniqueId for testing
 * 
 * 8     1/14/99 2:06p Glen
 * Integrate port client (step1)
 * 
 * 7     1/12/99 1:11p Darrelld
 * Add status return to reportTransciverRegister
 * 
 * 6     12/16/98 1:22p Darrelld
 * Update LtLink interface
 * 
 * 5     12/15/98 4:33p Darrelld
 * Reorganize base classes to make LtLink pure and virtual as the driven
 * snow.
 * 
 * 3     12/15/98 11:45a Darrelld
 * check in to accomplish move
 * 
 * 2     12/11/98 2:29p Darrelld
 * Add isOpen() to LtLinkBase members
 * 
 * 1     11/30/98 4:57p Darrelld
 * LTIP Test application
 * 
 */
// no longer windows dependent

#include "LtRouter.h"
#include <vxlTarget.h>
#include <stdio.h>
#include <assert.h>
#include <LtLinkBase.h>
#if FEATURE_INCLUDED(IP852)
#include <LtIpPackets.h>
#endif

//
// Constructor
//
LtLinkBase::LtLinkBase()
{
	m_pNet						= NULL;
	m_bActive					= false;
	m_bLinkOpen					= false;
	m_bFlushMode				= false;
	m_bProtoAnalyzerMode		= false;
	m_bLoopback					= false;
	m_nBacklog					= 0;
	m_nWindowSize				= 16;
	m_nReceiveQueueDepth		= 4;
	m_nTransmitQueueDepth		= 4;
	m_nReceiveTaskPriority		= 0;
	m_nServicePinState			= SERVICE_OFF;
	m_nTransmitSlot				= 0;
	m_bDumpEnable				= false;
	m_bDumpHeadersOnly			= false;
	m_bExitReceiveTask			= false;

	// set the queues as "heads"
	m_qFreePkts.init( true );
	m_qTransmit.init( true );
	m_qReceive.init( true );
	m_qReceiveP.init( true );

	m_lock = semMCreate( SEM_Q_PRIORITY | SEM_INVERSION_SAFE );
	m_tidReceive = ERROR;

	clearStatistics();

}

//
// Destructor
//
LtLinkBase::~LtLinkBase()
{
	if ( isOpen() )
	{	close();
	}
	stopReceiveTask();
	drainFreeQueue( m_qFreePkts );
	semDelete( m_lock );

}




LtSts LtLinkBase::registerNetwork(LtNetwork& net)
{
	LtSts sts = LTSTS_OK;

	if ( m_bActive )
	{	sts = LTSTS_INVALIDSTATE;
	}
	else
	{
		if ( m_pNet != &net )
		{
			m_pNet = &net;
			m_pNet->registerLink( *this );
		}
	}
	return sts;
}


// Enumerate Interfaces supported by this link
// idx values start with zero. For each valid interface the name
// is returned in the buffer and true is returned.
// False is returned if there is no interface with that index.

static const char*	szIface = "LON1";

boolean LtLinkBase::enumInterfaces( int idx, LPSTR pNameRtn, int nMaxSize )
{
	boolean		bOk = false;
	if ( idx == 0 && nMaxSize >= (int)sizeof(szIface) )
	{
		strcpy( pNameRtn, szIface );
		bOk = true;
	}

	return bOk;
}

// Open the interface for use
// Call with a name returned by enumInterfaces
LtSts LtLinkBase::open( const char* pName )
{
	LtSts	sts = LTSTS_ERROR;

	if ( !m_bLinkOpen && 0 != strcmp( pName, szIface ) )
	{	sts = LTSTS_OK;
		m_bLinkOpen = true;
	}
	return sts;
}

boolean		LtLinkBase::isOpen()
{	return m_bLinkOpen;
}


// Close the currently open interface
LtSts LtLinkBase::close()
{
	m_bLinkOpen = false;
	drainFreeQueue( m_qFreePkts );
	return LTSTS_OK;
}

// This function resets the driver and the comm port.  Any buffered
// outgoing messages or messages in progress are not sent.  Any incoming
// messages in process are lost.   All statistics are reset.  Following this
// reset function call, the network layer will invoke the
// "setCommParams" method (defined below) to set the communication
// parameters. 
// All send and receive requests are completed immedately.
void LtLinkBase::reset()
{
	lock();
	m_bActive = false;
	drainReceiveQueue( m_qReceiveP );
	drainReceiveQueue( m_qReceive );
	drainTransmitQueue( m_qTransmit );
	clearStatistics();
	unlock();
}



// This method allows the network to set the priority of the task which
// invokes the packetReceived callback.
LtSts LtLinkBase::setReceivePriority(int priority)
{	
	LtSts sts = LTSTS_INVALIDSTATE;
	if ( !m_bActive )
	{
		m_nReceiveTaskPriority = priority;
	}
	return sts;
}

// Get the max sizes of send and receive queues in the driver
void LtLinkBase::getMaxQueueDepth( int* pnReceiveQueueDepth, int* pnTransmitQueueDepth )
{
	*pnReceiveQueueDepth = LTLINK_MAXQUEUEDEPTH;
	*pnTransmitQueueDepth = LTLINK_MAXQUEUEDEPTH;
}

// Set the sizes of the send and receive queues in the driver
// May cause a "reset" to occur.
// <<< What's this supposed to return??>>>
LtSts LtLinkBase::setQueueDepths( int nReceiveQueueDepth, int nTransmitQueueDepth )
{
	m_nReceiveQueueDepth = nReceiveQueueDepth;
	m_nTransmitQueueDepth = nTransmitQueueDepth;
	return LTSTS_OK;
}

// Send a packet with completion indication.
// Packet and the data may have been allocated in any way.
// nPrioritySlot is not used at present and must be -1 for this call.
// Packet is sent priority/non-priority based on bPriority 
// (not based on LPDU priority bit!).  Packets are sent in the order
// of this call. On completion of the transmit the
// driver invokes "packetSent" method of "LtNetwork".
// The method synchronously returns either LTSTS_PENDING or LTSTS_QUEUEFULL.
// Two bytes immediately before pData are available for driver use.
LtSts LtLinkBase::sendPacket(void* referenceId,
				int nPrioritySlot,
				byte flags,
				byte* pData,
				int nDataLength,
				boolean bPriority)
{
	if ( m_pNet )
	{
		m_linkStats.m_nTransmittedPackets++;
		m_pNet->packetComplete( referenceId, LTSTS_OK );
	}
	return LTSTS_OK;
}

// This function is used to retrieve the standard transceiver ID as 
// detected from an SMX transceiver.  If the driver does not support this
// feature or there is no SMX transceiver attached, then -1 is returned.
// It is the responsibility of the LtNetwork code to read this value and
// to set the comm parameters to the corresponding values (see
// "setCommParams").
int LtLinkBase::getStandardTransceiverId()
{

	return -1;
}

// This function is used to retrieve the LonTalk Unique ID from the network
// interface hardware.  If the network interface hardware does support this
// capability, false is returned, otherwise true.
boolean LtLinkBase::getUniqueId(LtUniqueId& uniqueId)
{
	int i;
	byte	data[LT_UNIQUE_ID_LENGTH];

	for ( i=0; i< LT_UNIQUE_ID_LENGTH; i++ )
	{
		data[i] = 'A'+i -1;
	}
	uniqueId.set( &data[0] );
	return true;
}

// Request notification when all packets have been sent and there is
// no input activity.  Use "on" to turn this mode on or off.  Default is off.
void LtLinkBase::flush(boolean on)
{
	m_bFlushMode = on;
	if ( on && m_pNet )
	{
		m_pNet->flushCompleted();
	}
}

// Like flush above except that any pending output activity is terminated
// immediately.
void LtLinkBase::terminate()
{
	if ( m_pNet )
	{
		m_pNet->terminateCompleted();
	}

}

// Sets the 16 bytes of communication parameters.  Note that the special
// purpose mode transceiver parameters (7 bytes) must be loaded in
// reverse, from value 7 down to value 1.  This function returns once
// all initialization is complete, including special purpose mode
// configuration.  If special purpose mode configuration fails, this
// routine will give up and return within a few seconds.
LtSts LtLinkBase::setCommParams(const LtCommParams& commParams)
{
	m_bActive = true;
	return LTSTS_OK;

}

// Gets the 16 bytes of communication parameters.  Note that in special
// purpose mode, the 7 transceiver configuration bytes should be reported
// as 0s since they are not readable.
LtSts LtLinkBase::getCommParams( LtCommParams& commParams)
{
	LtCommParams	ltcpNone;

	memset( &ltcpNone, 0, sizeof(LtCommParams) );
	commParams = ltcpNone;
	return LTSTS_OK;
}

// Gets the Nth byte of special purpose mode transceiver status register data
// (7 registers numbered 1-7).  Value is returned by the
// "reportTransceiverRegister" function.
LtSts LtLinkBase::getTransceiverRegister(int n)
{
	LtSts sts = LTSTS_INVALIDSTATE;
	if ( m_pNet )
	{
		m_pNet->reportTransceiverRegister( n, 0, LTSTS_OK );
		sts = LTSTS_OK;
	}
	return sts;
}

// Reports statistics from the driver/LON-C.
// These statistics are 16 bits and cap at 0xFFFF.
void LtLinkBase::getStatistics(LtLinkStats& stats)
{
	stats = m_linkStats;
}
void LtLinkBase::getStatistics(LtLinkStats *&pStats)
{
	pStats = &m_linkStats;
}
// 0 the statistics.
void LtLinkBase::clearStatistics()
{
	m_linkStats.clearStats();
}

// Determines the state of the service pin.  "state" values are
// defined as per the LON-C specification.
void LtLinkBase::setServicePinState(LtServicePinState state)
{
	const char* p = null;
	switch (state)
	{
	case SERVICE_ON:
		p = "ON";
		break;
	case SERVICE_OFF:
		p = "OFF";
		break;
	case SERVICE_BLINKING:
		p = "BLINKING";
		break;
	default:
		assert(0);
		break;
	}
#if !PRODUCT_IS(FTXL)
	printf("Setting service LED state to %s\n", p);
#endif
	m_nServicePinState = state;
}

// Controls whether physical/link layer function in Protocol Analyzer
// mode.  This mode differs from normal mode in the following respects:
// 1. In direct mode, if a transition is detected but RX flag is not 
// raised within 32 bit times, report a timeout error.
// 2. Following an invalid packet, ignore transitions during beta1 (arriving
// prior to the first beta2 slot) as these are assumed to be continuations
// of the error.
// 3. In normal mode, packet errors are discarded by the driver or LON-C.
// In Protocol Analyzer mode, error conditions are reported via
// packetReceived.
// Packets are queued for reception as with normal receive using
// queueReceive.
void LtLinkBase::setProtocolAnalyzerMode(boolean on)
{
	m_bProtoAnalyzerMode = on;

}


boolean LtLinkBase::getProtocolAnalyzerMode()
{
	return m_bProtoAnalyzerMode;
}

// Puts the driver into a loopback mode where every transmitted packet
// is treated as if it were received on the network.  Default is off.
// Loopback is achieved by putting the LON-C hardware in loopback mode.
void LtLinkBase::setLoopbackMode(boolean on)
{
	m_bLoopback = on;
}

boolean LtLinkBase::getLoopbackMode()
{
	return m_bLoopback;
}

// Performs a self test of the comm port and returns a result.  Return
// value is 0 for passed, -1 for failure.  May take a few seconds to 
// return in failure case.  Self test will disrupt normal packet activity
// but must not affect the network (i.e., cause spurious transmissions).
LtSts LtLinkBase::selfTest()
{
	return LTSTS_OK;
}

// This function reports the results of any power-on self test in the
// LON-C hardware.  Return value is same as for "selfTest".
LtSts LtLinkBase::reportPowerSelfTest()
{
	return LTSTS_OK;
}

// For testing - allows the current backlog to be set to a certain value.
// This could be called dynamically at any time during operation of the
// LON-C.
void LtLinkBase::setCurrentBacklog(int backlog)
{
	m_nBacklog = backlog;
}
// For debugging/analysis - allows the current backlog value to be reported.
int LtLinkBase::getCurrentBacklog()
{
	return m_nBacklog;
}

// For testing (or possible future enhancement) - allow the randomizing
// window size to be changed.  Default is 16.  Maximum value is 64.
// The set function would be called once, immediately following a
// "reset" of the LON-C only.
void LtLinkBase::setWindowSize(int windowSize)
{
	m_nWindowSize = windowSize;
}
int LtLinkBase::getWindowSize()
{
	return m_nWindowSize;
}

// For testing - forces a packets to use the specified slot for
// "transmitSlot" indicates a desired beta2 slot to send
// the packet on.  -1 would indicate automatic selection based on the
// MAC layer rules.  A value of 0 to N-1 would indicate to transmit
// on a specific slot where 0 is the first slot following beta1 (i.e.,
// the first priority slot if there is priority, otherwise the first
// randomizing slot).  The value of N should be at a minimum 127 but if
// it could be made as large as 1063 very easily, this could be of some
// value.  This capability is intended primarily for debugging.
void LtLinkBase::setTransmitSlot(int transmitSlot)
{
	m_nTransmitSlot = transmitSlot;
}
int LtLinkBase::getTransmitSlot()
{
	return m_nTransmitSlot;
}


//
// LtLinkReceive
//
// friendly task [ aka thread ] to do receives
//
int LtLinkReceive( int a1, ... )
{
	LtLinkBase*	pLink = (LtLinkBase*) a1;
	pLink->receiveTask();
	return 0;
}



void	LtLinkBase::startReceiveTask()
{
	if ( m_tidReceive == ERROR )
	{
        m_bExitReceiveTask = false;
		// create the receive task
		// it will block on the event until it is awakened
		// priority of 111 is one less than 110 which is where the engine runs
		m_tidReceive = taskSpawn( getRcvTaskName(), getRcvTaskPriority(), 0,
								getRcvTaskStackSize(), LtLinkReceive, (int)this,
								0,0,0,0,0,0,0,0,0);
		assert( m_tidReceive != ERROR );
	}
}

void	LtLinkBase::stopReceiveTask()
{
	if ( m_tidReceive != ERROR )
	{
		m_bExitReceiveTask = true;
		
		// This sometimes hangs in PC simulation environment!
		// Haven't determined if it's the IP side or the LON side, but IP is suspect.
		// If it continues, implement wakeReceiveTask() where necessary.

		wakeReceiveTask();	// Wake for shutdown sync

		while ( m_tidReceive != ERROR )
		{
			taskDelay( msToTicks(50) );
		}
		//taskDelete( m_tidReceive );
		//m_tidReceive = ERROR;
	}
}

//
// getLLPkt
//
// get a packet descriptor
//
LLPktQue* LtLinkBase::getLLPkt()
{
	LLPktQue*	pPkt;
	LtQue*		pItem;

	if ( m_qFreePkts.removeTail( &pItem ) )
	{
		pPkt = (LLPktQue*)pItem;
	}
	else
	{	pPkt = new LLPktQue();
	}
	return pPkt;
}

//
// freeLLPkt
//
// free a packet descriptor
//
void	LtLinkBase::freeLLPkt( LLPktQue* pLLPkt )
{
	if ( m_qFreePkts.getCount() > 20 )
	{	delete pLLPkt;
	}
	else
	{	m_qFreePkts.insertHead( pLLPkt );
	}
}

//
//	drainFreeQueue
//
//	Empty the free queue
//
void LtLinkBase::drainFreeQueue( LtQue& qReceive )
{
	LLPktQue*	pPkt;
	LtQue*		pItem;

	while( qReceive.removeHead( &pItem ) )
	{
		pPkt = (LLPktQue*)pItem;
		delete pPkt;
		pPkt = NULL;
		pItem = NULL;
	}

}

//
//	drainReceiveQueue
//
//	return packets to the client
//
void LtLinkBase::drainReceiveQueue( LtQue& qReceive )
{
	LLPktQue*	pPkt;
	LtQue*		pItem;
	int			nSlot = 0;

	while( !qReceive.isEmpty() )
	{
		qReceive.removeHead( &pItem );
		pPkt = (LLPktQue*)pItem;
		if ( m_pNet )
		{
			// if the user doesn't know better than to try to requeue, then
			// dead-lock.
			//unlock();
			m_pNet->packetReceived( pPkt->m_refId, 0, pPkt->m_bPriority, nSlot, 
									false, 0, LTSTS_RESET );
			//lock();
		}
		freeLLPkt( pPkt );
		pPkt = NULL;
		pItem = NULL;
	}

}

//
//	drainTransmitQueue
//
//	return un-sent packets to the client
//
void LtLinkBase::drainTransmitQueue( LtQue& qTransmit )
{
	LLPktQue*	pPkt;
	LtQue*		pItem;

	while( !qTransmit.isEmpty() )
	{
		qTransmit.removeHead( &pItem );
		pPkt = (LLPktQue*)pItem;
		if ( m_pNet )
		{
			//unlock();
			m_pNet->packetComplete( pPkt->m_refId, LTSTS_RESET );
			//lock();
		}
		freeLLPkt( pPkt );
		pPkt = NULL;
		pItem = NULL;
	}

}


//
// queueTransmit
//
// Queue a transmit if we get a buffer not available indication
//
void	LtLinkBase::queueTransmit( void* refId, byte flags, byte* pData, int nDataLength )
{
	LLPktQue*	pPkt = getLLPkt();
	pPkt->m_pktFlags = flags;
	pPkt->m_refId = refId;
	pPkt->m_pData = pData;
	pPkt->m_nDataLength = nDataLength;

	m_qTransmit.insertTail( pPkt );
	// If implemented in derived class, this will start a timer to kick off a retransmit
	startDelayedRetransmit();
}


//
// queueReceive
//
// Queue a receive packet buffer for the receive task to fill up.
//
LtSts LtLinkBase::queueReceive( void* referenceId,
						boolean bPriority,
						byte flags,
						byte* pData,
						int nMaxLength)
{
	LtSts	sts = LTSTS_INVALIDSTATE;
	LLPktQue*	pPkt;

	lock();
	if ( m_nReceiveQueueDepth <= (bPriority ? m_qReceiveP.getCount() : m_qReceive.getCount()) )
	{
		sts = LTSTS_QUEUEFULL;
	}
	else if ( m_bActive && isOpen() )
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

// control of dumping packets
static boolean		gbDumpEnable = false;
static boolean		gbDumpHeadersOnly = false;

boolean	LtLinkBase::dumpEnable( boolean bEnable )
{
	boolean bOld = m_bDumpEnable;
	m_bDumpEnable = bEnable;
	return bOld;
}

boolean	LtLinkBase::dumpEnableGlobal( boolean bEnable )
{
	boolean bOld = gbDumpEnable;
	gbDumpEnable = bEnable;
	return bOld;
}

boolean	LtLinkBase::dumpHeadersOnly( boolean bEnable )
{
	boolean bOld = m_bDumpHeadersOnly;
	m_bDumpHeadersOnly = bEnable;
	return bOld;
}

boolean	LtLinkBase::dumpHeadersOnlyGlobal( boolean bEnable )
{
	boolean bOld = gbDumpHeadersOnly;
	gbDumpHeadersOnly = bEnable;
	return bOld;
}

//
// dumpLtPacket
//
// General event logger for packets
// Tag states the class and function
// Dump packet without RFC header
//
void	LtLinkBase::dumpLtPacket( LPCSTR tag, byte* pData, int nLen )
{
#if FEATURE_INCLUDED(IP852)
	LtIpPktHeader	Pkt;
	if ( m_bDumpEnable || gbDumpEnable )
	{
		vxlReportEvent("LtLinkBase::dumpPacket - %s\n", tag);
		Pkt.dumpLtPacket( tag, pData, nLen );
	}
#endif
}


//
// dumpPacket
//
// General event logger for packets
// Tag states the class and function
//
void	LtLinkBase::dumpPacket( LPCSTR tag, byte* pData, int nLen )
{
#if FEATURE_INCLUDED(IP852)
	if ( m_bDumpEnable || gbDumpEnable )
	{
		LtIpPktHeader	Pkt;
		vxlReportEvent("LtLinkBase::dumpPacket - %s\n", tag);
		Pkt.dumpPacket( tag, pData, nLen, m_bDumpHeadersOnly || gbDumpHeadersOnly );
	}
#endif
}

// some stubs
// These can't be pure virtual in this class, but must be implemented by
// derived classes.
//
void	LtLinkBase::receiveTask(void)
{
	assert(false);
}

// Derived classes should implement one or the other of these two
void	LtLinkBase::startDelayedRetransmit(void)
{
}
void	LtLinkBase::startImmediateRetransmit(void)
{
}

LtSts	LtLinkBase::tryTransmit( void* refId, byte flags, byte* pData, int nDataLength )
{
	assert( false );
	return LTSTS_INVALIDSTATE;
}

// end
