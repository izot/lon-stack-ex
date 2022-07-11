/***************************************************************
 *  Filename: semLib.h
 *
 * Copyright © 1998-2022 Dialog Semiconductor
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
 *  Description:  Header file for VxWorks emulation layer.
 *
 *	DJ Duffy Oct 1998
 *
 ****************************************************************/

/*
 * $Log: /VNIstack/LNS_V3.2x/Target/include/semLib.h $
 * 
 * 8     4/07/04 4:38p Rjain
 * Added new VxLayer Dll. 
 * 
 * 6     12/29/99 9:03a Glen
 * Require explicit inclusion of semaphore debugging (and its associated
 * leak)
 * 
 * 4     3/12/99 12:00p Darrelld
 * Finalize debug features
 * 
 * 3     3/12/99 9:51a Darrelld
 * Add debug features to semLib to catch memory leaks
 * 
 * 2     11/09/98 11:56a Darrelld
 * Updates after native trial
 * 
 * 1     11/04/98 8:40a Darrelld
 * Windows Target header files
 */


#ifndef _SEMLIB_INCLUDE_H
#define _SEMLIB_INCLUDE_H 1
#include <vxWorks.h>
#include "VxLayerDll.h"
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef linux
#define S_objLib_OBJ_TIMEOUT 793
#define S_objLib_OBJ_UNAVAILABLE 0x3D0002
#endif
//
//  Semaphore types and data
//
// Semaphore constants we use
/* semaphore options */

#define SEM_Q_MASK		0x3	/* q-type mask */
#define SEM_Q_FIFO		0x0	/* first in first out queue */
#define SEM_Q_PRIORITY		0x1	/* priority sorted queue */
#define SEM_DELETE_SAFE		0x4	/* owner delete safe (mutex opt.) */
#define SEM_INVERSION_SAFE	0x8	/* no priority inversion (mutex opt.) */

/* binary semaphore initial state */

typedef enum		/* SEM_B_STATE */
    {
    SEM_EMPTY,			/* 0: semaphore not available */
    SEM_FULL			/* 1: semaphore available */
    } SEM_B_STATE;


// Types we support
#define VXL_SEM_BINARY 1
#define VXL_SEM_MUTEX 2

typedef struct _SEMSTUFF* SEM_ID;

// Use CreateSemphore with count of 1
VXLAYER_API SEM_ID	semBCreatez( int options, SEM_B_STATE initstate  );

// Use CreateMutex
VXLAYER_API SEM_ID	semMCreatez( int options);

#ifdef _DEBUG_SEMAPHORES
SEM_ID	semBCreateX( char* pFp, int line, int options, SEM_B_STATE initstate  );
SEM_ID	semMCreateX( char* pFp, int line, int options);
// show locks outstanding and cleanup last lock semaphore
// to avoid memory leak.
void	semShow();
void semCleanLock();
#define semMCreate(op) semMCreateX( __FILE__, __LINE__, op)
#define semBCreate(op, st) semBCreateX( __FILE__, __LINE__, op, st)
#else // _DEBUG
#define semMCreate semMCreatez
#define semBCreate semBCreatez
#endif // _DEBUG

// CloseHandle and throw away the stuff
VXLAYER_API STATUS	semDelete( SEM_ID sem );

// WaitObject
VXLAYER_API STATUS	semTake( SEM_ID sem, int timeout );

// Use ReleaseSemaphore or
// Release Mutex
VXLAYER_API STATUS	semGive( SEM_ID sem );

// Same as a Give. Not sure we should ever need this actually
VXLAYER_API STATUS	semFlush( SEM_ID sem );


#ifdef  __cplusplus
}
#endif

#endif // _SEMLIB_INCLUDE_H

