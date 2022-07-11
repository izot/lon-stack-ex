/*
 * File: FtxlNvdUserDefined.c
 * $Revision: #2 $
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
 * This file contains a skeleton implementation of the IzoT non-volatile data (NVD) 
 * functions. The LonTalk Interface Developer enables the functions in this 
 * file by defining the macro LON_NVD_MODEL_USER_DEFINED to 1 in FtxlDev.h. 
 *
 * See also FtxlNvdFlashDirect.c for an alternate implementation using 
 * using Altera flash access routines directly, and FtxlNvdFlashFs.c for 
 * an alternate implementation using a flash file system.
 * 
 * You must implement each of the functions defined in this file.
 *
 */

#include "FtxlDev.h"

/* 
 * FtxlDev.h defines the boolean LON_NVD_MODEL_USER_DEFINED. Include this code 
 * only if LON_NVD_MODEL_USER_DEFINED is TRUE.
 */
#if LON_NVD_MODEL_USER_DEFINED

#include "FtxlApi.h"
#include "Osal.h"

/*
 * ******************************************************************************
 * SECTION: NVD CALLBACK PROTOTYPES
 * ******************************************************************************
 *
 *  This section contains a skeleton implementation the IzoT Device Stack API callback 
 *  functions supporting memory for Non-Volatile Data (NVD). 
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
    /* You must either implement this function or select a different 
     * non-volatile driver model in the LonTalk Interface Developer.
     */
    #error "TBD: Implement LonNvdOpenForRead"
    return NULL;
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
    /* You must either implement this function or select a different 
     * non-volatile driver model in the LonTalk Interface Developer.
     */
    #error "TBD: Implement LonNvdOpenForWrite"
    return NULL;
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
    /* You must either implement this function or select a different 
     * non-volatile driver model in the LonTalk Interface Developer.
     */
    #error "TBD: Implement LonNvdClose"
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
 *  It is not necessary for this function to actually destroy the data or free 
 *  it.
 */
void LonNvdDelete(const LonNvdSegmentType type)
{
    /* You must either implement this function or select a different 
     * non-volatile driver model in the LonTalk Interface Developer.
     * Note that in some implementations this function may do nothing.
     */
    #error "TBD: Implement LonNvdDelete"
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
    /* You must either implement this function or select a different 
     * non-volatile driver model in the LonTalk Interface Developer.
     */
    #error "TBD: Implement LonNvdRead"
    return LonApiNvdFileError;
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
    /* You must either implement this function or select a different 
     * non-volatile driver model in the LonTalk Interface Developer.
     */
    #error "TBD: Implement LonNvdWrite"
    return LonApiNvdFileError;
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
    /* You must either implement this function or select a different 
     * non-volatile driver model in the LonTalk Interface Developer.
     */
    #error "TBD: Implement LonNvdIsInTransaction"
    return 1;
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
    /* You must either implement this function or select a different 
     * non-volatile driver model in the LonTalk Interface Developer.
     */
    #error "TBD: Implement LonNvdEnterTransaction"
    return LonApiNvdFileError;
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
    /* You must either implement this function or select a different 
     * non-volatile driver model in the LonTalk Interface Developer.
     */
    #error "TBD: Implement LonNvdExitTransaction"
    return LonApiNvdFileError;
}

#endif /* LON_NVD_MODEL_USER_DEFINED */
