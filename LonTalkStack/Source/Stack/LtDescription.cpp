//
// LtDescription.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtDescription.cpp#2 $
//

#include <LtStackInternal.h>
#include <LtDescription.h>

LtDescription::LtDescription()
{
    m_description = NULL;
    m_dataLen = 0;
	m_options = PARTIAL_DESC_DATA;
}

LtDescription::LtDescription(int dataLen)
{
	m_dataLen = 0;
	m_description = NULL;
	set(NULL, 0, dataLen, PARTIAL_DESC_DATA);
}

LtDescription::LtDescription(const void* description, int dataLen, LtDescriptionOptions options)
{
	m_dataLen = 0;
	m_description = NULL;
	set(description, 0, dataLen, options);
}

LtDescription::~LtDescription()
{
    delete[] (char *)m_description;
}

void LtDescription::get(void* &pBuffer, int &len)
{
	LtDescriptionOptions options;
	get(pBuffer, len, options);
	if (!(options & COMMIT_DESC_DATA))
	{
		len = 0;
		release(pBuffer);
		pBuffer = null;
	}
}

void LtDescription::get(void* &pBuffer, int &len, LtDescriptionOptions &options)
{
	options = getOptions();
	len = getDataLen();
	pBuffer = (void*) new byte[len];
	memcpy(pBuffer, m_description, len);
}

void LtDescription::release(void* pBuffer)
{
	delete[] (byte*)pBuffer;
}

void LtDescription::setLength(int length)
{
	set(null, 0, length, PARTIAL_DESC_DATA);
	m_dataLen = length;
}

void LtDescription::set(const void *description, const int offset, const int dataLen, LtDescriptionOptions options)
{
	int setLen = offset + dataLen;

	// Extend string if necessary
	if (setLen > m_dataLen)
	{
		if (m_description)
		{
			void* pSave = m_description;
			m_description = new char[setLen];
			memcpy(m_description, pSave, m_dataLen);
			memset((byte*) m_description + m_dataLen, 0, setLen - m_dataLen);
			delete[] (char*)pSave;
		}
		m_dataLen = setLen;
	}

	// Copy in any new data.
	if (dataLen)
	{
		if (m_description == NULL)
		{
			m_description = new char[setLen];
			memset(m_description, 0, setLen);
		}
		if (description)
		{
			memcpy((byte*) m_description + offset, description, dataLen);
		}
    }

	m_options = options;
}

LtDescription& LtDescription::operator=(const LtDescription &original)
{
    set(original.m_description, 0, original.m_dataLen);
	return *this;
}

int LtDescription::getSerialLength()
{
	return 3 + m_dataLen;
}

void LtDescription::serialize(unsigned char* &pBuffer)
{
	int options = (int) m_options;
	PTONB(pBuffer, options);
	PTONS(pBuffer, m_dataLen);
	if (m_dataLen)
	{
		PTONBN(m_dataLen, pBuffer, m_description);
	}
}

void LtDescription::deserialize(unsigned char* &pBuffer, int nVersion)
{
	int options;
	int dataLen; 
	PTOHB(pBuffer, options);
	PTOHS(pBuffer, dataLen);
	set(pBuffer, 0, dataLen, (LtDescriptionOptions) options);
	pBuffer += dataLen;
}
