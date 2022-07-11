/***************************************************************
 *  Filename: VxLQues.c
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
 *  Description:  Implementation of Queues Services for VxWorks emulation layer.
 *
 *	DJ Duffy Oct 1998
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/VxLayer/VxLQues.c#1 $
//
/*
 * $Log: /VNIstack/Dcx_Dev/VxLayer/VxLQues.c $
 * 
 * 7     6/24/08 7:56a Glen
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
 * 6     3/26/07 2:39p Bobw
 * EPRS FIXED: 
 * 
 * 5     12/21/99 3:34p Glen
 * Protect queue count within lock
 * 
 * 3     11/09/98 2:05p Darrelld
 * Updates for Windows operation
 * 
 * 2     11/06/98 9:42a Darrelld
 * Fix some problems and update the queue classes
 * 
 * 1     10/19/98 2:31p Darrelld
 * Vx Layer Sources
 * 
 * 1     10/16/98 1:43p Darrelld
 * Vx Layer Sources
 */

// Definitions for this layer
#include <stdlib.h>
#include <VxWorks.h>
#include <VxLayer.h>
#include <VxlPrivate.h>


////////////////////////////////////////////////////////////////////
//
// Queue Services
//
// These services implement doublly linked queues with optional
// critical sections so they work between threads.
// These services are private to VxWorks Layer.
// They must be in 'C' because of that.
// They are not intended for use on VxWorks itself without modification
// since they use Win32 stuff directly.
//


// Init the queue, and maybe a critical section
void	QueInit( Que* pHead, BOOL bLock )
{
	pHead->bLock = bLock;
	pHead->elementCount = 0;
	if ( bLock )
	{
		OsalCreateCriticalSection( &pHead->csLock );
		vxlTrace("QueInit - create lock 0x%08x\n", pHead->csLock );
	}
	pHead->pNxt = pHead;
	pHead->pPrv = pHead;
}

// Remove all items and free them
void	QueFreeAll( Que* pHead )
{
	Que*	pItem;
	QueLock( pHead );
	while ( pHead->pNxt && pHead->pNxt != pHead )
	{
		pItem = pHead->pNxt;
		QueRemove( pHead->pNxt );
		free( pItem );
	}
	pHead->elementCount = 0;
	QueUnlock( pHead );
}

// Zap the queue, and maybe delete the critical section
// No items touched
void	QueDelete( Que* pHead )
{
	if ( pHead->bLock )
	{
		vxlTrace("QueDelete - delete lock 0x%08x\n", pHead->csLock );
		OsalDeleteCriticalSection( & pHead->csLock );
		pHead->bLock = FALSE;
	}
	pHead->pNxt = NULL;
	pHead->pPrv = NULL;
}


void	QueLock( Que* pHead )
{
	if ( pHead->bLock )
	{
		OsalEnterCriticalSection( pHead->csLock );
	}
}

void	QueUnlock( Que* pHead )
{
	if ( pHead->bLock )
	{
		OsalLeaveCriticalSection( pHead->csLock );
	}
}

void	QueInsert( Que* pPrev, Que* pItem )
{
	Que*	pNext;

	//QueLock();	// Don't do this, since callers do it for us.
	pNext = pPrev->pNxt;
	pItem->pNxt = pNext;
	pItem->pPrv = pPrev;
	pNext->pPrv = pItem;
	pPrev->pNxt = pItem;
	//QueUnLock();
}

void	QueRemove( Que* pItem )
{
	Que*	pPrev = pItem->pPrv;
	Que*	pNext = pItem->pNxt;

	pPrev->pNxt = pNext;
	pNext->pPrv = pPrev;
	pItem->pNxt = NULL;		// Tidy for debug
	pItem->pPrv = NULL;
}

BOOL	QueIsEmpty( Que* pHead )
{
	BOOL	bEmpty = FALSE;
	if ( pHead->pNxt == pHead && pHead->pPrv == pHead )
	{	bEmpty = TRUE;
	}
	return bEmpty;
}


//
//	These services lock queues if necessary
//

BOOL	QueInsertTail( Que* pHead, Que* pItem )
{
	Que*	pTail;
	BOOL	bWasEmpty;

	QueLock( pHead );
	pHead->elementCount++;
	bWasEmpty = QueIsEmpty( pHead );
	pTail = pHead->pPrv;
	QueInsert( pTail, pItem );
	QueUnlock( pHead );
	return bWasEmpty;
}


BOOL	QueInsertHead( Que* pHead, Que* pItem )
{
	BOOL	bWasEmpty;

	QueLock( pHead );
	pHead->elementCount++;
	bWasEmpty = QueIsEmpty( pHead );
	QueInsert( pHead, pItem );
	QueUnlock( pHead );
	return bWasEmpty;
}

//
// RemoveTail
//
// Return TRUE if we got an item
//
BOOL	QueRemoveTail( Que* pHead, Que** ppItem )
{
	Que*	pTail;
	BOOL	bGotOne;

	*ppItem = NULL;

	QueLock( pHead );
	if ( (bGotOne = !QueIsEmpty( pHead ) ) )
	{
		pHead->elementCount--;
		pTail = pHead->pPrv;
		QueRemove( pTail );
		*ppItem = pTail;
	}
	QueUnlock( pHead );
	return bGotOne;
}

//
// RemoveHead
//
// Remove an item from head of queue and return
// true if we got one
//
BOOL	QueRemoveHead( Que* pHead, Que** ppItem )
{
	Que*	pItem;
	BOOL	bGotOne;

	*ppItem = NULL;

	QueLock( pHead );
	if ( (bGotOne = !QueIsEmpty( pHead ) ) )
	{
		pHead->elementCount--;
		pItem = pHead->pNxt;
		QueRemove( pItem );
		*ppItem = pItem;
	}
	QueUnlock( pHead );
	return bGotOne;
}

//
// QueElementCount
//
// Get the number of elements in the queue
//
int QueElementCount( Que* pHead )
{
	return pHead->elementCount;
}

//
// QueCheck
//
// Check queue links for up to maxItems
//
BOOL	QueCheck( Que* pHead, int maxItems )
{
	int		i = 0;
	Que*	pItem;
	Que*	pPrev = pHead;
	BOOL	bQok = TRUE;

	QueLock( pHead );
	pItem = pHead->pNxt;
	while ( pItem != pHead && i < maxItems )
	{
		if ( pItem->pPrv != pPrev )
		{
			bQok = FALSE;
			break;
		}
		pPrev = pItem;
		pItem = pItem->pNxt;
		i++;
	}
	return bQok;
}


// end
