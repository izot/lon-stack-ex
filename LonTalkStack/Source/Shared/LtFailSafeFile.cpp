/*
 * File: LtFailSafeFile.cpp
 *
 * Copyright © 2022 Dialog Semiconductor
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in 
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Description:
 * Read or write a persistent data file using a multi-step process to guarantee that
 * changes are atomic. This is to make sure the file remains consistent in the event 
 * of power-failures or other asynchronous resets in the middle of these operations. 
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <VxlTypes.h>
#include <semLib.h>
#include <sysLib.h>
#include "LtStackInternal.h"
#include "LtPlatform.h"
#include "LtFailSafeFile.h"

/*
Fail-Safe Algorithm

Assumptions:
 * The objective is simply to make the modification atomic, and never completely lose the data set. 
 * If both the old and the new data are available, either one may be safely used, but new is prefered.
 * Writing to a file can fail and leave the file corrupted or missing. 
   It cannot be trusted until some other detectable file system action is taken.
 * Renaming a file will not corrupt the contents, but may fail and leave the file missing 
   (actually, with an unknown name). A rename must not be done unless a good backup copy is available.
 * Flushing the disk actually works, or isn't needed (the data is committed after that point)

There are three variants of the file path that can be used to recover the data when reading.
The order of preference is:
  1. the normal path
  2. the new path ($new)
  3. the old path ($old)

There is also a temporary path ($temp) that is never recovered from. It is only used as a
location to write or copy when the first two paths don't exist (and thus none of the three main
paths can be safely written to).

The basic approach is to never write or copy data directly to a file that is the highest preference
order of existing files, and to use rename to 'move' the file after it has been written or copied. 
Also, never rename a file unless a safe backup copy (of lower preference) exists.

For files that are written using one of these methods, you must always call GetSafeFile() for that 
file at least once after a reboot before reading or writing that file, so that any temporary state can
be cleaned up. It may safely be called before any read or write of a file. ReadSafeFile() takes
care of this automatically.

This mechanism currently only protect files that are written completely, then closed. It does
not work for files that are appended to, or kept open for writing.

*/

const char * const LtFailSafeFile::m_newFileEnding = "$new";
const char * const LtFailSafeFile::m_oldFileEnding = "$old";
const char * const LtFailSafeFile::m_tempFileEnding = "$temp";
const int LtFailSafeFile::m_tempFileEndingLength = strlen(LtFailSafeFile::m_tempFileEnding);


// Global lock - not a member because of compilation issues with SEM_ID on the DC.
static SEM_ID m_gLock = NULL;

LtFailSafeFile::LtFailSafeFile()
{
	m_bTrace = false;
	if (m_gLock == NULL)
	{
		m_gLock = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
	}
}

LtFailSafeFile::~LtFailSafeFile()
{
}

void LtFailSafeFile::Shutdown()
{
	if (m_gLock != NULL)
	{
		semDelete(m_gLock);
		m_gLock = NULL;
	}
}

// Run the algorithm to find the safe copy of the file.
// All temp files will be gone by the end.
boolean LtFailSafeFile::GetSafeFile(const char* path)
{
	boolean bOk = false;

	char *newPath = (char*)malloc(strlen(path) + 10);
	char *oldPath = (char*)malloc(strlen(path) + 10);
	char *tempPath = (char*)malloc(strlen(path) + 10);

	if ((newPath != NULL) && (oldPath != NULL) && (tempPath != NULL) &&
		(m_gLock != NULL) && (semTake(m_gLock, WAIT_FOREVER) != ERROR))
	{
		// Create the various temp paths
		strcpy(newPath, path);
		strcat(newPath, m_newFileEnding);
		strcpy(oldPath, path);
		strcat(oldPath, m_oldFileEnding);
		strcpy(tempPath, path);
		strcat(tempPath, m_tempFileEnding);	

		// If the normal path exists, use it (first preference)
		if (LtIpFileExists(path))
		{
			if (m_bTrace)
				printf("Found specified file -- using it\n");
			bOk = true;
		}
		else if (LtIpFileExists(newPath))
		{
			// The new path exists, so use it (second preference). Back up
			// to the old path, the rename to the normal path
			if (m_bTrace)
			{
				printf("Specified file not found, NEW copy found -- using it\n");
				printf("Copying NEW to OLD\n");
			}
			if (LtIpCopyFile(newPath, oldPath))
			{
				LtIpFlushDisk();
				if (m_bTrace)
					printf("Renaming NEW to specified file\n");
				bOk = LtIpRenameFile(newPath, path);
				LtIpFlushDisk();
			}
		}
		else if (LtIpFileExists(oldPath))
		{
			// Only the old path exists (third preference). Copy to the
			// non-recoverable temp path, and then rename it, keeping the old path
			// as the backup copy.
			if (m_bTrace)
			{
				printf("Specified file not found, OLD copy found -- using it\n");
				printf("Copying NEW to TEMP\n");
			}
			if (LtIpCopyFile(oldPath, tempPath))
			{
				LtIpFlushDisk();
				if (m_bTrace)
					printf("Renaming TEMP to specified file\n");
				bOk = LtIpRenameFile(tempPath, path);
				LtIpFlushDisk();
			}
		}
		// cleanup any temp paths
		if (m_bTrace)
		{
			if (LtIpFileExists(newPath))
				printf("Removing NEW file\n");
			if (LtIpFileExists(oldPath))
				printf("Removing OLD file\n");
			if (LtIpFileExists(tempPath))
				printf("Removing TEMP file\n");
		}
		LtIpDeleteFile(newPath);
		LtIpDeleteFile(oldPath);
		LtIpDeleteFile(tempPath);
	
		semGive(m_gLock);
	}

	// Free everything we allocated. Allocation failures will be NULL
	free(newPath);
	free(oldPath);
	free(tempPath);

	return bOk;
}


// Find the safe copy of a file and read it
int LtFailSafeFile::ReadSafeFile(const char* path, byte* pData, int nSize)
{
	FILE*	fp;
	int		nBytes = 0;
	
	GetSafeFile(path);

	fp = fopen(path, "r+b");
	if (fp != NULL)
	{
		fread(pData, nSize, 1, fp);
		fclose(fp);
		nBytes = nSize;
	}
	return nBytes;
}

// Safely make a temp file the new official copy.
// The passed-in name must have the "$temp" ending.
boolean LtFailSafeFile::MakeTempFileOfficialCopy(char* tempPath)
{
	boolean		bOk = false;
	const char *pName;
	int tempPathLen = strlen(tempPath);
	int tempEndingLen = strlen(m_tempFileEnding);
	int basePathLen = tempPathLen - tempEndingLen;
	char *newPath = (char*)malloc(tempPathLen+1);
	char *oldPath = (char*)malloc(tempPathLen+1);
	char *basePath = (char*)malloc(basePathLen+1);

	if ((newPath != NULL) && (oldPath != NULL) && (tempPath != NULL) &&
		(m_gLock != NULL) && (semTake(m_gLock, 10*sysClkRateGet()) != ERROR))
	{
		if (tempPathLen > tempEndingLen)
		{
			pName = &tempPath[basePathLen];
			if (strcmp(pName, m_tempFileEnding) == 0)
			{
				// we have a valid "$temp" file
				// Create the various path names
				strncpy(basePath, tempPath, basePathLen);
				basePath[basePathLen] = 0;
				strcpy(newPath, basePath);
				strcat(newPath, m_newFileEnding);
				strcpy(oldPath, basePath);
				strcat(oldPath, m_oldFileEnding);
			
				if (LtIpFileExists(basePath))
				{
					// Just in case, delete any new path so we can rename to it.
					LtIpDeleteFile(newPath);
					if (LtIpRenameFile(tempPath, newPath))
					{
						// Just in case, delete any old path so we can rename to it.
						LtIpDeleteFile(oldPath);
						LtIpFlushDisk();
						if (LtIpRenameFile(basePath, oldPath))
						{
							LtIpFlushDisk();
							if (LtIpRenameFile(newPath, basePath))
							{
								LtIpFlushDisk();
								LtIpDeleteFile(oldPath);
								bOk = true;
							}
						}
					}
				}
				else
				{
					LtIpFlushDisk();
					bOk = LtIpRenameFile(tempPath, basePath);
					LtIpFlushDisk();
				}
				
				// Update the provided filename to match what's on flash
				strcpy(tempPath, basePath);
			}
		}
		semGive(m_gLock);
	}

	// Free everything we allocated. Allocation failures will be NULL
	free(newPath);
	free(oldPath);
	free(basePath);

	return bOk;
}

// Write to a safe file using a list of chunks of data ("parts")
// The list is terminated by an entry with a NULL data pointer.
boolean LtFailSafeFile::WriteSafeFile(const char* path, const FileParts* parts)
{
	boolean		bOk = false;

	char *newPath = (char*)malloc(strlen(path) + 10);
	char *oldPath = (char*)malloc(strlen(path) + 10);
	char *tempPath = (char*)malloc(strlen(path) + 10);
	
	if ((newPath != NULL) && (oldPath != NULL) && (tempPath != NULL) &&
		(m_gLock != NULL) && (semTake(m_gLock, 10*sysClkRateGet()) != ERROR))
	{
		// Create the various temp paths
		strcpy(newPath, path);
		strcat(newPath, m_newFileEnding);
		strcpy(oldPath, path);
		strcat(oldPath, m_oldFileEnding);
		strcpy(tempPath, path);
		strcat(tempPath, m_tempFileEnding);	

		// If the normal path exists, write to the new path, rename the normal to the old path
		// as the backup, then rename the new path to the normal path.
		if (LtIpFileExists(path))
		{
			if (WriteSafeFileParts(newPath, parts))
			{
				// Just in case, delete any old path so we can rename to it.
				LtIpDeleteFile(oldPath);
				LtIpFlushDisk();
				if (LtIpRenameFile(path, oldPath))
				{
					LtIpFlushDisk();
					if (LtIpRenameFile(newPath, path))
					{
						LtIpFlushDisk();
						LtIpDeleteFile(oldPath);
						bOk = true;
					}
				}
			}
		}
		else
		{
			// The normal path doesn't yet exist, so none of the recoverable paths can
			// be safely written to. We want to make sure we get a complete, consistent file, 
			// or nothing, so write to the non-recoverable temp path, then rename.
			if (WriteSafeFileParts(tempPath, parts))
			{
				LtIpFlushDisk();
				bOk = LtIpRenameFile(tempPath, path);
				LtIpFlushDisk();
			}
		}

		semGive(m_gLock);
	}

	// Free everything we allocated. Allocation failures will be NULL
	free(newPath);
	free(oldPath);
	free(tempPath);

	return bOk;
}

// Write a safe file in a single piece
boolean LtFailSafeFile::WriteSafeFile(const char* path, const byte* pData, int nSize)
{
	boolean bOk;
	FileParts parts[2];

	// Create a "parts" array, then use the method to write that way.
	parts[0].pData = pData;
	parts[0].nSize = nSize;
	parts[1].pData = NULL;
	parts[1].nSize = 0;

	bOk = WriteSafeFile(path, parts);

	return bOk;
}

// Internal function to write the file in pieces.
// This just makes sure all the writes are flushed to the disk.
// The list is terminated by an entry with a NULL data pointer.
// A zero size will not terminate it.
boolean LtFailSafeFile::WriteSafeFileParts(const char* path, const FileParts* parts)
{
	boolean bOk = false;
	void* fileID;
	int i;

	if (LtIpOpenSynchedFile(path, &fileID))
	{
		bOk = true;
		i = 0;
		while (bOk && (parts[i].pData != NULL))
		{
			bOk = LtIpWriteToSynchedFile(fileID, parts[i].pData, parts[i].nSize);
			i++;
		}
		if (!LtIpCloseSynchedFile(fileID))
		{
			bOk = false;
		}
	}
	return bOk;
}

const char *LtFailSafeFile::GetTempFileEnding()
{ 
	return m_tempFileEnding; 
}
