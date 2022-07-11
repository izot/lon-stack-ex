//
// LtBitMap.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtBitMap.cpp#1 $
//

#include "LtRouter.h"
#include "LtBitMap.h"

#define OP_SET 1
#define OP_CLEAR 2

LtBitMap::~LtBitMap()
{
	semDelete(m_sem);
	delete bits;
}

boolean LtBitMap::op(int op, int i) {
    int offset = i / 32;
    int mask = 1 << (i % 32);
    int cur = bits[offset];
    boolean val = (cur & mask) == mask;
    switch (op) {
        case OP_SET:
            // Set
            if (!val) {
                bits[offset] = cur | mask;
                count++;
                if (curPos == -1) {
                    curPos = i;
                }
            }
            break;
        case OP_CLEAR:
            // Clear
            if (val) {
                bits[offset] = cur & ~mask;
                count--;
            }
            break;
    }

    return val;
}

boolean LtBitMap::opSafe(int oper, int i) {

	semTake(m_sem, WAIT_FOREVER);

	int val = op(oper, i);

	semGive(m_sem);

    return val;
}

LtBitMap::LtBitMap() {
	m_sem = semMCreate( SEM_Q_PRIORITY | SEM_INVERSION_SAFE );
	bits = null;
    curPos = -1;
    count = 0;
}

void LtBitMap::setSize(int cnt) {
	bitsLength = (cnt - 1) / 32 + 1;
    if (cnt != 0) {
        bits = new int[bitsLength];
		memset(bits, 0, bitsLength*sizeof(int));		
    }
}

void LtBitMap::set(int i) {
    opSafe(OP_SET, i);
}

void LtBitMap::clear(int i) {
    opSafe(OP_CLEAR, i);
}

boolean LtBitMap::get(int i) {
    return opSafe(3, i);
}

int LtBitMap::getNext() {
    int rtn = -1;

	semTake(m_sem, WAIT_FOREVER);

    if (count != 0) {
        rtn = curPos;
		op(OP_CLEAR, rtn);
        // Find next position
        if (count == 0) {
            curPos = -1;
        } else {
            int index = curPos / 32;
			int offset = curPos % 32;
            while ((bits[index] >> offset) == 0) {
                if (++index == bitsLength) {
                    index = 0;
                }
				offset = 0;
				curPos = index * 32;
            }
            // Found next int with bits set.
            int val = bits[index] >> offset;
            while ((val & 1) == 0) {
                curPos++;
                val >>= 1;
            }
        }
    }

	semGive(m_sem);

    return rtn;
}
