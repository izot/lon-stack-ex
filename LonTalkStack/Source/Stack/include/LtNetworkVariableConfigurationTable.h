#ifndef LTNETWORKVARIABLECONFIGURATIONTABLE_H
#define LTNETWORKVARIABLECONFIGURATIONTABLE_H

//
// LtNetworkVariableConfigurationTable.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtNetworkVariableConfigurationTable.h#1 $
//

#define NV_TABLE_NVS_ALIASES	0
#define NV_TABLE_NVS			1
#define NV_TABLE_ALIASES		2	 
#define NV_TABLE_PRIVATES		3
#define NV_TABLE_DEFAULT		NV_TABLE_NVS_ALIASES

#define NV_EXTRA_STORE_SIZE (1 + sizeof(int))
typedef struct 
{
	int count;
} NvConfigTableStoreHeader;

class LtDeviceStack;

class LtNetworkVariableConfigurationTable : public LtConfigurationEntity
{

private:
	NodeDef* m_pNodeDef;
    LtDeviceStack* m_pStack;
    LtNetworkVariableConfiguration** m_nvs;
    int numNvs;
    int numAliases;
    int numPrivate;
    int firstPrivate;
    boolean configured;
	int*			m_selectorMap;
	boolean			m_bRehash;
	int				m_nSelectorMapSize;
	int				m_nCount;
    int             m_lockedCount;

	int mapIndex(int index, int nType);
    LtErrorType loadEntry(int index, byte* data, int& offset, int nVersion);
	LtNetworkVariableConfiguration* newEntry(int index);
	LtNetworkVariableConfiguration* add(int index);
	void copyNvConfig(int index,
					  LtNetworkVariableConfiguration* pSource, 
					  LtNetworkVariableConfiguration* pDest);	
		// Same as get(int index, LtNetworkVariableConfiguration* pNvc, int nType), but
		// does not return an error if nvc is not allocated - returns NULL instead
	LtErrorType getPointer(int index, LtNetworkVariableConfiguration **ppNvc, int nType=NV_TABLE_DEFAULT);

protected:

public:
    LtNetworkVariableConfigurationTable();
    virtual ~LtNetworkVariableConfigurationTable();
	void setNodeDef(NodeDef* pNodeDef) {m_pNodeDef = pNodeDef;}
	NodeDef* getNodeDef() { return m_pNodeDef; }
    void setStack(LtDeviceStack* pStack) {m_pStack = pStack;}
    LtDeviceStack* getStack(void) { return m_pStack; }
    LtErrorType getNvDirSafe(int nvIndex, boolean &bOutput);
	void setCounts(int nvCount, int privateCount, int aliasCount);
	void getCounts(int& nvCount, int& privateNvCount, int& aliasCount);
    LtErrorType get(int index, LtNetworkVariableConfiguration* pNvc, int nType=NV_TABLE_DEFAULT);
    LtErrorType get(int index, LtNetworkVariableConfiguration **ppNvc, int nType=NV_TABLE_DEFAULT);
    LtErrorType getNv(int index, LtNetworkVariableConfiguration* pNvc);
    LtErrorType getNv(int index, LtNetworkVariableConfiguration** ppNvc);
    LtErrorType setNv(int index, LtNetworkVariableConfiguration &nvc, boolean bEvent=true);
	LtErrorType getPrivateNv(int &index, boolean bOutput, boolean bSourceSelection, int nvIndex, int nvSelector, LtOutgoingAddress* pAddress);
    LtErrorType set(int index, LtNetworkVariableConfiguration& nvc, int nType=NV_TABLE_DEFAULT, boolean bEvent=true);
    LtErrorType get(int& index, int nSelector, boolean bOutput, LtNetworkVariableConfiguration &nvc); 
    void clear(int index, int count);
	void clearNv(int index, int count);
    void incrementIncarnation(int index);
    LtNetworkVariableConfiguration* getNext(int nvIndex, LtVectorPos &pos);
    void store(byte* data, int &offset);
    LtErrorType load(byte* data, int offset, int nVersion);
    static int getStorSize(int totalNvCount, int privateNvs);
    int getMaxStoreSize();
	int getStoreSize();
	int getTableType(int index);
	boolean isBound(int index, int flags=ISBOUND_ANY);

    void lock();
    void unlock();
    boolean isLocked();

	int getPrimaryIndex(int index);

	int getSelectorMapIndex(int selector, boolean bOutput)
	{
		return ((selector&0x3fff) | (bOutput ? 0x4000 : 0)) * 17 % m_nSelectorMapSize;
	}
	void mapSelectorToIndex(int i, int selector, boolean bOutput);
	void updateSelectorMap();

	boolean mappingsAvailable() { return m_nSelectorMapSize != 0; }
	void nvChange();

	LtErrorType initialize(int fromIndex, int toIndex, int nType);
	LtErrorType enumerate(int index, LtApdu &response, int nType);
	LtErrorType update(int index, byte* pData, int len, int nType);

	virtual LtErrorType initialize(int fromIndex, int toIndex, byte* pData, int len, int domainIndex);
	virtual LtErrorType enumerate(int index, boolean authenticated, LtApdu &response);
	virtual LtErrorType update(int index, byte* pData, int len);
	virtual LtErrorType checkLimits(int cmd, byte* pData, int len);
};

class LtAliasTable : public LtConfigurationEntity
{
private:
	LtNetworkVariableConfigurationTable& m_nvt;

public:
	LtAliasTable(LtNetworkVariableConfigurationTable& nvt) : m_nvt(nvt)
	{
	}

	virtual LtErrorType initialize(int fromIndex, int toIndex, byte* pData, int len, int domainIndex);
	virtual LtErrorType enumerate(int index, boolean authenticated, LtApdu &response);
	virtual LtErrorType update(int index, byte* pData, int len);
	virtual LtErrorType checkLimits(int cmd, byte* pData, int len);
};

#endif
