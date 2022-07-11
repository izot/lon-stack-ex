//	snddidrm.c	implementing _IsiSendDidrm
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
//	Revision 0, July 2005, Bernd Gauweiler

#include "isi_int.h"

//  _IsiSendDidrm sends a DIDRM or DIDCF message (subject to the message code passed into this
//  function), using Neuron ID addressing and unacknowledged service w/ 3 repeats. The Neuron ID
//  for the destination address is obtained from a DIDRQ cache (lastDidrq)
#ifdef  ISI_SUPPORT_DADAS
static unsigned channelType, deviceCount;

void _IsiSendDidrm(IsiMessageCode code)
{
    const LonDomain *pDomain;
    LonSendAddress destination;

    pDomain = access_domain(ISI_PRIMARY_DOMAIN_INDEX);

    if ((isi_out.Header.Code=code) == isiDidrm)
    {
        // we safe the device count and channel type data from the DIDRM, and reuse that data later with the DIDCF.
        // the idea is to help implememations to match DIDCF with DIDRM - reality is that this implementation doesn't
        // match those fields at all, but accepts all DIDRM and DIDCF ***as long as they report the same domain ID***.
        // Otherwise, using redundant DAS would be a bad joke, as messages from one DAS would invalidate those from
        // another.
        channelType = _isiVolatile.ChannelType;
        deviceCount = _isiPersist.Devices;
    }

    isi_out.Msg.Didrm.ChannelType = channelType;
    isi_out.Msg.Didrm.DeviceCountEstimate = deviceCount;
    LON_SET_ATTRIBUTE(isi_out.Msg.Didrm, ISI_DID_LENGTH, LON_GET_ATTRIBUTE_P(pDomain,LON_DOMAIN_ID_LENGTH) & 0x07);
    memcpy(isi_out.Msg.Didrm.DomainId, pDomain->Id, LON_GET_ATTRIBUTE(isi_out.Msg.Didrm,ISI_DID_LENGTH));
    memcpy(isi_out.Msg.Didrm.NeuronId, read_only_data.UniqueNodeId, NEURON_ID_LEN);

    // 0x80 causes this to go out on the secondary domain - see niddest.ns for details.
    _IsiNidDestination(&destination, 0x80, ISI_DIDRM_RETRIES, lastDidrq.NeuronId);
    _IsiSend(ISI_MESSAGE_CODE, LonServiceRepeated, &isi_out, (unsigned)sizeof(IsiMessageHeader) + (unsigned)sizeof(IsiDidrm), &destination);
}

#endif  //  ISI_SUPPORT_DADAS
//	end of snddidrm.c
