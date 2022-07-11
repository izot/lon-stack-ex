/*
 * File: FtxlHandlers.c
 * $Revision: #4 $
 *
 * Abstract:
 * This file contains most of the callback handler functions and event handler 
 * functions that are to be implemented by the application.  
 *
 * Copyright: 
 * Copyright (c) 2008-2014 Echelon Corporation.  All rights reserved.
 *
 * Note:
 * This file is IzoT Device Stack API Software as defined in the Software
 * License Agreement that governs its use.
 *
 * Use of this code is subject to your compliance with the terms of the
 * Echelon IzoT(tm) Software Developer's Kit License Agreement which is
 * available at www.echelon.com/license/izot_sdk/.
 *
 */

#include "FtxlDev.h"
#include <string.h>
#include "malloc.h"

/* 
 * The actual implementations of the callback handlers that this example needs
 * are available in main.c. Here are the forward declarations for those functions. 
 */
extern void myEventReady(void);
extern void myReset(const LonResetNotification* const pResetNotification);
extern void myOnline(void);
extern void myNvUpdateOccurred(const unsigned nvIndex, const LonReceiveAddress* const pNvInAddr);
extern unsigned myGetCurrentNvSize(const unsigned index);
extern void myServiceLedStatus(LtServicePinState state);
extern void myMsgArrived(const LonReceiveAddress* const pAddress, 
                   const LonCorrelator correlator,
                   const LonBool priority, 
                   const LonServiceType serviceType, 
                   const LonBool authenticated, 
                   const LonByte code, 
                   const LonByte* const pData, const unsigned dataLength);


extern void GetMyIpAddress(int *pAddress, int *pPort);
const char *GetMyNetworkInterface(void);


#if LON_DMF_ENABLED
    /* This LonTalk Interface Developer-generated utility is used by the direct 
     * memory file (DMF) callback functions to translate from the virtual 
     * memory address within the IzoT Transceiver to the host memory address.  
     */
extern const LonApiError 
    LonTranslateWindowArea(LonBool write, unsigned start, unsigned size, 
                           char** ppHostAddress, LonMemoryDriver* driver);
#endif  /* LON_DMF_ENABLED */
 
#if PRODUCT_IS(IZOT)

void RegisterMyCallbackEvents(void)
{
    // register my event ready handler
    LonEventReadyRegistrar(myEventReady);

    // register my Get Current Nv Size 
    LonGetCurrentNvSizeRegistrar((LonGetCurrentNvSizeFunction) myGetCurrentNvSize);

    // register my reset handler 
    LonResetRegistrar(myReset);

    // register my online handler
    LonOnlineRegistrar(myOnline);

    // register my NV update occured handler
    LonNvUpdateOccurredRegistrar(myNvUpdateOccurred);

    // register my LonServiceLedStatus
    LonServiceLedStatusRegistrar(myServiceLedStatus);

    LonMsgArrivedRegistrar(myMsgArrived);
}

#else
/*
 * ******************************************************************************
 * SECTION: CALLBACK PROTOTYPES
 * ******************************************************************************
 *
 *  This section defines the prototypes for the IzoT Device Stack API callback 
 *  functions.
 *
 *  Remarks: 
 *  Callback functions are called by the IzoT protocol stack 
 *  immediately, as needed, and may be called from any IzoT task.  The 
 *  application *must not* call into the IzoT Device Stack API from within a callback.
 */

/*
 *  Callback: LonEventReady
 *  Signals that an event is ready to be processed.
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
    /* 
     * Signal application task that an event is ready 
     */
    myEventReady();
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
    return myGetCurrentNvSize(index);
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
 *  Note that resetting the IzoT device only affects the IzoT LonTalk protocol 
 *  stack and does not cause a processor or application software reset.
 */
void LonReset(const LonResetNotification* const pResetNotification)
{
    myReset(0);
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
    /*
     * TBD
     */
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
    /*
     * TBD
     */
}

/*
 *  Event: LonOnline
 *  Occurs when the IzoT device has entered the online state.
 */
void LonOnline(void)
{
    myOnline();
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
    /*
     * TBD
     */
}

/*
 *  Event: LonServicePinHeld
 *  Occurs when the service pin has been continuously activated for a 
 *  configurable time.
 *
 *  Remarks:
 *  Use the LonTalk Interface Developer to enable this feature and to specify 
 *  the duration for which the service pin must be activated to trigger this 
 *  event handler. 
 */
void LonServicePinHeld(void)
{
    /*
     * TBD
     */
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
    myNvUpdateOccurred(index, pSourceAddress);
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
 *  acknowledged messages, it is successfully complete when the IzoT LonTalk 
 *  protocol stack receives an acknowledgement from each of the destination 
 *  devices, and is unsuccessfully complete if not all acknowledgements are 
 *  received.  Poll requests always use the request service type, and 
 *  generates a successful completion if responses are received from all 
 *  expected devices, and generates a failed completion otherwise.
 */
void LonNvUpdateCompleted(const unsigned index, const LonBool success)
{
    /*
     * TBD
     */
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
    /*
     * TBD
     */
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
    /*
     * TBD
     */
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
    /*
     * TBD
     */
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
    /*
     * TBD
     */

    /*
    if (serviceType == LonServiceRequest)
    {
        typedef struct
         {
            LonByte majorVersion;
            LonByte minorVersion;
            LonByte build;
            LonDoubleWord systemMemory;
            LonDoubleWord inUse;
         } StatsResp;
         unsigned majorVersion;
         unsigned minorVersion;
         unsigned buildNumber;
         
         StatsResp resp;
         memset(&resp, 0, sizeof(resp));
         struct mallinfo memStats = mallinfo();
         LON_SET_UNSIGNED_DOUBLEWORD(resp.systemMemory,memStats.arena);
         LON_SET_UNSIGNED_DOUBLEWORD(resp.inUse, memStats.uordblks);
         if (LonGetVersion(&majorVersion, &minorVersion, &buildNumber) == LonApiNoError)
         {
            resp.majorVersion = (LonByte) majorVersion;
            resp.minorVersion = (LonByte) minorVersion;
            resp.build = (LonByte) buildNumber;
         }
         LonSendResponse(correlator, 0xFF, (LonByte *)&resp, sizeof(resp));       
    }
    */
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
    /*
     * TBD
     */
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
    /*
     * TBD
     */
}

/*
 * ******************************************************************************
 * SECTION: DMF CALLBACK PROTOTYPES
 * ******************************************************************************
 *
 *  This section defines the prototypes for the IzoT Device Stack API callback 
 *  functions supporting direct memory files (DMF) read and write.  This file
 *  contains complete default implementations of these callback functions. These 
 *  callback functions use a helper function, LonTranslateWindowArea(), that is 
 *  generated by the LonTalk Interface Developer to translate from the virtual 
 *  memory address within the IzoT Transceiver to the host memory address.  
 *  These functions typically do not need to be modified.
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
#if     LON_DMF_ENABLED
    char* pHostAddress = NULL;
    LonMemoryDriver driver = LonMemoryDriverUnknown;
    LonApiError result = LonTranslateWindowArea(FALSE, address, size, &pHostAddress, &driver);
 
    if (result == LonApiNoError) 
    {
        if(driver == LonMemoryDriverStandard) 
        {
            (void)memcpy(pData, pHostAddress, size);
        } 
        else 
        {
            /*
             * TODO: add code to support alternative data storage, 
             * such as using paged memory, or serial interface memory 
             *  devices such as IIC EEPROM devices. When done with 
             * success, return LonApiNoError.
             */
 
            result = LonApiDmfNoDriver;
        }
    }
    return result;
#else
    return LonApiNotAllowed;
#endif   /* LON_DMF_ENABLED */
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
 *  Transceiver�s 64 KB address space.  The IzoT protocol stack 
 *  automatically calls the <LonNvdAppSegmentHasBeenUpdated> function to 
 *  schedule an update whenever this callback returns *LonApiNoError*.
 *
 */
const LonApiError LonMemoryWrite(const unsigned address, 
						         const unsigned size,
						         const void* const pData)
{
#if LON_DMF_ENABLED
    char* pHostAddress = NULL;
    LonMemoryDriver driver = LonMemoryDriverUnknown;
    LonApiError result = LonTranslateWindowArea(TRUE, address, size, &pHostAddress, &driver);
 
    if (result == LonApiNoError) 
    {
        if(driver == LonMemoryDriverStandard) 
        {
            (void)memcpy(pHostAddress, pData, size);
        }
        else
        {
            /*
             * TODO: add code to support alternative data storage, 
             * such as using paged memory, or serial interface memory 
             * devices such as IIC EEPROM devices.
             * When done with success, return LonApiNoError.
             */
 
            result = LonApiDmfNoDriver;
        }
    }
    return result;
#else
    return LonApiNotAllowed;
#endif  /* LON_DMF_ENABLED */
}

/*
 * ******************************************************************************
 * SECTION: NVD EVENT HANDLER PROTOTYPES
 * ******************************************************************************
 *
 *  This section defines the prototypes for the IzoT Device Stack API event handler 
 *  functions supporting memory for Non-Volatile Data (NVD).
 *
 *  Remarks:
 *  Like callback functions, event handlers are called from the IzoT Device Stack API.  
 *  However, unlike callback functions, event handlers are only called in the 
 *  context of the application task, and only when the application calls the 
 *  <LonEventPump> function. Also, the application can make IzoT Device Stack API 
 *  function calls from within an event handler.
 * 
 *  Complete implementations for these event handlers are provided in 
 *  FtxlNvdFlashDirect.c, and FtxlNvdFlashFs.c. A skeleton implementation 
 *  these event handlers is provided in FtxlNvdUserDefined.c.
 */

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
    /*
     * TBD
     */
}


#if FEATURE_INCLUDED(IP852)
/*
 *  Function: LonGetMyIpAddress
 *  Get the IP address and port number of an IP852 interface.  
 *
 *  Returns:
 *  void.
 * 
 *  Remarks:
 *  Returns the IP address and port used for an IP852 interface.  
 */
FTXL_EXTERNAL_FN void LonGetMyIpAddress(int *pAddress, int *pPort)
{
    GetMyIpAddress(pAddress, pPort);
}

#else
/*
 *  Function: LonGetMyNetworkInterface
 *  Get the network interface name to use to open the native LonTalk interface.  
 *
 *  Returns:
 *  Name of network interface.
 * 
 *  Remarks:
 *  Returns the name of the network interface, such as the serial port name.  
 */
FTXL_EXTERNAL_FN const char *LonGetMyNetworkInterface(void)
{
    return GetMyNetworkInterface();
}
#endif
#endif

