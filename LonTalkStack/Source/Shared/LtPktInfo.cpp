//
// LtPktInfo.cpp
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

#include "LtaDefine.h"
#if PRODUCT_IS(VNISTACK)
#include <windows.h>
#include "Counter64.h"
using namespace C64;
#endif

#include "LtRouter.h"
#include "LtCUtil.h"

#define CRC_LENGTH 2

int LtPktData::hashCode()
{
	int hc = 0;
	// Incorporating domain takes extra cycles so we skip it.  In multiple
	// domain system, two matching addresses in different domains fall in same
	// bucket. 
	switch (getAddressFormat())
	{
	case LT_AF_GROUP:
		hc = getDestGroup();
		break;
	case LT_AF_GROUP_ACK:
		hc = getDestGroup();
		break;
	case LT_AF_SUBNET_NODE:
		hc = (getDestSubnet() << 8) + getDestNode();
		break;
	case LT_AF_BROADCAST:
		hc = getDestSubnet();
		break;
	case LT_AF_UNIQUE_ID:
		hc = getUniqueId().hashCode();
		break;
	default:
		break;	// nothing
	}
	hc += getSourceNode().hashCode();
	return hc;
}

void LtPktData::setIncomingSicbData(boolean bIsValidL2Packet, byte l2PacketType)
{
#if PRODUCT_IS(VNISTACK)
	_ftime(&m_timeOfDay);
    static COUNTER32 c32(Counter64::C64_MS);    // 32-bit ms high-precision counter (with internal 64-bit resolution)
	m_highResTimeStamp = c32.Count();
#endif

	m_bIsValidL2L3Packet = bIsValidL2Packet;
	m_nL2PacketType = l2PacketType;
}

bool LtPktData::getSsi(byte& reg1, byte& reg2)
{
	if (m_bSsiValid)
	{
		reg1 = m_nSsiReg1;
		reg2 = m_nSsiReg2;
	}
	return m_bSsiValid;
}

void LtPktData::setSsi(bool isValid, byte reg1=0, byte reg2=0)
{
	m_bSsiValid = isValid;
	m_nSsiReg1 = reg1;
	m_nSsiReg2 = reg2;
}

void LtPktInfo::initPacket()
{
	m_parseState = LT_NONE; 
	m_pData = null; 
	m_bCrcFixup = false;
	m_bRouteMultiple = false;
#if PRODUCT_IS(ILON)
	m_bRouteMultipleNoXmit = false;
#endif
	m_pSourceClient = null;
	// zero these since they direct packets returning to a destination
	m_ipSrcAddr = 0;
	m_ipSrcPort = 0;
	setIgnoreAuthentication( false );
	setCrumb( NULL );
	setIncomingSicbData(true, L2_PKT_TYPE_INCOMING);
}

// This function parses part of an incoming packet.  It looks
// at the layer 2 header, layer 3 header and addresses.
// 
LtErrorType LtPktInfo::parsePktLayers2and3()
{
    LtErrorType errorType = LT_NO_ERROR;
	byte* pPkt;
	int nLen;
	byte* pPktStart;
	int nDomLen;
	int nTemp;

	// Do nothing if packet has already been parsed.
	if (m_parseState != LT_NONE) return errorType;

	m_nLen = getMessageData(&m_pData);

	pPkt = m_pData;
	nLen = m_nLen;

	pPktStart = pPkt;

	// Set layer 2 fields.  Note that we don't care about the backlog field
	// for incoming packets.
	nTemp = *pPkt++;
	m_bPriority = (nTemp & 0x80) ? true : false;
	m_bAltPath = (nTemp & 0x40) ? true : false;
    m_nDeltaBacklog = nTemp & 0x3f;
    
	// Set layer 3 header fields.
	nTemp = *pPkt++;
	nDomLen = nTemp & 3;
	nTemp >>= 2;
	m_nAddressFormat = (LtAddressFormat) (nTemp & 3);	
	nTemp >>= 2;
	m_nEnclPdu = (LtPduType) (nTemp & 3);
	nTemp >>= 2;
	m_nVersion = nTemp & 3;

	// Set source subnet/node info
    nTemp = *pPkt++;
	m_subnetNode.set(nTemp, *pPkt & 0x7f);

	// Adjust address format based on MSB of node.
	if (m_nAddressFormat == LT_AF_SUBNET_NODE && (*pPkt & 0x80) == 0)
	{
		m_nAddressFormat = LT_AF_GROUP_ACK;
	}

	// Set destination address
	pPkt++;
	switch (m_nAddressFormat)	
	{
		case LT_AF_BROADCAST:
			m_nDestSubnet = *pPkt++;
			break;
		case LT_AF_GROUP:
			m_nGroup = *pPkt++;
			break;
		case LT_AF_GROUP_ACK:
		case LT_AF_SUBNET_NODE:
			m_nDestSubnet = *pPkt++;
			m_nDestNode = *pPkt++ & 0x7f;
			if (m_nAddressFormat == LT_AF_GROUP_ACK)
			{
				m_nGroup = *pPkt++;
				m_nMember = *pPkt++;
			}
			break;
		case LT_AF_UNIQUE_ID:
			m_nDestSubnet = *pPkt++;
			m_uniqueId.set(pPkt);
			pPkt += 6;
			break;
		default:
			break;	// nothing
	}

	// Finally, do the domain
	if (nDomLen == 2)
	{
		nDomLen = 3;
	} 
	else if (nDomLen == 3)
	{
		nDomLen = 6;
	}
	m_domain.set(pPkt, nDomLen);
	pPkt += nDomLen;

	// Save packet pointer and length.  Subtract two for the CRC too.
	nLen -= (pPkt - pPktStart) + 2;
	if (nLen <= 0)
	{
		errorType = LT_UNKNOWN_PDU;
		m_nL2PacketType = L2_PKT_TYPE_PACKET_TOO_SHORT;
	}

    if (!isValidVersion())
    {
        errorType = LT_UNKNOWN_PDU;
    }

	if (errorType == LT_NO_ERROR)
	{
		m_pData = pPkt;
		m_nLen = nLen;
	}
	else
	{
		m_bIsValidL2L3Packet = false;
	}

	m_parseState = LT_L2;

    return errorType;
}

LtServiceType LtPktInfo::nToServiceType[LT_AUTHPDU+1][LT_NUM_PDU_TYPE] =
{
	// TPDU  
	{ LT_ACKD,	    LT_UNACKD_RPT, LT_ACK,       LT_UNKNOWN, LT_REMINDER_ACKD,   LT_REMMSG_ACKD },

	// SPDU
	{ LT_REQUEST,   LT_UNKNOWN,    LT_RESPONSE,  LT_UNKNOWN, LT_REMINDER_REQUEST,LT_REMMSG_REQUEST },

	// AUTHPDU
	{ LT_CHALLENGE, LT_CHALLENGE_OMA,  LT_REPLY,  LT_REPLY_OMA, LT_UNKNOWN,         LT_UNKNOWN },
};

LtTypeConversion LtPktInfo::nFromServiceType[LT_SERVICE_TYPES] =
{
	{ LT_TPDU, 0 },				// LT_ACKD
	{ LT_TPDU, 1 },				// LT_UNACKD_RPT			
	{ LT_APDU, 0 },				// LT_UNACKD
	{ LT_SPDU, 0 },				// LT_REQUEST
	{ LT_SPDU, 2 },				// LT_RESPONSE
	{ LT_TPDU, 4 },				// LT_REMINDER_ACKD
	{ LT_SPDU, 4 },				// LT_REMINDER_REQUEST
	{ LT_TPDU, 5 },				// LT_REMMSG_ACKD
	{ LT_SPDU, 5 },				// LT_REMMSG_REQUEST
	{ LT_AUTHPDU, 0 },			// LT_CHALLENGE
	{ LT_AUTHPDU, 2 },			// LT_REPLY
    { LT_TPDU, 2 },             // LT_ACK
	{ LT_AUTHPDU, 1 },			// LT_CHALLENGE_OMA
	{ LT_AUTHPDU, 3 },			// LT_REPLY_OMA
};

// This function parses the layer 4 header of the packet.  It
// assumes that "parsePkt" has already been called.  This is done
// in two stages to avoid unnecessary parsing of layer 4 when
// the packet is to be discarded.
LtErrorType LtPktInfo::parsePktLayer4()
{
	byte* pPkt;
	int nLen;
	LtErrorType errorType = LT_NO_ERROR;

	if (m_parseState == LT_L4) return errorType;

	pPkt = m_pData;
	nLen = m_nLen;

	if (m_nEnclPdu == LT_APDU)
	{
		m_nServiceType = LT_UNACKD;
	}
	else
	{
		int nTemp = *pPkt++;
		int type;

		nLen--;
        if (m_nVersion == LT_L2_PKT_VER_LS_ENHANCED_MODE)
        {
            m_nTxNumber = ((nTemp & 0x0f) << 8) | *pPkt++;
            nLen--;
        }
        else
        {
		    m_nTxNumber = (nTemp & 0x0f);
        }
		nTemp >>= 4;
		if (getEnclPdu() == LT_AUTHPDU)
		{
			type = (nTemp & 3);
			nTemp >>= 2;
			m_origFmt = LtAddressFormat (nTemp & 3); 
			m_nGroup = pPkt[LT_CHALLENGE_LENGTH];
		}
		else
		{
			type = (nTemp & 7);
			nTemp >>= 3;
			m_bAuth = (nTemp & 1);
			m_origFmt = m_nAddressFormat;
		}
		if (type >= LT_NUM_PDU_TYPE)
		{
			m_nServiceType = LT_UNKNOWN;
			errorType = LT_UNKNOWN_PDU;
		}
		else
		{
			m_nServiceType = nToServiceType[m_nEnclPdu][type];

            // EPR 23157
            // If this is a reminder, peek at the reminder length.
            // If the length is > 8, then this is a corrupt packet - 
            // discard it.
            if (ANY_REMINDER(m_nServiceType) &&
                nLen > 0 && *pPkt > 8)
            {   
			    m_nServiceType = LT_UNKNOWN;
			    errorType = LT_UNKNOWN_PDU;
            }
		}
	}
	if (nLen < 0 ||
		(nLen == 0 && m_nServiceType != LT_ACK))
	{
        m_nServiceType = LT_UNKNOWN;
		errorType = LT_UNKNOWN_PDU;
	}
	if (errorType == LT_NO_ERROR)
	{
		m_pData = pPkt;
		m_nLen = nLen;
        m_parseState = LT_L4;
	}

	return errorType;
}

// This function is called to construct the packet once the various fields have
// been set in the structure.  Returns true if the build succeeds.  
LtErrorType LtPktInfo::buildPkt(boolean bLayer2)
{
	byte* pPkt;
	int nLen;
	LtErrorType errorType = LT_NO_ERROR;

	if (bLayer2)
	{
		setMessageNoRef(m_pData, m_nLen);
	}
	else
	{
		if (m_pData == null)
		{
			// No APDU, as in an ACK
			setData(null, 0);
		}

		pPkt = m_pData;
		nLen = m_nLen;

		LtTypeConversion &typeConversion = nFromServiceType[m_nServiceType];
		m_nEnclPdu = typeConversion.m_nEnclPdu;

		if (m_nEnclPdu != LT_APDU)
		{
			if (m_nServiceType >= LT_SERVICE_TYPES)
			{
				errorType = LT_INVALID_PARAMETER;
			}
			else
			{
				int type;
				int nHdr;
                if (m_nVersion == LT_L2_PKT_VER_LS_ENHANCED_MODE)
                {
                    nHdr = (m_nTxNumber >> 8) & 0xf;
                }
                else
                {   // Assume LT_L2_PKT_VER_LS_LEGACY_MODE
                    nHdr = m_nTxNumber & 0xf;
                }

				if (m_nServiceType == LT_CHALLENGE || m_nServiceType == LT_CHALLENGE_OMA ||
                    m_nServiceType == LT_REPLY || m_nServiceType == LT_REPLY_OMA)
				{
					if (m_origFmt == LT_AF_GROUP ||
						m_origFmt == LT_AF_BROADCAST)
					{
						pPkt[LT_CHALLENGE_LENGTH] = m_nGroup;
					}
					else
					{
						// We don't send the extra byte if not group mode.  This
						// assumes that the upper layers always allocate the
						// extra space.
						nLen--;
					}
				}
				else
				{
					// Original format is same as address format.
					m_origFmt = m_nAddressFormat;
				}
				// First determine the PDU type based on the service type
				LtTypeConversion &typeConversion = nFromServiceType[m_nServiceType];
				type = typeConversion.m_nType;
				m_nEnclPdu = typeConversion.m_nEnclPdu;

                if (m_nVersion == LT_L2_PKT_VER_LS_ENHANCED_MODE)
                {
                    // Include least significant 8 bits of transaction nnumber
                    *--pPkt = m_nTxNumber & 0xff;
                    nLen++;
                }
				nHdr |= type << 4;
				nHdr |= (m_nEnclPdu == LT_AUTHPDU) ? (m_origFmt << 6) : (m_bAuth << 7);
				*--pPkt = nHdr;
				nLen++;
			}
		}
		// Now set the address information
		if (errorType == LT_NO_ERROR)
		{
			// First set the domain
			int nDomLen = m_domain.getLength();
			int nMask = 0x80;
			int fmt = m_nAddressFormat;

			byte* pPktStart = pPkt;

			if (nDomLen != 0)
			{
				pPkt -= nDomLen;
				m_domain.get(pPkt);
			}
			// Convert domain length to packet form
			if (nDomLen == 3)
			{
				nDomLen = 2;
			}
			else if (nDomLen == 6)
			{
				nDomLen = 3;
			}

			// Set the destination address
			switch (m_nAddressFormat)
			{
			case LT_AF_BROADCAST:
				*--pPkt = m_nDestSubnet;
				break;
			case LT_AF_GROUP:
				*--pPkt = m_nGroup;
				break;
			case LT_AF_GROUP_ACK:
				*--pPkt = m_nMember;
				*--pPkt = m_nGroup;
				nMask = 0;
				fmt = LT_AF_SUBNET_NODE;
				// NO BREAK
			case LT_AF_SUBNET_NODE:
				*--pPkt = m_nDestNode | 0x80;
				*--pPkt = m_nDestSubnet;
				break;
			case LT_AF_UNIQUE_ID:
				pPkt -= m_uniqueId.getLength();
				m_uniqueId.get(pPkt);
				*--pPkt = m_nDestSubnet;
				break;
			default:
				break;	// nothing
			}

			// Set the source address
			*--pPkt = m_subnetNode.getNode() | nMask;
			*--pPkt = m_subnetNode.getSubnet();

			// Set the layer 3 header
			*--pPkt = (m_nVersion << 6) | 
					  (m_nEnclPdu << 4) |
					  (fmt << 2) |
					  (nDomLen);

			// Set the layer 2 header
			*--pPkt = (m_bPriority << 7) |
					  (m_bAltPath << 6) |
					  (m_nDeltaBacklog);

			// Adjust the length.  
			nLen += (pPktStart - pPkt) + CRC_LENGTH;

			setMessageNoRef(pPkt, nLen);

			setIncomingSicbData(true, L2_PKT_TYPE_INCOMING);

		}

		m_parseState = LT_L4;

        if (errorType == LT_NO_ERROR && nLen > MAX_LINK_PKT_SIZE)
        {   // This is too big to send.
            errorType = LT_NET_BUF_TOO_SMALL;
        }
	}

	// Mark packet as needing CRC fixup
	setCrcFixup(true);

	return errorType;
}

void LtPktInfo::dump()
{
    printf("\tdomain: ");
    getDomain().dump();
	printf("\tformat: ");
    switch (getAddressFormat())
    {
    case LT_AF_BROADCAST:
		printf("LT_AF_BROADCAST\n");
        printf("\tsubnet: %d\n", getDestSubnet());
        break;
    case LT_AF_GROUP_ACK:
		printf("LT_AF_GROUP_ACK\n");
        printf("\tsubnet: %d\n", getDestSubnet());
        printf("\tnode: %d\n", getDestNode());
		printf("\tgroup: %d\n", getDestGroup());
		printf("\tmember: %d\n", getDestMember());
        break;
    case LT_AF_SUBNET_NODE:
		printf("LT_AF_SUBNET_NODE\n");
        printf("\tsubnet: %d\n", getDestSubnet());
        printf("\tnode: %d\n", getDestNode());
        break;
    case LT_AF_GROUP:
		printf("LT_AF_GROUP\n");
        printf("\tgroup: %d\n", getDestGroup());
        break;
    case LT_AF_UNIQUE_ID:
		printf("LT_AF_UNIQUE_ID\n");
        printf("\tunique id: ");
        getUniqueId().dump();
        break;
	default:
		break;	// nothing
    }
}

void LtPktInfo::getChallengeReply(byte* pData)
{
    memcpy(pData, getData(), LT_CHALLENGE_LENGTH);
}

void LtPktInfo::setChallengeReply(byte* pData)
{
	// Challenge data is funky in that part of the header follows the challenge.
	// So, add one to length which is adjusted later is necessary.  Same for reply.
	setData(pData, LT_CHALLENGE_LENGTH + 1);
}

void LtPktInfo::resetApdu()
{
    m_pData = null;
}

LtErrorType LtPktInfo::setData(byte* pData, int nLen)
{
    resetApdu();
    return addData(pData, nLen);
}

LtErrorType LtPktInfo::addData(byte* pData, int nLen)
{
	LtErrorType errorType = LT_NO_ERROR;
	byte* pStart;
	int nMax = getMessageData(&pStart);

	if (m_pData == null)
	{
		// First time adding to packet.  Always make room for the CRC.
		m_pData = pStart + nMax - CRC_LENGTH;
		m_nLen = 0;
	}
	if (m_nLen + nLen > nMax)
	{
		errorType = LT_INVALID_PARAMETER;
	}
	else if (nLen)
	{
		m_pData -= nLen;
		m_nLen += nLen;
		memcpy(m_pData, pData, nLen);
	}
	return errorType;
}

void LtPktInfo::removeData(byte* pData, int nLen)
{
	if (nLen && (nLen <= m_nLen))
	{
		memcpy(pData, m_pData, nLen);
		m_nLen -= nLen;
		m_pData += nLen;
	}
}

void LtPktInfo::domainFixup(LtDomain& domain, int nSubnet)
{
	// Since we are going to rebuild the packet, make sure it is fully parsed first.
	parsePktLayer4();

	byte* pData;
	
	getMessageData(&pData);	// FB: is this needed?

	// This routine requires that the packet data be at the end of the data area so
	// that we grow the domain downwards.  Unfortunately, there is no way for us
	// to check this.  
	
	// This is used to patch up both the domain and the source subnet in the packet.
	// Both the parsed and raw data are fixed up.
	m_subnetNode.setSubnet(nSubnet);
	m_domain = domain;
	buildPkt(false);
}

void LtPktInfo::subnetFixup(int nSubnet)
{
	byte* pData;
	// This is used to patch the source subnet in the packet.  Both
	// the parsed and raw data are fixed up.
	m_subnetNode.setSubnet(nSubnet);
	getMessageData(&pData);
	pData[2] = nSubnet;

	// Mark packet as needing CRC fixup.
	setCrcFixup(true);
}

void LtPktInfo::setCrc()
{
	if (getCrcFixup())
	{
		// Generate a valid CRC for this packet
		byte* pData;
		int nLen = getMessageData(&pData);

		LtCRC16(pData, nLen-CRC_LENGTH);

		// Don't need to do this again.
		setCrcFixup(false);
	}
}
