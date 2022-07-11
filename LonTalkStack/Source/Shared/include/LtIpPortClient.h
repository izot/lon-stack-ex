#ifndef _LTIPPORTCLIENT_H
#define _LTIPPORTCLIENT_H
/***************************************************************
 *  Filename: LtIpPortClient.h
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
 *  Description:  interface for the LtIpPortClient class.
 *
 *	DJ Duffy Jan 1999
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Shared/include/LtIpPortClient.h#1 $
//
/*
 * $Log: /Dev/Shared/include/LtIpPortClient.h $
 * 
 * 34    8/01/07 5:50p Fremont
 * EPR 45753 - clearing the network statistics of an internal device
 * cleared the console linkstats display, which was intended to show the
 * external packet statistics. In standalone mode on the iLON, this
 * happened automatically on any internal device commission. Solution:
 * keep a shadow shadow copy of the statistics that were actually shared,
 * increment them in the driver, and read/clear the shadow copies when
 * accessing the internal device.
 * 
 * 33    6/20/07 3:58p Bobw
 * Support service pin held function
 * 
 * 32    7/15/05 11:23a Bobw
 * EPR 35786
 * Change LonLinkWin to support the layer 2 mip to be pre-opened.  The
 * VniServer opens the device to determine its mip type (determinMipType).
 * Prior to this change, the VniServer opened the device, and if was a
 * layer 2 mip, closed it, so that lonLinkWin could re-open it.  Now
 * lonLinkWin just uses the handle opened previously (if set).  Note that
 * there are other changes to determinMipType necessary to complete this
 * EPR.
 * 
 * 31    4/12/05 9:45a Bobw
 * EPR 36422
 * Support LDV attachment events for layer 2 mips as well as layer 5 mips.
 * 
 * 30    4/28/04 6:49p Bobw
 * EPR 32593
 * Support VNI based protocol analyzer.  Pass bad packets up, and time
 * stamp them at first oportunity.  
 * 
 * 29    4/23/04 5:25p Bobw
 * Keep track of whether or not read only data has been updated.  Update
 * network buffers only if it has been.  If it hasn't, then refresh the
 * read only data from the driver's cache.  Otherwise we have the
 * situation that stack A sets the read only data and updates the network
 * buffers, then stack B resets, but since it still has stale read only
 * data it sets them back to the old values.  Note that the read only data
 * of a stack will still not be updated with the latest network buffers
 * until it resets - but this seems OK.
 * 
 * 28    4/23/04 4:16p Bobw
 * EPR 32997
 * On Layer2 mip, maintain a cache of network buffers.  On startup, read
 * the buffers from the mip (if they have not already been read), and
 * update the stacks read-only data to reflect the network buffers (but
 * not app buffers).  On reseting a stack, check to see if the buffers in
 * read only data differ from those in the LONLINK cache, and if so, write
 * them back out to the mip.
 * 
 * 27    7/28/03 12:49p Bobw
 * EPR 27738
 * Remove code to set priority to highest priorty, simplify, and support
 * changing the comm parameters using a "last one wins" philosophy.
 * 
 * 26    6/23/03 11:09a Glen
 * Development related to supporting NES devices on top of the new Power
 * Line SLTA.  This includes making phase detection and bi-directional
 * query status work.  Also, changed stack such that if a transceiver ID
 * is unknown, the comm params are left unchanged.
 * 
 * 25    3/13/02 12:15p Bobw
 * EPR 23990
 * Support long device driver names.
 * 
 * 24    12/11/01 1:49p Bobw
 * EPR 23157
 * Validate CRC even though the neuron has arleady done so, since the
 * neuron sometimes drops the first byte AFTER validating the CRC.
 * 
 * 23    11/06/01 9:25a Glen
 * Need control of zero crossing synchronization and attenuation for
 * LonTalk Validator.  Added these flags to sendPacket().  This propagated
 * to lots of places.  Also added control options to LtMsgOut.
 * 
 * 22    3/13/00 5:25p Darrelld
 * Eliminate VVector
 * 
 * 21    2/23/00 9:07a Darrelld
 * LNS 3.0 Merge
 * 
 * 19    12/27/99 7:01a Darrelld
 * Make sure IP side service state is reflected in LED
 * 
 * 18    12/20/99 10:09a Darrelld
 * Track asynchronous reset requests
 * 
 * 17    11/24/99 12:58p Darrelld
 * Fix for priority packet transmission - one packet only please
 * 
 * 16    11/07/99 11:03a Darrelld
 * Add wink support
 * 
 * 15    8/19/99 9:52a Darrelld
 * More buffers
 * 
 * 14    7/15/99 1:03p Glen
 * Implement LtStart procedures
 * 
 * 13    3/01/99 6:56p Darrelld
 * Tweak callback mechanism
 * 
 * 12    3/01/99 2:19p Glen
 * New allocator release callback
 * 
 * 11    2/11/99 10:00a Darrelld
 * Count tracking was problematic - removed it.
 * 
 * 10    2/05/99 1:44p Darrelld
 * More buffers for L2 MIP
 * 
 * 9     2/04/99 1:51p Glen
 * Fix buffer exhaustion problem
 * 
 * 8     2/01/99 11:15a Glen
 * Joint Test 3 integration
 * 
 * 6     1/22/99 2:20p Glen
 * 
 * 5     1/22/99 9:11a Glen
 * add confidential statement
 * 
 * 4     1/21/99 6:07p Glen
 * Get ready to run on VxWorks
 * 
 * 3     1/18/99 8:52a Glen
 * Initial bringup
 * 
 * 2     1/14/99 2:06p Glen
 * Integrate port client (step1)
 * 
 * 1     1/14/99 10:31a Glen
 * 
 * 1     1/14/99 9:13a Glen
 * 
 * 2     1/13/99 9:55a Darrelld
 * Complete for testing
 * 
 * 1     1/12/99 5:03p Darrelld
 * 
 * 
 */
////////////////////////////////////////////////////////////////////////////////////////

#include <LtDriver.h>
#include <LtRouter.h>
#include <LtPktAllocator.h>

#include <assert.h>

// Forward classes
class LtIpPortClient;

class LtpcPktAllocator : public LtPktAllocator
{
private:
	LtIpPortClient* m_pClient;
public:
	void registerClient(LtIpPortClient* pClient) { m_pClient = pClient; }
};

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//  Class LtLpPortLink
//
// This class is the interface between a stack and the LtIpPortClient object.
//

class LtIpPortLink : public LtLink, public LtObject
{
	// allow access by port client
public:
	LtNetwork*			m_pNet;			// Our client stack is known to us as a LtNetwork
	LtIpPortClient*		m_pPortClient;	// Our master port client
	boolean				m_bProtAnalMode;
	LtServicePinState	m_nPinState;
	boolean				m_aRegsReq[8];	// registers we've requested
	LtCommParams		m_commParams;	// client comm params
	LtLink*		getLink();

public:
	LtIpPortLink();
	virtual ~LtIpPortLink() {};

	void destroyInstance() { delete this; }

	void	setPortClient( LtIpPortClient*	pPortClient )
	{
		assert( m_pPortClient == NULL );
		m_pPortClient = pPortClient;
	}

	//
	// The following interfaces are required as an LtLink:
	//

	// The following commentary is shorthand for the actions of the routines.
	// assert(false)			The method is not supported and fails immediately if called.
	// No Action				Return success, do nothing.
	// Instance					Store / retrieve an instance variable per LtIpPortLink object.
	// PassThrough				Pass this call through without synchronization or other processing
	//							to the LtLink port object.
	// 
	virtual boolean enumInterfaces( int idx, LPSTR pNameRtn, int nMaxSize )
	{	assert(false);
		return false;
	}
	virtual LtSts open( const char* pName )
	{	assert(false);
		return LTSTS_ERROR;
	}
	virtual boolean		isOpen()
	{	assert(false);
		return false;
	}
	virtual LtSts close()
	{	assert(false);
		return LTSTS_ERROR;
	}
	// Instance
	virtual LtSts registerNetwork(LtNetwork& net)
	{	assert(m_pNet == NULL );
		m_pNet = &net;
		return LTSTS_OK;
	}
	// No Action
	virtual LtSts setReceivePriority(int priority)
	{	return LTSTS_OK;
	}
	virtual void getMaxQueueDepth( int* pnReceiveQueueDepth, int* pnTransmitQueueDepth )
	{	assert(false);
	}
	virtual LtSts setQueueDepths( int nReceiveQueueDepth, int nTransmitQueueDepth )
	{	assert(false);
		return LTSTS_ERROR;
	}
	virtual LtSts sendPacket(void* referenceId,
					int nPrioritySlot,
					byte flags,
					byte* pData,
					int nDataLength,
					boolean bPriority)
	{	assert(false);
		return LTSTS_ERROR;

	}
	virtual LtSts queueReceive( void* referenceId,
						boolean bPriority,
						byte flags,
						byte* pData,
						int nMaxLength)
	{	assert(false);
		return LTSTS_ERROR;
	}
	// Special action required.
	virtual void reset();

	// Passthrough
	virtual int getStandardTransceiverId()
	{	return getLink()->getStandardTransceiverId();
	}
	// pass through
	virtual boolean getUniqueId(LtUniqueId& uniqueId)
	{	return getLink()->getUniqueId( uniqueId);
	}
	// No Action
	virtual void flush(boolean on) {};
	// No Action
	virtual void terminate() {};
	// Special action
	virtual LtSts setCommParams(const LtCommParams& commParams);
	// Special action
	virtual LtSts getCommParams( LtCommParams& commParams);

	// Special handling
	// Scoreboard the callbacks under a port client lock.
	virtual LtSts getTransceiverRegister(int n);

	// Pass through
	virtual void getStatistics(LtLinkStats& stats)
	{	getLink()->getStatistics( stats );
	}
	virtual void getStatistics(LtLinkStats *&pStats)
	{	getLink()->getStatistics( pStats );
	}
	// Pass through
	virtual void clearStatistics()
	{	getLink()->clearStatistics();
	}
	// Special handling
	// On = 1, Blink = 2, Off = 3
	virtual void setServicePinState(LtServicePinState state);

	// Special handling
	// On = 1, off = 2
	virtual void setProtocolAnalyzerMode(boolean on);
	virtual boolean getProtocolAnalyzerMode();

	virtual void setLoopbackMode(boolean on)
	{	assert(false);
	}
	virtual boolean getLoopbackMode()
	{	assert(false);
		return false;
	}
	virtual LtSts selfTest()
	{	assert(false);
		return LTSTS_ERROR;
	}
	// Pass through
	virtual LtSts reportPowerSelfTest()
	{	return getLink()->reportPowerSelfTest();
	}
	virtual void setCurrentBacklog(int backlog)
	{	assert(false);
	}
	virtual int getCurrentBacklog()
	{	assert(false);
		return 0;
	}
	virtual void setWindowSize(int windowSize)
	{	assert(false);
	}
	virtual int getWindowSize()
	{	assert(false);
		return 0;
	}
	virtual void setTransmitSlot(int transmitSlot)
	{	assert(false);
	}
	virtual int getTransmitSlot()
	{	assert(false);
		return 0;
	}

	virtual LtSts getFrequency(int& frequency)
	{
		return getLink()->getFrequency(frequency);
	}

    // Get and set network buffers.  Links that do not have network buffers can do nothing.
    virtual LtSts getNetworkBuffers(LtReadOnlyData& readOnlyData)
    {
        return getLink()->getNetworkBuffers(readOnlyData);
    }
	virtual LtSts setNetworkBuffers(LtReadOnlyData& readOnlyData) 
    {
        return getLink()->setNetworkBuffers(readOnlyData);
    }
};

//typedef LtTypedVVector<LtIpPortLink> LtIpPortLinkVector;
typedef LtTypedVector<LtIpPortLink> LtIpPortLinkVector;

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//  Class LtLpPortClient
//
// This class fronts as a Link client for stacks to perform link maintenance functions
// and fronts as a Client to move packets between the link and the engine.

class LtIpPortClient : public LtNetwork, public LtLreClientBase, public LtMsgAllocOwner
{
protected:
	LtLink*				m_pLink;
	SEM_ID				m_sem;				// This is a "binary semaphore"
	SEM_ID				m_semReset;			// This is too.
	LtLreServer*		m_pServer;			// Our server
	LtIpPortLinkVector	m_vNets;			// network clients
	boolean				m_bResetRequested; // A reset has been requested

	// Identity as an LtLreClient
    char		   *m_name;		            // used in start for open to link
	boolean			m_bLreClientActive;		// are we active as an LRE client?
	boolean			m_bQueueReceivesNeeded;	// need to try to fill the receive queues
	int				m_nReceiveQueueDepth;	// How many receives to pend?
	int				m_nTransmitQueueDepth;	// How many transmits to buffer in driver?
	LtpcPktAllocator*	m_pAlloc;
	boolean			m_bProtocolAnalyserMode; // Protocol analyser mode
	LtServicePinState m_nServicePinState;	// service pin state
	boolean			m_aRegsReq[8];			// registers we've requested
	LtCommParams	m_commParams;			// master comm params
	boolean			m_bGotCommParams;
	boolean			m_bResetting;

	LtServicePinState m_ipServiceState;

	// Lock this object services
	void	lock();
	void	unlock();

	// Special lock for resets - can't use above locks because otherwise
	// we deadlock on lock acquisition in callbacks.
	void	resetLock();
	void    resetUnlock();

public:
	LtIpPortClient( LtChannel *pChannel );

	virtual ~LtIpPortClient();

	// Called by LtLreServer.
	// Pass packet to LtLink
	virtual void processPacket(boolean bPriority, LtPktInfo *pPkt);

	// Used to start up or stop the client.  true is returned if the function
	// succeeded.
	// For the LtIpPortClient
	// Need to open or close the link as well.
	virtual boolean start();
	virtual boolean stop();

	virtual boolean getRoute(int& index, LtRoutingMap *pRoute);

	// Identity as an LtLreClient

public:

	virtual void		setAllocator( LtpcPktAllocator* pAlloc )
	{	m_pAlloc = pAlloc;
		m_pAlloc->registerClient(this);
	}
	virtual void		setServer( LtLreServer* pServer )
	{	m_pServer = pServer;
	}
	virtual void		setTransmitQueueDepth( int nDepth )
	{	m_nTransmitQueueDepth = nDepth;
	}
	virtual void		setReceiveQueueDepth( int nDepth )
	{	m_nReceiveQueueDepth = nDepth;
	}
	void	setName(const char*	pName );
    char* getName()			{ return m_name; }

	// LtNetwork Methods

	// Access link for a passthrough operation
	// Also, register this object as the network for that link.
	// For some unknown reason, calling this registerLink causes run time crash!
	virtual void		registerLink( LtLink& link );
	LtLink*		getLink()
	{	return m_pLink;
	}
	// packet received from LtLink.
	// send it through the LreServer to be routed.
	void packetReceived(void* referenceId,
							int nLengthReceived,
							boolean bPriority,
							int receivedSlot,
							boolean isValidLtPacket, 
							byte l2PacketType, 
							LtSts sts,
							byte ssiReg1=0,
							byte ssiReg2=0);
	// packet send complete to the link.
	// release the packet.
	void packetComplete(void* referenceId, LtSts sts );

	// IO related members
	void	queueReceives();

	void	resetAndSetCommParams(boolean bRequeue=true);

	virtual void reportTransceiverRegister(int n, int value, LtSts sts);

	void servicePinDepressed();
    void servicePinReleased();

	void flushCompleted();
	void resetRequested();
	void terminateCompleted();

	// Interface for special handling with LtIpPortLink class

	LtIpPortLink*	registerClient( LtNetwork* pNetClient );
	boolean			removeClient( LtNetwork* pNetClient );

	void	reset( LtIpPortLink* pPortLink );
	LtSts	setCommParams( LtCommParams& commParams );
	LtSts	getCommParams( LtIpPortLink* pPL, LtCommParams& commParams );
	LtSts	getTransceiverRegister( LtIpPortLink* pPL, int n );

	void	setServicePinState( LtIpPortLink* pPL, LtServicePinState state );
	void	setProtocolAnalyzerMode( LtIpPortLink* pPL, boolean on );
	boolean getProtocolAnalyzerMode( LtIpPortLink* pPL );

	virtual boolean masterRelease(LtMsgRef* pMsg);

	static int	VXLCDECL flickerTask( int obj, ... );
	void wink();

	void setServicePinStateSpecial(LtServicePinState state);

#ifdef WIN32
    void registerLdvHandle(int ldvHandle);
#endif

    // The LtIpPortClient should pass all layer 2 packets, even if they do not look
    // like valid packets.  This way we can pass other protocols.
    boolean getNeedAllLayer2Packets() { return TRUE; }
};



#endif // _LTIPPORTCLIENT_H

