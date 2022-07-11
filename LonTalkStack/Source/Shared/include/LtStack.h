#ifndef _LTSTACK_H
#define _LTSTACK_H

//
// LtStack.h
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

#include "LtaDefine.h"

#include "LtBlob.h"
#include "LtRouter.h"

#include "LtPersistence.h"
#include "LtDriver.h"
#include "LtMisc.h"
#include "LtMsgOverride.h"
#include "LtDomainConfiguration.h"
#include "LtAddressConfiguration.h"
#include "LtOutgoingAddress.h"
#include "LtIncomingAddress.h"
#include "LtNetworkVariable.h"
#include "LtNetworkVariableConfiguration.h"
#include "LtPacketControl.h"
#include "LtApdu.h"
#include "LtMsgTag.h"
#include "LtMsgIn.h"
#include "LtMsgOut.h"
#include "LtRespIn.h"
#include "LtRespOut.h"
#include "LtStatus.h"
#include "LtNetworkStats.h"
#if FEATURE_INCLUDED(MONITOR_SETS)
#include "LtMonitorSetTable.h"
#endif
#include "LtApplication.h"

class LtStack;

class LtStack 
{
public:
	virtual ~LtStack() {}

    virtual LtErrorType registerApplication(int index, LtApplication* lonApp, 
			int numDomainEntries, int numAddressEntries,
			int numStaticNvEntries, int numDynamicNvEntries,
            int numPrivateNvEntries, int aliasEntries,
			int numMonitorNvEntries, int numMonitorPointEntries,
			int numMonitorSetEntries, int numMessageTags,
			int bindingConstraintLevel,
			const char* pNodeSdString, LtProgramId* pProgramId) = 0;

	virtual LtErrorType registerNetworkVariable(LtNetworkVariable* pNv) = 0;
	virtual LtErrorType registerNetworkVariable(LtNetworkVariable* pNv, int nvIndex, 
									int nvSelector, LtOutgoingAddress* pAddress) = 0;
	virtual LtErrorType deregisterNetworkVariable(LtNetworkVariable* pNv) = 0;
	virtual LtErrorType deregisterAllPrivates() = 0;

	virtual LtNetworkVariable* getNetworkVariable(int nvIndex, int &arrayIndex) = 0;
	virtual LtNetworkVariable* getNetworkVariable(char* szName, int &arrayIndex) = 0;
    virtual boolean getCurrentNvLengthFromApp(LtNetworkVariable* pNv, int &length)= 0;

    virtual void registerMemory(int address, int size) = 0;
	virtual int getXcvrId() = 0;
    virtual void setXcvrId(int xcvrId) = 0;
    virtual void setCommParameters(LtCommParams& commParams) = 0;
    virtual void setXcvrReg(byte* xcvrReg) = 0;
	virtual void setTxIdLifetime(int duration) = 0;
    virtual LtErrorType setAuthenticationKey(int domainIndex, byte* key) = 0;
    virtual void setApplicationEventThrottle(boolean value) = 0;
    virtual void setDirectCallbackMode(boolean value) = 0;
    virtual void processApplicationEvents() = 0;
    virtual void release(LtMsgIn* msg) = 0;
    virtual void release(LtRespIn* msg) = 0;
    virtual void doNvUpdates(int type) = 0;
    virtual boolean propagatePoll(boolean poll, int index) = 0;
    virtual boolean propagate(int index) = 0;
	virtual boolean propagate(LtNetworkVariable* pNv, int arrayIndex=0, LtMsgOverride* pOverride=null) = 0;
    virtual boolean poll(int index) = 0;
	virtual boolean poll(LtNetworkVariable* pNv, int arrayIndex=0, LtMsgOverride* pOverride=null) = 0;
    virtual boolean getOffline() = 0;
    virtual void setOffline(boolean offline) = 0;
    virtual void setMessageEventMaximum(int count) = 0;
    virtual void setMessageOutMaximum(int count, int countPri) = 0;
    virtual LtMsgOut* msgAlloc() = 0;
    virtual LtMsgOut* msgAllocPriority() = 0;
    virtual void send(LtMsgOut* msg, boolean throttle = true) = 0;
    virtual void cancel(LtMsgOut* msg) = 0;
    virtual LtRespOut* respAlloc(LtMsgIn* msg) = 0;
    virtual void send(LtRespOut* resp) = 0;
    virtual void cancel(LtRespOut* resp) = 0;
    virtual void sendResponse(LtMsgIn* request, byte* response, int len) = 0;
    virtual LtErrorType clearStatus() = 0;
    virtual LtErrorType getReadOnlyData(byte* readOnlyData) = 0;
    virtual void reset() = 0;
    virtual void setEepromLock(boolean locked) = 0;
    virtual void setErrorLog(int errorNum) = 0;
    virtual LtErrorType retrieveStatus(LtStatus& status) = 0;
    virtual void setServiceLed(boolean ledOn) = 0;
    virtual void flush(boolean commIgnore) = 0;
    virtual void flushCancel() = 0;
    virtual void goOffline() = 0;
    virtual void goUnconfigured() = 0;
	virtual void goConfigured() = 0;
    virtual void shutDown() = 0;
	virtual void setPersistencePath(const char* szPath) = 0;
    virtual void sendServicePinMessage() = 0;
	virtual void resetPersistence() = 0;
	virtual boolean isBound(LtNetworkVariable* pNv, int arrayIndex=0, int flags=ISBOUND_ANY) = 0;

	virtual LtErrorType getDomainConfiguration(int nIndex, LtDomainConfiguration* pDc) = 0;
	virtual LtErrorType updateDomainConfiguration(int nIndex, LtDomainConfiguration* pDc, boolean bStore = false, boolean bRestore = false) = 0;

	virtual LtErrorType getOverrideDefaults(int addressIndex, LtMsgOverride* pMsgOverride) = 0;
	virtual LtErrorType getBoundNvConfiguration(LtNetworkVariable* pNv, int arrayIndex, LtNetworkVariableConfiguration* &pNvc) = 0;

	virtual LtLogicalChannel* getChannel() = 0;
	
	virtual void getLayerRange(int &min, int &max) = 0;
    virtual void getNmVersion(int &nmVer, int &capabilities) = 0;

	// Creates an instance of a derived class of this base class.
	static LtStack* create(LtLogicalChannel* pChannel, boolean bRouterSide=false, int nNumReceiveTxs=50, int nNumTransmitTxs=300);
	static LtStack* createLayer2(LtLogicalChannel* pChannel);
};

#endif
