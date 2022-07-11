#ifndef LTPKTALLOCATOR_H
#define LTPKTALLOCATOR_H

//
// LtPktAllocator.h
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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// LtPktAllocator
//
// Intelligent packet allocator
// uses look aside lists for both data blocks and for packet objects.
//

// define the following symbol to enable one lock rather than individual
// queue locks in the packet allocator.
// Benchmark testing indicates that this is not a win by about 10%

#define LTPKTALLOC_ONELOCK

//typedef LtTypedVVector<LtPktInfo> LtPktInfoVec;
typedef LtTypedVector<LtPktInfo> LtPktInfoVec;

// Blocks have a hidden header, which points to a block handle
typedef struct 
{
	class LtBlk *pLtBlk;	
} BlockHeader;

class LtBlk : public LtQue, public LtObject
{
public:
	LtBlk(int size);
	virtual ~LtBlk();
	byte *getBlockAddr(void);
	static LtBlk *getLtBlk(byte *blk);
	static void deleteBlock(byte *blk);

private:
	BlockHeader *m_pBlk;
};


typedef LtTypedVector<LtBlk> LtBlkVec;

class LtPktAllocator : public LtMsgAllocator
{
public:
	LtPktAllocator( );
	virtual ~LtPktAllocator();
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
	void				setMasterFree( boolean bFree ) { m_bFreeMasters = bFree; }

	void				enableDebug( boolean bEnable )	{ m_bDebug = bEnable; }
	void				dumpCrumbs();
	boolean				allItemsReturned();
	void				showAllocator();
protected:
	//int m_nBlkSize;
	int		m_nBlks;		// initial allocation
	int		m_nNxtBlks;		// allocation increment
	int		m_nMaxBlks;		// max allocation
	int		m_nRefs;		// initial allocation
	int		m_nNxtRefs;		// allocation increment
	int		m_nMaxRefs;		// max allocation

	int		m_nBlksNow;		// blocks we have now
	int		m_nRefsNow;		// refs we have now
	boolean	m_bAdjustDown;	// we need to adjust downward later as items come in

	LtQue	m_qBlks;
	LtQue	m_qRefs;		// packets actually
#ifdef LTPKTALLOC_ONELOCK
	SEM_ID	m_semLock;		// lock mutex

	virtual void	lock();
	virtual void	unlock();
#endif // LTPKTALLOC_ONELOCK
	virtual void	loadLists();
	virtual void	allocMore();
	virtual int		getBlockSize();
protected:
	int				m_nBlkSize;
	boolean			m_bFreeMasters;
	boolean			m_bDebug;				// debug enable
	LtPktInfoVec	m_vAllocMsgs;			// vector of allocated message refs for debugging
	LtBlkVec		m_vAllocBlks;			// vector of allocated blocks for debugging
	SEM_ID			m_vectorSemLock;		// Lock for debug vectors
	void			lockDebugVector();
	void			unlockDebugVector();

};


#endif
