#ifndef _TASKLIB_INCLUDE_H
#define _TASKLIB_INCLUDE_H 1
/***************************************************************
 *  Filename: taskLib.h
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
 *  Description:  Header file for VxWorks emulation layer.
 *
 *	DJ Duffy Oct 1998
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Target/include/taskLib.h#1 $
//
/*
 * $Log: /VNIstack/LNS_V3.2x/Target/include/taskLib.h $
 * 
 * 9     4/09/04 1:57p Rjain
 * Put some function declarations in the right place.
 * 
 * 8     4/07/04 4:38p Rjain
 * Added new VxLayer Dll. 
 * 
 * 7     5/24/02 3:03p Fremont
 * 
 * 6     5/23/02 5:52p Fremont
 * Add new funcs
 * 
 * 4     11/01/99 9:49a Glen
 * 
 * 3     11/20/98 8:59a Darrelld
 * Check in updates
 * 
 * 2     11/09/98 11:56a Darrelld
 * Updates after native trial
 * 
 * 1     11/04/98 8:40a Darrelld
 * Windows Target header files
 * 
 */

#include <vxWorks.h>
#include <VxLayer.h>
#include "VxLayerDll.h" // VXLAYER_API
#ifdef __cplusplus
extern "C" {
#endif


// Define this obsolete value because a bunch of EDC code is using it
#define VX_SUPERVISOR_MODE	0x0001	/* OBSOLETE: tasks always in sup mode */

//
// Create a task and return the task id for the task or NULL
//
VXLAYER_API
int	taskSpawn(
 	const char* name,	// name of the task / thread
 	int priority,		// 0 - 255
 	int options,		// ignored
 	int stacksize,		// size of stack on VxWorks,
 						// ignored on Win
 	FUNCPTR entryPt,	// entry point function, 
 						// on VxWorks same as FUNCPTR
 	int Arg1, int arg2, int arg3, int arg4, int arg5,
	int arg6, int arg7, int arg8, int arg9, int Arg10 );	// parameters
//
// return the task id for the current task / thread
//
VXLAYER_API int taskIdSelf(void);

//
// Verify that a task is active
//
STATUS taskIdVerify( int tid );

//
// Is a task suspended
//
STATUS taskIsSuspended( int tid );

//
// Is task ready to run
//
STATUS taskIsReady( int tid );

//
// Delete a task
//
VXLAYER_API STATUS taskDelete( int tid );
STATUS taskDeleteForce( int tid );

//
// Suspend a task
//
STATUS taskSuspend( int tid );

//
// Resume a task
//
STATUS taskResume( int tid );

//
// Delay the current task
//

// 
// taskLock
//
STATUS taskLock(void);

// 
// taskUnlock
//
STATUS taskUnlock(void);

// 
// taskNameToId
//
int taskNameToId(char *name);

//
// printfAllTasks
//
void printfAllTasks(void);

VXLAYER_API
STATUS	taskDelay( int ticks );

//
// Get/Set task priority
//

STATUS  taskPriorityGet( int tid, int *priority );
STATUS  taskPrioritySet( int tid, int priority );

//
// taskExit
//
void	taskExit();

VXLAYER_API HANDLE	vxlGetThreadHandle( int tid );

#ifdef  __cplusplus
}
#endif

#endif // _TASKLIB_INCLUDE_H
