//	Period.c	implementing _IsiGetPeriod
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
//	Revision 1, 8-June-2005, Bernd: EPR 37262, need jitter on Tperiod

#include "isi_int.h"

//	Tjitter:
//	ISI message spreading detects distinguishable (broadcast) packets that are less than 1 second apart, and aims at a better
//	distribution by spreading. Spreading means that the second device to bcast a message within that short distance of some other
//	device re-allocates its own bcast slot, therefore hopefully getting the transmissions further apart.
//	If, however, packets are too close to each other, they will corrupt each other and spreading can't catapult them apart. Further,
//	none of the nodes would be aware of the problem. Tjitter addresses this. Tjitter is a small random variation on the time at which
//	a device broadcasts; even if at some point packets actually get send simultaneously (where "simultaneous" really means within
//	94ms on a secondary frequency power line transmission, as the worst case), jittering will set those "simultaneous" packets
//	sufficiently apart so that they no longer wipe themselves out, and so that spreading can kick in and set the packets apart.
//	Tjitter must therefore set packets apart at least by 94ms, and less than 1 second. We therefore implement the jitter as a
//	variation -250ms..+250ms in steps of 250ms.

#ifdef	ISI_SUPPORT_TIMG
#	define	CURRENT_DEVICECOUNT	((unsigned long)Devices)
	unsigned long _IsiGetPeriod(unsigned Devices) {
#else
#	define	CURRENT_DEVICECOUNT ((unsigned long)ISI_DEFAULT_DEVICECOUNT)
	unsigned long _IsiGetPeriod(void) {
#endif

	//      device count * 20s * 4 ticks per second	- 1 + [0..2]
	//		The correction by "-1+[0..2]" equals a correction [-1..+1], resulting in the desired jitter of -250..+250ms
	//		Assumes: ISI_TICKS_PER_SECOND == 4
	return CURRENT_DEVICECOUNT * (unsigned long)_isiVolatile.Transport.TicksPerSlot - 1ul + (unsigned long)(getRandom() % 3u);
}

//	end of Period.c
