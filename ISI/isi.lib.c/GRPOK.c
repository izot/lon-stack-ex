//	GrpOk.c	implementing _IsiIsGroupAcceptable
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

//	_IsiIsGroupAcceptable retunrns ISI_NOT_ACCEPTABLE if the given Group cannot be accepted,
//	or returns a suitable address table index otherwise. A Group is acceptable if the device is already member
// 	of that group, or if a free address table entry is available
//	Set the Join parameter to TRUE if you want the function to have the side effect of also updating the address
//	table entry accordingly.

unsigned _IsiIsGroupAcceptable(unsigned Group, LonBool Join)
{
	signed Index;
	signed FirstEmpty;
	address_struct_alt adr;

	FirstEmpty = -1;

	// Scan table from back to front so that we find either a matching group entry or the
	// first empty entry.
    Index = (signed)_address_table_count();     // getting the value from the stack, instead of from the read_only_data
    while (--Index >= 0)
    {
		// OK, this is a bit sleazy, but we use the non-group structure to look
		// for a group match and just take advantage of our knowledge that the
		// group type always has the MSB set.
		adr = *(const address_struct_alt*)access_address(Index);
		if ((adr.typeSize & ADDR_GROUP_MASK) && adr.group == Group)
        {
			// we are already member of this group:
			return Index;
		}
        else if (adr.typeSize == 0)
        {
			FirstEmpty = Index;
		}
	}

	// Now, we have iterated through the entire address table and found we are not a member of this group. Let's see if
	// we could become a member (an address table entry free?)
    if (FirstEmpty != -1)
    {
        // found an unused address table entry.
        if (Join)
        {
            adr.member = 0;				// Sets domain and member to 0
            adr.typeSize = ADDR_GROUP_MASK;	// Set type MSB (group) and size of 0
            adr.group = Group;				// Sets group number
            // Fixed the problem where the RepeatTimer should be the upper 4 bits
			adr.timer1 = _isiVolatile.Transport.RepeatTimer | _isiPersist.RepeatCount;
			adr.timer2 = (_isiVolatile.Transport.GroupRcvTimer<<LON_ADDRESS_GROUP_RECEIVE_TIMER_SHIFT) | _isiVolatile.Transport.TransmitTimer;
			update_address((LonAddress*)&adr, FirstEmpty);
        }
    }
    // ASSUMES: ISI_NOT_ACCEPTABLE == -1
    return FirstEmpty;
}

//	end of GrpOk.c
