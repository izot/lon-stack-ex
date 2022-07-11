//
// LtNvRam.cpp
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
// This file contains the code which supports reading and writing
// to NVRAM on multiple platforms.
//
// Store non-volatile ram items
// These are written often, and are small items
// Key values must be unique across all users
// Someday, if multiple processes are used, then
// this class should add a qualifier to the ram dir for the process
// Nobody outside of this class needs to set the path since
// NV ram items are not saved or backed up beyond a reboot.
// NV ram items should be lost on change of machine identity or
// ever restored from a backup. Better to have none, than have stale values.
//

#ifdef WIN32
#include <windows.h>
#include <malloc.h>
#include <memory.h>
#include <direct.h>
#endif

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include <VxlTypes.h>
#include <LtNvRam.h>

#include "LonTalk.h"
#include "LtUniqueId.h"
#include "LtPlatform.h"
#include "LtFailSafeFile.h"

// It might be nice if there were a File System Abstraction Layer that the application used so that differences in file systems
// were contained in one file.  For now, we do this on an ad-hoc basis. 
extern int CreateDirectoryTree	(const char* szPath);

// Scratch pad area for path construction
char LtNvRam::m_szPath[MAX_PATH] = {0};

// There is one global NV RAM object.
// all members are static. So never instantiate this
// object. Use the class form to call all members
// LtNvRam::set(...)
//static LtNvRam ltNvRam;


void LtNvRam::setPath(char* pFile, const char* pKey, boolean pathInKey)
{
    if (pKey == NULL)
    {   // Some device types (LNS PA, for example), have no persistence.
        *pFile = 0;
    }
    else
    {
        if (pathInKey)
        {
            strcpy(pFile, pKey);
            #ifndef ILON_PLATFORM
                char *p = strrchr(pFile, DIR_SEPARATOR_CHAR);
                if (p != NULL)
                {
                    *p = 0;
                    if (!LtIpDirExists(pFile))
                    {
						#ifdef WIN32
							_mkdir(pFile);
						#elif defined(ILON_PLATFORM) // Vxworks only
							mkdir(pFile);	
						#else
							CreateDirectoryTree(pFile);
						#endif
                    }
                    *p = DIR_SEPARATOR_CHAR;
                }
            #endif
        }
        else
        {
	        // If we don't have the base path yet, make it.
	        if ( (strlen(m_szPath) == 0) || !LtIpDirExists(m_szPath) )
	        {
		        // get the persistence path from the platform class then
		        LtPlatform::getPersistPath( m_szPath, MAX_PATH );
        #ifndef ILON_PLATFORM
		        // Old comment: "we should get rid of this..." Why? Not sure but,
		        // it would make sense to have a file system abstraction layer that hid these details
		        // from code such as this.
		        if ( strlen(m_szPath) < ( sizeof(m_szPath) - (strlen(NVRAMDIR) + 2) ) )
		        {
			        strcat(m_szPath, DIR_SEPARATOR_STRING NVRAMDIR);
					#ifdef WIN32
						_mkdir(m_szPath);
					#elif defined(ILON_PLATFORM) // Vxworks only
						mkdir(m_szPath);	
					#else
						CreateDirectoryTree(m_szPath);
					#endif
			        strcat(m_szPath, DIR_SEPARATOR_STRING);
		        }
        #endif
	        }
	        sprintf(pFile, "%s%s", m_szPath, pKey);
        }
    }
}

//
// get
// return bytes actually read
//
int LtNvRam::get(const char* pKey, byte* pValue, int nSize, boolean pathInKey)
{
	char	path[MAX_PATH];
	int		nBytes = 0;
	LtFailSafeFile fsf;

    if (pKey != NULL)
    {
	    setPath(path, pKey, pathInKey);

	    nBytes = fsf.ReadSafeFile(path, pValue, nSize);
    }
	return nBytes;
}

//
// set
// return true if ok
//
boolean LtNvRam::set(const char* pKey, const byte* pValue, int nSize, boolean pathInKey)
{
	char		path[MAX_PATH];
	boolean		bOk = false;
	LtFailSafeFile fsf;

    if (pKey != NULL)
    {
	    setPath(path, pKey, pathInKey);

	    bOk = fsf.WriteSafeFile(path, pValue, nSize);
    }
	return bOk;
}


void LtNvRam::deletePersistence(const char *pKey, boolean pathInKey)
{
	char		path[MAX_PATH];

    if (pKey != NULL)
    {
	    setPath(path, pKey, pathInKey);
        LtIpDeleteFile(path);
    }
}
