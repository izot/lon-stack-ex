//	RcvCsmc.c	implementing _IsiReceiveCsmc
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
//	Revision 0, February 2005, Bernd Gauweiler

#include "isi_int.h"

void _IsiReceiveCsmc(const IsiCsmc* pCsmc)
{
	const IsiConnection *pConnection;
    LonByte connectionState, connectionExtend;

    _IsiAPIDebug("_IsiReceiveCsmc ");
    _IsiAPIDump("0x", (void *)pCsmc, sizeof(IsiCsmc), "\n");

    if (_isiVolatile.State != isiStateAccepted)
    {
        // the CSMC didn't really apply to us. It seems we were still pending for a different connection,
        // maybe because we missed a number of messages...? Let's call _IsiReceiveCsmx(), just to clear out
        // the state.
        _IsiReceiveCsmx(pCsmc);
    }
    else
    {
        // search the connection table. This time we are looking for the first not-hosted but pending entry.
        // Take the extend flag and assembly number from there, and implement the darn thing:
        pConnection = IsiGetConnection(_isiVolatile.pendingConnection);
        connectionState = LON_GET_ATTRIBUTE_P(pConnection,ISI_CONN_STATE);       
        if (pConnection->Member != ISI_NO_ASSEMBLY
            && connectionState == isiConnectionStatePending
            && !memcmp(&pConnection->Header.Cid, &pCsmc->Cid, (unsigned)sizeof(IsiCid)))
        {
            // this connection table entry is one that refers to a remotely hosted connection (we are just a plain member):
            connectionExtend = LON_GET_ATTRIBUTE_P(pConnection,ISI_CONN_EXTEND);
            _IsiImplementEnrollment(connectionExtend, pConnection->Member);
        }
    }
}

//	end of RcvCsmc.c
