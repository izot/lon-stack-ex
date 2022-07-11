/***************************************************************
 *  Filename: LonLink.cpp
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
 *  Description:  Implementation of the LonLink class.
 *		This is a base class for native LonTalk-side LtLink drivers
 *		talking to a MIP.
 *		Renamed from LonLinkWin.cpp to become generic.
 *
 *	DJ Duffy Nov 1998
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Shared/LonLink.cpp#3 $
//
/*
 * $Log: /DCX/Dev/dc3/dcd/EVNI/Shared/LonLink.cpp $
 * 
 * 1     12/05/07 11:15p Mwang
 * 
 * 52    8/09/07 5:08p Bobw
 * EPR 44789
 * 
 * 51    8/01/07 5:50p Fremont
 * EPR 45753 - clearing the network statistics of an internal device
 * cleared the console linkstats display, which was intended to show the
 * external packet statistics. In standalone mode on the iLON, this
 * happened automatically on any internal device commission. Solution:
 * keep a shadow shadow copy of the statistics that were actually shared,
 * increment them in the driver, and read/clear the shadow copies when
 * accessing the internal device.
 * 
 * 50    8/01/07 5:41p Fremont
 * EPR 43171 - create mechanism so that LonScanner can see internal
 * packets on the iLON. Must set the flag 'sendInternalPacketsToPort'
 * 
 * 49    5/20/05 10:45a Fremont
 * 
 * 48    4/01/05 10:41a Fremont
 * more diag code
 * 
 * 47    1/10/05 1:49p Fremont
 * Reorgainize for platform-specific functions, add debug code
 * 
 * 46    10/14/04 3:58p Vprashant
 * EPRS FIXED: 34985
 * 
 * 45    10/04/04 3:15p Vprashant
 * Protocol Analyser related changes
 * 
 * 44    5/04/04 1:48p Bobw
 * Resynch network buffers after comm reset, since a comm reset may reboot
 * the mip.
 * 
 * 43    4/28/04 6:49p Bobw
 * EPR 32593
 * Support VNI based protocol analyzer.  Pass bad packets up, and time
 * stamp them at first oportunity.  
 * 
 * 42    4/23/04 5:25p Bobw
 * Keep track of whether or not read only data has been updated.  Update
 * network buffers only if it has been.  If it hasn't, then refresh the
 * read only data from the driver's cache.  Otherwise we have the
 * situation that stack A sets the read only data and updates the network
 * buffers, then stack B resets, but since it still has stale read only
 * data it sets them back to the old values.  Note that the read only data
 * of a stack will still not be updated with the latest network buffers
 * until it resets - but this seems OK.
 * 
 * 41    4/23/04 4:16p Bobw
 * EPR 32997
 * On Layer2 mip, maintain a cache of network buffers.  On startup, read
 * the buffers from the mip (if they have not already been read), and
 * update the stacks read-only data to reflect the network buffers (but
 * not app buffers).  On reseting a stack, check to see if the buffers in
 * read only data differ from those in the LONLINK cache, and if so, write
 * them back out to the mip.
 * 
 * 40    2/02/04 6:43p Bobw
 * EPR 27738
 * 
 * Save the LonLink m_commParams when the comm parms are being sent to the
 * device - otherwise the cache in LonLink is stale, which of course
 * defeats the purpose of the cache, and also prevents changing the data
 * back to the original values.
 * 
 * 39    12/23/03 2:09p Fremont
 * EPR 30901 - bogus CRC error msg due to processing a new link msg as a
 * LonTalk packet
 * 
 * 38    6/30/03 5:20p Fremont
 * fix slow timeout for comm params
 * 
 * 37    6/23/03 11:09a Glen
 * Development related to supporting NES devices on top of the new Power
 * Line SLTA.  This includes making phase detection and bi-directional
 * query status work.  Also, changed stack such that if a transceiver ID
 * is unknown, the comm params are left unchanged.
 * 
 * 36    3/18/03 4:25p Fremont
 * fix warning
 * 
 * 35    6/14/02 5:59p Fremont
 * remove warnings
 * 
 * 34    5/21/02 1:48p Fremont
 * Fix an assert in the simulator
 * 
 * 33    4/19/02 11:06a Vprashant
 * replaced taskDelay by semaphore synchronization
 * 
 * 32    4/06/02 3:40p Vprashant
 * added retransmit routine
 * 
 * 31    2/26/02 5:39p Glen
 * tryTransmit no longer releases packets.  This is only done by the timer
 * routine or the caller of sendPacket.
 * 
 * 30    1/15/02 1:35p Vprashant
 * introduced a delay of 2 ticks to allow the receiveTask to startup
 * fully.
 * 
 * 29    1/04/02 4:48p Glen
 * Remove warning
 * 
 * 28    12/21/01 12:14p Vprashant
 * 
 * 27    12/21/01 11:59a Vprashant
 * commented out calls to driverRead which occur before an event is
 * received
 * 
 * 26    12/14/01 10:56a Fremont
 * Add some comments about codes returned by MIP
 * 
 * 25    11/07/01 10:32a Fremont
 * rename m_flags to m_pktFlags to avoid conflict with stupid Tornado
 * incude file (mbuf.h)
 * 
 * 24    11/06/01 9:25a Glen
 * Need control of zero crossing synchronization and attenuation for
 * LonTalk Validator.  Added these flags to sendPacket().  This propagated
 * to lots of places.  Also added control options to LtMsgOut.
 * 
 * 23    11/02/01 1:18p Fremont
 * obsolete the Pentagon LONC port packet overhead
 * 
 * 22    10/24/01 12:09p Fremont
 * make platform independent
 * 
 * 21    12/05/00 7:52p Bobw
 * Fix lockup that occurs if there are messages being processed by the
 * recieve thread at the time that the channel is shut down.
 * 
 * 20    10/10/00 2:17p Darrelld
 * Fix thread rundown
 * 
 * 19    3/14/00 9:15a Darrelld
 * Fix enumerator for operation on PCs
 * 
 * 18    1/24/00 2:30p Glen
 * Latest fix lost setting of m_bActive when setting comm params
 * 
 * 17    1/24/00 10:38a Glen
 * Implement setting comm params and retrieve xcvr reg.
 * 
 * 16    1/05/00 4:32p Glen
 * EPRS FIXED: 
 * 
 * 14    7/30/99 9:03a Darrelld
 * Cleanup and streamline
 * 
 * 13    6/18/99 2:52p Darrelld
 * Allow testing on PCs with no Lon hardware
 * 
 * 12    2/22/99 9:36a Darrelld
 * Adjust for changes in base class that no longer create receive event.
 * 
 * 11    2/16/99 3:02p Glen
 * Remove msToTicks; driver unification
 * 
 * 10    2/05/99 2:06p Darrelld
 * simulate LONC header overwrite
 * 
 * 9     2/05/99 1:54p Darrelld
 * increase flush limit on open
 * 
 * 8     2/05/99 8:57a Glen
 * 
 * 7     1/28/99 11:03a Darrelld
 * Support transceiver register fetch for testing
 * 
 * 6     1/21/99 6:07p Glen
 * Get ready to run on VxWorks
 * 
 * 5     1/15/99 1:45p Glen
 * 
 * 4     12/15/98 4:33p Darrelld
 * Reorganize base classes to make LtLink pure and virtual as the driven
 * snow.
 * 
 * 3     12/03/98 12:56p Darrelld
 * Commentary and return CRC with data on receive.
 * 
 * 2     12/02/98 4:59p Darrelld
 * Check-in updates
 * 
 * 1     11/30/98 4:55p Darrelld
 * LonLink class for use on NT
 * 
 */
//
//////////////////////////////////////////////////////////////////////


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <VxWorks.h>
#include "LonTalk.h"
#include "VxlTypes.h"
#include "LtXcvrId.h"

#include <LtMip.h>
#include <LtNetworkManagerLit.h>
#include <time.h>
#include "LtCUtil.h"

// Define this to activate test code in this file and in LonLink.h
//#define DEBUG_LONLINK
#include "LonLink.h"

/*
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
*/

//////////////////////////////////////////////////////////////////////
// LOCAL DATA
//////////////////////////////////////////////////////////////////////


//		Layer 2 Msg:
//		byte[0]: 0x12
//		byte[1]: number of bytes to follow (N)
//		byte[2..N+1]: LonTalk packet contents less CRC 
//		(e.g., 0x00, 0x30, 0x01, 0x82, 0x03, 0x51) is a broadcast unackd query status
//		 from node 1/2 on domain length 0. )

//		The byte array received is as follows:
//		byte[0]: 0x1a
//		byte[1]: number of bytes to follow (N)
//		byte[2..N+1]: LonTalk packet including CRC


// Debug code
// The #define for this must come before the LonLink.h header file (see above).
#ifdef DEBUG_LONLINK

// Note: you can 'reboot' a Neuron by writing 0 as the second byte of the comm params.
// This will cause a reload of EEPROM (xcvr and buffer config) from ROM.

extern "C"
{
LonLink* g_pLonLink = NULL;
void LonLinkGetBuffers()
{
	LtReadOnlyData readOnlyData;

	if (g_pLonLink != NULL)
	{
		// make it look like we are not in sync
		g_pLonLink->m_buffersInSync = false;
		if (g_pLonLink->getNetworkBuffers(readOnlyData) == LTSTS_OK)
		{
			// The above call reads all the buffer data, but only copies the net stuff.
			// Copy the cached data to get it all.
			readOnlyData = g_pLonLink->m_bufferConfiguration;
			printf("MIP Buffer Config:\n");
			printf("App - in size: %d, out size: %d, in num: %d, out num: %d, out-pri num: %d\n",
					readOnlyData.getAppInBufSize(), 
					readOnlyData.getAppOutBufSize(),
					readOnlyData.getNumAppInBufs(),
					readOnlyData.getNumAppOutBufs(0),
					readOnlyData.getNumAppOutBufs(1));

			printf("Net - in size: %d, out size: %d, in num: %d, out num: %d, out-pri num: %d\n", 
					readOnlyData.getNetInBufSize(), 
					readOnlyData.getNetOutBufSize(),
					readOnlyData.getNumNetInBufs(), 
					readOnlyData.getNumNetOutBufs(0), 
					readOnlyData.getNumNetOutBufs(1)); 
		}
	}
}
LtCommParams LonLinkCommParmsCopy;
bool LonLinkCommParmsInit = false;
void LonLinkSetComm()
{
	if (g_pLonLink != NULL)
	{
		if (!LonLinkCommParmsInit)
		{
			// copy correct comm parms
			memcpy(LonLinkCommParmsCopy.m_nData, g_pLonLink->m_commParams.m_nData, 
				sizeof(LonLinkCommParmsCopy.m_nData));
			LonLinkCommParmsInit = true;
		}
		// make it look like we are changing them, even if we aren't
		g_pLonLink->m_commParams.m_nData[0] = LonLinkCommParmsCopy.m_nData[0] ^ 0xff; 
		g_pLonLink->setCommParams(LonLinkCommParmsCopy);
	}
}

#ifndef WIN32
#include "usrLib.h"
void LonLinkModifySavedCommParms()
{
	m(&LonLinkCommParmsCopy, 1);
}
#endif
void LonLinkGetComm()
{
	if (g_pLonLink != NULL)
	{
		if (g_pLonLink->getCommParams(LonLinkCommParmsCopy) == LTSTS_OK)
		{
			LonLinkCommParmsInit = true;
			for (int i = 0; i < (int)sizeof(LonLinkCommParmsCopy.m_nData); i++)
			{
				printf("%02X ", LonLinkCommParmsCopy.m_nData[i]);
				if (((i+1) % 8) == 0)
					printf("\n");
			}
		}
		else
			printf("Failed!\n");
	}
}
void LonLinkQueryStatus()
{
	if (g_pLonLink != NULL)
	{
		g_pLonLink->testQueryStatus();
	}
}
void LonLinkSetPhase()
{
	if (g_pLonLink != NULL)
	{
		g_pLonLink->setPhaseMode();
	}
}
void LonLinkResetMip()
{
	if (g_pLonLink != NULL)
	{
		g_pLonLink->resetMip();
	}
}
void LonLinkShowQueues()
{
	printf("Transmit queue: current %d, fills up to %d\n", g_pLonLink->m_qTransmit.getCount(), 
		g_pLonLink->m_nTransmitQueueDepth);
	printf("Standard receive queue: current %d, empties down from %d\n", g_pLonLink->m_qReceive.getCount(), 
		g_pLonLink->m_nReceiveQueueDepth);
	printf("Priority receive queue: current %d, empties down from %d\n", g_pLonLink->m_qReceiveP.getCount(), 
		g_pLonLink->m_nReceiveQueueDepth);
	printf("Free queue: current %d, fills up to %d\n", g_pLonLink->m_qFreePkts.getCount(), 20);
}

int LonLinkSetGetDelay = 0;
void LonLinkSetCommGetBuff()
{
	LonLinkSetComm();
	taskDelay(LonLinkSetGetDelay);
	LonLinkGetBuffers();
}

} // extern "C"
#endif // DEBUG_LONLINK

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


LonLink::LonLink()
{
	m_bDelayedRetransmitPending = false;
	m_frequency = -1;
	m_bClearOneAndOnlyOne = false;
	m_lastRxCount = 0;

	m_localRespLen = 0;

    m_buffersInSync = 0;
}

LonLink::~LonLink()
{

}
//
// open
//
// Open the interface for use
// Call with a name returned by enumInterfaces
//

LtSts LonLink::open( const char* pName )
{
	LtSts	sts = LTSTS_OPENFAILURE;

	_semWaitForRcvTask = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
	lock();
	close();
	clearStatistics();

#ifdef DEBUG_LONLINK
	g_pLonLink = this;
#endif

	sts = driverOpen(pName);
	if (sts == LTSTS_OK)
	{	
		driverRegisterEvent();
		// Clear the receive buffer of any stale messages.  In case
		// stuff is flying in, bail out after a while.
		for (int i=0; i<500; i++)
		{
			byte data[256];
			// changed VP -> corrected position of braces
			if (driverRead(data, sizeof(data)) != LTSTS_OK)
			{
				break;
			}
		}
		// Turn on phase mode
		setPhaseMode();
	}
	startReceiveTask();
	unlock();
	/*
	 * At this point we have to let the receiveTask started in 
	 * startReceiveTask() come up and start waiting for data.
	 * Removed the earlier taskDelay() mechanism and synchronized
	 * with a semaphore
	 */
	if (semTake(_semWaitForRcvTask,WAIT_FOREVER) == ERROR)
		return LTSTS_OPENFAILURE;

	semDelete(_semWaitForRcvTask);

	// Used to query CommParams here; but on ECN we can start the VNI proxy even though
	// the PLCA interface is powered down, in which case the command hangs. We now wait
	// until we know the NI is up before querying the CPs.

	return sts;
}

//
// close
//
// Close the currently open interface
//
LtSts LonLink::close()
{
	if ( isOpen() )
	{
		stopReceiveTask();
		reset();
		driverClose();
	}
	return LTSTS_OK;
}

//
// tryTransmit
//
// Attempt to transmit a packet. Return status
//
LtSts		LonLink::tryTransmit( void* refId, byte flags, byte* pData, int nDataLength )
{
	byte	buf[MAX_LPDU_SIZE+10];
	int		nLen = 0;	// init to remove warning
	LtSts	sts = LTSTS_INVALIDSTATE;
	LtSicb* pSicb;
	bool	dontTransmit = ((flags & MI_DONT_TRANSMIT) != 0);

	if ( isOpen() )
	{
		// Pentagon compatibility code: obsolete
		// Emulate LONC which writes into buffer header
		// *(pData-LONC_OVERHEAD+1) = 0xa5;
        int dataOffset = 2;

		pSicb = (LtSicb*)buf;

		pSicb->cmd = MI_COMM;

		// If this is a priority packet, send it to the priority queue
		if (*pData & 0x80)
		{
			pSicb->que = MI_TQP;
		}
		else
		{
			pSicb->que = MI_TQ;
		}
		// Include any flags specified by the user
		pSicb->que |= (flags & MI_L2_FLAGS);

        nLen = nDataLength;
        if (nDataLength >= L2_PKT_LEN_EXTENDED)
        {
            // Need to use extended length
            pSicb->len = L2_PKT_LEN_EXTENDED;
            // Extended length is in host order.
            if (nDataLength > MAX_LPDU_SIZE)
            {
                // Too big, need to truncate
                nDataLength = MAX_LPDU_SIZE;
            }
            memcpy(&buf[dataOffset], &nDataLength, 2);
            dataOffset += 2;
        }
        else
        {
		    pSicb->len = (byte)nLen;
        }
		// The reset of the SICB is passed in
		memcpy( &buf[dataOffset], pData, nLen );

		// This flag may be passed in just to allow sending the packet to the LSPA.
		if (!dontTransmit)
		{
			sts = driverWrite(pSicb, nLen+dataOffset);
			if ((sts != LTSTS_OK) && (sts != LTSTS_QUEUEFULL))
			{
				m_linkStats.m_nTransmissionErrors++;
				m_linkStats.m_nTransmissionErrorsShadow++;
			}
		}
		else
		{
			sts = LTSTS_OK;
		}

		if ( sts == LTSTS_OK )
		{
			// Don't increment statistic if not actually sent
			if (!dontTransmit)
			{
				m_linkStats.m_nTransmittedPackets++;
			}
			// Note: Contents of buf will be modified after this call
			// This shouldn't be a problem as buf is a local copy of pData
			sendToProtocolAnalyser((byte*)pSicb, false); // no CRC
		}
	}

	if ((sts != LTSTS_QUEUEFULL) && !dontTransmit)

	{
		dumpPacket("LonLink - tryTransmit", pData, nLen );
	}

	return sts;
}


//
// sendPacket
//
// Send a packet. Actually, if the timer is running, then queue it.
// if no timer, then try to transmit, and if it fails, then queue and
// start timer.
//
LtSts LonLink::sendPacket(void* referenceId,
				int nPrioritySlot,
				byte flags,
				byte* pData,
				int nDataLength,
				boolean bPriority)
{
	LtSts	sts = LTSTS_INVALIDSTATE;
	lock();
	do
	{
		if ( ! isOpen() || m_pNet == NULL )
		{	sts = LTSTS_INVALIDSTATE;
			break;
		}
		if ( (m_bDelayedRetransmitPending || !m_qTransmit.isEmpty()) && ((flags & MI_DONT_TRANSMIT) == 0))
		{
			sts = LTSTS_QUEUEFULL;	// try queueing
		}
		else
		{
			sts = tryTransmit( referenceId, flags, pData, nDataLength);
		}
		if ((sts == LTSTS_QUEUEFULL) && (m_qTransmit.getCount() < m_nTransmitQueueDepth))
		{
			queueTransmit( referenceId, flags, pData, nDataLength );
			sts = LTSTS_PENDING;
		}
	}
	while (false);
	unlock();
	return sts;
}


//
// receiveTask
//
// The actual task code is in context of our class
// Wait for a receive event, and when it comes along, then
// read a packet and look at it.
// If it's a receive data, then get the data out and send
// it along to the client, if the user has a packet buffer
// queued, else toss it out and wait some more.
//
void LonLink::receiveTask()
{
	byte		data[MAX_LPDU_SIZE+10];
	byte*		pData;
	int			nSize;
    int         packetLength;
	LLPktQue*	pPkt;
	LtQue*		pItem;
	LtSts		sts;
	boolean		bPrior;
	int			nSlot;
	int			dataOffset;
	int			nonDataOverhead;
	boolean		bProcessPkt;
	boolean		validPacket;
	int			ssiReg1 = 0;
	int			ssiReg2 = 0;

	semGive(_semWaitForRcvTask);

	lock();
	while(true)
	{
		int receivedCount = 0;

		unlock();
		driverReceiveEvent();
		if ( m_bExitReceiveTask )
		{	break;
		}
		lock();
		while (true)
		{
			nSlot = 0;
			dataOffset = 2;
			nonDataOverhead = 0;

			if ( m_bExitReceiveTask )
			{	break;
			}
			if ( isOpen() )
			{
				bProcessPkt = false;
				validPacket = false;
				bPrior = false;
				sts = driverRead(data, sizeof(data));

				if (sts != LTSTS_INVALIDSTATE)
				{
					// Some packet has been received uplink.
					// After reading it, send one queued up packet downlink.
					// This is only implemented on the iLON's
					startImmediateRetransmit();
				}
				if (sts != LTSTS_OK)
				{
					// Nothing available - wait for next signal
					if (receivedCount == 0)
					{
						// After receiving a packet and then receiving a packet, we clear the one and only one bit.
						clearOneAndOnlyOne();
					}
					break;
				}

                packetLength = data[1];
                if (packetLength == L2_PKT_LEN_EXTENDED)
                {
                    // Uses extended length.  The driver puts this in host format.
                    memcpy(&packetLength, &data[dataOffset], 2);
                    dataOffset += 2;
                }
				if ( data[0] == L2_PKT_TYPE_INCOMING )
				{
					// This code is for traditional incoming L2 packets
					bProcessPkt = true;
					validPacket = true;
				}
				else if ( data[0] == L2_PKT_TYPE_MODE1_INCOMING || 
						  data[0] == L2_PKT_TYPE_MODE2_INCOMING )
				{
					// This code is for mode 1 and 2 incoming L2 packets.  These packets contain
					// the phase reading byte which we report as a "slot" value.  The "slot"
					// parameter for packet reception was intended to allow future reporting
					// of slot arrival but we never did that so we usurp it here to mean the
					// position relative to the zero crossing.  Mode 2 packets also contain
					// SSI information at the end of the packet.

					// MIP adds 1 set reading to ensure that packet buffer is never marked as 0 state.
					nSlot = data[dataOffset++]-1;
					nonDataOverhead = 1;
					bProcessPkt = true;
					validPacket = true;
					if (data[0] == L2_PKT_TYPE_MODE2_INCOMING)
					{
						// These packets have two bytes of SSI appended so we extract this so that
						// it looks like a regular packet
						nonDataOverhead += 2;
						BYTE* p = &data[packetLength];
						ssiReg1 = p[0];
						ssiReg2 = p[1];
						// We don't need to manage OAOO bit in this case.
						m_bClearOneAndOnlyOne = false;
					}
				}
				else if ( data[0] == L2_PKT_TYPE_LOCAL_NM_RESP )
				{
					// Response to local NM command
					LtSicb* pSicb = (LtSicb*) data;
					m_localRespLen = pSicb->dlen;
					memcpy(m_localResponse, pSicb->data, m_localRespLen);
					m_semLocalResponse.signal();
				}
				else if ( data[0] == L2_PKT_TYPE_FREQUENCY_REPORT )
				{
					// Frequency report.
					m_frequency = data[dataOffset];
				}
				else if ( data[0] == L2_PKT_TYPE_RESET )
				{
					// On any reset: [for PL only]
					// Send a command to the MIP to convert it to MIP mode 1.  This mode supports sending
					// of phase related data with every packet.  This will also result in us receiving an 
					// uplink message containing the power line frequency.
					setPhaseMode();
				}
				else if ( (data[0] & 0xf0) == 0x30 )
				{
					/* Network errors
					Some possible error command bytes returned by L2 MIP

					NI Command byte: 0x3z where z is

					0 : timeout
					1 : CRC error
					2 : packet too long
					3 : preamble too long
					4 : preamble too short
					5 : packet too short
					*/
					m_linkStats.m_nTransmissionErrors++;	
					m_linkStats.m_nTransmissionErrorsShadow++;
					bProcessPkt = true;
				}
				else
				{	// process any stuff we don't understand, but mark as invalid
					bProcessPkt = true;
				}
			}
			else
			{	break;	// not open
			}

			if (!bProcessPkt)
			{
				// Not a packet to be processed below, ignore and try again
				continue;
			}
			if (validPacket)
			{
				// check the priority bit from the packet.
				// MSB of first byte of the packet.
				bPrior = (0 != (data[dataOffset] & 0x80));
				m_linkStats.m_nReceivedPackets++;
			}

			receivedCount++;

			// 'data' is the raw SICB
			sendToProtocolAnalyser(data, true);	// has CRC

			if ( bPrior )
			{	m_linkStats.m_nReceivedPriorityPackets++;
			}

			if ( bPrior?!m_qReceiveP.isEmpty() : !m_qReceive.isEmpty() )
			{
				sts = LTSTS_OK;
				if ( bPrior )
				{	m_qReceiveP.removeHead( &pItem );
				}
				else
				{	m_qReceive.removeHead( &pItem );
				}
				pPkt = (LLPktQue*)pItem;
				pData = pPkt->m_pData;
				nSize = packetLength-nonDataOverhead;

				if ( nSize > pPkt->m_nDataLength )
				{
					// packet is too big for the waiting receive buffer
					// so count a missed packet and then put the receive
					// buffer back on the head of the queue to be used again.
					sts = LTSTS_OVERRUN;
					m_linkStats.m_nMissedPackets++;
					m_linkStats.m_nMissedPacketsShadow++;
					if ( bPrior )
					{	m_qReceiveP.insertHead( pItem );
					}
					else
					{	m_qReceive.insertHead( pItem );
					}
				}
				else if ( pPkt && m_pNet )
				{
					memcpy( pData, &data[dataOffset], nSize );
					dumpPacket("LonLink - receiveTask", pData, nSize );
					unlock();
					// no difference now between PA and normal mode
					m_pNet->packetReceived( pPkt->m_refId, nSize, pPkt->m_bPriority, nSlot, 
											validPacket, data[0], sts, ssiReg1, ssiReg2 );
					lock();
					freeLLPkt(pPkt);
				}
				pPkt = NULL;
				pItem = NULL;
			}
			else
			{	// we didn't have a buffer for this packet
				m_linkStats.m_nMissedPackets++;
				m_linkStats.m_nMissedPacketsShadow++;
			}
			/* 
			 * NOTE:  We should probably break from this while loop
			 * and go to the begining of the parent while loop. This would 
			 * ensure that only upon receiving an event through 
			 * driverReceiveEvent() do we perform a driverRead. In the present 
			 * state it would perform a driverRead and return with an error and
			 * go to the parent loop. This is the behaviour with the ilon100dp.
			 * //break;
			 */
		}
	}
	// Report that we have exited
	m_tidReceive = ERROR;
}

//
// reset
//
// emulate the behaviour of the real link
//
void LonLink::reset()
{
	// put the whole foolish thing under a single lock just the way it should be.
	// LtLinkBase uses a critical section for a lock, so this will work properly.
	//
	lock();
	if ( isOpen() )
	{
		LtLinkBase::reset();
	}
	unlock();
}

// Gets the Nth byte of special purpose mode transceiver status register data
// (7 registers numbered 1-7). 
// If a transceiver register read is requested when the driver is closed,
// reportTransceiverReigister is called with LTSTS_INVALIDSTATE.
// If a transceiver register read is in progress and a reset occurs,
// reportTransceiverRegister can return LTSTS_RESET.
// If a transceiver register read is in progress and 
// another call to getTransceiverRegister is made, 
// this second call would queue the request
// (could simply have a dirty bit for each register, no need to block 
// until the first request completes).

LtSts LonLink::getTransceiverRegister(int n)
{
	LtSts sts = LTSTS_OK;
	int respLen = LT_NUM_REGS;

	// It's expensive to read these and the API reads them one at a time whereas the local
	// NM command reads them all.  So only read when the first one is asked for.  Assumes
	// that upper layers don't request individual register values.
	if (n == 1)
	{
		sts = localCommand(LT_QUERY_XCVR_STATUS, null, 0, m_lastXcvrReg, respLen);
		if (sts == LTSTS_OK)
		{
			// Node has special purpose registers and the app has interest in them.  Start 
			// managing the one and only one bit.
			m_bClearOneAndOnlyOne = true;
		}
	}
	m_pNet->reportTransceiverRegister(n, m_lastXcvrReg[n-1], sts);
	return sts;
}

LtSts LonLink::localCommand(int cmd, byte* pData, int len, byte* pResp, int &respLen)
{
	LtSts sts = isOpen() ? LTSTS_OK : LTSTS_COMMPORTINITFAILURE;
	// Could have response with no data but don't support it now.
	boolean bResponse = respLen != 0;

	if (sts == LTSTS_OK)
	{
		// Lock because we only allow one sync operation at a time.
		m_lockLocal.lock();
		byte buf[50];
		if (respLen)
		{
			memset(pResp, 0, respLen);
		}
		memset(buf, 0, sizeof(buf));
		LtSicb* pSicb = (LtSicb*) buf;
		pSicb->que = 2;
		pSicb->cmd = 2;
		pSicb->tag = 0;
		pSicb->svc = bResponse ? LT_REQUEST : LT_UNACKD;
		pSicb->auth = 1;
		pSicb->data[0] = cmd;
		if (len)
		{
			memcpy(&pSicb->data[1], pData, len);
		}
		// Add one for message code.
		pSicb->dlen = 1 + len;

		// Subtracted 2 from the length (length does not include cmd and len bytes)
		pSicb->len = pSicb->dlen + sizeof(LtSicb) - sizeof(pSicb->data) - 2;
		int count = bResponse ? 300 : 1;
		m_semLocalResponse.wait(0);
		do
		{
			// driverWrite requires the complete frame length which includes the header so add 2.
			driverWrite(buf, pSicb->len+2);
		}
		while (--count && m_semLocalResponse.wait(msToTicks(1000)) == ERROR && interfaceEnabled());

		if (bResponse)
		{
			if (count == 0)
			{
				sts = LTSTS_TIMEOUT;
			}
			else if (!interfaceEnabled() || m_localResponse[0] != NMPASS(cmd))
			{
				sts = LTSTS_ERROR;
			}
			else if (m_localRespLen > 1)
			{
				respLen = min(respLen, m_localRespLen - 1);
				memcpy(pResp, &m_localResponse[1], respLen);
			}
		}
		m_lockLocal.unlock();
	}
	return sts;
}

// Debug code
#ifdef DEBUG_LONLINK
void LonLink::testQueryStatus()
{
	LtSts sts = LTSTS_OK;
	struct QueryStatus
	{
		USHORT xmit_errors;
		USHORT tx_timeouts;
		USHORT rcv_tx_full;
		USHORT lost_msgs;
		USHORT missed_msgs;
		byte reset_cause;
		byte node_state;
		byte version;
		byte error_log;
		byte model_num;
	} resp;
	int respLen = sizeof(resp);

	sts = localCommand(LT_QUERY_STATUS, NULL, 0, (byte*)&resp, respLen);

	if (sts != LTSTS_OK)
		printf("Query Status failed: sts = %#x\n", sts);
	else
	{
		printf("Query Succeeded\n");
		printf("Xmit error: %d\n", resp.xmit_errors);
		printf("TX timeouts: %d\n", resp.tx_timeouts);
		printf("Rcv TX full: %d\n", resp.rcv_tx_full);
		printf("Lost msgs: %d\n", resp.lost_msgs);
		printf("Missed msgs: %d\n", resp.missed_msgs);
		printf("Reset cause: %d (%#x)\n", resp.reset_cause, resp.reset_cause);
		printf("Node state: %#x\n", resp.node_state);
		printf("Version: %d\n", resp.version);
		printf("Error log: %d\n", resp.error_log);
		printf("Model number: %d (%#x)\n", resp.model_num, resp.model_num);
	}

}
void LonLink::resetMip()
{
	byte resetMsg[2];
	resetMsg[0] = 0x50;
	resetMsg[1] = 0x00;
	driverWrite(resetMsg, 2);

}
#endif	// DEBUG_LONLINK


LtSts LonLink::getCommParams(LtCommParams& commParams)
{
	LtSts sts = LTSTS_OK;
	LtReadMemory rmem;
	int respLen = sizeof(m_commParams.m_nData);

	rmem.mode = LT_CONFIG_RELATIVE;
	rmem.addresshi = 0;
	rmem.addresslo = 8;
	rmem.len = sizeof(commParams.m_nData);
	sts = localCommand(LT_READ_MEMORY, (byte*) &rmem, sizeof(rmem),
		m_commParams.m_nData, respLen);

	memcpy(&commParams, &m_commParams, sizeof(commParams));

	return sts;
}

LtSts LonLink::getNeuronId(byte* neuronId)
{
	LtSts sts = LTSTS_OK;
	LtReadMemory rmem;
	int respLen = 6;

	rmem.mode = LT_READ_ONLY_RELATIVE;
	rmem.addresshi = 0;
	rmem.addresslo = 0;
	rmem.len = 6;
	sts = localCommand(LT_READ_MEMORY, (byte*) &rmem, sizeof(rmem),
		neuronId, respLen);

	return sts;
}

LtSts LonLink::setCommParams(const LtCommParams& commParams)
{
	LtSts sts = LTSTS_OK;
	// Set the comm parameters in the network interface.  This is done
	// via a local NM write.  We only write the comm params if we believe
	// they have changed.  This is to avoid writing the Neuron's EEPROM
	// on every start-up.
	m_bActive = true;
	if (memcmp(commParams.m_nData, m_commParams.m_nData, sizeof(m_commParams.m_nData)) != 0)
	{
		LtWriteMemory wmem;
		int len;
		int respLen = 0;

		// In some cases the MIP may have EEPROM lock set.  So, we turn this off first.  No harm done if it is not on.
		wmem.mode = LT_STATS_RELATIVE;
		wmem.addresshi = 0;
		wmem.addresslo = 0x18;	// one byte EEPROM lock follows 12 2-byte stats
		wmem.len = 1;
		wmem.flags = 0;
		wmem.data[0] = 0;
		len = 1 + sizeof(LtWriteMemory) - sizeof(wmem.data) + wmem.len;
		respLen = 0;
		// Just give it our best effort...
		localCommand(LT_WRITE_MEMORY, (byte*)&wmem, len, null, respLen);
		
		wmem.mode = LT_CONFIG_RELATIVE;
		wmem.addresshi = 0;
		wmem.addresslo = 8;
		wmem.len = sizeof(commParams.m_nData);
		wmem.flags = 12; // reset and compute checksum
		memcpy(wmem.data, commParams.m_nData, wmem.len);
		len = 1 + sizeof(LtWriteMemory) - sizeof(wmem.data) + wmem.len;
		respLen = 0;
		sts = localCommand(LT_WRITE_MEMORY, (byte*) &wmem, len, null, respLen);

        if (sts == LTSTS_OK)
        {
            memcpy(m_commParams.m_nData, commParams.m_nData, sizeof(m_commParams.m_nData));
        }
		m_buffersInSync = FALSE;	// May have just rebooted node...
	}
	return sts;
}

LtSts LonLink::readBuffers()
{
	LtSts sts = LTSTS_OK;
	LtReadMemory rmem;
    byte bufferInfo[LT_READ_ONLY_DATA_BUFFER_NUM_BYTES];
	int respLen = sizeof(bufferInfo);

	rmem.mode = LT_READ_ONLY_RELATIVE;
	rmem.addresshi = 0;
	rmem.addresslo = LT_READ_ONLY_DATA_BUFFER_OFFSET;
	rmem.len = LT_READ_ONLY_DATA_BUFFER_NUM_BYTES;
	sts = localCommand(LT_READ_MEMORY, (byte*) &rmem, sizeof(rmem),
		               bufferInfo, respLen);

    if (sts == LTSTS_OK)
    {
        m_bufferConfiguration.fromLonTalk(LT_READ_ONLY_DATA_BUFFER_OFFSET, 
                                          LT_READ_ONLY_DATA_BUFFER_NUM_BYTES, 
                                          bufferInfo);
    }
    m_buffersInSync = (sts == LTSTS_OK);

    return sts;
}

LtSts LonLink::writeBuffers()
{
	LtSts sts = LTSTS_OK;
	LtWriteMemory wmem;
	int len;
	int respLen = 0;
    byte *pData;

    m_bufferConfiguration.toLonTalk(LT_READ_ONLY_DATA_BUFFER_OFFSET, 
                                    LT_READ_ONLY_DATA_BUFFER_NUM_BYTES, &pData);

	wmem.mode = LT_READ_ONLY_RELATIVE;
	wmem.addresshi = 0;
	wmem.addresslo = LT_READ_ONLY_DATA_BUFFER_OFFSET;
	wmem.len = LT_READ_ONLY_DATA_BUFFER_NUM_BYTES;
	wmem.flags = 9; // reset and compute app checksum
	memcpy(wmem.data, pData, wmem.len);
	len = 1 + sizeof(LtWriteMemory) - sizeof(wmem.data) + wmem.len;
	respLen = 0;
	sts = localCommand(LT_WRITE_MEMORY, (byte*) &wmem, len, null, respLen);
    m_buffersInSync = (sts == LTSTS_OK);
    delete[] pData;

	return sts;
}

LtSts LonLink::getNetworkBuffers(LtReadOnlyData& readOnlyData)
{
	LtSts sts = LTSTS_OK;

    if (!m_buffersInSync)
    {   // network buffer cache is stale - resync
        sts = readBuffers();
    }

    if (sts == LTSTS_OK)
    {   // Get network buffers from cache
        readOnlyData.setNetworkBuffers(m_bufferConfiguration);
		readOnlyData.setPendingUpdate(FALSE);
    }
    return sts;
}

LtSts LonLink::setNetworkBuffers(LtReadOnlyData& readOnlyData)
{
	LtSts sts = LTSTS_OK;

    // Write the network buffers if they don't match (or we don't know what the
    // originals are).
    if (!m_buffersInSync || !m_bufferConfiguration.netBuffersMatch(readOnlyData))
    {
        m_bufferConfiguration.setNetworkBuffers(readOnlyData);
        sts = writeBuffers();
		if (sts == LTSTS_OK)
		{
			readOnlyData.setPendingUpdate(FALSE);
		}
    }

    return sts;
}

//
// setPhaseMode
//
// Command 0x40 causes the mode to be set where mode 1 has a phase reading value in each
// packet.  It also results in a frequency report uplink (0x40 uplink) that has the power
// line frequency.  Mode 2 causes two bytes of SSI data to be added to the frame.
// This only has any effect if used with a PL31x0 chip.
void LonLink::setPhaseMode(void)
{
	byte modeData[2];
	modeData[0] = 0x40 | 2;
	modeData[1] = 0x00;
	driverWrite(modeData, 2);
}

LtSts LonLink::getFrequency(int& frequency)
{
	frequency = m_frequency;
	if (m_frequency == -1)
	{
		// This shouldn't be.  Just in case it happened due to a failure of some sort, attempt to 
		// get the frequency again.  This call will fail but future one's might work.
		setPhaseMode();
		frequency = 0;
	}
	return LTSTS_OK;
}

//
// setPerfectXcvrReg
//
// This routine mimics perfect transceiver registers for local communications
//
void LonLink::setPerfectXcvrReg(byte* pData, int len, bool bAlternatePath)
{
	byte perfect[7] = {0x04, 0x00, 0x04, 0xff, 0xff, 0x00, 0x00};
	if (bAlternatePath)
	{
		perfect[2] |= 0x08;
	}
	memcpy(pData, perfect, len);
}

//
// clearOneAndOnlyOne
//
// The Power Line transceiver has the notion of the one and only one bit in the transceiver
// status register.  This bit is set when one and only one packet has been received since the
// last transceiver register read.  This allows you to determine that the data corresponds to
// the last packet received.  It assumes that the register is being read regularly.  To satisfy
// this requirement, we read the register when the registers weren't read by the last message.
// We do this using the PL31x0 form of register read since it is far less obtrusive.  This means
// this doesn't work for the older transceivers.
//
void LonLink::clearOneAndOnlyOne()
{
	if (m_bClearOneAndOnlyOne && m_lastRxCount != m_linkStats.m_nReceivedPackets)
	{
		byte buf[50];
		memset(buf, 0, sizeof(buf));
		LtSicb* pSicb = (LtSicb*) buf;
		pSicb->que = 2;
		pSicb->cmd = 2;
		pSicb->tag = 0;
		pSicb->svc = LT_UNACKD;
		pSicb->auth = 1;

		/*
		 * LtSicb.data[] is declared to be one-element only.
		 * gcc 4.5 starts to produce warning about array-bounds.
		 * We should change data[] declaration to be the full
		 * size, but it's too risky now, so just suppress the warning.
		 */
		#ifdef __GNUC__
		#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Warray-bounds"
		#define GCC_ARRAY_BOUNDS_IGNORED
		#endif
		#endif
		pSicb->data[0] = LT_READ_MEMORY;
		pSicb->data[1] = LT_ABSOLUTE;
		pSicb->data[2] = 0xFF;
		pSicb->data[3] = 0x7B;
		pSicb->data[4] = 0x01;
		#ifdef GCC_ARRAY_BOUNDS_IGNORED
		#pragma GCC diagnostic pop
		#endif

		pSicb->dlen = 5;
		pSicb->len = pSicb->dlen + sizeof(LtSicb) - sizeof(pSicb->data) - 2;
		driverWrite(buf, pSicb->len+2);

		m_lastRxCount = m_linkStats.m_nReceivedPackets;
	}
}

// end
