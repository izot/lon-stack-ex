//
// LtDomainConfiguration.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtDomainConfiguration.cpp#2 $
//

#include "LtStack.h"

/**
 * This class defines a domain configuration which includes domain ID, subnet
 * and node address. 
 *
 */

//
// Private Member Functions
//
byte LtDomainConfiguration::m_byNullKey[LT_OMA_DOMAIN_KEY_LENGTH] = {0,0,0,0,0,0,0,0,0,0,0,0};


//
// Protected Member Functions
//

LtDomainConfiguration::LtDomainConfiguration(int index) 
{
    m_nIndex = index;
    m_bClone = false;
    m_bUseOma = false;
    m_bUseLsEnhancedMode = false;
    m_bEnableDhcp = false;
	setKey(null);
}

LtDomainConfiguration::LtDomainConfiguration(int index, LtDomain& domain, byte* pKey, 
                                             int subnet, int node, boolean useOma) 
{
    m_nIndex = index;
    m_domain = domain;
    setAddress(subnet, node);
    m_bUseOma = useOma;
	setKey(pKey);
    m_bClone = false;
    m_bUseLsEnhancedMode = false;
    m_bEnableDhcp = false;
}

void LtDomainConfiguration::setAddress(int subnet, int node) 
{
    m_subnetNode.set(subnet, node);
}

int LtDomainConfiguration::getIndex() 
{
	return m_nIndex;
}

void LtDomainConfiguration::setIndex(int index) 
{
    m_nIndex = index;
}

byte* LtDomainConfiguration::getKey() 
{
	return m_byKey;
}

void LtDomainConfiguration::setKey(byte* pKey)
{
	if (pKey == null)
	{
		pKey = m_byNullKey;
	}
    if (m_bUseOma)
    {
	    memcpy(m_byKey, pKey, sizeof(m_byKey));
    }
    else
    {
	    memcpy(m_byKey, pKey, LT_CLASSIC_DOMAIN_KEY_LENGTH);
        memcpy(&m_byKey[LT_CLASSIC_DOMAIN_KEY_LENGTH], 
               &m_byNullKey[LT_CLASSIC_DOMAIN_KEY_LENGTH], 
               sizeof(m_byKey)-LT_CLASSIC_DOMAIN_KEY_LENGTH);
    }
}

void LtDomainConfiguration::updateKey(boolean bIncrement, byte* pData)
{
    int length = m_bUseOma ? sizeof(m_byKey) : LT_CLASSIC_DOMAIN_KEY_LENGTH;
	for (int i = 0; i < length; i++)
	{
		if (bIncrement)
		{
			m_byKey[i] += pData[i];
		}
		else
		{
			m_byKey[i] = pData[i];
		}
	}
}

boolean LtDomainConfiguration::equals(LtDomainConfiguration& match, boolean includeAddress) 
{
    return m_domain == match.m_domain && 
           (!includeAddress || equals(match.m_subnetNode)) &&
           isFlexDomain() == match.isFlexDomain();
}

boolean LtDomainConfiguration::equals(LtSubnetNode& sn) {
	return m_subnetNode == sn && !m_bClone;
}

int LtDomainConfiguration::getKeyLenForStyle(LtLonTalkDomainStyle style)
{
    int keyLen = 0;
    switch (style)
    {
      case LT_CLASSIC_DOMAIN_STYLE:
        keyLen = LT_CLASSIC_DOMAIN_KEY_LENGTH;
        break;

      case LT_KEYLESS_DOMAIN_STYLE:
        keyLen = 0;
        break;

      case LT_OMA_DOMAIN_STYLE:
        keyLen = LT_OMA_DOMAIN_KEY_LENGTH;
        break;
    }
    return(keyLen);
}

LtErrorType LtDomainConfiguration::fromLonTalk(byte data[], int& length, LtLonTalkDomainStyle style) {

	LtErrorType err = LT_NO_ERROR;
    int keyLen = getKeyLenForStyle(style);
    length = BASE_DOMAIN_STORE_SIZE + keyLen;

    // Parse length byte
    int lengthField = data[8] & 0x7;
    int omaField = (data[8] >> 3) & 0x3;
    int useDhcp = (data[8] == 0xff) ? 0 : (data[8] >> 5) & 1;
    int useLsEnhacedMode = (data[8] == 0xff) ? 0 : (data[8] >> 6) & 1;
    int invalidField = (data[8] >> 7) & 1;
    int len = invalidField ? LT_DOMAIN_EMPTY : lengthField;

    if (len != 0 && len != 1 && len != 3 && len != 6 && len != LT_DOMAIN_EMPTY) 
	{
		err = LT_INVALID_DOMAIN;
    }
	else 
	{
		m_domain = LtDomain(data, len);
        memcpy(m_byKey, &data[BASE_DOMAIN_STORE_SIZE], keyLen);
		m_subnetNode.set(data[6], data[7] & 0x7f);
		m_bClone = (data[7] & 0x80) == 0x00;
        m_bUseOma = (omaField == 1);
        m_bUseLsEnhancedMode = useLsEnhacedMode;
        m_bEnableDhcp = useDhcp;
	}
	return err;
}

int LtDomainConfiguration::toLonTalk(byte* data, LtLonTalkDomainStyle style) {
    int keyLen = getKeyLenForStyle(style);
    int storageSize = BASE_DOMAIN_STORE_SIZE + keyLen;
    int len = m_domain.getAll(data);
    int keyOffset = 0;

    if (m_bUseOma || m_bUseLsEnhancedMode || m_bEnableDhcp)
    {
        len &= 0x87;  // Clear out all bits other than the len and the invalid bit.
    }
    if (m_bUseOma)
    {   
        if (m_nIndex == 1 && style == LT_CLASSIC_DOMAIN_STYLE)
        {   // Have to report the second half of the key...
            keyOffset = LT_CLASSIC_DOMAIN_KEY_LENGTH;
        }
        else
        {
            // Set oma field to 1
            len |= 8;
        }
    }
    if (m_bUseLsEnhancedMode)
    {
        len |= 0x40;
    }
    if (m_bEnableDhcp)
    {
        len |= 0x20;
    }
    data[8] = len;
	memcpy(&data[BASE_DOMAIN_STORE_SIZE], m_byKey+keyOffset, keyLen);
    data[6] = getSubnet();
    data[7] = ((m_bClone ? 0x00 : 0x80) | getNode());
    return storageSize;
}

void LtDomainConfiguration::package(LtBlob *pBlob)
{
    byte lonTalkData[OMA_DOMAIN_STORE_SIZE];
    if (pBlob->isPacking())
    {
        toLonTalk(lonTalkData, LT_OMA_DOMAIN_STYLE);
        pBlob->package(lonTalkData, sizeof(lonTalkData));
    }
    else
    {
        pBlob->package(lonTalkData, sizeof(lonTalkData));
        int length;
        fromLonTalk(lonTalkData, length, LT_OMA_DOMAIN_STYLE);
    }
    pBlob->package(&m_nIndex);
    pBlob->package(&m_alternateSubnetNode);
}

LtErrorType LtDomainConfiguration::getSubnetNode(int index, LtSubnetNode &subnetNode)
{
	LtErrorType err = LT_NO_ERROR;
	if (index == 0)
	{
		subnetNode = m_subnetNode;
	}
	else if (index == 1)
	{
		subnetNode = m_alternateSubnetNode;
	}
	else
	{
		err = LT_INVALID_PARAMETER;
	}
	return err;
}

LtErrorType LtDomainConfiguration::updateSubnetNode(int index, const LtSubnetNode &subnetNode)
{
	LtErrorType err = LT_NO_ERROR;
	if (index == 0)
	{
		m_subnetNode = subnetNode;
	}
	else if (index == 1)
	{
		m_alternateSubnetNode = subnetNode;
	}
	else
	{
		err = LT_INVALID_PARAMETER;
	}
	return err;
}

LtErrorType LtDomainConfiguration::getSubnetNodeIndex(LtSubnetNode& sn, int &subnetNodeIndex)
{
	LtErrorType err = LT_NO_ERROR;

	if (m_subnetNode == sn)
	{
		subnetNodeIndex = 0;
	}
	else if (m_alternateSubnetNode == sn)
	{
		subnetNodeIndex = 1;
	}
	else
	{
		err = LT_NOT_FOUND;
	}
	return err;
}

//
// Public Member Functions
//


LtDomainConfiguration::LtDomainConfiguration() {
    m_nIndex = FLEX_DOMAIN_INDEX;
	setAddress(0x00, 0x80);
    m_bUseOma = false;
	setKey(null);
    m_bClone = false;
    m_bUseLsEnhancedMode = false;
    m_bEnableDhcp = false;
}

/**
 * Sets up a domain configuration.
 * @param domain
 *              The domain ID and length.
 * @param subnet
 *              The subnet number.
 * @param node
 *              The node number.
 */
LtDomainConfiguration::LtDomainConfiguration(LtDomain& domain, byte* pKey, 
                                             int subnet, int node, boolean useOma) {
    m_domain = domain;
    setAddress(subnet, node);
    m_bUseOma = useOma;
	setKey(pKey);
    m_bClone = false;
    m_nIndex = FLEX_DOMAIN_INDEX;
    m_bUseLsEnhancedMode = false;
    m_bEnableDhcp = false;
}

/**
 * @return the subnet number.
 */
int LtDomainConfiguration::getSubnet() {
    return m_subnetNode.getSubnet();
}

/** 
 * @return the node number.
 */
int LtDomainConfiguration::getNode() {
    return m_subnetNode.getNode();
}

/**
 * This method returns true if the domain configuration represents a "flex" domain.
 * A flex domain is one in which the device is not configured.  For example,
 * if the device is configured in domain A and B but receives a Unique ID message
 * addressed to itself but on domain C, then the domain configuration will 
 * represent domain C and "isFlexLtDomain" will return true.
 * @return      true if flex domain.
 */
boolean LtDomainConfiguration::isFlexDomain() {
    return m_nIndex == FLEX_DOMAIN_INDEX;
}

int LtDomainConfiguration::hashCode() {
	return m_domain.hashCode() + m_subnetNode.hashCode();
}
