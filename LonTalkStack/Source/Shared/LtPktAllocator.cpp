//
// LtPktAllocator.cpp
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
#include <assert.h>
#include "LtRouter.h"
#include <vxlTarget.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// LtPktAllocator 
//
// Intelligent packet allocator
// uses look aside lists for both data blocks and for packet objects.
//
//#define ENABLE_CRUMBS 1

// add or delete the lock and unlock code
#ifdef LTPKTALLOC_ONELOCK
#define LTPKTLOCK lock();
#define LTPKTUNLOCK unlock();
#else // LTPKTALLOC_ONELOCK
#define LTPKTLOCK
#define LTPKTUNLOCK
#endif // LTPKTALLOC_ONELOCK

#ifdef ENABLE_CRUMBS // debug

//
// insert a packet in the allocator vector to indicate
// it has been allocated.
//
void PktDebugInsert( LtPktInfoVec& vec, LtMsgRef* pMsg )
{
	boolean	bFound;
	LtPktInfo*	pPkt = (LtPktInfo*)pMsg;
	if ( pPkt->m_pHead )
	{
		vxlReportEvent("PktDebugInsert - Head pointer not NULL 0x%08x\n", pPkt );
	}
	bFound = vec.isElement( pPkt );
	if ( bFound )
	{
		vxlReportEvent("PktDebugInsert - double allocation 0x%08x\n", pPkt );
	}
	else
	{
		pPkt->setCrumb( "PktDebugInsert" );
		vec.addElement( pPkt );
	}
}

//
// remove a packet from the allocator vector to indicate
// that it has been freed.
//
void PktDebugRemove( LtPktInfoVec& vec, LtMsgRef* pMsg )
{
	boolean	bFound;
	LtPktInfo*	pPkt = (LtPktInfo*)pMsg;
	if ( pPkt->m_pHead )
	{
		vxlReportEvent("PktDebugRemove - Head pointer not NULL 0x%08x\n", pPkt );
	}
	bFound = vec.isElement( pPkt );
	if ( !bFound )
	{
		vxlReportEvent("PktDebugRemove - double deallocation 0x%08x\n", pPkt );
	}
	else
	{
		vec.removeElement( pPkt );
	}
}

#define INSERT_DEBUG( pMsg )					\
	if ( m_bDebug )								\
	{	lockDebugVector();						\
		PktDebugInsert( m_vAllocMsgs, pMsg );	\
		unlockDebugVector();					\
	}

#define REMOVE_DEBUG( pMsg )					\
	if ( m_bDebug )								\
	{	lockDebugVector();						\
		PktDebugRemove( m_vAllocMsgs, pMsg );	\
		unlockDebugVector();					\
	}

//
// insert a block in the allocator vector to indicate
// it has been allocated.
//
void BlkDebugInsert( LtBlkVec& vec, LtBlk *pLtBlk)
{
	boolean	bFound;

	if ( pLtBlk->m_pHead )
	{
		vxlReportEvent("BlkDebugInsert - Head pointer not NULL 0x%08x\n", pLtBlk );
	}
	bFound = vec.isElement( pLtBlk );
	if ( bFound )
	{
		vxlReportEvent("BlkDebugInsert - double allocation 0x%08x\n", pLtBlk );
	}
	else
	{
		vec.addElement( pLtBlk );
	}
}

//
// remove a packet from the allocator vector to indicate
// that it has been freed.
//
void BlkDebugRemove( LtBlkVec& vec, LtBlk *pLtBlk )
{
	boolean	bFound;
	if ( pLtBlk->m_pHead )
	{
		vxlReportEvent("BlkDebugRemove - Head pointer not NULL 0x%08x\n", pLtBlk );
	}
	bFound = vec.isElement( pLtBlk );
	if ( !bFound )
	{
		vxlReportEvent("BlkDebugRemove - double deallocation 0x%08x\n", pLtBlk );
	}
	else
	{
		vec.removeElement( pLtBlk );
	}
}

#define INSERT_BLK_DEBUG( pLtBlk )				\
	if ( m_bDebug )								\
	{  	lockDebugVector();						\
		BlkDebugInsert( m_vAllocBlks, pLtBlk ); \
		unlockDebugVector();					\
	}

#define REMOVE_BLK_DEBUG( pLtBlk )				\
	if ( m_bDebug )								\
	{  	lockDebugVector();						\
		BlkDebugRemove( m_vAllocBlks, pLtBlk ); \
		unlockDebugVector();					\
	}


#ifdef WIN32
#define XXXPrintf vxlPrintf
#else // WIN32
#define XXXPrintf printf
#endif

// list of all allocators so we can dump their crumbs
//typedef LtTypedVVector<LtPktAllocator> LtPktAllocVec;
typedef LtTypedVector<LtPktAllocator> LtPktAllocVec;

LtPktAllocVec	gv_PktAllocs;

LtPktAllocator*		ga_PktAllocs[10] = {0,0,0,0,0,0,0,0,0,0};

void	addPktAlloc( LtPktAllocator* pAlloc )
{
	int		i;
	for ( i=0; i<10; i++ )
	{
		if ( ga_PktAllocs[i] == NULL )
		{	ga_PktAllocs[i] = pAlloc;
			break;
		}
	}
}

void removePktAlloc( LtPktAllocator* pAlloc )
{
	int		i;
	for ( i=0; i<10; i++ )
	{
		if ( ga_PktAllocs[i] == pAlloc )
		{	ga_PktAllocs[i] = NULL;
			break;
		}
	}
}

// dump the crumbs for all allocators
void	dumpPktCrumbs()
{
	int		i;
	for ( i=0; i<10; i++ )
	{
		if ( ga_PktAllocs[i] )
		{
			ga_PktAllocs[i]->dumpCrumbs();
		}
	}
}

#else
#define INSERT_DEBUG( pMSG )
#define REMOVE_DEBUG( pMsg )
#define INSERT_BLK_DEBUG( pLtBlk )
#define REMOVE_BLK_DEBUG( pLtBlk ) 
#endif // debug

// LtBlk 
// 

LtBlk::LtBlk(int size)
{
	m_pBlk = (BlockHeader *)malloc(sizeof(BlockHeader) + size);
	if (m_pBlk != NULL)
	{
		m_pBlk->pLtBlk = this;
	}
}

LtBlk::~LtBlk()
{
	if (m_pBlk != NULL)
	{
		free(m_pBlk);
	}
}

byte *LtBlk::getBlockAddr(void)
{
	byte* pBlk = NULL;
	if (m_pBlk != NULL)
	{	// Step beyond the header
		pBlk = (byte*)(m_pBlk + 1);
	}
	return(pBlk);
}

LtBlk* LtBlk::getLtBlk(byte* blk)
{
	LtBlk* pLtBlk = NULL;
	if (blk != NULL)
	{	// Back up by the header size, and then get the ltBlk
		pLtBlk = (((BlockHeader *)blk)-1)->pLtBlk;
	}
	return(pLtBlk);
}

void LtBlk::deleteBlock(byte* blk)
{
	delete getLtBlk(blk);
}

//
// constructor
//
LtPktAllocator::LtPktAllocator( ) :
#ifdef LTPKTALLOC_ONELOCK
	m_qBlks( true, false ),		// queue of blocks is a head, but not locked
	m_qRefs( true, false )		// queue of refs is a head, but not locked
#else // LTPKTALLOC_ONELOCK
	m_qBlks( true, true ),		// queue of blocks is a head, and locked
	m_qRefs( true, true )		// queue of refs is a head, and locked
#endif // LTPKTALLOC_ONELOCK
{
	m_nBlkSize			= 600;	// assume a long default
	// Makes sense for the limits for both to be the same
	m_nBlks				= 100;
	m_nNxtBlks			= 10;
	m_nMaxBlks			= 200;
	m_nRefs				= 100;
	m_nNxtRefs			= 10;
	m_nMaxRefs			= 200;
	m_nBlksNow			= 0;
	m_nRefsNow			= 0;
	m_bAdjustDown		= false;		// we need to adjust downward on next free
#ifdef LTPKTALLOC_ONELOCK
	m_semLock			= semMCreate( SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
#endif // LTPKTALLOC_ONELOCK
	m_bFreeMasters		= true;
	m_bDebug			= false;

#ifdef ENABLE_CRUMBS
	m_vectorSemLock		= semMCreate( SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
	m_bDebug			= true;
	//printf("add %x to allocs\n", this);
	//gv_PktAllocs.addElement( this );
	addPktAlloc( this );
#endif // ENABLE_CRUMBS
}

//
// destructor
//
LtPktAllocator::~LtPktAllocator()
{
	freeAll();
	#ifdef ENABLE_CRUMBS
	// remove us from the allocator list
	//gv_PktAllocs.removeElement( this );
	removePktAlloc(this);

	if ( m_nRefsNow || m_nBlksNow )
	{	dumpCrumbs( );
		if ( m_nBlksNow != m_nRefsNow )
		{	vxlReportEvent("LtPktAllocator - blocks remaining in destructor %d\n",
					m_nBlksNow );
		}
	}
	#else // ENABLE_CRUMBS

	// Better not be any objects outstanding now.
// FUTURE: These asserts can happen under traffic so just remove for now.  Need to understand why they occur.
	//assert( m_nRefsNow == 0 );
	//assert( m_nBlksNow == 0 );
	#endif // ENABLE_CRUMBS
#ifdef LTPKTALLOC_ONELOCK
	if ( m_semLock )
	{	semDelete( m_semLock );
		m_semLock = NULL;
	}
#endif // LTPKTALLOC_ONELOCK
#ifdef ENABLE_CRUMBS
	if (m_vectorSemLock)
	{
		semDelete(m_vectorSemLock);
		m_vectorSemLock = NULL;
	}
#endif
}

void LtPktAllocator::init( int nBlkSize, int nBlks, int nNxtBlks, int nMaxBlks )
{
    if (nBlks > nMaxBlks)
    {
        nBlks = nMaxBlks;
        nNxtBlks = 0;
    }
    else if (nBlks + nNxtBlks > nMaxBlks)
    {
        nNxtBlks = nMaxBlks - nBlks;
    }

	m_nBlkSize		= nBlkSize;
	m_nBlks			= nBlks;
	m_nNxtBlks		= nNxtBlks;
	m_nMaxBlks		= nMaxBlks;

	assert( nMaxBlks >= nBlks );
	assert( (nBlks+nNxtBlks) <= nMaxBlks );
	loadLists();
}

void LtPktAllocator::initMsgRefs( int nRefs, int nNxtRefs, int nMaxRefs )
{
    if (nRefs > nMaxRefs)
    {
        nRefs = nMaxRefs;
        nNxtRefs = 0;
    }
    else if (nRefs + nNxtRefs > nMaxRefs)
    {
        nNxtRefs = nMaxRefs - nRefs;
    }

    m_nRefs			= nRefs;
	m_nNxtRefs		= nNxtRefs;
	m_nMaxRefs		= nMaxRefs;

	assert( nMaxRefs >= nRefs );
	assert( (nRefs+nNxtRefs) <= nMaxRefs );
	loadLists();
}
#ifdef LTPKTALLOC_ONELOCK
 //
//
//	lock
//
void LtPktAllocator::lock()
{
	assert( m_semLock );
	__UNUSED__ STATUS sts;
	sts = semTake( m_semLock, WAIT_FOREVER );
	assert( sts == OK );
}

//
//	unlock
//
void LtPktAllocator::unlock()
{
	assert( m_semLock );
	__UNUSED__ STATUS sts;
	sts = semGive( m_semLock );
	assert( sts == OK );
}
#endif // LTPKTALLOC_ONELOCK

//
// loadLists
//
// Adjust the lists up or down to the new limits
//
void LtPktAllocator::loadLists()
{
	LTPKTLOCK
	int			nRefs = m_nRefsNow;
	int			nBlks = m_nBlksNow;
	//LtPktInfo*	pPkt;
	LtQue*		pItem;

	m_bAdjustDown = false;

	while ( nRefs < m_nRefs )
	{
		pItem = new LtPktInfo();
		if ( pItem )
		{
			m_qRefs.insertTail( pItem );
			nRefs++;
		}
		else
		{	break;
		}
	}

	while ( nRefs > m_nMaxRefs )
	{
		if ( m_qRefs.removeHead( &pItem ) )
		{
			delete pItem;
			nRefs--;
		}
		else
		{	m_bAdjustDown = true;
			break;
		}
	}
	m_nRefsNow = nRefs;

	while ( nBlks < m_nBlks )
	{
		pItem = new LtBlk(m_nBlkSize);
		if (pItem != NULL)
		{
			pItem->init(false);
			m_qBlks.insertTail( pItem );
			nBlks++;
		}
		else
		{
			break;
		}
	}

	while ( nBlks > m_nMaxBlks )
	{
		if ( m_qBlks.removeHead( &pItem ) )
		{
			delete pItem;
			nBlks--;
		}
		else
		{	// remember that we need to adjust downward on
			// the next free.
			m_bAdjustDown = true;
			break;
		}
	}
	m_nBlksNow = nBlks;
	LTPKTUNLOCK
}

//
// allocMore
//
// Adjust the lists up by the increment if needed
//
void LtPktAllocator::allocMore()
{
	int			i;
	LtQue*		pItem;
	LTPKTLOCK
	if ( (m_qRefs.getCount() == 0) && (m_nRefsNow < m_nMaxRefs) )
	{
		int		nRefs = m_nRefsNow;
		for ( i=0; i< m_nNxtRefs && nRefs < m_nMaxRefs; i++ )
		{
			pItem = new LtPktInfo();
			if ( pItem )
			{
				m_qRefs.insertTail( pItem );
				nRefs++;
			}
			else
			{	break;
			}
		}
		m_nRefsNow = nRefs;
	}

	if ( (m_qBlks.getCount() == 0) && (m_nBlksNow < m_nMaxBlks) )
	{
		int		nBlks = m_nBlksNow;
		for ( i=0; i< m_nNxtBlks && nBlks < m_nMaxBlks; i++ )
		{
			pItem = new LtBlk(m_nBlkSize);
			if (pItem != NULL)
			{
				pItem->init(false);
				m_qBlks.insertTail( pItem );
				nBlks++;
			}
			else
			{	break;
			}
		}
		m_nBlksNow = nBlks;
	}
	LTPKTUNLOCK
}


//
// freeAll
//
// Free all items we have, but do not assume that this is
// all the items there are.
// So we just delete the items we have and then return.
//
void LtPktAllocator::freeAll()
{
	LtQue*	pItem;
	LTPKTLOCK
	while ( m_qRefs.removeHead( &pItem ) )
	{
		delete pItem;	// virtual destructors take care of polymorphism
		m_nRefsNow --;	// count down the objects we have
	}

	while ( m_qBlks.removeHead( & pItem ) )
	{
		delete pItem;
		m_nBlksNow -- ;
	}
	LTPKTUNLOCK
}

//
// allItemsReturned
//
// all items are returned, so we can delete the allocator
//
boolean LtPktAllocator::allItemsReturned()
{
	LTPKTLOCK
	boolean		bOk;
	bOk = (m_qRefs.getCount() == m_nRefsNow) && (m_qBlks.getCount() == m_nBlksNow );
	LTPKTUNLOCK
	return bOk;
}

// Allocate a block from list, grow if necessary, and NULL if none
byte*	LtPktAllocator::alloc()
{
	byte*	pBlk = NULL;
	LtQue*	pItem = NULL;
	LTPKTLOCK
	if ( 0 == m_qBlks.getCount() )
	{	allocMore();
	}
	m_qBlks.removeHead( &pItem );
	if ( pItem )
	{	LtBlk *pLtBlk = static_cast<LtBlk *>(pItem);
		pBlk = pLtBlk->getBlockAddr();
		INSERT_BLK_DEBUG(pLtBlk);
		// Pentagon code: obsolete
		// pBlk += LONC_OVERHEAD;		// Reposition block to allow for driver's need
	}
	// remains zero if none removed
	LTPKTUNLOCK
	return	pBlk;
}


// Allocate a CMsgRef object from a queue somewhere
LtMsgRef*	LtPktAllocator::allocMsgRef()
{
	LtMsgRef*	pRef = NULL;
	LtQue*		pItem = NULL;
	LTPKTLOCK
	if ( 0 == m_qRefs.getCount() )
	{	allocMore();
	}
	m_qRefs.removeHead( &pItem );
	if ( pItem )
	{
		pRef = (LtMsgRef*)pItem;
		pRef->setAllocator( this );
		INSERT_DEBUG( pRef )
	}
	LTPKTUNLOCK
	return pRef;
}


// Allocate a CMsgRef object and a block and put them together as a master message
// If we can't get both, return neither.
LtMsgRef*	LtPktAllocator::allocMessage()
{
	LTPKTLOCK
	LtMsgRef*	pRef = allocMsgRef();
	if ( pRef )
	{
		byte*		pBuf = alloc();
		if ( pBuf )
		{
			pRef->makeMaster( m_bFreeMasters );
			pRef->setBlock( pBuf );
		}
		else
		{
			free( pRef );
			pRef = NULL;
		}
	}
	LTPKTUNLOCK
	return pRef;
}


// Free a block back to queue, delete if above some threshold, optionally.
void	LtPktAllocator::free( byte* pBlk )
{
	LTPKTLOCK
	// Pentagon code: obsolete
	// pBlk -= LONC_OVERHEAD;		// See LONC_OVERHEAD above
	LtBlk *pLtBlk = LtBlk::getLtBlk(pBlk);
	LtQue*	pItem = pLtBlk;
	if (pItem != NULL)
	{
		REMOVE_BLK_DEBUG(pLtBlk);
		pItem->init(false);
		m_qBlks.insertTail( pItem );
	}
	LTPKTUNLOCK
}


// Free a message reference object to queue, delete if above some threshold, optionally.
void	LtPktAllocator::free( LtMsgRef* pMsgRef )
{
	LTPKTLOCK
	REMOVE_DEBUG( pMsgRef );
	pMsgRef->init(false);
	m_qRefs.insertTail( pMsgRef );
	if ( m_bAdjustDown )
	{	loadLists();
	}
	LTPKTUNLOCK
}


// Free a master message to queue, delete if above some threshold, optionally.
// Performs a CMsgRef.Release() if a client message
// Performs a Free
void	LtPktAllocator::freeMessage( LtMsgRef* pMsgRef )
{
	LTPKTLOCK
	pMsgRef->release();
	free( pMsgRef );
	if ( m_bAdjustDown )
	{	loadLists();
	}
	LTPKTUNLOCK
}

int LtPktAllocator::getBlockSize()
{
	// Pentagon code: obsolete
	// return m_nBlkSize - LONC_OVERHEAD;
	return m_nBlkSize;
}

#ifdef ENABLE_CRUMBS
void	LtPktAllocator::dumpCrumbs( )
{
	LTPKTLOCK
	XXXPrintf("LtPktAllocator::dumbCrumbs - allocator 0x%08x pkts %d max %d now %d\n",
					(UINT)this, m_vAllocMsgs.getCount(), m_nMaxRefs, m_nRefsNow );
	LtPktInfo*		pPkt;
	LtVectorPos		pos;
	char*			cmb;
	
	while ( m_vAllocMsgs.getElement( pos, &pPkt ) )
	{
		cmb = pPkt->getCrumb();
		if ( cmb == NULL )	cmb = "none";
		XXXPrintf("                      0x%08x %ld %s\n",
									(UINT)pPkt, pPkt->getTimestamp(), cmb );
	}
	LTPKTUNLOCK
}

//
// lockDebugVector
//
// Lock the debug vectors used when ENABLE_CRUMBS is set.
// 
void			LtPktAllocator::lockDebugVector()
{
	assert( m_vectorSemLock );
	STATUS sts;
	sts = semTake( m_vectorSemLock, WAIT_FOREVER );
	assert( sts == OK );
}

//
// unlockDebugVector
//
// Unlock the debug vectors used when ENABLE_CRUMBS is set.
// 
void			LtPktAllocator::unlockDebugVector()
{
	assert( m_vectorSemLock );
	STATUS sts;
	sts = semGive( m_vectorSemLock );
	assert( sts == OK );
}


#else
void				LtPktAllocator::dumpCrumbs()
{
}
void			LtPktAllocator::lockDebugVector()
{
}
void			LtPktAllocator::unlockDebugVector()
{
}

#endif

void LtPktAllocator::showAllocator()
{
	vxlPrintf("allocator 0x%08x pkt refs %d (%d), max %d, now %d\n"
			  "                     pkt blks %d (%d), max %d, now %d\n",
					(UINT)this, m_vAllocMsgs.getCount(), m_nRefs, m_nMaxRefs, m_nRefsNow,
					m_vAllocBlks.getCount(), m_nBlks, m_nMaxBlks, m_nBlksNow);
}

// end
