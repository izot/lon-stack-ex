//	DefaultCallback.c	implementing Isi default callback
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
const LonBool isiFilterResponseArrived(const LonResponseAddress* const pAddress, const unsigned tag, 
                        const LonByte code, const LonByte* const pData, const unsigned dataLength)
{
    // The IsiProcessResponse returns FALSE if the response is processed by the ISI engine.
    LonBool retval = !IsiProcessResponse(_IsiGetCurrentType(), code, pData, dataLength);
    _IsiAPIDebug("isiFilterResponseArrived = %d\n", retval);
    return retval;
}

const LonBool isiFilterMsgArrived(const LonReceiveAddress* const pAddress,
                          const LonCorrelator correlator,
                          const LonBool priority,
                          const LonServiceType serviceType,
                          const LonBool authenticated,
                          const LonByte code,
                          const LonByte* const pData, const unsigned dataLength)
{
    LonBool retval = FALSE;
    LonBool isIsiApproveMsg;

    _IsiAPIDebug("Incoming MSG code %02X, length %u, data ", (unsigned)code, dataLength);
    _IsiAPIDump("0x", (void*)pData, dataLength, "\n");

    if (_IsiGetCurrentType() == isiTypeDas)
        isIsiApproveMsg = IsiApproveMsgDas(code, pData, dataLength);
    else
        isIsiApproveMsg = IsiApproveMsg(code, pData, dataLength);

    // The IsiProcessMsg returns FALSE if the message is processed by the ISI engine.  
    if (isIsiApproveMsg && (!IsiProcessMsg(_IsiGetCurrentType(), code, (IsiMessage *)pData, dataLength)
#ifdef	ISI_SUPPORT_CONTROLLED_CONNECTIONS
            || IsiProcessCtrlEnrollmentRequest(code, (IsiMessage *)pData, dataLength, correlator)
#endif
        ))
    {
        retval = TRUE;
    }
    // else the message does not get processed by the ISI engine.  Left for application to deal with.
    _IsiAPIDebug("isiFilterMsgArrivedStart = %d\n", retval);
    return retval;
}

//	end of DefaultCallback.c
