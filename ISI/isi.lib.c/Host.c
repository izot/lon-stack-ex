//	Host.c	implementing _IsiBecomeHost
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
//	Revision 1, June 2005, Bernd Gauweiler: added support for short and extended variants of the CSMO/A messages


#include "isi_int.h"
//#include <mem.h>

#ifdef  ISI_SUPPORT_TURNAROUNDS
//  _IsiHaveAtLeastOneOutputNv does pretty much what the name suggests: the function returns TRUE if the given
//  assembly contains at least one output NV, and returns FALSE otherwise.
LonBool _IsiHaveAtLeastOneOutputNv(unsigned Assembly)
{
    unsigned result;
    unsigned nv;
    unsigned width;
    const LonNvEcsConfig* pNv;

    result = FALSE;

    width = IsiGetWidth(Assembly);
    while (width--) {
        // note we benefit from postdecrementing width, as width is 1-based, but offset (as used in the IsiGet[Next]Nv()
        // calls, is 0-based.
        nv = IsiGetNvIndex(Assembly, width, ISI_NO_INDEX);
        while (nv != ISI_NO_INDEX)
        {
            pNv = IsiGetNv(nv);
            if (LON_GET_ATTRIBUTE_P(pNv,LON_NV_ECS_DIRECTION))  // nv_direction)
            {
                // its an output:
                result = TRUE;
                goto done;
            }
            nv = IsiGetNvIndex(Assembly, width, nv);
        }
    }
done:
    return result;
}
#endif  // ISI_SUPPORT_TURNAROUNDS

LonBool _IsiBecomeHost(unsigned Assembly, LonBool Auto, const IsiCsmoData* pData)
{
	// Prior to anything else, be sure to cancel all pending enrollment sessions:
	_IsiUpdateUiNormal();

	// Create the CSMO with application-specific data:
	if (pData != NULL)
    {
		memcpy(&isi_out.Msg.Csmo.Data, pData, (unsigned)sizeof(IsiCsmoData));
	}
	else
	{
		IsiCreateCsmo(Assembly, &isi_out.Msg.Csmo.Data);
	}

	if (!_IsiCreateCid())
    {
        return FALSE;   // Can't find unused serial number in the 0-245 value range among all local in-use connection table records
    }

	isi_out.Msg.Csmo.Header.Selector = _IsiGetSelectors(LON_GET_ATTRIBUTE(isi_out.Msg.Csmo.Data,ISI_CSMO_WIDTH));     // .Width);

	//	find sufficient connection table space, and set it up:
	if (_IsiApproveCsmo(&isi_out.Msg.Csmo, Auto, Assembly, ISI_NO_ASSEMBLY))
    {
		_isiVolatile.Group = isi_out.Msg.Csmo.Data.Group;
		_isiVolatile.ShortTimer = ISI_T_CSMO;
        // a bold state assignment is OK here even for turnaround support: this is the first step in becoming
        // a local host; the straight-forward assignment automatically clears the guest states as needed at this stage:
		_IsiUpdateUiAndStateEnroll(isiStateInviting, isiPendingHost, Assembly);

#ifdef  ISI_SUPPORT_TURNAROUNDS
        // if turnaround support is enabled and this local device is about to become the host,
        // fake an incoming CSMO (turn arounds the CSMO broadcast itself) so that the local
        // device may join into the new connection.
        // Notice that ISI turnarounds are limited to host assemblies that have at least one
        // NVO and to manual connections.
        if (!Auto && _IsiHaveAtLeastOneOutputNv(Assembly))
        {
            _IsiReceivePtrCsmo(&isi_out.Msg.Csmo, Assembly, FALSE, !(_isiVolatile.Flags & isiFlagExtended), TRUE);
        }

        // EPR 38633: under some conditions, the call to _IsiReceivePtrCsmo could stop the Timeout timer at zero,
        // and the open enrollment would no longer time out. So, no matter what happend in the turnaround, let's
        // just make sure we've got the enrollment timout timer set correctly:
        _isiVolatile.Timeout = ISI_T_ENROLL;
#endif  //  ISI_SUPPORTS_TURNAROUNDS

		_IsiBroadcast(Auto ? isiCsma : isiCsmo, ISI_PRIMARY_DOMAIN_INDEX, 3);
		return TRUE;
	}
	return FALSE;
}

//	end of Host.c
