//	AcqDom.c	implementing IsiAcquireDomain
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
//  Revision 0, February 2005, Bernd Gauweiler
//  Revision 1, July 2005, BG: Actually implement the body

#include "isi_int.h"
#ifdef WIN32
#include <control.h>
#endif

void IsiAcquireDomain(LonBool SharedServicePin)
{

    if(!SharedServicePin) 
    {
        //  if this device shares the service pin with the ISI registration button, the firmware will have
        //  b'cast the service pin message already by now. If, however, the registration button is different
        //  from the service pin, we'll send a service pin message anyhow. This allows for the same installation
        //  paradigm to be used, whether in self-installed or managed mode, helps with transitioning a self-installed
        //  node to a managed network, and helps with generic tools (LNS discovery, NodeUtil, etc).
        //
        //  Note it is important that we send the service pin message first, prior to the regular business of acquiring
        //  the domain: Firstly, this mimicks the regular firmware behavior: when you activate the service pin, the
        //  firmware issues the related message, optionally followed with anything the application might do as a
        //  result of sensing the service pin activation. Secondly, it is important because the DAS assumes that
        //  domain ID donor devices always send the service pin message first, optionally followed by a DIDRQ message.
        //  See MsgDas.c for more details.
        //
        //  Also note this is one of the few operations ISI actually performs even if the engine is running. The intention
        //  is to allow for the same user interface to be used for domain registration, ISI or not. If a non-service pin UI
        //  is used for that operation, just go ahead and call IsiAcquireDomain(). If the SharedServicePin flag is set to
        //  FALSE, we'll send a service pin message in any case.
        service_pin_msg_send();
    }

	if (_isiVolatile.Running && !_isiVolatile.State) {
        // This is the API that is used to kick-off the process.
        // We use _IsiAcquireDomain to do whatever needs doing each time this process is restarted,
        // but do all setting-up in this routine:
        // Note there is no simple DIDRQ re-send in the way we resend CSMO messages and others. Here, the short
        // timer is set to ISI_T_RM and the Group field is set to ISI_DIDRQ_RETRIES.
        // Notice we use the Group field - this is OK because enrollment, device acquisition and domain acquisition
        // procedures are mutually exclusive.
        _isiVolatile.Group = ISI_DIDRQ_RETRIES;
        _IsiAcquireDomain();
    }
}

void _IsiAcquireDomain(void)
{
    _IsiAPIDebug("Start _IsiAcquireDomain - NuID=%d UniqueId = %02x %02x %02x %02x %02x %02x\n", 
                _isiPersist.Nuid,
                read_only_data.UniqueNodeId[0], read_only_data.UniqueNodeId[1], read_only_data.UniqueNodeId[2],
                read_only_data.UniqueNodeId[3], read_only_data.UniqueNodeId[4], read_only_data.UniqueNodeId[5]);

    // Broadcast the DIDRQ message and advance to the state where we await a DIDRM response from the DAS.
    memcpy(&isi_out.Msg.Didrq.NeuronId, read_only_data.UniqueNodeId, NEURON_ID_LEN);
    isi_out.Msg.Didrq.Nuid = _isiPersist.Nuid;
    _IsiBroadcast(isiDidrq, ISI_SECONDARY_DOMAIN_INDEX, 3);
    _IsiUpdateUiAndStateTimeout(ISI_T_RM, isiStateAwaitDidrx, isiRegistered, 0);

    _IsiAPIDebug("End _IsiAcquireDomain\n");
}

//	end of AcqDom.c
