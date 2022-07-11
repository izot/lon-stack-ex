/***************************************************************
 *  Filename: IpLink.c
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
 *  Description:  implementation of 'C' utilities
 *
 *	DJ Duffy Nov 1998
 *	Some content originally from Adept, Inc.
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Shared/LtCUtil.c#1 $
//
/*
 * $Log: /DCX/Dev/dc3/dcd/EVNI/Shared/LtCUtil.c $
 * 
 * 1     12/05/07 11:15p Mwang
 * 
 * 10    12/09/99 10:44a Glen
 * 
 * 
 * 8     9/24/99 8:14a Darrelld
 * Faster CRC algorithm
 * 
 * 7     7/16/99 3:50p Darrelld
 * add ticksToMs
 * 
 * 6     3/11/99 11:02a Darrelld
 * Add msToTicksX to avoid need to pre-initialize the clkRate.
 * 
 * 5     3/02/99 8:51a Glen
 * UNIX compatibility
 * 
 * 4     2/16/99 3:02p Glen
 * Add msToTicks
 * 
 * 3     1/22/99 9:11a Glen
 * add confidential statement
 * 
 * 2     12/02/98 4:58p Darrelld
 * Check-in updates
 * 
 * 1     12/02/98 4:42p Darrelld
 * 'C' utilities
 * 
 * 
 */
//
//////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <vxWorks.h>
#include <sysLib.h>
#include <VxlTypes.h>
#include <limits.h>
#include <LtCUtil.h>

int clkRate = -1;

/*
 * The following table was generated with the following code.
 *
 	unsigned int i, j, r;

	for (i = 0; i <= UCHAR_MAX; i++) {
		r = i << (16 - CHAR_BIT);
		for (j = 0; j < CHAR_BIT; j++)
			if (r & 0x8000U) r = (r << 1) ^ 0x1021U;
			else             r <<= 1;
		printf("Table entry %d is %04x\n", i, (r & USHRT_MAX));
	}
 */

unsigned short crctable[UCHAR_MAX+1] = {
    /* 000..009 */ 0x0000,0x1021,0x2042,0x3063,0x4084,0x50A5,0x60C6,0x70E7,0x8108,0x9129,
    /* 010..019 */ 0xA14A,0xB16B,0xC18C,0xD1AD,0xE1CE,0xF1EF,0x1231,0x0210,0x3273,0x2252,
    /* 020..029 */ 0x52B5,0x4294,0x72F7,0x62D6,0x9339,0x8318,0xB37B,0xA35A,0xD3BD,0xC39C,
    /* 030..039 */ 0xF3FF,0xE3DE,0x2462,0x3443,0x0420,0x1401,0x64E6,0x74C7,0x44A4,0x5485,
    /* 040..049 */ 0xA56A,0xB54B,0x8528,0x9509,0xE5EE,0xF5CF,0xC5AC,0xD58D,0x3653,0x2672,
    /* 050..059 */ 0x1611,0x0630,0x76D7,0x66F6,0x5695,0x46B4,0xB75B,0xA77A,0x9719,0x8738,
    /* 060..069 */ 0xF7DF,0xE7FE,0xD79D,0xC7BC,0x48C4,0x58E5,0x6886,0x78A7,0x0840,0x1861,
    /* 070..079 */ 0x2802,0x3823,0xC9CC,0xD9ED,0xE98E,0xF9AF,0x8948,0x9969,0xA90A,0xB92B,
    /* 080..089 */ 0x5AF5,0x4AD4,0x7AB7,0x6A96,0x1A71,0x0A50,0x3A33,0x2A12,0xDBFD,0xCBDC,
    /* 090..099 */ 0xFBBF,0xEB9E,0x9B79,0x8B58,0xBB3B,0xAB1A,0x6CA6,0x7C87,0x4CE4,0x5CC5,
    /* 100..109 */ 0x2C22,0x3C03,0x0C60,0x1C41,0xEDAE,0xFD8F,0xCDEC,0xDDCD,0xAD2A,0xBD0B,
    /* 110..119 */ 0x8D68,0x9D49,0x7E97,0x6EB6,0x5ED5,0x4EF4,0x3E13,0x2E32,0x1E51,0x0E70,
    /* 120..129 */ 0xFF9F,0xEFBE,0xDFDD,0xCFFC,0xBF1B,0xAF3A,0x9F59,0x8F78,0x9188,0x81A9,
    /* 130..139 */ 0xB1CA,0xA1EB,0xD10C,0xC12D,0xF14E,0xE16F,0x1080,0x00A1,0x30C2,0x20E3,
    /* 140..149 */ 0x5004,0x4025,0x7046,0x6067,0x83B9,0x9398,0xA3FB,0xB3DA,0xC33D,0xD31C,
    /* 150..159 */ 0xE37F,0xF35E,0x02B1,0x1290,0x22F3,0x32D2,0x4235,0x5214,0x6277,0x7256,
    /* 160..169 */ 0xB5EA,0xA5CB,0x95A8,0x8589,0xF56E,0xE54F,0xD52C,0xC50D,0x34E2,0x24C3,
    /* 170..179 */ 0x14A0,0x0481,0x7466,0x6447,0x5424,0x4405,0xA7DB,0xB7FA,0x8799,0x97B8,
    /* 180..189 */ 0xE75F,0xF77E,0xC71D,0xD73C,0x26D3,0x36F2,0x0691,0x16B0,0x6657,0x7676,
    /* 190..199 */ 0x4615,0x5634,0xD94C,0xC96D,0xF90E,0xE92F,0x99C8,0x89E9,0xB98A,0xA9AB,
    /* 200..209 */ 0x5844,0x4865,0x7806,0x6827,0x18C0,0x08E1,0x3882,0x28A3,0xCB7D,0xDB5C,
    /* 210..219 */ 0xEB3F,0xFB1E,0x8BF9,0x9BD8,0xABBB,0xBB9A,0x4A75,0x5A54,0x6A37,0x7A16,
    /* 220..229 */ 0x0AF1,0x1AD0,0x2AB3,0x3A92,0xFD2E,0xED0F,0xDD6C,0xCD4D,0xBDAA,0xAD8B,
    /* 230..239 */ 0x9DE8,0x8DC9,0x7C26,0x6C07,0x5C64,0x4C45,0x3CA2,0x2C83,0x1CE0,0x0CC1,
    /* 240..249 */ 0xEF1F,0xFF3E,0xCF5D,0xDF7C,0xAF9B,0xBFBA,0x8FD9,0x9FF8,0x6E17,0x7E36,
    /* 250..255 */ 0x4E55,0x5E74,0x2E93,0x3EB2,0x0ED1,0x1EF0
};


/*******************************************************************************
Function:  cUtilInit()
Purpose:   Initialize C utilities
*******************************************************************************/
void	cUtilInit()
{
	clkRate = sysClkRateGet();
}

/*******************************************************************************
Function:  msToTicksX
Returns:   number of ticks
Purpose:   To convert milliseconds to tick counts in a platform independent way.
This version does not need initialized. It is initialized by the first call,
if required.
*******************************************************************************/
int		msToTicksX(int msec)
{
	int ticks;
	if ( clkRate == -1 )
	{	clkRate = sysClkRateGet();
	}
	ticks = msec*clkRate / 1000;
	assert(clkRate != -1);
	if (ticks == 0) ticks = 1;
	return ticks;
}

/*******************************************************************************
Function:  ticksToMsX
Returns:   number of ms
Purpose:   To convert ticks to milliseconds in a platform independent way.
This version does not need initialized. It is initialized by the first call,
if required.
*******************************************************************************/
int	ticksToMs(int ticks)
{
	int msec;
	if ( clkRate == -1 )
	{	clkRate = sysClkRateGet();
	}
	msec = (ticks*1000) / clkRate;
	return msec;
}


/*******************************************************************************
Function:  msToTicks
Returns:   number of ticks
Purpose:   To convert milliseconds to tick counts in a platform independent way.
*******************************************************************************/
int		msToTicks(int msec)
{
	int ticks;
	if ( clkRate == -1 )
	{	clkRate = sysClkRateGet();
	}
	ticks = msec*clkRate / 1000;
	assert(clkRate != -1);
	if (ticks == 0) ticks = 1;
	return ticks;
}

/*******************************************************************************
Function:  CRC16
Returns:   16 bit CRC computed.
Purpose:   To compute the 16 bit CRC for a given buffer.

  CAUTION = this routine writes the CRC in two bytes following the supplied data

Comments:  None.
*******************************************************************************/
void LtCRC16(byte bufInOut[], int sizeIn)
{
	unsigned int crc = USHRT_MAX;
	int i;
	byte* p = bufInOut;

	for (i = 0; i < sizeIn; i++)
	{
	   crc = (crc << CHAR_BIT) ^ crctable[(byte)(crc >> (16 - CHAR_BIT)) ^ *p++];
	}
	crc = ~crc & USHRT_MAX;

	bufInOut[sizeIn]     = (crc >> 8);
	bufInOut[sizeIn + 1] = (crc & 0x00FF);
}

