//
// LtProtocolAnalyzerClient.cpp
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

#include "LtProtocolAnalyzerClient.h"
#include "LtLrePAClientBase.h"

LtProtocolAnalyzerClient::LtProtocolAnalyzerClient()
{
    m_pStack = NULL;
    m_pClient = NULL;
}

LtProtocolAnalyzerClient::~LtProtocolAnalyzerClient()
{
    stop();
}

#define PA_PASSWORD 0x0854abc93
LtErrorType LtProtocolAnalyzerClient::start(LtChannel *pChannel,
                                            int appIndex, 
                                            boolean directCallbackMode, 
                                            int messageEventMaximum, 
                                            int password)
{
    LtErrorType sts;

    stop();
    m_pStack = static_cast<LtLayer2Stack *>(LtStack::createLayer2(pChannel));
    m_pClient = new LtLrePAClientBase(this);

    if (password != PA_PASSWORD)
    {
        sts = LT_NOT_IMPLEMENTED;
    }
    else
    {
        byte programIdBuf[LT_PROGRAM_ID_LENGTH];
        memset(programIdBuf, 0, sizeof(programIdBuf));
        LtProgramId programId(programIdBuf);

        sts = m_pStack->registerApplication(appIndex, m_pClient, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "", &programId);
        if (sts == LT_NO_ERROR)
        {
            m_pStack->setDirectCallbackMode(directCallbackMode);
            m_pStack->setTxIdLifetime(3000);
            m_pStack->setMessageEventMaximum(messageEventMaximum);
            m_pStack->setMessageOutMaximum(100, 100);
            printf("directCallbackMode = %d\n", directCallbackMode);
            printf("messageEventMaximum = %d\n", messageEventMaximum);
        }
    }
    return(sts);
}
void LtProtocolAnalyzerClient::stop()
{
    if (m_pStack != NULL)
    {
        m_pStack->shutDown();
    }
    delete m_pClient;
    delete m_pStack;
    m_pClient = NULL;
    m_pStack = NULL;
}

void LtProtocolAnalyzerClient::release(LtMsgIn* msg)
{
    m_pStack->release(msg);
}

void LtProtocolAnalyzerClient::processApplicationEvents(void)
{
    m_pStack->processApplicationEvents();
}

void LtProtocolAnalyzerClient::send(byte *pData, int len)
{
    LtMsgOut *pMsgOut = m_pStack->msgAlloc();

    if (pMsgOut != NULL)
    {
        pMsgOut->setData(pData, 0, len);
        m_pStack->send(pMsgOut, FALSE);
    }
}

