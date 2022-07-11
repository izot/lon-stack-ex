/*
 * File: FtxlNvdFlashFs.c
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
 * This file contains an example implementation of the IzoT non-volatile 
 * data (NVD) functions using the standard C runtime file system access 
 * functions. The LonTalk Interface Developer enables the functions in this 
 * file by defining the macro LON_NVD_MODEL_FILE_SYSTEM to 1 in FtxlDev.h. 
 * See also FtxlNvdFlashDirect.c for an alternate implementation using 
 * using Altera flash access routines directly.
 * 
 * You might need to port this file to support your hardware and 
 * application requirements.
 *
 */

#include "FtxlDev.h"

/* 
 * FtxlDev.h defines the boolean LON_NVD_MODEL_FILE_SYSTEM. Include this code 
 * only if LON_NVD_MODEL_FILE_SYSTEM is TRUE.
 */
#if LON_NVD_MODEL_FILE_SYSTEM

#include "FtxlApi.h"
#include "Osal.h"
#include <stdio.h>

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

/* Debug trace flag. */
LonBool nvdTraceEnabled = 0;

/*
 * ******************************************************************************
 * SECTION: Forward References
 * ******************************************************************************
 */

    /* Construct the full path name of an NVD file  */
static void GetNvdPath(LonNvdSegmentType type, LonBool tx, char *buf, int bufSize);

    /* Print a time stamp for NVD tracing. */
static void PrintTimeStamp();

    /* Translate a segment type to a name for NVD tracing. */
static const char *GetNvdName(LonNvdSegmentType type);

/******************************************************************************
 * Event Handler Default Implementation Functions
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
FTXL_EXTERNAL_FN const LonNvdHandle DefNvdOpenForRead(const LonNvdSegmentType type)
{
    FILE *fp;
    char path[100];

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("DefNvdOpenForRead(%s)\n", GetNvdName(type));  
    }
    
    /* Get the data file's full path name */
    GetNvdPath(type, FALSE, path, sizeof(path));

    /* Open for read access. */
    fp = fopen(path, "rb");

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("End DefNvdOpenForRead(%s), fp = %p\n",  
               GetNvdName(type), fp);  
    }

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
const LonNvdHandle DefNvdOpenForWrite(const LonNvdSegmentType type, 
                                      const size_t size)
{
    FILE *fp;
    char path[100];

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("Start DefNvdOpenForWrite(%s, %d)\n",  GetNvdName(type), size);
    }

    /* Get the data file's full path name */
    GetNvdPath(type, FALSE, path, sizeof(path));

    /* Open for write access. */
    fp = fopen(path, "wb");

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("End DefNvdOpenForWrite(%s, %d), fp = %p\n",
               GetNvdName(type), size, fp);  
    }

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
void DefNvdClose(const LonNvdHandle handle)
{
    FILE *fp = LonNvdHandleToFs(handle);

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("DefNvdClose(%p)\n",  handle);
    }

    if (fp != NULL)
    {
        fclose(fp);
    }
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
void DefNvdDelete(const LonNvdSegmentType type)
{
     char path[100];

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("DefNvdDelete(%s)\n", GetNvdName(type));  
    }

    /* Get the data file's full path name */
    GetNvdPath(type, FALSE, path, sizeof(path));

    /* The function unlink() is not supported by Nios NewLibC.  If a version of
     * unlink() is defined for your file system, you can uncomment the call
     * below.  Otherwise the file will remain allocated, but will not be
     * used by device.
     */

    /* unlink(path); */
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
const LonApiError DefNvdRead(const LonNvdHandle handle, 
					         const size_t offset, 
					         const size_t size, 
					         void * const pBuffer) 
{
    LonApiError sts = LonApiNoError;
    FILE *fp = LonNvdHandleToFs(handle);

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("Start DefNvdRead(%p, %d, %d)\n",
               handle, offset, size);  
    }

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

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("End DefNvdRead(%p, %d, %d), sts = %d\n",
               handle, offset, size, sts);  
    }

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
const LonApiError DefNvdWrite (const LonNvdHandle handle, 
                               const size_t offset, 
                               const size_t size, 
                               const void* const pData) 
{
    LonApiError sts = LonApiNoError;
    FILE *fp = LonNvdHandleToFs(handle);

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("Start DefNvdWrite(%p, %d, %d)\n",
               handle, offset, size);  
    }

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

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("End DefNvdWrite(%p, %d, %d), sts = %d\n",
               handle, offset, size, sts);  
    }

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
const LonBool DefNvdIsInTransaction(const LonNvdSegmentType type)
{
    /* inTransaction is set to TRUE.  Any error reading the transaction record 
     * will be interpreted as being in a transaction - that is, the data 
     * segment is invalid.
     */
    LonBool inTransaction = 1;
    char path[100];
    FILE *fp;

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("Start DefNvdIsInTransaction(%s)\n",  GetNvdName(type)); 
    }

    /* Get the transaction file's full path name */
    GetNvdPath(type, TRUE, path, sizeof(path));

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

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("End DefNvdIsInTransaction(%s), inTransaction = %d\n",  
               GetNvdName(type), inTransaction);  
    }
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
const LonApiError DefNvdEnterTransaction(const LonNvdSegmentType type)
{
    LonApiError sts = LonApiNvdFileError;
    char path[100];
    FILE *fp;

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("Start DefNvdEnterTransaction(%s)\n", GetNvdName(type));  
    }

    /* Get the transaction file's full path name */
    GetNvdPath(type, TRUE, path, sizeof(path));

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

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("End DefNvdEnterTransaction(%s), sts = %d\n",  
               GetNvdName(type), sts);  
    }
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
const LonApiError DefNvdExitTransaction(const LonNvdSegmentType type)
{
    LonApiError sts = LonApiNvdFileError;
    char path[100];
    FILE *fp;
    
    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("Start DefNvdExitTransaction(%s)\n",  GetNvdName(type));
    }

    /* Get the transaction file's full path name */
    GetNvdPath(type, TRUE, path, sizeof(path));

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

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("End DefNvdExitTransaction(%d), sts = %d\n",  type, sts);  
    }

    return(sts);
}

#if !PRODUCT_IS(IZOT)
/*
 * ******************************************************************************
 * SECTION: NVD CALLBACK PROTOTYPES
 * ******************************************************************************
 *
 *  This section contains the IzoT Device Stack API callback functions supporting 
 *  memory for Non-Volatile Data (NVD). 
 *
 *  Remarks:
 *  Example implementations of these functions are provided in 
 *  FtxlNvdFlashDirect.c and FtxlNvdFlashFs.c.  The application developer may 
 *  need to modify these functions to fit the type of non-volatile storage 
 *  supported by the application.
 *
 *  Callback functions are called by the IzoT protocol stack 
 *  immediately, as needed, and may be called from any IzoT task.  The 
 *  application *must not* call into the IzoT Device Stack API from within a 
 *  callback.
 */

/* 
 *  Callback: LonNvdOpenForRead
 *  Open a non-volatile data segment for reading.
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
const LonNvdHandle LonNvdOpenForRead(const LonNvdSegmentType type)
{
    return DefNvdOpenForRead(type);
}

/* 
 *  Callback: LonNvdOpenForWrite
 *  Open a non-volatile data segment for writing.
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
const LonNvdHandle LonNvdOpenForWrite(const LonNvdSegmentType type, 
                                      const size_t size)
{
    return DefNvdOpenForWrite(type, size);
}

/* 
 *  Callback: LonNvdClose
 *  Close a non-volatile data segment.
 *
 *  Parameters:
 *  handle - handle of the non-volatile segment returned by <LonNvdOpenForRead> 
 *           or <LonNvdOpenForWrite>
 *
 *  Remarks:
 *  This function closes the non-volatile memory segment associated with this 
 *  handle and invalidates the handle. 
 */
void LonNvdClose(const LonNvdHandle handle)
{
    DefNvdClose(handle);
}

/* 
 *  Callback: LonNvdDelete
 *  Delete non-volatile data segment.
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
void LonNvdDelete(const LonNvdSegmentType type)
{
    DefNvdDelete(type);
}

/* 
 *  Callback: LonNvdRead
 *  Read a section of a non-volatile data segment.
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
const LonApiError LonNvdRead(const LonNvdHandle handle, 
					         const size_t offset, 
					         const size_t size, 
					         void * const pBuffer) 
{
    return DefNvdRead(handle, 
					  offset, 
					  size, 
					  pBuffer);
}

/* 
 *  Callback: LonNvdWrite
 *  Write a section of a non-volatile data segment.
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
const LonApiError LonNvdWrite (const LonNvdHandle handle, 
                               const size_t offset, 
                               const size_t size, 
                               const void* const pData) 
{
    return DefNvdWrite (handle, offset, size, pData); 
}

/* 
 *  Callback: LonNvdIsInTransaction
 *  Returns TRUE if an NVD transaction was in progress last time the device 
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
const LonBool LonNvdIsInTransaction(const LonNvdSegmentType type)
{
    return DefNvdIsInTransaction(type);
}

/* 
 *  Callback: LonNvdEnterTransaction
 *  Initiate a non-volatile transaction.
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
const LonApiError LonNvdEnterTransaction(const LonNvdSegmentType type)
{
    return DefNvdEnterTransaction(type);
}

/* 
 *  Callback: LonNvdExitTransaction
 *  Complete a non-volatile transaction.
 *
 *  Parameters:
 *  type - non-volatile segment type
 *
 *  Remarks:
 *  This function is called by the IzoT Device Stack API after <LonNvdWrite> has 
 *  returned success and there are no further updates required.   
 */
const LonApiError LonNvdExitTransaction(const LonNvdSegmentType type)
{
    return DefNvdExitTransaction(type);
}
#else
void RegisterMyNvdFlashFs(void)
{
    // register my NvdOpenForReadRegistrar handler
    LonNvdOpenForReadRegistrar(DefNvdOpenForRead);
    // register my vdOpenForWriteRegistrar handler
    LonNvdOpenForWriteRegistrar(DefNvdOpenForWrite);
    // register my NvdCloseRegistrar handler
    LonNvdCloseRegistrar(DefNvdClose);
    // register my NvdOpenForReadRegistrar handler
    LonNvdDeleteRegistrar(DefNvdDelete);
    // register my NvdRead handler
    LonNvdReadRegistrar(DefNvdRead);
    // register my NvdWrite handler
    LonNvdWriteRegistrar(DefNvdWrite);
    // register my NvdOpenForReadRegistrar handler
    LonNvdIsInTransactionRegistrar(DefNvdIsInTransaction);
    // register my NvdEnterTransaction handler
    LonNvdEnterTransactionRegistrar(DefNvdEnterTransaction);
    // register my NvdExitTransaction handler
    LonNvdExitTransactionRegistrar(DefNvdExitTransaction);
}

#endif /* PRODUCT_IS(IZOT) */

/*
 * ******************************************************************************
 * SECTION: Support functions
 * ******************************************************************************
 *
 * This section contains the support functions used to support the NVD functions
 * defined in this file.
 */

/* 
 *  Function: GetNvdPath
 *  Construct the full path name of an NVD file based on the <LonNvdSegmentType> 
 *  and whether it is a data file or transaction file.  
 *
 */
static void GetNvdPath(LonNvdSegmentType type, LonBool tx, char *buf, int bufSize)
{
    sprintf(buf, LON_NVD_ROOT_NAME "/%s.%s", GetNvdName(type), tx ? "tx" : "dat");
}


/* 
 *  Function: PrintTimeStamp
 *  Print a time stamp used for NVD tracing.
 */
static void PrintTimeStamp()
{
    int time = OsalGetTickCount()*1000/OsalGetTicksPerSecond();
    printf("[%d.%.3d]", time/1000, time % 1000);
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
    default:
        name = "????";
    }
    return name;
}

#endif /* LON_NVD_MODEL_FILE_SYSTEM */

