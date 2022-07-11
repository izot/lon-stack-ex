//
// LtPlatform.cpp
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
//

#include "LtaDefine.h"
#ifdef WIN32
#include <windows.h>
#include <winreg.h>
#include <direct.h>
#include <sys\stat.h>
#elif defined(__VXWORKS__)
#include <vxWorks.h>
#include <usrLib.h>
#include <ioLib.h>
#include <time.h>
#include <System.h>
#include <stat.h>		// moved from sys/stat.h is
#include "echelon\ilon.h"
#else
#include <sys/stat.h>
#endif
#include <ctype.h>

#include "LtStack.h"
#include "pncLtUniqueIdList.h"
#include "LtStackInternal.h"
#include "vxlTarget.h"
#include "LtStart.h"
#if FEATURE_INCLUDED(IP852)
#include "LtIpMaster.h"
#include "routerControl.h"
#endif
#if PRODUCT_IS(DCX)
#include "Max.h"
#include "vldv.h"
#endif
#include <assert.h>

#if defined(ILON_PLATFORM) || defined(linux)&& !defined(__VXWORKS__)
#include "unistd.h"
#include <dirent.h>
#if defined(linux)
  #include <errno.h>
#if !(defined(ILON_PLATFORM) || defined(LONTALK_IP852_STACK_PLATFORM) || defined(IZOT_IP852_PLATFORM)|| defined(IZOT_PLATFORM) || defined(LIFTBR_PLATFORM))
  #include "ech_logger.h"
#endif
#endif
#endif

#ifdef unix
extern "C" int CopyFile(const char *f1name, const char *f2name, int bFailIfExists);
extern int CreateDirectoryTree	(const char* szPath);
#define makeDir CreateDirectoryTree
#endif

#if FEATURE_INCLUDED(NVRAM)
#if FEATURE_INCLUDED(EMBEDDED)
const boolean LtPlatform::m_errorLogPathInKey = FALSE;
#else
const boolean LtPlatform::m_errorLogPathInKey = TRUE;
#endif
#endif

static char szPersistPath[MAX_PATH];

byte LtPlatform::getErrorLog()
{
    byte errorLog = 0;
#if FEATURE_INCLUDED(NVRAM)
    if (m_szErrorLogName == NULL)
        setErrorLogPath(szPersistPath);
    LtNvRam::get(m_szErrorLogName, &errorLog, sizeof(errorLog), m_errorLogPathInKey);
#endif
	return errorLog;
}

void LtPlatform::setErrorLog(byte errorLog)
{
#if FEATURE_INCLUDED(NVRAM)
    if (m_szErrorLogName == NULL)
        setErrorLogPath(szPersistPath);
    LtNvRam::set(m_szErrorLogName, &errorLog, sizeof(errorLog), m_errorLogPathInKey);
#endif
}

void LtPlatform::setPersistPath( const char* pPath )
{
	strncpy(szPersistPath, pPath, sizeof(szPersistPath) - 1);
	szPersistPath[sizeof(szPersistPath) - 1] = 0;
}

// Nice simple interface for easy integration...
void LtPlatformSetPersistPath( const char* pPath)
{
	LtPlatform::setPersistPath(pPath);
}

int LtIpMakeDir(const char* szDir)
{
#ifdef WIN32
	return _mkdir(szDir);
#else
	return makeDir(szDir);
#endif
}

#if PERSISTENCE_TYPE_IS(STANDARD)
boolean LtPlatform::getPersistPath( char* pPathRtn, int nMaxLen )
{
#if !defined(WIN32)
	boolean		bOk = true;
	if ( nMaxLen < 16 )
	{
		bOk = false;
	}
	else if (strlen(szPersistPath))
	{
		strncpy(pPathRtn, szPersistPath, nMaxLen);
	}
	else
	{
		// use a constant string on the target
		strncpy( pPathRtn, FILEPATH_ROOT "ltConfig/", nMaxLen );
		makeDir(pPathRtn);
	}
	return bOk;
#else	// WIN32
	// On windows, look some up in the registry and add something
	// to it.
	// Registry path is HK_LOCAL_MACHINE / SOFTWARE / "LonWorks" /
	// "LonWorks Path"
	LONG	result; // should be ERROR_SUCCESS
	HKEY	key = NULL;
	HKEY	key2 = NULL;
	DWORD	type; // = REG_SZ
	DWORD	length = MAX_PATH;
	boolean	bOk = false;

	do
	{
		length = strlen(szPersistPath);
		if ( length && LtIpDirExists(szPersistPath))
		{
			if ( length < (ULONG)nMaxLen )
			{	bOk = true;
			}
			break;
		}
		result = RegOpenKeyEx
			( HKEY_LOCAL_MACHINE,
			"SOFTWARE",
			0, KEY_READ, &key );
		if ( result != ERROR_SUCCESS )
		{	break;
		}
		result = RegOpenKeyEx
			( key,
			"LonWorks",
			0, KEY_READ, &key2 );
		if ( result != ERROR_SUCCESS )
		{	break;
		}
		length = MAX_PATH;
		result = RegQueryValueEx( key2, "LonWorks Data Path", NULL,
								&type, (BYTE*)szPersistPath, &length );
		if ( result != ERROR_SUCCESS )
		{	break;
		}
		// check for enough extra space for additions
		if ( ( MAX_PATH - length ) < 30 )
		{	break;
		}
		if ( (strlen(szPersistPath)+30) > (ULONG)nMaxLen )
		{	break;
		}
		bOk = true;
		// make sure path has a trailing backslash and add our path
		if ( szPersistPath[strlen(szPersistPath)-1] != '\\' )
		{	strcat( szPersistPath, "\\" );
		}
		// create the path string and the sub folders too along the way
		strcat( szPersistPath, "System" );
		_mkdir( szPersistPath );
		strcat( szPersistPath, "\\LtConfig" );
		_mkdir( szPersistPath );
#if PRODUCT_IS(ILON)
		// Add extra folder layers for iLON testing on Windows
		char ilonBuf[20];
		strcat( szPersistPath, "\\iLONtest" );
		_mkdir( szPersistPath );
		sprintf(ilonBuf, "\\iLON%d", m_iLonSimIndex);
		strcat( szPersistPath, ilonBuf );
		_mkdir( szPersistPath );
#endif
		strcat( szPersistPath, "\\" );

	} while (false );
	if ( key )
	{	result = RegCloseKey( key );
	}
	if ( key2 )
	{	result = RegCloseKey( key2 );
	}
	if ( !bOk )
	{	strcpy( pPathRtn, "" );
	}
	else
	{	strcpy( pPathRtn, szPersistPath );
	}
	return bOk;
#endif // WIN32
}

int LtPlatform::remainingDiskSpace()
{
	int bytesFree = 0;
#if defined(WIN32) || defined(unix)
	// TODO: Assume lots (TBD)
	bytesFree = 0x7fffffff;
#else
    struct statfs fsInfo;

    if (statfs(FILEPATH_ROOT, &fsInfo) == OK)
    {
		bytesFree = fsInfo.f_bfree * fsInfo.f_bsize;
	}
#endif
	return bytesFree;
}

#else

boolean LtPlatform::getPersistPath( char* pPathRtn, int nMaxLen )
{
    boolean bOk = true;    
    int pathLen = strlen(szPersistPath);

    if ( nMaxLen < 16 )
		bOk = false;
	else
    {
        if (!(pathLen))
        {
		    // Folder has not set yet.  Use default folder 
            strcpy(szPersistPath, DEFAULT_NVD_FOLDER);
        }
        strncpy(pPathRtn, szPersistPath, nMaxLen);
    }
	return bOk;
}

int LtPlatform::remainingDiskSpace()
{
	return 0;
}

#endif

LtPlatform::LtPlatform()
{
#if FEATURE_INCLUDED(NVRAM)
    m_szErrorLogName = NULL;
#endif
};

LtPlatform::~LtPlatform()
{
#if FEATURE_INCLUDED(NVRAM)
    delete m_szErrorLogName;
#endif
};

void LtPlatform::ltShutdown(void)
{
#if FEATURE_INCLUDED(MULTI_APP) && (defined(WIN32) || (defined(ILON_PLATFORM) && defined(linux)))
    semDelete(m_semUidLock);
#endif
}

#if !FEATURE_INCLUDED(MULTI_APP)

// If you only have a single app, then these platform specific routines are basically just stubs.

void LtPlatform::generateUniqueId(LtUniqueId* pUid)
{
    LonUniqueId uid;
    /* Simple "srand()" seed: just use "time()" */
    unsigned int iseed = (unsigned int)time(NULL);

    srand (iseed);
    int i = rand();

    uid[0] = (LonByte)0xFE; // first byte muse be FE
    uid[1] = (LonByte)(i & 0xff); 
    while(uid[1] == 0xAA)
    {
        // Second byte can be any number except 0xAA
        i = rand();
        uid[1] = (LonByte)(i & 0xff);
    }
    i = rand();
    uid[2] = (LonByte)(i >> 24 & 0xff); 
    uid[3] = (LonByte)(i >> 16 & 0xff); 
    uid[4] = (LonByte)(i >> 8 & 0xff); 
    uid[5] = (LonByte)(i & 0xff); 
    pUid->set((const byte *)&uid);
}


LtUniqueId LtPlatform::m_uniqueId;

#if FEATURE_INCLUDED(IP852)
boolean LtPlatform::m_IsFirstRun = true;
#endif

boolean LtPlatform::getUniqueId(LtUniqueId* pUid)
{
#if FEATURE_INCLUDED(IP852)
    LtPersistence persistence(0);
    persistence.setType(LonNvdSegUniqueId);
    if (!(m_uniqueId.isSet()))
    {
        // The unique Id has not been set.  Check the persistent file
        LtUniqueId newUId;
        if (persistence.readUniqueID(&newUId) == LT_PERSISTENCE_OK)
        {
            m_uniqueId.set(newUId);
        }
    }
#endif
    pUid->set(m_uniqueId);
    return true;
}

void LtPlatform::setUniqueId(LtUniqueId &uid)
{
#if FEATURE_INCLUDED(IP852)
    LtPersistence persistence(0);
    persistence.setType(LonNvdSegUniqueId);

	LtUniqueId newUId;
    boolean bExistingUIdIsAvail = (persistence.readUniqueID(&newUId) == LT_PERSISTENCE_OK);
    if (!(uid.isSet()))
    {
        // the app doesn't specify the uniqueID to register, use the persistent unique ID which is saved in the NVD path
        if (bExistingUIdIsAvail)
        {
            uid.set(newUId);
			m_IsFirstRun = LtPersistence::getResetFlag();  // If there was a reset, treat it as IsFirstRun
        }
        else
        {
            // If there is no uniqueID file in the NVD path, generate a new one
            generateUniqueId(&uid);
			m_IsFirstRun = true;
        }
    }
	else
	{
		// The app has specified the unique ID to register, check if it's the same with the one saved in NVD path
		if (bExistingUIdIsAvail)
			// The app is not running for the first time if the UniqueID is the same with the one being saved in
			// NVD path
            // If there was a reset, treat it as IsFirstRun
            m_IsFirstRun = (uid == newUId) ? LtPersistence::getResetFlag() : true;
		else
			m_IsFirstRun = true;
	}
#endif

    m_uniqueId.set(uid);

#if FEATURE_INCLUDED(IP852)
    persistence.writeUniqueID(uid);
#endif

}

void LtPlatform::reset()
{
}

#else  // FEATURE_INCLUDED(MULTI_APP)

// EPANG TODO - simulate for iLON Linux for now
#if defined(WIN32) || (defined(ILON_PLATFORM) && defined(linux))
extern "C"
{
	int pncSerialRomRead(int i);
};

int pncSerialRomRead(int i)
{
	if (i == 6) return 3;
	if (i == 3) return 0x0051;
	if (i == 4) return 0x0201;
	if (i == 5) return 0x00fd;
	return 0xffff;
}
SEM_ID    LtPlatform::m_semUidLock = semMCreate(SEM_Q_FIFO);

#endif

LtErrorType LtPlatform::setIndex(int index)
{
#if FEATURE_INCLUDED(MULTI_APP)
	imageIndex = index;
#endif
#if FEATURE_INCLUDED(NVRAM) && FEATURE_INCLUDED(EMBEDDED)
	char key[MAX_KEY_SIZE];
	sprintf(key, ERROR_LOG_KEY, imageIndex);
    delete m_szErrorLogName;
    m_szErrorLogName = new char[strlen(key) + 1];
    strcpy(m_szErrorLogName,key);
#endif
	return LT_NO_ERROR;
}

// EPANG TODO - simulate for iLON Linux for now
#if defined(WIN32) || (defined(ILON_PLATFORM) && defined(linux))

LtUniqueId* LtPlatform::m_pUidMap = null;

void LtPlatform::setUniqueIdMap(LtUniqueId* pUid)
{
	m_pUidMap = pUid;
}

struct LtKeyedUidMap
{
    boolean    allocated;
    int        imageIndex;  // the key.
    LtUniqueId uniqueId;
};
LtKeyedUidMap* LtPlatform::m_pKeyedUidMap = NULL;
int LtPlatform::m_keyedUidMapUseCount = 0;

#if PRODUCT_IS(ILON)
int LtPlatform::m_iLonSimIndex = 0;
#endif

#define MAX_KEYED_UID_ENTRIES 1000

LtKeyedUidMap *LtPlatform::findKeyedUniqueId(int imageIndex)
{
    LtKeyedUidMap *p = NULL;
    if (m_pKeyedUidMap != NULL)
    {
        for (int i = 0; i < MAX_KEYED_UID_ENTRIES; i++)
        {
            if ((!m_pKeyedUidMap[i].allocated) || (m_pKeyedUidMap[i].imageIndex == imageIndex))
            {
                p = &m_pKeyedUidMap[i];
                break;
            }
        }
    }
    return(p);
}

// Note that setUniqueId and clearUniqueId  are not thread safe
void LtPlatform::setUniqueId(LtUniqueId &uniqueId, int imageIndex)
{
    lockUid();
    if (m_pKeyedUidMap == NULL)
    {
        m_pKeyedUidMap = new LtKeyedUidMap[MAX_KEYED_UID_ENTRIES];
        memset(m_pKeyedUidMap, 0, sizeof(LtKeyedUidMap)*MAX_KEYED_UID_ENTRIES);
    }
    for (int i = 0; i < MAX_KEYED_UID_ENTRIES; i++)
    {
        if (!m_pKeyedUidMap[i].allocated)
        {
            m_pKeyedUidMap[i].imageIndex = imageIndex;
            m_pKeyedUidMap[i].uniqueId = uniqueId;
            m_pKeyedUidMap[i].allocated = TRUE;
            m_keyedUidMapUseCount++;
            break;
        }
    }
    unlockUid();
}

void LtPlatform::clearUniqueId(int imageIndex)
{
    lockUid();
    LtKeyedUidMap *p = findKeyedUniqueId(imageIndex);
    if (p != NULL)
    {
        memset(p, 0, sizeof(*p));
        if (--m_keyedUidMapUseCount == 0)
        {
            delete m_pKeyedUidMap;
            m_pKeyedUidMap = NULL;
        }
    }
    unlockUid();
}

#if PRODUCT_IS(ILON)
// Set an index for iLON simulations to maintain separate folders for each one
void LtPlatform::setILonSimIndex(int simIndex)
{
	m_iLonSimIndex = simIndex;
}
#endif	// ILON
#endif	// WIN32

#ifdef LIFTBR_PLATFORM
static byte nids[6][6]; //Need to define max image count
void LtPlatform::setUniqueId(LtUniqueId* pUid, int imageIndex) {
	memcpy(nids[imageIndex], pUid, 6);
}
#endif

//
// Allocate a unique ID given the index of this device.
//
boolean LtPlatform::getUniqueId(LtUniqueId* pUid)
{
	boolean result = true;
    int index = imageIndex;
#if PRODUCT_IS(DCX)
	byte nid[LT_UNIQUE_ID_LENGTH];

	if (vldv_getUniqueNid(index, nid) == LDV_OK)
		pUid->set(nid);
	else
		result = false;
	
// EPANG TODO - simulate for iLON Linux for now
#elif defined(WIN32) || (defined(ILON_PLATFORM) && defined(linux))
    lockUid();
	if (index == -1)
	{
		// Ignore index of -1; this is used by the Layer2 Stack.
		result = false;
	}
	else
	{
		if (m_pUidMap == null)
		{
            LtKeyedUidMap *p = findKeyedUniqueId(imageIndex);
            if (p == NULL)
            {
			    // Default to Toshiba's first UID.  (?)
			    byte temp[6] = {0x80,0x00,0x00,0x00,0x40,0x00};
#if PRODUCT_IS(ILON)
				if (index < 16)
				{
					// Use the platform sim index, combine with app image index
					temp[4] |= (m_iLonSimIndex & 0x3f0) >> 4;	// keep six bits
					temp[5] = ((m_iLonSimIndex & 0xf) << 4) | (index & 0xf);
				}
				else
				{
					result = false;	// too many UIDs for an iLON
				}
#else
			    temp[5] = index;
#endif
			    pUid->set(temp);
            }
            else
            {
                *pUid = p->uniqueId;
            }
		}
		else
		{
			*pUid = m_pUidMap[index];
		}
	}
    unlockUid();
#elif defined(LIFTBR_PLATFORM)
	pUid->set(nids[index]);
#else
	pncLtUniqueIdList list;
	assert(index < list.getCount());
	if (index < list.getCount())
	{
		list.get(index, pUid);
	}
	else
	{
		vxlReportUrgent("LonTalk device count exceeds available LonTalk IDs (%d)\n", list.getCount());
		result = false;
	}
#endif
	return result;
}

void LtPlatform::reset()
{
	//LEDOFF;
}

#endif // FEATURE_INCLUDED(MULTI_APP)

void LtPlatform::setErrorLogPath(const char *errorLogPath)
{
#if FEATURE_INCLUDED(NVRAM) && !FEATURE_INCLUDED(INCLUDE_APP_INDEX_IN_PERSISTENCE_FILES)
    // Actually, this size is not quite right, since ERROR_LOG_KEY contains format chars
    // as well - but thats OK, its close, and its big enough
    delete m_szErrorLogName;
    m_szErrorLogName = new char[strlen(errorLogPath) + sizeof(ERROR_LOG_KEY) + 1];
    sprintf(m_szErrorLogName, ERROR_LOG_KEY, errorLogPath);
#endif
}

// These are almost source compatible
#ifdef WIN32
#define STAT _stat
#else
#define STAT stat
#endif

#if PERSISTENCE_TYPE_IS(STANDARD) || FEATURE_INCLUDED(IP852)
boolean LtIpDirExists(const char* szDir)
{
	boolean bExists = FALSE;
	char path[MAX_PATH];
	struct STAT info;
	int len;

	// Must strip off a trailing directory separator
	len = strlen(szDir) - 1;
	if ((len >= 0) && (len < (MAX_PATH - 1)) &&
		((szDir[len] == DIR_SEPARATOR_CHAR) || (szDir[len] == ALT_DIR_SEPARATOR_CHAR)))
	{
		strcpy(path, szDir);
		path[len] = 0;
		szDir = path;
	}

	if ((STAT(const_cast<char *>(szDir), &info) == 0) && (info.st_mode & S_IFDIR))
	{
	 	bExists = TRUE;
	}
	return(bExists);
}

boolean LtIpFileExists(const char* szFile)
{
	boolean bExists = FALSE;
	struct STAT info;

	if ((STAT(const_cast<char *>(szFile), &info) == 0) && !(info.st_mode & S_IFDIR))
	{
	 	bExists = TRUE;
	}
	return(bExists);
}

boolean LtIpFileSize(const char* szFile, ULONG* pSize)
{
	boolean bOk = FALSE;
	struct STAT info;

	if ((STAT(const_cast<char *>(szFile), &info) == 0))
	{
	 	 *pSize = info.st_size;
		 bOk = TRUE;
	}
	return(bOk);
}

#if FEATURE_INCLUDED(APP_CONTROL)
#if defined(linux)
void LinuxDeletePath(const char *path)
{
	struct dirent *d = NULL;
	DIR *dir = NULL; /* pointer to directory head*/
	char buf[64]={0}; /* buffer to store the complete file/dir name*/
	struct stat statbuf; /* to obtain the statistics of file/dir */
	int retval =0; /* to hold the return value*/

	memset(&statbuf,0,sizeof(struct stat));
	retval = stat(path,&statbuf);

	/* if the stat returned success and path provided is of valid directory*/
	if(S_ISDIR(statbuf.st_mode) && (retval==0))
	{
		dir = opendir(path); /* open the directory*/

		/* reads entry one by one*/
		while((d = readdir(dir)))
		{
			if((strcmp(d->d_name,".")!=0) && (strcmp(d->d_name,"..")!=0))
			{
				sprintf(buf,"%s/%s",path,d->d_name);
				retval = stat(buf,&statbuf);

				if(retval == 0)
				{
					if(!S_ISDIR(statbuf.st_mode))
					{
						/* This is file, remove the entry*/
						remove(buf);
					}
					else
					{
						/*
						  This is a directory, recursive search in it
						  once all files are removed, delete the directory
						*/
						LinuxDeletePath(buf);
						remove(buf);
					}
				}
			}
		}

		/* Now remove the head directory provided, as it empty now*/
		remove(path);
	}
//	return retval==0;
}
#endif

// This should implement the deletion of directory tree
void LtIpDeleteDir(const char* szDir)
{
#ifdef WIN32
	vxlReportUrgent("Directory deletion not implemented\n");
	/* This does not delete a tree
	_rmdir(szDir);
	*/
#elif defined(__VXWORKS__)
	if (fileIsDir((char *)szDir))
	{
		deleteDir((char *)szDir);
    }
#else
	LinuxDeletePath(szDir);
#endif
}
#endif

void LtIpDeleteFile(const char* szFile)
{
#ifdef WIN32
    DeleteFile(szFile);
#else
	remove(szFile);
#endif
}

// The vxworks implementation of rename() has bugs,
// and iLonSafeRename() attempts to overcome them.
boolean LtIpRenameFile(const char* from, const char* to)
{
	boolean bOk = false;
#if !defined(__VXWORKS__)
	bOk = (rename(from, to) == 0);
#else
	bOk = (iLonSafeRename(const_cast<char *>(from), const_cast<char *>(to)) == OK);
#endif
	return bOk;
}

// This can be used in conjunction with the "synched file" routines below to
// achive similar behavior on Windows and vxworks. The Windows versions of those
// routines call special Windows API functions to create a "write-through" file,
// so flushing the whole disk is not necessary (or desirable). Vxworks has no
// such feature, so flushing the whole disk is necessary.
void LtIpFlushDisk()
{
#if defined(WIN32)
	// Do nothing
#elif PRODUCT_IS(ILON)
	// flushing the whole disk is only necessary with VxWorks.
#if defined(__VXWORKS__)
	flushDisk();
#endif
#elif defined(unix)
	// Not required on DC implementation on Linux because the flush/fsync is
	// handled at a higher layer (DcxFile class).
#else
	assert(0);
#endif
}

// Functions to write to file that is flushed to the disk.
// See above comments about flushing the disk.
boolean LtIpOpenSynchedFile(const char* path, void **pFileID)
{
	boolean		bOk = false;

#ifdef WIN32
	HANDLE f = CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
	if (f != INVALID_HANDLE_VALUE)
#else
	FILE* f = fopen(path, "wb");
	if (f != NULL)
#endif
	{
		*pFileID = (void *)f;
		bOk = true;
	}
	return bOk;
}

boolean LtIpWriteToSynchedFile(void* fileID, const byte* pData, int nSize)
{
	boolean bOk = false;
#ifdef WIN32
	DWORD nWritten;
	if (WriteFile((HANDLE)fileID, pData, nSize, &nWritten, NULL))
	{
		bOk = true;
	}
#else
	if (fwrite(pData, nSize, 1, (FILE*)fileID) == 1)
	{
		fflush((FILE*)fileID);
		LtIpFlushDisk();
		bOk = true;
	}
#endif
	return bOk;
}

boolean LtIpCloseSynchedFile(void* fileID)
{
	boolean bOk;
#ifdef WIN32
	bOk = (CloseHandle((HANDLE)fileID) != 0);
#else
	bOk = (fsync(fileno((FILE*)fileID))==0) && (fclose((FILE*)fileID) == 0);
#endif
	return bOk;
}

boolean LtIpWriteSynchedFile(const char* path, const byte* pData, int nSize)
{
	boolean		bOk = false;
	void*		fileID;

	if (LtIpOpenSynchedFile(path, &fileID))
	{
		if (LtIpWriteToSynchedFile(fileID, pData, nSize))
		{
			bOk = LtIpCloseSynchedFile(fileID);
        }
    }
	return bOk;
}

#if defined(linux)
/*********************************************************************
 * copy f1name to f2name.  f1name must exist.
 * If bFailIfExists is TRUE, f2name must not exist.
 * If f2name already exists and are the same, no copy is done.
 * returns 1 on success, 0 on failure with error reason in gDcxFerrno
 *********************************************************************/
int LinuxCopyFile(const char *f1name, const char *f2name, int bFailIfExists)
{
	FILE *fp1, *fp2;
	size_t len;

	fp1 = fopen(f1name, "r");
	if (!fp1)
	{
		return 0; // failure
	}

	fp2 = fopen(f2name, "r");
	if (fp2) {
		if (bFailIfExists) {
			fclose(fp1);
			fclose(fp2);
			return 0; // failure
		}
		else {
			fclose(fp2);
			remove(f2name);	// delete it so that we can copy
			fseek(fp1, 0L, SEEK_SET);	// rewind file1
		}
	}

	fp2 = fopen(f2name, "w");
	if (!fp2) {
		fclose(fp1);
		return 0; // failure
	}

	while (1) {
		BYTE   buf[256];

		len = fread(buf, 1, sizeof(buf), fp1);
		if (len) {
			if (1 != fwrite(buf, len, 1, fp2)) {
				fclose(fp1);
				fclose(fp2);
				remove(f2name);
				return 0; // failure
			}
		}
		else {
			fclose(fp1);
			if (OK != fclose(fp2)) {
				return 0; // failure
			}
			break;
		}
	}

	return 1;	// success
}
#endif 

boolean LtIpCopyFile(const char* fromPath, const char* toPath)
{
	boolean		bOk = false;

	if (LtIpFileExists(fromPath))
	{
		boolean	bTargetExists = LtIpFileExists(toPath);
#ifdef WIN32
		BOOL bFailIfExists = FALSE;

		bOk = (CopyFile(fromPath, toPath, bFailIfExists) != 0);
#elif defined(__VXWORKS__)
		bOk = (copy(const_cast<char *>(fromPath), const_cast<char *>(toPath)) == OK);
#elif defined(linux)
		BOOL bFailIfExists = FALSE;
		bOk = LinuxCopyFile(fromPath, toPath, bFailIfExists);
#else
		assert(0);
#endif
		// Clean up a failed copy
		if (!bOk & !bTargetExists)
			remove(toPath);
    }
	return bOk;
}

#if defined(linux)
// do a mkdir with permission and error display
int CreateDirectory(const char *szPath, mode_t mode)
{
	int e = OK;

    if (access(szPath, F_OK))
	{
		e = mkdir(szPath, mode);

		if (e)
		{
			vxlReportUrgent("Unable to create directory %s\n", szPath);
			// not print the error for iLON since this call requires DC
			// same with the IP852 since this call requires SMIPLIB
#if not (defined(ILON_PLATFORM) || defined(LONTALK_IP852_STACK_PLATFORM) || defined(IZOT_IP852_PLATFORM) || defined(IZOT_PLATFORM) || defined(LIFTBR_PLATFORM))
            LogPError("mkdir failure. OS error %ld - %s\n",
                        errno,
                        strerror(errno));
#endif
		}
	}

	return e;	// 0 == OK
}

int fileExists(const char *name)
{
	struct stat s;

	if (stat(name, &s) == 0)
	{
		return TRUE;	// file exists
	}

	return FALSE;
}

// create a directory hierarchy, because mkdir() doesn't
int CreateDirectoryTree	(const char* szPath)
{
	int e = OK;

	if (!fileExists(szPath))
	{
		size_t len = strlen(szPath);
        vxlReportEvent("Creating directory %s\n", szPath);

		if (len)
		{
			if (szPath[len - 1] == '/')
			{
				len--;
			}
		}

		if (len)
		{
			char opath[MAX_PATH];

			if (len < sizeof(opath))
			{
				mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;

				memcpy(opath, szPath, len);
				opath[len] = '\0';

				for (char *p=opath; *p; p++)
				{
					if (*p == '/' && p != opath)
					{
						*p = '\0';
						e = CreateDirectory(opath, mode);
						if (e)
						{
							break;
						}
						*p = '/';
					}
				}

				e = CreateDirectory(opath, mode);
			}
			else
			{
				vxlReportUrgent("Directory path is too long\n");
				e = -2;
			}
		}
		else
		{
			vxlReportUrgent("Directory path is empty\n");
			e = -3;
		}
	}

	return e;	// 0 == OK
}

#endif
#endif // #if PERSISTENCE_TYPE_IS(STANDARD)

