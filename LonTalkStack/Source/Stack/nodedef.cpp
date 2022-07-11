//
// nodedef.cpp
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
// This file contains class implementations for LonWorks node definitions.  See
// nodedef.h for more information.
//

//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/nodedef.cpp#2 $
//

//
// TBD
// 1. Generate program definition from SI data (useful for LNS and perhaps testing)
// 2. Generate program definition from XIF (needed for VNI)
// 3. Handling LonMark objects, CPs, etc. (future enhancement)
//    This includes addressing issues with persistence.  How are these objects
//    regenerated on start-up?  Possibilities:
//    a. App registers iterms and persistence modifies them.
//    b. App gets callbacks for NVs only (XIF scenario)
//    c. Add callbacks to LonMark objects such as NV added to LMO?

#include "LtRouter.h"
#include "nodedef.h"
#include "LtNetworkManagerLit.h"

NodeDef::NodeDef()
{
	m_szDescription = NULL;
	m_bNvArrayExplosion = false;
	m_bEndDefDone = false;
	reset();
}

NodeDef::~NodeDef()
{
	reset();
}

#if PERSISTENCE_TYPE_IS(FTXL)
// Return the the maximum number of bytes that will be consumed for
// serialized data given the current static interface and an estitmated 
// NV SD length for dynamic NVs.
int  NodeDef::getMaxSerialLength(int averageDynamicNvdSdLength)
{
	int totalLength = ND_PERSISTENCE_HEADER_LEN + m_nDescLen;		// Room for header
    // Compute length for all static NVs
	int nvIndex = 0;
    NdNetworkVariable* pNv;

    while (nvIndex < getStaticNetworkVariableCount())
    {
        pNv = getNv(nvIndex++);
        if (pNv != NULL)
        {
            totalLength += pNv->getSerialLength();
		    if (!pNv->getForked()) nvIndex += pNv->getElementCount() - 1;
        }
    }

    int averageDynamicNvSize = ND_BASE_SERIAL_LENGTH + averageDynamicNvdSdLength;
    totalLength += getMaximumDynamicNetworkVariableCount() * averageDynamicNvSize;

    return totalLength;
}
#endif

void NodeDef::serialize(byte* &pBuffer, int &len)
{
	// Note NV arrays are only serialized once unless they are exploded 
	// with changeable types in which case each element is serialized (but
	// just its type).

	lock();

	// First, compute the length of all the NV info.
	int totalLength = ND_PERSISTENCE_HEADER_LEN + m_nDescLen;		// Room for header
	int nvIndex = 0;
	NdNetworkVariable* pNv;
	int nvCount = 0;
	while ((pNv = getNvs()->getNext(nvIndex)) != NULL)
	{
		if (!pNv->getForked()) nvIndex += pNv->getElementCount() - 1;
		if (!pNv->getTemporary())
		{
			int length = pNv->getSerialLength();
			if (length)
			{
				totalLength += length;
				nvCount++;
			}
		}
	}

	// Allocate memory for the serialization
	pBuffer = new byte[totalLength];
	len = totalLength;

	byte* pBuf = pBuffer;

	m_programId.serialize(pBuf);
	PTONS(pBuf, nvCount);
	PTONS(pBuf, m_nAliasCount);
	PTONS(pBuf, m_nAddressTableCount);
	PTONB(pBuf, m_bBinding2);
	PTONB(pBuf, m_bBinding3);
	PTONS(pBuf, m_nDomainCount);
	PTONS(pBuf, m_nMaximumMonitorNetworkVariableCount);
	PTONS(pBuf, m_nMaximumMonitorPointCount);
	PTONS(pBuf, m_nMaximumMonitorSetCount);
	PTONB(pBuf, m_bNvArrayExplosion);
	PTONS(pBuf, m_nMaximumDynamicNetworkVariableCount);
	PTONS(pBuf, m_nMaximumPrivateNetworkVariableCount);
	PTONB(pBuf, m_nMaxMemWriteSize);
	PTONS(pBuf, m_nReceiveTransactionCount);
	PTONB(pBuf, m_bStaticNvChangeable);
	PTONB(pBuf, m_programType);
	PTONS(pBuf, m_nDescLen);
	PTONBN(m_nDescLen, pBuf, m_szDescription);

	nvIndex = 0;
	while ((pNv = getNvs()->getNext(nvIndex)) != NULL)
	{
		int length = 0;
		if (!pNv->getForked()) nvIndex += pNv->getElementCount() - 1;
		if (!pNv->getTemporary())
		{
			pNv->serialize(pBuf, length);
		}
	}

	// Initialize remainder of memory to make "purify" happy.
	memset(pBuf, 0x5A, pBuffer + totalLength - pBuf);

	// We need to add message tag persistence 
	assert(getMessageTagCount() == 0 || !m_bOneTimeCreation);

	unlock();
}

LtPersistenceLossReason NodeDef::deserialize(byte* pBuffer, int imageLen, int nVersion)
{
	int nvIndex;
	int nvType;
	int nvCount;	
	int nvLength;
	int flags;
	int arrayLen;
	int rateEstimate;
	int maxRateEstimate;
	char name[ND_NAME_LEN];
	LtPersistenceLossReason reason = LT_PERSISTENCE_OK;
	LtProgramId key(m_programId);

	// Version 1 mistakenly defined program ID to have length 6.
	key.deserialize(pBuffer, nVersion==1);

	if (!key.isCompatible(m_programId))
	{
		reason = LT_PROGRAM_ID_CHANGE;
	}
	else
	{
		boolean bBinding2;
		boolean bBinding3;
		int nAliasCount;
		int nAddressTableCount;
		int nDomainCount;
		int nMaxMonNvCount = 0;
		int nMaxMonPtCount = 0;
		int nMaxMonSetCount = 0;
		int nMaxDynNvCount;
		int nMaxPrvNvCount;
        boolean useLegacyNvTypes = nVersion < NodeDefVer_BigNvTypes;

		PTOHS(pBuffer, nvCount);
		PTOHS(pBuffer, nAliasCount);
		PTOHS(pBuffer, nAddressTableCount);
		if (nVersion == 1)
		{
			// Version 1 had extraneous field here.
			pBuffer += 2;
		}
		PTOHB(pBuffer, bBinding2);
		PTOHB(pBuffer, bBinding3);
		if (nVersion < NodeDefVer_ECS)
		{
			// Domain count was 8 bits.
			PTOHB(pBuffer, nDomainCount);
		}
		else
		{
			// Domain count is now 16 bits.
			PTOHS(pBuffer, nDomainCount);
			PTOHS(pBuffer, nMaxMonNvCount);
			PTOHS(pBuffer, nMaxMonPtCount);
			PTOHS(pBuffer, nMaxMonSetCount);
		}
		PTOHB(pBuffer, m_bNvArrayExplosion);
		PTOHS(pBuffer, nMaxDynNvCount);
		PTOHS(pBuffer, nMaxPrvNvCount);
		PTOHB(pBuffer, m_nMaxMemWriteSize);
		PTOHS(pBuffer, m_nReceiveTransactionCount);
		PTOHB(pBuffer, m_bStaticNvChangeable);
		byte scratch;
		PTOHB(pBuffer, scratch);
		m_programType = (NdProgramType)scratch;
		PTONS(pBuffer, m_nDescLen);
		setNodeSd((const char*) pBuffer);
		pBuffer += m_nDescLen;

		while (nvCount--)
		{
			byte* pBufferSave = pBuffer;
			int len;
			pBuffer += 4;
			PTOHS(pBuffer, nvIndex);
			PTOHS(pBuffer, len);
			PTOHL(pBuffer, flags);
			PTOHB(pBuffer, nvLength);
			PTOHS(pBuffer, arrayLen);
			PTOHS(pBuffer, rateEstimate);
			PTOHS(pBuffer, maxRateEstimate);
			PTOHBN(ND_NAME_BASE_LEN, pBuffer, name);
			// Make sure name is terminated
			name[ND_NAME_BASE_LEN] = 0;
			int sdLength;
			PTOHB(pBuffer, sdLength);
			const char* pSdString = sdLength ? (char*) pBuffer : "";
			pBuffer += sdLength;
			if (m_bOneTimeCreation || (flags & NV_SD_DYNAMIC))
			{
				createNvs(useLegacyNvTypes, nvIndex, true, pBuffer, nvLength, flags, arrayLen,
					pSdString, name, rateEstimate, maxRateEstimate);
				if (flags & NV_SD_FORKED)
				{
					nvCount -= arrayLen-1;
				}
			}
			else
			{
                if (useLegacyNvTypes)
                {
				    PTOHB(pBuffer, nvType);
                }
                else
                {
                    PTOHS(pBuffer, nvType);
                }
				modifyNv(nvIndex, nvType, pSdString, name, rateEstimate, maxRateEstimate);
			}
			pBuffer = pBufferSave + len;
		}
		if (bBinding2 != m_bBinding2 ||
			bBinding3 != m_bBinding3 ||
			nAliasCount != m_nAliasCount ||
			nAddressTableCount != m_nAddressTableCount ||
			nDomainCount != m_nDomainCount ||
			nMaxDynNvCount != m_nMaximumDynamicNetworkVariableCount ||
			nMaxPrvNvCount != m_nMaximumPrivateNetworkVariableCount)
		{
			reason = LT_PROGRAM_ATTRIBUTE_CHANGE;
		}
		else if (nVersion >= NodeDefVer_ECS)
		{
			if (nMaxMonPtCount != m_nMaximumMonitorPointCount ||
				nMaxMonNvCount != m_nMaximumMonitorNetworkVariableCount ||
				nMaxMonSetCount != m_nMaximumMonitorSetCount)
			{
				reason = LT_PROGRAM_ATTRIBUTE_CHANGE;
			}
		}
	}
	return reason;
}

void NodeDef::ndInitDone()
{
	// First count NVs for purposes of setting name hash table size
	int nvCount = getStaticNetworkVariableCount() +
		getMaximumDynamicNetworkVariableCount() +
		getMaximumPrivateNetworkVariableCount();
	getNvArraysNamed()->setCapacity(nvCount*2);
}

NdErrorType NodeDef::endDefinition()
{
	NdErrorType err = ND_OK;

	lock();

	// Allow for case where this is called more than once.  Subsequent calls are ignored.
	if (!m_bEndDefDone)
	{
		// This function completes the definition of the static part of the
		// interface.  The process that needs to occur at this point is to
		// assign network NV indices to each NV (and NV array element).
		// It would be nice if this were done on the fly in which case this
		// call would not be necessary.  To do this, each collection would 
		// need to be tied back to this object in order to register NVs globally.
		// This would apply to device NVs, LonMark object members and
		// CP NVs (both LMO and NV CPs).  This doesn't sound so hard but is 
		// kind of messy.  Anyway, this function could always be deprecated.

		// For now, just support device NVs.  LonMark support comes later.
		int nvIndex = 0;
		NdNetworkVariable* pNv;
		while (err == ND_OK && (pNv=getDeviceNetworkVariable(nvIndex)) != NULL)
		{
			nvIndex++;
			err = getNvs()->add(pNv);
			getNvArrays()->add(pNv);
		}

		// Restore existing definitions from persistent memory
		if (err == ND_OK)
		{
			if (restore() != LT_PERSISTENCE_OK)
			{
				persistenceUpdate();
			}
		}
	}

	m_bEndDefDone = true;

	unlock();

	return ND_OK;
}

NdNetworkVariable* NodeDef::get(char* szName, int& arrayIndex)
{
	NdNetworkVariable* pNv;
	int nIndex;

	lock();

	pNv = getNvArraysNamed()->get(szName, arrayIndex, nIndex);
	if (pNv != NULL &&
		pNv->getArrayLength())
	{
		if (arrayIndex >= pNv->getArrayLength())
		{
			pNv = NULL;
		}
		else
		{
			pNv = getNv(pNv->getNvArrayIndex() + arrayIndex);
		}
	}

	unlock();

	return pNv;
}

NdNetworkVariable* NodeDef::get(int nvIndex, int &arrayIndex)
{
	lock();

	NdNetworkVariable* pNv = getNv(nvIndex);
	if (pNv != NULL)
	{
		arrayIndex = nvIndex - pNv->getNvArrayIndex();
	}
	else
	{
		arrayIndex = 0;
	}

	unlock();

	return pNv;
}

NdErrorType NodeDef::removeNvs(int nvIndex, int nvCount)
{
	NdErrorType err = ND_OK;

	// Definition of a remove is that a remove of any array element constitutes
	// removal of the entire array.  Handle both exploded and non-exploded arrays.
	
	// First look for special case of starting index in middle of array.
	NdNetworkVariable* pNv = getNv(nvIndex);
	if (pNv != NULL &&
		pNv->getNvArrayIndex() != nvIndex)
	{
		nvCount += nvIndex - pNv->getNvArrayIndex();
		nvIndex = pNv->getNvArrayIndex();
	}

	// Look for special case of ending index in middle of array.
	pNv = getNv(nvIndex + nvCount - 1);
	if (pNv != NULL && pNv->isArrayElement())
	{
		nvCount += pNv->getNvArrayIndex() + pNv->getMaster()->getArrayLength() - nvIndex - nvCount;
	}

	while (nvCount)
	{
		pNv = getNv(nvIndex);
		int count = 1;
		if (pNv != NULL)
		{
			count = pNv->getElementCount();
			getNvArraysNamed()->remove(pNv);
			getClient()->nvDeleted(pNv);
		}
		for (int i = 0; i < count; i++)
		{
			getNvs()->remove(nvIndex++);
			nvCount--;
		}
	}

	return err;
}

NdErrorType NodeDef::addNetworkVariable(NdNetworkVariable* pNv)
{
	lock();
	// Only device NVs supported
	addDeviceNetworkVariable(pNv);
	pNv->setParent(this);
	NdErrorType err = getNvArraysNamed()->add(pNv);
	unlock();
	return err;
}

int NodeDef::getAddressTableCount()
{
	return m_nAddressTableCount;
}

boolean NodeDef::isEatAddressCountRequired()
{
    // If device supports more than 15 address table entries and is not ECS enabled,
    // we need to support the eat_address_capacity
    return (m_maxNmVer < 1 && getAddressTableCount() > MAX_LEGACY_ADDRESS_TABLE_ENTRIES);
}

void NodeDef::setIncludeEatAddressCount(boolean unconditional)
{
    if (unconditional || isEatAddressCountRequired())
    {
        addOptionalCapability(ND_CAPABILITY_EAT_ADDRESS_CAPACITY, 1);
    }
}

int NodeDef::getEcsAddressCapacity()
{
    if (isEatAddressCountRequired())
    {
        // If eat is require, cap ECS address to legacy value, so EAT unaware managers can handle this
        // count.
        return MAX_LEGACY_ADDRESS_TABLE_ENTRIES;
    }
    return getAddressTableCount();
}

int NodeDef::getEatAddressCapacity()
{
    return min(getAddressTableCount(), MAX_EAT_ADDRESS_TABLE_ENTRIES);
}

void NodeDef::addOptionalCapability(int offset, int size)
{
    if (offset >= ND_CAPABILITY_LENGTH)
    {
        int requiredSize = (offset-ND_CAPABILITY_LENGTH) + size;
        m_numOptionalCapabilitiesBytes = max(m_numOptionalCapabilitiesBytes, requiredSize);
    }
}

int NodeDef::getAliasCount()
{
	return m_nAliasCount;
}

void NodeDef::getNvStats(int start, int end, int &count, int &maxIndex)
{
	count = 0;
	maxIndex = -1;
	for (int index = start; index < end; index++)
	{
		if (getNv(index) != NULL)
		{
			count++;
			maxIndex = index;
		}
	}
}

void NodeDef::getMcNvStats(int &count, int &maxIndex)
{
	int sdcnt = getStaticNetworkVariableCount() + getMaximumDynamicNetworkVariableCount();

	// Get monitor NV stats
	getNvStats(sdcnt, sdcnt + getMaximumMonitorNetworkVariableCount(),
			   count, maxIndex);
}

int NodeDef::getNetworkVariableNamesUnique()
{
	return getNvArraysNamed()->getNetworkVariableNamesUnique();
}

void NodeDef::getStaticDynamicNvStats(int &count, int &maxIndex)
{
	// Count dynamics
	getNvStats(getStaticNetworkVariableCount(), 
			   getStaticNetworkVariableCount() + getMaximumDynamicNetworkVariableCount(),
			   count, maxIndex);
	// Add in statics
	count += getStaticNetworkVariableCount();
	if (count && maxIndex == -1)
	{
		maxIndex = count-1;
	}
}

int NodeDef::getTotalNvCapacity()
{
	return getStaticNetworkVariableCount() + 
		getMaximumDynamicNetworkVariableCount() +
		getMaximumMonitorNetworkVariableCount();
}

int NodeDef::getNvCount()
{
	return getNvs()->getCount();
}

int NodeDef::getBinding2()
{
	return m_bBinding2;
}

int NodeDef::getBinding3()
{
	return m_bBinding3;
}

void NodeDef::getBufferConfiguration(NdBufferConfiguration& buffers)
{
	buffers = m_buffers;
}

int NodeDef::getDomainCount()
{
	return m_nDomainCount;
}

int NodeDef::getDynamicNetworkVariableCount()
{
	return getNvs()->getDynamicNvCount();
}

int NodeDef::getExplicitMessageProcessing()
{
	return m_bExplicitMessageProcessing;
}

int NodeDef::getLayer5Mip()
{
	return m_bLayer5Mip;
}

int NodeDef::getOma()
{
    return m_bOma;
}

int NodeDef::getMaximumDynamicNetworkVariableCount()
{
	return m_nMaximumDynamicNetworkVariableCount;
}

int NodeDef::getMaximumMonitorNetworkVariableCount()
{
	return m_nMaximumMonitorNetworkVariableCount;
}

int NodeDef::getMaximumMonitorPointCount()
{
	return m_nMaximumMonitorPointCount;
}

int NodeDef::getMaximumMonitorSetCount()
{
	return m_nMaximumMonitorSetCount;
}

int NodeDef::getMaximumNetworkVariableIndex()
{
	return getNvs()->getMaxInUse();
}

int NodeDef::getMaximumPrivateNetworkVariableCount()
{
	return m_nMaximumPrivateNetworkVariableCount;
}

int NodeDef::getMaxMemWriteSize()
{
	return m_nMaxMemWriteSize;
}

int NodeDef::getMessageTagCount()
{
	return m_nMessageTagCount;
}

int NodeDef::getNvArrayExplosion() 
{
	return m_bNvArrayExplosion;
}

NdNetworkVariable* NodeDef::getNv(int nIndex)
{
	return getNvs()->get(nIndex);
}

const char* NodeDef::getNodeSd()
{
	if (m_szDescription == NULL)
	{
		return "";
	}
	return m_szDescription;
}

int NodeDef::getNodeSdLen()
{
	return m_nDescLen;
}

unsigned char* NodeDef::getProgramId()
{
	return m_programId.getData();
}

LtProgramId& NodeDef::getProgramIdRef()
{
	return m_programId;
}

NdProgramType NodeDef::getProgramType()
{
	return m_programType;
}

int NodeDef::getReceiveTransactionCount()
{
	return m_nReceiveTransactionCount;
}

int NodeDef::getStaticNetworkVariableCount()
{
	return m_nStaticNetworkVariableCount;
}

void NodeDef::reset()
{
	m_siVersion = 1;
	m_siHdrLen = VER1_HDR_LEN;
	m_siLength = -1;

	m_nAddressTableCount = 0;
	m_nAliasCount = 0;
	m_bBinding2 = 0;
	m_bBinding3 = 0;
	m_nDomainCount = 0;
	m_bExplicitMessageProcessing = 0;
	m_nMaximumDynamicNetworkVariableCount = 0;
	m_nMaximumPrivateNetworkVariableCount = 0;
	m_nMaxMemWriteSize = 0;
	m_nReceiveTransactionCount = 0;
	m_bStaticNvChangeable = false;

	m_bOneTimeCreation = false;

	m_programType = PT_UNKNOWN;

	delete m_szDescription;
	m_szDescription = NULL;

	// Clear out buffer counts.
	NdBufferConfiguration buffers;
	m_buffers = buffers;

	// Clear out node config property definitions
	getConfigProps()->reset();

	// Clear out node network variable definitions
	getDeviceNetworkVariables()->reset();

	// Clear out LonMark object definitions
	getLonMarkObjects()->reset();

	// Clear out all global network variable definitions
	getNvs()->reset();
	getNvArrays()->reset();
	getNvArraysNamed()->reset();

    m_bOverrideExtendedCapabilityFlags = FALSE;
    memset(m_extendedCapabilityFlagOverrides, 0, sizeof(m_extendedCapabilityFlagOverrides));
    m_numOptionalCapabilitiesBytes = 0;
    m_dynamicFbCapacity = 0;  
    m_maxNmVer = ND_MAX_NM_VER;
}

NdErrorType NodeDef::set(int& value, int nCount, int limit)
{
	if (nCount < 0 || nCount > limit)
	{
		return ND_VALUE_OUT_OF_RANGE;
	}
	value = nCount;
	return ND_OK;
}

NdErrorType NodeDef::setAddressTableCount(int nCount)
{
    NdErrorType error = set(m_nAddressTableCount, nCount);
    setIncludeEatAddressCount(false);
    return error;
}

NdErrorType NodeDef::setAliasCount(int nCount)
{
	return set(m_nAliasCount, nCount);
}

NdErrorType NodeDef::setBinding2(int bBinding2)
{
	return set(m_bBinding2, bBinding2, 1);
}

NdErrorType NodeDef::setBinding3(int bBinding3)
{
	return set(m_bBinding3, bBinding3, 1);
}

NdErrorType NodeDef::setBufferConfiguration(NdBufferConfiguration& buffers)
{
	NdErrorType err = buffers.validate();
	if (err == ND_OK)
	{
		m_buffers = buffers;
	}
	return err;
}

NdErrorType NodeDef::setDomainCount(int nCount)
{
	return set(m_nDomainCount, nCount);
}

NdErrorType NodeDef::setExplicitMessageProcessing(int bExpMsg)
{
	return set(m_bExplicitMessageProcessing, bExpMsg, 1);
}

NdErrorType NodeDef::setLayer5Mip(int bLayer5Mip)
{
	return set(m_bLayer5Mip, bLayer5Mip, 1);
}

NdErrorType NodeDef::setOma(int bOma)
{
	return set(m_bOma, bOma, 1);
}

NdErrorType NodeDef::setStaticNetworkVariableCount(int nCount)
{
	return set(m_nStaticNetworkVariableCount, nCount);
}

NdErrorType NodeDef::setMaximumDynamicNetworkVariableCount(int nCount)
{
	if (nCount)
	{
		m_siVersion = 2;
		m_siHdrLen = VER2_HDR_LEN;
	}
	return set(m_nMaximumDynamicNetworkVariableCount, nCount);
}

NdErrorType	NodeDef::setMaximumMonitorNetworkVariableCount(int numMonitorNvEntries)
{
	return set(m_nMaximumMonitorNetworkVariableCount, numMonitorNvEntries);
}

NdErrorType NodeDef::setMaximumMonitorPointCount(int numMonitorPointEntries)
{
	return set(m_nMaximumMonitorPointCount, numMonitorPointEntries);
}

NdErrorType	NodeDef::setMaximumMonitorSetCount(int numMonitorSetEntries)
{
	return set(m_nMaximumMonitorSetCount, numMonitorSetEntries);
}

NdErrorType NodeDef::setMaximumPrivateNetworkVariableCount(int nCount)
{
	return set(m_nMaximumPrivateNetworkVariableCount, nCount);
}

NdErrorType NodeDef::setMaxMemWriteSize(int nSize)
{
	return set(m_nMaxMemWriteSize, nSize, 256);
}

NdErrorType NodeDef::setMessageTagCount(int nTags)
{
	return set(m_nMessageTagCount, nTags);
}

NdErrorType NodeDef::setNetworkVariableNamesUnique(int bValue)
{
	return getNvArraysNamed()->setNetworkVariableNamesUnique(bValue);
}

NdErrorType NodeDef::setNodeSd(const char* szNodeSd)
{
	delete m_szDescription;
	if (szNodeSd == NULL)
	{
		m_nDescLen = 1;
	}
	else
	{
		m_nDescLen = strlen(szNodeSd) + 1;
	}
	m_szDescription = new char[m_nDescLen];
	memset(m_szDescription, 0, m_nDescLen);
	if (szNodeSd != NULL)
	{
		strcpy(m_szDescription, szNodeSd);
	}
	return ND_OK;
}

NdErrorType NodeDef::setNvArrayExplosion(int bNvArrayExplosion)
{
	m_bNvArrayExplosion = bNvArrayExplosion;
	return ND_OK;
}

NdErrorType NodeDef::setOneTimeCreation(int bOneTimeCreation)
{
	m_bOneTimeCreation = bOneTimeCreation;
	return ND_OK;
}

NdErrorType NodeDef::setProgramId(unsigned char* pProgramId)
{
	m_programId.setData(pProgramId);
	return ND_OK;
}

// This copies the "modifiable" flag as well as the data
NdErrorType NodeDef::setProgramId(LtProgramId& programId)
{
	m_programId = programId;
	return ND_OK;
}

NdErrorType NodeDef::setProgramType(NdProgramType type)
{
	int pt;
	NdErrorType err = set(pt, type, PT_HOST_NEURON);
	type = (NdProgramType) pt;
	return err;
}

NdErrorType NodeDef::setReceiveTransactionCount(int nCount)
{
	return set(m_nReceiveTransactionCount, nCount);
}

NdErrorType NodeDef::addNonStaticNv(int nvIndex, NdNetworkVariable* pNv, boolean bSetName)
{
	lock();
	pNv->setParent(this);
	NdErrorType err = getNvs()->add(nvIndex, pNv);
	if (bSetName)
	{
		getNvArraysNamed()->add(pNv);
	}
	unlock();
	return err;
}

NdErrorType NodeDef::modifyNv(int nvIndex, NdNvType nvType, const char* szSdString,
							  char* szName, int rateEst, int maxRateEst)
{
	NdNetworkVariable* pNv = getNv(nvIndex);

	if (pNv == NULL)
	{
		// Strange, the NV is in persistence but wasn't registered.  Ignore for now.
	}
	else
	{
		// Note that static NVs don't support arrays with changeable type.
		assert(!(pNv->getArrayLength() && pNv->getChangeable()));

		if (nvType != pNv->getType())
		{
			pNv->setType(nvType);
			getClient()->nvChanged(pNv, NV_CHANGE_TYPE);
		}
		if (strcmp(szSdString, pNv->getDescription()) != 0)
		{
			pNv->setDescription(szSdString, 0, strlen(szSdString));
			getClient()->nvChanged(pNv, NV_CHANGE_SD);
		}
		if (strcmp(szName, pNv->getName()) != 0)
		{
			pNv->setName(szName);
			getClient()->nvChanged(pNv, NV_CHANGE_NAME);
		}
		if (rateEst != pNv->getRateEstimate() ||
			maxRateEst != pNv->getMaxRateEstimate())
		{
			pNv->setRateEstimate(rateEst);
			pNv->setMaxRateEstimate(maxRateEst);
			getClient()->nvChanged(pNv, NV_CHANGE_RATES);
		}
	}
	return ND_OK;
}

NdErrorType NodeDef::createNvs(boolean legacyNvType, int nvIndex, boolean bMultiType, 
                               byte* pNativeOrderNvTypes, int nLength, int nFlags, int nArrayLength,
		const char* pSdString, char* pNvName, int rateEstimate, int maxRateEstimate)
{
	NdErrorType err = ND_OK;
	NdNetworkVariable* pMaster = NULL;

	// NV arrays are exploded if:
	// 1. The NV types are changeable or
	// 2. The app wants all NV arrays exploded.
	int iterations = nArrayLength;
	if (nArrayLength && 
		((getNvArrayExplosion() && (nFlags & NV_SD_DYNAMIC)) || 
		 (nFlags & NV_SD_CHANGEABLE)))
	{
		nFlags |= NV_SD_FORKED;
		pMaster = new NdNetworkVariable(0, nLength, nFlags, nArrayLength, pSdString,
			pNvName, rateEstimate, maxRateEstimate);
		// Clear strings - only master needs them.
		pSdString = NULL;
		pNvName = NULL;
		pMaster->setNvIndex(nvIndex);
		nArrayLength = 0;
	}
	else
	{
		iterations = 1;
	}

	for (int i = 0; err == ND_OK && i < iterations; i++)
	{
		// Last NV in an array deletes the master on delete.
		if (i == iterations-1 && (nFlags & NV_SD_FORKED))
		{
			nFlags |= NV_SD_MASTER_DELETE;
		}
		NdNvType type;
        byte *pNvTypes = pNativeOrderNvTypes;
        if (legacyNvType)
        {
            PTOHB(pNvTypes, type);
        }
        else
        {
            PTOHS(pNvTypes, type);
        }
		if (bMultiType) pNativeOrderNvTypes = pNvTypes;
		NdNetworkVariable* pNv = new NdNetworkVariable(type, nLength, nFlags, nArrayLength,
			pSdString, pNvName, rateEstimate, maxRateEstimate);
		pNv->setNvIndex(nvIndex + i);
		if (pMaster)
		{
			pNv->setMaster(pMaster);
		}
		// Notify app of addition.
		NdNetworkVariable *pNewNv = getClient()->nvAdded(pNv);	
		if (pNewNv == NULL)
		{
			pNewNv = pNv;
		}
		else if (pNewNv != pNv)
		{
			delete pNv;
		}

		if (pNewNv->getStatic())
		{
			err = addDeviceNetworkVariable(pNewNv);
		}
		else
		{
			err = addNonStaticNv(nvIndex+i, pNewNv, i==0);
		}
	}

	return err;
}

LtPersistenceLossReason NodeDef::restore()
{
	LtPersistenceLossReason reason = getPersistenceServer()->restore();
	// For one time definition, we need to do an "end definition" to pull 
	// in the definitions.  However, there is a recursion problem here.
	assert(!m_bOneTimeCreation);
	return reason;
}

void NodeDef::persistenceUpdate()
{
	getPersistenceServer()->schedule();
}

void NodeDef::setExtendedCapabilityFlagOverrides(const byte *pExtendedCapabilityFlags, int length)
{
    m_bOverrideExtendedCapabilityFlags = true;
    memcpy(m_extendedCapabilityFlagOverrides, pExtendedCapabilityFlags, min(((int)sizeof(m_extendedCapabilityFlagOverrides)), length));
}

boolean NodeDef::getExtendedCapabilityFlagOverrides(byte *pExtendedCapabilityFlags)
{
    if (m_bOverrideExtendedCapabilityFlags)
    {
        memcpy(pExtendedCapabilityFlags, m_extendedCapabilityFlagOverrides, sizeof(m_extendedCapabilityFlagOverrides));
    }
    return m_bOverrideExtendedCapabilityFlags;
}

void NodeDef::setDynamicFbCapacity(int dynamicFbCapacity)
{
    m_dynamicFbCapacity = dynamicFbCapacity;
    addOptionalCapability(ND_CAPABILITY_DYN_FB_COUNT_OFFSET, 2);
}

void NodeDef::setMaxNmVer(int maxNmVer)
{
    m_maxNmVer = (maxNmVer < ND_MAX_NM_VER) ? maxNmVer : ND_MAX_NM_VER;
    setIncludeEatAddressCount(false);
}

NdBufferConfiguration::NdBufferConfiguration()
{
	// Assume typical minimums
	nNetIn = 1;
	nNetOut = 1;
	nPriNetOut = 1;
	nSizeNetIn = 66;
	nSizeNetOut = 42;
	nAppIn = 1;
	nAppOut = 1;
	nPriAppOut = 1;
	nSizeAppIn = 22;
	nSizeAppOut = 20;
}

NdErrorType NdBufferConfiguration::validate()
{
	NdErrorType err = ND_OK;

	return err;
}

NdErrorType NdBufferConfiguration::getEncoded(unsigned char* pEncoded)
{
	return ND_OK;
}

NdErrorType NdBufferConfiguration::setEncoded(unsigned char* pEncoded)
{
	return ND_OK;
}

NodeDefMaker::NodeDefMaker()
{
	reset();
}

NodeDefMaker::~NodeDefMaker()
{
}

void NodeDefMaker::reset()
{
	m_nLastNvIndex = -1;
	m_nLastNvOffset = -1;
    m_numXnvtRecords = 0;
    m_includeXnvt = false;
	NodeDef::reset();
}

NdErrorType NodeDefMaker::makeXif(char* szXifFile)
{
	NdErrorType err = ND_FILE_CREATION_FAILURE;
	FILE* pXif = fopen(szXifFile, "w");

	if (pXif != NULL)
	{
		// Write the header
		char* p = strrchr(szXifFile, '\\');
		if (p == NULL)
		{
			p = szXifFile;
		}
		printf("File: %s, XIF Version 4.1\n", p);
		printf("Copyright (c) 1999, Echelon Corporation\n");
		//printf("Run on %s\n", asctime(localtime(NULL));
		// TBD
	}
	return err;
}

//
// makeSiData
// 
// This function creates the self identification data using
// the appropriate format.  Currently, that is version 1 unless
// you have dynamic NVs in which case it must be version 2.
//
// SI data is housed in a combination of the read only data 
// structure and the SI data array.
NdErrorType NodeDefMaker::makeSiData(unsigned char*& pReadOnlyData, int& roLen, 
									 unsigned char*& pSiData, int& siLen)
{
	NdErrorType err = makeReadOnlyData(pReadOnlyData, roLen);
	if (err == ND_OK)
	{
		err = makeSiDataArray(pSiData, siLen);
	}
	return err;
}

NdErrorType NodeDefMaker::makeReadOnlyData(unsigned char*& pReadOnlyData, int& len)
{
	return ND_OK;	
}

NdErrorType NodeDefMaker::makeSiDataArray(unsigned char*& pSiData, int& len)
{
	getMaximumDynamicNetworkVariableCount();
	return ND_OK;
}

int NodeDefMaker::siComputeLength()
{
	// Compute length of version 1 SI data.
	int length;
	int nvCount = getNvArrays()->getStaticNvCount();
    m_numXnvtRecords = 0;
    m_includeXnvt = false;
	length = VER1_HDR_LEN +		    // Header = 6
			 nvCount*2 + 	        // Two fixed bytes per NV
			 getNodeSdLen() +	    // Plus node SD string
			 + 7;					// Trailer
	for (int i=0; i<nvCount; i++)
	{
		NdNetworkVariable* pNv = getNvArrays()->get(i);
		int len;
		pNv->makeSiExtensionData(NULL, MAX_NV_EXTENSION, len);
		length += len;
        if (pNv->getType() > 255)
        {
            m_numXnvtRecords++;
            m_includeXnvt = true;
        }
	}
    if (m_includeXnvt)
    {
        // We have to include a capability record, followed by a
        // 2 byte XNVT record count, followed by 2 bytes for each record
        length += getCapabilitiesLength() + 2 + 2*m_numXnvtRecords;
    }
	return length;
}

NdErrorType NodeDefMaker::makeSiDataHeader(unsigned char*& pSiData, int &offset, int &len)
{
	if (len != 0 && offset <= m_siHdrLen)
	{
		int maxNvs;
		unsigned char hdr[VER2_HDR_LEN];
		if (m_siVersion == 1)
		{
			if (m_siLength == -1)
			{
				m_siLength = siComputeLength();
			}
			maxNvs = getNvArrays()->getStaticNvCount();  // In version 1, each NV array is a single entry
		}
		else
		{
			m_siLength = sizeof(hdr) + getCapabilitiesLength();
			maxNvs = getStaticNetworkVariableCount();
		}

		maxNvs += getMaximumDynamicNetworkVariableCount();

		hdr[0] = (unsigned char) (m_siLength>>8);
		hdr[1] = (unsigned char) m_siLength;
		hdr[2] = (unsigned char) maxNvs;
		hdr[3] = (unsigned char) m_siVersion;
		hdr[4] = (unsigned char) (maxNvs>>8);
		hdr[5] = (unsigned char) min(getMessageTagCount(), MAX_LEGACY_ADDRESS_TABLE_ENTRIES);

		if (m_siVersion == 2)
		{
			int len = getNodeSdLen();
			int nvCount;
			int maxInUse;
			
			getStaticDynamicNvStats(nvCount, maxInUse);

			hdr[6] = (unsigned char) (getStaticNetworkVariableCount()>>8);
			hdr[7] = (unsigned char) getStaticNetworkVariableCount();
			hdr[8] = (unsigned char) (nvCount>>8);
			hdr[9] = (unsigned char) nvCount;
			hdr[10]= (unsigned char) (maxInUse>>8);
			hdr[11]= (unsigned char) maxInUse;
			hdr[12]= (unsigned char) (getAliasCount()>>8);
			hdr[13]= (unsigned char) getAliasCount();
			hdr[14]= (unsigned char) (len>>8);
			hdr[15]= (unsigned char) len;
			hdr[16]= (unsigned char) 0xC0;
		}
		int length = m_siHdrLen - offset;
		if (length > len)
		{
			length = len;
		}
		memcpy(pSiData, &hdr[offset], length);
		pSiData += length;
		len -= length;
	}
	offset -= m_siHdrLen;
	if (offset < 0) offset = 0;
	return ND_OK;
}

NdErrorType NodeDefMaker::makeSiSdString(unsigned char*& pSiData, int& offset, int& len)
{
	if (len != 0)
	{
		int stringLength = getNodeSdLen();
		if (offset < stringLength)
		{
			int length = stringLength - offset;
			if (length > len)
			{
				length = len;
			}
			strncpy((char*) pSiData, getNodeSd() + offset, length);
			pSiData += length;
			offset += length;
			len -= length;
		}
		offset -= stringLength;
		if (offset < 0) offset = 0;
	}
	return ND_OK;
}

NdErrorType NodeDefMaker::makeSiNvSummary(unsigned char*& pSiData, int &offset, int &len)
{
	if (len != 0)
	{
		int nvCount = getNvArrays()->getStaticNvCount();
		while (len != 0 && offset < nvCount*2)
		{
			NdNetworkVariable* pNv = getNvArrays()->get(offset/2);
			if (offset%2 == 0)
			{
				int flags = (unsigned char) pNv->getFlags() & NV_SD_FLAGS_MASK;
				// For config bits, use flipped non-config flags
				flags |= (unsigned char)((~pNv->getFlags() >> NV_SD_NONCONFIG_SHIFT) & NV_SD_NONCONFIG_MASK);
				if (pNv->hasExtensionData())
				{
					flags |= 0x80;
				}
				*pSiData++ = flags;
				len--;
				offset++;
			}
			if (len)
			{
                NdNvType nvType = pNv->getType();
                if (nvType > 255)
                {
                    *pSiData++ = 255;
                }
                else
                {
				    *pSiData++ = (unsigned char)nvType;
                }
				len--;
				offset++;
			}
		}
		offset -= nvCount*2;
		if (offset < 0) offset = 0;
	}
	return ND_OK;
}

NdErrorType NodeDefMaker::makeSiNvs(unsigned char*& pSiData, int &offset, int &len)
{
	unsigned char buf[MAX_NV_EXTENSION];
	int nvCount = getNvArrays()->getStaticNvCount();
	if (m_nLastNvIndex == -1 || m_nLastNvOffset > offset ||
		m_nLastNvIndex >= nvCount)
	{
		// Have to start over from 0
		m_nLastNvIndex = 0;
		m_nLastNvOffset = 0;
	}
	int curOffset = m_nLastNvOffset;
	NdNetworkVariable* pNv; 
	while (len != 0 && 
		   m_nLastNvIndex < nvCount &&
		   (pNv = getNvArrays()->get(m_nLastNvIndex)) != NULL)
	{
		int resultLen;
		pNv->makeSiExtensionData(buf, sizeof(buf), resultLen);
		if (resultLen && (curOffset + resultLen > offset))
		{
			// Copy in the data
			int copyLen = resultLen - (offset-curOffset);
			if (copyLen > len)
			{
				copyLen = len;
			}
			memcpy(pSiData, &buf[offset-curOffset], copyLen);
			pSiData += copyLen;
			offset += copyLen;
			len -= copyLen;
		}
		if (len != 0) 
		{
			m_nLastNvIndex++;
			curOffset += resultLen;
			m_nLastNvOffset = curOffset;
		}
	}
	offset -= curOffset;
	if (offset < 0) offset = 0;
	return ND_OK;
}

NdErrorType NodeDefMaker::makeSiTrailer(unsigned char* &pSiData, int &offset, int &len)
{
	int capabilityLength = getCapabilitiesLength();
	// Version 1 includes alias info and V2 compatibility info.
	int length = capabilityLength + (m_siVersion == 1?7:0);
	byte* pBuffer = new byte[length];
	byte* pBuf = pBuffer;
	if (len != 0 && offset < length)
	{
		if (m_siVersion == 1)
		{
			// Alias section
			PTONB(pBuf, 0xff);
			PTONS(pBuf, getAliasCount());
    		// V2 Compatibility
			PTONS(pBuf, getNodeSdLen());
			PTONS(pBuf, getStaticNetworkVariableCount());
		}
		// Capability record
		PTONS(pBuf, capabilityLength);
		PTONB(pBuf, 1); // Cap rec version
		PTONB(pBuf, 0); // Minimum Version
		PTONB(pBuf, getMaxNmVer()); // Maximum Version
		PTONB(pBuf, 1 + m_bBinding2 + m_bBinding3); 

		byte extendedCapabilityFlags[6];
		memset(extendedCapabilityFlags, 0, sizeof(extendedCapabilityFlags));

        if (!getExtendedCapabilityFlagOverrides(extendedCapabilityFlags))
        {
		    extendedCapabilityFlags[0] = 
                EXTCAP_FIXED_STATIC_NVS | 
#if FEATURE_INCLUDED(MONITOR_SETS)
                EXTCAP_MONITORING | 
#endif
                EXTCAP_OMA_COMMANDS;
		    if (!getNvArraysNamed()->getNetworkVariableNamesUnique())
		    {
			    extendedCapabilityFlags[0] |= EXTCAP_NONUNIQUE_NV_NAMES;
		    }
		    if (getLayer5Mip())
		    {
			    extendedCapabilityFlags[0] |= EXTCAP_INCOMING_GROUP_RESTRICTED;
		    }
        
            if (getOma())
            {   
                extendedCapabilityFlags[0] |= EXTCAP_OMA_KEYS;
            }
#if FEATURE_INCLUDED(XNVT_SUPPORT)            
            extendedCapabilityFlags[1] = EXTCAP_SUPPORTS_XNVT;
#endif
            if (getDynamicFbCapacity())
            {
                extendedCapabilityFlags[1] |= EXTCAP_SUPPRESS_DYN_FB_DEF |
                                              EXTCAP_SUPPRESS_DYN_FB_MBR_DEF;

                // Not necessarily the case - but its what the i.lon supports.  
                // If an app wants to suppress this, it can set the extcap overrrides.
                extendedCapabilityFlags[1] |= EXTCAP_SUPPORTS_DYNAMIC_NVS_ON_STATIC_FBS;
            }
        }
		PTONBN((int)sizeof(extendedCapabilityFlags), pBuf, extendedCapabilityFlags);

		PTONS(pBuf, getDomainCount());
		PTONS(pBuf, getEcsAddressCapacity());
		PTONS(pBuf, getMessageTagCount());
		PTONS(pBuf, getMaximumMonitorNetworkVariableCount());
		PTONS(pBuf, getMaximumMonitorPointCount());
		PTONS(pBuf, getMaximumMonitorSetCount());
#if FEATURE_INCLUDED(MONITOR_SETS)
		PTONS(pBuf, 1024);  // Monitor set description length
#else
        PTONS(pBuf, 0);
#endif

		int nvCount;
		int maxInUse;

		getMcNvStats(nvCount, maxInUse);

		PTONS(pBuf, nvCount);
		PTONS(pBuf, maxInUse);

        if (getNumOptionalCapabilitiesBytes() >= 2)
        {
            PTONS(pBuf, getDynamicFbCapacity());
        }
        if (getNumOptionalCapabilitiesBytes() >= 3)
        {   
            PTONB(pBuf, getEatAddressCapacity());
        }
		int copyLen = min(length - offset, len);
		memcpy(pSiData, &pBuffer[offset], copyLen);
        pSiData += copyLen;
		offset += copyLen;
		len -= copyLen;
	}
	offset -= length;
	if (offset < 0) offset = 0;
	delete pBuffer;
	return ND_OK;
}

NdErrorType NodeDefMaker::makeSiXnvt(unsigned char* &pSiData, int &offset, int &len)
{
    int length = m_includeXnvt ? 2 +  2*m_numXnvtRecords : 0;
	byte* pBuffer = new byte[length];
	byte* pBuf = pBuffer;
	if (len != 0 && offset < length)
	{
        PTONS(pBuf, m_numXnvtRecords);
	    int nvCount = getNvArrays()->getStaticNvCount();
        int xnvtIndex = 0;
	    for (int i=0; i<nvCount && xnvtIndex < m_numXnvtRecords; i++)
	    {
		    NdNetworkVariable* pNv = getNvArrays()->get(i);
            if (pNv->getType() > 255)
            {
                PTONS(pBuf, pNv->getType());
                xnvtIndex++;
            }
	    }

		int copyLen = min(length - offset, len);
		memcpy(pSiData, &pBuffer[offset], copyLen);
        pSiData += copyLen;
		offset += copyLen;
		len -= copyLen;
	}
	offset -= length;
	if (offset < 0) offset = 0;
	delete pBuffer;
	return ND_OK;
}

NdErrorType NodeDefMaker::makeSiData(unsigned char* pSiData, int offset, int len)
{
	// This routine determines where in the SI data we are located and inserts
	// the appropriate information.
	int length = len;
	makeSiDataHeader(pSiData, offset, len);
	if (m_siVersion == 1)
	{
		makeSiNvSummary(pSiData, offset, len);
		makeSiSdString(pSiData, offset, len);
		makeSiNvs(pSiData, offset, len);
	}
	makeSiTrailer(pSiData, offset, len);

    // Make the extended NV type table if necessary.  Note that this is after the "trailer"
    // because the trailer is really for information available from the capbility command.
    if (m_siVersion == 1)
    {
        makeSiXnvt(pSiData, offset, len);  
    }
	// If nothing was returned, it's an error.
	return (length==len) ? ND_VALUE_OUT_OF_RANGE : ND_OK;
}	

NdErrorType NodeDefMaker::makeSiDataAliasRelative(unsigned char* pSiData, int offset, int len)
{
	// The "trailer" starts with the alias extension which is where
	// the alias relative read starts.
	return makeSiTrailer(pSiData, offset, len);
}

NdErrorType NodeDefMaker::processXifFile(char* szXifFile, int bExplodeNvArrays)
{
	// TBD
	return ND_OK;
}

NdErrorType NodeDefMaker::processXif(char* szXifFile, int bExplodeNvArrays)
{
	NdErrorType err = ND_OK;

	if (strstr(szXifFile, ".XFB") || strstr(szXifFile, ".xfb"))
	{
		err = ND_NOT_IMPLEMENTED;
	}
	else
	{
		err = processXifFile(szXifFile, bExplodeNvArrays);
	}
	return err;
}


int NdNvArraysNamed::getHashCode(char* szName, int& arrayIndex)
{
	int hashCode = 0;
	while (*szName)
	{
		hashCode += ((int)(*szName++) * 799);
		if (*szName == '[')
		{
			arrayIndex = atoi(szName+1);
			break;
		}
	}
	// For now, only use 5/6 of the capacity since we have a bug when 
	// wraparound occurs.
	int divisor = getCapacity() * 5/6;
	hashCode = hashCode % divisor;
	return hashCode;
}

NdErrorType NdNvArraysNamed::add(NdNetworkVariable* pNv)
{
	NdErrorType err = ND_OK;

	int nIndex;
	int arrayIndex;
	char name[ND_NAME_LEN];
	pNv = pNv->getMaster();

	// For now, if names are not unique, don't even store them in the hash table.  This
	// way we allow adding of duplicates.  What doesn't work is look-up by name from the
	// application side but this meaningless anyway if duplicates are allowed.
	if (getNetworkVariableNamesUnique() && pNv->getHasName())
	{
		pNv->getArrayName(name);
		
		if (get(name, arrayIndex, nIndex) != NULL)
		{
			err = ND_DUPLICATE;
		}
		else
		{
			int hashCode = getHashCode(name, arrayIndex);
			while (NdNvArrays::get(hashCode) != NULL)
			{
				if (++hashCode == getCapacity()) hashCode = 0;
			}
			NdNvArrays::add(hashCode, pNv);
		}
	}

	return err;
}

NdErrorType NdNvArraysNamed::remove(NdNetworkVariable* pNv)
{
	int arrayIndex;
	char name[ND_NAME_LEN];
	pNv = pNv->getMaster();
	if (pNv->getHasName())
	{
		pNv->getName(name);
		int nIndex;

		if (get(name, arrayIndex, nIndex) != NULL)
		{
			// Found one to remove
			NdNvArrays::remove(nIndex);
			// Relocate everything after this up to the next blank.
			// This is done repeatedly by moving down any entry
			// not in its home location.
			int targetCode = nIndex;
			if (++nIndex == getCapacity()) nIndex = 0;
			while ((pNv = NdNetworkVariables::get(nIndex)) != NULL)
			{
				pNv->getName(name);
				int actualCode = getHashCode(name, arrayIndex);
				if ((actualCode <= targetCode &&
					 (targetCode < nIndex || actualCode != nIndex)) ||
				    (actualCode > targetCode &&
				     (targetCode < nIndex && actualCode > nIndex)))
				{
					// Move this down
					NdNvArrays::remove(nIndex);
					NdNvArrays::add(targetCode, pNv);
					targetCode = nIndex;
				}
				if (++nIndex == getCapacity()) nIndex = 0;
			}
		}
	}
	return ND_OK;
}

NdNetworkVariable* NdNvArraysNamed::get(char* szName, int &arrayIndex, int &nIndex)
{
	NdNetworkVariable* pNv = NULL;
	arrayIndex = 0;
	
	if (getCount())
	{
		int hashCode = getHashCode(szName, arrayIndex);
		char* pName = szName;
		char arrayName[ND_NAME_LEN];
		if (arrayIndex)
		{
			strncpy(arrayName, szName, ND_NAME_LEN);
			pName = arrayName;
			*strchr(pName, '[') = 0;
		}
		while ((pNv = NdNetworkVariables::get(hashCode)) != NULL &&
			strcmp(pName, pNv->getArrayName()) != 0)
		{
			if (++hashCode == getCapacity()) hashCode = 0;
		}

		nIndex = hashCode;
	}

	return pNv;
}

