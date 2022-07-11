#ifndef LTROUTERMAP_H
#define LTROUTERMAP_H

//
// LtRouteMap.h
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

#define LT_NUM_SUBNETS 256
#define LT_NUM_GROUPS 256

#define BITS_PER_UNIT (sizeof(byte)*8)

// These are LonTalk architectural values
// DO NOT MODIFY
typedef enum
{
    LT_CONFIGURED_ROUTER	= 0,
    LT_LEARNING_ROUTER		= 1,
    LT_BRIDGE				= 2,
    LT_REPEATER				= 3,
    LT_ROUTER_TYPES,
	LT_INVALID_ROUTER_TYPE
} LtRouterType;

class LtRouteMap
{
private:
    byte*	m_pMask;
    int     m_nSize;
    int     m_nMax;
public:
    LtRouteMap(byte* pMask, int nSize) : m_pMask(pMask), m_nSize(nSize)
    {
		clear();
        m_nMax = 0;
    }

	LtRouteMap(byte* pMask, int nSize, byte* pSet, int nMax)
	{
		copy(pMask, nSize, pSet, nMax);
	}
	boolean operator ==(LtRouteMap& map)
	{
    return (m_nSize == 0 || memcmp(m_pMask, map.m_pMask, m_nSize) == 0) &&
			m_nSize == map.m_nSize;
	}

	void copy(byte* pMask, int nSize, byte* pSet, int nMax) 
	{
		m_pMask = pMask;
		m_nSize = nSize;
		m_nMax = nMax;
		memcpy(pMask, pSet, nSize);
	}

    inline int getMax() { return m_nMax;}

    boolean get(int nSubnet)
    {
        return (m_pMask[nSubnet/BITS_PER_UNIT] & (1<<(nSubnet%BITS_PER_UNIT))) ? true : false;
    }
    void set(int nSubnet, boolean bValue=true)
    {
        int mask = (1<<(nSubnet%BITS_PER_UNIT));
        if (bValue)
        {
            m_pMask[nSubnet/BITS_PER_UNIT] |= mask;
            if (nSubnet > m_nMax)
            {
                m_nMax = nSubnet;
            }
        }
        else
        {
            m_pMask[nSubnet/BITS_PER_UNIT] &= ~mask;
        }
    }
    void setAll(boolean bValue=true)
    {
		memset(m_pMask, bValue ? -1 : 0, m_nSize);
		m_nMax = bValue ? (m_nSize * BITS_PER_UNIT - 1) : 0;
    }
    void clear()
    {
        setAll(false);
    }
	void set(byte* pData, int offset, int len)
	{
		assert(len <= m_nSize);
		memcpy(m_pMask + offset, pData, len);
        // Update "max".
        for (int i = 0; i < len; i++)
        {
            if (pData[i])
            {
                int index = (offset + i + 1) * 8 - 1;
                if (m_nMax < index) m_nMax = index;
            }
        }
	}
	void get(byte* pData, int offset, int len)
	{
		assert(len <= m_nSize);
		memcpy(pData, m_pMask + offset, len);
	}
	int get(byte* pData)
	{
		memcpy(pData, m_pMask, m_nSize);
		return m_nSize;
	}
	int getStoreSize()
	{
		return m_nSize;
	}
};

class LtSubnets : public LtRouteMap
{
private:
    byte    m_iMask[LT_NUM_SUBNETS/BITS_PER_UNIT];
public:
    LtSubnets() : LtRouteMap(m_iMask, sizeof(m_iMask)) {}
	LtSubnets(LtSubnets& map) : LtRouteMap(m_iMask, sizeof(m_iMask), map.m_iMask, map.getMax()) {}
	void operator =(LtSubnets& map) { copy(m_iMask, sizeof(m_iMask), map.m_iMask, map.getMax()); }
};

class LtGroups : public LtRouteMap
{
private:
    byte    m_iMask[LT_NUM_GROUPS/BITS_PER_UNIT];
public:
    LtGroups() : LtRouteMap(m_iMask, sizeof(m_iMask)) {}
	LtGroups(LtGroups& map) : LtRouteMap(m_iMask, sizeof(m_iMask), map.m_iMask, map.getMax()) {}
	void operator =(LtGroups& map) { copy(m_iMask, sizeof(m_iMask), map.m_iMask, map.getMax()); }
};

class LtRoutingMap
{
private:
	LtRouterType	m_routerType;
	LtDomain		m_domain;
	LtSubnets		m_subnets;
	LtGroups		m_groups;

public:
	LtRoutingMap() { m_routerType = LT_CONFIGURED_ROUTER; }

	LtRoutingMap(LtRouterType t, LtDomain& d, LtSubnets& s, LtGroups& g) : 
	  m_routerType(t), m_domain(d), m_subnets(s), m_groups(g) {}

	LtRouterType	getRouterType()						{ return m_routerType; }
	void			setRouterType(LtRouterType type)	{ m_routerType = type; }

	LtDomain& 		getDomain()							{ return m_domain; }
	void			setDomain(LtDomain& domain)			{ m_domain = domain; }

	boolean			isValid();

	// getSubnets and getGroups don't apply to bridges or repeaters.

	// For a learning router, the subnet mask
	// indicates the subnets known to be on
	// the router's far side.  
	LtSubnets& 		getSubnets()						{ return m_subnets; }
	void			setSubnets(LtSubnets& subnets)		{ m_subnets = subnets; }

	// For learning router all bits are set.
	LtGroups& 		getGroups()							{ return m_groups; }
	void			setGroups(LtGroups& groups)			{ m_groups = groups; }
	boolean			operator == (LtRoutingMap& map )
	{
		boolean result = false;
		if ( m_routerType == map.m_routerType &&
			m_domain == map.m_domain &&
			m_subnets == map.m_subnets &&
			m_groups == map.m_groups
			)
		{	result = true;
		}
		return result;
	}
	int				getStoreSize();
	int				load(byte* pData);
	int 			store(byte* pData);
};

#endif
