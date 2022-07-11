//
// LtaLdv.h
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
// $Header$
//

#ifndef LTALDV_H
#define LTALDV_H

#include "LtMipApp.h"
#include "Osal.h"

class LtaLdv : public LtMipApp
{
private:
	#define		m_maxMsgCount 5
	int			m_msgIn;
	int			m_msgOut;
	OsalHandle  m_event;
	union
	{
		LtSicb sicb;
		byte data[256+sizeof(OpSignal)];	// OpSignal info is appended to SICB
	} m_msg[m_maxMsgCount];

public:
	LtaLdv(int nIndex, LtLogicalChannel* pChannel, int nAddressTableCount=15);
	~LtaLdv();

	virtual void registerEvent(OsalHandle pEvent);
	virtual void receive(LtSicb* pSicb, OpSignal* pSsi);

	LtErrorType get(LtSicb* pSicb, int len);
	static LtaLdv* getLdv(short handle);
	static short reserveHandle(short shandle=-1);
	static void setLdv(short handle, LtaLdv* pNewLdv);
	static bool clearLdv(short handle);

	// This is a network interface and thus doesn't send service pin messages.
	void sendServicePinMessage() {}
	
	void signalClient(void);
};

#if FEATURE_INCLUDED(APP_CONTROL)
//
// Classes need for application start-up and control
// These classes are not needed if app start and control
// is managed via a mechanism other than that provided by
// the LtStart class (for example, Windows applications 
// would probably not use this mechanism).
//
class LtaLdvControl : public LtaBaseControl
{
public:
	LtaLdvControl(int nIndex, LtLogicalChannel* pChannel) : LtaBaseControl(nIndex, pChannel) {}
	void activate();
};

class LtaLdvRegistrar : public LtAppRegistrar
{
public:
	LtaLdvRegistrar();
	virtual ~LtaLdvRegistrar(){};
	LtAppControl* createInstance(int nIndex, LtLogicalChannel* pChannel);
	const char* getName() { return "LtaLdv"; }
	boolean getStartByDefault() { return false; }
	int getMaxInstances() { return 1; }
};

// The global instance
extern LtaLdvRegistrar ltaLdvReg;
#endif

#endif // LTALDV_H

