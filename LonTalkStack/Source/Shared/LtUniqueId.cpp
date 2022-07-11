//
// LtUniqueId.cpp
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


/**
 * This class defines the unique ID (also known as the Neuron Id)
 *
 */

//
// Private Member Functions
//


//
// Protected Member Functions
//

byte LtUniqueId::m_emptyId[LT_UNIQUE_ID_LENGTH] = {0,0,0,0,0,0};

void LtUniqueId::get(byte data[]) {
	memcpy(data, m_byId, sizeof(m_byId));
}

void LtUniqueId::package(LtBlob *pBlob)
{
    pBlob->package(m_byId, sizeof(m_byId));
}

//
// Public Member Functions
//


/**
 * @param data
 *              6 byte ID array
 * @param offset
 *              offset into data of start of ID.
 */
LtUniqueId::LtUniqueId(const byte* data) {
    set(data);
}

void LtUniqueId::set(ULONGLONG value)
{
    for (int i = 0; i < LT_UNIQUE_ID_LENGTH; i++) 
	{
        m_byId[i] = (byte)(value&0xff);
		value >>= 8;
    }
}

int LtUniqueId::getLength() {
    return LT_UNIQUE_ID_LENGTH;
}

int LtUniqueId::getData(int i) 
{
    return (int)m_byId[i];
}

void LtUniqueId::getData(byte* id) {
	memcpy(id, m_byId, sizeof(m_byId));
}

boolean LtUniqueId::operator ==(LtUniqueId& nid) {
	return memcmp(m_byId, nid.m_byId, sizeof(m_byId)) == 0;
}

char* LtUniqueId::toText(LtUniqueIdText& text)
{
    char* p = text;
    int i;
    for (i = 0; i < LT_UNIQUE_ID_LENGTH - 1; ++i)
    {
        sprintf(p, "%02X:", m_byId[i]);
        p += 3;
    }
    sprintf(p, "%02X", m_byId[i]);
    return text;
}

void LtUniqueId::dump() 
{
    LtUniqueIdText text;
    printf("%s\n", toText(text));
}
