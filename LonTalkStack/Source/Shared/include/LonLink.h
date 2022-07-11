/***************************************************************
 *  Filename: LonLink.h
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
 *  Description:  Header file for LonLink LonTalk Link class
 *
 *	DJ Duffy Nov 1998
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Shared/include/LonLink.h#2 $
//
/*
 * $Log: /VNIstack/Dcx_Dev/Shared/include/LonLink.h $
 * 
 * 25    6/24/08 7:54a Glen
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
 * 24    7/15/05 11:23a Bobw
 * EPR 35786
 * Change LonLinkWin to support the layer 2 mip to be pre-opened.  The
 * VniServer opens the device to determine its mip type (determinMipType).
 * Prior to this change, the VniServer opened the device, and if was a
 * layer 2 mip, closed it, so that lonLinkWin could re-open it.  Now
 * lonLinkWin just uses the handle opened previously (if set).  Note that
 * there are other changes to determinMipType necessary to complete this
 * EPR.
 * 
 * 23    4/12/05 9:45a Bobw
 * EPR 36422
 * Support LDV attachment events for layer 2 mips as well as layer 5 mips.
 * 
 * 22    1/10/05 1:56p Fremont
 * Reorgainize for platform-specific functions, add debug code
 * 
 * 21    10/14/04 3:55p Vprashant
 * EPRS FIXED: 34985
 * 
 * 20    10/04/04 3:14p Vprashant
 * moved sendToProtocolAnalyser from LonLinkILon to LonLink
 * 
 * 19    4/23/04 5:25p Bobw
 * Keep track of whether or not read only data has been updated.  Update
 * network buffers only if it has been.  If it hasn't, then refresh the
 * read only data from the driver's cache.  Otherwise we have the
 * situation that stack A sets the read only data and updates the network
 * buffers, then stack B resets, but since it still has stale read only
 * data it sets them back to the old values.  Note that the read only data
 * of a stack will still not be updated with the latest network buffers
 * until it resets - but this seems OK.
 * 
 * 18    4/23/04 4:16p Bobw
 * EPR 32997
 * On Layer2 mip, maintain a cache of network buffers.  On startup, read
 * the buffers from the mip (if they have not already been read), and
 * update the stacks read-only data to reflect the network buffers (but
 * not app buffers).  On reseting a stack, check to see if the buffers in
 * read only data differ from those in the LONLINK cache, and if so, write
 * them back out to the mip.
 * 
 * 17    6/23/03 11:09a Glen
 * Development related to supporting NES devices on top of the new Power
 * Line SLTA.  This includes making phase detection and bi-directional
 * query status work.  Also, changed stack such that if a transceiver ID
 * is unknown, the comm params are left unchanged.
 * 
 * 16    6/14/02 5:59p Fremont
 * remove warnings
 * 
 * 15    4/19/02 11:09a Vprashant
 * 
 * 14    4/06/02 3:40p Vprashant
 * added retransmit routine
 * 
 * 13    11/07/01 3:35p Fremont
 * provide for receive task priority override
 * 
 * 12    11/06/01 2:38p Fremont
 * Add link receive task name override
 * 
 * 11    11/06/01 9:25a Glen
 * Need control of zero crossing synchronization and attenuation for
 * LonTalk Validator.  Added these flags to sendPacket().  This propagated
 * to lots of places.  Also added control options to LtMsgOut.
 * 
 * 10    10/24/01 12:09p Fremont
 * make platform independent
 * 
 * 9     1/24/00 10:38a Glen
 * Implement setting comm params and retrieve xcvr reg.
 * 
 * 7     2/16/99 3:03p Glen
 * Driver unification
 * 
 * 6     2/16/99 11:09a Glen
 * Add LtVxWorks.h
 * 
 * 5     1/28/99 11:02a Darrelld
 * Support transceiver register fetch for testing
 * 
 * 4     12/15/98 4:33p Darrelld
 * Reorganize base classes to make LtLink pure and virtual as the driven
 * snow.
 * 
 * 3     12/15/98 11:45a Darrelld
 * check in to accomplish move
 * 
 * 2     12/02/98 4:59p Darrelld
 * Check-in updates
 * 
 * 1     11/30/98 4:55p Darrelld
 * LonLink class for use on NT
 * 
 */
//////////////////////////////////////////////////////////////////////

#if !defined(LONLINK_H)
#define LONLINK_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "LtLinkBase.h"
#include <RefQues.h>
#include <wdLib.h>
#include "LtVxWorks.h"
#include "VxClass.h"
#include "LonTalk.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Debug code
#ifdef DEBUG_LONLINK
	extern "C" void LonLinkGetBuffers();
	extern "C" void LonLinkSetComm();
	extern "C" void LonLinkQueryStatus();
	extern "C" void LonLinkSetPhase();
	extern "C" void LonLinkResetMip();
	extern "C" void LonLinkShowQueues();
#endif

class LonLink : public LtLinkBase  
{
public:
	LonLink();
	virtual ~LonLink();

	// Open the interface for use
	// Call with a name returned by enumInterfaces
	LtSts open( const char* pName );

	// Close the currently open interface
	LtSts close();

	LtSts sendPacket(void* referenceId,
					int nPrioritySlot,
					byte flags,
					byte* pData,
					int nDataLength,
					boolean bPriority);
	virtual LtSts getTransceiverRegister(int n);
	virtual	void  reset();
	virtual LtSts setCommParams(const LtCommParams& commParams);
	virtual LtSts getCommParams(LtCommParams& commParams);
	virtual LtSts getFrequency(int& frequency);
	virtual LtSts getNetworkBuffers(LtReadOnlyData& readOnlyData);
	virtual LtSts setNetworkBuffers(LtReadOnlyData& readOnlyData);
	virtual LtSts getNeuronId(byte* neuronId);

	virtual bool interfaceEnabled()	{ return true; }

	static  void  setPerfectXcvrReg(byte* pData, int len, bool bAlternatePath);

#ifdef WIN32
    virtual void registerLdvHandle(int ldvHandle) 
    {
    };
#endif

protected:

	virtual LtSts	tryTransmit( void* refId, byte flags, byte* pData, int nDataLength );
	virtual void	receiveTask();
	virtual const char*	getRcvTaskName() { return("lonLinkRcv"); }
	virtual int		getRcvTaskPriority() { return(LINK_RCV_TASK_PRIORITY); }

	boolean			m_bDelayedRetransmitPending;		// transmit timer running

	// Platform-specific driver functions
	virtual LtSts driverOpen(const char* pName) = 0;
	virtual void driverClose() = 0;
	virtual LtSts driverRead(void *pData, short len) = 0;
	virtual LtSts driverWrite(void *pData, short len) = 0;
	virtual LtSts driverRegisterEvent() = 0;
	virtual void driverReceiveEvent() = 0;
	virtual void setPhaseMode(void);
	virtual void sendToProtocolAnalyser(byte* pData, bool crcIncluded = true) {}

	VxcSignal m_semLocalResponse;
	VxcLock   m_lockLocal;
	SEM_ID		_semWaitForRcvTask;

	int    m_localRespLen;
	byte   m_localResponse[MAX_APDU_SIZE];
	LtCommParams m_commParams;
    LtReadOnlyData m_bufferConfiguration;  // This caches the buffer portion of the read only data;
    bool   m_buffersInSync;
	int	   m_frequency;
	byte   m_lastXcvrReg[LT_NUM_REGS];
	int    m_lastRxCount;
	bool   m_bClearOneAndOnlyOne;

    LtSts readBuffers();
    LtSts writeBuffers();
	LtSts localCommand(int cmd, byte* pData, int len, byte* pResp, int &respLen);
	void  clearOneAndOnlyOne(void);

// Debug code
#ifdef DEBUG_LONLINK
	friend void LonLinkGetBuffers();
	friend void LonLinkSetComm();
	friend void LonLinkQueryStatus();
	friend void LonLinkSetPhase();
	friend void LonLinkResetMip();
	friend void LonLinkShowQueues();

	void testQueryStatus();
	void resetMip();
#endif
};

#endif // !defined(LONLINK_H)
