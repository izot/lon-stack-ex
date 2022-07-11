#ifndef _LTMSGTAG_H
#define _LTMSGTAG_H
//
// LtMsgTag.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtMsgTag.h#1 $
//

/**
 * This class defines the message tag object used to define outgoing
 * message destination attributes and for completion event and
 * response correlation.
 */


class LtMsgTag {

private:
	boolean		m_bBindable;
    int			m_nIndex;

protected:

public:
    LtMsgTag(boolean bBindable=false, int nIndex=0)
	{
		m_bBindable = bBindable;
		m_nIndex = nIndex;
	}

	virtual ~LtMsgTag() {}

	boolean getBindable()
	{
		return m_bBindable;
	}

    int getIndex()
	{
		return m_nIndex;
	}

};

#endif
