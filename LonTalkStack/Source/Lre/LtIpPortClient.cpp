/***************************************************************
 *  Filename: LtIpPortClient.cpp
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
 *  Description:  implementation for the LtIpPortClient class.
 *
 *	DJ Duffy Jan 1999
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Lre/LtIpPortClient.cpp#2 $
//
/*
 * $Log: /VNIstack/Dcx_Dev/Lre/LtIpPortClient.cpp $
 * 
 * 64    6/24/08 7:54a Glen
 * Support for EVNI running on the DCM
 * 1. Fix GCC problems such as requirement for virtual destructor and
 * prohibition of <class>::<method> syntax in class definitions
 * 2. Fix up case of include files since Linux is case sensitive
 * 3. Add LonTalk Enhanced Proxy to the stack
 * 4. Changed how certain optional features are controlled at compile time
 * 5. Platform specific stuff for the DC such as file system support
 * 
 * 1     12/05/07 11:15p Mwang
 * 
 * 63    11/20/07 6:34p Iphan
 * EPRS FIXED: 46644 - iLON crashes - Exception vector #4 in task
 * "SvcPinHdlr"
 * 
 * 
 * 62    9/18/07 4:20p Bobw
 * EPR 45227
 * Log LT_NET_BUF_TOO_SMALL in error log if message doesn't fit in network
 * buffers.
 * 
 * 61    8/01/07 5:41p Fremont
 * EPR 43171 - create mechanism so that LonScanner can see internal
 * packets on the iLON. Must set the flag 'sendInternalPacketsToPort'
 * 
 * 60    6/26/07 4:01p Bobw
 * 
 * 59    6/20/07 3:58p Bobw
 * Support service pin held function
 * 
 * 58    6/19/07 1:00p Bobw
 * OSAL upgrade
 * 
 * 57    3/29/07 12:58p Bobw
 * More FTXL development
 * 
 * 56    7/15/05 11:23a Bobw
 * EPR 35786
 * Change LonLinkWin to support the layer 2 mip to be pre-opened.  The
 * VniServer opens the device to determine its mip type (determinMipType).
 * Prior to this change, the VniServer opened the device, and if was a
 * layer 2 mip, closed it, so that lonLinkWin could re-open it.  Now
 * lonLinkWin just uses the handle opened previously (if set).  Note that
 * there are other changes to determinMipType necessary to complete this
 * EPR.
 * 
 * 55    5/18/05 5:53p Fremont
 * Set packet type if too short for CRC
 * 
 * 54    4/12/05 9:45a Bobw
 * EPR 36422
 * Support LDV attachment events for layer 2 mips as well as layer 5 mips.
 * 
 * 53    7/27/04 5:14p Fremont
 * #ifdef out some annoying tracing
 * 
 * 52    5/04/04 2:22p Bobw
 * 
 * 51    5/04/04 1:50p Bobw
 * EPR 33094
 * Don't trace CRC error if neuron detected that its a CRC error.  Also
 * changed printf to vxlReportEvent.
 * 
 * 50    4/28/04 6:49p Bobw
 * EPR 32593
 * Support VNI based protocol analyzer.  Pass bad packets up, and time
 * stamp them at first oportunity.  
 * 
 * 49    7/28/03 1:24p Fremont
 * remove unused var
 * 
 * 48    7/28/03 12:49p Bobw
 * EPR 27738
 * Remove code to set priority to highest priorty, simplify, and support
 * changing the comm parameters using a "last one wins" philosophy.
 * 
 * 47    7/22/03 3:30p Fremont
 * Init reset request flag (force reset on startup), return real sts in
 * getCommParams()
 * 
 * 46    6/23/03 11:09a Glen
 * Development related to supporting NES devices on top of the new Power
 * Line SLTA.  This includes making phase detection and bi-directional
 * query status work.  Also, changed stack such that if a transceiver ID
 * is unknown, the comm params are left unchanged.
 * 
 * 45    3/18/03 4:27p Fremont
 * fix warning
 * 
 * 44    6/14/02 5:59p Fremont
 * remove warnings
 * 
 * 43    6/14/02 4:14p Fremont
 * remove space from task name
 * 
 * 42    4/24/02 5:43p Fremont
 * don't print pkt length error message if sts == LTSTS_RESET
 * 
 * 41    4/15/02 4:15p Fremont
 * protect against too-short packets
 * 
 * 40    3/21/02 12:02p Fremont
 * remove obsolete LED management code
 * 
 * 39    3/13/02 12:15p Bobw
 * EPR 23990
 * Support long device driver names.
 * 
 * 38    2/26/02 5:38p Glen
 * Release packet unless it was queued.
 * 
 * 37    2/19/02 3:03p Vprashant
 * call packetComplete only in case of LTSTS_QUEUEFULL
 * 
 * 36    1/10/02 7:31p Vprashant
 * increased stack size
 * 
 * 35    12/21/01 1:56p Vprashant
 * doubled stack size
 * 
 * 34    12/11/01 1:49p Bobw
 * EPR 23157
 * Validate CRC even though the neuron has arleady done so, since the
 * neuron sometimes drops the first byte AFTER validating the CRC.
 * 
 * 33    11/06/01 9:25a Glen
 * Need control of zero crossing synchronization and attenuation for
 * LonTalk Validator.  Added these flags to sendPacket().  This propagated
 * to lots of places.  Also added control options to LtMsgOut.
 * 
 * 32    3/23/00 9:01a Darrelld
 * Fix behaviour when interface name is blank
 * 
 * 31    2/23/00 9:05a Darrelld
 * LNS 3.0 Merge
 * 
 * 30    1/24/00 2:11p Glen
 * Implement setting comm params and retrieve xcvr reg.
 * 
 * 28    10/28/99 11:12a Darrelld
 * Only set comm params if they changed.
 * 
 * 27    10/25/99 9:31a Darrelld
 * ILON AGGREGATION
 * 
 * 26    9/23/99 6:18a Darrelld
 * Manage service LED and priority on shutdown
 * 
 * 25    9/22/99 11:47a Darrelld
 * Service LED on start up wasn't right
 * 
 * 24    8/20/99 7:54a Darrelld
 * Add urgent tracing
 * 
 * 23    8/19/99 9:20a Darrelld
 * Service Led ON if no apps
 * 
 * 22    8/11/99 9:34a Darrelld
 * Restore corrupted packet after detection
 * 
 * 21    7/15/99 1:03p Glen
 * Implement LtStart procedures
 * 
 * 20    5/06/99 11:31a Glen
 * Fix null pointer bug
 * 
 * 19    4/06/99 11:22a Glen
 * Remove power line CRC workaround
 * 
 * 18    3/02/99 8:50a Glen
 * UNIX compatibility
 * 
 * 17    3/02/99 8:13a Glen
 * Remove printfs
 * 
 * 16    3/01/99 6:55p Darrelld
 * Tweak callback mechanism
 * 
 * 15    3/01/99 2:18p Glen
 * New allocator release callback 
 * 
 * 14    2/11/99 10:00a Darrelld
 * Count tracking was problematic - removed it.
 * 
 * 13    2/08/99 10:18a Glen
 * 
 * 12    2/08/99 10:07a Glen
 * Use getExternalRoute
 * 
 * 11    2/05/99 1:53p Darrelld
 * Add some asserts
 * 
 * 10    2/04/99 1:51p Glen
 * Fix buffer exhaustion problem
 * 
 * 9     2/01/99 11:16a Glen
 * Joint Test 3 integration
 * 
 * 8     1/22/99 7:16p Glen
 * Swap SEM_EMPTY and SEM_FULL
 * 
 * 7     1/22/99 2:23p Glen
 * 
 * 6     1/22/99 2:18p Glen
 * 
 * 5     1/22/99 11:01a Glen
 * 
 * 4     1/22/99 9:13a Glen
 * more routing algorithms
 * 
 * 3     1/15/99 1:46p Glen
 * Bring up of port client
 * 
 * 2     1/14/99 2:04p Glen
 * Integrate port client
 * 
 * 1     1/14/99 9:13a Glen
 * 
 * 2     1/13/99 9:55a Darrelld
 * Complete for testing
 * 
 * 1     1/12/99 5:04p Darrelld
 * 
 * 
 */
////////////////////////////////////////////////////////////////////////////////////////

#include <vxWorks.h>
#include <string.h>
#include <VxlTypes.h>
#include <LtRouter.h>
#include <LtMisc.h>
#include <LtPlatform.h>
#include <LtCUtil.h>
#include <vxlTarget.h>
#include <tickLib.h>
#include <LtIpPlatform.h>
#include <LtMip.h>

#define CP_NODE_PRI 7
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
/************************************************************************************
	Functional description

	The PortClient object "fronts" for several stacks that need to communicate
	with a LonTalk LtLink object.
	Packets sent from stacks are sent outside of this communication since the stacks
	are LreClients directly with the LreServer. This object also serves as an LreClient.
	It communicates with the LreServer and transmits packets to the LonTalk link,
	and receives packets from the LonTalk link and then sends them to the LreServer
	for routing. In it's role as an LreClient, this object acts as a repeater so that
	all packets are sent on the channel.

	Usage:
 
	This object requires a certain body of information to function:
	1 - Address of the LreServer. call setServer
	2 - Packet allocator. call setAllocator with the object.
	3 - Name to use to open the link. call setName.
	4 - The link object. Call registerLink. This function automatically performs
		a registerNetwork on the link.
	5 - start. Call this to start the link. Call stop to stop the link.

	Clients of this object call:
	registerClient( LtNetwork* )		to register as a client to obtain a link
	removeClient( LtNetwork* )			to remove

	Event clients are supported as well via
	registerClient( LtEventClient* )
	deregisterClient( LtEventClient* )
	notify
	These are not used by the object itself at this time.

***********************************************************************************/

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//  Class LtIpPortLink
//
// This class is the interface between a stack and the LtIpPortClient object.
//
// 
// Constructor
//
LtIpPortLink::LtIpPortLink()
{
	m_pNet				= NULL;
	m_pPortClient		= NULL;
	m_bProtAnalMode		= false;
	m_nPinState			= SERVICE_OFF;
	memset( &m_aRegsReq, 0, sizeof(m_aRegsReq) );
	memset( &m_commParams, 0, sizeof(LtCommParams) );
}

// getLink
//
// return the link pointer from the client object to do a pass through
//
LtLink*		LtIpPortLink::getLink()
{
	assert(m_pPortClient);
	return m_pPortClient->getLink();
}

// Special actions
//
// Special actions are forwarded to the port client object.
// The call throughs pass this port link object along as well since
// many of these operations require local state as well as global state.
//

void LtIpPortLink::reset()
{	m_pPortClient->reset( this );
}

LtSts LtIpPortLink::setCommParams(const LtCommParams& commParams)
{
	LtCommParams	cp;
	cp = commParams;
	return m_pPortClient->setCommParams(cp);
}


LtSts LtIpPortLink::getCommParams( LtCommParams& commParams)
{
	LtSts	sts;
	LtCommParams	cp;
	sts = m_pPortClient->getCommParams(this, cp );
	commParams = cp;
	return sts;
}


LtSts LtIpPortLink::getTransceiverRegister(int n)
{	return m_pPortClient->getTransceiverRegister( this, n );
}

	// Special handling
// On = 1, Blink = 2, Off = 3
void LtIpPortLink::setServicePinState(LtServicePinState state)
{	m_pPortClient->setServicePinState( this, state );
}

// Special handling
// On = 1, off = 2
void LtIpPortLink::setProtocolAnalyzerMode(boolean on)
{	m_pPortClient->setProtocolAnalyzerMode( this, on );
}
boolean LtIpPortLink::getProtocolAnalyzerMode()
{	return m_pPortClient->getProtocolAnalyzerMode( this );
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//  Class LtLpPortClient
//
// This class fronts as a Link client for stacks to perform link maintenance functions
// and fronts as a Client to move packets between the link and the engine.

//
// constructor
//
LtIpPortClient::LtIpPortClient( LtChannel *pChannel )
	: LtLreClientBase( LT_LONTALK_PORT, pChannel)
{
	m_pServer				= NULL;
	m_pLink					= NULL;
	m_name                  = NULL;
    setName("");
	m_bLreClientActive		= false;
	m_bQueueReceivesNeeded  = false;
	m_nReceiveQueueDepth	= 10;
	m_pAlloc				= NULL;
	m_sem = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
	assert( m_sem != NULL );
	m_semReset = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
	assert( m_semReset != NULL );
	m_bProtocolAnalyserMode	= false;
	m_nServicePinState		= SERVICE_ON;
    m_ipServiceState        = SERVICE_OFF;
	memset( &m_aRegsReq, 0, sizeof(m_aRegsReq) );
	memset( &m_commParams, 0, sizeof(LtCommParams) );
	m_bGotCommParams		= false;
	m_bResetting			= false;
	// On startup, this indicates we should always reset and set the comm params,
	// which, which is required to get the link initialized
	m_bResetRequested		= true;
}

//
// destructor
//
LtIpPortClient::~LtIpPortClient()
{
	if ( m_sem )
	{
		semDelete( m_sem );
	}
	if (m_semReset)
	{
		semDelete( m_semReset );
	}

	// blow the list of LtIpPortLink objects away cleanly
	m_vNets.removeAllElements( true );

    delete m_name;
}


//
// lock
//
// exclusive global access to the port client object.
// used sparingly.
//
void	LtIpPortClient::lock()
{
	STATUS	sts;

	// we may want an assert that doesn't go away
	// when debug is not defined
	assert( m_sem != NULL );
	sts = semTake( m_sem, WAIT_FOREVER );
	assert( sts == OK );
}

//
// unlock
//
// release global lock on the port client object
//
void	LtIpPortClient::unlock()
{
	STATUS	sts;

	// we may want an assert that doesn't go away
	// when debug is not defined
	assert( m_sem != NULL );
	sts = semGive( m_sem );
	assert( sts == OK );
}


//
// resetLock
//
// exclusive global access to reset/setCommParams
//
void	LtIpPortClient::resetLock()
{
	STATUS	sts;

	// we may want an assert that doesn't go away
	// when debug is not defined
	assert( m_semReset != NULL );
	sts = semTake( m_semReset, WAIT_FOREVER );
	assert( sts == OK );
}

//
// resetUnlock
//
// release reset clock
//
void	LtIpPortClient::resetUnlock()
{
	STATUS	sts;

	// we may want an assert that doesn't go away
	// when debug is not defined
	assert( m_semReset != NULL );
	sts = semGive( m_semReset );
	assert( sts == OK );
}


//
// processPacket
//
// Called on LreClient object by LreServer.
// Pass packet to LtLink to transmit onto the LonTalk link.
//
void LtIpPortClient::processPacket(boolean bPriority, LtPktInfo *pPkt)
{
	LtSts	sts;
	byte	pktFlags;

	// if we have a link registered
	// then send the packet to the link to be sent
	// m_bLreClientActive
	if ( m_bLreClientActive )
	{
		assert( m_pLink );
		pktFlags = pPkt->getFlags();
#if PRODUCT_IS(ILON)
		if (pPkt->getRouteMultipleNoXmit() && !pPkt->getRouteMultiple()) // These should be mutually exclusive...
		{
			// Add a flag bit to tell the driver not to actually transmit the packet.
			// This is just to allow the iLON to send it to the LSPA.
			pktFlags |= MI_DONT_TRANSMIT;
		}
#endif
		// Subtract two from the length because the Link generates its own CRC.
#ifdef __ECHELON_TRACE
		printf("Send packet length = %d\n", pPkt->getDataSize());
#endif
		sts = m_pLink->sendPacket( pPkt, 0, pktFlags, pPkt->getDataPtr(), pPkt->getDataSize() - 2,
									bPriority );

		if (sts != LTSTS_PENDING)
		{
			// The packet was either sent or failed to be sent so release it.  If the status
			// indicates the packet is pending, then it will be released by the link once
			// it has been sent.
			packetComplete( pPkt, sts );

            if (sts == LTSTS_TOOLONG)
            {
                LtLreClient *pSource = pPkt->getSourceClient();
                if (pSource != NULL)
                {
                    pSource->setErrorLog(LT_NET_BUF_TOO_SMALL);
                }
            }
		}
	}
	else
	{	// if we are shutdown, then just release the packet as if
		// it had been sent
		packetComplete( pPkt, LTSTS_INVALIDSTATE );
	}

}

//
// start
//
// Used to start up or stop the client.  true is returned if the function
// succeeded.
// For the LtIpPortClient
// Need to open or close the link as well.
boolean LtIpPortClient::start()
{
	boolean		bOk = false;
	LtSts		sts;
	assert( m_pLink );

	m_pAlloc->setMasterFree( false );
	m_pAlloc->registerOwner( this );

	if ( !m_bLreClientActive )
	{
		sts = m_pLink->open( m_name );
		// don't sweat an open failure if the interface name is blank
		if ( 0 == strlen( m_name ) || sts == LTSTS_OK )
		{	bOk = true;
			m_bLreClientActive = true;
			sts = m_pLink->setQueueDepths(m_nReceiveQueueDepth, m_nTransmitQueueDepth);
			m_pLink->setServicePinState(SERVICE_ON);
			assert(sts == LTSTS_OK || sts == LTSTS_RESET);
			resetAndSetCommParams();
		}
		else
		{
			vxlReportUrgent("LonTalk Port open failure %d\n", sts);
		}
	}
	else
	{
		queueReceives();
	}

	return bOk;
}

//
// stop
//
// shut down the link.
// First reset to get our packets back.
// then close the link
//
boolean LtIpPortClient::stop()
{
	boolean		bOk = false;
	assert( m_pLink );
	boolean bActive = m_bLreClientActive;
	// This flag indicates we are "stopped" and is key to ensuring that other threads
	// don't try to take the "lock" while we have it here.
	m_bLreClientActive = false;
	lock();
	if ( bActive)
	{
		// Until we know close behaves as desired (doesn't return until
		// all buffers have been returned), do a reset first to ensure
		// this.  
		resetAndSetCommParams(false);

		m_pLink->close();
		bOk = true;
	}
	unlock();
	return bOk;
}

// LtNetwork Methods
//
// called by link object on our LtNetwork identity
//

void LtIpPortClient::registerLink( LtLink& link )
{	
	if (m_pLink != &link)
	{
		assert(m_pLink == null);
		m_pLink = &link;
		m_pLink->registerNetwork( *this );
	}
}

//
// packetReceived
//
// packet received from LtLink.
// send it through the LreServer to be routed.
//
void LtIpPortClient::packetReceived(void* referenceId,
						int nLengthReceived,
						boolean bPriority,
						int receivedSlot,
						boolean isValidPacket,
						byte    l2PacketType,
						LtSts sts,
						byte	ssiReg1,
						byte	ssiReg2)
{
	LtPktInfo*	pPkt = (LtPktInfo*)referenceId;

	#ifdef ENABLE_CRUMBS
	pPkt->setCrumb( "LtIpPc - packetReceived" );
	#endif // ENABLE_CRUMBS

	pPkt->setPhaseReading(receivedSlot);
	pPkt->setSsi(l2PacketType==L2_PKT_TYPE_MODE2_INCOMING, ssiReg1, ssiReg2);

	byte* pData = pPkt->getDataPtr();
	boolean discard = false;
	boolean tracePkt = false;
	int sbcrchi = 0;	// init to remove warning
	int sbcrclo = 0;	// init to remove warning

	// We don't protect against length errors here - we assume the caller is well behaved.

	if (isValidPacket)
	{
		if (nLengthReceived < 2)
		{
			if (sts != LTSTS_RESET)
				vxlReportEvent("Packet received with invalid length (%d)\n", nLengthReceived);
			isValidPacket = FALSE;
			l2PacketType = L2_PKT_TYPE_PACKET_TOO_SHORT;
		}
		else 
		{
			// EPR 23157.  The neuron has a bug in which the first byte of the packet is sometimes 
			// dropped AFTER calculating CRC.  So recalculate CRC and drop the message if it doesn't
			// match. 
			int crchi = pData[nLengthReceived-2];
			int crclo = pData[nLengthReceived-1];
			LtCRC16(pData, nLengthReceived-2);
			sbcrchi = pData[nLengthReceived-2];
			sbcrclo = pData[nLengthReceived-1];
			if (sts == LTSTS_OK &&
				(pData[nLengthReceived-2] != crchi ||
				 pData[nLengthReceived-1] != crclo))
			{
				// LED funcs obsolete
				//LEDON;
				vxlReportEvent("Packet received with invalid CRC!!!!!\n");
				tracePkt = true;
				isValidPacket = FALSE;
				l2PacketType = L2_PKT_TYPE_CRC;
				//LEDOFF;
			}
			pData[nLengthReceived-2] = crchi;
			pData[nLengthReceived-1] = crclo;
		}
	}
	pPkt->setIncomingSicbData(isValidPacket, l2PacketType);

	if (tracePkt)
	{
		char *buf = new NOTHROW char[nLengthReceived*3 + 100];
		if (buf != NULL)
		{
			char *pBuf;
			sprintf(buf, "sts=%d pri=%d slot=%d RX%d:", sts, bPriority, receivedSlot, nLengthReceived);
			pBuf = &buf[strlen(buf)];
			for (int i=0; i<nLengthReceived; i++)
			{
				sprintf(pBuf, " %02X", pData[i]);
				pBuf += 3;
			}
			sprintf(pBuf, " (s.b.) %02X %02X\n", sbcrchi, sbcrclo);
			vxlReportEvent(buf);

			delete[] buf;
		}
	}

#if 0
	{
		static ULONG	nLreLastTick = 0;
		static ULONG    deadtime = 10000;
		ULONG	nTickNow = tickGet();
		ULONG	nMsDead;
		nMsDead = nTickNow -nLreLastTick;
		nMsDead = ticksToMs( nMsDead );
		if ( nMsDead > deadtime )
		{
			vxlReportEvent("LtLrePortClient::packetReceived - sts=%d pri=%d slot=%d RX%d\n", sts, bPriority, receivedSlot, nLengthReceived);
		}
		nLreLastTick = nTickNow;
	}
#endif

	if ( m_bLreClientActive && !discard )
	{
		if (sts == LTSTS_OK)
		{
			pPkt->setMessageNoRef( NULL, nLengthReceived );
			m_pServer->routePacket( bPriority, this, pPkt );
		}
		else
		{
			pPkt->release();
		}
	}
	else
	{
		// else, just release the packet
		packetComplete( referenceId, LTSTS_INVALIDSTATE );
	}
}


//
// packetComplete
//
// packet send complete to the link.
// release the packet.
//
void LtIpPortClient::packetComplete(void* referenceId, LtSts sts )
{
	// m_bLreClientActive
	// regardless of the state of anything, just release the packet
	// since we are the end of the line.
	LtPktInfo*	pPkt = (LtPktInfo*)referenceId;
	pPkt->release();
}

//
// Private members
//

//
// queueReceives
//
// Queue receive buffers to the link.
// either priority or non-priority
//
void	LtIpPortClient::queueReceives()
{
	LtPktInfo*		pPkt = NULL;

	m_bQueueReceivesNeeded = false;

	lock();

	while ( m_bLreClientActive )
	{
		pPkt = m_pAlloc->allocPacket();
		if ( pPkt == NULL )
		{	
			// The allocator emptied before we could complete!?!  One way this
			// could happen is if a packet arrived before we completed this 
			// process and that packet forced lots of deep copies or 
			// clones, exhausting the allocator.  It would be nice if we
			// could queue receives prior to starting up the input stream
			// but the current driver doesn't allow this.  Another attempt
			// to queue receives will be made later when a master is returned
			m_bQueueReceivesNeeded = true;
			break;
		}
		pPkt->setPriority(false);
		if (!masterRelease(pPkt))
		{
			#ifdef ENABLE_CRUMBS
			pPkt->setCrumb( "LtIpPc - queueReceives: released" );
			#endif // ENABLE_CRUMBS
			pPkt->release();
			break;
		}
	}

	unlock();
}


// Interface for special handling with LtIpPortLink class

//
// registerClient
//
// A stack calls here to tie into the PortClient
// We return a PortLink object for the stack to use to communicate with
// us.
//
LtIpPortLink*	LtIpPortClient::registerClient( LtNetwork* pNetClient )
{
	lock();
	LtIpPortLink*	pPLink = new LtIpPortLink();
	assert( pPLink );

	pPLink->m_pNet = pNetClient;
	pPLink->m_pPortClient = this;

	m_vNets.addElement( pPLink );
	unlock();
	return pPLink;
}

//
// removeClient
//
// A stack calls here with it's own address, previously used to registerClient.
// to remove itself from membership.
//
boolean			LtIpPortClient::removeClient( LtNetwork* pNetClient )
{
	lock();
	LtIpPortLink*	pPLink = NULL;
	LtVectorPos		pos;
	boolean			bFound = false;

	while ( m_vNets.getElement( pos, &pPLink ) )
	{
		if ( pPLink->m_pNet == pNetClient )
		{
			m_vNets.removeElement( pPLink );
			delete pPLink;
			bFound = true;
			break;
		}
	}

	unlock();

	if (bFound)
	{
		// Update service LED
		setServicePinState(NULL, SERVICE_OFF);
	}

	return bFound;
}

//
// Special handling routines between the PortLink and the Link
//

void	LtIpPortClient::resetAndSetCommParams(boolean bRequeue)
{
	resetLock();

	// The following two operations are done together under lock
	// to exclude other clients.  It is assumed that these routines
	// won't return until all buffers have been returned from the
	// driver.
	m_bResetting = true;
	if (m_commParams.m_nData[0])
	{
		m_pLink->reset();
		m_pLink->setCommParams( m_commParams );
	}
	m_bResetting = false;

	if (bRequeue)
	{
		queueReceives();
	}

	resetUnlock();
}

//
// reset
//
// a NOP. reset must always be paired, atomically, with setCommParams.
// so we ignore reset and perform reset when setCommParams comes along.
//
void	LtIpPortClient::reset( LtIpPortLink* pPortLink )
{
	// Do nothing.
	// reset will be done when setCommParams is done.
}

//
// setCommParams
//
// Update comm parameters if they have changed.  Note that there may
// be multiple devices all using the link.  Normally the config data 
// just gets updated from the device - thus keeping them all in synch.  However
// if the config data is updated via a network mangement message, and then the
// device gets reset, this routine will detect a change and update them in the
// driver.  Thus the last one wins.
//
// Ideally the config data of every device would get updated automatically.  But
// I don't know how to do that.  As it is, the config data will get refreshed
// from the driver on reboot.
//
LtSts	LtIpPortClient::setCommParams( LtCommParams& commParams )
{
	LtIpPortLink*	pPLink = NULL;
	LtVectorPos		pos;

	lock();

	LtCommParams	originalCps = m_commParams;

	m_commParams = commParams;	// save the master set
	m_bGotCommParams = true;
	while ( m_vNets.getElement( pos, &pPLink ) )
	{
        pPLink->m_commParams = commParams;
	}

	unlock();

	// resetting comm port is expensive.  So only do it if necessary.
	// The flag forces this to happen on startup, so the link will be initialized
	if (memcmp(&m_commParams, &originalCps, sizeof(m_commParams)) != 0 ||
		m_bResetRequested)
	{
		m_bResetRequested = false;
		resetAndSetCommParams();
	}

	return LTSTS_OK;
}


LtSts	LtIpPortClient::getCommParams( LtIpPortLink* pPL, LtCommParams& commParams )
{
	LtSts sts = LTSTS_OK;

	// return saved comm params from this object.
	if (m_commParams.m_nData[0] == 0x00)
	{
		// Don't have the values yet.  Get them.
		sts = m_pLink->getCommParams(m_commParams);
	}
	commParams = m_commParams;
	return sts;
}

//
// getTransceiverRegister
//
// There are 7 registers, 1-7. Zero is invalid.
// Allow any client to request a register.
// Remove duplicate requests to the link.
// Deliver the results to all that asked.
//
LtSts	LtIpPortClient::getTransceiverRegister( LtIpPortLink* pPL, int n )
{
	LtSts	sts = LTSTS_ERROR;

	if ( n < 1 || n > 7 )
	{	return sts;
	}
	lock();
	pPL->m_aRegsReq[n] = true;		// requested. Ignore case of duplicate requests from
									// same client. They will get one callback, not N.
	if ( m_aRegsReq[n] )
	{	// already requeseted, nothing to do now.
	}
	else
	{	m_aRegsReq[n] = true;
		sts = m_pLink->getTransceiverRegister(n );
	}
	unlock();
	return sts;
}

//
// reportTransceiverRegister
//
// Report the result to all who asked.
// CAUTION - The port client remains locked during the callback,
// so calling getTransceiverRegister or anything else from the
// callback will hang the thread.
//
void LtIpPortClient::reportTransceiverRegister(int n, int value, LtSts sts)
{

	lock();
	LtIpPortLink*	pPLink = NULL;
	LtVectorPos		pos;

	m_aRegsReq[n] = false;
	while ( m_vNets.getElement( pos, &pPLink ) )
	{
		if ( pPLink->m_aRegsReq[n] )
		{
			pPLink->m_aRegsReq[n] = false;
			//unlock();
			// She may NOT request another transceiver register here.
			// so we need NOT unlock the object.
			// This is safer since the enumeration is protected.
			pPLink->m_pNet->reportTransceiverRegister( n, value, sts );
			//lock();
		}
	}
	unlock();
}

int	VXLCDECL LtIpPortClient::flickerTask( int obj, ... )
{
	LtIpPortClient* pClient = (LtIpPortClient*)obj;
	for (int i=0; i<25; i++)
	{
		pClient->m_pLink->setServicePinState( (i&1)?SERVICE_OFF:SERVICE_ON );
		taskDelay(100*sysClkRateGet()/1000);
	}
	pClient->m_pLink->setServicePinState( pClient->m_nServicePinState );
	return 0;
}


void LtIpPortClient::wink()
{
#if !PRODUCT_IS(FTXL)
	// Temporarily flicker the service LED.  When done, restore it to correct state.
	// Flicker is done by software - turning LED off/on quickly.
	taskSpawn("LED_flicker", LED_FLICKER_PRIORITY, 0, 4096, flickerTask, (int)this,0,0,0,0, 0,0,0,0,0);
#endif
}

void LtIpPortClient::setServicePinStateSpecial(LtServicePinState state)
{
	m_ipServiceState = state;
	setServicePinState(NULL, state);
}

//
// setServicePinState
//
// Set the greatest service pin state requested by any of the clients.
// priority order is ON=high, BLINKING, OFF=low.
//
void	LtIpPortClient::setServicePinState( LtIpPortLink* pPL, LtServicePinState state )
{
	boolean				bDoIt = false;
	lock();

	// Store what this client ask for
	if (pPL)
	{
		pPL->m_nPinState = state;
	}

	// If she asked for ON, then she gets it, unless it's already set.
	if ( state == SERVICE_ON )
	{
		if ( m_nServicePinState != SERVICE_ON )
		{
			m_nServicePinState = state;
			bDoIt = true;
		}
	}
	else
	{
		// Shucks. We have to scan to find whether the max we have is OFF or BLINKING
		LtIpPortLink*	pPLink = NULL;
		LtVectorPos		pos;
		LtServicePinState	nMaxState = m_vNets.isEmpty() ? SERVICE_ON : SERVICE_OFF;

		LtServicePinState	nState = m_ipServiceState;
		do
		{
			if (pPLink != NULL)
			{
				nState = pPLink->m_nPinState;
			}
			if ( nState == SERVICE_ON &&
				(nMaxState == SERVICE_BLINKING || nMaxState == SERVICE_OFF)
				)
			{	nMaxState = nState;
			}
			else if ( nState == SERVICE_BLINKING && nMaxState == SERVICE_OFF )
			{	nMaxState = nState;
			}
		}
		while ( m_vNets.getElement( pos, &pPLink ) );

		if ( nMaxState != m_nServicePinState )
		{	bDoIt = true;
			m_nServicePinState = nMaxState;
		}
	}
	// If we are supposed to set the state, then tell the link.
	if ( bDoIt )
	{
		m_pLink->setServicePinState( m_nServicePinState );
	}

	unlock();
}


//
// setProtocolAnalyzerMode
//
// Set the OR of these requests by the client to the link.
//
void	LtIpPortClient::setProtocolAnalyzerMode( LtIpPortLink* pPL, boolean on )
{
	boolean				bDoIt = false;
	lock();

	// Store what this client ask for
	pPL->m_bProtAnalMode = on;

	// If she asked for ON, then she gets it, unless it's already set.
	if ( on )
	{
		if ( !m_bProtocolAnalyserMode )
		{
			m_bProtocolAnalyserMode = true;
			bDoIt = true;
		}
	}
	else
	{
		// Shucks. We have to scan to find whether the max we have is ON or OFF
		LtIpPortLink*	pPLink = NULL;
		LtVectorPos		pos;
		boolean			bMax = false;

		while ( m_vNets.getElement( pos, &pPLink ) )
		{
			assert( pPLink);
			bMax = bMax || pPLink->m_bProtAnalMode;
		}
		if ( bMax == m_bProtocolAnalyserMode )
		{	bDoIt = true;
			m_bProtocolAnalyserMode = bMax;
		}
	}
	// If we are supposed to set the state, then tell the link.
	if ( bDoIt )
	{
		m_pLink->setProtocolAnalyzerMode( m_bProtocolAnalyserMode );
	}

	unlock();
}

//
// getProtocolAnalyzerMode
//
// Just return our local understanding of this value to the client.
//
boolean LtIpPortClient::getProtocolAnalyzerMode( LtIpPortLink* pPL )
{
	return m_bProtocolAnalyserMode;
}


void LtIpPortClient::flushCompleted()
{
	// TBD
}

void LtIpPortClient::terminateCompleted()
{
	// TBD
}

void LtIpPortClient::resetRequested()
{
	LtVectorPos		pos;
	LtIpPortLink*   pLink;

	// Record fact that a reset was requested.  This allows us to ensure
	// that at least one reset is done.
	m_bResetRequested = true;

	lock();
	while ( m_vNets.getElement( pos, &pLink ) )
	{
		pLink->m_pNet->resetRequested();
	}
	unlock();
}

void LtIpPortClient::servicePinDepressed()
{
	LtVectorPos		pos;
	LtIpPortLink*   pLink;

	lock();
	while ( m_vNets.getElement( pos, &pLink ) )
	{
		pLink->m_pNet->servicePinDepressed();
	}
	unlock();
}

void LtIpPortClient::servicePinReleased()
{
	LtVectorPos		pos;
	LtIpPortLink*   pLink;

	lock();
	while ( m_vNets.getElement( pos, &pLink ) )
	{
		pLink->m_pNet->servicePinReleased();
	}
	unlock();
}


// Used to get the routes associated with a client determined by
// IP or LonTalk routing messages.  Function returns true if it succeeds, 
// false if no more routes.  Index is expected to be 0..1 to cover
// two domains.
boolean LtIpPortClient::getRoute(int& index, LtRoutingMap *pRoute)
{
	boolean bResult = false;
	// Say we're a repeater so that we get everything.
	if (index++ == 0)
	{
		pRoute->setRouterType(LT_REPEATER);
		bResult = true;
	}
	return bResult;
}

int		gnPktStamp = 0;

boolean LtIpPortClient::masterRelease(LtMsgRef* pMsg)
{
	if (m_bResetting) return false;
	if (pMsg == null) return true;
	boolean result = true;
	LtPktInfo* pPkt = (LtPktInfo*) pMsg;
	LtSts sts;

	boolean bPriority = pPkt->getPriority();

	// Need to make sure we put the reference back.
	pPkt->setMessageData(pPkt->getBlock(), pPkt->getBlockSize(), pPkt);
	pPkt->initPacket();

	#ifdef ENABLE_CRUMBS
	pPkt->setCrumb( "Queued to driver" );
	pPkt->setTimestamp( gnPktStamp );
	gnPktStamp++;
	#endif // ENABLE_CRUMBS
	sts = m_pLink->queueReceive( pPkt, bPriority, pPkt->getFlags(), pPkt->getBlock(),
			  					 pPkt->getBlockSize() );

	if (sts != LTSTS_PENDING)
	{
		// Expected queue is full.  Try the other.
		bPriority = !bPriority;
		#ifdef ENABLE_CRUMBS
		pPkt->setCrumb( "Queued to driver - other queue" );
		#endif // ENABLE_CRUMBS
		sts = m_pLink->queueReceive( pPkt, bPriority, pPkt->getFlags(), pPkt->getBlock(),
				  					 pPkt->getBlockSize() );
		if (sts != LTSTS_PENDING)
		{
			// Driver is full.  This can occur if a reset occurs while
			// buffers are outstanding.  It also can occur while doing
			// receive queue stuffing (queueReceives).
			result = false;
		}
	}

	if (m_bQueueReceivesNeeded)
	{
		queueReceives();
	}

	return result;
}

void	LtIpPortClient::setName(const char *pName )
{
    delete m_name;
    m_name = new char[strlen(pName)+1];
    strcpy(m_name, pName);
}

#ifdef WIN32
void LtIpPortClient::registerLdvHandle(int ldvHandle)
{
    m_pLink->registerLdvHandle(ldvHandle);
}
#endif

// end
