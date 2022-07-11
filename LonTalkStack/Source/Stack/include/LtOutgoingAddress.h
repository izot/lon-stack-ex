#ifndef LTOUTGOINGADDRESS_H
#define LTOUTGOINGADDRESS_H

//
// LtOutgoingAddress.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtOutgoingAddress.h#1 $
//

/**
 * This class defines the outgoing address used for explicit addressing of 
 * explicit messages.
 *
 */

class LTA_EXTERNAL_CLASS LtOutgoingAddress: public LtAddressConfiguration {

private:
    // Destination address information (those forms not supported by AddressConfiguration)
    LtUniqueId				m_destinationUniqueId;
    int						m_nSizeOverride;
    LtDomainConfiguration	m_domain;
	int						m_subnetNodeIndex;

protected:
    LtOutgoingAddress(LtBlob &blob) : LtAddressConfiguration(blob)
    {
        packageMyData(&blob);
    }
    void packageMyData(LtBlob *pBlob);

    void package(LtBlob *pBlob);
    friend class LtStackBlob;

	void setDestinationUniqueId(LtUniqueId& id) { m_destinationUniqueId = id; }

public:
    LtDomainConfiguration& getDomainConfiguration();
    int getSize();

public:
    LtOutgoingAddress();
    LtOutgoingAddress(LtOutgoingAddress& address);
    void copy(LtAddressConfiguration& ac);
    void setDomainConfiguration(LtDomainConfiguration& domain);
	void setLocal() { setAddressType(LT_AT_LOCAL); }
	void set(int type, int domain, int retry, int timer, ULONGLONG destId, int misc);
    void setSubnetNode(int domain, int subnet, int node,
                              int timer, int retry);
    void setGroup(int domain, int group, int groupSize,
                         int timer, int retry);
    void setBroadcast(int domain, int subnet, int backlog,
                             int timer, int retry);
    void setUniqueId(int domain, int subnet, LtUniqueId& id,
                            int timer, int retry);
    void setGroupSize(boolean inGroup);
	int toLonTalk(byte data[], int nVersion);
    LtErrorType fromLonTalk(byte data[], int& len, int nVersion);
	void setSubnetNode(int sn, int nd);

	LtUniqueId& getDestinationUniqueId() { return m_destinationUniqueId; }

	int getSubnetNodeIndex() { return m_subnetNodeIndex; }
	void setSubnetNodeIndex(int index) { m_subnetNodeIndex = index; }

	int getTxDuration(LtServiceType st);
};

#endif
