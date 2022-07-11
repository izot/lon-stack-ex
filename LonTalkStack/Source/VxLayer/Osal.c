#include "Osal.h"
#include <windows.h>
#include <stdio.h>
#include <time.h>

/*
 *  Typedef: TaskData
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
    HANDLE      handle;
    int         useCount;   
    int         taskIndex;
    OsalTaskId  taskId;
    OsalTaskEntryPointType pEntry;
} TaskData;

    /* Statistics for various resources. */
static OsalStatistics osStats =
{
    {0,0,0}, /* tasks */
    {0,0,0}, /* criticalSections */
    {0,0,0}, /* events */
    {0,0,0}, /* binarySemaphores */ 
};

__int64 systemTimeOffset = 0;  // signed offset from actual system time in milleseconds

/******************************************************************************
 *                            Forward References
 *****************************************************************************/

    /* Increment a resource numInUse statistic. */
static void IncOsalStat(OsalResourceStats *pStat);
    /* Decrement a resource numInUse statistic. */
static void DecOsalStat(OsalResourceStats *pStat);
    /* Clear a resource statistic */
static void ClearOsalStat(OsalResourceStats *pStat);
    /* Display the used and unused stack size of the specified task. */
static void DisplayStackStatistics(int priority);


/******************************************************************************
 *
 *                            Critical Sections
 *
 *****************************************************************************/
OsalStatus OsalCreateCriticalSection(OsalHandle *pHandle)
{
    OsalStatus sts;
	CRITICAL_SECTION *pCs = (CRITICAL_SECTION *)malloc(sizeof(CRITICAL_SECTION));
    if (pCs == NULL)
    {
        sts = OSALSTS_CSERROR;
    }
    else
    {
	    InitializeCriticalSection(pCs);
        *pHandle = (OsalHandle)pCs;
        sts = OSALSTS_SUCCESS;
        IncOsalStat(&osStats.criticalSections);
    }
	return sts;
}

OsalStatus OsalDeleteCriticalSection(OsalHandle *pHandle)
{
    if (*pHandle != NULL)
    {
	    DeleteCriticalSection((CRITICAL_SECTION *)*pHandle);
	    free(*pHandle);
        *pHandle = NULL;
        DecOsalStat(&osStats.criticalSections);
    }
    return OSALSTS_SUCCESS;
}

OsalStatus OsalEnterCriticalSection(OsalHandle handle)
{
	EnterCriticalSection((CRITICAL_SECTION *)handle);
    return OSALSTS_SUCCESS;
}

OsalStatus OsalLeaveCriticalSection(OsalHandle handle)
{
	LeaveCriticalSection((CRITICAL_SECTION *)handle);
    return OSALSTS_SUCCESS;
}

/******************************************************************************
 *
 *                            Binary Semaphores
 *
 *****************************************************************************/
OsalStatus OsalCreateBinarySemaphore(OsalHandle *pHandle, OsalBinarySemState initialState)
{
    OsalStatus sts;
    HANDLE handle;
    int	nInitCount =  initialState == OSAL_SEM_SET ? 1 : 0;
	// no security, init count, max count 1, no name
    handle = CreateSemaphore( NULL, nInitCount, 1, NULL );
    if (handle == NULL)
    {
        sts = OSALSTS_BSEM_ERROR;
    }
    else
    {
        *pHandle = handle;
        sts = OSALSTS_SUCCESS;
        IncOsalStat(&osStats.binarySemaphores);
    }
	return (sts);
}

OsalStatus OsalDeleteBinarySemaphore(OsalHandle *pHandle)
{
    if (*pHandle != NULL)
    {
	    CloseHandle(*pHandle);
        *pHandle = NULL;
        DecOsalStat(&osStats.binarySemaphores);
    }
    return OSALSTS_SUCCESS;
}

OsalStatus OsalWaitForBinarySemaphore(OsalHandle handle, unsigned int ticks)
{
	OsalStatus oslSts;
    int waitSts = WaitForSingleObject(handle, ticks);
	if (waitSts == WAIT_OBJECT_0)
	{
		oslSts = OSALSTS_SUCCESS;		
	}
	else if (waitSts == WAIT_TIMEOUT)
	{
		oslSts = OSALSTS_TIMEOUT;
	}
    else
    {
        oslSts = OSALSTS_BSEM_ERROR;
    }
	return oslSts;
}

OsalStatus OsalReleaseBinarySemaphore(OsalHandle handle)
{
	OsalStatus sts;
	LONG	prevCount = 0;

	if (ReleaseSemaphore( handle, 1, &prevCount))
	{
		sts = OSALSTS_SUCCESS;		
	}
	else
	{
		sts = OSALSTS_BSEM_ERROR;
	}
	return sts;
}

/******************************************************************************
 *
 *                                  Events
 *
 *****************************************************************************/
OsalStatus OsalCreateEvent(OsalHandle *pHandle)
{
    OsalStatus sts;
    HANDLE handle = CreateEvent( NULL, FALSE, FALSE, NULL );
    if (handle == NULL)
    {
        sts = OSALSTS_EVENT_ERROR;
    }
    else
    {
        *pHandle = handle;
        sts = OSALSTS_SUCCESS;
        IncOsalStat(&osStats.events);
    }
	return (sts);}

OsalStatus OsalDeleteEvent(OsalHandle *pHandle)
{
    if (*pHandle != NULL)
    {
	    CloseHandle(*pHandle);
        *pHandle = NULL;
        DecOsalStat(&osStats.events);
    }
    return OSALSTS_SUCCESS;
}

OsalStatus OsalWaitForEvent(OsalHandle handle, unsigned int ticks)
{
	OsalStatus oslSts;
    int waitSts = WaitForSingleObject(handle, ticks);
	if (waitSts == WAIT_OBJECT_0)
	{
		oslSts = OSALSTS_SUCCESS;		
	}
	else if (waitSts == WAIT_TIMEOUT)
	{
		oslSts = OSALSTS_TIMEOUT;
	}
    else
    {
        oslSts = OSALSTS_EVENT_ERROR;
    }
	return oslSts;
}

OsalStatus OsalSetEvent(OsalHandle handle)
{
	OsalStatus sts;
	if (SetEvent(handle))
    {
        sts = OSALSTS_SUCCESS;
    }
    else
    {
        sts = OSALSTS_EVENT_ERROR;
    }
    return sts;
}

/*=============================================================================
 *                          Timing Primatives
 *============================================================================*/
OsalTickCount OsalGetTickCount(void)
{
    return GetTickCount();
}

OsalTickCount OsalGetTicksPerSecond(void)
{
    return CLOCKS_PER_SEC;	// 1000 for Windows
}

/*=============================================================================
 *                          Tasking Primatives
 *============================================================================*/

static int		nTaskIndexId = -1;		// threadLocal of task index
DWORD WINAPI MyOsTaskStart( LPVOID parm )
{
	TaskData *pTaskData = (TaskData *)parm;
    TlsSetValue(nTaskIndexId, (void*) pTaskData->taskIndex);

    pTaskData->pEntry(pTaskData->taskIndex);
    DecOsalStat(&osStats.tasks);
    
    OsalCloseTaskHandle((OsalHandle)pTaskData);
    return 1;
}

OsalStatus OsalCreateTask(OsalTaskEntryPointType pEntry,
                          int taskIndex, int stackSize, 
                          int priority, 
                          OsalHandle *pHandle, 
                          OsalTaskId *pTaskId)
{
    OsalStatus  sts = OSALSTS_CREATE_TASK_FAILED;
    TaskData *pTaskData;
    if (nTaskIndexId == -1)
	{
	    // So we can find our own task block from inside the thread.
		nTaskIndexId = TlsAlloc();
    }

    pTaskData = (TaskData *)malloc(sizeof(*pTaskData));
    if (pTaskData != NULL)
    {
        pTaskData->taskIndex = taskIndex;
        pTaskData->pEntry = pEntry;

        /* Use count is 2, because both the task and the caller have references to it. */
        pTaskData->useCount = 2; 
        pTaskData->handle = CreateThread(
				NULL,					// security - none
				stackSize,				// size of stack for thread
				MyOsTaskStart,		    // common start routine
				(LPVOID) pTaskData,		// parameter is the index
				STACK_SIZE_PARAM_IS_A_RESERVATION, // create in running state, use stack size as virtual memory reservation 
				(DWORD*)&pTaskData->taskId         // Store thread id in case we need it
				);

        if (pTaskData->handle != NULL)
        {
		    int ntPri = priority > 100 ? THREAD_PRIORITY_NORMAL : THREAD_PRIORITY_ABOVE_NORMAL;
		    SetThreadPriority(pTaskData->handle, ntPri);
            sts = OSALSTS_SUCCESS;
            IncOsalStat(&osStats.tasks);
            *pHandle = (OsalHandle)pTaskData;
            *pTaskId = pTaskData->taskId;
        }
        else
        {
            free(pTaskData);
        }
    }
    return sts;
}

OsalStatus OsalCloseTaskHandle(OsalHandle handle)
{
    TaskData *pTaskData = (TaskData *)handle;

    if (--pTaskData->useCount == 0)
    {
        CloseHandle(pTaskData->handle);
        free(pTaskData);
    }
    return OSALSTS_SUCCESS;
}

OsalTaskId OsalGetTaskId(void)
{
    return GetCurrentThreadId();
}

int OsalGetTaskIndex(void)
{
    return (int) TlsGetValue(nTaskIndexId);
}

OsalStatus OsalSleep(int ticks)
{
	DWORD	winSts;
    OsalStatus  sts = OSALSTS_SUCCESS;
	winSts = SleepEx( ticks*1000/OsalGetTicksPerSecond(), TRUE );
	// Documentation says returns 0 on timeout by I get this
	// so allow either.
	if ( winSts != 0 && winSts != STATUS_TIMEOUT )
	{
        sts = OSALSTS_SLEEP_ERROR;
    }
    return sts;
}

/*=============================================================================
 *                           Debug Support
 *============================================================================*/
void OsalPrintDebugString(const char *string)
{
    OutputDebugString(string);
    printf(string);
}

int OsalGetLastOsError(void)
{
    return GetLastError();
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

// Ordinary users can't adjust the system clock, and it might not be such a good idea anyway.
// Instead, just set an offset.
// OsalClockSetTime only supports CLOCK_REALTIME
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
