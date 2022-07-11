#ifndef LTMSGIN_H
#define LTMSGIN_H

//
// LtMsgIn.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtMsgIn.h#1 $
//


/**
 * This class defines the incoming message object passed from layer 7 of the
 * LonTalk Stack to the LonTalk Application.
 *
 */

class LTA_EXTERNAL_CLASS LtMsgIn: private LtApduIn
{
	friend class LtRespOut;
	friend class LtDeviceStack;
	friend class LtNetworkManager;
	friend class LtRouterApp;
	friend class LtMipApp;


protected:
    LtMsgIn(LtBlob &blob) : LtApduIn(blob) {};
    void package(LtBlob *pBlob);
    friend class VniPackage;

public:
    LtMsgIn() {}
    int getCode()					{ return LtApdu::getCode(); }
	byte* getData()					{ return LtApdu::getData(); }
    int getLength()					{ return LtApdu::getDataLength(); }
    void getData(byte* pData, int offset, int length)
									{ LtApdu::getData(pData, offset, length); }
	byte getData(int offset)		{ return LtApdu::getData(offset); }

    boolean getPriority()			{ return LtApdu::getPriority(); }
    boolean getAuthenticated()		{ return LtApdu::getAuthenticated(); }
	LtServiceType getServiceType()	{ return LtApdu::getServiceType(); }

    LtIncomingAddress& getAddress()	{ return *(LtIncomingAddress*) this; }

	#if PRODUCT_IS(VNISTACK)
	void getTimeStamp(_timeb &timeOfDay, DWORD &highResTimeStamp) 
	{ 
		LtApduIn::getTimeStamp(timeOfDay, highResTimeStamp);
	}
	#endif
	byte getL2PacketType()			{ return LtApduIn::getL2PacketType(); }
	boolean getIsValidL2Packet()	{ return LtApduIn::getIsValidL2Packet(); }
	int		getPhaseReading()		{ return LtApduIn::getPhaseReading(); }

    int getPath()		      { return LtApduIn::getAlternatePath(); }
};

#endif
