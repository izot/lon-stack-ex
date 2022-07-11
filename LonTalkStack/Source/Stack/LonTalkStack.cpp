//
// LonTalkStack.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LonTalkStack.cpp#2 $
//

#include "LtStackInternal.h"
#include "LtXcvrId.h"
#include "vxlTarget.h"
#include "LtIpPlatform.h"

// 
// Temporary for IP side apps
//
LtIpPortClient* pLipc = null;

#if PRODUCT_IS(ILON)
const boolean LonTalkStack::m_xidKeyContainsPath = FALSE;
#else
const boolean LonTalkStack::m_xidKeyContainsPath = TRUE;
#endif


//
// Private Member Functions
//

void LonTalkStack::sendServicePinMessage() 
{
    LtUniqueId* pNid = getReadOnly()->getUniqueId();
    LtProgramId* pPid = getReadOnly()->getProgramId();
	LtApduOut* pApdu = new LtApduOut();
	if (pApdu != null)
	{
		pApdu->setNoCompletion(true);
		pApdu->setServiceType(LT_UNACKD);
		pApdu->setCode(LT_SERVICE_PIN);
		pApdu->setData(pNid->getData(), 0, pNid->getLength());
		pApdu->setData(pPid->getData(), pNid->getLength(), pPid->getLength());
		pApdu->setAddressType(LT_AT_BROADCAST);
		pApdu->setSubnet(0);
		LtDomainConfiguration domain;
		domain.getDomain().setZeroLength();
		pApdu->setDomainConfiguration(domain);
		getStack()->sendMessage(pApdu, false);
	}
}

//
// Protected Member Functions
//


void LonTalkStack::halt() 
{
}

void LonTalkStack::resume() 
{
}

void LonTalkStack::readyToReceive() 
{
    // Init (default) comm params
    if (m_bXcvrIdUnknown) 
	{
		setXcvrId(4);
    }
	setComm();
}

LtErrorType LonTalkStack::fetchXcvrReg(byte* data, int offset) 
{
	regValueFailure = false;
    for (int i = 1; !regValueFailure && i <= LT_NUM_REGS; i++) {
        regValueReceived = false;
		LtSts sts = getDriver()->getTransceiverRegister(i);
		if (sts != LTSTS_PENDING && sts != LTSTS_OK)
		{
			regValueFailure = true;
			break;
		}
		int limit = 0;
		while (!regValueReceived && !regValueFailure)
		{
			taskDelay(msToTicks(25));
			if (limit++ > 400)
			{
				regValueFailure = true;
			}
		}
        data[offset + i - 1] = regValue;
    }
	return regValueFailure ? LT_XCVR_REG_OP_FAILURE : LT_NO_ERROR;
}

void LonTalkStack::reset(boolean bBoot) 
{
	if (!bBoot)
	{
	    LonTalkNode::halt();
		halt();
	}
    
    LonTalkNode::reset(bBoot);

    LtReadOnlyData *pReadOnlyData = getStack()->getReadOnly();
	if (!bBoot)
	{
		getDriver()->reset();
		if (pReadOnlyData != NULL && pReadOnlyData->getPendingUpdate())
		{	// Write pending update out to mip
            getDriver()->setNetworkBuffers(*pReadOnlyData);
		}
		// Make sure comm param are set.  If CPs not yet initialized, skip this
		LtCommParams cps;
		getNetworkImage()->configData.getCommParams(cps);
		if (cps.m_nData[0] != 0)
		{
			setDriverComm(&cps);
		}
	}
	if (pReadOnlyData != NULL)
	{	// refresh read only data...
        getDriver()->getNetworkBuffers(*pReadOnlyData);
    }

    LonTalkNode::setServiceLed();
    
	LtPlatform::reset();

	if (!bBoot)
	{
	    LonTalkNode::resume();
		resume();
	}
}

int LonTalkStack::getModeAndState() {
    return getNetworkImage()->getState() | 
           (getStack()->getOffline() ? 0x08 : 0x00); 
}

LtErrorType LonTalkStack::clearStatus() {
    getNetworkStats()->reset();
	clearResetCause();
    clearErrorLog();
    return LT_NO_ERROR;
}

void LonTalkStack::flush(boolean commIgnore) 
{
	getStack()->getLayer4()->setCommIgnore(commIgnore);
    m_bFlushPending = true;
	getStack()->flushCheck();
	// Flushing through the LRE and driver just for this stack is not implemented.
	// How would it even be done anyway?  Tag each message, etc?  Not clear it's
	// worth doing.
}

void LonTalkStack::flushCancel() {
	getStack()->getLayer4()->setCommIgnore(false);
    m_bFlushPending = false;
    //getDriver()->flush(false);
}

void LonTalkStack::terminate() {
	getStack()->getLayer4()->terminate();        
    terminatePending = true;
    getDriver()->terminate();
    while (terminatePending) 
	{
		taskDelay(msToTicks(50));
    }
}

//
// Public Member Functions
//

LonTalkStack::LonTalkStack(LtLogicalChannel* pChannel, LtLreServer* pLre) : m_linkStub(pChannel)
{
    m_bXcvrIdUnknown = true;
    resetRequest = false;
	m_bFlushPending = false;
	terminatePending = false;
	m_pLtpc = null;
	int xcvrId;
    
    lonApp = null;
	m_pStack = null;
	m_pLtpc = null;


	// Obsolete
	//LEDOFF;

	LtMisc::initTrace();
    
	LtLink* pLink = getLink(pChannel);
	if (pLink != null)
	{
		setDriver(pLink);
	}
	else
	{
		setDriver(&m_linkStub);	
	}

	xcvrId = pChannel->getXcvrId();
	if (xcvrId == -1)
	{
		setXcvrId(4);
	}
	else
	{
		setXcvrId(xcvrId);
	}

	LonTalkNode::registration(getDriver(), pLre);

    resume();
}

LonTalkStack::~LonTalkStack()
{
	if (m_pLtpc != null)
	{
		m_pLtpc->removeClient(this);
	}
}

LtLink* LonTalkStack::getLink(LtLogicalChannel* pChannel)
{
	LtLink* pLink = null;
	if (!pChannel->getIpChannel() && !pChannel->isLayer5Interface())
	{
		LtLtLogicalChannel* pLtChannel = (LtLtLogicalChannel*) pChannel;
		m_pLtpc = pLtChannel->getLtpc();
		if (m_pLtpc != null)
		{
			pLink = m_pLtpc->registerClient(this);
		}
	}
	return pLink;
}

void LonTalkStack::setDriverComm(LtCommParams* pCps)
{
    specialPurpose = (pCps->m_nData[1] & 0x40) == 0x40;
    getDriver()->setCommParams(*pCps);
}

void LonTalkStack::setCommParams(LtCommParams* pCps)
{
    LtCommParams oldCps;
    getNetworkImage()->configData.getCommParams(oldCps);
    if (memcmp(oldCps.m_nData, pCps->m_nData, sizeof(oldCps.m_nData)) != 0)
    {   // Comm parameters changed - update them in config data and schedule them to be stored.
        getNetworkImage()->configData.setCommParams(*pCps);
        getNetworkImage()->store();
    }
    setDriverComm(pCps);
}

boolean LonTalkStack::isSpecialPurpose() {
    return specialPurpose;
}

boolean LonTalkStack::getEepromLock() {
    return getNetworkStats()->getEepromLock() && getStack()->getOffline();
}

void LonTalkStack::setXcvrId(int xcvrId) 
{
	m_bXcvrIdUnknown = false;
	m_nXcvrId = xcvrId;
}

#if defined(ILON_PLATFORM) && defined (__VXWORKS__)
extern bool overrideXcvrId;
extern int xcvrIdOverride;
#endif

void LonTalkStack::setComm()
{
#if PRODUCT_IS(FTXL) && !defined(WIN32)
    // FTXL always uses comm parms from config if they exist, 
    // and comm parms from the application if they do not.
    // This function does not need to do anything.
    // Driver will be initialized in reset function.
    // FtxlAPI will set comm parms if not already
    // set later.
#else
	LtCommParams xidCps;	// from xcvr ID table
	LtCommParams cfgCps;	// from config data
	LtCommParams drvCps;	// from driver
	LtCommParams *pCpsToUse;
	LtErrorType xcvrIdErr;
	LtSts drvErr;
    boolean xidIsPersistent = true;
	int oldXid = 0;
#if PRODUCT_IS(ILON)
	char xidKey[20];

	sprintf(xidKey, "xid%d", getStack()->getIndex());
#else
	char xidKey[MAX_PATH];

    if (getStack()->getPersistencePath() == NULL)
    {   // Protocal analyzer, for example, does not require persistence.
        xidIsPersistent = FALSE;
    }
    else
    {
	    sprintf(xidKey, "%s" DIR_SEPARATOR_STRING "xid", getStack()->getPersistencePath());
    }
#endif

// EPANG TODO - not defined for iLON linux fornow
#if defined(ILON_PLATFORM) && defined (__VXWORKS__)
	// Special method on the iLON to override xcvr id to force some other type.
	// Used only for special tests/trials/demos.
	if (overrideXcvrId && !getStack()->getChannel()->getIpChannel())
	{
		m_nXcvrId = xcvrIdOverride;
	}
#elif PRODUCT_IS(DCX)
	// Set to the special NES XCVR ID (see 208 in LtXcvrId.cpp).
	// This is now handled via a new parameter to vldv_open() which specifies the XID required
#endif

	// Normally, we will treat the config data as the owner of comm params
	// so that changes via network management will be preserved
	getNetworkImage()->configData.getCommParams(cfgCps);

	// We will normally compare the config data against the XID table values
	xcvrIdErr = LtXcvrId::getCommParams(m_nXcvrId, &xidCps);

	// In the case where the XID is unknown or custom,
	// compare against the values returned by the driver
	pCpsToUse = &drvCps;
	drvErr = getDriver()->getCommParams(drvCps);
	if ((drvErr != LTSTS_OK) || (drvCps.m_nData[0] == 0))
	{
		if ((xcvrIdErr == LT_NO_ERROR) && (m_nXcvrId != 30))	// 30 = custom
			pCpsToUse = &xidCps;
		else
			pCpsToUse = &cfgCps;	// if all else fails, preserve the status quo
	}

#if FEATURE_INCLUDED(NVRAM)
    if (xidIsPersistent)
    {
	    LtNvRam::get(xidKey, (byte*) &oldXid, sizeof(oldXid), m_xidKeyContainsPath);
    }
    else
#endif
    {
        oldXid = m_nXcvrId;
    }

#if PRODUCT_IS(ILON) && !defined(WIN32)
	// Tweak for special TP1250	xvcr on iLON target HW
	// Standard xcvr is differential, but the iLON (internally) is single ended
	if (m_nXcvrId == 3)
	{
		xidCps.m_nData[1] = COMM_SINGLE;
	}
#endif // PRODUCT_IS(ILON)
		
	// Check for a major change to the xcvr type.
	// If the first	two bytes (type, pin direction, etc) of the comm parameters
	// are different, or the xcvr ID is different, then	discard
	// the config data copy (except priority) and use the table or driver copy.
	if ((xcvrIdErr == LT_NO_ERROR) && (m_nXcvrId != 30) &&
			((oldXid != m_nXcvrId) || !pCpsToUse->isCompatible(xidCps)))
	{
		// Use the XID copy
		pCpsToUse = &xidCps;
		// Also adjust priority, if necessary
		int nNewPriority;
		
		// Take the priority from the driver copy, if possible
		if ((drvErr != LTSTS_OK) || (drvCps.m_nData[0] == 0))
			nNewPriority = pCpsToUse->setNewPriority(cfgCps);
		else
			nNewPriority = pCpsToUse->setNewPriority(drvCps);

		if (nNewPriority)
		{
			vxlReportUrgent("LonTalk channel priority lowered to %d due to transceiver change.\n", nNewPriority);
		}
	}

#if FEATURE_INCLUDED(NVRAM)
    // Record the fact that we have handled an XID change, but only after the 
	// config data and driver data match, so we know the update was completed.
	if ((oldXid != m_nXcvrId) && 
		(drvErr == LTSTS_OK) &&
		(memcmp(cfgCps.m_nData, drvCps.m_nData, sizeof(cfgCps.m_nData)) == 0))
	{
		LtNvRam::set(xidKey, (byte*) &m_nXcvrId, sizeof(m_nXcvrId), m_xidKeyContainsPath);
	}
#endif

	// If the new xcvr params don't match either config or driver data, log it
	if (((cfgCps.m_nData[0] != 0) && // only if the config data exists
			(memcmp(pCpsToUse->m_nData, cfgCps.m_nData, sizeof(pCpsToUse->m_nData)) != 0)) ||
		((drvErr == LTSTS_OK) && 	// only if we got driver data
			(drvCps.m_nData[0] != 0) &&
			(memcmp(pCpsToUse->m_nData, drvCps.m_nData, sizeof(pCpsToUse->m_nData)) != 0)))

	{
		vxlReportUrgent("Transceiver comm parameters updated (xcvr ID: %d)", m_nXcvrId);
	}

	// Commit any changes (and trigger the link initialization!)
	setCommParams(pCpsToUse);
#endif
}

void LonTalkStack::setXcvrReg(byte* pData)
{
	LtCommParams cps;
    getDriver()->getCommParams(cps);
    memcpy(&cps.m_nData[9], pData, 7);
    setCommParams(&cps);
}

void LonTalkStack::setMem(int addr, int size) 
{
    getStack()->getNetworkManager()->setMem(addr, size);
}

void LonTalkStack::shutDown() 
{
	getNetworkManager()->shutdown();
}

LtApdu* LonTalkStack::getApdu() 
{
	LtApdu* pApdu = getStack()->getApdu();
    return pApdu;
}


void LonTalkStack::resetRequested()
{
	getStack()->getLayer4()->setResetRequested();
}

boolean LonTalkStack::flushPending()
{
	return m_bFlushPending;
}

void LonTalkStack::terminateCompleted()
{
	terminatePending = false;
	getStack()->terminateCompleted();
}

void LonTalkStack::reportTransceiverRegister(int n, int value, LtSts sts)
{
	regValueFailure = sts != LTSTS_OK;
	regValueReceived = true;
	regValue = value;
}


void LtLinkStub::setServicePinState(LtServicePinState state)
{
    if (pLipc != NULL)
    {
	    pLipc->setServicePinStateSpecial(state);
    }
}

