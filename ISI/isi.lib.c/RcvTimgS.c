//	RcvTimgS.c	implementing _IsiReceiveTimgS
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
//

#include "isi_int.h"

//	_IsiReceiveTimgS handles receipt of TIMG messages (ISI-S and ISI-DA only)

#ifdef	ISI_SUPPORT_TIMG
//  The pointer version of the TIMG receiver was introduced with phase 2, July 2005,
//  since there are now two cases where an incoming TIMG needs processing: one that
//  covers the arrival of an actual TIMG message (via _IsiReceiveTimgS(), above),
//  and one that contains the arrival of a faked TIMG. The faked TIMG is made up on-
//  the-fly when the ISI-DA domain acquisition process completes with the arrival of
//  a matching DIDCF: this DIDCF message has a domain part (which leads to recommissioning
//  of the primary domain, and a timing guidance part (which leads here).
//
// 15-Sept-2005, BG: prototype change again with phase 3: eliminate the pointer argument (NCC generates
// pretty bad code for those, as it only uses the pointer registers for scratch and fails to allocate
// pointer arguments to a pointer register), and replace the pointer argument with the two discrete arguments
// that matter. Much easier, resuling in more compact call code, and more compact code for this function.
void _IsiReceiveTimg(unsigned DeviceEstimate, unsigned ChannelType) {

    _IsiAPIDebug(
    		"_IsiReceiveTimg device est=%u, channel %u\n",
    		DeviceEstimate,
    		ChannelType
    );

    if (_isiPersist.Devices != DeviceEstimate)
    {
        // a new, different, estimate has arrived. Update the local figures, and throw the dice again (re-allocate the broadcast slot)
        _IsiSetDasDeviceCountEst(DeviceEstimate);
        _isiVolatile.Wait = _IsiAllocSlot(DeviceEstimate);
    }
#ifdef ISI_SUPPORT_DIAGNOSTICS
    // fire the diagnostics event now (as opposed to firing at the end of the function), if ever: firing now constitutes the last use of
    // the DeviceEstimate argument, prior to the first use of the ChannelType argument. Not having to juggle both arguments on the stack
    // allows for more compact code (whether NCC takes advantage of that is a different matter).
    _IsiConditionalDiagnostics(isiReceiveTimg, DeviceEstimate);
#endif	//	ISI_SUPPORT_DIAGNOSTICS

    // update transport properties:
    _IsiSelectTransportProps(ChannelType);
}
#endif	//	ISI_SUPPORT_TIMG

//	end of RcvTimgS.c
