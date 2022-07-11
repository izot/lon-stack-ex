#ifndef _LTLINKBASE_H
#define _LTLINKBASE_H

/***************************************************************
 *  Filename: LtLinkBase.h
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
 *  Description:  Header for LtLinkBase class
 *
 *	DJ Duffy Dec 1998
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Shared/include/LtLinkBase.h#1 $
//
/*
 * $Log: /VNIstack/FtXl_Dev/NiosWs/FtxlLib/ShareIp/include/LtLinkBase.h $
 * 
 * 27    8/20/07 1:04p Bobw
 * EPR 45942
 * Reduced stack sizes for FTXL to 8192 bytes each stack (controlled by
 * lit int TaskPrioty.h), reducing stack size from 290K to 72K.  The
 * following shows the stack use:
 * 
 * Prio  Allocated         Used	% Used
 * 1     8192	                2068	25.24%
 * 2     8192	                1956	23.88%
 * 3     8192	                1212	14.79%
 * 4     8192	                2656	32.42%
 * 5     8192	                1972	24.07%
 * 6     8192	                2276	27.78%
 * 
 * 26    8/01/07 5:50p Fremont
 * EPR 45753 - clearing the network statistics of an internal device
 * cleared the console linkstats display, which was intended to show the
 * external packet statistics. In standalone mode on the iLON, this
 * happened automatically on any internal device commission. Solution:
 * keep a shadow shadow copy of the statistics that were actually shared,
 * increment them in the driver, and read/clear the shadow copies when
 * accessing the internal device.
 * 
 * 25    6/20/07 3:58p Bobw
 * Support service pin held function
 * 
 * 24    1/10/05 1:56p Fremont
 * Reorgainize for platform-specific functions, add debug code
 * 
 * 23    4/23/04 5:25p Bobw
 * Keep track of whether or not read only data has been updated.  Update
 * network buffers only if it has been.  If it hasn't, then refresh the
 * read only data from the driver's cache.  Otherwise we have the
 * situation that stack A sets the read only data and updates the network
 * buffers, then stack B resets, but since it still has stale read only
 * data it sets them back to the old values.  Note that the read only data
 * of a stack will still not be updated with the latest network buffers
 * until it resets - but this seems OK.
 * 
 * 22    4/23/04 4:16p Bobw
 * EPR 32997
 * On Layer2 mip, maintain a cache of network buffers.  On startup, read
 * the buffers from the mip (if they have not already been read), and
 * update the stacks read-only data to reflect the network buffers (but
 * not app buffers).  On reseting a stack, check to see if the buffers in
 * read only data differ from those in the LONLINK cache, and if so, write
 * them back out to the mip.
 * 
 * 21    4/06/02 3:27p Vprashant
 * dont start timer incase of i.LON platform
 * 
 * 20    11/07/01 3:36p Fremont
 * provide for receive task priority and stack size overrides
 * Provide receive task wakeup func
 * 
 * 19    11/07/01 10:32a Fremont
 * rename m_flags to m_pktFlags to avoid conflict with stupid Tornado
 * incude file (mbuf.h)
 * 
 * 18    11/06/01 2:38p Fremont
 * Add link receive task name override
 * 
 * 17    11/06/01 9:25a Glen
 * Need control of zero crossing synchronization and attenuation for
 * LonTalk Validator.  Added these flags to sendPacket().  This propagated
 * to lots of places.  Also added control options to LtMsgOut.
 * 
 * 16    10/24/01 12:04p Fremont
 * change prototype
 * 
 * 15    10/10/00 2:16p Darrelld
 * Fix thread rundown
 * 
 * 14    3/13/00 5:35p Darrelld
 * Eliminate VVector
 * 
 * 12    7/30/99 8:59a Darrelld
 * Cleanup and streamline
 * 
 * 11    4/23/99 5:22p Darrelld
 * Router testing
 * 
 * 10    4/20/99 3:52p Darrelld
 * Global dump enable members are static
 * 
 * 9     4/19/99 10:29a Darrelld
 * Add service pin read and send
 * 
 * 8     3/18/99 4:17p Darrelld
 * Add cloneInstance
 * 
 * 7     3/15/99 10:39a Darrelld
 * Intelligent control of dumping packets
 * 
 * 6     3/11/99 11:13a Darrelld
 * dumpPacket is now public rather than protected
 * 
 * 5     2/22/99 9:38a Darrelld
 * Remove windows dependencies
 * 
 * 4     1/21/99 6:07p Glen
 * Get ready to run on VxWorks
 * 
 * 3     12/16/98 1:22p Darrelld
 * Update LtLink interface
 * 
 * 2     12/15/98 4:33p Darrelld
 * Reorganize base classes to make LtLink pure and virtual as the driven
 * snow.
 * 
 * 1     12/15/98 12:09p Darrelld
 * 
 */
#include <vxWorks.h>
#include <wdLib.h>
#include <semLib.h>
#include <LtDriver.h>
#include <RefQues.h>
#include <LtObject.h>
#include "taskPriority.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// LLPktQue class
//
// describe a packet to receive into
//
class LLPktQue : public LtQue
{
public:
	void*	m_refId;
	boolean	m_bPriority;
	byte	m_pktFlags;		// Keep name in sync with LtPktInfo: see LtPktInfo.h for details
	byte*	m_pData;
	int		m_nDataLength;
};



//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//
// Network layer to driver interface
//
class LtLinkBase : public LtLink, public LtObject
{

public:
	LtLinkBase();
	virtual ~LtLinkBase();
	//
	// The following interfaces are required:
	//
public:
	virtual void destroyInstance() { delete this; }
	virtual LtLinkBase* cloneInstance()
	{	return new LtLinkBase();
	}
	virtual boolean enumInterfaces( int idx, LPSTR pNameRtn, int nMaxSize );
	virtual LtSts open( const char* pName );
	virtual boolean		isOpen();
	virtual LtSts close();
	virtual LtSts registerNetwork(LtNetwork& net);
	virtual LtSts setReceivePriority(int priority);
	virtual void getMaxQueueDepth( int* pnReceiveQueueDepth, int* pnTransmitQueueDepth );
	virtual LtSts setQueueDepths( int nReceiveQueueDepth, int nTransmitQueueDepth );
	virtual LtSts sendPacket(void* referenceId,
					int nPrioritySlot,
					byte flags,
					byte* pData,
					int nDataLength,
					boolean bPriority);
	virtual LtSts queueReceive( void* referenceId,
						boolean bPriority,
						byte flags,
						byte* pData,
						int nMaxLength);
	virtual void reset();
	virtual int getStandardTransceiverId();
	virtual boolean getUniqueId(LtUniqueId& uniqueId);
	virtual void flush(boolean on);
	virtual void terminate();
	virtual LtSts setCommParams(const LtCommParams& commParams);
	virtual LtSts getCommParams( LtCommParams& commParams);
	virtual LtSts getTransceiverRegister(int n);
	virtual void getStatistics(LtLinkStats& stats);
	virtual void getStatistics(LtLinkStats *&pStats);
	virtual void clearStatistics();
	virtual void setServicePinState(LtServicePinState state);
	virtual void setProtocolAnalyzerMode(boolean on);
	virtual boolean getProtocolAnalyzerMode();
	virtual void setLoopbackMode(boolean on);
	virtual boolean getLoopbackMode();
	virtual LtSts selfTest();
	virtual LtSts reportPowerSelfTest();
	virtual void setCurrentBacklog(int backlog);
	virtual int getCurrentBacklog();
	virtual void setWindowSize(int windowSize);
	virtual int getWindowSize();
	virtual void setTransmitSlot(int transmitSlot);
	virtual int getTransmitSlot();

	static boolean	dumpEnableGlobal( boolean bEnable );
	static boolean	dumpHeadersOnlyGlobal( boolean bEnable );
	virtual boolean	dumpEnable( boolean bEnable );
	virtual boolean	dumpHeadersOnly( boolean bEnable );
	virtual void	dumpPacket( LPCSTR tag, byte* pData, int nLen );
	virtual void	dumpLtPacket( LPCSTR tag, byte* pData, int nLen );

	virtual LtServicePinState getServicePinState()
	{	return m_nServicePinState;
	}
	virtual void sendServicePin()
	{
		if ( m_pNet )
		{	m_pNet->servicePinDepressed();
		}
	}
    virtual void servicePinReleased()
    {
		if ( m_pNet )
		{	m_pNet->servicePinReleased();
		}
    }
    virtual LtSts getNetworkBuffers(LtReadOnlyData& readOnlyData){return LTSTS_OK;}
	virtual LtSts setNetworkBuffers(LtReadOnlyData& readOnlyData) {return LTSTS_OK;}

protected:

	virtual void	lock()
	{	//EnterCriticalSection( &m_lock );
		semTake( m_lock, WAIT_FOREVER );
	}
	virtual void	unlock()
	{	//LeaveCriticalSection( &m_lock );
		semGive( m_lock );
	}

	virtual LtSts	tryTransmit( void* refId, byte flags, byte* pData, int nDataLength );
	virtual void	queueTransmit( void* refId, byte flags, byte* pData, int nDataLength );
	virtual void	startDelayedRetransmit();	// Derived classes should implement only 
	virtual void	startImmediateRetransmit();	// one of these two methods.
	friend int		LtLinkReceive( int a1, ... );
	virtual void	receiveTask();
	// Some default parameters for the receive task
	virtual const char*	getRcvTaskName() { return("linkRcv"); }
	virtual int		getRcvTaskPriority() { return(LINK_RCV_TASK_PRIORITY); }
	virtual int		getRcvTaskStackSize() { return(LINK_RCV_TASK_STACK_SIZE); }

	virtual void	drainReceiveQueue( LtQue& qReceive );
	virtual void	drainTransmitQueue( LtQue& qTransmit );
	// manage a lookaside list of packet descriptor items
	// to avoid new / delete
	virtual LLPktQue* getLLPkt();
	virtual void	freeLLPkt( LLPktQue* pLLPkt );
	virtual void	drainFreeQueue( LtQue& qReceive );

	virtual void	startReceiveTask();
	virtual void	stopReceiveTask();
	virtual void	wakeReceiveTask() {}	// Wake for shutdown sync


	
	SEM_ID				m_lock;				// lock the object
	LtQue				m_qFreePkts;		// free LLPktQue items
	LtQue				m_qTransmit;		// transmit queue
	LtQue				m_qReceive;			// receive queue
	LtQue				m_qReceiveP;		// receive queue
	//HANDLE				m_hReceiveEvent;	// receive event
	void*				m_hReceiveEvent;	// receive event
	int					m_tidReceive;		// task ID of receive thread


protected:
	LtNetwork*			m_pNet;
	boolean				m_bActive;
	boolean				m_bLinkOpen;
	boolean				m_bFlushMode;
	boolean				m_bProtoAnalyzerMode;
	boolean				m_bLoopback;
	int					m_nBacklog;
	int					m_nWindowSize;
	int					m_nReceiveQueueDepth;
	int					m_nTransmitQueueDepth;
	int					m_nReceiveTaskPriority;
	LtLinkStatsShadow	m_linkStats;
	LtServicePinState	m_nServicePinState;
	int					m_nTransmitSlot;
	boolean				m_bDumpEnable;
	boolean				m_bDumpHeadersOnly;
	boolean				m_bExitReceiveTask;
};



#endif // _LTLINKBASE_H
