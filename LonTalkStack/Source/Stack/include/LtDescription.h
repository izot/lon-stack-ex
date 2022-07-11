#ifndef _LTDESCRIPTION_H
#define _LTDESCRIPTION_H

//
// LtDescription.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtDescription.h#1 $
//

#define PARTIAL_DESC_DATA 0x00
#define  COMMIT_DESC_DATA 0x01
typedef byte LtDescriptionOptions;

class LTA_EXTERNAL_CLASS LtDescription
{
public:
    LtDescription();
	LtDescription(int dataLen);
    LtDescription(const void *description, int dataLen, LtDescriptionOptions options);
    virtual ~LtDescription();
    void *getDescription() { return m_description; }
    LtDescription& operator=(const LtDescription &original);
	void set(const void *description, const int offset, const int dataLen, LtDescriptionOptions options = COMMIT_DESC_DATA);
	boolean isValid() { return (m_dataLen == 0 || (m_options & COMMIT_DESC_DATA) == COMMIT_DESC_DATA); }
	LtDescriptionOptions getOptions() { return m_options; }
	void setLength(int length);

	int getLength() { return isValid() ? getDataLen() : 0; }
	void get(void* &pBuffer, int& len);
	void get(void* &bBuffer, int &len, LtDescriptionOptions &options);
    int getDataLen() { return m_dataLen; }

	void release(void* pBuffer);	// Release a buffer allocated by "get"

	LT_SERIALIZABLE;

private:
    void *m_description;
	int   m_dataLen;
	LtDescriptionOptions m_options;
};

#endif
