//	CancelE.c	implementing IsiCancelEnrollment
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

#ifdef WIN32
#include <control.h>
#endif
#include "isi_int.h"

const IsiApiError IsiCancelEnrollment(void)
{
    unsigned index;
    IsiConnection connection;
    IsiApiError sts = IsiEngineNotRunning;

    _IsiAPIDebug("Start IsiCancelEnrollment - State=%d\n", _isiVolatile.State); 

    if (_isiVolatile.Running)
    {
	    if (_isiVolatile.State & HOST_STATES)
        {
		    // this happens on the host.
 		    _IsiSendCsmx();
	    }
	    if (_isiVolatile.State & CONNECTION_STATES)
        {
		    // this happens on the guest and host

            //  we clear out the connection table from all pending connections. This is not required stricly speaking; we could just
            //  leave the pending entries behind and set the global state to normal. This is sufficient (and the pending but stale
            //  connection table entries get re-used with the next invitation), but it prevents the engine from accepting the next
            //  resend CSMI that applies to the same connection: this re-send would be recognized as a duplicate, and ignored (see
            //  rcvcsmo.c). Doing this extra bit of cleaning allows to accept assembly #1, then cancel this acceptance, and (a minute
            //  or so later, as soon as the next CSMI came in), to accept assembly #2 instead.
            for (index = _isiVolatile.pendingConnection; _IsiNextConnection(index, &connection); ++index) 
                // if (connection.State == isiConnectionStatePending)
                if (LON_GET_ATTRIBUTE(connection,ISI_CONN_STATE) == isiConnectionStatePending)
                {
                    _IsiClearConnection(&connection, index);
                }
            savePersistentData(IsiNvdSegConnectionTable);   

            //
            // EPR 38983+38960: Part of the problem described in these EPRs is that enrollment cancellation, which might occur automatically as result of
            // receiving a new CSMO/A/R while being the host of another pending enrollment, does not notify the UI upon cancellation, so let's fire the
            // cancellation event instead of the isiNormal event
            _IsiUpdateUi(isiCancelled);
            // ASSUMES: isiStateNormal == 0
		    _isiVolatile.State &= ~CONNECTION_STATES;
	    }
        sts = IsiApiNoError;
    }
    _IsiAPIDebug("End IsiCancelEnrollment=%d\n", sts);
    return sts;
}

//	end of CancelE.c
