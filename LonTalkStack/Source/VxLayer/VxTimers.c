/***************************************************************
 *  Filename: VxTimers.c
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
 *  Description:  Implementation of Timers for VxWorks emulation layer.
 *
 *	DJ Duffy Oct 1998
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/VxLayer/VxTimers.c#1 $
//
/*
 * $Log: /VNIstack/Dcx_Dev/VxLayer/VxTimers.c $
 * 
 * 29    6/24/08 7:56a Glen
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
 * 28    8/20/07 1:04p Bobw
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
 * 27    6/19/07 1:00p Bobw
 * OSAL upgrade
 * 
 * 26    4/03/07 10:00a Fremont
 * Fix LockTimerList semaphore
 * 
 * 25    3/29/07 12:58p Bobw
 * More FTXL development
 * 
 * 24    3/26/07 2:39p Bobw
 * EPRS FIXED: 
 * 
 * 23    4/07/04 4:38p Rjain
 * Added new VxLayer Dll. 
 * 
 * 22    1/31/03 10:37a Bobw
 * EPR 28227
 * Implement timers using a directory of timer blocks, rather than a
 * simple array, to avoid using jhuge blocks of memory (since allocating
 * huge blocks of memory, especially frequently growing blocks creates a
 * great deal of fragmentation).
 * 
 * 21    10/21/02 4:19p Bobw
 * Move incrment of timer count inside of lock, to prevent race condition.
 * 
 * 20    9/25/02 6:13p Bobw
 * Fix bug when we have lots of timers
 * 
 * 19    8/07/00 10:59a Glen
 * Allow more than 65K timers
 * 
 * 18    3/27/00 2:59p Bobw
 * Reinit timer allocation high water mark when uniiting timers.
 * 
 * 17    2/29/00 1:56p Glen
 * EPRS FIXED: 16247 - changed tick counters to be unsigned
 * 
 * 16    12/17/99 12:17p Glen
 * Fix timer bugs
 * 
 * 15    12/10/99 2:37p Glen
 * Made it work with OS delays
 * 
 * 14    12/10/99 11:23a Glen
 * Allow for flexible number of timers
 * Provide more efficient timer chaining
 * 
 * 12    2/12/99 3:09p Glen
 * Simulate VxWorks better by holding lock during callback
 * 
 * 11    2/05/99 11:30a Darrelld
 * Replace SleepEx with WaitForSingleObject
 * 
 * 10    2/01/99 11:17a Glen
 * Joint Test 3 integration
 * 
 * 9     1/11/99 9:53a Glen
 * Fix trace message
 * 
 * 8     12/30/98 6:26p Glen
 * bActive not managed correctly (results in list corruption)
 * 
 * 7     12/29/98 12:33p Glen
 * vxlInsertTimerChain failed to insert properly into head of non-empty
 * queue.
 * 
 * 6     12/18/98 9:59a Glen
 * 
 * 5     11/20/98 9:36a Darrelld
 * add sysLib and tickLib includes
 * 
 * 4     11/09/98 11:58a Darrelld
 * Updates after native trial
 * 
 * 3     11/06/98 9:42a Darrelld
 * Fix some problems and update the queue classes
 * 
 * 2     10/23/98 5:05p Darrelld
 * Enhancements and socket testing
 * 
 * 1     10/16/98 1:43p Darrelld
 * Vx Layer Sources
 */

#include "LtaDefine.h"
#include <stdlib.h>
#include <string.h>
#include "Osal.h"
#ifdef WIN32
#include <windows.h>
#endif
#include <stdio.h>

// Definitions for vxWorks
#include "VxWorks.h"
#include "VxLayer.h"
#include	<assert.h>
//#include <msgQLib.h>
#include <taskLib.h>
//#include <semLib.h>
#include <wdLib.h>
#include <sysLib.h>
#include <tickLib.h>
#include "VxLayerDll.h" // VXLAYER_API
#include "taskPriority.h"

////////////////////////////////////////////////////////////////////
//
// Timers
//
// Watch dog timers are timers which run at interrupt level.
// They can be triggered from any task[thread] but are
// executed in the context of the timer interrupt, which is
// no task [thread]
// We simulate this behaviour by having a timer thread.
// Timer routines are therefore synchronized
// with access to the timer thread.
// The timer thread has no other job other than timers.
// It waits on the next timer using WaitForSingleObject and can
// be scheduled via a Release.
// It uses OsalGetTickCount to figure the amount of time to sleep until the
// timer should be triggered.
//
// The number of timers grows as demand increases.  It is not limited.
// There are a number of timer chains.  This allows for more efficient
// insertion if there are many expirations pending.  It does make for
// less efficient timeout processing but the cost is highest when there
// are few timer expirations pending - presumably when the system is 
// least busy.  Seems like a good trade-off.
//
// Of course we handle the case of more than one
// timer going off at the same time.
//
// Actually, these routines could have used the NT services:
// CreateWaitableTimer
// SetWaitableTimer
// CancelWaitableTimer
// In this case, the behaviour would have been slightly different.
// For these services, the completion routines run in the context of the
// thread that started the timer, rather than the context of an "ISR" which
// is modeled by another thread in the current implementation.
//

//
// sysClkRateGet
//
// Return the ticks per second for this system.
//
VXLAYER_API
int		sysClkRateGet(void)
{
	return OsalGetTicksPerSecond();
}

//
// Data for timer control
//

// A timer id has an index and an incarnation to allow
// protection for reuse of the timer slot.
// Not sure that VxWorks provides this protection
//
//
// We desire protection for timers. This protection will
// detect stale timers and prevent their use.
// To do this, a WDOG_ID is actually a structure with an index
// into the timer array and an incarnation number.
// But VxWorks believes that a WDOG_ID is an address, so code will
// be checking it for NULL to see if the value is legal.
// So we need to eventually cast a structure with two shorts to an
// address. We do this by way of casting storing the structure in
// an int, and then casting the value of the into to a pointer to int.
// 'C' lets us do this. But it does not allow casting between structures
// and pointers.
//

int vxlMaxTimers = 0;
int nFreeChain = 0;
int vxlTimerCount = 0;
int vxlTimerFreeIndex = 0;
BOOL vxlProcessingTimeouts = FALSE;

// The actual WDOG_ID as we will be using it.
// Both the index and incarnation are available.
// WDOG_ID contains 8 bit incarnation number and
// 24 bit index allowing up to 2^24 timers.
#define WDOG_INDEX_MAX 0xffffff
#define WDOG_INCARN(wi) (((unsigned int)wi)>>24)
#define WDOG_INDEX(wi) (((unsigned int)wi)&WDOG_INDEX_MAX)
#define MAKE_WDOG_ID(idx, incarn) ((WDOG_ID)(idx | (incarn<<24)))

// Timer control structure
typedef struct _VXTIMER
{
	BOOL		bBusy;				// Somebody created it
	BOOL		bActive;			// Somebody started it
	int			nextIdx;			// Index of next timer in chain or zero.  Also used on free chain.
	int			prevIdx;			// index of previous timer in chain.  Not used for free chain.
	WDOG_ID	    wid;				// Timer ID for this timer
	DWORD		nExpireTicks;		// Ticks value at expiry
	FUNCPTR		pRoutine;			// Callback routine
	int			nParam;				// Param for callback routine
} VXTIMER;

unsigned char	nTimerIncarn = 57;	// incarnation of timers - starts at arbitrary position.

// Timers are global to the system, and not private to each task
// Also, Timer routines are delivered in VxWorks at interrupt level
// or as work routines. Not in the context of the thread that started
// them. So we implement timers are a separate thread. This thread
// then executes the timer routines. As close as we can get to 
// VxWorks behaviour.

// The number of timers on the PC may become very large.
// Rather than storing timers as a single array, it is divided into more
// managable "chunks".  This way when we expand the number of timers 
// a) We don't have to copy all the old timers
// b) We dont run into heap fragmentation problems due repeatedly allocating
//    and freeing ever bigger blocks of data.
// Timers are accessed by a directory (which grows dynamically) which points to arrays of
// blocks. 
#define VXLTIMER_DIRECTORY_EXPANSION_SIZE 1024  // # of entries to expand directory by
#define MAX_VXL_TIMER_BLOCK_ENTRIES 1024
int vxlTimerDirSize = 0;
typedef struct _VXTIMER_BLOCK
{
    VXTIMER timers[MAX_VXL_TIMER_BLOCK_ENTRIES];
} VXTIMER_BLOCK;

VXTIMER_BLOCK**	vxlTimerDirectory = null;
#define TIMER_DIR_INDEX(index) ((index)/MAX_VXL_TIMER_BLOCK_ENTRIES)
#define BLOCK_INDEX(index) ((index)%MAX_VXL_TIMER_BLOCK_ENTRIES)
#define GET_TIMER_ENTRY(index) (&vxlTimerDirectory[TIMER_DIR_INDEX(index)]->timers[BLOCK_INDEX(index)])

//
// Critical section to lock the timers while
// being accessed.
//
OsalHandle	csTimerLock;
//
// Semaphore to release the timer wait when we
// start a timer
//
OsalHandle semTimerStart;

// One time init of timers
BOOL				bTimerLockInit;

// Head of the timer chain. It's an index.
// zero means chain is empty.
#define				VXL_TIMER_NUM_CHAINS	100
#define				VXL_TIMER_UNIT			20
#define				VXL_MAX_EXPIRATION_DELAY (VXL_TIMER_NUM_CHAINS * VXL_TIMER_UNIT)
#define				GET_CHAIN_IDX(time)		(((DWORD)time / VXL_TIMER_UNIT) % VXL_TIMER_NUM_CHAINS)
int					vxlTimerChains[VXL_TIMER_NUM_CHAINS];

// Number of pending timers
int					vxlPendingTimerCount = 0;

#define				VXL_MAX_DELAY	0x7fffffff
DWORD				vxlExpirationTime = VXL_MAX_DELAY;

// Id for timer thread that we have started
int					vxlTimerThreadId = 0;

// Forward declarations
void LockTimerList();
void UnlockTimerList();
void vxlTimerRemoveChain( int idx );

DWORD processTimeouts(int chainIdx, DWORD ticksNow)
{
	int idx;
	int ticksDiff = 0;

	while ((idx = vxlTimerChains[chainIdx]) != 0)
	{
        VXTIMER *pTimer = GET_TIMER_ENTRY(idx);
		if ( !pTimer->bActive || !pTimer->bBusy )
		{	vxlReportError( "vxlTimerThread - inactive timer in list");
			// just toss it out after message
			vxlTimerRemoveChain( idx );
		}
		else
		{
			ticksDiff = ticksNow - pTimer->nExpireTicks;
			if ( ticksDiff >= 0 )
			{	// This timer just went off
				// Pull it out of the chain since routine might
				// put it back in the list.  This is OK since the
				// lock uses a critical section which is nestable.
				vxlTimerRemoveChain( idx );

				// Note that it is legal for this callback to do either
				// a wdStart or wdCancel.  This should work.  Starting
				// a timer at this point will either:
				// a) add it to this chain, in which case we will process it now.
				// b) add it to a chain preceding this in which case it is not
				//    expiring anyway.
				// c) add it after this chain in which case we will get to it
				//    when we return from here.
				pTimer->pRoutine( pTimer->nParam );
				// And we are done with that timer.
				// we will find the next one soon enough
				ticksDiff = 0;
			}
			else
			{
				ticksDiff = -ticksDiff;
				break;
			}
		}
	}
	return ticksDiff;
}

//
// vxlTimerThread
//
// This is the thread that implements timers
//
// Its a vx task. Hey, why not. We went to all that trouble
// to make a simple way to start a thread, so we will use it.
// The thread is started when we create a timer, since before
// that time we don't need it.
//
int	vxlTimerThread(  int arg1,... )	// parameters ignored
{
	// This tasks checks all the timer chains to find any that have
	// expired and to determine the new time to sleep.
	//
	// If there are no entries in the queue, then just sleep for a while.
	// We will be altered when something shows up before then. Don't worry.
	//
	// The timer entries are in sorted order based on the expiry tick time.
	// Each is in a chain based on its expiration time.
	// So when we look, then we have tolook at the ones at the
	// beginning of each chain up until the chain for the current time.
	// We then have to find the next one with a minimal amount of time to
	// go and then sleep for that time.
	//
	// Seems odd to lock here, but we must or vxlTimerChain might race
	// with us and we might miss a timer.
	//
	while ( vxlRunning() )
	{
		DWORD minDuration = VXL_MAX_DELAY;
		DWORD ticksNow;

		LockTimerList();

		vxlProcessingTimeouts = TRUE;

		ticksNow = OsalGetTickCount();

		if ( vxlPendingTimerCount != 0 )
		{
			// Process the timeouts
			int i;
			int chainIdx;
			int quitIdx;
			BOOL bCheckDuration = false;
			BOOL bComputeMinimum = false;
			int ticksDiff = vxlExpirationTime - ticksNow;

			if (ticksDiff > 0)
			{
				// We have an expiration time in the future.  Just ignore it and sleep
				// again.  This can happen when a timer is set with no other expirations
				// pending.
				minDuration = ticksDiff;
			}
			else
			{
				// The current time should be the same as the expiration time.
				// However, in case it isn't, check for it being different 
				// enough that we need to check all chains.
				chainIdx = GET_CHAIN_IDX(vxlExpirationTime);
				if (abs(ticksNow - vxlExpirationTime) > VXL_MAX_EXPIRATION_DELAY)
				{
					// Took too long to run - check all chains.
					quitIdx = (chainIdx+VXL_TIMER_NUM_CHAINS-1) % VXL_TIMER_NUM_CHAINS;
					bComputeMinimum = true;
				}
				else
				{
					quitIdx = GET_CHAIN_IDX(ticksNow);
				}

				// Check timer count in list in case some timers were cancel (or deleted)
				// in an expiration callback.
				for (i=0; vxlPendingTimerCount && i<VXL_TIMER_NUM_CHAINS; i++)
				{
					DWORD duration = processTimeouts(chainIdx, ticksNow);

					if (chainIdx == quitIdx)
					{
						// We went as far as we needed to go.  Now allow us
						// to drop out of the loop when we find a small
						// timeout.
						bCheckDuration = true;
						bComputeMinimum = true;
					}

					if (duration)
					{
						if (bComputeMinimum && duration < minDuration)
						{
							minDuration = duration;
						}

						// Last chain might have duration which is close to 
						// (but not equal to max) or might have close to
						// 0.  So we must check its duration but ignore those
						// close to the maximum.
						if (bCheckDuration && minDuration < VXL_MAX_EXPIRATION_DELAY-VXL_TIMER_UNIT)
						{
							// This duration requires less than the max delay so
							// wait this long.  We know chains after this one can't
							// have a shorter delay since chains are ordered by time.
							break;
						}
					}

					// Go to next chain
					chainIdx = (chainIdx + 1) % VXL_TIMER_NUM_CHAINS;
				}

				// Record when we are supposed to wake up.  
				vxlExpirationTime = ticksNow + minDuration;
			}
		}	

		vxlProcessingTimeouts = FALSE;

		// Unlock list prior to sleeping
		UnlockTimerList();		
		// Sleep until we timeout or we are woken due to a new timer expiration
		// that is to occur before we are due to run.
		OsalWaitForBinarySemaphore( semTimerStart,	minDuration );
	}	// While forever

	OsalDeleteBinarySemaphore(&semTimerStart);
	OsalDeleteCriticalSection(&csTimerLock);

	return 0;
}

void	vxlTimerGo()
{
	if (semTimerStart)
	{
		OsalReleaseBinarySemaphore(semTimerStart);
	}
}

void	vxlTimerUnalloc()
{
    int i;
    for (i = 0; i < vxlTimerDirSize; i++)
    {
        free(vxlTimerDirectory[i]);
    }
    free(vxlTimerDirectory);
	vxlTimerDirectory = null;
    vxlTimerDirSize = 0;
	vxlMaxTimers = 0;
	vxlTimerCount = 0;
    nFreeChain = 0;
}

void	vxlTimerAlloc()
{
    int dirIndex = TIMER_DIR_INDEX(vxlMaxTimers);
    if (dirIndex >= vxlTimerDirSize)
    {
        VXTIMER_BLOCK **pNewDir;
        int nSize = (vxlTimerDirSize + VXLTIMER_DIRECTORY_EXPANSION_SIZE) * sizeof(VXTIMER_BLOCK *);
        if (vxlTimerDirectory == NULL)
        {
            pNewDir = malloc(nSize);
        }
        else
        {
            pNewDir = realloc(vxlTimerDirectory, nSize);
        }

        if (pNewDir != NULL)
        {
            vxlTimerDirectory = pNewDir;
		    memset(&vxlTimerDirectory[vxlTimerDirSize], 0, VXLTIMER_DIRECTORY_EXPANSION_SIZE*sizeof(VXTIMER_BLOCK *));
            vxlTimerDirSize += VXLTIMER_DIRECTORY_EXPANSION_SIZE;
        }
    }
    if (dirIndex < vxlTimerDirSize)
    {
        VXTIMER_BLOCK *pTimerBlock = (VXTIMER_BLOCK *)malloc(sizeof(VXTIMER_BLOCK));
        if (pTimerBlock != NULL)
        {
            int i;
            memset(pTimerBlock, 0, sizeof(VXTIMER_BLOCK));
            vxlTimerDirectory[dirIndex] = pTimerBlock;
            for (i = 0; i < MAX_VXL_TIMER_BLOCK_ENTRIES-1; i++)
            {
                pTimerBlock->timers[i].nextIdx = vxlMaxTimers + i + 1;
            }
            assert(nFreeChain == 0);    // If we are here, there should be nothing on the free chain.
            pTimerBlock->timers[MAX_VXL_TIMER_BLOCK_ENTRIES-1].nextIdx = nFreeChain;
            nFreeChain = vxlMaxTimers;
            vxlMaxTimers += MAX_VXL_TIMER_BLOCK_ENTRIES;
        }
    }
	if (vxlTimerCount == 0)
	{
		// Zero is reserved for ease of chaining
		GET_TIMER_ENTRY(0)->bBusy = true;
        nFreeChain = 1;
		vxlTimerCount = 1;
	}
}

//
// LockTimerList
//
// Lock the timers list in case we modify it
//
void	LockTimerList( void)
{
	// One time init of the critical section
	if ( ! bTimerLockInit )
	{
		int		tmrTaskId;

		bTimerLockInit = TRUE;
		OsalCreateCriticalSection( &csTimerLock );
		vxlTrace("LockTimerList - create timer lock 0x%08x\n", csTimerLock );

		memset(vxlTimerChains, 0, sizeof(vxlTimerChains));

		vxlTimerAlloc();		
		
		// Create the semaphore to release the timer thread
		// when we start a timer
		// Initially signalled to block the thread
		if (OsalCreateBinarySemaphore( &semTimerStart, OSAL_SEM_SET) != OSALSTS_SUCCESS)
		{	vxlReportError( "LockTimerList - Unable to create semaphore");
		}

		// Start the timer thread, since we appear to be needing it        
		tmrTaskId = taskSpawn( "VxlTimerThread", 
                               VXL_TIMER_THREAD_PRIORITY, 0, 
                               VXL_TIMER_THREAD_STACK_SIZE,
					(FUNCPTR)vxlTimerThread, 0,0,0,0,0,0,0,0,0,0 );
		vxlTimerThreadId = tmrTaskId;
		if ( tmrTaskId == 0 )
		{
			vxlReportError( "LockTimerList - Unable to start timer thread");
		}
		// This thread may or may not get started and lock the list
		// and then unlock it when it sleeps
		// we are about to lock it here
	}
	OsalEnterCriticalSection( csTimerLock );
}

//
// UnlockTimerList
//
// Unlock the task list when we are done.
//
void	UnlockTimerList(void)
{
	OsalLeaveCriticalSection( csTimerLock );
}

//
// vxlTimerInsertChain
//
// *** TIMERS MUST BE LOCKED ****
//
// Insert a timer in a chain at a specific point
// If insert point is zero then put at start of chain.
//
void vxlTimerInsertChain( int idx )
{
	int		prv = 0;
	int		ticksDiff;

	VXTIMER* pTimer = GET_TIMER_ENTRY(idx);

	// First, determine which timer chain to insert this
	// item into.
	int*	pTimerChain = &vxlTimerChains[GET_CHAIN_IDX(pTimer->nExpireTicks)];

	// Now figure the correct place to put the timer in the list
	// scan the list for the end or an expire time greater than ours

    int		nxt = *pTimerChain;

	DWORD	ticksThen = pTimer->nExpireTicks;

	while ( nxt != 0 )
	{
		// Since the tick count is unsigned, and wraps around, we need
		// to do the comparison this way. One might be wrapped and one
		// not wrapped yet. but the difference will be correct for
		// the comparison. > 0 preserves order of queuing for
		// timers going off at the same time.
        VXTIMER* pNextTimer = GET_TIMER_ENTRY(nxt);
		ticksDiff = pNextTimer->nExpireTicks - ticksThen;
		if ( ticksDiff > 0 )
		{	break;
		}
		// Remember where the previous entry was and step forward
		prv = nxt;
		nxt = pNextTimer->nextIdx;
	}

	// Can't insert timer zero, since it's never used
	if ( idx == 0 )
	{	vxlReportError("vxlTimerInsertChain - attempt with idx = 0");
		return;
	}

	if ( prv == 0 )
	{
		*pTimerChain = idx;
	}
	// It is OK that prv or nxt might be zero since zero is
	// not used.
	pTimer->nextIdx = nxt;
	GET_TIMER_ENTRY(prv)->nextIdx = idx;
	pTimer->prevIdx = prv;
	GET_TIMER_ENTRY(nxt)->prevIdx = idx;
	pTimer->bActive = TRUE;
	vxlPendingTimerCount++;

	if (!vxlProcessingTimeouts)
	{
		ticksDiff = vxlExpirationTime - ticksThen;
		if (ticksDiff > 0 || vxlPendingTimerCount == 1)
		{
			vxlExpirationTime = ticksThen;
			OsalReleaseBinarySemaphore( semTimerStart);
		}
	}
}

//
// vxlTimerRemoveChain
//
// *** TIMERS MUST BE LOCKED ****
//
// Remove a timer from the chain by index.
// Adjust start of chain if needed and don't
// allow removeal of index zero timer
//
void vxlTimerRemoveChain( int idx )
{
	int		nxt;
	int		prv;
    VXTIMER *pTimer = GET_TIMER_ENTRY(idx);
	int*	pTimerChain = &vxlTimerChains[GET_CHAIN_IDX(pTimer->nExpireTicks)];

	// Can't remove timer zero, since it's never used
	if ( idx == 0 )
	{	vxlReportError("vxlTimerRemoveChain - attempt with idx = 0");
		return;
	}

	prv = pTimer->prevIdx;
	nxt = pTimer->nextIdx;
	// caution, nxt and prv might be zero in following statements
	// it's ok since entry zero is not used.
	GET_TIMER_ENTRY(prv)->nextIdx = nxt;
	GET_TIMER_ENTRY(nxt)->prevIdx = prv;

	// if we were start of chain, then start with next item
	if ( *pTimerChain == idx )
	{
		*pTimerChain = nxt;
	}
	pTimer->bActive = FALSE;
	vxlPendingTimerCount--;
}



//
// wdValid
//
// Returns true if timer index is valid - that is, timer
// entry has been allocated and incarnation number matches
//
STATUS wdValid(WDOG_ID wid, const char * szFuncName)
{
	STATUS sts = OK;
	int idx = WDOG_INDEX(wid);
    VXTIMER *pTimer = GET_TIMER_ENTRY(idx);

	if (((idx < vxlMaxTimers) && !pTimer->bBusy) || 
        (WDOG_INCARN(pTimer->wid) != WDOG_INCARN(wid)))
	{
		char s[50];
		sts = ERROR;
		sprintf(s, "%s - timer incarn mismatch", szFuncName);
		vxlReportError(s);
	}
	return sts;
}

//
// wdCreate
//
// Create a timer.
// Here, we allocate it in the timer list and return it's ID
// That's all we really need to do now.
// The timer thread doesn't care about the timer until we start it.
//
VXLAYER_API
WDOG_ID		wdCreate( void )
{
	WDOG_ID		wid = NULL;
	BOOL		bFound = FALSE;

	LockTimerList();

	if (vxlTimerCount == vxlMaxTimers)
	{
		vxlTimerAlloc();
	}

	if (vxlMaxTimers <= WDOG_INDEX_MAX)
	{
        VXTIMER *pTimer = GET_TIMER_ENTRY(nFreeChain);
        assert(!pTimer->bBusy);
        if (!pTimer->bBusy)
        {
            int index = nFreeChain;
			// Just to be sure, clobber it all.
            nFreeChain = pTimer->nextIdx;  // Next on the free chain.
			memset( pTimer, 0, sizeof(VXTIMER) );
			pTimer->bBusy = TRUE;
			pTimer->bActive = FALSE;
			nTimerIncarn++;
			pTimer->wid = MAKE_WDOG_ID(index, nTimerIncarn);
			wid = pTimer->wid;
			bFound = TRUE;
		    vxlTimerCount++;
		}
	}

	UnlockTimerList();

	if (!bFound)
	{
		vxlReportError("wdCreate - timer couldn't be created.");
	}
	return wid;
}

//
// wdDelete
//
// Discard a timer
//
VXLAYER_API
STATUS	wdDelete( WDOG_ID wid )
{
	STATUS		sts = OK;
	int			idx = WDOG_INDEX(wid);
    
	LockTimerList();

	sts = wdValid(wid, "wdDelete");

	if ( sts == OK )
	{
        VXTIMER *pTimer = GET_TIMER_ENTRY(idx);

		if ( pTimer->bActive )
		{
			vxlTimerRemoveChain( idx );
		}
		pTimer->bBusy = FALSE;
        pTimer->nextIdx = nFreeChain;
        pTimer->prevIdx = 0; // Not used on free chain.
        nFreeChain = idx;

		if (--vxlTimerCount == 1)
		{
			// Clean up memory.  It should be rare that 
			// all timers are deleted so this shouldn't be
			// too expensive.  It beats requiring all users
			// to call a cleanup routine.
			vxlTimerUnalloc();
		}
	}

	UnlockTimerList();

	return sts;
}

//
// wdStart
//
// Start a timer and set the callback routine
// We need to wake up the timer thread so the current timer is checked
//
VXLAYER_API
STATUS	wdStart( WDOG_ID wid, int delay, FUNCPTR pRoutine, int nParam )
{
	DWORD		ticksNow;
	DWORD		ticksThen;
	STATUS		sts = OK;
	int			idx = WDOG_INDEX(wid);

	LockTimerList();		// *** TIMER LIST LOCKED ***

	sts = wdValid(wid, "wdStart");

	if (sts == OK)
	{
        VXTIMER *pTimer = GET_TIMER_ENTRY(idx);

		// VxWorks says that if a timer is active and we start again
		// then the last start takes precidence. So make it so.
		if ( pTimer->bActive )
		{
			vxlTimerRemoveChain( idx );
		}

		// Compute the expire ticks time and then fill in the
		// timer block with all the poop.
		ticksNow = OsalGetTickCount();
		ticksThen = ticksNow + delay;
		pTimer->nExpireTicks = ticksThen;
		pTimer->pRoutine = pRoutine;
		pTimer->nParam = nParam;

		// Insert into correct place in the timer chain
		vxlTimerInsertChain( idx );
	}

	UnlockTimerList();	// *** TIMER LIST UNLOCKED ***

	return sts;
}

//
// wdCancel
//
// Cancel a timer regardless of it's state.
//
VXLAYER_API
STATUS	wdCancel( WDOG_ID wid )
{

	STATUS		sts = OK;
	int			idx = WDOG_INDEX(wid);

	LockTimerList();
	
	sts = wdValid( wid , "wdCancel");

	if (sts == OK)
	{
		// It might or might not be in the timer chain just now.
		// Not an error if it's not.
		if ( GET_TIMER_ENTRY(idx)->bActive )
		{
			vxlTimerRemoveChain( idx );
		}
	}

	UnlockTimerList();

	return sts;
}

#ifdef WIN32
//
// vxlPerfTimer
//
// Return the performance counter
//
BOOL	vxlPerfTimer( ULONGLONG* pCounter )
{
	return QueryPerformanceCounter( (LARGE_INTEGER*)pCounter );
}

//
// vxlPerfTimerFrequency
//
// Return the performance counter
//
BOOL	vxlPerfTimerFrequency( ULONGLONG* pTicksPerSec )
{
	return QueryPerformanceFrequency( (LARGE_INTEGER*)pTicksPerSec );
}
#endif

// end
