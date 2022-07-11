/***************************************************************
 *  Filename: FtxlVectorHandlers .cpp
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
 *  Description:  registration and implementation of the callbacks vectors
 *
 ****************************************************************/

#include <stdio.h>
#include <string.h>
#include "FtxlApiInternal.h"
#include "FtxlTypes.h"
#include "LtPlatform.h"

#if PRODUCT_IS(IZOT)

#ifdef WIN32
#include <memory.h>
#endif

// all the callback vectors
LonCallbackVectors theLonCallbackVectors;

#define USE_DEFAULT_IMPLEMENTATION 1

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifdef USE_DEFAULT_IMPLEMENTATION
FTXL_EXTERNAL_FN const LonNvdHandle DefaultNvdOpenForRead(const LonNvdSegmentType type);
FTXL_EXTERNAL_FN const LonNvdHandle DefaultNvdOpenForWrite(const LonNvdSegmentType type, 
                                      const size_t size);
FTXL_EXTERNAL_FN void DefaultNvdClose(const LonNvdHandle handle);
FTXL_EXTERNAL_FN void DefaultNvdDelete(const LonNvdSegmentType type);
FTXL_EXTERNAL_FN const LonApiError DefaultNvdRead(const LonNvdHandle handle, 
					         const size_t offset, 
					         const size_t size, 
					         void * const pBuffer); 
FTXL_EXTERNAL_FN const LonApiError DefaultNvdWrite (const LonNvdHandle handle, 
                               const size_t offset, 
                               const size_t size, 
                               const void* const pData); 
FTXL_EXTERNAL_FN const LonBool DefaultNvdIsInTransaction(const LonNvdSegmentType type);
FTXL_EXTERNAL_FN const LonApiError DefaultNvdEnterTransaction(const LonNvdSegmentType type);
FTXL_EXTERNAL_FN const LonApiError DefaultNvdExitTransaction(const LonNvdSegmentType type);
#endif

/*
 * ******************************************************************************
 * SECTION: Support functions
 * ******************************************************************************
 *
 * This section contains the support functions used to support the NVD vector handlers functions
 * defined in this file.
 */

/*
 *  Function: LonDeregisterAllCallbacks
 *  Deregister all callbackGet functions.   
 *  It is not an error to deregister a callback twice, but only an
 *  unclaimed callback can be registered.

 *  Returns:
 *  void.
 * 
 *  Remarks:
 *  Returns the IP address and port used for an IP852 interface.  
 */
FTXL_EXTERNAL_FN void LonDeregisterAllCallbacks(void)
{
    APIDebug("LonDeregisterAllCallbacks\n");
    memset((void *)&theLonCallbackVectors, 0, sizeof(theLonCallbackVectors));
    APIDebug("LonDeregisterAllCallbacks - %d\n", LonApiNoError);
}

/*
 *  Callback: LonEventReady
 *  Signals that an event is ready to be processed.  
 *  This callback function must be first registered by calling the 
 *  LonEventReadyRegistrar
 *
 * Remarks:
 * This function is called whenever the IzoT protocol stack adds a new 
 * event for the  application to process.  Typically, this function is used by 
 * the application to set an operating system event which is then used to 
 * signal the application thread to call the <LonEventPump> function.
 *
 * Note that this is an immediate callback, and therefore can be called at any 
 * time, from any thread.  The application cannot call any IzoT functions from 
 * this callback. 
 */
void LonEventReady(void)
{
    if (theLonCallbackVectors.eventReady) 
    {
        try
        {
            CALLBACK_EXEC("LonEventReady");
            theLonCallbackVectors.eventReady();
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonEventReady");
        }
    }
    else
        CALLBACK_NOT_REGISTERED("LonEventReady");
}

FTXL_EXTERNAL_FN const LonApiError
LonEventReadyRegistrar(LonEventReadyFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.eventReady = handler;
    CALLBACK_REGISTER("LonEventReady", sts);
    return sts;
}

/*
 *  Callback: LonGetCurrentNvSize
 *  Gets the current size of a network variable.
 *
 *  Parameters:
 *  index - the local index of the network variable
 *
 *  Returns:     
 *  Current size of the network variable. Zero if the index is invalid.
 *
 *  Remarks:
 *  If the network variable size is fixed, this function should return 
 *  <LonGetDeclaredNvSize>. If the network variable size is changeable, the 
 *  current size should be returned. The default implementation for changeable 
 *  type network variables returns 0, and must be updated by the application 
 *  developer.  
 *
 *  The IzoT protocol stack will not propagate a network variable with 
 *  size 0, nor will it generate an update event if a network variable update 
 *  is received from the network when the current network variable size is 0.
 *
 *  Note that even though this is a callback function, it *is* legal for the 
 *  application to call <LonGetDeclaredNvSize> from this callback.
 */
const unsigned LonGetCurrentNvSize(const unsigned index)
{
    if (theLonCallbackVectors.getCurrentNvSize)
    {
        try
        {
            CALLBACK_EXEC_IDX("LonGetCurrentNvSize", index);
            return theLonCallbackVectors.getCurrentNvSize(index);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION_IDX("LonGetCurrentNvSize", index);
            return 0;
        }
    }
    else
    {
        // Callback vector is NULL.  Provides default implementation
        CALLBACK_NOT_REGISTERED_DEF_IDX("LonGetCurrentNvSize", index);
        return GetDefaultCurrentNvSize(index);
    }
}

FTXL_EXTERNAL_FN const LonApiError
LonGetCurrentNvSizeRegistrar(LonGetCurrentNvSizeFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.getCurrentNvSize = handler;
    CALLBACK_REGISTER("LonGetCurrentNvSize", sts);
    return sts;
}

/* 
 *  Event: LonReset
 *  Occurs when the IzoT protocol stack has been reset.
 *
 *  Parameters:
 *  pResetNotification - <LonResetNotification> structure with capabilities 
 *      and identifying data
 *
 *  Remarks:
 *  The pointer to <LonResetNotification> is provided for call compatibility with 
 *  the ShortStack LonTalk Compact API.  For IzoT, the value of 
 *  pResetNotification is always NULL.
 * 
 *  Whenever the IzoT device has been reset, the mode of the device is changed 
 *  to *online*, but no LonOnline() event is generated.
 *  
 *  Note that resetting the IzoT device only affects the IzoT protocol 
 *  stack and does not cause a processor or application software reset.
 */
void LonReset(const LonResetNotification* const pResetNotification)
{
    if (theLonCallbackVectors.reset)
    {
        try
        {
            CALLBACK_EXEC("LonReset");
            theLonCallbackVectors.reset(pResetNotification);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonReset");
        }
    }
    else
        CALLBACK_NOT_REGISTERED("LonReset");
}

FTXL_EXTERNAL_FN const LonApiError
LonResetRegistrar(LonResetFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.reset = handler;
    CALLBACK_REGISTER("LonReset", sts);
    return sts;
}

/*
 *  Event: LonWink
 *  Occurs when the IzoT device receives a WINK command.
 *
 *  Remarks:
 *  This event is not triggered when wink sub-commands (extended install 
 *  commands) are received.
 */
void LonWink(void)
{
    if (theLonCallbackVectors.wink)
    {
        try
        {
            CALLBACK_EXEC("LonWink");
            theLonCallbackVectors.wink();
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonWink");
        }
    }
    else
        CALLBACK_NOT_REGISTERED("LonWink");
}

FTXL_EXTERNAL_FN const LonApiError
LonWinkRegistrar(LonWinkFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.wink = handler;
    CALLBACK_REGISTER("LonWink", sts);
    return sts;
}

/*
 *  Event: LonOffline
 *  Occurs when the IzoT device has entered the offline state.
 *
 *  Remarks:
 *  While the device is offline, the IzoT protocol stack will not 
 *  generate network variable updates, and will return an error when <
 *  LonPropagateNv> is called.
 *
 *  Note that offline processing in IzoT differs from that in ShortStack.  
 *  When a ShortStack Micro Server receives a request to go offline, it sends 
 *  the request to the ShortStack LonTalk Compact API, which calls the 
 *  application callback <LonOffline>.  The Micro Server does not actually go 
 *  offline until the <LonOffline> callback function returns and the 
 *  ShortStack LonTalk Compact API sends a confirmation message to the Micro 
 *  Server.  In contrast, the IzoT device goes offline as soon as it receives 
 *  the offline request. The <LonOffline> event is handled asynchronously.
 */
void LonOffline(void)
{
    if (theLonCallbackVectors.offline)
    {
        try
        {
            CALLBACK_EXEC("LonOffline");
            theLonCallbackVectors.offline();
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonOffline");
        }
    }
    else
        CALLBACK_NOT_REGISTERED("LonOffline");
}

FTXL_EXTERNAL_FN const LonApiError
LonOfflineRegistrar(LonOfflineFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.offline = handler;
    CALLBACK_REGISTER("LonOffline", sts);
    return sts;
}

/*
 *  Event: LonOnline
 *  Occurs when the IzoT device has entered the online state.
 */
void LonOnline(void)
{
    if (theLonCallbackVectors.online)
    {
        try
        {
            CALLBACK_EXEC("LonOnline");
            theLonCallbackVectors.online();
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonOnline");
        }
    }
    else
        CALLBACK_NOT_REGISTERED("LonOnline");
}

FTXL_EXTERNAL_FN const LonApiError
LonOnlineRegistrar(LonOnlineFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.online = handler;
    CALLBACK_REGISTER("LonOnline", sts);
    return sts;
}

/*
 *  Event: LonServicePinPressed
 *  Occurs when the service pin has been activated. 
 *
 *  Remarks:
 *  Note that the IzoT protocol stack sends a service pin message 
 *  automatically any time the service pin has been pressed.
 */
void LonServicePinPressed(void)
{
    if (theLonCallbackVectors.servicePinPressed)
    {
        try
        {
            CALLBACK_EXEC("LonServicePinPressed");
            theLonCallbackVectors.servicePinPressed();
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonServicePinPressed");
        }
    }
    else
        CALLBACK_NOT_REGISTERED("LonServicePinPressed");
}

FTXL_EXTERNAL_FN const LonApiError
LonServicePinPressedRegistrar(LonServicePinPressedFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.servicePinPressed = handler;
    CALLBACK_REGISTER("LonServicePinPressed", sts);
    return sts;
}

/*
 *  Event: LonServicePinHeld
 *  Occurs when the service pin has been continuously activated for a 
 *  configurable time.
 *
 *  Remarks:
 *  Not applicable for IzoT device. 
 *
 */
void LonServicePinHeld(void)
{
    if (theLonCallbackVectors.servicePinHeld)
    {
        try
        {
            CALLBACK_EXEC("LonServicePinHeld");
            theLonCallbackVectors.servicePinHeld();
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonServicePinHeld");
        }
    }
    else
        CALLBACK_NOT_REGISTERED("LonServicePinHeld");
}

FTXL_EXTERNAL_FN const LonApiError
LonServicePinHeldRegistrar(LonServicePinHeldFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.servicePinHeld = handler;
    CALLBACK_REGISTER("LonServicePinHeld", sts);
    return sts;
}

/*
 *  Event: LonNvUpdateOccurred
 *  Occurs when new input network variable data has arrived.
 *
 *  Parameters:
 *  index - global index (local to the device) of the network variable in question 
 *  pSourceAddress - pointer to source address description 
 *
 *  Remarks:
 *  The network variable with local index given in this event handler has been 
 *  updated with a new value. The new value is already stored in the network 
 *  variable's location; access the value through the global variable 
 *  representing the network variable, or obtain the pointer to the network 
 *  variable's value from the <LonGetNvValue> function. The pSourceAddress 
 *  pointer is only valid for the duration of this event handler.
 * 
 *  For an element of a network variable array, the index is the global network 
 *  variable index plus the array-element index. For example, if nviVolt[0] has
 *  global network variable index 4, then nviVolt[1] has global network variable 
 *  index 5.
 */
void LonNvUpdateOccurred(const unsigned index, const LonReceiveAddress* const pSourceAddress)
{
    if (theLonCallbackVectors.nvUpdateOccurred)
    {
        try
        {
            CALLBACK_EXEC_IDX("LonNvUpdateOccurred", index);
            theLonCallbackVectors.nvUpdateOccurred(index, pSourceAddress);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION_IDX("LonNvUpdateOccurred", index);
        }
    }
    else
        CALLBACK_NOT_REGISTERED_IDX("LonNvUpdateOccurred", index);
}

FTXL_EXTERNAL_FN const LonApiError
LonNvUpdateOccurredRegistrar(LonNvUpdateOccurredFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.nvUpdateOccurred = handler;
    CALLBACK_REGISTER("LonNvUpdateOccurred", sts);
    return sts;
}

/*
 *  Event:   LonNvUpdateCompleted
 *  Signals completion of a network variable update.
 *
 *  Parameters:
 *  index - global index (local to the device) of the network variable that 
 *          was updated
 *  success - indicates whether the update was successful or unsuccessful 
 *
 *  Remarks:
 *  This event handler signals the completion of a network variable update or 
 *  poll transaction (see <LonPropagateNv> and <LonPollNv>). For 
 *  unacknowledged or repeated messages, the transaction is complete when the 
 *  message has been sent with the configured number of retries. For 
 *  acknowledged messages, it is successfully complete when the IzoT  
 *  protocol stack receives an acknowledgement from each of the destination 
 *  devices, and is unsuccessfully complete if not all acknowledgements are 
 *  received.  Poll requests always use the request service type, and 
 *  generates a successful completion if responses are received from all 
 *  expected devices, and generates a failed completion otherwise.
 */
void LonNvUpdateCompleted(const unsigned index, const LonBool success)
{
    if (theLonCallbackVectors.nvUpdateCompleted)
    {
        try
        {
            CALLBACK_EXEC_IDX("LonNvUpdateCompleted", index);
            theLonCallbackVectors.nvUpdateCompleted(index, success);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION_IDX("LonNvUpdateCompleted", index);
        }
    }        
    else
        CALLBACK_NOT_REGISTERED_IDX("LonNvUpdateCompleted", index);
}

FTXL_EXTERNAL_FN const LonApiError
LonNvUpdateCompletedRegistrar(LonNvUpdateCompletedFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.nvUpdateCompleted = handler;
    CALLBACK_REGISTER("LonNvUpdateCompleted", sts);
    return sts;
}


/*
 *  Event:   LonNvAdded
 *  A dynamic network variable has been added.
 *
 *  Parameters:
 *  index - The index of the dynamic network variable that has been added 
 *  pNvDef - Pointer to the network variable definition <LonNvDefinition> 
 *
 *  Remarks:
 *  The IzoT protocol stack calls this function when a dynamic network 
 *  variable has been added.  During device startup, this function is called 
 *  for each dynamic network variable that had been previously defined.  The 
 *  pNvDef pointer, along with all of its contents, is invalid when the 
 *  function returns.  If the application needs to maintain this information, 
 *  it must copy the data, taking care to avoid copying the pointers contained 
 *  within <LonNvDefinition>.
 *
 *  Note that when a network variable is first added, the name and the
 *  self-documentation string may be blank.  A network manager may update the 
 *  name or the self-documentation string in a subsequent message, and the 
 *  IzoT protocol stack will call the <LonNvTypeChanged> event handler.
 */
void LonNvAdded(const unsigned index, const LonNvDefinition* const pNvDef)
{
    if (theLonCallbackVectors.nvAdded)
    {
        try
        {
            CALLBACK_EXEC_IDX("LonNvAdded", index);
            theLonCallbackVectors.nvAdded(index, pNvDef);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION_IDX("LonNvAdded", index);
        }
    }
    else
        CALLBACK_NOT_REGISTERED_IDX("LonNvAdded", index);
}

FTXL_EXTERNAL_FN const LonApiError
LonNvAddedRegistrar(LonNvAddedFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.nvAdded = handler;
    CALLBACK_REGISTER("LonNvAdded", sts);
    return sts;
}

/*
 *  Event:   LonNvTypeChanged
 *  One or more attributes of a dynamic network variable has been changed.
 *
 *  Parameters:
 *  index - The index of the dynamic network variable that has been changed 
 *  pNvDef - Pointer to the network variable definition <LonNvDefinition>
 *
 *  Remarks:
 *  The IzoT protocol stack calls this function when a dynamic network 
 *  variable definition been changed.  The pNvDef pointer, along with all of 
 *  its contents, is invalid when the function returns.  If the application 
 *  needs to maintain this information, it must copy the data, taking care to 
 *  avoid copying the pointers contained within <LonNvDefinition>.
 */
void LonNvTypeChanged(const unsigned index, const LonNvDefinition* const pNvDef)
{
    if (theLonCallbackVectors.nvTypeChanged)
    {
        try
        {
            CALLBACK_EXEC_IDX("LonNvTypeChanged", index);
            theLonCallbackVectors.nvTypeChanged(index, pNvDef);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION_IDX("LonNvTypeChanged", index);
        }
    }
    else
        CALLBACK_NOT_REGISTERED_IDX("LonNvTypeChanged", index);
}

FTXL_EXTERNAL_FN const LonApiError
LonNvTypeChangedRegistrar(LonNvTypeChangedFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.nvTypeChanged = handler;
    CALLBACK_REGISTER("LonNvTypeChanged", sts);
    return sts;
}

/*
 *  Event:   LonNvDeleted
 *  A dynamic network variable has been deleted.
 *
 *  Parameters:
 *  index - The index of the dynamic network variable that has been deleted 
 *
 *  Remarks:
 *  The IzoT protocol stack calls this function when a dynamic network 
 *  variable has been deleted.  
 */
void LonNvDeleted(const unsigned index)
{
    if (theLonCallbackVectors.nvDeleted)
    {
        try
        {
            CALLBACK_EXEC_IDX("LonNvDeleted", index);
            theLonCallbackVectors.nvDeleted(index);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION_IDX("LonNvDeleted", index);
        } 
    }
    else
        CALLBACK_NOT_REGISTERED_IDX("LonNvDeleted", index);
}

FTXL_EXTERNAL_FN const LonApiError
LonNvDeletedRegistrar(LonNvDeletedFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.nvDeleted = handler;
    CALLBACK_REGISTER("LonNvDeleted", sts);
    return sts;
}

/*
 *  Event: LonMsgArrived
 *  Occurs when an application message arrives.
 *
 *  Parameters:
 *  pAddress - source and destination address (see <LonReceiveAddress>)
 *  correlator - correlator to be used with <LonSendResponse>
 *  authenticated - TRUE if the message was (successfully) authenticated
 *  code - message code
 *  pData - pointer to message data bytes, might be NULL if dataLength is zero
 *  dataLength - length of bytes pointed to by pData
 *
 *  Remarks:
 *  This event handler reports the arrival of a message that is neither a network 
 *  variable message or a non-Nv message that is otherwise processed by the 
 *  IzoT device (such as a network management command). Typically, this is used 
 *  with application message codes in the value range indicated by the 
 *  <LonApplicationMessageCode> enumeration. All pointers are only valid for 
 *  the duration of this event handler. 
 *
 *  If the message is a request message, then the function must deliver a 
 *  response using <LonSendResponse> passing the provided *correlator*.  
 *  Alternatively, if for any reason the application chooses not to respond to 
 *  a request, it must explicitly release the correlator by calling 
 *  <LonReleaseCorrelator>.
 *
 *  Application messages are always delivered to the application, regardless 
 *  of whether the message passed authentication or not. It is up to the 
 *  application to decide whether authentication is required for any given 
 *  message and compare that fact with the authenticated flag. The 
 *  authenticated flag is clear (FALSE) for non-authenticated messages and for
 *  authenticated messages that do not pass authentication. The authenticated 
 *  flag is set only for correctly authenticated messages. 
 */
void LonMsgArrived(const LonReceiveAddress* const pAddress, 
                   const LonCorrelator correlator,
                   const LonBool priority, 
                   const LonServiceType serviceType, 
                   const LonBool authenticated, 
                   const LonByte code, 
                   const LonByte* const pData, const unsigned dataLength)
{
    if (theLonCallbackVectors.msgArrived)
    {
        try
        {
            CALLBACK_EXEC("LonMsgArrived");
            theLonCallbackVectors.msgArrived(pAddress, correlator, priority,
                            serviceType, authenticated, code,
                            pData, dataLength);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonMsgArrived");
        }
    }
    else
        CALLBACK_NOT_REGISTERED("LonMsgArrived");
}

FTXL_EXTERNAL_FN const LonApiError
LonMsgArrivedRegistrar(LonMsgArrivedFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.msgArrived = handler;
    CALLBACK_REGISTER("LonMsgArrived", sts);
    return sts;
}

/*
 *  Event: LonResponseArrived
 *  Occurs when a response arrives.
 *
 *  Parameters:
 *  pAddress - source and destination address used for response (see 
 *             <LonResponseAddress>)
 *  tag - tag to match the response to the request
 *  code - response code
 *  pData - pointer to response data, may by NULL if dataLength is zero
 *          dataLength - number of bytes available through pData.
 *
 *  Remarks:
 *  This event handler is called when a response arrives.  Responses may be 
 *  sent by other devices when the IzoT device sends a message using 
 *  <LonSendMsg> with a <LonServiceType> of *LonServiceRequest*.
 */
void LonResponseArrived(const LonResponseAddress* const pAddress, 
                        const unsigned tag, 
                        const LonByte code, 
                        const LonByte* const pData, const unsigned dataLength)
{
    if (theLonCallbackVectors.responseArrived)
    {
        try
        {
            CALLBACK_EXEC("LonResponseArrived");
            theLonCallbackVectors.responseArrived(pAddress, tag, code,
                            pData, dataLength);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonResponseArrived");
        }
    }
    else
        CALLBACK_NOT_REGISTERED("LonResponseArrived");
}

FTXL_EXTERNAL_FN const LonApiError
LonResponseArrivedRegistrar(LonResponseArrivedFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.responseArrived = handler;
    CALLBACK_REGISTER("LonResponseArrived", sts);
    return sts;
}

/* 
 *  Event: LonMsgCompleted
 *  Occurs when a message transaction has completed.  See <LonSendMsg>.
 *
 *  Parameters:
 *  tag - used to correlate the event with the message sent  
 *        Same as the *tag* specified in the call to <LonSendMsg> 
 *  success - TRUE for successful completion, otherwise FALSE
 *
 *  Remarks:
 *  For unacknowledged or repeated messages, the transaction is complete when 
 *  the message has been sent with the configured number of retries. For 
 *  acknowledged messages, the IzoT protocol stack calls 
 *  <LonMsgCompleted> with *success* set to TRUE after receiving 
 *  acknowledgments from all of the destination devices, and calls 
 *  <LonMsgCompleted> with *success* set to FALSE if the transaction timeout 
 *  period expires before receiving acknowledgements  from all destinations.  
 *  Likewise, for request messages, the transaction is considered successful 
 *  when the IzoT protocol stack receives a response from each of the 
 *  destination devices, and unsuccessful if the transaction timeout expires 
 *  before responses have been received from all destination devices.
 */
void LonMsgCompleted(const unsigned tag, const LonBool success)
{
    if (theLonCallbackVectors.msgCompleted)
    {
        try
        {
            CALLBACK_EXEC("LonMsgCompleted");
            theLonCallbackVectors.msgCompleted(tag, success);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonMsgCompleted");
        }
    }
    else
        CALLBACK_NOT_REGISTERED("LonMsgCompleted");
}

FTXL_EXTERNAL_FN const LonApiError
LonMsgCompletedRegistrar(LonMsgCompletedFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.msgCompleted = handler;
    CALLBACK_REGISTER("LonMsgCompleted", sts);
    return sts;
}

/*
 *  Event: LonServiceLedStatus
 *  Occurs when the service pin change the state. 
 *
 *  Parameters:
 *  state - determines the state of the service pin.  "state" values are  
 *          defined as per the LON-C specification.
 *
 *  Remarks:
 *  Note that the IzoT protocol stack calls <LonServiceLedStatus>
 *  when the status of service pin is changed. 
 */
void LonServiceLedStatus(LtServicePinState state)
{
    if (theLonCallbackVectors.serviceLedStatus)
    {
        try
        {
            CALLBACK_EXEC("LonServiceLedStatus");
            theLonCallbackVectors.serviceLedStatus(state);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonServiceLedStatus");
        }
    }
    else
        CALLBACK_NOT_REGISTERED("LonServiceLedStatus");
}

FTXL_EXTERNAL_FN const LonApiError
LonServiceLedStatusRegistrar(LonServiceLedStatusFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.serviceLedStatus = handler;
    CALLBACK_REGISTER("LonServiceLedStatus", sts);
    return sts;
}


/*
 * ******************************************************************************
 * SECTION: DMF CALLBACK PROTOTYPES
 * ******************************************************************************
 *
 *  This section defines the prototypes for the IzoT Device Stack API callback 
 *  functions supporting direct memory files (DMF) read and write.  This file
 *  contains complete default implementations of these callback functions. 
 *
 *  Remarks:
 *  Callback functions are called by the IzoT protocol stack 
 *  immediately, as needed, and may be called from any IzoT task.  The 
 *  application *must not* call into the IzoT Device Stack API from within a callback.
 */
 
/* 
 *  Callback: LonMemoryRead
 *  Read memory in the IzoT device's memory space.
 *
 *  Parameters:
 *  address - virtual address of the memory to be read
 *  size - number of bytes to read
 *  pData - pointer to a buffer to store the requested data
 *
 *  Remarks:
 *  The IzoT protocol stack calls <LonMemoryRead> whenever it receives 
 *  a network management memory read request that fits into the registered 
 *  file access window. This callback function is used to read data starting 
 *  at the specified virtual memory address within the IzoT Transceiver. This 
 *  function applies to reading template files, CP value files, user-defined 
 *  files, and possibly other data. The address space for this command is 
 *  limited to the IzoT Transceiver's 64 KB address space.
 *
 */
const LonApiError LonMemoryRead(const unsigned address, 
						        const unsigned size,
						        void* const pData)
{
    if (theLonCallbackVectors.memoryRead)
    {
        try
        {
            CALLBACK_EXEC("LonMemoryRead");
            return theLonCallbackVectors.memoryRead(address, size, pData);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonMemoryRead");
            return LonApiCallbackExceptionError;
        }
    }
    else
    {
#if     LON_DMF_ENABLED
        CALLBACK_NOT_REGISTERED("LonMemoryRead");
        return LonApiCallbackNotRegistered;
#else
        CALLBACK_NOT_REGISTERED("LonMemoryRead");
        return LonApiNotAllowed;
#endif   /* LON_DMF_ENABLED */
    }
}

FTXL_EXTERNAL_FN const LonApiError
LonMemoryReadRegistrar(LonMemoryReadFunction handler)
{
    LonApiError sts = LonApiNoError;
#if     LON_DMF_ENABLED
    theLonCallbackVectors.memoryRead = handler;
#else
    sts = LonApiNotAllowed;
#endif   /* LON_DMF_ENABLED */
    CALLBACK_REGISTER("LonMemoryRead", sts);
    return sts;
}


/* 
 *  Callback: LonMemoryWrite
 *  Update memory in the IzoT device's memory space.
 *
 *  Parameters:
 *  address - virtual address of the memory to be update
 *  size - number of bytes to write
 *  pData - pointer to the data to write
 *
 *  Remarks:
 *  The IzoT protocol stack calls <LonMemoryWrite> whenever it 
 *  receives a network management memory write request that fits into the 
 *  registered file access window. This callback function is used to write 
 *  data at the specified virtual memory address for the IzoT Transceiver.  
 *  This function applies to CP value files, user-defined files, and possibly 
 *  other data. The address space for this command is limited to the IzoT 
 *  Transceivers 64 KB address space.  The IzoT protocol stack
 *  automatically calls the <LonNvdAppSegmentHasBeenUpdated> function to 
 *  schedule an update whenever this callback returns *LonApiNoError*.
 *
 */
const LonApiError LonMemoryWrite(const unsigned address, 
						         const unsigned size,
						         const void* const pData)
{
    if (theLonCallbackVectors.memoryWrite)
    {
        try
        {
            CALLBACK_EXEC("LonMemoryWrite");
            return theLonCallbackVectors.memoryWrite(address, size, pData);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonMemoryWrite");
            return LonApiCallbackExceptionError; 
        }
    }
    else
    {
#if LON_DMF_ENABLED
        CALLBACK_NOT_REGISTERED("LonMemoryWrite");
        return LonApiCallbackNotRegistered;
#else
        CALLBACK_NOT_REGISTERED("LonMemoryWrite");
        return LonApiNotAllowed;
#endif  /* LON_DMF_ENABLED */
    }
}

FTXL_EXTERNAL_FN const LonApiError
LonMemoryWriteRegistrar(LonMemoryWriteFunction handler)
{
    LonApiError sts = LonApiNoError;
#if     LON_DMF_ENABLED
    theLonCallbackVectors.memoryWrite = handler;
#else
    sts = LonApiNotAllowed;
#endif   /* LON_DMF_ENABLED */
    CALLBACK_REGISTER("LonMemoryRead", sts);
    return sts;
}

/*
 * ******************************************************************************
 * SECTION: NVD CALLBACK PROTOTYPES
 * ******************************************************************************
 *
 *  This section contains the IzoT Device Stack API callback functions supporting 
 *  memory for Non-Volatile Data (NVD). 
 *
 *  Remarks:
 *  Example implementations of these functions are provided in 
 *  FtxlNvdFlashDirect.c and FtxlNvdFlashFs.c.  The application developer may 
 *  need to modify these functions to fit the type of non-volatile storage 
 *  supported by the application.
 *
 *  Callback functions are called by the IzoT protocol stack 
 *  immediately, as needed, and may be called from any IzoT task.  The 
 *  application *must not* call into the IzoT Device Stack API from within a 
 *  callback.
 */

/* 
 *  Callback: LonNvdOpenForRead
 *  Open a non-volatile data segment for reading.
 *
 *  Parameters:
 *  type -  type of non-volatile data to be opened
 *
 *  Returns:
 *  <LonNvdHandle>.   
 *
 *  Remarks:
 *  This function opens the data segment for reading.  If the file exists and 
 *  can be opened, this function returns a valid application-specific 32-bit 
 *  value as the handle.  Otherwise it returns 0.  The handle returned by this 
 *  function will be used as the first parameter when calling <LonNvdRead>.  
 *  The application must maintain the handle used for each segment. The 
 *  application can invalidate a handle when <LonNvdClose> is called for that 
 *  handle.  
 */
const LonNvdHandle LonNvdOpenForRead(const LonNvdSegmentType type)
{
    if (theLonCallbackVectors.nvdOpenForRead)
    {
        try
        {
            CALLBACK_EXEC("LonNvdOpenForRead");
            return theLonCallbackVectors.nvdOpenForRead(type);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonNvdOpenForRead");
            return 0;
        }
    }
    else
    {
#ifdef USE_DEFAULT_IMPLEMENTATION
        // Execute default implementation 
        CALLBACK_NOT_REGISTERED_DEF("LonNvdOpenForRead");
        return DefaultNvdOpenForRead(type);
#else
    
        CALLBACK_NOT_REGISTERED("LonNvdOpenForRead");
        return 0;
#endif
    }
}

FTXL_EXTERNAL_FN const LonApiError
LonNvdOpenForReadRegistrar(LonNvdOpenForReadFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.nvdOpenForRead = handler;
    CALLBACK_REGISTER("LonNvdOpenForRead", sts);
    return sts;
}

/* 
 *  Callback: LonNvdOpenForWrite
 *  Open a non-volatile data segment for writing.
 *
 *  Parameters:
 *  type - type of non-volatile data to be opened
 *  size - size of the data to be stored
 *
 *  Returns:
 *  <LonNvdHandle>.   
 *
 *  Remarks:
 *  This function is called by the IzoT protocol stack after changes 
 *  have been made to data that is backed by persistent storage.  Note that 
 *  many updates might have been made, and this function is called only after 
 *  the stack determines that all updates have been completed. 
 *
 *  An error value is returned if the data cannot be written.
 */
const LonNvdHandle LonNvdOpenForWrite(const LonNvdSegmentType type, 
                                      const size_t size)
{
    if (theLonCallbackVectors.nvdOpenForWrite)
    {
        try
        {
            CALLBACK_EXEC("LonNvdOpenForWrite");
            return theLonCallbackVectors.nvdOpenForWrite(type, size);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonNvdOpenForWrite");
            return 0;
        }
    }
    else
    {
#ifdef USE_DEFAULT_IMPLEMENTATION
        // Execute default implementation 
        CALLBACK_NOT_REGISTERED_DEF("LonNvdOpenForWrite");
        return DefaultNvdOpenForWrite(type, size);
#else
        CALLBACK_NOT_REGISTERED("LonNvdOpenForWrite");
        return 0;
#endif
    }
}

FTXL_EXTERNAL_FN const LonApiError
LonNvdOpenForWriteRegistrar(LonNvdOpenForWriteFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.nvdOpenForWrite = handler;
    CALLBACK_REGISTER("LonNvdOpenForWrite", sts);
    return sts;
}

/* 
 *  Callback: LonNvdClose
 *  Close a non-volatile data segment.
 *
 *  Parameters:
 *  handle - handle of the non-volatile segment returned by <LonNvdOpenForRead> 
 *           or <LonNvdOpenForWrite>
 *
 *  Remarks:
 *  This function closes the non-volatile memory segment associated with this 
 *  handle and invalidates the handle. 
 */
void LonNvdClose(const LonNvdHandle handle)
{
    if (theLonCallbackVectors.nvdClose)
    {
        try
        {
            CALLBACK_EXEC("LonNvdClose");
            theLonCallbackVectors.nvdClose(handle);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonNvdClose");
        }
    }
    else
    {
#ifdef USE_DEFAULT_IMPLEMENTATION
        // Execute default implementation 
        CALLBACK_NOT_REGISTERED_DEF("LonNvdClose");
        DefaultNvdClose(handle);
#else
        CALLBACK_NOT_REGISTERED("LonNvdClose");
#endif
    }
}

FTXL_EXTERNAL_FN const LonApiError
LonNvdCloseRegistrar(LonNvdCloseFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.nvdClose = handler;
    CALLBACK_REGISTER("LonNvdClose", sts);
    return sts;
}

/* 
 *  Callback: LonNvdDeleteEx
 *  Delete non-volatile data segment.
 *
 *  Parameters:
 *  type - type of non-volatile data to be deleted
 *
 *  Remarks:
 *  This function is used to delete the non-volatile memory segment referenced 
 *  by the data type.  The IzoT Device Stack API attempts to close the file before 
 *  deleting it.  
 *
 *  Note that this function can be called even if the segment does not exist.  
 *  It is not necessary for this function to actually destroy the data or free it.
 */
void LonNvdDelete(const LonNvdSegmentType type)
{
    if (theLonCallbackVectors.nvdDelete)
    {
        try
        {
            CALLBACK_EXEC("LonNvdDelete");
            theLonCallbackVectors.nvdDelete(type);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonNvdDelete");
        }
    }
    else
    {
#ifdef USE_DEFAULT_IMPLEMENTATION
        // Execute default implementation 
        CALLBACK_NOT_REGISTERED_DEF("LonNvdDelete");
        DefaultNvdDelete(type);
#else
        CALLBACK_NOT_REGISTERED("LonNvdDelete");
#endif
    }
}

FTXL_EXTERNAL_FN const LonApiError
LonNvdDeleteRegistrar(LonNvdDeleteFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.nvdDelete = handler;
    CALLBACK_REGISTER("LonNvdDelete", sts);
    return sts;
}

/* 
 *  Callback: LonNvdRead
 *  Read a section of a non-volatile data segment.
 *
 *  Parameters:
 *  handle - handle of the non-volatile segment returned by <LonNvdOpenForRead>
 *  offset - offset within the segment
 *  size - size of the data to be read
 *  pBuffer - pointer to buffer to store the data
 *
 *  Remarks:
 *  This function is called by the IzoT Device Stack API during initialization to 
 *  read data from persistent storage. An error value is returned if the 
 *  specified handle does not exist.    
 *
 *  Note that the offset will always be 0 on the first call after opening
 *  the segment. The offset in each subsequent call will be incremented by
 *  the size of the previous call.
 */
const LonApiError LonNvdRead(const LonNvdHandle handle, 
					         const size_t offset, 
					         const size_t size, 
					         void * const pBuffer) 
{
    if (theLonCallbackVectors.nvdRead)
    {
        try
        {
            CALLBACK_EXEC("LonNvdRead");
            return theLonCallbackVectors.nvdRead(handle, offset, size, pBuffer );
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonNvdRead");
            return LonApiCallbackExceptionError; 
        }
    }
    else
    {
#ifdef USE_DEFAULT_IMPLEMENTATION
        // Execute default implementation 
        CALLBACK_NOT_REGISTERED_DEF("LonNvdRead");
        return DefaultNvdRead(handle, offset, size, pBuffer);
#else
        CALLBACK_NOT_REGISTERED("LonNvdRead");
        return LonApiCallbackNotRegistered;
#endif  
    }
}

FTXL_EXTERNAL_FN const LonApiError
LonNvdReadRegistrar(LonNvdReadFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.nvdRead = handler;
    CALLBACK_REGISTER("LonNvdRead", sts);
    return sts;
}

/* 
 *  Callback: LonNvdWrite
 *  Write a section of a non-volatile data segment.
 *
 *  Parameters:
 *  handle - handle of the non-volatile segment returned by <LonNvdOpenForWrite>
 *  offset - offset within the segment
 *  size - size of the data to be read
 *  pData - pointer to the data to write into the segment
 *
 *  Remarks:
 *  This function is called by the IzoT Device Stack API after changes have been made 
 *  to data that is backed by persistent storage.  Note that many updates 
 *  might have been made, and this function is called only after the stack 
 *  determines that all updates have been completed.   
 *
 *  Note that the offset will always be 0 on the first call after opening
 *  the segment. The offset in each subsequent call will be incremented by
 *  the size of the previous call.
 */
const LonApiError LonNvdWrite (const LonNvdHandle handle, 
                               const size_t offset, 
                               const size_t size, 
                               const void* const pData) 
{
    if (theLonCallbackVectors.nvdWrite)
    {
        try
        {
            CALLBACK_EXEC("LonNvdWrite");
            return theLonCallbackVectors.nvdWrite(handle, offset, size, pData );
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonNvdWrite");
            return LonApiCallbackExceptionError; 
        }
    }
    else
    {
#ifdef USE_DEFAULT_IMPLEMENTATION
        // Execute default implementation 
        CALLBACK_NOT_REGISTERED_DEF("LonNvdWrite");
        return DefaultNvdWrite(handle, offset, size, pData );
#else
        CALLBACK_NOT_REGISTERED("LonNvdWrite");
        return LonApiCallbackNotRegistered;
#endif
    }
}

FTXL_EXTERNAL_FN const LonApiError
LonNvdWriteRegistrar(LonNvdWriteFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.nvdWrite = handler;
    CALLBACK_REGISTER("LonNvdWrite", sts);
    return sts;
}

/* 
 *  Callback: LonNvdIsInTransaction
 *  Returns TRUE if an NVD transaction was in progress last time the device 
 *  shut down.
 *
 *  Parameters:
 *  type - non-volatile segment type
 *
 *  Remarks:
 *  This function is called by the IzoT Device Stack API during initialization 
 *  prior to reading the segment data. This callback must return TRUE if an NVD 
 *  transaction had been started and never committed.  If this function returns 
 *  TRUE, the IzoT Device Stack API will discard the segment, otherwise, the IzoT 
 *  Device Stack API will attempt to read the persistent data. 
 */
const LonBool LonNvdIsInTransaction(const LonNvdSegmentType type)
{
    if (theLonCallbackVectors.nvdIsInTransaction)
    {
        try
        {
            CALLBACK_EXEC("LonNvdIsInTransaction");
            return theLonCallbackVectors.nvdIsInTransaction(type);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonNvdIsInTransaction");
            return false;
        }
    }
    else
    {
#ifdef USE_DEFAULT_IMPLEMENTATION
        // Execute default implementation 
        CALLBACK_NOT_REGISTERED_DEF("LonNvdIsInTransaction");
        return DefaultNvdIsInTransaction(type);
#else
        CALLBACK_NOT_REGISTERED("LonNvdIsInTransaction");
        return LonApiCallbackNotRegistered;
#endif  
    }
}

FTXL_EXTERNAL_FN const LonApiError
LonNvdIsInTransactionRegistrar(LonNvdIsInTransactionFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.nvdIsInTransaction = handler;
    CALLBACK_REGISTER("LonNvdIsInTransaction", sts);
    return sts;
}

/* 
 *  Callback: LonNvdEnterTransaction
 *  Initiate a non-volatile transaction.
 *
 *  Parameters:
 *  type - non-volatile segment type
 *
 *  Remarks:
 *  This function is called by the IzoT Device Stack API when the first request 
 *  arrives to update data stored in the specified segment.  The API updates 
 *  the non-persistent image, and schedules writes to update the non-volatile 
 *  storage at a later time.  
 */
const LonApiError LonNvdEnterTransaction(const LonNvdSegmentType type)
{
    if (theLonCallbackVectors.nvdEnterTransaction)
    {
        try
        {
            CALLBACK_EXEC("LonNvdEnterTransaction");
            return theLonCallbackVectors.nvdEnterTransaction(type);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonNvdEnterTransaction");
            return LonApiCallbackExceptionError; 
        }
    }
    else
    {
#ifdef USE_DEFAULT_IMPLEMENTATION
        // Execute default implementation 
        CALLBACK_NOT_REGISTERED_DEF("LonNvdEnterTransaction");
        return DefaultNvdEnterTransaction(type); 
#else
        CALLBACK_NOT_REGISTERED("LonNvdEnterTransaction");
        return LonApiCallbackNotRegistered;
#endif  
    }
}

FTXL_EXTERNAL_FN const LonApiError
LonNvdEnterTransactionRegistrar(LonNvdEnterTransactionFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.nvdEnterTransaction = handler;
    CALLBACK_REGISTER("LonNvdEnterTransaction", sts);
    return sts;
}

/* 
 *  Callback: LonNvdExitTransaction
 *  Complete a non-volatile transaction.
 *
 *  Parameters:
 *  type - non-volatile segment type
 *
 *  Remarks:
 *  This function is called by the IzoT Device Stack API after <LonNvdWrite> has 
 *  returned success and there are no further updates required.   
 */
const LonApiError LonNvdExitTransaction(const LonNvdSegmentType type)
{
    if (theLonCallbackVectors.nvdExitTransaction)
    {
        try
        {
            CALLBACK_EXEC("LonNvdExitTransaction");
            return theLonCallbackVectors.nvdExitTransaction(type);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonNvdExitTransaction");
            return LonApiCallbackExceptionError; 
        }
    }
    else
    {
#ifdef USE_DEFAULT_IMPLEMENTATION
        // Execute default implementation 
        CALLBACK_NOT_REGISTERED_DEF("LonNvdExitTransaction");
        return DefaultNvdExitTransaction(type); 
#else
        CALLBACK_NOT_REGISTERED("LonNvdExitTransaction");
        return LonApiCallbackNotRegistered;
#endif  
    }
}

FTXL_EXTERNAL_FN const LonApiError
LonNvdExitTransactionRegistrar(LonNvdExitTransactionFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.nvdExitTransaction = handler;
    CALLBACK_REGISTER("LonNvdExitTransaction", sts);
    return sts;
}

/*
 * Callback: LonNvdGetApplicationSegmentSize
 * Returns the size of all persistent application data.
 */
const unsigned LonNvdGetApplicationSegmentSize(void)
{
    if (theLonCallbackVectors.nvdGetApplicationSegmentSize)
    {
        try
        {
            CALLBACK_EXEC("LonNvdGetApplicationSegmentSize");
            return theLonCallbackVectors.nvdGetApplicationSegmentSize();
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonNvdGetApplicationSegmentSize");
            return 0;
        }
    }
    else
    {
#ifdef USE_DEFAULT_IMPLEMENTATION
        CALLBACK_NOT_REGISTERED_DEF("LonNvdGetApplicationSegmentSize");
        return GetDefaultApplicationSegmentSize();;
#else
        CALLBACK_NOT_REGISTERED("LonNvdGetApplicationSegmentSize");
        return LonApiCallbackNotRegistered;
#endif  
    }
}

FTXL_EXTERNAL_FN const LonApiError
LonNvdGetApplicationSegmentSizeRegistrar(LonNvdGetApplicationSegmentSizeFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.nvdGetApplicationSegmentSize = handler;
    CALLBACK_REGISTER("LonNvdGetApplicationSegmentSize", sts);
    return sts;
}

/*
 * Callback: LonNvdDeserializeSegment
 * Called by the API when application data needs loading from non-volatile storage
 */
const LonApiError LonNvdDeserializeSegment(const void* const pData, const size_t size)
{
    if (theLonCallbackVectors.nvdDeserializeSegment)
    {
        try
        {
            CALLBACK_EXEC("LonNvdDeserializeSegment");
            return theLonCallbackVectors.nvdDeserializeSegment(pData, size);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonNvdDeserializeSegment");
            return LonApiCallbackExceptionError; 
        }
    }
    else
    {
#ifdef USE_DEFAULT_IMPLEMENTATION
        CALLBACK_NOT_REGISTERED_DEF("LonNvdDeserializeSegment");
        return DefaultSerializeSegment(FALSE, (void* const)pData, size);
#else
        CALLBACK_NOT_REGISTERED("LonNvdDeserializeSegment");
        return LonApiCallbackNotRegistered;
#endif  
    }
}

FTXL_EXTERNAL_FN const LonApiError
LonNvdDeserializeSegmentRegistrar(LonNvdDeserializeSegmentFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.nvdDeserializeSegment = handler;
    CALLBACK_REGISTER("LonNvdDeserializeSegment", sts);
    return sts;
}

/*
 * Callback: LonNvdSerializeSegment
 * Called by the API when application data needs transferring to non-volatile storage
 */
const LonApiError LonNvdSerializeSegment(void* const pData, const size_t size)
{
    if (theLonCallbackVectors.nvdSerializeSegment)
    {
        try
        {
            CALLBACK_EXEC("LonNvdSerializeSegment");
            return theLonCallbackVectors.nvdSerializeSegment(pData, size);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonNvdSerializeSegment");
            return LonApiCallbackExceptionError; 
        }
    }
    else
    {
#ifdef USE_DEFAULT_IMPLEMENTATION
        CALLBACK_NOT_REGISTERED_DEF("LonNvdSerializeSegment");
        return DefaultSerializeSegment(TRUE, pData, size);
#else
        CALLBACK_NOT_REGISTERED("LonNvdSerializeSegment");
        return LonApiCallbackNotRegistered;
#endif  
    }
}

FTXL_EXTERNAL_FN const LonApiError
LonNvdSerializeSegmentRegistrar(LonNvdSerializeSegmentFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.nvdSerializeSegment = handler;
    CALLBACK_REGISTER("LonNvdSerializeSegment", sts);
    return sts;
}

/* 
 *  Event: LonNvdStarvation
 *  Signals that the NVD write task has taken more than 60 seconds. 
 *
 *  Parameters:
 *  seconds - Number of seconds since the persistent update was scheduled
 *
 *  Remarks:
 *  This event is fired if the non-volatile write task has been locked out for 
 *  more than 60 seconds.  When this event has been received, the application 
 *  may elect to suspend high priority tasks and call <LonNvdFlushData> in 
 *  order to wait for the non-volatile data to be completely written.
 */
void LonNvdStarvation(const unsigned seconds)
{
    if (theLonCallbackVectors.nvdStarvation)
    {
        try
        {
            CALLBACK_EXEC("LonNvdStarvation");
            theLonCallbackVectors.nvdStarvation(seconds);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonNvdStarvation");
        }
    }
    else
        CALLBACK_NOT_REGISTERED("LonNvdStarvation");
}

FTXL_EXTERNAL_FN const LonApiError
LonNvdStarvationRegistrar(LonNvdStarvationFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.nvdStarvation = handler;
    CALLBACK_REGISTER("LonNvdStarvation", sts);
    return sts;
}

/* 
 *  Event: LonFilterMsgArrived
 *  Occurs when an application message arrives.
 *
 *  Parameters:
 *  pAddress - source and destination address (see <LonReceiveAddress>)
 *  correlator - correlator to be used with <LonSendResponse>
 *  authenticated - TRUE if the message was (successfully) authenticated
 *  code - message code
 *  pData - pointer to message data bytes, might be NULL if dataLength is zero
 *  dataLength - length of bytes pointed to by pData
 *
 *  Remarks:
 *  This event handler filters the arrival of a message.  Typically this is used by 
 *  the ISI engine to filter the ISI message.  If the message does not get processed 
 *  by the filter handler, the message will be passed to the <LonMsgArrived> handler. 
 *
 */
FTXL_EXTERNAL_FN const LonBool
LonFilterMsgArrived(const LonReceiveAddress* const pAddress, 
                   const LonCorrelator correlator,
                   const LonBool priority, 
                   const LonServiceType serviceType, 
                   const LonBool authenticated, 
                   const LonByte code, 
                   const LonByte* const pData, const unsigned dataLength)
{
    if (theLonCallbackVectors.filterMsgArrived)
    {
        try
        {
            CALLBACK_EXEC("LonFilterMsgArrived");
            return theLonCallbackVectors.filterMsgArrived(pAddress, correlator, priority,
                            serviceType, authenticated, code,
                            pData, dataLength);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonFilterMsgArrived");
            return false;
        }
    }
    else
    {
        CALLBACK_NOT_REGISTERED("LonFilterMsgArrived");
        return false;
    }
}

FTXL_EXTERNAL_FN const LonApiError
LonFilterMsgArrivedRegistrar(LonFilterMsgArrivedFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.filterMsgArrived = handler;
    CALLBACK_REGISTER("LonFilterMsgArrivedRegistrar", sts);
    return sts;
}

/* 
 *  Event: LonFilterResponseArrived
 *  Occurs when a response arrives.
 *
 *  pAddress - source and destination address used for response (see 
 *             <LonResponseAddress>)
 *  tag - tag to match the response to the request
 *  code - response code
 *  pData - pointer to response data, may by NULL if dataLength is zero
 *          dataLength - number of bytes available through pData.
 *
 *  Remarks:
 *  This event handler filters the arrival of a response.  Typically this is used by 
 *  the ISI engine to filter the ISI message response.  If the response does not get processed 
 *  by the filter handler, the message will be passed to the <LonResponseArrived> handler.
 *
 */
FTXL_EXTERNAL_FN const LonBool
LonFilterResponseArrived(const LonResponseAddress* const pAddress, 
                        const unsigned tag, 
                        const LonByte code, 
                        const LonByte* const pData, const unsigned dataLength)
{
    if (theLonCallbackVectors.filterResponseArrived)
    {
        try
        {
            CALLBACK_EXEC("LonFilterResponseArrived");
            return theLonCallbackVectors.filterResponseArrived(pAddress, tag, code,
                            pData, dataLength);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonFilterResponseArrived");
            return false;
        }
    }
    else
    {
        CALLBACK_NOT_REGISTERED("LonFilterResponseArrived");
        return false;
    }
}

FTXL_EXTERNAL_FN const LonApiError
LonFilterResponseArrivedRegistrar(LonFilterResponseArrivedFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.filterResponseArrived = handler;
    CALLBACK_REGISTER("LonFilterResponseArrivedRegistrar", sts);
    return sts;
}

/* 
 *  Event: LonFilterMsgCompleted
 *  Occurs when a message transaction has completed.  See <LonSendMsg>.
 *
 *  Parameters:
 *  tag - used to correlate the event with the message sent  
 *        Same as the *tag* specified in the call to <LonSendMsg> 
 *  success - TRUE for successful completion, otherwise FALSE
 *
 *  Remarks:
 *  This event handler filters the completion of a message.  Typically this is used by 
 *  the ISI engine to filter the completion of the ISI message.  If the completion message 
 *  does not get processed by the filter handler, the message will be passed to the 
 *  <LonMsgCompleted> handler.
 */
const LonBool
LonFilterMsgCompleted(const unsigned tag, const LonBool success)
{
    if (theLonCallbackVectors.filterMsgCompleted)
    {
        try
        {
            CALLBACK_EXEC("LonFilterMsgCompleted");
            return theLonCallbackVectors.filterMsgCompleted(tag, success);
        }
        catch (...)
        {
            CALLBACK_EXCEPTION("LonFilterMsgCompleted");
            return false;
        }
    }
    else
    {
        CALLBACK_NOT_REGISTERED("LonFilterMsgCompleted");
        return false;
    }
}

FTXL_EXTERNAL_FN const LonApiError
LonFilterMsgCompletedRegistrar(LonFilterMsgCompletedFunction handler)
{
    LonApiError sts = LonApiNoError;
    theLonCallbackVectors.filterMsgCompleted = handler;
    CALLBACK_REGISTER("LonFilterMsgCompletedRegistrar", sts);
    return sts;
}

#endif
