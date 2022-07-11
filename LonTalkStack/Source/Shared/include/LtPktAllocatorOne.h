#ifndef LTPKTALLOCATORONE_H
#define LTPKTALLOCATORONE_H

//
// LtPktAllocatorOne.h
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
/* $Log: /Shared/include/LtPktAllocatorOne.h $
 * 
 * 1     2/11/99 9:13a Darrelld
 * Old dumb packet allocator for performance testing
 */

#define MAX_MSGSIZE 512

//////////////////////////////////////////////////////////////////
//
// Message Reference Allocator class 
//
//
// Message Allocator implementation
// This allocator is an old version using malloc / free used for
// performance testing.
//

class LtPktAllocatorOne : public LtMsgAllocator
{
public:
	LtPktAllocatorOne( );
	virtual ~LtPktAllocatorOne();
	virtual void init( int nBlkSize, int nBlks, int nNxtBlks, int nMaxBlks );
	virtual void initMsgRefs( int nRefs, int nNxtRefs, int nMaxRefs );
	virtual void freeAll();

	// Allocate a block from list, grow if necessary, and NULL if none
	byte*		alloc();
	// Allocate a LtMsgRef object from a queue somewhere
	LtMsgRef*	allocMsgRef();
	// Allocate a LtMsgRef object and a block and put them together as a master message
	LtMsgRef*	allocMessage();
	
	virtual inline LtPktInfo*  allocPacket() 
	{ 
		LtPktInfo* pPkt = (LtPktInfo*) allocMessage();
		if (pPkt != null)
		{
			pPkt->initPacket();
		}
		return pPkt;
	}

	// Free a block back to queue, delete if above some threshold, optionally.
	virtual void		free( byte* pBlk );
	// Free a message reference object to queue, delete if above some threshold, optionally.
	virtual void		free( LtMsgRef* pMsgRef );
	// Free a master message to queue, delete if above some threshold, optionally.
	// Performs a LtMsgRef.Release() if a client message
	// Performs a Free
	virtual void		freeMessage( LtMsgRef* pMsgRef );
	virtual int			getBlockSize();

protected:
	int m_nBlkSize;
	int m_nBlocks;
	SEM_ID m_sem;
};

#endif // LTPKTALLOCATORONE_H
