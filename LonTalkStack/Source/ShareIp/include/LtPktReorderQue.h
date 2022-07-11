/***************************************************************
 *  Filename: LtPktReorderQue.h
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
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/ShareIp/include/LtPktReorderQue.h#1 $
//
/*
 * $Log: /ShareIp/include/LtPktReorderQue.h $
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
#ifndef _LTPKTREORDERQUE_H
#define _LTPKTREORDERQUE_H
// keep users out of the queue directly
class LtPktReorderQue : protected LtQue
{
public:
	ULONG	m_nStalePackets;
	ULONG	m_nDiscardedPackets;
	ULONG	m_nDuplicatesInserted;
	ULONG	m_nDuplicatesRemoved;

	LtPktReorderQue() : LtQue(true) // as head
	{
		m_nDiscardedPackets		= 0;
		m_nStalePackets			= 0;
		m_nDuplicatesInserted	= 0;
		m_nDuplicatesRemoved	= 0;
	}

	virtual ~LtPktReorderQue()
	{
		discard();
	}
	// discard all packets in the queue
	// we had a session reset or are shutting down
	void discard();

	// insert a packet and fill in the timestamp
	// and sequence number
	void insert( LtPktInfo* pPkt, LtIpPktHeader& phd );
	// return sequence number of first packet on queue
	boolean	sequenceOfFirst( ULONG& seqRtn );
	// remove a packet of the correct sequence number or NULL
	LtPktInfo* removeSequence( ULONG seq );

	// Discard stale packets older than timestamp by delta
	int discardStale( ULONG timestamp, ULONG delta );

	BOOL lockedIsEmpty()
	{	return LtQue::lockedIsEmpty();
	}

};

#endif // _LTPKTREORDERQUE_H
