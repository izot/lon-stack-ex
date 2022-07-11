#ifndef LTADDRESSCONFIGURATIONTABLE_H
#define LTADDRESSCONFIGURATIONTABLE_H
//
// LtAddressConfigurationTable.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtAddressConfigurationTable.h#1 $
//



#define ADDR_EXTRA_STORE_SIZE sizeof(int)	// AddrTableIndex
// Address table storage header
typedef struct 
{
	int count;
} AddressTableStoreHeader;

class LtGroupAddresses : public LtTypedVector<LtAddressConfiguration>
{
};

class LtDomainGroups : public LtTypedVector<LtGroupAddresses>
{
};

class LtAddressConfigurationTable : public LtConfigurationEntity, public VxcLock
{

private:
    LtAddressConfiguration** m_addr;
	LtDomainGroups			 m_domainGroups;
	boolean					 m_bRebuildMap;

    int                      m_lastMatchingIndex;
	int                      m_count;
	LtDeviceStack*			 m_pStack;
    LtErrorType get(int index, LtAddressConfiguration** ppAc);
protected:

public:
    LtAddressConfigurationTable();
    virtual ~LtAddressConfigurationTable();
	int getCount() { return m_count; }
	void setCount(LtDeviceStack* pStack, int count);
	LtErrorType get(int index, LtAddressConfiguration* pAc);
    LtErrorType set(int index, LtAddressConfiguration& ac);
    LtErrorType update(LtDomain& oldDomain, LtDomain& newDomain);
    void fixup(LtOutgoingAddress& oa);
    LtErrorType update(LtAddressConfiguration& newValue);
    void store(byte* data, int &offset);
	LtDeviceStack* getStack() { return m_pStack; }
	int getStoreSize();
    static int getStoreSize(int numEntries);
    int getMaxStoreSize();

	void getGroups(int domainIndex, LtGroups& gp);
	void rebuildMap();
	LtAddressConfiguration* get(int domainIndex, int group);

	// LtConfigurationEntity methods
	virtual LtErrorType initialize(int fromIndex, int toIndex, byte* pData, int len, int domainIndex);
	virtual LtErrorType enumerate(int index, boolean authenticated, LtApdu &response);
	virtual LtErrorType update(int index, byte* pData, int len);
	virtual LtErrorType checkLimits(int cmd, byte* pData, int len);
};

#endif
