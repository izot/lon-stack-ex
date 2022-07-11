/***************************************************************
 *  Filename: VxMsgQ.c
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
 *  Description:  Implementation of Message Queues for VxWorks emulation layer.
 *
 *	DJ Duffy Oct 1998
 *
 ****************************************************************/

/*
 * $Log: /VNIstack/Dcx_Dev/VxLayer/VxMsgQ.c $
 * 
 * 10    6/24/08 7:56a Glen
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
 * 9     3/26/07 2:39p Bobw
 * EPRS FIXED: 
 * 
 * 8     4/07/04 4:38p Rjain
 * Added new VxLayer Dll. 
 * 
 * 7     7/03/02 4:22p Glen
 * msgQReceive() was returning 0 instead of ERROR!
 * 
 * 6     12/21/99 3:34p Glen
 * Protect queue count within lock
 * 
 * 4     12/14/98 9:30a Glen
 * Add msgQNumMsgs
 * 
 * 3     11/09/98 11:58a Darrelld
 * Updates after native trial
 * 
 * 2     11/06/98 9:42a Darrelld
 * Fix some problems and update the queue classes
 * 
 * 1     10/16/98 1:43p Darrelld
 * Vx Layer Sources
 */

#include <stdlib.h>
#include <string.h>
#include "Osal.h"

// Definitions for this layer
// Conflict with Windows. They think this is zero.
// Imagine the arrogance.
//#undef ERROR
#include <VxWorks.h>
#include <VxLayer.h>
#include <VxlPrivate.h>
#include <msgQLib.h>
#include "VxLayerDll.h" // VXLAYER_API


////////////////////////////////////////////////////////////////////
//
// Message Queues
//
// These message services COPY the data. They are not high performance,
// but they are "safer" than the reference services.
// 
//

typedef struct _MSGQMSG
{
	// *** ORDER DEPENDENT ***
	Que		qItem;			// queue item [ beware, never use as a head ]
	int		nBytes;			// number of bytes
	char	bytes[1];		// message starts here
	// *** ORDER DEPENDENT ***
} MSGQMSG;


//
// msgQCreate
//
// Create a message queue and it's signalling event
//
VXLAYER_API
MSG_Q_ID	msgQCreate( int maxMsg, int maxLen, int options )
{
	MSG_Q_ID	pMQ = (MSGSTUFF*)malloc( sizeof(MSGSTUFF) );

	vxlTrace( "msgQCreate - MSG_Q_ID 0x%08x\n", pMQ );

	while ( pMQ )	// So's we can break out
	{
		// make clean up easy.
		memset( pMQ, 0, sizeof(MSGSTUFF) );
		// We got one, now init it
		// no security, autoreset, init off, no name
		if (OsalCreateEvent(&pMQ->hEvent) != OSALSTS_SUCCESS)
		{	vxlReportLastError("msgQCreate - OsalCreateEvent");
			msgQDelete( pMQ );
			pMQ = NULL;
			break;
		}
		QueInit( &pMQ->qHead, TRUE );
		pMQ->maxMsgs = maxMsg;
		pMQ->maxMsgLength = maxLen;
		pMQ->options = options;

		break;
	}

	return pMQ;
}

//
// msgQDelete
//
// Delete the messages, the queue and the event if we have one.
// Then free the stucture itself
//
VXLAYER_API
STATUS	msgQDelete( MSG_Q_ID msgQ )
{
    OsalDeleteEvent(&msgQ->hEvent);
	// We own the messages, so delete them
	QueFreeAll( &msgQ->qHead );
	// Now zap and clean up the queue itself
	QueDelete( &msgQ->qHead );
	free( msgQ );
	return OK;
}

//
// msgQSend
//
// Copy a message and queue it to the receiver
// Only set the event when the queue goes from empty to non-empty
// ignore priority intentially
//
VXLAYER_API
STATUS	msgQSend( MSG_Q_ID msgQ, char* buf, int nBytes, int timeout, int priority )
{
	int			nSize = sizeof(MSGQMSG) + nBytes;
	MSGQMSG*	pMsg;
	STATUS		sts = OK;

	if ( msgQ->qHead.pNxt == NULL || !msgQ->qHead.bLock )
	{
		vxlReportError("msgQSend - invalid queue");
		return ERROR;
	}
	pMsg = (MSGQMSG*)malloc( nSize );
	if ( pMsg == NULL )
	{	vxlReportError("msgQSend - message allocation failure");
		return ERROR;
	}
	memcpy( &pMsg->bytes, buf, nBytes );
	pMsg->nBytes = nBytes;
	if ( QueInsertHead( &msgQ->qHead, &pMsg->qItem ) )
	{
		if ( OsalSetEvent( msgQ->hEvent ) != OSALSTS_SUCCESS)
		{	vxlReportLastError("msgQSend - SetEvent failed");
			sts = ERROR;
		}
	}
	
	return sts;
}


//
// msgQReceive
//
// Return a message and obey timeout
// Number of bytes or ERROR for no message
//
VXLAYER_API
int	msgQReceive( MSG_Q_ID msgQ, char* buf, int maxBytes, 
						int timeout )
{
	int			nBytes = 0;
	MSGQMSG*	pMsg;
	Que*		pItem = NULL;
    
	OsalStatus  osalSts;

	if ( msgQ->qHead.pNxt == NULL || !msgQ->qHead.bLock )
	{
		vxlReportError("msgQReceive - invalid queue");
		SetVxErrno( 899 ); // Need a real value here
		return 0;
	}

	// Grab a message if we can, else wait for timeout period.
	while ( !QueRemoveTail( &msgQ->qHead, &pItem ) )
	{
		osalSts = OsalWaitForEvent( msgQ->hEvent, timeout );
		if ( osalSts == OSALSTS_SUCCESS )
		{	// Event signalled, but it could be a hoax
			// so check for an item, and wait again if not.
			// Event is supposed to be "self clearing".
			continue;
		}
		else if ( osalSts == OSALSTS_TIMEOUT )
		{	// Timeout, so return no message
			break;
		}
		else
		{	// Something untoward happened
			vxlReportLastError("msgQReceive - WAIT failed");
			break;
		}
	}

	// If there's an item, then there's a message.
	// copy the item to the user buffer and then free the message
	if ( pItem )
	{
		pMsg = (MSGQMSG*)pItem;      
		nBytes = maxBytes < pMsg->nBytes ? maxBytes : pMsg->nBytes;
		memcpy( buf, &pMsg->bytes, nBytes );
		free( pItem );
	}

	return (nBytes == 0) ? ERROR : nBytes;	
}

VXLAYER_API
int msgQNumMsgs( MSG_Q_ID msgQ )
{
	return QueElementCount( &msgQ->qHead );
}


// end
