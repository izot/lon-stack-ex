#ifndef LTDOMAIN_H
#define LTDOMAIN_H

//
// LtDomain.h
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

#define LT_DOMAIN_LENGTH 6
#define LT_DOMAIN_EMPTY  0xff

class LTA_EXTERNAL_CLASS LtDomain 
{
private:
	byte m_nLen;
	byte m_nId[LT_DOMAIN_LENGTH];

private:
    LtDomain(LtBlob &blob)
    {
        package(&blob);
    }
    void package(LtBlob *pBlob);
    friend class LtBlob;
public:
    LtDomain();
    LtDomain(byte* pData, int nLen);

	inline void set(byte* pData, int nLen)
	{
   		m_nLen = nLen;
		if (nLen) 
		{
			memcpy(m_nId, pData, nLen);
		}
	}

	inline void set(LtDomain& domain)
	{
		set(domain.m_nId, domain.m_nLen);
	}

    void setDomain(int length, byte* data);
    void setZeroLength();
    int getData(int i);
    int getAll(byte* data);
    int get(byte* data);
    boolean inUse();
    boolean isValid();

	ULONGLONG toLong()
	{
        int shift = 8;
        ULONGLONG value = (m_nLen & 0xff);
        for (int i = 0; i < LT_DOMAIN_LENGTH; i++) {
            value |= (((ULONGLONG) m_nId[i]) & 0xff) << shift;
            shift += 8;
        }
		return value;
	}

    int getLength();
    void getData(byte* data);
    boolean operator ==(LtDomain& domain);
	int hashCode();

    void dump();
};

class LTA_EXTERNAL_CLASS LtSubnetNode 
{
private:
	byte	m_nSubnet;
	byte	m_nNode;

private:
    LtSubnetNode(LtBlob &blob)
    {
        package(&blob);
    }
    void package(LtBlob *pBlob);
    friend class LtBlob;

public:
    inline          LtSubnetNode()          
	{
		m_nSubnet = 255;
		m_nNode = 255;
	}

	inline LtSubnetNode(int nSubnet, int nNode) { set(nSubnet, nNode); }

	inline void set(int nSubnet, int nNode) 
	{ 
		m_nSubnet = nSubnet; 
		m_nNode = nNode;
	}

	inline void set(LtSubnetNode& sn)
	{
		set(sn.m_nSubnet, sn.m_nNode);
	}

	inline void setSubnet(int nSubnet) { m_nSubnet = nSubnet; }

	inline byte getNode() { return m_nNode; }
	inline byte getSubnet() { return m_nSubnet; }

	inline boolean operator ==(LtSubnetNode& subnetNode)
	{
		return m_nSubnet == subnetNode.m_nSubnet &&
			   m_nNode == subnetNode.m_nNode;
	}

	inline boolean inUse()
	{
		return m_nNode != 255;
	}

	int hashCode()
	{
		return (((int)m_nSubnet) << 8) + m_nNode;
	}
};

#endif
