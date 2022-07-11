//	RcvCsme.c	implementing _IsiReceiveCsme
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

void _IsiReceiveCsme(const IsiCsme* pCsme)
{
	IsiConnection Connection;
    LonByte haveCsme;

    _IsiAPIDebug("_IsiReceiveCsme ");
    _IsiAPIDump("0x", (void *)pCsme, sizeof(IsiCsme), "\n");

    if (_isiVolatile.State & isiStateInviting)
    {
        Connection = *IsiGetConnection(_isiVolatile.pendingConnection);

        haveCsme = LON_GET_ATTRIBUTE(Connection,ISI_CONN_CSME);

        if ( !haveCsme        //        .HaveCsme)
            && (Connection.Host != ISI_NO_ASSEMBLY)
            && !memcmp(&Connection.Header.Cid, &pCsme->Cid, (unsigned)sizeof(IsiCid)) )
        {
            Connection.Attributes1 |= (1 << ISI_CONN_CSME_SHIFT);	// only set this with the first applicable connection table entry! See Implemnt.c for reason
            IsiSetConnection(&Connection, _isiVolatile.pendingConnection);
            savePersistentData(IsiNvdSegConnectionTable);
#ifdef  ISI_SUPPORT_TURNAROUNDS
            IsiUpdateUserInterface(isiApprovedHost, Connection.Host);
            _isiVolatile.State &= ~HOST_STATES;
            _isiVolatile.State |= isiStatePlannedParty;
#else
            _IsiUpdateUiAndState(isiStatePlannedParty, isiApprovedHost, Connection.Host);
#endif  //  ISI_SUPPORT_TURNAROUNDS

		}
	}
}

//	end of RcvCsme.c
