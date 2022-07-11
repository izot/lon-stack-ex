//
// LtVector.cpp
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
#include <VxlTypes.h>
#include <LtObject.h>
#include <LtVector.h>
#include <string.h>

//#include "LtRouter.h"

LtVector::LtVector(int nMax) : m_nNum(nMax) 
{
    m_items = new LtObject*[m_nNum];
    m_nMax = -1;
    m_nCount = 0;
}

LtVector::~LtVector()
{
	delete m_items;
}

void LtVector::grow()
{
    LtObject** pNew = new LtObject*[m_nNum];
    memcpy(pNew, m_items, (size())*sizeof(m_items[0]));
    delete m_items;
    m_items = pNew;
}

boolean LtVector::isElement(LtObject* pElement)
{
    for (int i = 0; i <= m_nMax; i++)
    {
        if (m_items[i] == pElement)
        {
            return true;
        }
    }
    return false;
}

LtObject* LtVector::elementAt(int nIndex)
{
	if (nIndex <= m_nMax)
	{
		return m_items[nIndex];
	}
	return null;
}

void LtVector::addElement(LtObject* pElement) 
{
    if (size() == m_nNum)
    {
        // Exhausted array size.  Grow it.
        m_nNum *= 2;
        grow();
    }
    m_items[++m_nMax] = pElement;
    m_nCount++;
}
    
void LtVector::addElementAt(int nIndex, LtObject* pElement)
{
    if (nIndex >= m_nNum)
    {
        while (nIndex >= m_nNum)
        {
            m_nNum *= 2;
        }
        grow();
    }
    if (nIndex > m_nMax)
    {
        memset(&m_items[size()], 0, (nIndex-size())*sizeof(m_items[0]));
        m_nMax = nIndex;
        m_nCount++;
    }
    else if (m_items[nIndex] == null)
    {
        m_nCount++;
    }
    m_items[nIndex] = pElement;
}

// remove an element from a vector.
// do not use in an iteration unless you immediately exit
// the iteration. Does not fix the iteration position, so an element
// is skipped.
void LtVector::removeElement(LtObject* pElement)
{
    for (int i = 0; i <= m_nMax; i++)
    {
        if (m_items[i] == pElement)
        {
            m_nCount--;
            m_items[i] = m_items[m_nMax--];
            break;
        }
    }
}

// remove an element by position.
// this is safe in an iteration since it adjusts the pos so elements are not skipped
void LtVector::removeElement( LtVectorPos& pos )
{
	int idx = pos.getIndex();
	m_nCount--;
	m_items[idx] = m_items[m_nMax--];
	pos.getPrevious();
}


void LtVector::removeElementAt(int index) {
    if (index == m_nMax)
    {
        m_nMax--;
    }
    if (m_items[index] != null)
    {
        m_nCount--;
    }
    m_items[index] = null;
}

void LtVector::removeAllElements(boolean deleteValues)
{
    if (deleteValues)
    {
        while (m_nCount && m_nMax >= 0)
        {
			LtObject* pObject = m_items[m_nMax];
			if (pObject)
			{
				delete pObject;
				m_items[m_nMax] = null;
			}
			m_nMax--;
        }
    }
    else
    {
        m_nMax = -1;
    }
    m_nCount = 0;
}

boolean LtVector::getElement(LtVectorPos& vectorPos, LtObject** ppValue)
{
    LtObject* pValue = null;
    while (pValue == null && vectorPos.getNext() <= m_nMax)
    {
        pValue = m_items[vectorPos.getIndex()];
    }
    *ppValue = pValue;
    return pValue != null;
}

#if 0 // eliminate LtVVector class
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

LtVVector::LtVVector(int nMax) : m_nNum(nMax) 
{
    m_items = new void*[m_nNum];
    m_nMax = -1;
    m_nCount = 0;
}

LtVVector::~LtVVector()
{
	delete m_items;
}

void LtVVector::grow()
{
    void** pNew = new void*[m_nNum];
    memcpy(pNew, m_items, (size())*sizeof(m_items[0]));
    delete m_items;
    m_items = pNew;
}

boolean LtVVector::isElement(void* pElement)
{
    for (int i = 0; i <= m_nMax; i++)
    {
        if (m_items[i] == pElement)
        {
            return true;
        }
    }
    return false;
}

void* LtVVector::elementAt(int nIndex)
{
	if (nIndex <= m_nMax)
	{
		return m_items[nIndex];
	}
	return null;
}

void LtVVector::addElement(void* pElement) 
{
    if (size() == m_nNum)
    {
        // Exhausted array size.  Grow it.
        m_nNum *= 2;
        grow();
    }
    m_items[++m_nMax] = pElement;
    m_nCount++;
}
    
void LtVVector::addElementAt(int nIndex, void* pElement)
{
    if (nIndex >= m_nNum)
    {
        while (nIndex >= m_nNum)
        {
            m_nNum *= 2;
        }
        grow();
    }
    if (nIndex > m_nMax)
    {
        memset(&m_items[size()], 0, (nIndex-size())*sizeof(m_items[0]));
        m_nMax = nIndex;
        m_nCount++;
    }
    else if (m_items[nIndex] == null)
    {
        m_nCount++;
    }
    m_items[nIndex] = pElement;
}

void LtVVector::removeElement(void* pElement)
{
    for (int i = 0; i <= m_nMax; i++)
    {
        if (m_items[i] == pElement)
        {
            m_nCount--;
            m_items[i] = m_items[m_nMax--];
            break;
        }
    }
}

void LtVVector::removeElementAt(int index) {
    if (index == m_nMax)
    {
        m_nMax--;
    }
    if (m_items[index] != null)
    {
        m_nCount--;
    }
    m_items[index] = null;
}

void LtVVector::removeAllElements(boolean deleteValues)
{
    if (deleteValues)
    {
        while (m_nCount-- > 0 && m_nMax >= 0)
        {
            deleteElement( m_items[m_nMax--]);
        }
    }
    else
    {
        m_nMax = -1;
    }
    m_nCount = 0;
}

boolean LtVVector::getElement(LtVectorPos& vectorPos, void** ppValue)
{
    void* pValue = null;
    while (pValue == null && vectorPos.getNext() <= m_nMax)
    {
        pValue = m_items[vectorPos.getIndex()];
    }
    *ppValue = pValue;
    return pValue != null;
}
#endif // eliminate LtVVector class
