#ifndef _LTCONFIGDATA_H
#define _LTCONFIGDATA_H
//
// LtConfigData.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtConfigData.h#1 $
//

#define LT_CD_COMMPARAMS			8
#define LT_CD_NMAUTH				24
#define LT_CD_NONGRP_RECEIVE_TIMER	24
#define LT_CD_ROUTER_TYPE			55  

#define LT_CD_NUM_CONFIG_DATA_BYTES	56

class LtConfigData {

private:
    byte lonTalk[LT_CD_NUM_CONFIG_DATA_BYTES];

protected:

public:
	LtConfigData();
    boolean getNmAuth();
    int getNonGroupReceiveTimer();
	LtRouterType getRouterType();
	LtErrorType setRouterType(LtRouterType rtrType);
    void getCommParams(LtCommParams& commParams);
    void setCommParams(LtCommParams& commParams);
    int getLength();
    byte* toLonTalk(int offset, int length);
    void toLonTalk(byte* data, int offset, int length);
    boolean fromLonTalk(byte* data, int offset, int length, boolean validate = true);
};

#endif
