#ifndef LTROUTERAPP_H
#define LTROUTERAPP_H

#include <LtStart.h>

//
// LtRouterApp.h
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

class LtRouterApp;

class LtRouterApp: public LtRouterStack, public LtApplication 
{

private:
    static byte siData[10];

	LtProgramId pid;

	LtRouterApp*	m_pOtherSide;	
	int				m_nIndex;
	boolean			m_bResetPending;

protected:

public:
    LtRouterApp(int index, LtLogicalChannel* pChannel, int nManufacturer);
	~LtRouterApp();

	void registerOtherSide(LtRouterApp* pOtherSide) { m_pOtherSide = pOtherSide; }
	LtRouterApp* getOtherSide()						{ return m_pOtherSide; }

	void processEscape(LtMsgIn* pMsg);
	void relayResponse(LtMsgIn* pMsg, int nCode, byte* pData, int nLen);
	void otherSideReset();

    void wink();
    void offline();
    void online();
	void initializing();
    void msgArrives(LtMsgIn* msg);
    void msgCompletes(LtMsgTag* tag, boolean success);
    void respArrives(LtMsgTag* pTag, LtRespIn* response);
    void reset();
    void flushCompletes();
    void servicePinPushed();
    LtProgramId* getProgramId();
    byte* getSiData(int* pLength);
    void applicationEventIsPending();
    boolean readMemory(int address, byte* data, int length);
    boolean writeMemory(int address, byte* data, int length);
	void persistenceLost(LtPersistenceLossReason reason);
};

#if FEATURE_INCLUDED(APP_CONTROL)
class LtRouterAppRegistrar : public LtRouterRegistrar
{
public:
	LtRouterAppRegistrar();

	LtAppControl* createInstance(int nIndex, LtLtLogicalChannel *pLtChannel, LtIpLogicalChannel *pIpChannel);
	const char* getName();
};

class LtRouterAppControl : public LtAppControl
{
private:
	LtRouterApp*	m_pRouterFar;
	LtRouterApp*	m_pRouterNear;
	LtLogicalChannel* m_pIpChannel;

public:
	LtRouterAppControl(int index, LtLogicalChannel* pLtChannel,
		LtLogicalChannel* pIpChannel);

	virtual ~LtRouterAppControl();

	LtLogicalChannel* getIpChannel() { return m_pIpChannel; }

	void activate();
	void deactivate();
	void resetPersistence();
	void sendServicePinMsg();
	LtStack* getStack(int index);
	LtErrorType install(int nDomainIndex, LtDomainConfiguration* pDcNear, 
								LtDomainConfiguration* pDcFar);
	boolean isRouterApp() { return TRUE; }
	LtRouterApp* getRouterAppSide(boolean farSide = FALSE);

	// Should be able to remove once master eliminates direct access to router far side.
	LtRouterApp* getRouterFar() { return m_pRouterFar; }
};

#else

class LtIpLogicalChannel;

class LtIp852Router
{
public:
    LtIp852Router();
    ~LtIp852Router();
    
    LtErrorType Start(int ltAppIndex, LtUniqueId &ltUid, LtLtLogicalChannel *pLtChannel, 
                      int ipAppIndex, LtUniqueId &ipUid, int ipAddress, int ipPort);

    LtErrorType Shutdown();

	void sendServicePinMsg();

private:
	LtRouterApp*	    m_pLtRouter;
	LtRouterApp*	    m_pIpRouter;
    LtIpLogicalChannel* m_pIpChannel;
};

#endif

#endif
