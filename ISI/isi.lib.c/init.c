//	init.c	implementing _IsiInitialize
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
//	revision 0, January 2005, Bernd Gauweiler
//	revision 1, June 2005, Bernd: Added support for IsiGetRepeatCount

#include "isi_int.h"
#ifdef WIN32
#include <control.h>
#endif
#include <stddef.h>
//#include <mem.h>

void _IsiInitialize(IsiFlags Flags)
{
    const LonDomain* pDomainCfg;
	int DomCfgLength;
    LonByte DomainIdLength;
    const LonByte* pId;
    unsigned int address_count;
    unsigned uIdx, uSelLow;
    union {
        LonAliasEcsConfig   Alias;
        IsiConnection  Connection;
        LonAddress Address;
    } Preset;

    memset(&_isiVolatile, 0, sizeof(_isiVolatile));

    // store behavior flags as requested. Store them right-away so that we don't have to carry this
    // argument around any longer.
    _isiVolatile.Flags = Flags;

    _isiVolatile.ConnectionTableSize = IsiGetConnectionTableSize();
    memset(&Preset, 0x00, (unsigned)sizeof(Preset));

    initializeData(_isiPersist.BootType);       // get the persitent data, connection table and read_only_data and config_data from the stack

    if (_isiPersist.BootType == isiReboot)
    {
#   ifdef	ISI_SUPPORT_TIMG
        _isiPersist.Devices = ISI_DEFAULT_DEVICECOUNT;
#   endif	//	ISI_SUPPORT_TIMG

        // 17-May-2005, BG: Notice we do NOT reinitialize the serial number when returning the unit to factory defaults.
        // Return to factory defaults is a fairly common operation, at least for those devices using IsiCompact*.lib.
        // It is safe to let the serial run without resetting; resetting would increase the risk of connection leaks.
        //	_isiPersist.Serial = 1;

        // clear address table:
        address_count = _address_table_count();     // getting the value from the stack, instead of from the read_only_data
        for (uIdx=0; uIdx < address_count; ++uIdx)
        {
            update_address((const LonAddress*)&Preset.Address, uIdx);
        }
        watchdog_update();

#   ifdef ISI_SUPPORT_ALIAS
        memset(&Preset.Alias, 0xFFu, (unsigned)sizeof(Preset.Alias));

        // clear alias table
        for (uIdx = 0; uIdx < AliasCount; ++uIdx) 
        {
            IsiSetAlias(&Preset.Alias, uIdx);
        }
        watchdog_update();
#   endif	//	ISI_SUPPORT_ALIAS

        // clear nv table - note we re-use the alias_struct, as this is a superset of nv_struct:
        uSelLow = 0;
        for (uIdx=0; uIdx < NvCount; ++uIdx)
        {
            Preset.Alias.Alias = *IsiGetNv(uIdx);
            // Set SelectorHi
            LON_SET_ATTRIBUTE(Preset.Alias.Alias,LON_NV_ECS_SELHIGH, 0x3Fu);
            Preset.Alias.Alias.SelectorLow = --uSelLow;
            LON_SET_UNSIGNED_WORD(Preset.Alias.Alias.AddressIndex, ISI_NO_ADDRESS);
            IsiSetNv(&Preset.Alias.Alias, uIdx);
        }

        watchdog_update();
    }

    // figure out local channel type
    // standard program Id. Withdraw the channel type from the ID ...OR... unknown channel. Assume PL-20N:
    _isiVolatile.ChannelType = (read_only_data.ProgramId[0] & 0x80u) ? read_only_data.ProgramId[ID_STR_LEN-2u] : isiChannelPl20N;

    // get the channel parameter set
    _IsiSelectTransportProps(_isiVolatile.ChannelType);

    //  Get the secondary domain set as needed.
    //  Note that an application must have two domains enabled.  If they use the "one_domain" pragma, then
    //  the setting of the secondary domain will fail and an "invalid domain index" error will be logged and
    //  ISI won't work.

    //  EPR 38748: If multiple DAS send DIDRM messages that use the same transaction ID, the ISI-DA device could incorrectly stamp and trash them
    //  as duplicates (same message code, same transaction ID, same source address), as all come via Neuron ID addressing on the secondary domain.
    //  The fix is NOT to use a constant S/N pair of 1/1 on the secondary domain, but to randomly choose a value pair S/N = 1..255/1..127. While
    //  we cannot detect or prevent S/N collissions on the secondary domain, this will make the misinterpretation of DIDRM as duplicates pretty
    //  unlikely; 1 in 255*127, actually. Unlike the primary domain, we don't need to make special reservations about channel-specific subnet buckets
    //  or reserved node Id values; the secondary domain is irrelevant when the device is used in a managed network. Note we still must use the
    //  secondary domain in a clone domain configuration, as we cannot guarantee uniqueness of the secondary S/N pair.
    (void)_IsiSetDomain(ISI_SECONDARY_DOMAIN_INDEX, 0, NULL, _IsiRand(254,1), _IsiRand(126,1));

    // ASSUMES: isiReboot, isiReset and isiRestart are ordered in this way.
    if (_isiPersist.BootType < isiRestart)
    {
        memset(&Preset, 0x00, (unsigned)sizeof(Preset));    // EPR 38945 - prior to this fix, we wrote alias table patterns into the connection table. oops.
        // clear connection table:
        for (uIdx=0; uIdx < _isiVolatile.ConnectionTableSize; ++uIdx)
        {
            IsiSetConnection(&Preset.Connection, uIdx);
            watchdog_update();
        }

        // generate a new NUID 0..255:
        _isiPersist.Nuid = _IsiRand(256, 0);

        if (!(gIsiFlags & isiFlagDisableAddrMgmt))
            // Pick up the DHCP if there is any change in the address assignment
            _IsiEnableAddrMgmt();
        // Set the domain tables to their respective defaults.
        _IsiVerifyDomainsS();
    }
    else
    {
        // Check if the Primary domain has changed.
        pDomainCfg = access_domain(ISI_PRIMARY_DOMAIN_INDEX);
     	DomCfgLength = LON_GET_ATTRIBUTE_P(pDomainCfg,LON_DOMAIN_ID_LENGTH) & 0x07u;
        pId = (LonByte *)IsiGetPrimaryDid(&DomainIdLength);
        if ((DomCfgLength != DomainIdLength) ||
             memcmp(&pDomainCfg->Id, pId, sizeof(LonDomainId)))
        {
            // the primay domain has changed Update it.
            _IsiAPIDebug("The primary domain has changed! Old DomainID=%x %x %x %x %x %x, Length=%d, Subnet=%d, Node=%d\n",
                    pDomainCfg->Id[0],pDomainCfg->Id[1],pDomainCfg->Id[2],pDomainCfg->Id[3],
                    pDomainCfg->Id[4],pDomainCfg->Id[5], DomCfgLength, pDomainCfg->Subnet,
                    LON_GET_ATTRIBUTE_P(pDomainCfg, LON_DOMAIN_NODE));
            _IsiVerifyDomainsS();

            _IsiAPIDebug("New DomainID=%x %x %x %x %x %x, Length=%d\n",
            		pId[0],pId[1],pId[2],pId[3],pId[4],pId[5], DomainIdLength);
        }
    }

    // ** For Pilon, the repeat count is specified in IsiStart().  Override the RepeatCount to the one specified in IsiStart()
    // get the repeat count for NV connections:
    // Notice we fetch this value only at re-initialization time: Once at least one connection is established, the repeat
    // count can no longer be changed, as this would require a more intelligent address table allocator (see _IsiIsGroupAcceptable)
    // and address table garbage collector (see _IsiSweepAddressTable). To make sure the repeat count only changes at the
    // correct time, we make the call into the application override rather than having the application call us.
    // Also notice we only support repeat counts 1,2, or 3: less than 1 is bad as it doesn't use the secondary frequency on
    // powerline transceivers. More than 3 is bad because we have no way of increasing the receive timers on the receiving
    // devices. Also note portions of this ISI implementation assume the repeat value never to exceed 3 (grep "Assumes").
    _isiPersist.RepeatCount = max(1, IsiGetRepeatCount() & 0x03);

    // when we start up, we always allocate a new slot.
    // This first slot is randomly chosen at D * Tperiod, where D is the current device count. This is what IsiAllocSlot() provides. We add 5s
    // to be sure we stay away from reset to give us some breathing space following a site-wide power-up.
#ifdef ISI_SUPPORT_TIMG
    _isiVolatile.Wait = ISI_TICKS_PER_SECOND * 5u + _IsiAllocSlot(_isiPersist.Devices);
#else
    _isiVolatile.Wait = ISI_TICKS_PER_SECOND * 5u + _IsiAllocSlot();
#endif  //  ISI_SUPPORT_TIMG

    // .Wait, as calculated just above, can be quite some while: on PL-20 channels, this can be 200 Devices * 10s = 2000s (over half an hour) away from
    // point zero (=now, when executing this very routine). To help with device discovery, we issue a DRUM earlier (the early DRUM) - this goes out within
    // a shorter interval. The interval is calculated that, over the entire interval, the early DRUM consumes no more than 42.8% of the total bandwidth,
    // assumeing a bandwidth of 14 packets per second:
#ifdef  ISI_SUPPORT_TIMG
    _isiVolatile.specialDrum = ((unsigned long)_IsiRand(_isiPersist.Devices / 3u, 5)) * ISI_TICKS_PER_SECOND;
#else
    _isiVolatile.specialDrum = ((unsigned long)_IsiRand(ISI_DEFAULT_DEVICECOUNT / 3u, 5)) * ISI_TICKS_PER_SECOND;
#endif  //  ISI_SUPPORT_TIMG
    // .specialDrum, as calculated just above, might actually end up being longer than .Wait, which indicates the start of the redular broadcasting scheme.
    // Thus, the "early DRUM" might actually be late compared to the first DRUM issued by the b'cast scheduler (which always starts with DRUM anyway). If
    // this is the case, we can stop the specialDrum timer before it actually has started.
    // Note we cannot simply use the specialDrum timer to start the broadcaster at the same time (as I was mistaken to believe at some stage): only .Wait as
    // calculated above will spread the inital slot randomly over the appropriate interval; the .specialDrum timer is -potentially- quicker.
    if (_isiVolatile.specialDrum >= _isiVolatile.Wait) {
        // this is not really an early DRUM, since it would be issued after the broadcaster has started. Thus, we give up on that plan:
        _isiVolatile.specialDrum = 0;
    }

    _isiVolatile.Running = TRUE;
    _isiPersist.BootType = isiRestart;
    _isiVolatile.pendingConnection = ISI_NO_INDEX;

#   ifndef ISI_SUPPORT_AUTOMATIC_CONNECTIONS
    // a library that supports automatic connections will start with slotUsage == slotCsmr, which has a value of zero.
    // RAM variables need not be initialized to zero explicitly in Neuron C, so we only set the one value that is not zero:
    _isiVolatile.Periodic.slotUsage = slotCsmi;
#   endif  //  ISI_SUPPORT_AUTOMATIC_CONNECTIONS
    savePersistentData(IsiNvdSegPersistent);
    savePersistentData(IsiNvdSegConnectionTable);
}

