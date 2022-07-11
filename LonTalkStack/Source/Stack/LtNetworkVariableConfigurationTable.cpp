//
// LtNetworkVariableConfigurationTable.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtNetworkVariableConfigurationTable.cpp#1 $
//

#include "LtStackInternal.h"


LtNetworkVariableConfigurationTable::LtNetworkVariableConfigurationTable()
{
	m_nCount = 0;
	m_selectorMap = null;
	m_nSelectorMapSize = 0;
	m_nvs = null;
    m_lockedCount = 0;
}

LtNetworkVariableConfigurationTable::~LtNetworkVariableConfigurationTable()
{
    for (int i = 0; i < m_nCount; i++)
    {
        delete m_nvs[i];
    }
    delete m_nvs;
	delete m_selectorMap;
}

LtErrorType LtNetworkVariableConfigurationTable::getNvDirSafe(int nvIndex, boolean &bOutput)
{ 
    return getStack()->getNetworkVariableDirection(nvIndex, bOutput); 
}

int LtNetworkVariableConfigurationTable::getTableType(int index)
{
	int nType = NV_TABLE_NVS;
	if (index >= numNvs)
	{
		if (index < firstPrivate)
		{
			nType = NV_TABLE_ALIASES;
		}
		else
		{
			nType = NV_TABLE_PRIVATES;
		}
	}
	return nType;
}

void LtNetworkVariableConfigurationTable::getCounts(int& nvCount, int& privateNvCount, int& aliasCount)
{
	nvCount = numNvs;
	privateNvCount = numPrivate;
	aliasCount = numAliases;
}

void LtNetworkVariableConfigurationTable::setCounts(int nvCount, int privateCount, int aliasCount) 
{
	lock();

	m_nCount = nvCount + aliasCount + privateCount;
    m_nvs = new LtNetworkVariableConfiguration* [m_nCount];
	m_nSelectorMapSize = (m_nCount) * 2;
	if (m_nSelectorMapSize)
	{
		m_selectorMap = new int[m_nSelectorMapSize];
    	memset(m_selectorMap, 0xff, m_nSelectorMapSize*sizeof(m_selectorMap[0]));
	}
	m_bRehash = true;
    numNvs = nvCount;
    numAliases = aliasCount;
    firstPrivate = numNvs + numAliases;
    numPrivate = privateCount;
    for (int i=0; i<m_nCount; i++) 
	{
        m_nvs[i] = NULL;
	}
    configured = true;

	unlock();
}

int LtNetworkVariableConfigurationTable::mapIndex(int index, int nType)
{
	if (nType == NV_TABLE_ALIASES)
	{
		index += numNvs;
	}
	else if (nType == NV_TABLE_PRIVATES)
	{
		index += firstPrivate;
	}
	return index;
}

LtErrorType LtNetworkVariableConfigurationTable::getPointer(int index, LtNetworkVariableConfiguration **ppNvc, int nType)
{
	LtErrorType err = LT_NO_ERROR;
    if (index < 0)
    {
        err = LT_INVALID_NV_INDEX;
    }
    else
    {
	    int max = numNvs-1;
	    if (nType == NV_TABLE_NVS_ALIASES)
	    {
		    max = firstPrivate-1;
	    }
	    else if (nType == NV_TABLE_ALIASES)
	    {
		    max = firstPrivate-1;
	    }
	    else if (nType == NV_TABLE_PRIVATES)
	    {
		    max = m_nCount-1;
	    }
	    index = mapIndex(index, nType);
        if (index < 0 || index > max)
	    {
		    err = LT_INVALID_NV_INDEX;
	    }
	    else
	    {
            *ppNvc = m_nvs[index];
	    }
    }
	return err;
}

LtErrorType LtNetworkVariableConfigurationTable::get(int index, LtNetworkVariableConfiguration **ppNvc, int nType)
{
	LtErrorType err = getPointer(index, ppNvc, nType);
	if (err == LT_NO_ERROR && (*ppNvc == NULL))
	{
		err = LT_NOT_FOUND;
	}
	return(err);
}
void LtNetworkVariableConfigurationTable::copyNvConfig(int index,
													   LtNetworkVariableConfiguration* pSource, 
													   LtNetworkVariableConfiguration* pDest)
{
	if (pSource == NULL)
	{
		LtNetworkVariableConfiguration nvc;
        nvc.init(index, getPrimaryIndex(index), getTableType(index));
        getStack()->getDefaultNetworkVariableConfigAttributes(index, nvc);
		*pDest = nvc;
	}
	else
	{
		*pDest = *pSource;
	}
}
LtErrorType LtNetworkVariableConfigurationTable::get(int index, LtNetworkVariableConfiguration* pNvc, int nType)
{
    LtNetworkVariableConfiguration* pLocalNvc;

    LtErrorType err = getPointer(index, &pLocalNvc, nType);
    if (err == LT_NO_ERROR)
    {
		copyNvConfig(mapIndex(index, nType), pLocalNvc, pNvc);
    }
    return(err);
}

// getNv() is like get() except that getNv is used to fetch public or private
// NV entries.  Therefore, aliases must be skipped.
LtErrorType LtNetworkVariableConfigurationTable::getNv(int index, LtNetworkVariableConfiguration **ppNvc) 
{	
	LtErrorType err = LT_NO_ERROR;
	if (index >= numNvs) 
	{
		index += numAliases;
	}
    if (index < 0)
    {
		err = LT_INVALID_NV_INDEX;
    }
	else if (index >= m_nCount) 
	{
		err = LT_INVALID_NV_INDEX;
    }
	else if (m_nvs[index] == NULL)
    {
        err = LT_NOT_FOUND;
    }
    else
	{
        *ppNvc = m_nvs[index];
	}
	return err;
}

LtErrorType LtNetworkVariableConfigurationTable::getNv(int index, LtNetworkVariableConfiguration* pNvc)
{
    LtNetworkVariableConfiguration* pLocalNvc;

    LtErrorType err = getNv(index, &pLocalNvc);
    if (err == LT_NOT_FOUND)
    {   // Doesn't matter - we will just manufacture it.
        pLocalNvc = NULL;
        err = LT_NO_ERROR;
    }
	if (err == LT_NO_ERROR) 
    {
		if (index >= numNvs) 
		{
			index += numAliases;
		}
		copyNvConfig(index, pLocalNvc, pNvc);
    }
    return err;
}

LtErrorType LtNetworkVariableConfigurationTable::setNv(int index, LtNetworkVariableConfiguration &nvc, boolean bEvent)
{
	int nvType = (index >= numNvs) ? NV_TABLE_PRIVATES : NV_TABLE_NVS;

	if (nvType == NV_TABLE_PRIVATES)
	{
		index -= numNvs;
	}
	return set(index, nvc, nvType, bEvent);
}

void LtNetworkVariableConfigurationTable::mapSelectorToIndex(int i, int selector, boolean bOutput)
{
	// Don't map selector's which are the unbound selector for the given index.  This
	// avoids having to worry about having mappings sometimes and sometimes not for
	// unbound NVs.
	if (selector != LT_UNUSED_SELECTOR &&
		(i>=numNvs || !LtNetworkVariableConfiguration::unboundSelectorForIndex(selector, i)))
	{
		int index = getSelectorMapIndex(selector, bOutput);
		while (true)
		{
			if (m_selectorMap[index] == -1)
			{
				m_selectorMap[index] = i;
				break;		
			}
			if (++index == m_nSelectorMapSize) index = 0;
		}
	}
}

void LtNetworkVariableConfigurationTable::updateSelectorMap()
{
	// The selector map hashes selector/direction pairs to nv table indices.
	// The hash table is allocated to be twice the size that's necessary to 
	// accommodate all NV entries to allow for sparse packing and thus quick
	// termination in the case of no match.  It also has the property that
	// the first matching pair is used (thus the need to rehash on any
	// change to the NV table - we could do this more elegantly on updates
	// but this seems OK (and easier to implement)).
	memset(m_selectorMap, 0xff, m_nSelectorMapSize*sizeof(m_selectorMap[0]));
	for (int i = 0; i < m_nCount; i++)
	{
		LtNetworkVariableConfiguration* pNvc = m_nvs[i];
        if (pNvc == NULL)
        {
		    // Map all NVs (that actually exist), even those with no selection.  These may need
		    // to be looked up for matching NV purposes.
            boolean bOutput;
            if (getNvDirSafe(i, bOutput) == LT_NO_ERROR)
            {
 				LtNetworkVariableConfiguration nvc(i, getTableType(i), 0);
                nvc.setOutput(bOutput);
				mapSelectorToIndex(i, nvc.getSelector(), nvc.getOutput());
            }
        }
		else
		{
			mapSelectorToIndex(i, pNvc->getSelector(), pNvc->getOutput());
		}
	}
}


LtErrorType LtNetworkVariableConfigurationTable::set(int index, LtNetworkVariableConfiguration& nvc, int nType, boolean bEvent) 
{
    LtNetworkVariableConfiguration *pNvc;
	int eventNvIndex[2];

	memset(eventNvIndex, -1, sizeof(eventNvIndex));
	lock();

	LtErrorType err = getPointer(index, &pNvc, nType);

	if (err == LT_NO_ERROR)
	{
	    int absoluteIndex = mapIndex(index, nType);
		boolean bAlias = nvc.isAlias(); // = nType == NV_TABLE_ALIASES;
		boolean bHadBoundSelector = FALSE;
		if (pNvc != NULL)
		{
			bHadBoundSelector = !LtNetworkVariableConfiguration::unboundSelectorForIndex(pNvc->getSelector(), absoluteIndex);

			if (bAlias && pNvc->getPrimaryIndex() != LT_UNUSED_INDEX)
			{
				// Remove previous primary relationship
				LtNetworkVariableConfiguration* pPrimary;

				if (getPointer(pNvc->getPrimary(), &pPrimary, NV_TABLE_NVS) == LT_NO_ERROR)
				{
					eventNvIndex[0] = pNvc->getPrimary();
					if (pPrimary != NULL)
					{
						pPrimary->removeAlias(absoluteIndex);
					}
				}
			}
		}
		if (!bAlias)
		{
			eventNvIndex[0] = absoluteIndex;
		}
		if (bAlias && nvc.getPrimaryIndex() != LT_UNUSED_INDEX)
		{
			// Add new primary relationship
            LtNetworkVariableConfiguration* pPrimary;
			if (getPointer(nvc.getPrimary(), &pPrimary, NV_TABLE_NVS) == LT_NO_ERROR)
			{
				eventNvIndex[1] = nvc.getPrimary();
				if (pPrimary == NULL)
				{
					pPrimary = add(mapIndex(nvc.getPrimary(), NV_TABLE_NVS));
					pPrimary->setOutput(nvc.getOutput());
				}
				pPrimary->addAlias(absoluteIndex);
			}
		}
		// If the old entry was hashed, we currently don't have the
		// means to remove it easily.  Would be a nice enhancement.
		// Instead, rehash.
		if (!bHadBoundSelector)
		{
            // Only map selector if we don't need to re-hash anyway.  If we do need to re-hash, there is
            // no point to updating the table since it will get rebuilt when we need it, and besides,
            // since values are never removed (except via re-hashing), binding/unbinding/rebinding can
            // fill up the table if no NV updates ever come in (EPR 67509)
            if (!m_bRehash)
            {
			    mapSelectorToIndex(absoluteIndex, nvc.getSelector(), nvc.getOutput());
            }
		}
		else
		{
			m_bRehash = true;
		}

		if (getPointer(index, &pNvc, nType) == LT_NO_ERROR)
		{
			if ((pNvc == NULL) && nvc.inUse())
			{	// Adding a new one.
				pNvc = add(absoluteIndex);
			}
			if (pNvc != NULL)
			{	// Update it.
				*pNvc = nvc;
			}
		}
	}

	unlock();
	if (err == LT_NO_ERROR)
	{
		if (bEvent)
		{
			for (int i=0; i < (int)LENGTH(eventNvIndex); i++)
			{
				// Generate an NV changed event up to the application.  This is needed
				// by LNS to determine when to switch from polling to not.
				if (eventNvIndex[i] != -1)
				{
					NdNetworkVariable* pNv = m_pNodeDef->getNv(eventNvIndex[i]);
					if (pNv)
					{
						m_pNodeDef->getClient()->nvChanged(pNv, NV_CHANGE_CONFIG);
					}
				}
			}
		}
	}
	return err;
}

int LtNetworkVariableConfigurationTable::getPrimaryIndex(int index)
{
	int nType = getTableType(index);
	// Each NV config points to a primary.
	// Public NVs (static, dynamic, MC) point to themselves.
	// Aliases point to a public or private NV (set later).
	// Private NVs point to themselves but at the NV index, not
	// the NV config index (i.e., excludes aliases).
	if (nType == NV_TABLE_PRIVATES)
	{
		return index - numAliases;
	}
	else if (nType == NV_TABLE_ALIASES)
	{
		return LT_UNUSED_INDEX;
	}
	return index;
}

void LtNetworkVariableConfigurationTable::nvChange()
{
	m_bRehash = true;
}

LtErrorType LtNetworkVariableConfigurationTable::get(int& index, int nSelector, boolean bOutput, LtNetworkVariableConfiguration &nvc) 
{
	LtErrorType err = LT_NOT_FOUND;

    assert(isLocked());
	if (index != -2 && mappingsAvailable()) 
	{
		int nvIndex;
		LtNetworkVariableConfiguration *pNvc = NULL;

		if (index == -1)
		{
			index = getSelectorMapIndex(nSelector, bOutput);
		}

		if (m_bRehash)
		{
			m_bRehash = false;
			updateSelectorMap();
		}

		// Search through all the entries looking for this index until an entry
		// with a matching selector and direction or matching index is found. 
		while (err == LT_NOT_FOUND)
		{
			nvIndex = m_selectorMap[index];

			if (++index == m_nSelectorMapSize) index = 0;

			if (nvIndex == -1)
			{
				// Check for unbound NV
				index = -2;
				if (LtNetworkVariableConfiguration::unboundSelectorToIndex(nSelector, nvIndex) &&
					getPointer(nvIndex, &pNvc, NV_TABLE_NVS) == LT_NO_ERROR)
				{
					if (pNvc == NULL || pNvc->getSelector() == nSelector)
					{
                        boolean bNvIsOutput;
                        if (getNvDirSafe(nvIndex, bNvIsOutput) == LT_NO_ERROR && bNvIsOutput == bOutput)
						{
							err = LT_NO_ERROR; 
						}
					}
				}
				break;
			}
			else 
			{
				pNvc = m_nvs[nvIndex];

				if (pNvc != NULL &&
				    pNvc->getSelector() == nSelector &&
					pNvc->getOutput() == bOutput)
				{
					err = LT_NO_ERROR;
				}
			}
		}
		if (err == LT_NO_ERROR)
		{
			copyNvConfig(nvIndex, pNvc, &nvc);
		}
	}
	return err;
}

void LtNetworkVariableConfigurationTable::clearNv(int index, int count) 
{
	if (index >= numNvs) index += numAliases;
	clear(index, count);
}

void LtNetworkVariableConfigurationTable::clear(int index, int count) 
{
	// By default, we don't clear out private NVs.  Leave it up to the app to manage
	// these how it sees fit.
    if (count == -1) count = firstPrivate;
    for (int k = index; k < index + count; k++) 
	{
		if (m_nvs[k] != NULL)
		{
			m_nvs[k]->init(getTableType(k));
		}
    }
	nvChange();
}

void LtNetworkVariableConfigurationTable::incrementIncarnation(int index)
{
    if (m_nvs[index] != NULL)
    {
        m_nvs[index]->incrementIncarnation();
    }
}

LtNetworkVariableConfiguration* LtNetworkVariableConfigurationTable::getNext(int nvIndex, LtVectorPos &pos)
{
    LtNetworkVariableConfiguration* pNvc = null;
    LtNetworkVariableConfiguration* pPrimary = null;
	if ((getNv(nvIndex, &pPrimary) == LT_NO_ERROR) && pPrimary != NULL)
	{
		int aliasIndex;
		if (pPrimary->nextAlias(pos, aliasIndex))
		{
			get(aliasIndex, &pNvc);
		}
    }
    return pNvc;
}

void LtNetworkVariableConfigurationTable::store(byte* data, int &offset) 
{

	int count = 0;
	byte* p_count_location = &data[offset];

	// we allocate sizeof(NvConfigTableStoreHeader) to maintain
	
	offset += sizeof(NvConfigTableStoreHeader);
    for (int i = 0; i < m_nCount; i++) 
	{
        if ((m_nvs[i] != NULL) && m_nvs[i]->inUse())
        {
			count++;
			// Store the index -- assuming not more than 2 bytes
			data[offset] = (byte) i;
			data[offset+1] = (byte) (i >> 8);
			data[offset+2] = (byte) (i >> 16);
			data[offset+3] = (byte) (i >> 24);

			offset+= sizeof(int);  

            offset += m_nvs[i]->toLonTalk(&data[offset]);
            if (i >= firstPrivate) 
		    {
                offset += m_nvs[i]->getAddress()->toLonTalk(&data[offset], 2);
            }
        }
    }
	// Store the terminator

	// Store the count -  bytes allocated before
	*p_count_location++ = count;
	*p_count_location++ = count >> 8;
	*p_count_location++ = count >> 16;
	*p_count_location = count >> 24;

}

LtNetworkVariableConfiguration* LtNetworkVariableConfigurationTable::newEntry(int index)
{
	LtNetworkVariableConfiguration* pNvc = new LtNetworkVariableConfiguration;
    pNvc->init(index, getPrimaryIndex(index), getTableType(index));
	pNvc->setIsACopy(FALSE);
	return pNvc;
}

LtNetworkVariableConfiguration* LtNetworkVariableConfigurationTable::add(int index)
{	
	// To simplify locking considerations, they are never deleted, only added.
	// However, those that are in use are stored in the persistence data, so
	// on the next open no object will be allocated for those that are not
	// in use.  Note that by not deleting them it is also possible to preserve the incarnation
    // number.
	LtNetworkVariableConfiguration* pNvc = newEntry(index);
	if (index < m_nCount)
	{
		m_nvs[index] = pNvc;
	}
	return pNvc;
}

LtErrorType LtNetworkVariableConfigurationTable::loadEntry(int index, byte* data, int &offset, int nVersion)
{

	boolean allocated = FALSE;
    boolean nvConfigComplete = true;
	LtErrorType err = LT_NO_ERROR;
	if (index >= m_nCount)
	{
		err = LT_INVALID_NV_INDEX;
	}
	else
	{
		LtNetworkVariableConfiguration *pNvc = m_nvs[index];
		if (pNvc == NULL)
		{	// Allocate if it does not exist.
			allocated = TRUE;
			pNvc = newEntry(index);
		}
		offset += pNvc->fromLonTalk(&data[offset], false, nVersion);
        if (nVersion < NetImageVer_eliminateNvHasDefaultsFlag)
        {
            nvConfigComplete = data[offset++];
        }
        else
        {
            nvConfigComplete = TRUE;
        }
		if (pNvc->getIsAlias() && pNvc->getPrimaryIndex() != LT_UNUSED_INDEX)
		{
			// Add new primary relationship
			LtNetworkVariableConfiguration *pPrimary;
			if (getPointer(pNvc->getPrimaryIndex(), &pPrimary, NV_TABLE_NVS) == LT_NO_ERROR)
			{
				if (pPrimary == NULL)
				{   // Allocate it.
					pPrimary = add(pNvc->getPrimaryIndex());
					pPrimary->setOutput(pNvc->getOutput());
				}
				pPrimary->addAlias(index);
			}
		}
		if (nVersion < NetImageVer_ECS)
		{
			offset++;
		}
		if (index >= firstPrivate)
		{
			if (nVersion < NetImageVer_ECS || !nvConfigComplete)
			{
				pNvc->setNvUpdateSelection(LT_SELECTION_NEVER);
				pNvc->setNvResponseSelection(LT_SELECTION_NEVER);
				pNvc->setNvRequestSelection(LT_SELECTION_NEVER);
			}
			if (nvConfigComplete) 
			{
				int len;
				err = pNvc->getAddress()->fromLonTalk(&data[offset], len, nVersion);
				offset += len;
				pNvc->setAddressTableIndex(LT_EXPLICIT_ADDRESS);
				if (nVersion < NetImageVer_ECS)
				{
					// This one was stored little endian for no good reason.
					int idx = LtMisc::makeint(data[offset+1], data[offset]);
					pNvc->setNvIndex(idx);
					offset += 2;
				}
			}
		}
		if (allocated)
		{
			if (pNvc->inUse())
			{	// Put it in the table.
				m_nvs[index] = pNvc;
			}
			else
			{	// Version 2 and prior store empty ones - don't keep it around.
				delete pNvc;
			}
		}
	}
	return err;
}

LtErrorType LtNetworkVariableConfigurationTable::load(byte* data, int offset, int nVersion) 
{
	LtErrorType err = LT_NO_ERROR;
    if (nVersion < NetImageVer_compact)
    {   // These versions contain all NVs, and do not include the nv index.
        for (int i = 0; err == LT_NO_ERROR && i < m_nCount; i++) 
	    {
			loadEntry(i, data, offset, nVersion);
        }
    }
    else
    {   // These versions start with the index, and include only those that are in use.

		int count = LtMisc::makeint32(data[offset+3], data[offset+2],
				data[offset+1], data[offset]);

		offset += sizeof(NvConfigTableStoreHeader);   //size of count
        int index;

		for (int i = 0; i < count; i++)
        {
            index = LtMisc::makeint32(data[offset+3], data[offset+2],
								data[offset+1], data[offset]);
            offset += sizeof(int);   // size of index
            loadEntry(index, data, offset, nVersion);
		}
    }
	assert(err == LT_NO_ERROR);
	return err;
}

int LtNetworkVariableConfigurationTable::getStorSize(int totalNvCount, int privateNvs)
{
	return 
		((LtNetworkVariableConfiguration::getStoreSize() + NV_EXTRA_STORE_SIZE) * totalNvCount) +
		(LtAddressConfiguration::getStoreSize() * privateNvs) + 
		sizeof(NvConfigTableStoreHeader);
}

int LtNetworkVariableConfigurationTable::getMaxStoreSize()
{
	return getStorSize(m_nCount, numPrivate);
}

int LtNetworkVariableConfigurationTable::getStoreSize()
{
	int count = 0;
	int privateAddressEntries = 0;
	// Count how many addresses need to be stored;
    for (int i = 0; i < m_nCount; i++) 
	{
		if (m_nvs[i] != NULL)
		{
			++count;
            if (i >= firstPrivate) 
			{	// Have to store an address entry as well.
				privateAddressEntries++;
			}
		}
    }

	return getStorSize(count, privateAddressEntries);
}

LtErrorType LtNetworkVariableConfigurationTable::initialize(int fromIndex, int toIndex, byte* pData, int len, int domainIndex)
{
	return initialize(fromIndex, toIndex, NV_TABLE_NVS);
}

LtErrorType LtNetworkVariableConfigurationTable::initialize(int fromIndex, int toIndex, int nType)
{
	LtErrorType err = LT_NO_ERROR;

	lock();

	for (int index = fromIndex; err == LT_NO_ERROR && index <= toIndex; index++)
	{
		LtNetworkVariableConfiguration* pNvc;

		err = getPointer(index, &pNvc, nType);

		if (err == LT_INVALID_NV_INDEX)
		{
			// Ignore over range errors if "all" was specified.
			if (toIndex == LT_ALL_ECS_INDEX)
			{
				err = LT_NO_ERROR;
			}
			break;
		}
		else
		{
			if (pNvc != NULL)
            {
                if (pNvc->getIsAlias() && pNvc->getPrimaryIndex() != LT_UNUSED_INDEX)
			    {
				    // Remove primary relationship
                    LtNetworkVariableConfiguration *pPrimary;
				    if (get(pNvc->getPrimary(), &pPrimary, NV_TABLE_NVS) == LT_NO_ERROR)
				    {
					    pPrimary->removeAlias(mapIndex(index, nType));
				    }
			    }
 			    pNvc->initialize();
           }
		}
	}
	unlock();
	return err;
}

LtErrorType LtNetworkVariableConfigurationTable::enumerate(int index, boolean authenticated, LtApdu &response)
{
	return enumerate(index, response, NV_TABLE_NVS);
}

LtErrorType LtNetworkVariableConfigurationTable::enumerate(int index, LtApdu &response, int nType)
{
	LtErrorType err = LT_END_OF_ENUMERATION;
    LtNetworkVariableConfiguration *pNvc;
	int i = index;

	while (getPointer(i, &pNvc, nType) == LT_NO_ERROR)
	{
		if (pNvc != NULL && pNvc->inUse())
		{
			// Found the next address.  Return it.
			byte byData[MAX_APDU_SIZE];
			response.setData(0, i>>8);
			response.setData(1, i&0xff);
			response.setData(byData, 2, pNvc->toLonTalk(byData, true));
			err = LT_NO_ERROR;
			break;
		}
		i++;
	}

	return err;
}

LtErrorType LtNetworkVariableConfigurationTable::update(int index, byte* pData, int len)
{
	return update(index, pData, len, NV_TABLE_NVS);
}

LtErrorType LtNetworkVariableConfigurationTable::update(int index, byte* pData, int len, int nType)
{
	LtErrorType err;
	LtNetworkVariableConfiguration nvc;

    err = get(index, &nvc, nType);
	if (err == LT_NO_ERROR)
	{
		nvc.fromLonTalk(pData, len);
		err = set(index, nvc, nType);
	}

	return err;
}

boolean LtNetworkVariableConfigurationTable::isBound(int index, int flags)
{
	// An NV is bound if it is bound or has a bound alias.  
    LtNetworkVariableConfiguration* pNvc;
	boolean bIsBound = false;

	if (getNv(index, &pNvc) == LT_NO_ERROR)
	{
		bIsBound = pNvc->isBound(flags);
	}
	if (!bIsBound)
	{
		// Check aliases
		LtVectorPos pos;
		while (!bIsBound && (pNvc=getNext(index, pos)) != null)
		{
			bIsBound = pNvc->isBound(flags);
		}
	}
	return bIsBound;
}

LtErrorType LtNetworkVariableConfigurationTable::getPrivateNv(int &index, boolean bOutput, boolean bSourceSelection, int nvIndex, int nvSelector, LtOutgoingAddress* pAddress)
{
	LtErrorType err = LT_NO_ERROR;
	int searchIndex = -1;
    LtNetworkVariableConfiguration nvc;

    lock();
	// Search through the specified table looking for a match of the input
	// attributes.  The only efficient way to find a match is by selector.
	while ((err = get(searchIndex, nvSelector, bOutput, nvc)) == LT_NO_ERROR)
	{
		if (nvc.getIndex() >= firstPrivate &&
			bSourceSelection == (nvc.getNvUpdateSelection() != LT_SELECTION_NEVER) &&
			nvIndex == nvc.getNvIndex() &&
			pAddress->equals(*nvc.getAddress()))
		{
		    index = nvc.getIndex() - numAliases;
			break;
		}
	}
    unlock();
	return err;
}

LtErrorType LtNetworkVariableConfigurationTable::checkLimits(int cmd, byte* pData, int len)
{
	boolean bOk = true;
	switch (cmd)
	{
	case LT_NM_UPDATE:
	case LT_NM_CREATE:
		bOk = len == LT_NV_STORE_SIZE;
		break;
	default:
		bOk = LtConfigurationEntity::checkLimits(cmd, pData, len) == LT_NO_ERROR;
		break;
	}
	return bOk ? LT_NO_ERROR : LT_INVALID_PARAMETER;
}

void LtNetworkVariableConfigurationTable::lock()  
{   
    // This locks the NV configuration table.  Since updates to the
    // NV configuration table sometimes need access to NV defintions (see getNvDirSafe)
    // and NV definition sometimes has to access the NV configuration table (see getPrivateNv)
    // lock them at the same time, otherwise we can run into a deadlock.  
    // Easiest and most efficient way to ensure that they are locked at the same time is
    // to use the same lock.
    getStack()->lockNvs();
    m_lockedCount++;
}
void LtNetworkVariableConfigurationTable::unlock()
{ 
    m_lockedCount--;
    getStack()->unlockNvs();
}

boolean LtNetworkVariableConfigurationTable::isLocked()
{
    return m_lockedCount != 0;
}
//
// The alias table is really just part of the NV table.  However, the NM commands treat
// it as a separate entity.  These functions make this mapping happen.
//

LtErrorType LtAliasTable::initialize(int fromIndex, int toIndex, byte* pData, int len, int domainIndex)
{
	return m_nvt.initialize(fromIndex, toIndex, NV_TABLE_ALIASES);
}

LtErrorType LtAliasTable::enumerate(int index, boolean authenticated, LtApdu &response)
{
	return m_nvt.enumerate(index, response, NV_TABLE_ALIASES);
}

LtErrorType LtAliasTable::update(int index, byte* pData, int len)
{
	return m_nvt.update(index, pData, len, NV_TABLE_ALIASES);
}

LtErrorType LtAliasTable::checkLimits(int cmd, byte* pData, int len)
{
	boolean bOk = true;
	switch (cmd)
	{
	case LT_NM_UPDATE:
	case LT_NM_CREATE:
		bOk = len == LT_NV_STORE_SIZE + 2;
		break;
	default:
		bOk = LtConfigurationEntity::checkLimits(cmd, pData, len) == LT_NO_ERROR;
		break;
	}
	return bOk ? LT_NO_ERROR : LT_INVALID_PARAMETER;
}

