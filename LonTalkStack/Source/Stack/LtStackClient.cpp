//
// LtStackClient.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtStackClient.cpp#1 $
//

#include "LtStackInternal.h"

void LtStackClient::processPacket(boolean pPriority, LtPktInfo* pPkt)
{
	// Queue the packet up for the stack.  
	getStack()->processPacket(pPriority, pPkt);
}

void LtStackClient::updateStats(const byte* pApdu, bool isUnackd, bool domainMatch, bool gotThisMsg)
{
	// Only the main client updates stats; otherwise we would bump the stats once per
	// client and we really only want to bump them once per stack.
	if (getStack()->getMainClient() == this)
	{
		getStack()->updateStats(pApdu, isUnackd, domainMatch, gotThisMsg);
	}
}

void LtStackClient::setErrorLog(int error) 
{
	getStack()->putErrorLog(error);
}

boolean LtUniqueIdClient::getAddress(int& index, LtUniqueId *pUniqueId)
{
	if (index++ == 0)
	{
		*pUniqueId = *(LtUniqueId*) this;
		return true;
	}
	return false;
}	

boolean LtUniqueIdClient::stop()
{
	// Synchronize the network image before stopping.
	getStack()->getNetworkImage()->sync();
	return true;
}

boolean LtSubnetNodeClient::getAddress(int& nIndex, LtDomain *pDomain, LtSubnetNode *pSubnetNode, LtGroups *pGroup)
{
	LtDomainConfiguration* pDom = (LtDomainConfiguration*) this;
    if (pDom->getDomain().inUse())
    {
		if (pDom->getSubnetNode(nIndex++, *pSubnetNode) == LT_NO_ERROR &&
			pSubnetNode->inUse())
		{
	 		*pDomain = pDom->getDomain();
			getStack()->getNetworkImage()->addressTable.getGroups(pDom->getIndex(), *pGroup);
			return true;
		}
	}
	return false;
}

boolean LtSubnetNodeClient::getAddress(int& index, LtUniqueId *pUniqueId)
{
	if (index++ == 0)
	{
		// Subnet node clients also spec a unique ID so that unique ID messages
		// on this domain vector into this client.  Only do this if the domain
		// is configured.
		LtDomainConfiguration* pDom = (LtDomainConfiguration*) this;
        if (pDom->getDomain().inUse())
        {
			*pUniqueId = *(LtUniqueId*) this;
			return true;
		}
	}
	return false;
}	

boolean LtRouterDomainClient::getRoute(boolean bEeprom, LtRoutingMap* pRoute)
{
	LtDomainConfiguration* pDc = (LtDomainConfiguration*) this;
	LtDomain* pDmn = &pDc->getDomain();
	if (pDmn->isValid())
	{
		*pRoute = getRoute(bEeprom);
		pRoute->setDomain(*pDmn);
		if (getStack()->getRouterMode() == LT_RTR_MODE_TEMP_BRIDGE)
		{
			pRoute->setRouterType(LT_BRIDGE);
		}
		else
		{
			pRoute->setRouterType(getStack()->getRouterType());
		}
		return true;
	}
	return false;
}

boolean LtRouterDomainClient::getExternalRoute(int& index, LtRoutingMap* pRoute, int* pRoutingSubnet)
{
	boolean result = false;
	if (index++ == 0)
	{
		result = getRoute(false, pRoute);
		if (result && pRoutingSubnet)
		{
			*pRoutingSubnet = getSubnet();
		}
	}
	return result;
}

LtErrorType LtRouterDomainClient::setRoute(LtRoutingMap* pRoute, boolean bEeprom)
{
	LtErrorType err = LT_NO_ERROR;
	getRoute(bEeprom) = *pRoute;
	if (bEeprom == false)
	{
		// Notify the LRE of the change.  EEPROM changes don't affect the routing
		// algorithm.
		notify();
	}
	return err;
}
