//
// LtApplication.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtApplication.h#2 $
//

#ifndef LTAPPLICATION_H
#define LTAPPLICATION_H

/**
 * This interface defines the methods which must be implemented by an application
 * using the LonTalk Java Stack as implemented via the package LonTalk.Stack and
 * accessed via the class <A href="COM.Echelon.LonTalk.Api.Layer7Api.html">Layer7</A>.  
 * The functions in this interface will be called by the Layer7 class upon receipt 
 * of a message, a timeout, or some other indication.
 * <p>
 * Depending on how the protocol stack is initialized, these callbacks will either be made
 * directly from one of the protocol stack's threads or will be made in the application's
 * thread.  If the function to be performed is at all lengthy or requires sleeping
 * or synchronization, it is recommended that the callback notify or spawn a
 * separate thread of execution to perform the related processing.
 * This is a very strong recommendation if you choose to have the callbacks made
 * directly from the protocol stack's thread.  
 * <p>
 * The use of the protocol stack is limited to a single application.  If more than
 * one application needs to use the protocol stack, arbitration must be handled by an
 * intermediate layer sitting between the protocol stack and the applications.  This intermediate
 * layer then looks like the one and only application to the protocol stack.
 *
 * If the application wishes to support dynamic NVs or changeable NV types, it must
 * also implement the AppDynamic interface.
 */

#include "LtDriver.h"

class LtApplication
{

public:
	virtual ~LtApplication() {}

    /**
     * This method is invoked when the LonTalk Stack resets.  A reset could result
     * from a network management command or via the reset pin of the LON-C block.
     */
    virtual void reset()= 0;

    /**
     * This method is invoked when a wink request is received over the network.
     */
    virtual void wink()= 0;

    /**
     * This method is invoked when an offline request is received over the network.
     */
    virtual void offline()= 0;

    /**
     * This method is invoked when an online request is received over the network.
     */
    virtual void online()= 0;

	/**
	 * This method is invoked when an initialize command is received (indicating 
	 * the start of a full commission.
	 */
	virtual void initializing()= 0;

    /**
     * This method is invoked when a network variable update arrives either as
     * an update message or as a response to a poll.
     * @param index
     *              The network variable index.  For network variable arrays, this
     *              is the NV base index plus the array element index.
     * @param address
     *              The source address of the network variable update.
     */
    virtual void nvUpdateOccurs(LtNetworkVariable* pNv, int arrayIndex,
		LtNetworkVariable* pSourceNv, int sourceArrayIndex,
		LtIncomingAddress* address)
	{
	}

    /**
     * This method is invoked when a network variable update completes.  For unackd
     * or unackd repeat updates, successful completion means the message(s) was queued
     * to be sent.  For acknowledged updates, successful completion means all
     * acknowledgments were received.  For polls, successful completion means all
     * responses were received and at least one had valid data (i.e., target was
     * online, had matching selector and passed authentication).
     * @param index
     *              The network variable index.  For network variable arrays, this
     *              is the NV base index plus the array element index.
     * @param success
     *              true if the transaction completed successfully.
     */
    virtual void nvUpdateCompletes(LtNetworkVariable* pNv, int arrayIndex, boolean success)
	{
	}

	/**
	 * This method is invoked when an NV is added.  This occurs when dynamic NVs are
	 * added.  The application may choose to create a new version of the NV object
	 * which will be used in place of the one provided.  The stack will delete the
	 * old version in this case.  Return NULL or the old version to indicate no
	 * replacement.  Replacement might be used to derive from LtNetworkVariable.
	 */
	virtual LtNetworkVariable* nvAdded(LtNetworkVariable* pNv)
	{
		return pNv;
	}

	/**
	 * This method is called when an NV attribute changes.  It is also called during
	 * initialization to reflect any stored changes from the initial values.
	 */
	virtual void nvChanged(LtNetworkVariable* pNv, NvChangeType type) {}

	/**
	 * This method is called when a dynamic NV is deleted.  It is the application's
	 * responsibility to delete the object.
	 */
	virtual void nvDeleted(LtNetworkVariable* pNv) 
	{
		delete pNv;
	}

	/**
	 * This method is called to find the current length of a network variable
     * with changeable length from the application. If the application does not
     * support this functionality, the callback should return false, and the
     * stack will make a best guess at the length based on the last time
     * the NV was updated, either from the network or from the application.
	 */
	virtual boolean getCurrentNvLength(LtNetworkVariable* pNv, int &length) 
	{
		return false;
	}

    /**
     * This method is invoked when an application message is received.  Such a message
     * will have a code in the range of 0x00 to 0x4F.
     * @param msg
     *              The incoming message.
     */
    virtual void msgArrives(LtMsgIn* msg)= 0;

    /**
     * This method is invoked when a message completes.  For unackd
     * or unackd repeat updates, successful completion means the message(s) was queued
     * to be sent.  For acknowledged and request/response messages, successful
     * completion means all acknowledgments/responses were received.
     * @param tag
     *              The message tag provided in the original outgoing message.
     * @param success
     *              true if the transaction completed successfully.
     */
    virtual void msgCompletes(LtMsgTag* tag, boolean success)= 0;

    /**
     * This method is invoked when a response arrives.
     * @param response
     *              The incoming response.
     */
    virtual void respArrives(LtMsgTag* tag, LtRespIn* response)= 0;

    /**
     * This method informs the application when the service pin has been depressed.  The
     * protocol stack automatically sends a service pin message as a result of service pin 
     * depression.  This callback allows the application to do additional actions if so
     * desired.
     */
    virtual void servicePinPushed()= 0;


   /**
     * This method informs the application when the service pin has been released. 
     */
    virtual void servicePinHasBeenReleased() {};

    /**
     * This method informs the application when a flush request has completed.
     */
    virtual void flushCompletes()= 0;

    /**
     * This method is invoked in non-direct callback mode (see Layer7Api.java)
     * to cause the application thread to run.  It is up to the application whether
     * this is effected by suspend/resume, wait/notify or sleep/interrupt.  Once 
     * running, it is up to the application to invoke processApplicationEvents() to handle
     * the event.
     */
    virtual void applicationEventIsPending()= 0;
    
    /**
     * This method is used to read memory from a specified LonTalk address.
     * Only addresses registered with Layer7Api.registerMemory() are read.
     * return true if read is successful, false otherwise
     * @param address
     *          Address to read
     * @param data
     *          Byte array for data (read data.length bytes).
     */
    virtual boolean readMemory(int address, byte* data, int length)= 0;
    
    /**
     * This method is used to write memory at a specified LonTalk address.
     * Only addresses registered with Layer7Api.registerMemory() are written.
     * return true if write is successful, false otherwise
     * @param address
     *          Address to write
     * @param data
     *          Byte array of data to write
     */
    virtual boolean writeMemory(int address, byte* data, int length)= 0;

	/**
	 * This method is called when an application's persistent data is lost.
	 * @param reason
	 *			Reason for data loss.
	 */
	virtual void persistenceLost(LtPersistenceLossReason reason)= 0;

#if PERSISTENCE_TYPE_IS(FTXL)

	/**
	 * This method is called if the stack has scheduled an update of the
     * applications persistent data, but that update has not occured after
     * some specified timeout.  This may indicate a real time problem
	 * @param seconds
	 *			Number of seconds since the update was scheduled.
	 */
    void persistenceStarvation(unsigned seconds);
#endif

    /**
	 * This method is deprecated.  It remains for the benefit of the
	 * Java interface which still uses this.
     * This method fetches self identification data (SI data).  The SI data is
     * a byte array which meets the definition found in the host application
     * programmer's guide.
	 * @param length
	 *				pointer to the returned length
     * @return      a byte array containing the SI data.
     */
    virtual byte* getSiData(int* pLength) { return null; }  // returns byte array containing SI data

     /**
	 * This method is called when the service led status is changed 
	 */
    virtual void setServiceLedStatus(LtServicePinState state) {}

    /*
     * This method is used to get the current NV size
     */
    virtual unsigned getCurrentNvSize(const unsigned index) { return 0;}
    
    /*
     * This method is called to get the default Segment size
     */
    virtual unsigned getDefaultApplicationSegmentSize()  { return 0; }

    /*
     * This method is called to get the default non-volatile data serialization
     */
    virtual void defaultSerializeSegment(boolean toNvMemory, void* const pData, const size_t size) {}

#if FEATURE_INCLUDED(MONITOR_SETS)
	/**
	 * This method is invoked when a MonitorSet is added.
     * The application may choose to create a new version of the MonitorSet object
	 * which will be used in place of the one provided.  The stack will delete the
	 * old version in this case.  Return NULL or the old version to indicate no
	 * replacement.  Replacement might be used to derive from LtMonitorSet.
	 */
    virtual LtMonitorSet *monitorSetAdded(LtMonitorSet *pMonitorSet)
    {
        return pMonitorSet;
    }

	/**
	 * This method is called when an MonitorSet attribute changes. 
	 */
    virtual void monitorSetChanged(LtMonitorSet *pMonitorSet, McChangeType type) {};

	/**
	 * This method is called when a MonitorSet is deleted.  It is the application's
	 * responsibility to delete the object.
	 */
    virtual void monitorSetDeleted(LtMonitorSet *pMonitorSet)
    {
        delete pMonitorSet;
    }

	/**
	 * This method is invoked when a MonitorPoint is added.
     * The application may choose to create a new version of the MonitorPoint object
	 * which will be used in place of the one provided.  The stack will delete the
	 * old version in this case.  Return NULL or the old version to indicate no
	 * replacement.  Replacement might be used to derive from LtMonitorPoint.
	 */
    virtual LtMonitorPoint *monitorPointAdded(LtMonitorPoint *pMonitorPoint)
    {
        return pMonitorPoint;
    }

	/**
	 * This method is called when an MonitorPoint attribute changes. 
	 */
    virtual void monitorPointChanged(LtMonitorPoint *pMonitorPoint, McChangeType type) {};


	/**
	 * This method is called when a MonitorPoint is deleted.  It is the application's
	 * responsibility to delete the object.
	 */
    virtual void monitorPointDeleted(LtMonitorPoint *pMonitorPoint) 
    {
        delete pMonitorPoint;
    }
#endif
 };

#endif
