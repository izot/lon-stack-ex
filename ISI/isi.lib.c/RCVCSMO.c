//	RcvCsmo.c	implementing _IsiReceiveCsmo
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
//	Revision 1, June 2005, Bernd Gauweiler: added support for CSMAEX/CSMOEX/CSMREX
//  	29-July-2013, Bernd Gauweiler:
//	The primary CSMO receiver is _IsiReceiveCsmo. In the original (Neuron C)
//	implementation, message processors used to access the incoming message data
//	through pointers into the incoming message buffer. However, with the
//	addition of support for turnaround connections, we had to process some of
//	these messages from other locations, pretending that we just received them.
//	That's why _IsiReceiveCsmo redirects to _IsiReceivePtrCsmo, while code that
//	processes the turnaround enrollment offer calls the latter directly.
//
//	The plain receiver (IsiReceiveCsmo) creates a local temporary copy of the
//	message so that the second-level processor _IsiReceivePtrCsmo can modify
//	this data if required - this may be required to handle short messages.
//
#include "isi_int.h"
//#include <mem.h>
#include <stddef.h>
#ifdef WIN32
#include <control.h>
#endif

void _IsiReceiveCsmo(LonBool Auto, LonBool ShortForm, const IsiCsmo* pCsmo)
{
	IsiCsmo csmo = *pCsmo;
    _IsiReceivePtrCsmo(&csmo, ISI_NO_ASSEMBLY, Auto, ShortForm, FALSE);
}

void _IsiReceivePtrCsmo(IsiCsmo* pCsmo, unsigned localHostAssembly, LonBool Auto, LonBool ShortForm, LonBool IsLocalTurnaroundCsmo)
{
	// this CSMO might be a duplicate of one that we received earlier. Find out:
	unsigned Connection;
	const IsiConnection* pConnection;
	unsigned Assembly, minimumDuplicateState;

    if (ShortForm)
    {
		// when receiving a short enrollment message (CSMO, CSMA, CSMR), we simply
		// fill in the extended fields with reasonable defaults and treat short and
		// extended messages alike:
		memset(&pCsmo->Data.Extended, 0, (unsigned)sizeof(pCsmo->Data.Extended));
		pCsmo->Data.Extended.Member = 1u;
	}

    // minimumDuplicateState: if we are engaged in some pending enrollment, a CSMO might be a duplicate if a matching connection table
    // entry is found in pending, inuse, of higher. However, if we are NOT engaged in a pending enrollment, only those connection table
    // entries that refer to actually implemented connections count (isiInUse or higher).
    // Consider this scenario: open enrollment on device A. Device B starts flashing and its assemblies enter pending state. Now reset
    // device B. When it's back up and running again and the next CSMO re-send arrives, device B must not consider pending connection
    // table entries a duplicate (because the persistent connection table has been left in that state by the untimely reset).
    minimumDuplicateState = _isiVolatile.State & CONNECTION_STATES ? isiConnectionStatePending : isiConnectionStateInUse;

    // find out if this is a duplicate (re-send) of a CSMO received earlier. Quietly ignore these
    // duplicates, as the connection host keeps sending these for increased reach.
	for(Connection = 0; Connection < _isiVolatile.ConnectionTableSize; ++Connection)
    {
		pConnection = IsiGetConnection(Connection);
		if ((unsigned)LON_GET_ATTRIBUTE_P(pConnection,ISI_CONN_STATE) >= minimumDuplicateState
		    && !memcmp(&pConnection->Header.Cid, &pCsmo->Header.Cid, sizeof(IsiCid))
#ifdef  ISI_SUPPORT_TURNAROUNDS
             // do not mistake those locally hosted for duplicates...
            && !IsLocalTurnaroundCsmo
#endif  //  ISI_SUPPORT_TURNAROUNDS
        )
        {
			// this is a duplicate. Get out of here:
			return;
		}
        watchdog_update();
	}

    //  CSMO/A/R is not a duplicate.
    //  EPR 39003: a CSMO/A/R must be ignored unless then engine is idle. This ISI implementation has an enrollment engine
    //  governed by the CONNECTION_STATE state flags, and an engine concerned with domain configuration, governed by the
    //  remaining state flags. Currently, both engines cannot run concurrently: they share the _isiVolatile.Timeout variable.
    //  We could spin-off a separate 16-but timeout timer for the engine that deals with domain configuration, but it makes
    //  no sense: the engine dealing with the domain configuration will most likely introduce a change to the domain cfg.
    //  This is a bad time to fiddle with any connection-related enrollment business, so we simply ignore any CSMA/O/R
    //  arriving unless we are idle. The domain-related services can only be started in the idle state also:
    //
    if (_isiVolatile.State & ~CONNECTION_STATES) return;

    // not a duplicate. If we are already engaged in some pending enrollment (in whichever role),
    // we chancel out:
    if (!IsLocalTurnaroundCsmo)
    {
        IsiCancelEnrollment();
    }

    if (_IsiApproveCsmo(pCsmo, Auto, localHostAssembly, ISI_NO_ASSEMBLY))
    {
		//  This connection is provisionally approved and might be accepted.
		//	Ask the application to provide a mapping to the assemblies. For each
		//	assembly advised, notify the UI driver in turn.
		Assembly = IsiGetAssembly(&pCsmo->Data, Auto, ISI_NO_ASSEMBLY);
		_isiVolatile.Group = pCsmo->Data.Group;
		while (Assembly != ISI_NO_ASSEMBLY)
        {
#ifdef  ISI_SUPPORT_TURNAROUNDS
            if (Assembly != localHostAssembly)
            {
                // when a local host invites other local assemblies, it cannot invite itself. Must skip the local host assembly:
#else
            {
#endif  //  ISI_SUPPORT_TURNAROUNDS

#ifdef	ISI_SUPPORT_CONNECTION_REMOVAL
                {
#else
    			// if we have no support for connection removal, then we cannot really replace existing connections on the fly;
    			// one must return the device to factory defaults to do so. Thus, in the absence of support for connection removal,
    			// we only put those assemblies into the pending state that are actually unconnected.
    			if (!IsiIsConnected(Assembly))
                {
#endif  //  ISI_SUPPORT_CONNECTION_REMOVAL

#ifdef  ISI_SUPPORT_TURNAROUNDS
                    // if we support turnarounds, we must be careful not to wipe out the host state flags, but just to OR in the
                    // guest state flags.
                    _isiVolatile.Timeout = ISI_T_ENROLL;
                    IsiUpdateUserInterface(isiPending, Assembly);
                    _isiVolatile.State |= isiStateInvited;
#else
                    _IsiUpdateUiAndStateEnroll(isiStateInvited, isiPending, Assembly);
#endif  //  ISI_SUPPORT_TURNAROUNDS

#ifdef	ISI_SUPPORT_AUTOMATIC_CONNECTIONS
    				if (Auto)
                    {
#ifdef ISI_SUPPORT_ALIAS
    					// 17-Oct-2013  Bernd: ISI_SUPPORT_ALIAS means that we can support connection extensions. If we can,
    					// we will now automatically extend a previously existing connection if that connection also was made
    					// automatically. If we can't support extensions, we don't have a choice and must replace. If we can't
    					// replace (ISI_SUPPORT_CONNECTION_REMOVAL), we aborted acceptance out a few lines ago already.
    					// This behavior change is an implementation-specific detail not discussed in the ISI protocol document
    					// as far as I can tell, so we can just make this enhancement for ISI on IzoT without worrying about
    					// an ISI protocol violation.
    					_IsiAcceptEnrollment(IsiIsAutomaticallyEnrolled(Assembly), Assembly);
#else
    					_IsiAcceptEnrollment(FALSE, Assembly);
#endif	//	ISI_SUPPORT_ALIAS
    					// automatic connections, once approved, get automatically implemented:
    				   _IsiReceiveCsmc((const IsiCsmc*)pCsmo);

    				   //  because IsiApproveCsmo books sufficient connection table space for just
    				   //  one assembly, we cannot automatically accept any CSMA/CSMR with more than
    				   //  one assembly. Thus, we're done now and must break the loop:
    				   break;
    				}
#endif	//	ISI_SUPPORT_AUTOMATIC_CONNECTIONS
                }
            }   //  assembly != localHostAssembly
			Assembly = IsiGetAssembly(&pCsmo->Data, Auto, Assembly);
		}
	}
}

//	end of RcvCsmo.c
