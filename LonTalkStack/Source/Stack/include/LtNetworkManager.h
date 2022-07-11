#ifndef _LTNETWORKMANAGER_H
#define _LTNETWORKMANAGER_H
//
// LtNetworkManager.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtNetworkManager.h#1 $
//

#include <LtNetworkManagerLit.h>

class LtDmAddress : public LtObject
{

private:
    int addr;
    int size;

protected:

public:
    LtDmAddress(int addr, int size);
    boolean inRange(int addr, int size);
};

#define LT_RTR_TABLE_SEGMENT_SIZE 8

class LtRtrTableAttributes 
{
public:
	int m_nDomainIndex;
	boolean m_bGroup;
	boolean m_bEeprom;
	int m_nSegment;		// Segment # 0..3 of size LT_RTR_TABLE_SEGMENT_SIZE
};

#define CONFIG_ENTITIES_SIZE NM_NUM_RESOURCES

class LtNetworkManager : public LtTaskOwner
{
	friend class LtDeviceStack;

private:
	int							m_taskId;
	int							m_freq;
	int							m_clockFactor;

    MSG_Q_ID					m_queApdus;
	LtTypedVector<LtDmAddress>  m_vecDmAddrs;
    LtDeviceStack*				m_pStack;
	LtConfigurationEntity*		m_configEntities[CONFIG_ENTITIES_SIZE];

    boolean respondToQuery;
    boolean processing;

    LtErrorType validate(LtApdu& apdu, int min, int max = -1);
    LtErrorType validateRouterCommand(LtApdu& apdu, int min, int max = -1);
    int determineNvIndex(LtApdu& apdu);
    int determineNvOffset(LtApdu& apdu);
    int convertAddress(byte data[], int offset);
    LtErrorType readMemory(byte data[], int& len, byte** ppResult);
    LtErrorType processQueryRequest(boolean qualifies, int type, LtApdu& response);
    LtErrorType processQueryId(LtApdu& apdu, LtApdu& response);
    LtErrorType processRespondToQuery(LtApdu& apdu);
    LtErrorType processUpdateDomain(LtApdu& apdu);
    LtErrorType processLeaveDomain(LtApdu& apdu);
    LtErrorType processQueryDomain(LtApdu& apdu, LtApdu& response);
    LtErrorType processSecurity(LtApdu& apdu);
    LtErrorType processUpdateAddress(LtApdu& apdu);
    LtErrorType processUpdateGroupAddress(LtApdu& apdu);
    LtErrorType processQueryAddress(LtApdu& apdu, LtApdu& response);
    LtErrorType processUpdateNetworkVariable(LtApdu& apdu);
    LtErrorType processExpandedUpdateNetworkVariable(LtApdu& apdu, int subCode);
    LtErrorType processQueryNetworkVariable(LtApdu& apdu, LtApdu& response);
    LtErrorType processExpandedQueryNetworkVariable(LtApdu& apdu, LtApdu& response, int subCode); 
    LtErrorType processNodeMode(LtApdu& apdu);
    LtErrorType processChecksumRecalc(LtApdu& apdu, boolean& store);
    LtErrorType processReadMemory(LtApdu& apdu, LtApdu& response);
    LtErrorType processWriteMemory(LtApdu& apdu);
    LtErrorType processMemoryRefresh(LtApdu& apdu);
    LtErrorType processQueryStatus(LtApdu& apdu, LtApdu& response, boolean validate);
    LtErrorType processClear(LtApdu& apdu);
    LtErrorType processQueryXcvrStatus(LtApduIn& apdu, LtApdu& response);
    LtErrorType processBidirXcvrStatus(LtApduIn& apdu, LtApdu& response);
    LtErrorType processProxyAgent(LtApdu& apdu);
    LtErrorType processProxy(LtApduIn& apdu, LtApdu& response);
	LtErrorType processSetRouterMode(LtApdu& apdu);
	LtErrorType getTableAttributes(LtApdu& apdu, LtRtrTableAttributes& att, boolean bSet);
	LtErrorType setRoutingTable(LtApdu& apdu, boolean bSet, byte* pData);
	LtErrorType processClearRoutingTable(LtApdu& apdu);
	LtErrorType processSetRoutingTable(LtApdu& apdu);
	LtErrorType processQueryRoutingTable(LtApdu& apdu, LtApdu& response);
	LtErrorType processRouteMod(LtApdu& apdu, boolean bSet, boolean bGroup);
	LtErrorType processQueryRouterStatus(LtApdu& apdu, LtApdu& response);
	LtErrorType processRouterEscape(LtApdu& apdu);
	LtErrorType processExtendedCommand(LtApdu& apdu, LtApdu& response, boolean &bStore);
    LtErrorType processExpandedCommand(LtApdu& apdu, LtApdu& response, boolean &bStore);
	LtErrorType processComputePhase(LtApduIn& apdu, LtApdu& response);

	LtConfigurationEntity* getConfigurationEntity(int resource);
	NmErrCode toNmErr(LtErrorType err);

protected:
	LtErrorType getNeuronPhaseData(int& frequency, int& clockFactor);

public:
    LtNetworkManager();
	~LtNetworkManager();

	LtDeviceStack* getStack()					{ return m_pStack; }
	void setStack(LtDeviceStack* pStack);

    void deliver(LtApduIn* apdu);
    boolean isBusy();
    LtApdu* fetch();
    void run();
	void shutdown(void);
    LtErrorType process(LtApduIn& apdu);
    void setMem(int addr, int size);
    boolean appMemCheck(int addr, int size);

    LtErrorType fetchXcvrStatus(LtApduIn& request, LtApdu& response, int offset=0);

	void persistenceLost();

	static int VXLCDECL start( int nm, ... );
};



#endif
