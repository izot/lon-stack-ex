#ifndef _LTUNIQUEID_H
#define _LTUNIQUEID_H

#include "LtBlob.h"
//
// LtUniqueId.h
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

#include <LtaDefine.h>

/**
 * This class defines the unique ID (also known as the Neuron Id)
 *
 */
#define LT_UNIQUE_ID_LENGTH   6

// storage for a text version of the Unique ID
// (use with LtUniqueId::toText)
typedef char LtUniqueIdText[3 * LT_UNIQUE_ID_LENGTH];

class LTA_EXTERNAL_CLASS LtUniqueId
{

private:
    byte m_byId[LT_UNIQUE_ID_LENGTH];

private:
    LtUniqueId(LtBlob &blob)
    {
        package(&blob);
    }
    void package(LtBlob *pBlob);
    friend class LtBlob;
protected:
	static byte m_emptyId[LT_UNIQUE_ID_LENGTH];

public:
    inline              LtUniqueId()        
	{
		memset(m_byId, 0, sizeof(m_byId));
	}

    LtUniqueId(const byte* data);

    inline void set(const byte* pData) { memcpy(m_byId, pData, LT_UNIQUE_ID_LENGTH); }
    inline void set(LtUniqueId& id) { set(id.m_byId); }
	void set(ULONGLONG value);

    static int getLength();
    int getData(int i);
    void getData(byte* data);
    boolean operator ==(LtUniqueId& nid);
    void get(byte* data);
	byte* getData() { return m_byId; }
	inline int hashCode() { return m_byId[5] + (m_byId[4]<<8) +
								   (m_byId[3]<<16) + (m_byId[2]<<24); }
	char* toText(LtUniqueIdText& text);
    void dump();
	boolean isSet()
	{
		return memcmp(m_byId, m_emptyId, LT_UNIQUE_ID_LENGTH) != 0;
	}
};

#endif
