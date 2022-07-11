//
// DynamicNvs.cpp
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
// This file contains class implementations for LonWorks dynamic NV message processing.
// See nodedef.h for more information.
//

//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/DynamicNvs.cpp#2 $
//

#include <LtStackInternal.h>
#include <nodedef.h>
#include <DynamicNvs.h>
#include "vxlTarget.h"

int DynamicNvs::defaultsToFlags(int defaults)
{
	int flags = 0;
	switch (defaults & 3)
	{
		case 0:
			flags |= NV_SD_ACKD;
			break;
		case 1:
			flags |= NV_SD_UNACKD_RPT;
			break;
		default:
			flags |= NV_SD_UNACKD;
			break;
	}
	if (defaults & 4)
	{
		flags |= NV_SD_PRIORITY;
	}
	if (defaults & 8)
	{
		flags |= NV_SD_AUTHENTICATED;
	}
	if (defaults & 16)
	{
		flags |= NV_SD_OUTPUT;
	}
	return flags;
}

int DynamicNvs::flagsToDefaults(int flags)
{
	int defaults = 0;
	if (flags & NV_SD_UNACKD_RPT)
	{
		defaults = 1;
	}
	else if (flags & NV_SD_UNACKD)
	{
		defaults = 2;
	}
	if (flags & NV_SD_PRIORITY)
	{
		defaults |= 4;
	}
	if (flags & NV_SD_AUTHENTICATED)
	{
		defaults |= 8;
	}
	if (flags & NV_SD_OUTPUT)
	{
		defaults |= 16;
	}
	return defaults;
}

int DynamicNvs::attributesToFlags(int attributes)
{
	// Dynamic NVs are assumed to always have changeable type and length.
	int flags = NV_SD_CHANGEABLE | NV_SD_CHANGEABLE_LENGTH;
	if (attributes & 1)  
	{
		flags |= NV_SD_CONFIG_CLASS;
	}
	if (attributes & 2)
	{
		flags |= NV_SD_AUTH_CONFIG;
	}
	if (attributes & 4)
	{
		flags |= NV_SD_PRIORITY_CONFIG;
	}
	if (attributes & 8)
	{
		flags |= NV_SD_SERVICE_CONFIG;
	}
	if (attributes & 16)
	{
		flags |= NV_SD_OFFLINE;
	}
	if (attributes & 32)
	{
		flags |= NV_SD_POLLED;
	}
	if (attributes & 64)
	{
		flags |= NV_SD_SYNC;
	}
	return flags;

}

int DynamicNvs::flagsToAttributes(int flags)
{
	int attributes = 0;
	if (flags & NV_SD_CONFIG_CLASS) 
	{
		attributes |= 1;
	}
	if (!(flags & NV_SD_AUTH_NONCONFIG))
	{
		attributes |= 2;
	}
	if (!(flags & NV_SD_PRIORITY_NONCONFIG))
	{
		attributes |= 4;
	}
	if (!(flags & NV_SD_SERVICE_NONCONFIG))
	{
		attributes |= 8;
	}
	if (flags & NV_SD_OFFLINE)
	{
		attributes |= 16;
	}
	if (flags & NV_SD_POLLED)
	{
		attributes |= 32;
	}
	if (flags & NV_SD_SYNC)
	{
		attributes |= 64;
	}
	return attributes;
}

int DynamicNvs::flagsToExten(int flags)
{
	int exten = 0;

	if (flags & NV_SD_MAX_RATE)
	{
		exten |= 0x80;
	}
	if (flags & NV_SD_RATE)
	{
		exten |= 0x40;
	}
	if (flags & NV_SD_NAME)
	{
		exten |= 0x28;
	}
	if (flags & NV_SD_DESCRIPTION)
	{
		exten |= 0x10;
	}
	return exten;
}

NdErrorType DynamicNvs::validateDynamicNvIndex(int nvIndex, int &count)
{
	NdErrorType err = ND_OK;
	int statics = m_pNodeDef->getStaticNetworkVariableCount();
	
	if (nvIndex < statics)
	{
		err = ND_VALUE_OUT_OF_RANGE;
	}
	else if (count == LT_ALL_ECS_INDEX)
	{
		count = m_pNodeDef->getTotalNvCapacity() - nvIndex + 1;
	}
	else if (nvIndex + count - 1 > m_pNodeDef->getTotalNvCapacity())
	{
		err = ND_VALUE_OUT_OF_RANGE;
	}

	return err;
}

NdErrorType DynamicNvs::define(unsigned char* pMsg, int len)
{	
	// This routine parses a dynamic NV define message.
	NdErrorType err = ND_OK;

	lock();

	if (len < 7)
	{
		err = ND_INSUFFICIENT_DATA;
	}
	else
	{
		// Extract flags
		int nvIndex;
		int arrayLen;
		int nvLen;
		int defaults;
		int attributes;
		byte *pNativeNvType;
        boolean legacyType = true;

		PTOHS(pMsg, nvIndex);
		PTOHS(pMsg, arrayLen);
		PTOHB(pMsg, nvLen);
		PTOHB(pMsg, defaults);
		PTOHB(pMsg, attributes);
        pNativeNvType = pMsg++;

        if (len >= 9)
        {
            // Next two bytes form the XNVT record. 
            if (*pMsg & 0x80)
            {  // High bit is set.  Treat this as a UNVT.
                *pNativeNvType = 0;
            }
            else
            {
                legacyType = FALSE;  // use a 2 byte SNVT ID.
                pNativeNvType = pMsg;
            }
            pMsg += 2;
        }

		len = arrayLen ? arrayLen : 1;
		err = validateDynamicNvIndex(nvIndex, len);

		if (err == ND_OK)
		{
			// If any NV resides at a specified index, nix it.
			for (int j = nvIndex; j < nvIndex + len; j++)
			{
				if (m_pNodeDef->getNv(j) != NULL)
				{
					// Note that remove will remove all entries in the array if this
					// falls in an array.
					remove(j, 1);
				}
			}
		}

		if (err == ND_OK)
		{
			int flags = NV_SD_DYNAMIC;
			flags |= defaultsToFlags(defaults);
			flags |= attributesToFlags(attributes);
			m_pNodeDef->createNvs(legacyType, nvIndex, false, pNativeNvType, nvLen, flags, arrayLen, NULL, NULL, NO_RATE_ESTIMATE, NO_RATE_ESTIMATE);
			// Make sure NV configuration is initialized.
			m_pNvTable->clear(nvIndex, len);
		}
	}

	unlock();

	if (err == ND_OK)
	{
		persistenceUpdate();
	}

	return err;
}

NdErrorType DynamicNvs::remove(int nvIndex, int nvCount)
{
	NdErrorType err = ND_OK;

	lock();

	// Definition of a remove is that a remove of any array element constitutes
	// removal of the entire array.  Handle both exploded and non-exploded arrays.
	
	// First look for special case of starting index in middle of array.
	NdNetworkVariable* pNv = m_pNodeDef->getNv(nvIndex);
	if (pNv != NULL &&
		pNv->getNvArrayIndex() != nvIndex)
	{
		nvCount += nvIndex - pNv->getNvArrayIndex();
		nvIndex = pNv->getNvArrayIndex();
	}

	// Look for special case of ending index in middle of array.
	pNv = m_pNodeDef->getNv(nvIndex + nvCount - 1);
	if (pNv != NULL && pNv->isArrayElement())
	{
		nvCount += pNv->getNvArrayIndex() + pNv->getMaster()->getArrayLength() - nvIndex - nvCount;
	}

	while (nvCount)
	{
		pNv = m_pNodeDef->getNv(nvIndex);
		int count = 1;
		if (pNv != NULL)
		{
			count = pNv->getElementCount();
			m_pNodeDef->getNvArraysNamed()->remove(pNv);
			m_pNodeDef->getClient()->nvDeleted(pNv);
		}
		// Clear out all the NV configuration associated with
		// this dynamic NV (even if there is none).
		m_pNvTable->clear(nvIndex, count);
		for (int i = 0; i < count; i++)
		{
            m_pNvTable->incrementIncarnation(nvIndex);
			m_pNodeDef->getNvs()->remove(nvIndex++);
			nvCount--;
		}
	}

	unlock();

	return err;
}

NdErrorType DynamicNvs::remove(unsigned char* pMsg, int len)
{
	NdErrorType err = ND_OK;

	lock();

	if (len < 4)
	{
		err = ND_INSUFFICIENT_DATA;
	}
	else
	{
		int nvIndex;
		int nvCount;

		PTOHS(pMsg, nvIndex);
		PTOHS(pMsg, nvCount);

		// A count of 0 means delete 1!  Go figure.
		if (nvCount == 0) nvCount = 1;

		err = validateDynamicNvIndex(nvIndex, nvCount);

		if (err == ND_OK)
		{
			err = remove(nvIndex, nvCount);
		}
	}

	unlock();

	if (err == ND_OK)
	{
		persistenceUpdate();
	}

	return err;
}

NdErrorType DynamicNvs::change(unsigned char* pMsg, int len)
{
	NdErrorType err = ND_OK;
	NvChangeType changeType = NV_CHANGE_TYPE; // initialize to fix warning

	lock();

	if (len < 3)
	{
		err = ND_INSUFFICIENT_DATA;
	}
	else
	{
		int type;
		int nvIndex;

		PTOHB(pMsg, type);
		PTOHS(pMsg, nvIndex);

		NdNetworkVariable* pNv = m_pNodeDef->getNv(nvIndex);

		if (pNv == NULL ||
			pNv->getPrivate())
		{
			if (nvIndex < m_pNodeDef->getTotalNvCapacity())
			{
				err = ND_OK;
			}
			else
			{
				err = ND_VALUE_OUT_OF_RANGE;
			}
		}
		else if (pNv->getStatic() &&
                ((type != NV_INFO_SNVT_INDEX) && (type != NV_INFO_SNVT_INDEX_2)) &&
				 !m_pNodeDef->getStaticNvChangeable())
		{
			err = ND_NOT_PERMITTED;
		}
		else
		{
			switch (type)
			{
                case NV_INFO_RATE_EST:
				{
					int flags;
					int re;
					int mre;

					PTOHB(pMsg, flags);
					PTOHB(pMsg, re);
					PTOHB(pMsg, mre);
					
					if (flags & 1)
					{
						pNv->setRateEstimate(re);
					}
					if (flags & 2)
					{
						pNv->setMaxRateEstimate(mre);
					}
					if (flags & 4)
					{
						pNv->setRateEstimate(-1);
					}
					if (flags & 8)
					{
						pNv->setMaxRateEstimate(-1);
					}
					changeType = NV_CHANGE_RATES;
					break;
				}
				case NV_INFO_NAME:
				{
					int nIndex;
					int nArrayIndex;
					char name[ND_NAME_BASE_LEN+1];
					memcpy(name, pMsg, ND_NAME_BASE_LEN);
					name[ND_NAME_BASE_LEN] = 0;
					NdNetworkVariable *pNdNv;
					//vxlReportEvent("Processing name update for dynamic NV: index = %d, name = %s\n",
					//	nvIndex, name);
					if (((pNdNv = m_pNodeDef->getNvArraysNamed()->get(name, nArrayIndex, nIndex)) != NULL) && (nvIndex != pNdNv->getNvIndex()))
					{
						vxlReportEvent("Duplicate dynamic NV name '%s' at NV index %d,\n"
										"\tconflicts with NV at index %d\n",
										name, nvIndex, pNdNv->getNvIndex());
						err = ND_DUPLICATE;
					}
					else
					{
						// NOTE: could make this a no-op if it found itself. For now, just log a warning
						if ((pNdNv != NULL) && ((nvIndex == pNdNv->getNvIndex())))
						{
							vxlReportEvent("Redundant dynamic NV rename '%s' at NV index %d\n", name, nvIndex);
						}
						m_pNodeDef->getNvArraysNamed()->remove(pNv);
						pNv->setName(name);
						m_pNodeDef->getNvArraysNamed()->add(pNv);
					}
					changeType = NV_CHANGE_NAME;
					break;
				}
				case NV_INFO_SD_TEXT:
				{
					int length;
					int offset;
					
					PTOHB(pMsg, length);
					PTOHS(pMsg, offset);
					pNv->getMaster()->setDescription((char*) pMsg, offset, length);
					changeType = NV_CHANGE_SD;
					break;
				}
				case NV_INFO_SNVT_INDEX:
				case NV_INFO_SNVT_INDEX_2:
				{
					if (!pNv->getChangeable())
					{
						err = ND_NOT_PERMITTED;
					}
					else
					{
						NdNvType nvType;
                        if (type == NV_INFO_SNVT_INDEX)
                        {
						    PTOHB(pMsg, nvType);
                        }
                        else // NV_INFO_SNVT_INDEX_2
                        {
                            PTOHS(pMsg, nvType);
                            if (nvType & 0x8000)
                            {
                                nvType = 0;
                            }
                        }
					    pNv->setType(nvType);
					    changeType = NV_CHANGE_TYPE;
					}
					break;
				}
				default:
				{
					err = ND_NOT_IMPLEMENTED;
					break;
				}
			}
			if (err == ND_OK)
			{
				// Callback the application (for forked NV, must be on every
				// element of array!)
				for (int nvIndex = pNv->getNvArrayIndex();
					 nvIndex < pNv->getNvArrayIndex() + pNv->getAppInstanceCount();
					 nvIndex++)
				{
					NdNetworkVariable* pNv = m_pNodeDef->getNv(nvIndex);
					m_pNodeDef->getClient()->nvChanged(pNv, changeType);
				}
			}
		}
	}

	unlock();

	if (err == ND_OK)
	{
		persistenceUpdate();
	}

	return err;
}

NdErrorType DynamicNvs::query(unsigned char* pMsg, int len, unsigned char* pResp, int &respLen)
{
	NdErrorType err = ND_OK;
	unsigned char* pRespStart = pResp;

	lock();

	if (len < 3 ||
		respLen < MAX_APDU_SIZE-1)
	{
		err = ND_INSUFFICIENT_DATA;
		respLen = 0;
	}
	else
	{
		int command;
		int nvIndex;

		respLen = 0;

		PTOHB(pMsg, command)
		PTOHS(pMsg, nvIndex)

		NdNetworkVariable* pNv = m_pNodeDef->getNv(nvIndex);

		if (pNv == NULL ||
			pNv->getPrivate())
		{
			if (command == 0)
			{
				byte empty[9];
				memset(empty, 0, sizeof(empty));
				PTONBN((int)sizeof(empty), pResp, empty);
			}
			else
			{
				err = ND_VALUE_OUT_OF_RANGE;
			}
		}
		else
		{
            NdNvType nvType = pNv->getType();
            NdLegacyNvType legacyNvType = min(nvType, 255);
			if (pNv->getForked())
			{
				// This is an exploded NV array.  NV type comes from the individual
				// NV element but everything else comes from the master.
				pNv = pNv->getMaster();
				if (pNv == NULL)
				{
					// This should never happen
					err = ND_VALUE_OUT_OF_RANGE;
				}
			}

			if (err == ND_OK)
			{
				switch (command)
				{
					case NV_INFO_DESC:
                    case NV_INFO_DESC_2:
					{
						int arrayLen = pNv->getArrayLength();
						int arrayIndex = arrayLen ? (nvIndex - pNv->getNvArrayIndex()) : 0;
						// Note that if this layout changes, then the code above must change
						// as well (see byte empty[9];)
						PTONB(pResp, (pNv->getLength()<<3) | (pNv->getDynamic() ? 2 : 1));
						PTONB(pResp, flagsToDefaults(pNv->getFlags()));
						PTONB(pResp, flagsToAttributes(pNv->getFlags()));                        
						PTONB(pResp, legacyNvType);
						PTONB(pResp, flagsToExten(pNv->getFlags()));
						PTONS(pResp, arrayLen);
						PTONS(pResp, arrayIndex);
                        if (command == NV_INFO_DESC_2)
                        {   
                            // NV_INFO_DESC_2 includes the XNVT record and a byte NV length                            
                            PTONS(pResp, nvType); // Currently we only support SNVT XNVT records 
                            PTONB(pResp, pNv->getLength());
                        }
						if (pNv->getHasName())
						{
							char name[ND_NAME_LEN];
							pNv->getName(name);
							PTONBN(ND_NAME_BASE_LEN, pResp, name);
						}
						break;
					}
					case NV_INFO_RATE_EST:
						// There is no defined value to return if estimate is not
						// defined.  Assume that caller knows based on having queried
						// ext bits (see 0 above).
						PTONB(pResp, pNv->getRateEstimate());
						PTONB(pResp, pNv->getMaxRateEstimate());
						break;
                    case NV_INFO_NAME:
						if (!pNv->getHasName())
						{
							err = ND_NOT_AVAILABLE;
						}
						else
						{
							char name[ND_NAME_LEN];
							pNv->getName(name);
							PTONBN(ND_NAME_BASE_LEN, pResp, name);
						}
						break;
					case NV_INFO_SD_TEXT:
					{
						int offset;
						int len;

						PTOHS(pMsg, offset)
						PTOHB(pMsg, len)

						if (!(pNv->getFlags() & NV_SD_DESCRIPTION))
						{
							err = ND_NOT_AVAILABLE;
						}
						else
						{
							const char* pDesc = pNv->getDescription();
							int strLen = strlen(pDesc) + 1;

							if (offset > strLen)
							{
								err = ND_VALUE_OUT_OF_RANGE;
							}
							else
							{
								if (len > strLen - offset)
								{
									len = strLen - offset;
								}
								PTONB(pResp, len);
								PTONBN(len, pResp, pDesc + offset);
							}
						}
						break;
					}
					case NV_INFO_SNVT_INDEX:
						PTONB(pResp, legacyNvType)
						break;

                    case NV_INFO_SNVT_INDEX_2:
                        PTONS(pResp, nvType); // Currently only supports SNVT XNVT
                        break;
				}
			}
		}
	}
	respLen = pResp - pRespStart;

	unlock();

	return err;
}
