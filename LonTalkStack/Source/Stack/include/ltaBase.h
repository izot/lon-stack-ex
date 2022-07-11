//
// ltaBase.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/ltaBase.h#1 $
//
#ifndef _LTABASE_H
#define _LTABASE_H


#include "LtCUtil.h"
#include "wdLib.h"

#include "LtStackInternal.h"
#include "LtStart.h"
#if FEATURE_INCLUDED(APP_CONTROL)
#include "LtApplicationControl.h"
#endif

#ifdef SJDHLJKDSHFKJSDJLHF
#include "lta.h"
#include "LtStack.h"
#include "LtPlatform.h"
#include "LtNetworkManager.h"
#include "LtNetworkImage.h"
#include "LonTalkNode.h"
#include "LtDeviceStack.h"
#endif

#if !defined(WIN32) && PRODUCT_IS(ILON)
#include "Observer.h"
#endif

class LtaTimer
{
private:
	WDOG_ID	m_id;
	LtApplication* m_pApp;
	boolean m_bExpired;

public:
	LtaTimer()
	{
		m_id = wdCreate();
		m_bExpired = false;
	}

	~LtaTimer()
	{
		wdDelete(m_id);
	}

	LtApplication* getApp() { return m_pApp; }

	void setApp(LtApplication* pApp)
	{
		m_pApp = pApp;
	}

	void start(int msec)
	{
		m_bExpired = false;
		wdStart(m_id, msToTicksX(msec), timeout, (int) this);
	}

	void stop()
	{
		wdCancel(m_id);
		m_bExpired = false;
	}

	void expired() 
	{
		m_bExpired = true;
		m_pApp->applicationEventIsPending();
	}

	boolean isExpired()
	{
		boolean bResult = m_bExpired;
		m_bExpired = false;
		return bResult;
	}

	static int VXLCDECL timeout(int object,...);
};

/*
 * Although I dont see any harm in making this inherit from Subject in
 * all cases, I would rather leave the windows platform code unchanged
 */

#if !defined(WIN32) && PRODUCT_IS(ILON)
class LtaBase : public LtAppNodeStack, public LtApplication, public Subject
#else
class LtaBase : public LtAppNodeStack, public LtApplication 
#endif
{
public:
	LtaBase(const char* szName, int priority, int stacksize, LtLogicalChannel* pChannel, int nRx = 200, int nTx = 200);
	virtual ~LtaBase();

	// Call this method in the destructor of the app to initiate shutdown.
	virtual void stopApp();
	virtual void run()=0;
	virtual boolean appShutdown();

	void eventWait();


    void applicationEventIsPending();
	void persistenceLost(LtPersistenceLossReason reason);
    void msgArrives(LtMsgIn* msg);
    void respArrives(LtMsgTag* pTag, LtRespIn* response);

	void wink();
    void offline() {}
    void online() {}
	void initializing() {}

    void nvUpdateOccurs(LtNetworkVariable* pNv, int arrayIndex,
		LtNetworkVariable* pSourceNv, int sourceArrayIndex,
		LtIncomingAddress* address) {}
    void nvUpdateCompletes(LtNetworkVariable* pNv, int arrayIndex, boolean success) {}

    void msgCompletes(LtMsgTag* tag, boolean success) {}
    void reset() {}
    void flushCompletes() {}
    void servicePinPushed() {}
    void setServiceLedStatus(LtServicePinState state) {};

    boolean readMemory(int address, byte* data, int length) { return false; }
    boolean writeMemory(int address, byte* data, int length) { return false; }

	// Your derived class should set m_bAppInitialized
	// to "true" when you have finished initializing the class.  
	// This variable prevents run() from being called before the 
	// virtual function table is initialized.
	boolean isInitialized(void) {return m_bAppInitialized;}

protected:
	SEM_ID	m_semEvent;
	int		m_tidApp;
	boolean m_bAppShutdownRequest;
	boolean m_bAppShutdown;
	boolean m_bAppInitialized;
	char	m_szName[20];

	static int VXLCDECL appTask( int app, ... );

};

#if FEATURE_INCLUDED(APP_CONTROL)
//
// Classes need for application start-up and control
// These classes are not needed if app start and control
// is managed via a mechanism other than that provided by
// the LtStart class (for example, Windows applications 
// would probably not use this mechanism).
//
class LtaBaseControl : public LtApplicationControl
{
private:
	LtApplication* m_pApp;
	LtStack* m_pStack;

public:
	LtaBaseControl(int nIndex, LtLogicalChannel* pChannel) : LtApplicationControl(nIndex, pChannel) 
	{
		m_pApp = NULL;
		m_pStack = NULL;
	}
	void sendServicePinMsg();
	int getState();
	void deactivate();
	void resetPersistence();
	LtErrorType install(int nDomainIndex, LtDomainConfiguration* pDcMain,
								LtDomainConfiguration* pDcRtrFar = NULL);

	void setApp(LtApplication* pApp) { m_pApp = pApp; }
	LtApplication* getApp() { return m_pApp; }

	void setStack(LtStack* pStack) { m_pStack = pStack; }
	LtStack* getStack() { return m_pStack; }

	LtStack* getStack(int index);
};
#endif

#endif	// _LTABASE_H


