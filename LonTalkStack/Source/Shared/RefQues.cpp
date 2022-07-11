/***************************************************************
 *  Filename: RefQues.cpp
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
 *  Description:  Implementation of Reference Queues Services.
 *
 *	DJ Duffy Oct 1998
 *
 ****************************************************************/ 
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Shared/RefQues.cpp#1 $
//
/*
 * $Log: /Dev/Shared/RefQues.cpp $
 * 
 * 18    6/14/02 5:59p Fremont
 * remove warnings
 * 
 * 17    3/14/00 9:53a Darrelld
 * remove ($)Log
 * 
 * 16    2/23/00 9:07a Darrelld
 * LNS 3.0 Merge
 * 
 * 14    11/18/99 3:04p Darrelld
 * Convert binary semaphores to mutexes
 * 
 * 13    8/31/99 10:29a Glen
 * Add new LtWaitQue class
 * 
 * 12    7/27/99 1:46p Darrelld
 * LtQue: Use a Msemaphore to allow nesting
 * 
 * 11    4/13/99 3:01p Darrelld
 * Add some copyMessage members
 * 
 * 10    3/02/99 8:51a Glen
 * UNIX compatibility
 * 
 * 9     3/01/99 6:55p Darrelld
 * Tweak callback mechanism
 * 
 * 8     3/01/99 2:47p Glen
 * New allocator release callback
 * 
 * 7     2/11/99 12:08p Darrelld
 * Avoid extra lock in LtRefQue receive
 * 
 * 6     2/11/99 11:00a Darrelld
 * Performance enhancement and testing
 * 
 * 5     2/01/99 4:38p Darrelld
 * Keep semaphores to avoid creating them in LtMsgRef
 * 
 * 4     2/01/99 3:39p Darrelld
 * Fix for operation with smart packet allocators
 * 
 * 3     1/22/99 7:15p Glen
 * Reversed SEM_FULL and SEM_EMPTY
 * 
 * 2     1/22/99 9:48a Glen
 * 
 * 1     1/22/99 9:46a Glen
 * 
 * 10    11/24/98 4:28p Darrelld
 * Fix memory leak
 * 
 * 9     11/24/98 4:02p Darrelld
 * Fix release comments
 * 
 * 8     11/24/98 2:13p Darrelld
 * remove dispose since it was a bad idea in the first place
 * 
 * 7     11/24/98 11:30a Darrelld
 * LPSTR to byte*
 * 
 * 6     11/24/98 11:15a Darrelld
 * Properly dispose of the master
 * 
 * 5     11/09/98 2:05p Darrelld
 * Updates for Windows operation
 * 
 * 4     11/09/98 11:58a Darrelld
 * Updates after native trial
 * 
 * 3     11/06/98 9:42a Darrelld
 * Fix some problems and update the queue classes
 * 
 * 2     10/29/98 12:08p Darrelld
 * mods to project. Time server / client work.
 * 
 * 1     10/19/98 2:31p Darrelld
 * Vx Layer Sources
 * 
 * 2     10/16/98 4:17p Darrelld
 * Fix memory leaks
 * 
 * 1     10/16/98 2:34p Darrelld
 * Vx Layer Source
 */

// Do not include windows
// this file is built on VxWorks

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <vxWorks.h>

#include "LtaDefine.h"

#include "RefQues.h"
#if 0
#ifndef _DEBUG
#define _DEBUG 1
#endif


//#define DEBUG_NEW new(THIS_FILE, __LINE__)
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#endif

// These queues must be portable to VxWorks, so use
// VxWorks services, not Win32 services here.


//////////////////////////////////////////////////////////////////
//
// The Locked Queue class 
//
//
// General protected FIFO / LIFO queue class
// For use as base class for messages, etc.


//
// Constructors
//
// assume item
LtQue::LtQue()
{
	init( false, false );
}

//
// init as head
//

int		nLtQueSems = 0;

// constructor
//
// allow options for as head and locked or not to allow
// for case where a head is used inside another locked structure.
//
LtQue::LtQue( BOOL bAsHead, BOOL bLocked )
{
	init( bAsHead, bLocked );
}

void	LtQue::init( BOOL bAsHead, BOOL bLocked )
{
	m_pNxt = NULL;
	m_pPrv = NULL;
	m_pHead = NULL;
	m_sem = NULL;
	m_nCount = 0;
	m_bHead = FALSE;
	m_bLocked = FALSE;
	if ( bAsHead )
	{
		m_pNxt = this;
		m_pPrv = this;
		m_bHead = TRUE;
		if ( bLocked )
		{
			m_bLocked = TRUE;
			//m_sem = semBCreate( SEM_Q_FIFO, SEM_FULL );
			// we need semaphores that can be nested for this one
			m_sem = semMCreate( SEM_Q_PRIORITY | SEM_INVERSION_SAFE );
			nLtQueSems++;
			assert( m_sem != NULL );
			// if error, and sem is NULL
		}
	}
}


//
// Destructor
//
// toss out the sem if I'm a head
//
LtQue::~LtQue()
{
	destruct();
}

void LtQue::destruct()
{
	// Don't destruct if we are on a queue
	assert( m_pHead == NULL );
	if ( m_bLocked && m_sem )
	{
		semDelete( m_sem );
		nLtQueSems--;
	}
}

//
// InitHead
//
// Queue head services
// call on head of queue only
//
void	LtQue::initHead()
{
	init(true);
}

//
// IsEmpty
//
// Is this queue empty? Assume Locked
//
BOOL	LtQue::isEmpty()
{
	BOOL	bEmpty = FALSE;

	assert( m_bHead );
	bEmpty = m_nCount == 0;

	if ( bEmpty )
	{	assert( m_pNxt==this && m_pPrv == this );
	}
	else
	{	assert( m_pNxt!=this && m_pPrv != this );
	}

	return bEmpty;
}

//
// LockedIsEmpty
//
// Is this queue empty? Under a lock
//
BOOL	LtQue::lockedIsEmpty()
{
	BOOL	bEmpty = FALSE;

	assert( m_bHead );

	lock();
	bEmpty = m_nCount == 0;

	if ( bEmpty )
	{	assert( m_pNxt==this && m_pPrv == this );
	}
	else
	{	assert( m_pNxt!=this && m_pPrv != this );
	}

	unlock();
	return bEmpty;
}


//
// InsertHead
//
// Current item is the head
//
// True if first item added
//
BOOL	LtQue::insertHead( LtQue* pItem )
{
	BOOL	bFirst;

	assert( m_bHead );
	lock();
	bFirst = isEmpty();
	// Insert the item, with the head as predecessor
	pItem->insert( this );
	// Item is now on a queue
	pItem->m_pHead = this;
	m_nCount++;
	unlock();
	return bFirst;
}

//
// InsertTail
//
// True if first item added
//
BOOL	LtQue::insertTail( LtQue* pItem )
{
	LtQue*	pPrev;
	BOOL	bFirst;

	assert( m_bHead );
	lock();
	bFirst = isEmpty();
	pPrev = m_pPrv;
	assert( pPrev );
	pItem->insert( pPrev );
	// Item is now on a queue
	pItem->m_pHead = this;
	m_nCount++;
	unlock();
	return bFirst;
}

//
// RemoveHead
//
// True if item removed
//
BOOL	LtQue::removeHead( LtQue** ppItem )
{
	BOOL	bEmpty;
	LtQue*	pItem = NULL;

	assert( m_bHead );
	lock();
	bEmpty = isEmpty();
	if ( !bEmpty )
	{
		pItem = m_pNxt;
		// Must be on a queue
		assert( pItem->m_pHead );
		pItem->remove();
		m_nCount--;
	}

	unlock();
	*ppItem = pItem;
	return !bEmpty;
}

//
// RemoveTail
//
// True if item removed
//
BOOL	LtQue::removeTail( LtQue** ppItem )
{
	BOOL	bEmpty;
	LtQue*	pItem = NULL;

	assert( m_bHead );
	lock();
	bEmpty = isEmpty();
	if ( !bEmpty )
	{
		pItem = m_pPrv;
		// must be on a queue
		assert( pItem->m_pHead );
		pItem->remove();
		m_nCount--;
	}

	unlock();
	*ppItem = pItem;
	return !bEmpty;
}



// Queue item services
// call on an item of a queue only, not on head
// Only needed for non-FIFO or non-LIFO queues

// default constructor assumes this
void	LtQue::initItem()
{
	init( false );
}

//
// Remove
//
// Remove the current item from any queue
// Don't call on a head obviously
// No locking is done. Assume that the head is already locked
//
void	LtQue::remove()
{
	LtQue*	pPrev = m_pPrv;
	LtQue*	pNext = m_pNxt;

	pPrev->m_pNxt = pNext;
	pNext->m_pPrv = pPrev;
	m_pNxt = NULL;		// Tidy for debug
	m_pPrv = NULL;
	// No longer on a queue
	m_pHead = NULL;
}

//
// Insert
//
// Insert this item just after another item, which might be
// a head
// No locking is done
//
void	LtQue::insert( LtQue* pPrev )
{
	LtQue*	pNext;

	// Crash on double insert
	assert( m_pHead == NULL );

	pNext = pPrev->m_pNxt;
	m_pNxt = pNext;
	m_pPrv = pPrev;
	pNext->m_pPrv = this;
	pPrev->m_pNxt = this;
}

// Queue head services that are protected
// Must be head to lock
void	LtQue::lock()
{
	__UNUSED__ STATUS	sts;

	// we may want an assert that doesn't go away
	// when debug is not defined
	assert( m_bHead );
	if ( m_bLocked )		// we allow non-lockable heads now
	{
		assert( m_sem != NULL );
		sts = semTake( m_sem, WAIT_FOREVER );
		assert( sts == OK );
	}
}


void	LtQue::unlock()
{
	__UNUSED__ STATUS	sts;

	// we may want an assert that doesn't go away
	// when debug is not defined
	assert( m_bHead );
	if ( m_bLocked )		// allow non-lockable heads
	{
		assert( m_sem != NULL );
		sts = semGive( m_sem );
		assert( sts == OK );
	}
}


//////////////////////////////////////////////////////////////////
//
// Reference Queue
//
// This class is the message queue that sends and receives messages
//

//
// Constructor
//
LtWaitQue::LtWaitQue() :
		m_qMsgs( true )				// This is a queue head
{
	// FULL so receivers block until a message arrives
	m_semReceiveWait = semBCreate( SEM_Q_FIFO, SEM_FULL );
	m_tidWaitor = 0;	// no waitor
}

//
// Destructor
//
LtWaitQue::~LtWaitQue()
{
	semDelete( m_semReceiveWait );
}



//
// Send
//
//	Send a message, timeout unnecessary, since resources always available
//
STATUS	LtWaitQue::send( LtQue* pMsg )
{

	STATUS	sts = OK;
	BOOL	bEmpty = FALSE;

	bEmpty = m_qMsgs.insertHead( pMsg );

	// Only wait if the queue is empty
	if ( bEmpty )
	{
		sts = semGive( m_semReceiveWait );
	}

	return sts;
}

//
// Receive
//
// Receive a message or timeout in the standard way
//
STATUS	LtWaitQue::receive( LtQue** ppMsg, int timeout )
{
	STATUS	sts = OK;
	//BOOL	bTaken = FALSE;

	// Make sure we return nothing
	*ppMsg = NULL;

#if 0 // avoid extra lock
	// Only wait if the queue is empty
	if ( m_qMsgs.lockedIsEmpty() )
	{
		sts = semTake( m_semReceiveWait, timeout );
		if ( sts == OK )
		{	bTaken = TRUE;
		}
	}
#endif // avoid extra lock

	// sts == OK if message waiting, or we got the semaphore,
	// meaning message arrived
	// sts == ERROR on timeout
	while ( sts == OK )
	{
		// Try to remove a message, and if so then
		if ( m_qMsgs.removeTail( ppMsg ) )
		{
			break;	// got a message so outta here
		}
		else
		{
			//sts = ERROR;
			// we got no message, so we gotta wait
			sts = semTake( m_semReceiveWait, timeout );
			if ( sts != OK )
			{
				break;	// timed out and no message
			}
		}
	}
	// If we took the semaphore, then give it back
	// ACTUALLY - this is a bug. If we got it or if we didn't
	// doesn't matter, we want to leave the semaphore locked
	// since it will be "given" again when the message queue
	// goes from EMPTY to NON-EMPTY
	//if ( bTaken )
	//{
		//semGive( m_semReceiveWait );
	//}

	return sts;
}

//
// Flush
//
// Remove and free all messages from the queue
//

STATUS	LtRefQue::flush()
{
	LtQue*	pQmsg;
	LtMsgRef*	pRmsg;

	while ( ! m_qMsgs.lockedIsEmpty() )
	{
		// Try to remove a message, and if so then
		if ( m_qMsgs.removeTail( &pQmsg ) )
		{
			pRmsg = (LtMsgRef*)pQmsg;
			pRmsg->release();
		}
	}

	return OK;

}



//////////////////////////////////////////////////////////////////
//
// The Message Reference class
//
// This class holds the reference to the message data itself.
// There are two allocators. Both are only used for the master
// One is for the data, and the other is for the master LtRefQue object.
// If NULL then 'free()'  or 'delete' is used.
//
#if 0 // commentary
int				m_nRef;			// ref count on this
char*			m_pBuf;			// address of data
int				m_nSize;		// size of data
LtRefQue*		m_pRefQue;		// queue that we last used
// We have to solve the problem of multiple threads wanting to
// release the master at the same time.
SEM_ID			m_semMasterLock;// master lock
BOOL			m_bFreeMaster;	// free master on last deRef
CMRef*			m_pMaster;		// master message
LtMsgAllocator*	m_pAllocator;	// the allocator used
#endif // commentary

//
// constructor
//
LtMsgRef::LtMsgRef( BOOL bMaster )
{
	m_semMasterLock	= NULL;
	init( bMaster );
}

//
// init
//
// used when allocated without constructor
//
void	LtMsgRef::init( BOOL bMaster )
{
	LtQue::init(false);
	m_nRef			= 0;
	m_pBuf			= NULL;
	m_pBlk			= NULL;
	m_nSize			= 0;
	//m_semMasterLock	= NULL;
	m_bFreeMaster	= FALSE;
	m_pMaster		= NULL;
	m_pRefQue		= NULL;
	m_pAllocator	= NULL;

	if ( bMaster )
	{	makeMaster( true );
	}
}


//
// destructor
//
LtMsgRef::~LtMsgRef()
{
	destruct();
	// Free the master lock if we are a master message
	if ( m_semMasterLock )
	{
		semDelete( m_semMasterLock );
		m_semMasterLock = NULL;
	}
}

//
// destruct
//
// called by allocator that queues the msg refs
//
void LtMsgRef::destruct()
{
	// If we have a buffer at this stage, and we are master
	// the we are supposed to free it.
	// Allocator must zero these fields before delete to avoid this.
	if ( m_pMaster == NULL  && m_pBlk )
	{
		assert( m_pAllocator == NULL );
		::free( m_pBlk ); // stdlib.h
		m_pBlk = NULL;
	}
	m_nRef = 0;
	LtQue::destruct();
}


// Call on non-master only
//
// SetMaster
//
// Set the master message pointer
// Take care when resetting, since ref counts can be messed up.
//
void	LtMsgRef::setMaster( LtMsgRef* pMas )
{
	m_pMaster = pMas;
}

//
// getMaster
//
// Return the master message pointer for this reference
// So if it's us, then return this, else return the pointer
// to the master.
//
LtMsgRef*	LtMsgRef::getMaster()
{
	if ( m_pMaster == NULL )
	{	return this;
	}
	else
	{	return m_pMaster;
	}
}

// Call on a master only.
//
// MakeMaster
//
// Make the current reference a master message
//
void	LtMsgRef::makeMaster( BOOL bFree )
{
	assert( m_pMaster == 0 );
	// Only create the master lock if we need to
	if ( m_semMasterLock == NULL )
	{
		m_semMasterLock = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
	}
	assert( m_semMasterLock);
	m_bFreeMaster = bFree;
}

//
// alloc
//
// allocate a message from the allocator and return it
//
byte*	LtMsgRef::alloc()
{
	assert( m_pAllocator );
	assert( m_pMaster == 0 );
	assert( m_semMasterLock );
	m_pBlk = m_pAllocator->alloc();
	return m_pBlk;
}

//
// LockMaster
//
// Lock the master of this message, or ourselves.
// So we can fiddle with the reference count probably.
//
LtMsgRef*	LtMsgRef::lockMaster()
{
	LtMsgRef*		pMaster = m_pMaster? m_pMaster : this;
	assert( pMaster->m_pMaster == NULL );
	assert( pMaster->m_semMasterLock );
	semTake( pMaster->m_semMasterLock, WAIT_FOREVER );
	return pMaster;
}

//
// UnlockMaster
//
// Unlock the master for this reference [ or ourselves ]
// so we can fiddle with the reference count
//
LtMsgRef*	LtMsgRef::unlockMaster()
{
	LtMsgRef*		pMaster = m_pMaster? m_pMaster : this;
	assert( pMaster->m_pMaster == NULL );
	assert( pMaster->m_semMasterLock );
	semGive( pMaster->m_semMasterLock );
	return pMaster;
}


//
// SetMessage
//
// Declare a message for this reference and optionally declare
// a master for the message as well.
//
void	LtMsgRef::setMessageData( byte* pBuf, int nSize, LtMsgRef* pMas )
{
	m_pBuf = pBuf;
	m_nSize = nSize;
	// Allow redundant setting of ourselves as
	// the master
	if ( pMas && pMas != this)
	{
		assert( m_pMaster == NULL );
		m_pMaster = pMas;
	}
	else
	{	// this is the master block
		// If we don't know by this time, then the m_pBlk
		// needs to be set to this data
		// It can be reset later with no check
		if ( m_pBlk == NULL )
		{	m_pBlk = pBuf;
		}
	}
	// Add a reference to our master message
	addRef();
}
//
// copyMessage
//
// Copy the data portion of the message.
// Dont muck with whether or not the source or target are masters etc.
// Dont muck with the ref counts
//
void	LtMsgRef::copyMessage( LtMsgRef* pMsg )
{
	int		nBytes = pMsg->getDataSize();
	if ( m_pAllocator )
	{
		if ( nBytes > getBlockSize() )
		{	assert(false);
			nBytes = getBlockSize();
		}
	}
	memcpy( getBlock(), pMsg->getDataPtr(), nBytes );
	setMessageNoRef( getBlock(), nBytes );
}


//
// getMessageData
//
// Return message pointer and the size of the message
//
int		LtMsgRef::getMessageData( byte** ppBuf )
{
	*ppBuf = m_pBuf;
	return m_nSize;
}

//
// cloneMessage
//
// Return another reference to this message
//
LtMsgRef* LtMsgRef::cloneMessage()
{
	LtMsgRef*			pMas = getMaster();
	LtMsgRef*			pNew = NULL;
	LtMsgAllocator*		pAlloc = pMas->m_pAllocator;
	do
	{
		if ( NULL == pAlloc )
		{	pNew = new LtMsgRef();
			assert( pNew );
		}
		else
		{	pNew = pAlloc->allocMsgRef();
			// No MsgRefs left, then bug out and return NULL
			if ( pNew == NULL )
			{	break;
			}
			// Allocator is responsible for setting the allocator
			// of the new message ref to the same as old one
			assert( pNew->m_pAllocator == pAlloc );
		}
		pNew->setMessageData( m_pBuf, m_nSize, pMas );
	} while (false);
	return pNew;
}


//
// AddRef
//
// Add a reference on the master message
//
void	LtMsgRef::addRef()
{
	LtMsgRef*	pMaster;
	pMaster = lockMaster();
	pMaster->m_nRef++;
	unlockMaster();
}

//
// Release
//
// Release a reference on the master message
// Free this message whether or not it has a deallocator.
// Return true if message was actually freed.
//
// There are four cases:
//
// 1 -	pMas = Alloc()
//		pMas->SetMessage(...)	// adds a ref to master
//		Que.send( pMas );		// send the master away
//
// 2 -	pMas = Alloc()
//		pMas->AddRef()			// protect master. No setMessageData
//		for( ... )				// one or more trips around here
//		{
//			pMsg = Alloc()
//			pMsg->SetMessage(... )	// adds a ref to master
//			Que.send( pMsg );		// send a ref
//		}
//		pMas->release()			// release the master without ever sending it
//
// 3 -	pMas = Alloc()
//		pMas->SetMessage( ... )	// adds a ref
//		for (...)				// one or more trips around here
//		{
//			pMsg = pMas.cloneMessage()	// adds a ref too
//			Que.send( pMsg )
//		}
//		Que.send( pMas )		// send the master too
//
// 4 -	pMas = Alloc()				// 
//		pMas->SetMessageData(...)	// set a messgage
//		pMas->AddRef()				// additional count for protection
//		for (...)
//		{
//			pMsg = pMas->cloneMessage()
//			Que.send( pMsg )
//		}
//		pMas.release()				// discard the master
//
//		Case four is possible, but note that a receiver of the message may
//		modify it in other ways than a release. If receiver releases it
//		all is well.
//
void	LtMsgRef::release()
{
	LtMsgRef*			pMaster;
	LtMsgAllocator*		pAlloc = null;

	BOOL	bFree = FALSE;
	pMaster = lockMaster();
	if ( 0 == --(pMaster->m_nRef) )
	{	bFree = pMaster->m_bFreeMaster;
		pAlloc = pMaster->m_pAllocator;
	}
	unlockMaster();

	// Free the master block if we were told to when ref count goes to zero
	// Since we are the last one to release it, we don't need a lock since
	// there are no others referencing the message.
	if ( bFree )
	{
		if ( pAlloc )
		{
			// Free the buffer with the allocator, then
			// forget about it so destructor does not
			pAlloc->free( (byte*)pMaster->m_pBlk );
		}
		else
		{
			::free( pMaster->m_pBlk );
		}
		// We are freeing the master, so if it's not us, then
		// dispose of the master and dispose of us
		if ( pMaster != this )
		{
			pMaster->dispose();
		}
		// Tidy things up so that we don't inadvertently look at stale data
		// now dispose of us.
		dispose();
		// Make sure we don't try to use this master again below.
		pMaster = null;
	}
	else
	{
		// We did not free the master.
		// So if this is not the master, then we need to dispose of us.
		if ( pMaster != this )
		{
			// dispose of us properly
			dispose();
		}
	}

	// Notify the owner of the allocator of this release.  Only master
	// releases are notified (whether the master is freed or not).  If the
	// master was freed, null is reported.
	if (pAlloc)
	{	
		if (!pAlloc->returnMaster( pMaster ) && pMaster != null)
		{
			// Failed to return master and we didn't free it so free it now.
			pAlloc->free( (byte*)pMaster->m_pBlk );
			pMaster->dispose();
		}
	}
	// BEWARE - The object you called this on is GONE or soon will be
}

//
// setAllocator
//
//
void LtMsgRef::setAllocator( LtMsgAllocator* pAlloc )
{
	assert(m_pAllocator == NULL );
	m_pAllocator = pAlloc;
}

//
// getAllocator
//
//
LtMsgAllocator*	LtMsgRef::getAllocator()
{
	return m_pAllocator;
}

//
// dispose
//
// dispose of us using the allocator or delete
// PROTECTED: NOT CALLED ANYMORE BY CLIENTS
//
void	LtMsgRef::dispose()
{
	LtMsgAllocator*	pAlloc = m_pAllocator;
	m_pBlk = null;
	m_pMaster = null;
	if ( pAlloc )
	{
		destruct();
		pAlloc->free( this );
	}
	else
	{	// destruct called by destructor, so allocator must be NULL
		m_pAllocator = NULL;
		delete this;
	}
}

// end

