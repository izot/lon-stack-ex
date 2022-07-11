//
// LtMsgOverride.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtMsgOverride.cpp#1 $
//

#include <LtStack.h>

LtMsgOverride::LtMsgOverride()
{
}

LtMsgOverride::LtMsgOverride
					 (LtMsgOverrideOptions options,
					  LtServiceType service,
					  boolean priority,
					  int txTimer,
					  int rptTimer,
					  byte retryCount,
					  byte *authKey,
                      boolean keyIsOMA)
{
	m_options = options;
	m_service = service;
	m_bPriority = priority;
	m_txTimer = txTimer;
	m_rptTimer = rptTimer;
	m_retryCount = retryCount;
    m_bKeyIsOMA = keyIsOMA;
	if (authKey)
	{
        int keyLen = keyIsOMA ? sizeof(m_authkey) : LT_CLASSIC_DOMAIN_KEY_LENGTH;
		memcpy(m_authkey, authKey, keyLen);
        if (!keyIsOMA)
        {
            memset(&m_authkey[keyLen], 0, sizeof(m_authkey) - keyLen);
        }
	}
}

LtMsgOverride::~LtMsgOverride()
{
}

void LtMsgOverrideOptions::package(LtBlob *pBlob) 
{
    pBlob->package(&m_options);
}

LtMsgOverrideOptions LtMsgOverride::getOptions()
{
	return m_options;
}

LtServiceType LtMsgOverride::getService()
{
	return m_service;
}

boolean LtMsgOverride::getPriority()
{
	return m_bPriority;
}

int LtMsgOverride::getRptTimer()
{
	return m_rptTimer;
}

int LtMsgOverride::getTxTimer()
{
	return m_txTimer;
}

byte LtMsgOverride::getRetryCount()
{
	return m_retryCount;
}

byte* LtMsgOverride::getAuthKey()
{
	return m_authkey;
}

void LtMsgOverride::setDefaults(LtMsgOverride* pMsgOverride)
{
	// This method allows the msg override class to be used to convey
	// default information (i.e., the value for the fields which aren't
	// overridden.  For any field which is not overridden in this object
	// AND is overridden in the supplied object, adopt the supplied override.
	LtMsgOverrideOptions options = getOptions();
	LtMsgOverrideOptions defaults = pMsgOverride->getOptions();

	if (!options.overrideService() && defaults.overrideService())
	{
		m_service = pMsgOverride->getService();
	}
	if (!options.overridePriority() && defaults.overridePriority())
	{
		m_bPriority = pMsgOverride->getPriority();
	}
	if (!options.overrideRetryCount() && defaults.overrideRetryCount())
	{
		m_retryCount = pMsgOverride->getRetryCount();
	}
	if (!options.overrideRptTimer() && defaults.overrideRptTimer())
	{
		m_rptTimer = pMsgOverride->getRptTimer();
	}
	if (!options.overrideTxTimer() && defaults.overrideTxTimer())
	{
		m_txTimer = pMsgOverride->getTxTimer();
	}
}

void LtMsgOverride::package(LtBlob *pBlob) 
{
    LtStackBlob stackBlob(pBlob);
    stackBlob.package(&m_options);
	// At one point this only blobbed options if the overrides were set.
	// However, this structure is also used to report defaults for non-
	// overridden values so this optimization can't be used.
	pBlob->package(&m_service, sizeof(m_service));
	pBlob->package(&m_bPriority);
	pBlob->package(&m_txTimer);
	pBlob->package(&m_rptTimer);
	pBlob->package(&m_retryCount);
	pBlob->package(m_authkey, sizeof(m_authkey));
    pBlob->package(&m_bKeyIsOMA);
}
