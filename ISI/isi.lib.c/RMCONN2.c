//	RmConn2.c	implementing _IsiRemovePtrConnection
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

void _IsiRemovePtrConnection(const IsiConnection* pConnection, unsigned assembly)
{
    // ASSUMES: ConnectionOffsetMask == 0xFC (i.e. (OffsetAuto & ConnectionOffsetMask) == Offset * 4)
    LonByte nvIndex, aliasIndex;
    LonAliasEcsConfig Alias;
    LonByte SelHi;

    unsigned offsetTimesFour = pConnection->Desc.OffsetAuto & ConnectionOffset_MASK;
    LonWord selector = pConnection->Header.Selector;
    signed widthMinusOne = LON_GET_ATTRIBUTE_P(pConnection,ISI_CONN_WIDTH)- 1u;

    // For each selector governed by this connection table entry, scan the entire NV and alias table.
    // Anything using a matching selector will be disconnected.
    // When done, we also need to try to identify address table entries that are no longer used, so that we
    // can free that entry.	Note the caller is in charge of handling the address table.
    // The function returns a non-zero number of address table work is needed, and returns 0 otherwise.

    if (assembly != ISI_NO_ASSEMBLY)
    {
        while(widthMinusOne >= 0)
        {
            nvIndex = IsiGetNvIndex(assembly, offsetTimesFour + widthMinusOne, ISI_NO_INDEX);
            while (nvIndex != ISI_NO_INDEX)
            {
                // notice we use alias_struct Alias here just to reduce the number of local bytes on the stack;
                // alias struct is a superset of nv_struct, so we can use the larger structure instead of both:
                Alias.Alias = *IsiGetNv(nvIndex);
                SelHi = LON_GET_ATTRIBUTE(Alias.Alias,LON_NV_ECS_SELHIGH);
                if (_IsiInSelectorRange(selector, widthMinusOne, make_long(Alias.Alias.SelectorLow, SelHi)))
                {
                    _IsiBind(Alias.Alias, ISI_NO_ADDRESS, make_long(0xFFu-nvIndex, 0x3F), FALSE);
                    IsiSetNv(&Alias.Alias, nvIndex);
                }
#ifdef	ISI_SUPPORT_ALIAS
                // Alias table:
                for (aliasIndex=0; aliasIndex < AliasCount; ++aliasIndex)
                {
                    Alias = *IsiGetAlias(aliasIndex);
                    SelHi = LON_GET_ATTRIBUTE(Alias.Alias,LON_NV_ECS_SELHIGH);
                    if ( (LON_GET_UNSIGNED_WORD(Alias.Primary) == nvIndex)
                        && (_IsiInSelectorRange(selector, widthMinusOne, make_long(Alias.Alias.SelectorLow, SelHi))))
                    {
                        memset(&Alias, 0xFFu, sizeof(Alias));
                        IsiSetAlias(&Alias, aliasIndex);
                    }
                }
#endif	//	ALIAS
                nvIndex = IsiGetNvIndex(assembly, offsetTimesFour + widthMinusOne, nvIndex);
            }
            --widthMinusOne;
        }
        // EPR 36728: application needs a way to tell that it is no longer connected.
        IsiUpdateUserInterface(isiDeleted, assembly);
    }   //  ISI_NO_ASSEMBLY
}
#endif	//	ISI_SUPPORT_CONNECTION_REMOVAL

//	end of RmConn2.c
