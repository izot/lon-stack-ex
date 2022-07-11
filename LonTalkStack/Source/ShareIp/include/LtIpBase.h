#if !defined(AFX_LTIPBASE_H__38BF21AA_C74D_11D2_9232_00C04F8EC2B8__INCLUDED_)
#define AFX_LTIPBASE_H__38BF21AA_C74D_11D2_9232_00C04F8EC2B8__INCLUDED_
/***************************************************************
 *  Filename: LtIpBase.h
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
 *  Description:  Header file for Lt IP base class.
 *
 *	DJ Duffy Feb 1999
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/ShareIp/include/LtIpBase.h#1 $
//
/*
 * $Log: /Dev/ShareIp/include/LtIpBase.h $
 * 
 * 13    6/09/05 6:07p Fremont
 * EPRS FIXED: 37362 - IP-852 channel uses too much memory. Point all
 * packet allocations to the "master" object, and don't allocate any
 * buffers for the slaves.
 * 
 * 12    4/28/04 6:49p Bobw
 * EPR 32593
 * Support VNI based protocol analyzer.  Pass bad packets up, and time
 * stamp them at first oportunity.  
 * 
 * 11    11/17/00 10:05a Darrelld
 * Remove bogus counters
 * 
 * 9     8/17/99 4:15p Darrelld
 * Symbolize packet size for allocator
 * 
 * 8     7/28/99 4:46p Darrelld
 * Implement master release for packet starvation fix
 * 
 * 7     5/06/99 5:09p Darrelld
 * Enhancments to RFC packets
 * 
 * 6     4/20/99 4:12p Darrelld
 * add "confidential" statement
 * 
 * 5     4/13/99 4:58p Darrelld
 * Enhance for Aggregation and BW Limiting and test
 * 
 * 4     3/12/99 4:47p Darrelld
 * intermediate checkin
 * 
 * 3     3/11/99 5:01p Darrelld
 * intermediate checkin
 * 
 * 2     3/09/99 4:21p Darrelld
 * Add isActive()
 * 
 * 1     2/22/99 9:19a Darrelld
 * moved here from LTIPtest
 * 
 * 
 */

// LtIpBase.h: interface for the LtIpBase class.
//
//////////////////////////////////////////////////////////////////////
#include <LtNetworkBase.h>

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// class LtLink;
class LtPktAllocator;


class LtIpBase : public LtIpNetwork, public LtMsgAllocOwner
{
protected:
	boolean			m_bActive;
	int				m_nReceiveQueueDepth;
	// DJDFIX remove these bogus counters
//	int				m_nRecvsQd;
//	int				m_nRecvsQdP;
	LtPktAllocator*	m_pAlloc;
	//LtLink*			m_pLink; // declared in LtNetworkBase
	SEM_ID			m_lock;
	boolean			m_bMaster;

	void construct(LtIpBase* pMasterLtIpBase);
public:
	LtIpBase(LtIpBase* pMasterLtIpBase);
	LtIpBase();
	virtual ~LtIpBase();

	// block size for our allocator
	// almost as large as a UDP frame
	// allows for IP overhead in frame
	enum { ALLOC_BLOCK_SIZE = 548 };
	// lock and unlock are public for this class so that the
	// LtIpMaster can perform atomic operations of multiple calls
	// on the LtIpClient.
	// Also the lock is a Mutex so that it is nestable in the
	// current thread.
	//
	virtual void		lock()
	{
		semTake( m_lock, WAIT_FOREVER );
	}
	virtual void		unlock()
	{
		semGive( m_lock );
	}

	// Manage the link on start and stop
	virtual boolean		start();
	virtual boolean		stop();
	virtual boolean		isActive()
	{	return m_bActive; }

    virtual void setActive(boolean b) { m_bActive = b; }   
	// for the derived class to use
	virtual void packetReceived( boolean bPriority, 
								LtPktInfo* pPkt, LtSts sts ) = 0;
	// Note that the pPkt contains ipSrcAddr and ipSrcPort fields
	// that are set to direct packet. If set, UDP to that source.
	// If not, then use UDP or TCP to default destination
	virtual LtSts sendPacket( LtPktInfo* pPkt, boolean bPriority );
	// send data in a packet. Allow specifying ipaddr and port, but default
	// to using normal destination
	virtual LtSts sendData( byte* pData, int nLength,
							ULONG ipAddr = 0, word port=0 );
	
	// LtNetwork Methods
	virtual void registerLink( LtLink& pLink );
	virtual void packetReceived(void* referenceId,
							int nLengthReceived,
							boolean bPriority,
							int receivedSlot,
							boolean isValidLtPacket, 
							byte l2PacketType,
							LtSts sts);

	// LtIpNetwork Methods
	virtual void packetReceived(void* referenceId,
								int nLengthReceived,
								boolean bPriority,
								ULONG ipSrcAddr,
								word ipSrcPort,
								LtSts sts);

	
	virtual void packetComplete(void* referenceId, LtSts sts );

	// IO related members
	virtual boolean	masterRelease( LtMsgRef* pMsg );
	virtual void	queueReceives( boolean bPriority );

};

#endif // !defined(AFX_LTIPBASE_H__38BF21AA_C74D_11D2_9232_00C04F8EC2B8__INCLUDED_)
