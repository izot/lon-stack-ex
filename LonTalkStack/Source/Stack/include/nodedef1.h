//
// nodedef1.h
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

#ifndef _NODEDEF1_H
#define _NODEDEF1_H

#include <LtNetData.h>

typedef enum
{
	ND_OK,
	ND_FILE_NOT_FOUND,
	ND_PARTIAL_DEFINITION,
	ND_VALUE_OUT_OF_RANGE,
	ND_STRING_TOO_LONG,
	ND_FILE_CREATION_FAILURE,
	ND_NOT_IMPLEMENTED,
	ND_INSUFFICIENT_DATA,
	ND_INVALID_PARAMETER,
	ND_NOT_AVAILABLE,
	ND_DUPLICATE,
	ND_NOT_PERMITTED,
} NdErrorType;

#define ND_NAME_BASE_LEN			16		// Base name len
#define ND_NAME_LEN					24		// Maximum name length for all elements
											// (MTs, CPs, NVs, LMOs)
//
// SI Data definitions
//
#define VER1_HDR_LEN				6
#define VER2_HDR_LEN				17
#define MAX_SD_STRING_LEN			1024
#define MAX_NV_EXTENSION			MAX_SD_STRING_LEN + ND_NAME_LEN + 4

typedef enum 
{
	PT_UNKNOWN = 0,
	PT_MIP = 1,
	PT_NEURON = 2,
	PT_HOST = 3,
	PT_HOST_NEURON = 4,	// Host app with Neuron NV limits
} NdProgramType;

typedef enum
{
	NV_CHANGE_TYPE = 0,
	NV_CHANGE_NAME = 1,
	NV_CHANGE_SD = 2,
	NV_CHANGE_RATES = 3,
	NV_CHANGE_CONFIG = 4
} NvChangeType;

typedef enum
{
	MC_CHANGE_ATTRIBUTES = 0,
	MC_CHANGE_DESCRIPTION = 1
} McChangeType;

class LTA_EXTERNAL_CLASS NdBufferConfiguration
{
public:
	NdBufferConfiguration();

	// Network buffer counts
	int nNetIn;
	int nNetOut;
	int nPriNetOut;

	// Network buffer sizes
	int nSizeNetIn;
	int nSizeNetOut;

	// Application buffer counts
	int nAppIn;
	int nAppOut;
	int nPriAppOut;

	// Application buffer sizes
	int nSizeAppIn;
	int nSizeAppOut;

	// Get/Set 5 byte encoded/packed version for read only data
	NdErrorType getEncoded(unsigned char* pEncoded);
	NdErrorType setEncoded(unsigned char* pEncoded);

	NdErrorType validate();
};

class LTA_EXTERNAL_CLASS NdCollectionElement
{
private:
	int m_nIndex;
	char m_szName[ND_NAME_LEN];

public:
	NdCollectionElement()
	{
		m_nIndex = -1;
		m_szName[0] = 0;
	}
	virtual ~NdCollectionElement() {}

	int getIndex() { return m_nIndex; }
	void setIndex(int nIndex) { m_nIndex = nIndex; }

	void getName(char* szName) { memcpy(szName, m_szName, sizeof(m_szName)); }
	char* getName() { return m_szName; }
	NdErrorType setName(char* szName) 
	{
		m_szName[ND_NAME_LEN-1] = 0;
		strncpy(m_szName, szName, ND_NAME_LEN);
		if (m_szName[ND_NAME_LEN-1] != 0)
			return ND_STRING_TOO_LONG;
		return ND_OK;
	}

	int getSerialLength() { return 2; }
	void serialize(byte* &pBuffer) { PTONS(pBuffer, m_nIndex); }
	void deserialize(byte* &pBuffer) { PTOHS(pBuffer, m_nIndex); }
};

// Start with start size and multiply up until reaching the add size then start adding.
#define COLLECTION_START_SIZE 4
#define COLLECTION_ADD_SIZE	2048
#define COLLECTION_MULTIPLIER 2

// 
// This macro creates a collection by defining the collection and interfaces to it
// including:
//	int get<collection>Count();			// Total number of items in collection.
//  Nd<collection>* get(int nIndex);	// Get Nth item
//  NdErrorType set(Nd<collection>* p<collection>);
// 
// General comment on adding objects to collections.
// If the object's index has not been set, it is assigned the next index after
// the last object in the collection (it does not fill in gaps).  The object
// is copied so the passed in object is freed by the caller.
//
#define DEFINE_COLLECTION(collection) \
	private: \
		Nd##collection##s	m_coll##collection##s; \
		Nd##collection##s* get##collection##s() { return &m_coll##collection##s; } \
		\
	public: \
		int get##collection##Count() { return get##collection##s()->getCount(); } \
		Nd##collection * get##collection(int nIndex) { return get##collection##s()->get(nIndex); } \
		NdErrorType add##collection(Nd##collection * p##collection) { return get##collection##s()->add(p##collection); }

template<class CollectionType> class NdCollection
{
private:
	CollectionType** m_elements;
	int m_nCount;
	int m_nMaxInUse;
	int m_nCapacity;
	int m_bDeleteOnReset;

public:
	NdCollection()
	{
		m_elements = NULL;
		m_nCapacity = 0;
		m_nCount = 0;
		m_nMaxInUse = -1;
		m_bDeleteOnReset = true;
	}

	~NdCollection()
	{
		delete m_elements;
	}

	int getCount() { return m_nCount; }
	int getMaxInUse() { return m_nMaxInUse; }
	int getCapacity() { return m_nCapacity; }

	void setDeleteOnReset(int bDeleteOnReset) { m_bDeleteOnReset = bDeleteOnReset; }
	void setCapacity(int nCapacity) 
	{ 
		CollectionType** temp = m_elements;
		m_elements = new CollectionType*[nCapacity];
		for (int i = 0; i < nCapacity; i++)
		{
			m_elements[i] = NULL;
		}
		if (temp != NULL)
		{
			memcpy(m_elements, temp, m_nCapacity*sizeof(CollectionType*));
			delete temp;
		}
		m_nCapacity = nCapacity; 
	}

	// This method assumes indices are managed by the collection
	// and tracked per element.  
	// Use the alternate form of "add" to allow the index to
	// be allocated by the caller.
	NdErrorType add(CollectionType* pElement)
	{
		if (pElement->getIndex() == -1)
		{
			pElement->setIndex(m_nCount);
		}
		return add(pElement->getIndex(), pElement);
	}

	NdErrorType add(int nIndex, CollectionType* pElement)
	{
		m_nCount++;
		if (nIndex > m_nMaxInUse)
		{
			m_nMaxInUse = nIndex;
		}
		int newCapacity = m_nCapacity;
		while (nIndex >= newCapacity)
		{
			if (newCapacity == 0)
			{
				newCapacity = COLLECTION_START_SIZE;
			}
			else if (newCapacity > COLLECTION_ADD_SIZE)
			{
				newCapacity += COLLECTION_ADD_SIZE;
			}
			else 
			{
				newCapacity *= COLLECTION_MULTIPLIER;
			}
		}
		if (newCapacity != m_nCapacity)
		{
			setCapacity(newCapacity);
		}
		m_elements[nIndex] = pElement;
		return ND_OK;
	}

	CollectionType* get(int nIndex)
	{
		CollectionType* pElement = NULL;
		if (nIndex >= 0 && nIndex <= m_nMaxInUse)
		{
			pElement = m_elements[nIndex];
		}
		return pElement;
	}

	CollectionType* getNext(int& nIndex)
	{
		CollectionType* pElement = NULL;
		while (pElement == NULL && nIndex >= 0 && nIndex <= m_nMaxInUse)
		{
			pElement = get(nIndex++);
		}
		return pElement;
	}

	// Remove the element - caller must free the memory.
	void remove(int nIndex)
	{
		if (nIndex >= 0 && nIndex <= m_nMaxInUse)
		{
			if (nIndex == m_nMaxInUse)
			{
				while ((--m_nMaxInUse > 0) && (m_elements[m_nMaxInUse] == NULL));
			}
			if (m_elements[nIndex] != NULL)
			{
				--m_nCount;
			}
			m_elements[nIndex] = NULL;
		}
	}

	void remove(CollectionType* pElement)
	{
		remove(pElement->getIndex());
	}

	void removeLast()
	{
		remove(m_nMaxInUse);
	}

	void deleteLast()
	{
		delete get(m_nMaxInUse);
		removeLast();
	}

	void reset()
	{
		if (m_bDeleteOnReset)
		{
			while (m_nCount)
			{
				deleteLast();
			}
		}
		else
		{
			m_nCount = 0;
			m_nMaxInUse = -1;
		}
	}
};

#endif
