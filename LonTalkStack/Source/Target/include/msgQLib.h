/***************************************************************
 *  Filename: msgQLib.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Target/include/msgQLib.h#1 $
//
/*
 * $Log: /VNIstack/LNS_V3.2x/Target/include/msgQLib.h $
 * 
 * 6     4/07/04 4:38p Rjain
 * Added new VxLayer Dll. 
 * 
 * 4     1/22/99 9:11a Glen
 * add confidential statement
 * 
 * 3     12/14/98 9:30a Glen
 * Add msgQNumMsgs
 * 
 * 2     11/09/98 11:56a Darrelld
 * Updates after native trial
 * 
 * 1     11/04/98 8:40a Darrelld
 * Windows Target header files
 */


#ifndef _MSGQLIB_INCLUDE_H
#define _MSGQLIB_INCLUDE_H 1
#include <vxWorks.h>
#include "VxLayerDll.h" // VXLAYER_API
#include <VxLayer.h>
#include <VxlPrivate.h>
#ifdef __cplusplus
extern "C"
{
#endif


//
// Message Queue types and data
//
#define MSG_Q_FIFO 0x00
#define MSG_Q_PRIORITY 0x01
#define MSG_PRI_NORMAL 0x00
#define MSG_PRI_URGENT 0x01

typedef struct _MSGSTUFF
{
	int		maxMsgs;
	int		maxMsgLength;
	int		options;
	OsalHandle	hEvent;			// event to wait on
	Que		qHead;			// Lockable queue for messages

}	MSGSTUFF;


typedef struct _MSGSTUFF* MSG_Q_ID;

VXLAYER_API
MSG_Q_ID	msgQCreate( int maxMsg, int maxLen, int options );

VXLAYER_API
STATUS		msgQDelete( MSG_Q_ID msgQ );

VXLAYER_API
STATUS		msgQSend( MSG_Q_ID msgQ, char* buf, int nBytes, int timeout, int priority );

VXLAYER_API
int			msgQReceive( MSG_Q_ID msgQ, char* buf, int maxBytes, 
						int timeout );

VXLAYER_API
int			msgQNumMsgs( MSG_Q_ID msgQ );



#ifdef  __cplusplus
}
#endif

#endif // _MSGQLIB_INCLUDE_H

