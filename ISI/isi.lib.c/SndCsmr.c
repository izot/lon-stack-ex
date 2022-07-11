//	SndCsmr.c	implementing _IsiSendCsmr
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
//	Revision 1, June 2005, Bernd Gauweiler: added support for CSMR and CSMREX short and extended messages


#include "isi_int.h"
//#include <mem.h>

void _IsiSendCsmr(unsigned Index, const IsiConnection* pConnection)
{
#ifdef  ISI_SUPPORT_DADAS
    IsiConnection connection = *pConnection;

    _IsiAPIDebug("Start _IsiSendCsmr - Index=%d ConnectionState=%d\n", Index, LON_GET_ATTRIBUTE_P(pConnection,ISI_CONN_STATE));

    IsiCreateCsmo(connection.Host, &isi_out.Msg.Csmr.Data);
    memcpy(&isi_out.Msg.Csmr.Header, &connection.Header, (unsigned)sizeof(connection.Header));
    // A device supporting automatic connections and ISI-DA or ISI-DAS is supposed to monitor DIDCF messages,
    // and Tcsmr after receiving a Tcsmr message, to broadcast all CSMR messages - the idea is to speed up the
    // automatic connection process when a new device is registered on the domain. To do so, each ISI-DA(S)
    // device receiving a DIDCF flags all automatic, locally hosted, connections with that new state.
    // Whenever the regular broadcaster sends a CSMR, it resets this connections's state to isiConnectionStateInUse.
    // When the Tcsmr timer expires following the receipt of a DIDCF, the TickDA and TickDas routines broadcast
    // each remaining CSMR.
    if (LON_GET_ATTRIBUTE_P(pConnection,ISI_CONN_STATE) > isiConnectionStateInUse)
    {
        LON_SET_ATTRIBUTE(connection,ISI_CONN_STATE,isiConnectionStateInUse);
        IsiSetConnection(&connection, Index);
        savePersistentData(IsiNvdSegConnectionTable);
    }
#else
    IsiCreateCsmo(pConnection->Host, &isi_out.Msg.Csmr.Data);
    memcpy(&isi_out.Msg.Csmr.Header, &pConnection->Header, (unsigned)sizeof(pConnection->Header));
#   pragma  ignore_notused  Index
#endif
    _IsiBroadcast(isiCsmr, ISI_PRIMARY_DOMAIN_INDEX, 1);
    _IsiAPIDebug("End _IsiSendCsmr\n");
}

//	end of SndCsmr.c
