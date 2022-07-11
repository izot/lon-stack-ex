/***************************************************************
 *  Filename: LtNetworkBase.cpp
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
 *  Description:  Stubs for LtNetworkBase class
 *
 *	DJ Duffy Nov 1998
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Shared/LtNetwork.cpp#1 $
//
/*
 * $Log: /VNIstack/LNS_V3.2x/Shared/LtNetwork.cpp $
 * 
 * 6     4/28/04 6:49p Bobw
 * EPR 32593
 * Support VNI based protocol analyzer.  Pass bad packets up, and time
 * stamp them at first oportunity.  
 * 
 * 4     4/20/99 4:10p Darrelld
 * Add "confidential" statement
 * 
 * 3     4/13/99 4:58p Darrelld
 * Enhance for Aggregation and BW Limiting and test
 * 
 * 2     3/12/99 11:58a Darrelld
 * Paper over lack of virtual destructor in LtLink
 * 
 * 1     2/22/99 9:20a Darrelld
 * moved here from LTIPtest
 * 
 * 3     1/28/99 4:20p Darrelld
 * Update for LtNetwork becoming a pure virtual class
 * 
 * 2     1/13/99 9:54a Darrelld
 * Add status for reportTransceiverRegister
 * 
 * 1     11/30/98 4:57p Darrelld
 * LTIP Test application
 * 
 */

#include <string.h>

#include "VxWorks.h"
#include "VxlTypes.h"

#include "LtNetworkBase.h"

//
// Constructor
//
LtNetworkBase::LtNetworkBase()
{
	m_pLink		= NULL;

}

//
// Destructor
//
LtNetworkBase::~LtNetworkBase()
{


}


//
// registerLink
//
// Recall that we forgot to define LtLink with a virtual destructor
// so we need to Paste over this by having a network variable with
// the different definition
// 
void LtNetworkBase::registerLink(LtLink& link)
{
	m_pLink = (LtLinkBase*)&link;
}

// Provides an incoming packet. The packet was provided to the
// the driver via the queueReceive call to the LtLink object.
// This method is used when the LtLink is not set to protocol analyser mode.
void LtNetworkBase::packetReceived(void* referenceId, int nLengthReceived, 
							       boolean bPriority, int nSlotReceived,
								   boolean isValidLtPacket, 
								   byte l2PacketType, LtSts sts,
								   byte ssiReg1, byte ssiReg2)
{
        // just release the packet
    packetComplete( referenceId, LTSTS_INVALIDSTATE );
}

// Notifies the network client that the LON-C hardware received a reset
// signal.  It is up to the network layer to actually initiate the LON-C
// reset (using the reset() function).
// Following such a reset, a call to "setCommParams" 
// must be made by the network layer before proper operation can resume.
void LtNetworkBase::resetRequested()
{
}

// Notifies network client that all output activity has completed
// or has been terminated and that there is currently no input activity.
// Note that this callback may be invoked prior to a return from the
// initiating "flush".
void LtNetworkBase::flushCompleted()
{
}

// Notifies network client that all output activity has been
// terminated (as a result of a "terminate" call).  Note that this callback
// may be invoked prior to a return from the initiating "terminate".
void LtNetworkBase::terminateCompleted()
{
}

// Notifies network client of the results of a transceiver register
// query for register "n".  
void LtNetworkBase::reportTransceiverRegister(int n, int value, LtSts sts )
{

}

// Notifies network client that a service pin depression occurred.
void LtNetworkBase::servicePinDepressed()
{

}

// If a packet is sent with sendPacket specifying a referenceId,
// then upon transmission, then this function is called.  
//
// Note that this callback may be invoked prior to a return from the
// initiating "sendPacket".
void LtNetworkBase::packetComplete(void* referenceId, LtSts sts )
{

}

//
// packetReceived
//
// override this function to obtain the ipSrcAddress and ipSrcPort.
// The other packetReceived call is not called by an Iplink object if
// registerIpNetwork is used to set the network object.
//
void LtIpNetwork::packetReceived(void* referenceId,
								int nLengthReceived,
								boolean bPriority,
								ULONG ipSrcAddr,
								word ipSrcPort,
								LtSts sts)
{
        // just release the packet
    packetComplete( referenceId, LTSTS_INVALIDSTATE );
}
// end
