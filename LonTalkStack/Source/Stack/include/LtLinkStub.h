//
// LtLinkStub.h
//
// Copyright © 2022 Dialog Semiconductor
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in 
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtLinkStub.h#1 $
//

class LtLinkStub : public LtLink
{
private: 
	LtNetwork* m_pNet;
	LtCommParams m_comm;
	int m_xcvrId;

public:
	LtLinkStub(LtLogicalChannel* pChannel)
	{
		m_xcvrId = pChannel->getXcvrId();
		memset(m_comm.m_nData, 0, sizeof(m_comm.m_nData));
	}

	//
	// The following interfaces are required:
	//
	virtual void destroyInstance() { delete this; }

	// Enumerate Interfaces supported by this link
	// idx values start with zero. For each valid interface the name
	// is returned in the buffer and true is returned.
	// False is returned if there is no interface with that index.
	virtual boolean enumInterfaces( int idx, LPSTR pNameRtn, int nMaxSize )
	{
		return false;
	}

	// Open the interface for use
	// Call with a name returned by enumInterfaces
	virtual LtSts open( const char* pName )
	{
		return LTSTS_OK;
	}

	virtual boolean		isOpen()
	{
		return true;
	}

	// Close the currently open interface
	virtual LtSts close() 
	{
		return LTSTS_OK;
	}

	// This method is used for a LtNetwork client to attach itself to the
	// LtLink driver and thus be informed of incoming traffic via the
	// LtNetwork callbacks.  Only one client is supported.
	virtual LtSts registerNetwork(LtNetwork& net)
	{
		m_pNet = &net;
		return LTSTS_OK;
	}

	// This method allows the network to set the priority of the task which
	// invokes the packetReceived callback.
	virtual LtSts setReceivePriority(int priority)
	{
		return LTSTS_OK;
	}

	// Get the max sizes of send and receive queues in the driver
	virtual void getMaxQueueDepth( int* pnReceiveQueueDepth, int* pnTransmitQueueDepth )
	{
	}

	// Set the sizes of the send and receive queues in the driver
	// May cause a "reset" to occur.
	// returns LTSTS_INVALIDSTATE if the driver is closed.
	// returns LTSTS_ERROR if the values are not valid.
	virtual LtSts setQueueDepths( int nReceiveQueueDepth, int nTransmitQueueDepth )
	{
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
	virtual LtSts sendPacket(void* referenceId,
					int nPrioritySlot,
					byte flags,
					byte* pData,
					int nDataLength,
					boolean bPriority)
	{
		return LTSTS_OK;
	}

	// Queue a receive request to the driver.
	// Driver queues buffer for receive and calls packetReceive method of
	// LtNetwork when a message is received or a reset occurs.
	// Synchronous return of either LTSTS_PENDING or LTSTS_QUEUEFULL.
	virtual LtSts queueReceive( void* referenceId,
						boolean bPriority,
						byte flags,
						byte* pData,
						int nMaxLength)
	{
		return LTSTS_OK;
	}

	// This function resets the driver and the comm port.  Any buffered
	// outgoing messages or messages in progress are not sent.  Any incoming
	// messages in process are lost.   All statistics are reset.  Following this
	// reset function call, the network layer will invoke the
	// "setCommParams" method (defined below) to set the communication
	// parameters. 
	// All send and receive requests are completed immedately.
	// If a reset is in progress and another call to "reset" is made, 
	// the driver will reset again once the first reset completes.  
	// Note that "reset completes" here excludes reconfiguration of 
	// the special purpose mode transceiver.  
	// This function has no effect if the driver is closed.
	virtual void reset()
	{
	}

	// This function is used to retrieve the standard transceiver ID as 
	// detected from an SMX transceiver.  If the driver does not support this
	// feature or there is no SMX transceiver attached, then -1 is returned.
	// It is the responsibility of the LtNetwork code to read this value and
	// to set the comm parameters to the corresponding values (see
	// "setCommParams").
	virtual int getStandardTransceiverId()
	{
		// Specify IP local area channel type 
		return m_xcvrId;
	}

	// This function is used to retrieve the LonTalk Unique ID from the network
	// interface hardware.  If the network interface hardware does support this
	// capability, false is returned, otherwise true.
	virtual boolean getUniqueId(LtUniqueId& uniqueId)
	{
		return false;
	}

	// Request notification when all packets have been sent and there is
	// no input activity.  Use "on" to turn this mode on or off.  Default is off.
	// If a flush is requested when the driver is closed, flushCompletes is called.
	// If a reset occurs while a flush call is blocked, flushCompletes is called 
	// once the reset completes.
	// If a flush is in progress and another call to flush is made (by the same or 
	// different thread), then this would have no effect.  
	// If a flush is in progress and a terminate is done, flushCompletes and 
	// terminateCompletes are called when the terminate completes.
	virtual void flush(boolean on)
	{
		m_pNet->flushCompleted();
	}

	// Like flush above except that any pending output activity is terminated
	// immediately.
	// If a terminate is requested when the driver is closed, terminateCompletes is called.
	// If a reset occurs while a terminate call is blocked, terminateCompletes 
	// is called once the reset completes.
	// If a terminate is in progress and another call to terminate is made, 
	// then this would have no effect.
	// If a terminate is in progress and a flush is done, flushCompletes and 
	// terminateCompletes are called when the terminate completes.
	virtual void terminate()
	{
		m_pNet->terminateCompleted();
	}

	// Sets the 16 bytes of communication parameters.  Note that the special
	// purpose mode transceiver parameters (7 bytes) must be loaded in
	// reverse, from value 7 down to value 1.  This function returns once
	// all initialization is complete, including special purpose mode
	// configuration.  If special purpose mode configuration fails, this
	// routine will give up and return within a few seconds.
	// Returns LTSTS_TIMEOUT, LTSTS_INVALIDSTATE, LTSTS_NOTRESET or LTSTS_ERROR
	// Even though this routine doesn't return until the transceiver is configured,
	// other driver calls should not be blocked while this configuration is taking place
	// as it might take a while.  If a call to setCommParams is made following another
	// call to setCommParams without an intervening reset(), setCommParams would 
	// return LTSTS_NOTRESET.  If the driver is closed, this reports LTSTS_INVALIDSTATE.
	virtual LtSts setCommParams(const LtCommParams& commParams)
	{
		m_comm = commParams;
		return LTSTS_OK;
	}

	// Gets the 16 bytes of communication parameters.  Note that in special
	// purpose mode, the 7 transceiver configuration bytes should be reported
	// as 0s since they are not readable.
	// Returns LTSTS_TIMEOUT, LTSTS_INVALIDSTATE or LTSTS_ERROR
	virtual LtSts getCommParams( LtCommParams& commParams)
	{
		commParams = m_comm;
		return LTSTS_OK;
	}

	// Gets the Nth byte of special purpose mode transceiver status register data
	// (7 registers numbered 1-7).  Value is returned by the
	// "reportTransceiverRegister" function.
	// If a transceiver register read is requested when the driver is closed,
	// reportTransceiverReigister is called with LTSTS_INVALIDSTATE.
	// If a transceiver register read is in progress and a reset occurs,
	// reportTransceiverRegister can return LTSTS_RESET.
	// If a transceiver register read is in progress and 
	// another call to getTransceiverRegister is made, 
	// this second call would queue the request
	// (could simply have a dirty bit for each register, no need to block 
	// until the first request completes).
	virtual LtSts getTransceiverRegister(int n)
	{
		m_pNet->reportTransceiverRegister(n, 0, LTSTS_INVALIDSTATE);
		return LTSTS_OK;
	}

	// Reports statistics from the driver/LON-C.
	// These statistics are 16 bits and cap at 0xFFFF.
	virtual void getStatistics(LtLinkStats& stats)
	{
		byte* pStats = (byte*) &stats;
		for (int i = 0; i < (int)sizeof(stats); i++) *(pStats++) = 0;
	}
	virtual void getStatistics(LtLinkStats *&pStats)
	{
		pStats = NULL;
	}

	// 0 the statistics.
	virtual void clearStatistics()
	{
	}

	// Determines the state of the service pin.  "state" values are
	// defined as per the LON-C specification.
	virtual void setServicePinState(LtServicePinState state);

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
	virtual void setProtocolAnalyzerMode(boolean on)
	{
	}
	virtual boolean getProtocolAnalyzerMode()
	{
		return false;
	}

	// Puts the driver into a loopback mode where every transmitted packet
	// is treated as if it were received on the network.  Default is off.
	// Loopback is achieved by putting the LON-C hardware in loopback mode.
	virtual void setLoopbackMode(boolean on)
	{
	}
	virtual boolean getLoopbackMode()
	{
		return false;
	}

	// Performs a self test of the comm port and returns a result.
	// May take a few seconds to 
	// return in failure case.  Self test will disrupt normal packet activity
	// but must not affect the network (i.e., cause spurious transmissions).
	// returns LTSTS_ERROR or LTSTS_OK
	// If possible, other functions should be executed while the self test is in progress.
	// A reset request during the self test would cause the self test to fail.
	// Outgoing requests and register reads would pend until the test completed.
	// If the driver is closed, this reports a failure.
	virtual LtSts selfTest()
	{
		return LTSTS_OK;
	}

	// This function reports the results of any power-on self test in the
	// LON-C hardware.  Return value is same as for "selfTest".
	virtual LtSts reportPowerSelfTest()
	{
		return LTSTS_OK;
	}

	// For testing - allows the current backlog to be set to a certain value.
	// This could be called dynamically at any time during operation of the
	// LON-C.
	virtual void setCurrentBacklog(int backlog)
	{
	}
	// For debugging/analysis - allows the current backlog value to be reported.
	virtual int getCurrentBacklog()
	{
		return 0;
	}

	// For testing (or possible future enhancement) - allow the randomizing
	// window size to be changed.  Default is 16.  Maximum value is 64.
	// The set function would be called once, immediately following a
	// "reset" of the LON-C only.
	virtual void setWindowSize(int windowSize)
	{
	}
	virtual int getWindowSize()
	{
		return 16;
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
	virtual void setTransmitSlot(int transmitSlot)
	{
	}
	virtual int getTransmitSlot()
	{
		return -1;
	}
};

