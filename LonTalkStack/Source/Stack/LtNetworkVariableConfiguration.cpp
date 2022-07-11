//
// LtNetworkVariableConfigurationBase.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtNetworkVariableConfiguration.cpp#3 $
//

#include "LtStackInternal.h"

//
// Private Member Functions
//

// This lock is not a static member to avoid the need for vxclock.h by users of ltnetworkvariableconfiguration.h
// (such as the VniClient files).  It is a pointer to the lock, instead of a VxcLock, because there
// appears to be a problem instantiation a static VxcLock in some environments (like loading an OCX under
// WinMe).  The problem probably occurs during the InitializeCriticalSection portion of the VxcLock constructor.
// See EPR 18953.
static VxcLock *m_pLock = NULL;

LtNetworkVariableConfigurationBase::LtNetworkVariableConfigurationBase(int nIndex, int nType, int x) 
{
    m_nIncarnationNumber = 0;
    init(nIndex, LT_UNUSED_INDEX, nType);
}

LtNetworkVariableConfigurationBase::LtNetworkVariableConfigurationBase() 
{
    m_nIncarnationNumber = 0;
	init(0, LT_UNUSED_INDEX, NV_TABLE_NVS);
}

LtNetworkVariableConfigurationBase::~LtNetworkVariableConfigurationBase()
{
	delete m_pAliases;
}

void LtNetworkVariableConfigurationBase::package(LtBlob *pBlob) 
{
    LtStackBlob stackBlob(pBlob);
	pBlob->PACKAGE_VALUE(m_bIsAlias);
	pBlob->PACKAGE_VALUE(m_bPriority);
	pBlob->PACKAGE_VALUE(m_bTurnaround);
	pBlob->PACKAGE_VALUE(m_bAuthenticated);
	pBlob->PACKAGE_VALUE(m_bOutput);
	pBlob->PACKAGE_VALUE(m_bReadByIndex);
	pBlob->PACKAGE_VALUE(m_bWriteByIndex);
	pBlob->PACKAGE_VALUE(m_bRemoteNmAuth);
	pBlob->PACKAGE_VALUE(m_bSourceSelectionOnly);

	pBlob->PACKAGE_VALUE(m_nNvUpdateSelection);
	pBlob->PACKAGE_VALUE(m_nNvResponseSelection);
	pBlob->PACKAGE_VALUE(m_nIndex);
	pBlob->PACKAGE_VALUE(m_nAddressTableIndex);
	pBlob->PACKAGE_VALUE(m_nNvIndex);
	pBlob->PACKAGE_VALUE(m_nSelector);
	pBlob->PACKAGE_VALUE(m_nPrimaryIndex);
    pBlob->PACKAGE_VALUE(m_serviceType);
    stackBlob.package(&m_address);
}

boolean LtNetworkVariableConfigurationBase::isBound(int flags) 
{
	// EIA definition is selector is bound OR address index is bound.  However, if
	// the flags do not specify polled (as might be requested by an application), 
	// then constrain to actively bound (output that can send or input that can 
	// receive updates).
	boolean bIsBound = m_nSelector <= LT_MAX_BOUND_SELECTOR;

	if (!(flags & ISBOUND_POLLED))
	{
		if (getOutput())
		{
			bIsBound &= hasAddress();
		}
		else
		{
			bIsBound &= (m_nNvUpdateSelection != LT_SELECTION_NEVER);
		}
	}
	else
	{
		bIsBound |= hasAddress();
	}

	return bIsBound;
}

boolean LtNetworkVariableConfigurationBase::inUse() 
{
    return (!m_bIsAlias && isBound()) || 
		   (m_bIsAlias && m_nPrimaryIndex != LT_UNUSED_ALIAS);
}

//
// Protected Member Functions
//


//
// Public Member Functions
//
boolean LtNetworkVariableConfigurationBase::unboundSelector(int selector)
{
	return selector != LT_UNUSED_SELECTOR && selector > LT_MAX_BOUND_SELECTOR;
}

boolean LtNetworkVariableConfigurationBase::unboundSelectorToIndex(int selector, int& index)
{
	index = LT_MAX_SELECTOR - selector;
	return unboundSelector(selector);
}

boolean LtNetworkVariableConfigurationBase::unboundSelectorForIndex(int selector, int index)
{
	return computeSelector(index) == selector;
}

int LtNetworkVariableConfigurationBase::computeSelector(int index)
{
    int selector;
	if (index >= LT_MAX_SELECTOR - LT_MAX_BOUND_SELECTOR)
	{
		selector = LT_UNUSED_SELECTOR;
	}
	else
	{
		selector = LT_MAX_SELECTOR - index;
	}
    return(selector);
}

void LtNetworkVariableConfigurationBase::initSelector()
{
	if (isAlias())
	{
		m_nSelector = LT_MAX_BOUND_SELECTOR + 1;
	}
	else
	{
		m_nSelector = computeSelector(m_nIndex);
	}
}

void LtNetworkVariableConfigurationBase::init(int nType) 
{
    if (m_pLock == NULL)
    {
        m_pLock = new VxcLock;
    }
    m_bPriority = false;
    m_bTurnaround = false;
    m_bAuthenticated = false;
    m_bOutput = m_bIsAlias;
	m_bWriteByIndex = false;
	m_bReadByIndex = false;
	m_bRemoteNmAuth = false;
	m_nNvUpdateSelection = nType == NV_TABLE_PRIVATES ? LT_SELECTION_NEVER : LT_SELECTION_UNCONDITIONAL;
	m_nNvRequestSelection = m_nNvResponseSelection = m_nNvUpdateSelection;
	m_nNvIndex = LT_UNUSED_INDEX;
    m_serviceType = LT_ACKD;
    m_nAddressTableIndex = LT_UNBOUND_INDEX;
	m_pAliases = null;
	m_bSourceSelectionOnly = false;
	initSelector();
}

void LtNetworkVariableConfigurationBase::init(int nIndex, int nPrimary, int nType) 
{
    m_bIsAlias = nType == NV_TABLE_ALIASES;
    m_nIndex = nIndex;
	m_nPrimaryIndex = nPrimary;
    init(nType);
}

void LtNetworkVariableConfigurationBase::initialize()
{
    m_bTurnaround = false;
	m_bWriteByIndex = false;
	m_bRemoteNmAuth = false;
	m_nNvUpdateSelection = LT_SELECTION_UNCONDITIONAL;
	m_nNvResponseSelection = LT_SELECTION_UNCONDITIONAL;
	m_nNvRequestSelection = LT_SELECTION_UNCONDITIONAL;
	m_nNvIndex = LT_UNUSED_INDEX;
    m_nAddressTableIndex = LT_UNBOUND_INDEX;
	initSelector();
	if (m_bIsAlias)
	{
		m_bPriority = false;
		m_bOutput = false;
		m_serviceType = LT_ACKD;
		m_bAuthenticated = false;
		m_nPrimaryIndex  = LT_UNUSED_INDEX;
	}
}

boolean LtNetworkVariableConfigurationBase::hasAddress() 
{
    return m_nAddressTableIndex != LT_UNBOUND_INDEX;
}

boolean LtNetworkVariableConfigurationBase::hasOutputAddress()
{
    return hasAddress() && !m_bSourceSelectionOnly;
}

int LtNetworkVariableConfigurationBase::getPrimary() 
{
    return m_nPrimaryIndex;
}

int LtNetworkVariableConfigurationBase::toLonTalk(byte* data, boolean bExact, int nVersion, bool expanded) 
{
    // This function compacts the data into the format
    // required by LonTalk Network Management messages
	int nLen = 0;
	int selector = min(m_nSelector, LT_MAX_SELECTOR);
    data[nLen++] = ((m_bPriority ? 0x80 : 0x00) |
                (m_bOutput ? 0x40 : 0x00) |
                (selector & 0x3f00) >> 8);
    data[nLen++] = selector;
	
	if (nVersion < NetImageVer_ECS)
	{
        int addrTableIndex;
        if (!expanded && m_nAddressTableIndex >= 0x0f)
        {
            // legacy LonTalk representation caps the address index 15.
            addrTableIndex = 0x0f;
        }
        else
        {
            addrTableIndex = m_nAddressTableIndex;
        }

	    data[nLen++] = (m_bTurnaround ? 0x80 : 0x00) |
		               (m_serviceType << 5) |
			           (m_bAuthenticated ? 0x10 : 0x00) |
				       (addrTableIndex & 0xf);
        if (expanded)
        {
            // Append upper part of address table index as uper part of address
            data[nLen++] = addrTableIndex & 0xf0;  
        }
		if (m_bIsAlias) 
		{
			// Include primary value
            if (expanded)
            {
				data[nLen++] = (m_nPrimaryIndex >> 8);
				data[nLen++] = m_nPrimaryIndex;
            }
			else if (m_nPrimaryIndex == LT_UNUSED_INDEX)
			{
				data[nLen++] = LT_MAX_INDEX;
			}
			else if (m_nPrimaryIndex >= LT_MAX_INDEX) 
			{
				data[nLen++] = LT_ESCAPE_INDEX;
				data[nLen++] = (m_nPrimaryIndex >> 8);
				data[nLen++] = m_nPrimaryIndex;
			} 
			else 
			{
				data[nLen++] = m_nPrimaryIndex;
			}
		}
	}
	else
	{
		data[nLen++] = (m_bTurnaround ? 0x80 : 0x00) |
			   	  (m_bAuthenticated ? 0x40 : 0x00) |
				  (m_bWriteByIndex ? 0x20 : 0x00) |
				  (m_bRemoteNmAuth ? 0x10 : 0x00) |
				  ((m_nNvResponseSelection & 0x03) << 2);
		data[nLen++] = (byte) ((m_bReadByIndex ? 0x80 : 0x00) |
						  ((m_serviceType & 0x03) << 5) |
						  ((m_nNvRequestSelection & 0x03) << 3) |
						  ((m_nNvUpdateSelection & 0x03) << 1) |
						  (m_bSourceSelectionOnly ? 0x01 : 0x00));
		data[nLen++] = (byte) (m_nAddressTableIndex >> 8);
		data[nLen++] = (byte) (m_nAddressTableIndex & 0xff);
		data[nLen++] = (byte) (m_nNvIndex >> 8);
		data[nLen++] = (byte) (m_nNvIndex & 0xff);
		if (m_bIsAlias)
		{
			data[nLen++] = (byte) (m_nPrimaryIndex >> 8);
			data[nLen++] = (byte) (m_nPrimaryIndex & 0xff);
		}
	}

    return bExact ? nLen : LT_NV_STORE_SIZE;
}

int LtNetworkVariableConfigurationBase::fromLonTalk(byte data[], int length, int nVersion, bool expanded) 
{
    m_bPriority = (data[0] & 0x80) == 0x80;
    m_bOutput = (data[0] & 0x40) == 0x40;
    m_nSelector = LtMisc::makeint((byte)(data[0] & 0x3f), data[1]);
    m_bTurnaround = (data[2] & 0x80) == 0x80;

	if (nVersion < NetImageVer_ECS)
	{
		m_bSourceSelectionOnly = false;
	    m_serviceType = (LtServiceType) (m_bOutput ? ((data[2] & 0x60) >> 5) : 0);
		m_bAuthenticated = (data[2] & 0x10) == 0x10;
		m_nAddressTableIndex = (int) (data[2] & 0x0f);
        if (expanded)
        {
            m_nAddressTableIndex |= (int) (data[3] & 0xf0);
            if (m_nAddressTableIndex == 0xff)
            {
			    m_nAddressTableIndex = LT_UNBOUND_INDEX;
            }
        }
		else if (m_nAddressTableIndex == 0xf)
		{
			m_nAddressTableIndex = LT_UNBOUND_INDEX;
		}
		if (m_bIsAlias) 
		{
			// Include primary value
            if (expanded)
            {
                m_nPrimaryIndex = LtMisc::makeint(data[4], data[5]);
            }
			else if (data[3] == LT_ESCAPE_INDEX)
			{
				if (length == 0 || length > 4)
				{
					m_nPrimaryIndex = LtMisc::makeint(data[4], data[5]);
				}
				else
				{
					m_nPrimaryIndex = LT_UNUSED_INDEX;
				}
			} 
			else
			{
				m_nPrimaryIndex = LtMisc::makeint((byte)0, data[3]);
			}
		}
	}
	else
	{
		m_bAuthenticated = (data[2] & 0x40) == 0x40;
		m_bWriteByIndex = (data[2] & 0x20) == 0x20;
		m_bRemoteNmAuth = (data[2] & 0x10) == 0x10;
		m_nNvResponseSelection = (data[2] >> 2) & 0x3;
		m_bReadByIndex = (data[3] & 0x80) == 0x80;
	    m_serviceType = (LtServiceType) ((data[3] >> 5) & 0x3);
		m_nNvRequestSelection = (data[3] >> 3) & 0x3;
		m_nNvUpdateSelection = (data[3] >> 1) & 0x3;
		m_bSourceSelectionOnly = (data[3] & 0x01) == 0x01;
		m_nAddressTableIndex = LtMisc::makeint(data[4], data[5]);
		m_nNvIndex = LtMisc::makeint(data[6], data[7]);
		if (m_bIsAlias)
		{
			m_nPrimaryIndex = LtMisc::makeint(data[8], data[9]);
		}
	}

    if (m_nSelector == LT_MAX_SELECTOR && !hasAddress())
	{   // The unbound selector for all NVs > 4096 is LT_MAX_SELECTOR, but the stack
        // translates this into -1.  But don't do this if the NV has an address - in that
        // case its clearly not an "unbound" NV - its an NV that is monitoring an 
        // unbound NV whose index 0. 
		initSelector();
	}

    return LT_NV_STORE_SIZE;
}

void LtNetworkVariableConfigurationBase::addAlias(int index)
{
	m_pLock->lock();
	if (m_pAliases == null)
	{
		m_pAliases = new LtVector();
	}
	m_pAliases->addElement((LtObject*)index);
	m_pLock->unlock();
}

void LtNetworkVariableConfigurationBase::removeAlias(int index)
{
	m_pLock->lock();
	if (m_pAliases != null)
	{
		m_pAliases->removeElement((LtObject*)index);
	}
	m_pLock->unlock();
}

boolean LtNetworkVariableConfigurationBase::nextAlias(LtVectorPos &pos, int &index)
{
	boolean bResult = false;
	m_pLock->lock();
	if (m_pAliases != null)
	{
		LtObject* p;
		bResult = m_pAliases->getElement(pos, &p);
		index = (int)p;
	}
	m_pLock->unlock();
	return bResult;
}


void LtNetworkVariableConfigurationBase::incrementIncarnation()
{
    ++m_nIncarnationNumber;
}

#include "vxlTarget.h"
boolean LtNetworkVariableConfigurationBase::incarnationMatches(int incarnation)
{
    if (incarnation == m_nIncarnationNumber)
    {
        return TRUE;
    }
    vxlReportEvent("Bad NV Incarnation Number for index %d.  Got %d, expected %d\n", 
                   m_nIndex, incarnation, m_nIncarnationNumber);
    return FALSE;
}

void LtNetworkVariableConfigurationBase::ltShutdown(void)
{
    delete m_pLock;
    m_pLock = NULL;
}

