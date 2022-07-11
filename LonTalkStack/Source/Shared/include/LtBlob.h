// LtBlob.h: interface for the LtBlob class.
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
//////////////////////////////////////////////////////////////////////

#ifndef LTBLOB_H
#define LTBLOB_H

#include <LtaDefine.h>

#include <VxlTypes.h>
// #include "windows.h"
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class LTA_EXTERNAL_CLASS LtBlob  
{
private:
    boolean m_packing;
    int     m_blobSize;
    int     m_allocatedSize;
    void    *m_pBlobData;
    void    *m_pNext;
    void    allocBuf(int size);
	int     m_nextIndex;

public:
	LtBlob();   // Create an empty blob.  It is initially in packing mode.
    LtBlob(void *pData);    // Create a blob with the data already filled in, from a previous copyBlob.  It is in unpacking mode.
	virtual ~LtBlob();
    void    emptyBlob();
    boolean isPacking() { return(m_packing); }; // Returns true if package is is "packing mode", FALSE if package is in "unpacking mode".

    /* The various package methodes store the specified data at the end of the 
       blob when in packing mode, and extract the "next data" when in unpacking 
       mode. */
    void package(unsigned char *pOneByte)    {package(pOneByte, sizeof(*pOneByte));};
    void package(unsigned short *pTwoByte)   {package(pTwoByte, sizeof(*pTwoByte));};
    void package(unsigned int   *pFourByte)  {package(pFourByte, sizeof(*pFourByte));};
    void package(unsigned long   *pFourByte) {package(pFourByte, sizeof(*pFourByte));};
    void package(char *pOneByte)             {package(pOneByte, sizeof(*pOneByte));};
    void package(short *pTwoByte)            {package(pTwoByte, sizeof(*pTwoByte));};
    void package(int   *pFourByte)           {package(pFourByte, sizeof(*pFourByte));};
    void pack(unsigned char oneByte)         {pack(&oneByte, sizeof(oneByte));};
    void pack(unsigned short twoByte)        {pack(&twoByte, sizeof(twoByte));};
    void pack(unsigned int   fourByte)       {pack(&fourByte, sizeof(fourByte));};
    void pack(unsigned long   fourByte)      {pack(&fourByte, sizeof(fourByte));};
    void pack(char oneByte)                  {pack(&oneByte, sizeof(oneByte));};
    void pack(short twoByte)                 {pack(&twoByte, sizeof(twoByte));};
    void pack(int   fourByte)                {pack(&fourByte, sizeof(fourByte));};

    void package(void *pData, int len); /* Arbitrary */
    void pack(const void *pData, int len); /* Arbitrary */
    void packageString(const char *str);
    char *unpackageString();
    void releaseString(char *str);
    int blobSize(); // Return the size needed to hold a transportable copy of the blob. 
    void copyBlob(void *pData); // Copy the blob into the specified data buffer. 
    /* In packing mode, store a TRUE if p is NULL, FALSE if p is not NULL, and 
       return the value.  In unpacking mode, extract the state of the pointer 
       previously packed. */
    boolean packagePointerState(void *p) 
    {
        boolean isAllocated;
        if (m_packing)
        {
            isAllocated = (p != NULL);
        }
        package(&isAllocated);
        return(isAllocated);
    }
    #define PACKAGE_VALUE(value) package(&value, sizeof(value))
    void package(class LtDomain *pDomain);
    void package(class LtSubnetNode *pSubnetNode);
    void package(class LtUniqueId *pUniqueId);
};


// LtStackBlob - should this be in a different file, in stack/include?
class LTA_EXTERNAL_CLASS LtStackBlob 
{
public:
    LtStackBlob(LtBlob *pBlob) { m_pBlob = pBlob; }
    LtBlob *pBlob() { return m_pBlob; }

    void package(class LtDomainConfiguration *pDomainConfiguration);
    void package(class LtOutgoingAddress *pLtOutgoingAddress);
    void package(class LtRefId *pLtRefId);
    void package(class LtNetworkVariableConfiguration *pLtNetworkVariableConfiguration);
    void package(class LtIncomingAddress *pLtIncomingAddress);
    void package(class LtStatus *pStatus);
    void package(class LtAddressConfiguration *pAddressConfiguration);
    void package(class LtCommParams *pCommParams);
    void package(class LtMsgOverrideOptions *pMsgOverrideOptions);
    void package(class LtMsgOverride *pMsgOverride);

private:
    LtBlob *m_pBlob;
};

#endif // !defined(LTBLOB_H)
