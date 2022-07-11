#ifndef _LTSTACKCLIENT_H
#define _LTSTACKCLIENT_H

//
// LtStackClient.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtStackClient.h#1 $
//

// Each unique address in the stack is assigned a LRE client.  It 
// is the stack's responsibility to ensure that duplicate addresses
// are not presented.  Duplicates are possible and particularly 
// likely with group addresses.  
class LtStackClient : public LtLreClient
{
private:
	LtDeviceStack*			m_pStack;

protected:
	LtDeviceStack*			getStack()			{ return m_pStack; }

public:
	LtStackClient(LtDeviceStack* pStack, LtChannel *pChannel) : 
		LtLreClient(pStack->isNodeStack()?LT_NODE:LT_ROUTER_SIDE, pChannel), 
		m_pStack(pStack) {}
    ~LtStackClient() {}

	void processPacket(boolean bPriority,
							   LtPktInfo *pPkt);

	void* getId()
	{
		return (void*) m_pStack;
	}

	LtLreClientOwner* getOwner()
	{
		return m_pStack;
	}

	boolean ownerIsLtDeviceStack()
	{
		return true;
	}

	boolean getIsMulti()
	{
		return true;
	}

    void setErrorLog(int error);

    void updateStats(const byte* pApdu, bool isUnackd, bool domainMatch, bool gotThisMsg);
};

class LtLayer2Client : public LtStackClient
{
public:
	LtLayer2Client(LtDeviceStack* pStack, LtLogicalChannel* pChannel) :
	  LtStackClient(pStack, pChannel) {}

    boolean getNeedAllLayer2Packets()
    {
        return true;
    }


    boolean needValidCrc() { return true; }  // The CRC is reported to the client - needs to be valid.
};

class LtUniqueIdClient : public LtStackClient, public LtUniqueId, public LtDomainConfiguration 
{
public:
	LtUniqueIdClient(LtDeviceStack* pStack, LtLogicalChannel* pChannel, LtUniqueId* pUid) : 
		LtStackClient(pStack, pChannel), LtUniqueId(*pUid) {}

	// Function returns the unique ID(s) associated with this client.  false means
	// there is none.
	boolean getAddress(int& index, LtUniqueId *pUniqueId);

	boolean getNeedAllBroadcasts()
	{
		return true;
	}

	boolean start()
	{
		// When we get a "start", generate an event.  The reason for this is so that the router
		// can get the latest and greatest after a boot.
		getStack()->notify();
		return true;
	}
	boolean stop();
};

class LtSubnetNodeClient : public LtUniqueIdClient
{
public:
	LtSubnetNodeClient(LtDeviceStack* pStack, LtLogicalChannel* pChannel, LtUniqueId* pUid) : 
		LtUniqueIdClient(pStack, pChannel, pUid) {}

	boolean getAddress(int& nIndex, LtDomain *pDomain, LtSubnetNode *pSubnetNode, LtGroups *pGroup);
	boolean getAddress(int& index, LtUniqueId *pUniqueId);
	boolean getNeedAllBroadcasts()
	{
		return false;
	}
};

// Note that this scheme has a built-in assumption that a router side is not configured
// more than once in a domain.  Given the way the routing commands are set up to 
// domain indices, etc., this seems to be a requirement anyway.  A Neuron will ignore
// the second domain entry if it's a duplicate but this implemenation will end up using
// essentially an orring of the two different tables (it will route to the subnets 
// specified in either table).  I don't think this is a problem though.  
// A relatively straightforward fix would be to report a route only for the first
// client in a given domain.
class LtRouterDomainClient : public LtSubnetNodeClient
{
private:
	LtRoutingMap	m_route[2];
public:
	LtRouterDomainClient(LtDeviceStack* pStack, LtLogicalChannel* pChannel, LtUniqueId* pUid) : 
		LtSubnetNodeClient(pStack, pChannel, pUid) {}

	LtRoutingMap& getRoute(boolean bEeprom) { return m_route[bEeprom ? 1 : 0]; }

	LtErrorType setRoute(LtRoutingMap* pRoute, boolean bEeprom);

	boolean getRoute(boolean bEeprom, LtRoutingMap* pRoute);

	boolean getExternalRoute(int& index, LtRoutingMap* pRoute, int* pRoutingSubnet);
};

#endif
