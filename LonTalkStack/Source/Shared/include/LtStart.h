#ifndef LTSTART_H
#define LTSTART_H

//
// LtStart.h
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

#ifdef __cplusplus
extern "C" {
#endif
int ltstart();
#ifdef __cplusplus
};
#endif

#ifdef __cplusplus

#if !FEATURE_INCLUDED(APP_CONTROL)

// Just a stub for FTXL...
class LtStart
{
public:
    static boolean properEnvironment() { return true; }
};

#else // FEATURE_INCLUDED(APP_CONTROL)
//
// Number of app instances which can be created.
//
#define NUM_APP_INSTANCES 16
// Define order numbers for known iLON apps to perserve LUID allocation order
#define APP_REG_ORDER_ROUTER 0
#define APP_REG_ORDER_LTAHOST 1
#define APP_REG_ORDER_RNI 2
#define APP_REG_ORDER_LTAPERF 3
#define APP_REG_ORDER_DEFAULT NUM_APP_INSTANCES

class LtStack;
class LtRouterApp;

//
// LonTalk Application base class
//
class LtAppControl
{
private:
	int m_nIndex;
	LtLogicalChannel* m_pChannel;
public:
	LtAppControl(int nIndex, LtLogicalChannel* pChannel) : m_nIndex(nIndex),
		m_pChannel(pChannel) {}

	int getIndex() { return m_nIndex; }
	LtLogicalChannel* getChannel() { return m_pChannel; }

	virtual ~LtAppControl() {}
	virtual void activate() = 0;
	virtual void deactivate() = 0;
	virtual void sendServicePinMsg() = 0;
	virtual void resetPersistence() = 0;
	virtual LtErrorType install(int nDomainIndex, LtDomainConfiguration* pDcMain, 
								LtDomainConfiguration* pDcRtrFar = NULL) 
		{ return LT_NOT_IMPLEMENTED; }
	virtual LtStack* getStack(int index) = 0;

	// Add a way to get to a router without modifing the LtStack class chain
	virtual boolean isRouterApp() { return FALSE; }
	virtual LtRouterApp* getRouterAppSide(boolean farSide = FALSE) { return NULL; }
};

class LtRegistrar
{
private:
	boolean	m_bRouter;
	int m_nOrder;	// Applications must be ordered

public:
	LtRegistrar(boolean bRouter) : m_bRouter(bRouter) {}
	virtual ~LtRegistrar(){};
	boolean getIsRouter() { return m_bRouter; }

	virtual const char* getName() = 0;
	virtual int getMaxInstances() = 0;
	// Return 0 if not a LonTalk app
	virtual int getNumberOfLonTalkIndices() = 0;
	virtual boolean getStartByDefault() = 0;
	virtual boolean getDefaultToIp() = 0;
	virtual int getOrder() = 0;
};

class LtAppRegistrar : public LtRegistrar
{
public:
	LtAppRegistrar() : LtRegistrar(false) {}

	virtual LtAppControl* createInstance(int index, LtLogicalChannel* pChannel) = 0;

	virtual const char* getName() = 0;

	virtual int getMaxInstances() { return -1; }
	virtual int getNumberOfLonTalkIndices() { return 1; }
	virtual boolean getStartByDefault() { return true; }
	virtual boolean getDefaultToIp() { return false; }
	virtual int getOrder() {return APP_REG_ORDER_DEFAULT; }
};

class LtIpLogicalChannel;

class LtRouterRegistrar : public LtRegistrar
{
public:
	LtRouterRegistrar() : LtRegistrar(true) {}
	virtual ~LtRouterRegistrar(){};

	virtual LtAppControl* createInstance(int index, LtLtLogicalChannel* pLtChannel,
		LtIpLogicalChannel* pIpChannel) = 0;

	virtual const char* getName() { return "Router"; }

	virtual int getMaxInstances() { return 1; }
	virtual int getNumberOfLonTalkIndices() { return 2; }
	virtual boolean getStartByDefault() { return true; }
	virtual boolean getDefaultToIp() { return false; }
	virtual int getOrder() {return APP_REG_ORDER_ROUTER; }
};

// 
// Data structure used for control of applications
//
#define APP_NAME_SIZE 17

class LtAppRegistryEntry
{
private:
	int m_nCount;
public:
	void init() { m_nCount = 0; }
	char m_szName[APP_NAME_SIZE];	// Application name, null terminated
	LtRegistrar* m_pRegistrar;	// Application registration class
	int m_nOrder;
	
	int getCount() { return m_nCount; }
	void incCount() { m_nCount++; }
	void decCount() { m_nCount--; }
};

//
// Number of app types which can be registered.  Note that there may be multiple
// apps of a given type.
//
#define NUM_APP_TYPES 16
#define APP_KEY_SIZE  10

// Note the lack of constructor.  This is because initialization of the registry
// must be done explicitly since static constructors may invoke this code before
// its constructor runs.
class LtAppRegistry
{
private:
	int m_nCount;
	LtAppRegistryEntry m_entries[NUM_APP_TYPES];
public:
	void init() { m_nCount = 0; }
	LtAppRegistryEntry* add(const char* szName, int nOrder=APP_REG_ORDER_DEFAULT);
	LtAppRegistryEntry* find(const char* szName);
	LtAppRegistryEntry* get(int nIndex);
	void show();
	void showIdle(const char* szFormat);
	int getCount() {return m_nCount;}
};

class LtAppInfo
{
public:
	LtAppInfo()
	{
		m_bValid = false;
	}
	LtAppInfo(const char* szName, boolean bIpSide, boolean bActive, boolean bRouter, int nCount)
	{
		m_bValid = true;
		strncpy(this->m_szName, szName, sizeof(this->m_szName));
		this->m_bIpSide = bIpSide;
		this->m_bActive = bActive;
		this->m_bRouter = bRouter;
		this->m_nCount = nCount;
	}
	boolean m_bValid;
	char m_szName[APP_NAME_SIZE];
	boolean m_bIpSide;
	boolean m_bRouter;
	boolean m_bActive;
	int m_nCount;
};

class LtInit; // Generic class reference for LtStart to avoid header file requirement

typedef struct AppSlotStatus
{ 
	byte m_luid[LT_UNIQUE_ID_LENGTH];
	char m_szName[APP_NAME_SIZE];
	bool m_bActive;
	bool m_bReserved;
} AppSlotStatus;

// Global routines not under LtStart (for DCI access)
boolean isLuidRNI(byte *pLuid);
boolean isLuidLocal(byte *pLuid);
// Global routines for FPM app management
boolean getAppSlotStatus(int index, AppSlotStatus *pAppInfo);
int getAppIndexFromLuid(byte *pLuid);

// See comment above on why there is no static constructor.  Rely on load
// time setting of m_bInit to 0.
class LtStart
{
	friend boolean isLuidRNI(byte *pLuid);
	friend boolean isLuidLocal(byte *pLuid);
	friend boolean getAppSlotStatus(int index, AppSlotStatus *pAppInfo);
	friend int getAppIndexFromLuid(byte *pLuid);

private:
	static boolean						m_bInit;
	static LtAppRegistry				m_registry;
	static LtAppControl* 				m_apps[NUM_APP_INSTANCES+1];
	static LtAppRegistryEntry*			m_regs[NUM_APP_INSTANCES+1];
	static LtUniqueId					m_luidTable[NUM_APP_INSTANCES];
	static boolean						m_bLuidTableFilled;
	static LtInit*						m_pInit;
	static const char*					m_pLonPortName;
	static int							m_nLastRniIndex;

	static void makeAppKey(char* szKey, int nIndex);
	static int getIndex(int startIndex, int nIndexRequest, int nCount);
	static LtAppControl* getActiveAppCtrl(int nIndex, boolean bActive=true);
	static void deleteAppCtrl(int nIndex);
	static int spawnApp(const char* szName, int nIndex, boolean bIpSide, boolean bActive, boolean bVerbose=false, boolean bRemote=false, int nIndexRequest = -1);
	static int findApp(const char* szAppName, int nSubIndex);
	static boolean verifyAppsStarted();
	static void cleanupDeletedImage(int nIndex);

	static void setAppInfo(int nIndex, LtAppInfo* pInfo);

public:	// Exposed App Ctrl functions
	static LtErrorType getAppInfo(int nIndex, LtAppInfo* pInfo);
	static LtAppControl* getAppCtrl(int nIndex, boolean bReportError=true);
	static boolean getAppCtrlIndex(const char* szName, int &nIndex, boolean bReportError=true);

public:
	static boolean properEnvironment();
	static boolean appsStarted();
	static boolean luidTableFilled() { return m_bLuidTableFilled; }
	static void fillLuidTable();
	static void getAppSlotList();

	static int run(boolean bStartApp=true);
	static int stop();
	static int deleteall();

	static LtErrorType registerApp(LtRegistrar* pRegistrar);
	static LtLreServer* getLre(LtLogicalChannel* pChannel);

	static int appRegistryShow();
	static int appInstanceShow(boolean bShowAll=false);
	static void getDomainSubnetNode(char *appName, char *returnStr);
	static void getDomainSubnetNode(int appIndex, int stackIndex, char* returnStr);

	static boolean validIndex(const char* szNameOrIndex, int &nIndex, boolean bReportError=true, boolean bUnloadedAreValid=false);
	static int createApp(const char* szName, boolean bIpSide, boolean bRemote=false, int nIndexRequest = -1);
	static void deleteApp(int nIndex, boolean bNotify = false);
	static void sendServicePinMsg(int nIndex);
	static void sendServicePinMsgAll();
	static void activate(int nIndex, boolean bRemote=false, boolean bVerbose=true, boolean bForce=false);
	static void deactivate(int nIndex, boolean bRemote=false, boolean bVerbose=true);
	static boolean activateApp(const char* szAppName, int nSubIndex, boolean bRemote=false, boolean bVerbose=true);
	static boolean deactivateApp(const char* szAppName, int nSubIndex, boolean bRemote=false, boolean bVerbose=true);
	static LtErrorType install(int nAppIndex, int nDomainIndex, LtDomainConfiguration* pDcMain,
								LtDomainConfiguration* pDcRtrFar = NULL);
	static void goUnconfigured(int nIndex);
	static int getAppState(int index, BOOL masked = FALSE);
	static LtErrorType getRouterType(int index, LtRouterType *pType);
	static void setNoPhysicalLonPort();
	static void setLonPortName(char *name);

};
#endif // !PRODUCT_IS_FTXL

#endif

#endif
