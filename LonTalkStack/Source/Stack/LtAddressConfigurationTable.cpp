//
// LtAddressConfigurationTable.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtAddressConfigurationTable.cpp#2 $
//

#include "LtStackInternal.h"

//
// Private Member Functions
//


//
// Protected Member Functions
//


//
// Public Member Functions
//


LtAddressConfigurationTable::LtAddressConfigurationTable() 
{
	m_lastMatchingIndex = 0;
	m_addr = null;
	m_count = 0;
}

LtAddressConfigurationTable::~LtAddressConfigurationTable()
{
    for (int i = 0; i < m_count; i++)
    {
        delete m_addr[i];
    }
    delete m_addr;
	m_domainGroups.clear(true);
}

void LtAddressConfigurationTable::setCount(LtDeviceStack* pStack, int count) 
{
	lock();

	m_pStack = pStack;
    m_addr = new LtAddressConfiguration*[count];
    for (int i = 0; i < count; i++)
    {
        m_addr[i] = NULL;
    }
	m_count = count;

	unlock();
}

LtErrorType LtAddressConfigurationTable::get(int index, LtAddressConfiguration* pAc)
{
	LtAddressConfiguration* pLocalAc;
    LtErrorType err = get(index, &pLocalAc);
	if (err == LT_NO_ERROR)
	{
		if (pLocalAc == NULL)
		{
			LtAddressConfiguration ac;
			*pAc = ac;
		}
		else
		{
			*pAc = *pLocalAc;
		}
	}
	return err;
}

LtErrorType LtAddressConfigurationTable::get(int index, LtAddressConfiguration** ppAc) 
{
	LtErrorType err = LT_NO_ERROR;
	if (index < 0 || index >= m_count) 
	{
		err = LT_INVALID_ADDR_TABLE_INDEX;
	}
	else
	{
		*ppAc = m_addr[index];
	}
	return err;
}

LtErrorType LtAddressConfigurationTable::set(int index, LtAddressConfiguration& ac) 
{
	lock();

	LtErrorType err = LT_NO_ERROR;
	if (index >= m_count) 
	{
		err = LT_INVALID_ADDR_TABLE_INDEX;
	}
	else
	{
		LtAddressConfiguration *pAc = m_addr[index];
		ac.setIndex(index);
		if (ac.isBound() && (pAc == NULL))
		{	// Allocate one if its bound.
			pAc = new LtAddressConfiguration;
		}
		if (pAc != NULL)
		{
			*pAc = ac;
			m_addr[index] = pAc;
		}
		m_bRebuildMap = true;
	}

	unlock();
	return err;
}

//
// Get a group mask for use by routing tables.  This mask includes
// all groups which can receive.
//
void LtAddressConfigurationTable::getGroups(int domainIndex, LtGroups &gp)
{
	lock();

	rebuildMap();

	LtGroupAddresses* pGa = m_domainGroups.elementAt(domainIndex);

	if (pGa != null)
	{
		int size = pGa->size();
		for (int i=0; i<size; i++)
		{
			LtAddressConfiguration* pAc = pGa->elementAt(i);
			if (pAc && pAc->getRestrictions() != LT_GRP_OUTPUT_ONLY)
			{
				gp.set(i);
			}
		}
	}
	unlock();
}

LtAddressConfiguration* LtAddressConfigurationTable::get(int domainIndex, int group)
{
	LtAddressConfiguration* pAc = null;

	lock();

	rebuildMap();

	LtGroupAddresses* pGa = m_domainGroups.elementAt(domainIndex);

	if (pGa != null)
	{
		pAc = pGa->elementAt(group);
	}
	
	unlock();

	return pAc;
}

void LtAddressConfigurationTable::rebuildMap()
{
	if (m_bRebuildMap)
	{
		m_bRebuildMap = false;
		m_domainGroups.clear(true);
		for (int i = 0; i < m_count; i++) 
		{
			LtAddressConfiguration* pAc;

			if (get(i, &pAc) == LT_NO_ERROR)
			{
				if (pAc != NULL &&
                    pAc->getAddressType() == LT_AT_GROUP &&
					pAc->getRestrictions() != LT_GRP_OUTPUT_ONLY)
				{
					int dmn = pAc->getDomainIndex();
					LtGroupAddresses* pGroups = m_domainGroups.elementAt(dmn);
					if (pGroups == null)
					{
						pGroups = new LtGroupAddresses();
						m_domainGroups.addElementAt(dmn, pGroups);
					}
					if (pGroups->elementAt(pAc->getGroup()) == null)
					{
						// Note that if a node is in a group multiple times,
						// we use the first one.  The rules for how to behave
						// if the group attributes are mixed is not clear.
						pGroups->addElementAt(pAc->getGroup(), pAc);
					}
				}
			}
		} 
	}
}

// For better performance, would be better if we did fixup of all 
// NV addresses only when a move or changed occurred.
void LtAddressConfigurationTable::fixup(LtOutgoingAddress& oa) 
{
    // If the address is a group address, then determine if we need to adjust
    // the size.  We don't adjust the size field directly since the address
    // object could be used multiple times.  So, we adjust a copy of the size.
    boolean inGroup = false;
    if (oa.getAddressType() == LT_AT_GROUP &&
		oa.getRestrictions() == LT_GRP_NORMAL) 
	{
		LtAddressConfiguration* pAc = get(oa.getDomainIndex(), oa.getGroup());
		// We're in the group for ack purposes if it's a normal group.
		// Noack or output only groups don't count.
		inGroup = pAc && pAc->getRestrictions() == LT_GRP_NORMAL;
    }

    oa.setGroupSize(inGroup);
}

LtErrorType LtAddressConfigurationTable::update(LtAddressConfiguration& newValue) 
{
	LtErrorType err = LT_NO_ERROR;
    // Used to update certain fields of matching entries.  Only
    // valid for groups.
    boolean updated = false;
    
    if (newValue.getAddressType() != LT_AT_GROUP) 
	{
		err = LT_INVALID_PARAMETER;
    }

	if (err == LT_NO_ERROR)
	{
		for (int i = 0; i < m_count; i++) 
		{
			if (m_addr[i] != NULL)
			{
				LtAddressConfiguration& chk = *m_addr[i];
				if (chk.equals(newValue)) 
				{
					// Inherit certain fields.
					chk.setSize(newValue.getSize());
					chk.setRetry(newValue.getRetry());
					chk.setTxTimer(newValue.getTxTimer());
					chk.setRptTimer(newValue.getRptTimer());
					chk.setRcvTimer(newValue.getRcvTimer());
					updated = true;
					break;
				}
			}
		}  

	    if (!updated) 
		{
			err = LT_INVALID_PARAMETER;
		}
	}	
	return err;
}

void LtAddressConfigurationTable::store(byte* data, int &offset) 
{
	int count = 0;
	byte*  p_count_location = &data[offset];
	offset += sizeof(AddressTableStoreHeader);   
    for (int i = 0; i < m_count; i++) 
	{
		if (m_addr[i] != NULL)
		{
			count++;
			data[offset] = (byte) i;
			data[offset+1] = (byte) (i >> 8);
			data[offset+2] = (byte) (i >> 16);
			data[offset+3] = (byte) (i >> 24);
			offset += sizeof(int);    // size of index (i)
			offset += m_addr[i]->toLonTalk(&data[offset], 2);
		}
    }
	// Write out the value of count
	*p_count_location++ = count;
	*p_count_location++ = count >> 8;
	*p_count_location++ = count >> 16;
	*p_count_location = count >> 24;
	
}

int LtAddressConfigurationTable::getStoreSize(int numEntries)
{
	return (numEntries * (LtAddressConfiguration::getStoreSize() + ADDR_EXTRA_STORE_SIZE))
		+ sizeof(AddressTableStoreHeader);
}

int LtAddressConfigurationTable::getMaxStoreSize()
{
	return getStoreSize(m_count);
}

int LtAddressConfigurationTable::getStoreSize()
{
	int count = 0;
	// Count how many addresses need to be stored;
    for (int i = 0; i < m_count; i++) 
	{
		if (m_addr[i] != NULL)
		{
			++count;
		}
    }
	return getStoreSize(count);
}

LtErrorType LtAddressConfigurationTable::initialize(int fromIndex, int toIndex, byte* pData, int len, int domainIndex)
{
	LtErrorType err = LT_NO_ERROR;

	for (int index = fromIndex; err == LT_NO_ERROR && index <= toIndex; index++)
	{
		LtAddressConfiguration ac;

		// Commentary - this is a strange means of making the change since the
		// stack method calls back into this class.  Who should drive this operation,
		// this object or the stack?
		err = getStack()->updateAddressConfiguration(index, &ac);

		if (err == LT_INVALID_ADDR_TABLE_INDEX)
		{
			// Ignore over range errors as toIndex of -1 means all.
			if (toIndex == 0xffff)
			{
				err = LT_NO_ERROR;
			}
			break;
		}
	}

	return err;
}

LtErrorType LtAddressConfigurationTable::enumerate(int index, boolean authenticated, LtApdu &response)
{
	LtErrorType err = LT_END_OF_ENUMERATION;

	for (int i=index; i<m_count; i++)
	{
		LtAddressConfiguration* pAc;
		if (get(i, &pAc) == LT_NO_ERROR &&
            pAc != NULL && 
			pAc->inUse())
		{
			// Found the next address.  Return it.
			byte byData[MAX_APDU_SIZE];
			response.setData(0, i>>8);
			response.setData(1, i&0xff);
			response.setData(byData, 2, pAc->toLonTalk(byData, 2));
			err = LT_NO_ERROR;
			break;
		}
	}

	return err;
}

LtErrorType LtAddressConfigurationTable::update(int index, byte* pData, int len)
{
	LtErrorType err;
    LtAddressConfiguration ac;

	err = ac.fromLonTalk(pData, len, 2);

	if (err == LT_NO_ERROR)
	{
		err = getStack()->updateAddressConfiguration(index, &ac);
	}

	return err;
}

LtErrorType LtAddressConfigurationTable::checkLimits(int cmd, byte* pData, int len)
{
	boolean bOk = true;
	switch (cmd)
	{
	case LT_NM_UPDATE:
	case LT_NM_CREATE:
		bOk = len == 9;
		break;
	default:
		bOk = LtConfigurationEntity::checkLimits(cmd, pData, len) == LT_NO_ERROR;
		break;
	}
	return bOk ? LT_NO_ERROR : LT_INVALID_PARAMETER;
}
