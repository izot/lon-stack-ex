#ifndef LTMSGOVERRIDE_H
#define LTMSGOVERRIDE_H

//
// LtMsgOverride.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtMsgOverride.h#1 $
//

#include <LtDomain.h>
#include <LtDomainConfiguration.h>

#define LT_OVRD_NONE		0x00

#define LT_OVRD_SERVICE		0x01
#define LT_OVRD_PRIORITY	0x02
#define LT_OVRD_TX_TIMER	0x04
#define LT_OVRD_RPT_TIMER	0x08
#define LT_OVRD_RETRY_COUNT	0x10
#define LT_OVRD_AUTH_KEY	0x20

class LTA_EXTERNAL_CLASS LtMsgOverrideOptions
{
private:
	byte m_options;
protected:
    friend class LtStackBlob;
    LtMsgOverrideOptions(LtBlob &blob) 
    {
        package(&blob);
    }
    void package(LtBlob *pBlob);

public:
	LtMsgOverrideOptions() 
	{
		m_options = LT_OVRD_NONE;
	}
	LtMsgOverrideOptions(byte options)
	{
		m_options = options;
	}
	byte getOptions() { return m_options; }

	boolean hasOverrides() { return m_options != LT_OVRD_NONE; }

	boolean overrideService() { return m_options & LT_OVRD_SERVICE; }
	boolean overridePriority() { return m_options & LT_OVRD_PRIORITY; }
	boolean overrideTxTimer() { return m_options & LT_OVRD_TX_TIMER; }
	boolean overrideRptTimer() { return m_options & LT_OVRD_RPT_TIMER; }
	boolean overrideRetryCount() { return m_options & LT_OVRD_RETRY_COUNT; }
	boolean overrideAuthKey() { return m_options & LT_OVRD_AUTH_KEY; }
};

class LTA_EXTERNAL_CLASS LtMsgOverride
{
private:
	LtMsgOverrideOptions m_options;
	LtServiceType m_service;
	boolean m_bPriority;
	int m_txTimer;
	int m_rptTimer;
	byte m_retryCount;
	byte m_authkey[LT_OMA_DOMAIN_KEY_LENGTH];
    boolean m_bKeyIsOMA;

protected:
    friend class LtStackBlob;
    LtMsgOverride(LtBlob &blob) 
    {
        package(&blob);
    }
    void package(LtBlob *pBlob); 

public:
	friend class LtMonitorPoint;

public:
	LtMsgOverride();
	LtMsgOverride(LtMsgOverrideOptions options,
				  LtServiceType service,
				  boolean priority,
				  int txTimer,
				  int rptTimer,
				  byte retryCount,
				  byte *authkey=NULL,
                  boolean keyIsOMA = FALSE);
	~LtMsgOverride();

	boolean hasOverrides() { return m_options.hasOverrides(); }

	LtMsgOverrideOptions getOptions();
	LtServiceType getService();
	boolean getPriority();
	int getTxTimer();
	int getRptTimer();
	byte getRetryCount();
	byte *getAuthKey();
    boolean getKeyIsOma() { return m_bKeyIsOMA; }
	void setDefaults(LtMsgOverride* pMsgOverride);
};

#endif
