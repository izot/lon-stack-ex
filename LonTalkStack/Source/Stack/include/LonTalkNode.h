#ifndef _LonTalkNode_h
#define _LonTalkNode_h
//
// LonTalkNode.h
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

#include <nodedef.h>
#include <assert.h>

// Reset causes
#define LT_CLEARED			0x00
#define LT_POWER_UP			0x01
#define LT_EXTERNAL_RESET	0x02
#define LT_SOFTWARE_RESET	0x14

typedef enum
{
	LT_RTR_MODE_NORMAL,
	LT_RTR_MODE_INIT_SUBNETS,
	LT_RTR_MODE_TEMP_BRIDGE,

	LT_RTR_MODE_TYPES	// Number of router modes
} LtRouterMode;

class LonTalkNode : public NodeDefMaker
{
private:
	LtDeviceStack*		m_pStack;

    LtPlatform			m_platform;
	LtNetworkManager	m_networkManager;
	LtReadOnlyData*		m_pReadOnlyData;
	LtLink*      		m_pDriver;
    
    // Node state information
    int					m_nState;
    LtNetworkImage*		m_pNetImage;
    int					m_nResetCause;
    LtNetworkStats*		m_pNetStats;
    int					m_nCurrentError;
	LtRouterMode		m_rtrMode;
	boolean				m_bServiceOverride;

	LtPersistence		m_persistence;

	bool 				affectsServiceLed;

public:

    void halt();
    void resume();
    void reset(boolean bBoot = false);
    LonTalkNode();
	virtual ~LonTalkNode();

	LtDeviceStack* getStack() { return m_pStack; }
	void setStack(LtDeviceStack* pStack);
	void setCounts(int numAddressEntries, int numDomainEntries, int numMessageTags);

	LtPersistence* getPersistence() { return &m_persistence; }

	void setErrorLog(int error);
    void putErrorLog(int error);
    void errorLog(int error);
	void errorLogConditional(int error);
    void clearErrorLog();
    int getErrorLog();
    void clearResetCause();
    int getResetCause()						{ return m_nResetCause; }

    LtNetworkImage* getNetworkImage()		{ assert(m_pNetImage); return m_pNetImage; }
	LtNetworkStats* getNetworkStats()		{ return m_pNetStats; }
	LtNetworkManager* getNetworkManager()	{ return &m_networkManager; }
	LtReadOnlyData* getReadOnly()			{ return m_pReadOnlyData; }
	LtPlatform*	getPlatform()				{ return &m_platform; }

	LtLink* getDriver()						{ return m_pDriver; }
	void setDriver(LtLink* pDriver)			{ m_pDriver = pDriver; }

    void setServiceLed();
	void winkServiceLed();
	void setServiceOverride(boolean bOverride);
    
    LtErrorType changeState(int newState, boolean bClearKeys);
    void registration(LtLink* pDriver, LtLreServer* pLre);
	boolean unconfigured()					{ return m_pNetImage->unconfigured(); }
	LtRouterMode getRouterMode();
	LtErrorType setRouterMode(LtRouterMode rtrMode);
	LtRouterType getRouterType();
	LtErrorType setRouterType(LtRouterType rtrType);
	void 		setServiceLedImpact(bool flag);
};

#endif
