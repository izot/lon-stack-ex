//	MsgDas.c	implementing IsiProcessMsgDas
//
// Copyright © 2005-2022 Dialog Semiconductor
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

#include "isi_int.h"
#ifdef WIN32
#include <control.h>
#endif
#include <stddef.h>
#if 0
#include <netmgmt.h>
#endif

//  the lastDidrq holds the last DIDRQ that we received in the right state of mind. This contains the Neuron ID of the remote device,
//  which in turn is used for addressing of DIDRM and DIDCF messages.
IsiDidrq    lastDidrq;

extern LonByte donorId[NEURON_ID_LEN];     //  see fetch.c: the neuron ID of the donor device, or all zeroes if none

#ifdef  ISI_SUPPORT_DADAS
//  extended DAS states
IsiDasExtendedStates isiDasExtState;
#endif  //  ISI_SUPPORT_DADAS

LonBool IsiProcessMsgDas(const LonByte code, const IsiMessage* msgIn, const unsigned dataLength)
{
#ifdef  ISI_SUPPORT_DADAS
	IsiMessageCode isiCode;
    LonBool retval = FALSE;

    _IsiAPIDebug("Start IsiProcessMsgDas - Code=%d State=%d\n", code, _isiVolatile.State);

    if (_isiVolatile.State == isiStateCollect && code == LONTALK_SERVICE_PIN_MESSAGE)
    {
        LonSendAddress destination;

        //  This is a service pin message, not an ISI message. We need to filter this one first,
        //  as all remaining code assumes the incoming message is a ISI message.
        const NM_service_pin_msg* pService;
        pService = (const NM_service_pin_msg*)msgIn;  // _IsiGetMsgInDataPtr();
        _IsiAPIDebug("Its a service pin message, not an ISI message.\n");
        if (memcmp(donorId, "\0\0\0\0\0", NEURON_ID_LEN))
        {
            //  this is the second service pin message. If the NEURON ID matches the previous donor ID,
            //  accept it and proceed to actually querying the donor's domain configuration. Otherwise,
            //  cancel out with error.
            if (memcmp(donorId, pService->neuron_id, NEURON_ID_LEN))
            {
                // Mismatch. Forget about it:
                _IsiAPIDebug("NEURON ID mismatch.  Donor=%x %x %x %x %x %x NeuronID=%x %x %x %x %x %x\n",
                        donorId[0], donorId[1],donorId[2],donorId[3],donorId[4],donorId[5],
                        pService->neuron_id[0], pService->neuron_id[1],pService->neuron_id[2],pService->neuron_id[3],pService->neuron_id[4],pService->neuron_id[5]);
                _IsiUpdateUiAndStateTimeout(0, isiNormal, isiAborted, isiAbortMismatchService);
            } 
            else
            {
                // OK, this is a match.
                //
                // We expect a success or failure response, however, the application might fail to cooperate correctly
                // (it must implement when(resp_arrives) and call ISI from there). To prevent us getting stuck in this
                // case, we set an ISI timeout (ISI_T_QDR) which, upon expiry, signals failure and returns us to normal
                // operation.
                NM_query_domain_request queryDomain;
                queryDomain.domain_index = ISI_PRIMARY_DOMAIN_INDEX;

                //
                // 0x00 causes this to go  out on the primary domain - see niddest.ns for details
                // [gmr]:  If you require a ack/response, it will always come back using the domain of the requestor,
                // regardless of whether the target is in that domain or not.  If it is not, it stores the domain ID in
                // the "flex domain" with subnet/node of 0/0.  So, you can use any domain you like to talk to a node with
                // NID addressing but that domain and source subnet must be routable and there can't be more than one
                // sender at a time using the flex domain (not a protocol restriction, just a Neuron resource limitation).
                //
                _IsiNidDestination(&destination, 0x00, ISI_QUERY_DOMAIN_RETRIES, (const LonByte *)donorId);
                _IsiSend(LONTALK_QUERY_DOMAIN_MESSAGE, LonServiceRequest, &queryDomain, sizeof(queryDomain), &destination);
                _isiVolatile.State = isiStateAwaitQdr;
                _isiVolatile.Timeout = ISI_T_QDR;
            }
        }
        else
        {
            //  the donor buffer is clear - thus, we know this is the first of two service pin messages
            //  Copy the donor's NID into a buffer, return a standard LonTalk WINK message, and await the
            //  second (confirming) service pin message from the same donor.
            _IsiAPIDebug("Donor ID is clear.  This is the first of two service pin messages\n");
            memcpy(donorId, pService->neuron_id, NEURON_ID_LEN);
            //  send a Wink message to the potential donor:
            //  0x00 causes this to go  out on the primary domain - see niddest.ns for details
            _IsiNidDestination(&destination, 0x00, ISI_WINK_REPEATS, (const LonByte *)donorId);
            _IsiSend(LONTALK_WINK_MESSAGE, LonServiceRepeated, NULL, 0, &destination);

            // re-set the timeout (but remain in this state)
            _isiVolatile.Timeout = ISI_T_ACQ;
        }
    }
    else
    {
        // Follow is processing for a normal ISI message:
        // reset the spreading timer:
        _isiVolatile.Spreading = 0;

        isiCode = msgIn->Header.Code;
        _IsiAPIDebug("Its a normal ISI message- Code=%d\n", isiCode);
        if (isiCode == isiDrum || isiCode == isiDrumEx)
        {
            _IsiReceiveDrumDas(msgIn);
        }
        else
        if (isiCode == isiDidrq)
        {
            if (_isiVolatile.State & isiStateAwaitDidrx)
            {
                // OK - we've been waiting for a DIDRQ, and here it comes. Good. If one comes that we have not been waiting for
                // (we'd be in a different state), we will just quietly drop the message.
                // If we are ready to honor the request, we need to copy it to lastDidrq (so that the requestor's Neuron ID is
                // kept safe for sending DIDRM and DIDCF later), and issue a DIDRM:
                memcpy(&lastDidrq, &msgIn->Msg.Didrq, sizeof(IsiDidrq));
                _IsiSendDidrm(isiDidrm);
                _isiVolatile.State |= isiStateAwaitConfirm;
                _isiVolatile.Timeout = ISI_T_ACQ;
            }
            else if (_isiVolatile.State == isiStateCollect)
            {
                // We have received a DIDRQ domain request message, but we are ourselves in the middle of the domain sniffing
                // process. We do not have a good domain ID to supply therefore. We flag this DIDRQ and, once domain sniffing
                // is completed, we automatically enter device acquisition. This allows for domain donor devices that share the
                // service pin with the ISI registration button, causing not only the service pin to be propagated as required
                // by the sniffing process, but also causing the donor to enter domain acquisition mode because the registration
                // button has been activated. For the donor, this is benign: it will -eventually- time out, report failure, and
                // continue using the last known good domain. The error report, however, might confuse the user. We try to avoid
                // this by flagging receipt of DIDRQ here. Presumably, the domain sniffing process is completed before the donor's
                // domain acquisition attempt times out. Thus, when the donor starts the next attempt, we'd be in the right mode.
                // Of course we cannot do this for any DIDRQ that we receive, but only for those that match the donor ID previously
                // recorded.
                if (!memcmp(donorId, msgIn->Msg.Didrq.NeuronId, NEURON_ID_LEN))
                {
                    // a match. flag this:
                    isiDasExtState |= isiDasAutoDeviceAcquisition;
                }
            }
        }
        else if (isiCode == isiTimg)
        {
            // handle everything else just normally, except incoming TIMG messages.
            // Incoming TIMG messages probably originate from another DAS, but since we are a DAS ourselves, we wouldn't really know
            // what to do with this alien TIMG message. There is no way to merge DAS knowledge, or to tell whether the alien one has
            // better data or not. In lieu of a better plan, we simply drop it - but return TRUE to mark this as an unprocessed ISI
            // message. That way, some fancy DAS might still be able to do something useful with it.
            retval = TRUE;
        }
        else
        {
            watchdog_update();
            retval = IsiProcessMsgDa(code, msgIn, dataLength);
        }
    }

	// otherwise, since the message has been processed, return FALSE - technically, this is unreachable code. We need to provide this statement
    // nonetheless, as the compiler doesn't understand that this code cannot be reached, and produces a "function must return a value" error
    // otherwise.
    _IsiAPIDebug("IsiProcessMsgDas = %d\n", retval);
	return retval;
#else
    // DAS not supported with this library:
    return TRUE;
#endif  //  ISI_SUPPORT_DADAS
}
//	end of MsgDas.c
