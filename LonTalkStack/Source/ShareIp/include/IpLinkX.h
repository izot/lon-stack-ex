#if !defined(AFX_IPLINK_H__662677B9_8940_11D2_91CB_00C04F8EC2B8__INCLUDED_)
#define AFX_IPLINK_H__662677B9_8940_11D2_91CB_00C04F8EC2B8__INCLUDED_
/***************************************************************
 *  Filename: IpLinkX.h
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
 *  Description:  interface for the CIpLink class.
 *				This is a surrogate class for use with the Config Server
 *
 *	DJ Duffy Nov 1998
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/ShareIp/include/IpLinkX.h#1 $
//
/*
 * $Log: /Dev/ShareIp/include/IpLinkX.h $
 * 
 * 4     11/06/01 9:25a Glen
 * Need control of zero crossing synchronization and attenuation for
 * LonTalk Validator.  Added these flags to sendPacket().  This propagated
 * to lots of places.  Also added control options to LtMsgOut.
 * 
 * 2     1/11/00 12:42p Dwf
 * added virtual to sendPacketTo()
 * 
 * 1     12/10/99 3:53p Darrelld
 * MFC based segment base classes
 * 

 * 
 */
//
//////////////////////////////////////////////////////////////////////


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

typedef enum {
	LTSTS_PENDING = 0,			// immediate return, packet queued
	LTSTS_OK = 1,				// success
	LTSTS_QUEUEFULL,			// immediate return, send/receive failure
	LTSTS_TIMEOUT,				// timeout occurred
	LTSTS_CRCERROR,				// CRC error on received packet
	LTSTS_TOOLONG,				// Transmitted packet length too long
	LTSTS_TOOSHORT,				// Received packet less than 8 bytes
	LTSTS_PREAMTOOLONG,			// Preamble too long causing neuron timeout
	LTSTS_PREAMTOOSHORT,		// Preamble too short causing receive error
	LTSTS_OVERRUN,				// Overrun of FIFO
	LTSTS_RESET,				// reset occurred
	LTSTS_TERMINATED,			// terminate occurred
	LTSTS_ERROR,				// need a short list of meaningful errors
	LTSTS_COLLIDED,				// packet discarded due to collisions
	LTSTS_COMMPORTINITFAILURE,	// Comm port init failure
	LTSTS_INVALIDSTATE,			// Invalid state for call
	LTSTS_OPENFAILURE,			// Unable to open the device
	LTSTS_NOTRESET,				// A previous reset was expected
	LTSTS_END					// highest value of status
} LtSts;


class CIpLink
{
public:
	CIpLink();
	virtual ~CIpLink();

	virtual LtSts sendPacketTo( LtPktInfo* pPkt, ULONG ipAddr, word port, 
									byte* pData, int nDataLen );
#if 0 // trim class
	boolean enumInterfaces( int idx, LPSTR pNameRtn, int nMaxSize );

	virtual void		setDstIp( ULONG ipAddr, word port )
	{	m_ipDstAddr = ipAddr;
		m_ipDstPort = port;
	}
	virtual void		setSrcIp( ULONG ipAddr, word port )
	{	m_ipSrcAddr = ipAddr;
		m_ipSrcPort = port;
	}
	// send a packet to an explicit ipaddr/port
	virtual LtSts sendPacketTo( void* refId, ULONG ipAddr, word port, byte* pData, int nLen );

	virtual LtLinkBase*	cloneInstance()
	{	return new CIpLink();
	}
	virtual void reportStatus();

	virtual setMasterLink( CIpLink* pLink )
	{	m_pMasterLink = pLink;
	}
	virtual boolean isMaster()
	{	return ( m_pMasterLink == NULL );
	}
	virtual void	setPktsPerSlave( int nPkts )
	{	m_nPktsPerSlave = nPkts;
	}

	virtual int		masterSocket()
	{	return m_socket;
	}

	// max size of a UPD frame
	enum { MAX_UDP_FRAME_SIZE = 576
	};

	LtSts open( const char* pName );

	LtSts close();

	boolean	isOpen()
	{	return m_bIpActive;
	}

	LtSts sendPacket(void* referenceId,
					int nPrioritySlot,
					byte flags,
					byte* pData,
					int nDataLength,
					boolean bPriority);

	LtSts queueReceive( void* referenceId,
						boolean bPriority,
						byte flags,
						byte* pData,
						int nMaxLength);
	void reset();

	void setNoHeader( BOOL bNoHeader )
	{	m_bNoHeader = bNoHeader;
	}
	void registerIpNetwork( LtIpNetwork* pIpNet )
	{	m_pIpNet = pIpNet;
		m_pNet = (LtNetwork*)pIpNet;
	}
	virtual void	dumpPacket( LPSTR tag, byte* pData, int nLen );
	int		receiveNoNet( ULONG* ipSrc, ULONG* ipSrcPort, byte* pData, int nMaxLen );

protected:
	void			receiveTask();
	int				bindSocket( int type );
	virtual void	registerSlave( CIpLink* pLink, ULONG ipAddr );
	virtual void	deregisterSlave( CIpLink* pLink, ULONG ipAddr );
	virtual void	receivePacket( byte* data, int nPktSize, VXSOCKADDR rcvSockAddr );

	CIpLink*		m_pMasterLink;	// master links owns the socket
	int				m_nPktsPerSlave;// packets allocated per slave
	BOOL			m_bIpActive;
	int				m_socket;
	ULONG			m_ipDstAddr;	// of our partner
	ULONG			m_ipSrcAddr;	// of ourselves
	word			m_ipDstPort;	// our partner
	word			m_ipSrcPort;	/// ourselves
	VXSOCKADDR		m_ipDstSockAddr;// Address struct for dest
	VXSOCKADDR		m_ipRcvSockAddr;// Address struct for receive

	BOOL			m_bNoHeader;	// No header on outgoing packets
	LtIpNetwork*	m_pIpNet;		// enhanced network object

	LtHipTable		m_htSlaves;		// hash table of slaves keyed by Dest ip Address


	// keep track of lock owner
	#define LOCKDEPTHMAX 20
	char*			m_pszLockFile[LOCKDEPTHMAX];
	int				m_nLockLine[LOCKDEPTHMAX];
	int				m_nLockDepth;
	void	xLock( char* pszFile, int nLine );
	void	xUnlock( char* pszFile, int nLine );
public:
	void	xReportLockStack();
#endif // trim class

};

#endif // !defined(AFX_IPLINK_H__662677B9_8940_11D2_91CB_00C04F8EC2B8__INCLUDED_)
