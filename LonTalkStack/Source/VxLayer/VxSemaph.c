/***************************************************************
 *  Filename: VxSemaph.c
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
 *  Description:  Implementation of Semaphores for VxWorks emulation layer.
 *
 *	DJ Duffy Oct 1998
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/VxLayer/VxSemaph.c#2 $
//
/*
 * $Log: /VNIstack/Dcx_Dev/VxLayer/VxSemaph.c $
 * 
 * 15    6/24/08 7:56a Glen
 * Support for EVNI running on the DCM
 * 1. Fix GCC problems such as requirement for virtual destructor and
 * prohibition of <class>::<method> syntax in class definitions
 * 2. Fix up case of include files since Linux is case sensitive
 * 3. Add LonTalk Enhanced Proxy to the stack
 * 4. Changed how certain optional features are controlled at compile time
 * 5. Platform specific stuff for the DC such as file system support
 * 
 * 1     12/05/07 11:19p Mwang
 * 
 * 14    3/26/07 2:39p Bobw
 * EPRS FIXED: 
 * 
 * 13    10/17/06 5:51p Fremont
 * Adjust semaphore use counting, put safeguards in place for leaving a
 * critical section
 * 
 * 12    4/07/04 4:38p Rjain
 * Added new VxLayer Dll. 
 * 
 * 11    3/02/00 3:10p Glen
 * Use critical sections instead of mutex
 * 
 * 10    1/07/00 5:21p Glen
 * Testing of Layer 5 MIP
 * 
 * 9     12/29/99 9:02a Glen
 * Require explicit inclusion of semaphore debugging (and its associated
 * leak)
 * 
 * 7     3/12/99 12:00p Darrelld
 * Finalize debug features
 * 
 * 6     3/12/99 9:51a Darrelld
 * Add debug features to semLib to catch memory leaks
 * 
 * 5     3/11/99 4:53p Darrelld
 * Maintain counts of semaphores
 * 
 * 4     1/22/99 7:15p Glen
 * Fixed SEM_EMPTY bug
 * 
 * 3     11/09/98 11:58a Darrelld
 * Updates after native trial
 * 
 * 2     11/06/98 9:42a Darrelld
 * Fix some problems and update the queue classes
 * 
 * 1     10/16/98 1:43p Darrelld
 * Vx Layer Sources
 */

#include <stdlib.h>
#include <string.h>
#include "Osal.h"

// Definitions for vxWorks
#include <VxWorks.h>
#include <VxLayer.h>
#include <taskLib.h>
#include <semLib.h>
#include "VxLayerDll.h"


////////////////////////////////////////////////////////////////////
//
// Semaphores
//
// We create either a binary semaphore or a mutex.
// We could easily, but do not now implement counted semaphores.
// Doesn't look like we need them.
//

typedef struct _SEMSTUFF
{
	int		type;	// in case we need a type for some reason
	OsalHandle	osHandle;			
	//int		threadid; // thread id of the owner (or last owner) (for debugging)
	void*		threadid; // ** use void* to force display in hex
	int     count;
	char*	pFilePath;
	int		nLineNo;
	struct _SEMSTUFF*	pNextSem;
	struct _SEMSTUFF*	pPrevSem;
} SEMSTUFF;

int		gSemBs = 0;
int		gSemMs = 0;
struct _SEMSTUFF*	pSemList = NULL;	// list of allocated semObjects
SEM_ID	gSemLock = NULL;
boolean	gbDeleteLast = false;

//
// lock and unlock the semaphore trace list
//
VXLAYER_API 
void lockSem()
{
	if ( gSemLock == NULL )
	{	gSemLock = semMCreatez( 0 );
	}
	semTake( gSemLock, WAIT_FOREVER );
}
VXLAYER_API
void unlockSem()
{
	semGive( gSemLock );
	// Last one out of the room, turn off the lights
}

//
// semCleanLock
//
// clean up the lock semaphore
//
VXLAYER_API
void semCleanLock()
{
	SEM_ID	semLock = gSemLock;
	gSemLock = NULL;
	gbDeleteLast = true;
	semDelete( semLock );
	gbDeleteLast = false;
}

//
// addSem
//
// add semaphore to trace list
//
VXLAYER_API
void addSem( SEM_ID pSem )
{
	SEM_ID	pSem2;
	lockSem();
	pSem2 = pSemList;
	if ( pSem2 )
	{
		pSem2->pPrevSem = pSem;
	}
	pSem->pNextSem = pSemList;
	pSem->pPrevSem = NULL;
	pSemList = pSem;
	unlockSem();
}

//
// remSem
// remove a semaphore from trace list
//
VXLAYER_API
void remSem( SEM_ID pSem )
{
#ifdef _DEBUG_SEMAPHORES
	lockSem();
	if ( pSem->pNextSem )
	{
		pSem->pNextSem->pPrevSem = pSem->pPrevSem;
	}
	if ( pSem->pPrevSem )
	{
		pSem->pPrevSem->pNextSem = pSem->pNextSem;
	}
	if ( pSemList == pSem )
	{
		pSemList = pSem->pNextSem;
	}
	unlockSem();
#endif
}
//
// semBCreatez
//
// non-debug version
// Use CreateSemphore with count based on initstate
// ignore options SEM_Q_FIFO, SEM_Q_PRIORITY
//
VXLAYER_API
SEM_ID	semBCreatez( int options, SEM_B_STATE initstate )
{
	SEM_ID		pSem = NULL;

    SetVxErrno( OK );
	pSem = (SEM_ID)malloc( sizeof(SEMSTUFF) );
	memset(pSem, 0, sizeof(SEMSTUFF));

	vxlTrace( "semBCreate - SEMSTUFF 0x%08x\n", pSem );

	if ( pSem )
	{
        OsalBinarySemState initOsalState;
        initOsalState = (initstate == SEM_FULL) ? OSAL_SEM_SET : OSAL_SEM_CLEAR;
		pSem->pNextSem = NULL;
		pSem->pPrevSem = NULL;
		pSem->pFilePath = NULL;
		pSem->nLineNo = 0;

		pSem->type = VXL_SEM_BINARY;
		pSem->count = 0;
		pSem->osHandle = NULL;
	    // Options are SEM_FULL and SEM_EMPTY
	    //                 1            0
        if (OsalCreateBinarySemaphore(&pSem->osHandle, initOsalState) != OSALSTS_SUCCESS)
		{
			vxlReportLastError("semBCreate - OsalCreateBinarySemaphore");
			SetVxErrno( 799 ); // need real error
			free( pSem );
			pSem = NULL;
		}
	}
	else
	{
		vxlReportError("semBCreate - unable to allocate semaphore");
		SetVxErrno( 798 ); // need real error
	}
	if ( pSem ) gSemBs++;
	return	pSem;
}

//
// semMCreatez
//
// non-debug version
//
VXLAYER_API
SEM_ID	semMCreatez( int options )
{
	SEM_ID		pSem = NULL;

	SetVxErrno( OK );
	pSem = (SEM_ID)malloc( sizeof(SEMSTUFF) );
	if ( pSem )
	{
		pSem->pNextSem = NULL;
		pSem->pPrevSem = NULL;
		pSem->pFilePath = NULL;
		pSem->nLineNo = 0;
		pSem->count = 0;

		pSem->type = VXL_SEM_MUTEX;
		pSem->osHandle = NULL;

        if (OsalCreateCriticalSection(&pSem->osHandle) != OSALSTS_SUCCESS)
        {
		    vxlReportError("semMCreate - unable to OsalCreateCriticalSection");
            free(pSem);
            pSem = NULL;
        }
	}
	else
	{
		vxlReportError("semMCreate - unable to allocate mutex");
		SetVxErrno( 796 ); // need real error
	}
	if ( pSem ) gSemMs++;
	return	pSem;
}

//
// semBCreateX
//
// Debug version with trace of allocator
//
VXLAYER_API
SEM_ID	semBCreateX( char* pFp, int line, int options, SEM_B_STATE initstate )
{
	SEM_ID		pSem;
	pSem = semBCreatez( options, initstate );
	if ( pSem )
	{
		pSem->pFilePath = pFp;
		pSem->nLineNo = line;
		addSem( pSem );
	}
	return pSem;
}

//
// semMCreateX
//
// Debug version with trace of allocator
//
VXLAYER_API
SEM_ID	semMCreateX( char* pFp, int line,  int options )
{
	SEM_ID		pSem;
	pSem = semMCreatez( options );
	if ( pSem )
	{
		pSem->pFilePath = pFp;
		pSem->nLineNo = line;
		addSem( pSem );
	}
	return pSem;
}

//
// semShow
//
// Display all currently open semaphores
//
VXLAYER_API
void	semShow()
{
	SEM_ID	pSem;
	boolean	bOldTrace;
	lockSem();
	bOldTrace = vxlTraceEnable( true );
	pSem = pSemList;
	vxlTrace( "semShow - allocated semaphores - Total %d Binary %d Mutexes %d\n",
				gSemBs+gSemMs, gSemBs, gSemMs );
	while ( pSem )
	{
		vxlTrace("%s(%d) : sem 0x%08x %s\n",
			pSem->pFilePath?pSem->pFilePath:"no file",
			pSem->nLineNo,
			pSem,
			pSem->type==VXL_SEM_MUTEX?"Mutex ":"Binary"
			);
		pSem = pSem->pNextSem;
	}
	vxlTraceEnable( bOldTrace );
}

// CloseHandle and throw away the stuff
VXLAYER_API
STATUS	semDelete( SEM_ID pSem )
{
	STATUS	sts = OK;
	// defend against NULL pointer
	if ( pSem )
	{
		vxlTrace( "semDelete - SEM 0x%08x\n", pSem );
		if ( pSem->type == VXL_SEM_MUTEX )
		{	gSemMs--;
			OsalDeleteCriticalSection(&pSem->osHandle);
		}
		else
		{	gSemBs--;
			OsalDeleteBinarySemaphore(&pSem->osHandle);
		}
		if ( !gbDeleteLast )
		{	remSem( pSem );	// remove from debug list
		}
		free( pSem);
	}
	else
	{
		vxlReportError("semDelete - NULL SEM_ID");
		sts = ERROR;
	}
	return sts;
}

// WaitObject
VXLAYER_API
STATUS	semTake( SEM_ID pSem, int timeout )
{
	OsalStatus	osalSts;
	STATUS	sts = OK;

	SetVxErrno( OK );

	if (pSem)
	{
		switch (pSem->type)
		{
		case VXL_SEM_BINARY:
			while ( pSem->osHandle )
			{
                osalSts = OsalWaitForBinarySemaphore(pSem->osHandle, timeout );
				if ( osalSts == OSALSTS_SUCCESS )
				{	
					pSem->count++;
					pSem->threadid = (void*)OsalGetTaskId();
					break;
				}
				else if ( osalSts == OSALSTS_TIMEOUT )
				{	sts = ERROR;
					SetVxErrno( S_objLib_OBJ_TIMEOUT ); // real error for timeout [EINTR]
				}
				else
				{
					sts = ERROR;
					SetVxErrno( 794 ); // real error
				}
				break;
			}
			break;
		case VXL_SEM_MUTEX:
			if (OsalEnterCriticalSection(pSem->osHandle) == OSALSTS_SUCCESS)
            {
			    pSem->count++;
			    pSem->threadid = (void*)OsalGetTaskId();
            }
            else
            {
                sts = ERROR;
            }
			break;
		default:
			sts = ERROR;
			vxlReportError("semTake - corrupt sem type");
			break;
		}
	}
	return sts;
}


// Use OsalReleaseBinarySemaphore or
// OsalLeaveCriticalSection
VXLAYER_API
STATUS	semGive( SEM_ID pSem )
{
	STATUS	sts = OK;

	if ( pSem )
	{
		switch ( pSem->type )
		{
		case VXL_SEM_BINARY:
			pSem->count--;
			pSem->threadid = 0;
            if (OsalReleaseBinarySemaphore(pSem->osHandle) != OSALSTS_SUCCESS)
            {
				// Don't report this error. It is reported frequently
				// when using semaphore as an event to signal another
				// task since there is a race.
				//vxlReportLastError("semGive - OsalReleaseBinarySemaphore");
				sts = ERROR;
				SetVxErrno( 779 ); // need real error code
			}
			break;
			
		case VXL_SEM_MUTEX:
			if (pSem->count <= 0)
			{
				// don't give a crititcal section more times than it is taken!
				sts = ERROR;
			}
			else if (pSem->threadid != (void*)OsalGetTaskId())
			{
				// don't allow a different thread to give up a mutex
				// This should just generate an error, even on Windows, but it seems
				// to corrupt the critical section and proceed as if nothing was wrong.
				sts = ERROR;
			}
			else
			{
				if (--pSem->count == 0)
				{
					pSem->threadid = 0;
				}
				OsalLeaveCriticalSection(pSem->osHandle);
			}
			break;
		default:
			sts = ERROR;
			vxlReportError("semGive - corrupt sem type");
			break;
		}
	}
	else
	{
		sts = ERROR;
		vxlReportError("semGive - corrupt semaphore");
	}

	return sts;
}



// Same as a Give. Not sure we should ever need this actually
VXLAYER_API
STATUS	semFlush( SEM_ID sem )
{
	return semGive( sem );
}


// end
