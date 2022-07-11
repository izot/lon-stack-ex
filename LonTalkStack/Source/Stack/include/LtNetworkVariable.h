//
// LtNetworkVariable.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtNetworkVariable.h#1 $
//

#ifndef _LTNETWORKVARIABLE_H
#define _LTNETWORKVARIABLE_H

#include <NdNetworkVariable.h>

typedef NdNvType LtNvType;

// Filter flags for "isBound"
#define ISBOUND_POLLED		0x1
#define ISBOUND_NON_POLLED	0x2
#define ISBOUND_ANY			(ISBOUND_POLLED | ISBOUND_NON_POLLED)

typedef enum
{
	LT_NV_PUBLIC = 0,
	LT_NV_DYNAMIC = 1,
	LT_NV_PRIVATE = 2
} LtNvClass;

class LtStack;

class LTA_EXTERNAL_CLASS LtNetworkVariable : public NdNetworkVariable
{
	friend		class LtDeviceStack;

private:
	byte*		m_pData;
	boolean		m_bFreeData;
	LtStack*	m_pStack;

protected:
	LtStack* getStack() { return m_pStack; }
	void setStack(LtStack* pStack) { m_pStack = pStack; }

public:
	// Public NV constructor
	LtNetworkVariable(
		byte* pData,			// Network variable address (NULL if data kept in object)
		int nLength,			// Network variable length (for array, for each element)
		LtNvType nvType,		// NV type
		int arrayLength,		// Array length, 0 for scalar.
		boolean isOutput,		// true if output
		int flags,				// SD flags (see NdNetworkVariable.h)
		char* szName,			// NV name
		char* szSelfDoc = null,	// Self doc string
		int rateEstimate = -1,	// Rate estimate
		int maxRateEstimate = -1// Max rate estimate
	);

	// Private NV constructor (arrays not supported)
	LtNetworkVariable(
		byte* pData,			// Network variable address (NULL if data kept in object)
		int nLength,			// Network variable length (for array, for each element)
		LtNvType nvType,		// NV type
		boolean isOutput,		// true if output
		int flags = 0,			// NV state flags (same as SD flags, see NdNetworkVariable.h)
		char* szName = null		// NV name
	);

	// Copy constructors
	LtNetworkVariable(NdNetworkVariable* pNv);
	LtNetworkVariable(LtNetworkVariable* pNv);

	virtual ~LtNetworkVariable();

	int getNvIndex();

	void getNvData(byte* pData, int &nLength, int arrayIndex = 0);

	void setNvData(byte* pData, int nLength, int arrayIndex = 0);

	void getOverrideDefaults(LtMsgOverride* pMsgOverride, int arrayIndex = 0);

	LtNvClass getClassification();

	int getLength();			// NV length (for array, length of each element)
	int getLengthFromDevice();	// NV length based on last update or response received.
	LtNvType getType();			// NV type (0 for typeless)
	int getArrayLength();		// NV array length (0 for non-array)
	boolean getIsOutput();		// true if output
	int getFlags();				// returns flags
	void getSdString(char* szSdString, int nLength);	// returns self doc string
	void getName(char* szName, int nLength);			// returns NV name
	int getRateEstimate();		// returns rate estimate
	int getMaxRateEstimate();	// returns maximum rate estimate

	// Operations
	boolean propagate(int arrayIndex=0, LtMsgOverride* pOverride=null);
	boolean poll(int arrayIndex=0, LtMsgOverride* pOverride=null);
	boolean isBound(int arrayIndex=0, int flags=ISBOUND_ANY);

    byte* getNvDataPtr(int arrayIndex = 0) { return &m_pData[arrayIndex*getLength()]; }

    int getCurLength();

};

#endif
