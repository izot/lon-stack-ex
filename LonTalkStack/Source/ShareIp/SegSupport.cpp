/***************************************************************
 *  Filename: SegSupport.cpp
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
 *  Description:  Implementation file for LonTalk IP Segment Packet Support.
 *				This file supports segments on vxWorks. There is another file
 *				which provides support on MFC environments.
 *
 *	DJ Duffy Nov 1999
 *
 ****************************************************************/

/*
 * $Log: /Dev/ShareIp/SegSupport.cpp $
 *
 * 4     12/21/01 1:58p Vprashant
 * doubled stack size
 *
 * 3     10/15/01 1:45p Fremont
 * shorten task names
 *
 * 2     3/13/00 5:28p Darrelld
 * Segmentation work
 *
 * 1     12/10/99 3:49p Darrelld
 *
*/

#include <vxWorks.h>
#include <wdLib.h>
#include <semLib.h>
#include <tickLib.h>
#include <LtRouter.h>
#include <SegSupport.h>
#include <vxlTarget.h>

int SegSupTimeoutTask( int a1, ... );

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// LtIpSegmentor
//
// high level class maintains a list of segment requiests until they are satisfied
// or times them out.
//


LtIpSegBase::LtIpSegBase()
{
	m_tTimer			= wdCreate();
	m_lock				= semMCreate( SEM_Q_PRIORITY | SEM_INVERSION_SAFE );
	m_semTime			= semBCreate( SEM_Q_FIFO, SEM_EMPTY );
	m_bTimerTaskExit	= false;	// don't exit yet

	// spawn the task now, since we can't spawn it later when the timer goes off
	//m_tidTime			= taskSpawn( "SegTimeout", LTIP_SEGMENT_TASK_PRIORITY, 0, 5*1024,
	m_tidTime			= taskSpawn( "SegTimeout", LTIP_SEGMENT_TASK_PRIORITY, 0, 10*1024,
									SegSupTimeoutTask,
									(int)this, 2,3,4,5,6,7,8,9,0 );
	assert( m_tidTime );
}


LtIpSegBase::~LtIpSegBase()
{
	// delete the timer
	m_bTimerTaskExit = true;	// trigger exit of the task
	semGive(m_semTime);			// trigger the task to run
	while ( m_tidTime )			// wait on task to exit cleanly
	{
		taskDelay( msToTicksX( 20 ) );
	}
	if ( m_tTimer )
	{	wdDelete( m_tTimer );
		m_tTimer = NULL;
	}
#if 0 // bad idea to hammer a task
	lock();
	// we are locked, so task is idle, so we can delete it now
	if ( m_tidTime )
	{
		taskDelete( m_tidTime );
	}
	unlock();
#endif
	if ( m_semTime )
	{	semDelete( m_semTime );
		m_semTime = 0;
	}
	if ( m_lock )
	{	semDelete( m_lock );
		m_lock = 0;
	}
}


//
// SegmentTimeout
//
// timer routine
//
// This is the task that's running all the time.
int SegSupTimeoutTask( int a1, ... )
{
	// there are problems doing this lock on the object
	// in a timer routine. Since the timer rate is low, 5 sec,
	// we create and destroy a task each time it ticks.
	// On a 50 MHZ machine create time is 28us and delete / exit
	// time is also about 28us. So cost is very low.
	LtIpSegBase*	pSeg = (LtIpSegBase*)a1;
	pSeg->TimeoutTask();
	return 0;
}

//
// This is the timer entry point that can't do much except trigger a binary
// semaphore
//
int SegSupTimeout( int a1, ... )
{
	LtIpSegBase*	pSeg = (LtIpSegBase*)a1;
	semGive(pSeg->m_semTime);
	return 0;
}

//
// TimeoutTask
//
// Discard stale requests
//
void LtIpSegBase::TimeoutTask()
{

	while ( true )
	{	// wait until the timer goes off
		semTake( m_semTime, WAIT_FOREVER );
		if ( m_bTimerTaskExit )
		{	break;
		}
		lock();
		ULONG			nTickNow = tickGet();
		boolean			bMore = false;

		// pass in the current tickcount for timing so we do all
		// timing based on right now
		bMore = doTimeout( nTickNow );

		if ( bMore && !m_bTimerTaskExit )
		{
			//sts = wdStart( m_tTimer, msToTicksX( TIMEOUT_TIMERMS ), SegmentTimeout, (int)this );
			startTimer( TIMEOUT_TIMERMS );
		}
		unlock();
	}
	// we are no longer executing
	m_tidTime = 0;
#ifdef TESTSEGS
	vxlReportEvent("LtIpSegBase::TimeoutTask - task exit %d\n", tickGet() );
#endif // TESTSEGS
}

//
// start a timer
//
void LtIpSegBase::startTimer( int msTime )
{
	STATUS		sts;
	sts = wdStart( m_tTimer, msToTicksX( msTime ), SegSupTimeout, (int)this );
}

