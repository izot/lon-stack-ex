#ifndef LTPKTINFOX_H
#define LTPKTINFOX_H
//
// LtPktInfoX.h
//
// Copyright © 2022 Dialog Semiconductor
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in 
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//


//////////////////////////////////////////////////////////////////
//
// The Locked Queue class 
//
//
// General protected FIFO / LIFO queue class
// For use as base class for messages, etc.

class LtQue
{
public: 				// opened up to allow LtPktReorderQue to work
	LtQue*	m_pNxt;
	LtQue*	m_pPrv;
	LtQue*	m_pHead;	// points to head if on a queue.
						// prevents multiple insertions
protected:
	BOOL	m_bHead;	// True for I'm the head
	BOOL	m_bLocked;	// True for I'm lockable
//	SEM_ID	m_sem;		// This is a "binary semaphore"
	int 	m_nCount;	// Item count

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
	BOOL	insertHead( LtQue* pItem ); 	// True if first item added
	BOOL	insertTail( LtQue* pItem ); 	// True if first item added
	BOOL	removeHead( LtQue** ppItem );	// True if item removed
	BOOL	removeTail( LtQue** ppItem );	// True if item removed
	int 	getCount()						// return the count of items
	{	return m_nCount;
	};

	// Queue item services
	// call on an item of a queue only, not on head
	// Only needed for non-FIFO or non-LIFO queues
	void	initItem(); 	// default constructor assumes this
	void	remove();
	void	insert( LtQue* pPrev );
	boolean isQueued()
	{	return m_pHead !=NULL; }
	
protected:
	// Queue head services that are protected
	// Must be head to lock
	void	lock();
	void	unlock();
};

class LtPktInfo : public LtQue
{
private:

	byte*			m_pData;		// Pointer to current parse/build position
	int 			m_nLen; 		// Length of data to be parsed or length of data built.
	int 			m_nBlkSize;
	byte*			m_pBlock;
	boolean			m_bIgnoreAuthentication;


protected:

public:
							LtPktInfo()
	{
		m_pBlock = NULL;
		m_pData = NULL;
		m_nLen = 0;
		m_nBlkSize = 0;
		m_bIgnoreAuthentication = false;
	}
	virtual 			   ~LtPktInfo() 
	{
		if ( m_pBlock )
		{
			::free( m_pBlock );
			m_pBlock = NULL;
		}
	}
	int getLength() { return m_nLen; }
	byte*		getDataPtr()						{ return m_pData; }
	int 		getDataSize()						{ return m_nLen; }
	int 		getBlockSize()						{ return m_nBlkSize; }
	void		setBlock( byte* pBlk )				{ m_pBlock = pBlk; }
	byte*		getBlock()							{ return m_pBlock; }
	void		release()							{ delete this; };
	void		setMessageData( byte* pData, int nLen, LtPktInfo* pMaster )
	{	m_pData = pData;
		m_nLen = nLen;
		//assert( pMaster == this || pMaster == NULL );
	}
	void				setIgnoreAuthentication( boolean bIgnore=true)
	{	m_bIgnoreAuthentication = bIgnore;
	}
	boolean			ignoreAuthentication()
	{	return m_bIgnoreAuthentication;
	}


};
#endif	// LTPKTINFOX_H
