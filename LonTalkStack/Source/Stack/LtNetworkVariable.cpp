//
// LtNetworkVariable.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtNetworkVariable.cpp#1 $
//

#include "LtStack.h"

// Public NV constructor
LtNetworkVariable::LtNetworkVariable(
		byte* pData,			// Network variable address (NULL if data kept in object)
		int nLength,			// Network variable length (for array, for each element)
		LtNvType nvType,		// NV type
		int arrayLength,		// Array length, 0 for scalar.
		boolean isOutput,		// true if output
		int flags,				// SD flags
		char* szName,			// NV name
		char* szSelfDoc,		// Self doc string
		int rateEstimate,		// Rate estimate
		int maxRateEstimate		// Max rate estimate
) : NdNetworkVariable(nvType, nLength, flags | (isOutput ? NV_SD_OUTPUT : 0),
		arrayLength, szSelfDoc, szName, rateEstimate, maxRateEstimate)
{
	m_bFreeData = false;
	if (pData == null)
	{
		// No data was provided.  Allocate some
		pData = new byte[nLength * getElementCount()];
		// initialize the NV value to zeros
		if (pData) 
			memset(pData, 0, nLength * getElementCount());
		m_bFreeData = true;
	}
	m_pData = pData;
}

// Private NV constructor
LtNetworkVariable::LtNetworkVariable(
		byte* pData,			// Network variable address (NULL if data kept in object)
		int nLength,			// Network variable length (for array, for each element)
		LtNvType nvType,		// NV type
		boolean isOutput,		// true if output
		int flags,				// NV flags
		char* szName			// NV name
) : NdNetworkVariable(nvType, nLength, flags | NV_SD_PRIVATE | (isOutput ? NV_SD_OUTPUT : 0),
		0, "", szName)
{
	m_bFreeData = false;
	if (pData == null)
	{
		// No data was provided.  Allocate some
		pData = new byte[nLength];
		// initialize the NV value to zeros
		if (pData) 
			memset(pData, 0, nLength);
		m_bFreeData = true;
	}
	m_pData = pData;
}

// Copy constructors
LtNetworkVariable::LtNetworkVariable(NdNetworkVariable* pNv) : NdNetworkVariable(pNv)
{
	m_pData = new byte[getLength() * getElementCount()];
	// initialize the NV value to zeros
	if (m_pData) 
		memset(m_pData, 0, getLength() * getElementCount());
	m_bFreeData = true;
}

LtNetworkVariable::LtNetworkVariable(LtNetworkVariable* pNv) : NdNetworkVariable(pNv)
{
	// New NV gets old NVs pointers
	m_pData = new byte[getLength() * getElementCount()];
	// initialize the NV value to zeros
	if (m_pData) 
		memset(m_pData, 0, getLength() * getElementCount());
	m_bFreeData = true;
	m_pStack = pNv->m_pStack;
}

LtNetworkVariable::~LtNetworkVariable()
{
	if (m_bFreeData)
	{
		delete m_pData;
	}
}

int LtNetworkVariable::getNvIndex()
{
	return NdNetworkVariable::getNvIndex();
}

void LtNetworkVariable::getNvData(byte* pData, int &nLength, int arrayIndex)
{
	int len = nLength;
	if (nLength > getLength()) len = getLength();

	memcpy(pData, &m_pData[arrayIndex*getLength()], len);
	nLength = len;
}

void LtNetworkVariable::setNvData(byte* pData, int nLength, int arrayIndex)
{
	int len = nLength;
	if (nLength > getLength()) len = getLength();

	setLengthForUpdate(len);

	memcpy(&m_pData[arrayIndex*getLength()], pData, len);
}

LtNvClass LtNetworkVariable::getClassification()
{
	int flags = getFlags();
	LtNvClass nvClass = LT_NV_PUBLIC;

	if (flags & NV_SD_PRIVATE)
	{
		nvClass = LT_NV_PRIVATE;
	}
	else if (flags & NV_SD_DYNAMIC)
	{
		nvClass = LT_NV_DYNAMIC;
	}
	return nvClass;
}

int LtNetworkVariable::getFlags()
{
	return NdNetworkVariable::getFlags() & NV_SD_PUBLIC_MASK;
}

int LtNetworkVariable::getLength()
{
	return NdNetworkVariable::getLength();
}

int LtNetworkVariable::getLengthFromDevice()
{
	return NdNetworkVariable::getLengthFromDevice();
}

LtNvType LtNetworkVariable::getType()
{
	return NdNetworkVariable::getType();
}

int LtNetworkVariable::getArrayLength()
{
	return NdNetworkVariable::getArrayLength();
}

boolean LtNetworkVariable::getIsOutput()
{
	return (getIntFlags() & NV_SD_OUTPUT) ? true : false;
}

void LtNetworkVariable::getSdString(char* szSdString, int nLength)
{
	getDescription(szSdString, nLength);
}

void LtNetworkVariable::getName(char* szName, int nLength)
{
	NdNetworkVariable::getName(szName, nLength);
}

int LtNetworkVariable::getRateEstimate()
{
	return NdNetworkVariable::getRateEstimate();
}

int LtNetworkVariable::getMaxRateEstimate()
{
	return NdNetworkVariable::getMaxRateEstimate();
}

boolean LtNetworkVariable::propagate(int arrayIndex, LtMsgOverride* pOverride)
{
	return getStack()->propagate(this, arrayIndex, pOverride);
}

boolean LtNetworkVariable::poll(int arrayIndex, LtMsgOverride* pOverride)
{
	return getStack()->poll(this, arrayIndex, pOverride);
}

boolean LtNetworkVariable::isBound(int arrayIndex, int flags)
{
	return getStack()->isBound(this, arrayIndex, flags);
}

void LtNetworkVariable::getOverrideDefaults(LtMsgOverride* pMsgOverride, int arrayIndex)
{
	// This routine sets the "default" values for a message override.  This is
	// not that great a concept but it will have to do.  Message override is
	// really an output structure.  However, in this case it is used to read
	// the overrides.  Any override not set is used to convey the "default"
	// value (or non-override value).

	// One nasty aspect of this feature is aliases.  If there are aliases 
	// configured, then there is really no one default.  However, for practical
	// reasons we just take the primary's values (or the first alias if no
	// primary).
	
	// So, to fill out the message override we need to find the relevant NV
	// configuration, then use it to find the relevant address table entry.
	// Note that on a layer 5 mip the address look-up can be somewhat expensive.
    LtNetworkVariableConfiguration* pNvc;

	if (getStack()->getBoundNvConfiguration(this, arrayIndex, pNvc) == LT_NO_ERROR)
	{
		LtMsgOverrideOptions options(LT_OVRD_SERVICE | LT_OVRD_PRIORITY);
		LtMsgOverride msgOverride(options, pNvc->getServiceType(), pNvc->getPriority(), 0, 0, 0);
		
		pMsgOverride->setDefaults(&msgOverride);

		getStack()->getOverrideDefaults(pNvc->getAddressTableIndex(), pMsgOverride);
	}
}

int LtNetworkVariable::getCurLength()
{
    int length;
    if (getChangeableLength() && 
        getStack()->getCurrentNvLengthFromApp(this, length))
    {
        setCurLength(length);
    }
    return NdNetworkVariable::getCurLength();
}
