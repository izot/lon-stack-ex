//
// LtAddressConfiguration.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtAddressConfiguration.cpp#2 $
//

#include "LtStackInternal.h"

//
// Private Member Functions
//


//
// Protected Member Functions
//


boolean LtAddressConfiguration::equals(LtAddressConfiguration& cmp) 
{
    boolean matches = m_nAddressType == cmp.m_nAddressType &&
              m_nDomainIndex == cmp.m_nDomainIndex;
    if (matches) 
	{
        switch (m_nAddressType) 
		{
        case LT_AT_GROUP:
            matches = m_nDestId == cmp.m_nDestId &&
					  getRestrictions() == cmp.getRestrictions();
            break;
        case LT_AT_SUBNET_NODE:
            matches = m_nDestId == cmp.m_nDestId && 
                      m_nSubnet == cmp.m_nSubnet;
            break;
        case LT_AT_BROADCAST:
        case LT_AT_BROADCAST_GROUP:
            matches = m_nSubnet == cmp.m_nSubnet;
            break;
        }
    }

    return matches;
}

//
// Public Member Functions
//


LtAddressConfiguration::LtAddressConfiguration() 
{
    init();
}

LtAddressConfiguration::LtAddressConfiguration(int index) 
{
    init();
    m_nIndex = index;
}

void LtAddressConfiguration::copy(LtAddressConfiguration& ac) 
{
	*this = ac;
}

LtAddressConfiguration::LtAddressConfiguration(int type, int domainIndex, int id) {
    init();
    m_nAddressType = type;
    m_nDomainIndex = domainIndex;
    m_nDestId = id;
    m_nSubnet = id;
}

void LtAddressConfiguration::init() 
{
	m_nAddressType = LT_AT_UNBOUND;
    m_nBacklog = 0;
	m_nDomainIndex = 0;
	m_nRetry = 0;
	m_nRptTimer = m_nTxTimer = msToTicks(300);
    m_nRcvTimer = 0;
    m_nMaxResponses = 0;
    m_nIndex = 0;
    m_nDestId = 0;
    m_nSubnet = 0;
    m_nSize = 0;
    m_nMember = 0;
	m_nTxTimerDeltaLast = 0;
	setRestrictions(LT_GRP_NORMAL);
}

void LtAddressConfiguration::package(LtBlob *pBlob)
{
    pBlob->package(&m_nTxTimer);
    pBlob->package(&m_nRptTimer);
    pBlob->package(&m_nRcvTimer);
    pBlob->package(&m_nRetry);
    pBlob->package(&m_nAddressType);
    pBlob->package(&m_nMaxResponses);
    pBlob->package(&m_nIndex);
    pBlob->package(&m_nSubnet);
    pBlob->package(&m_nDomainIndex);
    pBlob->package(&m_nDestId);
    pBlob->package(&m_nSize);
    pBlob->package(&m_nMember);
    pBlob->package(&m_nBacklog);
	pBlob->package(&m_nRestrictions);
	pBlob->package(&m_nTxTimerDeltaLast);
}

boolean LtAddressConfiguration::isBound() {
    return m_nAddressType != LT_AT_UNBOUND;
}

boolean LtAddressConfiguration::isBoundExternally() {
    return m_nAddressType != LT_AT_UNBOUND && m_nAddressType != LT_AT_LOCAL &&
		m_nAddressType != LT_AT_TURNAROUND_ONLY;
}

int LtAddressConfiguration::toLonTalk(byte* data, int version)
{
	int length = LT_ADDRESS_STORE_SIZE;

    data[2] = LtMisc::fromTxTimer(m_nRptTimer)<<4 | m_nRetry;
    data[3] = LtMisc::fromRcvTimer(m_nRcvTimer)<<4 | LtMisc::fromTxTimer(m_nTxTimer);
	switch (m_nAddressType)
	{
	case LT_AT_UNBOUND:
		data[0] = LT_AT_UNBOUND;
		data[1] = 0;
		data[4] = 0;
		break;
	case LT_AT_TURNAROUND_ONLY:
		data[0] = LT_AT_UNBOUND;
		data[1] = 1;
		data[4] = 0;
		break;
	case LT_AT_SUBNET_NODE:
		data[0] = LT_AT_SUBNET_NODE;
		data[1] = m_nDestId;
		data[4] = m_nSubnet;
		break;
	case LT_AT_GROUP:
		data[0] = (m_nSize | 0x80);
		data[1] = m_nMember & 0x3f;
		data[4] = m_nDestId;
		break;
	case LT_AT_BROADCAST:
	case LT_AT_BROADCAST_GROUP:
		data[0] = m_nAddressType;
		data[1] = m_nBacklog;
		data[3] = (m_nMaxResponses<<4) | LtMisc::fromTxTimer(m_nTxTimer);
		data[4] = m_nSubnet;
		break;
	case LT_AT_UNIQUE_ID:
		data[0] = LT_AT_UNIQUE_ID;
		data[4] = m_nSubnet;
		// We don't have a unique ID in this object.  Must be set by subclass.
		break;
	default:
		assert(0);
		break;
    }

	if (version == 1)
	{
		if (data[0] != LT_AT_UNBOUND && m_nDomainIndex)
		{
			data[1] |= 0x80;
		}
		length -= 2;
	}
	else
	{
		if (m_nAddressType == LT_AT_GROUP)
		{
			data[1] |= (getRestrictions() << 6);
		}
		data[5] = (byte)(m_nDomainIndex>>8);
		data[6] = (byte)(m_nDomainIndex&0xff);
	}

    return length;
}

LtErrorType LtAddressConfiguration::fromLonTalk(byte data[], int& len, int version) 
{
	LtErrorType err = LT_NO_ERROR;
    m_nAddressType = (data[0] & 0x80) ? LT_AT_GROUP : data[0];
    if ((m_nAddressType < 0 || m_nAddressType > LT_AT_BROADCAST) && 
            (m_nAddressType != LT_AT_BROADCAST_GROUP &&
             m_nAddressType != LT_AT_GROUP)) 
	{
		err = LT_BAD_ADDRESS_TYPE;
    }
	else
	{
		m_nSize = data[0] & 0x7f;
		m_nMember = data[1] & 0x3f;
		setRestrictions(LT_GRP_NORMAL);
		m_nBacklog = m_nMember;
		m_nDomainIndex = (data[1] & 0x80) >> 7;
		m_nRptTimer = LtMisc::toTxTimer((data[2] & 0xf0)>>4);
		m_nTxTimer = LtMisc::toTxTimer(data[3] & 0x0f);
		m_nMaxResponses = (data[3] & 0x70) >> 4;
		m_nRcvTimer = LtMisc::toRcvTimer((data[3] & 0xf0) >> 4);
		m_nRetry = data[2] & 0x0f;
		m_nDestId = data[4] & 0xff;
		m_nSubnet = m_nDestId;
		if (m_nAddressType == LT_AT_SUBNET_NODE) 
		{
			m_nDestId = data[1] & 0x7f;
		}
		if (m_nAddressType == LT_AT_UNBOUND && data[1] == 1)
		{
			m_nAddressType = LT_AT_TURNAROUND_ONLY;
		}
		m_nTxTimerDeltaLast = 0;
	}
	len = LT_ADDRESS_STORE_SIZE;
	if (version == 1)
	{
		len -= 2;
	}
	else
	{
		setRestrictions(data[1] >> 6);
		m_nDomainIndex = LtMisc::makeint(data[5], data[6]);
	}
	return err;
}
