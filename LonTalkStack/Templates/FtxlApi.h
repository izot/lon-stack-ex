/*
 * File: FtxlApi.h
 * Title: IzoT Device Stack 1.0 API Reference
 *
 * $Revision: #10 $
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
 * Abstract:
 * This file declares prototypes for IzoT SDK Device Stack API functions. 
 * This file is part of the IzoT SDK 1.0.
 *
 */

#ifndef _FTXL_API_H
#define _FTXL_API_H

#include <stdlib.h>
#ifdef  __cplusplus
extern "C" {
#endif

#define FTXL_EXTERNAL_FN extern

#include "LonPlatform.h"
#include "FtxlTypes.h"

/*
 * ******************************************************************************
 * SECTION: IzoT SDK Device Stack API
 * ******************************************************************************
 *
 * This section details the IzoT SDK Device Stack API functions.
 */

/*
 *  Function: LonRegisterUniqueId
 *  Registers the unique ID (Neuron ID).
 *
 *  Parameters:
 *  pId   - pointer to the the unique ID
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  The IzoT application must register a value unique ID prior to calling LonLidCreateStack.
 *  The stack will create a random unique ID if there is no unique ID registered prior to
 *  calling LonLidCreateStack. 
 *
 *  The *Unique ID* is also known as the *Neuron ID*, however, *Neuron ID* is 
 *  a deprecated term. 
 */
FTXL_EXTERNAL_FN const LonApiError LonRegisterUniqueId(const LonUniqueId* const pId);


/*
 * Function: LonEventPump
 * Process IzoT events. 
 *
 * Remarks:
 * This function must be called periodically by the application.  This
 * function processes any events that have been posted by the Izot   
 * protocol stack. Typically this function is called in response to the 
 * <LonEventReady> callback, but must *not* be called directly from a 
 * callback. Instead, the <LonEventReady> callback typically sets an operating 
 * system event to schedule the main application task to call this function.
 *
 * This function must be called at least once every 10 ms.  Use the following 
 * formula to determine the minimum call rate:
 *   rate = MaxPacketRate / (InputBufferCount - 1) 
 * where MaxPacketRate is the maximum number of packets per second arriving 
 * for the device and InputBufferCount is the number of input buffers defined 
 * for the application
 */
FTXL_EXTERNAL_FN void LonEventPump(void);

/*
 *  Function: LonGetUniqueId
 *  Gets the register unique ID (Neuron ID).
 *
 *  Parameters:
 *  pId   - pointer to the the unique ID
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  The IzoT application must register a value unique ID prior to calling LonLidCreateStack.
 *  The stack will create a random unique ID if there is no unique ID registered prior to 
 *  calling LonLidCreateStack. 
 *
 *  The *Unique ID* is also known as the *Neuron ID*, however, *Neuron ID* is 
 *  a deprecated term. 
 */
FTXL_EXTERNAL_FN const LonApiError LonGetUniqueId(LonUniqueId* const pId);

/*
 *  Function: LonGetVersion
 *  Returns the Izot Device Stack API version number.
 *
 *  Parameters:
 *  pMajorVersion - pointer to receive the Izot Device Stack API's major version 
 *      number.
 *  pMinorVersion - pointer to receive the Izot Device Stack API's minor version 
 *      number.
 *  pBuildNumber - pointer to receive the Izot Device Stack API's build number.
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  This function provides the version of the Izot Device Stack API.  Note that 
 *  this function can be called at any time, including prior to calling <LonLidCreateStack>. 
 */
FTXL_EXTERNAL_FN const LonApiError LonGetVersion(unsigned* const pMajorVersion, 
                                                 unsigned* const pMinorVersion,
                                                 unsigned* const pBuildNumber);

/*
 *  Function: LonGetDeclaredNvSize
 *  Gets the declared size of a network variable.
 *
 *  Parameters:
 *  index - the index of the network variable
 *
 *  Returns:     
 *  Declared initial size of the network variable as defined in the Neuron C 
 *  model file.  Zero if the network variable corresponding to index doesn't 
 *  exist.
 *
 *  Note that this function *may* be called from the LonGetCurrentNvSize() 
 *  callback.
 */
FTXL_EXTERNAL_FN const unsigned LonGetDeclaredNvSize(const unsigned index);

/*
 *  Function: LonGetNvValue
 *  Returns a pointer to the network variable value.
 *
 *  Parameters:
 *  index - the index of the network variable
 *
 *  Returns:
 *  Pointer to the network variable value, or NULL if invalid.
 *
 *  Remarks:
 *  You can use this function to obtain a pointer to either a static or
 *  dynamic network variable value.
 */
FTXL_EXTERNAL_FN volatile void* const LonGetNvValue(const unsigned index);

/*
 *  Function: LonSetNvValue
 *  Set a new the network variable value in packed, big-endian.
 *
 *  Parameters:
 *  index - the local index of the network variable
 *  pValue - Pointer to the network variable value to set
 *  nLength - the length of the network variable
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Use this function to set a new the network variable value in packed, big-endian.
 *  This makes it easier for the Python application to pass data to stack which expects
 *  big endian packed byte arrays.
 */
FTXL_EXTERNAL_FN const LonApiError LonSetNvValue(const unsigned index, void* const pValue);

/*
 *  Function: LonQueryNvType
 *  Queries type information about a network variable.
 *
 *  Parameters:
 *  index - the index of the network variable
 *  pNvDef - pointer to a <LonNvDefinition> 
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  Use this function to obtain information about a network variable.  The
 *  definition is returned in the <LonNvDefinition> buffer provided.  The
 *  <LonNvDefinition> buffer contains pointers to the network variable name
 *  and self-documentation strings, which are allocated by the IzoT Device Stack API.  
 *  The application must call LonFreeNvTypeData to free these strings
 *  when it is done with them.
 */
FTXL_EXTERNAL_FN const LonApiError LonQueryNvType(const unsigned index, 
                                                  LonNvDefinition* const pNvDef);

/*
 *  Function: LonFreeNvTypeData
 *  Frees internal buffers allocated by LonQueryNvType.
 *
 *  Parameters:
 *  pNvDef - pointer to a <LonNvDefinition> that was previously initialized by 
 *  LonQueryNvType.
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  Use this function to free the internally allocated string buffers 
 *  contained in the <LonNvDefinition> structure.  Note that this function does
 *  not free the <LonNvDefinition> structure itself, only the strings that it 
 *  references.
 */
FTXL_EXTERNAL_FN const LonApiError LonFreeNvTypeData(LonNvDefinition* const pNvDef);

/*
 * Function: LonPollNv
 * Polls a bound, polling, input network variable.
 *
 * Parameters:
 * index - index of the input network variable 
 *
 * Returns:
 * <LonApiError>.
 * 
 * Remarks:
 * Call this function to poll an input network variable. Polling an input network 
 * variable causes the device to solicit the current value of all output network 
 * variables that are bound to this one.
 *
 * The function returns *LonApiNoError* if the API has successfully queued the 
 * request. Note that the successful completion of this function does not
 * indicate the successful arrival of the requested values. The values received 
 * in response to this poll are reported by one or more of calls to the 
 * <LonNvUpdateOccurred> event handler. 
 *
 * LonPollNv operates on input network variables that have been declared with 
 * the Neuron C *polled* attribute, only. Only output network variables that 
 * are bound to the input network variable will be received.
 *
 * Note that it is *not* an error to poll an unbound polling input network 
 * variable.  If this is done, the application will not receive any 
 * <LonNvUpdateOccurred> events, but will receive a <LonNvUpdateCompleted>
 * event with the success parameter set to TRUE.
 */
FTXL_EXTERNAL_FN const LonApiError LonPollNv(const unsigned index);

/*
 *  Function: LonPropagateNv
 *  Propagates the value of a bound output network variable to the network.
 *
 *  Parameters:
 *  index - the index of the network variable
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  Note that it is not an error to propagate an unbound non-polled output.  
 *  If this is done, the IzoT protocol stack will not send any updates 
 *  to the network, but will generate a <LonNvUpdateCompleted> event with the 
 *  success parameter set to TRUE.
 *
 *  If LonPropagateNv() returns *LonApiNoError*, the <LonNvUpdateCompleted> 
 *  event will be triggered when the NV update has successfully completed or 
 *  failed.  
 *
 *  If LonPropagateNv() is called multiple times before the network variable is 
 *  sent, the behavior is dependent on whether the network variable has the 
 *  synchronous attribute:
 *
 *    - If the network variable is declared with the Neuron C *sync* attribute, 
 *      the network variable will be sent on the network each time 
 *      <LonPropagateNv> is called (subject to application buffer limits).  
 *      The value sent will be the value of the network variable at the time 
 *      of the call to <LonPropagateNv>.  <LonNvUpdateCompleted> will be 
 *      called each time as well.  
 *    
 *    - If the network variable is *not* declared with the Neuron C *sync* 
 *      attribute, only the latest value of the network variable will be sent 
 *      out onto the network, and the <LonNvUpdateCompleted> function will be 
 *      called only once.  If there are no application buffers available, the 
 *      network variable will be propagated at a later time, when one becomes 
 *      available.
 */
FTXL_EXTERNAL_FN const LonApiError LonPropagateNv(const unsigned index);


/*
 *  Function: LonSendServicePin
 *  Propagates a service pin message.
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  Use this function to propagate a service pin message to the network. 
 *  The function will fail if the device is not yet fully initialized.
 */
FTXL_EXTERNAL_FN const LonApiError LonSendServicePin(void);

/*
 *  Function: LonSendMsg
 *  Send an explicit (non-NV) message.
 *
 *  Parameters:
 *  tag - message tag for this message
 *  priority - priority attribute of the message
 *  serviceType - service type for use with this message
 *  authenticated - TRUE to use authenticated service
 *  pDestAddr - pointer to destination address
 *  code - message code
 *  pData - message data, may be NULL if length is zero
 *  length - number of valid bytes available through pData
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  This function is called to send an explicit message.  For application 
 *  messages, the message code should be in the range of 0x00..0x2f.  Codes in 
 *  the 0x30..0x3f range are reserved for protocols such as file transfer.
 *
 *  If the tag field specifies one of the bindable messages tags 
 *  (tag < # bindable message tags), the pDestAddr is ignored (and can be 
 *  NULL) because the message is sent using implicit addressing.
 *
 *  A successful return from this function indicates only that the message has 
 *  been queued to be sent.  If this function returns success, the IzoT Device 
 *  Stack API will call <LonMsgCompleted> with an indication of the 
 *  transmission success.
 *
 *  If the message is a request, <LonResponseArrived> event handlers are 
 *  called when corresponding responses arrive.
 */
FTXL_EXTERNAL_FN const LonApiError LonSendMsg(const unsigned tag, const LonBool priority, 
                                              const LonServiceType serviceType, 
                                              const LonBool authenticated,
                                              const LonSendAddress* const pDestAddr, 
                                              const LonByte code, 
                                              const LonByte* const pData, const unsigned length);
/*
 *  Function: LonSendResponse
 *  Sends a response.
 *
 *  Parameters:
 *  correlator - message correlator, received from <LonMsgArrived>
 *  code - response message code
 *  pData - pointer to response data, may be NULL if length is zero
 *  length - number of valid response data bytes in pData
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  This function is called to send an explicit message response.  The correlator
 *  is passed in to <LonMsgArrived> and must be copied and saved if the response 
 *  is to be sent after returning from that routine.  A response code should be 
 *  in the 0x00..0x2f range.
 */
FTXL_EXTERNAL_FN const LonApiError LonSendResponse(const LonCorrelator correlator, 
                                                   const LonByte code, const LonByte* const pData, const unsigned length);

/*
 *  Function: LonReleaseCorrelator
 *  Release a request correlator without sending a response.
 *
 *  Parameters:
 *  correlator - The correlator, obtained from <LonMsgArrived>
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  This function is called to release a correlator obtained from 
 *  <LonMsgArrived> without sending a response.  The application must either 
 *  send a response to every message with a service type of request, or release
 *  the correlator, but not both.  
 */
FTXL_EXTERNAL_FN const LonApiError LonReleaseCorrelator(const LonCorrelator correlator);

/*
 * ******************************************************************************
 * SECTION: IzoT SDK Device Stack Extended API 
 * ******************************************************************************
 *
 * This section details IzoT SDK Device Stack extended API functions consisting of 
 * query functions and update functions. These functions are not typically required,
 * and are intended for use by advanced application developers only.
 */

/*
 *  Function: LonQueryStatus
 *  Request local status and statistics.
 *
 *  Parameters:
 *  pStatus - pointer to a <LonStatus> structure
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  Call this function to obtain the local status and statistics of the IzoT 
 *  device. The status will be stored in the <LonStatus> structure provided. 
 */
FTXL_EXTERNAL_FN const LonApiError LonQueryStatus(LonStatus* const pStatus);

/* 
 *  Function: LonClearStatus
 *  Clears the status statistics on the IzoT device.
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  This function can be used to clear the IzoT device status and statistics 
 *  records.
 */
FTXL_EXTERNAL_FN const LonApiError LonClearStatus(void);

/*
 *  Function: LonQueryTransceiverStatus
 *  Request local transceiver status information.
 *
 *  Parameters:
 *  pTransceiverParameters - pointer to a <LonTransceiverParameters> structure
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  Call this function to query the status of the local IzoT Transceiver. 
 *  The parameters are stored in the <LonTransceiverParameters> structure
 *  provided. 
 *
 *  Note that this function is provided only for compatibility with the ShortStack 
 *  LonTalk Compact API.  The transceiver status information is useful only for 
 *  transceivers supporting special purpose mode.  Because IzoT Device does not use a 
 *  special purpose mode transceiver, this function always returns 
 *  LonApiInvalidParameter.
 */
FTXL_EXTERNAL_FN const LonApiError LonQueryTransceiverStatus(LonTransceiverParameters* const pTransceiverParameters);

/*
 *  Function: LonQueryConfigData
 *  Request a copy of local configuration data.
 *
 *  Parameters:
 *  pConfig - pointer to a <LonConfigData> structure
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  Call this function to request a copy of device's configuration data.
 *  The configuration is stored in the <LonConfigData> structure
 *  provided. 
 */
FTXL_EXTERNAL_FN const LonApiError LonQueryConfigData(LonConfigData* const pConfig);

/* 
 *  Function: LonUpdateConfigData
 *  Updates the configuration data on the IzoT device.
 *
 *  Parameters:
 *  pConfig - pointer to <LonConfigData> configuration data
 *  
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  Call this function to update the device's configuration data based on the 
 *  configuration stored in the <LonConfigData> structure.
 */
FTXL_EXTERNAL_FN const LonApiError LonUpdateConfigData(const LonConfigData* const pConfig);

/*
 *  Function: LonSetNodeMode
 *  Sets the device's mode and/or state.
 *
 *  Parameters:
 *  mode - mode of the IzoT device, see <LonNodeMode>
 *  state - state of the IzoT device, see <LonNodeState>
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  Use this function to set the IzoT device's mode and state. 
 *  If the mode parameter is *LonChangeState*, then the state parameter may be 
 *  set to one of *LonApplicationUnconfig*, *LonNoApplicationUnconfig*, 
 *  *LonConfigOffLine* or *LonConfigOnLine*.  Otherwise the state parameter 
 *  should be *LonStateInvalid* (0).  Note that while the <LonNodeState> 
 *  enumeration is used to report both the state and the mode (see <LonStatus>), 
 *  it is *not* possible to change both the state and mode (online/offline) at 
 *  the same time.
 *
 *  You can also use the shorthand functions <LonGoOnline>, <LonGoOffline>, 
 *  <LonGoConfigured>, and <LonGoUnconfigured>.
 */
FTXL_EXTERNAL_FN const LonApiError LonSetNodeMode(const LonNodeMode mode, const LonNodeState state);

/*
*  Function: LonGoOnline
*  Sets the IzoT device online.
*
*  Returns:
*  <LonApiError>.
*  
*  Remarks:
*  Call this function to put the IzoT device into online mode.
*/
#define LonGoOnline()         LonSetNodeMode(LonApplicationOnLine, LonStateInvalid)

/*
*  Function: LonGoOffline
*  Sets the IzoT device offline.
*
*  Returns:
*  <LonApiError>.
*  
*  Remarks:
*  Call this function to put the IzoT device into offline mode.
*/
#define LonGoOffline()        LonSetNodeMode(LonApplicationOffLine, LonStateInvalid)

/*
*  Function: LonGoConfigured
*  Sets the IzoT device state to configured.
*
*  Returns:
*  <LonApiError>.  
*
*  Remarks:
*  Call this function to set the IzoT device state to *LonConfigOnLine*.
*/
#define LonGoConfigured()     LonSetNodeMode(LonChangeState, LonConfigOnLine)

/*
*  Function: LonGoUnconfigured
*  Sets the IzoT device state to unconfigured.
*
*  Returns:
*  <LonApiError>.
*
*  Remarks:
*  Call this function to set the IzoT device state to *LonApplicationUnconfig*.
*/
#define LonGoUnconfigured()   LonSetNodeMode(LonChangeState, LonApplicationUnconfig)

/*
 *  Function: LonQueryDomainConfig
 *  Request a copy of a local domain table record.
 *
 *  Parameters:
 *  index - index of requested domain table record (0, 1)
 *  pDomain - pointer to a <LonDomain> structure
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  Call this function to request a copy of a local domain table record. 
 *  The information is returned through the <LonDomain> structure provided.
 */
FTXL_EXTERNAL_FN const LonApiError LonQueryDomainConfig(const unsigned index, LonDomain* const pDomain);

/* 
 *  Function:   LonUpdateDomainConfig
 *  Updates a domain table record on the IzoT device.
 *
 *  Parameters:
 *  index - the index of the domain table to update
 *  pDomain - pointer to the domain table record
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  This function can be used to update one record of the domain table.
 */
FTXL_EXTERNAL_FN const LonApiError LonUpdateDomainConfig(const unsigned index, const LonDomain* const pDomain);

/*
 *  Function: LonQueryAddressConfig
 *  Request a copy of address table configuration data.
 *
 *  Parameters:
 *  index - index of requested address table entry
 *  pAddress - pointer to a <LonAddress> structure
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  Call this function to request a copy of the address table configuration data.
 *  The configuration is stored in the <LonAddress> structure
 *  provided. 
 */
FTXL_EXTERNAL_FN const LonApiError LonQueryAddressConfig(const unsigned index, LonAddress* const pAddress);

/* 
 *  Function: LonUpdateAddressConfig
 *  Updates an address table record on the IzoT device.
 *
 *  Parameters:
 *  index - index of the address table to update
 *  pAddress - pointer to address table record 
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  Use this function to write a record to the local address table.
 */
FTXL_EXTERNAL_FN const LonApiError LonUpdateAddressConfig(const unsigned index, const LonAddress* const pAddress);

/*
 *  Function: LonQueryNvConfig
 *  Request a copy of network variable configuration data.
 *
 *  Parameters:
 *  index - index of requested NV configuration table entry
 *  pNvConfig - pointer to a <LonNvEcsConfig> structure
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  Call this function to request a copy of the local network variable 
 *  configuration data. The configuration is stored in the <LonNvEcsConfig> 
 *  structure provided.
 */
FTXL_EXTERNAL_FN const LonApiError LonQueryNvConfig(const unsigned index, LonNvEcsConfig* const pNvConfig);

/*
 *  Function: LonUpdateNvConfig
 *  Updates a network variable configuration table record on the IzoT device.
 *
 *  Parameter:
 *  index - index of network variable
 *  pNvConfig - network variable configuration
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  This function can be used to update one record of the network variable
 *  configuration table.
 */
FTXL_EXTERNAL_FN const LonApiError LonUpdateNvConfig(const unsigned index, const LonNvEcsConfig* const pNvConfig);

/*
 *  Function: LonQueryAliasConfig
 *  Request a copy of alias configuration data.
 *
 *  Parameters:
 *  index - index of requested alias configuration table entry
 *  pAlias - pointer to a <LonAliasEcsConfig> structure
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  Call this function to request a copy of the alias configuration data.
 *  The configuration is stored in the <LonAliasEcsConfig> structure
 *  provided. 
 */
FTXL_EXTERNAL_FN const LonApiError LonQueryAliasConfig(const unsigned index, LonAliasEcsConfig* const pAlias);


/* 
 *  Function: LonUpdateAliasConfig
 *  Updates an alias table record on the IzoT device.
 *
 *  Parameters:
 *  index - index of alias table record to update
 *  pAlias - pointer to the alias table record
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  This function writes a record in the local alias table.
 */
FTXL_EXTERNAL_FN const LonApiError LonUpdateAliasConfig(const unsigned index, const LonAliasEcsConfig* const pAlias);

/*
 *  Function: LonNvIsBound
 *  Determines whether a network variable is bound.
 *
 *  Parameters:
 *  index - index of the network variable
 *  pIsBound - pointer to receive the "is-bound" attribute
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  Call this function to determine whether a network variable is bound.
 *  A network variable is bound if it, or any of its aliases, has a bound 
 *  selector or an address table entry. The unbound selector for a given 
 *  network variable is equal to (0x3fff  NV index).  A network variable
 *  or alias has an address if the address index is not equal to 0xffff.
 */
FTXL_EXTERNAL_FN const LonApiError LonNvIsBound(const unsigned index, LonBool* const pIsBound);

/*
 *  Function: LonMtIsBound
 *  Determines whether a message tag is bound.
 *
 *  Parameters:
 *  tag - the message tag
 *  pIsBound - pointer to receive the "is-bound" attribute
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  Call this function to determine whether a message tag is bound.
 *  A message tag is bound if the associated address type is anything other
 *  than *LonAddressUnassigned*.
 */
FTXL_EXTERNAL_FN const LonApiError LonMtIsBound(const unsigned tag, LonBool* const pIsBound);

/*
 * ******************************************************************************
 * SECTION: CALLBACK PROTOTYPES
 * ******************************************************************************
 *
 *  This section defines the prototypes for the IzoT LonTalk API callback 
 *  functions.
 *
 *  Remarks: 
 *  Callback functions are called by the IzoT protocol stack 
 *  immediately, as needed, and may be called from any IzoT task.  The 
 *  application *must not* call into the IzoT Device Stack API from within a callback.
 * 
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
 * time, from any thread.  The application cannot call any IzoT Device Stack functions
 * from this callback. 
 */
FTXL_EXTERNAL_FN void LonEventReady(void);

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
FTXL_EXTERNAL_FN const unsigned LonGetCurrentNvSize(const unsigned index);

/*
 * ******************************************************************************
 * SECTION: EVENT HANDLER PROTOTYPES
 * ******************************************************************************
 *
 *  This section defines the prototypes for the IzoT Device Stack API event handler 
 *  functions.
 *
 *  Remarks:
 *  Like callback functions, event handlers are called from the IzoT Device Stack API.  
 *  However, unlike callback functions, event handlers are only called in the 
 *  context of the application task, and only when the application calls the 
 *  <LonEventPump> function. Also, the application may make IzoT Device Stack API 
 *  function calls from within an event handler.
 * 
 */

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
FTXL_EXTERNAL_FN void LonReset(const LonResetNotification* const pResetNotification);

/*
 *  Event: LonWink
 *  Occurs when the IzoT device receives a WINK command.
 *
 *  Remarks:
 *  This event is not triggered when wink sub-commands (extended install 
 *  commands) are received.
 */
FTXL_EXTERNAL_FN void LonWink(void);

/*
 *  Event: LonOffline
 *  Occurs when the IzoT device has entered the offline state.
 *
 *  Remarks:
 *  While the device is offline, the IzoT protocol stack will not 
 *  generate network variable updates, and will return an error when 
 *  <LonPropagateNv> is called.
 *
 *  Note that offline processing in IzoT Device Stack differs from that in ShortStack.  
 *  When a ShortStack Micro Server receives a request to go offline, it sends 
 *  the request to the ShortStack LonTalk Compact API, which calls the 
 *  application callback <LonOffline>.  The Micro Server does not actually go 
 *  offline until the <LonOffline> callback function returns and the 
 *  ShortStack LonTalk Compact API sends a confirmation message to the Micro 
 *  Server.  In contrast, the IzoT device goes offline as soon as it receives 
 *  the offline request. The <LonOffline> event is handled asynchronously.
 */
FTXL_EXTERNAL_FN void LonOffline(void);

/*
 *  Event: LonOnline
 *  Occurs when the IzoT device has entered the online state.
 */
FTXL_EXTERNAL_FN void LonOnline(void);

/*
 *  Event: LonServicePinPressed
 *  Occurs when the service pin has been activated. 
 *
 *  Remarks:
 *  Note that the IzoT protocol stack sends a service pin message 
 *  automatically any time the service pin has been pressed.
 */
FTXL_EXTERNAL_FN void LonServicePinPressed(void);

/*
 *  Event: LonServicePinHeld
 *  Occurs when the service pin has been continuously activated for a 
 *  configurable time.
 *
 *  Remarks:
 *  Not applicable for an IzoT Device. 
 */
FTXL_EXTERNAL_FN void LonServicePinHeld(void);

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
FTXL_EXTERNAL_FN void LonNvUpdateOccurred(const unsigned index, const LonReceiveAddress* const pSourceAddress);

/*
 *  Event:   LonNvUpdateCompleted
 *  Signals completion of a network variable update.
 *
 *  Parameters:
 *  index - global index (local to the device) of the network variable that was updated
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
FTXL_EXTERNAL_FN void LonNvUpdateCompleted(const unsigned index, const LonBool success);

/*
 *  Event:   LonNvAdded
 *  A dynamic network variable has been added.
 *
 *  Parameters:
 *  index - The index of the dynamic network variable that has been added 
 *  pNvDef - Pointer to the network variable definition <LonNvDefinition> 
 *
 *  Remarks:
 *  The IzoT Device protocol stack calls this function when a dynamic network 
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
 *  IzoT Device protocol stack will call the <LonNvTypeChanged> event handler.
 */
FTXL_EXTERNAL_FN void LonNvAdded(const unsigned index, const LonNvDefinition* const pNvDef);

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
FTXL_EXTERNAL_FN void LonNvTypeChanged(const unsigned index, const LonNvDefinition* const pNvDef);

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
FTXL_EXTERNAL_FN void LonNvDeleted(const unsigned index);

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
FTXL_EXTERNAL_FN void LonMsgArrived(const LonReceiveAddress* const pAddress, 
                          const LonCorrelator correlator,
                          const LonBool priority, 
                          const LonServiceType serviceType, 
                          const LonBool authenticated, 
                          const LonByte code, 
                          const LonByte* const pData, const unsigned dataLength);

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
FTXL_EXTERNAL_FN void LonResponseArrived(const LonResponseAddress* const pAddress, 
                               const unsigned tag, 
                               const LonByte code, 
                               const LonByte* const pData, const unsigned dataLength);

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
FTXL_EXTERNAL_FN void LonMsgCompleted(const unsigned tag, const LonBool success);

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
FTXL_EXTERNAL_FN void LonServiceLedStatus(LtServicePinState state);

/*
 * ******************************************************************************
 * SECTION: DMF CALLBACK PROTOTYPES
 * ******************************************************************************
 *
 *  This section defines the prototypes for the IzoT Device Stack API callback 
 *  functions supporting direct memory files (DMF) read and write.  Complete 
 *  default implementations of these callback functions can be found in 
 *  "FtxlHandlers.c".  These callback functions use a helper function, 
 *  LonTranslateWindowArea(), created by the IzoT application 
 *  to translate from the virtual memory address within the IzoT 
 *  Transceiver to the host memory address.  These functions typically do
 *  not need to be modified.
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
						        void* const pData);

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
 *  Transceiver's 64 KB address space.  The IzoT protocol stack
 *  automatically calls the <LonNvdAppSegmentHasBeenUpdated> function to 
 *  schedule an update whenever this callback returns *LonApiNoError*.
 *
 */
const LonApiError LonMemoryWrite(const unsigned address, 
						         const unsigned size,
						         const void* const pData);

/*
 * ******************************************************************************
 * SECTION: IzoT SDK Device Stack NVD API
 * ******************************************************************************
 *
 *  This section details the IzoT SDK Device Stack API functions that support 
 *  Non-Volatile Data (NVD).
 *
 *  Remarks:
 *  Non-volatile data is stored in data segments, identified by 
 *  <LonNvdSegmentType>, and are used to store IzoT persistent configuration 
 *  data. 
 */

/*
 *  Function: LonNvdAppSegmentHasBeenUpdated
 *  Informs the IzoT protocol stack that the application data segment 
 *  has been updated.  
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  Use this function to inform the IzoT protocol stack that some 
 *  application data been updated that should be written out to the 
 *  *LonNvdSegApplicationData* non-volatile data segment.  The IzoT  
 *  protocol stack will schedule a write to the *LonNvdSegApplicationData* 
 *  segment after the flush timeout has expired. 
 *
 *  It is generally not necessary to call this function when application data 
 *  has been updated by a network management write command or a network 
 *  variable update, because the IzoT protocol stack automatically 
 *  calls this function whenever the <LonMemoryWrite> callback handler returns 
 *  *LonApiNoError* and whenever a network variable update is received for a 
 *  network variable with the  *LON_NV_CONFIG_CLASS* attribute. However, the 
 *  application must call this function whenever it updates application-
 *  specific non-volatile data directly.
 * 
 */
FTXL_EXTERNAL_FN const LonApiError LonNvdAppSegmentHasBeenUpdated(void);

/*
 *  Function: LonNvdFlushData
 *  Flush all non-volatile data out to persistent storage.  
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *  This function can be called by the application task to block until all 
 *  non-volatile data writes have been completed.  The application might do 
 *  this, for example, in response to a <LonNvdStarvation> event.
 */
FTXL_EXTERNAL_FN const LonApiError LonNvdFlushData(void);

/*
 *  Function: LonNvdGetMaxSize
 *  Gets the number of bytes required to store persistent data.
 *
 *  Parameters:
 *  segmentType - The segment type, see <LonNvdSegmentType>
 *
 *  Returns:
 *  The number of bytes required to store persistent data for the specified 
 *  segment.
 *
 *  Remarks:
 *  This function will not typically be called directly by the application,
 *  but is used by the non-volatile data handlers defined in 
 *  FtxlNvdFlashDirect.c to reserve space for non-volatile data segments.
 */
FTXL_EXTERNAL_FN const int LonNvdGetMaxSize(LonNvdSegmentType segmentType);

/*
 * ******************************************************************************
 * SECTION: NVD CALLBACK PROTOTYPES
 * ******************************************************************************
 *
 *  This section defines the prototypes for the IzoT Device Stack API callback 
 *  functions supporting memory for Non-Volatile Data (NVD). 
 *
 *  Remarks:
 *  Example implementations of these functions are provided in 
 *  FtxlNvdFlashDirect.c and FtxlNvdFlashFs.c.  The application developer may 
 *  need to modify these functions to fit the type of non-volatile storage 
 *  supported by the application.
 *
 *  Callback functions are called by the IzoT protocol stack 
 *  immediately, as needed, and may be called from any IzoT task.  The 
 *  application *must not* call into the IzoT Device Stack API from within a callback.
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
const LonNvdHandle LonNvdOpenForRead(const LonNvdSegmentType type);

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
const LonNvdHandle LonNvdOpenForWrite(const LonNvdSegmentType type, const size_t size);

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
void LonNvdClose(const LonNvdHandle handle);

/* 
 *  Callback: LonNvdDelete
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
 void LonNvdDelete(const LonNvdSegmentType type);

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
                             void * const pBuffer);

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
const LonApiError LonNvdWrite(const LonNvdHandle handle, 
                              const size_t offset, 
                              const size_t size,
                              const void * const pData);

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
 *  LonTalk API will attempt to read the persistent data. 
 */
const LonBool LonNvdIsInTransaction(const LonNvdSegmentType type);

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
const LonApiError LonNvdEnterTransaction(const LonNvdSegmentType type);

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
const LonApiError LonNvdExitTransaction(const LonNvdSegmentType type);

/* 
 *  Callback: LonNvdGetApplicationSegmentSize
 *  Returns the number of bytes required to store the application non-volatile 
 *  data segment.
 *
 *  Remarks:
 *  This callback returns the total size of the non-volatile application 
 *  segment *LonNvdSegApplicationData*, in bytes. 
 */
const unsigned LonNvdGetApplicationSegmentSize(void);

/* 
 *  Callback: LonNvdDeserializeSegment
 *  Update the affected application control structures from a serialized image.
 *
 *  Parameters:
 *  pData - pointer to the serialized image
 *  size - size of the data, in bytes
 *
 *  Remarks:
 *  This callback is used to copy the serialized image to 
 *  the IzoT application control structures (CP value file, CP NVs, etc). 
 */
const LonApiError LonNvdDeserializeSegment(const void * const pData,
									       const size_t		  size);

/* 
 *  Callback: LonNvdSerializeSegment
 *  Return a serialized image of the specified segment.
 *
 *  Parameters:
 *  pData - pointer to store the serialized image
 *  size - size of the buffer, in bytes
 *
 *  Remarks:
 *  This callback is used to get a snapshot of the application segment data.
 *  It copies all of the data from the control structures to create a single serialized image.
 */
const LonApiError LonNvdSerializeSegment(void * const             pData,
								         const size_t		      size);

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
FTXL_EXTERNAL_FN void LonNvdStarvation(const unsigned seconds);

/*
 * ******************************************************************************
 * SECTION: IzoT SDK Device Stack Developer Support
 * ******************************************************************************
 *
 * This section details the IzoT SDK Device Stack API functions used by the IzoT 
 * application.  These functions are not typically accessed by the 
 * IzoT application directly.
 */
 
/*
 * Function: LonLidCreateStack
 * Initialize the IzoT Device Stack API and the IzoT Transceiver.  This function is 
 * the first function called by the generated <LonInit> function. 
 *
 * Parameters:
 * pInterface - Pointer to a <LonStackInterfaceData> structure, which contains  
 *   data to define the static attributes of the program 
 * pControlData - Pointer to a <LonControlData> structure, which contains data 
 *   used to control the runtime aspects of the stack.  These aspects can be 
 *   set by the application at runtime. 
 *
 * Returns:
 * <LonApiError>.
 * 
 *  Remarks:
 *  Initializes and configures the IzoT driver and the IzoT protocol 
 *  stack. This must be the first call into the IzoT Device Stack API, and cannot 
 *  be called again until <LonLidDestroyStack> has been called.  After this 
 *  function has been called, the following functions can be called:  
 *  <LonLidRegisterStaticNv>, <LonLidRegisterMemoryWindow>, 
 *  <LonLidStartStack>, and <LonLidDestroyStack>.
 *
 *  Note that the stack expects reasonable values for all of the initialization 
 *  parameters.  The stack does not attempt to provide detailed error information
 *  when a parameter is out of range.
 *
 *  If <LonLidCreateStack> returns any error, the stack will simply not function.  
 *  It will not send or receive  messages over the network.  The service LED 
 *  will be left on (applicationless).
 */
FTXL_EXTERNAL_FN const LonApiError 
    LonLidCreateStack(const LonStackInterfaceData* const pInterface,
                      const LonControlData       * const pControlData);


/*
 * Function: LonLidRegisterStaticNv
 * Registers a static network variable with the IzoT Device Stack API.
 *
 *  Parameters:
 *  pNvDef - pointer to a <LonNvDefinition> structure
 *
 *  Returns:
 *  <LonApiError>.
 * 
 *  Remarks:
 *  This function registers a static network variable with the IzoT Device Stack API.
 *  This function can be called only after <LonLidCreateStack>, but before <LonLidStartStack>.
 */
FTXL_EXTERNAL_FN const LonApiError LonLidRegisterStaticNv(const LonNvDefinition* const pNvDef);

/*
 *  Function: LonLidRegisterMemoryWindow
 *  Register memory addresses to be mapped to IzoT application managed memory.
 *
 *  Parameters:
 *  windowAddress - the starting ('Neuron') address (in host byte order) of
 *     the window
 *  windowSize - the size of the window, in bytes
 *
 *  Returns:
 *  <LonApiError>.
 * 
 *  Remarks:
 *  This function is used to open up a window in the IzoT Transceiver memory
 *  space.  Any network management memory read that uses absolute addressing 
 *  and that falls completely within the window will call the <LonMemoryRead> 
 *  callback function. Any network management memory write that uses absolute 
 *  addressing and that falls completely within the window will call the
 *  <LonMemoryWrite> callback function.  
 *
 *  This function can only be called after <LonLidCreateStack>, but before 
 *  <LonLidStartStack>.  The address space for these memory windows is between 
 *  0x0001 and 0xffff. 
 */
FTXL_EXTERNAL_FN const LonApiError LonLidRegisterMemoryWindow(const unsigned windowAddress, 
									  				          const unsigned windowSize);
/*
 *  Function: LonLidStartStack
 *  Completes the initialization of the IzoT protocol stack.  
 *
 *  Returns:
 *  <LonApiError>.
 * 
 *  Remarks:
 *  Starts running the stack.  This is typically the last function called by 
 *  <LonInit>. It is illegal to call <LonLidRegisterStaticNv> or 
 *  <LonLidRegisterMemoryWindow> after calling this function.  
 */
FTXL_EXTERNAL_FN const LonApiError LonLidStartStack(void);

/*
 *  Function: LonLidDestroyStack 
 *  Stops the IzoT protocol stack and frees all memory that it has 
 *  allocated. 
 *
 *  Returns:
 *  <LonApiError>.
 * 
 *  Remarks:
 *  Waits for non-volatile writes to complete, stops the stack, and frees all 
 *  temporary memory created during execution of the stack.  The service LED is 
 *  lit to indicate that the device is applicationless.
 */
FTXL_EXTERNAL_FN void LonLidDestroyStack(void);

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
FTXL_EXTERNAL_FN void LonGetMyIpAddress(int *pAddress, int *pPort);

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
FTXL_EXTERNAL_FN const char *LonGetMyNetworkInterface(void);


/*
 *  Function: LonGenerateUniqueId
 *  Generate uniqueID.  The first byte is always FE (currently used for virtual neuron ID's).
 *  To avoid looking like a virtual neuron ID (whose format is FEAAAAAAAAxxxx) the second byte 
 *  can be any value but AA (this allows virtual neuron ID space to increase dramatically).  
 *  The remaining bytes can take on any value, except 00000000.  This gives a space of 255 * 2^32.  
 *  The 5 variable bytes are chosen at random (using the time of day clock as a seed) and the .  
 *  This obviously is not guaranteed to produce unique neuron Ids.  The odds of having two 
 *  virtual network interfaces with the same neuron ID where the number of virtual network 
 *  interfaces is N, is approximately given by the following formula:  (255*2^32) / ((n^2)/2).  
 *  Assuming that you have less than 100 virtual network interfaces the odds of a collision is 
 *  less than 2 billion to one.  
 *       
 *
 *  Returns:
 *  The generated unique ID.
 * 
 *  Remarks:
 *   
 */ 
FTXL_EXTERNAL_FN const LonApiError LonGenerateUniqueId(LonUniqueId* pDeviceID);

/*
 *  Function: LonGetNvdFsPath
 *  This function is used by the default implementation of NVD (non-volatile data) callbacks 
 *  <LonNvdOpenForRead>, <LonNvdOpenForWrite>, <LonNvdDelete>, <LonNvdIsInTransaction>,
 *  <LonNvdEnterTransaction> and <LonNvdExitTransaction> to determine the location of the 
 *  non-volatile data folder/path.
 *
 *  Parameters:
 *  pFsPath - Pointer to the buffer to hold the non-volatile data folder/path
 *  maxLength - size of the buffer
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *
 */
FTXL_EXTERNAL_FN const LonApiError LonGetNvdFsPath(char* pFsPath, int maxLength);

/*
 *  Function: LonSetNvdFsPath
 *  Sets the non-volatile data folder. 
 *  
 *  Parameters:
 *  pFsPath - the path to save the non-volatile data
 *
 *  Returns:
 *  <LonApiError>.
 *
 */
FTXL_EXTERNAL_FN const LonApiError LonSetNvdFsPath(const char* pFsPath);

/*
 *  Function: LonSetDeviceUri
 *  Sets the device URI.  This API must be called before the create stack <LonLidCreateStack>
 *  
 *  Parameters:
 *  pDeviceURI - Pointer to device URI
 *      //12.13.67.89:1234/uc ->  IP Address and port for unicast (using IP-852 config server)
 *      //12.13.67.89:1234/mc ->  IP Address and port for multiast
 *
 *  Returns:
 *  <LonApiError>.
 *
 */
FTXL_EXTERNAL_FN const LonApiError LonSetDeviceUri(const char* pDeviceURI);

/*
 *  Function: LonGetDeviceUri
 *  Returns the Device URI used to create the stack.
 *
 *  Parameters:
 *  pDeviceURI - Pointer to the buffer to hold the device URI
 *  maxLength - size of the buffer
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Remarks:
 *
 */
FTXL_EXTERNAL_FN const LonApiError LonGetDeviceUri(char* pDeviceURI, int maxLength);


/*
 *  Function: LonSetTracefile
 *  Sets the filename to use for the trace logging.  When the pFilename is NULL, trace logging stops and a previously opened 
 *  tracefile is closed. 
 *  Parameters:
 *  pFilename - the filename for the trace logging
 *  append - if it sets to true, it means to open file for writing at the end of the file (appending) without removing the EOF marker
 *             before writing new data to the file; creates the file first if it doesn't exist.
 *           if it sets to false, it means to open an empty trace file for writing. If the given file exists, 
 *             its contents are destroyed.
 *
 *  Returns:     
 *   none 
 *
 */
FTXL_EXTERNAL_FN void LonSetTracefile(const char* pFilename, LonBool append);

/*
 *  Function: LonGetPersistentUniqueID
 *  Gets the unique ID that is currently saved in the persistent(nvd) file.  This only applies for IP-852 interface.
 *  When the IzoT stack object is created via the <LonLidCreateStack> the ID that is unique for each IP-852 device
 *  is saved into a persistent file located in the folder specified in <LonSetNvdFsPath>.
 *  It returns a zero unique ID if there is none.
 *       
 *  Parameters:
 *  pId - pointer to the buffer to hold the unique ID
 *  
 *  Returns:
 *  <LonApiError>.
 * 
 *  Remarks:
 *   
 */ 
FTXL_EXTERNAL_FN const LonApiError LonGetPersistentUniqueID(LonUniqueId* pId);

/*
 *  Function: LonQueryReadOnlyData
 *  Request copy of local read-only data.
 *
 *  Parameters:
 *  pReadOnlyData - pointer to a <LonReadOnlyData> structure
 *
 *  Returns:
 *  <LonApiError>.
 *
 *  Call this function to request a copy of device's read-only data.
 *  The read-only data will be stored in the <LonReadOnlyData> structure
 *  provided. 
 */
FTXL_EXTERNAL_FN const LonApiError LonQueryReadOnlyData(LonReadOnlyData* const pReadOnlyData);

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
FTXL_EXTERNAL_FN const LonBool LonFilterMsgArrived(const LonReceiveAddress* const pAddress, 
                          const LonCorrelator correlator,
                          const LonBool priority, 
                          const LonServiceType serviceType, 
                          const LonBool authenticated, 
                          const LonByte code, 
                          const LonByte* const pData, const unsigned dataLength);

/* 
 *  Event: LonFilterResponseArrived
 *  Occurs when a response arrives.
 *
 *  pAddress - source and destination address used for response (see 
 *             <LonResponseAddress>)
 *  tag - tag to match the response to the request
 *  code - response code
 *  pData - pointer to response data, may by NULL if dataLength is zero
 *  dataLength - number of bytes available through pData.
 *
 *  Remarks:
 *  This event handler is called when a response arrives.  Responses may be 
 *  sent by other devices when the IzoT device sends a message using 
 *  <LonSendMsg> with a <LonServiceType> of *LonServiceRequest*.
 *
 */
FTXL_EXTERNAL_FN const LonBool LonFilterResponseArrived(const LonResponseAddress* const pAddress, 
                        const unsigned tag, 
                        const LonByte code, 
                        const LonByte* const pData, const unsigned dataLength);


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
FTXL_EXTERNAL_FN const LonBool LonFilterMsgCompleted(const unsigned tag, const LonBool success);

/*
 *  Function: LonGetAppSignature
 *  Gets the application's signature
 *  
 *  Returns:     
 *  The application signature which was sepcified by the application 
 *  when the stack is created (<LonLidCreateStack>).
 */
FTXL_EXTERNAL_FN unsigned LonGetAppSignature();

/*
 *  Function: LonGetAliasCount
 *  Gets the size of the alias table
 *  
 *  Returns:     
 *  The size of the alias table which is specified by the application 
 *  when the stack is created (<LonLidCreateStack>).
 */
FTXL_EXTERNAL_FN unsigned LonGetAliasCount();

/*
 *  Function: LonGetAddressTableCount
 *  Gets the size of the address table
 *  
 *  Returns:     
 *  The size of the address table which is specified by the application 
 *  when the stack is created (<LonLidCreateStack>).
 */
FTXL_EXTERNAL_FN unsigned LonGetAddressTableCount();

/*
 *  Function: LonGetStaticNVCount
 *  Gets the size of the address table
 *  
 *  Returns:     
 *  The number of static network variable specified by the application 
 *  when the stack is created (<LonLidCreateStack>).
 */
FTXL_EXTERNAL_FN unsigned LonGetStaticNVCount();

/*
 *  Function: LonIsFirstRun
 *  Determine whether or not an application is running for the first time
 *  
 *  Returns:     
 *  True if this is the first time the application is running with the same
 *  setup and configuration.
 *  You can depend on this information if you need to preset certain
 *  values only when the first time the applicaiton is running.
 */
FTXL_EXTERNAL_FN const LonApiError LonIsFirstRun(LonBool* const pIsFirstRun);

/*
 *  Function: LonGetSIDataLength
 *  Computes the length of the version 1 SI data
 *  
 *  Returns: 
 *  The SI data length in bytes
 *
 *  Remarks:  
 *  This function is used to find out how big the data buffer needs to be allocated for the SI data.
 */
FTXL_EXTERNAL_FN const int LonGetSIDataLength();

/*
 *  Function: LonGetSIData
 *  Gets the version 1 SI data 
 *  
 *  Parameters:
 *  pSIData - pointer to the begining of SI data
 *  dataLen - the length of bytes pointed to by pSIData
 *
 *  Returns:  
 *      LonApiNoError - success
 *      LonApiNotInitialized - stack has not been created/initialized (<LonLidCreateStack>
 *      LonApiNotAllowed - pSIData buffer is too small.
 *
 *  Remarks:
 *  Use <LonGetSIDataLength> to find out the SI data length prior to calling this routine.
 *
 */
FTXL_EXTERNAL_FN const LonApiError LonGetSIData(LonByte* pSIData, const int dataLen);

/*
 *  Function: LonGetDidFromLocalAddress
 *  Gets the domain, subnet and node ID from the local IP address.
 *  The first two bytes of the IP address represent the LONTalk services domain ID,
 *  the third byte represents the LONTalk services subnet and the fourth byte
 *  represents the LONTalk services node ID.
 *  
 *  Parameters:
 *  pDomId - pointer to the domain Id
 *  domLength - the length of domain Id
 *  subnetID - subnet Id
 *  nodeID - node Id
 *
 *  Returns:  
 *      LonApiNoError - success
 *      LonApiNoIpAddress - can't retrieve Did from the local IP Address
 *
 */
FTXL_EXTERNAL_FN const LonApiError LonGetDidFromLocalAddress(LonByte* pDomId, unsigned* domLength, unsigned* subnetID, unsigned* nodeID);

/*
 *  Function: LonResetPersistence
 *  Remove the persistence data files. This API must be called before the create stack <LonLidCreateStack>
 *  
 *  Parameters:
 *  resetType - See the PersistenceResetType for different types of reset
 *
 *  Returns:
 *  <LonApiError>.
 *
 */
FTXL_EXTERNAL_FN const LonApiError LonResetPersistence(PersistenceResetType resetType);

/*
 *  In IzoT, LonTalk Stack callback functions are registered
 *  with a callback handler of a suitable type, and must be
 *  de-registered by calling the corresponding handler registrar
 *  with a NULL pointer for the callback.
 *
 *  For example, the LonWink event is implemented with a function
 *  of type LonWinkFunction, and registered using the LonWinkRegistrar
 *  API as shown in this hypothetical example:
 *
 *  typedef void (*LonWinkFunction)(void);
 *  extern LonApiError LonWinkRegistrar(LonWinkFunction handler);
 *
 *  void myWinkHandler(void) {
 *     flash_leds();
 *  }
 *
 *  int main(void) {
 *     ...
 *     // register wink handler:
 *     LonWinkRegistrar(myWinkHandler);
 *     ...
 *     // un-register wink handler:
 *     LonWinkRegistrar(NULL);
 *  }
 *
 *  The LonDeregisterAllCallbacks() API can be used to deregister all callback
 *  functions.
 *  It is not an error to deregister a callback twice, but only an
 *  unclaimed callback can be registered.
 */
FTXL_EXTERNAL_FN const LonApiError LonEventReadyRegistrar(LonEventReadyFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonGetCurrentNvSizeRegistrar(LonGetCurrentNvSizeFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonResetRegistrar(LonResetFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonWinkRegistrar(LonWinkFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonOfflineRegistrar(LonOfflineFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonOnlineRegistrar(LonOnlineFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonServicePinPressedRegistrar(LonServicePinPressedFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonServicePinHeldRegistrar(LonServicePinHeldFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonNvUpdateOccurredRegistrar(LonNvUpdateOccurredFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonNvUpdateCompletedRegistrar(LonNvUpdateCompletedFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonNvAddedRegistrar(LonNvAddedFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonNvTypeChangedRegistrar(LonNvTypeChangedFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonNvDeletedRegistrar(LonNvDeletedFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonMsgArrivedRegistrar(LonMsgArrivedFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonResponseArrivedRegistrar(LonResponseArrivedFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonMsgCompletedRegistrar(LonMsgCompletedFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonMemoryReadRegistrar(LonMemoryReadFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonMemoryWriteRegistrar(LonMemoryWriteFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonServiceLedStatusRegistrar(LonServiceLedStatusFunction handler);

// NVD callbacks 
FTXL_EXTERNAL_FN const LonApiError LonNvdOpenForReadRegistrar(LonNvdOpenForReadFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonNvdOpenForWriteRegistrar(LonNvdOpenForWriteFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonNvdCloseRegistrar(LonNvdCloseFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonNvdDeleteRegistrar(LonNvdDeleteFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonNvdReadRegistrar(LonNvdReadFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonNvdWriteRegistrar(LonNvdWriteFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonNvdIsInTransactionRegistrar(LonNvdIsInTransactionFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonNvdEnterTransactionRegistrar(LonNvdEnterTransactionFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonNvdExitTransactionRegistrar(LonNvdExitTransactionFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonNvdGetApplicationSegmentSizeRegistrar(LonNvdGetApplicationSegmentSizeFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonNvdDeserializeSegmentRegistrar(LonNvdDeserializeSegmentFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonNvdSerializeSegmentRegistrar(LonNvdSerializeSegmentFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonNvdStarvationRegistrar(LonNvdStarvationFunction handler);

// ISI callbacks
FTXL_EXTERNAL_FN const LonApiError LonFilterMsgArrivedRegistrar(LonFilterMsgArrivedFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonFilterResponseArrivedRegistrar(LonFilterResponseArrivedFunction handler);
FTXL_EXTERNAL_FN const LonApiError LonFilterMsgCompletedRegistrar(LonFilterMsgCompletedFunction handler);

FTXL_EXTERNAL_FN LonCallbackVectors theLonCallbackVectors;
FTXL_EXTERNAL_FN void LonDeregisterAllCallbacks(void);

#ifdef  __cplusplus
}
#endif




#endif /* _FTXL_API_H */
