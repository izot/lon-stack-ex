//	CancelA.c	implementing IsiCancelAcquisition
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
//  Revision 0, July 2005, Bernd Gauweiler

#include "isi_int.h"

// IsiCancelAcquisition() is now available both on ISI-DA and ISI-DAS devices and does just what the name suggests.
// The cancellation applies to both device and domain acquisition. A UI event isiNormal will be fired to assist the
// application with the UI maintenance.

#ifdef  ISI_SUPPORT_DADAS
void IsiCancelAcquisition(void) {
    if (_isiVolatile.Running && (_isiVolatile.State & (isiStateAwaitDidrx | isiStateAwaitConfirm | isiStateCollect | isiStateAwaitQdr))) {
        isiDasExtState = isiDasNormal;
        _IsiUpdateUiAndStateTimeout(0, isiStateNormal, isiNormal, ISI_NO_ASSEMBLY);
    }
}
#endif  //  ISI_SUPPORT_DADAS
        //
//	end of CancelA.c
