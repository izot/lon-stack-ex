//
// LtDomainConfigurationTable.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtDomainConfigurationTable.cpp#2 $
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


LtDomainConfigurationTable::LtDomainConfigurationTable() 
{
	m_nCount = 0;
	m_dmn = null;
}

LtDomainConfigurationTable::~LtDomainConfigurationTable()
{
    for (int i = 0; i < m_nCount; i++)
    {
        delete m_dmn[i];
    }
    delete m_dmn;
}

void LtDomainConfigurationTable::setCount(LtDeviceStack* pStack, int count) 
{
	m_pStack = pStack;
	m_dmn = new LtDomainConfiguration*[count];
    for (int i = 0; i < count; i++)
    {
        m_dmn[i] = getStack()->allocDomainConfiguration(i);
    }
	m_nCount = count;
}

int LtDomainConfigurationTable::getCount() 
{
    return m_nCount;
}

LtErrorType LtDomainConfigurationTable::get(int index, LtDomainConfiguration** ppDc) 
{
	LtErrorType err = LT_NO_ERROR;
	*ppDc = null;
	if (index >= m_nCount) 
	{
		err = LT_INVALID_DOMAIN;
    }
	else if (index == FLEX_DOMAIN_INDEX)
	{
		err = LT_FLEX_DOMAIN;
	}
	else
	{
		*ppDc = m_dmn[index];
	}
	return err;
}

LtDomainConfiguration* LtDomainConfigurationTable::get(LtDomainConfiguration* match) 
{
    return get(&match->getDomain());
}

LtDomainConfiguration* LtDomainConfigurationTable::get(LtDomain* match) 
{
    LtDomainConfiguration* dc = null;

    // Find a matching LtDomain and return it.
    for (int i = 0; i < m_nCount; i++) {
        LtDomainConfiguration* chk = m_dmn[i];
        if (chk->getDomain() == *match) {
            dc = chk;
            break;
        }
    }

    return dc;
}

LtErrorType LtDomainConfigurationTable::set(int index, LtDomainConfiguration* dc) 
{
	LtErrorType err = LT_NO_ERROR;
	if (index >= m_nCount) 
	{
		err = LT_INVALID_DOMAIN;
	}
	else
	{
		dc->setIndex(index);
		*m_dmn[index] = *dc;
	}
	return err;
}

void LtDomainConfigurationTable::store(byte* data, int &offset) 
{ 
    for (int i = 0; i < m_nCount; i++) 
	{
        offset += m_dmn[i]->toLonTalk(&data[offset], LT_OMA_DOMAIN_STYLE);
    }
}

int LtDomainConfigurationTable::getMaxStoreSize()
{
	return m_nCount * LtDomainConfiguration::getStoreSize();
}

int LtDomainConfigurationTable::getStoreSize()
{
	return getMaxStoreSize();
}

// LtConfigurationEntity methods
LtErrorType LtDomainConfigurationTable::initialize(int fromIndex, int toIndex, byte* pData, int len, int domainIndex)
{
	LtErrorType err = LT_NO_ERROR;

	// Sets all domain entries to the "empty" value.  Essentially does
	// a leave domain on each entry.
	for (int index = fromIndex; err == LT_NO_ERROR && index <= toIndex; index++)
	{
		LtDomainConfiguration dc;

		// Commentary - this is a strange means of making the change since the
		// stack method calls back into this class.  Who should drive this operation,
		// this object or the stack?
		err = getStack()->updateDomainConfiguration(index, &dc);

		if (err == LT_INVALID_DOMAIN)
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

void LtDomainConfigurationTable::goUnconfiguredConditional()
{
	// If not in any domains, go unconfigured
	int i=0;
	boolean validDomain = false;

	while (true) 
	{
		LtDomainConfiguration* pDc;
		if (get(i++, &pDc) == LT_INVALID_DOMAIN)
		{
			break;
		}
		if (pDc->getDomain().inUse()) 
		{
			validDomain = true;
		}
	}
	if (!validDomain) 
	{
		getStack()->goUnconfigured();
	}
}

LtErrorType LtDomainConfigurationTable::enumerate(int index, boolean authenticated, LtApdu &response)
{
	LtErrorType err = enumerate(index, response, LT_CLASSIC_DOMAIN_STYLE);
    if (err == LT_NO_ERROR && authenticated)
    {
        static_cast<LtApduOut&>(response).setDownlinkEncryption();
    }
    return(err);
}

LtErrorType LtDomainConfigurationTable::update(int index, byte* pData, int len)
{
	return update(index, index, pData, len, LT_CLASSIC_DOMAIN_STYLE);
}

LtErrorType LtDomainConfigurationTable::resourceSpecificCommand(int cmd, int index, byte* pData, int len, boolean authenticated, LtApdu &response)
{
	LtErrorType err = LT_NOT_IMPLEMENTED;
    boolean store = FALSE;
	switch (cmd)
    {
      case LT_NM_SET_AUTH:
          err = setAuthKey(index, pData, LT_CLASSIC_DOMAIN_KEY_LENGTH);
          store = TRUE;
        break;

      case LT_NM_UPDATE_NO_KEY:
          int indexOfKey;

          PTOHS(pData, indexOfKey);
          len -= 2;

	      err = update(index, indexOfKey, pData, len, LT_KEYLESS_DOMAIN_STYLE);
          store = TRUE;
        break;

      case LT_NM_ENUMERATE_NO_KEY:
        err = enumerate(index, response, LT_KEYLESS_DOMAIN_STYLE);
        break;

      case LT_NM_ENUMERATE_AUTH_OMA:
        err = enumerateAuth(index, response);
        if (err == LT_NO_ERROR && authenticated)
        {
            static_cast<LtApduOut&>(response).setDownlinkEncryption();
        }
        break;

      case LT_NM_SET_AUTH_OMA:
          err = setAuthKey(index, pData, LT_OMA_DOMAIN_KEY_LENGTH);
          store = TRUE;
        break;
	}

    if (err == LT_NO_ERROR && store)
    {
        getStack()->getNetworkImage()->setHasBeenEcsChanged(true);
		if (!getStack()->getNetworkImage()->store())
		{
			err = LT_EEPROM_WRITE_FAILURE;
		}
    }
	return err;
}

LtErrorType LtDomainConfigurationTable::checkLimits(int cmd, byte* pData, int len)
{
	boolean bOk = true;
	switch (cmd)
	{
	case LT_NM_SET_AUTH:
		bOk = len == 9;
		break;
    case LT_NM_UPDATE_NO_KEY:
		bOk = len == 13;
        break;
    case LT_NM_ENUMERATE_NO_KEY:
		bOk = len == 2;
        break;
    case LT_NM_SET_AUTH_OMA:
		bOk = len == 15;
        break;
    case LT_NM_INITIALIZE:
        bOk = len >= 4;
        break;

    case LT_NM_ENUMERATE_AUTH_OMA:
		bOk = len == 2;
        break;
	case LT_NM_UPDATE:
	case LT_NM_CREATE:
		bOk = len == 17;
		break;
	default:
		bOk = LtConfigurationEntity::checkLimits(cmd, pData, len) == LT_NO_ERROR;
		break;
	}
	return bOk ? LT_NO_ERROR : LT_INVALID_PARAMETER;
}

// Utility  methods to support LtConfigurationEntity
LtErrorType LtDomainConfigurationTable::enumerate(int index, LtApdu &response,
                                                  LtLonTalkDomainStyle domainStyle)
{
	LtErrorType err = LT_END_OF_ENUMERATION;

	for (int i=index; i<m_nCount; i++)
	{
		LtDomainConfiguration* pDc;
		if (get(i, &pDc) == LT_NO_ERROR && pDc->getDomain().isValid())
		{
			// Found the next domain.  Return it.
			byte byData[MAX_APDU_SIZE];
			response.setData(0, i>>8);
			response.setData(1, i&0xff);
			response.setData(byData, 2, pDc->toLonTalk(byData, domainStyle));

			err = LT_NO_ERROR;
			break;
		}
	}

	return err;
}

// Return the domain configuration of the domain to use for authenticating the 
// flex domain. This is the first valid domain, if any, or domain index 0, if not.
LtErrorType LtDomainConfigurationTable::getFlexAuthDomain(LtDomainConfiguration** ppDc)
{
	LtErrorType err = LT_END_OF_ENUMERATION;

	for (int i=0; i<m_nCount; i++)
	{
		LtDomainConfiguration* pDc;
		if (get(i, &pDc) == LT_NO_ERROR && pDc->getDomain().isValid())
		{
            *ppDc = pDc;
			err = LT_NO_ERROR;
			break;
		}
	}

    if (err == LT_END_OF_ENUMERATION)
    {
        err = get(0, ppDc);
    }

	return err;
}

LtErrorType LtDomainConfigurationTable::update(int index, int indexOfKey, byte* pData, int len, 
                                               LtLonTalkDomainStyle domainStyle)
{
    LtDomainConfiguration dc;

	LtErrorType err = LT_NO_ERROR;
    if (len != BASE_DOMAIN_STORE_SIZE + dc.getKeyLenForStyle(domainStyle))
    {
        err = LT_INVALID_PARAMETER;
    }
    else
    {
        err = dc.fromLonTalk(pData, len, domainStyle);
    }
    if (err == LT_NO_ERROR)
    {
        if (domainStyle == LT_KEYLESS_DOMAIN_STYLE)
        {
            LtDomainConfiguration* pCurrentDc;
            if (get(indexOfKey, &pCurrentDc) == LT_NO_ERROR)
            {   // Copy key from current configuration, if any.
                dc.setKey(pCurrentDc->getKey());
            }
        }
        else if (domainStyle == LT_CLASSIC_DOMAIN_STYLE)
        {   // Special processing for OMA when using "classic" domain commands involving
            // domain 0 or domain 1.
            // Classic commands dictate that 
            // a) OMA keys are used for both domain 0 and 1 IFF domain zero's OMA bit is set.
            // b) If OMA is set, both domain 0 and domain use the same key, and it is 
            //    formed by concatinating the key from domain 0 and domain 1.
            //    
            LtDomainConfiguration *pDc0 = NULL;
            LtDomainConfiguration *pDc1 = NULL;
            if (index == 0)
            {
                if (get(1, &pDc1) == LT_NO_ERROR)
                {
                    pDc0 = &dc;
                }
            }
            else if (index == 1)
            {
                if (get(0, &pDc0) == LT_NO_ERROR)
                {
                    pDc1 = &dc;
                }
            }
            if (pDc0 && pDc1)
            {   // Both exist. 
                byte key[LT_OMA_DOMAIN_KEY_LENGTH];
                const byte *pDc1Key = pDc1->getKey();  // Point to the "48 bit key" in domain 1.
                if (pDc1->getUseOma())
                {   // Its the second half of the real key.
                    pDc1Key += LT_CLASSIC_DOMAIN_KEY_LENGTH;
                }
                pDc1->setUseOma(pDc0->getUseOma());
                if (pDc0->getUseOma())
                {   // Both domains use the same key - form it.
                    memcpy(key, pDc0->getKey(), LT_CLASSIC_DOMAIN_KEY_LENGTH);
                    memcpy(&key[LT_CLASSIC_DOMAIN_KEY_LENGTH], pDc1Key, LT_CLASSIC_DOMAIN_KEY_LENGTH);
                    pDc0->setKey(key);
                    pDc1->setKey(key);
                }
                else
                {   // The two keys are now independent.
                    memset(key, 0, sizeof(LT_OMA_DOMAIN_KEY_LENGTH));
                    memcpy(key, pDc0->getKey(), LT_CLASSIC_DOMAIN_KEY_LENGTH);
                    pDc0->setKey(key);
                    memset(key, 0, sizeof(LT_OMA_DOMAIN_KEY_LENGTH));
                    memcpy(key, pDc1Key, LT_CLASSIC_DOMAIN_KEY_LENGTH);
                    pDc1->setKey(key);
                }
                if (index == 0)
                {
		            err = getStack()->updateDomainConfiguration(1, pDc1);
                }
                else
                {
                    err = getStack()->updateDomainConfiguration(0, pDc0);
                }
            }
        }
    }

	if (err == LT_NO_ERROR)
	{
		err = getStack()->updateDomainConfiguration(index, &dc);
	}
    return err;
}

LtErrorType LtDomainConfigurationTable::enumerateAuth(int index, LtApdu &response)
{
	LtErrorType err = LT_END_OF_ENUMERATION;

	for (int i=index; i<m_nCount; i++)
	{
		LtDomainConfiguration* pDc;
		if (get(i, &pDc) == LT_NO_ERROR &&
			pDc->getDomain().isValid())
		{
			// Found the next domain.  Return it.
			response.setData(0, i>>8);
			response.setData(1, i&0xff);
            response.setData(2, pDc->getUseOma());
			response.setData(pDc->getKey(), 3, LT_OMA_DOMAIN_KEY_LENGTH);
			err = LT_NO_ERROR;
			break;
		}
	}

	return err;
}

LtErrorType LtDomainConfigurationTable::setAuthKey(int index, byte* pData, int keyLen)
{
	LtErrorType err = LT_NO_ERROR;
	byte key[LT_OMA_DOMAIN_KEY_LENGTH];
	int startIndex = index;
	int endIndex = index;
	boolean bIncrement = (pData[0]&0x80) == 0x80;
	pData++;

	if (index == NM_MAX_INDEX)
	{
		startIndex = 0;
		endIndex = m_nCount-1;
	}

    memset(key, 0, sizeof(key));
    memcpy(key, pData, min((int)sizeof(key), keyLen));

	for (int i = startIndex; i <= endIndex && (err == LT_NO_ERROR); i++)
	{
		LtDomainConfiguration *pDc;
		if (get(i, &pDc) == LT_NO_ERROR)
		{
			pDc->updateKey(bIncrement, key);
            err = getStack()->updateDomainConfiguration(i, pDc);
		}
	}
    return err;
}

