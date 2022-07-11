///
// LtNetworkManager.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtNetworkManager.cpp#6 $
//

#include "LtStackInternal.h"
#include "LonLink.h"
#include "VxLayer.h"


//
// Private Member Functions
//


int VXLCDECL LtNetworkManager::start( int nm, ... )
{
	LtNetworkManager* pNm = (LtNetworkManager*) nm;
	pNm->run();
	return 0;
}

NmErrCode LtNetworkManager::toNmErr(LtErrorType err)
{
	switch (err)
	{
		case LT_NOT_IMPLEMENTED:
			return NMERR_UNSUPPORTED;
		case LT_INVALID_DOMAIN:
		case LT_INVALID_ADDR_TABLE_INDEX:
		case LT_INVALID_NV_INDEX:
		case LT_INVALID_INDEX:
			return NMERR_NOT_FOUND;
		case LT_END_OF_ENUMERATION:
			return NMERR_ENUMERATION_END;
		case LT_INVALID_PARAMETER:
		case LT_NV_LENGTH_MISMATCH:
			return NMERR_ILLEGAL_PARMS;
		case LT_AUTHENTICATION_MISMATCH:
			return NMERR_AUTHENTICATION;
		case LT_EEPROM_WRITE_FAILURE:
			return NMERR_WRITE_FAILURE;
		case LT_OWNER_DOES_NOT_EXIST:
			return NMERR_OWNER_DOES_NOT_EXIST;
		default:
			break;
	}
	return NMERR_INTERNAL_FAILURE;
}

LtErrorType LtNetworkManager::validateRouterCommand(LtApdu& apdu, int min, int max)
{
	LtErrorType err = LT_NO_ERROR;
	if (getStack()->isNodeStack())
	{
		err = LT_INVALID_PARAMETER;
	}
	else
	{
		err = validate(apdu, min, max);
	}
	return err;
}

LtErrorType LtNetworkManager::validate(LtApdu& apdu, int min, int max) 
{
	LtErrorType err = LT_NO_ERROR;
    int dataLen = apdu.getDataLength();
	if (max == -1)
	{
		max = min;
	}
    if (dataLen < min || dataLen > max) 
	{
		err = LT_INVALID_PARAMETER;
    }
	return err;
}

int LtNetworkManager::determineNvIndex(LtApdu& apdu) 
{
	int nvIndex = apdu.getData(0);
    if (nvIndex == LT_ESCAPE_INDEX) 
	{
        nvIndex = LtMisc::makeint(apdu.getData(1), apdu.getData(2));
    }
	
	int maxLegacyNvCount = getStack()->getStaticNetworkVariableCount() + getStack()->getMaximumDynamicNetworkVariableCount();
	if (nvIndex >= maxLegacyNvCount)
	{
		// Legacy commands only see static and dynamic NVs (not monitor NVs).  So skip the
		// monitor NVs so that we access alias correctly
		nvIndex += getStack()->getMaximumMonitorNetworkVariableCount();
	}

    return nvIndex;
}

int LtNetworkManager::determineNvOffset(LtApdu& apdu) 
{
    if (apdu.getData(0) == LT_ESCAPE_INDEX) 
	{
        return 3;
    }
    return 1;
}

int LtNetworkManager::convertAddress(byte data[], int offset) 
{
    // Convert absolute reads of certain addresses as a courtesy to
    // tools which try to do absolute addressing.
    if (data[offset] == LT_ABSOLUTE && data[offset + 1] == 0xf0) 
	{
        data[offset] = LT_READ_ONLY_RELATIVE;
        data[offset + 1] = 0x00;
    }
    return LtMisc::makeint(data[offset + 1], data[offset + 2]);
}

LtErrorType LtNetworkManager::readMemory(byte data[], int& length, byte** ppResult) 
{
	LtErrorType err = LT_NO_ERROR;
    int address = convertAddress(data, 0);
    length = data[3];
    byte* result = null;

    switch (data[0]) 
	{
		case LT_READ_ONLY_RELATIVE:
			err = getStack()->getReadOnly()->toLonTalk(address, length, &result);
			break;
		case LT_CONFIG_RELATIVE:
			result = getStack()->getNetworkImage()->configData.toLonTalk(address, length);
			break;
		case LT_STATS_RELATIVE:
			err = getStack()->getNetworkStats()->toLonTalk(&result, address, length);
			break;
		case LT_ABSOLUTE:
			// Following are supported:
			//  1. One byte read of location 0 reports system version.
			//  2. App memory reads.
			//  3. Turns out LB reads 15 bytes at location 0 as a don't care
			//     command when checking authentication.  So, we'll just return
			//     0's when reading absolute memory (unless the read overlaps
			//     app mem and other mem).
			if (address == 0 && length == 1) 
			{
				result = new byte[1];
				result[0] = (byte) LT_SYSTEM_VERSION;
			}
			else if (appMemCheck(address, length)) 
			{
				err = LT_APP_MESSAGE;
			}
#if PRODUCT_IS(FTXL) || PRODUCT_IS(LONTALK_STACK) || PRODUCT_IS(IZOT)
            else
            {   // FTXL does not allow reads of any memory outside the registered area.
                err = LT_INVALID_PARAMETER;
            }
#else
			// Not fully in app mem - disallow if partially app mem.
			else if (appMemCheck(address, 1) || appMemCheck(address+length-1, 1))
			{
				err = LT_INVALID_PARAMETER;
			}
			else
			{
				result = new byte[length];
				memset(result, 0, length);
			}
#endif
			break;
		default:
		{
			err = LT_INVALID_PARAMETER;
			break;
		}
    }
	*ppResult = result;
	return err;
}

LtErrorType LtNetworkManager::processQueryRequest(boolean qualifies, int type, LtApdu& response) 
{
	LtErrorType err = LT_NO_ERROR;
    if (qualifies) 
	{
        qualifies = false;

        switch (type) 
		{
			case 0:
				qualifies = getStack()->getNetworkImage()->unconfigured();
				break;
			case 1:
				// qualifies = respondToQuery;
				if (respondToQuery)
					qualifies = 1;
				else
					qualifies = 0;
				break;
			case 2:
				qualifies = getStack()->getNetworkImage()->unconfigured() && respondToQuery;
				break;
			default:
				err = LT_INVALID_PARAMETER;
				break;
        }

        if (qualifies) 
		{
            // Fill in appropriate response.
			byte data[MAX_APDU_SIZE];
			getStack()->getReadOnly()->getUniqueId()->getData(data);
            getStack()->getReadOnly()->getProgramId()->getData(&data[LtUniqueId::getLength()]);
			response.setData(data, 0, LtUniqueId::getLength() + LtProgramId::getLength());
        }
    }
    if (err == LT_NO_ERROR && !qualifies) 
	{
        err = LT_NOT_QUALIFIED;
    }
	return err;
}

LtErrorType LtNetworkManager::processQueryId(LtApdu& apdu, LtApdu& response) 
{
	LtErrorType err = LT_NO_ERROR;
    if (apdu.getDataLength() != 1 && apdu.getDataLength() < 6) 
	{
		err = LT_INVALID_PARAMETER;
    }

	if (err == LT_NO_ERROR)
	{
		boolean qualifies = true;

		if (apdu.getDataLength() >= 5) 
		{
			// Conditional query ID.
			if (apdu.getDataLength() != 5 + apdu.getData(4)) 
			{
				err = LT_INVALID_PARAMETER;
			} 
			else 
			{
				// Proper data format.  Check for memory match
				int len;
				byte* mem;
				err = readMemory(apdu.getData() + 1, len, &mem);
				if (err == LT_NO_ERROR)
				{
					for (int i = 0; i < len; i++) 
					{
						if (apdu.getData(5 + i) != (int)mem[i]) 
						{
							qualifies = false;
							break;
						}
					}
					delete mem;
				}
			}
		}
		if (err == LT_NO_ERROR)
		{
			err = processQueryRequest(qualifies, (int)apdu.getData(0), response);
		}
	}
	return err;
}

LtErrorType LtNetworkManager::processRespondToQuery(LtApdu& apdu) 
{
    LtErrorType err = validate(apdu, 1);
	if (err == LT_NO_ERROR)
	{
		// respondToQuery = apdu.getData(0)!=0?true:false;
		if (apdu.getData(0)!=0)
			respondToQuery = true;
		else
			respondToQuery = false;
	}
	return err;
}

LtErrorType LtNetworkManager::processUpdateDomain(LtApdu& apdu) 
{
	LtErrorType err;
	err = getConfigurationEntity(NM_DOMAIN)->update(apdu.getData(0), apdu.getData()+1, apdu.getDataLength()-1);
	return err;
}

LtErrorType LtNetworkManager::processLeaveDomain(LtApdu& apdu) 
{
    LtErrorType err = validate(apdu, 1);
	if (err == LT_NO_ERROR)
	{
		LtDomainConfiguration dc;
		err = getStack()->updateDomainConfiguration(apdu.getData(0), &dc);

		if (err == LT_NO_ERROR)
		{
			getStack()->getNetworkImage()->domainTable.goUnconfiguredConditional();
		}
	}
	return err;
}

LtErrorType LtNetworkManager::processQueryDomain(LtApdu& apdu, LtApdu& response) 
{
    LtErrorType err = validate(apdu, 1);
	if (err == LT_NO_ERROR)
	{
		LtDomainConfiguration dc;
		LtErrorType err = getStack()->getDomainConfiguration(apdu.getData(0), &dc);
		if (err == LT_NO_ERROR)
		{
			byte byData[MAX_APDU_SIZE];
			int nLen = dc.toLonTalk(byData, LT_CLASSIC_DOMAIN_STYLE);
			response.setData(byData, 0, nLen);
            if (apdu.getAuthenticated())
            {
                static_cast<LtApduOut&>(response).setDownlinkEncryption();
            }
		}
	}
	return err;
}

LtErrorType LtNetworkManager::processSecurity(LtApdu& apdu) 
{
    LtErrorType err = validate(apdu, 7);
	if (err == LT_NO_ERROR)
	{
		LtDomainConfiguration dc;
		err = getStack()->getDomainConfiguration(apdu.getData(0), &dc);
		if (err == LT_NO_ERROR)
		{
			dc.updateKey(true, apdu.getData() + 1);
			err = getStack()->updateDomainConfiguration(apdu.getData(0), &dc);
		}
	}
	return err;
}

LtErrorType LtNetworkManager::processUpdateAddress(LtApdu& apdu) 
{
    LtErrorType err = validate(apdu, 5, 6);
	if (err == LT_NO_ERROR)
	{
		int index = apdu.getData(0);

		if (index >= MAX_EAT_ADDRESS_TABLE_ENTRIES)
		{	// Legacy command cannot be used to query address table entries beyond
			// max EAT address table entries.
			err = LT_INVALID_ADDR_TABLE_INDEX;
		}
		else
		{
			LtAddressConfiguration ac;
			int len;
			err = ac.fromLonTalk(apdu.getData() + 1, len, 1);
			if (err == LT_NO_ERROR)
			{
				err = getStack()->updateAddressConfiguration(index, &ac);
			}
		}
	}
	return err;
}

LtErrorType LtNetworkManager::processUpdateGroupAddress(LtApdu& apdu) 
{
    LtErrorType err = validate(apdu, 5);
	if (err == LT_NO_ERROR)
	{
		if (apdu.getOutput())
		{
			err = LT_INVALID_PARAMETER;
		}
		else
		{
			LtApduIn& apduIn = (LtApduIn&) apdu;

			if (apduIn.getAddressFormat() != LT_AF_GROUP)
			{
				err = LT_INVALID_PARAMETER;
			}
			else
			{
				LtAddressConfiguration ac;
				int len;
				err = ac.fromLonTalk(apdu.getData(), len, 1);
				if (err == LT_NO_ERROR)
				{
					// Use domain index and group that we matched on.
					ac.setDomainIndex(apduIn.getDomainConfiguration().getIndex());
					ac.setGroup(apduIn.getGroup());
					err = getStack()->getNetworkImage()->addressTable.update(ac);
				}
			}
		}
	}
	return err;
}

LtErrorType LtNetworkManager::processQueryAddress(LtApdu& apdu, LtApdu& response) 
{
    LtErrorType err = validate(apdu, 1);
	if (err == LT_NO_ERROR)
	{
		LtAddressConfiguration ac;
		int index = apdu.getData(0);

		if (index >= MAX_EAT_ADDRESS_TABLE_ENTRIES)
		{	// Legacy command cannot be used to query address table entries beyond
			// max EAT address table entries.
			err = LT_INVALID_ADDR_TABLE_INDEX;
		}
		else
		{
			err = getStack()->getAddressConfiguration(index, &ac);
			if (err == LT_NO_ERROR)
			{
				byte byData[MAX_APDU_SIZE];
				int nLen = ac.toLonTalk(byData, 1);
				response.setData(byData, 0, nLen);
			}
		}
	}
	return err;
}

LtErrorType LtNetworkManager::processUpdateNetworkVariable(LtApdu& apdu) 
{
    LtErrorType err = validate(apdu,4,9);
	if (err == LT_NO_ERROR)
	{
		int idx = determineNvIndex(apdu);
		LtNetworkVariableConfiguration nvc;

		err = getStack()->getNetworkImage()->nvTable.get(idx, &nvc);
		if (err == LT_NO_ERROR)
		{
            int nvOffset = determineNvOffset(apdu);
			nvc.fromLonTalk(apdu.getData() + nvOffset, apdu.getDataLength() - nvOffset, 1);
			err = getStack()->getNetworkImage()->nvTable.set(idx, nvc);
		}
	}
	return err;
}

LtErrorType LtNetworkManager::processExpandedUpdateNetworkVariable(LtApdu& apdu, int subcode) 
{
    LtErrorType err = validate(apdu, subcode == LT_EXP_UPDATE_NV_CONFIG ? 7 : 9);
	if (err == LT_NO_ERROR)
	{
		int idx = LtMisc::makeint(apdu.getData(1), apdu.getData(2));
		LtNetworkVariableConfiguration nvc;

        int nType = subcode == LT_EXP_UPDATE_NV_CONFIG ? NV_TABLE_NVS : NV_TABLE_ALIASES;
		err = getStack()->getNetworkImage()->nvTable.get(idx, &nvc, nType);
		if (err == LT_NO_ERROR)
		{
            int nvOffset = determineNvOffset(apdu);
			nvc.fromLonTalk(apdu.getData() + 3, apdu.getDataLength() - 3, 1, true);
			err = getStack()->getNetworkImage()->nvTable.set(idx, nvc, nType);
		}
	}
	return err;
}


LtErrorType LtNetworkManager::processQueryNetworkVariable(LtApdu& apdu, LtApdu& response) 
{
    LtErrorType err = validate(apdu, determineNvOffset(apdu));
	if (err == LT_NO_ERROR)
	{
        LtNetworkVariableConfiguration nvc;
		err = getStack()->getNetworkImage()->nvTable.get(determineNvIndex(apdu), &nvc);
		if (err == LT_NO_ERROR)
		{
			byte byData[MAX_APDU_SIZE];
			int nLen = nvc.toLonTalk(byData, true, 1);
			response.setData(byData, 0, nLen);
		}
	}
	return err;
}

LtErrorType LtNetworkManager::processExpandedQueryNetworkVariable(LtApdu& apdu, LtApdu& response, int subcode) 
{
    LtErrorType err = validate(apdu, 3);
	if (err == LT_NO_ERROR)
	{
        LtNetworkVariableConfiguration nvc;
		int idx = LtMisc::makeint(apdu.getData(1), apdu.getData(2));
		err = getStack()->getNetworkImage()->nvTable.get(idx, &nvc, 
            subcode == LT_EXP_QUERY_NV_CONFIG ? NV_TABLE_NVS : NV_TABLE_ALIASES);
		if (err == LT_NO_ERROR)
		{
			byte byData[MAX_APDU_SIZE];
			int nLen = nvc.toLonTalk(byData, true, 1, true);
            response.setData(0, subcode);
			response.setData(byData, 1, nLen);
		}
	}
	return err;
}

LtErrorType LtNetworkManager::processExtendedCommand(LtApdu& apdu, LtApdu& response, boolean &bStore)
{
	LtErrorType err = LT_APP_MESSAGE;
    byte data[MAX_APDU_SIZE];
	int respLen = 0;
	int length = apdu.getDataLength();
	byte* pData = apdu.getData();

	if (length != 0)
	{
        int cmd;
		
		PTOHB(pData, cmd);
		length--;

		err = LT_INVALID_PARAMETER;

		switch (cmd) 
		{
			case LT_APP_WINK:
			case LT_UPDATE_NV_VALUE:
				err = LT_APP_MESSAGE;
				break;

			case LT_SEND_ID_INFO:
				// Deprecated
				err = LT_INVALID_PARAMETER;
				break;

			case LT_APP_NV_DEFINE:
			case LT_APP_NV_REMOVE:
			case LT_QUERY_NV_INFO:
			case LT_QUERY_NODE_INFO:
			case LT_UPDATE_NV_INFO:
				respLen = sizeof(data);
                err = getStack()->dynamicNv(cmd, 
					pData, length,
					data, respLen);
                break;

			case LT_NM_GET_CAPABILITY_INFO:
			{
				// This command is essentially just accessing the SI data following the alias
				// entry.  
				int offset;
				int requestLen;

				PTOHS(pData, offset);
				PTOHB(pData, requestLen);
				length -= 3;

				respLen = requestLen;
				err = LT_NO_ERROR;
				if (getStack()->makeSiDataAliasRelative(data, offset, requestLen) != ND_OK)
				{
					respLen = 0;
					err = LT_INVALID_PARAMETER;
				}
				break;
			}

			default:
			{
				if (length)
				{
					int resource;
					// Must be a resource command
					PTOHB(pData, resource);
					length--;

					LtConfigurationEntity* pConfigEntity = getConfigurationEntity(resource);
					if (pConfigEntity != null)
					{
						err = pConfigEntity->checkLimits(cmd, pData, length);
					}
					if (err == LT_NO_ERROR)
					{
						int startIndex = 0;
						int endIndex = 0;

                        if (resource != NM_NODE || cmd == LT_NM_ENUMERATE)
                        {   // Get startIndex.  Node commands, other than enumerate, have no start index.
						    PTOHS(pData, startIndex);
                            length -= 2;
                        }
						switch (cmd)
						{
							case LT_NM_INITIALIZE:

                                if (resource != NM_NODE)
                                {
								    PTOHS(pData, endIndex);
                                    length -= 2;
                                }
                                {
                                    LtApduIn *pApduIn = static_cast<LtApduIn *>(&apdu);
                                    err = pConfigEntity->initialize(startIndex, endIndex, pData, length,
                                                                 pApduIn->getDomainConfiguration().getIndex());
                                }
								bStore = pConfigEntity->affectsNetworkImage();
								break;

							case LT_NM_CREATE:
							    err = pConfigEntity->create(startIndex, pData, length);
								bStore = pConfigEntity->affectsNetworkImage();
								if (bStore && err == LT_NO_ERROR)
								{
									getStack()->getNetworkImage()->setHasBeenEcsChanged(true);
								}
								break;

							case LT_NM_REMOVE:
                                if (resource != NM_NODE)
                                {
								    PTOHS(pData, endIndex);
                                }
								err = pConfigEntity->remove(startIndex, endIndex);
								bStore = pConfigEntity->affectsNetworkImage();
								break;

							case LT_NM_UPDATE:
								err = pConfigEntity->update(startIndex, pData, length);
								bStore = pConfigEntity->affectsNetworkImage();
								if (bStore && err == LT_NO_ERROR)
								{
									getStack()->getNetworkImage()->setHasBeenEcsChanged(true);
								}
								break;

							case LT_NM_ENUMERATE:
								err = pConfigEntity->enumerate(startIndex, apdu.getAuthenticated(), response);
								break;

							default:
							{
								if (cmd <= LT_NM_MAX_GEN_SUBCOMMAND)
								{
									err = LT_NOT_IMPLEMENTED;
								}
								else
								{
									// Must be resource specific.
									// Resource specific commands must do their own
									// persistence update.

									err = pConfigEntity->resourceSpecificCommand(cmd, startIndex, pData, length, apdu.getAuthenticated(), response);
								}
								break;
							}
						}
					}
				}
			}
			break;
       }
		if (respLen)
		{
			response.setData(data, 0, respLen);
        }
	}

	return err;
}

LtErrorType LtNetworkManager::processExpandedCommand(LtApdu& apdu, LtApdu& response, boolean &bStore)
{
    LtErrorType err = LT_INVALID_PARAMETER;
	int length = apdu.getDataLength();
	byte* pData = apdu.getData();

	if (length != 0)
	{
        int cmd;
		
		PTOHB(pData, cmd);
       
        response.setData(0, cmd);  // responses start with a command
		switch (cmd) 
		{
            case LT_EXP_QUERY_COMMAND_SET_VERSION:
                if (length == 1)
                {
                    int nmVersion;
                    int nmCapablities;
                    getStack()->getLayer4()->getNmVersion(nmVersion, nmCapablities);
                    if (nmVersion < 2)
                    {   
                        err = LT_NOT_IMPLEMENTED;
                    }
                    else
                    {
                        err = LT_NO_ERROR;
                        response.setData(1, nmVersion);
                        response.setData(2, nmCapablities >> 8);
                        response.setData(3, nmCapablities & 0xff);
                    }
                }
                break;
            case LT_EXP_UPDATE_NV_BY_INDEX:
                err = LT_APP_MESSAGE;
				break;

            case LT_EXP_JOIN_DOMAIN_NO_KEY:	
                if (getStack()->omaSupported() && length == (BASE_DOMAIN_STORE_SIZE + 2))
                {
                    int index;
                    LtDomainConfiguration dc;
                    int domainStructLen = length - 2;

                    PTOHB(pData, index);
	                err = dc.fromLonTalk(pData, domainStructLen, LT_KEYLESS_DOMAIN_STYLE);
	                if (err == LT_NO_ERROR)
                    {
		                LtDomainConfiguration currentDc;
                        if (getStack()->getDomainConfiguration(index, &currentDc) == LT_NO_ERROR)
                        {   // Copy key from current configuration, if any.
                            dc.setKey(currentDc.getKey());
                        }
		                err = getStack()->updateDomainConfiguration(index, &dc);
                        bStore = TRUE;
	                }
                }
                break;

            case LT_EXP_QUERY_DOMAIN_NO_KEY:
                if (getStack()->omaSupported() && length == 2)
                {
                    int index;
                    LtDomainConfiguration dc;

                    PTOHB(pData, index);
                    err = getStack()->getDomainConfiguration(index, &dc);
	                if (err == LT_NO_ERROR)
                    {
			            byte byData[MAX_APDU_SIZE];
			            response.setData(byData, 1, dc.toLonTalk(byData, LT_KEYLESS_DOMAIN_STYLE));
                    }
                }
                break;

            case LT_EXP_QUERY_OMA_KEY:			
                if (getStack()->omaSupported() && length == 1)
                {
                    LtDomainConfiguration dc;
                    
                    err = getStack()->getDomainConfiguration(0, &dc);
	                if (err == LT_NO_ERROR)
                    {
			            response.setData(dc.getKey(), 1, LT_OMA_DOMAIN_KEY_LENGTH);
                        if (apdu.getAuthenticated())
                        {
                            static_cast<LtApduOut&>(response).setDownlinkEncryption();
                        }
                    }
                }
                break;

            case LT_EXP_UPDATE_OMA_KEY:	
                if (getStack()->omaSupported() && length == (LT_OMA_DOMAIN_KEY_LENGTH + 2))
                {
	                boolean bIncrement;
                    LtDomainConfiguration dc;
                    
                    PTOHB(pData, bIncrement);
                    err = getStack()->getDomainConfiguration(0, &dc);
                    if (err == LT_NO_ERROR)
                    {   // Copy key from current configuration, if any.
 			            dc.updateKey(bIncrement, pData);
		                err = getStack()->updateDomainConfiguration(0, &dc);

                        // This command acts on both domain 0 and domain 1 (if they both
                        // exist.  Funky - should use ECS commands instead of 
                        // these expanded commands.
                        LtDomainConfiguration dc1;
                        if (getStack()->getDomainConfiguration(1, &dc1) == LT_NO_ERROR)
                        {
                            if (dc1.getUseOma())
                            {   // Keys should be the same
                                dc1.setKey(dc.getKey());
                            }
                            else
                            {   // Only the last portion of the increment applies...
                                byte increment[LT_OMA_DOMAIN_KEY_LENGTH];
                                memcpy(increment, pData+LT_CLASSIC_DOMAIN_KEY_LENGTH, LT_CLASSIC_DOMAIN_KEY_LENGTH);
                                memset(increment+LT_CLASSIC_DOMAIN_KEY_LENGTH, 0, LT_CLASSIC_DOMAIN_KEY_LENGTH);
                                dc1.updateKey(bIncrement, increment);
                            }
                            getStack()->updateDomainConfiguration(1, &dc1);                                                            
                        }
                        bStore = TRUE;
                    }
                }

                break;

            case LT_EXP_INIT_CONFIG:
                // NOTE:  The VNI does not process the auth array because NV auth config is not stored for
                // unbound NVS - the authentication comes from the NV definition.
                err = getStack()->getNetworkImage()->expInitConfig();
                break;

            case LT_EXP_UPDATE_NV_CONFIG:
            case LT_EXP_UPDATE_ALIAS_CONFIG:	
                err = processExpandedUpdateNetworkVariable(apdu, cmd);
                bStore = TRUE;
                break;

            case LT_EXP_QUERY_NV_CONFIG:	
            case LT_EXP_QUERY_ALIAS_CONFIG:	
                err = processExpandedQueryNetworkVariable(apdu, response, cmd);
                break;

            case LT_EXP_SET_LS_ADDR_MAPPING_ANNOUNCEMENTS:
                getStack()->lsAddrMappingConfigFromLonTalk(pData);
                bStore = TRUE;
                err = LT_NO_ERROR;
                break;

            case LT_EXP_QUERY_LS_ADDR_MAPPING_ANNOUNCEMENTS:
                {
                    byte respData[10];
                    getStack()->lsAddrMappingConfigToLonTalk(respData);
                    response.setData(respData, response.getDataLength(), sizeof(respData));
                }
                err = LT_NO_ERROR;
                break;
#if FEATURE_INCLUDED(IZOT)
            case LT_EXP_QUERY_IP_ADDR:
                {
                    byte ipAddr[16];
                    int ipAddrLen = getStack()->queryIpAddr((LtApduIn *)&apdu, ipAddr);
                    if (ipAddrLen != 0)
                    {
                        response.setData(ipAddr, response.getDataLength(), ipAddrLen);
                        err = LT_NO_ERROR;
                    }
                }
                break;
#endif
        }
	}
	return err;
}

LtErrorType LtNetworkManager::processNodeMode(LtApdu& apdu) 
{
    LtErrorType err = validate(apdu, 1 , 2);
	if (err == LT_NO_ERROR)
	{
		LtApduIn& apduIn = (LtMsgIn&) apdu;
		if (!getStack()->isNodeStack() &&
			apduIn.getAddressFormat() == LT_AF_BROADCAST)
		{
			// Router ignores broadcast node mode commands.  Allows for
			// broadcast offline or online without causing routers
			// to shut down (and thus not forward the message!).
			err = LT_NOT_QUALIFIED;
		}
	}
	if (err == LT_NO_ERROR)
	{
		switch (apdu.getData(0))
		{
		case LT_MODE_OFFLINE:
		case LT_MODE_ONLINE:
			err = LT_APP_MESSAGE;
			break;
		case LT_MODE_RESET_TX:
			getStack()->getLayer4()->resetTx();
			// Fall through to normal reset.
			// NO BREAK
		case LT_MODE_RESET_BUFFER:
		case LT_MODE_RESET:

		default:
			getStack()->getLayer4()->setResetRequested();
			break;
		case LT_MODE_CHANGE_STATE:
			err = validate(apdu, 2);
			if (err == LT_NO_ERROR)
			{
				err = getStack()->changeState(apdu.getData(1), false);   
			}
			break;
		}

	}

	return err;
}

LtErrorType LtNetworkManager::processChecksumRecalc(LtApdu& apdu, boolean& store) 
{
    LtErrorType err = validate(apdu, 1);
	if (err == LT_NO_ERROR)
	{
		store = (apdu.getData(0) & 4) == 4;
	}
	return err;
}

LtErrorType LtNetworkManager::processReadMemory(LtApdu& apdu, LtApdu& response) 
{
	int len;
	// For LB compatibility, allow lengths of 15.  Too much isn't a problem for reads
	// anyway.
    LtErrorType err = validate(apdu, 4, 19);
	if (err == LT_NO_ERROR)
	{
		byte* data;
	    err = readMemory(apdu.getData(), len, &data);
		if (err == LT_NO_ERROR)
		{
			response.setData(data, 0, len);
			delete data;
		}
	}
	return err;
}

LtErrorType LtNetworkManager::processWriteMemory(LtApdu& apdu) 
{
    LtErrorType err = validate(apdu,5,237);
	if (err == LT_NO_ERROR)
	{
		int length = apdu.getData(3);
		if (apdu.getDataLength() != 5+length &&
			(apdu.getDataLength() != 16 || length > 11)) 
		{
			err = LT_INVALID_PARAMETER;
		}

		if (err == LT_NO_ERROR)
		{
			int offset = convertAddress(apdu.getData(), 0);
			int flags = apdu.getData(4);
			int type = apdu.getData(0);
			boolean recomputeRequired = false;
			boolean recompute = (flags & 4) == 4;
    
			switch (type) 
			{
				case LT_READ_ONLY_RELATIVE:
					err = getStack()->getReadOnly()->fromLonTalk(offset, length, apdu.getData() + 5);
					if (err == LT_NO_ERROR)
					{
						getStack()->getReadOnly()->setPendingUpdate(TRUE);
						// Cause persistent update of network image for updates to the program ID,
						// which is stored in both the network image and the read-only data image.
						getStack()->getNetworkImage()->store();
					}
					break;
				case LT_CONFIG_RELATIVE:
					recomputeRequired = !getStack()->getNetworkImage()->configData.fromLonTalk(apdu.getData() + 5, offset, length);
					// Because some configuration changes affect the LRE (notably, router
					// mode), we update the LRE on any write.
					getStack()->routerModeChange();
		            if (!getStack()->getNetworkImage()->store(recompute))
		            {
			            err = LT_EEPROM_WRITE_FAILURE;
		            }
					break;
				case LT_STATS_RELATIVE:
					err = getStack()->getNetworkStats()->fromLonTalk(apdu.getData() + 5, offset, length);
					break;
				case LT_ABSOLUTE:
					err = LT_INVALID_PARAMETER;
					if (appMemCheck(offset, length)) 
					{
						err = LT_APP_MESSAGE;
					}
					break;
				default:
					err = LT_INVALID_PARAMETER;
					break;
			}
			if (err == LT_NO_ERROR)
			{
				if (!recompute && recomputeRequired && !getStack()->getNetworkImage()->unconfigured()) 
				{
					// Changing config relative without recomputing the checksum makes the 
					// node go unconfigured if in a configured state.
                    // Note that this is done explicitly here, rather than relying on a checksum
                    // failure during reset, because the network image persistence is not re-read 
                    // nor is the checksum re-validated during reset.
				    getStack()->changeState(LT_UNCONFIGURED, true);   
			        getStack()->errorLog(LT_CNFG_CS_ERROR);
					getStack()->getLayer4()->setResetRequested();
				}
				if ((flags & 8) == 8) 
				{
					getStack()->getLayer4()->setResetRequested();
				}
			}
		}
	}
	return err;
}

LtErrorType LtNetworkManager::processMemoryRefresh(LtApdu& apdu) 
{
    // Assume platform has no refreshable memory (or its handled by OS)
    return LT_INVALID_PARAMETER;
}

LtErrorType LtNetworkManager::processQueryStatus(LtApdu& apdu, LtApdu& response, boolean bValidate) 
{
	LtErrorType err = LT_NO_ERROR;
    if (bValidate) 
	{
        err = validate(apdu, 0);
    }

	if (err == LT_NO_ERROR)
	{
		// Return first five stats
		byte* pStats;
		err = getStack()->getNetworkStats()->toLonTalk(&pStats, 0, 10);
		if (err == LT_NO_ERROR)
		{
			response.setData(pStats, 0, 10);
			delete pStats;
			response.setData(10, (byte) getStack()->getResetCause());
			response.setData(11, (byte) getStack()->getModeAndState());
			response.setData(12, (byte) LT_VERSION);
			response.setData(13, (byte) getStack()->getErrorLog());
			response.setData(14, (byte) getStack()->getReadOnly()->getModel());
		}
	}
	return err;
}

LtErrorType LtNetworkManager::processClear(LtApdu& apdu) 
{
    LtErrorType err = validate(apdu, 0);
	if (err == LT_NO_ERROR)
	{
	    err = getStack()->LonTalkStack::clearStatus();
	}
	return err;
}

//
// fetchXvcrStatus
//
// This is used to fetch xcvr status either in the case of an incoming request or an incoming response
// specified as msgIn.  We then fill in the specified dataSink APDU at the specified offset.
//
LtErrorType LtNetworkManager::fetchXcvrStatus(LtApduIn& msgIn, LtApdu& dataSink, int offset) 
{
	LtErrorType err = LT_NO_ERROR;
    if (getStack()->isSpecialPurpose()) 
	{
   		// To properly simulate this operation, we must perform different actions depending on
		// whether the data came through the interface or not.  If it did, use the registers. 
		// Otherwise, we fake out the register values as if the transmission had been perfect.
    	// If the request is coming from a local NM, then we always go to the hardware.
		byte data[LT_NUM_REGS];
		LonLink::setPerfectXcvrReg(data, sizeof(data), msgIn.getAlternatePath() == LT_ALT_PATH);
		if (msgIn.getViaLtNet() || msgIn.getAddressFormat() == LT_AF_TURNAROUND)
		{
	        err = getStack()->fetchXcvrReg(data, 0);
	        // The request may have some of the information we need which is the SSI info.  So, if the
	        // SSI info is valid, we insert this now.
	        if (msgIn.getSsi(data[LT_PLC_REG_STATUS], data[LT_PLC_REG_PRIMARY]))
	        {
	        	// We only have two register values but fill in 3.  Only one of 4/5 is used, depending on the value of 3.
	        	data[LT_PLC_REG_SECONDARY] = data[LT_PLC_REG_PRIMARY];
	        }
		}
		dataSink.setData(data, offset, LT_NUM_REGS);
    } 
	else 
	{
		err = LT_INVALID_PARAMETER;
    }
	return err;
}

LtErrorType LtNetworkManager::processQueryXcvrStatus(LtApduIn& apdu, LtApdu& response) 
{
    LtErrorType err = validate(apdu, 0);
	if (err == LT_NO_ERROR)
	{
		err = fetchXcvrStatus(apdu, response);
	}
	return err;
}

LtErrorType LtNetworkManager::processBidirXcvrStatus(LtApduIn& apdu, LtApdu& response) 
{
    LtErrorType err = validate(apdu, 1);
	if (err == LT_NO_ERROR)
	{
		err = fetchXcvrStatus(apdu, response);
	}
	return err;
}

LtErrorType LtNetworkManager::processProxyAgent(LtApdu& apdu) 
{
	LtErrorType err = LT_NO_ERROR;
	LtApduIn& apduIn = (LtMsgIn&) apdu;

    if (apdu.getDataLength() < 6 ||
        apduIn.getDomainConfiguration().isFlexDomain()) 
	{
		err = LT_INVALID_PARAMETER;
    }

	if (err == LT_NO_ERROR)
	{
		int addressType = apdu.getData(1);
		int validLength = 6;

		if (addressType == LT_AT_UNIQUE_ID) 
		{
			validLength += LtUniqueId::getLength();
		}
		if (apdu.getDataLength() == validLength) 
		{
			LtOutgoingAddress oa;
			int len;
			err = oa.fromLonTalk(apdu.getData() + 1, len, 1);
			if (err == LT_NO_ERROR)
			{
				// Must use index from incoming message.
				oa.setDomainIndex(apduIn.getDomainConfiguration().getIndex());
				LtApduOut* pProxy = new LtApduOut(&apduIn);
				pProxy->setUpProxy();
				pProxy->setAlternatePath(apduIn.getAlternatePath());
				*(LtOutgoingAddress*) pProxy = oa;
				pProxy->setCode(LT_PROXY);
				pProxy->setData(0, apduIn.getData(0));
				pProxy->setServiceType(apduIn.getServiceType());
				getStack()->sendMessage(pProxy);
				// Clear request status so that we don't respond now.
				apdu.setServiceType(LT_ACKD);
			}
		} 
		else 
		{
			err = LT_INVALID_PARAMETER;
		}
	}
	return err;
}

LtErrorType LtNetworkManager::processProxy(LtApduIn& apdu, LtApdu& response) 
{
	LtErrorType err = LT_NO_ERROR;

    if (apdu.getDataLength() == 1) 
	{
        // Proxy target message
        switch (apdu.getData(0)) 
		{
        case 0:
            // Query unconfigured
            err = processQueryRequest(true, 0, response);
            break;
        case 1:
            // Query status
            err = processQueryStatus(apdu, response, false);
            break;
        case 2:
            // Query xcvr status
            err = fetchXcvrStatus(apdu, response);
            break;
        }
    }
    else
    {
        err = processProxyAgent(apdu);
    }
	return err;
}

//
// Protected Member Functions
//


//
// Public Member Functions
//


LtNetworkManager::LtNetworkManager() 
{
	m_freq = -1;
	m_clockFactor = 100;

    respondToQuery = false;
    processing = false;
	for (int i = 0; i < CONFIG_ENTITIES_SIZE; i++)
	{
		m_configEntities[i] = NULL;
	}
	m_queApdus = msgQCreate(10, sizeof(LtApdu*), MSG_Q_FIFO);
	m_taskId = 	vxlTaskSpawn("NetworkMgr", 
                          LT_NETWORK_MGR_TASK_PRIORITY, 0, 
                          LT_NETWORK_MGR_TASK_STACK_SIZE, start,
				 (int)this, 0,0,0,0, 0,0,0,0,0);
	registerTask(m_taskId, m_queApdus, NULL);
}

LtNetworkManager::~LtNetworkManager()
{
	m_vecDmAddrs.clear(true);
}

void LtNetworkManager::shutdown()
{
	waitForTasksToShutdown();
}

void LtNetworkManager::deliver(LtApduIn* pApdu)
{
    // Add apdu to the queue and schedule the network manager
	msgQSend(m_queApdus, (char*) &pApdu, sizeof(pApdu), WAIT_FOREVER, MSG_PRI_NORMAL);
}

boolean LtNetworkManager::isBusy() 
{
    return processing;
}

void LtNetworkManager::run() 
{
    while (!taskShutdown())
	{
        LtApduIn* pApdu;
        while (msgQReceive(m_queApdus, (char*) &pApdu, sizeof(pApdu), WAIT_FOREVER) == sizeof(pApdu))
		{
			if (taskShutdown()) break;

            // Process the apdu
			processing = true;
            if (process(*pApdu) == LT_APP_MESSAGE)
			{
		        getStack()->LtDeviceStack::receive(pApdu);
			}
			else
			{
				getStack()->release((LtMsgIn*)pApdu);
			}
			processing = false;
        }
    }
	msgQDelete(m_queApdus);
}

LtErrorType LtNetworkManager::processSetRouterMode(LtApdu& apdu)
{
	LtErrorType err = validateRouterCommand(apdu, 1);

	if (err == LT_NO_ERROR)
	{
		LtRouterMode mode = (LtRouterMode) apdu.getData(0);
		err = getStack()->setRouterMode(mode);
        // Propagate to other side if not a turnaround

		if (err == LT_NO_ERROR && !apdu.getTurnaround())
		{
			// Change to a far side escape version of this message so that
			// the other side does it too (assumes that APDU is designed in such
			// a way that data can be extended).  Note that if this is a request
			// then we get the response from the far side; this is different than
			// the Neuron but arguably better, I think.
			apdu.setCode(LT_ROUTER_ESCAPE);
			apdu.setData(0, LT_SET_ROUTER_MODE);
			apdu.setData(1, mode);
			err = LT_APP_MESSAGE;
		}
	}

	return err;
}

LtErrorType LtNetworkManager::getTableAttributes(LtApdu& apdu, LtRtrTableAttributes& att, boolean bSet)
{
	LtErrorType err = validateRouterCommand(apdu, bSet ? 9 : 1);

	if (err == LT_NO_ERROR)
	{
		byte value = apdu.getData(0);
		att.m_bGroup		= (value & 0x80) ? true : false;
		att.m_nDomainIndex	= (value & 0x40) ? 1 : 0;
		att.m_bEeprom		= (value & 0x20) ? true : false;
		att.m_nSegment		= (value & 0x03);
	}
	return err;
}

LtErrorType LtNetworkManager::setRoutingTable(LtApdu& apdu, boolean bSet, byte* pData)
{
	LtRtrTableAttributes att;
	LtErrorType err = getTableAttributes(apdu, att, bSet);

	if (err == LT_NO_ERROR)
	{
		LtRoutingMap map;
		// EEPROM option not applicable to clear and set
		err = getStack()->getRoute(att.m_nDomainIndex, true, &map);
		if (err == LT_NO_ERROR)
		{
			if (att.m_bGroup)
			{
				map.getGroups().set(pData, att.m_nSegment*LT_RTR_TABLE_SEGMENT_SIZE, LT_RTR_TABLE_SEGMENT_SIZE);
			}
			else
			{
				map.getSubnets().set(pData, att.m_nSegment*LT_RTR_TABLE_SEGMENT_SIZE, LT_RTR_TABLE_SEGMENT_SIZE);
			}
			// Write it back
			err = getStack()->setRoute(att.m_nDomainIndex, true, &map);
		}
	}

	return err;
}

LtErrorType LtNetworkManager::processClearRoutingTable(LtApdu& apdu)
{
	byte empty[LT_RTR_TABLE_SEGMENT_SIZE];
	memset(empty, 0, sizeof(empty));
	return setRoutingTable(apdu, false, empty);
}

LtErrorType LtNetworkManager::processSetRoutingTable(LtApdu& apdu)
{
	return setRoutingTable(apdu, true, apdu.getData() + 1);
}

LtErrorType LtNetworkManager::processQueryRoutingTable(LtApdu& apdu, LtApdu& response)
{
	LtRtrTableAttributes att;
	LtErrorType err = getTableAttributes(apdu, att, false);

	if (err == LT_NO_ERROR)
	{
		LtRoutingMap map;
		err = getStack()->getRoute(att.m_nDomainIndex, att.m_bEeprom, &map);
		if (err == LT_NO_ERROR)
		{
			byte bits[LT_RTR_TABLE_SEGMENT_SIZE];
			if (att.m_bGroup)
			{
				map.getGroups().get(bits, att.m_nSegment*LT_RTR_TABLE_SEGMENT_SIZE, LT_RTR_TABLE_SEGMENT_SIZE);
			}
			else
			{
				map.getSubnets().get(bits, att.m_nSegment*LT_RTR_TABLE_SEGMENT_SIZE, LT_RTR_TABLE_SEGMENT_SIZE);
			}
			response.setData(bits, 0, LT_RTR_TABLE_SEGMENT_SIZE);
		}
	}

	return err;
}

LtErrorType LtNetworkManager::processRouteMod(LtApdu& apdu, boolean bSet, boolean bGroup)
{
	LtErrorType err = validateRouterCommand(apdu, 2);

	if (err == LT_NO_ERROR)
	{
		LtRtrTableAttributes att;
        att.m_nDomainIndex = (apdu.getData(0) & 0x40) ? 1 : 0;
		att.m_bEeprom = apdu.getData(0) & 0x01;
		att.m_bGroup = bGroup;
		// Get the routing table
		LtRoutingMap map;
		err = getStack()->getRoute(att.m_nDomainIndex, att.m_bEeprom, &map);
		if (err == LT_NO_ERROR)
		{
			if (bGroup)
			{
				map.getGroups().set(apdu.getData(1), bSet);
			}
			else
			{
				map.getSubnets().set(apdu.getData(1), bSet);
			}
			err = getStack()->setRoute(att.m_nDomainIndex, att.m_bEeprom, &map);
		}
	}

	return err;
}

LtErrorType LtNetworkManager::processQueryRouterStatus(LtApdu& apdu, LtApdu& response)
{
	LtErrorType err = validateRouterCommand(apdu, 0);
	
	if (err == LT_NO_ERROR)
	{
		response.setData(0, getStack()->getRouterType());
		response.setData(1, getStack()->getRouterMode());
	}
	return err;
}

LtErrorType LtNetworkManager::processRouterEscape(LtApdu& apdu)
{
	LtErrorType err = validateRouterCommand(apdu, 1, MAX_APDU_SIZE);
	if (err == LT_NO_ERROR)
	{
		// Send up to app
		err = LT_APP_MESSAGE;
	}
	return err;
}

LtConfigurationEntity* LtNetworkManager::getConfigurationEntity(int resource)
{
	LtConfigurationEntity* p = NULL;
	if (resource < NM_NUM_RESOURCES)
	{
		p = m_configEntities[resource];
	}
	return p;
}

void LtNetworkManager::setStack(LtDeviceStack* pStack)
{
	m_pStack = pStack;

	LtNetworkImage *pNi = getStack()->getNetworkImage();

	// Set up the configuration entity pointers
	m_configEntities[NM_NODE] = getStack();
	m_configEntities[NM_DOMAIN] = &pNi->domainTable;
	m_configEntities[NM_NV_CONFIG] = &pNi->nvTable;
	m_configEntities[NM_ADDRESS] = &pNi->addressTable;
	m_configEntities[NM_ALIAS_CONFIG] = &pNi->aliasTable;
#if FEATURE_INCLUDED(MONITOR_SETS)
	m_configEntities[NM_MCSET] = getStack()->getMonitorSetTable();
	m_configEntities[NM_MCPOINT] = getStack()->getMonitorPointTable();
#endif
}

void LtNetworkManager::persistenceLost()
{
	// When persistence is lost, we initialize everything and
	// go unconfigured.
	m_configEntities[NM_NODE]->initialize(LT_UNCONFIGURED, 0, NULL, 0, FLEX_DOMAIN_INDEX);
	getStack()->getNetworkImage()->store();
}

LtErrorType LtNetworkManager::process(LtApduIn& apdu) 
{
	LtErrorType err = LT_NO_ERROR;
    boolean success = true;
    LtApdu* pResponse;
	pResponse = new LtApduOut((LtMsgIn*) &apdu);
	LtApdu& response = *pResponse;

    int code = apdu.getCode();

	int authCode = code;
    int subcode = apdu.getDataLength() ? apdu.getData(0): 0;

	if (code == LT_ROUTER_ESCAPE)
	{
		authCode = apdu.getData(0);
	}

	// Proxies and queries needn't be authenticated.
	// NM Escape auth should be confirmed by app.
	// Service pin needn't be either.
    if (authCode != LT_QUERY_ID &&
        authCode != LT_RESPOND_TO_QUERY &&
        authCode != LT_QUERY_STATUS &&
        authCode != LT_QUERY_STATUS_FLEX_DOMAIN &&
        authCode != LT_PROXY &&
		authCode != LT_SERVICE_PIN &&
		authCode != LT_NM_ESCAPE &&
		authCode != LT_BIDIR_XCVR_STATUS &&
        (authCode != LT_EXPANDED || (subcode != LT_EXP_QUERY_COMMAND_SET_VERSION)) &&

        !apdu.getAuthenticated() &&
		!getStack()->unconfigured() &&
        getStack()->getNetworkImage()->configData.getNmAuth()) 
	{
        err = LT_AUTHENTICATION_MISMATCH;                    
    }

	boolean bModifyingLegacyCommand =
         code == LT_UPDATE_DOMAIN ||
         code == LT_LEAVE_DOMAIN ||
         code == LT_SECURITY ||
         code == LT_UPDATE_ADDRESS ||
         code == LT_UPDATE_GROUP_ADDRESS ||
         code == LT_UPDATE_NETWORK_VARIABLE ||
         code == LT_CHECKSUM_RECALC ||
         (code == LT_WRITE_MEMORY && subcode == LT_CONFIG_RELATIVE) ||
         (code == LT_NODE_MODE && subcode == LT_MODE_CHANGE_STATE) ||
		 code == LT_SET_ROUTER_MODE ||
		 code == LT_CLEAR_ROUTING_TABLE ||
		 code == LT_SET_ROUTING_TABLE ||
		 code == LT_SET_GROUP_FWD ||
		 code == LT_SET_SUBNET_FWD ||
		 code == LT_CLEAR_GROUP_FWD ||
		 code == LT_CLEAR_SUBNET_FWD;

	boolean bModifyingEcsCommand = false;
	if (code == LT_ECS)
	{
		bModifyingEcsCommand = 
			 subcode == LT_APP_NV_DEFINE ||
			 subcode == LT_APP_NV_REMOVE ||
			 subcode == LT_UPDATE_NV_INFO ||
			 subcode == LT_NM_INITIALIZE ||
			 subcode == LT_NM_CREATE ||
			 subcode == LT_NM_REMOVE ||
			 subcode == LT_NM_UPDATE ||
			 subcode == LT_NM_SET ||
			 subcode == LT_NM_SET_DESC;
	}

	boolean bModifyingExpCommand = false;
	if (code == LT_EXPANDED)
	{
		bModifyingExpCommand = 
			 subcode == LT_EXP_JOIN_DOMAIN_NO_KEY ||
			 subcode == LT_EXP_UPDATE_OMA_KEY;
	}

	boolean bModifyingCommand = bModifyingEcsCommand || bModifyingLegacyCommand || bModifyingExpCommand;
    boolean store = bModifyingLegacyCommand;

	// Check for NM lock out due to persistent data loss.  This is a lame
	// attempt to sync up the node with the network manager after a reset
	// while updates were pending.  See "COMMENTARY1" below.
	// Not responding to local NM is problematic because there are no retries
    // and also it is assumed that the host will also be power cycled and thus 
    // should know to resync anyway.
    if (err == LT_NO_ERROR &&
		apdu.isRequest() &&
		!apdu.getTurnaround() &&											
		bModifyingCommand &&
		getStack()->getNetworkImage()->getBlackout())
	{
		// Return no response.
		err = LT_NOT_QUALIFIED;
	}

    // Check for EEPROM lock
    if (err == LT_NO_ERROR &&
		getStack()->getEepromLock() &&
		bModifyingCommand)
	{
        // Simulate EEPROM write failure                    
		err = LT_EEPROM_WRITE_FAILURE;
    }                    

	// ECS Lockout:
	// Check for lock-out due to non-backward compatibility.  In 
	// particular, if this node has ever been tweaked via ECS commands,
	// then we deny tweaking via legacy commands.  The reasons for this
	// are twofold:
	// a) A legacy tool can not correctly recover an ECS node.
	// b) A legacy tool can not reconfigure an ECS node (unless it has
	//    been initialized which clears the lock).
	if (err == LT_NO_ERROR &&
		getStack()->getNetworkImage()->getHasBeenEcsChanged() &&
        (code == LT_UPDATE_ADDRESS ||
         code == LT_UPDATE_GROUP_ADDRESS ||
         code == LT_UPDATE_NETWORK_VARIABLE ||
		 code == LT_QUERY_ADDRESS ||
		 code == LT_QUERY_NETWORK_VARIABLE ||
		 code == LT_UPDATE_DOMAIN ||
		 code == LT_LEAVE_DOMAIN ||
		 code == LT_QUERY_DOMAIN))
	{
		err = LT_INVALID_PARAMETER;
	}

	// MIP filter.  Certain network management commands are processed by the application
	// when the node operates as a MIP.  MIPs are assumed not to support ECS capability.  
	// If they did, then we would need to figure out a way to split up the ECS capability 
	// between the L5 MIP (domains and address tables) vs the host (NVs).  
	if (err == LT_NO_ERROR && getStack()->isMip())
	{
		if (code == LT_UPDATE_NETWORK_VARIABLE ||
			code == LT_QUERY_NETWORK_VARIABLE ||
			code == LT_FETCH_NETWORK_VARIABLE ||
			code == LT_ECS)
		{
			err = LT_APP_MESSAGE;
		}
	}

	if (err == LT_NO_ERROR)
	{
		switch (code) {
        case LT_EXPANDED:
			err = processExpandedCommand(apdu, response, store);
			break;

		case LT_QUERY_ID:
			err = processQueryId(apdu, response);
			break;
		case LT_RESPOND_TO_QUERY:
			err = processRespondToQuery(apdu);
			break;

		case LT_UPDATE_DOMAIN:
			err = processUpdateDomain(apdu);
			break;
		case LT_LEAVE_DOMAIN:
			err = processLeaveDomain(apdu);
			break;
		case LT_QUERY_DOMAIN:
			err = processQueryDomain(apdu, response);
			break;
		case LT_SECURITY:
			err = processSecurity(apdu);
			break;

		case LT_UPDATE_ADDRESS:
			err = processUpdateAddress(apdu);
			break;
		case LT_UPDATE_GROUP_ADDRESS:
			err = processUpdateGroupAddress(apdu);
			break;
		case LT_QUERY_ADDRESS:
			err = processQueryAddress(apdu, response);
			break;

		case LT_UPDATE_NETWORK_VARIABLE:
			err = processUpdateNetworkVariable(apdu);
			break;
		case LT_QUERY_NETWORK_VARIABLE:
			err = processQueryNetworkVariable(apdu, response);
			break;
		case LT_FETCH_NETWORK_VARIABLE:
			err = validate(apdu, 1, 3);
			if (err == LT_NO_ERROR)
			{
				if (apdu.getData(0) == LT_ESCAPE_INDEX) 
				{
					err = validate(apdu, 3);
				}
				if (err == LT_NO_ERROR)
				{
					err = LT_APP_MESSAGE;
				}
			}
			break;

		case LT_NODE_MODE:
			err = processNodeMode(apdu);
			break;
		case LT_ECS:
			err = processExtendedCommand(apdu, response, store);
			break;
		case LT_CHECKSUM_RECALC:
			err = processChecksumRecalc(apdu, store);
			break;
		case LT_QUERY_SI_DATA:
			err = validate(apdu, 3);
			if (err == LT_NO_ERROR)
			{
				err = LT_APP_MESSAGE;
			}
			break;

		case LT_READ_MEMORY:
			err = processReadMemory(apdu, response);
			break;
		case LT_WRITE_MEMORY:
            store = FALSE;  // Let processWriteMemory do the store, so it can 
                            // control whether checksum is recalculated or not.
			err = processWriteMemory(apdu);
			break;
		case LT_MEMORY_REFRESH:
			err = processMemoryRefresh(apdu);
			break;

		case LT_QUERY_STATUS:
			err = processQueryStatus(apdu, response, true);
			break;

        case LT_CLEAR_STATUS:
			err = processClear(apdu);
			break;

		case LT_PROXY:
			err = processProxy(apdu, response);
			break;
		case LT_QUERY_XCVR_STATUS:
			err = processQueryXcvrStatus(apdu, response);
			break;

		case LT_BIDIR_XCVR_STATUS:
			err = processBidirXcvrStatus(apdu, response);
			break;

        case LT_QUERY_STATUS_FLEX_DOMAIN:
            err = processQueryStatus(apdu, response, TRUE);
            response.setRespondOnFlexDomain(true);
            break;

		case LT_SET_ROUTER_MODE:
			err = processSetRouterMode(apdu);
			store = true;
			break;
		case LT_CLEAR_ROUTING_TABLE:
			err = processClearRoutingTable(apdu);
			break;
		case LT_SET_ROUTING_TABLE:
			err = processSetRoutingTable(apdu);
			store = true;
			break;
		case LT_SET_GROUP_FWD:
			err = processRouteMod(apdu, true, true);
			store = true;
			break;
		case LT_SET_SUBNET_FWD:
			err = processRouteMod(apdu, true, false);
			store = true;
			break;
		case LT_CLEAR_GROUP_FWD:
			err = processRouteMod(apdu, false, true);
			store = true;
			break;
		case LT_CLEAR_SUBNET_FWD:
			err = processRouteMod(apdu, false, false);
			store = true;
			break;
		case LT_QUERY_ROUTING_TABLE:
			err = processQueryRoutingTable(apdu, response);
			break;
		case LT_QUERY_ROUTER_STATUS:
			err = processQueryRouterStatus(apdu, response);
			break;
		case LT_ROUTER_ESCAPE:
			err = processRouterEscape(apdu);
			break;
		case LT_SERVICE_PIN:
		case LT_NM_ESCAPE:
			err = LT_APP_MESSAGE;
			break;
		case LT_COMPUTE_PHASE:
			err = processComputePhase(apdu, response);
			break;
		default:
			err = LT_INVALID_PARAMETER;
			break;
		}
    }

	if (store && err == LT_NO_ERROR) 
	{
		if (!getStack()->getNetworkImage()->store())
		{
			err = LT_EEPROM_WRITE_FAILURE;
		}
	}

	if (err != LT_NO_ERROR)
	{
		success = false;
        getStack()->errorLog(err);
	}

    if (err != LT_APP_MESSAGE) 
	{
		if (apdu.isRequest())
		{
			response.setCode(apdu.getNmCode(success));
		    if (authCode == LT_ECS && err != LT_NO_ERROR)
		    {
			    response.setData(0, toNmErr(err));
		    }
            else if (authCode == LT_EXPANDED && err != LT_NO_ERROR)
		    {   // Expanded commands always include the sub-command after the 
                // the response code, even on failure.  
                if (code == LT_ROUTER_ESCAPE && apdu.getDataLength() > 1)
                {
                    response.setData(0, apdu.getData(1));
                }
                else if (apdu.getDataLength() != 0)
                {
                    response.setData(0, apdu.getData(0));
                }

		    }
			if (err == LT_NOT_QUALIFIED) 
			{
				response.setNullResponse(true);
			}
            
			getStack()->getLayer4()->send((LtApduOut*) pResponse, true /* wait */, false /* dont throttle */);

			// Response deleted by stack (or by app for local NM).
			pResponse = null;
		}
    } 
	delete pResponse;
	return err;
}

void LtNetworkManager::setMem(int addr, int size) 
{
    LtDmAddress* pDmAddr = new LtDmAddress(addr, size);
    m_vecDmAddrs.addElement(pDmAddr);
}

boolean LtNetworkManager::appMemCheck(int addr, int size) 
{
	LtVectorPos pos;
	LtDmAddress* pDma;
    // Check to see if the given address/size falls into the
    // application's direct memory addresses.
    while (m_vecDmAddrs.getElement(pos, &pDma))
	{
        if (pDma->inRange(addr, size)) 
		{
            return true;
        }
    }
    return false;
}

LtDmAddress::LtDmAddress(int addr, int size) 
{
    this->addr = addr;
    this->size = size;
}

boolean LtDmAddress::inRange(int addr, int size) 
{
    return (addr >= this->addr &&
            addr + size <= this->addr + this->size);
}

/*
 * The following is contents of mail sent by Glen on 5/3/99.  See "COMMENTARY1" above.
 * BEGINNING OF COMMENTARY1:

LonWorks network management tools generally assume that once a network image modification 
is issued to a LonTalk node and it responds in the affirmative, then the change has been 
permanently made.  In the case of the PNC complying with this assumption is problematic.  
The PNC does have NVRAM but not enough to store worst case network image sizes.  So, 
network image changes must be stored in flash.  It is not practical to synchronously 
write to flash on every network image update so the changes must be buffered.  This 
violates the above assumption.  

Bob Walker considers this problem in the VNI paper and concludes that a new LonTalk 
commit protocol is needed to address this.  This is great but doesn't help the PNC 
which must work with existing tools.  I think a backward compatible solution goes like this:

1. When an update is made, record the fact that an update is pending in NVRAM.  Remove 
the flag when all pending updates are written to flash.
2. On a reset, if an update is pending, assume the update was lost and go unconfigured. 
(LNS generally has nodes in the hard-offline state during updates and the LMAPI leaves 
nodes configured during updates)
3. Upon going unconfigured, fail to respond to the next update so that the tool detects 
a comm error (returning an NM failure is problematic since LNS will retry the update with 
perhaps multiple authentication keys; returning a NM failure with the wrong NM code would 
work best with LNS but seems kludgy).  This step is necessary because a tool might not 
detect a device going unexpectedly unconfigured in the middle of an update (LNS doesn't).  
Also, there could be a tool which puts the node unconfigured for updates and thus couldn't 
detected step 2 at all.  This scheme won't work for a tool which does application layer 
retries of updates on a comm error.

The above assumes that the combination of the node going unexpectedly unconfigured and 
getting a timeout will cause the tool (or the user) to restart the update procedure.   
Note it will be impossible to commission a PNC in an environment with flaky power, something 
that is possible with a Neuron based device.

The current LTAPI performs steps 1 & 2 but not step 3.  I plan to add step 3.  Note that 
if the time to reboot the device is sufficiently long that a comm error would have occurred 
prior to step 3 or the reboot happened after the update completed, then the commission must 
be re-attempted twice before it will succeed.

Another complication in this whole area is that of changes to application persistent LonTalk 
data.  This includes configuration property changes, SI data changes and dynamic NV changes.  
The application will want to implement an algorithm similar to the above.  There is already 
a function that the application can call to force the node unconfigured.  However, it would 
be nice to have a variant of this that has the specific connotation of going unconfigured 
due to a loss of pending data so that the stack can do step 3 only when necessary.  Thus, 
I will add a function called lostPersistentData() that will cause steps 2 and 3.

Note that I believe there are many host nodes out there that probably don't address this at 
all (including LNS clients).  Reboots during updates are rare and when they occur, the user 
may just guess he needs to re-commission the device (or might re-commission after seeing 
improper operation).

Finally, we should strive to design the new commit protocol in a way that will allow the 
step 3 behavior to be conditionally turned off.

  END OF COMMENTARY1 */
