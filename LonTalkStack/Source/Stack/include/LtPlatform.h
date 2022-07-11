#ifndef _LTPLATFORM_H
#define _LTPLATFORM_H

//
// LtPlatform.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtPlatform.h#4 $
//

#include <semLib.h>
#include <assert.h>
#include "LtUniqueId.h"

#ifndef boolean
typedef unsigned char boolean;
#endif
#ifndef byte
typedef unsigned char byte;
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef DIR_SEPARATOR_CHAR
// Directory separators
#ifdef WIN32
#define DIR_SEPARATOR_CHAR		'\\'
#define DIR_SEPARATOR_STRING	"\\"
#define ALT_DIR_SEPARATOR_CHAR	'/'
#define ALT_DIR_SEPARATOR_STRING "/"
#else
#define DIR_SEPARATOR_CHAR		'/'
#define DIR_SEPARATOR_STRING	"/"
#define ALT_DIR_SEPARATOR_CHAR	'\\'
#define ALT_DIR_SEPARATOR_STRING "\\"
#endif
#endif // DIR_SEPARATOR_CHAR

// Never define this here for the iLON target
#if defined(WIN32) || defined(linux) || !PRODUCT_IS(ILON)
  #ifndef FILEPATH_ROOT
    #ifdef linux
      #define FILEPATH_ROOT "/var/ilon/"
    #else
      #define FILEPATH_ROOT "/"
    #endif
  #endif  // ifndef FILEPATH_ROOT
#endif  // if defined(WIN32) || defined(linux) || !PRODUCT_IS(ILON)

void LtIpDeleteFile(const char* szFile);
boolean LtIpDirExists(const char* szDir);
boolean LtIpFileExists(const char* szFile);
boolean LtIpFileSize(const char* szFile, ULONG* pSize);
int LtIpMakeDir(const char* szDir);
void LtIpDeleteDir(const char* szDir);
boolean LtIpRenameFile(const char* from, const char* to);
boolean LtIpWriteSynchedFile(const char* path, const byte* pData, int nSize);
boolean LtIpOpenSynchedFile(const char* path, void **pFileID);
boolean LtIpWriteToSynchedFile(void* fileID, const byte* pData, int nSize);
boolean LtIpCloseSynchedFile(void* fileID);
boolean LtIpCopyFile(const char* fromPath, const char* toPath);
void LtIpFlushDisk();

int CreateDirectoryTree	(const char* szPath);

#define MAX_KEY_SIZE		100

#if FEATURE_INCLUDED(INCLUDE_APP_INDEX_IN_PERSISTENCE_FILES)
#define ERROR_LOG_KEY		"LTS_ERRLOG%d"
#else
#define ERROR_LOG_KEY       "%s" DIR_SEPARATOR_STRING "LTS_ERRLOG"
#endif

#ifdef __cplusplus
}
#endif

typedef struct LtKeyedUidMap LtKeyedUidMap;

class LtPlatform {
public:
    LtPlatform();
    ~LtPlatform();

	static int remainingDiskSpace();
	static boolean getPersistPath( char* pPathRtn, int nMaxLen );
	static void setPersistPath( const char* pPath );
    static void reset();
    void setErrorLogPath(const char *errorLogPath);

	byte getErrorLog(void);
	void setErrorLog(byte errorLog);

        // This should only be called on process exit
    static void ltShutdown(void);

#if FEATURE_INCLUDED(NVRAM)
private:
    char         *m_szErrorLogName;
	static const boolean m_errorLogPathInKey;
#endif

#if FEATURE_INCLUDED(MULTI_APP)
private:
	int			  imageIndex;
// EPANG TODO - simulate in iLON Linux for now
#if defined(WIN32) || (defined(ILON_PLATFORM) && defined(linux))
	static LtUniqueId*  m_pUidMap;
    static LtKeyedUidMap* m_pKeyedUidMap;
    static int m_keyedUidMapUseCount;

        // Lock for access to UID.  Only necessary on WIN32
    static SEM_ID    m_semUidLock; 
    static void lockUid() { semTake(m_semUidLock, WAIT_FOREVER); }
    static void unlockUid() { semGive(m_semUidLock); }

#if PRODUCT_IS(ILON)
	static int m_iLonSimIndex;
#endif
#endif

public:
	LtErrorType setIndex(int index);

    boolean getUniqueId(LtUniqueId* pUid);
// EPANG TODO - simulate in iLON Linux for now
#if defined(WIN32) || (defined(ILON_PLATFORM) && defined(linux))
	static void setUniqueIdMap(LtUniqueId* pMap);

    static void setUniqueId(LtUniqueId &uniqueId, int imageIndex);
    static void clearUniqueId(int imageIndex);
#if PRODUCT_IS(ILON)
	static void setILonSimIndex(int simIndex);
#endif
#endif
#ifdef LIFTBR_PLATFORM
	static void setUniqueId(LtUniqueId *uniqueId, int imageIndex);
#endif
private:
// EPANG TODO - simulate in iLON Linux for now
#if defined(WIN32) || (defined(ILON_PLATFORM) && defined(linux))
    static LtKeyedUidMap *findKeyedUniqueId(int imageIndex);
#endif

#else // FEATURE_INCLUDED(MULTI_APP)

public:
    static boolean getUniqueId(LtUniqueId* pUid);
    static void setUniqueId(LtUniqueId &uid);
    static void generateUniqueId(LtUniqueId* pUid);
	static boolean getIsFirstRun() { return m_IsFirstRun; }
private:
    static LtUniqueId m_uniqueId;
	static boolean m_IsFirstRun;
#endif
};

#endif
