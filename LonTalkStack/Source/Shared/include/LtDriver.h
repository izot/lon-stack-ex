#ifndef _LTDRIVER_H
#define _LTDRIVER_H
/***************************************************************
 *  Filename: LtDriver.h
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
 *  Description:  LtLink, LtNetwork etc. class definitions
 *
 *	Glen Riley Nov 1998
 *
 ****************************************************************/

/*
 * $Log: /VNIstack/Dcx_Dev/Shared/include/LtDriver.h $
 * 
 * 18    6/24/08 7:54a Glen
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
 * 17    11/16/07 1:52p Bobw
 * EPR 47445
 * Don't wait for network buffer to become available if none are defined.
 * 
 * 16    8/01/07 5:50p Fremont
 * EPR 45753 - clearing the network statistics of an internal device
 * cleared the console linkstats display, which was intended to show the
 * external packet statistics. In standalone mode on the iLON, this
 * happened automatically on any internal device commission. Solution:
 * keep a shadow shadow copy of the statistics that were actually shared,
 * increment them in the driver, and read/clear the shadow copies when
 * accessing the internal device.
 * 
 * 15    6/20/07 3:58p Bobw
 * Support service pin held function
 * 
 * 14    7/15/05 11:23a Bobw
 * EPR 35786
 * Change LonLinkWin to support the layer 2 mip to be pre-opened.  The
 * VniServer opens the device to determine its mip type (determinMipType).
 * Prior to this change, the VniServer opened the device, and if was a
 * layer 2 mip, closed it, so that lonLinkWin could re-open it.  Now
 * lonLinkWin just uses the handle opened previously (if set).  Note that
 * there are other changes to determinMipType necessary to complete this
 * EPR.
 * 
 * 13    4/12/05 9:45a Bobw
 * EPR 36422
 * Support LDV attachment events for layer 2 mips as well as layer 5 mips.
 * 
 * 12    4/28/04 6:49p Bobw
 * EPR 32593
 * Support VNI based protocol analyzer.  Pass bad packets up, and time
 * stamp them at first oportunity.  
 * 
 * 11    4/23/04 5:25p Bobw
 * Keep track of whether or not read only data has been updated.  Update
 * network buffers only if it has been.  If it hasn't, then refresh the
 * read only data from the driver's cache.  Otherwise we have the
 * situation that stack A sets the read only data and updates the network
 * buffers, then stack B resets, but since it still has stale read only
 * data it sets them back to the old values.  Note that the read only data
 * of a stack will still not be updated with the latest network buffers
 * until it resets - but this seems OK.
 * 
 * 10    4/23/04 4:16p Bobw
 * EPR 32997
 * On Layer2 mip, maintain a cache of network buffers.  On startup, read
 * the buffers from the mip (if they have not already been read), and
 * update the stacks read-only data to reflect the network buffers (but
 * not app buffers).  On reseting a stack, check to see if the buffers in
 * read only data differ from those in the LONLINK cache, and if so, write
 * them back out to the mip.
 * 
 * 9     6/23/03 11:09a Glen
 * Development related to supporting NES devices on top of the new Power
 * Line SLTA.  This includes making phase detection and bi-directional
 * query status work.  Also, changed stack such that if a transceiver ID
 * is unknown, the comm params are left unchanged.
 * 
 * 8     11/06/01 9:25a Glen
 * Need control of zero crossing synchronization and attenuation for
 * LonTalk Validator.  Added these flags to sendPacket().  This propagated
 * to lots of places.  Also added control options to LtMsgOut.
 * 
 * 7     11/01/01 11:54a Fremont
 * Remove dead LtCommParams def (now in own header)
 * 
 * 5     3/14/00 9:53a Darrelld
 * remove ($)Log
 * 
 * 4     2/23/00 9:07a Darrelld
 * LNS 3.0 Merge
 * 
 * 3     11/07/99 11:04a Darrelld
 * Add wink support
 * 
 * 2     11/04/99 11:01a Darrelld
 * When changing xcvrs, cap priority at max if necessary.  Don't go
 * unconfigured.
 * 
 * 1     2/17/99 1:44p Darrelld
 * PNC board's BSP
 * 
 * 18    2/16/99 3:03p Glen
 * Driver unification
 * 
 * 17    2/01/99 11:15a Glen
 * Joint Test 3 integration
 * 
 * 15    1/22/99 1:49p Glen
 * 
 * 14    1/22/99 11:16a Glen
 * 
 * 13    1/22/99 11:08a Glen
 * 
 * 12    1/22/99 10:39a Glen
 * 
 * 11    1/22/99 9:12a Glen
 * add confidential statement
 * 
 * 10    1/21/99 6:07p Glen
 * Get ready to run on VxWorks
 * 
 * 9     1/12/99 1:14p Darrelld
 * Adjust comments.
 * 
 * 8     1/12/99 1:11p Darrelld
 * Add status return to reportTransciverRegister
 * 
 * 7     12/16/98 1:22p Darrelld
 * Update LtLink interface
 * 
 * 6     12/15/98 11:46a Darrelld
 * check in to accomplish move
 * 
 * 5     12/11/98 2:29p Darrelld
 * Add isOpen() to LtLink members
 * 
 * 4     12/02/98 2:44p Glen
 * 
 * 3     12/02/98 2:42p Glen
 * 
 * 2     12/02/98 12:45p Darrelld
 * Include VxlTypes.h which needs to be updated to define boolean and byte
 * 
 * 1     11/30/98 4:54p Darrelld
 * LtDriver header file for LtLink and LtNetwork classes
 * 
 */
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// Helper types, classes
//
#include <VxlTypes.h>
#include <LtUniqueId.h>
#include <LtCommParams.h>
#include "LtProgramId.h"
#include "LtReadOnlyData.h"

#ifndef _FTXL_TYPES_H
#undef LtServicePinState
#undef SERVICE_OFF
#undef SERVICE_ON
#undef SERVICE_BLINKING
#undef SERVICE_FLICKER
typedef enum {
	SERVICE_OFF = 0,
	SERVICE_ON = 1,
	SERVICE_BLINKING = 2,

	// Software controlled only
	SERVICE_FLICKER = -1
} LtServicePinState;
#endif

typedef enum {
	LTSTS_PENDING = 0,			// immediate return, packet queued
	LTSTS_OK = 1,				// success
	LTSTS_QUEUEFULL,			// immediate return, send/receive failure
	LTSTS_TIMEOUT,				// timeout occurred
	LTSTS_CRCERROR,				// CRC error on received packet
	LTSTS_TOOLONG,				// Transmitted packet length too long
	LTSTS_TOOSHORT,				// Received packet less than 8 bytes
	LTSTS_PREAMTOOLONG,			// Preamble too long causing neuron timeout
	LTSTS_PREAMTOOSHORT,		// Preamble too short causing receive error
	LTSTS_OVERRUN,				// Overrun of FIFO
	LTSTS_RESET,				// reset occurred
	LTSTS_TERMINATED,			// terminate occurred
	LTSTS_ERROR,				// need a short list of meaningful errors
	LTSTS_COLLIDED,				// packet discarded due to collisions
	LTSTS_COMMPORTINITFAILURE,	// Comm port init failure
	LTSTS_INVALIDSTATE,			// Invalid state for call
	LTSTS_OPENFAILURE,			// Unable to open the device
	LTSTS_NOTRESET,				// A previous reset was expected
    LTSTS_NO_NETBUFS_DEFINED,   // No network buffers with the specified priority are defined.
	LTSTS_END					// highest value of status
} LtSts;

#define L2_PKT_TYPE_TIMEOUT				0x00
#define L2_PKT_TYPE_CRC					0x01
#define L2_PKT_TYPE_PACKET_TOO_LONG		0x02
#define L2_PKT_TYPE_PREAMBLE_TOO_LONG	0x03
#define L2_PKT_TYPE_PREAMBLE_TOO_SHORT	0x04
#define L2_PKT_TYPE_PACKET_TOO_SHORT	0x05
#define L2_PKT_TYPE_LOCAL_NM_RESP		0x16	// Response to local NM command
#define L2_PKT_TYPE_INCOMING			0x1a	// traditional incoming L2 packets
#define L2_PKT_TYPE_MODE1_INCOMING		0x1b	// mode 1 incoming L2 packets.  
#define L2_PKT_TYPE_MODE2_INCOMING		0x1c	// mode 2 incoming L2 packets.
#define L2_PKT_TYPE_FREQUENCY_REPORT	0x40	// Frequency report
#define L2_PKT_TYPE_RESET				0x50	// Reset

#define L2_PKT_LEN_EXTENDED             0xff    // l2 packet length uses extended format.  The actual length
                                                // appears in the following 2 bytes.
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//  Statistics Class
//
class LtLinkStats {
public:
	LtLinkStats() { m_shadowed = false; }

	// Transmission errors.  This counts:
	//  a. CRC errors
	//  b. Packets meeting the bit sync threshold but having no byte sync.
	//  c. Packets with byte sync but 0 or 1 bytes of LPDU (if not already
	//     covered by (a)).
	//  d. Packets with between 2 to 7 bytes of LPDU (optional).
	int m_nTransmissionErrors;
	// Missed packets.  This counts:
	//  a. Packets lost due to lack of input buffers.
	//  b. Packets lost due to insufficient buffer size.
	//  c. Packets lost due to unavailability of DMA channel.
	int m_nMissedPackets;
	// Collisions (total number of collisions detected)
	int m_nCollisions;
	// Backlog overflows.  This counts the number of times backlog had to be   
	// capped at the maximum of 63 because the sum of the delta backlog and
	// the current backlog exceeded 63.
	int m_nBacklogOverflows;

	// Note, the following statistics are "nice-to-have"
	// Number of packets transmitted
	int m_nTransmittedPackets;
	// Total number of packets received
	int	m_nReceivedPackets;
	// Number of priority packets received
	int	m_nReceivedPriorityPackets;

	// Backoffs - this counts each time any transmission attempt is abandoned
	// due to receive data being detected prior to transmission starting.  This
	// could also be called "instances of collision avoidance". 
	int m_nBackoffs;

	void clearStats()
	{
		m_nTransmissionErrors = m_nMissedPackets = m_nCollisions = m_nBacklogOverflows = m_nTransmittedPackets = m_nReceivedPackets	= m_nReceivedPriorityPackets = m_nBackoffs = 0; 
	}

	boolean m_shadowed;	// indicates whether this object is really the next one (avoid RTTI)
};

// This is to allow the internal devices to get these values from the driver
// and clear them without clearing the driver's values
class LtLinkStatsShadow	: public LtLinkStats
{
public:
	LtLinkStatsShadow() { m_shadowed = true; }

	int m_nTransmissionErrorsShadow;
	int m_nMissedPacketsShadow;
	int m_nBacklogOverflowsShadow;	// Is not actually used anywhere; exists for completeness.
	int m_nCollisionsShadow;

	void clearShadowStats() { m_nTransmissionErrorsShadow = m_nMissedPacketsShadow = m_nBacklogOverflowsShadow = m_nCollisionsShadow = 0; }
};

#define LPDU_OVERHEAD  14	// Bytes needs for private driver storage
							// Not usable by application.

// Forward reference
class LtNetwork;

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//
// Network layer to driver interface
//
class LtLink {

public:
	virtual ~LtLink() {}
	//
	// The following interfaces are required:
	//

	// Enumerate Interfaces supported by this link
	// idx values start with zero. For each valid interface the name
	// is returned in the buffer and true is returned.
	// False is returned if there is no interface with that index.
	virtual boolean enumInterfaces( int idx, LPSTR pNameRtn, int nMaxSize ) = 0;

	// Open the interface for use
	// Call with a name returned by enumInterfaces
	virtual LtSts open( const char* pName ) = 0;
	virtual boolean		isOpen() = 0;

	// Close the currently open interface
	virtual LtSts close() = 0;

	// This method is used for a LtNetwork client to attach itself to the
	// LtLink driver and thus be informed of incoming traffic via the
	// LtNetwork callbacks.  Only one client is supported.
	virtual LtSts registerNetwork(LtNetwork& net) = 0;

	// This method allows the network to set the priority of the task which
	// invokes the packetReceived callback.
	virtual LtSts setReceivePriority(int priority) = 0;

	// Get the max sizes of send and receive queues in the driver
	virtual void getMaxQueueDepth( int* pnReceiveQueueDepth, int* pnTransmitQueueDepth ) = 0;

	// Set the sizes of the send and receive queues in the driver
	// May cause a "reset" to occur.
	// returns LTSTS_INVALIDSTATE if the driver is closed.
	// returns LTSTS_ERROR if the values are not valid.
	virtual LtSts setQueueDepths( int nReceiveQueueDepth, int nTransmitQueueDepth ) = 0;

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
					boolean bPriority) = 0;

	// Queue a receive request to the driver.
	// Driver queues buffer for receive and calls packetReceive method of
	// LtNetwork when a message is received or a reset occurs.
	// Synchronous return of either LTSTS_PENDING or LTSTS_QUEUEFULL.
	virtual LtSts queueReceive( void* referenceId,
						boolean bPriority,
						byte flags,
						byte* pData,
						int nMaxLength) = 0;

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
	virtual void reset() = 0;

	// This function is used to retrieve the standard transceiver ID as 
	// detected from an SMX transceiver.  If the driver does not support this
	// feature or there is no SMX transceiver attached, then -1 is returned.
	// It is the responsibility of the LtNetwork code to read this value and
	// to set the comm parameters to the corresponding values (see
	// "setCommParams").
	virtual int getStandardTransceiverId() = 0;

	// This function is used to retrieve the LonTalk Unique ID from the network
	// interface hardware.  If the network interface hardware does support this
	// capability, false is returned, otherwise true.
	virtual boolean getUniqueId(LtUniqueId& uniqueId) = 0;

	// Request notification when all packets have been sent and there is
	// no input activity.  Use "on" to turn this mode on or off.  Default is off.
	// If a flush is requested when the driver is closed, flushCompletes is called.
	// If a reset occurs while a flush call is blocked, flushCompletes is called 
	// once the reset completes.
	// If a flush is in progress and another call to flush is made (by the same or 
	// different thread), then this would have no effect.  
	// If a flush is in progress and a terminate is done, flushCompletes and 
	// terminateCompletes are called when the terminate completes.
	virtual void flush(boolean on) = 0;

	// Like flush above except that any pending output activity is terminated
	// immediately.
	// If a terminate is requested when the driver is closed, terminateCompletes is called.
	// If a reset occurs while a terminate call is blocked, terminateCompletes 
	// is called once the reset completes.
	// If a terminate is in progress and another call to terminate is made, 
	// then this would have no effect.
	// If a terminate is in progress and a flush is done, flushCompletes and 
	// terminateCompletes are called when the terminate completes.
	virtual void terminate() = 0;

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
	virtual LtSts setCommParams(const LtCommParams& commParams) = 0;

	// Gets the 16 bytes of communication parameters.  Note that in special
	// purpose mode, the 7 transceiver configuration bytes should be reported
	// as 0s since they are not readable.
	// Returns LTSTS_TIMEOUT, LTSTS_INVALIDSTATE or LTSTS_ERROR
	virtual LtSts getCommParams( LtCommParams& commParams) = 0;

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
	virtual LtSts getTransceiverRegister(int n) = 0;

	// Reports statistics from the driver/LON-C.
	// These statistics are 16 bits and cap at 0xFFFF.
	virtual void getStatistics(LtLinkStats& stats) = 0;
	virtual void getStatistics(LtLinkStats *&pStats) = 0;
	// 0 the statistics.
	virtual void clearStatistics() = 0;

	// Determines the state of the service pin.  "state" values are
	// defined as per the LON-C specification.
	virtual void setServicePinState(LtServicePinState state) = 0;

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
	virtual void setProtocolAnalyzerMode(boolean on) = 0;
	virtual boolean getProtocolAnalyzerMode() = 0;

	// Puts the driver into a loopback mode where every transmitted packet
	// is treated as if it were received on the network.  Default is off.
	// Loopback is achieved by putting the LON-C hardware in loopback mode.
	virtual void setLoopbackMode(boolean on) = 0;
	virtual boolean getLoopbackMode() = 0;

	// Performs a self test of the comm port and returns a result.
	// May take a few seconds to 
	// return in failure case.  Self test will disrupt normal packet activity
	// but must not affect the network (i.e., cause spurious transmissions).
	// returns LTSTS_ERROR or LTSTS_OK
	// If possible, other functions should be executed while the self test is in progress.
	// A reset request during the self test would cause the self test to fail.
	// Outgoing requests and register reads would pend until the test completed.
	// If the driver is closed, this reports a failure.
	virtual LtSts selfTest() = 0;

	// This function reports the results of any power-on self test in the
	// LON-C hardware.  Return value is same as for "selfTest".
	virtual LtSts reportPowerSelfTest() = 0;

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

	virtual LtSts getFrequency(int& frequency) { return LTSTS_INVALIDSTATE; }

    // Get and set network buffers.  Links that do not have network buffers can do nothing.
    virtual LtSts getNetworkBuffers(LtReadOnlyData& readOnlyData){return LTSTS_OK;}
	virtual LtSts setNetworkBuffers(LtReadOnlyData& readOnlyData) {return LTSTS_OK;}

#ifdef WIN32
    // This is an optional method, only available on windows platforms.
    // Used to get LDV events (such as connect, disconnect)...
    virtual void registerLdvHandle(int ldvHandle)
    {
    }
#endif


	enum {
		LTLINK_MAXQUEUEDEPTH = 100,
		LTLINK_END
	};
protected:

};

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// Driver to network layer interface
//
class LtNetwork {
public:
	virtual ~LtNetwork() {}
	//
	// The following interfaces are required.
	//
	virtual void registerLink(LtLink& link) = 0;

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
								byte	ssiReg1=0,
								byte	ssiReg2=0) = 0;

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
	// Status reported may include: LTSTS_OK, LTSTS_INVALIDSTATE, LTSTS_RESET
	// Callbacks are always performed. If any outstanding during a reset, then
	// LTSTS_RESET is returned.
	virtual void reportTransceiverRegister(int n, int value, LtSts sts) = 0;

	// Notifies network client that a service pin depression occurred.
	virtual void servicePinDepressed() = 0;

    // Notifies network client that service pin has been released.
    virtual void servicePinReleased() = 0;

	// If a packet is sent with sendPacket specifying a referenceId,
	// then upon transmission, then this function is called.  
	//
	// Note that this callback may be invoked prior to a return from the
	// initiating "sendPacket".
	virtual void packetComplete(void* referenceId, LtSts sts ) = 0;

protected:
	// Granted, this is not really appropriate for a pure virtual base class but
	// we leave it in to retain base class compatibility with the Toshiba version
	// of this file (removing an unused member throws off the whole class table
	// and thus destroys binary compatibility).
	LtLink*		m_pLink;
};

#endif // _LTDRIVER_H
