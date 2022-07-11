//
// LtPersistence.cpp
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
// This file contains class implementations for LonWorks node definitions.  See
// nodedef.h for more information.
//

#include "LtaDefine.h"
#ifdef WIN32
#include <windows.h>
#include <direct.h>
#elif defined(__VXWORKS__)
#include <ioLib.h>
#include <usrLib.h>
#else
#include <unistd.h>
#endif

#include <LtRouter.h>

#if PERSISTENCE_TYPE_IS(FTXL)
    #include "tickLib.h"
#else
    #include "LtNvRam.h"
#endif

#include "LtPersistence.h"
#include "LtPlatform.h"

#include "vxlTarget.h"

#if PERSISTENCE_TYPE_IS(FTXL)
LtPersistenceMonitor* LtPersistence::m_pPersistenceMonitor = NULL;   
boolean               LtPersistence::m_bShutdown = FALSE;
int                   LtPersistence::m_tid = ERROR;
SEM_ID                LtPersistence::m_semPending = NULL;
SEM_ID                LtPersistence::m_taskMutex = NULL;
LtPersistence*        LtPersistence::m_pPersitenceList[LonNvdSegNumSegmentTypes] = {NULL};
boolean               LtPersistence::m_bResetFlag = FALSE;
#endif

// Arguably "path in key" is a separate feature.  But for now we tie it to these features.
#if FEATURE_INCLUDED(MULTI_APP)&& FEATURE_INCLUDED(APP_INDEX_IN_PERSISTENCE_FILES)
const boolean LtPersistence::m_pendingPathInKey = FALSE;
#else
const boolean LtPersistence::m_pendingPathInKey = TRUE;
#endif

boolean LtPersistenceHeader::valid()
{
	boolean isValid = false;
	if (signature == LT_IMAGE_SIGNATURE0)
	{
		version = 1;
		length -= 7;
		isValid = true;
	}
	else if (signature == LT_IMAGE_SIGNATURE1)
	{
		isValid = true;
	}
	return isValid;
}

unsigned int LtPersistenceHeader::getLength()
{
	return length;
}

#if PERSISTENCE_TYPE_IS(FTXL)
void LtPersistence::setUpdateTime()
{
    m_lastUpdate = tickGet();
}
    
ULONG LtPersistence::guardBandRemainig()
{
    ULONG timeElapsed = tickGet() - m_lastUpdate;
    ULONG timeRemaining;
    if (timeElapsed <= m_guardBandDuration)
    {
        timeRemaining = m_guardBandDuration - timeElapsed;
    }
    else
    {
        timeRemaining = 0;  /* already expired. */
    }
    return(timeRemaining);
}

ULONG LtPersistence::store()
{
	boolean bPending;
    ULONG guardTimeLeft = (ULONG)WAIT_FOREVER;  

	semTake(m_semStore, WAIT_FOREVER);
	bPending = m_bPendingStore;

	if (bPending) 
    {
        guardTimeLeft = guardBandRemainig();
        if ((guardTimeLeft == 0 || m_bSync) && !m_readyForBackup)
        {
		    // We timed out (or a sync was requested) so flush the data, unless we
            // are all prepared for a backup.
            m_bStoreInProgress = true;
			m_bPendingStore = false;
	        semGive(m_semStore);
			saveConfig();
    	    semTake(m_semStore, WAIT_FOREVER);
            m_bStoreInProgress = false;
		}
	    m_bSync = false;
	    if (!m_bPendingStore)
	    {
		    setPending(false);
            guardTimeLeft = (ULONG)WAIT_FOREVER;
	    }
        else
        {
            guardTimeLeft = guardBandRemainig();
        }
    }
    semGive(m_semStore);

    return(guardTimeLeft);
}

int	VXLCDECL LtPersistence::storeTask( int obj, ... )
{
	while (!m_bShutdown)
	{
        int i;
        ULONG waitTime = (ULONG)WAIT_FOREVER;

        for (i = 0; i < LonNvdSegNumSegmentTypes; i++)
        {
            if (m_pPersitenceList[i] != NULL)
            {
                ULONG timeLeft;
                timeLeft = m_pPersitenceList[i]->store();
                if (timeLeft < waitTime)
                {
                    waitTime = timeLeft;
                }
            }
        }
        if (m_pPersistenceMonitor != NULL && waitTime == (ULONG)WAIT_FOREVER)
        {   
            // Nothing to do.  Mark it as finished...
            m_pPersistenceMonitor->notifyNvdComplete();
        }
		semTake(m_semPending, waitTime);
    }

    semTake(m_taskMutex, WAIT_FOREVER);
    m_tid = ERROR;
    semGive(m_taskMutex);

    return 0;
}

void LtPersistence::setPending(boolean bPending)
{
	if (m_bCommitFailureNotifyMode)
	{
        if (bPending)
        {
            LonNvdEnterTransaction(m_type);
        }
        else
        {
            LonNvdExitTransaction(m_type);
        }
	}
}

boolean LtPersistence::getPending()
{
    return LonNvdIsInTransaction(m_type);
}

#else
int	VXLCDECL LtPersistence::storeTask( int obj, ... )
{
	LtPersistence* pObj = (LtPersistence*) obj;
	pObj->storeWait();
	return 0;
}

void LtPersistence::setPending(boolean bPending)
{
	if (m_bCommitFailureNotifyMode)
	{
        LtNvRam::set(m_szPending, &bPending, sizeof(bPending), m_pendingPathInKey);
	}
}

boolean LtPersistence::getPending()
{
	boolean bPending = false;
	LtNvRam::get(m_szPending, &bPending, sizeof(bPending), m_pendingPathInKey);
	return bPending;
}

#endif

LtPersistence::LtPersistence(unsigned short nCurrentVersion)
{
	m_bPendingStore = false;
#if PERSISTENCE_TYPE_IS(FTXL)
    if (m_semPending == NULL)
    {
	    m_semPending = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
    }
    if (m_taskMutex == NULL)
    {
        m_taskMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
    }
#else
	m_semPending = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
#endif
	m_semStore = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
	m_nCurrentVersion = nCurrentVersion;
	m_tid = ERROR;
	m_nCurrentVersion = nCurrentVersion;
	m_bSync = false;
#if PERSISTENCE_TYPE_IS(FTXL)
    m_guardBandDuration = ticksToMs(1000);
    m_bStoreInProgress = false;
#else
    m_nHoldTime = 1000;
#endif
	m_bCommitFailureNotifyMode = false;
	m_bLocked = false;
	m_bChecksum = true;
	m_nChecksum = 0;
    m_readyForBackup = false;

#if PERSISTENCE_TYPE_IS(FTXL)
    m_type = LonNvdSegNumSegmentTypes;
#else
	m_nIndex = 0;
	m_szImageName[0] = 0;
	char imagePath[MAX_PATH];
    LtPlatform::getPersistPath(imagePath, sizeof(imagePath));
	setPath(imagePath);
#endif
#if PERSISTENCE_TYPE_IS(FTXL)
    m_appSignature = 0;
#endif
    m_szNvdFsPath[0] = 0;
}

LtPersistence::~LtPersistence()
{
#if PERSISTENCE_TYPE_IS(FTXL)
    if (m_type < LonNvdSegNumSegmentTypes)
    {
        boolean lastOne = true;
        semTake(m_taskMutex, WAIT_FOREVER);
        m_pPersitenceList[m_type] = NULL;
        int i;

        for (i = 0; i < LonNvdSegNumSegmentTypes && lastOne; i++)
        {
            lastOne = (m_pPersitenceList[i] == NULL);
        }

        if (lastOne)
        {
	        m_bShutdown = true;
		    semGive(m_semPending);
        }

        semGive(m_taskMutex);

        if (lastOne)
        {
            while (m_tid != ERROR && m_bShutdown)
            {
                taskDelay(1);
            }
        }
    }
#else
	semDelete(m_semPending);
#endif
	semDelete(m_semStore);
}

#if PERSISTENCE_TYPE_IS(STANDARD)

void LtPersistence::storeWait()
{
	while (true)
	{
		boolean bWriteIt;

		semTake(m_semStore, WAIT_FOREVER);
		m_bSync = false;
		bWriteIt = m_bPendingStore;
		if (!bWriteIt)
		{
			setPending(false);
			m_tid = ERROR;
		}
		semGive(m_semStore);

		if (!bWriteIt) break;

		// Wait 1 second before doing the store.  This way, any additional
		// store requests that may be coming can be batched.  Once we go
		// for 1 second with no store requests, go ahead and do the processing.
		// Also, on any reset we try to flush any pending stores.
		if ((semTake(m_semPending, msToTicks(m_nHoldTime)) == ERROR || m_bSync) &&
            !m_readyForBackup)
		{
			// We timed out (or a sync was requested) so flush the data, unless we
            // are all prepared for a backup.
			m_bPendingStore = false;
			saveConfig();
		}
	}
}
#endif

LtPersistenceLossReason LtPersistence::restore()
{
	LtPersistenceLossReason reason;
	byte* pBuffer = null;
	int imageLen;
	int nVersion;

	if (getPending())
	{
		reason = LT_RESET_DURING_UPDATE;
	}
	else
	{
		reason = read(pBuffer, imageLen, nVersion);
	}
	if (reason == LT_PERSISTENCE_OK)
	{
		reason = m_pClient->deserialize(pBuffer, imageLen, nVersion);
	}
    if (pBuffer != NULL)
    {
    	free(pBuffer);
    }
	if (reason != LT_PERSISTENCE_OK)
	{
		m_pClient->notifyPersistenceLost(reason);
	}

	return reason;
}

void LtPersistence::start()
{
	setPending(true);
	m_bPendingStore = true;
}

#if PERSISTENCE_TYPE_IS(FTXL)

boolean LtPersistence::schedule()
{
	boolean bScheduled = true;
	// Schedule an update
    m_lastUpdate = tickGet();

    if (m_pPersistenceMonitor != NULL)
    {
        m_pPersistenceMonitor->notifyNvdScheduled(m_guardBandDuration);
    }

    if (!m_bPendingStore)
	{
		// Flag the image as being in a state of flux.  This is cleared once
		// the image is written.
		semTake(m_semStore, WAIT_FOREVER);
		start();
		semGive(m_semStore);

        semTake(m_taskMutex, WAIT_FOREVER);
		if (m_tid == ERROR)
		{
			m_tid = taskSpawn("LtPersist", 
                              LT_PERSISTENCE_TASK_PRIORITY, 0, 
                              LT_PERSISTENCE_TASK_STACK_SIZE, storeTask,
	  		    	          (int)this, 0,0,0,0, 0,0,0,0,0);
		}
        semGive(m_taskMutex);
	}
    semGive(m_semPending);
	return bScheduled;
}
#else
boolean LtPersistence::schedule()
{
	boolean bScheduled = true;
#if PRODUCT_IS(ILON)
	if (LtPlatform::remainingDiskSpace() < 10000)
	{
		bScheduled = false;
	}
	else 
#endif
	// Schedule an update
    if (m_bPendingStore)
	{
		semGive(m_semPending);
	}
	else
	{
		// Flag the image as being in a state of flux.  This is cleared once
		// the image is written.
		semTake(m_semStore, WAIT_FOREVER);
		start();
		if (m_tid == ERROR)
		{
			m_tid = taskSpawn("LtPersist", 
                              LT_PERSISTENCE_TASK_PRIORITY, 0, 
                              LT_PERSISTENCE_TASK_STACK_SIZE, storeTask,
	  		    	          (int)this, 0,0,0,0, 0,0,0,0,0);
		}
		semGive(m_semStore);
	}
	return bScheduled;
}
#endif

void LtPersistence::registerPersistenceClient(LtPersistenceClient* pClient)
{
	m_pClient = pClient;
}

void LtPersistence::setHoldTime(int nTime)
{
#if PERSISTENCE_TYPE_IS(FTXL)
    m_guardBandDuration = msToTicks(nTime);
#else
	m_nHoldTime = nTime;
#endif
}

boolean LtPersistence::validateChecksum(LtPersistenceHeader* pHdr, byte* pImage)
{
	boolean result = true;
	// Can't checksum signature 0 checksum.
	if (pHdr->signature != LT_IMAGE_SIGNATURE0)
	{
		if (computeChecksum(pImage, pHdr->getLength()) != pHdr->checksum)
		{
			result = false;
		}
	}
	return result;
}

int LtPersistence::computeChecksum(byte* pImage, int length)
{
	// If checksumming is suppressed, just use the last one computed.
	// This allows for two cases:
	//  1. Someone wants to force a checksum violation (yes, this is done)
	//  2. Someone modifies memory but in skips checksumming to save time
	//     and modifies memory in such a way as to perserve the checksum value
	//     (not sure if anyone does this for the network image, but I believe 
	//      the debugger may do this when setting breakpoints).
	if (m_bChecksum)
	{
		int i; 
		unsigned short checksum = 0;
		for (i = 0; i < length - 1; i++) 
		{
			checksum += pImage[i];
		}
		checksum += (unsigned short) length;
		m_nChecksum = checksum;
	}

	m_bChecksum = true;

	return m_nChecksum;
}

void LtPersistence::setLocked(boolean bLocked)
{
	m_bLocked = bLocked;
}

void LtPersistence::write(byte* pImage, LtPersistenceHeader* pHdr)
{
	boolean failure = false;

#if PERSISTENCE_TYPE_IS(STANDARD)
	// Always make sure the directory exists - in case someone deleted it!
	LtIpMakeDir(m_szImagePath);
#endif

	// Write synchronously to the file system.  On PC, done via WIN32 calls.  On
	// other platforms, assume FFS writes are synchronous.

#if PERSISTENCE_TYPE_IS(FTXL)
    LonNvdHandle f = LonNvdOpenForWrite(m_type, sizeof(*pHdr)+pHdr->length);
    if (f != NULL)
    {
        if (LonNvdWrite(f, 0, sizeof(*pHdr), pHdr) != 0 ||
            LonNvdWrite(f, sizeof(*pHdr), pHdr->length, pImage) != 0)
        {
			failure = true;
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
		vxlReportUrgent("Persistence Update Failure: file system write error.\n");
		m_pClient->notifyPersistenceLost(LT_PERSISTENT_WRITE_FAILURE);
	}
}

LtPersistenceLossReason LtPersistence::read(byte* &pImage, int& imageLength, int& nVersion)
{
	LtPersistenceLossReason reason = LT_PERSISTENCE_OK;
	LtPersistenceHeader hdr(0);

#if PERSISTENCE_TYPE_IS(FTXL)
    LonNvdHandle f = LonNvdOpenForRead(m_type);
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
		else if (!hdr.valid())
		{
			reason = LT_SIGNATURE_MISMATCH;
		}
#if PERSISTENCE_TYPE_IS(FTXL)
        else if (hdr.appSignature != m_appSignature)
        {
            reason = LT_SIGNATURE_MISMATCH;
        }
#endif
		else if (hdr.version > m_nCurrentVersion)
		{
			reason = LT_VERSION_NOT_SUPPORTED;
		}
		else
		{
			nVersion = hdr.version;
			imageLength = hdr.getLength();
			pImage = (byte *) malloc(imageLength);
			if (pImage == null ||
#if PERSISTENCE_TYPE_IS(FTXL)
                LonNvdRead(f, sizeof(hdr), imageLength, pImage) != 0 ||
#else
                fread(pImage, imageLength, 1, f) != 1 ||
#endif
				!validateChecksum(&hdr, pImage))
			{
				reason = LT_CORRUPTION;
				free(pImage);
				pImage = null;
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

#if PERSISTENCE_TYPE_IS(FTXL)
void LtPersistence::setType(LonNvdSegmentType type)
{
    m_type = type;
    if (type < LonNvdSegNumSegmentTypes)
    {
        m_pPersitenceList[type] = this;
        m_bShutdown = false;
    }
}

boolean LtPersistence::isCommitComplete()
{
	return !m_bPendingStore && !m_bStoreInProgress;
}

void LtPersistence::setNvdFsPath(const char* szNvdFSPath)
{
    memset(&m_szNvdFsPath, 0, sizeof(m_szNvdFsPath));
    if (szNvdFSPath == NULL)
    {
        // Nvd path is not specified, use the default path
        strcpy(m_szNvdFsPath, DEFAULT_NVD_FOLDER);
        strcat(m_szNvdFsPath, DIR_SEPARATOR_STRING);
    }        
    else
    {
        int pathLen = strlen(szNvdFSPath);
	    if (pathLen < (int)sizeof(m_szNvdFsPath))
	    {
		    strcpy(m_szNvdFsPath, szNvdFSPath);
		    LtIpMakeDir(m_szNvdFsPath); // make sure that the folder exists
        }
        if ((pathLen == 0 || m_szNvdFsPath[pathLen-1] != DIR_SEPARATOR_CHAR) &&
            (pathLen + 1 < (int)sizeof(m_szNvdFsPath)))
        {
            strcat(m_szNvdFsPath, DIR_SEPARATOR_STRING);
        }
	}
}

void LtPersistence::writeUniqueID(LtUniqueId &uid)
{
    LtPersistenceHeader hdr(m_nCurrentVersion);
  
#if PERSISTENCE_TYPE_IS(FTXL)
    hdr.appSignature = m_appSignature;
#endif

    hdr.checksum = computeChecksum(uid.getData(), uid.getLength());
	hdr.length = uid.getLength();

    write(uid.getData(), &hdr);
}

LtPersistenceLossReason LtPersistence::readUniqueID(LtUniqueId* pId)
{
    LtPersistenceLossReason reason = LT_PERSISTENCE_OK;
    byte* pBuffer = null;
    int imageLen;
	int nVersion;
     
    reason = read(pBuffer, imageLen, nVersion);
    if (reason == LT_PERSISTENCE_OK)
        pId->set(pBuffer);       
    else
    if (reason == LT_NO_PERSISTENCE)
    	vxlReportUrgent("Read Unique ID: No persistence file (%x)\n", reason);
    else
    	vxlReportUrgent("Read Unique ID: %s (%x)\n", getPersistenceLostReason(reason), reason);
    return (reason);
}

#else
void LtPersistence::setImageName()
{
	if (!m_pendingPathInKey)
	{
 	    sprintf(m_szImage, "%s%s%d.dat", m_szImagePath, m_szImageName, m_nIndex);
	    sprintf(m_szPending, "%s%s%d", m_szImageName, IMAGE_PENDING_KEY, m_nIndex);
	}
	else
	{
	    sprintf(m_szImage, "%s%s.dat", m_szImagePath, m_szImageName);
	    sprintf(m_szPending, "%s%s%s", m_szImagePath, m_szImageName, IMAGE_PENDING_KEY);
	}
}

void LtPersistence::setPath(const char* szPath)
{
    int pathLen = strlen(szPath);
	if (pathLen < (int)sizeof(m_szImagePath))
	{
		strcpy(m_szImagePath, szPath);
		LtIpMakeDir(m_szImagePath);
        if ((pathLen == 0 || m_szImagePath[pathLen-1] != DIR_SEPARATOR_CHAR) &&
            (pathLen + 1 < (int)sizeof(m_szImagePath)))
        {
            strcat(m_szImagePath, DIR_SEPARATOR_STRING);
        }
	}
	setImageName();
}

void LtPersistence::setName(const char* szImageName)
{
	if (strlen(szImageName) < sizeof(m_szImageName))
	{
		strcpy(m_szImageName, szImageName);
	}
	setImageName();
}

void LtPersistence::setIndex(int nIndex)
{
	m_nIndex = nIndex;
	setImageName();
}

boolean LtPersistence::isCommitComplete()
{
	return m_tid == ERROR;
}

#endif

void LtPersistence::commit()
{
	// Flush any pending updates
	m_bSync = true;
	semGive(m_semPending);
}

boolean LtPersistence::isStartComplete()
{
	return true;
}

void LtPersistence::sync()
{
	int i=0;
	// Wait for up to 10 seconds for write to complete.
	while (!isCommitComplete() && i++ < 100)
	{
		commit();
		taskDelay(msToTicks(100));
	}
}

void LtPersistence::prepareForBackup()
{
    sync();
    m_readyForBackup = true;
}

void LtPersistence::backupComplete()
{
    m_readyForBackup = false;
}

void LtPersistence::setSuppressChecksumCalculation(boolean bSuppress)
{
	m_bChecksum = !bSuppress;
}

void LtPersistence::saveConfig()
{
	byte* pImage;
	LtPersistenceHeader hdr(m_nCurrentVersion);
	int imageLen;

#if PERSISTENCE_TYPE_IS(FTXL)
    hdr.appSignature = m_appSignature;
#endif
	m_pClient->serialize(pImage, imageLen);

	hdr.checksum = computeChecksum(pImage, imageLen);
	hdr.length = imageLen;

	if (!m_bLocked)
	{
		// Write the data to a file.  To save on flash life, it might pay to
		// read the data first, compare it and then write it only on miscompare.
		// This applies to recommissions which do nothing.  
		//
		// Need to check on TrueFFS.  Does it do writes synchronously like
		// ToshFFS?  If not, is there a flush command?
		write(pImage, &hdr);
	}

	delete pImage;
}

void LtPersistence::setCommitFailureNotifyMode(boolean bMode)
{
	m_bCommitFailureNotifyMode = bMode;
}

void LtPersistence::resetPersistence()
{
#if PERSISTENCE_TYPE_IS(FTXL)
    LonNvdDelete(m_type);
#else
    LtIpDeleteFile(m_szImage);
#endif
}

const char* LtPersistence::getPersistenceLostReason(int reason)
{
	switch (reason)
	{
		case LT_CORRUPTION:
			return "an image corruption";
		case LT_PROGRAM_ID_CHANGE:
			return "a program ID change";
		case LT_SIGNATURE_MISMATCH:
			return "a signature mismatch";
		case LT_PROGRAM_ATTRIBUTE_CHANGE:
			return "a program attribute change";
		case LT_PERSISTENT_WRITE_FAILURE:
			return "a persistence write failure";
		case LT_NO_PERSISTENCE:
			return "missing persistence";
		case LT_RESET_DURING_UPDATE:
			return "a reset or power cycle while updating persistent data";
		case LT_VERSION_NOT_SUPPORTED:
			return "an unsupported version";
	}
	return "an unknown reason";
}

