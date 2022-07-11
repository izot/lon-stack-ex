#ifndef _VXLTARGET_H
#define _VXLTARGET_H
/***************************************************************
 *  Filename: VxlTarget.h
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
 *  Description:  Header file for VxWorks emulation layer tests.
 *
 *	DJ Duffy Oct 1998
 *
 ****************************************************************/

/*
 * $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/ShareIp/include/vxlTarget.h#1 $
 *
 *
 * $Log: /VNIstack/LNS_Dev/ShareIp/include/vxlTarget.h $
 * 
 * 21    3/26/07 2:44p Bobw
 * 
 * 20    5/19/04 8:11p Bobw
 * 
 * 19    5/19/04 9:03a Bobw
 * Improve tracing.  Add more granularity, and add L5 MIP message tracing
 * 
 * 18    4/09/04 1:57p Rjain
 * Put some function declarations in the right place.
 * 
 * 17    4/07/04 4:38p Rjain
 * Added new VxLayer Dll. 
 * 
 * 16    10/02/03 10:59a Fremont
 * add vxlInit()
 * 
 * 15    9/27/02 7:51a Glen
 * EPRS FIXED: 26973 - Use new nothrow
 * 
 * 14    7/11/02 3:59p Vprashant
 * Added vxlDummPrintf which does not print anything
 * 
 * 12    3/13/00 5:31p Darrelld
 * Interface for LNS
 * 
 * 10    11/17/99 3:40p Darrelld
 * EPR 15589
 * 
 * Defer vxlReports until after show.
 * Print urgent header.
 * Set print task to priority 0 for urgent trace.
 * 
 * 9     11/12/99 11:06a Darrelld
 * Changed vxlReportPrintf to vxlPrintf
 * 
 * 8     11/12/99 9:44a Darrelld
 * Use vxlReportPrintf for rtrstat output
 * Make synchronous mode work
 * 
 * 7     11/02/99 5:19p Darrelld
 * Fix printing of router statistics
 * 
 * 6     10/29/99 2:00p Darrelld
 * Add time stamping option to vxlReportEvent
 * 
 * 5     10/22/99 1:13p Darrelld
 * rtrstat command
 * 
 * 4     8/19/99 9:53a Darrelld
 * Trace levels
 * 
 * 3     8/18/99 2:43p Fremont
 * Make ANSI C safe
 * 
 * 2     7/08/99 1:15p Darrelld
 * add Urgent
 * 
 * 1     7/01/99 5:50p Darrelld
 * Target specific files
 * 
 * 3     12/02/98 12:49p Darrelld
 * Add event reporting
 * 
 * 2     10/23/98 5:05p Darrelld
 * Enhancements and socket testing
 * 
 * 1     10/19/98 2:31p Darrelld
 * Vx Layer Sources
 * 
 */

/* KEEP THIS FILE ANSI C COMPATIBLE (no // comments) */

#include <vxWorks.h>
#include    "VxLayerDll.h" // VXLAYER_API

#define VXL_REPORT_NONE		0
#define VXL_REPORT_URGENT	1
#define VXL_REPORT_EVENTS	2

#ifdef  __cplusplus
extern "C" {
#endif

/* Callback to process your own messages.
 */
typedef void VXLREPORTCALLBACK( char* pMsg );
/* When a callback is set, printf is not done.
 */
VXLAYER_API void	vxlSetReportCallback( VXLREPORTCALLBACK* callback );

VXLAYER_API int		vxlGetReportEventLevel();
VXLAYER_API void	vxlSetReportEventLevel(int nLevel);
VXLAYER_API void	vxlSetUseTimeStamps(BOOL bUseTimeStamps);

/* Basically, printf that's platform independent */
VXLAYER_API void	vxlPrintf(const char* fmt, ...);
VXLAYER_API void	vxlDummyPrintf(const char* fmt, ...);

/* These two are like vxlReportPrintf but are also controlled 
   via trace levels and have time stamping option */
VXLAYER_API void	vxlReportEvent(const char* fmt, ... );
VXLAYER_API void	vxlReportUrgent(const char* fmt, ... );

/* These functions are similiar to the vxlGet(Set)ReportEventLevel and VlxReportEvent(Urgent),
   except that they are controled by a trace type bit mask, rather than by an event level.
*/
VXLAYER_API int	vxlGetTraceTypes();
VXLAYER_API	void vxlSetTraceTypes(int optionsMask);
VXLAYER_API void vxlTraceType(int traceType, const char* fmt, ... );
VXLAYER_API BOOL vxlTracingType(int traceType);

#define VXL_TRACE_TYPE_L5MIP_MESSAGES	0x0001
#define VXL_TRACE_TYPE_L5MIP_REFID	    0x0002
#define VXL_TRACE_TYPE_L5MIP_TXID		0x0004
#define VXL_TRACE_TYPE_L5MIP_CACHE		0x0008

VXLAYER_API void	vxlMemoryCheck(void* p);
VXLAYER_API void	vxlMemoryFailure(void);

VXLAYER_API void	vxlClearEventRing();
VXLAYER_API void	vxlReportSynchronous( BOOL bSynch );
VXLAYER_API void    vxlReportGo();

VXLAYER_API void	vxlInit();	/* allow calling this directly for early init */

#ifdef  __cplusplus
}
#endif

#endif /* _VXLTARGET_H */
