/*
 * File: FtxlDefNvdFlashFs.c
 * $Revision: #3 $
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
 * This file contains a default implementation of the IzoT non-volatile 
 * data (NVD) functions using the standard C runtime file system access 
 * functions. 
 * 
 */
#include <stdio.h>
#include <string.h>
#include "FtxlApiInternal.h"
#include "FtxlTypes.h"
#include "LtPlatform.h"
#include "Osal.h"


#ifdef WIN32
#include <direct.h>  
#include <memory.h>
#else
#include <sys/stat.h>
extern "C" int CopyFile(const char *f1name, const char *f2name, int bFailIfExists);
extern int CreateDirectoryTree	(const char* szPath);
#define makeDir CreateDirectoryTree
#endif


#if PRODUCT_IS(IZOT)

#ifndef MAX_PATH
#define MAX_PATH 260
#endif


/*
 * ******************************************************************************
 * SECTION: Control Structures and definitions
 * ******************************************************************************
 *
 * This section contains macros and structures used to control the flash 
 * access routines.
 *
 */

/*  
 *  Macro: LonNvdHandleToFs
 *  Translate a <LonNvdHandle> to a file system pointer.  
 */
#define LonNvdHandleToFs(handle) ((FILE *)(handle))

/*  
 *  Macro: FsToLonNvdHandle
 *  Translate a file system pointer to a <LonNvdHandle>.  
 */
#define FsToLonNvdHandle(file) ((LonNvdHandle)(file))

/*
 * ******************************************************************************
 * SECTION: Variables
 * ******************************************************************************
 *
 */

/*
 * ******************************************************************************
 * SECTION: Forward References
 * ******************************************************************************
 */

    /* Construct the full path name of an NVD file  */
static void GetNvdDataFilePath(LonNvdSegmentType type, LonBool tx, char *buf, int bufSize);

     /* Translate a segment type to a name for NVD tracing. */
static const char *GetNvdName(LonNvdSegmentType type);

/*
 * ******************************************************************************
 * SECTION: Support functions
 * ******************************************************************************
 *
 * This section contains the support functions used to support the NVD functions
 * defined in this file.
 */

/* 
 *  Function: GetNvdDataFilePath
 *  Construct the full path name of an NVD file based on the given path, <LonNvdSegmentType> 
 *  and whether it is a data file or transaction file.  
 */
static void GetNvdDataFilePath(LonNvdSegmentType type, LonBool tx, char *buf, int bufSize)
{
    char nvdFsPath[MAX_PATH];

    LonGetNvdFsPath(nvdFsPath, sizeof(nvdFsPath));
    if (nvdFsPath[strlen(nvdFsPath)-1] != DIR_SEPARATOR_CHAR)
        strcat(nvdFsPath, DIR_SEPARATOR_STRING);

    // Always make sure the directory exists - in case someone deleted it!
    #ifdef WIN32
	    _mkdir(nvdFsPath);
    #else
	    makeDir(nvdFsPath);
    #endif

    sprintf(buf, "%s%s.%s", nvdFsPath, GetNvdName(type), tx ? "tx" : "dat");
}

 
/* 
 *  Function: GetNvdName
 *  Translate a segment type to a name, used for NVD tracing.
 *
 *  Parameters:
 *  type -  type of non-volatile data to be opened
 *
 *  Returns:
 *  character string representing the NVD segment.  
 *
 */
static const char *GetNvdName(LonNvdSegmentType type)
{
    const char *name;
    switch (type)
    {
    case LonNvdSegNetworkImage:
        name = "LonNvdSegNetworkImage";
        break;
    case LonNvdSegNodeDefinition:
        name = "LonNvdSegNodeDefinition";
        break;
    case LonNvdSegApplicationData:
        name = "LonNvdSegApplicationData";
        break;
    case LonNvdSegUniqueId:
        name = "LonNvdSegUniqueId";
        break;
    case IsiNvdSegConnectionTable:
        name = "IsiNvdSegConnectionTable";
        break;
    case IsiNvdSegPersistent:
        name = "IsiNvdSegPersistent";
        break;
    default:
        name = "????";
    }
    return name;
}


/******************************************************************************
 * Event Handler Default Implementation Functions for NVD
 ******************************************************************************/

/* 
 *  This function is called by the IzoT LonNvdOpenForRead event handler
 *  to open a non-volatile data segment for reading.
 *
 *  Parameters:
 *  type -  type of non-volatile data to be opened
 *
 *  Returns:
 *  <LonNvdHandle>.   
 *
 *  Remarks:
 *  This function opens the data segment for reading.  If the file exists and 
 *  can be opened, this function returns a valid application-specific 32-bit 
 *  value as the handle.  Otherwise it returns 0.  The handle returned by this 
 *  function will be used as the first parameter when calling <LonNvdRead>.  
 *  The application must maintain the handle used for each segment. The 
 *  application can invalidate a handle when <LonNvdClose> is called for that 
 *  handle.  
 */
const LonNvdHandle DefaultNvdOpenForRead(const LonNvdSegmentType type)
{
    FILE *fp;
    char path[MAX_PATH];

    APIDebug("Start DefaultNvdOpenForRead(%s)\n", GetNvdName(type));
    
    /* Get the data file's full path name */
    GetNvdDataFilePath(type, FALSE, path, sizeof(path));

    /* Open for read access. */
    fp = fopen(path, "rb");

    APIDebug("End DefaultNvdOpenForRead(%p)\n", fp);  

    return FsToLonNvdHandle(fp);
}

/* 
 *  This function is called by the IzoT LonNvdOpenForWrite event handler
 *  to open a non-volatile data segment for writing.
 *
 *  Parameters:
 *  type - type of non-volatile data to be opened
 *  size - size of the data to be stored
 *
 *  Returns:
 *  <LonNvdHandle>.   
 *
 *  Remarks:
 *  This function is called by the IzoT protocol stack after changes 
 *  have been made to data that is backed by persistent storage.  Note that 
 *  many updates might have been made, and this function is called only after 
 *  the stack determines that all updates have been completed, based on a flush 
 *  timeout defined by the LonTalk Interface Developer. 
 *
 *  An error value is returned if the data cannot be written.
 */
const LonNvdHandle DefaultNvdOpenForWrite(const LonNvdSegmentType type, const size_t size)
{
    FILE *fp;
    char path[MAX_PATH];

    APIDebug("Start DefaultNvdOpenForWrite(%s, %d)\n",  GetNvdName(type), size);

    /* Get the data file's full path name */
    GetNvdDataFilePath(type, FALSE, path, sizeof(path));

    /* Open for write access. */
    fp = fopen(path, "wb");

    APIDebug("End DefaultNvdOpenForWrite(%s, %d), fp = %p\n",
               GetNvdName(type), size, fp);  

    return FsToLonNvdHandle(fp);
}

/* 
 *  This function is called by the IzoT LonNvdClose event handler
 *  to close a non-volatile data segment.
 *
 *  Parameters:
 *  handle - handle of the non-volatile segment returned by <LonNvdOpenForRead> 
 *           or <LonNvdOpenForWrite>
 *
 *  Remarks:
 *  This function closes the non-volatile memory segment associated with this 
 *  handle and invalidates the handle. 
 */
void DefaultNvdClose(const LonNvdHandle handle)
{
    FILE *fp = LonNvdHandleToFs(handle);

    APIDebug("Start DefaultNvdClose(%p)\n",  handle);

    if (fp != NULL)
    {
        fclose(fp);
    }
    APIDebug("End DefaultNvdClose\n");
}

/* 
 *  This function is called by the IzoT LonNvdDelete event handler
 *  to delete non-volatile data segment.
 *
 *  Parameters:
 *  type - type of non-volatile data to be deleted
 *
 *  Remarks:
 *  This function is used to delete the non-volatile memory segment referenced 
 *  by the data type.  The IzoT Device Stack API attempts to close the file before 
 *  deleting it.  
 *
 *  Note that this function can be called even if the segment does not exist.  
 *  It is not necessary for this function to actually destroy the data or free it.
 */ 
void DefaultNvdDelete(const LonNvdSegmentType type)
{
    char path[MAX_PATH];

    APIDebug("DefaultNvdDelete(%s)\n", GetNvdName(type));

    /* Get the data file's full path name (.DAT) */
    GetNvdDataFilePath(type, FALSE, path, sizeof(path));
    LtIpDeleteFile(path);
    /* Get the tx file's full path name (.TX) */
    GetNvdDataFilePath(type, TRUE, path, sizeof(path));
    LtIpDeleteFile(path);
}

/* 
 *  This function is called by the IzoT LonNvdRead event handler
 *  to read a section of a non-volatile data segment.
 *
 *  Parameters:
 *  handle - handle of the non-volatile segment returned by <LonNvdOpenForRead>
 *  offset - offset within the segment
 *  size - size of the data to be read
 *  pBuffer - pointer to buffer to store the data
 *
 *  Remarks:
 *  This function is called by the IzoT Device Stack API during initialization to 
 *  read data from persistent storage. An error value is returned if the 
 *  specified handle does not exist.    
 *
 *  Note that the offset will always be 0 on the first call after opening
 *  the segment. The offset in each subsequent call will be incremented by
 *  the size of the previous call.
 */
const LonApiError DefaultNvdRead(const LonNvdHandle handle, 
					         const size_t offset, 
					         const size_t size, 
					         void * const pBuffer) 
{
    LonApiError sts = LonApiNoError;
    FILE *fp = LonNvdHandleToFs(handle);

    APIDebug("Start DefaultNvdRead(%p, %d, %d)\n",
               handle, offset, size);  

    if (fp != NULL)
    {
        /* File is open, seek to the specified offset. */
        if (fseek(fp, offset, SEEK_SET) == 0)
        {
            /* Read the requested size. */
            if (fread(pBuffer, size, 1, fp) == 1)
            {
                sts = LonApiNoError;
            }
        }
    }

    APIDebug("End DefaultNvdRead(%p, %ld, %ld), sts = %d\n",
               handle, offset, size, sts);  

    return sts;
}

/* 
 *  This function is called by the IzoT LonNvdWrite event handler
 *  to write a section of a non-volatile data segment.
 *
 *  Parameters:
 *  handle - handle of the non-volatile segment returned by <LonNvdOpenForWrite>
 *  offset - offset within the segment
 *  size - size of the data to be read
 *  pData - pointer to the data to write into the segment
 *
 *  Remarks:
 *  This function is called by the IzoT Device Stack API after changes have been made 
 *  to data that is backed by persistent storage.  Note that many updates 
 *  might have been made, and this function is called only after the stack 
 *  determines that all updates have been completed.   
 *
 *  Note that the offset will always be 0 on the first call after opening
 *  the segment. The offset in each subsequent call will be incremented by
 *  the size of the previous call.
 */
const LonApiError DefaultNvdWrite (const LonNvdHandle handle, 
                               const size_t offset, 
                               const size_t size, 
                               const void* const pData) 
{
    LonApiError sts = LonApiNoError;
    FILE *fp = LonNvdHandleToFs(handle);

    APIDebug("Start DefaultNvdWrite(%p, %d, %d)\n",
               handle, offset, size);  

     if (fp != NULL)
    {
        /* File is open, seek to the specified offset. */
        if (fseek(fp, offset, SEEK_SET) == 0)
        {
            /* Write the data. */
            if (fwrite(pData, size, 1, fp) == 1)
            {
                sts = LonApiNoError;
            }
        }
    }

    APIDebug("End DefaultNvdWrite(%p, %d, %d), sts = %d\n",
               handle, offset, size, sts);  

	return sts;
}

/* 
 *  This function is called by the IzoT LonNvdIsInTransaction event handler
 *  to find out if an NVD transaction was in progress last time the device 
 *  shut down.
 *
 *  Parameters:
 *  type - non-volatile segment type
 *
 *  Remarks:
 *  This function is called by the IzoT Device Stack API during initialization 
 *  prior to reading the segment data. This callback must return TRUE if an NVD 
 *  transaction had been started and never committed.  If this function returns 
 *  TRUE, the IzoT Device Stack API will discard the segment, otherwise, the IzoT 
 *  LonTalk API will attempt to read the persistent data. 
 */
const LonBool DefaultNvdIsInTransaction(const LonNvdSegmentType type)
{
    /* inTransaction is set to TRUE.  Any error reading the transaction record 
     * will be interpreted as being in a transaction - that is, the data 
     * segment is invalid.
     */
    LonBool inTransaction = 1;
    char path[MAX_PATH];
    FILE *fp;

    APIDebug("Start DefaultNvdIsInTransaction(%s)\n",  GetNvdName(type));

    /* Get the transaction file's full path name */
    GetNvdDataFilePath(type, TRUE, path, sizeof(path));

    /* Open the file for read access. */
    fp = fopen(path, "rb");
    if (fp != NULL)
    {
        /* Read the transaction record. */
        if (fread(&inTransaction, sizeof(inTransaction), 1, fp) != 1)
        {
            /* Failed to read the entire record.  Indicate that we must be in a 
             * transaction. 
             */
            inTransaction = TRUE;
        }
        fclose(fp);
    }

    APIDebug("End DefaultNvdIsInTransaction(%s), inTransaction = %d\n",
               GetNvdName(type), inTransaction);  

	return(inTransaction);
}

/* 
 *  This function is called by the IzoT LonNvdEnterTransaction event handler
 *  to initiate a non-volatile transaction.
 *
 *  Parameters:
 *  type - non-volatile segment type
 *
 *  Remarks:
 *  This function is called by the IzoT Device Stack API when the first request 
 *  arrives to update data stored in the specified segment.  The API updates 
 *  the non-persistent image, and schedules writes to update the non-volatile 
 *  storage at a later time.  
 */
const LonApiError DefaultNvdEnterTransaction(const LonNvdSegmentType type)
{
    LonApiError sts = LonApiNvdFileError;
    char path[MAX_PATH];
    FILE *fp;

    APIDebug("Start DefaultNvdEnterTransaction(%s)\n", GetNvdName(type));

    /* Get the transaction file's full path name */
    GetNvdDataFilePath(type, TRUE, path, sizeof(path));

    /* Open the file for write access. */
    fp = fopen(path, "wb");
    if (fp != NULL)
    {
        LonBool inTransaction = TRUE;
        /* Write the transaction record. */
        if (fwrite(&inTransaction, sizeof(inTransaction), 1, fp) == 1)
        {
            /* Succesfully wrote the transaction record. */
            sts = LonApiNoError;
        }
        fclose(fp);
    }

    APIDebug("End DefaultNvdEnterTransaction(%s), sts = %d\n",
               GetNvdName(type), sts);  
    return(sts);
}

/* 
 *  This function is called by the IzoT LonNvdExitTransaction event handler
 *  to complete a non-volatile transaction.
 *
 *  Parameters:
 *  type - non-volatile segment type
 *
 *  Remarks:
 *  This function is called by the IzoT Device Stack API after <LonNvdWrite> has 
 *  returned success and there are no further updates required.   
 */
const LonApiError DefaultNvdExitTransaction(const LonNvdSegmentType type)
{
    LonApiError sts = LonApiNvdFileError;
    char path[MAX_PATH];
    FILE *fp;
    
    APIDebug("Start DefaultNvdExitTransaction(%s)\n",  GetNvdName(type));

    /* Get the transaction file's full path name */
    GetNvdDataFilePath(type, TRUE, path, sizeof(path));

    /* Open the file for write access. */
    fp = fopen(path, "wb");
    if (fp != NULL)
    {
        LonBool inTransaction = FALSE;
        /* Write the transaction record. */
        if (fwrite(&inTransaction, sizeof(inTransaction), 1, fp) == 1)
        {
            /* Succesfully wrote the transaction record. */
            sts = LonApiNoError;
        }
        fclose(fp);
    }

    APIDebug("End DefaultNvdExitTransaction(%d), sts = %d\n",  type, sts);

    return(sts);
}

#endif


