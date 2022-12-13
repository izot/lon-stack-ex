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

#include "LtRouter.h"


/**
 * This class defines the domain ID. 
 * DomainConfiguration includes the domain and the subnet/node address.
 *
 */

//
// Private Member Functions
//


//
// Protected Member Functions
//


void LtDomain::setDomain(int nLength, byte* pData) {
	m_nLen = nLength;
	// Always copy six bytes since in some cases all bytes are significant
	// even if the length says otherwise.  For example, domain IDs stored
	// in configuration tables are read back in full.
	memcpy(m_nId, pData, sizeof(m_nId));
}

void LtDomain::setZeroLength() {
	m_nLen = 0;
	memset(m_nId, 0, sizeof(m_nId));
}

int LtDomain::getData(int i)  {
	return m_nId[i];
}

int LtDomain::getAll(byte* data) {
	memcpy(data, m_nId, sizeof(m_nId));
    return inUse()?getLength():0xff;
}

int LtDomain::get(byte* data) {
    int len = getLength();
    if (len != LT_DOMAIN_EMPTY) {
		memcpy(data, m_nId, len);
    }
    return len;
}

boolean LtDomain::inUse() {
	return m_nLen != LT_DOMAIN_EMPTY;
}

boolean LtDomain::isValid() {
    return inUse();
}

//
// Public Member Functions
//


/**
 * Method to create a domain ID.
 * @param length
 *              Domain ID length (one of 0,1,3,6)
 * @param data
 *              Domain ID
 */
LtDomain::LtDomain(byte* data, int length) {
    setDomain(length, data);        
}

LtDomain::LtDomain() {
    m_nLen = LT_DOMAIN_EMPTY;
	memset(m_nId, 0, sizeof(m_nId));
}

void LtDomain::package(LtBlob *pBlob)
{
    pBlob->package(&m_nLen);
    pBlob->package(m_nId, sizeof(m_nId));
}

/**
 * This method gets the domain ID length (one of 0,1,3,6).
 * @return domain ID length
 */
int LtDomain::getLength() { 
    if (inUse()) {
        return m_nLen;
    }
    // If not valid domain, return 0.  Invalid domain must be caught later.
    return 0;
}

/**
 * This method gets the domain ID bytes.
 * @param data
 *              Byte array containing domain ID.
 */
void LtDomain::getData(byte* data) {
    memcpy(data, m_nId, getLength());
}

/**
 * @param domain
 *              Domain to compare against
 * @return true if this domain's ID matches that of "domain".
 */
int xyz=0;
boolean LtDomain::operator ==(LtDomain& domain) {
    return (m_nLen == 0 || memcmp(m_nId, domain.m_nId, m_nLen) == 0) &&
			m_nLen == domain.m_nLen;
}

int LtDomain::hashCode() {
	// Used to use constructs like *(short*)&m_nId[4] but this violates MIPs architecture
	switch (m_nLen) {
	case 1:
		return 0x1000000 + m_nId[0];
	case 3:
		return 0x3000000 + m_nId[0] + (m_nId[1]<<8) + (m_nId[2]<<16);
	case 6:
		return 0x6000000 + m_nId[0] + (m_nId[1]<<8) + (m_nId[2]<<16) +
						   (m_nId[3]<<24) + (m_nId[4]) + (m_nId[5]<<8);
	}
	return 0;
}

void LtDomain::dump()
{
    int i;
    if (getLength() == 0)
    {
        printf("<none>      ");
    } 
    else
    {
        for (i = 0; i < getLength(); i++)
        {
            printf("%02X", m_nId[i]);
        }
		for (i = getLength(); i < LT_DOMAIN_LENGTH; i++)
		{
			printf("  ");
		}
    }
}

void LtSubnetNode::package(LtBlob *pBlob)
{
    pBlob->package(&m_nSubnet);
    pBlob->package(&m_nNode);
}
