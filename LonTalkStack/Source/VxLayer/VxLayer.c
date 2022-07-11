/***************************************************************
 *  Filename: VxLayer.c
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
 *  Description:  Implementation of VxWorks emulation layer.
 *
 *	DJ Duffy Oct 1998
 *
 ****************************************************************/

/*
 * $Log: /Dev/VxLayer/VxLayer.c $
 * 
 * 37    03/24/2010  Adrain Czaika
 *       TASKNAMEMAX: changed 32 --> 48
 *       printfAllTasks: added
 *
 *
 * 36    7/02/08 9:55a Fremont
 * Fixup for Glen's stack changes for the DCM
 * 
 * 35    6/24/08 7:56a Glen
 * Support for EVNI running on the DCM
 * 1. Fix GCC problems such as requirement for virtual destructor and
 * prohibition of <class>::<method> syntax in class definitions
 * 2. Fix up case of include files since Linux is case sensitive
 * 3. Add LonTalk Enhanced Proxy to the stack
 * 4. Changed how certain optional features are controlled at compile time
 * 5. Platform specific stuff for the DC such as file system support
 * 
 * 2     12/08/07 2:36a Henry
 * If strtok_r() is already defined as a macro, declaring it as a function
 * can produce gcc errors.  Somehow this was a problem only on host
 * release builds.
 * 
 * 1     12/05/07 11:19p Mwang
 * 
 * 34    10/22/07 11:23a Bobw
 * 
 * 33    10/22/07 10:37a Bobw
 * EPR 46992
 * Remove OsalSuspendTask and OsalResumeTask from the OSAL.  On non-ilon,
 * change vxlInit to use a critical section initialized from task spawn.
 * On the i.lon stick with taskLock and taskUnlock.  On the i.lon these
 * are implemented by vxWorks, but on the non-ilon they depend on the
 * ability to suspend tasks, so on the i.lon WIN32 implment
 * taskLock/taskUnlock using WIN32 suspend/resume
 * 
 * 32    8/10/07 6:01p Bobw
 * 
 * 31    8/10/07 2:25p Bobw
 * Remove unneeded and expensive code from FTXL
 * 
 * 30    3/29/07 12:58p Bobw
 * More FTXL development
 * 
 * 29    3/26/07 2:39p Bobw
 * EPRS FIXED: 
 * 
 * 28    10/17/06 5:49p Fremont
 * add total task count for debug, don't leave task table locked if
 * terminating self
 * 
 * 27    4/07/04 5:07p Rjain
 * Exported another function that is required for xDriver.
 * 
 * 26    4/07/04 4:38p Rjain
 * Added new VxLayer Dll. 
 * 
 * 25    1/31/03 10:37a Bobw
 * EPR 28227
 * Increase max tasks.
 * 
 * 24    11/01/02 11:12a Bobw
 * EPR 27592
 * Initialize TLS on DLL initialization - since some users of the stack
 * make calls into the stack before TaskSpawn (which normally does the TLS
 * init).
 * 
 * 23    7/15/02 2:28p Glen
 * Add stuff needed by ilon100.
 * 
 * 22    7/09/02 1:49p Glen
 * Correct resource lock out condition that could occur on Windows.
 * 
 * 21    5/24/02 3:03p Fremont
 * 
 * 20    5/23/02 5:53p Fremont
 * add taskLock/taskUnlock, taskForceDelete
 * 
 * 19    10/20/00 11:24a Bobw
 * Close handles to threads when they die, to prevent handle leak.
 * Support more threads to allow lots of LNS networks to be open.
 * 
 * 18    4/24/00 4:20p Glen
 * Thread local storage bug fix.
 * 
 * 17    3/23/00 10:01a Darrelld
 * Remove message boxes. Use report urgent messages.
 * 
 * 16    2/23/00 9:10a Darrelld
 * LNS 3.0 Merge
 * 
 * 15    1/14/00 5:21p Glen
 * Support retrieveStatus and clearStatus
 * 
 * 14    12/17/99 4:55p Glen
 * Use TlsAlloc
 * 
 * 13    12/09/99 4:10p Bobw
 * Send to debugger and console 
 * 
 * 11    8/05/99 5:37p Glen
 * Add logMsg
 * 
 * 10    5/14/99 5:45p Glen
 * Change priorities again
 * 
 * 9     5/14/99 12:50p Glen
 * Changed priority border to 100
 * 
 * 8     3/18/99 3:58p Glen
 * Fix problem with task exit and argument overwrite
 * 
 * 7     1/19/99 12:10p Glen
 * Support priority (<=200 is normal; >200 is above normal)
 * 
 * 6     11/20/98 8:58a Darrelld
 * Fix header file for tasks and add taskExit
 * 
 * 5     11/09/98 2:05p Darrelld
 * Updates for Windows operation
 * 
 * 4     11/09/98 11:58a Darrelld
 * Updates after native trial
 * 
 * 3     11/06/98 9:42a Darrelld
 * Fix some problems and update the queue classes
 * 
 * 2     10/23/98 5:04p Darrelld
 * Enhancements and socket testing
 * 
 * 1     10/19/98 2:31p Darrelld
 * Vx Layer Sources
 * 
 */

#include "LtaDefine.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "Osal.h"

#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#include <io.h>
#endif

// Definitions for VxLayer
#include <VxWorks.h>
#include <VxLayer.h>
#include <taskLib.h>

#include <stdarg.h>
#include <vxlTarget.h>

#ifdef linux
#include <pthread.h>
#include <errno.h>
#endif

// We are going to use this construct to declare thread local variables,
// since we don't have to deal with the storage this way.
// No pointers or structs with pointers. Just global data.
// There is no reliable way to declare a thread exit handler so
// clean up of this storage is potentially a problem if we have
// a dynamic thread population.
//
// This doesn't work with dynamically loaded DLLs.
#define THREADLOCAL  __declspec( thread )

struct mem_part* memSysPartId = NULL;
struct {int t[2];} vxAbsTicks;

#if !defined(__VXWORKS__)
static OsalHandle vxlInitLock = NULL;
#endif

static boolean gbVxlRunning = true;

////////////////////////////////////////////////////////////////////
//
// Error handling stuff
//

//
// vxlReportLastError
//
// Report an error returned by Win32
//
void vxlReportLastError( LPSTR tag )
{
	char	msg[MAX_PATH];
	int		err = OsalGetLastOsError();

	sprintf( msg, "%s\rError code = %d, 0x%08x",tag, err, err );
	// No windows here
	// Just report the message
	//MessageBox( NULL, msg, "VxLayer Error", MB_ICONERROR | MB_OK );
	vxlReportUrgent( msg );
}

//
// vxlReportError
//
// Report an error string only
//
void vxlReportError( LPSTR tag )
{
	// Just report the message
	//MessageBox( NULL, tag, "VxLayer Error", MB_ICONERROR | MB_OK );
	vxlReportUrgent( tag );
}


//
// vxlReportErrorPrintf
//
// Report an error string in the style of printf
//
void vxlReportErrorPrintf( char* fmt, ... )
{
	char	msg[MAX_PATH*2];
	va_list	ap;

	va_start( ap, fmt );
	vsprintf( msg, fmt, ap );

	// Just report the message
	//MessageBox( NULL, msg, "VxLayer Error", MB_ICONERROR | MB_OK );
	vxlReportUrgent( msg );
}


//
// vxlTrace
//
// Like printf, but ships it to Windows OutputDebugString
//

BOOL	g_bVxlTraceEnable = FALSE;

BOOL	vxlTraceEnable( BOOL bEnable )
{
	BOOL	bPrev = g_bVxlTraceEnable;
	g_bVxlTraceEnable = bEnable;
	return bPrev;
}

void vxlTrace( char* fmt, ... )
{
	char	msg[MAX_PATH*2];
	va_list	ap;

	if ( g_bVxlTraceEnable )
	{
		va_start( ap, fmt );
		vsprintf( msg, fmt, ap );
		OsalPrintDebugString( msg );
	}
}

BOOL vxlRunning()
{
	return gbVxlRunning;
}

#if !defined(ILON_PLATFORM) || !defined(__VXWORKS__)
/* Create the lock used by vxlInit to lock its intializatoin. This is called
 * by taskSpawn to ensure tht its created before thre is more than one 
 * task running.  The i.lon cannot depend on this, so it uses taskUnlock
 * instead.
 */
void initVxlInitLock()
{
    if (vxlInitLock == NULL)
    {
        OsalCreateCriticalSection(&vxlInitLock);
    }
}

void freeVxlInitLock()
{
    OsalDeleteCriticalSection(&vxlInitLock);
}

/* Lock VXL inititialization */
void lockVxlInit()
{
    initVxlInitLock();

    if (vxlInitLock != NULL)
    {
        OsalEnterCriticalSection(vxlInitLock);
    }
}

/* Unlock VXL inititialization */
void  unlockVxlInit()
{
    if (vxlInitLock != NULL)
    {
        OsalLeaveCriticalSection(vxlInitLock);
    }
}
#endif

//	enum {
//		OK = 0,				// Good 'ole unix
//		ERROR = -1			// truth is reversed for these folks
//		);

////////////////////////////////////////////////////////////////////
//
// Tasks
//

#if PRODUCT_IS(FTXL)
    #define TASKMAXTASKS 20
#else
    #define TASKMAXTASKS 8192 // 4096
#endif
// ACZAIKA
#define	TASKNAMEMAX	48

typedef struct _TASKBLOCK
{
	char        name[TASKNAMEMAX];      // task name
	BOOL		bBusy;					// task is active
	BOOL		bSuspended;				// task is suspended
	HANDLE		hThread;				// handle for thread
	void*		dwThreadId;				// ** Use void* to force display in hex
	DWORD		dwStackSize;			// initial stack size for thread
	FUNCPTR		entryPt;				// entry point function
	int			args[10];				// parameters
    int         vxError;

} TASKBLOCK;

//
// Array of task blocks
//
TASKBLOCK	aTasks[TASKMAXTASKS];
static int gTaskCount = 0;

//
// vxlShutdown
//
// Call this to release VXL resources
//
void vxlShutdown()
{
	int i;
	gbVxlRunning = false;
	vxlReportGo();
	vxlTimerGo();
	// Wait for the tasks to die
	for (i=0; gTaskCount && i<2000; i++)
	{
		OsalSleep(1);
	}
	if (gTaskCount)
	{
		assert(0);
		for (i=1; i<TASKMAXTASKS; i++)
		{
			if (aTasks[i].bBusy)
				printf("Task %s did not exit\n", aTasks[i].name);
		}
	}
}

//
// Semaphore to lock the task blocks while
// being accessed.
//
OsalHandle          csTaskLock;
BOOL				bTaskLockInit;

// This counts the taskLock()/taskUnlock() calls, not LockTaskList()/UnlockTaskList()
int gTaskLockCount = 0;

//
// vxlGetTaskBlock
//
// Return task block address from an id
//

TASKBLOCK*	vxlGetTaskBlock( int tid )
{
	return &aTasks[tid];
}

//
// vxlGetThreadHandle
//
// Return handle for thread by task id
//
VXLAYER_API
HANDLE	vxlGetThreadHandle( int tid )
{
	return aTasks[tid].hThread;
}

#if PRODUCT_IS(ILON)
// VxWorks uses errno.
// Let's not depend on that being portable, so redefine it as a
// subroutine so we can override it appropritely for Win NT
//

static int mainThreadErrorNo=0;

int VxErrno(void)
{
    int tid = OsalGetTaskIndex();
    if (tid == 0 || tid >= TASKMAXTASKS)
    {
        return mainThreadErrorNo;
    }
    else 
    {
        return aTasks[tid].vxError;
    }
}

void SetVxErrno( int err )
{
#ifdef linux
	// set the error number for Linux host
	errno = err;
#endif
    int tid = OsalGetTaskIndex();
    if (tid == 0 || tid >= TASKMAXTASKS)
    {
        mainThreadErrorNo = err;
    }
    else 
    {
        aTasks[tid].vxError = err;
    }
}
#endif

//
// LockTaskList
//
// Lock the task list in case we modify it
//
void	LockTaskList( void)
{
	// One time init of the critical section
	if ( ! bTaskLockInit )
	{
		OsalCreateCriticalSection( &csTaskLock );
		vxlTrace("LockTaskList - create task lock 0x%08x\n", csTaskLock );
		bTaskLockInit = TRUE;
		// Probably not necessary, but just to make sure.
		memset( &aTasks, 0, sizeof(aTasks) );
		gTaskCount = 0;
	}
	OsalEnterCriticalSection( csTaskLock );
}

//
// UnlockTaskList
//
// Unlock the task list when we are done.
//
void	UnlockTaskList(void)
{
	OsalLeaveCriticalSection( csTaskLock );
}

void	Cleanup(void)
{
	if (gTaskCount == 0)
	{
		// The last guy is gone.  Let's clean up the global resources.
		// NOTE: This assumes shutdown is orderly and serial with startup.
		// For example, we wouldn't be starting up a new context while the 
		// previous one was still being torn down.
		OsalDeleteCriticalSection( &csTaskLock );
        bTaskLockInit = FALSE;
		freeVxlInitLock();		
	}
}

// Fixed crash in the case when task exit very quickly
//
// OsalTaskEntryPoint
//
// The actual root level routine for the 'task'.
// We call the actual routine from here and pass all the myriad
// arguments that VxWorks requires.
//
void OsalTaskEntryPoint( int idx )
{
	LockTaskList();
	// Just a check to see if we have a valid
	// task block. Task may have been deleted before
	// we get here actually.
	if ( !aTasks[idx].bBusy )
	{
	    UnlockTaskList();
		return;
	}

    aTasks[idx].vxError = 0;
	UnlockTaskList();

	// Not clear about the return from a task on VxWorks
	aTasks[idx].entryPt(
				aTasks[idx].args[0],
				aTasks[idx].args[1],
				aTasks[idx].args[2],
				aTasks[idx].args[3],
				aTasks[idx].args[4],
				aTasks[idx].args[5],
				aTasks[idx].args[6],
				aTasks[idx].args[7],
				aTasks[idx].args[8],
				aTasks[idx].args[9] );

	// Release the task block
	LockTaskList();
    OsalCloseTaskHandle(aTasks[idx].hThread);
	memset( &aTasks[idx], 0, sizeof(TASKBLOCK) );
	gTaskCount--;
	UnlockTaskList();
	Cleanup();
}

// Fixed crash in the case when task exit very quickly
//
// taskSpawn
//
// Create a task and return the task id for the task or NULL
//
VXLAYER_API
int	taskSpawn(
 	const char* name,	// name of the task / thread
 	int priority,		// 0 - 255
 	int options,		// ignored
 	int stacksize,		// size of stack.  
 	FUNCPTR entryPt,	// entry point function, 
 						// on VxWorks same as FUNCPTR
 	int arg1, int arg2, int arg3, int arg4, int arg5,
	int arg6, int arg7, int arg8, int arg9, int arg10 )	// parameters
{
	int			i;
	BOOL		bFound = FALSE;
	STATUS		sts = OK;
    OsalTaskId taskId;
    OsalHandle taskHandle;

#if !defined(__VXWORKS__)
    /* Initialize the vxlInitLock if not already done so that we can be
     * assured that it is created before there are any more tasks created.
	 * NOTE: we assume that there is only one thread that starts up
	 * tasks to begin with!
     */
    initVxlInitLock();
#endif

	// Find an entry in the task list for us to use
	LockTaskList();

	// Never allocate task id zero, since that's special
	// it means the current task
	for ( i=1; i< TASKMAXTASKS; i++ )
	{
		if ( !aTasks[i].bBusy)
		{
			aTasks[i].bBusy = TRUE;
			bFound = TRUE;
			break;
		}
	}
	// Can't find a open entry to allocate a task.
	if ( !bFound )
	{
	    UnlockTaskList();
		SetVxErrno( 989 );	// what to set it to?
		return ERROR;
	}

	// Save the name
	strncpy(aTasks[i].name, name, TASKNAMEMAX);
	aTasks[i].name[TASKNAMEMAX-1] = 0;

	// Store away the parameters for the task
	aTasks[i].args[0] = arg1;
	aTasks[i].args[1] = arg2;
	aTasks[i].args[2] = arg3;
	aTasks[i].args[3] = arg4;
	aTasks[i].args[4] = arg5;
	aTasks[i].args[5] = arg6;
	aTasks[i].args[6] = arg7;
	aTasks[i].args[7] = arg8;
	aTasks[i].args[8] = arg9;
	aTasks[i].args[9] = arg10;
	aTasks[i].dwStackSize = stacksize;
	aTasks[i].entryPt	= entryPt;
    OsalCreateTask(OsalTaskEntryPoint, i, stacksize, priority, &taskHandle, &taskId);
    aTasks[i].hThread = taskHandle;
	// Create and start a thread that is the task

	if ( aTasks[i].hThread == NULL )
	{
		// Free the task block on an error
		memset( &aTasks[i], 0, sizeof(TASKBLOCK) );
		sts = ERROR;
	    UnlockTaskList();
        vxlReportLastError( "taskSpawn - CreateThread");
		SetVxErrno( 990 );		// what to set it to?
	}
	else
	{
        aTasks[i].dwThreadId = (void *)taskId;
		sts = i; // on success return vxTask ID
		gTaskCount++;
	    UnlockTaskList();
	}
	return sts;
}

//
// taskIdSelf
//
// return the task id for the current task / thread
//
VXLAYER_API
int taskIdSelf(void)
{
	return OsalGetTaskIndex();
}

//
// taskIdVerify
//
// Verify that a task is active
//
STATUS taskIdVerify( int tid )
{
	STATUS		sts = ERROR;
	// Do we really need to lock the task array for a look see?
	// Nope, chance it.
	if ( tid < 1 || tid >= TASKMAXTASKS )
	{	return sts;
	}
	// If the task is busy, then it's ok
	// else error for task not active.
	if ( aTasks[tid].bBusy )
	{	sts = OK;
	}
	return sts;
}

//
// Is a task suspended
//
STATUS taskIsSuspended( int tid )
{
	STATUS		sts = ERROR;
	// Do we really need to lock the task array for a look see?
	// Nope, chance it.
	if ( tid < 1 || tid >= TASKMAXTASKS )
	{	return sts;
	}
	// If the task is suspended, then it's ok
	// else error for task not suspended.
	if ( aTasks[tid].bBusy && 
		 aTasks[tid].bSuspended )
	{	sts = OK;
	}
	return sts;
}

//
// taskIsReady
//
// Is task ready to run, which means NOT suspended to us.
//
STATUS taskIsReady( int tid )
{
	STATUS		sts = ERROR;
	// Do we really need to lock the task array for a look see?
	// Nope, chance it.
	if ( tid < 1 || tid >= TASKMAXTASKS )
	{	return sts;
	}
	// If the task is NOT suspended, then it's ok
	// else error for task suspended.
	if ( aTasks[tid].bBusy && 
		 !aTasks[tid].bSuspended )
	{	sts = OK;
	}
	else
	{	sts = ERROR;
		SetVxErrno( 994 );	// What to set it to?
	}
	return sts;
}


#if (defined(WIN32) && !PRODUCT_IS(FTXL) && !PRODUCT_IS(LONTALK_STACK)) || !PRODUCT_IS(IZOT) || defined(linux)
//
// taskDelete
//
// Delete a task
//
VXLAYER_API
STATUS taskDelete( int tid )
{
#ifdef linux
	// We do not support cancelling task in Linux.
	// Cancelling task is not a good thing to do anyway and should be avoided!
	return OK;
#else
	STATUS		sts = ERROR;
	// Do we really need to lock the task array for a look see?
	// Nope, chance it.
	if ( tid < 1 || tid >= TASKMAXTASKS )
	{	return sts;
	}

	if (tid == taskIdSelf())
	{
		taskExit();	// no return from this
	}
	// Now we need to lock the task list since we are going to blow
	// a task away from the list.
	LockTaskList();
	// If the task is active, then it's ok
	// else error for task not around any more.
	// We are ignoring the race of threads having the same id
	// VxWorks doesn't protect against that either.
	// Also we assume you can delete a suspended thread.
	if ( aTasks[tid].bBusy )
	{	sts = OK;
		if (! TerminateThread( aTasks[tid].hThread, 0 ) )
		{
			vxlReportLastError("taskDelete - TerminateThread" );
			sts = ERROR;
			SetVxErrno( 995 );	// What to set it to?
		}
		// Free the task entry now (only after the thread is terminated)
        CloseHandle( aTasks[tid].hThread );
        aTasks[tid].hThread = NULL;
		memset( &aTasks[tid], 0, sizeof(TASKBLOCK) );
		gTaskCount--;
	}
	UnlockTaskList();
	Cleanup();
	return sts;
#endif
}


// This is not a conforming vxworks implementation, but we don't implement
// taskSafe()/taskUnsafe(), so it doesn't matter.
//
STATUS taskDeleteForce( int tid )
{
	return taskDelete(tid);
}


//
// taskExit
//
// Delete the current task
//
void taskExit()
{
	int		tid;

	tid = taskIdSelf();
	// Do we really need to lock the task array for a look see?
	// Nope, chance it.
	if ( tid < 1 || tid >= TASKMAXTASKS )
	{	return;
	}
	// Now we need to lock the task list since we are going to blow
	// a task away from the list.
	LockTaskList();
	// If the task is active, then it's ok
	// else error for task not around any more.
	// We are ignoring the race of threads having the same id
	// VxWorks doesn't protect against that either.
	// Also we assume you can delete a suspended thread.
	if ( aTasks[tid].bBusy )
	{
		//CloseHandle( aTasks[tid].hThread );
		// Free the task entry now.
		memset( &aTasks[tid], 0, sizeof(TASKBLOCK) );
		gTaskCount--;
	}
	UnlockTaskList();

	Cleanup();

	// Blow ourselves away
#ifdef _WIN32
	ExitThread(1);
#else
	pthread_exit(NULL);
#endif
}
#endif

#if defined(__VXWORKS__)
//
// taskSuspend
//
// Suspend a task
//
STATUS taskSuspend( int tid )
{
	STATUS		sts = ERROR;
	DWORD		winSts;
	// Do we really need to lock the task array for a look see?
	// Nope, chance it.
	if ( tid < 0 || tid >= TASKMAXTASKS )
	{	return sts;
	}

	// Check for the Suspend Self case
	if ( tid == 0 || tid == taskIdSelf() )
	{
		tid = taskIdSelf();
		sts = OK;
		winSts = SuspendThread( aTasks[tid].hThread );
		if ( winSts == 0xFFFFFFFF )
		{
			vxlReportLastError( "taskSuspend - SuspendThread self" );
			SetVxErrno( 998 );		// Not sure which error yet
			sts = ERROR;
		}
		return sts;
	}		
	// If the task is suspended, then it's ok
	// else error for task not suspended.
	// Protect against simultaneous access by another thread
	LockTaskList();
	if ( aTasks[tid].bBusy && 
		 !aTasks[tid].bSuspended )
	{	sts = OK;
		// note potential compatibility
		// Timers don't run in a Suspended thread
		// that someone else suspends.
		winSts = SuspendThread( aTasks[tid].hThread );
		if ( winSts == 0xFFFFFFFF )
		{
			vxlReportLastError( "taskSuspend - SuspendThread" );
			SetVxErrno( 998 );		// Not sure which error yet
			sts = ERROR;
		}
		else
		{	aTasks[tid].bSuspended = TRUE;
		}
	}
	else
	{	sts = ERROR;
		SetVxErrno( 997 );	// What to set it to?
	}
	UnlockTaskList();
	return sts;
}


//
// taskResume
//
// Resume a task
//
STATUS taskResume( int tid )
{
	STATUS		sts = ERROR;
	DWORD		winSts;
	// Do we really need to lock the task array for a look see?
	// Nope, chance it.
	if ( tid < 1 || tid >= TASKMAXTASKS )
	{	return sts;
	}
	// If the task is suspended, then it's ok
	// else error for task not suspended.
	if ( aTasks[tid].bSuspended )
	{	sts = OK;
	    winSts = ResumeThread( aTasks[tid].hThread );
	    if ( winSts == 0xFFFFFFFF )
		{
			vxlReportLastError("taskResume - ResumeThread");
			sts = ERROR;
			SetVxErrno( 999 );		// Don't know an appropriate error yet
		}
	}
	else
	{	sts = ERROR;
		SetVxErrno( 996 );	// What to set it to?
	}
	return sts;
}
#endif


//
// taskDelay
//
// Delay the current task. Wake up on something happening
// too.
//
VXLAYER_API
STATUS	taskDelay( int ticks )
{
    OsalStatus  osalSts;
	STATUS	sts = OK;

	osalSts = OsalSleep(ticks);
	if ( osalSts !=  OSALSTS_SUCCESS)
	{
		sts = ERROR;
		//SetVxErrno( EINTR );
		SetVxErrno( 991 );
	}

	return sts;
}

#if 0
// 
// taskLock
//
STATUS taskLock(void)
{
	int tid;
	int tidSelf = taskIdSelf();
	STATUS sts = OK;
	DWORD winSts;

	LockTaskList();
	gTaskLockCount++;
	// Only suspend the threads the first time
	if (gTaskLockCount == 1)
	{
		for (tid = 1; tid < TASKMAXTASKS; tid++)
		{
			if ((tid != tidSelf) && (aTasks[tid].bBusy))
			{
				winSts = SuspendThread( aTasks[tid].hThread );
				if ( winSts == 0xFFFFFFFF )
				{
					vxlReportLastError("taskLock - SuspendThread");
					sts = ERROR;
					SetVxErrno( 999 );		// Don't know an appropriate error yet
				}
			}
		}
	}
	UnlockTaskList();
	return(sts);
}


// 
// taskUnlock
//
STATUS taskUnlock(void)
{
	int tid;
	int tidSelf = taskIdSelf();
	STATUS sts = OK;
	DWORD winSts;

	LockTaskList();
	if (gTaskLockCount > 0)
	{
		gTaskLockCount--;
		if (gTaskLockCount == 0)
		{
			for (tid = 1; tid < TASKMAXTASKS; tid++)
			{
				if ((tid != tidSelf) && (aTasks[tid].bBusy))
				{
					winSts = ResumeThread( aTasks[tid].hThread );
					if ( winSts == 0xFFFFFFFF )
					{
						vxlReportLastError("taskUnlock - ResumeThread");
						sts = ERROR;
						SetVxErrno( 999 );		// Don't know an appropriate error yet
					}
				}
			}
		}
	}
	UnlockTaskList();
	return(sts);
}
#endif

// 
// taskNameToId
//
int taskNameToId(char *name)
{
	int result = ERROR;
	int tid;

	for (tid = 1; tid < TASKMAXTASKS; tid++)
	{
		if (aTasks[tid].bBusy && (strncmp(aTasks[tid].name, name, TASKNAMEMAX) == 0))
		{
			result = tid;
			break;
		}
	}

	return(result);
}

//
// printfTasks
//
void printfAllTasks(void)
{
	int tid = 1;

	printf("\nPRINTING ALL CREATED TASKS\n"
			 "[status] taskId, taskHandle, taskName\n"
			 "=====================================\n");
	for (tid = 1; tid < TASKMAXTASKS; ++tid)
	{
		if(aTasks[tid].bBusy) {
			printf("[busy] %04d,  %p,  '%s'\n", tid, aTasks[tid].hThread, aTasks[tid].name);
		}
		else if(aTasks[tid].bSuspended) {
				printf("[suspended] %04d,  %p,  '%s'\n", tid, aTasks[tid].hThread, aTasks[tid].name);
			}
	}
	printf("\n");
}

// 
// taskPriorityGet
//
STATUS  taskPriorityGet( int tid, int *priority )
{
	// TBD - needs to handle tid == 0.
	*priority = 100;
	return OK;
}

//
// taskPrioritySet
//
STATUS  taskPrioritySet( int tid, int priority )
{
	// TBD - needs to handle tid == 0.
	return OK;
}

//
// tickGet
//
VXLAYER_API
ULONG	tickGet()
{
	return OsalGetTickCount();
}

//
// logMsg
//
int	logMsg (const char *fmt, int arg1, int arg2,
			int arg3, int arg4, int arg5, int arg6)
{
	char	msg[MAX_PATH*2];
    sprintf(msg, fmt, arg1, arg2, arg3, arg4, arg5, arg6);
    OsalPrintDebugString(fmt);
	return 0;
}


#if defined(__VXWORKS__) || defined(WIN32)
//
// strtok_r
//
// "re-entrant" strtok.  Not for thread safety but instead there is
// no context assumed.  We achieve this by never calling strtok() with
// a NULL first argument.  Pretty strange, this one.
//
#ifndef strtok_r
char *	strtok_r (char *__s, const char *__sep, char **__ppLast)
{
	char* result;
	if (__s == NULL)
	{
		__s = *__ppLast;
	}
	result = strtok(__s, __sep);
	if (result != NULL)
	{
		*__ppLast = result + strlen(result) + 1;
	}
	return result;
}
#endif
#endif

#if PRODUCT_IS(ILON)
#include <memLib.h>

//
// intLock
//
int 	intLock (void)
{
	return 0;
}

//
// intUnlock
//
int		intUnlock(int x)
{
	x;
	return 0;
}

//
// memPartInfoGet
//
// Sorry, not much of a simulation.
//
STATUS  memPartInfoGet (PART_ID	partId, MEM_PART_STATS * ppartStats)
{
	partId;
	memset(ppartStats, 0, sizeof(MEM_PART_STATS));
	return OK;
}

//
// ftruncate
//
int ftruncate( int handle, long size )
{
	return _chsize(handle, size);
}
#endif
// end

