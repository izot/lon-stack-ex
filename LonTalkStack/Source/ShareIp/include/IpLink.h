#if !defined(AFX_IPLINK_H__662677B9_8940_11D2_91CB_00C04F8EC2B8__INCLUDED_)
#define AFX_IPLINK_H__662677B9_8940_11D2_91CB_00C04F8EC2B8__INCLUDED_
/***************************************************************
 *  Filename: IpLink.h
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
 *
 *	DJ Duffy Nov 1998
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/ShareIp/include/IpLink.h#1 $
//
/*
 * $Log: /iLON600/V1.0/ShareIp/include/IpLink.h $
 *
 * 21    12/09/04 6:23p Fremont
 * Change conditional compilation for self installed multicast
 *
 * 20    11/23/04 5:39p Fremont
 * EPRS FIXED: 35208 - Bombardier special - self-installed multicast IP
 * channel mode
 *
 * 19    9/09/03 2:54p Fremont
 * add time-to-live for alternate IP port
 *
 * 18    8/27/03 12:34p Fremont
 * support for determining source ports that may have been switched by a
 * firewall/NAT box.
 *
 * 17    3/25/03 11:39a Fremont
 * include IP port in hashing for slaves
 *
 * 16    11/06/01 2:38p Fremont
 * Add link receive task name override
 *
 * 15    11/06/01 9:25a Glen
 * Need control of zero crossing synchronization and attenuation for
 * LonTalk Validator.  Added these flags to sendPacket().  This propagated
 * to lots of places.  Also added control options to LtMsgOut.
 *
 * 14    9/24/01 5:57p Fremont
 * fix T2 warning
 *
 * 13    2/29/00 4:51p Darrelld
 * Segmentation fixes
 *
 * 11    8/17/99 4:15p Darrelld
 * Fix receive packet size
 *
 * 10    7/06/99 5:23p Darrelld
 * Lock nesting debugging
 *
 * 9     6/22/99 5:00p Darrelld
 * Support for master and slave links
 *
 * 8     4/22/99 5:02p Darrelld
 * Testing of routers
 *
 * 7     4/20/99 4:12p Darrelld
 * add "confidential" statement
 *
 * 6     4/13/99 4:58p Darrelld
 * Enhance for Aggregation and BW Limiting and test
 *
 * 5     4/02/99 5:21p Darrelld
 * Multi-Router Testing in Memory
 *
 * 4     3/18/99 4:17p Darrelld
 * Add cloneInstance
 *
 * 3     3/18/99 9:54a Darrelld
 * Add members for test
 *
 * 2     3/05/99 2:35p Darrelld
 * Add ipaddresses and ports
 *
 * 1     2/22/99 9:19a Darrelld
 * moved here from LTIPtest
 *
 * 2     12/15/98 4:33p Darrelld
 * Reorganize base classes to make LtLink pure and virtual as the driven
 * snow.
 *
 * 1     12/02/98 9:08a Darrelld
 *
 *
 */
//
//////////////////////////////////////////////////////////////////////


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <assert.h>
#include "LtLinkBase.h"
#include <VxSockets.h>
#include <LtObject.h>
#include <LtHashTable.h>
#include "SelfInstallMulticast.h"


//
// LtHipKey class - Hash key class
//
/* This key used to be IP addr only. The port was added
 * to distiguish multiple clients at the same IP addr. However,
 * this is really only needed if there actually ARE duplicate
 * addrs, and it can cause complications when NAT is used, because
 * some NAT routers will switch the outbound port to something else,
 * even if there is a static mapping for the port. This causes
 * a hash search with the port to fail. So, the keys should always be
 * created with the port, but supplying zero for the port when searching
 * will cause the port to be ignored. This is useful unless there
 * are duplicate addresses, in which case the port must be supplied,
 * and the above NAT problem must be dealt with some other way.
 */
class LtHipKey : public LtHashKey
{
public:
	ULONG		ipAddrValue;
	USHORT		ipPortValue;

	LtHipKey( ULONG ipAddr, USHORT ipPort )
	{	ipAddrValue = ipAddr;
		ipPortValue	= ipPort;
	}
	LtHipKey()
	{	ipAddrValue = 0;
		ipPortValue	= 0;
	}
	void	setValue( ULONG ipAddr, USHORT ipPort )
	{	ipAddrValue = ipAddr;
		ipPortValue	= ipPort;
	}
	// NOTE: use ONLY the IP address for the hash code. This
	// will cause all keys with the same IP address to fall in
	// the same hash bucket. These can then easily be detected
	// and searched by iterating all the records in that bucket.
	int		hashCode() { return ipAddrValue; }

	// The comparison operator uses the IP address and OPTIONALLY the port.
	// This allows searching either way, as appropriate.
	boolean operator ==( LtHashKey& key )
	{	return ((ipAddrValue == ((LtHipKey&)key).ipAddrValue) &&
				((ipPortValue == 0) || // OK if either port is zero, or they match
				 (((LtHipKey&)key).ipPortValue == 0) ||
				 (ipPortValue == ((LtHipKey&)key).ipPortValue)));
	}

};

class CIpLink;

//
// LtHipTable Hash table class
//
class LtHipTable : public LtTypedHashTable< LtHipKey, CIpLink>
{
public:
	// Use larger size now that channel has a max of 256 clients
	LtHipTable(int size = 83) : LtTypedHashTable<LtHipKey, CIpLink>(size) {}
	boolean hasDuplicates(LtHipKey* key);
	CIpLink* GetNextDup(LtHipKey* key, LtHashTablePos *pos);
};

class LtIpNetwork;
class LtIpMaster;

class CIpLink : public LtLinkBase
{
public:
	CIpLink();
	virtual ~CIpLink();
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
    virtual LtLinkBase*	copyInstance();

	virtual void reportStatus();

	virtual void setMasterLink( CIpLink* pLink )
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

    virtual int		masterSendSocket()
	{	return m_sendSocket;
	}

	void setLtIpMaster(LtIpMaster* pMaster)
	{
		m_pLtIpMaster = pMaster;
	}

	void setSelfInstalledMcastAddr(ULONG mcAddr)
	{
#if INCLUDE_SELF_INSTALLED_MULTICAST
		m_selfInstalledMcastAddr = mcAddr;
#else
        m_selfInstalledMcastAddr = 0;
#endif
	}
	boolean selfInstalledMcastMode() { return (m_selfInstalledMcastAddr != 0); }
	void setSelfInstalledMcastHops(int hops)
	{
		m_selfInstalledMcastHops = hops;
	}

    virtual int		senderMcastSocket()
	{	return m_ipMCastSenderPort;
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
	virtual void	dumpPacket( const char* tag, byte* pData, int nLen, ULONG ipSrcAddr = 0, ULONG ipDstAddr = 0 );
    virtual int		receiveNoNet( ULONG* ipSrc, ULONG* ipSrcPort, byte* pData, int nMaxLen );

protected:
	void			receiveTask();
	CIpLink*		determineReceiveClient(byte* pPkt, int nPktSize, USHORT& ipPortOverride, boolean& bMcastLoopBackPkt);
	virtual const char*	getRcvTaskName() { return("IPlinkRcv"); }
	int				bindSocket( int type );
    int             createSocket( int type );
    int             getSendSocketToUse();
	virtual void	registerSlave( CIpLink* pLink, ULONG ipAddr, USHORT ipPort );
	virtual void	deregisterSlave( CIpLink* pLink, ULONG ipAddr, USHORT ipPort );
	virtual void	receivePacket( byte* data, int nPktSize, VXSOCKADDR rcvSockAddr, USHORT ipPortOverride = 0 );

	CIpLink*		m_pMasterLink;	// master links owns the socket
	int				m_nPktsPerSlave;// packets allocated per slave
	BOOL			m_bIpActive;
	int				m_socket;       // recv socket
    int				m_sendSocket;   // use to send a diagram in multicast (separate from a socket that receive a diagram)	ULONG			m_ipDstAddr;	// of our partner
	ULONG			m_ipDstAddr;	// of our partner
    ULONG			m_ipSrcAddr;	// of ourselves
	word			m_ipDstPort;	// our partner
	word			m_ipSrcPort;	// ourselves
 	word			m_ipDstPortAlt; // alternate port for our partner (needed for NAT)
	time_t			m_altPortTTL;	// time-to-live for alternate port
	VXSOCKADDR		m_ipDstSockAddr;// Address struct for dest
	VXSOCKADDR		m_ipRcvSockAddr;// Address struct for receive

    USHORT			m_ipMCastSenderPort;	        // multicast sender port
    ULONG			m_selfInstalledMcastAddr;	    // Multicast IP address
	CIpLink*		m_pSelfInstalledMcastClient;	// The other client for multicast
	int				m_selfInstalledMcastHops;	    // Max hops (TTL) for outbound multicast messages

	BOOL			m_bNoHeader;	// No header on outgoing packets
	LtIpNetwork*	m_pIpNet;		// enhanced network object

	LtHipTable		m_htSlaves;		// hash table of slaves keyed by dest IP addr and port

	LtIpMaster*		m_pLtIpMaster;	// point to the owning LtIpMaster

	// keep track of lock owner
	#define LOCKDEPTHMAX 20
	char*			m_pszLockFile[LOCKDEPTHMAX];
	int				m_nLockLine[LOCKDEPTHMAX];
	int				m_nLockDepth;
	void	xLock( char* pszFile, int nLine );
	void	xUnlock( char* pszFile, int nLine );
public:
	void	xReportLockStack();


};

#endif // !defined(AFX_IPLINK_H__662677B9_8940_11D2_91CB_00C04F8EC2B8__INCLUDED_)
