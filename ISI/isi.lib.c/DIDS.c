//	dids.c	implementing _IsiVerifyDomainsS
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
//  Revision 0, January 2005, Bernd Gauweiler

#include "isi_int.h"
#include <stddef.h>


//	the function makes sure both domain table entries are set up correctly in the default domains. This is the
//	standard behavior of an ISI-S device, and the default operation for ISI-DA/-DAS devices. Any custom domain
//	ID and length is accounted for by using the IsiGetPrimary*() functions.

//  July 2005, BG: This function now only looks after the primary domain, but does so for all devices ISI-S/DA/DAS.
//  It is no longer called from IsiStart*(), but from within _IsiInitialize(), and it is no longer being called
//  at each and every reset. However, the secondary domain must be taken care of every time, since LNS is pulling
//  the carpet under our feet otherwise. The code to update the secondary domain has therefore been moved from this
//  routine into _IsiInitialize.
void _IsiVerifyDomainsS(void)
{
	LonByte Length;
	const LonByte* pId;

	pId = IsiGetPrimaryDid(&Length);

#ifdef ISI_SUPPORT_DIAGNOSTICS
	if (_IsiSetDomain(ISI_PRIMARY_DOMAIN_INDEX, Length, pId, _GetIsiSubnet(), _GetIsiNode()))
    {
		_IsiConditionalDiagnostics(isiSubnetNodeAllocation, ISI_PRIMARY_DOMAIN_INDEX);
	}
#else
	(void)_IsiSetDomain(ISI_PRIMARY_DOMAIN_INDEX, Length, pId, _GetIsiSubnet(), _GetIsiNode());
#endif
}

