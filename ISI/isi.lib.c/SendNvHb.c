//	SendNvHb.c	implementing _IsiSendNvHb
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
//	Revision 0, March 2005, Bernd Gauweiler

#ifdef WIN32
#include <control.h>
#endif
#include "isi_int.h"

#ifdef	ISI_SUPPORT_HEARTBEATS

static unsigned nextNv = 0;

LonBool _IsiIsHeartbeatCandidate(unsigned nv)
{
    union
    {
        const LonNvEcsConfig *pNv;
        const LonAliasEcsConfig *pAlias;
    } data;

#ifdef  ISI_SUPPORT_ALIAS
    unsigned aliasIndex;
#endif  //  ISI_SUPPORT_ALIAS

    data.pNv = IsiGetNv(nv);

    if (LON_GET_ATTRIBUTE_P(data.pNv, LON_NV_ECS_DIRECTION))    
    {
        // Say Yes (in principle) to output network variables
        if (LON_GET_ATTRIBUTE_P(data.pNv,LON_NV_ECS_SELHIGH) < 0x30u)
        {
            // the primary is bound. OK
            return TRUE;
        }
#ifdef  ISI_SUPPORT_ALIAS
        for (aliasIndex = 0; aliasIndex < AliasCount; ++aliasIndex)
        {
            data.pAlias = IsiGetAlias(aliasIndex);
            if (LON_GET_UNSIGNED_WORD(data.pAlias->Primary) == nv)
            {
                return TRUE;
            }
            watchdog_update();
        }
#endif  //  ISI_SUPPORT_ALIAS
    }
    return FALSE;
}

LonBool _IsiSendNvHb(void) {
    unsigned visited;
	LonBool hadOne;

    visited = hadOne = FALSE;

    while (!hadOne && visited++ < NvCount)
    {
        // _IsiIsHeartbeatCandidate says TRUE if the NV is an output and if the NV, or any of its aliases, is bound.
        if (_IsiIsHeartbeatCandidate(nextNv))
        {
            // IsiQueryHeartbeat is a callback typically overridden by the application. The default always says false
            // and never issues a heartbeat. The typical application override verifies that the NV index is good for
            // heartbeats and, if so, call IsiIssueHeartbeat() for this index. IsiIssueHeartbeat() must be called from
            // the application-defined override so that this code does not get linked to the application image unless
            // it is actually used.
            hadOne = IsiQueryHeartbeat(nextNv);
        }
        watchdog_update();
        ++nextNv;
        nextNv = nextNv % NvCount;
    }
	return hadOne;
}
#endif	//	ISI_SUPPORT_HEARTBEATS

//	end of SendNvHb.c
