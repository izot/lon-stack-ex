//
// NdNetworkVariable.cpp
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
// This file contains class implementations for network variables within 
// LonWorks node definitions.  See nodedef.h for more information.
//

//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/NdNetworkVariable.cpp#1 $
//

#include <LtRouter.h>
#include <nodedef.h>

NdNetworkVariable::NdNetworkVariable(NdNetworkVariable* pNv)
{
	// This is really a transfer constructor, not copy.
	*this = *pNv;
	if (pNv->isMaster())
	{
		setMaster(this);
	}
	pNv->strip();
	setParent(NULL);
}

NdNetworkVariable::NdNetworkVariable(NdNvType type, int nLength, int nFlags, int nArrayLength, 
	const char* pSdString, const char* pNvName, int rateEstimate, int maxRateEstimate)
{
	// Changeable length implies changeable type (could be decoupled by examining all uses of "NV_SD_CHANGEABLE" 
	// and deciding which don't apply to changeable length).
	if (nFlags & NV_SD_CHANGEABLE_LENGTH)
	{
		nFlags |= NV_SD_CHANGEABLE;
	}

	m_nNvIndex = -1;
	m_nvType = type;
	m_nLength = nLength;
	m_nArrayLength = nArrayLength;
	m_nFlags = nFlags;
	m_pMaster = this;
	m_szDescription = NULL;

	m_nLengthFromDevice = 0;
	m_nLengthForUpdate = nLength;

	if (pSdString != NULL)
	{
		setDescription(pSdString, 0, strlen(pSdString) + 1);
	}
	else
	{
		setDescription("", 0, 1);
	}
	m_nRateEstimate = rateEstimate;
	m_nMaxRateEstimate = maxRateEstimate;
	if (pNvName == NULL)
	{
		setName("");
	}
	else
	{
		setName(pNvName);
	}
	if (m_nMaxRateEstimate != NO_RATE_ESTIMATE)
	{
		m_nFlags |= NV_SD_MAX_RATE;
	}
	if (m_nRateEstimate != NO_RATE_ESTIMATE)
	{
		m_nFlags |= NV_SD_RATE;
	}
	setParent(NULL);
}

NdNetworkVariable::~NdNetworkVariable()
{
	delete m_szDescription;
	m_szDescription = NULL;

	if (m_nFlags & NV_SD_MASTER_DELETE)
	{
		delete m_pMaster;
	}
}

boolean NdNetworkVariable::hasExtensionData()
{
	return m_nArrayLength != 0 || 
		   (getFlags() & (NV_SD_MAX_RATE|NV_SD_RATE|NV_SD_NAME|NV_SD_DESCRIPTION)) != 0;
}

// This method is called when a new object is created to "take over"
// for this object.  It assumes the new object inherits pointers
// to other objects, strings, etc.
void NdNetworkVariable::strip()
{
	m_szDescription = NULL;
	m_pMaster = NULL;
}

void NdNetworkVariable::getArrayName(char* szName)
{
	getMaster()->NdCollectionElement::getName(szName);
}

void NdNetworkVariable::getName(char* szName, int nLength)
{
	// For arrays, construct a name from the master
	lock();
	assert(nLength>=ND_NAME_LEN);
	getArrayName(szName);
	if (getMaster()->getArrayLength() && !isMaster() && strlen(szName))
	{
		// We are a forked NV array element.  Tweak name accordingly
		char szIndex[8];
		sprintf(szIndex, "[%d]", getNvIndex() - getNvArrayIndex());
		strcat(szName, szIndex);
	}
	szName[nLength-1] = 0;
	unlock();
}

void NdNetworkVariable::setRateEstimate(int re)
{
	NdNetworkVariable* pNv = getMaster();
	pNv->m_nRateEstimate = re;
	if (re == NO_RATE_ESTIMATE)
	{
		pNv->m_nFlags &= ~NV_SD_RATE;
	}
	else
	{
		pNv->m_nFlags |= NV_SD_RATE;
	}
}

void NdNetworkVariable::setMaxRateEstimate(int re)
{
	NdNetworkVariable* pNv = getMaster();
	pNv->m_nMaxRateEstimate = re;
	if (re == NO_RATE_ESTIMATE)
	{
		pNv->m_nFlags &= ~NV_SD_MAX_RATE;
	}
	else
	{
		pNv->m_nFlags |= NV_SD_MAX_RATE;
	}
}

int NdNetworkVariable::getFlags()
{
	return getMaster()->m_nFlags;
}

int NdNetworkVariable::getRateEstimate()
{
	return getMaster()->m_nRateEstimate;
}

int NdNetworkVariable::getMaxRateEstimate()
{
	return getMaster()->m_nMaxRateEstimate;
}

const char* NdNetworkVariable::getDescription()
{
	const char* pDesc = getMaster()->m_szDescription;
	if (pDesc == NULL) pDesc = "";
	return pDesc;
}

void NdNetworkVariable::getDescription(char* szDesc, int maxLength)
{
	lock();
	const char* pDesc = getDescription();
	int nLen = strlen(pDesc) + 1;
	if (nLen > maxLength) nLen = maxLength;
	strncpy(szDesc, pDesc, nLen);
	szDesc[maxLength-1] = 0;
	unlock();
}

void NdNetworkVariable::setName(const char* szName)
{
	NdNetworkVariable* pNv = getMaster();
	if (szName[0] != 0)
	{
		pNv->m_nFlags |= NV_SD_NAME;
		memcpy(pNv->NdCollectionElement::getName(), szName, ND_NAME_LEN);
	}
	else
	{
		pNv->m_nFlags &= ~NV_SD_NAME;
	}

}

void NdNetworkVariable::setDescription(const char* pDesc, int offset, int length)
{
	NdNetworkVariable* pNv = getMaster();
	boolean bDelete = false;
	char* szOldDesc = pNv->m_szDescription;
	int oldLen = 0;
	if (szOldDesc != NULL)
	{
		oldLen = strlen(szOldDesc);
		if (offset + length > oldLen)
		{
			pNv->m_szDescription = NULL;
			bDelete = true;
		}
	}
	else
	{
		pNv->m_szDescription = NULL;
	}

	if (pNv->m_szDescription == NULL)
	{
		pNv->m_szDescription = new char[offset + length + 1];
		memset(pNv->m_szDescription, 0, offset + length + 1);
	}
	if (szOldDesc != NULL)
	{
		memcpy(pNv->m_szDescription, szOldDesc, oldLen + 1);
	}
	memcpy(pNv->m_szDescription + offset, pDesc, length);
	if (bDelete)
	{
		delete szOldDesc;
	}
	if (pNv->m_szDescription[0] != 0)
	{
		pNv->m_nFlags |= NV_SD_DESCRIPTION;
	}
	else
	{
		pNv->m_nFlags &= ~NV_SD_DESCRIPTION;
	}
}


int NdNetworkVariable::getArrayLength()
{
	return m_nArrayLength;
}

int NdNetworkVariable::getAppInstanceCount()
{
	// For a forked NV, this is the element count, otherwise
	// it is one.
	if (getForked())
	{
		return getMaster()->getElementCount();
	}
	return 1;
}

int NdNetworkVariable::isArrayElement()
{
	return getMaster()->getArrayLength() != 0;
}

int NdNetworkVariable::getElementCount()
{
	// For an array, it's the array length; otherwise, it's 1.
	if (getArrayLength())
	{
		return getArrayLength();
	}
	return 1;
}

int NdNetworkVariable::getSerialLength()
{
	int len = 0;
	if (isFirst())
	{
		int stringLength = strlen(getDescription());
		// Add one for trailing zero.
		if (stringLength) stringLength++;
		len = ND_BASE_SERIAL_LENGTH + stringLength;
		// Changeable types are appended to end for subsequent NVs.
		if (getMaster()->getArrayLength() > 1 && getChangeable())
		{
			len += sizeof(NdNvType)*(getMaster()->getArrayLength() - 1);
		}
	} 
	else if (getChangeable())
	{
		len = 2;
	}
	return len;
}

int NdNetworkVariable::isFirst()
{
	return (!getForked() || getMaster()->getNvArrayIndex() == getNvIndex());
}

int NdNetworkVariable::isMaster()
{
	return getMaster() == this;
}

void NdNetworkVariable::serialize(byte* &pBuffer, int &len)
{
	int length = getSerialLength();
	len = length;
	// Warning - if you change the layout of the serialization, you 
	// must also change the deserialize and getSerialLength functions.
	if (length == 2)
	{
		// Just dump the type
		PTONS(pBuffer, m_nvType);
	}
	else if (length)
	{
		unsigned short tag = 0xEFBE;
		PTONS(pBuffer, tag);
		PTONS(pBuffer, tag);
		PTONS(pBuffer, getNvIndex());
		PTONS(pBuffer, length);

		char name[ND_NAME_LEN];
		getName(name);

		int temp = getFlags();
		PTONL(pBuffer, temp);
		PTONB(pBuffer, m_nLength);
		short temp2 = getMaster()->getArrayLength();
		PTONS(pBuffer, temp2);
		temp2 = getRateEstimate();
		PTONS(pBuffer, temp2);
		temp2 = getMaxRateEstimate();
		PTONS(pBuffer, temp2);
		PTONBN(ND_NAME_BASE_LEN, pBuffer, name);
		int sdLength = 0;
		if (getHasSd())
		{
			const char* pDesc = getDescription();
			sdLength = strlen(pDesc) + 1;
			PTONB(pBuffer, sdLength);
			PTONBN(sdLength, pBuffer, pDesc);
		}
		else
		{
			PTONB(pBuffer, sdLength);
		}
		PTONS(pBuffer, m_nvType);
	}
}

NdErrorType NdNetworkVariable::makeSiExtensionData(unsigned char* pBuf, int len, int &resultLen)
{
	if (hasExtensionData())
	{
		// Create extension data up to length N.  If N exceeded, truncate SD string
		int origLen = len;
		unsigned char* pHdr = pBuf;
		unsigned char hdr = 0;
		// Adjust for header
		if (pBuf) pBuf++;
		len--;
		if (len && m_nRateEstimate != NO_RATE_ESTIMATE)
		{
			if (pBuf)
			{
				*pBuf++ = m_nRateEstimate;
			}
			hdr |= 0x40;
			len--;
		}
		if (len && m_nMaxRateEstimate != NO_RATE_ESTIMATE)
		{
			if (pBuf)
			{
				*pBuf++ = m_nMaxRateEstimate;
			}
			hdr |= 0x80;
			len--;
		}
		char name[ND_NAME_LEN];
		getName(name);
		int stringLength = strlen(name) + 1;
		if (stringLength > 1 && stringLength <= len)
		{
			if (pBuf)
			{
				strcpy((char*) pBuf, name);
				pBuf += stringLength;
			}
			hdr |= 0x20;
			len -= stringLength;
		}
		stringLength = strlen(m_szDescription) + 1;
		if (len && stringLength > 1)
		{
			if (stringLength <= len)
			{
				if (pBuf)
				{
					strcpy((char*) pBuf, m_szDescription);
					pBuf += stringLength;
				}
				len -= stringLength;
			}
			else
			{
				if (pBuf)
				{
					strncpy((char*) pBuf, m_szDescription, len);
					pBuf += len;
				}
				len = 0;
			}
			hdr |= 0x10;
		}
		if (len && m_nArrayLength)
		{
			if (pBuf)
			{
				*pBuf++ = (unsigned char) (m_nArrayLength>>8);
			}
			hdr |= 0x08;
			if (--len)
			{
				if (pBuf)
				{
					*pBuf = (unsigned char) m_nArrayLength;
				}
				len--;
			}
		}

		if (pBuf)
		{
			*pHdr = hdr;
		}
		resultLen = origLen - len;
	}
	else
	{
		resultLen = 0;
	}
	return (len == 0) ? ND_STRING_TOO_LONG : ND_OK;
}

void NdNetworkVariable::lock()
{
	if (getParent())
	{
		getParent()->lock();
	}
}

void NdNetworkVariable::unlock()
{
	if (getParent())
	{
		getParent()->unlock();
	}
}

void NdNetworkVariable::setLengthForUpdate(int l)
{
	if (getChangeableLength())
	{
		m_nLengthForUpdate = l;
	}
}
