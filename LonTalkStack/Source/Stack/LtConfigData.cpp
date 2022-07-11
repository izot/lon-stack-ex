//
// LtConfigData.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtConfigData.cpp#1 $
//

#include "LtStackInternal.h"


//
// Private Member Functions
//


//
// Protected Member Functions
//


//
// Public Member Functions
//

LtConfigData::LtConfigData() 
{
	memset(lonTalk, 0, sizeof(lonTalk));
	lonTalk[LT_CD_NMAUTH] = 0xd0;
	lonTalk[LT_CD_ROUTER_TYPE] = LT_CONFIGURED_ROUTER;  // Neuron's default
}

boolean LtConfigData::getNmAuth() 
{
    return (lonTalk[LT_CD_NMAUTH]&0x08)==0x08;
}

int LtConfigData::getNonGroupReceiveTimer() 
{
    return LtMisc::toRcvTimer((lonTalk[LT_CD_NONGRP_RECEIVE_TIMER] & 0xf0) >> 4);
}

LtRouterType LtConfigData::getRouterType()
{
	return (LtRouterType) lonTalk[LT_CD_ROUTER_TYPE];
}

void LtConfigData::getCommParams(LtCommParams& cp) 
{
	memcpy(cp.m_nData, &lonTalk[LT_CD_COMMPARAMS], sizeof(cp.m_nData));
}

//
// setRouterType
//
// Provides write access to router type field of network image.  Used for self-installation of routers.
//
LtErrorType LtConfigData::setRouterType(LtRouterType rtrType)
{
	LtErrorType err = LT_INVALID_PARAMETER;
	if (rtrType < LT_ROUTER_TYPES)
	{
		lonTalk[LT_CD_ROUTER_TYPE] = (byte)rtrType;
		err = LT_NO_ERROR;
	}
	return err;
}

void LtConfigData::setCommParams(LtCommParams& cp) 
{
    memcpy(&lonTalk[LT_CD_COMMPARAMS], cp.m_nData, sizeof(cp.m_nData));
}

int LtConfigData::getLength() 
{
    return sizeof(lonTalk);
}

byte* LtConfigData::toLonTalk(int offset, int length) 
{
    byte* data = new byte[length];
    toLonTalk(data, offset, length);
    return data;
}

void LtConfigData::toLonTalk(byte* data, int offset, int length) 
{
    if (offset+length <= (int)sizeof(lonTalk)) {
        memcpy(data, lonTalk+offset, length);
    }
}

// Return true if data is valid
boolean LtConfigData::fromLonTalk(byte* data, int offset, int length, boolean validate) 
{
    byte sum = 0;
	if (validate)
	{
		for (int i = 0; i < length; i++) 
		{
			sum += data[i] - lonTalk[offset + i];
		}
	}
    if (offset+length <= (int)sizeof(lonTalk)) 
	{
        memcpy(lonTalk+offset, data, length);
    } 
	else 
	{
        sum = 0xff;
    }
    return sum == 0;
}
