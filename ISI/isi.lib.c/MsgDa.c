//	MsgDa.c	implementing IsiProcessMsgDa
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
//
//	Revision 0, February 2005, Bernd Gauweiler

#include "isi_int.h"
#ifdef WIN32
#include <control.h>
#endif
//#include <mem.h>
#include <stddef.h>

static IsiDidrm lastDidrm;

LonBool IsiProcessMsgDa(const LonByte code, const IsiMessage* msgIn, const unsigned dataLength)
{
    LonBool result;
    IsiMessageCode isiCode;

    // Initialize the result to FALSE - notice the IsiProcess* functions use negative logic;
    // a FALSE result means that the message has been processed and is still valid. The simple
    // and sole reason for using negative logic here is that it simplifies some of the expressions
    // required in C source. We simply safe negations when evaluating IsiProcessMsg*() results.
    result = FALSE;

    //  First, let's see if this message is one of the common subset messages that can be
    //  handled by IsiProcessMsgS:
    if (IsiProcessMsgS(code, msgIn, dataLength))
    {
        // No. Could be a message specific to ISI-DA:
        watchdog_update();

        isiCode = msgIn->Header.Code;

        if (isiCode == isiDidrm)
        {
            if (_isiVolatile.State == isiStateAwaitDidrx)
            {
                // we have been waiting for one DIDRM, so here it is.
                // Set timeout to ISI_T_COLL, remember the DIDRM that we just received, and change into
                // isiStateCollect.
                // Notice we copy the entire message, even though only some bytes in the beginning will be used for validation and
                // detection of alien DAS - when the DIDRM is finally accepted, the "remaining" fields are actually used.
                memcpy(&lastDidrm, &msgIn->Msg.Didrm, (unsigned)sizeof(IsiDidrm));
                _isiVolatile.State = isiStateCollect;
                _isiVolatile.Timeout = ISI_T_COLL;
            } 
            else if (_isiVolatile.State & (isiStateCollect | isiStateAwaitConfirm))
            {
                if (memcmp(&lastDidrm, &msgIn->Msg.Didrm, (unsigned)offsetof(IsiDidrm, NeuronId)))
                {    //  assumes: IsiDidrm structure starts with domain length and ID
                    // a mismatching DIDRM. That's bad - it means multiple DAS are set to device acquisition mode. Note that we only compare
                    // the DIDRM fields up to, but excluding, the DAS' neuron ID. This allows for multiple (redundant) DAS all to be in device
                    // acquisition mode at the same time - as long as they all serve the same domain, that's fine. Since the user will only confirm
                    // the device at a single DAS, there is no competition between the redundant DAS devices. Even if the user confirms the same
                    // device at multiple DAS the first one that gets through to the device will win.
                    // In short, we bail out with error if we get responses from multiple domains:
                    _IsiUpdateUiAndStateTimeout(0, isiStateNormal, isiAborted, isiAbortMismatchingDidrm);
                }
            }
        } 
        else if (isiCode == isiDidcf && _isiVolatile.State == isiStateAwaitConfirm)
        {
            if (memcmp(&lastDidrm, &msgIn->Msg.Didrm, (unsigned)offsetof(IsiDidrm, NeuronId)))
            {    //  assumes: IsiDidrm structure starts with domain length and ID
                // a DIDCF that relates to a different domain. We could have gotten this from an alien DAS (one serving a different domain),
                // assuming someone not only switched the alien device into device acquisition mode but also confirmed (our) device. Thus,
                // we need to play safe and bail out - if there are multiple genuine concurrent domain acquisition processes, each DAS will
                // only address the correct device. In short, we should not have gotten this one:
                _IsiUpdateUiAndStateTimeout(0, isiStateNormal, isiAborted, isiAbortMismatchingDidcf);
            } 
            else
            {
                // Good news! A matching DIDCF! GO for it:

                // handle the new primary domain data:
#ifdef ISI_SUPPORT_DIAGNOSTICS
            	if (_IsiSetDomain(ISI_PRIMARY_DOMAIN_INDEX, LON_GET_ATTRIBUTE(lastDidrm,ISI_DID_LENGTH), lastDidrm.DomainId, _GetIsiSubnet(), _GetIsiNode()))
                {
            		_IsiConditionalDiagnostics(isiSubnetNodeAllocation, ISI_PRIMARY_DOMAIN_INDEX);
            	}
#else
            	(void)_IsiSetDomain(ISI_PRIMARY_DOMAIN_INDEX, lastDidrm.DidLength, lastDidrm.DomainId, _GetIsiSubnet(), _GetIsiNode());
#endif
                _IsiSendDrum();

                // handle the new channel type and device count information as received with the first DIDRM that we got
                _IsiReceiveTimg(lastDidrm.DeviceCountEstimate, lastDidrm.ChannelType);

                _IsiUpdateUi(isiRegistered);    // let the application know that we have completed successfully.

                // last not least, notify application and return to normal (idle) state
                _IsiUpdateUiAndStateTimeout(0, isiStateNormal, isiNormal, ISI_NO_ASSEMBLY);
            }

#ifdef  ISI_SUPPORT_AUTOMATIC_CONNECTIONS
            //  A device that supports both automatic connections and DIDCF messages is required to monitor the DIDCF messages.
            //  If the device is host to one or more automatic connections, it must wait Tcsmr after the DIDCF (where each DIDCF
            //  re-triggers the Tcsmr timer, which is defined as 1.5 * Tacq. Upon expiration, CSMR are issued.
            //  However, we only need to start Tcsmr if at least one locally hosted automatic connection exists. We re-use the
            //  _isiTcsmr variable for this by adding one to this variable for each applicable connection table entry - if _isiTcsmr
            //  is not 0 at the end of this, then we load it with ISI_T_CSMR. Note that we do NOT initialize _isiTcsmr to zero here;
            //  the timer might already be ticking. In this case, we simply re-trigger it.
            {
                unsigned index;
                IsiConnection connection;
                for(index = 0; _IsiNextConnection(index, &connection); ++index)
                {
                     if (LON_GET_ATTRIBUTE(connection,ISI_CONN_STATE) == isiConnectionStateInUse
                         && connection.Host != ISI_NO_ASSEMBLY
                         && connection.Desc.OffsetAuto == ConnectionAuto_MASK)
                    {
                        // this connection is an automatic, locally hosted, connection. Unless it is marked for explicit Tcsmr already (which could
                        // be due to some prior DIDCF), schedule it for a CSMR message to be send whenever Tcsmr expires.
                        LON_SET_ATTRIBUTE(connection,ISI_CONN_STATE,isiConnectionStateTcsmr);
                        IsiSetConnection(&connection, index);
                        ++_isiTcsmr;
                    }
                    watchdog_update();
                }
                savePersistentData(IsiNvdSegConnectionTable);
            }
            // And finally do not forget to set the Tcsmr timer, if needed:
            if (_isiTcsmr) 
            {
                //  Whenever we receive a DIDCF message, we consider re-sending all our CSMR messages. The idea is to allow the new device to
                //  swiftly join the present automatic connections rather than having it (and the impatient user) wait until the right CSMR comes
                //  past, eventually, via the regular broadcast scheme. The Tcsmr timer has a minimum value of ISI_T_CSMR_PAUSE. This allows the
                //  new device to complete its own configuration; if we just sent out CSMR right now, the device in question would most
                //  likely simply loose them while reconfiguring or resetting.
                //  Once this minimum hesitation is complete, we schedule the CSMRs to go out at a random time. The maximum time is a fourth of the
                //  current device count, in seconds. This allows for a reasonably quick CSMR broadcast without flooding the network too badly.
#ifdef  ISI_SUPPORT_TIMG
                _isiTcsmr = ISI_TICKS_PER_SECOND * (unsigned long)_IsiRand(_isiPersist.Devices / 4u, ISI_T_CSMR_PAUSE);
#else
                _isiTcsmr = ISI_TICKS_PER_SECOND * (unsigned long)_IsiRand(ISI_DEFAULT_DEVICECOUNT / 4u, ISI_T_CSMR_PAUSE);
#endif  //  ISI_SUPPORT_TIMG
            }
#endif  //  ISI_SUPPORT_AUTOMATIC_CONNECTIONS

        } 
        else if (isiCode != isiDidrq)
        {
            //  We are an ISI-DA device and, therefore, won't honor a DIDRQ message. We recognize the message,
            //  however, and indicate this by returning FALSE from this message. This is a quite possible situation;
            //  imagine the dreaded Christmas morning syndrome: you get some ISI toys for Christmas and so does your
            //  neighbor. As both of you start playing with the new gizmos, there will be multiple concurrent domain
            //  acquisition processes. Because the DAS only communicates with the requesting device via Neuron-ID-
            //  addressing and because of the use of the confirmed registration protocol, both you and your neighbor
            //  will be happy. However, for this to happen, the ISI-DA device must quietly swallow any incoming DIDRQ
            //  messages, and shall not abort its own domain acquisition just because somebody else on the media also
            //  got some cool Christmas present.

            //  So, for anything that wasn't processed by IsiProcessMsgS, and that wasn't processed above in this
            //  if-else ladder, and that is not a DIDRQ, return TRUE to indicate that the message is still valid and
            //  has not been processed:
            result = TRUE;
        }
    }

	// otherwise, since the message has been processed, return FALSE:
	return result;
}

//	end of MsgDa.c
