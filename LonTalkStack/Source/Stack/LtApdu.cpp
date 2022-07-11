//
// LtApdu.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtApdu.cpp#1 $
//

#include "LtStackInternal.h"

//
// Private Member Functions
//


//
// Protected Member Functions
//


/**
 * This method sets the outgoing address to be used for the
 * message.  This enables explicit addressing.  That is, the address specified in the outgoing address is
 * used rather than an address from the address table.
 * @param outgoingAddress
 *              The outgoing address to be used as the message destination.
 */
void LtApdu::setOutgoingAddress(LtOutgoingAddress& outgoingAddress) {
    *(LtOutgoingAddress*) this = outgoingAddress;
}

//
// Public Member Functions
//


LtApdu::LtApdu(boolean bOutput) 
{
	m_len = 0;
	m_bOutput = bOutput;
	m_nNvDataOffset = 1;
}

	
LtApdu::LtApdu(boolean bOutput, LtApdu* pReq) 
{
	// Used for sending responses.  Key things to copy are the reference
	// id, the turnaround bit, priority and NV index (may be others too).
	*(LtPacketControl*) this = *(LtPacketControl*) pReq;
	m_len = 0;
	m_bOutput = bOutput;
	m_nNvDataOffset = 1;
	setServiceType(LT_RESPONSE);
}

LtApdu::LtApdu(LtBlob &blob) : LtPacketControl(blob) 
{
    packageMyData(&blob);
}

void LtApdu::package(LtBlob *pBlob) 
{
    LtPacketControl::package(pBlob);
    packageMyData(pBlob);
}

void LtApdu::packageMyData(LtBlob *pBlob) 
{
    pBlob->package(&m_bOutput);
    pBlob->package(&m_len);
    pBlob->package(m_data, m_len);
	pBlob->package(&m_nNvDataOffset);
}


void LtApdu::reinit() 
{
	setDeltaBacklog(0);
	m_nNvDataOffset = 1;
}

void LtApdu::flipNvCode() {
    m_data[0] ^= 0x40;
}

int LtApdu::getFlippedNvCode() {
    return getCode() ^ 0x40;
}

boolean LtApdu::isRequest() {
    return getServiceType() == LT_REQUEST;
}

int LtApdu::getDataLength() {
    return (m_len > 0) ? (m_len - 1) : 0;
}

void LtApdu::getData(byte data[], int offset, int length) {
    memcpy(data, &m_data[offset + 1], length);
}

byte LtApdu::getData(int i) {
    return m_data[1 + i];
}

void LtApdu::setData(int i, int value) {
    if (i + 1 < (int)sizeof(m_data))
    {
        m_data[i + 1] = (byte) value;
    	m_len = max(i + 2, m_len);
    }
}

LtErrorType LtApdu::setNvData(byte data[], int len) 
{
    return setData(data, 1, len);
}

LtErrorType LtApdu::setData(byte data[], int offset, int len) 
{
	LtErrorType err = LT_NO_ERROR;
    if (offset + 1 + len > (int)sizeof(m_data)) 
	{
		err = LT_INVALID_PARAMETER;
    }
    else if (len != 0) 
	{
		memcpy(&m_data[offset + 1], data, len);
		m_len = max(offset + 1 + len, m_len);
    }
	return err;
}

void LtApdu::setCodeAndData(LtApdu* pApdu)
{
	m_len = pApdu->m_len;
	memcpy(m_data, pApdu->m_data, m_len);
}

void LtApdu::setCodeAndData(byte data[], int len)
{
	m_len = len;
	memcpy(m_data, data, m_len);
}

int LtApdu::getCode() {
    return m_data[0];
}

void LtApdu::setCode(int newCode) {
    m_data[0] = (byte) newCode;
	m_len = max(1, m_len);
}

LtRefId& LtApdu::getTag() {
    return getRefId();
}

LtErrorType LtApdu::getNvData(byte** ppData, int &lengthFromDevice, int len, boolean bChangeableLength) 
{
	LtErrorType err = LT_NO_ERROR;
	int offset = getNvDataOffset() + 1;
	lengthFromDevice = m_len - offset;
	int maxLen = len;
	int minLen = bChangeableLength ? 1 : len;
    if (lengthFromDevice < minLen || lengthFromDevice > maxLen)
	{
		err = LT_NV_LENGTH_MISMATCH;
    }
	else
	{
		*ppData = &m_data[offset];
	}
	return err;
}

boolean LtApdu::isNetworkVariable() {
    return getNvIndex() != -1;
}

boolean LtApdu::isNetworkVariableMsg() {
    return (getLength() > 0 && (getCode() & LT_APDU_NETWORK_VARIABLE_MASK) == LT_APDU_NETWORK_VARIABLE);
}

boolean LtApdu::isApplMsg() {
    return (getLength() > 0 && (getCode() & LT_APDU_MESSAGE_MASK) == LT_APDU_MESSAGE);
}

boolean LtApdu::isForeignMsg() {
    return (getLength() > 0 && (getCode() & LT_APDU_FOREIGN_MASK) == LT_APDU_FOREIGN);
}

LtErrorType LtApdu::getSelector(int& selector, boolean& bOutput) 
{
	LtErrorType err = LT_NO_ERROR;
    if (getDataLength() < 1) 
	{
		err = LT_NV_MSG_TOO_SHORT;
	}
	else
	{
        selector = LtMisc::makeint((byte)(getCode() & 0x3f), getData(0));
		bOutput = (getCode() & 0x40) == 0x40;
    }
	return err;
}

boolean LtApdu::forProxyHandler()
{
#if FEATURE_INCLUDED(LTEP)
	int code = getCode();
	return code == LT_APDU_ENHANCED_PROXY;
#else
	return false;
#endif
}

boolean LtApdu::forNetworkManager() {
    int code = getCode();
    return code >= LT_APDU_NETWORK_DIAGNOSTIC &&
           code < LT_APDU_NETWORK_VARIABLE;
}

boolean LtApdu::isNetworkDiagnostic()
{
	int code = getCode();
	return code >= LT_APDU_NETWORK_DIAGNOSTIC &&
		   code < LT_APDU_NETWORK_MANAGEMENT;
}

int LtApdu::getNmCode(boolean success) 
{
	return getNmCode(getCode(), success);
}

int LtApdu::getNmCode(int code, boolean success)
{
	return (code & 0x1f) | (success ? LT_NM_PASS : LT_NM_FAIL);
}

boolean LtApdu::getNmPass(int code)
{
	return getNmCode(code, true) == getCode();
}

boolean LtApdu::getProxyRelay()
{
	return getResponse() &&	getProxy();
}

void LtApdu::setUpProxy() 
{
    setNoCompletion(true);
	setProxy(1);
}

void LtApduIn::init(LtDeviceStack* pStack, LtPktInfo* pPkt, LtRefId& refId)
{
	byte ssiReg1, ssiReg2;
	// Set up APDU fields for upper layers
	setRefId(refId);
	setPriority(pPkt->getPriority());
	setAuthenticated(false);
	setCodeAndData(pPkt->getData(), pPkt->getLength());
	setPhaseReading(pPkt->getPhaseReading());
	bool ssiValid = pPkt->getSsi(ssiReg1, ssiReg2);
	setSsi(ssiValid, ssiReg1, ssiReg2);
	setViaLtNet(pPkt->getSourceClient()->getClientType() == LT_LONTALK_PORT);

	// Set incoming address info
	setAddressFormat(pPkt->getAddressFormat());
	setAlternatePath(pPkt->getAltPath() ? LT_ALT_PATH : LT_NORMAL_PATH);

	if (pStack->unconfigured())
	{
		// Always report flex domain in this case.
		LtDomainConfiguration dc(pPkt->getDomain(), null, pPkt->getSourceNode().getSubnet(),
			pPkt->getSourceNode().getNode());
		setDomainConfiguration(dc);
	}
	else
	{
		int domainIndex = 0;
		switch(getAddressFormat())
		{
			case LT_AF_UNIQUE_ID:
			case LT_AF_BROADCAST:
			{
				LtUniqueIdClient* pClient = (LtUniqueIdClient*) pPkt->getDestClient();
				domainIndex = pClient->getIndex();
				getSubnetNode().set(pPkt->getDestSubnet(), 0);
				break;
			}
			case LT_AF_SUBNET_NODE:
			{
				LtSubnetNodeClient* pClient = (LtSubnetNodeClient*) pPkt->getDestClient();
				domainIndex = pClient->getIndex();
				getSubnetNode().set(pPkt->getDestSubnet(), pPkt->getDestNode());
				break;
			}			
			case LT_AF_GROUP_ACK:
			{
				LtSubnetNodeClient* pClient = (LtSubnetNodeClient*) pPkt->getDestClient();
				domainIndex = pClient->getIndex();
				getSubnetNode().set(pPkt->getDestSubnet(), pPkt->getDestNode());
				setGroup(pPkt->getDestGroup());
				setGroupMember(pPkt->getDestMember());
				break;
			}
			case LT_AF_GROUP:
			{
				LtSubnetNodeClient* pClient = (LtSubnetNodeClient*) pPkt->getDestClient();
				domainIndex = pClient->getIndex();
				setGroup(pPkt->getDestGroup());
				break;
			}
			default:
				break;	// nothing
		}
		LtDomainConfiguration dc(domainIndex, pPkt->getDomain(), null, pPkt->getSourceNode().getSubnet(), pPkt->getSourceNode().getNode());
		setDomainConfiguration(dc);
	}

	// Set service type
	switch (pPkt->getServiceType())
	{
	case LT_REMMSG_ACKD:
		setServiceType(LT_ACKD);
		break;
	case LT_REMMSG_REQUEST:
		setServiceType(LT_REQUEST);
		break;
	default:
		setServiceType(pPkt->getServiceType());
		break;
	}
}

void LtApduIn::package(LtBlob *pBlob) 
{
    LtApdu::package(pBlob);
    LtIncomingAddress::package(pBlob);

#if PRODUCT_IS(VNISTACK)
	pBlob->package(&m_timeOfDay, sizeof(m_timeOfDay));
	pBlob->package(&m_highResTimeStamp);
#endif

	pBlob->package(&m_nL2PacketType);
	pBlob->package(&m_bIsValidL2Packet);		

}

void LtApduIn::addNvIndex(int nvIndex, int nvIncarnationNumber)
{
	if (getNvIndex() != -1 &&
		getNvIndex() != nvIndex)
	{
		// Have a new NV index to add.  Need to use a vector.  
		if (m_pNvIds == null)
		{
			m_pNvIds = new LtTypedVector<LtNvId>();
			m_pNvIds->addElement(new LtNvId(getNvIndex(), getNvIncarnationNumber()));
		}
		m_pNvIds->addElement(new LtNvId(nvIndex, nvIncarnationNumber));
	}
	else
	{
		setNvIndex(nvIndex);
        setNvIncarnationNumber(nvIncarnationNumber);
	}
}

boolean LtApduIn::getNextNvIndex(LtVectorPos &pos, int &nvIndex, int &nvIncarnationNumber)
{
	boolean bResult = false;
	if (m_pNvIds != null)
	{
        LtNvId *pLtNvId;
		bResult = m_pNvIds->getElement(pos, &pLtNvId);
        if (bResult && pLtNvId != NULL)
        {
            nvIndex = pLtNvId->getNvIndex();
            nvIncarnationNumber = pLtNvId->getNvIncarnationNumber();
        }
        else
        {
            bResult = FALSE;
        }
	}
	else
	{
		nvIndex = getNvIndex();
        nvIncarnationNumber = getNvIncarnationNumber();
		bResult = pos.getNext() == 0;
	}
	return bResult;
}

void LtApduIn::setLayer2Info(LtPktInfo *pL2Pkt)
{
	m_bIsValidL2Packet = pL2Pkt->getIsValidL2L3Packet();
	m_nL2PacketType =    pL2Pkt->getL2PacketType();
	setPhaseReading(pL2Pkt->getPhaseReading());
#if PRODUCT_IS(VNISTACK)
	pL2Pkt->getTimeStamp(m_timeOfDay, m_highResTimeStamp);
#endif
}

LtApduOut::LtApduOut() : LtApdu(true) 
{   
    m_pNvc = null; 
    m_downlinkEncryption = FALSE; 
    m_uplinkEncryption = FALSE;
}

LtApduOut::LtApduOut(LtApduIn* pRequest) : LtApdu(true, pRequest) 
{   
    m_pNvc = null; 
    m_downlinkEncryption = FALSE; 
    m_uplinkEncryption = FALSE;
}

LtApduOut::LtApduOut(LtBlob &blob) : LtApdu(blob), LtOutgoingAddress(blob) 
{
    LtStackBlob stackBlob(&blob);
    stackBlob.package(&m_override);
    blob.package(&m_downlinkEncryption);
    blob.package(&m_uplinkEncryption);
    m_pNvc = null;
}

void LtApduOut::package(LtBlob *pBlob) 
{
    LtApdu::package(pBlob);
    LtOutgoingAddress::package(pBlob);
    LtStackBlob stackBlob(pBlob);
    stackBlob.package(&m_override);
    pBlob->package(&m_downlinkEncryption);
    pBlob->package(&m_uplinkEncryption);
    m_pNvc = null;
}

void LtApduOut::setOverride(LtMsgOverride* pOverride)
{
	if (pOverride->hasOverrides())
	{
		m_override = *pOverride;
	}
}

LtMsgOverride* LtApduOut::getOverride()
{
	return &m_override;
}

void LtApduOut::applyOverride(boolean bLayer6)
{
	if (m_override.hasOverrides())
	{
		if (bLayer6)
		{
			if (m_override.getOptions().overrideService())
			{
				setServiceType(m_override.getService());
			}
			if (m_override.getOptions().overridePriority())
			{
				setPriority(m_override.getPriority());
			}
		}
		else
		{
			if (m_override.getOptions().overrideTxTimer())
			{
				setTxTimer(m_override.getTxTimer());
			}
			if (m_override.getOptions().overrideRptTimer())
			{
				setRptTimer(m_override.getRptTimer());
			}
			if (m_override.getOptions().overrideRetryCount())
			{
				setRetry(m_override.getRetryCount());
			}
		}
	}
}

void LtIncomingAddress::package(LtBlob *pBlob) 
{
    LtStackBlob stackBlob(pBlob);

    pBlob->PACKAGE_VALUE(m_addressFormat);
    stackBlob.package(&m_domain);			
	pBlob->package(&m_destNode);			
    pBlob->PACKAGE_VALUE(m_nGroup);			
    pBlob->PACKAGE_VALUE(m_nGroupMember);
}

void LtMsgIn::package(LtBlob *pBlob) 
{
    LtApduIn::package(pBlob);
}

void LtMsgOut::package(LtBlob *pBlob) 
{
    LtApduOut::package(pBlob);
}

void LtRefId::package(LtBlob *pBlob) 
{
    pBlob->PACKAGE_VALUE(m_nType);
    pBlob->package(&m_nIndex);
    pBlob->package(&m_nRefId);
}

void LtPacketControl::package(LtBlob *pBlob) 
{
    LtStackBlob stackBlob(pBlob);

	pBlob->PACKAGE_VALUE(m_bOrigPri);
    pBlob->PACKAGE_VALUE(m_bPriority);
    pBlob->PACKAGE_VALUE(m_bAuthenticated);
    pBlob->PACKAGE_VALUE(m_bTurnaround);
    pBlob->PACKAGE_VALUE(m_bSuccess);
    pBlob->PACKAGE_VALUE(m_bFailure);
    pBlob->PACKAGE_VALUE(m_bNullResponse);
    pBlob->PACKAGE_VALUE(m_bNoCompletion);
    pBlob->PACKAGE_VALUE(m_bAnyTxFailure);
    pBlob->PACKAGE_VALUE(m_bAnyValidResponse);
    pBlob->PACKAGE_VALUE(m_bProxy);
    pBlob->PACKAGE_VALUE(m_nAlternatePath);
    pBlob->PACKAGE_VALUE(m_nDeltaBacklog);
    pBlob->PACKAGE_VALUE(m_nAddressIndex); 
    pBlob->PACKAGE_VALUE(m_nNvIndex);
    pBlob->PACKAGE_VALUE(m_bRespondOnFlexDomain);
    pBlob->PACKAGE_VALUE(m_bZeroSync);
    pBlob->PACKAGE_VALUE(m_bAttenuate);
    stackBlob.package(&m_refId);
    pBlob->PACKAGE_VALUE(m_serviceType);
    pBlob->PACKAGE_VALUE(m_nProxyCount);
}

bool LtApduIn::getSsi(byte& reg1, byte& reg2)
{
	if (m_bSsiValid)
	{
		reg1 = m_nSsiReg1;
		reg2 = m_nSsiReg2;
	}
	return m_bSsiValid;
}

void LtApduIn::setSsi(bool isValid, byte reg1, byte reg2)
{
	m_bSsiValid = isValid;
	m_nSsiReg1 = reg1;
	m_nSsiReg2 = reg2;
}

void LtRespIn::package(LtBlob *pBlob) 
{
    LtApduIn::package(pBlob);
}

void LtRespOut::package(LtBlob *pBlob) 
{
    LtApduOut::package(pBlob);
}
