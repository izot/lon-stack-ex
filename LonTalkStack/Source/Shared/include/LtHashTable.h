//
// LtHashTable.h
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
// This class implements a hash table which maps keys to 
// objects.  It is not thread-safe.
//

#ifndef _LTHASHTABLE_H
#define _LTHASHTABLE_H

class LtHashKey;

class LtHashKey 
{
public:
    virtual ~LtHashKey() {}
	virtual int hashCode() = 0;
	virtual boolean operator ==(LtHashKey& key) = 0;
};

class LtHashRecord;

class LtHashTablePos
{
private:
    int m_nPos;
    LtHashRecord* m_pRec;
public:
    inline LtHashTablePos() { reset(); }
    inline int getIndex() { return m_nPos; }
	inline void setIndex(int index) { m_nPos = index; }
    inline int getNext() { return ++m_nPos; }
    inline void backup() { --m_nPos; }
    inline LtHashRecord* getRec() { return m_pRec; }
    inline void setRec(LtHashRecord* pRec) { m_pRec = pRec; }
	inline void reset() { m_nPos = -1; m_pRec = null; }
};

class LtHashRecord
{
private:
	LtHashRecord*   m_pNext;
    LtHashRecord*   m_pPrev;
	LtHashKey*	    m_pKey;
	LtObject*	m_pValue;
public:
	inline LtHashRecord(LtHashKey* pKey, LtObject* pValue) : m_pNext(null), m_pPrev(null), m_pKey(pKey), m_pValue(pValue) {}

	inline LtHashRecord*	getNext()					{ return m_pNext; }
    inline LtHashRecord*    getPrev()                   { return m_pPrev; }
	inline LtHashKey*		getKey()					{ return m_pKey; }
	inline LtObject*		getValue()					{ return m_pValue; }
	inline void				setNext(LtHashRecord* pNext){ m_pNext = pNext; }
    inline void             setPrev(LtHashRecord* pPrev){ m_pPrev = pPrev; }
	inline void				setKey(LtHashKey* pKey)		{ m_pKey = pKey; }
    inline void				setValue(LtObject* pValue) { m_pValue = pValue; }
};

class LtHashTable 
{
protected:
	LtHashRecord**	m_pRecords;
	int				m_nSize;
    int             m_nCount;

	LtHashRecord* findRec(LtHashKey* pKey, LtHashRecord** ppPrev);
	void insert(LtHashRecord* pPrev, LtHashKey* pKey, LtHashRecord* pRec);
    void remove(LtHashRecord* pRec);
    inline int getCount() { return m_nCount; }
    inline int getSize() { return m_nSize; }
	inline int getIndex(LtHashKey *pKey)
	{
		return (pKey->hashCode()&0x7fffffff) % m_nSize;
	}

	inline LtHashRecord* getRec(LtHashKey *pKey)
	{
		return m_pRecords[getIndex(pKey)];
	}

	inline void setRec(LtHashKey* pKey, LtHashRecord* pRecord)
	{
		m_pRecords[getIndex(pKey)] = pRecord;
	}

public:
	enum {DEFAULT_HASH_SIZE = 17};
	LtHashTable(int nSize=DEFAULT_HASH_SIZE);
	~LtHashTable()
	{
		clear(false);
        delete m_pRecords;
	}

    boolean isEmpty() { return m_nCount == 0; }
	void clear(boolean deleteValues=false);
	LtObject* get(LtHashKey* pKey);
	LtObject* set(LtHashKey* pKey, LtObject* pValue);
	LtObject* removeKey(LtHashKey* pKey);
    boolean getElement(LtHashTablePos& pos, LtHashKey** ppKey, LtObject** ppObject);
    void removeAt(LtHashTablePos& pos);
};

template<class KeyClass, class ValueClass> class LtTypedHashTable : public LtHashTable
{
public:
	LtTypedHashTable(int size = LtHashTable::DEFAULT_HASH_SIZE) : LtHashTable(size) {}
	inline ValueClass* get(KeyClass* pKey)
    {
        return (ValueClass*) LtHashTable::get((LtHashKey*) pKey);
    }
	inline ValueClass* set(KeyClass* pKey, ValueClass* pValue)
    {
        return (ValueClass*)LtHashTable::set((LtHashKey*) pKey, (LtObject*) pValue);
    }
	inline ValueClass* removeKey(KeyClass* pKey)
    {
        return (ValueClass*) LtHashTable::removeKey((LtHashKey*) pKey);
    }
    boolean getElement(LtHashTablePos &pos, KeyClass** pKey, ValueClass** pObject)
    {
        LtHashKey* pHashKey;
        LtObject* pHashObject;
        boolean rc = LtHashTable::getElement(pos, &pHashKey, &pHashObject);
        if (rc)
        {
            if (pKey != null)
            {
                *pKey = (KeyClass*) pHashKey;
            }
            *pObject = (ValueClass*) pHashObject;
        }
        return rc;
    }
};

#endif // _LTHASHTABLE_H
