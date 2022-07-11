#ifndef LT_LRE_PA_BASE_H
#define LT_LRE_PA_BASE_H
//
// LtLrePAClientBase.h
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

#include "LtStackInternal.h"

class LtProtocolAnalyzerClient;

class LtLrePAClientBase : public LtApplication
{
public:
	LtLrePAClientBase(LtProtocolAnalyzerClient *pClient);

    void reset() {};
    void wink() {};
    void offline() {};
    void online() {};
    void initializing() {};
    void msgArrives(LtMsgIn* msg);
    void msgCompletes(LtMsgTag* tag, boolean success) {};
    void respArrives(LtMsgTag* tag, LtRespIn* response) {};
    void servicePinPushed() {};
    void flushCompletes() {};
    void applicationEventIsPending();
    boolean readMemory(int address, byte* data, int length) { return false; };
    boolean writeMemory(int address, byte* data, int length) { return false; };
    void persistenceLost(LtPersistenceLossReason reason) {};


private:
    LtProtocolAnalyzerClient *m_pClient;
};

#endif
