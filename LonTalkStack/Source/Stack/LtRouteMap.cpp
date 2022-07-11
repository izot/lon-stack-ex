//
// LtRouteMap.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtRouteMap.cpp#1 $
//

#include <LtRouter.h>

boolean LtRoutingMap::isValid()
{
	return m_routerType >= 0 && m_routerType < LT_ROUTER_TYPES;
}

int LtRoutingMap::load(byte* pData)
{
	m_routerType = (LtRouterType) *pData++;
	int len = *pData++;
	m_subnets.set(pData, 0, len);
	pData += len;
	len = *pData++;
	m_groups.set(pData, 0, len);
	return getStoreSize();
}

int LtRoutingMap::store(byte* pData)
{
	*pData++ = (byte) m_routerType;
	byte* pSave = pData++;
	int len = m_subnets.get(pData);
	*pSave = len;
	pData += len;
	pSave = pData++;
	len = m_groups.get(pData);
	*pSave = len;
	return getStoreSize();
}

int LtRoutingMap::getStoreSize()
{
	return 3 + m_subnets.getStoreSize() + m_groups.getStoreSize();
}
