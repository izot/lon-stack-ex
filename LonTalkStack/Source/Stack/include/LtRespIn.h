#ifndef LTRESPIN_H
#define LTRESPIN_H

// LtRespIn.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtRespIn.h#1 $
//				** ECHELON CONFIDENTIAL **
//


/**
 * This class defines the incoming response object passed from layer 7 of the
 * LonTalk Stack to the LonTalk Application.
 *
 */

class LTA_EXTERNAL_CLASS LtRespIn: private LtApduIn
{
	friend class LtDeviceStack;
	friend class LtNetworkManager;
	friend class LtMipApp;

protected:
    LtRespIn(LtBlob &blob) : LtApduIn(blob) {};
    void package(LtBlob *pBlob);
    friend class VniPackage;

protected:
	LtRespIn(LtMsgOut* pMsg)		{ LtApdu::setRefId(pMsg->getRefId()); }

public:
    LtRespIn() {}

    int getCode()					{ return LtApdu::getCode(); }
	byte* getData()					{ return LtApdu::getData(); }
    int getLength()					{ return LtApdu::getDataLength(); }
    void getData(byte* pData, int offset, int length)
									{ LtApdu::getData(pData, offset, length); }
	byte getData(int offset)		{ return LtApdu::getData(offset); }

    LtIncomingAddress& getAddress() { return *(LtIncomingAddress*) this; }
};

#endif
