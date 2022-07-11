//
// LtProgramId.h
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


/**
 * This class defines the program ID.
 *
 */
#ifndef _LTPROGRAMID_H
#define _LTPROGRAMID_H

#define LT_PROGRAM_ID_LENGTH 8

class LTA_EXTERNAL_CLASS LtProgramId {

private:
    byte m_nId[LT_PROGRAM_ID_LENGTH];
	bool modifiable;

public:
	LtProgramId(); 
    void setData(int i, int d);
    void setData(byte* datat);
	boolean isCompatible(LtProgramId& pid);

public:
    LtProgramId(byte* data);
    LtProgramId(byte* data, bool m);
	LtProgramId(LtProgramId &pid);
    static int getLength();
    int getData(int i);
    byte* getData();
	void getData(byte* pData) { memcpy(pData, m_nId, LT_PROGRAM_ID_LENGTH); }
	boolean operator ==(LtProgramId& pid);

	void serialize(byte* &pBuf)
	{
		memcpy(pBuf, m_nId, LT_PROGRAM_ID_LENGTH);
		pBuf += LT_PROGRAM_ID_LENGTH;
	}
	void deserialize(byte* &pBuf, boolean special=false)
	{
		// Cover up for early bug.  Length was mistakenly set to 6 in version 1.
		int len = special ? 6 : LT_PROGRAM_ID_LENGTH;
		memcpy(m_nId, pBuf, len);
		pBuf += len;
	}
};

#endif // _LTPROGRAMID_H
