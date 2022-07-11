/***************************************************************
 *  Filename: vxlTarget.c
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
 *  Description:  Target specific routines in the vxlLayer
 *
 *	DJ Duffy July 1999
 *
 ****************************************************************/

/*
//
// Taken from: /VxLayer/VxlTest.c 6
//
*/
/*
 * $Log: /Dev/ShareIp/vxlTarget.c $
 * 
 * 26    7/02/08 9:55a Fremont
 * Fixup for Glen's stack changes for the DCM
 * 
 * 25    6/24/08 7:56a Glen
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
 * 24    10/22/07 10:37a Bobw
 * EPR 46992
 * Remove OsalSuspendTask and OsalResumeTask from the OSAL.  On non-ilon,
 * change vxlInit to use a critical section initialized from task spawn.
 * On the i.lon stick with taskLock and taskUnlock.  On the i.lon these
 * are implemented by vxWorks, but on the non-ilon they depend on the
 * ability to suspend tasks, so on the i.lon WIN32 implment
 * taskLock/taskUnlock using WIN32 suspend/resume
 * 
 * 23    8/20/07 1:04p Bobw
 * EPR 45942
 * Reduced stack sizes for FTXL to 8192 bytes each stack (controlled by
 * lit int TaskPrioty.h), reducing stack size from 290K to 72K.  The
 * following shows the stack use:
 * 
 * Prio  Allocated         Used	% Used
 * 1     8192	                2068	25.24%
 * 2     8192	                1956	23.88%
 * 3     8192	                1212	14.79%
 * 4     8192	                2656	32.42%
 * 5     8192	                1972	24.07%
 * 6     8192	                2276	27.78%
 * 
 * 22    3/29/07 12:58p Bobw
 * More FTXL development
 * 
 * 21    3/26/07 2:36p Bobw
 * 
 * 20    8/01/06 4:26p Fremont
 * fix warnings
 * 
 * 19    5/19/04 8:11p Bobw
 * 
 * 18    5/19/04 9:03a Bobw
 * Improve tracing.  Add more granularity, and add L5 MIP message tracing
 * 
 * 17    4/07/04 4:38p Rjain
 * Added new VxLayer Dll. 
 * 
 * 16    9/27/02 7:51a Glen
 * EPRS FIXED: 26973 - Use new nothrow
 * 
 * 15    7/11/02 4:00p Vprashant
 * Added vxlDummPrintf which does not print anything
 * 
 * 14    7/09/02 1:50p Glen
 * Correct resource lock out condition that could occur on Windows.
 * 
 * 13    5/29/02 3:24p Fremont
 * taskLock() is implemented now for WIN32, so use it
 * 
 * 12    5/09/02 4:20p Fremont
 * protect vxlInit from simultaneous calls. THIS DOES NOT WORK ON WINDOWS!
 * 
 * 11    1/10/02 7:51p Vprashant
 * increased stack size
 * 
 * 10    10/15/01 1:55p Fremont
 * shorten task name
 * 
 * 9     3/14/00 9:49a Darrelld
 * Export routines
 * 
 * 8     3/13/00 5:31p Darrelld
 * Interface for LNS
 * 
 * 7     2/23/00 9:08a Darrelld
 * LNS 3.0 Merge
 * 
 * 16    12/20/99 12:09p Darrelld
 * forward declaration of vxlPost
 * 
 * 15    12/20/99 6:46a Darrelld
 * Improve synchronous performance
 * 
 * 14    11/17/99 3:38p Darrelld
 * EPR 15589
 * 
 * Defer vxlReports until after show.
 * Print urgent header.
 * Set print task to priority 0 for urgent trace.
 * 
 * 13    11/16/99 3:43p Darrelld
 * Fix memory leak on PC by statically allocating event ring
 * 
 * 12    11/16/99 12:25p Darrelld
 * Fixed compile warning on PC
 * 
 * 11    11/12/99 4:39p Darrelld
 * Add Newline to end of lines with none
 * 
 * 10    11/12/99 11:07a Darrelld
 * Changed vxlReportPrintf to vxlPrintf
 * Prettied up stats output
 * 
 * 9     11/12/99 9:43a Darrelld
 * Use vxlReportPrintf for rtrstat output
 * Make synchronous mode work
 * 
 * 8     11/11/99 10:59a Darrelld
 * Event logging
 * 
 * 7     11/02/99 5:19p Darrelld
 * Fix printing of router statistics
 * 
 * 6     10/29/99 2:01p Darrelld
 * Add time stamping option to vxlReportEvent.
 * Don't use logMsg because it prints an ugly header.
 * 
 * 5     10/22/99 1:13p Darrelld
 * rtrstat command
 * 
 * 4     8/19/99 11:06a Darrelld
 * Fix incorrect literal names
 * 
 * 3     8/19/99 9:51a Darrelld
 * Trace levels
 * 
 * 2     7/08/99 1:15p Darrelld
 * Use logMsg and add Urgent
 * 
 * 1     7/01/99 5:50p Darrelld
 * Target specific files
 * 
 */
/*
//////////////////////////////////////////////////////////////////////
*/

#include <vxWorks.h>
#include <tickLib.h>
#include <semLib.h>
#include <taskLib.h>
#include <sysLib.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <VxlTypes.h>
#include <vxlTarget.h>
#include <LtIpPlatform.h>
#include <string.h>
#include <LtaDefine.h>
#include <time.h>
#include "VxLayerDll.h" // VXLAYER_API
#include "VxLayer.h"
#include "taskPriority.h"
#if PRODUCT_IS(IZOT)
#include "FtxlApiInternal.h"
#endif

static int gnVxlReportEventLevel = VXL_REPORT_EVENTS;	//VXL_REPORT_URGENT;
static int gnVxlTraceTypesMask = 0;

static boolean gbVxlUseTimeStamps = false;
static int gVxlReportTask = ERROR;
static int gVxlLastTime = 0;
static boolean gbVxlSyncMode = false;
SEM_ID gVxlSem = null;
int		vxlReportTask();
int		gnEventIdxOut = 0;
int		gnEventIdxIn = 0;
int		gbInitDone = 0;

VXLREPORTCALLBACK*	gpVxlReportCallback;

#if PRODUCT_IS(FTXL)
#define RINGSIZE 200
#else
#define RINGSIZE 2000
#endif
#define LINEMAX 500
#define URGENT_HEADER "\n*Urgent* "

struct EventRing
{
	char events[RINGSIZE][LINEMAX];
	char temp[LINEMAX];
} *gcEventRing = null;
boolean gbInitializingEvents = false;
int gIntializingTaskId = ERROR;  /* Task ID of task doing the initialization. */


#ifdef WIN32
/* To avoid "memory leaks" the windows version statically allocates
// the event ring and fills it in.
*/
static struct EventRing winEvents;
#endif /* WIN32 */

#if defined(ILON_PLATFORM) && defined(__VXWORKS__)
// Put a couple of vxLayer function stubs here that are not needed
// on the iLON target, but get called.
BOOL vxlRunning()
{
	return TRUE;
}

void vxlShutdown()
{
}
#endif // iLON target

/* Set the callback function pointer
 */
VXLAYER_API
void	vxlSetReportCallback( VXLREPORTCALLBACK* pCB )
{	gpVxlReportCallback = pCB;
}

void	vxlPost();


VXLAYER_API
boolean vxlTimeStampEvents()
{
	return (gnVxlReportEventLevel & 0x8000) == 0x8000;
}

VXLAYER_API
boolean vxlGetUseTimeStamps()
{
	return gbVxlUseTimeStamps;
}

VXLAYER_API
int  vxlGetReportEventLevel()
{
	return gnVxlReportEventLevel;
}

VXLAYER_API
void    vxlSetReportEventLevel(int nLevel)
{
	gnVxlReportEventLevel = nLevel;
	vxlClearEventRing();
}

VXLAYER_API
int	vxlGetTraceTypes()
{
	return gnVxlTraceTypesMask;
}

VXLAYER_API
void	vxlSetTraceTypes(int typesMask)
{
	gnVxlTraceTypesMask = typesMask;
}

VXLAYER_API
void	vxlSetUseTimeStamps(BOOL bUseTimeStamps)
{
	gbVxlUseTimeStamps = bUseTimeStamps;
}

VXLAYER_API
void	vxlClearEventRing()
{
	gnEventIdxIn = 0;
	gnEventIdxOut = 0;
}

VXLAYER_API
void	vxlReportGo()
{
	gbInitDone = true;
	if (gVxlSem)
	{
		semGive(gVxlSem);
	}
}

VXLAYER_API
void	vxlReportSynchronous( BOOL bSynch )
{
	int		prio = 0;

	if ( gVxlReportTask != ERROR )
	{
		if ( bSynch )
		{
			gbVxlSyncMode = true;
			vxlClearEventRing();
			taskPriorityGet( taskIdSelf(), &prio );
			taskPrioritySet( gVxlReportTask, prio ? prio-1 : 0 );
		}
		else
		{
			/* wait for all printing to complete
			 */
			while ( gnEventIdxIn != gnEventIdxOut )
			{	
				vxlPost();
				taskPriorityGet( taskIdSelf(), &prio );
				taskPrioritySet( gVxlReportTask, prio ? prio-1 : 0 );
				taskDelay( 1 );
			}
			taskPrioritySet( gVxlReportTask, VXL_REPORT_TASK_PRIORITY );
			gbVxlSyncMode = false;
		}
	}
}



void	vxlInit()
{
	if (gVxlReportTask == ERROR)
	{
		int i;
		/* protect this section from simultaneous calls */

#ifdef __VXWORKS__
        /* VxWorks ILON cannot depend on CS used by lockVxlInit being created
         * before calls from multiple tasks to this funciton, so lock
         * out all tasks.
         */
		taskLock();  
#else
        /* Non-VxWorks products create the CS in taskSpawn, to ensure that it is
         * created before more than one task exists, so we can be assured
         * that only one instance will be created.  Non-ilons do not
         * have a "taskLock()".
         */
        lockVxlInit();
#endif
		/* Use a flag to ensure that only one task can be 
		 * initializing events.  The reason not to just lock
		 * the whole initialization section is that another
		 * thread might own a resource that is needed to execute
		 * this code (at least that's the case on Windows where
		 * malloc requires a global mutex).
		 */
		if (gVxlReportTask != ERROR || gbInitializingEvents)
		{
			/* someone else is already initializing events or */
			/* has already initialized events. */
			/* wait for them to complete and then return. */
#ifdef __VXWORKS__
			taskUnlock();
#else
            unlockVxlInit();
#endif
            if (gIntializingTaskId != taskIdSelf())
            {
                /* Wait, unless this is the task that is doing the init.
                 * Note that under non-ilon this could happen if the 
                 * taskSpawn of vxlReportTask failed, in which case
                 * taskSpawn would attempt to report the event.
                 */
			    while (gbInitializingEvents)
			    {
				    taskDelay(1);
			    }
            }
			return;
		}
		gbInitializingEvents = true;
        gIntializingTaskId = taskIdSelf();
#ifdef __VXWORKS__
	    taskUnlock();
#else
        unlockVxlInit();
#endif

#ifdef WIN32
		/* avoid memory leaks on the PC */
		gcEventRing = &winEvents;
#else
		gcEventRing = malloc(sizeof(struct EventRing));
#endif /* WIN32 */
		for (i=0; i<RINGSIZE; i++)
		{
			gcEventRing->events[i][0] = 0;
		}           
		gVxlSem = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
		gVxlReportTask = taskSpawn("Tracing", 
                                   VXL_REPORT_TASK_PRIORITY, 0, 
                                   VXL_REPORT_TASK_STACK_SIZE, vxlReportTask,
	  					 0,0,0,0,0, 0,0,0,0,0);
		gbInitializingEvents = false;
	}
}

int		vxlPrep(int* pOffset)
{
	int nIdx;

	/* Note that this is not thread-safe.  If it we were
	// to take a lock, we could change the real-time performance
	// of the system.  So, we accept that we may lose events in 
	// rare cases.
	*/
	nIdx = gnEventIdxIn++;
	if ( nIdx >= RINGSIZE-1 )
	{	gnEventIdxIn = 0;
		nIdx = RINGSIZE-1;
	}

	if (vxlGetUseTimeStamps())
	{
		int time = tickGet() * 1000 / sysClkRateGet();
		sprintf(gcEventRing->events[nIdx], "@%08d (+%05d):   ", time, time-gVxlLastTime);
		gVxlLastTime = time;
		*pOffset = 21;
	}
	else
	{
		*pOffset = 0;
	}

	return nIdx;
}

void	vxlPost()
{
	if (gnEventIdxIn == gnEventIdxOut)
	{
		/* The buffer is full!!  Up the priority of the printing task.
		 */
		taskPrioritySet(gVxlReportTask, 0);
	}
	if (gVxlSem)
	{
		semGive(gVxlSem);
	}
}

VXLAYER_API
void	vxlReportEvent( const char* fmt, ... )
{
	int		nIdx;
	int		nOffset;
	va_list	ap;

	if ( gnVxlReportEventLevel < VXL_REPORT_EVENTS )
	{	return;
	}

	vxlInit();

	nIdx = vxlPrep(&nOffset);

	va_start( ap, fmt );
	vsprintf( &gcEventRing->events[nIdx][nOffset], fmt, ap );
#if PRODUCT_IS(IZOT)
     // Re-direct all the report tracing to the Pylon log file
     APIDebug(&gcEventRing->events[nIdx][nOffset]);
#else
	vxlPost();
#endif
}

VXLAYER_API
void	vxlReportUrgent( const char* fmt, ... )
{
	va_list	ap;

	vxlInit();

	va_start( ap, fmt );
	vsprintf( gcEventRing->temp, fmt, ap);

	if ( gnVxlReportEventLevel >= VXL_REPORT_URGENT )
	{
		int		nIdx;
		int		nOffset;

		nIdx = vxlPrep(&nOffset);
		strcpy(&gcEventRing->events[nIdx][nOffset], URGENT_HEADER);
		nOffset += sizeof(URGENT_HEADER)-1;
		strcpy(&gcEventRing->events[nIdx][nOffset], gcEventRing->temp);
		/* Run report task at high priority to ensure output occurs.
		   This is urgent after all. */
		taskPrioritySet( gVxlReportTask, 0);

#if PRODUCT_IS(IZOT)
        // Re-direct all the urgent tracing to the Pylon log file
        APIDebug(gcEventRing->temp);
#else
		vxlPost();
#endif
	}

	/* Always post this urgent tracing to the event log.
	 */
#if !defined(WIN32) && PRODUCT_IS(ILON)
    LtIpEventLog(gcEventRing->temp);
#endif
}

VXLAYER_API
void	vxlTraceType(int traceType, const char* fmt, ... )
{
	int		nIdx;
	int		nOffset;
	va_list	ap;

	if ( !(traceType&gnVxlTraceTypesMask) )
	{	return;
	}

	vxlInit();

	nIdx = vxlPrep(&nOffset);

	va_start( ap, fmt );
	vsprintf( &gcEventRing->events[nIdx][nOffset], fmt, ap );
	vxlPost();
}

VXLAYER_API
BOOL vxlTracingType(int traceType)
{
	if ( (traceType&gnVxlTraceTypesMask) )
	{	
		return true;
	}
	else
	{
		return false;
	}
}

//
// vxlMemoryCheck
//
// This routine is used to check if a pointer is NULL.  If so, a memory failure has occurred and
// we log a message.
//
VXLAYER_API
void	vxlMemoryCheck(void* p)
{
	if (p == NULL)
	{
		vxlMemoryFailure();
	}
}

//
// vxlMemoryFailure
//
// This routine is called whenever code detects a low memory condition.  Assuming that when memory
// is low, this could be called a lot, this routine throttles the output so as not to overdo it.
//
VXLAYER_API
void	vxlMemoryFailure(void)
{
	static time_t nextPrintTime = 0;
	time_t now;

	time(&now);

	if (nextPrintTime == 0 ||
		now > nextPrintTime)
	{
		vxlReportUrgent("Memory allocation failure occurred");
	}
	time(&nextPrintTime);
	nextPrintTime += 60;
}

VXLAYER_API
void	vxlDummyPrintf( const char* fmt, ... )
{
	/* Will not print anything!!! */
}

VXLAYER_API
void	vxlPrintf( const char* fmt, ... )
{
	int nIdx;
	int nOffset;
	va_list ap;

	vxlInit();
	nIdx = vxlPrep(&nOffset);
	va_start( ap, fmt );
	// Print without time stamps
	vsprintf( gcEventRing->events[nIdx], fmt, ap );
	vxlPost();
}

int		vxlReportTask()
{
	int			len;
	char*		pLine;
	/* This task dumps events to the console.  It's purpose is to ensure that real-time critical
	// stuff is not unduly interrupted by "printfs".  The idea is to run this task at a low 
	// priority to avoid delaying real-time threads.  If this thread gets way behind though, then
	// output will be forced by jacking up its priority.
	*/
	while (vxlRunning())
	{
		semTake(gVxlSem, WAIT_FOREVER);
		while (gnEventIdxOut != gnEventIdxIn)
		{
			pLine = &(gcEventRing->events[gnEventIdxOut][0]);
			/* Add a new line to the end of a line if it doesn't have one. */
			len = strlen(pLine);
			if ( pLine[len-1] != '\n' )
			{	pLine[len] = '\n';
				pLine[len+1] = 0;
			}
			if ( gpVxlReportCallback )
			{	gpVxlReportCallback( pLine );
			}
			else
			{	printf("%s", pLine );
			}
			gnEventIdxOut++;
			if (gnEventIdxOut == RINGSIZE) gnEventIdxOut = 0;
		}
		if (!gbVxlSyncMode)
		{
			/* Now that we're empty, in case we got jacked up, restore it.
			 */
			taskPrioritySet(gVxlReportTask, VXL_REPORT_TASK_PRIORITY);
		}
	}
	semDelete(gVxlSem);
	return 0;
}
