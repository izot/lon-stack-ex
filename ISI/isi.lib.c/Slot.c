//	Slot.c	implementing _IsiAllocSlot
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

//	 _IsiAllocSlot	re-throws the dice: it randomly picks a slot (a number between 1 and the total number of known ISI devices on the media),
//	and computes the number of ticks to wait until then
#ifdef ISI_SUPPORT_TIMG
unsigned long _IsiAllocSlot(unsigned Devices) {
	return _IsiRand(Devices, 0) * (unsigned long)_isiVolatile.Transport.TicksPerSlot;
}
#else
unsigned long _IsiAllocSlot(void) {
	return _IsiRand(ISI_DEFAULT_DEVICECOUNT, 0) * (unsigned long)_isiVolatile.Transport.TicksPerSlot;
}
#endif	//	ISI_SUPPORT_TIMG
//	end of Slot.c
