//
// LtHashTable.cpp
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

#include "LtRouter.h"

LtHashTable::LtHashTable(int nSize)
{
	m_pRecords = new LtHashRecord*[nSize];
	for (int i = 0; i < nSize; i++)
	{
		m_pRecords[i] = null;
	}
	m_nSize = nSize;
    m_nCount = 0;
}

void LtHashTable::clear(boolean deleteValues)
{
	for (int i = 0; m_nCount && i < m_nSize; i++)
	{
		LtHashRecord *pRec = m_pRecords[i];
		m_pRecords[i] = null;
		while (pRec != null)
		{
			delete pRec->getKey();
			if (deleteValues) delete pRec->getValue();
			LtHashRecord *pTemp = pRec;
			pRec = pRec->getNext();
			delete pTemp;
            m_nCount--;
		}
	}
    m_nCount = 0;
}

LtHashRecord* LtHashTable::findRec(LtHashKey *pKey, LtHashRecord** ppPrev)
{
	LtHashRecord* pRec = getRec(pKey);
	LtHashRecord* pPrev = null;
	while (pRec != null)
	{
		if (*pRec->getKey() == *pKey)
		{
			break;
		}
		pPrev = pRec;
		pRec = pRec->getNext();
	}
	if (ppPrev != null)
	{
		*ppPrev = pPrev;
	}

    return pRec;
}

LtObject* LtHashTable::get(LtHashKey *pKey)
{
	LtObject* pValue = null;
	LtHashRecord* pRec = findRec(pKey, null);
	if (pRec != null)
	{
		pValue = pRec->getValue();
	}
	return pValue;
}

LtObject* LtHashTable::set(LtHashKey* pKey, LtObject* pValue)
{
	LtObject* pOldValue = null;
	LtHashRecord* pPrev;
	LtHashRecord* pRec = findRec(pKey, &pPrev);
	if (pRec == null)
	{
        m_nCount++;
		insert(pPrev, pKey, new LtHashRecord(pKey, pValue));
	}
	else
	{
		pOldValue = pRec->getValue();
		pRec->setValue(pValue);
	}
	return pOldValue;
}

void LtHashTable::remove(LtHashRecord* pRec)
{
    LtHashRecord *pPrev = pRec->getPrev();
    LtHashRecord *pNext = pRec->getNext();

    if (pPrev == null)
    {
        setRec(pRec->getKey(), pNext);
    }
    else
    {
        pPrev->setNext(pNext);
    }
    if (pNext != null)
    {
        pNext->setPrev(pPrev);
    }
	delete pRec->getKey();
	delete pRec;
    m_nCount--;
}

LtObject* LtHashTable::removeKey(LtHashKey* pKey)
{
	LtObject* pValue = null;
	LtHashRecord* pPrev;
	LtHashRecord* pRec = findRec(pKey, &pPrev);
	if (pRec != null)
	{
        pValue = pRec->getValue();
        remove(pRec);
	}
	return pValue;
}

void LtHashTable::removeAt(LtHashTablePos &pos)
{
    LtHashRecord* pRec = pos.getRec();
    pos.setRec(pRec->getPrev());
    if (pos.getRec() == null)
    {
        pos.backup();
    }
    remove(pRec);
}

void LtHashTable::insert(LtHashRecord* pPrev, LtHashKey* pKey, LtHashRecord* pRec)
{
    LtHashRecord* pNext;
	if (pPrev == null)
	{
        pNext = getRec(pKey);
		setRec(pKey, pRec);
	}
	else
	{
        pNext = pPrev->getNext();
		pPrev->setNext(pRec);
        pRec->setPrev(pPrev);
    }
    if (pNext != null)
    {
        pNext->setPrev(pRec);
    }
    pRec->setNext(pNext);
}

boolean LtHashTable::getElement(LtHashTablePos& hashTablePos, LtHashKey** ppKey, LtObject** ppObject)
{
    LtHashRecord* pRec = hashTablePos.getRec();
    boolean found = false;

    if (pRec != null)
    {
        pRec = pRec->getNext();
    }
    while (pRec == null)
    {
        int pos = hashTablePos.getNext();
        if (pos < getSize())
        {
            pRec = m_pRecords[pos];
        }
        else
        {
            break;
        }
    }
    if (pRec != null)
    {
        hashTablePos.setRec(pRec);

        if (ppKey != null)
        {
            *ppKey = pRec->getKey();
        }
        *ppObject = pRec->getValue();
        found = true;
    }
    return found;
}

