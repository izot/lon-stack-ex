//
// LonLink.h
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

typedef enum {
	// Normal, valid packet
	PACKET_NORMAL = 0,

	// Direct mode only - transition detected but no valid preamble detected
	// within the expected preamble length.
	PACKET_TIMEOUT = 1,

	PACKET_CRC_ERROR = 2,

	// Packet longer than buffer accommodates.
	PACKET_TOO_LONG = 3,

	// Preamble too long, results in timeout.  In Neuron, this timeout is
	// determined by the watchdog timeout interval which is a function of
	// the clock rate.  At 10Mhz, it is around 840 milliseconds.
	PACKET_PREAMBLE_TOO_LONG = 4,

	// Preamble was not long enough to allow packet to be received.  That is,
	// the preamble passed the bit sync threshold but was not sufficiently
	// long to allow the data to be received.  Note that this phenomenon
	// shouldn't occur on this platform (which means the Protocol Analyzer
	// would not reflect such occurrences on other nodes).
	PACKET_PREAMBLE_TOO_SHORT = 5,

	// Packet is less than 8 bytes long (but had valid CRC).
	PACKET_TOO_SHORT = 6,

	// Packet receive error occurred such as an overrun error or FIFO not
	// empty on receive error.
	PACKET_OVERRUN_ERROR = 7
} LtLinkPacketType;

class LtLpdu;
class LtNetwork;

class LtCommParams {
public:
	byte m_byData[16];
	void set(const LtCommParams& cps) {
		memcpy(m_byData, cps.m_byData, sizeof(m_byData));
	}
};

class LtLinkStats {
public:
	// Transmission errors.  This counts:
	//  a. CRC errors
	//  b. Packets meeting the bit sync threshold but having no byte sync.
	//  c. Packets with byte sync but 0 or 1 bytes of LPDU (if not already
	//     covered by (a)).
	//  d. Packets with between 2 to 7 bytes of LPDU (optional).
	int transmissionErrors;
	// Missed packets.  This counts:
	//  a. Packets lost due to lack of input buffers.
	//  b. Packets lost due to insufficient buffer size.
	//  c. Packets lost due to unavailability of DMA channel.
	int missedPackets;
	// Collisions (total number of collisions detected)
	int collisions;
	// Backlog overflows.  This counts the number of times backlog had to be   
	// capped at the maximum of 63 because the sum of the delta backlog and
	// the current backlog exceeded 63.
	int backlogOverflows;

	// Note, the following statistics are "nice-to-have"
	// Number of packets transmitted
	int transmittedPackets;
	// Backoffs - this counts each time any transmission attempt is abandoned
	// due to receive data being detected prior to transmission starting.  This
	// could also be called "instances of collision avoidance". 
	int backoffs;
};

//
// Network layer to driver interface
//
class LtLink {
//
// The following interfaces are required:
//
public:
	// This method is used for a LonNetwork client to attach itself to the
	// LonLink driver and thus be informed of incoming traffic via the
	// LonNetwork callbacks.  Only one client is supported.
	virtual void registerNetwork(LtNetwork& net) = 0;

	// This method allows the network to set the priority of the task which
	// invokes the dataIndication callback.
	virtual void setReceivePriority(int priority) = 0;

	// Preallocate an LPDU.  This must be called prior to calling dataRequest
	// so that the packet can be built directly in the driver's memory.  Priority
	// and non-priority LPDUs come from distinct pools such that exhaustion of
	// non-priority buffers will not prevent allocation of a priority buffer.
	virtual LtLpdu* getTransmitLpdu(boolean priority) = 0;

	// Send a packet.  Packet must have been allocated via "getTransmitLpdu".
	// Packet is sent priority/non-priority based on how it was allocated 
	// (not based on LPDU priority bit!).  Packets are sent in the order
	// of this call (not based on allocation order).
	//
	// Note that sending can occur in a task context different than the 
	// original allocating task.  Therefore, this method must be "thread-safe".
	virtual void dataRequest(LtLpdu* lpdu) = 0;

	// Free a received packet.  This must be called to release an incoming 
	// LPDU (as delivered by LonNetwork.dataIndication()).  This allows 
	// the client to hold an LPDU longer than the time it takes to immediately 
	// process the LPDU.  This allows the client to avoid copying the data
	// when the data must be queued for some time.  The longer the client
	// holds the data, the more buffers the driver will need to avoid 
	// dropping packets.  Typically, the client should hold the data for
	// no more than 100 milliseconds.  See "reset()" for a description of how 
	// this function is used during a reset sequence.
	//
	// Note that freeing can occur in a task context different than the 
	// original receive task.  Therefore, this method must be "thread-safe".
	virtual void freeReceiveLpdu(LtLpdu *lpdu) = 0;

	// Free an allocated LPDU.  This can be used to release an LPDU which
	// was returned by "getTransmitLPDU" but will not be used.  See "reset()"
	// for a description of how this function is used during a reset 
	// sequence.
	// 
	// Note that freeing can occur in a task context different than the 
	// original allocating task.  Therefore, this method must be "thread-safe".
	virtual void freeTransmitLpdu(LtLpdu *lpdu) = 0;

	// This function resets the driver and the comm port.  Any buffered
	// outgoing messages or messages in progress are lost.  Any incoming
	// messages in process are lost.   All statistics are reset.  Following this
	// reset function call, the network layer will invoke the
	// "setCommParams" method (defined below) to set the communication
	// parameters. 
	//
	// The client must release all LPDUs that are pending to be freed as part
	// of the reset process (e.g., freeReceiveLpdu and freeTransmitLpdu).
	// However, it is not required that the LPDUs be freed prior to calling
	// reset.  Thus it is the driver's responsibility to hold off
	// allocating or delivering new LPDUs until the client has completed 
	// this freeing process.  Client is held in reset until all LPDUs are freed
	// and driver can resume normal operation.
	virtual void reset() = 0;

	// This function is used to retrieve the standard transceiver ID as 
	// detected from an SMX transceiver.  If the driver does not support this
	// feature or there is no SMX transceiver attached, then -1 is returned.
	// It is the responsibility of the LonNetwork code to read this value and
	// to set the comm parameters to the corresponding values (see
	// "setCommParams").
	virtual int getStandardTransceiverId() = 0;

	// This function is used to retrieve the LonTalk Unique ID from the network
	// interface hardware.  If the network interface hardware does support this
	// capability, false is returned, otherwise true.
	virtual boolean getUniqueId(LtUniqueId& uniqueId) = 0;

	// Request notification when all packets have been sent and there is
	// no input activity.  Use "on" to turn this mode on or off.  Default is off.
	virtual void flush(boolean on) = 0;

	// Like flush above except that any pending output activity is terminated
	// immediately.
	virtual void terminate() = 0;

	// Sets the 16 bytes of communication parameters.  Note that the special
	// purpose mode transceiver parameters (7 bytes) must be loaded in
	// reverse, from value 7 down to value 1.  This function returns once
	// all initialization is complete, including special purpose mode
	// configuration.  If special purpose mode configuration fails, this
	// routine will give up and return within a few seconds.
	virtual void setCommParams(const LtCommParams& commParams) = 0;

	// Gets the 16 bytes of communication parameters.  Note that in special
	// purpose mode, the 7 transceiver configuration bytes should be reported
	// as 0s since they are not readable.
	virtual void getCommParams(LtCommParams& commParams) = 0;

	// Gets the Nth byte of special purpose mode transceiver status register data
	// (7 registers numbered 1-7).  Value is returned by the
	// "reportTransceiverRegister" function.
	virtual void getTransceiverRegister(int n) = 0;

	// Reports statistics from the driver/LON-C.
	// These statistics are 16 bits and cap at 0xFFFF.
	virtual void getStatistics(LtLinkStats& stats) = 0;
	// 0 the statistics.
	virtual void clearStatistics() = 0;

	// Determines the state of the service pin.  "state" values are
	// defined as per the LON-C specification.
	virtual void setServicePinState(LtServicePinState state) = 0;

	// Send a packet with completion indication.  Once packet is sent,
	// driver invokes "packetSent" method of "LonNetwork".
	virtual void dataRequest(int referenceId,
						   LtLpdu* lpdu) = 0;

	// Controls whether physical/link layer function in Protocol Analyzer
	// mode.  This mode differs from normal mode in the following respects:
	// 1. In direct mode, if a transition is detected but RX flag is not 
	// raised within 32 bit times, report a timeout error.
	// 2. Following an invalid packet, ignore transitions during beta1 (arriving
	// prior to the first beta2 slot) as these are assumed to be continuations
	// of the error.
	// 3. In normal mode, packet errors are discarded by the driver or LON-C.
	// In Protocol Analyzer mode, error conditions are reported via
	// dataIndication.
	virtual void setProtocolAnalyzerMode(boolean on) = 0;
	virtual boolean getProtocolAnalyzerMode() = 0;

	// Puts the driver into a loopback mode where every transmitted packet
	// is treated as if it were received on the network.  Default is off.
	// Loopback is achieved by putting the LON-C hardware in loopback mode.
	virtual void setLoopbackMode(boolean on) = 0;
	virtual boolean getLoopbackMode() = 0;

	// Performs a self test of the comm port and returns a result.  Return
	// value is 0 for passed, -1 for failure.  May take a few seconds to 
	// return in failure case.  Self test will disrupt normal packet activity
	// but must not affect the network (i.e., cause spurious transmissions).
	virtual int selfTest() = 0;

	// This function reports the results of any power-on self test in the
	// LON-C hardware.  Return value is same as for "selfTest".
	virtual int reportPowerSelfTest() = 0;

	// For testing - allows the current backlog to be set to a certain value.
	// This could be called dynamically at any time during operation of the
	// LON-C.
	virtual void setCurrentBacklog(int backlog) = 0;
	// For debugging/analysis - allows the current backlog value to be reported.
	virtual int getCurrentBacklog() = 0;

	// For testing (or possible future enhancement) - allow the randomizing
	// window size to be changed.  Default is 16.  Maximum value is 64.
	// The set function would be called once, immediately following a
	// "reset" of the LON-C only.
	virtual void setWindowSize(int windowSize) = 0;
	virtual int getWindowSize() = 0;

	// For testing - forces a packets to use the specified slot for
	// "transmitSlot" indicates a desired beta2 slot to send
	// the packet on.  -1 would indicate automatic selection based on the
	// MAC layer rules.  A value of 0 to N-1 would indicate to transmit
	// on a specific slot where 0 is the first slot following beta1 (i.e.,
	// the first priority slot if there is priority, otherwise the first
	// randomizing slot).  The value of N should be at a minimum 127 but if
	// it could be made as large as 1063 very easily, this could be of some
	// value.  This capability is intended primarily for debugging.
	virtual void setTransmitSlot(int transmitSlot) = 0;
	virtual int getTransmitSlot() = 0;
};

//
// Driver to network layer interface
//
class LtNetwork {
//
// The following interfaces are required.
//
public:
	virtual void registerLink(LtLink& link) = 0;

	// Provides an incoming packet.  The lpdu must be released by the network
	// client using freeReceiveLpdu().  Note that it must be possible for
	// a freed input lpdu to receive a new packet independent of the 
	// free/in-use state of any other lpdu.  In other words, if an lpdu were
	// to never get freed, that should not lock up the receive engine.
	virtual void dataIndication(LtLpdu* lpdu) = 0;

	// Notifies the network client that the LON-C hardware received a reset
	// signal.  It is up to the network layer to actually initiate the LON-C
	// reset (using the reset() function).
	// Following such a reset, a call to "setCommParams" 
	// must be made by the network layer before proper operation can resume.
	virtual void resetRequested() = 0;

	// Notifies network client that all output activity has completed
	// or has been terminated and that there is currently no input activity.
	// Note that this callback may be invoked prior to a return from the
	// initiating "flush".
	virtual void flushCompleted() = 0;

	// Notifies network client that all output activity has been
	// terminated (as a result of a "terminate" call).  Note that this callback
	// may be invoked prior to a return from the initiating "terminate".
	virtual void terminateCompleted() = 0;

	// Notifies network client of the results of a transceiver register
	// query for register "n".  
	virtual void reportTransceiverRegister(int n, int value) = 0;

	// Notifies network client that a service pin depression occurred.
	virtual void servicePinDepressed() = 0;

	// This interface is used when in Protocol Analyzer mode.  It provides
	// a receive packet indication or packet error.
	// Under certain error conditions, it is acceptable to report a null
	// lpdu object.  
	// "receivedSlot" indicates the beta2 slot in which the packet was
	// received.  (see dataRequest for discussion of maximum value).
	// -1 indicates receipt of a packet prior to the first beta2 slot.
	virtual void dataIndication(LtLinkPacketType type,
							  int receivedSlot,
							  LtLpdu* lpdu) = 0;

	// If a packet is sent with dataRequest specifying a referenceId,
	// then upon successful transmission, then this function is called.  
	//
	// Note that this callback may be invoked prior to a return from the
	// initiating "dataRequest".
	virtual void packetSent(int referenceId) = 0;

	// If a packet is sent with dataRequest specifying a referenceId,
	// then upon discard of packet due to excessive collisions or termination,
	// this function is called.  This function is also invoked when packets
	// are not transmitted due to comm port initialization failure, (such as 
	// configuration failure of a special purpose mode transceiver) or self
	// test failure.
	//
	// Note that this callback may be invoked prior to a return from the
	// initiating "dataRequest".
	virtual void packetDiscarded(int referenceId) = 0;
};
