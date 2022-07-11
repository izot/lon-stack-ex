//
// LtOutgoingAddress.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtOutgoingAddress.cpp#1 $
//

#include "LtStack.h"

/**
 * This class defines the outgoing address used for explicit addressing of 
 * explicit messages.
 *
 */

//
// Private Member Functions
//


//
// Protected Member Functions
//

void LtOutgoingAddress::packageMyData(LtBlob *pBlob)
{
    LtStackBlob stackBlob(pBlob);
    pBlob->package(&m_destinationUniqueId);
    pBlob->package(&m_nSizeOverride);
	pBlob->package(&m_subnetNodeIndex);
    stackBlob.package(&m_domain);
}

void LtOutgoingAddress::package(LtBlob *pBlob)
{
    LtAddressConfiguration::package(pBlob);
    packageMyData(pBlob);
}
                         
LtDomainConfiguration& LtOutgoingAddress::getDomainConfiguration() {
    return m_domain;
}

void LtOutgoingAddress::setSubnetNode(int sn, int nd) {
	setSubnet(sn);
	setDestId(nd);
}

void LtOutgoingAddress::setGroupSize(boolean inGroup) {
    // To accommodate fact that node is not a member of group, bump the
    // size so that we expect acks from ALL members.
    m_nSizeOverride = LtAddressConfiguration::getSize() + (inGroup ? 0 : 1);
}

int LtOutgoingAddress::getSize() {
    return m_nSizeOverride;
}

//
// Public Member Functions
//


LtOutgoingAddress::LtOutgoingAddress() {
    setAddressType(LT_AT_UNBOUND);
	m_subnetNodeIndex = -1;
}

LtOutgoingAddress::LtOutgoingAddress(LtOutgoingAddress& address) {
    LtAddressConfiguration::copy(address);
    m_domain = address.m_domain;
    m_destinationUniqueId = address.m_destinationUniqueId;
	m_subnetNodeIndex = address.m_subnetNodeIndex;
}

void LtOutgoingAddress::copy(LtAddressConfiguration& ac) {
    LtAddressConfiguration::copy(ac);
    m_nSizeOverride = ac.getSize();
}

/**
 * Set the domain configuration.  The normal means for setting
 * the domain is to set the domain index in the 
 * <a href="COM.Echelon.LonTalk.Api.AddressConfiguration.html">address configuration</a>.
 * Not supported by all platforms (i.e., not supported by Neuron platform).
 * @param domain
 *          The domain configuration to be used.  Allows transmission on
 *          a domain other than the configured domain but will only work
 *          for unackd or unackd repeat service types.
 */
void LtOutgoingAddress::setDomainConfiguration(LtDomainConfiguration& domain) {
    m_domain = domain;
}

void LtOutgoingAddress::set(int type, int domain, int retry, int timer, ULONGLONG destId, int misc)
{
	setAddressType((LtAddressType)type);
	setDomainIndex(domain);
	setRetry(retry);
	setRptTimer(timer);
	setTxTimer(timer);
	switch (type)
	{
	case LT_AT_BROADCAST:
		setBacklog(misc);
		setSubnet((int)destId);
		break;
	case LT_AT_GROUP:
		setSize(misc);		// Group
	    m_nSizeOverride = misc;
		setDestId((int)destId);  // Group #
		break;
	case LT_AT_SUBNET_NODE:
		setSubnet(misc);	// Subnet/node
		setDestId((int)destId);
		break;
	case LT_AT_UNIQUE_ID:
		setSubnet(misc);
		getDestinationUniqueId().set(destId);
		break;
	}
}

/**
 * Sets up a subnet node address.
 * @param domain
 *              Destination domain (index)
 * @param subnet
 *              Destination subnet
 * @param node
 *              Destination node
 * @param timer
 *              Retransmit or repeat timer (milliseconds)
 * @param retry
 *              Retry count
 */
void LtOutgoingAddress::setSubnetNode(int domain, int subnet, int node,
                          int timer, int retry) {
    setAddressType(LT_AT_SUBNET_NODE);
	setDomainIndex(domain);
	setSubnet(subnet);
	setDestId(node);
	setRptTimer(timer);
	setTxTimer(timer);
	setRetry(retry);
}
                          
/**
 * Sets up a group address.
 * @param domain
 *              Destination domain (index)
 * @param group
 *              Destination group
 * @param groupSize
 *              Destination group size.  Add one to group size if originator
 *              is not a member of the group (known as open group).
 * @param timer
 *              Retransmit or repeat timer (milliseconds)
 * @param retry
 *              Retry count
 * 
 */
void LtOutgoingAddress::setGroup(int domain, int group, int groupSize,
                     int timer, int retry) {
	setAddressType(LT_AT_GROUP);
	setDomainIndex(domain);
	setSize(groupSize);
	setRptTimer(timer);
	setTxTimer(timer);
	setRetry(retry);
	LtAddressConfiguration::setGroup(group);

    m_nSizeOverride = groupSize;
}                                                     

/**
 * Sets up a broadcast address.
 * @param domain
 *              Destination domain (index)
 * @param subnet
 *              Destination subnet (0 for domain wide broadcast)
 * @param backlog
 *              For request/response, expected number of responses.  Allows for
 *              appropriate setting of backlog (only one response is actually
 *              returned).
 * @param timer
 *              Retransmit or repeat timer (milliseconds)
 * @param retry
 *              Retry count
 * 
 */
void LtOutgoingAddress::setBroadcast(int domain, int subnet, int backlog,
                         int timer, int retry) {
    setAddressType(LT_AT_BROADCAST);
	setDomainIndex(domain);
	setBacklog(backlog);
	setSubnet(subnet);
	setRptTimer(timer);
	setTxTimer(timer);
	setRetry(retry);
}                                

/**
 * Sets up a unique ID address.
 * @param domain
 *              Destination domain (index)
 * @param subnet
 *              Destination subnet (for routing purposes only, 0 for flood)
 * @param UniqueId
 *              Destination unique ID
 * @param timer
 *              Retransmit or repeat timer (milliseconds)
 * @param retry
 *              Retry count
 * 
 */
void LtOutgoingAddress::setUniqueId(int domain, int subnet, LtUniqueId& id,
                        int timer, int retry) {
    setAddressType(LT_AT_UNIQUE_ID);	
	setDomainIndex(domain);
	setSubnet(subnet);
	setRptTimer(timer);
	setTxTimer(timer);
	setRetry(retry);
	setDestinationUniqueId(id);
}

int LtOutgoingAddress::toLonTalk(byte data[], int nVersion) 
{
    int length = LtAddressConfiguration::toLonTalk(data, nVersion);
	if (getAddressType() == LT_AT_UNIQUE_ID) 
	{
		// Need to also set a unique ID
		getDestinationUniqueId().getData(&data[5]);
		length += getDestinationUniqueId().getLength();
	}
	else if (getAddressType() == LT_AT_GROUP) 
	{
		data[0] = 0x80 | getSize();
	}
	return length;
}

LtErrorType LtOutgoingAddress::fromLonTalk(byte data[], int& len, int nVersion) 
{
    LtErrorType err = LtAddressConfiguration::fromLonTalk(data, len, nVersion);
	if (err == LT_NO_ERROR)
	{
		if (getAddressType() == LT_AT_UNIQUE_ID) {
			// Need to also set a unique ID
			len += getDestinationUniqueId().getLength();
			getDestinationUniqueId().set(&data[5]);
		}
	}
	return err;
}

int LtOutgoingAddress::getTxDuration(LtServiceType st)
{
	int duration = 0;
	switch (st)
	{
	case LT_UNACKD_RPT:
		duration += getRptTimer() * (getRetry() + 1);
		break;
	case LT_ACKD:
	case LT_REQUEST:
		duration += getTxTimer() * (getRetry() + 1);
		break;
	default:
		break;	// nothing
	}
	return duration;
}
