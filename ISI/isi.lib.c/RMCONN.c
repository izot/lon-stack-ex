//	RmConn.c	implementing IsiRemoveConnection
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
//

#include "isi_int.h"

#ifdef ISI_SUPPORT_CONNECTION_REMOVAL

void _IsiRemoveConnection(IsiState RequiredState, unsigned Assembly, LonBool Global)
{
	unsigned Connection;
	IsiConnection ConnectionData;

	if (_isiVolatile.State == RequiredState)
    {
		for (Connection = 0;_IsiNextConnection(Connection, &ConnectionData); ++Connection)
        {
			if ((ConnectionData.Host == Assembly || ConnectionData.Member == Assembly) &&
                    LON_GET_ATTRIBUTE(ConnectionData,ISI_CONN_STATE) >= isiConnectionStateInUse)
            {
				_IsiAPIDebug(
						"_IsiRemoveConnection: removing connection %d for assembly %d\n",
						Connection, Assembly
				);

                // we'll always remove the member: either the requested assembly is the member (in which case it ought to be removed,
                // or the requested assembly is the host (in which case the entire connection gets removed, including the local member).
                // Note _IsiRemovePtrConnection does nothing if its second argument is ISI_NO_ASSEMBLY, so calling it "just in case" is OK.
                _IsiRemovePtrConnection(&ConnectionData, ConnectionData.Member);
				if (Global || (ConnectionData.Host == Assembly))
                {
                    // this is a local host. Notify the network, disconnect the host locally, and disconnect the local member (if any):
					if(!(LON_GET_ATTRIBUTE(ConnectionData.Desc.Bf,ConnectionOffset)))          // .Offset)
                    {
                        // only notify for the first connection table entry (EPR 37408)
                        _IsiSendCsmX(&ConnectionData, isiCsmd, 3);
                    }
                    // if there is a turnaround, make sure to delete the local member, too:
                    _IsiRemovePtrConnection(&ConnectionData, ConnectionData.Host);
                    // once we are done, notify the network again. This is simply to increase probability of delivery. The engine has no
                    // mechanism to continuously send CSMD messages as the required knowledge (the connection table entry) is lost once this
                    // routine is complete. Even if that data wasn't available, the engine wouldn't know for how long to resend CSMD messages,
                    // so it is just as well not to resend these messages at all. For slightly improved chances, we resend the CSMD again
                    // once all local removal procedure has been completed, as the last step before we actually lose the required knowledge:
                    if (!(LON_GET_ATTRIBUTE(ConnectionData.Desc.Bf,ConnectionOffset))) 
                    {
                        _IsiSendCsmX(&ConnectionData, isiCsmd, 3);
                    }
                    _IsiClearConnection(&ConnectionData, Connection);
                } 
                else
                {
                    // remove a local member, but leave the remaining connection intact
                    ConnectionData.Member = ISI_NO_ASSEMBLY;
                    IsiSetConnection(&ConnectionData, Connection);
                }   // leave or delete
			}   //  this conntab entry
		}   // all conntab entries
        savePersistentData(IsiNvdSegConnectionTable);
        // now look into the address table
        _IsiSweepAddressTable();
	}   // state OK
}
#endif	//	ISI_SUPPORT_CONNECTION_REMOVAL

//	end of RmConn.c
