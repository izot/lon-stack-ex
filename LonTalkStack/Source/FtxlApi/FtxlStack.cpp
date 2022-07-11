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

#include "FtxlStack.h"
#include "vxlTarget.h" // For vxlSetReportEvent
#include "tickLib.h"

FtxlStack::FtxlStack(LtLogicalChannel* pChannel, 
                     const LonControlData * const pControlData) :
    LtAppNodeStack(pChannel, pControlData->ReceiveTransCount, pControlData->TransmitTransCount,
        pControlData->Buffers.ApplicationBuffers.NonPriorityMsgOutCount, 
        pControlData->Buffers.ApplicationBuffers.PriorityMsgOutCount),
	m_nvdSegApplicationData(LonNvdSegApplicationData, NVD_SEG_VER_APPL_DATA)
{
    m_isOpen = false;
    m_servicePinHoldTimer = NULL;
    m_servicePinHoldTime = 0;
    m_bServicePinHeld = false;
    m_nServiceLedState = (LtServicePinState)0xff;
    m_nPrevServiceLedState = (LtServicePinState)0xff;

    m_starvationTimer = NULL;
    // NVD starvation is fixed at
    m_starvationTimeout = msToTicks(starvationTimeout*1000);
    m_expectedNvdStart = 0;
    m_bNvdStarvedOut = false;

    m_avgDynNvSdLength = 0;

    m_siData = NULL;
}

FtxlStack::~FtxlStack()
{
    if (m_servicePinHoldTimer != NULL)
    {
        wdDelete(m_servicePinHoldTimer);
    }

    if (m_siData != NULL)
        delete[] m_siData;
    stopApp();
}

const byte FtxlStack::m_commTabTable[NUM_FTXL_XCVR_TYPES][FTXL_NUM_COMM_BYTES] =
{
        // FtxlTransceiverTypeDefault (Not used)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},    
        // FtxlTransceiverType5MHz
    {0x1C, 0x2E, 0x01, 0x02, 0x05, 0x00, 0x01, 0x00, 0x04, 0x00, 0x4C, 0x00, 0x00, 0x00, 0x00, 0x00},
        // FtxlTransceiverType10MHz,
    {0x25, 0x2E, 0x08, 0x05, 0x0C, 0x0E, 0x0F, 0x00, 0x04, 0x00, 0xA4, 0x00, 0x00, 0x00, 0x00, 0x00},
        // FtxlTransceiverType20MHz,
    {0x2E, 0x2E, 0x16, 0x09, 0x1A, 0x2B, 0x2C, 0x00, 0x04, 0x00, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00},
        //FtxlTransceiverType40MHz,
    {0x37, 0x2E, 0x32, 0x13, 0x36, 0x64, 0x67, 0x00, 0x04, 0x00, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00},
};

LonApiError FtxlStack::createStack(const LonStackInterfaceData* const pInterface,
                                const LonControlData       * const pControlData,
                                const char* pNvdFsPath)
{
    LonApiError sts = LonApiNoError;
    LtErrorType ltSts; 
    LtProgramId pid ((byte *)pInterface->ProgramId, true);
    boolean netBuffersChanged = false;
    boolean appBuffersChanged = false;

    /* TBD - range checks */
    if (pInterface->Version != LON_STACK_INTERFACE_CURRENT_VERSION ||
        pControlData->Version != LON_CONTROL_DATA_CURRENT_VERSION)
    {
        sts = LonApiVersionNotSupported;
    }
    else
    {
        m_avgDynNvSdLength = pInterface->AvgDynNvSdLength;
#if PERSISTENCE_TYPE_IS(FTXL)
        setAppSignature(pInterface->Signature);
        setPeristenceGaurdBand(pControlData->NvdFlushGuardTimeout*1000);
        m_nvdSegApplicationData.setAppSignature(pInterface->Signature);
        m_nvdSegApplicationData.setPeristenceGaurdBand(pControlData->NvdFlushGuardTimeout*1000);
#endif

        // Set persitence path
        m_nvdSegApplicationData.setNvdFsPath(pNvdFsPath);
        getPersistence()->setNvdFsPath(pNvdFsPath);
        getNetworkImage()->getPersistence()->setNvdFsPath(pNvdFsPath);
        ltSts = registerApplication(0, this, 
                                    pInterface->DomainTblSize,
                                    pInterface->AddrTblSize,
                                    pInterface->StaticNvCount,
                                    pInterface->NvTblSize-pInterface->StaticNvCount,
                                    0, /* numPrivateNvEntries */
                                    pInterface->AliasTblSize,
                                    0, /* numMonitorNvEntries */
                                    0, /* numMonitorPointEntries */
                                    0, /* numMonitorSetEntries */
                                    pInterface->BindableMsgTagCount,
                                    3,  /* bindingConstraintLevel */
                                    pInterface->NodeSdString,
                                    &pid);

        sts = lt2LonError(ltSts);
    }
    if (sts == LonApiNoError)
    {
        setDirectCallbackMode(FALSE);
        setReceiveNsaBroadcasts(TRUE);
        setTxIdLifetime(pControlData->TransmitTransIdLifetime);
        setMessageOutMaximum(pControlData->Buffers.ApplicationBuffers.NonPriorityMsgOutCount,
                             pControlData->Buffers.ApplicationBuffers.PriorityMsgOutCount);
        setMessageEventMaximum(pControlData->Buffers.ApplicationBuffers.MsgInCount);
    }

    if (sts == LonApiNoError)
    {        
        ltSts = LT_NO_ERROR;
        LtCommParams commParms;
        LtReadOnlyData ro = *getReadOnly();

        getNetworkImage()->configData.getCommParams(commParms);
        if (commParms.m_nData[0] == 0)
        {
            // If the comm parms have not been set, assume that the buffers have
            // not been set yet either.
            if (ro.getNetInBufSize() != pControlData->Buffers.TransceiverBuffers.NetworkBufferInputSize ||
                ro.getNumNetInBufs() != pControlData->Buffers.TransceiverBuffers.NetworkInCount)
            {
                netBuffersChanged = true;
                ltSts = ro.setNetworkInputBuffers(
                    pControlData->Buffers.TransceiverBuffers.NetworkBufferInputSize,
                    pControlData->Buffers.TransceiverBuffers.NetworkInCount);
            }

            // Can't necessarily report the correct values for app buffers - but we should
            // at least report the best approximations
            if (ltSts == LT_NO_ERROR)
            {
                if (ro.getNetOutBufSize() != pControlData->Buffers.TransceiverBuffers.NetworkBufferOutputSize ||
                    ro.getNumNetOutBufs(0) != pControlData->Buffers.TransceiverBuffers.NonPriorityNetworkOutCount ||
                    ro.getNumNetOutBufs(1) != pControlData->Buffers.TransceiverBuffers.PriorityNetworkOutCount)
                {
                    netBuffersChanged = true;

                    ltSts = ro.setNetworkOutputBuffers(
                        pControlData->Buffers.TransceiverBuffers.NetworkBufferOutputSize,
                        pControlData->Buffers.TransceiverBuffers.NonPriorityNetworkOutCount,
                        pControlData->Buffers.TransceiverBuffers.PriorityNetworkOutCount);
                }
            }
        }
        /* Check the app buffers whether they have been set or not.  The app buffers really
         * be updated over the network, so if someone has tried, we should fix them up.
         */
        if (ltSts == LT_NO_ERROR)
        {
            if (ro.getNumAppInBufs() != ro.getApproximateCount(pControlData->Buffers.ApplicationBuffers.MsgInCount))
            {
                appBuffersChanged = true;
                ltSts = ro.setAppInputBuffers(
                    ro.getAppInBufSize(), 
                    ro.getApproximateCount(pControlData->Buffers.ApplicationBuffers.MsgInCount));
            }
        }
        if (ltSts == LT_NO_ERROR)
        {
            if (ro.getNumAppOutBufs(0) != ro.getApproximateCount(pControlData->Buffers.ApplicationBuffers.NonPriorityMsgOutCount) ||
                ro.getNumAppOutBufs(1) != ro.getApproximateCount(pControlData->Buffers.ApplicationBuffers.PriorityMsgOutCount))
            {
                appBuffersChanged = true;
                ltSts = ro.setAppOutputBuffers(
                            ro.getAppOutBufSize(), 
                            ro.getApproximateCount(pControlData->Buffers.ApplicationBuffers.NonPriorityMsgOutCount),
                            ro.getApproximateCount(pControlData->Buffers.ApplicationBuffers.PriorityMsgOutCount));
            }
        }
        sts = lt2LonError(ltSts);
        if (netBuffersChanged || appBuffersChanged)
        {
            if (sts == LonApiNoError)
            {
                getReadOnly()->fromLonTalk(0, ro.getLength(), ro.getData());
                if (netBuffersChanged)
                {
                    getReadOnly()->setPendingUpdate(true);
                }
            }
        }
    }

    if (sts == LonApiNoError)
    {
        LtCommParams commParms;
        boolean setCommParms = true;
        
        getNetworkImage()->configData.getCommParams(commParms);
        if (commParms.m_nData[0] == 0)
        {   
            /* Comm parms not yet set. */
            switch(pControlData->CommParmeters.TransceiverType)
            {
            case FtxlTransceiverTypeDefault:
                /* User does not want to modify the comm parms int the mip. 
                   Read the comm parms from the driver to update the config data. */
                setCommParms = (getDriver()->getCommParams(commParms) == LTSTS_OK);
                break;

            case FtxlTransceiverType5MHz:
            case FtxlTransceiverType10MHz:
            case FtxlTransceiverType20MHz:
            case FtxlTransceiverType40MHz:
                // Copy comm parms from table
        		memcpy(commParms.m_nData, m_commTabTable[pControlData->CommParmeters.TransceiverType], 
                       sizeof(commParms.m_nData));
                break;

            case FtxlTransceiverTypeCustom:
                // Use the user supplied comm parms
                memcpy(commParms.m_nData, pControlData->CommParmeters.CommParms, 
                       sizeof(commParms.m_nData));

            default:
                sts = LonApiInvalidParameter;
                setCommParms = false;
            }
        }
        if (setCommParms)
        {
            setCommParams(&commParms);
        }
    }

    if (sts == LonApiNoError && netBuffersChanged)
    {
        initiateReset();
    }

    if (sts == LonApiNoError)
    {
        if (pControlData->ServicePinInterval)
        {
            m_servicePinHoldTime = msToTicks(pControlData->ServicePinInterval*1000);
            m_servicePinHoldTimer = wdCreate();
        }
    }
    return sts;
}

LonApiError FtxlStack::startStack(void)
{
    LonApiError sts = LonApiNoError;
    NdErrorType ndSts; 

    ndSts = endDefinition();

    sts = nd2LonError(ndSts);
    if (sts == LonApiNoError)
    {
        m_isOpen = true; 
        boolean persistenceRestored;
	    m_nvdSegApplicationData.restore(persistenceRestored);
        if (!persistenceRestored)
        {
            goUnconfigured();
        }

        m_starvationTimer = wdCreate();
#if PERSISTENCE_TYPE_IS(FTXL)
        LtPersistence::setResetFlag(false);
        LtPersistence::registerPersistenceMonitor(this);
#endif
    }
    return sts;
}

void FtxlStack::stopApp() 
{ 
    if (m_starvationTimer != NULL)
    {
        wdDelete(m_starvationTimer);
        m_starvationTimer = NULL;
    }
#if PERSISTENCE_TYPE_IS(FTXL)
    LtPersistence::registerPersistenceMonitor(NULL);
#endif
    if (m_isOpen)
    {
        shutDown(); 
        m_isOpen = FALSE;
    }
}

void FtxlStack::eventPump()
{
	if (m_isOpen)
	{
        if (m_bNvdStarvedOut)
        {
            ULONG msec;
            m_bNvdStarvedOut = false;
            msec = ticksToMs(tickGet() - m_expectedNvdStart);
            LonNvdStarvation((msec+500)/1000);
        }
		processApplicationEvents();
        if (m_bServicePinHeld)
        {
            m_bServicePinHeld = false;
            LonServicePinHeld();
        }
        
        if ((m_nServiceLedState != m_nPrevServiceLedState) && (m_nServiceLedState != (LtServicePinState)0xff))
        {
#if PRODUCT_IS(IZOT)
            LonServiceLedStatus(m_nServiceLedState); // expose the service pin state to the callback function
#endif
            m_nPrevServiceLedState = m_nServiceLedState;
        }
	}
}

LonApiError FtxlStack::storNetworkImage(void)
{
    LonApiError sts;
    if (!getNetworkImage()->store())
    {
        sts = lt2LonError(LT_EEPROM_WRITE_FAILURE);
    }
    else
    {
        sts = LonApiNoError;
    }
    return sts;
}

//-----------------------------------------------------------------------
//                       Configuration access functions                           
//-----------------------------------------------------------------------

const LonApiError FtxlStack::queryNvType(LtNetworkVariable *pNv, int arrayIndex, LonNvDefinition* const pNvDef)
{
    LonApiError sts = LonApiNoError;

    pNvDef->Version = LON_NV_DEFINITION_CURRENT_VERSION;
    pNvDef->PValue = pNv->getNvDataPtr(arrayIndex);
    pNvDef->DeclaredSize = pNv->getLength();    // TBD...
    pNvDef->SnvtId = pNv->getType();
    pNvDef->ArrayCount = pNv->getArrayLength();
    pNvDef->Flags = (pNv->getFlags() & NV_SD_PUBLIC_MASK) & ~LON_NV_IS_OUTPUT;
    if (pNv->getIsOutput())
    {
        pNvDef->Flags |= LON_NV_IS_OUTPUT;
    }
    
    pNvDef->Name = new char[ND_NAME_LEN];
    pNv->getName((char *)pNvDef->Name, ND_NAME_LEN);
    pNvDef->SdString = new char[MAX_SD_STRING_LEN];
    pNv->getSdString((char *)pNvDef->SdString, MAX_SD_STRING_LEN);
    pNvDef->MaxRate = (pNv->getMaxRateEstimate() == -1) ? 
        LON_NV_RATE_UNKNOWN : pNv->getMaxRateEstimate();
    
    pNvDef->MeanRate = (pNv->getRateEstimate() == -1) ? 
        LON_NV_RATE_UNKNOWN : pNv->getRateEstimate();

    return sts;
}

void FtxlStack::freeNvType(LonNvDefinition* const pNvDef)
{
    delete[] pNvDef->Name;
    pNvDef->Name = NULL;
    delete[] pNvDef->SdString;
    pNvDef->SdString = NULL;
}

const LonApiError FtxlStack::queryNvConfig(const unsigned index,
                                           LonNvEcsConfig* const pNvConfig)
{
    byte nvConfigBuffer[LT_NV_STORE_SIZE];
    LtNetworkVariableConfiguration nvConfig;
    LonApiError sts = lt2LonError(getNetworkImage()->nvTable.getNv(index, &nvConfig));
    if (LON_SUCCESS(sts))
    {
        nvConfig.toLonTalk(nvConfigBuffer);
        memcpy(pNvConfig, nvConfigBuffer, sizeof(*pNvConfig));
    }
    return sts;
}

const LonApiError FtxlStack::updateNvConfig(const unsigned index,
                                            const LonNvEcsConfig* const pNvConfig)
{
    LtNetworkVariableConfiguration nvConfig;
	/* First read the nvConfig from the stack to make sure it's valide and to
	   set some internal flags (such as whether this is an alias or not. */
    LonApiError sts = lt2LonError(getNetworkImage()->nvTable.get(index, &nvConfig, NV_TABLE_NVS));
    if (LON_SUCCESS(sts))
    {	
		nvConfig.fromLonTalk((byte *)pNvConfig, sizeof(LonAliasEcsConfig));
		sts = lt2LonError(getNetworkImage()->nvTable.set(index, nvConfig, NV_TABLE_NVS));
        if (LON_SUCCESS(sts))
        {
            sts = storNetworkImage();
        }
    }
    return sts;
}

const LonApiError FtxlStack::queryAliasConfig(const unsigned index,
                                              LonAliasEcsConfig* const pAlias)
{
    byte nvConfigBuffer[LT_NV_STORE_SIZE];
    LtNetworkVariableConfiguration nvConfig;
    LonApiError sts = lt2LonError(getNetworkImage()->nvTable.get(index, &nvConfig, NV_TABLE_ALIASES));
    if (LON_SUCCESS(sts))
    {
        nvConfig.toLonTalk(nvConfigBuffer);
        memcpy(pAlias, nvConfigBuffer, sizeof(*pAlias));
    }
    
    if (sts == LonApiNvIndexInvalid)
    {
        sts = LonApiIndexInvalid;
    }
    return sts;
}

const LonApiError FtxlStack::updateAliasConfig(const unsigned index,
                                               const LonAliasEcsConfig* const pAlias)
{
    LtNetworkVariableConfiguration nvConfig;
	/* First read the nvConfig from the stack to make sure it's valide and to
	   set some internal flags (such as whether this is an alias or not. */
    LonApiError sts = lt2LonError(getNetworkImage()->nvTable.get(index, &nvConfig, NV_TABLE_ALIASES));
    if (LON_SUCCESS(sts))
    {	
		nvConfig.fromLonTalk((byte *)pAlias, sizeof(LonAliasEcsConfig));
		sts = lt2LonError(getNetworkImage()->nvTable.set(index, nvConfig, NV_TABLE_ALIASES));
        if (LON_SUCCESS(sts))
        {
            sts = storNetworkImage();
        }
    }
    
    if (sts == LonApiNvIndexInvalid)
    {
        sts = LonApiIndexInvalid;
    }
    return sts;
}

const LonApiError FtxlStack::queryAddressConfig(const unsigned index,
                                                LonAddress* const pAddress)
{
    LtAddressConfiguration ac;
	LonApiError sts = lt2LonError(getAddressConfiguration(index, &ac));
	if (LON_SUCCESS(sts))
    {
		// Convert to a classic address table - 1 bit domain index
        ac.toLonTalk((byte *)pAddress, 1); 
    }
    return sts;
}

const LonApiError FtxlStack::updateAddressConfig(const unsigned index,
                                                 const LonAddress* const pAddress)
{
	LonApiError sts;
    LtAddressConfiguration ac;
	int len;
	ac.fromLonTalk((byte *)pAddress, len, 1 /* Classic version */);
    sts = lt2LonError(updateAddressConfiguration(index, &ac));
    if (LON_SUCCESS(sts))
    {
        sts = storNetworkImage();
    }
    return sts;
}

const LonApiError FtxlStack::updateConfigData(const LonConfigData* const pConfig)
{
	LonApiError sts = lt2LonError(updateConfigurationData((byte *)pConfig, 0, sizeof(*pConfig)));
    if (LON_SUCCESS(sts))
    {
        sts = storNetworkImage();
    }
    return sts;
}

LonApiError FtxlStack::GetUniqueId(LonUniqueId* const pId)
{
	LonApiError sts = LonApiNoError;

    LtUniqueId uniqueId;
    int index = 0;
	if (getAddress(index, &uniqueId))
    {
        uniqueId.get((byte *)pId);
    }
    else
    {
        sts = LonApiNeuronIdNotAvailable;
    }
    return sts;
}

void FtxlStack::GetRecieveAddress(LonReceiveAddress &receiveAddress, LtIncomingAddress &addr)
{
    memset(&receiveAddress, 0, sizeof(receiveAddress));
    if (addr.getDomainConfiguration().isFlexDomain())
    {
        LON_SET_ATTRIBUTE(receiveAddress, LON_RECEIVEADDRESS_FLEX,
                          addr.getDomainConfiguration().getIndex());

    }
    else
    {
        LON_SET_ATTRIBUTE(receiveAddress, LON_RECEIVEADDRESS_DOMAIN,
                           addr.getDomainConfiguration().getIndex());
    }

    switch(addr.getAddressFormat())
    {
    case LT_AF_BROADCAST:
        LON_SET_ATTRIBUTE(receiveAddress, LON_RECEIVEADDRESS_FORMAT, 
                          LonReceiveDestinationAddressBroadcast);
        receiveAddress.Destination.Broadcast.SubnetId = addr.getSubnet();
        break;
    case LT_AF_GROUP:
        LON_SET_ATTRIBUTE(receiveAddress, LON_RECEIVEADDRESS_FORMAT, 
                          LonReceiveDestinationAddressGroup);
        receiveAddress.Destination.Group.GroupId = addr.getGroup();
        break;
    case LT_AF_SUBNET_NODE:
        LON_SET_ATTRIBUTE(receiveAddress, LON_RECEIVEADDRESS_FORMAT, 
                          LonReceiveDestinationAddressSubnetNode);
        receiveAddress.Destination.SubnetNode.Subnet = addr.getSubnetNode().getSubnet();
        receiveAddress.Destination.SubnetNode.Node = addr.getSubnetNode().getNode();
        break;
    case LT_AF_UNIQUE_ID:
        LON_SET_ATTRIBUTE(receiveAddress, LON_RECEIVEADDRESS_FORMAT, 
                          LonReceiveDestinationAddressUniqueId);
        receiveAddress.Destination.UniqueId.Subnet = addr.getSubnet();
        GetUniqueId(&receiveAddress.Destination.UniqueId.UniqueId);
        break;
    case LT_AF_TURNAROUND:
        LON_SET_ATTRIBUTE(receiveAddress, LON_RECEIVEADDRESS_FORMAT, 
                          LonReceiveDestinationAddressTurnaround);
        break;

    default:
        /* Should never happen. */
        vxlReportUrgent("Invalid receive address type %d\n", addr.getAddressFormat());
    }

    receiveAddress.Source.Subnet = addr.getDomainConfiguration().getSubnetNode().getSubnet();
    receiveAddress.Source.Node = addr.getDomainConfiguration().getSubnetNode().getNode();
}

LonApiError FtxlStack::applSegmentHasBeenUpdated(void)
{
	return m_nvdSegApplicationData.segmentHasBeenUpdated();
}

LonApiError FtxlStack::flushNvd(void)
{
    sync();
    m_nvdSegApplicationData.sync();
    return LonApiNoError;
}

int FtxlStack::nvdGetMaxSize(LonNvdSegmentType segmentType)
{
    int length = sizeof(LtPersistenceHeader);
    switch(segmentType)
    {
#if PERSISTENCE_TYPE_IS(FTXL)
    case LonNvdSegNetworkImage:
        length += getNetworkImage()->getMaxSerialLength();
        break;
    case LonNvdSegNodeDefinition:
        length += getMaxSerialLength(m_avgDynNvSdLength);
        break;
#endif
    case LonNvdSegApplicationData:
        length += LonNvdGetApplicationSegmentSize();
        break;
    case LonNvdSegUniqueId:
        length += sizeof(LtUniqueId);
        break;
    default:
        length = 0;
    }
    return length;
}

void FtxlStack::setNvdFsPath(const char *pNvdFsPath)
{
    m_nvdSegApplicationData.setNvdFsPath(pNvdFsPath);
}

const char* FtxlStack::getNvdFsPath()
{
    return m_nvdSegApplicationData.getNvdFsPath();
}

LonApiError FtxlStack::lt2LonError(LtErrorType ltSts)
{
    LonApiError sts;
    switch(ltSts)
    {
    case LT_NO_ERROR:
        sts = LonApiNoError;
        break;

    case LT_INVALID_STATE:
    case LT_NOT_IMPLEMENTED:		
        sts = LonApiNotAllowed;
        break;

    case LT_INVALID_PARAMETER:
    case LT_APP_NAME_TOO_LONG:
    case LT_DUPLICATE_OBJECT:		
    case LT_STALE_NV_INDEX:
    case LT_NV_LENGTH_MISMATCH:	
    case LT_NV_MSG_TOO_SHORT:		
        sts = LonApiInvalidParameter;
        break;

    case LT_INVALID_ADDRESS:
    case LT_BAD_ADDRESS_TYPE:		
    case LT_INVALID_DOMAIN:		
    case LT_INVALID_IPADDRESS:
        sts = LonApiMsgInvalidAddress;
        break;

    case LT_INVALID_INDEX:			
    case LT_INVALID_ADDR_TABLE_INDEX :
        sts = LonApiIndexInvalid;
        break;

    case LT_INVALID_NV_INDEX:
        sts = LonApiNvIndexInvalid;
        break;

    default:
        sts = LonApiNotAllowed;
    }
    return sts;
}

LonApiError FtxlStack::nd2LonError(NdErrorType ndSts)
{
    LonApiError sts;
    switch(ndSts)
    {
    case ND_OK:
        sts = LonApiNoError;
        break;
    
    case ND_STRING_TOO_LONG:
    case ND_PARTIAL_DEFINITION:
    case ND_INSUFFICIENT_DATA:
    case ND_INVALID_PARAMETER:
    case ND_DUPLICATE:
        sts = LonApiInvalidParameter;
        break;

    default:
        sts = LonApiNotAllowed;
    };

    return sts;
}

//=======================================================================
//                        LtApplication callbacks                        
//=======================================================================
    //-----------------------------------------------------------------------
    //                            Stack callbacks                           
    //-----------------------------------------------------------------------

/**
 * This method is invoked when the LonTalk Stack resets.  A reset could result
 * from a network management command or via the reset pin of the LON-C block.
 */
void FtxlStack::reset() 
{
	LonReset(NULL);
}   

/**
 * This method is invoked when a wink request is received over the network.
 */
void FtxlStack::wink()
{
	LonWink();
}  

/**
 * This method is invoked when an offline request is received over the network.
 */
void FtxlStack::offline()
{
	LonOffline();
}   

/**
 * This method is invoked when an online request is received over the network.
 */
void FtxlStack::online()
{
	LonOnline();
} 

//-----------------------------------------------------------------------
//                       Network Variable callbacks                           
//-----------------------------------------------------------------------

/**
 * This method is invoked when a network variable update arrives either as
 * an update message or as a response to a poll.
 * @param index
 *              The network variable index.  For network variable arrays, this
 *              is the NV base index plus the array element index.
 * @param address
 *              The source address of the network variable update.
 */
void FtxlStack::nvUpdateOccurs(LtNetworkVariable* pNv, int arrayIndex,
	LtNetworkVariable* pSourceNv, int sourceArrayIndex,
    LtIncomingAddress* address) 
{
    LonReceiveAddress receiveAddress;
    GetRecieveAddress(receiveAddress, *address);
    if (isConfiguredAndOnline())
    {
        LonNvUpdateOccurred(pNv->getNvIndex() + arrayIndex, &receiveAddress);
    }
    if (pNv->getFlags() & NV_SD_CONFIG_CLASS)
    {
        /* Signal that the application non-volatile data has been changed */
        LonNvdAppSegmentHasBeenUpdated();
    }
}

/**
 * This method is invoked when a network variable update completes.  For unackd
 * or unackd repeat updates, successful completion means the message(s) was queued
 * to be sent.  For acknowledged updates, successful completion means all
 * acknowledgments were received.  For polls, successful completion means all
 * responses were received and at least one had valid data (i.e., target was
 * online, had matching selector and passed authentication).
 * @param index
 *              The network variable index.  For network variable arrays, this
 *              is the NV base index plus the array element index.
 * @param success
 *              true if the transaction completed successfully.
 */
void FtxlStack::nvUpdateCompletes(LtNetworkVariable* pNv, int arrayIndex, boolean success)
{
    LonNvUpdateCompleted(pNv->getNvIndex() + arrayIndex, success);
}

/**
 * This method is invoked when an NV is added.  This occurs when dynamic NVs are
 * added.  The application may choose to create a new version of the NV object
 * which will be used in place of the one provided.  The stack will delete the
 * old version in this case.  Return NULL or the old version to indicate no
 * replacement.  Replacement might be used to derive from LtNetworkVariable.
 */

LtNetworkVariable* FtxlStack::nvAdded(LtNetworkVariable* pNv)
{
    LonNvDefinition nvType;
    if (LON_SUCCESS(queryNvType(pNv, 0, &nvType)))
    {
        LonNvAdded(pNv->getNvIndex(), &nvType);
        freeNvType(&nvType);
    }
    return pNv;
}


/**
 * This method is called when an NV attribute changes.  It is also called during
 * initialization to reflect any stored changes from the initial values.
 */
void FtxlStack::nvChanged(LtNetworkVariable* pNv, NvChangeType type)
{
    LonNvDefinition nvType;
    if (LON_SUCCESS(queryNvType(pNv, 0, &nvType)))
    {
        if (pNv->getDynamic())
        {
            LonNvTypeChanged(pNv->getNvIndex(), &nvType);
        }
        freeNvType(&nvType);
    }
}

/**
 * This method is called when a dynamic NV is deleted.  It is the application's
 * responsibility to delete the object.
 */

void FtxlStack::nvDeleted(LtNetworkVariable* pNv) 
{
    LonNvDeleted(pNv->getNvIndex());
	delete pNv;
}

/**
 * This method is called to find the current length of a network variable
 * with changeable length from the application. If the application does not
 * support this functionality, the callback should return false, and the
 * stack will make a best guess at the length based on the last time
 * the NV was updated, either from the network or from the application.
 */
boolean FtxlStack::getCurrentNvLength(LtNetworkVariable* pNv, int &length)
{
    if (pNv->getDynamic())
    {
        /* Dynamic NVs length can be changed only by changing the NV 
         * definition. Don't depend on the app to return the length.
         */
        length = pNv->getLength();
    }
    else
    {
        length = LonGetCurrentNvSize(pNv->getNvIndex());        

    }
    return true;
}


//-----------------------------------------------------------------------
//                       Explicit Messaging callbacks                           
//-----------------------------------------------------------------------

/**
 * This method is invoked when an application message is received.  Such a message
 * will have a code in the range of 0x00 to 0x4F.
 * @param msg
 *              The incoming message.
 */
void FtxlStack::msgArrives(LtMsgIn* msg)
{
    LonReceiveAddress receiveAddress;
    GetRecieveAddress(receiveAddress, msg->getAddress());
    LonCorrelator correlator = NULL;

    if (isConfiguredAndOnline())
    {
        if (msg->getServiceType() == LT_REQUEST)
        {
            correlator = msg;
        }

        // Check if we need to filter the message
#if PRODUCT_IS(IZOT)
        if (!LonFilterMsgArrived(&receiveAddress, 
                      correlator, 
                      msg->getPriority(),
                      (LonServiceType)msg->getServiceType(),
                      msg->getAuthenticated(),
                      (LonApplicationMessageCode)msg->getCode(),
                      msg->getData(),
                      msg->getLength())) 
#endif
        {
            // The message is not processed by the FilterMsgArrived callback, execute the standard LonMsgArrived 
            LonMsgArrived(&receiveAddress, 
                      correlator, 
                      msg->getPriority(),
                      (LonServiceType)msg->getServiceType(),
                      msg->getAuthenticated(),
                      (LonApplicationMessageCode)msg->getCode(),
                      msg->getData(),
                      msg->getLength());
        }
    }

    // Normally the API is responsible for freeing the message.  However, if the
    // message is a request AND we sent it to the app, the API will set the correlator
    // to point to the message and the app is responsible releasing the message, 
    // either implicitly by sending a response or explicitly by calling 
    // LonReleaseCorrelator. 
    if (correlator == NULL)
    {
        release(msg);
    }
}

/**
 * This method is invoked when a message completes.  For unackd
 * or unackd repeat updates, successful completion means the message(s) was queued
 * to be sent.  For acknowledged and request/response messages, successful
 * completion means all acknowledgments/responses were received.
 * @param tag
 *              The message tag provided in the original outgoing message.
 * @param success
 *              true if the transaction completed successfully.
 */
void FtxlStack::msgCompletes(LtMsgTag* tag, boolean success)
{
    // Check if we need to filter the message
#if PRODUCT_IS(IZOT)
	if (!LonFilterMsgCompleted(tag->getIndex(), success ? true : false))
#endif
    {
        // The message completed is not processed by the FilterMsgCompleted callback, execute the standard LonMsgCompleted 
        LonMsgCompleted(tag->getIndex(), success ? true : false);
    }
    delete tag;
}

/**
 * This method is invoked when a response arrives.
 * @param response
 *              The incoming response.
 */
void FtxlStack::respArrives(LtMsgTag* tag, LtRespIn* response) 
{
    LonResponseAddress responseAddress;
    boolean subnetNodeResp;
    LtIncomingAddress &addr = response->getAddress();
    memset(&responseAddress, 0, sizeof(responseAddress));

    if (addr.getDomainConfiguration().isFlexDomain())
    {
        LON_SET_ATTRIBUTE(responseAddress, LON_RESPONSEADDRESS_FLEX,
                          addr.getDomainConfiguration().getIndex());

    }
    else
    {
        LON_SET_ATTRIBUTE(responseAddress, LON_RESPONSEADDRESS_DOMAIN,
                           addr.getDomainConfiguration().getIndex());
    }

    if (addr.getAddressFormat() == LT_AF_GROUP_ACK)
    {
        subnetNodeResp = false;

        responseAddress.Destination.Group.Subnet = addr.getSubnetNode().getSubnet();
        LON_SET_ATTRIBUTE(responseAddress.Destination.Group, LON_RESPGROUP_NODE, 
                          addr.getSubnetNode().getNode());
        responseAddress.Destination.Group.Group = addr.getGroup();
        LON_SET_ATTRIBUTE(responseAddress.Destination.Group, LON_RESPGROUP_MEMBER,
                          addr.getMember());
    }
    else
    {
        subnetNodeResp = true;
        responseAddress.Destination.SubnetNode.Subnet = addr.getSubnetNode().getSubnet();
        LON_SET_ATTRIBUTE(responseAddress.Destination.SubnetNode, LON_RESPONSESOURCE_NODE, 
                          addr.getSubnetNode().getNode());
    }

    responseAddress.Source.Subnet = addr.getDomainConfiguration().getSubnetNode().getSubnet();
    LON_SET_ATTRIBUTE(responseAddress.Source, LON_RESPONSESOURCE_IS_SUBNETNODE, subnetNodeResp);
    LON_SET_ATTRIBUTE(responseAddress.Source, LON_RESPONSESOURCE_NODE,
                      addr.getDomainConfiguration().getSubnetNode().getNode());

    // Check if we need to filter the reponse  
#if PRODUCT_IS(IZOT)
    if (!LonFilterResponseArrived(&responseAddress,
					   tag->getIndex(), 
					   (LonApplicationMessageCode)response->getCode(), 
						response->getData(), response->getLength()))
#endif
    {
        // The response message is not processed by the FilterResponseArrived callback, execute the standard LonResponseArrived 
	    LonResponseArrived(&responseAddress,
					   tag->getIndex(), 
					   (LonApplicationMessageCode)response->getCode(), 
						response->getData(), response->getLength());
    }
};

//-----------------------------------------------------------------------
//                            Misc. callbacks                           
//-----------------------------------------------------------------------
    /**
     * This method informs the application when the service pin has been depressed.  The
     * protocol stack automatically sends a service pin message as a result of service pin 
     * depression.  This callback allows the application to do additional actions if so
     * desired.
     */
void FtxlStack::servicePinPushed() 
{
    LonServicePinPressed();
    if (m_servicePinHoldTimer != NULL)
    {
        wdStart(m_servicePinHoldTimer, m_servicePinHoldTime, servicePinHeldTimeout, (int)this);
    }
}

int FtxlStack::servicePinHeldTimeout(int pFtxlStack)
{
    ((FtxlStack *)pFtxlStack)->m_bServicePinHeld = true;
    LonEventReady();
    return 0;
}

   /**
     * This method informs the application when the service pin has been released. 
     */
void FtxlStack::servicePinHasBeenReleased()
{
    if (m_servicePinHoldTimer != NULL)
    {
        wdCancel(m_servicePinHoldTimer);
    }    
}

    /**
     * This method is invoked in non-direct callback mode (see Layer7Api.java)
     * to cause the application thread to run.  It is up to the application whether
     * this is effected by suspend/resume, wait/notify or sleep/interrupt.  Once 
     * running, it is up to the application to invoke processApplicationEvents() to handle
     * the event.
     */
void FtxlStack::applicationEventIsPending(void)
{
    LonEventReady();
}

/**
 * This method is used to read memory from a specified LonTalk address.
 * Only addresses registered with Layer7Api.registerMemory() are read.
 * @param address
 *          Address to read
 * @param data
 *          Byte array for data (read data.length bytes).
 */
boolean FtxlStack::readMemory(int address, byte* data, int length) 
{
    return LonMemoryRead(address, length, data) == LonApiNoError;
}

/**
 * This method is used to write memory at a specified LonTalk address.
 * Only addresses registered with Layer7Api.registerMemory() are written.
 * @param address
 *          Address to write
 * @param data
 *          Byte array of data to write
 */
boolean FtxlStack::writeMemory(int address, byte* data, int length)
{
    boolean success;
    if (LonMemoryWrite(address, length, data) == LonApiNoError)
    {
        /* Signal that the application non-volatile data has been changed */
        LonNvdAppSegmentHasBeenUpdated();
        success = true;
    }
    else
    {
        success = false;
    }
    return success;
}

/**
  * This method is called when the service led status is changed
  */
void FtxlStack::setServiceLedStatus(LtServicePinState state)
{
    m_nServiceLedState = state;
    LonEventReady();    
}

/*
 *  Function: getSiData
 *  Gets the SI data 
 *
 *  Parameters:
 *  pLength - the length of the SI data
 *
 *  Returns:     
 *  The byte array containing the SI data
 *
 */
byte* FtxlStack::getSiData(int* pLength)
{
	int siLen = siComputeLength();

    if (m_siData != NULL)
        delete[] m_siData;
    m_siData = new byte[siLen];
    *pLength = siLen;
	if (makeSiData(m_siData, 0, siLen) != ND_OK)
	{
		*pLength = 0;
	}

    return m_siData;
}
 
/*
 *  Function: getCurrentNvSize
 *  Gets the current size of a network variable.
 *
 *  Parameters:
 *  index - the index of the network variable
 *
 *  Returns:     
 *  Current size of the network variable as defined in the Neuron C 
 *  model file.  Zero if the network variable corresponding to index doesn't 
 *  exist.
 *
 *  Note that this function is called from the LonGetCurrentNvSize()if the callback vector 
 *  is not registered. 
 */
unsigned FtxlStack::getCurrentNvSize(const unsigned index)
{
    int size = 0;
    int arrayIndex;
    LtNetworkVariable *pNv = getNetworkVariable(index, arrayIndex);

    if (pNv != NULL)
    {
        size = pNv->getCurLength();
    }
    return size;
}

/*
 *  Function: GetDefaultApplicationSegmentSize
 *  Gets the current the application's segment size.  It is the sum of the declared size of all NV 
 *  with the LON_NV_CONFIG_CLASS flag set.
 *
 *  Parameters:
 *    none
 *
 *  Returns:     
 *  The application segment size which is the sum of the declared size of all NV with 
 *  the LON_NV_CONFIG_CLASS flag set. 
 *
 *  Note this is an internal function which called from the LonNvdGetApplicationSegmentSize()
 *  if the callback vector is not registered. 
 */
unsigned FtxlStack::getDefaultApplicationSegmentSize()
{
    unsigned nNvConfigClassSize = 0;
    int nIndex = 0;
    int arrayIndex;
    LtNetworkVariable* pNv;
	
    while ((pNv = getNetworkVariable(nIndex, arrayIndex)) != NULL)
    {
        if (pNv->getFlags() & LON_NV_CONFIG_CLASS)
            nNvConfigClassSize +=  (pNv->getCurLength() * pNv->getElementCount());
        nIndex += pNv->getElementCount();
    }
    return nNvConfigClassSize;
}

/*
 * Function: DefaultSerializeSegment
 * This is an internal function, used by the <LonNvdSerializeSegment>
 * and <LonNvdDeserializeSegment> callback functions.
 */
void FtxlStack::defaultSerializeSegment(boolean toNvMemory, void* const pData, const size_t size)
{
    int index = 0;
    int arrayIndex;
    size_t offset = 0;
    char* const pNvd = (char* const)pData;
    LtNetworkVariable* pNv;

    while ((pNv = getNetworkVariable(index, arrayIndex)) != NULL)
    {
        if (pNv->getFlags() & LON_NV_CONFIG_CLASS)
        {
            if (toNvMemory)
            {
                (void)memcpy(pNvd+offset, (void* const)LonGetNvValue(index), pNv->getCurLength() * pNv->getElementCount());
            }
            else
            {
                (void)memcpy((void* const)LonGetNvValue(index), pNvd+offset, pNv->getCurLength() * pNv->getElementCount());
            }
            offset += (pNv->getCurLength() * pNv->getElementCount());
        }
        index += pNv->getElementCount();
    }
}


void FtxlStack::notifyNvdScheduled(int timeTillUpdate) 
{
    if (m_starvationTimer != NULL)
    {
        // If it just timed out and has not been reported, cancel
        // report, since we are updating the start time.
        m_bNvdStarvedOut = FALSE;

        // compute time that we expect the write to start
        m_expectedNvdStart = tickGet() + timeTillUpdate;
        wdStart(m_starvationTimer, timeTillUpdate + m_starvationTimeout, 
                nvdStarvationTimeout, (int)this);
    }
}

void FtxlStack::notifyNvdComplete(void)
{
    if (m_starvationTimer != NULL)
    {
        wdCancel(m_starvationTimer);
    }
}

int FtxlStack::nvdStarvationTimeout(int pFtxlStack)
{
    ((FtxlStack *)pFtxlStack)->m_bNvdStarvedOut = true;
    LonEventReady();
    return 0;
}



//=======================================================================
//								Support classes
//=======================================================================

//-----------------------------------------------------------------------
//                            FtxlAppPersitencClient                           
//-----------------------------------------------------------------------

FtxlAppPersitencClient::FtxlAppPersitencClient(LonNvdSegmentType type, int version) : 
	m_persistence(version)
{
	m_persistence.registerPersistenceClient(this);
#if PERSISTENCE_TYPE_IS(FTXL)
    m_persistence.setType(type);
#endif
	m_persistence.setCommitFailureNotifyMode(true);
	m_segType = type;
}

void FtxlAppPersitencClient::serialize(unsigned char* &pBuffer, int &len)
{
	len = LonNvdGetApplicationSegmentSize();
	if (len != 0)
	{
		pBuffer = new byte[len];
		if (LonNvdSerializeSegment(pBuffer, len) != LonApiNoError)
		{
			len = 0;
		}
	}
	else
	{
		pBuffer = NULL;
	}
}

LtPersistenceLossReason FtxlAppPersitencClient::deserialize(unsigned char* pBuffer, int len, int nVersion)
{
	LtPersistenceLossReason reason = LT_PERSISTENCE_OK;
	if (LonNvdDeserializeSegment(pBuffer, len) != LonApiNoError)
	{
		reason = LT_CORRUPTION;
	}
	return reason;
}

LonApiError FtxlAppPersitencClient::segmentHasBeenUpdated()
{
	return m_persistence.schedule() ? LonApiNoError : LonApiNvdFailure;
}


LonApiError FtxlAppPersitencClient::restore(boolean &persistenceRestored)
{
	LonApiError sts = LonApiNoError;
    persistenceRestored = true;
	if (LonNvdGetApplicationSegmentSize() != 0)
	{
		// Only attempt to restore application data if the application supports that 
		// type of segment.
		if (m_persistence.restore() != LT_PERSISTENCE_OK)
		{
			// Persistent data does not exist.  Initialize it.
			segmentHasBeenUpdated();
            persistenceRestored = false;
		}
	}
	return sts;
}



