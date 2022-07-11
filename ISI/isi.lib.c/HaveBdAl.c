//	HaveBdAl.c	implementing _IsiHaveBoundAlias
//
// Copyright © 200502022 Dialog Semiconductor
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

//	HaveBoundAlias returns true of the NV with the given index has an alias that is bound, and associated with the primary NV. This is used by the
//	routine that takes care of heartbeats; ISI connections may end up in situations where the primary NV is unbound while aliases to that NV are
//	still bound.
//	The function returns ISI_NO_INDEX if there is no such alias, and returns an alias table index greater than or equal to AliasIndex otherwise.

#ifdef	ISI_SUPPORT_ALIAS

unsigned _IsiHaveBoundAlias(unsigned Primary, unsigned AliasIndex)
{
	const LonAliasEcsConfig* pAlias;

	while (AliasIndex < AliasCount)
    {
		pAlias = IsiGetAlias(AliasIndex);
		if ((Primary == LON_GET_UNSIGNED_WORD(pAlias->Primary)) && (LON_GET_ATTRIBUTE(pAlias->Alias,LON_NV_ECS_SELHIGH) <= 0x2Fu))
        {
			// got one! get out of here:
			return AliasIndex;
		}
		++AliasIndex;
	}
	return ISI_NO_INDEX;
}

#endif	//	ISI_SUPPORT_ALIAS

//	end of HaveBdAl.c
