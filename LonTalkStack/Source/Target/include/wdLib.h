/***************************************************************
 *  Filename: wdLib.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Target/include/wdLib.h#1 $
//
/*
 * $Log: /VNIstack/LNS_V3.2x/Target/include/wdLib.h $
 * 
 * 4     4/07/04 4:38p Rjain
 * Added new VxLayer Dll. 
 * 
 * 2     11/09/98 11:56a Darrelld
 * Updates after native trial
 * 
 * 1     11/04/98 8:40a Darrelld
 * Windows Target header files
 */


#ifndef _WDLIB_INCLUDE_H
#define _WDLIB_INCLUDE_H 1
#include <vxWorks.h>
#include    "VxLayerDll.h" // VXLAYER_API
#ifdef __cplusplus
extern "C"
{
#endif


//
// Timers
//
typedef int* WDOG_ID;


VXLAYER_API WDOG_ID		wdCreate( void );

VXLAYER_API STATUS		wdDelete( WDOG_ID wid );

VXLAYER_API STATUS		wdStart( WDOG_ID wid, int delay, FUNCPTR func, int param );

VXLAYER_API STATUS		wdCancel( WDOG_ID wid );




#ifdef  __cplusplus
}
#endif

#endif // _WDLIB_INCLUDE_H

