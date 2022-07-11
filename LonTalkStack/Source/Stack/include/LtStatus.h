#ifndef LTSTATUS_H
#define LTSTATUS_H

//
// LtStatus.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtStatus.h#1 $
//


/**
 * This class defines the LonTalk status information returned by the Layer7 API.
 */

class LTA_EXTERNAL_CLASS LtStatus 
{
public:
    /**
     * Number of transmission errors since last clear.  Includes CRC errors and
     * packets with preamble but no data.
     */
    int transmissionErrors;      
    
    /**
     * Number of transmit timeouts.  This counts the number of times acknowledged
     * or request response service was attempted without receiving all required 
     * acks/responses.
     */
    int transmitTimeouts;
    
    /**
     * Number of instances of receive transaction record exhaustion.  Each incoming
     * messages (other than unackd) is tracked for duplicate detection purposes using
     * a receive transaction record. 
     */
    int receiveTransactionFulls;
    
    /**
     * Number of lost messages.  This includes message lost at the network layer due
     * to an insufficient number of buffers.  This generally indicates that messages
     * addressed to the node are arriving faster than the application can handle
     * them.  If the condition is bursty in nature, then additional buffers may solve
     * the problem.
     */
    int lostMessages;
    
    /**
     * Number of missed messages.  This includes messages lost at the MAC layer due
     * to an insufficient number of buffers or insufficiently large buffers.  This
     * generally means that messages are arriving at the device faster than the 
     * protocol stack can process them.  If the condition is due to traffic bursts,
     * then additional buffers may solve the problem.
     */
    int missedMessages;
    
    /**
     * Reset cause.  The reset cause has the following form:
     * <p><pre>
     * XXXXXXX1     - power-up reset
     * XXXXXX10     - external reset
     * XXXX1100     - watchdog timer reset
     * XXX10100     - internal/software reset
     * </pre>
     */
    int resetCause;
    
    /** 
     * State.  The device state as follows:
     * <p><pre>
     * BXXXOSSS
     * where
     *  B is 1 for bypass offline, 0 otherwise.
     *  O is 1 for offline, 0 for online.
     *  SSS is 
     *      2   unconfigured
     *      3   applicationless
     *      4   configured
     *      6   hard-offline
     * </pre>
     */
    int state;
    
    /** 
     * Version. The device firmware version number.  128 is used by non-Neuron
     * implementations. 
     */
    int version;
    
    /**
     * Error log.  The most recently logged error (see Neuron Chip Data Book for
     * a description of error codes).
     */
    int error;
    
    /**
     * Model number.  The microprocessor model number.  128 is used by non-Neuron
     * implementations.
     */
    int model;
protected:
    friend class LtStackBlob;
    void package(LtBlob *pBlob);

public:
	LtStatus();
	void fromLonTalk(byte* pData);
};

#endif
