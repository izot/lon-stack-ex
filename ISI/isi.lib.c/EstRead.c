//	EstRead.c	implementing _IsiGetDasDeviceCountEst
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

// undocumented. Used for unit- and system testing. This allows the application to read back the current device count
// extimation. The function works for any device, DAS or not. On a DAS, it always returnes the current estimate (and
// never the override). On any other device, it returns the current estimate (there is no override).
LonByte _IsiGetDasDeviceCountEst(void) 
{
#ifdef ISI_SUPPORT_TIMG
	return _isiPersist.Devices;
#else
	return ISI_DEFAULT_DEVICECOUNT;
#endif	//	ISI_SUPPORT_TIMG
}

void _IsiSetDasDeviceCountEst(LonByte number)
{
#ifdef ISI_SUPPORT_TIMG
	_isiPersist.Devices = number;
#else
	_isiPersist.Devices = ISI_DEFAULT_DEVICECOUNT;
#endif
    if (IsiIsRunning())
        savePersistentData(IsiNvdSegPersistent);
}
