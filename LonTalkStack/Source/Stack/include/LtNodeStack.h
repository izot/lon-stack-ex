#ifndef _LTNODESTACK_H
#define _LTNODESTACK_H

//
// LtNodeStack.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtNodeStack.h#1 $
//

// Include all the include files you need to use "LtNodeStack" class.

#include <stdio.h>
#include <time.h>
#ifdef WIN32
#include <malloc.h>
#include <memory.h>
#endif
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include <vxWorks.h>
#include <semLib.h>

#include "LtaDefine.h"

#include "VxlTypes.h"
#include "LtObject.h"
#include "LtUniqueId.h"
#include "LtPersistence.h"
#include "LtPersistenceServer.h"
#include "LtBlob.h"
#include "LonTalk.h"
#include "LtMisc.h"
#include "LtCommParams.h"
#include "LtVector.h"
#include "LtMsgOverride.h"
#include "LtDomain.h"
//#include "LtUniqueId.h"
#include "LtProgramId.h"
#include "LtDomainConfiguration.h"
#include "LtAddressConfiguration.h"
#include "LtOutgoingAddress.h"
#include "LtNetworkVariable.h"
#include "LtNetworkVariableConfiguration.h"
#include "LtIncomingAddress.h"
#include "LtPacketControl.h"
#include "LtReadOnlyData.h"
#include "LtApdu.h"
#include "LtMsgTag.h"
#include "LtMsgIn.h"
#include "LtMsgOut.h"
#include "LtRespIn.h"
#include "LtRespOut.h"
#include "LtStatus.h"
#if FEATURE_INCLUDED(MONITOR_SETS)
#include "LtDescription.h"
#include "LtMonitorControl.h"
#include "LtMonitorSet.h"
#include "LtMonitorPoint.h"
#endif
#include "LtApplication.h"

#include "LtChannel.h"
#include "LtInit.h"
#if FEATURE_INCLUDED(IP852)
#include "LtIpChannel.h"
#endif

class LtDeviceStack;

class LTA_EXTERNAL_CLASS LtNodeStack
{
private:
    LtDeviceStack* m_pDeviceStack;
	boolean isNodeStack() { return true; }

public:
	// Constructor
    LtNodeStack(LtLogicalChannel* pChannel, int nRxTx, int nTxTx);

	// Destructor
	virtual ~LtNodeStack();

	// Methods - in alphabetical order
    void                    backupComplete(void);

    void					cancel(LtMsgOut* msg);

    void					cancel(LtRespOut* resp);

	LtErrorType				changeState(int newState);

    LtErrorType				clearStatus();

    void                    clearTransmitTxStats(void);

    void                    clearReceiveTxStats(void); 

	LtErrorType				deregisterAllPrivates();

	LtErrorType				deregisterNetworkVariable(LtNetworkVariable* pNv);

    LtErrorType             disableOma();

    LtErrorType             endDefinition();

    void					flush(boolean commIgnore);

    void					flushCancel();

	LtErrorType				getAddressConfiguration(int index, LtAddressConfiguration* pAc);

	LtErrorType				getConfigurationData(byte* pData, int offset, int length);

	boolean					getDirectCallbackMode();

	LtErrorType				getDomainConfiguration(int index, LtDomainConfiguration* pDc);

	void					getLayerRange(int &min, int &max);
    void                    getNmVersion(int &nmVer, int &capabilities);


	LtNetworkVariable*		getMatchingNetworkVariable(LtNetworkVariable* pNv,
												int nvIndex, int nvSelector, LtOutgoingAddress* pAddress);

#if FEATURE_INCLUDED(MONITOR_SETS)
    LtMonitorSet*			getMonitorSet(LtMsIndex afterMsIndex);

    LtMonitorSet*			getNextMonitorSet(LtMsIndex afterMsIndex);
#endif

	LtNetworkVariable*		getNetworkVariable(int nvIndex, int &arrayIndex);

	LtNetworkVariable*		getNetworkVariable(char* szName, int &arrayIndex);

	boolean					getNetworkVariableNamesUnique();

	// Note that this function doesn't work for private NV configuration but does work for aliases.
	// Hopefully this method can be deprecated.
	LtErrorType				getNetworkVariableConfiguration(int index, LtNetworkVariableConfiguration* pNvc);

	LtErrorType				getOverrideDefaults(int addressIndex, LtMsgOverride* pMsgOverride);

    LtErrorType				getReadOnlyData(LtReadOnlyData* pReadOnlyData);

	boolean					getReceiveAllBroadcasts();

	LtErrorType				getSubnetNode(int domainIndex, int subnetNodeIndex,
							  			  LtSubnetNode &subnetNode);

    boolean                 getTransmitTxStats(int &nMax, int &nFree, int &nPendingFree, int &nMaxAllocated, int &nInstantiated, int &searchRatio, int &txAllocationFailures); // L2 Only

    boolean                 getReceiveTxStats(int &nMax, int &nFree, int &nPendingFree, int &nMaxAllocated, int &nInstantiated, int &searchRatio, int &txAllocationFailures);  // L2 Only

	void					goConfigured();

    void					goOffline();

    void					goUnconfigured();

	int						getXcvrId();

	void					initiateReset();

	boolean					isMyAddress(int domainIndex, LtSubnetNode &subnetNode);

    LtMsgOut*				msgAlloc();

	LtMsgOut*				msgAlloc(boolean bPri, LtBlob* pBlob);

    LtMsgOut*				msgAllocPriority();

    boolean					poll(LtNetworkVariable* pNv, int arrayIndex = 0, LtMsgOverride* pOverride=null);

    void                    prepareForBackup(void);

    void					processApplicationEvents();

    boolean					propagate(LtNetworkVariable* pNv, int arrayIndex = 0, LtMsgOverride* pOverride=null);

    boolean                 readyForBackup(void);

	LtErrorType				registerApplication(int index, LtApplication* lonApp, 
												int numDomainEntries, int numAddressEntries, 
												int numStaticNvEntries, int numDynamicNvEntries,
												int numPrivateNvEntries, int aliasEntries,
												int numMonitorNvEntries, int numMonitorPointEntries,
												int numMonitorSetEntries, int numMessageTags,
												// If you set this to 2 or 3 for a version 1 app, it will go unconfigured.
												int bindingConstraintLevel,	
												const char* pNodeSdString, LtProgramId* pProgramId,
												LtUniqueId* pUniqueId);


    void					registerMemory(int address, int size);

    void registerHookedMessage(int size, const byte *pMsgData); 
    void deregisterHookedMessage(int size, const byte *pMsgData);

	LtErrorType				registerNetworkVariable(LtNetworkVariable* pNv);

	LtErrorType				registerNetworkVariable(LtNetworkVariable* pNv, int nvIndex, 
												int nvSelector, LtOutgoingAddress* pAddress);

    void					release(LtMsgIn* msg);

    void					release(LtRespIn* msg);

    void					reset();

	void					resetPersistence();

    LtRespOut*				respAlloc(LtMsgIn* msg);

	LtRespOut*				respAlloc(boolean bPri, LtBlob* pBlob);

    LtErrorType				retrieveStatus(LtStatus& status);

    void					send(LtRespOut* resp);

    void					send(LtMsgOut* msg, boolean throttle = true);

    void					sendServicePinMessage();

    LtErrorType             sendToXdriver(byte xDriverCommand, void *pData, int len);

    void					setApplicationEventThrottle(boolean value);

    LtErrorType				setAuthenticationKey(int domainIndex, byte* key);

    void					setCommParameters(LtCommParams& commParams);

    void					setDirectCallbackMode(boolean value);

    void                    setDynamicFbCapacity(int dynamicFbCapacity);

    void					setEepromLock(boolean locked);

    void					setErrorLog(int errorNum);

    void                    setExtendedCapabilityFlagOverrides(const byte *pExtendedCapabilityFlags, int length);

    void                    setMaxNmVer(int maxNmVer);

    void					setMessageEventMaximum(int count);

    void					setMessageOutMaximum(int count, int countPri);

	void					setNetworkVariableNamesUnique(boolean bValue);

                            // Used to reduce reported capabilities for node simulation.
    LtErrorType             setNmVersionOverride(int nmVersion, int nmCapabilities);

	void					setPersistencePath(char* szPath);

	void					setReceiveNsaBroadcasts(boolean bValue);

    void					setServiceLed(boolean ledOn);

	void					setTxIdLifetime(int duration);

    void					setXcvrId(int xcvrId);

    void					setXcvrReg(byte* xcvrReg);

	void					shutDown();

	LtErrorType				updateAddressConfiguration(int index, LtAddressConfiguration* pAc);

	LtErrorType				updateConfigurationData(byte* pData, int offset, int length);

	LtErrorType				updateDomainConfiguration(int index, LtDomainConfiguration* pDc);

	// Note that this function doesn't work for private NV configuration but does work for aliases.
	// Hopefully this method can be deprecated.
	LtErrorType				updateNetworkVariableConfiguration(int index, LtNetworkVariableConfiguration* pNvc);

	LtErrorType				updateSubnetNode(int domainIndex, int subnetNodeIndex,
											 const LtSubnetNode &subnetNode);

	LtErrorType				waitForPendingInterfaceUpdates(void);

};

#endif
