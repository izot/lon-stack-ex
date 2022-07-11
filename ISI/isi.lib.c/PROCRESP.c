//	ProcResp.c	implementing IsiProcessResponse
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
// IsiProcessResponse processes a response. It does nothing if the ISI engine
// is not running or is not in the right state, if no response is actually waiting
// in the input queue, or if the response code is none of those the ISI engine might
// have been waiting for. Whenever the routine does not process the response, it
// returns TRUE to indicate that the response is still valid, awaiting further processing.
// If, however, the routine decides to process the response, it will return FALSE.
//
//	Revision 0, July 2005, Bernd Gauweiler

#include "isi_int.h"
#ifdef WIN32
#include <control.h>
#endif
#include <stddef.h>

extern LonByte donorId[NEURON_ID_LEN];     //  see fetch.c: the neuron ID of the donor device, or all zeroes if none

#ifdef  ISI_SUPPORT_DADAS

// Return value is TRUE if the response doesn't get processed
// False, otherwise
LonBool IsiProcessResponse(IsiType Type, const LonByte code, const LonByte* const pData, const unsigned dataLength)
{
    LonBool result;

    result = !_isiVolatile.Running;

    if (_isiVolatile.Running)
    {
        // we are running and a response is waiting.
        if (_isiVolatile.State == isiStateAwaitQdr)
        {
            if (code == LONTALK_QUERY_DOMAIN_FAILURE || code == LONTALK_UPDATE_DOMAIN_FAILURE)
            {
                //
                // query domain failure response. This would occur in response to our query domain request,
                // which is issued as part of the domain sniffing process. Give up:
                //
                isiDasExtState = isiDasNormal;
                _IsiUpdateUiAndStateTimeout(0, isiNormal, isiAborted, isiAbortUnsuccessful);
            } 
            else if (code == LONTALK_QUERY_DOMAIN_SUCCESS)
            {
                //
                // The domain sniffing process requested the primary domain from the donor device, and here it comes!
                // Now, we simply need to obtain the response data and join this domain. Note that a DAS satisfies
                // a DIDRQ with DIDRM data taken from its own domain table; the IsiGetPrimaryDid() function is only
                // used on a DAS during initial initialization.
                //
                const NM_query_domain_response *pRemoteDomain;
                //pRemoteDomain = (const NM_query_domain_response*)_IsiGetRespInDataPtr();
                pRemoteDomain = (const NM_query_domain_response*)pData;         // _IsiGetRespInDataPtr();

                if (isiDasExtState & isiDasFetchDomain) 
                {
                    // we received this query domain response as the last step in a FetchDomain process. The purpose of
                    // this process is to read a remote device's primary domain ID, and to assign this ID to self (aka
                    // domain sniffing process).
#ifdef ISI_SUPPORT_DIAGNOSTICS
                    if (_IsiSetDomain(ISI_PRIMARY_DOMAIN_INDEX, pRemoteDomain->len, pRemoteDomain->id, _GetIsiSubnet(), _GetIsiNode())) 
                    {
                        _IsiConditionalDiagnostics(isiSubnetNodeAllocation, ISI_PRIMARY_DOMAIN_INDEX);
                    }
#else
                    (void)_IsiSetDomain(ISI_PRIMARY_DOMAIN_INDEX, pRemoteDomain->len, pRemoteDomain->id, _GetIsiSubnet(), _GetIsiNode());
#endif
                    _IsiSendDrum();

                    if (isiDasExtState & isiDasAutoDeviceAcquisition)
                    {
                        // if we received a DIDRQ message while we were in domain sniffing mode, and if that DIDRQ
                        // originates from our domain donor device, we try to satisfy the donor device's undesired
                        // domain acquisition procedure. There is a possible race condition if
                        memcpy(lastDidrq.NeuronId, donorId, NEURON_ID_LEN);
                        _IsiSendDidrm(isiDidrm);
                        _IsiSendDidrm(isiDidcf);
                    }

                    goto BeDoneWithSuccess;     //  this GOTO saves bytes

                }
                else if (isiDasExtState & isiDasFetchDevice_Query)
                {
                    // we received this query domain response as a step in the FetchDevice process. Until here, the FetchDevice
                    // and FetchDomain processes are identical. The purpose of the FetchDomain process is to obtain a remote device's
                    // primary domain ID and to assign that ID to self. The purpose of the FetchDevice process is to assign the local
                    // primary domain ID to the remote device (eliminate need for remote devices to support ISI-DA domain acquisition
                    // procedures). To do that, we read the remote domain ID, change the length and ID value to the local values while
                    // keeping the rest intact, assign that lot to the remote device again, and wait for a positive response.
                    const LonDomain *pLocalDomain;
                    NM_update_domain_request updateDomain;
                    LonSendAddress destination;

                    pLocalDomain = access_domain(ISI_PRIMARY_DOMAIN_INDEX);

                    updateDomain.domain_index = ISI_PRIMARY_DOMAIN_INDEX;
                    updateDomain.subnet = pRemoteDomain->subnet;
                    updateDomain.must_be_one = 1;
                    updateDomain.node = pRemoteDomain->node;
                    updateDomain.len = LON_GET_ATTRIBUTE_P(pLocalDomain,LON_DOMAIN_ID_LENGTH) & 0x07u;
                    memcpy(updateDomain.id, pLocalDomain->Id, DOMAIN_ID_LEN);
                    memcpy(updateDomain.key, pRemoteDomain->key, AUTH_KEY_LEN);

                    // 0x00 causes this to go  out on the primary domain - see niddest.ns for details
                    // [gmr]:  If you require a ack/response, it will always come back using the domain of the requestor,
                    // regardless of whether the target is in that domain or not.  If it is not, it stores the domain ID in
                    // the "flex domain" with subnet/node of 0/0.  So, you can use any domain you like to talk to a node with
                    // NID addressing but that domain and source subnet must be routable and there can't be more than one
                    // sender at a time using the flex domain (not a protocol restriction, just a Neuron resource limitation).
                    //
                    _IsiNidDestination(&destination, 0x00, ISI_UPDATE_DOMAIN_RETRIES, (LonByte *)&donorId);
                    _IsiSend(LONTALK_UPDATE_DOMAIN_MESSAGE, LonServiceRequest, &updateDomain, sizeof(updateDomain), &destination);
                    isiDasExtState = isiDasFetchDevice_Confirm;
                    _isiVolatile.Timeout = ISI_T_UDR;
                }
            } 
            else if (code == LONTALK_UPDATE_DOMAIN_SUCCESS && isiDasExtState == isiDasFetchDevice_Confirm)
            {
BeDoneWithSuccess:
                _IsiUpdateUi(isiRegistered);    // let the application know that we have completed successfully.

                // last not least, notify application and return to normal (idle) state
                isiDasExtState = isiDasNormal;
                _IsiUpdateUiAndStateTimeout(0, isiStateNormal, isiNormal, ISI_NO_ASSEMBLY);
            } else {
                result = TRUE;  // unrecognized code in this state. Left for application to deal with.
            }
        } else {
            result = TRUE;  // unrecognized state. Left for application to deal with.
        }

    }
    return result;
}
#endif  //  ISI_SUPPORT_DADAS
//	end of ProcResp.c
