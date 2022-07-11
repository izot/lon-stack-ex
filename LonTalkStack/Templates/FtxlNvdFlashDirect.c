/*
 * File: FtxlNvdFlashDirect.c
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
 * This file contains an example implementation of the IzoT non-volatile 
 * data (NVD) functions using Altera flash access routines. The LonTalk 
 * Interface Developer enables the functions in this file by defining 
 * the macro LON_NVD_MODEL_FLASH_DIRECT to 1 in FtxlDev.h. 
 * See also FtxlNvdFlashFs.c for an alternate implementation using 
 * a flash file system.
 * 
 * You might need to port this file to support your hardware and 
 * application requirements.
 *
 */

#include "FtxlDev.h"

/* 
 * FtxlDev.h defines the boolean LON_NVD_MODEL_FLASH_DIRECT. Include this code 
 * only if LON_NVD_MODEL_FLASH_DIRECT is true.
 */
#if LON_NVD_MODEL_FLASH_DIRECT

#include <stdio.h>
#include <string.h>
#include "includes.h"
#include "FtxlApi.h"
#include "Osal.h"
#include "sys/alt_flash.h"

/*
 * ******************************************************************************
 * SECTION: Control Structures and definitions
 * ******************************************************************************
 *
 * This section contains macros and structures used to control the flash 
 * access routines.
 *
 * Segment data and the associated transaction bits are stored in flash.  
 * Each segment is defined by one or more contiguous flash blocks dedicated 
 * for that segment. Because no other data can appear in a segment's block, 
 * each segment can be updated independently.
 *
 * Each segment consists of an <NvdTransactionRecord> followed by the segment 
 * data. The transaction record consists of a signature and a transaction state.  
 * The transaction is considered "in progress", meaning that the associated 
 * data is invalid unless the signature is valid AND the txState flag value 
 * is TX_DATA_VALID. Starting a transaction is fast, because all that has to 
 * be done is update the txState to something other than TX_DATA_VALID.  Note 
 * that you can change bits in flash from 1 to 0 without erasing them first.  
 * Starting a transaction only requires invalidating the transaction record, 
 * and so can be done without a time consuming block erase.
 *
 * The location of each segment is calculated at runtime, and recorded in the 
 * segment map array.  
 */

/*  
 *  Macro: TypeToHandle
 *  Translate a <LonNvdSegmentType> to a <LonNvdHandle>.  Add one to the
 *  type because the handle value of 0 is reserved.
 */
#define TypeToHandle(type) ((LonNvdHandle)((type)+1))

/*  
 *  Macro: HandleToType
 *  Translate a <LonNvdHandle> to <LonNvdSegmentType>.  
 */
#define HandleToType(handle) \
    ((handle) == 0 ? LonNvdSegNumSegmentTypes : ((LonNvdSegmentType)(handle))-1)

/*  
 *  Macro: TX_SIGNATURE
 *  The transaction signature.
 *  
 *  A unique value to identify an initialized transaction record.
 */
#define TX_SIGNATURE 0x89ABCDEF

/*  
 *  Macro: TX_DATA_VALID
 *  Value of the transaction state when the associated data is valid.
 */
#define TX_DATA_VALID 0xffffffff

/*
 *  Typedef: NvdTransactionRecord
 *  A non-volatile transaction record.  
 *
 *  The data in a segment is considered valid if and only if 
 *
 *      (tx.signature == TX_SIGNATURE) AND (txState =- TX_DATA_VALID)
 */
typedef struct
{
    unsigned signature;
    unsigned txState;
} NvdTransactionRecord;

/*
 *  Typedef: SegmentMap
 *  Used to define a directory of segments.  
 *
 *  The SegmentMap structure is used to define the segmentMap array. The 
 *  segmentMap array is built at runtime and is used to map segments to 
 *  offsets within flash.  In order to ensure that modifying one segment does 
 *  not effect another, all segments start on flash block boundaries. The 
 *  first part of the segment is the transaction record, with the data portion 
 *  following immediately after.
 */
typedef struct 
{
    unsigned segmentStart;   /* Offset of the start of the segment within 
                              * flash memory 
                              */
    unsigned txOffset;       /* Offset of the transaction record within flash 
                              * memory.
                              */
    unsigned dataOffset;     /* Offset of the data within flash memory */  
    unsigned maxDataSize;    /* The maximum size reserved for the data. */
} SegmentMap;

/*
 * ******************************************************************************
 * SECTION: Variables
 * ******************************************************************************
 *
 */

/* Debug trace flag. */
LonBool nvdTraceEnabled = 0;

/*
 * The segmentMap array is built at runtime and is used to map segments to 
 * offsets within flash.  The segmentMap array is indexed by segment type.  
 */
static SegmentMap segmentMap[LonNvdSegNumSegmentTypes];

/* 
 * The flashBlockSize is the size, in bytes, of each block of flash. This 
 * value is determined at runtime by calling the HAL function 
 * alt_get_flash_info.
 */
static int flashBlockSize = 0;

/*
 * The lowestUsedFlashDataOffset variable is used to allocate contiguous blocks 
 * of flash to each segment. When the flash size is determined, the value is 
 * set to the end of flash.  Then, as the amount of flash to be reserved for 
 * each segment is determined, the offset is reduced accordingly.
 */
static int lowestUsedFlashDataOffset = 0;

/* 
 * Table indexed by <LonNvdSegmentType> and containing the maximum size of the 
 * data segment.  Computed at runtime by calling <LonNvdGetMaxSize>.
 */
static int dataSegmentSize[LonNvdSegNumSegmentTypes] =
{
    0,       /* LonNvdSegNetworkImage    */
    0,       /* LonNvdSegNodeDefinition  */
    0,       /* LonNvdSegApplicationData */
};

/*
 * ******************************************************************************
 * SECTION: Forward References
 * ******************************************************************************
 */

    /* Open the flash for access via the HAL. */
static LonApiError OpenFlash(const LonNvdSegmentType type, alt_flash_fd **pFd);

    /* Initialize the segmentMap array. */
static LonApiError InitSegmentMap();

    /* Erase a segment. */
static LonApiError EraseSegment(alt_flash_fd *fd, const LonNvdSegmentType type, 
                                int size);

    /* Print a time stamp for NVD tracing. */
static void PrintTimeStamp();

    /* Translate a segment type to a name for NVD tracing. */
static const char *GetNvdName(LonNvdSegmentType type);

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
    /* This function does nothing more than translate the segment type to a 
     * handle. The actual flash data is opened and closed on each access.
     */
    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("LonNvdOpenForRead(%s)\n", GetNvdName(type));  
    }
	return TypeToHandle(type);
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
    LonApiError sts = LonApiNoError;
    alt_flash_fd *fd;
    LonNvdHandle lonHandle = NULL;

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("Start LonNvdOpenForWrite(%s, %ld)\n",  GetNvdName(type), size);  
    }

    /* Open the flash so that we can erase it */
    sts = OpenFlash(type, &fd);
    if (sts == LonApiNoError)
    {
        if (size <= segmentMap[type].maxDataSize)
        {
            /* Erase enough space for both the TX record and all of the data. 
             * After the block has been erased, we can update individual bytes.
             * Note that this leaves the transaction record in the following
             * state:
             *   signature = 0xffffffff (invalid)
             *   txState   = 0xffffffff (TX_DATA_VALID)
             * Because the signature is invalid, a transaction is still 
             * in progress.
             */
            sts = EraseSegment(fd, type, size + sizeof(NvdTransactionRecord));
        }

        /* Close the flash.  It will be opened again when necessary. */
        alt_flash_close_dev(fd);
    }

    if (sts == LonApiNoError)
    {   
        /* Translate the type to a handle. */
        lonHandle = TypeToHandle(type);
    }

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("End LonNvdOpenForWrite(%s, %ld), sts = %d\n",  
               GetNvdName(type), size, sts);  
    }
    return lonHandle;
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
    /* The flash is opened and closed on each access - so there is nothing to 
     * do in this function. 
     */
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
    /* There is nothing to be gained by deleting the segment, because the space 
     * is reserved.  This function is just a stub.  
     */
    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("LonNvdDelete(%s)\n", GetNvdName(type));  
    }
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
    LonApiError sts = LonApiNoError;
    alt_flash_fd *fd;

    /* Find the type from the handle. */
    LonNvdSegmentType type = HandleToType(handle);

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("Start LonNvdRead(%s, %ld, %ld)\n",  
               GetNvdName(type), offset, size);  
    }

    /* Open the flash */
    sts = OpenFlash(type, &fd);
    if (sts == LonApiNoError)
    {
        /* Read the data using the segmentMap directory. */
        if (alt_read_flash(fd, segmentMap[type].dataOffset + offset, 
                           pBuffer, size) != 0)
	    {
		    sts = LonApiNvdFileError;
		}
        /* Close the flash. */
        alt_flash_close_dev(fd);
	}					

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("End LonNvdRead(%s, %ld, %ld), sts = %d\n",  
               GetNvdName(type), offset, size, sts);  
    }

    return sts;
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
 *  This function is called by the IzoT Device Stack API after changes have been 
 *  made to data that is backed by persistent storage.  Note that many updates 
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
    LonApiError sts = LonApiNoError;
    alt_flash_fd *fd;

    /* Find the type from the handle. */
    LonNvdSegmentType type = HandleToType(handle);

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("Start LonNvdWrite(%s, %ld, %ld)\n",  
               GetNvdName(type), offset, size);  
    }

    /* Open the flash */
    sts = OpenFlash(type, &fd);
    if (sts == LonApiNoError)
    {
            /* Calculate the starting offset within the flash. The flashOffset 
             * will be updated as each block of data is written.
             */
        int flashOffset = offset + segmentMap[type].dataOffset; 
            /* dataRemaining is the number of bytes left to write. */
        int dataRemaining = size;
            /* Get a pointer to the next data to be written. */
        unsigned char *pBuf = (unsigned char *)pData;

        /* Write a block of data at a time.  All of the data that needs to be 
         * written was erased by LonNvdOpenForWrite.
         */
        while (sts == LonApiNoError && dataRemaining)
        {
                // Offset within flash block
            int blockOffset = flashOffset % flashBlockSize; 
                // Offset of beginning of block
            int blockStart = flashOffset - blockOffset;     
                // Number of bytes in block starting at offset
            int dataInBlock = flashBlockSize - blockOffset;
                // Number of bytes to write this time
            int sizeToWrite;

            if (dataInBlock < dataRemaining)
            {
                /* Write the whole block */
                sizeToWrite = dataInBlock;
            }
            else
            {
                /* Write only the partial block remaining. */
                sizeToWrite = dataRemaining;
            }

            /* Update the current block. */
            if (alt_write_flash_block(fd, blockStart, flashOffset, 
                                      pBuf, sizeToWrite) != 0)
            {
                sts = LonApiNvdFileError;
            }
            else
            {
                /* Adjust offsets, data pointer and data remaining. */
                flashOffset += sizeToWrite;
                pBuf += sizeToWrite;
                dataRemaining -= sizeToWrite;
            }
        }

        /* Close the flash. */
        alt_flash_close_dev(fd);
	}					

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("End LonNvdWrite(%s, %ld, %ld), sts = %d\n",  
               GetNvdName(type), offset, size, sts);  
    }

	return sts;
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
    /* inTransaction is set to TRUE.  Any error reading the transaction record 
     * will be interpreted as being in a transaction - that is, the data 
     * segment is invalid.
     */
    LonBool inTransaction = 1;
    alt_flash_fd *fd;

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("Start LonNvdIsInTransaction(%s)\n",  GetNvdName(type)); 
    }

    /* Open the flash to read the transaction. */
    if (OpenFlash(type, &fd) == LonApiNoError)
    {
        NvdTransactionRecord txRecord;
        /* Read the transaction record.  Because the transaction record is 
         * always at the beginning of a block, we assume that it cannot span 
         * blocks.
         */
        if (alt_read_flash(fd, segmentMap[type].txOffset, 
                           &txRecord, sizeof(txRecord)) == 0)
        {
            /* Successfully read the transaction record, so maybe the data is 
             * valid after all - if the signature matches and the state is
             * TX_DATA_VALID, the data is valid, and therefore we are
             * not in a transaction.
             */
            inTransaction = !(txRecord.signature == TX_SIGNATURE && 
                             txRecord.txState == TX_DATA_VALID);
        }
        /* Close the flash. */
        alt_flash_close_dev(fd);
    }

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("End LonNvdIsInTransaction(%s), inTransaction = %d\n",  
               GetNvdName(type), inTransaction);  
    }
	return(inTransaction);
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
    LonApiError sts = LonApiNoError;
    alt_flash_fd *fd;

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("Start LonNvdEnterTransaction(%s)\n", GetNvdName(type));  
    }

    /* Open flash. */
    sts = OpenFlash(type, &fd);
    if (sts == LonApiNoError)
    {
        NvdTransactionRecord txRecord;
        txRecord.signature = TX_SIGNATURE;
        txRecord.txState = 0;  // No longer valid

        /* Clear the transaction record.  If the transaction record was valid 
         * before it looks like this:
         *   signature = TX_SIGNATURE (valid)
         *   txState   = 0xffffffff (TX_DATA_VALID)
         * In that case we can update it to the following, because we are only
         * changing 1 bits to 0 bits:
         *   signature = TX_SIGNATURE (valid)
         *   txState   = 0 (invalid)
         * After this is done, the block must be erased before the transaction
         * can be updated to valid again (this is done by LonNvdOpenForWrite).
         * If the transaction record was not valid in the first place, writing 
         * this pattern will not make it valid - and so the transaction is
         * still in progress because we consider that a transaction is in 
         * progress any time the transaction record is not valid.
         */
        if (alt_write_flash_block(fd, segmentMap[type].segmentStart,
                                  segmentMap[type].txOffset, 
                                  &txRecord, sizeof(txRecord)) != 0)
        {
            sts = LonApiNvdFileError;
        }

        /* Done, close the flash. */
        alt_flash_close_dev(fd);
    }

    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("End LonNvdEnterTransaction(%s), sts = %d\n",  
               GetNvdName(type), sts);  
    }
    return(sts);
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
    LonApiError sts = LonApiNoError;
    alt_flash_fd *fd;
    
    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("Start LonNvdExitTransaction(%s)\n",  GetNvdName(type));
    }

    /* Open the flash. */
    sts = OpenFlash(type, &fd);
    if (sts == LonApiNoError)
    {
        NvdTransactionRecord txRecord;
        txRecord.signature = TX_SIGNATURE;
        txRecord.txState = TX_DATA_VALID; 

        /* We expect that this should work because this function is only 
         * called after successfully updating the data segment.  First the IzoT 
         * LonTalk protocol stack calls LonNvdOpenForWrite, which will erase 
         * the entire segment, including the transaction record, then the data 
         * will be 
         * updated.  The transaction record is still in the erased state:
         *   signature = 0xffffffff (invalid)
         *   txState   = 0xffffffff (TX_DATA_VALID)
         * Because we can change 1 bits to 0 bits, this write will result in:
         *   signature = TX_SIGNATURE (valid)
         *   txState   = 0xffffffff (TX_DATA_VALID)
         * Later, if we want to start a transaction, we can update the txState
         * making it invalid again.  
         */
        if (alt_write_flash_block(fd, segmentMap[type].segmentStart,
                                  segmentMap[type].txOffset, 
                                  &txRecord, sizeof(txRecord)) != 0)
        {
            sts = LonApiNvdFileError;
        }
        alt_flash_close_dev(fd);
    }
    if (nvdTraceEnabled)
    {
        PrintTimeStamp();
        printf("End LonNvdExitTransaction(%d), sts = %d\n",  type, sts);  
    }

    return(sts);
}

/*
 * ******************************************************************************
 * SECTION: Support functions
 * ******************************************************************************
 *
 * This section contains the support functions used to support direct flash 
 * access.
 */

/* 
 *  Function: OpenFlash
 *  Open a flash segment, initializing the segmentMap if necessary.
 *
 *  Parameters:
 *  type - segment type
 *  pFd -  pointer to flash file descriptor, to be returned by this function.
 *
 *  Returns:
 *  <LonApiError>.   
 */
static LonApiError OpenFlash(const LonNvdSegmentType type, alt_flash_fd **pFd)
{
    LonApiError sts = LonApiNoError;

    if (type > LonNvdSegNumSegmentTypes)
    {
        /* Invalid data type. */
        sts = LonApiNvdFileError;
    }
    else 
    {
        /* Initialize segmentMap, if necessary. */
        sts = InitSegmentMap(type);
        if (sts == LonApiNoError)
        {
            /* Open the flash. */
            *pFd = alt_flash_open_dev(LON_NVD_ROOT_NAME);
            if (*pFd == 0)
            {
                sts = LonApiNvdFileError;
            }
        }
    }
    return(sts);
}

/* 
 *  Function: InitSegmentMap
 *  If it has not already been done, initialize the segmentMap for the 
 *  specified type.  The first time this function is called, it must determine
 *  the flash size by opening the flash and reading the parameters from the HAL.
 *  Note that the segment types must be initialized in order.
 *
 *  Parameters:
 *  type - segment type
 *
 *  Returns:
 *  <LonApiError>.   
 */
static LonApiError InitSegmentMap(const LonNvdSegmentType type)
{
    LonApiError sts = LonApiNoError;

    /* flashBlockSize is initially 0, and will be updated by this function. */
    if (flashBlockSize == 0)
    {
        alt_flash_fd *fd;

        /* Open the flash */
        fd = alt_flash_open_dev(LON_NVD_ROOT_NAME);
        if (fd == 0)
        {
            sts = LonApiNvdFileError;
        }
        else
        {
            flash_region *pInfo;
            int number_of_regions;

            /* Get the flash information from the HAL.  */
            if ((alt_get_flash_info(fd, &pInfo, &number_of_regions) == 0) &&
                 number_of_regions >= 1 && pInfo->offset == 0)
            {
                /* This code always places all the data segments in the first 
                 * region, and expects that region to start at offset 0. The 
                 * segments are allocated starting at the highest address of 
                 * the flash region.
                 */

                /* Start at the end of flash. */
                lowestUsedFlashDataOffset = pInfo->region_size;

                /* Update the flash block size.  In addition to recording this 
                 * information, it serves as a flag to indicate that we do not 
                 * need to initialize the segmentMap again.
                 */
                flashBlockSize = pInfo->block_size;

            }
        }
    }
    
    if (dataSegmentSize[type] == 0)
    {
        /* This segment has not been initalized yet. */
        if (type != 0 && dataSegmentSize[type-1] == 0)
        {
            /* Each segement must be initialized in order. */
            sts = LonApiNvdFileError;
        }
        else
        {
            int flashOffset;
            int blockOffset;
            dataSegmentSize[type] = LonNvdGetMaxSize(type);

            /* The segments are allocated starting at the highest 
             * address of the flash region.  Starting at 
             * lowestUsedFlashDataOffset, adjust the offset 
             * to allow for the transaction record and data segment.
             */

            flashOffset = lowestUsedFlashDataOffset -
                (sizeof(NvdTransactionRecord) + dataSegmentSize[type]);

            blockOffset = flashOffset % flashBlockSize;
                
            /* Adjust offset to start on a block boundary */
            flashOffset -= blockOffset;

            /* NOTE: This function assumes that code is loaded into 
             * low flash and that there is enough room to fit the data 
             * without overwriting the code.
             */
            segmentMap[type].segmentStart = flashOffset;
            segmentMap[type].txOffset = flashOffset;
            segmentMap[type].dataOffset = flashOffset + 
                                       sizeof(NvdTransactionRecord);
            segmentMap[type].maxDataSize = dataSegmentSize[type];

            /* Update the lowestUsedFlashDataOffset flash offset. */
            lowestUsedFlashDataOffset = flashOffset;
        }
    }    
    return sts;
}

/* 
 *  Function: EraseSegment
 *  Erase enough blocks in the segment to satisfy the requested size, starting 
 *  from the beginning of the segment.  Note that this leaves the transaction
 *  record (at the beginning of the segment) with an invalid value, indicating 
 *  that a transaction is in progress.
 *
 *  Parameters:
 *  fd -  open flash file descriptor
 *  type - segment type
 *  size - size of data, in bytes
 *
 *  Returns:
 *  <LonApiError>.   
 *
 *  Remarks:
 *  After erasing the flash, all of the bytes will be 0xff.  Writing to the 
 *  flash can only change 1 bits to 0 bits.
 */
static LonApiError EraseSegment(alt_flash_fd *fd, const LonNvdSegmentType type, 
                                int size)
{
	LonApiError sts = LonApiNoError;

        /* Get the start of the segment. */
    int offset = segmentMap[type].segmentStart;
        /* Keep erasing blocks until bytesErased >= size. */
    int bytesErased = 0;

    /* Erase blocks, a block at a time, until the request has been satisfied. */
    for (bytesErased = 0; bytesErased < size; bytesErased += flashBlockSize)
    {
        /* Erase the block.  */
        if (alt_erase_flash_block(fd, offset, flashBlockSize) != 0)
        {
            sts = LonApiNvdFileError;
            break;
        }
        /* Next block. */
        offset += flashBlockSize;
    }
    return sts;
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

#endif /* LON_NVD_MODEL_FLASH_DIRECT */


