#ifndef _LTDEVICESTACK_H
#define _LTDEVICESTACK_H

//
// LtDeviceStack.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtDeviceStack.h#3 $
//

#if FEATURE_INCLUDED(MONITOR_SETS)
#include "LtMonitorSetTable.h"
#include "LtMonitorPointTable.h"
#endif

class LtMsgFilter : public LtObject
{
public:
    LtMsgFilter(int size, const byte *pMsgData);
    virtual ~LtMsgFilter();
    
    boolean matches(int size, const byte *pMsgData, boolean exact);

private:
    int   m_size;
    byte *m_pMsgData;
};

#define LT_DEVICE_INIT_OPT_PRESERVE_AUTH       0x80

#define DEFAULT_LS_ADDR_MAPPING_ANNOUNCEMENT_FREQUENCY (5*60*1000) // 5 minutes
#define DEFAULT_LS_ADDR_MAPPING_ANNOUNCEMENT_THROTTLE (100) 


class LtDeviceStack : 
	public LtStack, 
	public LonTalkStack,
	public LtLayer6, 
	public LtLreClientBase, 
	public NodeDefClient, 
#if FEATURE_INCLUDED(MONITOR_SETS)
    public LtMonitorControlClient,
#endif
	public LtConfigurationEntity
{
	friend class LtStackClient;
	friend class LtNetworkManager;

private:
	LtLogicalChannel*	m_pChannel;
	LtLreServer*		m_pLre;
	LtLreClient*		m_pMainClient;
    SEM_ID              m_sem;
	LtLayer4Intf*		m_pLayer4;

    boolean				m_bOffline;
    boolean				m_bGoOfflineConditional;
	boolean				m_bMessageLock;
    LtApplication*		lonApp;
    MSG_Q_ID			m_queApdus;
    LtNvMap 			nvUpdates[LT_PRIORITY_TYPES];
    int					maxApdu;
	LtPersistenceLossReason	m_nPersistenceLost;
    boolean				m_bDirectCallbackMode;
    boolean				m_bServicePinDepressed;
    boolean             m_bServicePinReleased;
    boolean				m_bFlushCompleted;
    boolean				m_bResetOccurred;
    int					maxPublicNvIndex;
    int					maxPrivateNvIndex;
	int					m_nIndex;
	DynamicNvs*			m_pDynamicNvs;
#if FEATURE_INCLUDED(MONITOR_SETS)
    LtMonitorSetTable   m_monitorSetTable;
	LtMonitorPointTable m_monitorPointTable;
#endif

	LtMsgTag*			m_pLocalOpMsgTag;
    LtErrorType         m_localOpErr;
    SEM_ID              m_localOpSem;
	LtStatus			m_currentStatus;
    char               *m_szPersistencePath;
    bool                m_shutDown;
    bool				m_hasExclusionUid;
    LtUniqueId			m_exclusionUid;

    // LS/IP address mapping configuration - in seconds
    ULONG               m_lsAddrMappingAnnounceFreq;      // Frequency of LS/IP address mapping announcements
    WORD                m_lsAddrMappingAnnounceThrottle;  //  Minimum time between consecutive announcements
    ULONG               m_lsAddrMappingAgeLimit;          // Age limit on learned arbitrary IP addresses.

    LtTypedVector<LtMsgFilter>  m_vecHookedMsgs;  // Messages that have been "hooked" by application

#if PRODUCT_IS(ILON)
	bool				m_propertyChangeHostEventFlag;	// for notifying the host about selected changes
#endif

	LtErrorType localStackOperation(int op, LtServiceType st);

	int m_nvIndex;
	int m_privateNvIndex;

    int maxMsg[LT_PRIORITY_TYPES];
    int msgCount[LT_PRIORITY_TYPES];
    LtErrorType handleNvUpdates(LtApduIn* apdu);
    LtErrorType processApdu(LtApduIn* apdu);
    LtErrorType dynamicNv(int cmd, byte* pData, int dataLen, byte* pResp, int &respLen);
    void eventNotification();
    boolean adjustBufLimit(boolean bPri, boolean bAdd, boolean bForce = false);

	boolean propagatePoll(boolean poll, LtNetworkVariable* pNv, int arrayIndex, LtMsgOverride* pOverride=null); 
    boolean propagatePoll(boolean poll, int index);

#if FEATURE_INCLUDED(MONITOR_SETS)
    // Monitor Set callbacks
    LtMonitorSet *monSetAdded(LtMonitorSet *pMonitorSet);
    void monSetChanged(LtMonitorSet *pMonitorSet, McChangeType type);
    void monSetDeleted(LtMonitorSet *pMonitorSet);

    LtMonitorPoint *monPointAdded(LtMonitorPoint *pMonitorPoint);
    void monPointChanged(LtMonitorPoint *pMonitorPoint, McChangeType type);
    void monPointDeleted(LtMonitorPoint *pMonitorPoint);
#endif

#if FEATURE_INCLUDED(LTEP)
	void processProxyRepeaterAsAgent(LtApduIn* pApdu, int txcpos);
#endif

    LtMsgFilter *findMatchingFilter(int size, const byte *pMsgData, boolean exact); 

    LtServicePinState	m_nServicePinState; 
protected:
    LtMsgOut* msgAllocGen(boolean pri, boolean bForce = false);
    void release(LtApduOut* msg);
    void release(boolean pri);
	void processPacket(boolean pPriority, LtPktInfo* pPkt);
	void updateStats(const byte* pApdu, bool isUnackd, bool domainMatch, bool gotThismsg);
#if FEATURE_INCLUDED(MONITOR_SETS)
	LtMonitorSetTable* getMonitorSetTable() { return &m_monitorSetTable; }
	LtMonitorPointTable* getMonitorPointTable() { return &m_monitorPointTable; }
#endif

public:
    LtDeviceStack(LreClientType clientType, LtLogicalChannel* pChannel, LtLreServer* pLre, 
                  int nRxTx, int nTxTx, 
                  int maxL4OutputPackets = 100,
                  int maxL4PriorityOutputPackets = 100);
	virtual ~LtDeviceStack();

	boolean stop();
    void sync();

	int getIndex() { return m_nIndex; }

	LtLogicalChannel* getChannel()  { return m_pChannel; }
	LtLreServer* getLre()			{ return m_pLre; }
    LtApplication* getApp();
	LtErrorType registerApplication(int index, LtApplication* lonApp, 
		int numDomainEntries, int numAddressEntries, 
		int numStaticNvEntries, int numDynamicNvEntries,
        int numPrivateNvEntries, int aliasEntries,
		int numMonitorNvEntries, int numMonitorPointEntries,
		int numMonitorSetEntries, int numMessageTags,
		int bindingConstraintLevel,	// If you set this to 2 or 3 for a version 1 app, it will go unconfigured.
		const char* pNodeSdString, LtProgramId* pProgramId);

	LtErrorType registerNetworkVariable(LtNetworkVariable* pNv);
	LtErrorType registerNetworkVariable(LtNetworkVariable* pNv, int nvIndex, 
									int nvSelector, LtOutgoingAddress* pAddress);
	LtErrorType deregisterNetworkVariable(LtNetworkVariable* pNv);
	LtErrorType deregisterAllPrivates();

	LtNetworkVariable* getNetworkVariable(int nvIndex, int &arrayIndex);
	LtNetworkVariable* getNetworkVariable(char* szName, int &arrayIndex);
    LtErrorType getNetworkVariableDirection(int nvIndex, boolean &bOutput);
    void getDefaultNetworkVariableConfigAttributes(LtNetworkVariable* pNv, 
                                                          LtNetworkVariableConfiguration &nvc);
    LtErrorType getDefaultNetworkVariableConfigAttributes(int nvIndex, 
                                                   LtNetworkVariableConfiguration &nvc);
	LtNetworkVariable* getMatchingNetworkVariable(LtNetworkVariable* pNv,
		int nvIndex, int nvSelector, LtOutgoingAddress* pAddress);

    void registerMemory(int address, int size);
    void registerHookedMessage(int size, const byte *pMsgData); 
    void deregisterHookedMessage(int size, const byte *pMsgData);
    boolean isMsgHooked(LtApdu& apdu);

	int getXcvrId();
    void setXcvrId(int xcvrId);
    void setCommParameters(LtCommParams& commParams);
    void setXcvrReg(byte* xcvrReg);
	void setTxIdLifetime(int duration);
    LtErrorType setAuthenticationKey(int domainIndex, byte* key);
    void setApplicationEventThrottle(boolean value);
	boolean getDirectCallbackMode() { return m_bDirectCallbackMode; }
    void setDirectCallbackMode(boolean value);
    void processApplicationEvents();
    void release(LtMsgIn* msg);
    void release(LtRespIn* msg);
    void doNvUpdates(int type);
	boolean propagate(int nvIndex);
	boolean poll(int nvIndex);
    boolean propagate(LtNetworkVariable* pNv, int arrayIndex = 0, LtMsgOverride* pOverride=null);
    boolean poll(LtNetworkVariable* pNv, int arrayIndex = 0, LtMsgOverride* pOverride=null);
	boolean isBound(LtNetworkVariable* pNv, int arrayIndex=0, int flags=ISBOUND_ANY);
    void goOfflineConditional();
    void resetNotConfigured();
    void registerLonTalkStack(LonTalkStack& stack);
    boolean getOffline();
    void setOffline(boolean offline);
    void servicePinDepressed();
    void servicePinReleased();
    virtual void sendServicePinMessage();
	void setReceiveNsaBroadcasts(boolean bValue);
	boolean getReceiveNsaBroadcasts();
	void setReceiveAllBroadcasts(boolean bValue);
	boolean getReceiveAllBroadcasts();
    void flushCompleted();
    void resetOccurred();
    void receive(LtApduIn* apdu);
    void setMessageEventMaximum(int count);
    LtApduIn* getApdu(boolean bResponse = false, boolean bIgnoreThrottle = false);
    void setMessageOutMaximum(int count, int countPri);
    LtMsgOut* msgAlloc();
    LtMsgOut* msgAllocPriority();
    LtRespOut* respAlloc(boolean priority, LtBlob *pBlob);
    LtMsgOut* msgAlloc(boolean priority,   LtBlob *pBlob);
    void send(LtMsgOut* msg, boolean throttle = true);
	void sendMessage(LtApduOut* pApdu, boolean wait = true, boolean throttle = true);
    void cancel(LtMsgOut* msg);
    LtRespOut* respAlloc(LtMsgIn* msg);
    void send(LtRespOut* resp);
    void cancel(LtRespOut* resp);
    void sendResponse(LtMsgIn* request, byte* response, int len);
    LtErrorType clearStatus();
    LtErrorType getReadOnlyData(byte* readOnlyData);
	LtErrorType getReadOnlyData(LtReadOnlyData* pReadOnlyData);
    void reset();
    void setEepromLock(boolean locked);
    void setErrorLog(int errorNum);
    LtErrorType retrieveStatus(LtStatus& status);
    void setServiceLed(boolean ledOn);
	void winkServiceLed();
    void sleep(boolean commIgnore);
    void flush(boolean commIgnore);
    void flushCancel();
    void goOffline();
    void goUnconfigured();
	void goConfigured();
    void shutDown();
	void initiateReset();
	void setPersistencePath(const char* szPath);
    const char *getPersistencePath() { return m_szPersistencePath; }
	void resetPersistence();
	LtApduOut* nvResponse(LtApduIn* pApdu, byte* pData, int nLen);
	LtErrorType changeState(int newState, boolean bClearKey = false);
#if PRODUCT_IS(ILON)
	void triggerPropertyChangeHostEvent();
#endif

	void getLayerRange(int &min, int &max);
    void getNmVersion(int &nmVer, int &capabilities);
        // This method disables support for OMA keys, OMA authentication, flex domain authentication and
        // expanded OMA commands.  It does not inhibit ECS OMA commands.  The EXTCAP_OMA_COMMANDS flag is set, but
        // the EXTCAP_OMA_KEYS and LT_EXP_CAP_OMA flags are not set.  This is like an ECS device running on
        // a legacy layer 5 mip.  It must be called prior to calling registerApplication.
    LtErrorType disableOma();
    boolean omaSupported();

        // Used to reduce reported capabilities for node simulation.
    LtErrorType setNmVersionOverride(int nmVersion, int nmCapabilities);


	void flushCheck();
	boolean outgoingActivity();
	boolean incomingActivity();

	virtual boolean isNodeStack() = 0;
	virtual boolean isLayer2() { return false; }
	virtual boolean isMip() { return false; }
    virtual LtErrorType handleNetworkManagement(LtApduIn* apdu);
	LtDomainConfiguration* getDomainConfiguration(LtDomain* pDomain);
	LtErrorType getDomainConfiguration(int nIndex, LtDomainConfiguration* pDc);
	LtDomainConfiguration* allocDomainConfiguration(int index);
    LtErrorType updateDomainConfiguration(int nIndex, LtDomainConfiguration* pDomain, boolean bStore = false, boolean bRestore = false);
	LtErrorType	getSubnetNode(int domainIndex, int subnetNodeIndex,
				  			  LtSubnetNode &subnetNode);
	LtErrorType	updateSubnetNode(int domainIndex, int subnetNodeIndex,
								 const LtSubnetNode &subnetNode);
    boolean isMyAddress(int domainIndex, LtSubnetNode &subnetNode);

	LtErrorType getConfigurationData(byte* pData, int offset, int length);
	LtErrorType updateConfigurationData(byte* pData, int offset, int length);
	boolean hasMatchingSrcDomainConfiguration(LtDomainConfiguration& pSrcDc);

    void loadAdditionalConfig(LtNetworkImageVersion ver, byte *pImage, int &offset);
    void storeAdditionalConfig(byte *pImage, int &offset);
    int getAdditionallConfigStoreSize(void);

        // Set the lsMappingConfig and return the length.
    int lsAddrMappingConfigFromLonTalk(const byte *pSerializedData);
        // Get the lsMappingConfig and return the length. If pSerializedData is NULL, just get the length
    int lsAddrMappingConfigToLonTalk(byte *pSerializedData);
#if FEATURE_INCLUDED(IZOT)
    // Query this devices IP address.  Note that the device might have more than one
    // IP address, so the destination addressing is taken into account
    int queryIpAddr(LtApduIn* pApduIn, byte *arbitraryIpAddr);
#endif
	LtErrorType getOverrideDefaults(int addressIndex, LtMsgOverride* pMsgOverride);
	LtErrorType getBoundNvConfiguration(LtNetworkVariable* pNv, int arrayIndex, LtNetworkVariableConfiguration* &pNvc);

	virtual void resetRoutes() {}
	virtual void routerModeChange() {}
	virtual void setLreStateInfo() {}

    LtErrorType getAddressConfiguration(int nIndex, LtAddressConfiguration* pAc);
    LtErrorType updateAddressConfiguration(int nIndex, LtAddressConfiguration* pAddress, boolean bRestore = false);

	LtLreClient* getMainClient()				{ return m_pMainClient; }
	void setMainClient(LtLreClient* pClient)	{ m_pMainClient = pClient; }

	LtErrorType getRoute(int nDomainIndex, boolean bEeprom, LtRoutingMap* pMap);
	LtErrorType setRoute(int nDomainIndex, boolean bEeprom, LtRoutingMap* pMap);

	void localReset();
	boolean loadRoutes(byte* pData);
	void storeRoutes(byte* pData);
	int getRoutesStoreSize();

	// LtLreClient methods
	boolean getAddress(int& index, LtDomain *d, LtSubnetNode *s, LtGroups *g);
	boolean getAddress(int& index, LtUniqueId *u);
	boolean getNeedAllBroadcasts();

	void resetApduQueue();
	DynamicNvs* getDynamicNvs() { return m_pDynamicNvs; }
    void lockNvs();  // Lock NV definitions
    void unlockNvs(); // Lock NV definitions

	NdNetworkVariable* nvAdded(NdNetworkVariable* pNv);
	void nvChanged(NdNetworkVariable* pNv, NvChangeType type);
	void nvDeleted(NdNetworkVariable* pNv);
    boolean getCurrentNvLengthFromApp(LtNetworkVariable* pNv, int &length);

	void notifyPersistenceLost(LtPersistenceLossReason reason);

	LtLayer4Intf* getLayer4() { return m_pLayer4; }

	// LtConfigurationEntity methods
	LtErrorType enumerate(int index, boolean authenticated, LtApdu &response);
	LtErrorType initialize(int startIndex, int endIndex, byte* pData, int len, int domainIndex);
	LtErrorType checkLimits(int cmd, byte* pData, int len);

#if FEATURE_INCLUDED(MONITOR_SETS)
    // Monitor Set methods
	boolean monSetExists(int index);
    LtMonitorSet *getNextMonitorSet(LtMsIndex afterMsIndex);
    LtMonitorSet *getMonitorSet(LtMsIndex afterMsIndex);
#endif

    LtErrorType sendToXdriver(byte xDriverCommand, void *pData, int len);

    void prepareForBackup(void);
    void backupComplete(void);
    boolean readyForBackup(void);

	LtErrorType waitForPendingInterfaceUpdates(void);
#if PERSISTENCE_TYPE_IS(FTXL)
    void setAppSignature(unsigned appSignature);
    unsigned getAppSignature();
    void setPeristenceGaurdBand(int flushGuardTimeout);
#endif

#if FEATURE_INCLUDED(LTEP)
	LtErrorType processLtep(LtApduIn* pApdu);
	LtErrorType processLtepCompletion(LtApduOut* pApduOut);
#else
	LtErrorType processLtep(LtApduIn* pApdu) {return LT_NO_ERROR;}
	LtErrorType processLtepCompletion(LtApduOut* pApduOut) {return LT_NO_ERROR;}
#endif
	
	void setExclusionUid(const byte* pUid);
    void setServicePinState(LtServicePinState state); 
    LtServicePinState getServicePinState() {return m_nServicePinState;}
};

class LtAppNodeStack : public LtDeviceStack
{
public:
	LtAppNodeStack(LtLogicalChannel* pChannel, int nRxTx, int nTxTx) : LtDeviceStack(LT_NODE, pChannel, pChannel->getLre(), nRxTx, nTxTx) {}
	LtAppNodeStack(LtLogicalChannel* pChannel, int nRxTx, int nTxTx, 
                   int maxL4OutputPackets, int maxL4PriorityOutputPackets) : 
        LtDeviceStack(LT_NODE, pChannel, pChannel->getLre(), nRxTx, nTxTx, maxL4OutputPackets, maxL4PriorityOutputPackets) {}
	~LtAppNodeStack() { stop(); }
	boolean isNodeStack() { return true; }
};

class LtRouterStack : public LtDeviceStack
{
public:
	LtRouterStack(LtLogicalChannel* pChannel, int nRxTx, int nTxTx) : LtDeviceStack(LT_ROUTER_SIDE, pChannel, pChannel->getLre(), nRxTx, nTxTx) {}
	~LtRouterStack() { stop(); }
	boolean isNodeStack() { return false; }

	void resetRoutes();
	void routerModeChange();
	void setLreStateInfo();

	boolean getExternalRoute(int& nIndex, LtRoutingMap* pRoute, int* pRoutingSubnet);
	void eventNotification(LtEventServer* pLre);
};

class LtLayer2Stack : public LtAppNodeStack
{
public:
	LtLayer2Stack(LtLogicalChannel* pChannel) : LtAppNodeStack(pChannel, 1, 1) {}
	boolean isLayer2() { return true; }
};

#endif
