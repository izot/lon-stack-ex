//	RcvDrumS.c	implementing _IsiReceiveDrumS
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

//	_IsiReceiveDrumS: receive a DRUM, detect and resolve collisions:

void _IsiReceiveDrumS(const IsiMessage* msgIn)
{
	const IsiDrum* pDrum;
	const LonDomain* pDomain;
	unsigned DomainIdLength;

	pDrum = &((const IsiMessage*)msgIn)->Msg.Drum;

    _IsiAPIDebug("_IsiReceiveDrumS ");
    _IsiAPIDump("0x", (void *)pDrum, sizeof(IsiDrum), "\n");

	if (memcmp(pDrum->NeuronId, read_only_data.UniqueNodeId, NEURON_ID_LEN))
    {
		// it's not from us. Let's see if it's for us:
		pDomain = access_domain(ISI_PRIMARY_DOMAIN_INDEX);
		DomainIdLength = pDomain->InvalidIdLength & 0x07u;

		if (LON_GET_ATTRIBUTE_P(pDrum,ISI_DRUM_DIDLENGTH) == DomainIdLength
		    && !memcmp(pDrum->DomainId, pDomain->Id, DomainIdLength) ) 
        {
			// heck - it's for us, and from someone else:
#ifdef ISI_SUPPORT_DIAGNOSTICS
			_IsiConditionalDiagnostics(isiReceiveDrum, 0);
#endif
			if (pDrum->SubnetId == pDomain->Subnet && pDrum->NodeId == LON_GET_ATTRIBUTE_P(pDomain,LON_DOMAIN_NODE) && !gIsiDerivableAddr)
            {
				// collision. Only try to resolve if the LONTalk Services subnet and node ID cannot be derived from the IP address
				if (_IsiSetDomain(ISI_PRIMARY_DOMAIN_INDEX, DomainIdLength, (const LonByte *)pDrum->DomainId, _IsiAllocSubnet(), _IsiAllocNode())) {
#ifdef ISI_SUPPORT_DIAGNOSTICS
					_IsiConditionalDiagnostics(isiSubnetNodeDuplicate, ISI_PRIMARY_DOMAIN_INDEX);
#endif
					_IsiSendDrum();
				}
			}
		}
	}
}

//	end of RcvDrumS.c
