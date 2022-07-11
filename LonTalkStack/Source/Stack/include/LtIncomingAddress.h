#ifndef LTINCOMINGADDRESS_H
#define LTINCOMINGADDRESS_H

//
// LtIncomingAddress.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtIncomingAddress.h#1 $
//


/**
 * This class defines the incoming address which can be examined for an
 * incoming NV update, NV poll response, or explicit message.
 *
 */

namespace Snm
{
	class Interface;
};

class LTA_EXTERNAL_CLASS LtIncomingAddress {

friend class LtLayer4;
friend class LtTransactions;
friend class Snm::Interface;

private:
    LtAddressFormat			m_addressFormat;	// Incoming address format
    LtDomainConfiguration	m_domain;			// Source domain/subnet/node
	LtSubnetNode			m_destNode;			// Destination subnet/node
    int						m_nGroup;			// Group (group or group_ack
    int						m_nGroupMember;		// Source group member (group_ack)

protected:
    LtIncomingAddress(LtBlob &blob) 
    {
        package(&blob);
    };
    void package(LtBlob *pBlob);
friend class LtStackBlob;
friend class LtLayer5Mip;

    inline LtIncomingAddress() {}
	void setAddressFormat(LtAddressFormat addressFormat) { m_addressFormat = addressFormat; }
	void setDomainConfiguration(LtDomainConfiguration& domain) { m_domain = domain; }
	void setGroup(int group) { m_nGroup = group; }
	void setGroupMember(int groupMember) { m_nGroupMember = groupMember; }

	void setTurnaroundAddress() { setAddressFormat(LT_AF_TURNAROUND); }

public:
	LtAddressFormat getAddressFormat() { return m_addressFormat; }
    LtDomainConfiguration& getDomainConfiguration() { return m_domain; }
	LtSubnetNode& getSubnetNode() { return m_destNode; }
	int getSubnet() { return m_destNode.getSubnet(); }
	int getNode() { return m_destNode.getNode(); }
	int getGroup() { return m_nGroup; }
	int getMember() { return m_nGroupMember; }
};

#endif
