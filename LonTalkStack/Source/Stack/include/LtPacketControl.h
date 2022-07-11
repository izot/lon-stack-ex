#ifndef LTPACKETCONTROL_H
#define LTPACKETCONTROL_H

//
// LtPacketControl.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtPacketControl.h#1 $
//

typedef enum
{
    LT_REF_RX,
    LT_REF_TX,
    LT_REF_MSGTAG,
    LT_REF_NV
} LtRefIdType;

class LTA_EXTERNAL_CLASS LtRefId 
{
private:
    LtRefIdType     m_nType;
    int             m_nIndex;
    int             m_nRefId;

protected:
    LtRefId(LtBlob *pBlob) 
    {
        package(pBlob);
    };
    void package(LtBlob *pBlob);
    friend class LtStackBlob;

public:
	LtRefId() { m_nRefId = 0; }
    LtRefId(LtRefIdType nType, int nIndex, int nRefId) : m_nType(nType), m_nIndex(nIndex), m_nRefId(nRefId) {}

    LtRefIdType getType()   { return m_nType; }
    int     getIndex()      { return m_nIndex; }
    int     getRefId()      { return m_nRefId; }

    void    setRefId(int nRefId) { m_nRefId = nRefId; }

	boolean matches(LtRefId& ref) { return m_nRefId == ref.m_nRefId; }
	void	bump()			{ ++m_nRefId; }
};

class LTA_EXTERNAL_CLASS LtPacketControl 
{
	DefineBool(OrigPri)
    DefineBool(Priority)
    DefineBool(Authenticated)
    DefineBool(Turnaround)
    DefineBool(Success)
    DefineBool(Failure)
    DefineBool(NullResponse)
    DefineBool(NoCompletion)
    DefineBool(AnyTxFailure)
    DefineBool(AnyValidResponse)
    DefineBool(Proxy)
    DefineInt(AlternatePath)
    DefineInt(DeltaBacklog)
    DefineInt(AddressIndex)     // -1 => explicit address
    DefineInt(NvIndex)          // -1 => explicit message
    DefineBool(RespondOnFlexDomain)
	DefineBool(ZeroSync)
	DefineBool(Attenuate)
    DefineInt(NvIncarnationNumber)
	DefineInt(ProxyCount)		// If Proxy is true, number of proxy hops from source at this repeater/agent

private:
	LtRefId			m_refId;
    LtServiceType   m_serviceType;
	LtVectorPos		m_aliasIndex;
        
protected:
    LtPacketControl(LtBlob &blob) 
    {
        package(&blob);
    };
    void package(LtBlob *pBlob);

public:
    LtPacketControl() : m_refId(LT_REF_RX, 0, 0)
	{
		// Don't memset - wipes out vptrs - memset(this, 0, sizeof(*this)); 
	    setOrigPri(0);
        setPriority(0);
        setAuthenticated(0);
        setTurnaround(0);
        setSuccess(0);
        setFailure(0);
        setNullResponse(0);
        setNoCompletion(0);
        setAnyTxFailure(0);
        setAnyValidResponse(0);
        setProxy(0);
        setAlternatePath(0);
        setDeltaBacklog(0);
		setAddressIndex(-1);
		setNvIndex(-1);
        m_serviceType = LT_ACKD;
        setRespondOnFlexDomain(0);
		setAttenuate(0);
		setZeroSync(0);
		setProxyCount(-1);
	}
	LtVectorPos &getAliasIndex() { return m_aliasIndex; }
	
    LtRefId& getRefId()	{ return m_refId; }
    void setRefId(LtRefId& ref) { m_refId = ref; }
	void setRefId(int refId)	{ m_refId.setRefId(refId); }

    LtServiceType getServiceType() { return m_serviceType; }
    void setServiceType(LtServiceType type) { m_serviceType = type; }
};

#endif
