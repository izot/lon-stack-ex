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

#include "LtRouter.h"

#include <LtPktAllocatorOne.h>
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// LtPktAllocator
//
// "Dumb as a post" packet allocator
// uses malloc / free and new / delete directly.
//

//
// constructor
//
LtPktAllocatorOne::LtPktAllocatorOne( )
{
	m_nBlkSize = MAX_PATH*2;	// assume a long default
	m_nBlocks = 0;
	m_sem = semBCreate(0, SEM_FULL);
}

//
// destructor
//
LtPktAllocatorOne::~LtPktAllocatorOne()
{
	semDelete(m_sem);
}

int LtPktAllocatorOne::getBlockSize()
{
	return m_nBlkSize;
}

void LtPktAllocatorOne::init( int nBlkSize, int nBlks, int nNxtBlks, int nMaxBlks )
{

	m_nBlkSize = nBlkSize;
}


void LtPktAllocatorOne::initMsgRefs( int nRefs, int nNxtRefs, int nMaxRefs )
{


}


void LtPktAllocatorOne::freeAll()
{


}



// Allocate a block from list, grow if necessary, and NULL if none
byte*	LtPktAllocatorOne::alloc()
{
	if (m_nBlocks < 20)
	{
		semTake(m_sem, WAIT_FOREVER);
		m_nBlocks++;
		semGive(m_sem);
		return	(byte*)malloc(m_nBlkSize);
	}
	return null;
}


// Allocate a CMsgRef object from a queue somewhere
LtMsgRef*	LtPktAllocatorOne::allocMsgRef()
{
	LtMsgRef*	pRef = new LtPktInfo();
	pRef->setAllocator( this );
	return pRef;
}


// Allocate a CMsgRef object and a block and put them together as a master message
LtMsgRef*	LtPktAllocatorOne::allocMessage()
{
	LtMsgRef*	pRef = allocMsgRef();
	if (pRef != null)
	{
		byte*		pBuf = alloc();
		if (pBuf == null)
		{
			free(pRef);
			return null;
		}
		pRef->makeMaster( true );
		pRef->setBlock( pBuf );
	}
	return pRef;
}


// Free a block back to queue, delete if above some threshold, optionally.
void	LtPktAllocatorOne::free( byte* pBlk )
{
	semTake(m_sem, WAIT_FOREVER);
	m_nBlocks--;
	semGive(m_sem);
	::free( pBlk );
}


// Free a message reference object to queue, delete if above some threshold, optionally.
void	LtPktAllocatorOne::free( LtMsgRef* pMsgRef )
{
	delete pMsgRef;
}


// Free a master message to queue, delete if above some threshold, optionally.
// Performs a CMsgRef.Release() if a client message
// Performs a Free
void	LtPktAllocatorOne::freeMessage( LtMsgRef* pMsgRef )
{
	pMsgRef->release();
	free( pMsgRef );
}



// end

