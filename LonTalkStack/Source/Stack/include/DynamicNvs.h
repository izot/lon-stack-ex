//
// DynamicNvs.h
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

#ifndef _DYNAMICNVS_H
#define _DYNAMICNVS_H

// NV Info definitions, for use with LT_UPDATE_NV_INFO and LT_QUER_NV_INFO commands
#define NV_INFO_DESC         0  // Deprecated - includes 1 byte SNVT ID
#define NV_INFO_RATE_EST     1
#define NV_INFO_NAME         2
#define NV_INFO_SD_TEXT      3
#define NV_INFO_SNVT_INDEX   4  // Deprecated - uses 1 byte SNVT ID.
#define NV_INFO_DESC_2       5  // Includes 2 byte SNVT ID
#define NV_INFO_SNVT_INDEX_2 6

class DynamicNvs
{
private:
	NodeDef *m_pNodeDef;
	LtNetworkVariableConfigurationTable *m_pNvTable;

public:
	DynamicNvs(NodeDef* pNodeDef, LtNetworkVariableConfigurationTable* pNvTable) : m_pNodeDef(pNodeDef), m_pNvTable(pNvTable)
	{
	}

	void lock() { m_pNodeDef->lock(); }
	void unlock() { m_pNodeDef->unlock(); }

	int defaultsToFlags(int defaults);
	int flagsToDefaults(int flags);
	int attributesToFlags(int attributes);
	int flagsToAttributes(int flags);
	int flagsToExten(int flags);

	NdErrorType validateDynamicNvIndex(int nvIndex, int &nvCount);
	NdErrorType remove(int nvIndex, int nvCount);

	NdErrorType define(unsigned char* pMsgData, int len);
	NdErrorType remove(unsigned char* pMsg, int len);
	NdErrorType query(unsigned char* pMsg, int len, unsigned char* pResp, int &respLen);
	NdErrorType change(unsigned char* pMsg, int len);

	void persistenceUpdate() { m_pNodeDef->persistenceUpdate(); }
};

#endif
