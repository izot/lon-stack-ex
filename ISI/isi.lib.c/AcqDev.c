//	AcqDev.c	implementing IsiStartDeviceAcquisition
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
//  Revision 0, February 2005, Bernd Gauweiler
//  Revision 1, July 2005, Bernd: Now we are actually implementing it...

#include "isi_int.h"

#ifdef  ISI_SUPPORT_DADAS
void IsiStartDeviceAcquisition(void)
{
    _IsiAPIDebug("Start IsiStartDeviceAcquisition - Running=%d State=%d\n", _isiVolatile.Running, _isiVolatile.State);
    if (_isiVolatile.Running)
    {
        if (!_isiVolatile.State)
        {
            // Start the device acquisition process.
            goto StartAcquisition;
        }
        else if (_isiVolatile.State & isiStateAwaitConfirm)
        {
            // In this state, this function call is the second in a row and serves as a confirmation that the correct device
            // has winked. That is, we're good to go and can fire the confirmation message, notify the application, and
            // retrigger device acquisition:
            _IsiSendDidrm(isiDidcf);
            _IsiUpdateUi(isiRegistered);
StartAcquisition:
            _IsiUpdateUiAndStateTimeout(ISI_T_ACQ, isiStateAwaitDidrx, isiRegistered, 0);
        }
	}
    _IsiAPIDebug("End IsiStartDeviceAcquisition\n");
}
#endif  //  ISI_SUPPORT_DADAS

//	end of AcqDev.c
