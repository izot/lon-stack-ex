#ifndef _LTCUTIL_H
#define _LTCUTIL_H
/***************************************************************
 *  Filename: IpLink.h
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
 *  Description:  interface for 'C' utilities.
 *
 *	DJ Duffy Nov 1998
 *
 ****************************************************************/

/*
 * $Log: /Dev/Shared/include/LtCUtil.h $
 * 
 * 7     2/08/00 7:28a Darrelld
 * Remove // comments
 * 
 * 6     7/16/99 3:50p Darrelld
 * add ticksToMs
 * 
 * 5     3/11/99 11:03a Darrelld
 * Add prototype for msToTicksX
 * 
 * 4     2/16/99 3:04p Glen
 * Add msToTicks
 * 
 * 3     1/22/99 9:11a Glen
 * add confidential statement
 * 
 * 2     12/02/98 4:58p Darrelld
 * Check-in updates
 * 
 * 1     12/02/98 4:43p Darrelld
 * 'C' utilties header file
 * 
 * 
 */

#include <VxlTypes.h>
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
Function:  cUtilInit()
Purpose:   Initialize C utilities
*******************************************************************************/
void	cUtilInit();

/*******************************************************************************
Function:  msToTicks
Returns:   number of ticks
Purpose:   To convert milliseconds to tick counts in a platform independent way.
*******************************************************************************/
int		msToTicks(int msec);
int		msToTicksX(int msec);	/* does not required pre-initialization */

/*******************************************************************************
Function:  ticksToMs
Returns:   number of milliseconds
Purpose:   To convert ticks to milliseconds in a platform independent way.
*******************************************************************************/
int		ticksToMs(int msec);	/* does not required pre-initialization */

/*******************************************************************************
Function:  CRC16
Returns:   16 bit CRC computed.
Purpose:   To compute the 16 bit CRC for a given buffer.

  CAUTION = this routine writes the CRC in two bytes following the supplied data

Comments:  None.
*******************************************************************************/
void LtCRC16(byte bufInOut[], int sizeIn);

#ifdef  __cplusplus
}
#endif
#endif // _LTCUTIL_H
