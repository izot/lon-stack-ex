//
// LtMipApp.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtMipApp.cpp#2 $
//

#undef TEST_MIP_APP

#ifdef TEST_MIP_APP
#include <windows.h>
#endif

#include <assert.h>
#include "LtMipApp.h"
#include "LonLink.h"

// By default 256 transmit transactions are supported
int LtMipApp::s_maxL5MIPTxTransactions = 256;
int LtMipApp::s_maxL5MIPRxTransactions = MAXIMUM_L5MIP_RECEIVE_TRANSACTIONS;

//
// Define 16 receive transactions and 32 transmit transactions.  These limits
// are based on a 4 bit rcvtx field for incoming SICBs and a 4 bit tag
// field for outgoing SICBs (*2 for priority/non-priority).
//
LtMipApp::LtMipApp(int appIndex, LtLogicalChannel* pChannel, const char *persistencePath, int nAddressTableCount) : 
    LtaBase("L5Mip", 110, 64*1024, pChannel,
		s_maxL5MIPRxTransactions, s_maxL5MIPTxTransactions)
{
	LtProgramId pid ((byte*)"L5Mip", true);

    if (persistencePath != NULL)
    {
        setPersistencePath(persistencePath);
    }
    else
    {
    	char path[MAX_PATH];
        LtPlatform::getPersistPath( path, sizeof(path) );
        setPersistencePath(path);
    }
	setServiceLedImpact(false);
	memset(&m_msgs, 0, sizeof(m_msgs));
	m_instance = 0;
	m_currentNssMipMode = NSS_TRANSPARENT_MODE;
	m_bInitialFlushState = true;
	setMessageOutMaximum(32, 32);
	registerApplication(appIndex, this, 2, nAddressTableCount, 0, 0, 0, 0, 0, 0, 0, 0, 1, null, &pid);
	setReceiveNsaBroadcasts(true);
}

LtMipApp::~LtMipApp()
{
	stopApp();
}

void LtMipApp::applicationEventIsPending(void)
{
	m_signal.signal();
}


void LtMipApp::run(void)
{
	reset();

	// First thing any good MIP does is send an uplink reset event.
	while (!appShutdown())
	{
		// Put in a one second timeout.  Waiting forever should be OK
		// but just to be safe we will periodically check for events too.
		m_signal.wait(msToTicks(1000));
		processApplicationEvents();
	}
}

LtErrorType LtMipApp::handleNetworkManagement(LtApduIn* pApdu)
{
	// The MIP application bypasses the default network management processing for most
	// network managment commands.
	LtErrorType err = LT_APP_MESSAGE;
	int code = pApdu->getCode();
    byte data[MAX_APDU_SIZE];
	memset(data, 0, sizeof(data));

    data[0] = (byte) pApdu->getNmCode(true);
	int len = 0;

	if (code == LT_NM_ESCAPE && pApdu->getDataLength() >= 2)
	{
		int subsub = pApdu->getData(LT_NM_ESCAPE_SUBSUBCOMMAND_OS);

		// There are two things that need to be handled here.  First, the product query and
		// second the legacy NSI commands.
		switch (pApdu->getData(LT_NM_ESCAPE_SUBCOMMAND_OS))
		{
			case LT_NM_ESCAPE_GENERAL:
			{
				if (subsub == LT_NM_ESCAPE_GENERAL_PRODUCT_QUERY)
				{
					// Send a product query response
					data[1] = 3;	// PCNSI product - required by NSS
					data[2] = 4;	// PCNSI/iLON model (as defined in firmware file n_nmgr.inc &&
									// NSS file ni_mgmt.h)
					data[3] = MIPAPP_VERSION;	// Version
					data[5] = getXcvrId();	// Transceiver ID
					len = 6;
				}
				else
				{
					// Send a failure response.
					data[0] = (byte) pApdu->getNmCode(false);;
					len = 1;
				}
				err = LT_NO_ERROR;
				break;
			}

			case LT_NM_ESCAPE_NSS:
			{
				// We handle certain NSS commands coming locally from the host
				if (pApdu->getAddressFormat() == LT_AF_TURNAROUND)
				{
					err = LT_NO_ERROR;
					switch (subsub)
					{
						case NM_NS_CHANGE_MIP_MODE:
						{
							m_currentNssMipMode = pApdu->getData(LT_NM_ESCAPE_DATA);
							data[0] = 0;
							len = 1;
							break;
						}

						case NM_NS_NSI_RESET_REQUIRED:
						{
							initiateReset();
							break;  /* no response */
						}

						case NM_NS_SET_NSS_DATA:
						{
							int length = pApdu->getDataLength()-LT_NM_ESCAPE_DATA;
							length = min(length, (int)sizeof(m_myNssData));
							pApdu->getData(m_myNssData, LT_NM_ESCAPE_DATA, length);
							len = 1;
							break;
						}

						case NM_NS_GET_NSS_DATA:
						{
							len = sizeof(m_myNssData) + 1;
							memcpy(&data[1], m_myNssData, len-1);
							break;
						}

						case NM_NS_GET_MIP_MODE:
						{
							data[0] = 0;
							data[1] = m_currentNssMipMode;
							len = 2;
							break;
						}

						case NM_NS_QUERY_MIP_EEVARS:
						{
							NmNsMipEevars ee;
							int length = sizeof(ee);

							memset(&ee, 0, length);
							ee.mipDeviceType = 12;
							ee.sizeofMipEevars = length;
							ee.version = MIPAPP_VERSION;

							len = length+1;
							memcpy(&data[1], (byte*)&ee, length);
							break;
						}

						default:
						{
							assert(0);
							err = LT_APP_MESSAGE;
							break;
						}
					}
				}
				break;
			}

			default:
				break;	// nothing
		}
    }

    if (len != 0 && pApdu->isRequest()) 
	{
		sendResponse((LtMsgIn*) pApdu, data, len);
    }

	return err;
}

void LtMipApp::receiveSicb(LtSicb* pSicb, OpSignal* pSsi)
{
	// Set the SICB's length field and then pass it on.
	pSicb->len = sizeof(LtSicb) - 2 + pSicb->dlen-1;
	receive(pSicb, pSsi);
}

void LtMipApp::incomingSicb(int queue, int tag, LtApduIn* pApdu)
{
	LtSicbFull ltSicb;
	LtSicb* pSicb = (LtSicb*)&ltSicb;

	if ((pApdu->getCode() >= 0x00) && (pApdu->getCode() <= 0x4F))
		isApplicationMessage = true;
	else isApplicationMessage = false;

	if (pApdu->isNetworkVariableMsg() == TRUE)
		isNetworkVariableMessage = true;
	else isNetworkVariableMessage = false;

	if (pApdu->getAddressFormat() == LT_AF_BROADCAST)
		isBroadCastMessage = true; 
	else isBroadCastMessage = false;

	memset(pSicb, 0, sizeof(*pSicb));

	// Set the incoming address
	pSicb->addr.in.fmt = pApdu->getAddressFormat();
	if (pApdu->getDomainConfiguration().isFlexDomain())
	{
		pSicb->addr.in.dmn = 1; // Should not matter, but set domain to 1 for compatibility
        pSicb->addr.in.flex = 1;
	}
	else
	{
		LtDomainConfiguration dc;
		dc = pApdu->getDomainConfiguration();
		// BobW 04/22/04 EPR 32785 - changed definition of dmn to exclude flex bit.
        pSicb->addr.in.flex = 0;
		pSicb->addr.in.dmn = dc.getIndex();
		pSicb->addr.in.sub = dc.getSubnet();
		pSicb->addr.in.nod = dc.getNode();
	}
	pSicb->addr.in.grp = 1;	// Assume not a group ack.
	pSicb->addr.in.addr.sn.sub = pApdu->getSubnetNode().getSubnet();
	pSicb->addr.in.addr.sn.nod = pApdu->getSubnetNode().getNode();
	pSicb->addr.in.addr.sn.mbo = 1;
	switch (pApdu->getAddressFormat())
	{
		case LT_AF_GROUP:
			pSicb->addr.in.addr.grp = pApdu->getGroup();
			break;
		case LT_AF_GROUP_ACK:
			pSicb->addr.in.addr.ga.sub = pApdu->getSubnetNode().getSubnet();
			pSicb->addr.in.addr.ga.nod = 0x80 | pApdu->getSubnetNode().getNode();
			pSicb->addr.in.addr.ga.grp = pApdu->getGroup();
			pSicb->addr.in.addr.ga.mem = pApdu->getMember();
			pSicb->addr.in.addr.ga.mbo = 1;
			pSicb->addr.in.grp = 0;
			pSicb->addr.in.fmt = LT_AF_SUBNET_NODE;
			break;
		default:
			break;	// nothing
	}

	// Apply wink address restoration (see sendNm())
	if (pApdu->getCode() == LT_WINK && 
		pApdu->getDataLength() == 0 &&
		pApdu->getAddressFormat() == LT_AF_TURNAROUND)
	{
		memcpy(&pSicb->addr.out, &m_winksicb.addr.out, sizeof(pSicb->addr.out));
	}

	pSicb->cmd = MI_COMM;
	pSicb->que = queue;
	pSicb->svc = pApdu->getServiceType();
	if (queue == MI_RESPONSE)
	{
		pSicb->svc = LT_REQUEST;
		pSicb->rsp = 1;
	}
	pSicb->tag = tag;
	pSicb->pri = pApdu->getPriority();
	pSicb->auth = pApdu->getAuthenticated();
	int apduLength = pApdu->getLength();
	if (apduLength <= MAX_APDU_SIZE)
	{
		memcpy(pSicb->data, pApdu->getCodeAndData(), apduLength);
		pSicb->dlen = apduLength;
	}
	// Get the signal strength information for this SICB.  
	m_opSignal.status = 1;
	if (pApdu->getSsi(m_opSignal.reg1, m_opSignal.reg2))
	{
		// The info is there.  So, set the status to indicate success and copy reg2 to reg3
		m_opSignal.reg3 = m_opSignal.reg2;
	}
	else
	{
		// This can happen if the MIP doesn't support SSI mode.  Just say we have a great signal.
		byte data[LT_NUM_REGS];
		LonLink::setPerfectXcvrReg(data, sizeof(data), pApdu->getAlternatePath() == LT_ALT_PATH);
		m_opSignal.reg1 = data[LT_PLC_REG_STATUS];
		m_opSignal.reg2 = data[LT_PLC_REG_PRIMARY];
		m_opSignal.reg3 = data[LT_PLC_REG_SECONDARY];
	}
	receiveSicb(pSicb, &m_opSignal);
}

void LtMipApp::msgArrives(LtMsgIn* pMsgIn)
{
	// Convert the message to SICB format and deliver to the application.  The application
	// must copy the contents of the SICB.
	incomingSicb(MI_INCOMING, getTag(pMsgIn), pMsgIn);
	if (!pMsgIn->isRequest())
	{
		release(pMsgIn);
	}
}

void LtMipApp::respArrives(LtMsgTag* pTag, LtRespIn* pResp)
{
	// Convert the response to SICB format and deliver to the application
	incomingSicb(MI_RESPONSE, pTag->getIndex(), pResp);
	release(pResp);
}

void LtMipApp::msgCompletes(LtMsgTag* pTag, boolean bSuccess)
{
	// Create a completion event.  Note that the application may assume
	// that the completion event contains the first 2 bytes of the APDU so
	// we extract these from the message tag.
	LtMipTag* pMipTag = (LtMipTag*)pTag;

	LtSicb* pSicb = pMipTag->getSicb();

	// Local NM commands don't generate completion events on a MIP
	if (pSicb->cmd != MI_NETMGMT)
	{
		pSicb->cmd = MI_COMM;
		pSicb->que = MI_RESPONSE;
		pSicb->dlen = 2;
		if (bSuccess)
		{
			pSicb->succ = 1;
		}
		else
		{
			pSicb->fail = 1;
		}
		receiveSicb(pSicb);
	}
	delete pMipTag;
}

LtErrorType LtMipApp::sendNm(LtSicb* pSicb)
{
	LtErrorType err = LT_NO_ERROR;

	// Make sure the RSP bit is set to zero.  Since NM messages are only requests and the Neuron ignores the RSP bit for 
	// such requests, we ignore the RSP bit to ensure the message is properly handled even if the app incorrectly sets the RSP bit.
	pSicb->rsp = 0;

	// Apply a filter for NSA relative writes.  These are implemented by the MIP application.
	int code = pSicb->data[0];
	LtWriteMemory* pWm = (LtWriteMemory*) &pSicb->data[1];
	if (code == LT_WRITE_MEMORY && pWm->mode == LT_ABSOLUTE && pWm->len == 0x01
			&& pWm->addresslo == 0xfd && pWm->addresshi == 0xf1)
	{
		// This is a LonTalk Error
		// Log this as a stack error
		setErrorLog(pWm->data[0]);

		// Send a passing response
		pSicb->data[0] = NMPASS(code);
		if (pSicb->svc == LT_REQUEST)
		{
			pSicb->cmd = MI_COMM;
			pSicb->que = MI_RESPONSE;
			pSicb->rsp = 1;
			pSicb->dlen = 1;
			receiveSicb(pSicb);
		}
	}
	else if (code == LT_WRITE_MEMORY && pWm->mode == LT_NSA_RELATIVE)
	{
		byte* pData = (byte*) &m_nsa;
		int offset = pWm->addresslo;
		int len = pWm->len;

		pSicb->data[0] = NMPASS(code);

		if (len+offset <= (int)sizeof(m_nsa))
		{
			memcpy(&pData[offset], pWm->data, len);
		}
		else
		{
			pSicb->data[0] = NMFAIL(code);
		}

		if (pSicb->svc == LT_REQUEST)
		{
			pSicb->cmd = MI_COMM;
			pSicb->que = MI_RESPONSE;
			pSicb->rsp = 1;
			pSicb->dlen = 1;
			receiveSicb(pSicb);
		}
	}
	else
	{
		// Apply a filter looking for winks.  These are used by the LON layer to resync to the MIP.
		// Unfortunately, the LON layer assumes that the explicit address area in the uplink wink
		// will match that in the downlink packet.  This won't be the case in this stack so we save
		// the explicit address and apply it later.
		if (pSicb->data[0] == LT_WINK && 
			pSicb->dlen == 1 &&
			pSicb->exp)
		{
			// Save outgoing data
			memcpy(&m_winksicb, pSicb, sizeof(m_winksicb));
		}
		err = sendMsg(pSicb);
	}
	return err;
}

LtErrorType LtMipApp::sendMsg(LtSicb* pSicb)
{
	LtErrorType err = LT_NO_ERROR;
	LtMsgOut* pMsg;

	if (pSicb->rsp)
	{
		LtMsgIn* pMsgIn = getMsgIn(pSicb->tag);
		if (pMsgIn != NULL)
		{
			sendResponse(pMsgIn, pSicb->data, pSicb->dlen);
			delete pMsgIn;
		}
	}
	else
	{
		if (pSicb->pri)
		{
			pMsg = msgAllocPriority();
		}
		else
		{
			pMsg = msgAlloc();
		}

		if (pMsg == NULL)
		{
			err = LT_NO_RESOURCES;
		}
		else
		{
			// Create a MIP tag corresponding to the SICB tag.  We malloc this rather than using
			// a static array of tags to cover the possibility of multiple outstanding transactions
			// on the same tag.
			int len;

			if (pSicb->cmd == MI_COMM)
			{
				int atype = pSicb->addr.out.type;

				if (atype & LT_ATTENUATE)
				{
					pMsg->setAttenuate(TRUE);
				}
				if (atype & LT_ZERO_SYNC)
				{
					pMsg->setZeroSync(TRUE);
				}
				if (atype & LT_OVERRIDE)
				{
					LtMsgOverrideOptions opt(LT_OVRD_AUTH_KEY);
					LtMsgOverride ovr(opt, LT_UNACKD, 0, 0, 0, 0, m_nsa.key[0], TRUE);
					pMsg->setOverride(&ovr);
				}

				pSicb->addr.out.type &= ~LT_TYPEMASK;

				if (pSicb->exp)
				{
					// Build the explicit address
					pMsg->fromLonTalk(&pSicb->addr.out.type, len, 1);

                    if (atype & LT_LONGTIME)
                    {
                        int encodedTxTimer = LtMisc::fromTxTimer(pMsg->getAddr().getTxTimer()) + 0x10;
                        int encodedRptTimer = LtMisc::fromTxTimer(pMsg->getAddr().getRptTimer()) + 0x10;

                        pMsg->getAddr().setTxTimer(LtMisc::toTxTimer(encodedTxTimer));
                        pMsg->getAddr().setRptTimer(LtMisc::toTxTimer(encodedRptTimer));
                    }
				}
			}
			else
			{
				// Only try once but give it lots of time
				pMsg->setRetry(0);
				pMsg->setTxTimer(8000);
				pMsg->setAddressType(LT_AT_LOCAL);
			}

			LtMipTag* pMsgTag = new LtMipTag(pSicb->tag, pSicb);
			pMsg->setTag(*pMsgTag);
			pMsg->setCodeAndData(pSicb->data, pSicb->dlen);
			pMsg->setAuthenticated(pSicb->auth);
			pMsg->setServiceType((LtServiceType)pSicb->svc);
			
			if (pSicb->pths)
			{
				// Path was specified
				pMsg->setPath(pSicb->pth ? LT_ALT_PATH : LT_NORMAL_PATH);
			}

			LtAppNodeStack::send(pMsg);
		}
	}
    return err;
}

LtErrorType LtMipApp::send(LtSicb* pSicb)
{
	LtErrorType err = LT_NO_ERROR;

	switch (pSicb->cmd)
	{
		case MI_COMM:
			err = sendMsg(pSicb);
			break;

		case MI_NETMGMT:
			err = sendNm(pSicb);
			break;

		case MI_RESET:
			initiateReset();
			break;

		case MI_FLUSH_CANCEL:
			m_bInitialFlushState = false;
			flushCancel();
			break;

		case MI_FLUSH:
			flush(FALSE);
			break;

		case MI_FLUSH_IGNORE:
			flush(TRUE);
			break;

		case MI_OFFLINE:
			// Response to a node mode offline request
			setOffline(TRUE);
			break;

		case MI_ONLINE:
			// Response to a node mode online request
			setOffline(FALSE);
			break;

		case MI_ESCAPE:
		{
			int cmd = pSicb->que;
			switch (cmd)
			{
				case MI_SSTATUS:
                    // First data byte has XID in upper 5 bits, !battery low, !batter failed in low order bits.
                    // Second data byte is MIP version number = 1.
					reportCommand(MI_ESCAPE, MI_SSTATUS, 2, getXcvrId()<<3 | 0x3, 0x1);
					break;

				case MI_ESCAPE_SERVICE:
					LtMipApp::sendServicePinMessage();
					break;
			}
			break;
		}

		case MI_MISC:
		{
			LdvMisc* pMisc = (LdvMisc*)pSicb;
			int cmd = pSicb->que;
			switch (cmd)
			{
				case MI_OPERATION:
				{
					// First data byte is an operation code. 
					switch (pMisc->subCommand)
					{
						case MI_OP_SIGNAL: 
						{
							// We always just report the current values.  Note that these values might
							// be stale (like for an SICB two packet times ago).  Therefore, it is
							// necessary that the receiver of this info use a rule that says if you receive
							// an SICB and then query signal strength, if you get another uplink frame
							// before getting the answer, assume the answer is stale.  The assumption is
							// that signal strength info is collected for low bandwidth media.  If this 
							// assumption changes, then we need to change the MIP protocol to send the
							// SSI data with the packet.
							m_opSignal.operation = MI_OP_SIGNAL;
							reportCommandData(MI_MISC, MI_OPERATION, sizeof(OpSignal), (byte*)&m_opSignal);
						}
					}
					break;
				}
				
				default:
				{
					// Ignore unknown commands
					break;
				}
			}
			break;
		}
		
		default:
			// Ignore unknown commands
			break;
	}

	return err;
}

//
// reportCommand
//
// Report a simple command.  "len" is 0 if there is no extra data, 1 to 2 otherwise.
void LtMipApp::reportCommand(int command, int subcommand, int len, int d0, int d1)
{
    // LtLocal is declared with only one data byte, but in fact can have additional data
    // bytes.  This routine can send up to 2 data bytes.
    byte buf[2];
    buf[0] = d0;
    buf[1] = d1;
    
    reportCommandData(command, subcommand, len, buf);
 }

//
// reportCommand
//
// Report a simple command.  "len" is the length of the extra data.
void LtMipApp::reportCommandData(int command, int subcommand, int len, byte* pData)
{
    // LtLocal is declared with only one data byte, but in fact can have additional data
    // bytes.  This routine can send up to 256 data bytes.
    char buf[LT_LOCAL_DATA_OFFSET + 256];
    LtLocal *pEvent = (LtLocal *)buf;

    assert((unsigned int)len < sizeof(buf)-LT_LOCAL_DATA_OFFSET);
    
    memset(buf, 0, sizeof(buf));
	pEvent->cmd = command;
	pEvent->subcmd = subcommand;
	pEvent->len = len;
	memcpy(&buf[LT_LOCAL_DATA_OFFSET], pData, len);
  
	receive((LtSicb*)pEvent);
}

void LtMipApp::reset(void)
{
	// Received a reset event from the stack
	const int len = 2;
	const int version = 3;
	reportCommand(MI_RESET, 0, len, 0, version);
}

void LtMipApp::flushCompletes(void)
{
	reportCommand(MI_FLUSH_COMPLETE);
}

void LtMipApp::sendServicePinMessage(void)
{
	if (!m_bInitialFlushState)
	{
		// Don't send service pins until initial flush state is cleared
		LtDeviceStack::sendServicePinMessage();
	}
}

int LtMipApp::getTag(LtMsgIn* pMsgIn)
{
	// Find an open tag.
	unsigned int maxAge = 0;
	int oldestTag = 0;
    int tag = 0;

	if (pMsgIn->getServiceType() == LT_REQUEST)
	{
		for (tag = 0; tag < MAXIMUM_L5MIP_SICB_TAGS; tag++)
		{
			// Unsigned math yields correct age even in case of instance wrap.
			unsigned int age = m_instance - m_msgs[tag].m_instance;
			if (m_msgs[tag].m_pMsgIn == NULL)
			{
				break;
			}
			else if (age > maxAge)
			{
				oldestTag = tag;
				maxAge = age;
			}
		}
		if (tag == MAXIMUM_L5MIP_SICB_TAGS)
		{
			// There was no room!  This can happen if the application doesn't respond
			// to requests.  It shouldn't be doing this but if it does we eventually
			// run out of space.  In this case we just use the oldest one.  OK, granted
			// in the pathological case of wrapping through the entire instance space
			// we may not strictly get the oldest one.  But this would only happen if 
			// failure to respond is extremely rare in which case any slot will do anyway.
			tag = oldestTag;
			delete m_msgs[tag].m_pMsgIn;
		}
		m_msgs[tag].m_pMsgIn = pMsgIn;
		m_msgs[tag].m_instance = m_instance++;
	}
	return tag;
}

LtMsgIn* LtMipApp::getMsgIn(int tag)
{
	LtMsgIn* pMsgIn = m_msgs[tag].m_pMsgIn;
	m_msgs[tag].m_pMsgIn = NULL;
	return pMsgIn;
}

//#define TEST_MIP_APP
#ifdef TEST_MIP_APP

// The following code can be used to test the above in the VNI environment as follows:
// 1. Define "TEST_MIP_APP" above.
// 2. Add this early in LtMip.cpp
/*
extern "C" LDVCode __declspec(dllexport) ldv_open_vni(pVoid id, pShort handle);
extern "C" LDVCode __declspec(dllexport) ldv_close_vni(short handle);
extern "C" LDVCode __declspec(dllexport) ldv_read_vni(short handle, pVoid msg_p, short len);
extern "C" LDVCode __declspec(dllexport) ldv_write_vni(short handle, pVoid msg_p, short len);

#define ldv_open ldv_open_vni
#define ldv_read ldv_read_vni
#define ldv_write ldv_write_vni
#define ldv_close ldv_close_vni
*/

// 3. Patch LtPlatform::getUniqueId() to return the Neuron's NID.  For example:
/*
#ifdef WIN32
	byte temp[6] = {0x00, 0x02, 0x47, 0x94, 0x89, 0x00};
    pUid->set(temp);
#else
*/
// 4. Include LtMipApp.cpp in the vnistack, rebuild and copy to \nss\bin.
// 5. Rebuild LON32 with references to ldv_xxx converted to ldv_xxx_vni for each LDV32 
//    API.  This can be done by just including the following in lon.h and adding vnistack.lib
//    to the LON32 DLL files.
/*
#define ldv_open ldv_open_vni
#define ldv_read ldv_read_vni
#define ldv_write ldv_write_vni
#define ldv_close ldv_close_vni
#define ldv_register_event ldv_register_event_vni
*/
//    and add these to ldvi.h
/*
#undef ldv_open
#undef ldv_read
#undef ldv_write
#undef ldv_close
#undef ldv_register_event
*/
//    and change end of ldv.h to
/*
LDVCode LDV_EXTERNAL_FN ldv_enable(unsigned long enable);
pStr LDV_EXTERNAL_FN ldv_get_version(void);
LDVCode ldv_xlate_device_name(const char *device_name, char *driver_name,
                              int *driver_name_len);

LDVCode _declspec(dllimport) ldv_open_vni(pVoid id, pShort handle);
LDVCode _declspec(dllimport) ldv_close_vni(short handle);
LDVCode _declspec(dllimport) ldv_read_vni(short handle, pVoid msg_p, short len);
LDVCode _declspec(dllimport) ldv_write_vni(short handle, pVoid msg_p, short len);
LDVCode _declspec(dllimport) ldv_register_event_vni(int handle, HANDLE hEvent);
*/
// 6. Copy lon32.dll to nss\bin.  Do not copy ldv32.dll - use the standard version.
// 7. Load LON<n> on a PCLTA with a layer 2 MIP and run nsstst32.exe.

enum
{
    LDV_OK=0,
    LDV_NOT_FOUND,
    LDV_ALREADY_OPEN,
    LDV_NOT_OPEN,
    LDV_DEVICE_ERR,
    LDV_INVALID_DEVICE_ID,
    LDV_NO_MSG_AVAIL,
    LDV_NO_BUFF_AVAIL,
    LDV_NO_RESOURCES,
    LDV_INVALID_BUF_LEN,
	LDV_NOT_ENABLED,
};

class LtMip : public LtMipApp
{
private:
	LtSicbFull sicb;
	boolean sicbInUse;

public:
	LtMip(LtLogicalChannel* pChannel) : LtMipApp(pChannel) 
	{
		sicbInUse = FALSE;
		setReceiveNsaBroadcasts(TRUE);
	}
	~LtMip() {}

	void receive(LtSicb* pSicb);
	LtSicb* getSicb() { return (LtSicb*)&sicb; }
	boolean getSicbInUse() { return sicbInUse; }
	void setSicbInUse(boolean bValue) { sicbInUse = bValue; }
};

typedef short LDVCode;
LtLtLogicalChannel* pChannel;
LtMip* pMip;
HANDLE eventHandle;
int openCount = 0;

void LtMip::receive(LtSicb* pSicb)
{
	// For this test, do a simple one deep buffer.
	while (sicbInUse) taskDelay(msToTicks(50));
	memcpy(pMip->getSicb(), pSicb, pSicb->len+2);
	sicbInUse = true;
	// Signal the app
	SetEvent(eventHandle);
}

extern "C" LDVCode LTA_EXTERNAL_FN ldv_open_vni(void* id, short* handle)
{
	if (++openCount == 1)
	{
		// Create the MIP application with its own channel.  Force the channel
		// to be layer 2 (otherwise we'll recurse trying to determining the MIP
		// type of this channel!
		pChannel = new LtLtLogicalChannel((const char*)id, NULL, NULL, true);
		pMip = new LtMip(pChannel);
		pMip->startMip();
	}
	*handle = 1;
	return LDV_OK;
}

extern "C" LDVCode LTA_EXTERNAL_FN ldv_close_vni(short handle)
{
	if (--openCount == 0)
	{
		delete pMip;
		delete pChannel;
	}
	return LDV_OK;
}

static int cnt = 0;
extern "C" LDVCode LTA_EXTERNAL_FN ldv_read_vni(short handle, void* msg_p, short len)
{
	LDVCode rtn = LDV_OK;
	if (pMip->getSicbInUse())
	{
		memcpy(msg_p, pMip->getSicb(), len);
		pMip->setSicbInUse(false);
		{
			char s[50];
			sprintf(s, "Read %02x/%02x%02x%02x succ:%d fail:%d\n", ((LtSicb*)msg_p)->data[0],
				((LtSicb*)msg_p)->data[1],
				((LtSicb*)msg_p)->data[2],
				((LtSicb*)msg_p)->data[3],
				((LtSicb*)msg_p)->succ,
				((LtSicb*)msg_p)->fail);
			OutputDebugString(s);
		}
	}
	else
	{
		rtn = LDV_NO_MSG_AVAIL;
	}
	return rtn;
}

extern "C" LDVCode LTA_EXTERNAL_FN ldv_write_vni(short handle, void* msg_p, short len)
{
	LDVCode rtn = LDV_OK;
	{
		char s[50];
		sprintf(s, "Send %02x/%02x%02x%02x svc:%d auth:%d\n", ((LtSicb*)msg_p)->data[0],
			((LtSicb*)msg_p)->data[1],
			((LtSicb*)msg_p)->data[2],
			((LtSicb*)msg_p)->data[3],
			((LtSicb*)msg_p)->svc,
			((LtSicb*)msg_p)->auth);
		OutputDebugString(s);
	}
	LtErrorType err = pMip->send((LtSicb*)msg_p);
	if (err != LT_NO_ERROR)
	{
		rtn = LDV_NO_RESOURCES;
	}
	return rtn;
}

extern "C" LDVCode LTA_EXTERNAL_FN ldv_register_event_vni(int handle, HANDLE hEvent)
{
	eventHandle = hEvent;
	return LDV_OK;
}

#endif
