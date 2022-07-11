#ifndef LTMSGOUT_H
#define LTMSGOUT_H

//
// LtMsgOut.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtMsgOut.h#1 $
//


/**
 * This class defines the message object exchanged between layer 7 of the
 * LonTalkStack and the LonTalk Application.
 * Example of sending an explicit message.
 * <code> <pre>
 *
 *    MsgTag tag = new MsgTag(false, 1);
 *    MsgOut msg = new MsgOut(10, false);
 *    msg.setTag(tag);
 *    msg.setCode(0x33);
 *    msg.setData(0, 5);
 *    send(msg);
 *
 * </pre> </code>
 *
 */

class LTA_EXTERNAL_CLASS LtMsgOut: public LtApduOut
{
	friend class LtDeviceStack;

protected:
    LtMsgOut(LtBlob &blob) : LtApduOut(blob) {};
    void package(LtBlob *pBlob);

public:
    LtMsgOut(boolean bPriority) : LtApduOut() 
	{ 
		LtApdu::setOrigPri(bPriority); 
		LtApdu::setPriority(bPriority); 
	}

    void setTag(LtMsgTag& tag)			
	{ 
		if (tag.getBindable())
		{
			setAddressIndex(tag.getIndex());
		}
		LtApdu::setRefId((int)&tag); 
	}
    void setCode(int code)				{ LtApdu::setCode(code); }
	void setLength(int length)			{ LtApdu::setLength(length); }
    void setData(int offset, int data)	{ LtApdu::setData(offset, data); }
    void setData(byte* data, int offset, int length)
										{ LtApdu::setData(data, offset, length); }

    void setAuthenticated(boolean value){ LtApdu::setAuthenticated(value); }
	void setZeroSync(boolean value)     { LtApdu::setZeroSync(value); }
	void setAttenuate(boolean value)    { LtApdu::setAttenuate(value); }
    void setPath(int path)				{ LtApdu::setAlternatePath(path); }
    void setServiceType(LtServiceType value)		
	{ 
		if (value <= LT_REQUEST)
		{
			LtApdu::setServiceType(value);
		}
	}
    LtOutgoingAddress& getAddr()		
	{ 
		setAddressIndex(LT_EXPLICIT_ADDRESS);
		return *(LtOutgoingAddress*) this; 
	}
	void setOverride(LtMsgOverride* pOverride)
	{
		LtApduOut::setOverride(pOverride);
	}
};

#endif
