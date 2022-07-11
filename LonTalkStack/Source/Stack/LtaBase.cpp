//
// ltaBase.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtaBase.cpp#1 $
//

#include "ltaBase.h"
#include "vxlTarget.h"
#include "VxLayer.h"
#include "LtPersistenceServer.h"
#if defined(ILON_PLATFORM) && defined (__VXWORKS__)
#include "LonLinkObserver.h"
#endif

#if PRODUCT_IS(ILON)
void LtaBaseControl::sendServicePinMsg()
{
	if (getStack())
	{
		getStack()->sendServicePinMessage();
	}
}

void LtaBaseControl::deactivate()
{
	if (getStack())
	{
		getStack()->shutDown();
	}
	if (getApp())
	{
		delete getApp();
	}
	setStack(NULL);
	setApp(NULL);
}

void LtaBaseControl::resetPersistence()
{
	if (getStack())
	{
		getStack()->shutDown();
		getStack()->resetPersistence();
	}
}

// This base class method only deals with a single stack.
// The Router derived class overrides this to handle both stacks.
LtErrorType LtaBaseControl::install(int nIndex, LtDomainConfiguration* pDcMain,
									LtDomainConfiguration* pDcRtrFar)
{
	LtErrorType err = LT_NOT_FOUND;

	if (pDcMain->getSubnet() == 0 || pDcMain->getNode() == 0)
	{
		err = LT_INVALID_ADDRESS;
	}
	else if (getStack())
	{
		// First, force it unconfigured so that the install will always work.
		// Without this, it failed if the state was hard-offline.
		getStack()->goUnconfigured(); 
		err = getStack()->updateDomainConfiguration(nIndex, pDcMain, true);
		getStack()->goConfigured();
	}
	return err;
}

LtStack* LtaBaseControl::getStack(int index)
{
	if (index == 0)
	{
		return getStack();
	}
	return NULL;
}
#endif

//
// Static routine needed for processing VxWorks timers
//
int	VXLCDECL LtaTimer::timeout( int object, ... )
{
	LtaTimer* pTimer =(LtaTimer*) object;
	pTimer->expired();
	return 0;
}

//
// Static routine needed for starting app task
//
int VXLCDECL LtaBase::appTask( int app, ... )
{
	LtaBase* pApp = (LtaBase*) app;

	// Delay here until the derived class is
	// finished with its initialization.  This
	// prevents run() from being called before
	// the virtual function table is fixed up.
	while (!pApp->isInitialized()) 
	{
		taskDelay(1);
	}
	pApp->run();
	return 0;
}

LtaBase::LtaBase(const char* szName, int priority, int stacksize, LtLogicalChannel* pChannel, int nRx, int nTx) : LtAppNodeStack(pChannel, nRx, nTx)
{
	// This app has a single task which processes events and timeouts
	m_semEvent = semBCreate(0, SEM_EMPTY);

	// A recommended priority is 120.  Task stack size of 3000 should
	// work for LTA.  Obviously, if you have significant stack requirements
	// this may need to be increased.
	// recommended stack size is 16kB.
	if (priority == 0)
	{
		priority = 120;
	}
	if (stacksize == 0)
	{
		stacksize = 16*1024;
	}
	m_bAppShutdownRequest = false;
	m_bAppShutdown = false;
	strncpy(m_szName, szName, sizeof(m_szName));
	m_szName[sizeof(m_szName)-1] = 0;

	// ACHTUNG!  Your derived class should set this to "true" when
	// you have finished initializing the class.  This variable
	// prevents run() from being called before the virtual function
	// table is initialized. - JV
	m_bAppInitialized = false;
#ifdef MIPS32
	
	LonLinkObserver::getInstance(this);

#endif

	m_tidApp = vxlTaskSpawn(szName, priority, 0, stacksize, appTask,
		(int)this, 0,0,0,0, 0,0,0,0,0);
}

LtaBase::~LtaBase()
{
	stopApp();
	semDelete(m_semEvent);
}

void LtaBase::stopApp()
{
	m_bAppShutdownRequest = true;
	while (!m_bAppShutdown) 
	{
		applicationEventIsPending();
		taskDelay(1);
	}
    shutDown();
}

boolean LtaBase::appShutdown()
{
	if (m_bAppShutdownRequest)
	{
		m_bAppShutdown = true;
		return true;
	}
	return false;
}

void LtaBase::eventWait()
{
	semTake(m_semEvent, WAIT_FOREVER);
}

void LtaBase::msgArrives(LtMsgIn* pMsg)
{
	release(pMsg);
}

void LtaBase::respArrives(LtMsgTag* pTag, LtRespIn* pResp)
{
	release(pResp);
}

void LtaBase::applicationEventIsPending() 
{
	semGive(m_semEvent);
}

void LtaBase::persistenceLost(LtPersistenceLossReason reason)
{
	if (reason != LT_NO_PERSISTENCE)
	{
		vxlReportUrgent("%s: persistent data lost due to %s.\n    Suggested action: recommission the application instance.\n", m_szName, LtPersistence::getPersistenceLostReason(reason));
	}
}

void LtaBase::wink()
{
#if defined(WIN32) || !PRODUCT_IS(ILON)
	winkServiceLed();
#else
	Notify();  // notify observer
#endif
}
