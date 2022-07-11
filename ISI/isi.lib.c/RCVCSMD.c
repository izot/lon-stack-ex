//	RcvCsmd.c	implementing _IsiReceiveCsmd
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

#ifdef	ISI_SUPPORT_CONNECTION_REMOVAL
void _IsiReceiveCsmd(const IsiCsmd* pCsmd)
{
	unsigned Connection;
	IsiConnection ConnectionData;

    _IsiAPIDebug("_IsiReceiveCsmd ");
    _IsiAPIDump("0x", (void *)pCsmd, sizeof(IsiCsmd), "\n");

	for (Connection = 0; _IsiNextConnection(Connection, &ConnectionData); ++Connection)
    {
		if (LON_GET_ATTRIBUTE(ConnectionData,ISI_CONN_STATE) >= isiConnectionStateInUse
		    && !memcmp(&ConnectionData.Header.Cid, &pCsmd->Cid, (unsigned)sizeof(IsiCid)))
        {
			// match. Remove this one locally:
            _IsiRemovePtrConnection(&ConnectionData, ConnectionData.Member);
			_IsiRemovePtrConnection(&ConnectionData, ConnectionData.Host);
			// clear this connection table entry:
			_IsiClearConnection(&ConnectionData, Connection);
		}
	}
    savePersistentData(IsiNvdSegConnectionTable);   

    // tidy-up the address table:
    _IsiSweepAddressTable();
}
#endif	//	ISI_SUPPORT_CONNECTION_REMOVAL

//	end of RcvCsmd.c
