#ifndef _VXLAYER_H
#define _VXLAYER_H
/***************************************************************
 *  Filename: VxLayer.h
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
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/VxLayer/include/VxLayer.h#1 $
//
/*
 * $Log: /VNIstack/LNS_Dev/VxLayer/include/VxLayer.h $
 * 
 * 17    6/24/08 11:53a Bobw
 * Integrate changes from DCM:
 * 
 * 1) Add some DLL extern declarations for a few functions
 * 2) Add LtPlatform.h to several files to pick up moved declarations of
 * file system access routines
 * 3) Delete file system access routines from LtIpPlatform.cpp that had
 * been copied to LtPlatform.cpp, and move LtIpDeleteDir.
 * 4) Fixed a couple of type-os in conditional declarations
 * 
 * 16    6/24/08 7:56a Glen
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
 * 15    10/22/07 10:37a Bobw
 * EPR 46992
 * Remove OsalSuspendTask and OsalResumeTask from the OSAL.  On non-ilon,
 * change vxlInit to use a critical section initialized from task spawn.
 * On the i.lon stick with taskLock and taskUnlock.  On the i.lon these
 * are implemented by vxWorks, but on the non-ilon they depend on the
 * ability to suspend tasks, so on the i.lon WIN32 implment
 * taskLock/taskUnlock using WIN32 suspend/resume
 * 
 * 14    9/07/07 5:23p Fremont
 * For EPR 45639 - partial fix for reducing task sizes on e3 HW. Alias
 * vxlTaskSpawn()  to  iLonTaskSpawn() (on the iLON only).
 * 
 * 13    8/10/07 2:25p Bobw
 * Remove unneeded and expensive code from FTXL
 * 
 * 12    4/09/04 1:57p Rjain
 * Put some function declarations in the right place.
 * 
 * 11    4/07/04 4:38p Rjain
 * Added new VxLayer Dll. 
 * 
 * 10    11/01/02 11:12a Bobw
 * EPR 27592
 * Initialize TLS on DLL initialization - since some users of the stack
 * make calls into the stack before TaskSpawn (which normally does the TLS
 * init).
 * 
 * 9     10/18/01 4:36p Fremont
 * Move MAX_PATH
 * 
 * 7     11/24/98 9:54a Darrelld
 * 
 * 6     11/24/98 9:52a Glen
 * 
 * 5     11/24/98 9:42a Glen
 * 
 * 4     11/20/98 9:37a Darrelld
 * Remove sysClkRateGet and tickGet routines
 * 
 * 3     11/09/98 11:58a Darrelld
 * Updates after native trial
 * 
 * 2     11/06/98 9:42a Darrelld
 * Fix some problems and update the queue classes
 * 
 * 1     11/04/98 8:37a Darrelld
 * header files
 * 
 * 2     10/23/98 5:05p Darrelld
 * Enhancements and socket testing
 * 
 * 1     10/19/98 2:31p Darrelld
 * Vx Layer Sources
 * 
 */

#include <VxlTypes.h>
#include "VxLayerDll.h" // VXLAYER_API
#include "LtaDefine.h"

#ifdef  __cplusplus
extern "C" {
#endif



//
// Error handling stuff
//
#ifndef __INCvxTypesOldh
typedef int STATUS;
#endif

#if !PRODUCT_IS(ILON)
// Stub this in FTXL...
#define SetVxErrno(x)
#else
// VxWorks uses errno.
// Let's not depend on that being portable, so redefine it as a
// subroutine so we can override it appropritely for Win NT
//
int VxErrno(void);

// not sure if we'll need this one, but on VxWorks I think it looks like
// this:
// errno = 
// which we can't really support very well.
void SetVxErrno( int err);
#endif

// type for task entry point
typedef void TASKENTRY( int,int,int,int,int,int,int,int,int,int);

// only VxWorks needs cast to entry point
#if !defined(__VXWORKS__)
#define vxlTaskSpawn taskSpawn
#else
// Intercept task spawn calls on the iLON.
// Insert cast of entry point.
#include "echelon/iLon.h"
#define vxlTaskSpawn(nm,pri,opt,stk,ent,p0,p1,p2,p3,p4,p5,p6,p7,p8,p9) \
	iLonTaskSpawn(nm,pri,opt,stk,(FUNCPTR)(ent),p0,p1,p2,p3,p4,p5,p6,p7,p8,p9)
#endif


#if !defined(__VXWORKS__)
void    lockVxlInit();
void    unlockVxlInit();
#endif

void	vxlReportLastError( LPSTR tag );
void	vxlReportError( LPSTR tag );
void	vxlReportErrorPrintf( char* fmt, ... );

void	vxlTrace( char* fmt, ... );
BOOL	vxlRunning(void);
VXLAYER_API void	vxlShutdown(void);
BOOL	vxlTraceEnable( BOOL bEnable );
void	vxlTimerGo();
#ifdef __cplusplus
}
#endif

// end
#endif // _VXLAYER_H
