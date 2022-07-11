//	CtrlEnrolment.c	implementing ISI controlled enrollment
//
// Copyright © 2005-2022 Dialog Semiconductor
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

#include "isi_int.h"

#ifdef	ISI_SUPPORT_CONTROLLED_CONNECTIONS

// Send controlled enrollment response message (CTRP)
void _IsiSendCtrp(LonBool success, const LonCorrelator correlator)
{
    _IsiAPIDebug("Start _IsiSendCtrp - success=%d\n", success);

	memset(&isi_out, 0x00, (unsigned)sizeof(isi_out));
    isi_out.Header.Code = isiCtrp;
    isi_out.Msg.Ctrp.Success = success;
    memcpy(&isi_out.Msg.Ctrp.NeuronID, read_only_data.UniqueNodeId, NEURON_ID_LEN);
    LonSendResponse(correlator, ISI_MESSAGE_CODE, (const LonByte*)&isi_out, sizeof(isi_out));
    _IsiAPIDebug("End _IsiSendCtrp\n");
}

// Send controlled enrollment request message (CTRQ)
void _IsiSendCtrq(IsiCtrq* pCtrq, const LonUniqueId* pId)
{
    LonSendAddress destination;

    _IsiAPIDebug("Start _IsiSendCtrq - Control: %d Param: %d\n", pCtrq->Control, pCtrq->Parameter, pCtrq->Control, pCtrq->Parameter);

	memset(&isi_out, 0x00, (unsigned)sizeof(isi_out));
    isi_out.Header.Code = isiCtrq;
    memcpy(&isi_out.Msg.Ctrq, pCtrq, sizeof(IsiCtrq));
    _IsiNidDestination(&destination, 0x00, ISI_CTR_RETRIES, (const LonByte *)pId);
    _IsiAPIDebug("Dest NId: %02x %02x %02x %02x %02x %02x\n", 
        destination.UniqueId.NeuronId[0],destination.UniqueId.NeuronId[1], destination.UniqueId.NeuronId[2],
        destination.UniqueId.NeuronId[3],destination.UniqueId.NeuronId[4],destination.UniqueId.NeuronId[5]);
    _IsiSendIsi(LonServiceRequest, &isi_out, sizeof(isi_out), &destination);

    _IsiAPIDebug("End _IsiSendCtrq\n");
}

// Send controlled enrollment connection table reply with isiRdcf (connection table read failure), 
// or with isiRdcs (connection table read success) message for the 
// connection table index provided.
void _IsiSendConnectionTableResponse(unsigned index, LonBool success, const LonCorrelator correlator)
{
    unsigned length;

    _IsiAPIDebug("Start _IsiSendConnectionTableResponse - index=%u success=%d\n", index, success);

	memset(&isi_out, 0x00, (unsigned)sizeof(isi_out));

    if (success)
    {
        isi_out.Header.Code = isiRdcs;
        isi_out.Msg.Rdcs.Index = index;
        memcpy(&isi_out.Msg.Rdcs.Data, IsiGetConnection(index), sizeof(IsiConnection));
        length = sizeof(IsiMessageHeader)+sizeof(IsiRdcs);
    }
    else
    {
        isi_out.Header.Code = isiRdcf;
        length = sizeof(IsiMessageHeader);
    }
    LonSendResponse(correlator, ISI_MESSAGE_CODE, (const LonByte*)&isi_out, length);
    _IsiAPIDebug("End _IsiSendConnectionTableResponse\n");
}


// Issue the ISI controlled enrollment command (with parameter
// assembly) to the destination device indicated by pId.
IsiApiError IsiControlCommand(const LonUniqueId* pId, unsigned Assembly, IsiControl command)
{
    IsiCtrq ctrqData;
    IsiApiError sts = IsiEngineNotRunning;

    if (_isiVolatile.Running)
    {
        ctrqData.Control = command;
        ctrqData.Parameter = Assembly;

   	    // Prior to anything else, be sure to cancel all pending enrollment sessions:
	    _IsiUpdateUiNormal();

        _IsiSendCtrq(&ctrqData, pId);
        sts = IsiApiNoError;
    }
    return sts;
}

// Send the ISI Open controlled enrollment command (with parameter
// assembly) to the destination device indicated by pUniqueId.
IsiApiError IsiOpenControlledEnrollment(const LonUniqueId* pUniqueId, unsigned Assembly)
{
    IsiControlCommand(pUniqueId, Assembly, isiOpen); 
    return IsiApiNoError;
}

// Send the ISI Cancel controlled enrollment command (with parameter
// assembly) to the destination device indicated by pUniqueId.
IsiApiError IsiCancelControlledEnrollment(const LonUniqueId* pUniqueId, unsigned Assembly)
{
    IsiControlCommand(pUniqueId, Assembly, isiCancel); 
    return IsiApiNoError;
}

// Send the ISI Create controlled enrollment command (with parameter
// assembly) to the destination device indicated by pUniqueId.
IsiApiError IsiCreateControlledEnrollment(const LonUniqueId* pUniqueId, unsigned Assembly)
{
    IsiControlCommand(pUniqueId, Assembly, isiCreate); 
    return IsiApiNoError;
}

//  Process the controlled enrollment read connection table request.  
LonBool _IsiProcessRdctMsg(const IsiMessage* msgIn, const unsigned dataLength, const LonCorrelator correlator)
{
    unsigned connectionTableSize = IsiGetConnectionTableSize();
    unsigned index = connectionTableSize;
    
    if (_isiVolatile.Flags & isiControlledEnrollment) 
    {
        index = msgIn->Msg.Rdct.Index;

        if (msgIn->Msg.Rdct.Host != ISI_NO_ASSEMBLY)
        {
            // Try locating a connection table entry that is active and
            // report the requested assembly as the host assembly
            while (index < connectionTableSize)
            {
                const IsiConnection* pConnection = (IsiConnection *)IsiGetConnection(index);
                if (LON_GET_ATTRIBUTE_P(pConnection,ISI_CONN_STATE) >= isiConnectionStateInUse && 
                        pConnection->Host == msgIn->Msg.Rdct.Host)
                {
                    break;
                }
                ++index;
            }
        }
        else if (msgIn->Msg.Rdct.Member != ISI_NO_ASSEMBLY)
        {
            // Try locating a connection table entry that is active and
            // report the requested assembly as the member assembly
            while (index < connectionTableSize)
            {
                const IsiConnection* pConnection = (IsiConnection *)IsiGetConnection(index);
                if (LON_GET_ATTRIBUTE_P(pConnection,ISI_CONN_STATE) >= isiConnectionStateInUse && 
                        pConnection->Member == msgIn->Msg.Rdct.Member)
                {
                    break;
                }
                ++index;
            }
        }
    }
    // Reply with
    //  isiRdcs for Controlled enrollment read connection table success
    //  isiRdcf for Controlled enrollment read connection table failure
    _IsiSendConnectionTableResponse(index, index < connectionTableSize, correlator);

    return TRUE;
}

//  Process the controlled enrollment request message.  
LonBool _IsiProcessCtrqMsg(const IsiMessage* msgIn, const unsigned dataLength, const LonCorrelator correlator)
{
    LonBool isProcessed = FALSE;

    if (_isiVolatile.Flags & isiControlledEnrollment) 
    {
        // This is a controlled enrollment response
        if (msgIn->Msg.Ctrq.Control == isiOpen)
        {
            _IsiSendCtrp(TRUE, correlator);
            IsiOpenEnrollment(msgIn->Msg.Ctrq.Parameter);
            isProcessed = TRUE;
        }
        else
        if (msgIn->Msg.Ctrq.Control == isiCreate)
        {   
            _IsiSendCtrp(TRUE, correlator);
            IsiCreateEnrollment(msgIn->Msg.Ctrq.Parameter);
            isProcessed = TRUE;
        }
        else
        if (msgIn->Msg.Ctrq.Control == isiFactory)
        {
            _IsiSendCtrp(TRUE, correlator);
            IsiReturnToFactoryDefaults();
            isProcessed = TRUE;
        }
    }
    if (!isProcessed)
    {
        // Controlled enrollment is not supported  
        _IsiSendCtrp(FALSE, correlator);
    }

    return TRUE;
}

// Handle ISI controlled enrollment request message and controlled enrollment connection table request message
// Return TRUE if the message is processed.  Otherwise, it returns FALSE.
LonBool IsiProcessCtrlEnrollmentRequest(const LonByte code, const IsiMessage* msgIn, const unsigned dataLength, const LonCorrelator correlator)
{
    LonBool isProcessed = FALSE;

    IsiMessageCode isiCode = msgIn->Header.Code;

    if (code != ISI_MESSAGE_CODE)
        return isProcessed;        // It's not ISI message

    if (isiCode == isiCtrq)
        isProcessed = _IsiProcessCtrqMsg(msgIn, dataLength, correlator);        // It's a Controlled enrollment request message
    else
    if (isiCode == isiRdct)
        isProcessed = _IsiProcessRdctMsg(msgIn, dataLength, correlator);        // It's a Controlled enrollment read connection table request message
    return isProcessed;
}

// Send read connection table request for destination address and 
// connection table index and assembly information.
void IsiRequestConnectionTable(const LonUniqueId* pUniqueId, LonByte index, unsigned hostAssembly, unsigned memberAssembly) 
{
    _IsiRequestConnectionTable(pUniqueId, index, hostAssembly, memberAssembly);
}

void _IsiRequestConnectionTable(const LonUniqueId* pUniqueId, LonByte index, unsigned hostAssembly, unsigned memberAssembly) 
{
    LonSendAddress destination;

    _IsiAPIDebug("Start IsiRequestConnectionTable - Index: %d hostAssembly: %d memberAssembly: %d\n", 
            index, hostAssembly, memberAssembly); 

	memset(&isi_out, 0x00, (unsigned)sizeof(isi_out));
    isi_out.Header.Code = isiRdct;
    isi_out.Msg.Rdct.Host = hostAssembly;
    isi_out.Msg.Rdct.Index = index;
    isi_out.Msg.Rdct.Member = memberAssembly;
    _IsiNidDestination(&destination, 0x00, ISI_RDC_RETRIES, (const LonByte *)pUniqueId);
    _IsiAPIDebug("Dest NId: %02x %02x %02x %02x %02x %02x\n", 
        destination.UniqueId.NeuronId[0],destination.UniqueId.NeuronId[1], destination.UniqueId.NeuronId[2],
        destination.UniqueId.NeuronId[3],destination.UniqueId.NeuronId[4],destination.UniqueId.NeuronId[5]);
    _IsiSendIsi(LonServiceRequest, &isi_out, sizeof(isi_out), &destination);

    _IsiAPIDebug("End IsiRequestConnectionTable\n");
}


#endif	//	ISI_SUPPORT_CONTROLLED_CONNECTIONS


//	end of SndCtrq.c
