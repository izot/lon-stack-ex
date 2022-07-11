#ifndef _LTNETWORKBASE_HC30215B0_B70D_11d2_A833_00104B9F34CA
#define _LTNETWORKBASE_HC30215B0_B70D_11d2_A833_00104B9F34CA
/***************************************************************
 *  Filename: LtNetworkBase.h
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
 *  Description:  Definitions for LtNetworkBase class
 *
 *	DJ Duffy Jan 1999
 *
 ****************************************************************/


/*
 * $Log: /VNIstack/FtXl_Dev/NiosWs/FtxlLib/Shared/include/LtNetworkBase.h $
 * 
 * 8     6/20/07 3:58p Bobw
 * Support service pin held function
 * 
 * 7     4/28/04 6:49p Bobw
 * EPR 32593
 * Support VNI based protocol analyzer.  Pass bad packets up, and time
 * stamp them at first oportunity.  
 * 
 * 5     10/06/99 4:19p Darrelld
 * Add getLink member
 * 
 * 4     4/13/99 4:58p Darrelld
 * Enhance for Aggregation and BW Limiting and test
 * 
 * 3     3/12/99 11:58a Darrelld
 * Paper over lack of virtual destructor in LtLink
 * 
 * 2     3/11/99 3:51p Darrelld
 * removed m_pLink since its defined in LtNetwork
 * 
 * 1     2/22/99 9:19a Darrelld
 * moved here from LTIPtest
 * 
 * 1     1/28/99 4:15p Darrelld

 * 
 */


#include <LtDriver.h>
class LtLinkBase;

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// Driver to network layer interface
//
class LtNetworkBase : public LtNetwork
{
public:
	LtNetworkBase();
	virtual ~LtNetworkBase();
	//
	// The following interfaces are required.
	//
	virtual void registerLink(LtLink& link);

	// Provides an incoming packet. The packet was provided to the
	// the driver via the queueReceive call to the LtLink object.
	// This interface is also used when in Protocol Analyzer mode. 
	virtual void packetReceived(void* referenceId,
								int nLengthReceived,
								boolean bPriority,
								int receivedSlot,
								boolean isValidLtPacket, 
								byte    l2PacketType,
								LtSts sts,
								byte	ssiReg1,
								byte	ssiReg2);

	// Notifies the network client that the LON-C hardware received a reset
	// signal.  It is up to the network layer to actually initiate the LON-C
	// reset (using the reset() function).
	// Following such a reset, a call to "setCommParams" 
	// must be made by the network layer before proper operation can resume.
	virtual void resetRequested();

	// Notifies network client that all output activity has completed
	// or has been terminated and that there is currently no input activity.
	// Note that this callback may be invoked prior to a return from the
	// initiating "flush".
	virtual void flushCompleted();

	// Notifies network client that all output activity has been
	// terminated (as a result of a "terminate" call).  Note that this callback
	// may be invoked prior to a return from the initiating "terminate".
	virtual void terminateCompleted();

	// Notifies network client of the results of a transceiver register
	// query for register "n".
	// Status reported may include: LTSTS_OK, LTSTS_INVALIDSTATE, LTSTS_RESET
	// Callbacks are always performed. If any outstanding during a reset, then
	// LTSTS_RESET is returned.
	virtual void reportTransceiverRegister(int n, int value, LtSts sts);

	// Notifies network client that a service pin depression occurred.
	virtual void servicePinDepressed();

    // Notifies network client that a service pin has been released.  
    virtual void servicePinReleased() {}

	// If a packet is sent with sendPacket specifying a referenceId,
	// then upon transmission, then this function is called.  
	//
	// Note that this callback may be invoked prior to a return from the
	// initiating "sendPacket".
	virtual void packetComplete(void* referenceId, LtSts sts );
protected:
	// defined in base class but must paper over it here with
	// a class with a virtual destructor, which we left out of LtLink
	LtLinkBase*		m_pLink;


};


class LtIpNetwork : public LtNetworkBase
{
public:
	// Provides an incoming packet. The packet was provided to the
	// the driver via the queueReceive call to the LtLink object.
	// This interface is used to obtain the source ip node information
	virtual void packetReceived(void* referenceId,
								int nLengthReceived,
								boolean bPriority,
								ULONG ipSrcAddr,
								word ipSrcPort,
								LtSts sts);
	virtual LtLinkBase*	getLink()
	{	return m_pLink;
	}
};
#endif // _LTNETWORKBASE_HC30215B0_B70D_11d2_A833_00104B9F34CA
