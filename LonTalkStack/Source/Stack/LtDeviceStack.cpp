//
// LtDeviceStack.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtDeviceStack.cpp#5 $
//

#ifdef WIN32
#include <windows.h>
#endif

#include "LtStackInternal.h"
#if FEATURE_INCLUDED(L5MIP)
// No layer 5 MIP on i.LON
#include "LtLayer5Mip.h"
#endif
#include <vxlTarget.h>
#include <LtIpPlatform.h>
#if defined(ILON_PLATFORM) && defined(__VXWORKS__)
#include "echelon/iLon.h"
#endif
//
// Private Member Functions
//

LtStack* LtStack::create(LtLogicalChannel* pChannel, boolean bRouterSide, int nRxTx, int nTxTx)
{
	if (bRouterSide)
	{
		return new LtRouterStack(pChannel, nRxTx, nTxTx);
	}
	else
	{
		return new LtAppNodeStack(pChannel, nRxTx, nTxTx);
	}
}

LtStack* LtStack::createLayer2(LtLogicalChannel* pChannel)
{
	return new LtLayer2Stack(pChannel);
}

LtErrorType LtDeviceStack::handleNvUpdates(LtApduIn* pApdu) 
{
	LtErrorType err = LT_NO_ERROR;
	int nvIndex;
	LtVectorPos pos;
	LtNetworkVariable *pSourceNv = null;
	int sourceArrayIndex = 0;

	if (pApdu->getResponse())
	{
		pSourceNv = getNetworkVariable(pApdu->getNvIndex(), sourceArrayIndex);
	}

    int incarnation;
	while (pApdu->getNextNvIndex(pos, nvIndex, incarnation))
	{
		LtNetworkVariable* pNv;
		int arrayIndex;
	    byte* pData;

		pNv = getNetworkVariable(nvIndex, arrayIndex);
		if (pApdu->getResponse() && nvIndex == pApdu->getNvIndex())
		{
			pSourceNv = pNv;
			sourceArrayIndex = arrayIndex;
		}
		if (pNv == NULL)
		{   // NV does not exist.  If its an incoming request, return LT_INVALID_NV_INDEX,
            // (loggable), otherwise, its a response to a stale nv.
            err = pApdu->getResponse() ? LT_STALE_NV_INDEX : LT_INVALID_NV_INDEX;
		}
		else
		{
            LtNetworkVariableConfiguration nvc;
            LtErrorType err1 = getNetworkImage()->nvTable.getNv(nvIndex, &nvc);
            if (err1 == LT_NO_ERROR)
            {
                if (!nvc.incarnationMatches(incarnation))
		        {   // NV does not exist.  If its an incoming request, return LT_INVALID_NV_INDEX,
                    // (loggable), otherwise, its a response to a stale nv.
                    err1 = pApdu->getResponse() ? LT_STALE_NV_INDEX : LT_INVALID_NV_INDEX;
                }
            }
            if (err1 == LT_NO_ERROR)
            {
				int len;
				int maxNvLength;
				boolean changeable = pNv->getChangeableLength();
				if (changeable && getCurrentNvLengthFromApp(pNv, maxNvLength))
				{
					changeable = false;  // Use the length from app
				}
				else
				{
					maxNvLength = pNv->getLength();
				}
			
			    err1 = pApdu->getNvData(&pData, len, maxNvLength, changeable);
			    if (err1 == LT_NO_ERROR)
			    {
				    pNv->setLengthFromDevice(len);
				    pNv->setNvData(pData, len, arrayIndex);
				    lonApp->nvUpdateOccurs(pNv, arrayIndex, pSourceNv, sourceArrayIndex, (LtIncomingAddress*) pApdu);
			    }
            }
            if (err1 != LT_NO_ERROR)
            {
                err = err1;
            }
		}
	}
	return err;
}

NdNetworkVariable* LtDeviceStack::nvAdded(NdNetworkVariable* pNv)
{
	// Promote NdNetworkVariable to an LtNetworkVariable
	LtNetworkVariable* pNewNv = new LtNetworkVariable(pNv);

	// Register stack with this NV
	pNewNv->setStack(this);

	LtNetworkVariable* pNewerNv = lonApp->nvAdded(pNewNv);

	if (pNewerNv != NULL && pNewerNv != pNewNv)
	{
		delete pNewNv;
		pNewNv = pNewerNv;
	}
	return (NdNetworkVariable*) pNewNv;
}

void LtDeviceStack::nvChanged(NdNetworkVariable* pNv, NvChangeType type)
{
	lonApp->nvChanged((LtNetworkVariable*) pNv, type);
}

void LtDeviceStack::nvDeleted(NdNetworkVariable* pNv)
{
	// App will delete the object.
	lonApp->nvDeleted((LtNetworkVariable*) pNv);
}

boolean LtDeviceStack::getCurrentNvLengthFromApp(LtNetworkVariable* pNv, int &length) 
{
    boolean success = lonApp->getCurrentNvLength(pNv, length);
    if (success)
    {
        if (length > pNv->getLength())
        {
            length = pNv->getLength();
        }
    }
    return success;
}

void LtDeviceStack::flushCheck()
{
	if (flushPending() && !outgoingActivity() && 
		(!getLayer4()->getCommIgnore() || !incomingActivity()))
	{
		flushCancel();
		flushCompleted();
	}
}

LtErrorType LtDeviceStack::processApdu(LtApduIn* pApdu) 
{
	LtErrorType err = LT_NO_ERROR;
    boolean nv = pApdu->isNetworkVariable();
    boolean success = pApdu->getSuccess();
    int nvIndex = pApdu->getNvIndex();

	if (err == LT_NO_ERROR)
	{
		LtMsgTag* pTag = (LtMsgTag*) pApdu->getRefId().getRefId();
		if (pApdu->getSuccess() || pApdu->getFailure())
		{
			if (nv) 
			{
				LtNetworkVariable* pNv;
				int arrayIndex;
				pNv = getNetworkVariable(nvIndex, arrayIndex);
				if (pNv == NULL)
				{
					err = LT_INVALID_PARAMETER;
				}
				else
				{
                    LtNetworkVariableConfiguration nvc;
                    err = getNetworkImage()->nvTable.getNv(nvIndex, &nvc);
                    if (!nvc.incarnationMatches(pApdu->getNvIncarnationNumber()))
                    {   // Its an LT_INVALID_NV_INDEX, but since its a response, rather than
                        // an incomming message, call it a LT_STALE_NV_INDEX.  
                        err = LT_STALE_NV_INDEX;
                    }
                    else
                    {
					    lonApp->nvUpdateCompletes(pNv, arrayIndex, success);
                    }
				}
			} 
			else 
			{
				if (pTag == m_pLocalOpMsgTag)
				{
                    m_localOpErr = pApdu->getSuccess() ? LT_NO_ERROR : LT_LOCAL_MSG_FAILURE;
					delete m_pLocalOpMsgTag;
					m_pLocalOpMsgTag = null;
				}
				else if (pTag != null)
				{
					lonApp->msgCompletes(pTag, success);
				}
			}
			release(pApdu->getOrigPri());
		} 
		else if (pApdu->getResponse())
		{
			if (nv) 
			{
				err = handleNvUpdates((LtApduIn*) pApdu);
			} 
			else
			{
				LtRespIn* pRespIn = (LtRespIn*) pApdu;
				if (pTag == m_pLocalOpMsgTag)
				{
					// Special case for "retrieve_status"
					m_currentStatus.fromLonTalk(pApdu->getData());
					release(pRespIn);
				}
				else
				{
					lonApp->respArrives(pTag, pRespIn);
					// Up to app to free memory
				}
				pApdu = null;
			}
		} 
		else if (pApdu->forProxyHandler())
		{
			err = processLtep(pApdu);
		}
        else if (pApdu->forNetworkManager() && !isMsgHooked(*pApdu)) 
		{
			err = getNetworkManager()->process(*pApdu);
			if (err == LT_APP_MESSAGE)
			{
				err = handleNetworkManagement((LtApduIn*) pApdu);
			}
		}
		else if (nv) 
		{
			if (pApdu->isRequest()) 
			{
				if (getOffline()) 
				{
					sendMessage(nvResponse((LtApduIn*) pApdu, null, 0));
				} 
				else 
				{
					byte data[MAX_NV_LENGTH];
					int arrayIndex;
					int nLength = MAX_NV_LENGTH;
					LtNetworkVariable* pNv = getNetworkVariable(nvIndex, arrayIndex);
					if (pNv == NULL)
					{
						nLength = 0;
					}
					else
					{
						pNv->getNvData(data, nLength, arrayIndex);
                        sendMessage(nvResponse((LtApduIn*) pApdu, data, pNv->getCurLength()));
					}
				}
			} 
			else 
			{
				err = handleNvUpdates((LtApduIn*) pApdu);
			}
		} 
		else 
		{
			err = LT_APP_MESSAGE;
		}

		if (err == LT_APP_MESSAGE)
		{
			lonApp->msgArrives((LtMsgIn*) pApdu);
				// Up to app to free memory
			pApdu = null;
		}
	}

	delete pApdu;

	flushCheck();

	return err;
}

LtErrorType LtDeviceStack::dynamicNv(int cmd, byte* pData, int dataLen, byte* pResp, int &respLen)
{
	NdErrorType err = ND_OK;
	boolean bStore = false;

    switch (cmd) {
		case LT_APP_NV_DEFINE:
			err = getDynamicNvs()->define(pData, dataLen);
			bStore = true;
			respLen = 0;
            break;
        case LT_APP_NV_REMOVE:
			err = getDynamicNvs()->remove(pData, dataLen);
			bStore = true;
			respLen = 0;
            break;
        case LT_QUERY_NV_INFO:
			err = getDynamicNvs()->query(pData, dataLen, pResp, respLen);
            break;
        case LT_QUERY_NODE_INFO:
			if (*pData++ != 3)
			{
				err = ND_VALUE_OUT_OF_RANGE;
				respLen = 0;
			}
			else
			{
				int offset;
				int length;

				PTOHS(pData, offset);
				PTOHB(pData, length);

				const char* szDesc = getNodeSd();
				int slen = strlen(szDesc) + 1;
				if (offset > slen)
				{
					err = ND_VALUE_OUT_OF_RANGE;
				}
				else
				{
					if (offset + length > slen)
					{
						length = slen - offset;
					}
					respLen = length + 1;
					pResp[0] = (byte) length;
					memcpy(&pResp[1], szDesc + offset, length);
				}
            }
            break;
        case LT_UPDATE_NV_INFO:
			err = getDynamicNvs()->change(pData, dataLen);
			respLen = 0;
            break;
	}

	if (err == ND_OK && bStore)
	{
		// In case network image was updated...
		getNetworkImage()->store();
	}

	return err == ND_OK ? LT_NO_ERROR : LT_INVALID_PARAMETER;
}

LtErrorType LtDeviceStack::handleNetworkManagement(LtApduIn* pApdu) 
{
	LtErrorType err = LT_NO_ERROR;
    byte data[MAX_APDU_SIZE];
    data[0] = (byte) pApdu->getNmCode(true);
	int len = 1;
    switch (pApdu->getCode()) 
	{
    case LT_ECS:
    case LT_EXPANDED:
	{
        // Handle ECS and EXPANDED NV updates
        bool isNvUpdate = false;

		if (pApdu->getCode() == LT_ECS)
        {
            if (pApdu->getDataLength() && pApdu->getData(0) == LT_UPDATE_NV_VALUE)
		    {
                isNvUpdate = true;
            }
		    else
		    {
			    lonApp->wink();
		    }
        } 
        else  if (pApdu->getDataLength() && pApdu->getData(0) == LT_EXP_UPDATE_NV_BY_INDEX)
	    {
            isNvUpdate = true;
        }
        else
        {
            err = LT_INVALID_PARAMETER;
        }

        if (err == LT_NO_ERROR && isNvUpdate)
        {
            if (pApdu->getDataLength() > 3)
            {
                int index = LtMisc::makeint(pApdu->getData(1), pApdu->getData(2));

			    pApdu->setNvIndex(index);
                LtNetworkVariableConfiguration nvc;
                err = getNetworkImage()->nvTable.getNv(index, &nvc);
                if (err == LT_NO_ERROR)
                {
                    if (getNetworkImage()->nvTable.getTableType(index) != NV_TABLE_NVS)
                    {   // Don't support updating private NVs by index.
                        err = LT_INVALID_NV_INDEX;
                    }
                    else
                    {
                        pApdu->setNvIncarnationNumber(nvc.getIncarnationNumber());
			            pApdu->setNvDataOffset(3);
			            err = handleNvUpdates(pApdu);
                    }
                }
            }
            else
            {
                err = LT_INVALID_PARAMETER;
            }
		}

        if (pApdu->getCode() == LT_EXPANDED)
        {
            data[0] = (byte) pApdu->getNmCode(err == LT_NO_ERROR);
            data[1] = pApdu->getCode();  // expanded commands always include code in response, even if failure.
		    len++;
        }
	    else if (err != LT_NO_ERROR)
	    {
            data[0] = (byte) pApdu->getNmCode(false);
		    data[1] = getNetworkManager()->toNmErr(err);
		    len++;
	    }
        break;
	}

    case LT_NODE_MODE: 
	{
        boolean was = getOffline();
		setOffline(pApdu->getData(0) == LT_MODE_OFFLINE);
		boolean now = getOffline();
		if (was != now) 
		{
			if (now) 
			{
				lonApp->offline();
			} 
			else 
			{
				lonApp->online();
			}
		}
	    break;
	}

    case LT_FETCH_NETWORK_VARIABLE: 
	{
        // Validated by lowered layers.
        int index = pApdu->getData(0);
        int offset = 1;
        if (index == LT_ESCAPE_INDEX) 
		{
            index = (pApdu->getData(1) << 8) | pApdu->getData(2);
            offset = 3;
        }
		int nvLen = MAX_NV_LENGTH;
		int arrayIndex;
		LtNetworkVariable* pNv = NULL;
        
        if (getNetworkImage()->nvTable.getTableType(index) == NV_TABLE_NVS)
        {   // Don't support fetching private NVs by index.
            pNv = getNetworkVariable(index, arrayIndex);
        }
		if (pNv == NULL)
		{
            data[0] = (byte) pApdu->getNmCode(false);
		}
		else
		{
			pNv->getNvData(&data[offset + 1], nvLen, arrayIndex);
            len = pNv->getCurLength() + offset + 1;
            pApdu->getData(&data[1], 0, offset);
        }
        break;
    }
    case LT_QUERY_SI_DATA: 
	{
		int siLen;
        byte* siData = lonApp->getSiData(&siLen);
        int requestLen = ((int) pApdu->getData(2)) & 0xff;
        int offset = LtMisc::makeint(pApdu->getData(0), pApdu->getData(1));
		len = requestLen + 1;
		if (siData != null)
		{
			if (offset + requestLen > siLen) 
			{
				data[0] = (byte) pApdu->getNmCode(false);
				len = 1;
			} 
			else 
			{
				memcpy(&data[1], &siData[offset], requestLen);
			}
		}
		else
		{
			if (makeSiData(&data[1], offset, requestLen) != ND_OK)
			{
				data[0] = (byte) pApdu->getNmCode(false);
				len = 1;
			}
		}
        break;
    }
    case LT_READ_MEMORY: 
	{
        int address = LtMisc::makeint(pApdu->getData(1), pApdu->getData(2));
        int length = LtMisc::makeuint(pApdu->getData(3));
        byte mem[MAX_APDU_SIZE];
        if (lonApp->readMemory(address, mem, length))
        {
		    len = length + 1;
            data[0] = (byte) pApdu->getNmCode(true);
		    memcpy(&data[1], mem, length);
        }
        else
        {
            // Read failed. Override default response code
            data[0] = (byte) pApdu->getNmCode(false);
        }
        break;
    }     
    case LT_WRITE_MEMORY: 
	{
        int address = LtMisc::makeint(pApdu->getData(1), pApdu->getData(2));
        int length = LtMisc::makeuint(pApdu->getData(3));
        byte mem[MAX_APDU_SIZE];
		memcpy(mem, pApdu->getData() + 5, length);
        if (!lonApp->writeMemory(address, mem, length))
        {  
            // Write failed. Override default response code
            data[0] = (byte) pApdu->getNmCode(false);
        }
        break;
    }
	case LT_SERVICE_PIN:
	case LT_NM_ESCAPE:
	case LT_ROUTER_ESCAPE:
	{
		// Router escape must go to companion router stack.  Other side
		// response comes back asynchronously.
		// Service Pin also sent up to app.
		err = LT_APP_MESSAGE;
		len = 0;
		break;
	}
    default:
	{
        data[0] = (byte) pApdu->getNmCode(false);
        break;
	}
    }
    if (len != 0 && pApdu->isRequest()) 
	{
		sendResponse((LtMsgIn*) pApdu, data, len);
    }
	return err;
}

void LtDeviceStack::eventNotification() 
{
    if (m_bDirectCallbackMode) 
	{
        processApplicationEvents();
    } 
	else if (lonApp != NULL) 
	{
        lonApp->applicationEventIsPending();
    }
}

#if FEATURE_INCLUDED(MONITOR_SETS)
// Monitor Set callbacks
LtMonitorSet *LtDeviceStack::monSetAdded(LtMonitorSet *pMonitorSet)
{
	LtMonitorSet* pNewMs = lonApp->monitorSetAdded(pMonitorSet);

	if (pNewMs != NULL && pNewMs != pMonitorSet)
	{
		delete pMonitorSet;
		pMonitorSet = pNewMs;
	}
	return pMonitorSet;
}

void LtDeviceStack::monSetChanged(LtMonitorSet *pMonitorSet, McChangeType type)
{
    lonApp->monitorSetChanged(pMonitorSet, type);
}

void LtDeviceStack::monSetDeleted(LtMonitorSet *pMonitorSet)
{
    lonApp->monitorSetDeleted(pMonitorSet);
}

boolean LtDeviceStack::monSetExists(int index)
{
	return m_monitorSetTable.get(index) != NULL;
}

LtMonitorPoint *LtDeviceStack::monPointAdded(LtMonitorPoint *pMonitorPoint)
{
	LtMonitorPoint* pNewMp = lonApp->monitorPointAdded(pMonitorPoint);

	if (pNewMp != NULL && pNewMp != pMonitorPoint)
	{
		delete pMonitorPoint;
		pMonitorPoint = pNewMp;
	}
	return pMonitorPoint;
}

void LtDeviceStack::monPointChanged(LtMonitorPoint *pMonitorPoint, McChangeType type)
{
    lonApp->monitorPointChanged(pMonitorPoint, type);
}

void LtDeviceStack::monPointDeleted(LtMonitorPoint *pMonitorPoint)
{
    lonApp->monitorPointDeleted(pMonitorPoint);
}
#endif

//
// Protected Member Functions
//


void LtDeviceStack::processPacket(boolean pPriority, LtPktInfo *pPkt)
{
	// Ignore priority.  Could add priority and non-priority input
	// queues but deemed not necessary for now.
	getLayer4()->queuePacket(pPkt);
}

// Each client (except for the sender of the packet) is called with this method in order to 
// modify their layer 2 stats.
void LtDeviceStack::updateStats(const byte* pApdu, bool isUnackd, bool domainMatch, bool gotThisMsg)
{
	bool increment = true;
	// For certain clients, we only increment the layer 2 count if the packet doesn't follow certain
	// rules.  The case for this is the NES IP meter where we don't want to count packets from the
	// DC in the IP meter because this would throw off the discovery algorithm (it would discover its
	// own DC).  If we were routed this message, then we always count it.
	if (m_hasExclusionUid && !gotThisMsg)
	{
		// There are two distinct cases where we don't count the packet.
		// a. It is ATM: the first NID is the exclusion NID
		// b. It is not ATM: there was a domain match
		// Note that this assumes we only send ATM activity on the discovery domain.  Also, it assumes
		// that ATM only uses unacked service.  For other service types, the "code" isn't the next data
		// byte.  The L4 stuff comes next.
		if (isUnackd && LT_IS_ATM(pApdu[0]))
		{
			// The ATM source NID is at offset 2 of the data so we compare this against the exclusion NID
			LtUniqueId uid(pApdu+2);
			if (uid == m_exclusionUid)
			{
				increment = false;
			}			
		}
		else if (domainMatch)
		{
			increment = false;
		}
	}
	if (increment)
	{
		getNetworkStats()->bump(LT_LAYER2_RECEIVE);
	}
}

//
// setExclusionUid
//
// This method sets up the unique ID that is excluded from stats for ATM purposes (this is an NES feature).
//
void LtDeviceStack::setExclusionUid(const byte* pUid)
{
	m_hasExclusionUid = true;
	m_exclusionUid.set(pUid);
}

boolean LtDeviceStack::outgoingActivity()
{
	boolean result = false;
	for (int i = 0; i < LT_PRIORITY_TYPES; i++)
	{
		if (msgCount[i])
		{
			result = true;
			break;
		}
	}
	return result;
}

boolean LtDeviceStack::incomingActivity()
{
	return msgQNumMsgs(m_queApdus) != 0;
}

boolean LtDeviceStack::adjustBufLimit(boolean bPri, boolean bAdd, boolean bForce)
{
    int index = LtMisc::getPriorityType(bPri);
    semTake(m_sem, WAIT_FOREVER);
    boolean bOk = msgCount[index] < maxMsg[index];
    if (bOk || !bAdd || bForce)
    {
        bOk = TRUE; // EPR 21329 Return OK, if we did it, even it was forced.
        msgCount[index] += bAdd ? 1 : -1;
    }
    if (msgCount[index] < 0)
    {
		printf("Message count failure: bPri=%d, bAdd=%d, msgCount[0]=%d, msgCount[1]=%d\n", bPri, bAdd, msgCount[0], msgCount[1]);
        assert(0);
        msgCount[index] = 0;
    }
    semGive(m_sem);
    return bOk;
}

LtMsgOut* LtDeviceStack::msgAllocGen(boolean pri, boolean bForce) 
{
	LtMsgOut* p = NULL;
    if (adjustBufLimit(pri, true, bForce))
    {
		p = new NOTHROW LtMsgOut(pri);
		vxlMemoryCheck(p);
    }
	return p;
}

LtMsgOut* LtDeviceStack::msgAlloc(boolean pri, LtBlob *pBlob)
{
	LtMsgOut* p = NULL;
    if (adjustBufLimit(pri, true))
    {
		p = new NOTHROW LtMsgOut(*pBlob);
		vxlMemoryCheck(p);
    }
	return p;
}

void LtDeviceStack::release(boolean pri) 
{
    adjustBufLimit(pri, false);
    doNvUpdates(LtMisc::getPriorityType(pri));
}

//
// Public Member Functions
//


LtDeviceStack::LtDeviceStack(LreClientType clientType, LtLogicalChannel* pChannel, LtLreServer* pLre, 
                             int nRxTx, int nTxTx,
                             int maxL4OutputPackets,
                             int maxL4PriorityOutputPackets) :
	LonTalkStack(pChannel, pLre), 
	LtLreClientBase(clientType, pChannel),
	m_pChannel(pChannel), 
	m_pLre(pLre) 
{
#if FEATURE_INCLUDED(L5MIP)
    // No layer 5 MIP on i.LON
	if (m_pChannel->isLayer5Interface())
	{
		LtLtLogicalChannel* pLtChannel = (LtLtLogicalChannel*) pChannel;
		m_pLayer4 = new LtLayer5Mip(pLtChannel, pLtChannel->getNsaMip());
	}
	else
#endif
	{
		m_pLayer4 = new LtLayer4(nRxTx, nTxTx, 
                                 maxL4OutputPackets, 
                                 maxL4PriorityOutputPackets);
	}
        
    m_shutDown = FALSE;
	m_pMainClient = null;
	m_queApdus = null;
	memset(maxMsg, 0, sizeof(maxMsg));
	setMessageEventMaximum(10);
	m_sem = semMCreate( SEM_Q_PRIORITY | SEM_INVERSION_SAFE );
    m_bDirectCallbackMode = false;
	m_bMessageLock = false;
	setMessageOutMaximum(10, 10);
	memset(msgCount, 0, sizeof(msgCount));
	lonApp = null;
	m_pDynamicNvs = null;
    reset();
    getLayer4()->setStack(this);
    LtLayer6::setStack(this);
	LonTalkNode::setStack(this);
#if FEATURE_INCLUDED(MONITOR_SETS)
    m_monitorSetTable.setClient(this);
	m_monitorSetTable.setMpTable(&m_monitorPointTable);
	m_monitorPointTable.setClient(this);
#endif
    m_pLocalOpMsgTag = null;
    m_localOpErr = LT_LOCAL_MSG_FAILURE;
    m_localOpSem = semMCreate( SEM_Q_FIFO );
    m_szPersistencePath = NULL;

    m_nServicePinState = (LtServicePinState)0xff;  

    m_lsAddrMappingAnnounceFreq = DEFAULT_LS_ADDR_MAPPING_ANNOUNCEMENT_FREQUENCY;
    m_lsAddrMappingAnnounceThrottle = DEFAULT_LS_ADDR_MAPPING_ANNOUNCEMENT_THROTTLE;
    m_lsAddrMappingAgeLimit     = m_lsAddrMappingAnnounceFreq*2;
}

LtDeviceStack::~LtDeviceStack()
{
	// remove ourselves from the list in the channel.
	m_pLayer4->halt();
	m_pChannel->deregisterStackClient( this );
	if (getLre()) getLre()->deregisterOwner(this);
    semDelete(m_sem);
    semDelete(m_localOpSem);

	LtApduIn* pApdu;
    while (msgQReceive(m_queApdus, (char*) &pApdu, sizeof(pApdu), NO_WAIT) == sizeof(pApdu))
	{
		delete pApdu;
    }
	msgQDelete(m_queApdus);
	delete getDynamicNvs();
	delete m_pLayer4;
	delete getMainClient();
    delete m_szPersistencePath;
    m_vecHookedMsgs.clear(true);
}

boolean LtDeviceStack::stop()
{
    sync();
    return true;
}

void LtDeviceStack::sync()
{
	// Synchronize persistent information
	getNetworkImage()->sync();
	getPersistence()->sync();
#if FEATURE_INCLUDED(MONITOR_SETS)
    m_monitorSetTable.getPersistence()->sync();
	m_monitorPointTable.getPersistence()->sync();
#endif
}

void LtDeviceStack::prepareForBackup(void)
{
	getNetworkImage()->getPersistence()->prepareForBackup();
	getPersistence()->prepareForBackup();
#if FEATURE_INCLUDED(MONITOR_SETS)
    m_monitorSetTable.getPersistence()->prepareForBackup();
	m_monitorPointTable.getPersistence()->prepareForBackup();
#endif
}

void LtDeviceStack::backupComplete(void)
{
	getNetworkImage()->getPersistence()->backupComplete();
	getPersistence()->backupComplete();
#if FEATURE_INCLUDED(MONITOR_SETS)
    m_monitorSetTable.getPersistence()->backupComplete();
	m_monitorPointTable.getPersistence()->backupComplete();
#endif
}

boolean LtDeviceStack::readyForBackup(void)
{   // If one of them is ready, assume they all are...
    return(getNetworkImage()->getPersistence()->readyForBackup());
}

LtApplication* LtDeviceStack::getApp() 
{
    return lonApp;
}

LtErrorType LtDeviceStack::registerApplication(int index, LtApplication* lonApp, 
		int numDomainEntries, int numAddressEntries, 
		int numStaticNvEntries, int numDynamicNvEntries,
        int numPrivateNvEntries, int aliasEntries,
		int numMonitorNvEntries, int numMonitorPointEntries,
		int numMonitorSetEntries, int numMessageTags,
		int bindingConstraintLevel,
		const char* pNodeSdString, LtProgramId* pProgramId)
{
	LtErrorType err = LT_NO_ERROR;
    this->lonApp = lonApp;
    int numTotalNvs;
	int numPublicNvEntries = numStaticNvEntries + numDynamicNvEntries + numMonitorNvEntries; 

	m_nIndex = index;

	m_pDynamicNvs = new DynamicNvs(this, &getNetworkImage()->nvTable);

    getNetworkImage()->nvTable.setStack(this);

	m_nvIndex = 0;
	m_privateNvIndex = numPublicNvEntries;

	// Set node definition
	registerNodeDefClient((NodeDefClient*) this);
	setStaticNetworkVariableCount(numStaticNvEntries);
	setMaximumDynamicNetworkVariableCount(numDynamicNvEntries);
	setMaximumPrivateNetworkVariableCount(numPrivateNvEntries);
	setAddressTableCount(numAddressEntries);
	setDomainCount(numDomainEntries);
	setProgramId(*pProgramId);	// set by reference, so the "modifiable" flag is copied, too (for LtMipApp)
	setNodeSd(pNodeSdString);
	setAliasCount(aliasEntries);
	setMaximumMonitorNetworkVariableCount(numMonitorNvEntries);
	setMaximumMonitorPointCount(numMonitorPointEntries);
	setMaximumMonitorSetCount(numMonitorSetEntries);
	setMessageTagCount(numMessageTags);
	setLayer5Mip(m_pChannel->isLayer5Interface());
    #if FEATURE_INCLUDED(L5MIP)
        // No layer 5 MIP on i.LON. Windows iLON simuluation requires L2 MIP
	    if (m_pChannel->isLayer5Interface())
	    {
		    err = static_cast<LtLayer5Mip *>(m_pLayer4)->open();
	    }
    #endif

    setOma(omaSupported());   

	if (bindingConstraintLevel >= 2)
	{
		setBinding2(true);
	}
	if (bindingConstraintLevel >= 3)
	{
		setBinding3(true);
	}
	ndInitDone();

#if FEATURE_INCLUDED(MULTI_APP)
    if (err == LT_NO_ERROR)
    {
	    err = getPlatform()->setIndex(getIndex());
    }

	getPersistence()->setIndex(getIndex());
	getNetworkImage()->getPersistence()->setIndex(getIndex());
#endif

#if FEATURE_INCLUDED(MONITOR_SETS)
    m_monitorSetTable.getPersistence()->setIndex(getIndex());
	m_monitorPointTable.getPersistence()->setIndex(getIndex());
#endif
	getNetworkImage()->nvTable.setNodeDef(this);

	if (err == LT_NO_ERROR)
	{
		err = getLayer4()->getStartError();
	}

	if (err == LT_NO_ERROR)
	{
		err = getChannel()->getStartError();
	}

	if (err == LT_NO_ERROR)
	{
		LonTalkNode::setCounts(numAddressEntries, numDomainEntries, numMessageTags);
		LonTalkStack::setStack(this);

		maxPublicNvIndex = numPublicNvEntries - 1;
		maxPrivateNvIndex = numPublicNvEntries + numPrivateNvEntries - 1;
		numTotalNvs = maxPrivateNvIndex + 1;
		for (int i = 0; i < LT_PRIORITY_TYPES; i++) 
		{
			nvUpdates[i].setSize(numTotalNvs);
		}

		getNetworkImage()->setNvAliasCount(numPublicNvEntries, 
												numPrivateNvEntries, aliasEntries);

		err = getLayer4()->initProgram(*pProgramId);

		if (err == LT_NO_ERROR)
		{
			LonTalkStack::reset(true);
#if FEATURE_INCLUDED(MONITOR_SETS)
			m_monitorSetTable.restore();
			m_monitorPointTable.restore();
#endif
			resetRoutes();

			// Create an LRE client for the unique ID address.  This guy exists
			// independent of any other configuration.  For now, just grab the
			// one and only unique id - need to provide app name and map app
			// name to unique id.
			LtLreClient* pClient;
			if (isLayer2())
			{
				pClient = new LtLayer2Client(this, getChannel());
			}
			else
			{
				LtUniqueId uid;
				
				getPlatform()->getUniqueId(&uid);
				pClient = new LtUniqueIdClient(this, getChannel(), &uid);
			}
			setMainClient(pClient);
			// add ourselves to the list in the channel
			m_pChannel->registerStackClient( this );
			if (getLre()) getLre()->registerClient(pClient);

			readyToReceive();
			notify();
		}
	}

	if (numStaticNvEntries == 0)
	{
		// Will not be getting any NV registrations so simulate an "end"
		endDefinition();
	}

	if (err == LT_NO_ERROR)
	{
		// Now allow messages to be received.
		flushCancel();
	}

	return err;
}

// Provides a means for an application to initiate a reset.  Also used by stack
// to initiate a reset.
void LtDeviceStack::initiateReset()
{
	LonTalkStack::resetRequested();
}

void LtDeviceStack::localReset()
{
    if (LonTalkStack::getStack())
    {
	    LtDeviceStack::reset();
	    LonTalkStack::reset();
	    resetRoutes();
	    resetOccurred();
    }
}

// For registering public NVs
LtErrorType LtDeviceStack::registerNetworkVariable(LtNetworkVariable *pNv)
{
	return registerNetworkVariable(pNv, 0, 0, NULL);
}

void LtDeviceStack::lockNvs()
{   // Lock NV definitions.  
    getDynamicNvs()->lock();
}
void LtDeviceStack::unlockNvs()
{
    getDynamicNvs()->unlock();
}

// For registering private NVs (and indirectly for public NVs as well).
LtErrorType LtDeviceStack::registerNetworkVariable(LtNetworkVariable* pNv, int nvIndex, 
									int nvSelector, LtOutgoingAddress* pAddress)
{
	LtErrorType err = LT_NO_ERROR;
	int nIndex = pNv->getNvIndex();
	int arrayIndex;
	char name[ND_NAME_LEN];
	LtNetworkVariable* pDup;

    // lock NV definitions (see getNetworkVariableDirection())
    lockNvs();

	// To allow for NV operations originated via the NV object, register the stack with the NV.
	// This does require an extra word per NV object but I think it looks nice so it's worth it.
	pNv->setStack(this);

	if (nIndex == -1)
	{
		//
		// We don't keep a global private NV index because we want to avoid creating
		// too many private NV configuration records.  So, just find an empty slot each
		// time we create a new NV.
		//
		if (pNv->getPrivate())
		{
			LtNetworkVariable* pTemp;
			nIndex = maxPublicNvIndex + 1;
			while ((pTemp = getNetworkVariable(nIndex, arrayIndex)) != NULL)
			{
				nIndex += pTemp->getElementCount();
			}
			if (nIndex > maxPrivateNvIndex ||
				pNv->getElementCount() > 1)
			{
				// 
				// We don't currently have the source to auto-allocate indices for
				// private NV arrays.
				//
				err = LT_NO_RESOURCES;
			}
		}
		else
		{
			nIndex = m_nvIndex;
			m_nvIndex = nIndex + pNv->getElementCount();
		}
		pNv->setNvIndex(nIndex);
	}

	if (err == LT_NO_ERROR)
	{
		if (pNv->getPrivate())
		{
			if (nIndex != -1 &&
				getNetworkVariable(nIndex, arrayIndex) != NULL)
			{
				// registering a duplicate private (by index) is an auto remove
				removeNvs(nIndex, 1);
			}
		}

		pNv->getName(name, sizeof(name));

		// Check if NV with this name already exists
		if ((pDup = getNetworkVariable(name, arrayIndex)) != NULL)
		{
			err = LT_DUPLICATE_OBJECT;
		}
	}

	if (err == LT_NO_ERROR)
	{
		if (pNv->getPrivate())
		{
			addNonStaticNv(nIndex, pNv, true);
			if (!pNv->getTemporary())
			{
				persistenceUpdate();
			}
		}
		else
		{
			// Register NV definition with node definition object
			// For now, all NVs are registered as part of the node 
			addNetworkVariable(pNv);
		}
	}

	// Set the NV configuration for private NVs only.  
	// Note that we used to have to update the NV config for public as well, to update the
	// default service, priority, auth, etc.  However now, if the NV is unbound, the 
	// NV config is not even stored, and the service, priority, and auth come from the 
	// NV definition rather than the Nv Config.  
	// We must avoid setting NV config when it is not necessary (EPR 33411), since this makes the 
	// node vulnerable to resets.  For example, if an app starts up, and resets after it has
	// started to register its network varables but before the persistence file is written, it
	// will go unconfigured.  
	if (pNv->getPrivate())
	{
		for (int i = nIndex; err == LT_NO_ERROR && i < nIndex + pNv->getElementCount(); i++)
		{
			LtNetworkVariableConfiguration nvc;

			err = getNetworkImage()->nvTable.getNv(i, &nvc);

			if (err == LT_NO_ERROR)
			{
				// Regardless of what had been posted, override NV config.
				nvc.setNvIndex(nvIndex);
				nvc.setSelector(nvSelector);
				nvc.setAddressTableIndex(LT_EXPLICIT_ADDRESS);
				nvc.setNvUpdateSelection(pNv->getSourceSelection() ? LT_SELECTION_BYSOURCE : LT_SELECTION_NEVER);
				nvc.setNvResponseSelection(pNv->getSourceSelection() ? LT_SELECTION_BYSOURCE : LT_SELECTION_NEVER);
				nvc.setReadByIndex(!pNv->getReadBySelector());
				nvc.setWriteByIndex(pNv->getWriteByIndex());
				if (pAddress != null) 
				{
					*nvc.getAddress() = *pAddress;
				}
				if (pAddress == null ||
					pAddress->getDomainConfiguration().getDomain().inUse() ||
					pAddress->getAddressType() == LT_AT_UNIQUE_ID) 
				{
					// These are not supported because: 
					// 1. LtDomain and unique ID data aren't stored in EEPROM (could be done).
					// 2. Can't get responses for arbitrary domains (harder).
					assert(0);
					err = LT_INVALID_PARAMETER;
				}

                getDefaultNetworkVariableConfigAttributes(pNv, nvc);

				nvc.incrementIncarnation();
				getNetworkImage()->nvTable.setNv(i, nvc, false);
				if (!pNv->getTemporary())
				{
					// It is OK to do a "store" with temporaries present but not necessary.
					getNetworkImage()->store();
				}
			}
		}
	}

    unlockNvs();

	return err;
}

LtErrorType LtDeviceStack::deregisterAllPrivates()
{
	LtErrorType err = LT_NO_ERROR;

    // lock NV definitions (see getNetworkVariableDirection())
    lockNvs();

	if (removeNvs(maxPublicNvIndex+1, maxPrivateNvIndex-maxPublicNvIndex) != ND_OK)
	{
		err = LT_INVALID_PARAMETER;
	}
	else
	{
		m_privateNvIndex = maxPublicNvIndex+1;
		getNetworkImage()->nvTable.clearNv(maxPublicNvIndex+1, maxPrivateNvIndex-maxPublicNvIndex);
		persistenceUpdate();
		getNetworkImage()->store();
	}

    unlockNvs();

	return err;
}

LtErrorType LtDeviceStack::deregisterNetworkVariable(LtNetworkVariable* pNv)
{
	// Only support deregistering private NVs
	LtErrorType err = LT_NO_ERROR;
	int index = pNv->getNvIndex();
	boolean bTemp = pNv->getTemporary();

    // lock NV definitions (see getNetworkVariableDirection())
    lockNvs();

	if (!(pNv->getPrivate()) ||
		removeNvs(index, 1) != ND_OK)
	{
		err = LT_INVALID_PARAMETER;
	}
	else
	{
		// Update NV configuration
		LtNetworkVariableConfiguration nvc;
		getNetworkImage()->nvTable.getNv(index, &nvc);
		nvc.initialize();
		getNetworkImage()->nvTable.setNv(index, nvc, false);
		if (!bTemp)
		{
			persistenceUpdate();
			getNetworkImage()->store();
		}
	}
	
    unlockNvs();

	return err;
}

LtNetworkVariable* LtDeviceStack::getNetworkVariable(char* szName, int &arrayIndex)
{
	return (LtNetworkVariable*) LonTalkNode::get(szName, arrayIndex);
}

LtNetworkVariable* LtDeviceStack::getNetworkVariable(int nvIndex, int &arrayIndex)
{
	return (LtNetworkVariable*) LonTalkNode::get(nvIndex, arrayIndex);
}


LtErrorType LtDeviceStack::getNetworkVariableDirection(int nvIndex, boolean &bOutput)
{
    // This routine gets the direction of a network variable in a "safe" manner.
    // This is needed by the lower layers of the protocol in the case where NV
    // configuration is not available (a footprint optimization).  To guard against
    // NVs being deleted during lower layer processing, we lock this overall process.
    LtErrorType err = LT_NOT_FOUND;
    int arrayIndex;

    // Lock NV definitions as this is also used during NV registration
    lockNvs();

    LtNetworkVariable* pNv = getNetworkVariable(nvIndex, arrayIndex);
    if (pNv != NULL)
    {
        bOutput = pNv->getOutput();
        err = LT_NO_ERROR;
    }

    unlockNvs();

    return err;
}

void LtDeviceStack::getDefaultNetworkVariableConfigAttributes(LtNetworkVariable* pNv, 
                                                              LtNetworkVariableConfiguration &nvc)
{

	nvc.setOutput(pNv->getIsOutput());
	nvc.setPriority(pNv->getPriority());
	if (pNv->getFlags() & NV_SD_UNACKD)
	{
		nvc.setServiceType(LT_UNACKD);
	}
	else if (pNv->getFlags() & NV_SD_UNACKD_RPT)
	{
		nvc.setServiceType(LT_UNACKD_RPT);
	}
	else
	{
		nvc.setServiceType(LT_ACKD);
	}
	nvc.setAuthenticated(pNv->getAuth());
	nvc.setRemoteNmAuth(pNv->getNmAuth());
}

LtErrorType LtDeviceStack::getDefaultNetworkVariableConfigAttributes(int nvIndex, LtNetworkVariableConfiguration &nvc)                                                                    
{
    // This routine gets the NV attributes of a network variable in a "safe" manner.
    // This is needed by the lower layers of the protocol in the case where NV
    // configuration is not available (a footprint optimization).  To guard against
    // NVs being deleted during lower layer processing, we lock this overall process.
    LtErrorType err = LT_NOT_FOUND;
    int arrayIndex;

    // Lock NV definitions as this is also used during NV registration
    lockNvs();

    LtNetworkVariable* pNv = getNetworkVariable(nvIndex, arrayIndex);
    if (pNv != NULL)
    {
        getDefaultNetworkVariableConfigAttributes(pNv, nvc);
        err = LT_NO_ERROR;
    }

    unlockNvs();

    return err;
}
                                                               
// This routine gets a private network variable which matches that passed
// in.  "match" means it has the same selector, direction, source
// selection, NV index and address.  It is intended to be used find
// a private NV which already exists rather than registering a 
// redundant one.
LtNetworkVariable* LtDeviceStack::getMatchingNetworkVariable(LtNetworkVariable* pNv,
	int nvIndex, int nvSelector, LtOutgoingAddress* pAddress)
{
	LtNetworkVariable* pFoundNv = null;
	int index;
	if (getNetworkImage()->nvTable.getPrivateNv(index, pNv->getIsOutput(), pNv->getSourceSelection(),
		nvIndex, nvSelector, pAddress) == LT_NO_ERROR)
	{
		// Not returned because private NV arrays are not supported.
		int arrayIndex;
		pFoundNv = getNetworkVariable(index, arrayIndex);
	}
	return pFoundNv;
}

void LtDeviceStack::registerMemory(int address, int size) 
{
    setMem(address, size);
}

int LtDeviceStack::getXcvrId()
{
	return getLayer4()->getXcvrId();
}

void LtDeviceStack::setXcvrId(int xcvrId)
{
	getLayer4()->setXcvrId(xcvrId);
}

void LtDeviceStack::setTxIdLifetime(int duration)
{
	getLayer4()->setTxIdLifetime(duration);
}

void LtDeviceStack::setCommParameters(LtCommParams& commParams) 
{
	LonTalkStack::setCommParams(&commParams);
}

void LtDeviceStack::setXcvrReg(byte* xcvrReg) 
{
	LonTalkStack::setXcvrReg(xcvrReg);
}

LtErrorType LtDeviceStack::setAuthenticationKey(int domainIndex, byte* key) 
{
    return getNetworkImage()->setAuthKey(domainIndex, key);
}

void LtDeviceStack::setApplicationEventThrottle(boolean value) 
{
    m_bMessageLock = value;
	if (m_bMessageLock == false)
	{
		// Note that this function is thus called in the app's thread.  If the
		// app has direct callback mode on, then normally events are processed
		// in the stack's thread.  The app would need to lock properly to ensure
		// this didn't cause a problem.
		processApplicationEvents();
	}
}

void LtDeviceStack::setDirectCallbackMode(boolean value) 
{
    m_bDirectCallbackMode = value;
}

void LtDeviceStack::processApplicationEvents() 
{
	if (m_nPersistenceLost != LT_PERSISTENCE_OK)
	{
		// Force node unconfigured.  Do not do so
		// on write failures otherwise we may cause 
		// an infinite write failure notification
		// loop.
		if (m_nPersistenceLost != LT_NO_PERSISTENCE &&
			m_nPersistenceLost != LT_PERSISTENT_WRITE_FAILURE)
		{
			// Go unconfigured.
			changeState(LT_UNCONFIGURED, true);
		}
		lonApp->persistenceLost(m_nPersistenceLost);
		m_nPersistenceLost = LT_PERSISTENCE_OK;
	}
    if (m_bFlushCompleted) 
	{
        m_bFlushCompleted = false;
        lonApp->flushCompletes();
    } 
    if (m_bServicePinDepressed) 
	{
        m_bServicePinDepressed = false;
		// Send a service pin message
		sendServicePinMessage();
        lonApp->servicePinPushed();
    }
    if (m_bServicePinReleased) 
	{
        m_bServicePinReleased = false;
        lonApp->servicePinHasBeenReleased();
    }
    if (m_bResetOccurred) 
	{
        m_bResetOccurred = false;
        lonApp->reset();
    }

	LtApduIn* pApdu;
    while (!m_bMessageLock && msgQReceive(m_queApdus, (char*) &pApdu, sizeof(pApdu), NO_WAIT) == sizeof(pApdu))
	{
		LtErrorType err = processApdu(pApdu);
		if (err != LT_NO_ERROR)
		{
            errorLog(err);
        }
    }
// EPANG TODO - enable callback later for Linux iLON
#if defined(ILON_PLATFORM) && defined(__VXWORKS__)
//#if PRODUCT_IS(ILON) && !defined(WIN32)
	if (m_propertyChangeHostEventFlag)
	{
		int index = 0;
		LtUniqueId uniqueId;

		m_propertyChangeHostEventFlag = false;
		if (getAddress(index, &uniqueId))
		{
			callDevicePropEventCallback(uniqueId.getData());
		}
	}
#endif
}

void LtDeviceStack::release(LtApduOut* pMsg)
{
	if (pMsg != null)
	{
		boolean bPriority = pMsg->getOrigPri();
		delete pMsg;
		release(bPriority);
	}
}

void LtDeviceStack::release(LtMsgIn* pMsg) 
{
	getLayer4()->apduInAvailable();
	delete pMsg;
}

void LtDeviceStack::release(LtRespIn* pMsg) 
{
	delete pMsg;
}

void LtDeviceStack::doNvUpdates(int type) 
{
    int index;
	boolean poll;
	boolean bKeepTrying = true;

	// Keep trying as long as we encounter bogus NV indices (perhaps left over from an NV deregister).
	while (bKeepTrying)
	{
		index = nvUpdates[type].getNext(poll);
		if (index != -1) 
		{
			int arrayIndex;
			LtNetworkVariable* pNv = getNetworkVariable(index, arrayIndex);

			if (pNv != null)
			{
				bKeepTrying = false;
				// Don't try again if this fails since we may not have a buffer and we 
				// don't want to lose updates due to buffer shortage.
				propagatePoll(poll, pNv, arrayIndex);
			}
		}
		else
		{
			bKeepTrying = false;
		}
	}
}

boolean LtDeviceStack::propagatePoll(boolean poll, int nvIndex)
{
	boolean bSuccess = false;
	int arrayIndex;
	LtNetworkVariable* pNv = getNetworkVariable(nvIndex, arrayIndex);

	if (pNv != null)
	{
		bSuccess = propagatePoll(poll, pNv, arrayIndex);
	}
	return bSuccess;
}

boolean LtDeviceStack::propagatePoll(boolean poll, LtNetworkVariable* pNv, int arrayIndex, LtMsgOverride* pOverride) 
{
    // Send an NV update/poll message
    boolean success = true;
    LtApduOut* pApdu;
	LtNetworkVariableConfiguration* pNvc;
    byte data[MAX_NV_LENGTH];
    int len = 0;
	boolean bPriority = FALSE;

	int nvIndex = pNv->getNvArrayIndex() + arrayIndex;

    LtErrorType err = getNetworkImage()->nvTable.getNv(nvIndex, &pNvc);

    if (err == LT_NOT_FOUND)
    {   // EPR 19983 
        // The NV config of an unbound NV is not stored.  Since propagating to 
        // an unbound NV should always succeed, don't treaat this as an error.
        // When seeing that the NV config does not exists, the lower layers 
        // will send a completion event (success).
        success = true;
		if (poll == 2)
		{
			poll = !pNv->getOutput();   
		}
    }
	else if (err != LT_NO_ERROR)
	{   // Other errors indicate a failure of some sort.
		success = false;
	}
	else
	{
		bPriority = pNvc->getPriority();

		// In case of NV update posted, we lose poll/propagate attribute.
		// Just use opposite of direction.  Should work unless you're 
		// polling private outputs.  Should this be supported?
		if (poll == 2)
		{
			poll = !pNvc->getOutput();
		}
    }

    if (success)
    {
		if (!poll)
		{
			len = sizeof(data);
			pNv->getNvData(data, len, arrayIndex);
            len = pNv->getCurLength();
			success = true;
		} 

		if (success)
		{
			if (pOverride && pOverride->getOptions().overridePriority())
			{
				bPriority = pOverride->getPriority();
			}
			LtMsgOut* pMsg = msgAllocGen(bPriority);
			pApdu = pMsg;
			if (pApdu != null) 
			{
				if (pOverride && pOverride->hasOverrides())
				{
					pApdu->setOverride(pOverride);
				}
				pApdu->setNvIndex(nvIndex);
				if (poll) 
				{
					pApdu->setServiceType(LT_REQUEST);
				} 
				else 
				{
					if (pApdu->setNvData(data, len) != LT_NO_ERROR)
					{
						// Leave success as true in this case, no point in trying again.
						release(pMsg);
						pApdu = null;
					}
				}
				if (pApdu)
				{
					sendMessage(pApdu);
				}
			} 
			else
			{
				success = false;
			}
		}

		// We can't defer updates of sync NVs or NVs with override specifications
		if (!success && !pNv->getSync() && (!pOverride || !pOverride->hasOverrides()))
		{
			// Mark for later update
			nvUpdates[LtMisc::getPriorityType(bPriority)].set(poll, nvIndex);
			success = true;
		}
	}
    return success;
}

boolean LtDeviceStack::propagate(int nvIndex)
{
	return propagatePoll(false, nvIndex);
}

boolean LtDeviceStack::poll(int nvIndex)
{
	return propagatePoll(true, nvIndex);
}

boolean LtDeviceStack::propagate(LtNetworkVariable* pNv, int arrayIndex, LtMsgOverride* pOverride)
{
    return propagatePoll(false, pNv, arrayIndex, pOverride);
}
    
boolean LtDeviceStack::poll(LtNetworkVariable* pNv, int arrayIndex, LtMsgOverride* pOverride)
{
    return propagatePoll(true, pNv, arrayIndex, pOverride);
}

boolean LtDeviceStack::isBound(LtNetworkVariable* pNv, int arrayIndex, int flags)
{
	return getNetworkImage()->nvTable.isBound(pNv->getNvIndex() + arrayIndex, flags);
}

void LtDeviceStack::goOfflineConditional() 
{
    if (m_bGoOfflineConditional) {
        m_bOffline = true;
        m_bGoOfflineConditional = false;
    }
}

void LtDeviceStack::resetNotConfigured() 
{
    m_bGoOfflineConditional = true;
}

boolean LtDeviceStack::getOffline() 
{
    return m_bOffline;
}

void LtDeviceStack::setOffline(boolean offline) 
{
	m_bOffline = offline;
    getLayer4()->setOffline(offline);
	setLreStateInfo();
#if PRODUCT_IS(ILON)
	triggerPropertyChangeHostEvent();
#endif
}

void LtDeviceStack::servicePinDepressed() 
{
    m_bServicePinDepressed = true;
    eventNotification();
}

void LtDeviceStack::servicePinReleased() 
{
    m_bServicePinReleased = true;
    eventNotification();
}

void LtDeviceStack::sendServicePinMessage()
{
	getLayer4()->sendServicePinMessage();
}

LtErrorType LtDeviceStack::sendToXdriver(byte xDriverCommand, void *pData, int len)
{
    return getLayer4()->sendToXdriver(xDriverCommand, pData, len);
}

void LtDeviceStack::flushCompleted() 
{
    m_bFlushCompleted = true;
    eventNotification();
}

void LtDeviceStack::resetOccurred() 
{	
    m_bResetOccurred = true;
    eventNotification();
}

#if PRODUCT_IS(ILON)
// Provid a way to notify the host app of certain configuration changes.
void LtDeviceStack::triggerPropertyChangeHostEvent()
{
	m_propertyChangeHostEventFlag = true;
    eventNotification();
}
#endif

/* Old debug values
int maxQCount = 0;
int lastQCount = 0;
*/
void LtDeviceStack::receive(LtApduIn* pApdu) 
{
	LtMsgTag* pTag = (LtMsgTag*) pApdu->getRefId().getRefId();
	if ( !(pTag == null && m_pLocalOpMsgTag == null) && pTag == m_pLocalOpMsgTag)
	{
		processApdu(pApdu);
	}
	else
	{
		// Deliver apdu via a message queue.
		/* Old debug code
		int num = msgQNumMsgs(m_queApdus);
		if (num > maxQCount) maxQCount = num;
		lastQCount = num;
		*/
		if (msgQSend(m_queApdus, (char*) &pApdu, sizeof(pApdu), NO_WAIT, MSG_PRI_NORMAL) == ERROR)
		{
			// Delete failed messages
			delete pApdu;
		}
		eventNotification();
	}
}

void LtDeviceStack::resetApduQueue()
{
	if (m_queApdus != null)
	{
		// Drain the old queue and delete it
		LtApduIn* pApdu;
		while (msgQReceive(m_queApdus, (char*) &pApdu, sizeof(pApdu), NO_WAIT) == sizeof(pApdu))
		{
			delete pApdu;
		}
		msgQDelete(m_queApdus);
	}
	// Create a new queue
	m_queApdus = msgQCreate(
		maxApdu +							// General limit, though can be exceeded by turnaround
		maxMsg[LT_NON_PRIORITY_BUFFER] +	// These two chunks are for turnaround messages
		maxMsg[LT_PRIORITY_BUFFER] +
		10, // for margin
		sizeof(void*), MSG_Q_FIFO);
}

void LtDeviceStack::setMessageEventMaximum(int count) 
{
    maxApdu = count;
	resetApduQueue();
}

LtApduIn* LtDeviceStack::getApdu(boolean bResponse, boolean bIgnoreThrottle) 
{
	LtApduIn* pApdu = null;
    if (bIgnoreThrottle || msgQNumMsgs(m_queApdus) < maxApdu) 
	{
		if (bResponse)
		{
			pApdu = new NOTHROW LtRespIn();
		}
		else
		{
			pApdu = new NOTHROW LtMsgIn();
		}
		vxlMemoryCheck(pApdu);
    }
	if (pApdu == null)
	{
		getNetworkStats()->bump(LT_LOST_MESSAGES);
	}
    return pApdu;
}

void LtDeviceStack::setMessageOutMaximum(int count, int countPri) 
{
    maxMsg[LT_NON_PRIORITY_BUFFER] = count;
    maxMsg[LT_PRIORITY_BUFFER] = countPri;
	resetApduQueue();
}

LtMsgOut* LtDeviceStack::msgAlloc() 
{
    return msgAllocGen(false);
}

LtMsgOut* LtDeviceStack::msgAllocPriority() 
{
    return msgAllocGen(true);
}

void LtDeviceStack::send(LtMsgOut* msg, boolean throttle) 
{
    sendMessage((LtApduOut*) msg, true /*wait*/, throttle);
}

void LtDeviceStack::sendMessage(LtApduOut* pApdu, boolean wait, boolean throttle)
{
	LtLayer6::send(pApdu, wait, throttle);
}

void LtDeviceStack::cancel(LtMsgOut* msg) 
{
	release(msg);
}

LtRespOut* LtDeviceStack::respAlloc(LtMsgIn* pMsg) 
{
	LtRespOut* pResp = null;
    if (adjustBufLimit(pMsg->getOrigPri(), true))
    {
		pResp = new NOTHROW LtRespOut(pMsg);
		vxlMemoryCheck(pResp);
    }
	return pResp;
}

LtRespOut* LtDeviceStack::respAlloc(boolean priority, LtBlob *pBlob)
{
	LtRespOut* pResp = null;
    if (adjustBufLimit(priority, true))
    {
		pResp = new NOTHROW LtRespOut(priority, *pBlob);
		vxlMemoryCheck(pResp);
    }
	return pResp;
}

void LtDeviceStack::send(LtRespOut* resp) 
{
	boolean bPri = resp->getOrigPri();
    sendMessage((LtApduOut*) resp, true /* wait*/, false /* don't throttle */);
    // It would be better to reduce the send count when the response is sent in layer4.
    release(bPri);
}

void LtDeviceStack::cancel(LtRespOut* resp) 
{
	if (resp != null)
	{
		boolean bPriority = resp->getOrigPri();
		delete resp;
		release(bPriority);
	}
}

void LtDeviceStack::sendResponse(LtMsgIn* pRequest, byte* pData, int nLen) 
{
	LtRespOut* pResp = respAlloc(pRequest);
	if (pResp != null)
	{
		pResp->setCodeAndData(pData, nLen);
		send(pResp);
	}
}

/**
 * This method clears a subset of the status information kept by the LonTalkstack->
 */
LtErrorType LtDeviceStack::clearStatus() 
{
	// Because of differences in the L2 and L5 MIPs, send a local NM message
	// to clear the status
	return localStackOperation(LT_CLEAR_STATUS, LT_UNACKD);
}

/**
 * This method gets the readOnlyData.
 * @param readOnlyData
 *                  Byte array large enough to accommodate data (41 bytes).
 */
LtErrorType LtDeviceStack::getReadOnlyData(byte* pReadOnlyData) 
{
	return getLayer4()->getReadOnlyData(pReadOnlyData);
}

/**
 * This method resets Layer7 of the protocol stack
 */
void LtDeviceStack::reset() 
{
    m_bResetOccurred = false;
    m_bOffline = false;
    m_bGoOfflineConditional = false;
    m_bServicePinDepressed = false;
    m_bServicePinReleased = false;
    m_bFlushCompleted = false;
	m_nPersistenceLost = LT_PERSISTENCE_OK;
	setOffline(false);
#if PRODUCT_IS(ILON)
	m_propertyChangeHostEventFlag = false;
#endif
}

/**
 * This method controls whether the network image is locked or not.  If locked,
 * the network image can only be changed if the device is offline.
 * @param locked
 *                  true means network image is locked.
 */
void LtDeviceStack::setEepromLock(boolean locked) 
{
    getNetworkStats()->setEepromLock(locked);
}

/**
 * This method sets the LonTalk error log.
 * @param errorNum
 *                  the error number (1..127).
 */
void LtDeviceStack::setErrorLog(int errorNum) 
{
    if (errorNum >= 128) 
	{
        errorNum = LT_BAD_ERROR_NO;
    }
    putErrorLog(errorNum);
}

/**
 * This method gets the LonTalk device status.
 * @param status
 *                  the status structure to fill in.
 */
LtErrorType LtDeviceStack::retrieveStatus(LtStatus& status) 
{
	// Because the status retrieval differs on L2 vs L5 MIPs, the
	// easiest way to do this is to send a local NM message.  This
	// is a little less efficient on a L2 implementation but so what.
	LtErrorType err = localStackOperation(LT_QUERY_STATUS, LT_REQUEST);

	// Either timed out or got a status.  On timeout return previous values,
    // with success == FALSE.
	status = m_currentStatus;

    return err;
}

LtErrorType LtDeviceStack::localStackOperation(int code, LtServiceType st)
{
    LtErrorType err = LT_NO_RESOURCES;

    semTake(m_localOpSem, WAIT_FOREVER);

    m_localOpErr = err;

	m_pLocalOpMsgTag = new NOTHROW LtMsgTag();

	if (m_pLocalOpMsgTag != NULL)
	{
		// We don't want to have to deal with no buffers so force allocation
		LtMsgOut* pMsgOut = msgAllocGen(false, true);
		if (pMsgOut != NULL)
		{
			pMsgOut->setTag(*m_pLocalOpMsgTag);
			pMsgOut->setCode(code);
			pMsgOut->setServiceType(st);
			pMsgOut->setLocal();
			pMsgOut->setRetry(3);
			pMsgOut->setTxTimer(500);
			send(pMsgOut, false /* Don't throttle local stack operations */);

			while (m_pLocalOpMsgTag && !m_shutDown)
			{
				taskDelay(msToTicks(20));
			}
			err = m_localOpErr;
		}
		delete m_pLocalOpMsgTag;    // Normally deleted by stack threads, but clean up in case of
									// shutdown.
		m_pLocalOpMsgTag = NULL;
    }

	semGive(m_localOpSem);

    return err;
}

/**
 * This method sets the service LED.
 * @param ledOn
 *                  true means on solid, else off.
 */
void LtDeviceStack::setServiceLed(boolean ledOn) 
{
	setServiceOverride(ledOn);
	LonTalkNode::setServiceLed();
}
void LtDeviceStack::winkServiceLed()
{
	LonTalkNode::winkServiceLed();
}

/**
 * This method kicks off a flush process.  Once complete, a flushCompletes
 * indication is provided (via Application interface).
 * @param commIgnore
 *                  true means ignore incoming messages while flushing.
 */
void LtDeviceStack::flush(boolean commIgnore) 
{
    LonTalkStack::flush(commIgnore);
}

/**
 * This method cancels an outstanding flush operation.
 */
void LtDeviceStack::flushCancel() 
{
    LonTalkStack::flushCancel();
}

/**
 * This method allows the application to take itself offline.
 */
void LtDeviceStack::goOffline() 
{
    setOffline(true);
    lonApp->offline();
}

void LtDeviceStack::goConfigured()
{
	if (getNetworkImage()->unconfigured())
	{
		changeState(LT_CONFIGURED, false);
	}
}

void LtDeviceStack::goUnconfigured() 
{
	if (getNetworkImage()->initialize(LT_UNCONFIGURED) == LT_NO_ERROR)
	{
		getLayer4()->setResetRequested();
	}
#if PRODUCT_IS(ILON)
	triggerPropertyChangeHostEvent();
#endif
}

LtErrorType LtDeviceStack::changeState(int newState, boolean bClearKeys)
{ 
	LtErrorType	err;

	err = getLayer4()->changeState(newState, bClearKeys);
#if PRODUCT_IS(ILON)
	triggerPropertyChangeHostEvent();
#endif
	return err; 
}

void LtDeviceStack::shutDown() 
{
	stop();
    LonTalkStack::shutDown();
	getLayer4()->shutdown();
    LtLtLogicalChannel* pLtChannel = getChannel()->getLtLtLogicalChannel();
    if (pLtChannel != NULL)
    {
        pLtChannel->deregisterStack(getIndex());
    }
    m_shutDown = TRUE;
}

LtErrorType LtDeviceStack::getSubnetNode(int domainIndex, int subnetNodeIndex, 
										 LtSubnetNode &subnetNode)
{
	LtDomainConfiguration dc;
	LtErrorType err = getDomainConfiguration(domainIndex, &dc);
	if (err == LT_NO_ERROR)
	{
		err = dc.getSubnetNode(subnetNodeIndex, subnetNode);
	}
	return err;
}

LtErrorType LtDeviceStack::getDomainConfiguration(int nIndex, LtDomainConfiguration* pDc)
{
    return getLayer4()->getDomainConfiguration(nIndex, pDc);
}

LtErrorType LtDeviceStack::getAddressConfiguration(int nIndex, LtAddressConfiguration* pAc)
{
    return getLayer4()->getAddressConfiguration(nIndex, pAc);
}

LtDomainConfiguration* LtDeviceStack::allocDomainConfiguration(int index)
{
    LtSubnetNodeClient* pClient;
	LtUniqueId uid;

	getPlatform()->getUniqueId(&uid);
	if (isNodeStack())
	{
		pClient = new LtSubnetNodeClient(this, getChannel(), &uid);
	}
	else
	{
		pClient = new LtRouterDomainClient(this, getChannel(), &uid);
	}

	if (getLre()) getLre()->registerClient(pClient);

    return pClient;
}

LtErrorType LtDeviceStack::updateDomainConfiguration(int nIndex, LtDomainConfiguration* pDomain, boolean bStore, boolean bRestore)
{
	LtErrorType err = getLayer4()->updateDomainConfiguration(nIndex, pDomain, bStore, bRestore);
	notify();
#if PRODUCT_IS(ILON)
	triggerPropertyChangeHostEvent();
#endif
	return err;
}

// Beware that this API does not provide persistence for the alternate addresses.
LtErrorType LtDeviceStack::updateSubnetNode(int domainIndex, int subnetNodeIndex, 
										    const LtSubnetNode &subnetNode)
{
	LtErrorType err = getLayer4()->updateSubnetNode(domainIndex, subnetNodeIndex, subnetNode);
	notify();
#if PRODUCT_IS(ILON)
	triggerPropertyChangeHostEvent();
#endif
	return err;
}

LtErrorType LtDeviceStack::updateAddressConfiguration(int nIndex, LtAddressConfiguration* pAddress, boolean bRestore)
{
	LtErrorType err = getLayer4()->updateAddressConfiguration(nIndex, pAddress, bRestore);
	notify();
	return err;
}

LtErrorType LtDeviceStack::getConfigurationData(byte* pData, int offset, int length)
{
	return getLayer4()->getConfigurationData(pData, offset, length);
}
LtErrorType LtDeviceStack::updateConfigurationData(byte* pData, int offset, int length)
{
	return getLayer4()->updateConfigurationData(pData, offset, length);
}

void LtDeviceStack::loadAdditionalConfig(LtNetworkImageVersion ver, byte *pImage, int &offset)
{
    if (ver >= NetImageVer_LsEnhancedMode)
    {
        offset += lsAddrMappingConfigFromLonTalk(&pImage[offset]);
    }
}

void LtDeviceStack::storeAdditionalConfig(byte *pImage, int &offset)
{
    offset += lsAddrMappingConfigToLonTalk(&pImage[offset]);
}

int LtDeviceStack::getAdditionallConfigStoreSize(void)
{
    return lsAddrMappingConfigToLonTalk(NULL);  
}

    // Set the lsMappingConfig and return the length.
int LtDeviceStack::lsAddrMappingConfigFromLonTalk(const byte *pSerializedData)
{
    PTOHL(pSerializedData, m_lsAddrMappingAnnounceFreq);
    PTOHS(pSerializedData, m_lsAddrMappingAnnounceThrottle);
    PTOHL(pSerializedData, m_lsAddrMappingAgeLimit);

    LtLtLogicalChannel* pLtChannel = getChannel()->getLtLtLogicalChannel();
    if (pLtChannel != NULL)
    {
        pLtChannel->setLsAddrMappingConfig(getIndex(), 
                                           m_lsAddrMappingAnnounceFreq, 
                                           m_lsAddrMappingAnnounceThrottle, 
                                           m_lsAddrMappingAgeLimit);
    }

    return sizeof(m_lsAddrMappingAnnounceFreq) + 
           sizeof(m_lsAddrMappingAnnounceThrottle) + 
           sizeof(m_lsAddrMappingAgeLimit);
}
    // Get the lsMappingConfig and return the length. If pSerializedData is NULL, just get the length
int LtDeviceStack::lsAddrMappingConfigToLonTalk(byte *pSerializedData)
{
    if (pSerializedData != NULL)
    {
        PTONL(pSerializedData, m_lsAddrMappingAnnounceFreq);
        PTONS(pSerializedData, m_lsAddrMappingAnnounceThrottle);
        PTONL(pSerializedData, m_lsAddrMappingAgeLimit);
    }
    return sizeof(m_lsAddrMappingAnnounceFreq) + 
           sizeof(m_lsAddrMappingAnnounceThrottle) + 
           sizeof(m_lsAddrMappingAgeLimit);
}

#if FEATURE_INCLUDED(IZOT)
    // Query this devices IP address.  Note that the device might have more than one
    // IP address, so the destination addressing is taken into account
int LtDeviceStack::queryIpAddr(LtApduIn* pApduIn, byte *arbitraryIpAddr)
{
    LtLtLogicalChannel* pLtChannel = getChannel()->getLtLtLogicalChannel();
    if (pLtChannel != NULL && pLtChannel->getIsIzoTChannel())
    {
        LtDomainConfiguration *pDc;
        LtSubnetNode subnetNode(0,0);
        pDc = &pApduIn->getDomainConfiguration();
        if (pApduIn->getAddressFormat() == LT_AF_SUBNET_NODE)
        {
            subnetNode = pApduIn->getSubnetNode();
        }
        else
        {
            int lowerLimit = pDc->getIndex();
            int upperLimit;
            if (lowerLimit == FLEX_DOMAIN_INDEX)
            {
                lowerLimit = 0;
                upperLimit = getNetworkImage()->domainTable.getCount();
            }
            else
            {
                upperLimit = lowerLimit;
            }
            for (int i = lowerLimit; i <= upperLimit; i++)
            {
                LtDomainConfiguration *pDc2;
                if (getNetworkImage()->domainTable.get(i, &pDc2) == LT_NO_ERROR &&
                    pDc2->getSubnetNode(0, subnetNode) == LT_NO_ERROR)
                {
                    // Found it...
                    pDc = pDc2;
                    break;
                }
            }
        }
        return pLtChannel->queryIpAddr(pDc->getDomain(), subnetNode.getSubnet(), subnetNode.getNode(), arbitraryIpAddr); 
    }
    else
    {
        return 0;
    }
}
#endif

boolean LtDeviceStack::hasMatchingSrcDomainConfiguration(LtDomainConfiguration& srcDc)
{
	LtDomainConfiguration* pTgtDc;
	boolean bMatch = false;

	pTgtDc = getNetworkImage()->domainTable.get(&srcDc);
	if ((pTgtDc != NULL) && pTgtDc->equals(srcDc, true))
	{
		bMatch = true;
	}
	return bMatch;
}

LtErrorType LtDeviceStack::getOverrideDefaults(int addressIndex, LtMsgOverride* pMsgOverride)
{
	LtAddressConfiguration ac;
	LtErrorType err = getAddressConfiguration(addressIndex, &ac);
	if (err == LT_NO_ERROR)
	{
		LtMsgOverrideOptions options(LT_OVRD_TX_TIMER | LT_OVRD_RPT_TIMER | LT_OVRD_RETRY_COUNT);
		LtMsgOverride msgOverride(options, LT_UNACKD, false, 
			ac.getTxTimer(), ac.getRptTimer(), ac.getRetry());
		pMsgOverride->setDefaults(&msgOverride);
	}
	return err;
}

LtErrorType LtDeviceStack::getBoundNvConfiguration(LtNetworkVariable* pNv, int arrayIndex, LtNetworkVariableConfiguration* &pNvc)
{
	int nvIndex = pNv->getNvIndex() + arrayIndex;
	LtErrorType err = getNetworkImage()->nvTable.getNv(nvIndex, &pNvc);
	if (err == LT_NO_ERROR)
	{
        if (!pNvc->isBound())
		{
			// Get the first alias configuration
			LtVectorPos pos;
			int index;
			if (!pNvc->nextAlias(pos, index))
			{
				err = LT_NOT_FOUND;
			}
			else
			{
				err = getNetworkImage()->nvTable.get(index, &pNvc);
				if (err == LT_NO_ERROR && 
					!pNvc->isBound())
				{
					err = LT_NOT_FOUND;
				}
			}
		}
	}
	return err;
}

LtApduOut* LtDeviceStack::nvResponse(LtApduIn* pApduIn, byte* pData, int nLen)
{
	LtApduOut* pApduOut = new NOTHROW LtApduOut(pApduIn);
	vxlMemoryCheck(pApduOut);

	if (pApduOut != null)
	{
		*(LtApdu*) pApduOut = *(LtApduIn*) pApduIn;

		pApduOut->flipNvCode();
		pApduOut->setServiceType(LT_RESPONSE);
		if (nLen)
		{
			pApduOut->setNvData(pData, nLen);
		}
    }
	return pApduOut;
}

boolean LtDeviceStack::isMyAddress(int domainIndex, LtSubnetNode &subnetNode)
{
	boolean bIsMine = false;
	LtDomainConfiguration dc;
	if (getDomainConfiguration(domainIndex, &dc) == LT_NO_ERROR)
	{
		if (dc.getDomain().inUse())
		{
			int subnetNodeIndex = 0;
			LtSubnetNode mySubnetNode;
			while (dc.getSubnetNode(subnetNodeIndex++, mySubnetNode) == LT_NO_ERROR)
			{
				if (mySubnetNode.inUse() && subnetNode == mySubnetNode)
				{
					bIsMine = true;
					break;
				}
			}
		}
	}
	return bIsMine;
}

boolean LtDeviceStack::getAddress(int& nIndex, LtDomain* pDomain, 
								  LtSubnetNode* pAddress, LtGroups* pGroups)
{
	LtDomainConfiguration dc;
	int domainIndex = nIndex & 0xffff;
	int subnetNodeIndex = nIndex >> 16;
	while (getDomainConfiguration(domainIndex, &dc) == LT_NO_ERROR)
	{
		if (dc.getDomain().inUse())
		{
			if (dc.getSubnetNode(subnetNodeIndex++, *pAddress) == LT_NO_ERROR &&
				pAddress->inUse())
			{
	 			*pDomain = dc.getDomain();
				getNetworkImage()->addressTable.getGroups(dc.getIndex(), *pGroups);
				nIndex = domainIndex | (subnetNodeIndex << 16);
				return true;
			}
		}
		domainIndex++;
		subnetNodeIndex = 0;
	}
	return false;
}

boolean LtDeviceStack::getAddress(int& nIndex, LtUniqueId* pUniqueId)
{
	LtUniqueIdClient *pClient;

	pClient = (LtUniqueIdClient*)getMainClient();
	if (pClient != NULL)
	{
		*pUniqueId = *(LtUniqueId*)pClient;
	}
	return ((nIndex++ == 0) && (pClient != NULL));
}

boolean LtDeviceStack::getNeedAllBroadcasts()
{
	boolean bNeedAllBroadcasts = getNetworkImage()->unconfigured() || getReceiveAllBroadcasts();
	return bNeedAllBroadcasts;
}

LtErrorType LtDeviceStack::getRoute(int nDomainIndex, boolean bEeprom, LtRoutingMap* pRoute)
{
	LtErrorType err = LT_NO_ERROR;
	LtDomainConfiguration* pDc;

	err = getLayer4()->getDomainConfiguration(nDomainIndex, &pDc);
	if (err == LT_NO_ERROR)
	{
		LtRouterDomainClient* pClient = (LtRouterDomainClient*) pDc;
		pClient->getRoute(bEeprom, pRoute);
	}
	return err;
}

boolean LtDeviceStack::loadRoutes(byte* pData)
{
	LtRoutingMap rm;
	int index = 0;
	boolean valid = true;

	while (valid && getRoute(index, true, &rm) == LT_NO_ERROR)
	{
		pData += rm.load(pData);
		valid = rm.isValid();
		if (valid)
		{
			setRoute(index, true, &rm);
		}
		index++;
	}
	return valid;
}

void LtDeviceStack::storeRoutes(byte* pData)
{
	LtRoutingMap rm;
	int index = 0;

	while (getRoute(index, true, &rm) == LT_NO_ERROR)
	{
		pData += rm.store(pData);
		index++;
	}
}

int LtDeviceStack::getRoutesStoreSize()
{
	int len = 0;
	int index = 0;
	LtRoutingMap rm;

	while (getRoute(index, true, &rm) == LT_NO_ERROR)
	{
		len += rm.getStoreSize();
		index++;
	}
	return len;
}

LtErrorType LtDeviceStack::setRoute(int nDomainIndex, boolean bEeprom, LtRoutingMap *pMap)
{
	LtErrorType err = LT_NO_ERROR;
	int limit = bEeprom ? 2 : 1;

	for (int i = 0; i < limit; i++)
	{
		if (err == LT_NO_ERROR)
		{
			LtDomainConfiguration* pDc;

			err = getLayer4()->getDomainConfiguration(nDomainIndex, &pDc);
			if (err == LT_NO_ERROR)
			{
				LtRouterDomainClient* pClient = (LtRouterDomainClient*) pDc;
				err = pClient->setRoute(pMap, bEeprom);
			}
			bEeprom = false;
		}
	}

	// Notify router event clients
	notify();

	return err;
}

void LtDeviceStack::notifyPersistenceLost(LtPersistenceLossReason reason)
{
	if (reason != LT_NO_PERSISTENCE)
	{
		m_nPersistenceLost = reason;
		if (reason == LT_PERSISTENT_WRITE_FAILURE)
		{
			LonTalkNode::errorLog(LT_EEPROM_WRITE_FAILURE);
		}
		eventNotification();
	}
}

void LtDeviceStack::setPersistencePath(const char* szPath)
{
    m_szPersistencePath = new char[strlen(szPath) + 1];
    strcpy(m_szPersistencePath, szPath);
    getPlatform()->setErrorLogPath(szPath);

#if PERSISTENCE_TYPE_IS(STANDARD)
    getNetworkImage()->getPersistence()->setPath(szPath);
	getPersistence()->setPath(szPath);
#endif

#if FEATURE_INCLUDED(MONITOR_SETS)
    m_monitorSetTable.setPath(szPath);
	m_monitorPointTable.setPath(szPath);
#endif
}

#if PERSISTENCE_TYPE_IS(FTXL)
void LtDeviceStack::setAppSignature(unsigned appSignature)
{
    getNetworkImage()->getPersistence()->setAppSignature(appSignature);
    getPersistence()->setAppSignature(appSignature);

#if FEATURE_INCLUDED(MONITOR_SETS)
    m_monitorSetTable.setAppSignature(appSignature);
	m_monitorPointTable.setAppSignature(appSignature);
#endif
}

unsigned LtDeviceStack::getAppSignature()
{
    return getPersistence()->getAppSignature();
}

void LtDeviceStack::setPeristenceGaurdBand(int flushGuardTimeout)
{
    getNetworkImage()->getPersistence()->setHoldTime(flushGuardTimeout);
    getPersistence()->setHoldTime(flushGuardTimeout);

#if FEATURE_INCLUDED(MONITOR_SETS)
    m_monitorSetTable.setHoldTime(flushGuardTimeout);
	m_monitorPointTable.setHoldTime(flushGuardTimeout);
#endif
}
#endif

void LtDeviceStack::resetPersistence()
{
	getNetworkImage()->getPersistence()->resetPersistence();
	getPersistence()->resetPersistence();
#if FEATURE_INCLUDED(MONITOR_SETS)
    m_monitorSetTable.getPersistence()->resetPersistence();
    m_monitorPointTable.getPersistence()->resetPersistence();
#endif
}


#if FEATURE_INCLUDED(MONITOR_SETS)

// Monitor Set methods
// To iterate, start with 0 and then send in the returned set's
// index + 1.
LtMonitorSet *LtDeviceStack::getNextMonitorSet(LtMsIndex msIndex)
{
    return m_monitorSetTable.getNext(msIndex, LT_MC_VALID_DESCRIPTION_ONLY);
}

LtMonitorSet *LtDeviceStack::getMonitorSet(LtMsIndex afterMsIndex)
{
    return m_monitorSetTable.get(afterMsIndex, LT_MC_VALID_DESCRIPTION_ONLY);
}
// End of monitor set interfaces
#endif

void LtRouterStack::resetRoutes()
{
	// EEPROM routes become "RAM" routes on reset, thus wiping out any "temporary"
	// RAM settings.  Force learning routers to flood.  
	int i = 0;
	LtRoutingMap route;

	while (getRoute(i, true, &route) == LT_NO_ERROR)
	{
		if (route.getRouterType() == LT_LEARNING_ROUTER)
		{
			route.getSubnets().setAll();
		}
		setRoute(i++, false, &route);
	}
	setLreStateInfo();
}

void LtRouterStack::routerModeChange()
{
	// A router mode change only requires notification of one domain client.  This
	// is because the router mode is shared among all domains.
	LtDomainConfiguration* pDc;
	if (getLayer4()->getDomainConfiguration(0, &pDc) == LT_NO_ERROR)
	{
		LtRouterDomainClient* pClient = (LtRouterDomainClient*) pDc;
		pClient->notify();
		notify();
#if PRODUCT_IS(ILON)
		triggerPropertyChangeHostEvent();
#endif
	}
}

void LtRouterStack::setLreStateInfo()
{
	// Routers sides must update LRE state when they go unconfigured or offline.
	if (getLre()) 
		getLre()->setOffline(getOffline() || (getNetworkImage()->getState() != LT_CONFIGURED),
							 getLonTalkChannel());
	notify();
}

// LtLreClient method for retrieval of route.  The getExternalRoute method
// returns the subnet map based on the subnets which are known to reside 
// on the other side of the router.  In essence, this map translates directly
// into what is reported via LT/IP RFC messages.  For a configured router,
// the route is the routing table as configured for this router side by
// the network manager.  For a learning router, this is the subnet mask
// of the learned subnets as learned by the engine.    

// Used to get the route(s) for this router half.  These are
// fetched by the LTIP Control module to determine the routes to report
// to the other LTIP routers.  
boolean LtRouterStack::getExternalRoute(int& nIndex, LtRoutingMap* pRoute, int* pRoutingSubnet)
{
	while (!getNetworkImage()->unconfigured() &&
		   LtDeviceStack::getRoute(nIndex++, false, pRoute) == LT_NO_ERROR)
	{
		// Only return routes for configured domains
		if (pRoute->getDomain().isValid())
		{
			switch (pRoute->getRouterType())
			{
			case LT_BRIDGE:
				pRoute->getSubnets().setAll();
				pRoute->getGroups().setAll();
				break;
			case LT_LEARNING_ROUTER:
				pRoute->getGroups().setAll();
				break;
			default:
				break;	// nothing
			}
			return true;
		}
	}
	return false;
}

void LtRouterStack::eventNotification(LtEventServer* pServer)
{
	if (pServer == getLre())
	{
		// Log any subnet partition error
		int err = getLre()->getErrorLog();
		if (err)
		{
			errorLogConditional(err);
		}

		// Get each route and update them as necessary based on the changes in the LRE.
		// Should only occur for learning routers.
		int nIndex = 0;
		LtRoutingMap route;
		while (LtDeviceStack::getRoute(nIndex, false, &route) == LT_NO_ERROR)
		{
			if (route.getRouterType() == LT_LEARNING_ROUTER &&
				route.getDomain().isValid())
			{
				LtSubnets subnets;

				// Get the LREs updated view of this route
				if (getLre()->getExternalRoute(route.getDomain(), getChannel()->getLonTalk(), &subnets) == true)
				{
					route.setSubnets(subnets);
					setRoute(nIndex, false, &route);
				}
			}
			nIndex++;
		}
	}
}

LtErrorType LtDeviceStack::initialize(int startIndex, int endIndex, byte* pData, int len, int domainIndex)
{
	LtErrorType err = LT_NO_ERROR;

	// Notify the application that we are starting to "initialize".
	lonApp->initializing();

    int         state = 0;
    int         options = 0;
    if (len > 0)
    {
	    PTOHB(pData, state);
        if (--len > 0)
        {
            PTOHB(pData, options);
            --len;
        }
    }


#if FEATURE_INCLUDED(MONITOR_SETS)
    // First clear out monitor points, monitor sets and dynamic
	// NVs.  
	err = m_monitorSetTable.initialize(0, NM_MAX_INDEX, NULL, 0, domainIndex);

	if (err == LT_NO_ERROR)
	{
		err = m_monitorPointTable.initialize(0, NM_MAX_INDEX, NULL, 0, domainIndex);
	}
#endif

	if (err == LT_NO_ERROR)
	{
		if (getDynamicNvs()->remove(getStaticNetworkVariableCount(), getMaximumDynamicNetworkVariableCount()) != ND_OK)
		{
			// If it fails, treat as bad NV index.
			err = LT_INVALID_NV_INDEX;
		}
	}

	// Now clear out the network image.
	err = getNetworkImage()->initialize(state, options, domainIndex);

	return err;
}

LtErrorType LtDeviceStack::enumerate(int index, boolean authenticated, LtApdu &response)
{
	return LT_NOT_IMPLEMENTED;
}

LtErrorType LtDeviceStack::checkLimits(int cmd, byte* pData, int len)
{
	boolean ok = false;
	switch (cmd)
	{
		case LT_NM_INITIALIZE:
			ok = (len >= 1) && (len <= 2);
			break;
		default:
			ok = LtConfigurationEntity::checkLimits(cmd, pData, len) == LT_NO_ERROR;
			break;
	}
	return ok ? LT_NO_ERROR : LT_INVALID_PARAMETER;
}

// get the lowest & highest layers implemented on the host. 
// 3,6 = Vni running on a Layer 2 mip.  6,6 = VNI running on Layer 5 mip.
void LtDeviceStack::getLayerRange(int &min, int &max)
{
	min = 3;
	max = 6;
	if (getChannel()->isLayer5Interface())
	{
		min = 6;
	}
}

void LtDeviceStack::getNmVersion(int &nmVer, int &capabilities)
{
    getLayer4()->getNmVersion(nmVer, capabilities);
}

LtErrorType LtDeviceStack::disableOma()
{
    LtErrorType err;
    if (getApp() != NULL)
    {   // Must be called before APP is registered
        err = LT_INVALID_STATE;
    }
    else
    {
        err = getLayer4()->disableOma();
    }
    return err;
}

LtErrorType LtDeviceStack::setNmVersionOverride(int nmVersion, int nmCapabilities)
{
    LtErrorType err;
    if (getApp() != NULL)
    {   // Must be called before APP is registered
        err = LT_INVALID_STATE;
    }
    else
    {
        err = getLayer4()->setNmVersionOverride(nmVersion, nmCapabilities);
    }
    return err;
}

boolean LtDeviceStack::omaSupported()
{
    return getLayer4()->omaSupported();
}



boolean LtDeviceStack::getReceiveNsaBroadcasts()
{
	// Only this is needed to receive NSA broadcasts
	return getLayer4()->getReceiveAllBroadcasts();
}

void LtDeviceStack::setReceiveNsaBroadcasts(boolean bValue)
{
    // If L6 has been set to receive all broadcasts (not just NSA ones), don't change this
	if (!LtLayer6::getReceiveAllBroadcasts())
	{
		// L4 will receive all broadcasts.  Its up to layer 6 to filter out the non-NSA messages.
		getLayer4()->setReceiveAllBroadcasts(bValue);
		notify();
	}
}

boolean LtDeviceStack::getReceiveAllBroadcasts()
{
	// Both are required to receive all broadcasts
	return (LtLayer6::getReceiveAllBroadcasts() && getLayer4()->getReceiveAllBroadcasts());
}

// Really receive all broadcasts, not just NSA ones
void LtDeviceStack::setReceiveAllBroadcasts(boolean bValue)
{
    // Will receive all broadcasts. 
	LtLayer6::setReceiveAllBroadcasts(bValue);
	getLayer4()->setReceiveAllBroadcasts(bValue);
	notify();
}

LtErrorType LtDeviceStack::getReadOnlyData(LtReadOnlyData* pReadOnlyData)
{
	LtErrorType err = LT_NO_RESOURCES;
	byte* pRod = new NOTHROW byte[pReadOnlyData->getLength()];
	if (pRod != NULL)
	{
		err = getReadOnlyData(pRod);
		pReadOnlyData->fromLonTalk(0, pReadOnlyData->getLength(), pRod);
		delete pRod;
	}
    return err;
}

LtErrorType LtDeviceStack::waitForPendingInterfaceUpdates(void)
{
	return getLayer4()->waitForPendingInterfaceUpdates();
}

void LtDeviceStack::registerHookedMessage(int size, const byte *pMsgData) 
{
    LtMsgFilter* pMsgFilter = new LtMsgFilter(size, pMsgData);
    m_vecHookedMsgs.addElement(pMsgFilter);
}

void LtDeviceStack::deregisterHookedMessage(int size, const byte *pMsgData) 
{
    if (size == 0)
    {
        // Clear all
        m_vecHookedMsgs.clear(true);
    }
    else
    {
        LtMsgFilter* pMsgFilter = findMatchingFilter(size, pMsgData, true);
        if (pMsgFilter != NULL)
        {
            m_vecHookedMsgs.removeElement(pMsgFilter);
        }
    }
}

LtMsgFilter *LtDeviceStack::findMatchingFilter(int size, const byte *pMsgData, boolean exact) 
{
	LtVectorPos pos;
	LtMsgFilter* pMsgFilter;
    while (m_vecHookedMsgs.getElement(pos, &pMsgFilter))
	{
        if (pMsgFilter->matches(size, pMsgData, exact)) 
		{
            return pMsgFilter;
        }
    }
    return NULL;
}

boolean LtDeviceStack::isMsgHooked(LtApdu& apdu) 
{
    return (findMatchingFilter(apdu.getLength(), apdu.getCodeAndData(), FALSE) != NULL);
}

void LtDeviceStack::setServicePinState(LtServicePinState state)
{
    m_nServicePinState = state;
    getApp()->setServiceLedStatus(state);
}

LtMsgFilter::LtMsgFilter(int size, const byte *pMsgData)
{
    m_size = size;
    m_pMsgData = new byte[m_size];
    memcpy(m_pMsgData, pMsgData, m_size);
}

LtMsgFilter::~LtMsgFilter()
{
    delete[] m_pMsgData;
}
    
boolean LtMsgFilter::matches(int size, const byte *pMsgData, boolean exact)
{
    boolean matches = exact ? size == m_size : size >= m_size;
    if (matches)
    {
        matches = (memcmp(m_pMsgData, pMsgData, m_size) == 0);
    }
    return matches;
}
