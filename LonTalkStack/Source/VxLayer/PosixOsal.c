/*
 * File: PosixOsal.c
 *
 * Copyright © 2007-2022 Dialog Semiconductor
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
 * This file contains an  implementation of the eVNI Operating System 
 * Abstraction Layer (OSAL) targeted to use POSIX Threads, or p-threads.
 * 
 * This file needs to be ported if you use a different operation system.
 *
 */
 
#define __USE_UNIX98	// this actually has no effect because pthread.h includes <features.h> which undef's it!

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <signal.h>
#include <stdarg.h>
#include <unistd.h>
#include <assert.h>

///#include "Max.h"
#include "pthread.h"
#include "sched.h"

#if !(defined(ILON_PLATFORM) || defined(LONTALK_IP852_STACK_PLATFORM) || defined(IZOT_IP852_PLATFORM) || defined(IZOT_PLATFORM) || defined(LIFTBR_PLATFORM))
#include "ech_logger.h"
#endif

#if defined(DCX_PLATFORM)
# include "msm_msg.h"	// this is for DCX specific Osal variant
#endif

#include "vxWorks.h"
#include "VxlTypes.h"
#include "sntpcLib.h"

// See comment above.  Instead, just define the darn thing here:
extern int pthread_mutexattr_settype (pthread_mutexattr_t *__attr, int __kind)
     __THROW;

#if defined(WIN32)
#include "Windows.h"
#else
#include <unistd.h>
#include <time.h>
#define Sleep(msec) usleep(msec * 1000)
#endif

#define SUBPROCESS_MARKER	"/var/run/root%d.lck"
#define INSTANCE_MARKER		"/var/run/%s.lck"

#if defined(DCX_PLATFORM)
static void OsalGetProcessListForFile(OsalHandle *pHandle, const char *szFileName);
static void OsalReleaseSubprocessMarker(int pid);
#endif


// TEMP
#define OSAL_NOT_IMPLEMENTED ((OsalStatus)-1)
/******************************************************************************
 *                            Internal definitions 
 *****************************************************************************/

/*
 *  Typedef: TaskData
 *  Task specific data.
 *
 *  TaskData is allocated for each task.  This data is allocated by OsalCreateTask 
 *  and passed as the one and only parameter to the task entry point MyOsTaskStart.
 *  A pointer to this data is stored as task specific data using the key
 *  taskDataKey.  It is also passed back to the caller by OsalCreateTask.  The
 *  taskData includes a use count to keep track of these two uses, and
 *  will be freed either when the task dies (by MyOsTaskStart) or when
 *  the handl is closed (by OsalCloseTaskHandle).
 *
 */
typedef struct TaskData
{
    int         useCount;   
    int         taskIndex;
    OsalTaskId  taskId;
    pthread_t   pthread;
    OsalTaskEntryPointType pEntry;
} TaskData;

/*
 *  Typedef: OsalCv
 *  The internal structure used to represent a Binary Semaphore or Event.
 *
 *  Each OsalCV consists condition variable, the associated mutex and 
 *  state. 
 */
typedef struct OsalCv
{
    pthread_mutex_t     mutex;
    pthread_cond_t      cv;
    OsalBinarySemState  state;
} OsalCv;

/******************************************************************************
 *                            Internal Variables 
 *****************************************************************************/

    /* Current trace level.  See <OsalTrace>, <OsalSetTraceLevel> and 
     * <OsalGetTraceLevel> 
     */
static OsalTraceLevel osalTraceLevel = OSALTRACE_ERROR;

    /* Statistics for various resources. */
static OsalStatistics osStats =
{
    {0,0,0}, /* tasks */
    {0,0,0}, /* criticalSections */
    {0,0,0}, /* events */
    {0,0,0}, /* binarySemaphores */ 
};

/* This key is used to access the TaskData structure allocated for each OSAL 
 * task. 
 */
pthread_key_t taskDataKey;
int bTaskDataKeyCreated = 0; /* FALSE */
__int64 systemTimeOffset = 0;  // signed offset from actual system time in milleseconds

/******************************************************************************
 *                            Forward References
 *****************************************************************************/

    /* Create an OsalCv */
static OsalStatus OsalCreateCv(OsalHandle *pHandle, OsalBinarySemState initialState,
                               const char *title, OsalStatus genericError,
                               OsalResourceStats *pStat);
    /* Delete an OsalCv */
static OsalStatus OsalDeleteCv(OsalHandle *pHandle, const char *title,
                               OsalStatus genericError, OsalResourceStats *pStat);
    /* Wait for an OsalCv to be signaled. */
static OsalStatus OsalWaitForCv(OsalHandle handle, unsigned int ticks,
                                const char *title, OsalStatus genericError);

    /* Signal an OsalCv */
static OsalStatus OsalSignalCv(OsalHandle handle, const char *title,
                               OsalStatus genericError);

    /* Increment a resource numInUse statistic. */
static void IncOsalStat(OsalResourceStats *pStat);
    /* Decrement a resource numInUse statistic. */
static void DecOsalStat(OsalResourceStats *pStat);
    /* Clear a resource statistic */
static void ClearOsalStat(OsalResourceStats *pStat);

/* Convert ticks to a timespec. */
static void OsalGetTimeSpec(unsigned int ticks, struct timespec *pTimeSpec);

/* Utility routine to translate a trace level into a string. */
static const char *GetOsalTraceLevel(OsalTraceLevel level);
    /* Trace an OSAL event. */
static void OsalTrace(const char *eventName, void *id, int error, OsalStatus sts);

/*
 * ******************************************************************************
 * SECTION: Critical Section FUNCTIONS
 * ******************************************************************************
 *
 * This section contains the OSAL functions involving critical sections.
 *
 * Critical sections are used by a task to prevent access to a particular 
 * resource or code path by another task.  Unlike the other synchronization 
 * primitives, there is no timeout.  Critical sections also support "nesting" 
 * which allows a given task to enter a critical section more than once, so long 
 * as it "leaves" the critical section the same number of times as it "entered" 
 * the critical section.
 *
 * Note that there are two forms of this set of routines.  One form is for intraprocess only
 * and the other is for intraprocess and interprocess.  The latter form are called
 * Named Critical Sections.
 */

/*
 *  Function: OsalCreateCriticalSection
 *  Create a critical section.
 *
 *  Parameters:
 *  pHandle - pointer to receive a handle to the critical section.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  This function creates a critical section. 
 * 
 *  See also:
 *  <OsalDeleteCriticalSection>, <OsalEnterCriticalSection> and 
 *  <OsalLeaveCriticalSection>.
 */
OsalStatus OsalCreateCriticalSection(OsalHandle *pHandle)
{
    pthread_mutexattr_t attribute;
    OsalStatus sts = OSALSTS_SUCCESS;
    int err = 0;

    *pHandle = NULL;
    err = pthread_mutexattr_init(&attribute);
    if (err == 0)
    {
        err = pthread_mutexattr_settype(&attribute, PTHREAD_MUTEX_RECURSIVE_NP);
        if (err == 0)
        {
            pthread_mutex_t *pMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
            if (pMutex == NULL)
            {
                sts = OSALSTS_CSERROR;
                OsalTrace("OsalCreateCriticalSection Memory", pMutex, 0, sts);
            }
            else
            {
                err = pthread_mutex_init(pMutex, &attribute);
                if (err == 0)
                {
                    *pHandle = (OsalHandle)pMutex;
                    IncOsalStat(&osStats.criticalSections);
                }
                else
                {
                    free(pMutex);
                    pMutex = NULL;
                }
            }
        }
        pthread_mutexattr_destroy(&attribute);
    }
    if (sts == OSALSTS_SUCCESS && err != 0)
    {
        sts = OSALSTS_CSERROR;
    }
    OsalTrace("OsalCreateCriticalSection", *pHandle, 0, sts);

    return sts;
}

/*
 *  Function: OsalDeleteCriticalSection
 *  Delete a critical section.
 *
 *  Parameters:
 *  pHandle - pointer to a critical section handle.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  This function deletes a critical section and sets *pHandle to NULL. This 
 *  should not be called if any tasks may access the critical section again.
 *
 *  See also:
 *  <OsalCreateCriticalSection>.
 */
OsalStatus OsalDeleteCriticalSection(OsalHandle *pHandle)
{
    /* The critical section "handle" is really a pointer to a pthread_mutex_t. */
    pthread_mutex_t *pMutex = (pthread_mutex_t *)(*pHandle);
    OsalStatus sts = OSALSTS_SUCCESS;

    if (pMutex != NULL)
    {
        int err;

        /* Delete the operating system event. */
	    err = pthread_mutex_destroy(pMutex); 
        if (err != 0)
        {
            sts = OSALSTS_EVENT_ERROR;
        }
        else
        {
            DecOsalStat(&osStats.criticalSections);
        }
        OsalTrace("OsalDeleteCriticalSection", pMutex, err, sts);

        /* Free the CS structure. */
        free(pMutex);
        *pHandle = NULL;
    }
    return sts;
}

/*
 *  Function: OsalEnterCriticalSection
 *  Enter a critical section.
 *
 *  Parameters:
 *  handle - the handle of a critical section, returned by <OsalCreateCriticalSection>.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  Gain exclusive access to the critical section.  If another task has 
 *  entered the critical section wait until the task has left it. If the 
 *  calling task has already entered the critical section simply increment a 
 *  value to be decremented when leaving the critical section.
 *
 *  See also:
 *  <OsalCreateCriticalSection> and <OsalLeaveCriticalSection>.
 */
OsalStatus OsalEnterCriticalSection(OsalHandle handle)
{
    /* The critical section "handle" is really a pointer to a pthread_mutex_t. */
    pthread_mutex_t *pMutex = (pthread_mutex_t *)(handle);
    OsalStatus sts = OSALSTS_EVENT_ERROR;
    int err; 
        
    err = pthread_mutex_lock(pMutex);
    if (err == 0)
    {
        sts = OSALSTS_SUCCESS;
    }
    
    OsalTrace("OsalEnterCriticalSection", pMutex, err, sts);

    return sts;
}

/*
 *  Function: OsalLeaveCriticalSection
 *  Leave a critical section.
 *
 *  Parameters:
 *  handle - the handle of a critical section, returned by <OsalCreateCriticalSection>.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  Decrement the critical section count, and, if the count has become 0, let 
 *  go of the critical section so that another task can enter it.  This should 
 *  never be called by a task that does not own the critical section. 
 *  
 *  See also:
 *  <OsalCreateCriticalSection> and <OsalEnterCriticalSection>.
 */
OsalStatus OsalLeaveCriticalSection(OsalHandle handle)
{
    /* The critical section "handle" is really a pointer to a pthread_mutex_t. */
    pthread_mutex_t *pMutex = (pthread_mutex_t *)(handle);
    OsalStatus sts = OSALSTS_EVENT_ERROR;
    int err; 
        
    err = pthread_mutex_unlock(pMutex);
    if (err == 0)
    {
        sts = OSALSTS_SUCCESS;
    }
    OsalTrace("OsalLeaveCriticalSection", pMutex, err, sts);

    return sts;
}


/*
 *  Function: OsalCreateNamedCriticalSection
 *  Create a named critical section.
 *
 *  Parameters:
 *  pHandle - pointer to receive a handle to the critical section.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  This function creates a named critical section.  Note that it is up to the
 *  user to make sure that the name supplied is unique throughout the system.
 *
 *  See also:
 *  <OsalDeleteNamedCriticalSection>, <OsalEnterNamedCriticalSection> and
 *  <OsalLeaveNamedCriticalSection>.
 */
OsalStatus OsalCreateNamedCriticalSection(const char *szName, OsalHandle *pHandle)
{
	OsalNamedCriticalSection *p = (OsalNamedCriticalSection*)malloc(sizeof(OsalNamedCriticalSection));
    OsalStatus sts = OSALSTS_CSERROR;
    if (p != NULL)
    {
    	p->count = 0;
    	// Allocate a critical section local to this process.
    	if (OsalCreateCriticalSection(&p->lock) != OSALSTS_SUCCESS)
    	{
    		free(p);
    	}
    	else
    	{
			char fname[100];
			const char *pPath = "/tmp/namedcs/";
			mkdir(pPath, 0);
			system("chmod a+rwx /tmp/namedcs 2>/dev/null");
			snprintf(fname, sizeof(fname), "%s%s", pPath, szName);
			p->fd = open(fname, O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
			if (p->fd != -1)
			{
				// We don't want children to own this or else if the parent crashes the lock file will be tied up.
				OsalDisableInheritance(p->fd);
				*pHandle = (OsalHandle)p;
				sts = OSALSTS_SUCCESS;
			}
			char cmd[300];
			snprintf(cmd, sizeof(cmd), "chmod a+rw \"%s\" 2>/dev/null", fname);
			system(cmd);
    	}
	}
	return sts;
}

/*
 *  Function: OsalDeleteNamedCriticalSection
 *  Delete a critical section.
 *
 *  Parameters:
 *  pHandle - pointer to a named critical section handle.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  This function deletes a critical section and sets *pHandle to NULL. This
 *  should not be called if any tasks may access the critical section again.
 *
 *  See also:
 *  <OsalCreateNamedCriticalSection>.
 */
OsalStatus OsalDeleteNamedCriticalSection(OsalHandle *pHandle)
{
	OsalStatus sts = OSALSTS_SUCCESS;

	if (pHandle && *pHandle)
	{
		OsalNamedCriticalSection *p = *(OsalNamedCriticalSection **) pHandle;

		sts = OsalDeleteCriticalSection(&p->lock);
		close(p->fd);
		*pHandle = NULL;
		free(p);
	}
	return sts;
}

/*
 *  Function: OsalEnterNamedCriticalSection
 *  Enter a critical section.
 *
 *  Parameters:
 *  handle - the handle of a critical section, returned by <OsalCreateNamedCriticalSection>.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  Block until critical section can be entered
 *
 *  See also:
 *  <OsalCreateNamedCriticalSection> and <OsalEnterNamedCriticalSection>.
 */
OsalStatus OsalEnterNamedCriticalSection(OsalHandle handle)
{
	OsalStatus sts = OSALSTS_SUCCESS;
	OsalNamedCriticalSection *p = (OsalNamedCriticalSection *)handle;
	OsalEnterCriticalSection(p->lock);
	if (++p->count == 1)
	{
		sts = flock(p->fd, LOCK_EX) ? OSALSTS_EVENT_ERROR : OSALSTS_SUCCESS;
	}
    return sts;
}

/*
 *  Function: OsalLeaveNamedCriticalSection
 *  Leave a critical section.
 *
 *  Parameters:
 *  handle - the handle of a critical section, returned by <OsalCreateNamedCriticalSection>.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  See also:
 *  <OsalCreateNamedCriticalSection> and <OsalEnterNamedCriticalSection>.
 */
OsalStatus OsalLeaveNamedCriticalSection(OsalHandle handle)
{
	OsalStatus sts = OSALSTS_SUCCESS;
	OsalNamedCriticalSection *p = (OsalNamedCriticalSection *)handle;
	if (p->count-- == 1)
	{
		sts = flock(p->fd, LOCK_UN) ? OSALSTS_EVENT_ERROR : OSALSTS_SUCCESS;
	}
	if (p->count < 0)
	{
		// Someone did a leave without a corresponding enter.
		sts = OSALSTS_EVENT_ERROR;
		p->count = 0;
	}
	OsalLeaveCriticalSection(p->lock);
	// Note that releasing flock() doesn't guarantee that another process that's waiting will get it.
	// So, we release the processor just to make sure that if someone is waiting, they get a chance to grab
	// it.  This still is not a guarantee, but it seems to help a lot in practice.  It also assumes that
	// processes vying for the lock are at the same priority.
	usleep(1);
    return sts;
}

/*
 * ******************************************************************************
 * SECTION: Binary Semaphore FUNCTIONS
 * ******************************************************************************
 *
 * This section contains the OSAL functions involving binary semaphores.
 *
 * Like critical sections, binary semaphores are also used to control access 
 * to a resource.  Unlike critical sections, a binary semaphore must support a 
 * timeout, and can specify an initial condition.
 */

/*
 *  Function: OsalCreateBinarySemaphore
 *  Create a binary semaphore.
 *
 *  Parameters:
 *  pHandle - pointer to receive a handle to the binary semaphore.
 *  initialState - the initial state of the binary semaphore.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  This function creates a binary semaphore returning a handle to it.  The 
 *  initial state of the semaphore is set according to the <OsalBinarySemState>. 
 *
 *  If the initialState is OSAL_SEM_SET, the semaphore is "available" to the 
 *  first task that calls <OsalWaitForBinarySemaphore>.  If initialState is
 *  OSAL_SEM_CLEAR the semaphore is not available, and must be released by
 *  calling <OsalReleaseBinarySemaphore> before any task can gain access to
 *  it.
 * 
 *  See also:
 *  <OsalDeleteBinarySemaphore>, <OsalWaitForBinarySemaphore> and 
 *  <OsalReleaseBinarySemaphore>.
 */
OsalStatus OsalCreateBinarySemaphore(OsalHandle *pHandle,
                                     OsalBinarySemState initialState)
{
    return OsalCreateCv(pHandle, initialState, "OsalCreateBinarySemaphore",
                        OSALSTS_BSEM_ERROR, &osStats.binarySemaphores);
}

/*
 *  Function: OsalDeleteBinarySemaphore
 *  Delete a binary semaphore.
 *
 *  Parameters:
 *  pHandle - pointer to a binary semaphore .
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  This function deletes a binary semaphore and sets *pHandle to NULL. This 
 *  should not be called if any tasks may access the binary semaphore again.
 * 
 *  See also:
 *  <OsalCreateBinarySemaphore>
 */
OsalStatus OsalDeleteBinarySemaphore(OsalHandle *pHandle)
{
    return OsalDeleteCv(pHandle, "OsalDeleteBinarySemaphore",
                        OSALSTS_BSEM_ERROR, &osStats.binarySemaphores);
}

/*
 *  Function: OsalWaitForBinarySemaphore
 *  Wait for a binary semaphore.
 *
 *  Parameters:
 *  handle - the handle of a binary semaphore, returned by <OsalCreateBinarySemaphore>.
 *  ticks - number of ticks to wait.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  Wait until the binary semaphore is in the signaled state or times out.  If 
 *  ticks is 0 and the binary semaphore is not available, this function returns 
 *  immediately with a return status of OSALSTS_TIMEOUT. If ticks is 
 *  <OSAL_WAIT_FOREVER>, never timeout.
 *
 *  See also:
 *  <OsalCreateBinarySemaphore> and <OsalReleaseBinarySemaphore>.
 */
OsalStatus OsalWaitForBinarySemaphore(OsalHandle handle, unsigned int ticks)
{
    return OsalWaitForCv(handle, ticks, "OsalWaitForBinarySemaphore",
                         OSALSTS_BSEM_ERROR);
}

/*
 *  Function: OsalReleaseBinarySemaphore
 *  Release a binary semaphore.
 *
 *  Parameters:
 *  handle - the handle of a binary semaphore, returned by <OsalCreateBinarySemaphore>.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  Release the binary semaphore, setting it to the signaled state.
 *
 *  See also:
 *  <OsalWaitForBinarySemaphore>.
 */
OsalStatus OsalReleaseBinarySemaphore(OsalHandle handle)
{
    return OsalSignalCv(handle, "OsalReleaseBinarySemaphore", 
                        OSALSTS_BSEM_ERROR);
}

/*
 * ******************************************************************************
 * SECTION: Event FUNCTIONS
 * ******************************************************************************
 *
 * This section contains the OSAL functions involving events.
 *
 * Events are used by tasks that are waiting for something to happen.  They 
 * also support a timeout.
 */

/*
 *  Function: OsalCreateEvent
 *  Create an event.
 *
 *  Parameters:
 *  pHandle - pointer to receive a handle to the event.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  This function creates an event returning a handle to it.  The initial 
 *  state is "not signaled".
 * 
 *  See also:
 *  <OsalDeleteEvent>, <OsalWaitForEvent> and 
 *  <OsalSetEvent>.
 */
OsalStatus OsalCreateEvent(OsalHandle *pHandle)
{
    return OsalCreateCv(pHandle, OSAL_SEM_CLEAR, "OsalCreateEvent",
                        OSALSTS_EVENT_ERROR, &osStats.events);
}

/*
 *  Function: OsalDeleteEvent
 *  Delete an event.
 *
 *  Parameters:
 *  pHandle - pointer to an event handle.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  This function deletes an event and sets *pHandle to NULL. This 
 *  should not be called if any tasks may access the critical section again.
 *
 *  See also:
 *  <OsalCreateEvent>.
 */
OsalStatus OsalDeleteEvent(OsalHandle *pHandle)
{
    return OsalDeleteCv(pHandle, "OsalDeleteEvent", 
                        OSALSTS_EVENT_ERROR, &osStats.events);
}

/*
 *  Function: OsalWaitForEvent
 *  Wait for an event.
 *
 *  Parameters:
 *  handle - the handle of an event, returned by <OsalCreateEvent>.
 *  ticks - number of ticks to wait.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  Wait until the critical section is set or times out.  If ticks is 0, 
 *  and the event is not set, return immediately with a return status of 
 *  OSALSTS_TIMEOUT. If ticks is <OSAL_WAIT_FOREVER>, never timeout.
 *
 *  See also:
 *  <OsalCreateEvent> and <OsalSetEvent>.
 */
OsalStatus OsalWaitForEvent(OsalHandle handle, unsigned int ticks)
{
    return OsalWaitForCv(handle, ticks, "OsalWaitForEvent", OSALSTS_EVENT_ERROR);
}

/*
 *  Function: OsalSetEvent
 *  Set an event.
 *
 *  Parameters:
 *  handle - the handle of an event, returned by <OsalCreateEvent>.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  Set an event to the signaled state.
 *
 *  See also:
 *  <OsalCreateEvent> and <OsalWaitForEvent>.
 */
OsalStatus OsalSetEvent(OsalHandle handle)
{
    return OsalSignalCv(handle, "OsalSetEvent", OSALSTS_EVENT_ERROR);
}

/*
 * ******************************************************************************
 * SECTION: timing FUNCTIONS
 * ******************************************************************************
 *
 * This section contains the OSAL functions involving timing.
 */

/*
 *  Function: OsalGetTickCount
 *  Get the current system tick count.
 *
 *  Returns:
 *  The system tick count.
 *
 *  Remarks:
 *  Returns the number of ticks since system startup.  The number of ticks per 
 *  second can be determined by calling <OsalGetTicksPerSecond>.
 */
OsalTickCount OsalGetTickCount(void)
{
	struct timespec now;
	
	/*
	 * do not use clock() -- unlike Win32, it only counts the execution time
	 * do not use gettimeofday() -- elapsed time can become incorrect if the RTC is changed
	 * clock_gettime() is probably the right one to use but seems to be off by 50% on x86 --
	 * need to test it on the ARM platform.
	 * On x86, CLOCK_PROCESS_CPUTIME_ID appears to return the elapsed time and not execution time.
	 * The behavior seems to be implementation specific, so be sure to test it on ARM.
	 * Also could try CLOCK_MONOTONIC -- but make sure it is correct even on a time change.
	 */
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec * 1000 + (now.tv_nsec / 1000000);
}

/*
 *  Function: OsalGetTicksPerSecond
 *  Get the number of ticks in a second.
 *
 *  Returns:
 *  The number of ticks in one second.
 *
 *  Remarks:
 *  Returns the number of ticks in one second.
 *
 *  See Also:
 *  <OsalGetTickCount>.
 */
OsalTickCount OsalGetTicksPerSecond(void)
{
    return 1000;
}

/*
 * ******************************************************************************
 * SECTION: tasking FUNCTIONS
 * ******************************************************************************
 *
 * This section contains the OSAL functions involving tasking.
 */

//
// REMINDER - DcmpMt.cpp uses OSAL functions for signalling but PTHREAD directly for tasking.
//
/*
 *  Function: MyOsTaskStart
 *  uC/OS-II task entry point for OSAL tasks.
 *
 *  Parameters:
 *  parm - the taskIndex passed into to <OsalCreateTask>
 *
 *  Remarks:
 *  This function is the uC/OS-II task entry point for OSAL tasks.  <OsalCreateTask> 
 *  uses this task entry point rather than the OsalTaskEntryPointType pointer
 *  because the function prototypes differ (the function prototype of 
 *  MyOsTaskStart is OS dependent), and to perform OS dependent cleanup 
 *  processing when the task is done.
 *
 *  See Also:
 *  <OsalCreateTask>.
 */
void *MyOsTaskStart( void *parm )
{
    TaskData *pTaskData = (TaskData *)parm;

    if (osalTraceLevel >= OSALTRACE_VERBOSE)
    {
        printf("Start task taskIndex = %d", pTaskData->taskIndex);
    }

    pthread_setspecific(taskDataKey, pTaskData);

    pTaskData->pEntry(pTaskData->taskIndex);

    DecOsalStat(&osStats.tasks);

    pthread_setspecific(taskDataKey, NULL);
    if (--pTaskData->useCount == 0)
    {
        free(pTaskData);
    }

    return NULL;
}

/*
 *  Function: OsalCreateTask
 *  Create an OSAL task.
 *
 *  Parameters:
 *  taskIndex - Internal ID managed by the OSAL stack.  The OSAL layer passes
 *   this as the one and only argument to the OsalTaskEntryPoint() callback.
 *  stackSize - Suggested size of the task in bytes.  Depending on the 
 *   operating system, this may have to be adjusted by the OSAL layer.
 *  abstractPriority - The relative priority of this task within the
 *   system.  The OSAL layer is free to adjust the priority as needed, but it 
 *   is best to keep the priorities in the same order.  Priorities are defined 
 *   in three ranges, high, medium and low, with smaller numbers representing 
 *   higher priorities.  The ranges are defined by 
 *   <OSAL_HIGH_ABSTRACT_PRIORITY_START>, <OSAL_MEDIUM_ABSTRACT_PRIORITY_START>
 *   and <OSAL_LOW_ABSTRACT_PRIORITY_START>.
 *  pHandle - Pointer to an operating system dependent handle used to identify 
 *   the task at a later time.  Returned by the OSAL layer.
 *  pTaskId - Pointer to an operating system dependent task ID that can be used 
 *  for tracing or debug purposes.  This may be the same as the OsalHandle, or 
 *  may be different.  Some operating systems use an ID in debug displays, but 
 *  a handle to access the task programmatically.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  This function is used to create an OSAL task.  The entry point is passed in.
 */
OsalStatus OsalCreateTask(OsalTaskEntryPointType pEntry, int taskIndex, int stackSize,
                          int abstractPriority, 
                          OsalHandle *pHandle, 
                          OsalTaskId *pTaskId)
{
    OsalStatus  sts = OSALSTS_CREATE_TASK_FAILED;
    int err = 0;
    pthread_attr_t attr;

     if (!bTaskDataKeyCreated)
    {
        err = pthread_key_create(&taskDataKey, NULL);
        if (err == 0)
        {
            bTaskDataKeyCreated = 1; /* TRUE */
        }
    }

    if (err == 0)
    {
        TaskData *pTaskData = (TaskData *)malloc(sizeof(*pTaskData));
        if (pTaskData != NULL)
        {
            pTaskData->taskIndex = taskIndex;
            /* Can't think of anything better to put here. */
            pTaskData->taskId = (OsalTaskId)taskIndex + 1;            
            pTaskData->pEntry = pEntry;

            /* Use count is 2, because both the task and the caller have references to it. */
            pTaskData->useCount = 2; 
            err = pthread_attr_init(&attr);
            if (err == 0)
            {
                err = pthread_create(&pTaskData->pthread, &attr, MyOsTaskStart, pTaskData);
                pthread_attr_destroy(&attr);
            }

            if (err == 0)
            {
                if (pHandle)
                    *pHandle = (OsalHandle)pTaskData;
                if (pTaskId)
                    *pTaskId = pTaskData->taskId;

        		// Indicate that this thread will not be joined on exit
        		pthread_detach(pTaskData->pthread);

                sts = OSALSTS_SUCCESS;
                IncOsalStat(&osStats.tasks);

                if (osalTraceLevel >= OSALTRACE_VERBOSE)
                {
                    printf("Create task TID = %d, priority = %d , stackSize=%u\n", 
                           taskIndex, abstractPriority, (unsigned)stackSize);
                }
            }
            else
            {
                if (osalTraceLevel >= OSALTRACE_ERROR)
                {
                    printf("OSTaskCreateExt TID = %d, priority = %d  failed - err=%d\n", 
                           taskIndex, abstractPriority,  err);
                }
                free(pTaskData);
            }                
        }
        else
        {
            if (osalTraceLevel >= OSALTRACE_ERROR)
            {
                printf("OsalCreateTask TID = %d, priority = %d, stackSize=%u Memory allocation FAILED\n", 
                        taskIndex, abstractPriority, (unsigned)stackSize);
            }
        }
    }
    return sts;
}

static int iOsalPreviousTaskIndex = 0;

int OsalGenerateTaskIndex(void)
{
    return ++iOsalPreviousTaskIndex;
}

int OsalGetPreviousTaskIndex(void)
{
    return iOsalPreviousTaskIndex;
}


/*
 *  Function: OsalCloseTaskHandle
 *  close a task handle.
 *
 *  Parameters:
 *  handle - The handle returned by <OsalCreateTask>. 
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  This function is used to close a task's handle just prior to the task
 *  exiting the system.
 *
 *  See Also:
 *  <OsalCreateTask>.
 */
OsalStatus OsalCloseTaskHandle(OsalHandle handle)
{
    TaskData *pTaskData = (TaskData *)handle;

    if (--pTaskData->useCount == 0)
    {
        free(pTaskData);
    }
    if (osalTraceLevel >= OSALTRACE_VERBOSE)
    {
        printf("OsalCloseTaskHandle task handle = %p\n", handle);
    }
    return OSALSTS_SUCCESS;        
}

/*
 *  Function: OsalSuspendTask
 *  Suspends a task.
 *
 *  Parameters:
 *  handle - The handle returned by <OsalCreateTask>. 
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  Make the task go to sleep.  The task can be restarted with <OsalResumeTask>.
 *
 *  See Also:
 *  <OsalResumeTask>.
 */
OsalStatus OsalSuspendTask(OsalHandle handle)
{
#if defined(TBD)
    OsalStatus  sts = OSALSTS_SUSPEND_ERROR;

    /* Treat the OsalHandle as uC/OS-II task priority, and suspend the task. */
    INT8U err = OSTaskSuspend((int)handle);
    if (err == OS_NO_ERR)
    {
        sts = OSALSTS_SUCCESS;
    }
    OsalTrace("OsalSuspendTask", handle, err, sts);
    return sts;
#else
    return OSAL_NOT_IMPLEMENTED;
#endif
}

/*
 *  Function: OsalResumeTask
 *  Resume a Suspended task.
 *
 *  Parameters:
 *  handle - The handle returned by <OsalCreateTask>. 
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  Resume task that has been put to sleep via <OsalSuspendTask>.
 *
 *  See Also:
 *  <OsalSuspendTask>.
 */
OsalStatus OsalResumeTask(OsalHandle handle)
{
#if defined(TBD)
    OsalStatus  sts = OSALSTS_RESUME_ERROR;
    
    /* Treat the OsalHandle as uC/OS-II task priority, and resume the task. */
    if (OSTaskResume((int)handle) == OS_NO_ERR)
    {
        sts = OSALSTS_SUCCESS;
    }
    OsalTrace("OsalResumeTask", handle, 0, sts);
    return sts;  
#else
    return OSAL_NOT_IMPLEMENTED;
#endif
}

/*
 *  Function: OsalGetTaskId
 *  Get the task ID of the current task.
 *
 *  Returns:
 *  The task ID of the current task.
 *
 *  Remarks:
 *  Return the task ID of the current task. Same as the value return by 
 *  <OsalCreateTask> via *pTaskId
 */
OsalTaskId OsalGetTaskId(void)
{
    TaskData *pTaskData = (TaskData *)pthread_getspecific(taskDataKey);
    if (pTaskData == NULL)
    {
		// We tolerate a task with no task ID.  This could occur if the task
		// wasn't created by OSAL but still used OSAL primitives (typically 
		// indirectly via OSAL tasks with APIs).  The assumption is that 0
		// is a safe indicator of a non-OSAL task.
        return 0;
    }
    else
    {
        return pTaskData->taskId;
    }
}

/*
 *  Function: OsalGetTaskIndex
 *  Get the task index of the current task.
 *
 *  Returns:
 *  The task index of the current task.
 *
 *  Remarks:
 *  Return the task index of the current task.  Same as the taskIndex value 
 *  specified in the call to <OsalCreateTask> that created the task.
 */
int OsalGetTaskIndex(void)
{
    TaskData *pTaskData = (TaskData *)pthread_getspecific(taskDataKey);
    if (pTaskData == NULL)
    {
        if (osalTraceLevel >= OSALTRACE_WARNING)
        {
            printf("OsalGetTaskIndex called on non-task\n");
        }
        return 0;
    }
    else
    {
        return pTaskData->taskIndex;
    }
}

OsalThreadId OsalCreateThread(OsalEntryPoint threadEntry, void* threadData)
{
     pthread_t tid;
     if (pthread_create(&tid, NULL, threadEntry, threadData) == 0)
     {
    	 pthread_detach(tid);
     }
     else
     {
#if !(defined(ILON_PLATFORM) || defined(LONTALK_IP852_STACK_PLATFORM) || defined(IZOT_IP852_PLATFORM) || defined(IZOT_PLATFORM) || defined(LIFTBR_PLATFORM))
    	 LogCritical("Cannot start OSAL thread %x: %s", (int)threadEntry, strerror(errno));
#endif

    	 // This should never occur.  If it does, it is unlikely the client can recover gracefully.
    	 // So, the process dies and MAX will recover it.
    	 exit(1);
     }

     return tid;
}

/*
 *  Function: OsalGetThreadId
 *  Get the thread ID of the current thread.
 *
 *  Returns:
 *  The thread ID of the current thread.
 *
 *  Remarks:
 *  Returns a unique value for the current thread regardless of whether it was
 *  created by OSAL primitives or not
 */
OsalThreadId OsalGetThreadId(void)
{
	return (OsalThreadId)syscall(SYS_gettid);
}
/*
 *  Function: OsalSleep
 *  Sleep for a specified number of ticks.
 *
 *  Parameters:
 *  ticks - The number of ticks to sleep 
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  Suspend the task for the specified number of clock ticks.
 *
 *  See Also:
 *  <OsalGetTickCount>.
 */
OsalStatus OsalSleep(int ticks)
{
    Sleep(ticks*1000/OsalGetTicksPerSecond());
    return OSALSTS_SUCCESS;    
}

#if defined(DCX_PLATFORM)

/*
 *  Function: OsalGetProcessId
 *  Return the ID of the current process
 */
OsalProcessId OsalGetProcessId()
{
/*
	char szPid[100];

	szPid[0] = 0;
	// Get the process location via proc file system
	readlink("/proc/self", szPid, sizeof(szPid)-1);

	return (OsalProcessId)atoi(szPid);
*/
	return getpid();
}


/*
 *  Function: OsalGetProcessName
 *  Return the name of the current process
 */
void OsalGetProcessName(char *szName, int maxLength)
{
	char *szSlash;
	memset(szName, 0, maxLength);

	// Get the name via proc file system
	readlink("/proc/self/exe", szName, maxLength-1);

	// Remove the full path if present
	szSlash = strrchr(szName, '/');
	if (szSlash)
	{
		strcpy(szName, szSlash+1);
	}

}

/*
 *  Function: OsalGetProcessKey
 *  Return the name of the current process
 */
void OsalGetProcessKey(char *szName, int maxLength)
{
	OsalGetProcessName(szName, maxLength);

	if (strstr(szName, "AppLauncher") != NULL)
	{
		// This is a FPM. Use the FPM name as the key
		FILE *f;

		f = fopen("/proc/self/cmdline", "r");
		if (!f)
		{
			LogPError("[OSAL] Cannot read /proc/self/cmdline");
			return; // failure
		}

		// format of cmdline is AppLauncher/0arg1/0arg2 ...
		// for a FPM, arg1 is the FPM name
		char c;
		int nullCount = 0;
		int count = 0;
		szName[0] = 0;
		while ((c = getc(f)) != EOF && count < (MAX_UNIQUE_PROCESS_KEY_SIZE - 1))
		{
			if (c == 0)
			{
				nullCount++;
				if (nullCount == 2)
					break;
			}
			else
			{
				if (nullCount == 1)
				{
					szName[count++] = c;
				}
			}
		}
		szName[count] = 0;

		if (strlen(szName) == 0)
		{
			LogPError("[OSAL] GetProcessKey cannot find the FPM name from /proc/self/cmdline");
		}
	}
}

/*
 *  Function: OsalGetInstanceMarker
 */
static void OsalGetInstanceMarker(char *szMarker)
{
	char exeName[128];

	// Get the process name
	OsalGetProcessName(exeName, sizeof(exeName));

	sprintf(szMarker, INSTANCE_MARKER, exeName);
}

/*
 *  Function: OsalEnsureSingleInstance
 *  Exit if another instance of this process is running.
 */
void OsalEnsureSingleInstance(void)
{
	char lockFile[256];
	int fd;

	OsalGetInstanceMarker(lockFile);

	fd = open(lockFile, O_CREAT);

	// If we can't open it or lock it, someone else is running so quit
	if (fd == -1 || flock(fd, LOCK_EX|LOCK_NB))
	{
		char exeName[256];
		OsalGetProcessName(exeName, sizeof(exeName));
		printf("\nProcess \"%s\" exiting: not solitary instance or subprocesses remain\n", exeName);
		exit(99);
	}
}

//
// OsalProcessExists
//
// Return 1 if a process exists else 0
//
int OsalProcessExists(OsalProcessId pid)
{
	FILE *pf;
	char proc[30];
	sprintf(proc, "/proc/%d/", pid);
	pf = fopen(proc, "r");
	if (pf)
	{
		fclose(pf);
	}
	return pf != NULL;
}

//
// OsalDeadProcessWait
//
// Wait for a process to exit
//
// Note: if the process being killed is attached to the console and the killing process also is, then
// this can time out.  For this reason, the timeout is fairly short.  Feel free to fix this if you know how.
//
OsalStatus OsalDeadProcessWait(OsalProcessId pid)
{
	OsalStatus sts = OSALSTS_TIMEOUT;

	int waitTime = 5000;	// See comment above.
	int sleepTime = 100;
	int attempts = waitTime/sleepTime;
	while (attempts--)
	{
		// In case we spawned this process, do a wait() to prevent a zombie
		waitpid(pid, NULL, WNOHANG);

		if (OsalProcessExists(pid))
		{
			OsalSleep(sleepTime);
		}
		else
		{
			sts = OSALSTS_SUCCESS;
			break;
		}
	}
	if (attempts < 0)
	{
		LogPError("[OSAL] Kill process timed out");
	}
	return sts;
}

//
// OsalKillProcess
//
// Kill a process
//
OsalStatus OsalKillProcess(OsalProcessId pid)
{
	OsalStatus sts = OSALSTS_SUCCESS;

	if (OsalProcessExists(pid))
	{
		LogPDebug("[OSAL] Kill process %d", pid);

		// Note that kill could fail for a number of reasons such as the process exited (since the check above!)
		// or the process is a zombie.  If you want to know whether the process really exited, you must call
		// OsalKillProcessWait().  Note the recommended algorithm is to kill all processes first and then check
		// for death.  That way, zombies can exit as a result of the parent's death.
		kill(pid, SIGKILL);
	}

	OsalReleaseSubprocessMarker(pid);

	return sts;
}

//
// OsalKillProcessList
//
// This is done is 2 passes.  First pass kill the processes and second pass confirms death.  This
// properly handles zombies by making sure the parent is killed before checking for the zombie being
// gone.
static OsalStatus OsalKillProcessList(OsalHandle h)
{
	OsalStatus sts = OSALSTS_SUCCESS;
	int pid;
	// Pass 1 - kill all
	while ((pid = OsalGetNextProcess(h)))
	{
		OsalKillProcess(pid);
	}
	// Pass 2 - confirm death
	OsalResetProcessList(h);
	while ((pid = OsalGetNextProcess(h)))
	{
		OsalStatus tmpSts = OsalDeadProcessWait(pid);
		if (tmpSts != OSALSTS_SUCCESS)
		{
			sts = tmpSts;
		}
	}
	OsalFreeProcessList(h);
	return sts;
}

/*
 *  Function: OsalKillPreviousInstance
 *
 *  Kill the previous instance of this process and its children based on the lock file as
 *  previously procured by OsalEnsureSingleInstance().
 */
void OsalKillPreviousInstance(void)
{
	char lockFile[256];
	OsalHandle h;

	OsalGetInstanceMarker(lockFile);

	OsalGetProcessListForFile(&h, lockFile);

	OsalKillProcessList(h);
}

//
// OsalKillProcessTree
//
// Kill a process and its subprocesses
//
OsalStatus OsalKillProcessTree(OsalProcessId pid)
{
	OsalHandle h;
	OsalStatus sts = OSALSTS_SUCCESS, tmpSts = OSALSTS_SUCCESS;

	OsalGetProcessList(&h, pid);

	// As currently implemented, pid will not be in the process list
	// if it is already dead or if it has not yet acquired the subprocess marker lock.
	// So, we have to clean up this one explicitly.  Ideally OsalGetProcessList()
	// would always return pid, but that's for another day.
	OsalReleaseSubprocessMarker(pid);

	// OsalKillProcessList() frees the handle "h"
	sts = OsalKillProcessList(h);

	// Clean up the process in case it has not already cleaned up above.
	OsalKillProcess(pid);

	if ((tmpSts = OsalDeadProcessWait(pid))!= OSALSTS_SUCCESS)
	{
		sts = tmpSts;
	}

	return sts ;
}

//
// OsalReleaseSubprocessMarker
//
void OsalReleaseSubprocessMarker(int pid)
{
	char file[1024];
	sprintf(file, SUBPROCESS_MARKER, pid);
	OsalDeleteFile(file);
}

//
// OsalMarkSubprocesses
//
// Call this to indicate that you want to be able to enumerate the subprocesses of a process
//
OsalStatus OsalMarkSubprocesses(void)
{
	char file[1024];
	int i;
	sprintf(file, SUBPROCESS_MARKER, OsalGetProcessId());

	// Try twice to handle failure case
	for (i=2; i>0; i--)
	{
		int fd = open(file, O_CREAT);

		// If we can't open it or lock it, then there are orphans with this process ID.  Kill them!
		if (fd == -1 || flock(fd, LOCK_EX|LOCK_NB))
		{
			if (fd != -1)
			{
				close(fd);
			}
			OsalKillProcessTree(OsalGetProcessId());
		}
		else
		{
			break;
		}
	}
	return i==0 ? OSALSTS_TASK_ERROR : OSALSTS_SUCCESS;
}


//
// OsalGetProcessListForFile(OsalHandle *pHandle, const char *szFileName)
//
// Get processes that have a file handle for the file "szFileName".
// If you call this, you must call OsalFreeProcessList().
//
static void OsalGetProcessListForFile(OsalHandle *pHandle, const char *szFileName)
{
	char cmd[1024];

	*pHandle = NULL;
	sprintf(cmd, "fuser %s 2>/dev/null", szFileName);
	FILE *fd = popen(cmd, "r");
	if (fd)
	{
		char response[1024];
		int *pPids = NULL;
		char *saveptr = NULL;
		int count = 0;
		const char *szDelim = " \t\n\r";
		// pPids is a count followed by a list of processes
		while (fgets(response, sizeof(response)-1, fd) != NULL)
		{
			char *szPid = strtok_r(response, szDelim, &saveptr);
			while (szPid != NULL)
			{
				int processPid = atoi(szPid);
				const int allocStep = 100;
				if (count%allocStep == 0)
				{
					int newSize = ((count/allocStep+1)*allocStep+2)*sizeof(int);
					pPids = (int *)realloc(pPids, newSize);
				}
				count++;
				pPids[count] = processPid;

				szPid = strtok_r(NULL, szDelim, &saveptr);
			}
		}
		if (pPids)
		{
			// First int used as index for enumeration
			pPids[0] = 0;
			// 0 terminates the list
			pPids[count+1] = 0;
			*pHandle = pPids;
		}
		pclose(fd);
	}
}

//
// OsalGetProcessList
//
// This facility only returns subprocesses if the rootPid called OsalMarkSubprocesses().
// You must call OsalFreeProcessList().
//
void OsalGetProcessList(OsalHandle *pHandle, OsalProcessId rootPid)
{
	char file[1024];

	sprintf(file, SUBPROCESS_MARKER, rootPid);
	OsalGetProcessListForFile(pHandle, file);
}

//
// OsalResetProcessList
//
// Call this to re-enumerate a process list.
//
void OsalResetProcessList(OsalHandle handle)
{
	int *pPids = (int *)handle;
	if (pPids)
	{
		*pPids = 0;
	}
}

//
// OsalFreeProcessList
//
// Must be called after calling OsalGetProcessList()
//
void OsalFreeProcessList(OsalHandle handle)
{
	if (handle)
	{
		free(handle);
	}
}

//
// OsalGetNextProcess
//
// You must call OsalGetProcessListForFile() to use this facility.
// Returns the next process ID or 0 to terminate
//
OsalProcessId OsalGetNextProcess(OsalHandle handle)
{
	int pid = 0;
	int *pPids = (int *)handle;
	if (pPids)
	{
		(*pPids)++;
		pid = pPids[*pPids];
	}
	return pid;
}

#endif

/*
 *  Function: OsalProcessIsDaemon()
 *  Check if a running process is a daemon (session leader is self, sid == -1 or sid == pid)
 *
 *  Parameters:
 *  pid of the process to check. if pid is 0, then use process' own pid.
 *
 *  Returns:
 *  1 (true) if session leader is it's own pid or sid can't be read, 0 (false) otherwise.
 *  If the same pid is supplied from the last call, use the cached value of isDaemon.
 *
 *  Remarks:
 *  This function may be used to check if a running process is running as a daemon
 */
int OsalProcessIsDaemon(OsalProcessId pid)
{
	static int prevPid = -1;
	static int isDaemon = 0;

	// if pid is not supplied, assume it's own pid
	if (pid == 0)
		pid = getpid();

	if (prevPid != pid)
	{
		OsalProcessId sid = getsid(pid);
		isDaemon = (sid == -1 || sid == pid);
	}

    return isDaemon;
}


/*
 * ******************************************************************************
 * SECTION: OSAL Debug FUNCTIONS
 * ******************************************************************************
 *
 * This section contains the OSAL functions used to support debugging.  These 
 * functions may be stubbed if desired.
 */

/*
 *  Function: OsalSetTraceLevel
 *  Set OSAL tracing level
 *
 *  Parameters:
 *  traceLevel - the desired OSAL tracing level.
 *
 *  Remarks:
 *  This function may be used to enable OSAL tracing at the desired level
 */
void OsalSetTraceLevel(OsalTraceLevel traceLevel)
{
    osalTraceLevel = traceLevel;
}

/*
 *  Function: OsalGetTraceLevel
 *  Get current OSAL tracing level
 *
 *  Returns:
 *  The current OSAL tracing level.
 *
 *  Remarks:
 *  This function may be used to enable OSAL tracing at the desired level
 */
OsalTraceLevel OsalGetTraceLevel(void)
{
    return osalTraceLevel;
}

/*
 *  Function: OsalPrintDebugString
 *  Trace a debug string
 *
 *  Parameters:
 *  string - A string to be displayed or logged.
 *
 *  Remarks:
 *  This function is called when it has any debug
 *  information to display to the console or send to a log.  In debug systems, 
 *  for example, this could be implemented with a printf.
 *
 */
void OsalPrintDebugString(const char *string)
{
    printf("%s", string);
}

/*
 *  Function: OsalGetLastOsError
 *  Get the last error from the operating system.
 *
 *  Returns:
 *  The last error from the operating system.
 *
 *  Remarks:
 *  This function is called when an OSAL function returns an error to format 
 *  a trace message.  This function should return the last error code from the 
 *  underlying operating system, or any other meaningful error.  Error tracing 
 *  is enabled by TBD.
 */
int OsalGetLastOsError(void)
{
    /* Not implemented. */
    return 0;
}

// We return a time for convenience in passing time to printf()
// We require a time for thread safety (ctime is not thread safe on Linux)
char* OsalGetCurrentTime(char *pBuf)
{
	time_t t;
	time(&t);
	ctime_r(&t, pBuf);
	// ctime_r always returns 25 characters ending with \n.  Exceedingly strange.  Remove it.
	pBuf[24] = '\0';
	return pBuf;
}

/*
 *  Function: OsalPrintError
 *  Print an error from the operating system.
 *
 *  Remarks:
 *  This function is called when an OS function returns an error in order to
 *  dump that error in text format.
 */
void OsalPrintError(int osError, char *szContext)
{
	if (osError)
	{
		OsalTime t;
		printf("%s (OS error %d - %s) at %s\n",
					szContext ? szContext : "An OS error has occurred",
					osError,
					strerror(osError),
					OsalGetCurrentTime(t));
	}
}

/*
 *  Function: OsalGetStatistics
 *  Get operating system statistics.
 *
 *  Parameters:
 *  pStatistics - pointer to receive statistics.
 *
 *  Remarks:
 *  Get operating system statistics.  
 */
OsalStatus OsalGetStatistics(OsalStatistics * const pStatistics)
{
    *pStatistics = osStats;
    return OSALSTS_SUCCESS;
}

/*
 *  Function: OsalClearStatistics
 *  Clear operating system statistics.
 */
void OsalClearStatistics(void)
{
    ClearOsalStat(&osStats.tasks);
    ClearOsalStat(&osStats.criticalSections);
    ClearOsalStat(&osStats.events);
    ClearOsalStat(&osStats.binarySemaphores);
}

/*
 * ******************************************************************************
 * SECTION: Support Utilities
 * ******************************************************************************
 *
 * This section contains internal supports utilities used in this OSAL port.
 */

/* 
 * Function: OsalCreateCv
 * Create a condition variable and associated components.
 *
 * Parameters:
 *  pHandle - pointer to receive a handle to the condition variable.
 *  initialState - indicates whether the variable is initally set or clear.
 *  title - text name of the object type.
 *  genericError - error code to return in case an OS error occurs.
 *  pStat - pointer to statitics for this object.
 *  
 */
static OsalStatus OsalCreateCv(OsalHandle *pHandle, OsalBinarySemState initialState,
                               const char *title, OsalStatus genericError,
                               OsalResourceStats *pStat)
{
    OsalStatus sts = genericError;
    int err = 0;
    OsalCv *pCv = (OsalCv *)malloc(sizeof(OsalCv));
    if (pCv != NULL)
    {
        pCv->state = initialState;
        err = pthread_mutex_init(&pCv->mutex, NULL);
        if (err == 0)
        {
            err = pthread_cond_init(&pCv->cv, NULL);
            if (err == 0)
            {
                sts = OSALSTS_SUCCESS;

                IncOsalStat(pStat);
            }
        }

        if (sts != OSALSTS_SUCCESS)
        {
            free(pCv);
            pCv = NULL;
        }
    }
    OsalTrace(title, pCv, 0, sts);

    /* The OsalHandle is really a pointer to the OsalCv. */
    *pHandle = (OsalHandle)pCv;
    return (sts);
}

/*
 *  Function: OsalDeleteCv
 *  Delete a condition variable and associated components.
 *
 *  Parameters:
 *  pHandle - pointer to the OsalHandle.
 *  title - text name of the object type.
 *  genericError - error code to return in case an OS error occurs.
 *  pStat - pointer to statitics for this object.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 *  Remarks:
 *  This function deletes an OsalCV and sets *pHandle to NULL. 
 * 
 */
static OsalStatus OsalDeleteCv(OsalHandle *pHandle, const char *title,
                               OsalStatus genericError, OsalResourceStats *pStat)
{
    OsalStatus sts = OSALSTS_SUCCESS;
    int err = 0;
    /* The OsalHandle is really a pointer to the OsalCv. */
    OsalCv *pCv = *((OsalCv **)pHandle);
    if (pCv != NULL)
    {
        err = pthread_mutex_destroy(&pCv->mutex);
        if (err == 0)
        {
            err = pthread_cond_destroy(&pCv->cv);
        }

        if (err != 0)
        {
            sts = genericError;
        }
        else
        {
            DecOsalStat(pStat);
        }
        *pHandle = NULL;
        OsalTrace(title, pCv, err, sts);
        free(pCv);
    }
    return sts;
}

/*
 *  Function: OsalWaitForCv
 *  Wait for a condition variable.
 *
 *  Parameters:
 *  handle - the handle of the OsalCv.
 *  ticks - number of ticks to wait.
 *  title - text name of the object type.
 *  genericError - error code to return in case an OS error occurs.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 */
static OsalStatus OsalWaitForCv(OsalHandle handle, unsigned int ticks,
                                const char *title, OsalStatus genericError)
{
    /* The OsalHandle is really a pointer to the OsalCv. */
    OsalCv *pCv = (OsalCv *)handle;
    OsalStatus sts = OSALSTS_SUCCESS;
    int err;

    if (pCv == NULL)
    {
        sts = genericError;
        // We're not going to suspend at all!  Avoid potential spin loops
        OsalSleep(1000);
    }
    else
    {
        err = pthread_mutex_lock(&pCv->mutex);
        if (err == 0)
        {
            int gotAbsTime = 0;
            struct timespec absTime;
            OsalTickCount startTime = 0;

            /* pthread_cond_wait and pthread_cond_timedwait can both return GOOD 
             * spuriously, so check the value.
             */
            while (pCv->state == OSAL_SEM_CLEAR && err == 0)
            {
                /* Not currently set - need to wait... */
                if (ticks == OSAL_WAIT_FOREVER)
                {
                    err = pthread_cond_wait(&pCv->cv, &pCv->mutex);  
                }
                else
                {            
                    if (!gotAbsTime)
                    {
                        gotAbsTime = 1;
                        startTime = OsalGetTickCount();
                        OsalGetTimeSpec(ticks, &absTime);
                    }
                    err = pthread_cond_timedwait(&pCv->cv, &pCv->mutex, &absTime); 
                    if (err == 0 && pCv->state == OSAL_SEM_CLEAR)
                    {
                        /* A spurious wakeup.  Don't go to sleep if its already timed out. */
                        OsalTickCount currentTime = OsalGetTickCount();
                        if (currentTime-startTime < ticks)
                        {
                            startTime = currentTime;
                        }
                        else
                        {
                            err = ETIMEDOUT;
                        }
                    }
                }
            }

            if (err == 0)
            {
                if (pCv->state != OSAL_SEM_SET)
                {   
                    if (osalTraceLevel >= OSALTRACE_ERROR)
                    {
                        printf("ERROR: Invalid state in OsalWaitForCv = %d\n", pCv->state);
                    }
                    pthread_cond_signal(&pCv->cv);
                    sts = genericError;
                }
                else
                {
                    pCv->state = OSAL_SEM_CLEAR;
                }
            }
            pthread_mutex_unlock(&pCv->mutex);
        }
        
        if (err == ETIMEDOUT)
        {
            sts = OSALSTS_TIMEOUT;
        }
        else if (err != 0)
        {
            sts = genericError;
        }
        OsalTrace(title, pCv, err, sts);
    }

    return sts;
}

/*
 *  Function: OsalSignalCv
 *  Signal a condition variable.
 *
 *  Parameters:
 *  handle - the handle of the OsalCv.
 *  title - text name of the object type.
 *  genericError - error code to return in case an OS error occurs.
 *
 *  Returns:
 *  <OsalStatus>.
 *
 */
static OsalStatus OsalSignalCv(OsalHandle handle, const char *title,
                               OsalStatus genericError)
{
    /* The OsalHandle is really a pointer to the OsalCv. */
    OsalCv *pCv = (OsalCv *)handle;
    OsalStatus sts = OSALSTS_SUCCESS;
    int err;

    if (pCv == NULL)
    {
        sts = genericError;
    }
    else
    {
        err = pthread_mutex_lock(&pCv->mutex);
        if (err == 0)
        {
            pCv->state = OSAL_SEM_SET;
            err = pthread_cond_signal(&pCv->cv);  
            pthread_mutex_unlock(&pCv->mutex);
        }
        
        if (err != 0)
        {
            sts = genericError;
        }
        OsalTrace(title, pCv, err, sts);
    }
    return sts;
}

int OsalIsRealTimeClockOk(void)
{
	struct tm t;
	time_t		tnow;

	tnow = time(&tnow);
	gmtime_r(&tnow, &t);
	// For DCD application purposes, we declare years before 2010 not ok.  The reason is that these applications
	// need to know whether it is safe to propagate the time to meters.  In ANSI-IP architecture where the time comes
	// from the meter, we don't have a great way to designate this so we just use times before 2010 as the
	// indication.  When MDM gets the time from the meter, a time of 2070 (indicating clock error) is mapped to 2002.
	return t.tm_year >= 110;
}

/*
 *  Function: OsalGetTimeSpec
 *  Convert ticks to an absolute timespec.
 *
 *  Parameters:
 *  ticks - clock ticks to delay
 *  pTimeSpec - pointer to output timespec.
 *
 *  Remarks:
 *  This function is used to convert clock ticks to a timespec.
 */
static void OsalGetTimeSpec(unsigned int ticks, struct timespec *pTimeSpec)
{
    time_t msec = ticks*1000/OsalGetTicksPerSecond();
	int nsec;

    clock_gettime(CLOCK_REALTIME, pTimeSpec);

    pTimeSpec->tv_sec += (long)(msec/1000);
	nsec = pTimeSpec->tv_nsec + (long)((msec % 1000) * 1000 * 1000);
	pTimeSpec->tv_sec += nsec /(1000*1000*1000);
    pTimeSpec->tv_nsec = nsec % (1000*1000*1000);
}

/*
 *  Function: IncOsalStat
 *  Increment a resources numInUse statistic.
 *
 *  Parameters:
 *  pStat - pointer to the resources statistics
 */
static void IncOsalStat(OsalResourceStats *pStat)
{
    if (++pStat->numInUse > pStat->maxUsed)
    {
        /* Update new maxUsed */
        pStat->maxUsed = pStat->numInUse;
    }

    /* Increment numCreated, but cap at 0xffffffff */ 
    if (pStat->numCreated != 0xffffffff)
    {
        pStat->numCreated++;
    }
}

/*
 *  Function: DecOsalStat
 *  Decrement a resource numInUse statistic.
 *
 *  Parameters:
 *  pStat - pointer to the resources statistics
 */
static void DecOsalStat(OsalResourceStats *pStat)
{
    if (pStat->numInUse != 0)
    {
        pStat->numInUse--;
    }
}

/*
 *  Function: ClearOsalStat
 *  Clear a resource statistic, preserving current use counts
 *
 *  Parameters:
 *  pStat - pointer to the resources statistics
 *
 *  Remarks:
 *  Clearing the statistic does not change the number currently in use, but does reset
 *  the numCreated and a maxUsed.
 */
static void ClearOsalStat(OsalResourceStats *pStat)
{
    /* Reset the maxUsed to the current number in use. */
    pStat->maxUsed = pStat->numInUse;
    /* Reset the numCreated to the current number in use. */
    pStat->numCreated = pStat->numInUse;
}

/*
 *  Function: GetOsalTraceLevel
 *  Utility routine to translate a trace level into a string. 
 * 
 *  Parameters:
 *  level - the OSAL trace level;
 *
 *  Returns:
 *  A character string corresponding to the trace level.
 */
static const char *GetOsalTraceLevel(OsalTraceLevel level)
{
    const char *s = "";
    switch (level)
    {
        case OSALTRACE_ERROR:
            s = "OSAL ERROR";
            break;
        case OSALTRACE_WARNING:
            s = "OSAL WARNING";
            break;
        case OSALTRACE_VERBOSE:
            s = "OSAL INFO";
            break;
        default:
            s = "";            
			break;
    };      
    return s;
}

/*
 *  Function: OsalTrace
 *  Trace an OSAL event. 
 *
 *  Parameters:
 *  eventName - name of the event for display purposes.
 *  id - pointer used to identify the event.
 *  error - the OS error number.
 *  sts - the OsalStatus
 *
 *  Remarks:
 *  The OS error (error) and OsalStatus (sts) are used to determine the trace level.
 *  The trace level is then used to determine whether to perform the trace or not.
 */
static void OsalTrace(const char *eventName, void *id, int error, OsalStatus sts)
{
    OsalTraceLevel level;
    
    if (error == 0 && sts == OSALSTS_TIMEOUT)
    {
        error = ETIMEDOUT;
    }

    if (error == ETIMEDOUT)
    {
        level = OSALTRACE_WARNING;
    }
    else if (error || sts != 0)
    {
        level = OSALTRACE_ERROR;
    }
    else
    {
        level = OSALTRACE_VERBOSE;
    }

    if (level <=  osalTraceLevel)
    {
        printf("[%s]%s(%p) ", GetOsalTraceLevel(level), eventName, id);
        if (error == ETIMEDOUT)
        {
            printf("p-thread TIMEOUT\n");
        }
        else if (error)
        {
            printf("p-thread err=%d\n", error);
        }
        else if (sts != 0)
        {
            printf("sts=%d\n", sts);
        }
        else
        {
            printf("\n");
        }
    }  
}

/*
 *  Function: OsalClockGetTime
 *  Set the system offset, based on the new time
 * 
 *  Parameters:
 *  clock - clockID 
 *  pTimeSpec - pointer to output timespec.
 *
 *  Remarks:
 *  It's not a good idea that users adjust the system clock. Instead, just set an offset.
 *  OsalClockSetTime only supports CLOCK_REALTIME. It returns error if the clockID is not
 *  a CLOCK_REALTIME.
 */
int OsalClockSetTime(clockid_t clock, struct timespec *pTimeSpec)
{
    int		err = -1;

	if ( clock == CLOCK_REALTIME )
	{
        __int64 currentTime;
        __int64 newTime;
#if defined(WIN32)
        SYSTEMTIME systemTime;
        struct tm  tm_time;
  
        GetLocalTime(&systemTime);
        memset(&tm_time, 0, sizeof(tm_time));

        /* Get system time in "tm" format so we can get the number of seconds from
         * the epoch.
         */
        tm_time.tm_isdst = -1; /* Let system decide whether daylight savings time is in effect. */
        tm_time.tm_year = systemTime.wYear - 1900;
        tm_time.tm_mon  = systemTime.wMonth - 1;
        tm_time.tm_mday = systemTime.wDay;
        tm_time.tm_hour = systemTime.wHour;
        tm_time.tm_min  = systemTime.wMinute;
        tm_time.tm_sec  = systemTime.wSecond;
        // current time in milisecs
        currentTime = ((long)mktime(&tm_time) * 1000) + (long)(systemTime.wMilliseconds);
#else
        struct timespec localSeconds;

        clock_gettime(CLOCK_REALTIME, &localSeconds);
        // current time in milisecs
        currentTime = (localSeconds.tv_sec * 1000) + (localSeconds.tv_nsec / 1000000);
#endif
        newTime = pTimeSpec->tv_sec*1000 + (pTimeSpec->tv_nsec / 1000000);
        systemTimeOffset = newTime - currentTime;

		err = 0;
	}
	return err;
}

/*
 *  Function: OsalClockGetTime
 *  Returns the current timespec value for the specified clock.
 * 
 *  Parameters:
 *  clock - clockID (CLOCK_REALTIME or CLOCK_MONOTONIC)
 *  pTimeSpec - pointer to output timespec.
 *
 */
int OsalClockGetTime(clockid_t clock, struct timespec *pTimeSpec)
{
    int err = 0;     //  return 0 for succes, -1 

#if defined(WIN32)
    // The IP852 code uses the POSIX clock routines, which are supported by vxWorks/linux
    // but not on Win32, so we write them here for windows.
    // Note that these functions return a calculated UTC based on the current system
    // time and an offset.  This allows the SNTP client to correctly adjust the
    // IP852 time without updating the PC time (which requires elevated priveleges
    // on WIN7 (and probably VISTA).
    if (clock == CLOCK_REALTIME)
    {
        SYSTEMTIME systemTime;
        struct tm  tm_time;

        GetLocalTime(&systemTime);
        memset(&tm_time, 0, sizeof(tm_time));

        /* Get system time in "tm" format so we can get the number of seconds from
         * the epoch.
         */
        tm_time.tm_isdst = -1; /* Let system decide whether daylight savings time is in effect. */
        tm_time.tm_year = systemTime.wYear - 1900;
        tm_time.tm_mon  = systemTime.wMonth - 1;
        tm_time.tm_mday = systemTime.wDay;
        tm_time.tm_hour = systemTime.wHour;
        tm_time.tm_min  = systemTime.wMinute;
        tm_time.tm_sec  = systemTime.wSecond;

        pTimeSpec->tv_sec = (long)mktime(&tm_time);
        pTimeSpec->tv_nsec = (long)(systemTime.wMilliseconds * 1000 * 1000);
    }
    else
    {
        OsalTickCount tickCount = GetTickCount();
        pTimeSpec->tv_sec = tickCount/1000;
        pTimeSpec->tv_nsec = (tickCount % 1000) * 1000*1000;
    }
#else
    err = clock_gettime(clock, pTimeSpec);    
#endif
    if (clock == CLOCK_REALTIME)
    {
        // Adjust the value with the systemTimeOffset
        long nsec;
		pTimeSpec->tv_sec += (time_t)(systemTimeOffset/1000);
        nsec = pTimeSpec->tv_nsec + (long)((systemTimeOffset % 1000) * 1000 * 1000);
        pTimeSpec->tv_sec += nsec /(1000*1000*1000);
        pTimeSpec->tv_nsec = nsec % (1000*1000*1000);
    }

    return err;
}




/*
 * ******************************************************************************
 * SECTION: Execute Command Line FUNCTIONS
 * ******************************************************************************
 *
 * This section contains the OSAL functions involving execution of command line.
 */

static boolean bExecCmdTrace = false;
static unsigned int waitPidCalls = 0;

static int	ExecuteCmd(const char* cmd, va_list argptr)
{
	OsalTime t;
	pid_t	childPid = 0;
	int		rc = 0;
	int		status = 0;

	childPid = fork();
	if (childPid != 0)
	{
		int				loops = 10;

		if (bExecCmdTrace)
			printf("[%s] OsalExecuteCmd> Command PID %d [# %u]\n", OsalGetCurrentTime(t), childPid, waitPidCalls);

		// We are in the parent process. Wait for the command to excute & report its result.
		do
		{
			OsalTime t;
			waitPidCalls++;
			{
				OsalSleep(10);	// backoff
				rc = waitpid(childPid, &status, 0);
			}
			if (bExecCmdTrace)
				printf("[%s] OsalExecuteCmd> waitpid loop %d Result: %d Status: %d [%02x]\n", OsalGetCurrentTime(t), loops, rc, status, WEXITSTATUS(status));
		} while (rc==childPid && !WIFEXITED(status) && --loops);

		if (bExecCmdTrace)
			printf("[%s] OsalExecuteCmd> Command %s Result: %d Status: %d [%02x]\n", OsalGetCurrentTime(t), cmd, rc, status, WEXITSTATUS(status));
		status = WEXITSTATUS(status);
	}
	else
	{
		char* 		args[EXCMD_MAX_ARGS];
		int			argc = 0;
		char*		pArg = NULL;

		if (bExecCmdTrace)
			printf("[%s] OsalExecuteCmd> Exec: %s", OsalGetCurrentTime(t), cmd);

		// We are in the child process. We now want to exec the specified command
		args[argc++] = (char*)cmd;

		// Also include any additional configured arguments.
		while (argc < EXCMD_MAX_ARGS-1)
		{
			pArg = va_arg(argptr, char*);
			args[argc++] = pArg;
			if (pArg == NULL)
				break;
			if (bExecCmdTrace)
				printf(" %s", pArg);
		}
		args[argc] = NULL;

		if (bExecCmdTrace)
			printf(" \n");

		// Launch requested process. Following call does not return.
		execvp(args[0], args);

		printf("Error launching: %s\n", args[0]);
		exit(OSALSTS_EXEC_CMD_FAILED);
	}

	return status;
}


/*
 *  Function: OsalExecCmd
 *  Execute a command with variable list of arguments.  Last argument in the list
 *  MUST be NULL.
 *
 *  Returns: 0 if successful, otherwise exit status of the executed command.
 *  Status of command execution.
 *
 *  Remarks:
 *
 */
int OsalExecCmd(const char* cmd, ...)
{
	int	sts = 0;
	va_list argptr;

	va_start(argptr, cmd);

	sts = ExecuteCmd(cmd, argptr);

	va_end(argptr);

	return sts;
}

// Returns pointer to string containing output of command. Uses caller's buffer for output
static const char* ExecCmdEx(char * buf, int len, const char* cmd, va_list argptr)
{
	OsalTime	t;
	char		cmdBuf[100];
	FILE*		pCmdOut = NULL;
	const char* pArg;

	memset(buf, 0, len);
	memset(cmdBuf, 0, sizeof(cmdBuf));

	strncpy(cmdBuf, cmd, sizeof(cmdBuf));
	cmdBuf[sizeof(cmdBuf)-1] = '\0';

	while ((pArg = va_arg(argptr, char*)) != NULL)
	{
		if (strlen(cmdBuf)+strlen(pArg) > sizeof(cmdBuf)-2)
		{
			assert(0);
			return buf;
		}
		strcat(cmdBuf, " ");
		strcat(cmdBuf, pArg);
	}

	waitPidCalls++;
	if (bExecCmdTrace)
		printf("[%s] OsalExecCmdEx> %s [# %u]\n", OsalGetCurrentTime(t), cmdBuf, waitPidCalls);

	pCmdOut = popen(cmdBuf, "r");

	if (pCmdOut != NULL)
	{
		fread(buf, 1, len-1, pCmdOut);
		buf[len-1] = 0;
		fclose(pCmdOut);
	}
	if (bExecCmdTrace)
		printf("[%s] OsalExecCmdEx: cmd: %s result: %s\n", OsalGetCurrentTime(t), cmdBuf, buf);

	return buf;
}

/*
 *  Function: OsalExecCmdEx
 *  Execute a command with variable list of arguments.  Last argument in the list
 *  MUST be NULL.
 *
 *  Returns:
 *  Output string from command execution.
 *
 *  Remarks:
 *
 */
const char* OsalExecCmdEx(char * buf, int len, const char* cmd, ...)
{
	va_list argptr;

	va_start(argptr, cmd);

	ExecCmdEx(buf, len, cmd, argptr);

	va_end(argptr);

	return buf;
}

#if defined(DCX_PLATFORM)
//
// OsalKillAll()
//
// Kill all processes that have a specified file open.  The file may be the exe name
//
void OsalKillAll(const char *szFilename)
{
	OsalHandle osalHandle;

	OsalGetProcessListForFile(&osalHandle, szFilename);
	OsalKillProcessList(osalHandle);
}
#endif

#if !defined(DCX_PLATFORM)
void OsalDisableInheritance(int fd)
{
	fcntl(fd, F_SETFD, FD_CLOEXEC);
}

#endif
