#ifndef LT_LRE_PROTOCOL_ANALYZER_CLIENT_H
#define LT_LRE_PROTOCOL_ANALYZER_CLIENT_H
//
// LtProtocolAnalyzerClient.h
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

#include "LtNodeStack.h"

class LtLayer2Stack;
class LtLrePAClientBase; 

class LTA_EXTERNAL_CLASS LtProtocolAnalyzerClient 
{
public:
	LtProtocolAnalyzerClient();

	virtual ~LtProtocolAnalyzerClient();

    LtErrorType start(LtChannel *pLtChannel, int appIndex, boolean directCallbackMode, 
                      int messageEventMaximum, int password);
    void stop();

    void release(LtMsgIn* msg);
    void processApplicationEvents(void);

    // Callbacks...
    virtual void msgArrives(LtMsgIn* msg) = 0;
    virtual void applicationEventIsPending() {};

    void send(byte *pData, int len);

private:
    LtLayer2Stack     *m_pStack;
    LtLrePAClientBase *m_pClient;

};

#endif
