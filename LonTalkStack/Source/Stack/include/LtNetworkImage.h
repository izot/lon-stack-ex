#ifndef _LTNETWORKIMAGE_H
#define _LTNETWORKIMAGE_H
//
// LtNetworkImage.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtNetworkImage.h#2 $
//

#define LT_OS_PROGRAMID		0
#define LT_OS_ECSNODE		8
#define LT_OS_COUNTS		10
#define LT_OS_STATE			30
#define LT_OS_CONFIGDATA	34
#define LT_OS_DOMAIN		94
#define LT_OS_ADDRESS		294
#define LT_OS_NV_ROUTE		1594

class LonTalkNode;
class LonTalkStack;

typedef struct 
{
	int m_nvCount;
	int m_privateNvCount;
	int m_aliasCount;
	int m_addressCount;
	int m_domainCount;
} TableCounts;

typedef enum
{
    NetImageVer_original = 1,         // 1 - Original
    NetImageVer_ECS,                  // 2 - ECS commands    
    NetImageVer_compact,              // 3 - Do not store unbound NVs or Address Tables
    NetImageVer_OMA,                  // 4 - 96 bit (OMA) authentication keys.
    NetImageVer_eliminateNvHasDefaultsFlag,  // 5 - Eliminate "hasDefaults" flag from nv config.
    NetImageVer_encrypted,			  // 6 - contents are encrypted
    NetImageVer_LsEnhancedMode,       // 7 - support for LS enhanced mode 
} LtNetworkImageVersion;
#define CURRENT_NETIMG_VER NetImageVer_LsEnhancedMode

class LtNetworkImage : public LtPersistenceClient
{

private:
    int				m_nState;
	LtDeviceStack*	m_pStack;
	boolean			m_bBlackout;
	boolean			m_bHasBeenEcsChanged;

	LtPersistence	m_persistence;

protected:

public:
    LtDomainConfigurationTable domainTable;
    LtAddressConfigurationTable addressTable;
    LtNetworkVariableConfigurationTable nvTable;
	LtAliasTable aliasTable;
    LtConfigData configData;
    LtNetworkImage(LtDeviceStack* pStack);
	virtual ~LtNetworkImage();

	LtDeviceStack* getStack()				{ return m_pStack; }
    void setStack(LtDeviceStack* pStack)	{ m_pStack = pStack; }

    void reset(LonTalkNode& node);
    void initImage();
	void sync();

	void getCounts(TableCounts& counts);
    void setNvAliasCount(int numPublicNvEntries, int numPrivateNvEntries, int numAliasEntries);
    LtErrorType setAuthKey(int index, byte* key);
    boolean store(boolean bRecompute = true);	// Return true if successful
    LtErrorType changeState(int newState, boolean clearKeys);
    boolean unconfigured();
	boolean unconfigured(int state);
	boolean configurationChange(int newState);
    int getState();
	boolean getBlackout();


	boolean getHasBeenEcsChanged() { return m_bHasBeenEcsChanged; }
	void setHasBeenEcsChanged(boolean bEcsChange) { m_bHasBeenEcsChanged = bEcsChange; }

	void nvChange();
	LtPersistence* getPersistence() { return &m_persistence; }

	virtual void serialize(byte* &pBuffer, int &len);
	virtual LtPersistenceLossReason deserialize(byte* pBuffer, int len, int nVersion);
	virtual void notifyPersistenceLost(LtPersistenceLossReason reason);

#if PERSISTENCE_TYPE_IS(FTXL)
    // Return the the maximum number of bytes that will be consumed for
    // serialized data.
    int  getMaxSerialLength();
#endif

	LtErrorType initialize(int state, int options = 0, int domainIndex = 0);

        // Initialized NV, alias and address tables, LT_EXP_INIT_CONFIG style
    LtErrorType expInitConfig(void);
};
#endif
