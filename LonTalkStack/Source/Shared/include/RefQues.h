#ifndef _REFQUES_H
#define _REFQUES_H
/***************************************************************
 *  Filename: RefQues.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Shared/include/RefQues.h#1 $
//
/*
 * $Log: /VNIstack/Dcx_Dev/Shared/include/RefQues.h $
 * 
 * 14    6/24/08 7:54a Glen
 * Support for EVNI running on the DCM
 * 1. Fix GCC problems such as requirement for virtual destructor and
 * prohibition of <class>::<method> syntax in class definitions
 * 2. Fix up case of include files since Linux is case sensitive
 * 3. Add LonTalk Enhanced Proxy to the stack
 * 4. Changed how certain optional features are controlled at compile time
 * 5. Platform specific stuff for the DC such as file system support
 * 
 * 1     12/05/07 11:15p Mwang
 * 
 * 12    3/13/00 4:01p Darrelld
 * Eliminate VVector
 * 
 * 11    12/08/99 5:18p Glen
 * Support for vnistack.dll
 * 
 * 9     8/31/99 10:30a Glen
 * Add new LtWaitQue class
 * 
 * 8     7/27/99 1:46p Darrelld
 * Open head pointer to allow for derived class access
 * 
 * 7     7/16/99 3:51p Darrelld
 * Allow access to m_pNxt, m_pPrv for reordering classes
 * 
 * 6     4/13/99 3:01p Darrelld
 * Add some copyMessage members
 * 
 * 5     4/06/99 3:34p Darrelld
 * Add isQueued to LtQue
 * 
 * 4     3/01/99 6:56p Darrelld
 * Tweak callback mechanism
 * 
 * 3     3/01/99 2:19p Glen
 * New allocator release callback
 * 
 * 2     2/11/99 11:00a Darrelld
 * Performance enhancement and testing
 * 
 * 1     1/22/99 9:49a Glen
 * 
 * 15    1/22/99 9:11a Glen
 * add confidential statement
 * 
 * 14    12/14/98 5:46p Darrelld
 * SetMessageNoRef allows NULL data pointer for no change
 * 
 * 13    12/14/98 10:29a Darrelld
 * Add getBlockSize() call-through to allocator
 * 
 * 12    12/11/98 4:12p Darrelld
 * Add LtMsgAllocator:: getBlockSize()
 * 
 * 11    12/11/98 2:55p Darrelld
 * Add LtMsgRef:: getDataPtr() and getDataSize() to simplify use.
 * 
 * 10    12/03/98 10:19a Darrelld
 * LtRefQue::isEmpty uses getCount to avoid possible assert problem.
 * 
 * 9     12/03/98 10:14a Darrelld
 * Add getCount and isEmpty to LtRefQue
 * 
 * 8     11/24/98 4:28p Darrelld
 * Fix memory leak
 * 
 * 7     11/24/98 3:57p Glen
 * LtMsgRef needs a virtual destructor (to allow deletion via base class
 * ptr)
 * 
 * 6     11/24/98 2:13p Darrelld
 * remove dispose since it was a bad idea in the first place
 * 
 * 5     11/24/98 11:30a Darrelld
 * Change LPSTR to byte*
 * 
 * 4     11/24/98 11:14a Darrelld
 * add setMessageNoRef member
 * 
 * 3     11/20/98 4:44p Darrelld
 * Add getCount member
 * 
 * 2     11/06/98 9:42a Darrelld
 * Fix some problems and update the queue classes
 * 
 * 1     11/04/98 8:37a Darrelld
 * header files
 * 
 * 2     10/29/98 12:08p Darrelld
 * mods to project. Time server / client work.
 * 
 * 1     10/19/98 2:31p Darrelld
 * Vx Layer Sources
 * 
 * 1     10/16/98 1:43p Darrelld
 * Vx Layer Sources
 */

// Assume include of VxWorks.h 
// and public.h before we get here
// These queues must be portable to VxWorks, so use
// VxWorks services, not Win32 services here.
#include <VxlTypes.h>
#include <semLib.h>
#include <LtObject.h>

class LtMsgRef;

///////////////////////////////////////////////////////////////////////
// LtMsgAllocOwner
//
// This class is used as a superclass for owners of message allocators.
// If an owner registers itself with the allocator, then the owner
// wishes to be notified when a master is freed.
///////////////////////////////////////////////////////////////////////
class LtMsgAllocOwner
{
public:
	virtual boolean masterRelease(LtMsgRef* pMsg) = 0;
	virtual ~LtMsgAllocOwner() {}
};

//////////////////////////////////////////////////////////////////
//
// Message Reference Allocator class 
//
//
// Message Allocator pure base class
// Allows allocators to be built so that allocation is a queue removal
// rather than a general malloc.
//

class LtMsgRef;

class LtMsgAllocator
{
private:
	LtMsgAllocOwner* m_pOwner;
public:
	LtMsgAllocator( int nAlocClass )
	{	m_pOwner = null;
	}
	LtMsgAllocator()
	{	m_pOwner = null;
	}
	virtual ~LtMsgAllocator(){}

	virtual void init( int nBlkSize, int nBlks, int nNxtBlks, int nMaxBlks ) = 0;
	virtual void initMsgRefs( int nRefs, int nNxtRefs, int nMaxRefs ) = 0;
	virtual void freeAll() = 0;

	// Pure virtual for this added member would break lots of existing
	// code.
	virtual int		getBlockSize()
	{	return 0;
	}
	// Allocate a block from list, grow if necessary, and NULL if none
	virtual byte*		alloc() = 0;
	// Allocate a LtMsgRef object from a queue somewhere
	virtual LtMsgRef*	allocMsgRef() = 0;
	// Allocate a LtMsgRef object and a block and put them together as a master message
	virtual LtMsgRef*	allocMessage() = 0;
	// Free a block back to queue, delete if above some threshold, optionally.
	virtual void		free( byte* pBlk ) = 0;
	// Free a message reference object to queue, delete if above some threshold, optionally.
	virtual void		free( LtMsgRef* pMsgRef ) = 0;
	// Free a master message to queue, delete if above some threshold, optionally.
	// Performs a LtMsgRef.Release() if a client message
	// Performs a Free
	virtual void		freeMessage( LtMsgRef* pMsgRef ) = 0;
	virtual void		registerOwner( LtMsgAllocOwner* pOwner )
	{	m_pOwner = pOwner;
	}
	virtual boolean		returnMaster( LtMsgRef* pMsgRef )
	{	if (m_pOwner) return m_pOwner->masterRelease( pMsgRef );
		return true;
	}
protected:
#if 0	// commentary
	// possibly some real implementation with a LtQue
	int		m_nBlks;		// blocks we have now [ free and taken ]
	int		m_nMaxBlks;		// max blocks ever
	int		m_nNxtBlks;		// allocate this many blocks to grow
	int		m_nBlkSize;		// size of each block
	int		m_nRefs;		// Refs we have now [ free and taken ]
	int		m_nMaxRefs;		// max refs ever
	int		m_nNxtRefs;		// allocate this many refs to grow
	LtQue	m_qBlks;		// protected que of free blocks
	LtQue	m_qMsgRefs;		// protected que of message reference objects
	
#endif
};

//////////////////////////////////////////////////////////////////
//
// The Locked Queue class 
//
//
// General protected FIFO / LIFO queue class
// For use as base class for messages, etc.

class LTA_EXTERNAL_CLASS LtQue
{
public:					// opened up to allow LtPktReorderQue to work
	LtQue*	m_pNxt;
	LtQue*	m_pPrv;
	LtQue*	m_pHead;	// points to head if on a queue.
						// prevents multiple insertions
protected:
	BOOL	m_bHead;	// True for I'm the head
	BOOL	m_bLocked;	// True for I'm lockable
	SEM_ID	m_sem;		// This is a "binary semaphore"
	int		m_nCount;	// Item count

public:

	LtQue();									// assume item
	LtQue( BOOL bAsHead, BOOL bLocked=true );	// init as head, default to locked
	virtual ~LtQue();							// toss out the sem if I'm a head
	void	init( BOOL bAsHead, BOOL bLocked=true );
	void	destruct();

	// Queue head services
	// call on head of queue only
	void	initHead();
	BOOL	lockedIsEmpty();				// Check under lock
	BOOL	isEmpty();						// Assume locked
	BOOL	insertHead( LtQue* pItem );		// True if first item added
	BOOL	insertTail( LtQue* pItem );		// True if first item added
	BOOL	removeHead( LtQue** ppItem );	// True if item removed
	BOOL	removeTail( LtQue** ppItem );	// True if item removed
	int		getCount()						// return the count of items
	{	return m_nCount;
	};

	// Queue item services
	// call on an item of a queue only, not on head
	// Only needed for non-FIFO or non-LIFO queues
	void	initItem();		// default constructor assumes this
	void	remove();
	void	insert( LtQue* pPrev );
	boolean	isQueued()
	{	return m_pHead !=NULL; }
	
	// open lock and unlock to allow writing external diagnostics
//protected:
	// Queue head services that are protected
	// Must be head to lock
	void	lock();
	void	unlock();
};

//////////////////////////////////////////////////////////////////
//
// LtWaitQue - LtQue with wait for receive.
//

class LtWaitQue
{
public:

	LtWaitQue();
	virtual ~LtWaitQue();

	//	Send a message, timeout unnecessary, since storage always available
	STATUS	send( LtQue* pMsg );

	// Receive a message or timeout in the standard way
	STATUS	receive( LtQue** ppMsg, int timeout );

	// Primitives on our contained queue
	int		getCount()
	{	return m_qMsgs.getCount();
	};

	BOOL	isEmpty()
	{	return 0 == m_qMsgs.getCount();
	};


protected:
	// We need an event to signal a waitor for an event.
	// If there is one waitor at a time, then we can use
	// a binary semaphore. So assume this and include a
	// check so a second waitor [ doing a receive ] gets an error
	// if queue is being waited by more than one task.
	SEM_ID		m_semReceiveWait;
	int			m_tidWaitor;

	// The messages are queued here
	LtQue		m_qMsgs;		// queue of messages
	// Of course, access to the queue itself is locked.
};

//////////////////////////////////////////////////////////////////
//
// The Message Reference class
//
// This class holds the reference to the message data itself.
// There are two allocators. Both are only used for the master
// One is for the data, and the other is for the master LtRefQue object.
// If NULL then 'free()'  or 'delete' is used.
//

class LtRefQue;

class LtMsgRef : public LtQue, public LtObject
{
public:
	LtMsgRef( BOOL bMaster=FALSE);
	virtual ~LtMsgRef();

	// called by allocators that queue items
	void	init(BOOL bMaster = FALSE );
	void	destruct();

	// Call on non-master only
	void			setMaster( LtMsgRef* pMas );

	// Call on a master only.
	void			makeMaster( BOOL bFree );
	void			setBlock( byte* pBlk )
	{	m_pBlk = pBlk;
	}
	byte*			getBlock()
	{	return m_pBlk;
	}
	int				getBlockSize()
	{
		if ( m_pAllocator )
		{	return m_pAllocator->getBlockSize();
		}
		else
		{	return 0;
		}
	}
	// allocate a message for this master MsgRef and bind it
	byte*			alloc();
	LtMsgRef*		lockMaster();		// returns master
	LtMsgRef*		unlockMaster();		// returns master

	// Call on either
	LtMsgRef*		getMaster();
	// pMas == NULL declares a master message with MakeMaster
	void			setMessageData( byte* pBuf, int nSize, LtMsgRef* pMas );
	void			setMessageNoRef( byte* pBuf, int nSize )
	{	if ( pBuf ) m_pBuf = pBuf;
		m_nSize = nSize;
	};
	// copy the data portion of the message
	// leave the refs alone in our ref
	void			copyMessage( LtMsgRef* pMsg );
	int				getMessageData( byte** ppBuf );
	byte*			getDataPtr()
	{	return	m_pBuf; }
	int				getDataSize()
	{	return	m_nSize; }

	LtMsgRef*		cloneMessage();

	LtRefQue*		getQue()
	{	return m_pRefQue;
	}
	void			setQue( LtRefQue* pQue )
	{	m_pRefQue = pQue;
	}
	void			addRef();		// add a reference to master
	void			release();		// release a reference on master
									// and release this object
	void			setAllocator( LtMsgAllocator* pAlloc );
	LtMsgAllocator*	getAllocator();

protected:

	void	dispose();				// helper to cleanup
									// only called by release, not clients.

	LtMsgRef*		m_pMaster;		// master message or NULL
	byte*			m_pBuf;			// address of data
	int				m_nSize;		// size of data
	LtRefQue*		m_pRefQue;		// queue that we last used
	// We have to solve the problem of multiple threads wanting to
	// release the master at the same time.
	byte*			m_pBlk;			// data allocated to master
	int				m_nRef;			// ref count on this master
	SEM_ID			m_semMasterLock;// master lock
	BOOL			m_bFreeMaster;	// free master on last deRef
	LtMsgAllocator*	m_pAllocator;	// the allocator used
};


//////////////////////////////////////////////////////////////////
//
// Reference Queue
//
// This class is the message queue that sends and receives message
// refs
//

class LtRefQue : public LtWaitQue
{
public:
	//	Send a message, timeout unnecessary, since storage always available
	STATUS	send( LtMsgRef* pMsg )
	{
		return LtWaitQue::send( (LtQue*) pMsg );
	}

	// Receive a message or timeout in the standard way
	STATUS	receive( LtMsgRef** ppMsg, int timeout )
	{
		LtQue* pQue;
		STATUS sts = LtWaitQue::receive( &pQue, timeout );
		*ppMsg = (LtMsgRef*) pQue;
		return sts;
	}

	// Remove and free all messages from the queue
	STATUS	flush();
};


#endif // _REFQUES_H
