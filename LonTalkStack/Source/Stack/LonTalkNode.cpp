//
// LonTalkNode.cpp
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

#include "LtStackInternal.h"
#include "vxlTarget.h"

void LonTalkNode::halt() {
    getStack()->getLayer4()->halt();
}

void LonTalkNode::resume() {
    getStack()->getLayer4()->resume();
}

void LonTalkNode::setStack(LtDeviceStack* pStack)
{
	m_pStack = pStack;
    m_pNetImage = new LtNetworkImage(pStack);
	m_networkManager.setStack(pStack);
#if PERSISTENCE_TYPE_IS(FTXL)
    m_persistence.setType(LonNvdSegNodeDefinition);
#else
	m_persistence.setName("nodedef");
#endif
	m_persistence.setCommitFailureNotifyMode(true);
	m_persistence.registerPersistenceClient(this);
	this->registerPersistence(&m_persistence);
}

void LonTalkNode::setCounts(int nAddressCount, int nDomainCount, int nMessageTags) 
{ 
    m_pNetImage->domainTable.setCount(m_pStack, nDomainCount);
    m_pNetImage->addressTable.setCount(m_pStack, nAddressCount);
    m_pReadOnlyData = new LtReadOnlyData(m_pStack, nDomainCount, nAddressCount, nMessageTags);
}

void LonTalkNode::reset(boolean bBoot)
{
    m_nResetCause = LT_SOFTWARE_RESET;

	// Do not reload the network image except on a boot.  There is no particular reason
	// to reload on a regular reset - it just takes time.  And there is a problem with
	// doing so in that temporary private NVs are not reloaded (but the application model
	// is that temporaries are only lost on exit.  The disadvantage of not doing the reload
	// is that testing persistence becomes more difficult.  This will be done via a special
	// command.
	if (bBoot)
	{
		m_pNetImage->reset(*this);
	}
    
    m_pNetStats->reset();
    
    if (m_pNetImage->getState() != LT_CONFIGURED) {
        getStack()->resetNotConfigured();
    }
    
    getStack()->getLayer4()->reset();
    
	m_platform.reset();
    
	m_bServiceOverride = false;

    setServiceLed();

    m_rtrMode = LT_RTR_MODE_NORMAL;
}

LonTalkNode::LonTalkNode() : m_persistence(CURRENT_NODEDEF_VER)
{
	m_pNetStats = null;
	m_pNetImage = null;
	m_pReadOnlyData = null;
	m_nCurrentError = 255;
	m_bServiceOverride = false;
	affectsServiceLed = true;
}

LonTalkNode::~LonTalkNode()
{
	delete m_pNetStats;
	delete m_pNetImage;
	delete m_pReadOnlyData;
}


void
LonTalkNode::setServiceLedImpact(bool flag)
{
	affectsServiceLed = flag;
}

int LonTalkNode::getErrorLog()
{
	if (m_nCurrentError == 255)
	{
		m_nCurrentError = m_platform.getErrorLog();
	}
	return m_nCurrentError;
}

void LonTalkNode::putErrorLog(int error)
{
	getStack()->getLayer4()->setErrorLog(error);
}

void LonTalkNode::setErrorLog(int error) 
{
	if (error)
	{
		vxlReportEvent("LonTalk Stack error logged: %d\n", error);
	}
	if (error != m_nCurrentError)
	{
		m_nCurrentError = error;
		m_platform.setErrorLog(error);
	}
}

void LonTalkNode::errorLogConditional(int error)
{
	if (getErrorLog() == LT_NO_ERROR)
	{
		putErrorLog(error);
	}
}

void LonTalkNode::errorLog(int error) 
{
    if ((error&LT_ERROR_LOG_MASK) != 0) {
        putErrorLog(error);
    }
}

void LonTalkNode::clearErrorLog() {
	putErrorLog(LT_NO_ERROR);
}

void LonTalkNode::clearResetCause() {
    m_nResetCause = LT_CLEARED;
}

void LonTalkNode::setServiceOverride(boolean bOverride)
{
	m_bServiceOverride = bOverride;
}

void LonTalkNode::winkServiceLed()
{
	m_pDriver->setServicePinState(SERVICE_FLICKER);
    getStack()->setServicePinState(SERVICE_FLICKER);
}

void LonTalkNode::setServiceLed() 
{
	LtServicePinState state = SERVICE_OFF;

	if (!affectsServiceLed)
		return;
	// Layer 2 shouldn't contribute nothing or minimum to state.
	// minimum is better test so use it.
	if (!getStack()->isLayer2())
	{
		if (m_bServiceOverride)
		{
			state = SERVICE_ON;
		}
		else
		{
			switch (m_pNetImage->getState()) 
			{
				case LT_UNCONFIGURED:
					state = SERVICE_BLINKING;
					break;
					break;
				case LT_APPLICATIONLESS:
					state = SERVICE_ON;
					break;
			}
		}
	}
	m_pDriver->setServicePinState(state);
    getStack()->setServicePinState(state);
}

LtErrorType LonTalkNode::changeState(int newState, boolean bClearKeys) 
{
	LtErrorType err = LT_NO_ERROR;

    if (newState == LT_CONFIGURED) 
	{
        getStack()->goOfflineConditional();
    }
    err = m_pNetImage->changeState(newState, bClearKeys);

    setServiceLed();

	return err;
}

void LonTalkNode::registration(LtLink* pDriver, LtLreServer* pLre) {
	m_pDriver = pDriver;
    m_pNetStats = new LtNetworkStats(pDriver, pLre);
}

LtRouterMode LonTalkNode::getRouterMode()
{
	return m_rtrMode;
}

LtRouterType LonTalkNode::getRouterType()
{
	return m_pNetImage->configData.getRouterType();
}

//
// setRouterType
//
// Method to set the type of a router (repeater, bridge, etc.).  Used for self-installation of routers.
//
LtErrorType LonTalkNode::setRouterType(LtRouterType rtrType)
{
	LtErrorType err = m_pNetImage->configData.setRouterType(rtrType);

	if (err == LT_NO_ERROR)
	{
		// Store the change persistently and notify clients of change as necessary
		m_pNetImage->store();
		getStack()->routerModeChange();
	}
	return err;
}

LtErrorType LonTalkNode::setRouterMode(LtRouterMode rtrMode)
{
	LtErrorType err = LT_NO_ERROR;

	switch (rtrMode)
	{
	case LT_RTR_MODE_NORMAL:
	case LT_RTR_MODE_TEMP_BRIDGE:
		if (m_rtrMode != rtrMode)
		{
			m_rtrMode = rtrMode;
			// Notify the engine so that it can do it's thing.
			getStack()->routerModeChange();
		}
		break;
	case LT_RTR_MODE_INIT_SUBNETS:
		// Force temp version of routing tables to be refreshed from the
		// persistent version.
		getStack()->resetRoutes();
		break;
	default:
		err = LT_INVALID_PARAMETER;
		break;
	}

	return err;
}
