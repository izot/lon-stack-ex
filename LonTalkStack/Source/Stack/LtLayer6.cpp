//
// LtLayer6.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtLayer6.cpp#3 $
//

#include "LtStackInternal.h"

void LtLayer6::nvFailure(LtApduIn* pApdu) 
{
    if (pApdu->isRequest()) 
    {
        getStack()->getLayer4()->send(getStack()->nvResponse(pApdu, null, 0));
    }
}

LtErrorType LtLayer6::privateNvFetchResponse(LtApduIn* pApdu, boolean &valid)
{
	LtErrorType err = LT_NO_ERROR;

	valid = pApdu->getNmPass(LT_FETCH_NETWORK_VARIABLE) &&
			pApdu->getLength() > 2;
	
	if (valid)
	{
		int offset = (pApdu->getData(0) == LT_ESCAPE_INDEX) ? 3 : 1;

		pApdu->setNvDataOffset(offset);

		if (pApdu->getDataLength() <= offset)
		{
			valid = false;
		}
	}
	return err;
}

boolean LtLayer6::sourceAddressMatch(LtNetworkVariableConfiguration* pNvc, LtApduIn* pApdu)
{
	int addressIndex = pNvc->getAddressTableIndex();
	LtAddressConfiguration ac;
	LtAddressConfiguration* pAc = null;
	boolean bMatch = false;

	if (addressIndex == -1)
	{
		// Private NVs have address stored inside it.
		pAc = pNvc->getAddress();				
	}
	
	if (pAc == null)
	{
		if (getStack()->getAddressConfiguration(addressIndex, &ac) == LT_NO_ERROR)
		{
			pAc = &ac;
		}
	}
	if (pAc != null)
	{
		switch (pAc->getAddressType())
		{
			case LT_AT_TURNAROUND_ONLY:
				bMatch = pApdu->getAddressFormat() == LT_AF_TURNAROUND;
				break;
			case LT_AT_SUBNET_NODE:
				bMatch = pAc->getDestId() == pApdu->getDomainConfiguration().getNode() &&
						 pAc->getSubnet() == pApdu->getDomainConfiguration().getSubnet();
				break;
			case LT_AT_GROUP:
				bMatch = pApdu->getAddressFormat() == LT_AF_GROUP &&
						 pAc->getDestId() == pApdu->getGroup();
				break;
			case LT_AT_BROADCAST:
				bMatch = pAc->getSubnet() == 0 ||
						 pAc->getSubnet() == pApdu->getDomainConfiguration().getSubnet();
				break;
		}
	}
	if (bMatch)
	{
		// Domain indices can be compared since a node can only be in a given domain once.
		bMatch = pAc->getDomainIndex() == pApdu->getDomainConfiguration().getIndex();
	}

	return bMatch;
}

LtErrorType LtLayer6::incomingNetworkVariable(LtApduIn* pApdu, boolean &valid) 
{
    LtErrorType err = LT_NO_ERROR;
	boolean bFindOthers = true;
	boolean bRequest = pApdu->isRequest();
	valid = bRequest;

	// NV responses inherit the NV index of the originating request.  
	// This is handled by layer4.  

    // Set [additional] NV indices in the APDU based on the selector look-up.  
	// Additional selection is not done if the initiator has no response
	// selection.
	int nvIndex = pApdu->getNvIndex();

	int selector;
	boolean bOutput;

	err = pApdu->getSelector(selector, bOutput);

	if (nvIndex != -1)
	{
        LtNetworkVariableConfiguration nvc;
        err = getStack()->getNetworkImage()->nvTable.getNv(nvIndex, &nvc);
		if (err == LT_NO_ERROR)
		{
            if (!nvc.incarnationMatches(pApdu->getNvIncarnationNumber()))
            {
                err = LT_INVALID_NV_INDEX;
            }
            else
            {
			    bFindOthers = nvc.getNvResponseSelection() != LT_SELECTION_NEVER;
			    if (!pApdu->isNetworkVariableMsg())
			    {
				    selector = nvc.getSelector();
				    bOutput = nvc.getOutput();
			    }
            }
		}
	}

	if (err == LT_NO_ERROR)
	{
		int index = -1;
		boolean bGenFailure = false;

        // Have to lock while searching.  Otherwise another thread could start rebuilding the
        // selector table while we are in the process of searching it. Have to lock outside
        // of the the "get", since indicies are not guaranteed over rehashing.
        getStack()->getNetworkImage()->nvTable.lock();
		while (bFindOthers)
		{ 
            LtNetworkVariableConfiguration nvc;
			if (getStack()->getNetworkImage()->nvTable.get(index, selector, bOutput, nvc) == LT_NO_ERROR)
			{
				// Validate authentication
				if (!pApdu->getResponse() && nvc.getAuthenticated() && !pApdu->getAuthenticated()) 
				{
					bGenFailure = true;
					err = LT_AUTHENTICATION_MISMATCH;
				}
				// Validate direction.  Updates for outputs are allowed if the
				// update is a poll response.
				else if (nvc.getOutput() && !valid && !pApdu->getResponse()) 
				{
					err = LT_NV_UPDATE_ON_OUTPUT_NV;
				}
				else 
				{
					boolean bAdd = false;
					int selection;
					if (pApdu->getResponse())
					{
						selection = nvc.getNvResponseSelection();
					}
					else if (pApdu->isRequest())
					{
						selection = nvc.getNvRequestSelection();
					}
					else
					{
						selection = nvc.getNvUpdateSelection();
					}
					switch (selection)
					{
						case LT_SELECTION_UNCONDITIONAL:
							bAdd = true;
							break;
						case LT_SELECTION_BYSOURCE:
							bAdd = sourceAddressMatch(&nvc, pApdu);
							break;
					}
					if (bAdd)
					{
                        int incarnation;
                        if (nvc.isAlias())
                        {
                            LtNetworkVariableConfiguration primaryNvc;
                            getStack()->getNetworkImage()->nvTable.get(nvc.getPrimary(), &primaryNvc);
                            incarnation = primaryNvc.getIncarnationNumber();
                        }
                        else
                        {
                            incarnation = nvc.getIncarnationNumber();
                        }
						pApdu->addNvIndex(nvc.getPrimary(), incarnation);
						if (pApdu->getServiceType() == LT_REQUEST)
						{
							// Only need first target for request
							break;
						}
					}
				}
			} 
			else 
			{
				if (pApdu->getNvIndex() == -1)
				{
					// No match. 
					bGenFailure = true;		
					err = LT_INVALID_PARAMETER;
				}
				break;
			}
			if (err != LT_NO_ERROR)
			{
				getStack()->errorLog(err);
				err = LT_NO_ERROR;
			}
		}
        getStack()->getNetworkImage()->nvTable.unlock(); 
		if (bGenFailure)
		{
			nvFailure(pApdu);
		}
	}
	valid = valid || pApdu->getDataLength() > 1;
	return err;
}

LtErrorType LtLayer6::receive(LtApduIn* pApdu, LtApduOut* pApduOut) 
{
	LtErrorType err = LT_NO_ERROR;
    boolean valid = true;
    
	if (pApdu->getResponse() && pApduOut)
	{
		pApdu->setNvIndex(pApduOut->getNvIndex());
        pApdu->setNvIncarnationNumber(pApduOut->getNvIncarnationNumber());
	}

    if (!pApdu->valid()) 
    {
		err = LT_INVALID_PARAMETER;
    }
    else if (pApdu->getAddressFormat() == LT_AF_BROADCAST &&
             pApdu->getDomainConfiguration().isFlexDomain() &&
            !getStack()->unconfigured() &&
            pApdu->getCode() != LT_SERVICE_PIN && 
            pApdu->getCode() != LT_NM_ESCAPE &&
			!getReceiveAllBroadcasts())
    {   // Normally, configured stacks implementing > layer 2 do not receive broadcasts on the
        // flex domain. However, If LtDeviceStack::setReceiveNsaBroadcasts() has been called, 
        // the stack will receive all broadcasts, to allow receipt of the service pin and NSA messages
        // on any domain.  
		// Additionally, if LtDeviceStack::setReceiveAllBroadcasts() has been called, really
		// let all broadcasts through (used by the iLON to get ISI msgs).
		// Drop all other message types at this point.
        valid = FALSE;
		delete pApdu;
    }
    else if (pApdu->getProxyRelay()) 
    {
        // Proxy response.  Relay back out.
		assert((void*) pApdu == (void*) ((LtApduOut*) ((LtApdu*) pApdu)));
		if (pApduOut != null)
	    {
		    pApduOut->setAnyValidResponse(true);
		}
		getStack()->sendMessage((LtApduOut*) ((LtApdu*) pApdu));
    } 
    else if (pApdu->forNetworkManager()) 
    {
		if (pApdu->getResponse())
		{
			err = LT_INVALID_PARAMETER;
		}
		else 
        {
	        if (pApduOut != null)
	        {
		        pApduOut->setAnyValidResponse(true);
	        }

            if (pApdu->isNetworkDiagnostic() && !getStack()->isMsgHooked(*pApdu))
		    {
			    // These are handled by the network management thread.  This
			    // allows the app to do synchronous diagnostics with no direct
			    // callback mode (app also processes events).
			    getStack()->getNetworkManager()->deliver(pApdu);
		    }
		    else
		    {
			    // Some NM commands have callbacks which must occur in the 
			    // app thread if not direct callback mode.  This is done by
			    // having the app thread handle the network management.
		        getStack()->receive(pApdu);
		    }
        }
    } 
    else 
    {
		// If response to non-NV message, don't do NV processing.  Also, if this is a MIP, skip
		// NV processing (currently L6 MIPs are not supported).
		if (!(pApdu->getResponse() && pApduOut && !pApduOut->isNetworkVariable()) &&
			!getStack()->isMip())
		{
	        if (pApdu->isNetworkVariableMsg())
			{
				err = incomingNetworkVariable(pApdu, valid);
			}
			else if (pApdu->isNetworkVariable() && pApdu->getResponse())
			{
				// Private NV response.  Treat as NV update.
				err = privateNvFetchResponse(pApdu, valid);
				if (err == LT_NO_ERROR)
				{
					err = incomingNetworkVariable(pApdu, valid);
				}
			}
		}
		if (err == LT_NO_ERROR)
		{
	        if (valid) 
			{
	            if (pApduOut != null)
	            {
		            pApduOut->setAnyValidResponse(true);
	            }
			    getStack()->receive(pApdu);
			}
			else
			{
				delete pApdu;
			}
		}
    }
	if (err != LT_NO_ERROR)
	{
		delete pApdu;
	}
    return err;
}

void LtLayer6::completionEvent(LtApduOut* pApdu, boolean success) 
{
    // Generate a completion event based on the success flag
    boolean deliver = false;
    if (!pApdu->getNoCompletion() && !pApdu->getResponse())
    {
        deliver = true;
        if (pApdu != null && !success) 
        {
            pApdu->setAnyTxFailure(true);
        }
        if (pApdu != null && pApdu->getNvIndex() != -1) 
        {
            // Check if any aliases need to go out.  
            LtNetworkVariableConfiguration* pNvc = getStack()->getNetworkImage()->nvTable.getNext(pApdu->getNvIndex(), pApdu->getAliasIndex());
            if (pNvc != null) 
            {
                deliver = false;
                pApdu->reinit();
				pApdu->setAliasNvc(pNvc);
				pApdu->setAddressType(LT_AT_UNBOUND);
                getStack()->sendMessage(pApdu);
				pApdu = null;
            }
        }
    }
    if (!deliver) 
    {
        delete pApdu;
    }
    else
    {
        if (success && 
            ((!pApdu->getAnyTxFailure() &&
             (!pApdu->isRequest() ||
              pApdu->getNvIndex() == -1 ||
              pApdu->getAnyValidResponse())))) 
        {
            pApdu->setSuccess(true);
            pApdu->setFailure(false);
        }
        else 
        {
            pApdu->setFailure(true);
            pApdu->setSuccess(false);
        }
		assert((void*) pApdu == (void*) ((LtApduIn*) ((LtApdu*) pApdu)));
		boolean bLtep = pApdu->getProxyCount() != -1;
		if (bLtep)
		{
			getStack()->processLtepCompletion(pApdu);
		}
		else
		{
			getStack()->receive((LtApduIn*) ((LtApdu*) pApdu));
		}
    }
}

void LtLayer6::send(LtApduOut* pApdu, boolean wait, boolean throttle)
{
	LtErrorType err = LT_NO_ERROR;
    if (!pApdu->getResponse() && pApdu->isNetworkVariable()) 
    {
        // Convert index to selector    
        int nvIndex = pApdu->getNvIndex();
		LtNetworkVariableConfiguration* pNvc = pApdu->getAliasNvc();
		if (pNvc == null)
		{
            pApdu->setNvIncarnationNumber(0);   // If there is no configuration, use default incarnation number
                                                // If there is valid configuration, the incarnation number
                                                // will be set below.
			err = getStack()->getNetworkImage()->nvTable.getNv(nvIndex, &pNvc);
            if (err == LT_NOT_FOUND)
            {   // If the configuration doesn't exist, treat it as if it has no address
                err = LT_INVALID_PARAMETER;
            }
		}
        if (err == LT_NO_ERROR)
        {
            int incarnation;
            if (pNvc->isAlias())
            {
                LtNetworkVariableConfiguration primaryNvc;
                getStack()->getNetworkImage()->nvTable.get(pNvc->getPrimary(), &primaryNvc);
                incarnation = primaryNvc.getIncarnationNumber();
            }
            else
            {
                incarnation = pNvc->getIncarnationNumber();
            }
            pApdu->setNvIncarnationNumber(incarnation);
			if (!pNvc->hasOutputAddress()) 
			{
				// Terminate construction process.  
				err = LT_INVALID_PARAMETER;
			}
			else
			{
				boolean bRequest = pApdu->isRequest();
				boolean bMsgStyle = false;
				int addressType = LT_AT_SUBNET_NODE;
                
				pApdu->setAddressIndex(pNvc->getAddressTableIndex());
				bMsgStyle = bRequest ? pNvc->getReadByIndex() : pNvc->getWriteByIndex();
				if (pApdu->getAddressIndex() == LT_EXPLICIT_ADDRESS) 
				{
					// NV has explicit address attached (private NV)
					*(LtOutgoingAddress*) pApdu = *pNvc->getAddress();
					pApdu->applyOverride(false);
					addressType = pApdu->getAddressType();
				}
				else if (bMsgStyle)
				{
					// Get address type so we can decide to prevent index
					// based messaging for certain addressing types.
					LtAddressConfiguration ac;
					getStack()->getAddressConfiguration(pApdu->getAddressIndex(), &ac);
					addressType = ac.getAddressType();
				}

				if (bMsgStyle && 
					(addressType == LT_AT_GROUP ||
					 addressType == LT_AT_BROADCAST ||
					 addressType == LT_AT_BROADCAST_GROUP))
				{
					// Can't use update by index in multicast addressing modes.
					bMsgStyle = false;
				}

				pApdu->setPriority(pNvc->getPriority());
				pApdu->setAuthenticated(bMsgStyle ? pNvc->getRemoteNmAuth() : pNvc->getAuthenticated());
				pApdu->setServiceType(pNvc->getServiceType());
				pApdu->setTurnaround(pNvc->getTurnaround());
				pApdu->applyOverride(true);
				if (bRequest)
				{
					// Regardless of configured service type or 
					// override, use a request
					pApdu->setServiceType(LT_REQUEST);
				}
				LtRefId refId(LT_REF_NV, 0, pApdu->getNvIndex());
				pApdu->setRefId(refId);

				if (bMsgStyle)
				{
					if (bRequest)
					{
						// Create NV fetch.
						pApdu->setCode(LT_FETCH_NETWORK_VARIABLE);
						if (pNvc->getNvIndex() >= LT_ESCAPE_INDEX)
						{
							pApdu->setData(0, LT_ESCAPE_INDEX);
							pApdu->setData(1, ((unsigned short)pNvc->getNvIndex()) >> 8);
							pApdu->setData(2, (byte) pNvc->getNvIndex());
						}
						else
						{
							pApdu->setData(0, pNvc->getNvIndex());
						}
					}
					else
					{
						// Create an NV write
						pApdu->setCode(LT_ECS);
						pApdu->setData(0, LT_UPDATE_NV_VALUE);
						byte nvData[MAX_APDU_SIZE];
                        int nvLength = pApdu->getDataLength()-1;
						pApdu->getData(nvData, 1, nvLength);
						pApdu->setData(1, (byte) (pNvc->getNvIndex()>>8));
						pApdu->setData(2, (byte) pNvc->getNvIndex());
						pApdu->setData(nvData, 3, nvLength);
					}
				}
				else
				{
					int selector = pNvc->getSelector();
					int code = (byte) (selector >> 8) | 0x80;
					if (!pNvc->getOutput()) 
					{
						code |= 0x40;
					}
					pApdu->setData(0, (byte) selector);
					pApdu->setCode(code);
				}
			}
        }
    }
	if (err == LT_NO_ERROR)
	{
		if (pApdu->getAddressType() == LT_AT_LOCAL && !pApdu->getResponse())
		{
			// Future Enhancement: Force retry and tx timer to big numbers for a local operation
		}
		if (pApdu->isBound())
		{
			// Convert bound address to use explicit address flag.
			pApdu->setAddressIndex(LT_EXPLICIT_ADDRESS);
		}
		// Need to fix up explicit addresses.
		if (pApdu->getAddressIndex() == LT_EXPLICIT_ADDRESS && !pApdu->getResponse())
		{
			getStack()->getNetworkImage()->addressTable.fixup(*(LtOutgoingAddress*) pApdu);
		}

		getStack()->getLayer4()->send(pApdu, wait, throttle);
	}
	else
	{
		getStack()->completionEvent(pApdu, err == LT_INVALID_PARAMETER);
	}
}

