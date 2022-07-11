//
// LtProgramId.cpp
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

#include "LtRouter.h"

/**
 * This class defines the program ID.
 *
 */

//
// Private Member Functions
//


//
// Protected Member Functions
//


void LtProgramId::setData(int i, int d) {
    m_nId[i] = (byte)d; 
}

void LtProgramId::setData(byte data[]) {
    memcpy(m_nId, data, LT_PROGRAM_ID_LENGTH);
}

//
// Public Member Functions
//

LtProgramId::LtProgramId()
{
    memset(m_nId, 0, LT_PROGRAM_ID_LENGTH);
	modifiable = false;
}

LtProgramId::LtProgramId(byte* data) {
    setData(data);
	modifiable = false;
}

LtProgramId::LtProgramId(byte* data, bool m) {
	setData(data);
	modifiable = m;
}

LtProgramId::LtProgramId(LtProgramId &pid) {
	setData(pid.getData());
	modifiable = pid.modifiable;
}


int LtProgramId::getLength() {
    return LT_PROGRAM_ID_LENGTH;
}

int LtProgramId::getData(int i) {
    return m_nId[i];
}

byte* LtProgramId::getData() {
    return m_nId; 
}

boolean LtProgramId::operator ==(LtProgramId& pid)
{
	return memcmp(pid.m_nId, m_nId, getLength()) == 0;
}

boolean LtProgramId::isCompatible(LtProgramId& pid)
{
	// Program IDs are considered compatible if they are equal or
	// in the case of routers if they are equal when excluding
	// the XID byte.
	if (modifiable) return true;
	boolean bResult = memcmp(getData(), pid.getData(), getLength()) == 0;
	if (!bResult)
	{
		// Check for special case of router program ID
		if (getData(0) == 0x80 && getData(3) == 1 && getData(4) == 1)
		{
			LtProgramId temp(getData());
			temp.setData(6, pid.getData(6));
			bResult = memcmp(temp.getData(), pid.getData(), getLength()) == 0;
		}
	}
	return bResult;
}
