//
// LtBitMap.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtBitMap.h#1 $
//

#ifndef LTBITMAP_H
#define LTBITMAP_H

class LtBitMap {

private:
	int bitsLength;
	int* bits;
    int curPos;
    int count;
    boolean op(int op, int i);
	boolean opSafe(int op, int i);
	SEM_ID m_sem;

protected:

public:
    LtBitMap();
	~LtBitMap();
	void setSize(int cnt);
    void set(int i);
    void clear(int i);
    boolean get(int i);
    int getNext();
};

class LtNvMap : public LtBitMap
{
public:
	void setSize(int nvCount)
	{
		LtBitMap::setSize(nvCount*2);
	}
	void set(boolean poll, int index)
	{
		LtBitMap::set(index*2 + (poll ? 1 : 0));
	}
	int getNext(boolean& poll)
	{
		int val = LtBitMap::getNext();
		if (val != -1)
		{
			poll = val&1;
			val /= 2;
		}
		return val;
	}
};

#endif
