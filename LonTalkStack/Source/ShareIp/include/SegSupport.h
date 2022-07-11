#ifndef _SEGSUPPORT_H
#define _SEGSUPPORT_H

/***************************************************************
 *  Filename: SegSupport.h
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
 *  Description:  Header file for LonTalk IP Segment Packet support classes.
 *				These classes are used in the vxWorks environment.
 *				BEWARE - A different file with the same name exists for
 *				use in the MFC environment.
 *
 *	DJ Duffy Nov 1999
 *
 ****************************************************************/

/*
 * $Log: /Interim/ShareIp/include/SegSupport.h $
 * 
 * 4     3/13/00 5:28p Darrelld
 * Segmentation work
 * 
 * 2     12/10/99 4:04p Darrelld
 * MFC based segment base classes
 * 
 * 1     12/10/99 3:49p Darrelld

*/

#include <LtObject.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// LtIpSegBase
//
// Base class for segmentor objects
// Support locks and timers in a vxWorks specific way.
// This class is replaced in the MFC environment to allow critical sections or 
// other locking mechanisms to be used directly.
// The segmentor class to be free of any direct dependencies on vxWorks.
//
class LtIpSegBase : public LtObject
{
protected:
	SEM_ID			m_lock;					// lock on this and child data structures
	WDOG_ID			m_tTimer;				// timer to look for stale received requests
	SEM_ID			m_semTime;				// semaphore to trigger timer task
	int				m_tidTime;				// timer task id
	boolean			m_bTimerTaskExit;		// tell the timer task to exit

public:
	LtIpSegBase();
	virtual ~LtIpSegBase();

	virtual void	lock()
	{	semTake( m_lock, WAIT_FOREVER );
	}
	virtual void	unlock()
	{	semGive( m_lock );
	}
	enum {
		TIMEOUT_TIMERMS	= (1*1000)		// timer tick rate in milliseconds
	};

	virtual void	startTimer( int msTimeout );
	friend int SegSupTimeout( int a1, ... );
	friend int SegSupTimeoutTask( int a1, ... );
	virtual boolean	doTimeout( ULONG msTime) = 0;
	virtual void TimeoutTask();

};

#endif // _SEGSUPPORT_H

