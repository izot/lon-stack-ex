//	ApprCsmo.c	implementing _IsiApproveCsmo
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
//  Revision 0, February 2005, Bernd Gauweiler

#include "isi_int.h"
//#include <mem.h>

//	_IsiApproveCsmo returns FALSE if the CSMO cannot be accepted. If sufficient address table space is
//	available, or if the device is member of the CSMO's group already, the function proceeds and allocates
//	the required number of connection table entries, sets their state to Pending, and fills them in so that
//	they are "ready to go". The function may consume any connection table entry with a state numerically
//	less than InUse.
//	The connection table entry left by this function will be marked Pending, with the specified Assembly#.
LonBool _IsiApproveCsmo(const IsiCsmo* pCsmo, LonBool Auto, unsigned host, unsigned member)
{
	unsigned Index, Offset;
	signed Width;	// ASSUMES: Width field of Csmo is less than 8 bits wide.
	IsiConnection Connection;

	Width = 1;	// Default to a FALSE return

#ifdef  ISI_SUPPORT_TURNAROUNDS
    //  if this is a local csmo, the guest state bits must be clear. Otherwise, all bits must be clear (engine in normal, idle, state)
	if (!(_isiVolatile.State & GUEST_STATES)) {
#else
    if (!_isiVolatile.State) {
#endif  //  ISI_SUPPORT_TURNAROUNDS
		if (_IsiIsGroupAcceptable(pCsmo->Data.Group, FALSE) != ISI_NOT_ACCEPTABLE) {
			// the number of connection table entries required to cover the entire connection, since each connection table entry
			// may only govern up to four selectors, is Count := 1 + (Width - 1) / 4
            _isiVolatile.pendingConnection = ISI_NO_INDEX;
			Width = LON_GET_ATTRIBUTE(pCsmo->Data,ISI_CSMO_WIDTH);                // Width;
			for (Index = Offset = 0; _IsiNextConnection(Index, &Connection); ++Index)
            {
				if (Width > 0) 
                {
                    LonByte state = LON_GET_ATTRIBUTE(Connection,ISI_CONN_STATE); 
					if (state < isiConnectionStateInUse)
                    {
						// we can use this one:
                        if (_isiVolatile.pendingConnection == ISI_NO_INDEX)
                        {
                            _isiVolatile.pendingConnection = Index;
                        }

						memset(&Connection, 0, sizeof(Connection));
                        Connection.Desc.OffsetAuto = (Offset<<ConnectionOffset_SHIFT) | (Auto<<ConnectionAuto_SHIFT);

                        Connection.Host = host;
                        Connection.Member = member;

                        memcpy(&Connection.Header.Cid, &pCsmo->Header.Cid, (unsigned)sizeof(IsiCid));
						LON_SET_ATTRIBUTE(Connection,ISI_CONN_WIDTH,min(LON_GET_ATTRIBUTE(pCsmo->Data,ISI_CSMO_WIDTH), ISI_WIDTH_PER_CONNTAB));
						Connection.Header.Selector = _IsiAddSelector(pCsmo->Header.Selector, Offset);
						LON_SET_ATTRIBUTE(Connection,ISI_CONN_STATE,isiConnectionStatePending);
						Offset += ISI_WIDTH_PER_CONNTAB;
						Width -= ISI_WIDTH_PER_CONNTAB;
						goto SetConnection;  // This goto saves bytes.
					}
				} 
                else if (LON_GET_ATTRIBUTE(Connection,ISI_CONN_STATE) == isiConnectionStatePending)
                {
					// this must have been left over from a previous CSMO. Make sure to reset this to Free:
                    LON_SET_ATTRIBUTE(Connection,ISI_CONN_STATE,isiConnectionStateUnsed);
                    Connection.Host = Connection.Member = ISI_NO_ASSEMBLY;
SetConnection:
					IsiSetConnection((IsiConnection*)&Connection, Index);
				}
			}
            savePersistentData(IsiNvdSegConnectionTable);
		}
	}
	return Width <= 0;
}

//	end of ApprCsmo.c
