//	SweepAdr.c	implementing _IsiSweepAddressTable
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

void _IsiSweepAddressTable(void)
{
	unsigned AddressIndex, UserIndex, Users;
	const LonNvEcsConfig* pNv;
	LonAddress Adr;
    unsigned int address_count =_address_table_count();

    for (AddressIndex=0; AddressIndex < address_count; ++AddressIndex)
    {
		Adr = *access_address(AddressIndex);
		if (Adr.Broadcast.Type)
        {
			// this address table space is in use. Find out if any NV or alias refers to it:
			Users = 0;
			for (UserIndex=0; UserIndex<NvCount; ++UserIndex)
            {
				pNv = IsiGetNv(UserIndex);
				if ((LON_GET_ATTRIBUTE_P(pNv,LON_NV_ECS_SELHIGH) < 0x30u) && 
                        (LON_GET_UNSIGNED_WORD(pNv->AddressIndex) == AddressIndex))
                {
					// found a user of this address table entry. We're done here.
					++Users;
					break;
				}
			}

#ifdef	ISI_SUPPORT_ALIAS
			if (!Users)
            {
				const LonAliasEcsConfig* pAlias;
				for (UserIndex=0; UserIndex<AliasCount; ++UserIndex)
                {
					pAlias = IsiGetAlias(UserIndex);
					if ((LON_GET_UNSIGNED_WORD(pAlias->Primary) != ISI_ALIAS_UNUSED) && 
                        (LON_GET_UNSIGNED_WORD(pAlias->Alias.AddressIndex) == AddressIndex))  //(pAlias->alias_nv.nv_addr_index == AddressIndex))
                    {
						// in use by this alias.
						++Users;
						break;
					}
				}
			}
#endif //	ISI_SUPPORT_ALIAS

			if (!Users)
            {
				// no users for this address table entry. Free it:
				Adr.Broadcast.Type = 0;
				update_address((const LonAddress*)&Adr, AddressIndex);
			}
		}
	}
}

//	end of SweepAdr.c
