//
// LtMonitorControlTable.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtMonitorControlTable.h#1 $
//
#ifndef _LTMONITORCONTROLTABLE_H
#define _LTMONITORCONTROLTABLE_H

#include "LtMonitorSetDefs.h"
#include "LtMonitorControl.h"
#include "LtConfigurationEntity.h"

class LtMonitorSet;
class LtMonitorPoint;

class LtMonitorControlClient 
{
public:
	virtual ~LtMonitorControlClient(){};

    // Callbacks
    virtual LtMonitorSet *monSetAdded(LtMonitorSet *pMonitorSet) = 0;
    virtual void monSetChanged(LtMonitorSet *pMonitorSet, McChangeType type) = 0;
    virtual void monSetDeleted(LtMonitorSet *pMonitorSet) = 0;
	virtual boolean monSetExists(int index) { return false; }

    virtual LtMonitorPoint *monPointAdded(LtMonitorPoint *pMonitorPoint) = 0;
    virtual void monPointChanged(LtMonitorPoint *pMonitorPoint, McChangeType type) = 0;
    virtual void monPointDeleted(LtMonitorPoint *pMonitorPoint) = 0;

	virtual void notifyPersistenceLost(LtPersistenceLossReason reason) = 0;
};

typedef NdCollection<NdMonitorControl> NdMonitorControls;

typedef enum
{
    MonitorControlTableVer_original = 2,         // 2 - Original.  Why 2?
} LtMonitorControlTableVersion;
#define CURRENT_MONITOR_CONTROL_TABLE_VER MonitorControlTableVer_original

// This class defines a collection of monitor sets and thier configuration.
class LtMonitorControlTable: public LtConfigurationEntity, public LtPersistenceClient, public VxcLock
{
	DEFINE_COLLECTION(MonitorControl);

public:
	LtMonitorControlTable(const char* szName);
    virtual ~LtMonitorControlTable();

    void setClient(LtMonitorControlClient *pMcClient) { m_pMcClient = pMcClient; }

	NdMonitorControl* get(LtMcIndex mcIndex, boolean bValidOnly = LT_MC_ALL);

    NdMonitorControl *getNext(LtMcIndex &mcIndex, boolean bValidOnly = LT_MC_ALL);
    
    NdMonitorControl *add(NdMonitorControl* pMonitorControl);

    void remove(NdMonitorControl* pMonitorControl, boolean bNotify);

	// Callback links
    LtMonitorControl *monitorControlAdded(LtMonitorControl *pMonitorControl);
    void monitorControlChanged(LtMonitorControl *pMonitorControl, McChangeType type);
    void monitorControlDeleted(LtMonitorControl *pMonitorControl);

	// ConfigurationEntity
	LtErrorType initialize(int startIndex, int endIndex, byte* pData, int len, int domainIndex);
	LtErrorType enumerate(int index, boolean authenticated, LtApdu &response);
	LtErrorType update(int index, byte* pData, int len);
	LtErrorType create(int index, byte* pData, int len);
	LtErrorType setDesc(int index, LtDescriptionOptions options, byte* pData, int offset, int length);
	LtErrorType getDesc(int index, int offset, int length, LtApdu& pApdu);
	LtErrorType resourceSpecificCommand(int cmd, int index, byte* pData, int len, boolean authenticated, LtApdu &response);
	boolean affectsNetworkImage() { return false; }

    // Persistence
	int getSerialLength();
	virtual void serialize(unsigned char* &pBuffer, int &len);
	virtual LtPersistenceLossReason deserialize(unsigned char* pBuffer, int len, int nVersion);

	void notifyApp(LtMonitorControl* pMonitorControl, McChangeType type);

	virtual LtMonitorControl* createInstance() = 0;

	void notifyPersistenceLost(LtPersistenceLossReason reason);

	LtPersistenceLossReason restore()  { return m_persistence.restore(); }
    void setPath(const char* szPath)	{ m_persistence.setPath(szPath); }
    void setIndex(int nIndex)	{ m_persistence.setIndex(nIndex); }

    LtPersistence* getPersistence() { return &m_persistence; }

private:
	LtErrorType validate(LtMonitorControl* pMc);

    LtMonitorControlClient *m_pMcClient;
	LtPersistence m_persistence;
};

#endif
