//	IssuNvHb.c	implementing IsiIssueHeartbeat
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
//	Revision 0, July 2005, Bernd Gauweiler
//
#if 0
#include <access.h>
#include <msg_addr.h>
#include <addrdefs.h>
#ifdef WIN32
#include <control.h>
#endif
#endif

#include "isi_int.h"

LonBool IsiIssueHeartbeat(unsigned NvIndex)
{
    LonBool hadOne;
    union {
        const LonNvEcsConfig *pNv;
        const LonAliasEcsConfig *pAlias;
    } data;
    const LonByte* pNvData;
    unsigned length;
#ifdef  ISI_SUPPORT_ALIAS
    unsigned alias;
#endif  //  ISI_SUPPORT_ALIAS

    hadOne = FALSE;

    if (_isiVolatile.Running && _IsiIsHeartbeatCandidate(NvIndex))
    {
        pNvData = IsiGetNvValue(NvIndex, &length);
        data.pNv = IsiGetNv(NvIndex);

        if (LON_GET_ATTRIBUTE_P(data.pNv,LON_NV_ECS_SELHIGH) < 0x30)    //  ->nv_selector_hi < 0x30)
        {
            // OK, this one is bound. Note we could have an unbound primary but bound aliases - when we
            // remove connections, we don't free up aliases. So, IsiIsHeartbeatCandidate says TRUE if the
            // NV in question is bound *somehow*. Here, we can only propagate what is actually bound.
            if (_IsiPropagateNvHb(data.pNv, pNvData, length) && !hadOne)
                hadOne = TRUE;
        }
#ifdef  ISI_SUPPORT_ALIAS
        // now let's go through the aliases:
        for (alias=0; alias < AliasCount; ++alias)
        {
            data.pAlias = IsiGetAlias(alias);
            if (LON_GET_UNSIGNED_WORD(data.pAlias->Primary) == NvIndex)
            {
                // We assume all associated aliases to be bound. Unbound aliases are disassociated from their (former) primary:
                if (_IsiPropagateNvHb(&data.pAlias->Alias, pNvData, length) && !hadOne)
                    hadOne = TRUE;
            }
            watchdog_update();
        }
#endif  //  ISI_SUPPORT_ALIAS
    }
    return hadOne;
}

LonBool _IsiPropagateNvHb(const LonNvEcsConfig* pNv, const LonByte *pData, unsigned length)
{
    LonBool ok;
    LonByte data[31+1];        // buffer for low selector and NV data
    LonByte groupType;
    union {
        LonSendAddress msg_destination;   // where is this update going to?
        LonAddressTableGroup group;
    } destination;
    const LonAddress* pAddress;

    ok = FALSE;

    data[0] = LON_GET_ATTRIBUTE_P(pNv,LON_NV_ECS_SELHIGH);  
    memcpy(data+1, pData, length);

    pAddress = access_address(LON_GET_UNSIGNED_WORD(pNv->AddressIndex));     // >nv_addr_index);
    groupType = LON_GET_ATTRIBUTE(pAddress->Group,LON_ADDRESS_GROUP_TYPE);    
    if (groupType) 
    {
        ok = TRUE;
        memcpy(&destination.group, &pAddress->Group, sizeof(LonAddressTableGroup));
        // in msg_out_addr w/ group addressing, the rcv_timer should not be specified (see addrdefs.h for details)
        LON_SET_ATTRIBUTE(destination.group,LON_ADDRESS_GROUP_RECEIVE_TIMER,0);     // rcv_timer = 0;
        // ISI NVHB always travel with just one repeat, no matter what the address table says, so that NVHB can
        // participate with the ISI broadcast scheduler scheme
        LON_SET_ATTRIBUTE(destination.group,LON_ADDRESS_GROUP_RETRY,ISI_NVHB_REPEATS);      // retry = ISI_NVHB_REPEATS

        _IsiSend(0x80 | LON_GET_ATTRIBUTE_P(pNv,LON_NV_ECS_SELHIGH), LonServiceRepeated, data, length+1, &destination.msg_destination);
    }

    return ok;
}

//	end of IssuNvHb.c
