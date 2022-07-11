//	fetch.c	implementing IsiFetchDomain
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
//  IsiFetchDomain() kick-starts the domain sniffing process, which is used by a DAS to obtain the current domain ID
//  from any other device in the network. This remote device, the "domain ID donor", does not need to be a DAS itself,
//  since the entire procedure only uses standard messages (service pin and query domain).
//
//	Revision 0, July 2005, Bernd Gauweiler

#include "isi_int.h"

LonByte donorId[NEURON_ID_LEN];     //  the neuron ID of the donor device, or all zeroes if none

#ifdef  ISI_SUPPORT_DADAS
static void IsiFetchXxx(void)
{
    if (_isiVolatile.Running && !_isiVolatile.State)
    {
        // clear the donor ID:
        memset(donorId, 0, NEURON_ID_LEN);
        _IsiUpdateUiAndStateTimeout(ISI_T_ACQ, isiStateCollect, isiRegistered, 0);
    }
}

const IsiApiError IsiFetchDomain(void)
{
    IsiApiError sts = IsiEngineNotRunning;

    _IsiAPIDebug("Start IsiFetchDomain\n");
    if (_isiVolatile.Running)
    {
        isiDasExtState = isiDasFetchDomain;
        IsiFetchXxx();
        sts = IsiApiNoError;
    }
    _IsiAPIDebug("End IsiFetchDomain=%d\n", sts);
    return sts;
}

const IsiApiError IsiFetchDevice(void)
{
    IsiApiError sts = IsiEngineNotRunning;

    _IsiAPIDebug("Start IsiFetchDevice\n");
    if (_isiVolatile.Running)
    {
        isiDasExtState = isiDasFetchDevice_Query;
        IsiFetchXxx();
        sts = IsiApiNoError;
    }
    _IsiAPIDebug("End IsiFetchDevice=%d\n", sts);
    return sts;
}
#endif  //  ISI_SUPPORT_DADAS

//	end of fetch.c
