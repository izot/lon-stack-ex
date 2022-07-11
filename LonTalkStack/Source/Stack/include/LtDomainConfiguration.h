#ifndef LTDOMAINCONFIGURATION_H
#define LTDOMAINCONFIGURATION_H

//
// LtDomainConfiguration.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtDomainConfiguration.h#2 $
//


/**
 * This class defines a domain configuration which includes domain ID, subnet
 * and node address. 
 *
 */
#define FLEX_DOMAIN_INDEX   -1
#define BASE_DOMAIN_STORE_SIZE 9    /* Does not include key */
#define CLASS_DOMAIN_STORE_SIZE (BASE_DOMAIN_STORE_SIZE + LT_CLASSIC_DOMAIN_KEY_LENGTH)
#define OMA_DOMAIN_STORE_SIZE (BASE_DOMAIN_STORE_SIZE + LT_OMA_DOMAIN_KEY_LENGTH)
#define NUM_ADDRESSES_PER_DOMAIN 2

typedef enum
{
    LT_CLASSIC_DOMAIN_STYLE,    // Standard domain definition, with 48 bit key.
    LT_KEYLESS_DOMAIN_STYLE,    // LONTalk Domain without a key.
    LT_OMA_DOMAIN_STYLE,        // LONTalk Domain with 96 bit key.
} LtLonTalkDomainStyle;

class LTA_EXTERNAL_CLASS LtDomainConfiguration 
{
private:
    int						m_nIndex;
    LtDomain				m_domain;
	byte					m_byKey[LT_OMA_DOMAIN_KEY_LENGTH];
	static byte				m_byNullKey[LT_OMA_DOMAIN_KEY_LENGTH];
	LtSubnetNode			m_subnetNode;
	LtSubnetNode			m_alternateSubnetNode;
	boolean					m_bClone;
    boolean                 m_bUseOma;
    boolean                 m_bUseLsEnhancedMode;
    boolean                 m_bEnableDhcp;  // For completeness.  VNI doesn't actualy support DHCP.
protected:
    LtDomainConfiguration(LtBlob &blob)
    {
        package(&blob);
    }
    void package(LtBlob *pBlob);
    friend class LtStackBlob;
public:
    LtDomainConfiguration(int index);
    LtDomainConfiguration(int index, LtDomain& domain, byte* pKey, 
                          int subnet, int node, boolean useOma = FALSE);
    void setAddress(int subnet, int node);
    int getIndex();
    void setIndex(int index);
    boolean equals(LtDomainConfiguration& match, boolean includeAddress);
    boolean equals(LtDomain& match);
    boolean equals(LtSubnetNode& sn);
    LtErrorType fromLonTalk(byte data[], int& len, LtLonTalkDomainStyle style);
    int toLonTalk(byte* data, LtLonTalkDomainStyle style);
	static int getStoreSize() { return OMA_DOMAIN_STORE_SIZE; }
    static int getKeyLenForStyle(LtLonTalkDomainStyle style);

public:
    LtDomainConfiguration();
    LtDomainConfiguration(LtDomain& domain, byte* pKey, int subnet, int node, boolean useOma = FALSE);
	virtual ~LtDomainConfiguration() {}
    LtDomain& getDomain() { return m_domain; }
	LtSubnetNode& getSubnetNode() { return m_subnetNode; }
    int getSubnet();
    int getNode();
    boolean isFlexDomain();
	int hashCode();
	byte* getKey();
    boolean getUseOma() { return m_bUseOma; }
	void setKey(byte* pKey);
    void setUseOma(boolean useOma) { m_bUseOma = useOma; }
	void updateKey(boolean bIncrement, byte* pData);
	LtErrorType getSubnetNode(int index, LtSubnetNode &subnetNode);
	LtErrorType updateSubnetNode(int index, const LtSubnetNode &subnetNode);
	LtErrorType getSubnetNodeIndex(LtSubnetNode &subnetNode, int &index);
	static byte* getNullKey() { return m_byNullKey; }
	static boolean isFlexDomain(int index) { return index == FLEX_DOMAIN_INDEX; }
	boolean isClone() { return m_bClone; }

    boolean getUseLsEnhancedMode() { return m_bUseLsEnhancedMode; }
    void setUseLsEnhancedMode(boolean bUseLsEnhancedMode) { m_bUseLsEnhancedMode = bUseLsEnhancedMode; }
    
};

#endif
