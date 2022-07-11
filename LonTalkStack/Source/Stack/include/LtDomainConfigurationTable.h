#ifndef LTDOMAINCONFIGURATIONTABLE_H
#define LTDOMAINCONFIGURATIONTABLE_H

//
// LtDomainConfigurationTable.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtDomainConfigurationTable.h#1 $
//

#define LT_DOMAIN_INIT_OPT_PRESERVE_AUTH 0x80

class LtDomainConfigurationTable : public LtConfigurationEntity
{
private:
    LtDomainConfiguration** m_dmn;
	int m_nCount;
	LtDeviceStack*			m_pStack;

    // Utility  methods to support LtConfigurationEntity
    LtErrorType enumerate(int index, LtApdu &response, LtLonTalkDomainStyle domainStyle);
    LtErrorType update(int index, int indexOfKey, byte* pData, int len, LtLonTalkDomainStyle domainStyle);
    LtErrorType setAuthKey(int index, byte* pData, int keyLen);
    LtErrorType enumerateAuth(int index, LtApdu &response);

protected:

public:
    LtDomainConfigurationTable();
    virtual ~LtDomainConfigurationTable();
	void setCount(LtDeviceStack* pStack, int count);
    int getCount();
    LtErrorType get(int index, LtDomainConfiguration** ppDc);
    LtDomainConfiguration* get(LtDomainConfiguration* match);
    LtDomainConfiguration* get(LtDomain* match);
    LtErrorType set(int index, LtDomainConfiguration* pDc);
    void store(byte* data, int &offset);
	LtDeviceStack* getStack() { return m_pStack; }
	int getStoreSize();
    int getMaxStoreSize();
	void goUnconfiguredConditional();
    LtErrorType getFlexAuthDomain(LtDomainConfiguration** ppDc);

	// LtConfigurationEntity methods
	virtual LtErrorType initialize(int fromIndex, int toIndex, byte* pData, int len, int domainIndex);
	virtual LtErrorType enumerate(int index, boolean authenticated, LtApdu &response);
	virtual LtErrorType update(int index, byte* pData, int len);
	virtual LtErrorType checkLimits(int cmd, byte* pData, int len);
	virtual LtErrorType resourceSpecificCommand(int cmd, int index, byte* pData, int len, boolean authenticated, LtApdu &response);
};

#endif

