//	MakeE.c	implementing IsiMakeEnrollment
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



void _IsiMakeEnrollment(LonBool bExtend, unsigned Assembly) {
    const IsiConnection *pConnection;

    pConnection = IsiGetConnection(_isiVolatile.pendingConnection);

    if ((_isiVolatile.State & isiStateInvited) && pConnection->Host != Assembly) {
        // this happens on the guest, and causes acceptance (but not implementation) of the enrollment
        _IsiAcceptEnrollment(bExtend, Assembly);
    } else if ((_isiVolatile.State & isiStatePlannedParty) && pConnection->Host == Assembly) {
		// this happens on the host.
		// Note that, if two local assemblies are to take part, two calls must be made: one for the local member first,
		// then one for the (local) host. Although it is technically possible to combine both in one (just remove the
		// word "else" in the previous source code line), this would result in behavior different from the normal manual
		// ISI connection paradigm.
		_IsiImplementEnrollment(bExtend, Assembly);
#ifdef  ISI_SUPPORT_TURNAROUNDS
        // if turnaround support is enabled, other local assemblies might still be in waiting. Better clear it all out:
        _IsiUpdateUiAndState(isiStateNormal, isiNormal, ISI_NO_ASSEMBLY);
#endif  //  ISI_SUPPORT_TURNAROUNDS
    }
}

//	end of MakeE.c
