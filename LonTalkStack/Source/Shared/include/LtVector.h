//
// LtVector.h
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
// This class implements a vector of objects which is basically
// just an array.  Removing elements from the middle of the array
// moves the last element to the open spot.  This object is not
// thread-safe.
//
// You can also use a vector as a positional array by using 
// setElementAt and removeElementAt.  If you use removeElementAt,
// then the array is not compressed on removal.  This assumes you
// want to preserve the positional location of an object.  It 
// is possible to mix removeElement and removeElementAt but
// doing so is not recommended.
//

#ifndef LtVector_h
#define LtVector_h

#include <LtObject.h>

class LTA_EXTERNAL_CLASS LtVectorPos
{
private:
    int m_nPos;
public:
	inline LtVectorPos(int init) { m_nPos = init; }
    inline LtVectorPos() { reset(); }
    inline int getIndex() { return m_nPos; }
    inline int getNext() { return ++m_nPos; }
	inline int getPrevious() { return --m_nPos; }
	inline void reset() { m_nPos = -1; }
};

class LTA_EXTERNAL_CLASS LtVector : public LtObject
{
private:
    int m_nMax;
    int m_nCount;
    int m_nNum;
    LtObject** m_items;

    inline int getMax() { return m_nMax; }

    void grow();

	friend class LtVectorEnumeration;

public:
    LtVector(int nSize=8);
	~LtVector();

    boolean isElement(LtObject* pElement);

    void addElement(LtObject* pElement);
    void addElementAt(int index, LtObject* pElement);
    
	int size() { return getMax()+1; }

    int getCount() { return m_nCount; }

    inline boolean isEmpty() { return m_nCount == 0; }

    LtObject* elementAt(int index);

	// do not use the following in iterations, unless you exit the iteration
	void removeElement(LtObject* pElement);
	// the following is safe within iterations
	void removeElement( LtVectorPos& pos );
    void removeElementAt(int index);
    void removeElementAt(LtVectorPos& pos) { removeElementAt(pos.getIndex()); }
	void removeAllElements(boolean deleteValues=false);
    inline void clear(boolean deleteValues=false) { removeAllElements(deleteValues); }

    boolean getElement(LtVectorPos& vectorPos, LtObject** ppValue);
};

template<class VectorClass> class LtTypedVector : public LtVector
{
public:


    inline boolean isElement(VectorClass* pElement) { return LtVector::isElement(pElement); }
    inline void addElement(VectorClass* pElement) { LtVector::addElement(pElement); }
    inline void addElementAt(int index, VectorClass* pElement) { LtVector::addElementAt(index, pElement); }
    inline VectorClass* elementAt(int nIndex) { return (VectorClass*) LtVector::elementAt(nIndex); }
    inline void removeElement(VectorClass* pElement) { LtVector::removeElement(pElement); }
	inline void removeElement( LtVectorPos& pos ) { LtVector::removeElement( pos ); }
    boolean getElement(LtVectorPos& vectorPos, VectorClass** ppValue) 
    { 
        LtObject* pValue;
        boolean rc = LtVector::getElement(vectorPos, &pValue);
        *ppValue = (VectorClass*) pValue;
        return rc;
    }
};


#if 0 // eliminate LtVVector

// LtVVector class
// Stands for Void Vector.
// Intended for use through a template only.
class LtVVector
{
private:
    int m_nMax;
    int m_nCount;
    int m_nNum;
    void** m_items;

    inline int getMax() { return m_nMax; }

    void grow();
	virtual void deleteElement( void* pElement ) = 0;

public:
    LtVVector(int nSize=8);
	~LtVVector();

    boolean isElement(void* pElement);

    void addElement(void* pElement);
    void addElementAt(int index, void* pElement);
    
	int size() { return getMax()+1; }

    int getCount() { return m_nCount; }

    inline boolean isEmpty() { return m_nCount == 0; }

    void* elementAt(int index);

	void removeElement(void* pElement);
    void removeElementAt(int index);
    void removeElementAt(LtVectorPos& pos) { removeElementAt(pos.getIndex()); }
	void removeAllElements(boolean deleteValues=false);
    inline void clear(boolean deleteValues=false) { removeAllElements(deleteValues); }

    boolean getElement(LtVectorPos& vectorPos, void** ppValue);
};


template<class VectorClass> class LtTypedVVector : public LtVVector
{
public:
    inline boolean isElement(VectorClass* pElement) { return LtVVector::isElement(pElement); }
    inline void addElement(VectorClass* pElement) { LtVVector::addElement((VectorClass*)pElement); }
    inline void addElementAt(int index, VectorClass* pElement) { LtVVector::addElementAt(index, (VectorClass*)pElement); }
    inline VectorClass* elementAt(int nIndex) { return (VectorClass*) LtVVector::elementAt(nIndex); }
    inline void removeElement(VectorClass* pElement) { LtVVector::removeElement((VectorClass*)pElement); }
    boolean getElement(LtVectorPos& vectorPos, VectorClass** ppValue) 
    { 
        VectorClass* pValue;
        boolean rc = LtVVector::getElement(vectorPos, (void**)&pValue);
        *ppValue = pValue;
        return rc;
    }
private:
	virtual void deleteElement( void* pElement )
	{	delete (VectorClass*) pElement;
	}
};
#endif // eliminate LtVVector

#endif
