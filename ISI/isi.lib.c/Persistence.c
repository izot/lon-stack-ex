//	Persitence.c	implementing ISI persistence files
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

#include "isi_int.h"
#include <stddef.h>

#define CURR_VERSION            1
#define ISI_IMAGE_SIGNATURE0        0xCF82
#define ISI_APP_SIGNATURE0          0
#define ISI_PERSISTENCE_HEADER_LEN  100

unsigned app_signature = ISI_APP_SIGNATURE0;

//
// These macros move data from host format to network format.
// The result is a "network byte ordered" value pointed to by p
// and the input is a "host byte ordered" value in a
// whose type is given by the macro name as follows:
//		NL: host long (32 bits)
//		N3: host 3 byte item (24 bits)
//      NS: host short (16 bits)
//		NB: host byte (8 bits)
// Following the move, "p" points just past the item moved.
#define PTONL(p,a) *p++ = (LonByte)(a>>24); PTON3(p,a)
#define PTON3(p,a) *p++ = (LonByte)(a>>16); PTONS(p,a)
#define PTONS(p,a) *p++ = (LonByte)(a>>8);  PTONB(p,a)
#define PTONB(p,a) *p++ = (LonByte)a;

//
// These macros move data from network format to host format.
// The input is a "network byte ordered" value pointed to by p
// and the result is a "host byte ordered" value in a
// whose type is given by the macro name as follows:
//		HL: host long (32 bits)
//		H3: host 3 byte item (24 bits)
//      HS: host short (16 bits)
//		HB: host byte (8 bits)
// Following the move, "p" points just past the item moved.
// This works without causing alignment traps.
// (unlike constructs such as a = *(int*)&p[2])
#define PTOHL(p,a) { a = (((((p[0]<<8)+p[1])<<8)+p[2])<<8)+p[3]; p+=4; }
#define PTOH3(p,a) { a = ((((p[0]<<8)+p[1])<<8)+p[2]); p+=3; }
#define PTOHS(p,a) { a = (p[0]<<8)+p[1]; p+=2; }
#define PTOHB(p,a) { a = *p++; }

#define PTOHBN(n,p,a) \
{ LonByte* q = (LonByte*)(a); for(int i=0; i<n;i++) { *q++ = *p++; } }

typedef struct
{
	unsigned int length;
	unsigned short signature;
#if PERSISTENCE_TYPE_IS(FTXL)
    unsigned long  appSignature;
#endif
	unsigned short version;			// Major version - mismatch means forget it.
	unsigned short checksum;		// Checksum of length and data
} PersistenceHeader;

static LonBool bLocked = FALSE;

void lock()
{

}

void unlock()
{

}

int computeChecksum(LonByte* pImage, int length)
{
    int i; 
	unsigned short checksum = 0;
	for (i = 0; i < length - 1; i++) 
	{
		checksum += pImage[i];
	}
	checksum += (unsigned short) length;

	return checksum;
}

LonBool validateChecksum(PersistenceHeader* pHdr, LonByte* pImage)
{
	LonBool result = TRUE;

	if (computeChecksum(pImage, pHdr->length) != pHdr->checksum)
	{
		result = FALSE;
	}
	return result;
}

LtPersistenceLossReason readPersistence(LonNvdSegmentType type, LonByte** pImage, int* imageLength, int* nVersion)
{
	LtPersistenceLossReason reason = LT_PERSISTENCE_OK;
	PersistenceHeader hdr;
    
#if PERSISTENCE_TYPE_IS(FTXL)
    LonNvdHandle f = LonNvdOpenForRead(type);
#else
    FILE* f = fopen(m_szImage, "r+b");
	if (f == NULL)
	{
		// For backward compatibility, look for name without extension
		char szName[MAX_PATH];
		strcpy(szName, m_szImage);
		char* szExt = strrchr(szName, '.');
		if (szExt != NULL) *szExt = 0;
		f = fopen(szName, "r+b");
	}
#endif
    memset(&hdr, 0, sizeof(hdr));
	if (f != NULL) 
	{
#if PERSISTENCE_TYPE_IS(FTXL)
        if (LonNvdRead(f, 0, sizeof(hdr), &hdr) != 0)
#else
        if (fread(&hdr, sizeof(hdr), 1, f) != 1) 
#endif
		{
			reason = LT_CORRUPTION;
		}
		else if (hdr.signature != ISI_IMAGE_SIGNATURE0)
		{
			reason = LT_SIGNATURE_MISMATCH;
		}
#if PERSISTENCE_TYPE_IS(FTXL)
        else if (hdr.appSignature != app_signature)
        {
            reason = LT_SIGNATURE_MISMATCH;
        }
#endif
		else if (hdr.version > CURR_VERSION)
		{
			reason = LT_VERSION_NOT_SUPPORTED;
		}
		else
		{
			*nVersion = hdr.version;
			*imageLength = hdr.length;
			*pImage = (LonByte *) malloc(*imageLength);
			if (*pImage == NULL ||
#if PERSISTENCE_TYPE_IS(FTXL)
                LonNvdRead(f, sizeof(hdr), *imageLength, *pImage) != 0 ||
#else
                fread(pImage, imageLength, 1, f) != 1 ||
#endif
				!validateChecksum(&hdr, *pImage))
			{
				reason = LT_CORRUPTION;
				free(*pImage);
				*pImage = NULL;
			}
		}
#if PERSISTENCE_TYPE_IS(FTXL)
        LonNvdClose(f);
#else
		fclose(f);
#endif
	}
	else
	{
		reason = LT_NO_PERSISTENCE;
	}

	return reason;
}

void writePersistence(LonNvdSegmentType type, LonByte* pImage, PersistenceHeader* pHdr)
{
	LonBool failure = FALSE;

	// Write synchronously to the file system.  On PC, done via WIN32 calls.  On
	// other platforms, assume FFS writes are synchronous.

#if PERSISTENCE_TYPE_IS(FTXL)
    LonNvdHandle f = LonNvdOpenForWrite(type, sizeof(*pHdr)+pHdr->length);
    if (f != NULL)
    {
        if (LonNvdWrite(f, 0, sizeof(*pHdr), pHdr) != 0 ||
            LonNvdWrite(f, sizeof(*pHdr), pHdr->length, pImage) != 0)
        {
			failure = TRUE;
        }
        LonNvdClose(f);
    }

#elif  defined(WIN32)
	HANDLE f = CreateFile(m_szImage, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
	if (f != INVALID_HANDLE_VALUE)
	{
		DWORD nWritten;
		if (!WriteFile(f, pHdr, sizeof(*pHdr), &nWritten, NULL) ||
			!WriteFile(f, pImage, pHdr->length, &nWritten, NULL))
		{
			failure = true;
		}
		CloseHandle(f);
	}
#else
    FILE* f = fopen(m_szImage, "w+b");
	if (f != null)
	{
		if (fwrite(pHdr, sizeof(*pHdr), 1, f) != 1 ||
			fwrite(pImage, pHdr->length, 1, f) != 1)
		{
			failure = true;
		}
		// For ffs, also need to do an explicit fsync to force
		// data to be written to flash.
		if (fsync(fileno(f)))
		{
			failure = true;
		}
		
		if (fclose(f))
		{
			failure = true;
		}
	}
	else
	{
		failure = true;
	}
#endif

	if (failure)
	{
		
	}
}

// serialize data in _isiPersist
LtPersistenceLossReason serializeIsiNvdSegPersistentData(LonByte** pBuffer, int* len)
{
	int image_len = sizeof(IsiPersist);
    LonByte* pBuf;

	lock();
	// Allocate memory for the serialization
 	*pBuffer = (LonByte *) malloc(image_len);
    memset(*pBuffer, 0, image_len);
    *len = image_len;
	pBuf = *pBuffer;
#ifdef  ISI_SUPPORT_TIMG
	PTONB(pBuf, _isiPersist.Devices);
#endif  //  ISI_SUPPORT_TIMG
	PTONB(pBuf, _isiPersist.Nuid);
	PTONS(pBuf, _isiPersist.Serial);
	PTONS(pBuf, _isiPersist.BootType);
	PTONB(pBuf, _isiPersist.RepeatCount);
    unlock();

    return LT_PERSISTENCE_OK;
}

LtPersistenceLossReason serializeIsiNvdSegConnectionTable(LonByte** pBuffer, int* len)
{
	int image_len = IsiGetConnectionTableSize() * sizeof(IsiConnection);
    
    lock();
	// Allocate memory for the serialization
	*pBuffer = (LonByte *) malloc(image_len);
    *len = image_len;

    memcpy((void *)*pBuffer, (const void *)IsiGetConnection(0), image_len);
    unlock();

    return LT_PERSISTENCE_OK;
}

// deserialize data in Connection Table
LtPersistenceLossReason deserializeIsiNvdSegConnectionTable(LonByte* pBuffer, int len, int nVersion)
{
    LtPersistenceLossReason reason = LT_PERSISTENCE_OK;
    int image_len = IsiGetConnectionTableSize() * sizeof(IsiConnection);

    if (len >= image_len)
    {
        memcpy((void *)IsiGetConnection(0), (void *)pBuffer ,image_len);
        DumpConnectionTable();
    }
    else
        reason = LT_PROGRAM_ATTRIBUTE_CHANGE;

    return reason;
}

// deserialize data in _isiPersist
LtPersistenceLossReason deserializeIsiNvdSegData(LonByte* pBuffer, int len, int nVersion)
{
    LtPersistenceLossReason reason = LT_PERSISTENCE_OK;

	bLocked = TRUE;
    if (len >= sizeof(_isiPersist))
    {
        int dummy;
	    LonByte* pBuf = pBuffer;
#ifdef  ISI_SUPPORT_TIMG
	    PTOHB(pBuf, _isiPersist.Devices);
#endif  //  ISI_SUPPORT_TIMG
	    PTOHB(pBuf, _isiPersist.Nuid);
	    PTOHS(pBuf, _isiPersist.Serial);
	    PTOHS(pBuf, dummy);     // Don't override _isiPersist.BootType);
	    PTOHB(pBuf, _isiPersist.RepeatCount);
    }
    else
    {
        reason = LT_CORRUPTION;
    }
    bLocked = FALSE;

    return reason;
}

LtPersistenceLossReason deserialize(LonNvdSegmentType type, LonByte* pBuffer, int imageLen, int nVersion)
{
    if (type == IsiNvdSegConnectionTable)
        return deserializeIsiNvdSegConnectionTable(pBuffer, imageLen, nVersion);
    else
    if (type == IsiNvdSegPersistent)
        return deserializeIsiNvdSegData(pBuffer, imageLen, nVersion);
    else
        // Error
        return LT_NO_PERSISTENCE;
}

void savePersistentData(LonNvdSegmentType type)
{
	LonByte* pImage = NULL;
	PersistenceHeader hdr;
	int imageLen;
    LtPersistenceLossReason reason = LT_NO_PERSISTENCE;
    hdr.version = CURR_VERSION;
    hdr.length = 0;
	hdr.signature = ISI_IMAGE_SIGNATURE0;
    hdr.checksum = 0;
#if PERSISTENCE_TYPE_IS(FTXL)
    hdr.appSignature = app_signature;
#endif

    _IsiAPIDebug("savePersistentData - for type=%d\n", type); 
    if (type == IsiNvdSegConnectionTable)
        reason = serializeIsiNvdSegConnectionTable(&pImage, &imageLen);
    else
    if (type == IsiNvdSegPersistent)
        reason = serializeIsiNvdSegPersistentData(&pImage, &imageLen);
 
    if (reason == LT_PERSISTENCE_OK)
    {
	    hdr.checksum = computeChecksum(pImage, imageLen);
	    hdr.length = imageLen;

	    if (!bLocked)
	    {
		    // Write the data to a file
		    writePersistence(type, pImage, &hdr);
	    }
        _IsiAPIDebug("Checksum=%ld ImageLen=%d AppSignature=%ld\n",hdr.checksum, hdr.length,
            hdr.appSignature); 

        if (type == IsiNvdSegConnectionTable)
        {
            DumpConnectionTable();
        }
        else
        if (type == IsiNvdSegPersistent)
        {
            DumpIsiPersistData();
        }
        _IsiAPIDump("0x", (void *)pImage, imageLen, "\n"); 
        if (pImage != NULL)
	        free (pImage);
    }
}

LtPersistenceLossReason restorePersistentData(LonNvdSegmentType type)
{
    LtPersistenceLossReason reason;
	LonByte* pBuffer = NULL;
	int imageLen = 0;
	int nVersion = 0;

    _IsiAPIDebug("restorePersistentData - for type=%d\n", type); 
    reason = readPersistence(type, &pBuffer, &imageLen, &nVersion);
    if (reason == LT_PERSISTENCE_OK)
    {
        reason  = deserialize(type, pBuffer, imageLen, nVersion);
    }

    if (pBuffer != NULL)
    {
    	free (pBuffer);
    }

    return reason;
}

void _IsiSetAppSignature(unsigned signature)
{
    app_signature = signature;
}


void DumpIsiPersistData()
{
    LonByte deviceCount = 0;

#ifdef  ISI_SUPPORT_TIMG
    deviceCount = _isiPersist.Devices;
#endif  //  ISI_SUPPORT_TIMG

    _IsiAPIDebug ("DumpIsiPersistData\n");
    _IsiAPIDebug("\tLatest device counts=%d Nuid=%d Serial=%d RepeatCount=%d\n", 
        deviceCount, _isiPersist.Nuid, _isiPersist.Serial, _isiPersist.RepeatCount);
}

