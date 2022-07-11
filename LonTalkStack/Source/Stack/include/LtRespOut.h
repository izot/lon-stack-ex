#ifndef LTRESPOUT_H
#define LTRESPOUT_H

//
// LtRespOut.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtRespOut.h#1 $
//


/**
 * This class defines the response out object exchanged between layer 7 of the
 * LonTalkStack and the LonTalk Application.
 * Example of sending a response.
 * <code> <pre>
 *
 *    RespOut resp = new RespOut(msgIn, 1);
 *    resp.setCode(0x31);
 *    resp.setData(0, 6);
 *    send(resp);
 *
 * </pre> </code>
 */

class LTA_EXTERNAL_CLASS LtRespOut: public LtApduOut
{
	friend class LtDeviceStack;
	friend class LtNetworkManager;

protected:
    LtRespOut(boolean bPriority, LtBlob &blob) : LtApduOut(blob) 
    {
		LtApdu::setOrigPri(bPriority); 
    };
    void package(LtBlob *pBlob);

public:
    LtRespOut(LtMsgIn* pRequest) : LtApduOut(pRequest) {}

    void setNullResponse()				{ LtApdu::setNullResponse(true); }
    void setRespondOnFlexDomain()       { LtApdu::setRespondOnFlexDomain(true); }
    void setCode(int code)				{ LtApdu::setCode(code); }
	void setLength(int length)			{ LtApdu::setLength(length); }
    void setData(int offset, int data)	{ LtApdu::setData(offset, data); }
    void setData(byte* data, int offset, int length)
										{ LtApdu::setData(data, offset, length); }
};

#endif
