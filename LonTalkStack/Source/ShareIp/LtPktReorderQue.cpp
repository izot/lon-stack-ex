/***************************************************************
 *  Filename: LtPktReorderQue.cpp
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
 *  Description:  Packet reordering class.
 *
 *	DJ Duffy July 1999
 *
 ****************************************************************/ 

/*
 * $Log: /Dev/ShareIp/LtPktReorderQue.cpp $
 * 
 * 6     12/08/99 3:03p Darrelld
 * Add some crumbs
 * 
 * 5     7/30/99 12:43p Darrelld
 * 
 * 4     7/28/99 4:48p Darrelld
 * Fix packet reordering
 * 
 * 3     7/27/99 1:52p Darrelld
 * Fix packet reordering
 * 
 * 2     7/26/99 5:18p Darrelld
 * Debugging of IP router features
 * 
 * 1     7/21/99 3:13p Darrelld
 * 
*/

#include <LtRouter.h>
#include <LtIpPackets.h>
#include <LtPktReorderQue.h>

//
// discard
//
// Discard all packets from the waiting queue
//
void LtPktReorderQue::discard()
{
	LtQue*			pItem;
	LtPktInfo*		pPkt;
	lock();
	while ( !isEmpty() )
	{	removeHead( &pItem );
		m_nDiscardedPackets++;
		pPkt = (LtPktInfo*)pItem;
		pPkt->release();
	}
	unlock();
}

//
// insert
//
// insert a packet into the waiting queue in order of sequence number
//
void LtPktReorderQue::insert( LtPktInfo* pPkt, LtIpPktHeader& phd )
{
	lock();
	LtQue*		pNew = (LtQue*)pPkt;
	ULONG		seq = phd.sequence;

	pPkt->setTimestamp( phd.timestamp );
	pPkt->setSequence( seq );

	LtQue*		pItem = m_pNxt != this? m_pNxt: NULL;
	LtPktInfo*	pNext = pItem? (LtPktInfo*)pItem: NULL;
	LtQue*		pPrev = this;

	while ( pItem && ( ( seq - pNext->getSequence() ) > 0 ) )
	{
		pPrev = pItem;
		pItem = pItem->m_pNxt != this? pItem->m_pNxt: NULL;
		pNext = pItem? (LtPktInfo*)pItem: NULL;
	}
	if ( pNext && ( seq == pNext->getSequence() ) )
	{
		pPkt->release(); // discard duplicate packet
		m_nDuplicatesInserted++;
	}
	else
	{	pNew->insert( pPrev );
		pNew->m_pHead = this;
		m_nCount++;
	}
	unlock();
}

// return sequence number of first packet on queue
boolean	LtPktReorderQue::sequenceOfFirst( ULONG& seqRtn )
{
	LtQue*		pItem;
	LtPktInfo*	pPkt;
	ULONG		seq = ~0;
	boolean		bOk = false;
	lock();
	pItem = m_pNxt;
	// if queue empty, bug out
	if ( pItem != this )
	{
		pPkt = (LtPktInfo*)pItem;
		seq = pPkt->getSequence();
		seqRtn = seq;
		bOk = true;
	}
	unlock();
	return bOk;
}

// remove a packet of the correct sequence number or NULL
LtPktInfo* LtPktReorderQue::removeSequence( ULONG seq )
{
	LtQue*		pItem;
	LtPktInfo*	pPkt;
	int			seqDiff;
	LtPktInfo*	pPktReturn = NULL;

	lock();
	while ( true )
	{
		pItem = m_pNxt;
		// if queue empty, bug out
		if ( pItem == this )
		{	break;
		}
		pPkt = (LtPktInfo*)pItem;
		seqDiff = seq - pPkt->getSequence();
		if ( seqDiff < 0 )
		{	m_nDuplicatesRemoved++;
			pItem->remove();
			m_nCount--;
			pPkt->release();
		}
		else if ( seqDiff > 0 )
		{	break;
		}
		else // if ( seqDiff == 0 )
		{	pItem->remove();
			m_nCount--;
			pPktReturn = pPkt;
			break;
		}
	}
	unlock();
	return pPktReturn;
}

// Discard stale packets older by delta than timestamp
int LtPktReorderQue::discardStale( ULONG timestamp, ULONG delta )
{
	LtQue*		pItem;
	LtQue*		pNext;
	LtPktInfo*	pPkt;
	ULONG		timeDiff;
	int			nPkts = 0;

	lock();
	pItem = m_pNxt;
	while ( pItem != this )
	{
		pNext = pItem->m_pNxt;
		pPkt = (LtPktInfo*)pItem;
		timeDiff = timestamp - pPkt->getTimestamp();
		if ( timeDiff > delta )
		{	m_nStalePackets++;
			pItem->remove();
			m_nCount--;
			pPkt->setCrumb("LtPktReorderQue::discardStale released");
			pPkt->release();
			nPkts++;
		}
		pItem = pNext;
	}
	unlock();
	return nPkts;
}


// end
