// LtBlob.cpp: implementation of the LtBlob class.
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

#include <LtStackInternal.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define BLOB_PRE_ALLOCATION_SIZE 100

typedef struct BlobHeader
{
    int blobSize;
} BlobHeader;

void LtBlob::emptyBlob()
{
    if (m_pBlobData != NULL)
    {
        free(m_pBlobData);
    }
    m_allocatedSize = 0;
    m_blobSize      = 0;
    m_nextIndex     = 0;
    m_packing       = TRUE; 
    m_pBlobData     = NULL; 
    m_pNext         = NULL; 
}

// Create an empty blob.  It is initially in packing mode.
LtBlob::LtBlob(void) 
{
    m_pBlobData = NULL;
    emptyBlob();
}

void LtBlob::allocBuf(int size)
{
    m_allocatedSize = size;
    m_pBlobData = malloc(m_allocatedSize);
    m_pNext = m_pBlobData;
}

//  Create a blob with the data already filled in, from a previous copyBlob. 
//  It is in unpacking mode.
LtBlob::LtBlob(void *pData) 
{
    BlobHeader *pHeader = static_cast<BlobHeader *>(pData);
    m_pBlobData = NULL;
    emptyBlob();

    m_packing = FALSE;
    m_blobSize = pHeader->blobSize;
    allocBuf(m_blobSize);
    
    memcpy(m_pBlobData, pHeader+1, m_blobSize);
}

LtBlob::~LtBlob(void)
{
    if (m_pBlobData != NULL)
    {
        free(m_pBlobData);
    }
}

void LtBlob::pack(const void *pData, int len)
{
    if (m_allocatedSize < (m_blobSize + len))
    {
        int sizeNeeded = m_blobSize + len + BLOB_PRE_ALLOCATION_SIZE;
        if (m_pBlobData == NULL)
        {
            allocBuf(sizeNeeded);
        }
        else
        {
            m_pBlobData = realloc(m_pBlobData, sizeNeeded);
            m_pNext = static_cast<char *>(m_pBlobData) + m_blobSize;
            m_allocatedSize = sizeNeeded;
        }
    }
    memcpy(m_pNext, pData, len);
    m_blobSize += len;
    m_pNext = static_cast<char *>(m_pNext) + len;
    m_nextIndex += len;
}
// Add or extract data, depening on packing mode. 
void LtBlob::package(void *pData, int len)
{
    if (m_packing)
    {
        pack(pData,len);
    }
    else if (m_blobSize < (m_nextIndex + len))
    {
        // This is bad
    }
    else 
    {
        memcpy(pData, m_pNext, len);
        m_pNext = static_cast<char *>(m_pNext) + len;
        m_nextIndex += len;
    }

}

// Copy the blob into the specified data buffer. 
void LtBlob::copyBlob(void *pData)
{
    BlobHeader *pHeader = static_cast<BlobHeader *>(pData);
    pHeader->blobSize = m_blobSize;
    memcpy(pHeader + 1, m_pBlobData, m_blobSize);
}

int LtBlob::blobSize() 
{
    return (m_blobSize + sizeof(BlobHeader)); 
};
 
void LtBlob::packageString(const char *str)
{
    if (m_packing)
    {
        int len = strlen(str) + 1;
        package(&len);
        package((void *)str, len);
    }
}

char *LtBlob::unpackageString()
{
    char *str = NULL;
    if (!m_packing)
    {
        int len = 0;
        package(&len);
        if (len != 0)
        {
            str = (char *)malloc(len);
            package(str, len);
        }
    }
    return(str);
}
void LtBlob::releaseString(char *str)
{
    free(str);
}

void LtBlob::package(class LtDomain *pDomain)
{
    pDomain->package(this);
}

void LtBlob::package(class LtSubnetNode *pSubnetNode)
{
    pSubnetNode->package(this);
}

void LtBlob::package(class LtUniqueId *pUniqueId)
{
    pUniqueId->package(this);
}

// LtStackBlob - should this be in a different file, in stack/include?
void LtStackBlob::package(class LtDomainConfiguration *pDomainConfiguration)
{
    pDomainConfiguration->package(m_pBlob);
}

void LtStackBlob::package(class LtOutgoingAddress *pLtOutgoingAddress)
{
    pLtOutgoingAddress->package(m_pBlob);
}

void LtStackBlob::package(class LtNetworkVariableConfiguration *pLtNetworkVariableConfiguration)
{
    pLtNetworkVariableConfiguration->package(m_pBlob);
}

void LtStackBlob::package(class LtRefId *pLtRefId)
{
    pLtRefId->package(m_pBlob);
}

void LtStackBlob::package(class LtIncomingAddress *pLtIncomingAddress)
{
    pLtIncomingAddress->package(m_pBlob);
}

void LtStackBlob::package(class LtStatus *pStatus)
{
    pStatus->package(m_pBlob);
}

void LtStackBlob::package(class LtAddressConfiguration *pAddressConfiguration)
{
    pAddressConfiguration->package(m_pBlob);
}

void LtStackBlob::package(class LtCommParams *pCommParams)
{
    m_pBlob->package(pCommParams->m_nData, sizeof(pCommParams->m_nData));
}

void LtStackBlob::package(class LtMsgOverrideOptions *pMsgOverrideOptions)
{
    pMsgOverrideOptions->package(m_pBlob);
}

void LtStackBlob::package(class LtMsgOverride *pMsgOverride)
{
    pMsgOverride->package(m_pBlob);
}
