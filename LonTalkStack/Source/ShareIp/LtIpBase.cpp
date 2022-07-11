/***************************************************************
 *  Filename: LtIpBase.cpp
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
 *  Description:  Implementation file for Lt IP base class.
 *
 *	DJ Duffy Feb 1999
 *
 ****************************************************************/

/*
 * $Log: /Dev/ShareIp/LtIpBase.cpp $
 * 
 * 26    6/09/05 6:07p Fremont
 * EPRS FIXED: 37362 - IP-852 channel uses too much memory. Point all
 * packet allocations to the "master" object, and don't allocate any
 * buffers for the slaves.
 * 
 * 25    4/28/04 6:49p Bobw
 * EPR 32593
 * Support VNI based protocol analyzer.  Pass bad packets up, and time
 * stamp them at first oportunity.  
 * 
 * 24    9/29/03 6:29p Iphan
 * EPRS FIXED: 29122 - LtIpBase::stop - timeout! too long to get buffers
 * back
 * 
 * 23    5/02/03 3:21p Iphan
 * IKP05022003: - added checking of pPkt against null pointer reference.
 *                         - increased MAX MsgRef and MsgBlk to 600 and
 * 750 respectively.
 * 
 * 22    12/11/02 6:20p Fremont
 * fix uninitialized sts
 * 
 * 21    11/06/01 9:25a Glen
 * Need control of zero crossing synchronization and attenuation for
 * LonTalk Validator.  Added these flags to sendPacket().  This propagated
 * to lots of places.  Also added control options to LtMsgOut.
 * 
 * 20    11/17/00 10:05a Darrelld
 * Remove bogus counters
 * 
 * 19    3/13/00 5:35p Darrelld
 * Segmentation work
 * 
 * 17    12/18/99 5:28p Darrelld
 * Need more msg refs than bufs
 * 
 * 16    12/09/99 12:13p Darrelld
 * EPR 15860 - i.Lon fails as receiver when traffic is not aggregated.
 * 
 * 15    12/08/99 10:54a Darrelld
 * Fix aggregation
 * 
 * 14    12/07/99 1:35p Darrelld
 * Crumbs for checking IP
 * 
 * 13    10/07/99 12:06p Glen
 * Bump allocator limits
 * 
 * 12    8/20/99 9:37a Darrelld
 * Allow multiple starts on the same client
 * 
 * 11    8/17/99 4:15p Darrelld
 * Symbolize packet size for allocator
 * 
 * 10    7/28/99 4:46p Darrelld
 * Implement master release for packet starvation fix
 * 
 * 9     7/08/99 1:11p Darrelld
 * Fix deadlock problem
 * 
 * 8     7/06/99 1:04p Darrelld
 * Clean up timers and tasks
 * 
 * 7     7/02/99 8:45a Darrelld
 * Target specific files
 * 
 * 6     5/06/99 5:09p Darrelld
 * Enhancments to RFC packets
 * 
 * 5     4/22/99 5:02p Darrelld
 * Testing of routers
 * 
 * 4     4/20/99 4:12p Darrelld
 * add "confidential" statement
 * 
 * 3     4/13/99 4:58p Darrelld
 * Enhance for Aggregation and BW Limiting and test
 * 
 * 2     4/01/99 4:47p Darrelld
 * Router testing
 * 
 * 
 */
// LtIpBase.cpp: implementation of the LtIpBase class.
//
//////////////////////////////////////////////////////////////////////
#include <LtRouter.h>
#include <LtIpBase.h>
#include <IpLink.h>
#include <vxlTarget.h>
#include <tickLib.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define LINKBUFS 40

void LtIpBase::construct(LtIpBase* pMasterLtIpBase)
{
	m_bActive					= false;
	m_nReceiveQueueDepth		= LINKBUFS;		// default to 40 for now
	// DJDFIX remove bogus counters
	//m_nRecvsQd					= 0;
	//m_nRecvsQdP					= 0;

	m_bMaster = (pMasterLtIpBase == NULL);
	// we are NOT responsible for creating our own link object
	// but we ARE responsible for destroying it when we leave
	// The reason is that the link type is not known apiori
	// since a different link class is used for testing.
	m_pLink = NULL; // new CIpLink();
	if (m_bMaster)
	{
		m_pAlloc = new LtPktAllocator();
		m_pAlloc->setMasterFree( false );
		m_pAlloc->registerOwner( this );
		// Make sure we have more message refs than bufs.  Message refs
		// are needed for cloning and we want to make sure we have more
		// message refs than bufs in case all bufs are in queue somewhere
		// and cloning needs to occur.
		m_pAlloc->init( ALLOC_BLOCK_SIZE, LINKBUFS, 10, 600 );	// IKP05022003: increased from 400 to 600
		m_pAlloc->initMsgRefs( LINKBUFS, 10, 750 );							// IKP05022003: increased from 500 to 750
	}
	else
	{
		// must point to master's allocator, but how?
		m_pAlloc = pMasterLtIpBase->m_pAlloc;
	}
	m_lock = semMCreate( SEM_Q_PRIORITY | SEM_INVERSION_SAFE );
	assert( m_lock );

}

LtIpBase::LtIpBase(LtIpBase* pMasterLtIpBase)
{
	construct(pMasterLtIpBase);
}
LtIpBase::LtIpBase()
{
	// Assume it is the master
	construct(NULL);
}

LtIpBase::~LtIpBase()
{
	// wipe up our link after us
	if ( m_pLink )
	{
		// Don't call our derived class here
		LtIpBase::stop();
		delete m_pLink;
		m_pLink = NULL;
	}
	if ( m_bMaster && m_pAlloc )
	{
		m_pAlloc->freeAll();
		delete m_pAlloc;
		m_pAlloc = NULL;
	}
	if ( m_lock )
	{
		semDelete( m_lock );
		m_lock = NULL;
	}

}

//
// registerLink
//
// register a link with us, and register us with the link
// we only support the enhanced network object
void LtIpBase::registerLink( LtLink& link )
{
	if ( m_pLink != &link )
	{
		CIpLink*	pLink = (CIpLink*)&link;
		LtNetworkBase::registerLink( link );
		pLink->registerIpNetwork( this );
		pLink->setQueueDepths( LINKBUFS, LINKBUFS );
	}
}

//
// start
//
// Start the client operation by checking that all is well
// with the link and then queueing receives to the link.
//
boolean		LtIpBase::start()
{
	boolean		bOk = m_bActive;
	// we need to
	// 1 Not already be started
	// 2 have a link in the open state
	if ( !m_bActive &&
		  m_pLink &&
		  m_pLink->isOpen() &&
		  m_pAlloc
		  )
	{
		bOk = true;
		m_bActive = true;
		// Only queue normal packets since there's no priority on ip link
		queueReceives( false );
	}

	return bOk;
}


//
// stop
//
// Stop the link with a reset
//
boolean		LtIpBase::stop()
{
	boolean		bOk = false;
	ULONG		tickStart = tickGet();
	ULONG		tickNow = 0;
	// we need to
	// 1  already be started
	if ( m_bActive )
	{
		bOk = true;
		m_bActive = false;
		// pull the handle to get all our packets back
		// then wait until the packets arrive back from the ends
		// of the earth...
		m_pLink->reset();
		while ( m_bMaster && m_pAlloc &&	
			// DJDFIX remove bogus counters
			//( m_nRecvsQdP != 0 && m_nRecvsQd != 0 ) ||
			(m_pAlloc? !m_pAlloc->allItemsReturned():true )
			)
		{	taskDelay( msToTicksX(20) );
			tickNow = tickGet();

			m_pLink->reset();	

			if ( 3000 < ticksToMs( tickNow - tickStart ) )
			{
				vxlReportUrgent("LtIpBase::stop - timeout! too long to get buffers back" );
				m_pAlloc->dumpCrumbs();
				break;
			}
		}
		m_pLink->close();
	}

	return bOk;
}

//
// masterRelease
//
// when a packet is released, then it comes back here and we
// queue it to the link immediately, if we need to.
//

boolean	LtIpBase::masterRelease( LtMsgRef* pMsg )
{
	if ( !m_bActive )
	{	
		return false;
	}
	if (pMsg == null)
	{
		return true;
	}
	boolean result = true;
	LtPktInfo* pPkt = (LtPktInfo*) pMsg;
	LtSts sts;

	boolean bPriority = false;

	// Need to make sure we put the reference back.
	if ( pPkt->getBlock() == NULL || pPkt->getDataPtr() == NULL )
	{	vxlReportUrgent("LtIpBase::masterRelease - block or data ptr NULL\n" );
		return false;
	}
	pPkt->setMessageData(pPkt->getBlock(), pPkt->getBlockSize(), pPkt);
	pPkt->initPacket();

	pPkt->setCrumb( "LtIpBase::masterRelease" );
	sts = m_pLink->queueReceive( pPkt, bPriority, pPkt->getFlags(), pPkt->getBlock(),
			  					 pPkt->getBlockSize() );

	if (sts != LTSTS_PENDING)
	{
		pPkt->setCrumb( "LtIpBase::masterRelease - not pending" );
		if ( pPkt->getDataPtr() == NULL || pPkt->getBlock() == NULL )
		{	vxlReportUrgent("LtIpBase::masterRelease - data ptr NULL - %s\n",
							pPkt->getBlock() == NULL? "NO FIX" : "fixed");
			if ( pPkt->getBlock() )
			{	pPkt->setMessageNoRef( pPkt->getBlock(), pPkt->getBlockSize() );
			}
		}
		else
		{	result = false;
		}
	}
	else
	{
		// DJDFIX remove bogus counters
		//m_nRecvsQd++;
	}
	return result;
}


//
// queueReceives
//
// Queue receive buffers to the link
//
void LtIpBase::queueReceives( boolean bPriority )
{

	LtPktInfo*		pPkt = NULL;
	// DJDFIX remove bogus counters
	int				nRcvsQd =  0; // bPriority? m_nRecvsQdP : m_nRecvsQd;

	while ( m_bActive
		//	&& nRcvsQd < m_nReceiveQueueDepth // DJDFIX
		  )
	{
		pPkt = m_pAlloc->allocPacket();
		if ( pPkt == NULL )
		{	break;		// woops, the allocator is empty
		}
		pPkt->setMessageData(pPkt->getBlock(), pPkt->getBlockSize(), pPkt);
		LtSts	sts;
		pPkt->setCrumb( "LtIpBase::queueReceives" );
		sts = m_pLink->queueReceive( pPkt, bPriority, pPkt->getFlags(), pPkt->getBlock(),
							pPkt->getBlockSize() );
		if ( sts != LTSTS_PENDING )
		{
			pPkt->setCrumb( "LtIpBase::queueReceives released" );
			pPkt->release();
			break;
		}
		//vxlReportEvent("LtIpBase::queueReceives - queue 0x%08x\n", pPkt );
		nRcvsQd++;
	}
#if 0 // DJDFIX remove bogus counters
	if ( bPriority )
	{	m_nRecvsQdP = nRcvsQd;
	}
	else
	{	m_nRecvsQd = nRcvsQd;
	}
#endif
}




//
// packetReceived
//
// Packet received from the link. We need to send it to the
// engine.
//
void LtIpBase::packetReceived(void* referenceId,
						int nLengthReceived,
						boolean bPriority,
						int receivedSlot,
						boolean isValidLtPacket, 
						byte l2PacketType,
						LtSts sts)
{
	// for testing purposes were were called by an LT style link. Sigh.
	// just forward the message and use zero for src addr and port.
	packetReceived( referenceId,
					nLengthReceived,
					bPriority,
					0, // ipSrcAddr,
					0, // ipSrcPort,
					sts);
}
//
// packetReceived
//
// override this function to obtain the ipSrcAddress and ipSrcPort.
// The other packetReceived call is not called by an Iplink object if
// registerIpNetwork is used to set the network object.
//
void LtIpBase::packetReceived(void* referenceId,
								int nLengthReceived,
								boolean bPriority,
								ULONG ipSrcAddr,
								word ipSrcPort,
								LtSts sts)
{
	LtPktInfo*	pPkt = (LtPktInfo*)referenceId;
	// if we active
	//
#if 0 // DJDFIX remove bogus counters
	// adjust the queued receive packet count
	if ( bPriority )
	{	m_nRecvsQdP--;
	}
	else
	{	m_nRecvsQd--;
	}
#endif // DJDFIX
	if ( m_bActive )
	{
		if ( pPkt->getDataPtr() == NULL )
		{	vxlReportUrgent("LtIpBase::packetReceived - NULL data pointer, blk 0x%08x size %d- fixed\n",
								pPkt->getBlock(), pPkt->getDataSize() );
			pPkt->setMessageNoRef( pPkt->getBlock(), pPkt->getDataSize() );
		}
		pPkt->setCrumb("LtIpBase::packetReceived" );
		pPkt->setMessageNoRef( NULL, nLengthReceived );
		pPkt->initPacket();
		pPkt->setIpSrcAddr( ipSrcAddr );
		pPkt->setIpSrcPort( ipSrcPort );
		pPkt->setIncomingSicbData(true, L2_PKT_TYPE_INCOMING);
		// Call our derived class with the packet already set up
		// and the priority and status
		packetReceived( bPriority, pPkt, sts );
		queueReceives( bPriority );
	}
	else
	{
		// else, just release the packet
		pPkt->setCrumb("LtIpBase::packetReceived - not active" );
		packetComplete( referenceId, LTSTS_INVALIDSTATE );
	}
}

//
// sendPacket
//
// We front for the link in this class.
// So we provide a PktInfo style sendPacket
//
LtSts LtIpBase::sendPacket( LtPktInfo* pPkt, boolean bPriority )
{
	LtSts sts;
	assert( m_pLink );

	// if the address is specified in the packet, then use it
	// otherwise, use the default destination address.
	if ( pPkt->getIpSrcAddr() )
	{
		pPkt->setCrumb("LtIpBase::sendPacket - specified IP addr" );
		sts = ((CIpLink*)m_pLink)->sendPacketTo( pPkt,
						pPkt->getIpSrcAddr(), pPkt->getIpSrcPort(),
						pPkt->getDataPtr(), pPkt->getDataSize() );
	}
	else
	{										
		// << WHERE DO WE GET THE PRIORITY SLOT FOR THE FOLLOWING CALL >>
		pPkt->setCrumb("LtIpBase::sendPacket - default IP addr" );
		sts = m_pLink->sendPacket( pPkt, 0, pPkt->getFlags(), pPkt->getDataPtr(), pPkt->getDataSize(),
									bPriority );
	}
	return sts;
}

//
// sendData
//
// send a data block without a packet
//
LtSts LtIpBase::sendData( byte* pData, int nLength, ULONG ipAddr, word ipPort )
{
	LtSts sts;
	LtPktInfo*	pPkt;
	int			nSize;
	pPkt = m_pAlloc->allocPacket();
	if (pPkt != null)		// IKP05022003: added checking to pPkt against null pointer reference
	{
		nSize = pPkt->getBlockSize();
		nSize = MIN( nSize, nLength );
		memcpy( pPkt->getBlock(), pData, nSize );
		pPkt->setMessageData( pPkt->getBlock(), nSize, pPkt );
		pPkt->setIpSrcAddr( ipAddr );
		pPkt->setIpSrcPort( ipPort );
		sts = sendPacket( pPkt, false );
	}
	else
		sts = LTSTS_QUEUEFULL;

	return sts;
}

//
// packetComplete
//
// Link calls the network back here when it has sent the message.
//
void LtIpBase::packetComplete(void* referenceId, LtSts sts )
{
	// m_bLreClientActive
	// regardless of the state of anything, just release the packet
	// since we are the end of the line.
	LtPktInfo*	pPkt = (LtPktInfo*)referenceId;
	// if pPkt is NULL, it's a packet for testing and we don't need to bother
	//
	if ( pPkt )
	{
		pPkt->setCrumb("LtIpBase::packetComplete" );
		pPkt->release();
	}
}

