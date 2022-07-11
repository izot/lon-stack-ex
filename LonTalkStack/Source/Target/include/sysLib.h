#ifndef _SYSLIB_INCLUDE_H
#define _SYSLIB_INCLUDE_H 1
/***************************************************************
 *  Filename: sysLib.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Target/include/sysLib.h#1 $
//
/*
 * $Log: /Dev/Target/include/sysLib.h $
 * 
 * 6     5/25/07 4:41p Fremont
 * Change clock macro and comments
 * 
 * 5     4/07/04 4:38p Rjain
 * Added new VxLayer Dll. 
 * 
 * 4     1/22/02 5:26p Fremont
 * add prototype for setRtcFromOsTime()
 * 
 * 2     11/20/98 9:37a Darrelld
 * Add for sysClkRateGet
 * 
 * 1     11/20/98 9:29a Darrelld
 * 
 */

// This file name conflicts with the name of a 
// vxWorks header file. This file should be renamed.

#include <vxWorks.h>
#include    "VxLayerDll.h" // VXLAYER_API
#ifdef __cplusplus
extern "C"
{
#endif
//
// sysClkRateGet
//
// Obtain the ticks per second of the timer on this machine
// Always ms for Windoze
//

// Note: CLOCKS_PER_SEC macro is preferable, as it is available in Windows and vxworks
VXLAYER_API int	sysClkRateGet(void);


// Interface to set the RTC.
// Real prototype is in sysRtc.h in the BSP, but we can get there...
extern STATUS setRtcFromOsTime();

#ifdef  __cplusplus
}
#endif

#endif // _SYSLIB_INCLUDE_H
