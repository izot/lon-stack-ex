//
// LtProxy.cpp
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
// LonTalk Enhanced Proxy library
//
//  PS - proxy source - initiates transaction chain.
//  PR - proxy repeater - forwards proxy message.
//  PA - proxy agent - sends normal message to target.
//  PT - proxy target - terminates proxy chain.
//

#include "LtStackInternal.h"
#include "VxLayer.h"
#include "LtProxy.h"

#if FEATURE_INCLUDED(LTEP)

static const unsigned char addrSizeTable[PX_ADDRESS_TYPES] = 
{
	sizeof(ProxyHeader) + sizeof(ProxySicb) + sizeof(ProxyGroupAddress),
	sizeof(ProxyHeader) + sizeof(ProxySicb) + sizeof(ProxySubnetNodeAddress),
	sizeof(ProxyHeader) + sizeof(ProxySicb) + sizeof(ProxyNeuronIdAddress),
	sizeof(ProxyHeader) + sizeof(ProxySicb) + sizeof(ProxyBroadcastAddress),
	sizeof(ProxyHeader) + sizeof(ProxySicb) + sizeof(ProxyGroupAddressCompact),
	sizeof(ProxyHeader) + sizeof(ProxySicb) + sizeof(ProxySubnetNodeAddressCompact),
	sizeof(ProxyHeader) + sizeof(ProxySicb) + sizeof(ProxyNeuronIdAddressCompact),
	sizeof(ProxyHeader) + sizeof(ProxySicb) + sizeof(ProxySubnetNodeAddressCompact),
};

static int rptTimerValue[] = { 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072 };
static int rptTimerLongValue[] = { 4096, 6144, 8192, 12288, 16384 };

static int RptTimerValue(int idx, boolean isLong, int retry)
{
	int	result = 384;
	if (isLong)
		result =  rptTimerLongValue[idx];
	else
		result =  rptTimerValue[idx];

	return result;
}

void LtDeviceStack::processProxyRepeaterAsAgent(LtApduIn* pApdu, int txcpos)
{
	// Even though I am a repeater, I must also serve as an agent.  This
	// intended only for unackd/multicast/broadcast.  For this case, we
	// must allocate an additional buffer.  If not available, then we
	// just don't send the message.
	LtDomainConfiguration* pDc = &pApdu->getDomainConfiguration();

	int len = (int)(txcpos+sizeof(ProxyTxCtrl));

	// Make sure packet has an certain minimum size.
	if (len < pApdu->getLength()+1 && !pDc->isFlexDomain())
	{
		LtApduIn* pApduIn = new LtApduIn();
		pApduIn->setNoCompletion(true);
		pApduIn->getDomainConfiguration().setIndex(pDc->getIndex());
		pApduIn->setServiceType(LT_UNACKD);
		pApduIn->setCode(LT_APDU_ENHANCED_PROXY);
		pApduIn->setData(0, 0x00);
		pApduIn->setData(pApdu->getData()+len, 1, pApdu->getLength()-len+1);
		processLtep(pApduIn);
		delete pApduIn;
	}
}

//
// processLtepResponse
//
// Process a LTEP completion event
//
LtErrorType LtDeviceStack::processLtepCompletion(LtApduOut* pApdu)
{
	LtErrorType		err = LT_NO_ERROR;

	if (!pApdu->isRequest() || pApdu->getFailure())
	{
		if (pApdu->getSuccess())
		{
			pApdu->setCode(LT_ENHANCED_PROXY_SUCCESS);
			pApdu->setLength(0);
		}
		else
		{
			BYTE count = pApdu->getProxyCount();
			pApdu->setCode(LT_ENHANCED_PROXY_FAILURE);
			pApdu->setData(0, count);
			pApdu->setLength(1);
		}
		pApdu->setServiceType(LT_RESPONSE);
		pApdu->setFailure(false);
		pApdu->setSuccess(false);
		sendMessage(pApdu);
	}
	else
	{
		delete pApdu;
	}
	return err;
}

//
// processLtep
//
// Process LonTalk Enhanced Proxy
//
LtErrorType LtDeviceStack::processLtep(LtApduIn* pApdu)
{
	LtErrorType		err = LT_NO_ERROR;
	int				offset;
    int				count;
    int				alt;
	int				uniform;
	int				code;
	int				subnet;
	int				src_subnet;
	int				dst_subnet;
	int				txcpos;
    int				addressType;
	ProxyHeader		ph;
	ProxyTxCtrl		txc;
	int				dataLen = pApdu->getDataLength();
	byte*			pData = pApdu->getData();
	int				txTimer = 0;
	int				domIdx = pApdu->getDomainConfiguration().getIndex();

	ph = *(ProxyHeader*)pData;
	uniform = ph.uniform_by_src || ph.uniform_by_dest;
	src_subnet = pApdu->getDomainConfiguration().getSubnet();
	dst_subnet = pApdu->getSubnet();
	if (ph.uniform_by_src)
	{
		subnet = src_subnet;
	}
	else
	{
		subnet = dst_subnet;
	}
	count = ph.count;
	txcpos = (int)(sizeof(ProxyHeader) + (uniform ? 
					sizeof(ProxySubnetNodeAddressCompact)*count : 
					sizeof(ProxySubnetNodeAddress)*count));

    // Get txctrl values
    txc = *(ProxyTxCtrl*)(&pData[txcpos]);

	if (ph.all_agents && count)
	{
		processProxyRepeaterAsAgent(pApdu, txcpos);
	}

	LtApduOut*		pProxy = new LtApduOut(pApdu);

	pProxy->setProxy(true);
	pProxy->setProxyCount(count);

	// If we got something on the flex domain, we just use domain index 0.
	if (domIdx == -1)
		domIdx = 0;

    if (count != 0)
    {
		ProxySubnetNodeAddress *pa;

		pa = (ProxySubnetNodeAddress*) (&pData[sizeof(ProxyHeader)] - uniform);
		alt = pa->path;
		txTimer = RptTimerValue(txc.timer, ph.long_timer, txc.retry);
		pProxy->setSubnetNode(domIdx, uniform ? subnet : pa->subnet, pa->node, txTimer, txc.retry);
		pProxy->setServiceType(pApdu->getServiceType());

		count--;

        // Skip header and address
		offset = (int)(sizeof(ProxyHeader) + sizeof(ProxySubnetNodeAddress) - uniform);

        if (count == 0)
        {
            // Last address.  Skip txctrl too.
            offset += (int)sizeof(ProxyTxCtrl);
        }

        // To handle the case where repeated messages time out, we need to allow for
        // the fact that each repeater in the chain needs a little bit more time on the last timeout
        // than the previous guy.  So, we allow 512 msec for each round trip to propagate the failure
        // message.  This means adding 512*count msec at each hop.  
		// This mechanism is timed for A band power line.  If this proxy mechanism were employed in just about
		// any other medium, this would not be necessary.  So, the adjustment is deployed as a function of 
		// the tx_timer.  
		int adjust;
		adjust = 256;								// Add constant 256 msec at every hop
		if (ph.long_timer || txc.timer >= 10) adjust = 2*count;		// Add 512 msec per hop
		else if (txc.timer >= 8) adjust = 256*count;// Add 256 msec per hop
		pProxy->setTxTimerDeltaLast(adjust);

        // Send message on to next PR or PA
        code = LT_APDU_ENHANCED_PROXY;
		addressType = LT_AT_SUBNET_NODE;
		ph.count--;
		// Position new header into data space in preparation for copy below.
		pData[--offset] = *(unsigned char*)&ph;
    }
    else // Proxy Agent
    {
		ProxySicb* pProxySicb;

		pProxySicb = (ProxySicb*)&pData[sizeof(ProxyHeader)];
		txc = pProxySicb->txctrl;

        pProxy->setServiceType((LtServiceType)pProxySicb->service);

        // Set some explicit address fields.  These are assumed
        // to be the same for all address types.

        addressType = pProxySicb->type;
		offset = addrSizeTable[addressType];
		txTimer = RptTimerValue(txc.timer, false, txc.retry);

		// Remove "compact" bit
		addressType &= 0x3;

		{
			ProxyTargetAddress* pAddress;

			pAddress = (ProxyTargetAddress*)(pProxySicb + 1);

			if (pProxySicb->mode == PROXY_AGENT_MODE_ALTKEY)
			{
				int i;
				int len;
				boolean oma;
				ProxyAuthKey* pKey;
				BYTE			altKey[LT_OMA_DOMAIN_KEY_LENGTH];
				BYTE			*pKeyDelta;
				LtDomainConfiguration dc;

				pKey = (ProxyAuthKey*)pAddress;
				oma = pKey->type == AUTH_OMA;
				len = (unsigned char)(oma?sizeof(((ProxyOmaKey*)pKey)->key):sizeof(pKey->key));

				pKeyDelta = pKey->key;
				getDomainConfiguration(domIdx, &dc);
				for (i=0; i<len; i++)
				{
					altKey[i] = dc.getKey()[i] + *pKeyDelta;
					pKeyDelta++;
				}

				LtMsgOverride ovr(LtMsgOverrideOptions(LT_OVRD_AUTH_KEY), LT_UNACKD, 0, 0, 0, 0, (BYTE*)altKey, oma);
				pProxy->setOverride(&ovr);
				offset += (int)(sizeof(ProxyAuthKey)-sizeof(pKey->key)+len);
				pAddress = (ProxyTargetAddress*)((unsigned char*)pAddress + (int)(sizeof(ProxyAuthKey)-sizeof(pKey->key)+len));
			}

			LtUniqueId uid;
			switch (pProxySicb->type)
			{
			  case PX_NEURON_ID:
				uid = LtUniqueId(pAddress->nid.nid);
				pProxy->setUniqueId(domIdx,	pAddress->nid.subnet, uid, txTimer, txc.retry);
				break;
			  case PX_NEURON_ID_COMPACT:
				uid = LtUniqueId(pAddress->nidc.nid);
				pProxy->setUniqueId(domIdx,	0, uid, txTimer, txc.retry);
				break;
			  case PX_SUBNET_NODE_COMPACT_SRC:
				pProxy->setSubnetNode(domIdx, src_subnet, pAddress->snc.node, txTimer, txc.retry);
				break;
			  case PX_SUBNET_NODE_COMPACT_DEST:
				pProxy->setSubnetNode(domIdx, dst_subnet, pAddress->snc.node, txTimer, txc.retry);
				addressType = LT_AT_SUBNET_NODE;
				break;
			  case PX_GROUP:
				addressType = 0x80 | pAddress->gp.size;
				pProxy->setGroup(domIdx, pAddress->gp.group, pAddress->gp.size, txTimer, txc.retry);
				break;
			  case PX_GROUP_COMPACT:
				addressType = 0x80;
				pProxy->setGroup(domIdx, pAddress->gp.group, pAddress->gp.size, txTimer, txc.retry);
				break;
			  case PX_BROADCAST:
				pProxy->setBroadcast(domIdx, pAddress->bc.subnet, pAddress->bc.backlog, txTimer, txc.retry);
				break;
			  case PX_SUBNET_NODE:
				pProxy->setSubnetNode(domIdx, pAddress->sn.subnet, pAddress->sn.node, txTimer, txc.retry);
				break;
			  default:
				// Force failed completion from lower layers.
				addressType = 0x7f;
				break;
			}
		}

		switch (pProxySicb->mode)
		{
			case PROXY_AGENT_MODE_ZERO_SYNC:
			{
				// This packet must be sent synchronized to the zero crossing.
				pProxy->setZeroSync(true);
				break;
			}
			case PROXY_AGENT_MODE_ATTENUATE:
			{
				pProxy->setAttenuate(true);
				break;
			}
		}

	    code = pData[offset++];
		alt = pProxySicb->path;
    }

	// Include sanity checks on incoming packet length
	if (dataLen >= (int)(sizeof(ProxyHeader) + sizeof(ProxySicb) + 1) && offset <= dataLen)
	{
		pProxy->setData(&pData[offset], 0, dataLen-offset);
		pProxy->setCode(code);
		pProxy->setAlternatePath(alt ? LT_ALT_PATH : LT_NORMAL_PATH);
		pProxy->setAuthenticated(pApdu->getAuthenticated());
		sendMessage(pProxy);
		// Clear request status so that we don't respond now.
		pApdu->setServiceType(LT_ACKD);
	}
	else
	{
		// Improperly formed proxy message
		err = LT_INVALID_PARAMETER;
	}
	return err;
}

#endif  // FEATURE_INCLUDED(LTEP)
