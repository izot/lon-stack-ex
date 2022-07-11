//	RcvCsmx.c	implementing _IsiReceiveCsmx
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

//	Notice the _IsiReceiveCsmx() function is called whenever a CSMX (connection cancellation) message comes in,
//	and when _IsiReceiveCsmc() (CSMC = connection confirmation) detects that we might still be pending for a no
//	longer valid enrollment.

void _IsiReceiveCsmx(const IsiCsmx* pCsmx)
{
    unsigned Connection;
    IsiConnection ConnectionData;

    _IsiAPIDebug("_IsiReceiveCsmx ");
    _IsiAPIDump("0x", (void *)pCsmx, sizeof(IsiCsmx), "\n");

	if (_isiVolatile.State & CONNECTION_STATES)
    {
        for (Connection = 0; _IsiNextConnection(Connection, &ConnectionData); ++Connection)
        {
            if ((LON_GET_ATTRIBUTE(ConnectionData,ISI_CONN_STATE)  == isiConnectionStatePending)
                && !memcmp(&ConnectionData.Header.Cid, &pCsmx->Cid, (unsigned)sizeof(IsiCid)))
            {
                // match. Cancel this one locally - notice we must cancel for all assemblies (hence ISI_NO_ASSEMBLY,
                // which should be read in this context as "no specific assembly". This is required because, subject
                // to this device's local enrollment status, specific assemblies might not yet be known and registered
                // in the connection table record.
                IsiUpdateUserInterface(isiCancelled, ISI_NO_ASSEMBLY);
                _isiVolatile.State &= ~CONNECTION_STATES;
            }
        }
	}
}

//	end of RcvCsmx.c
