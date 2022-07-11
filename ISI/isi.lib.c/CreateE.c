//	CreateE.c	implementing IsiCreateEnrollment
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

#include "isi_int.h"

const IsiApiError IsiCreateEnrollment(unsigned Assembly)
{
    IsiApiError sts = IsiEngineNotRunning;
    _IsiAPIDebug("Start IsiCreateEnrollment - Assembly=%d\n", Assembly);

    if (_isiVolatile.Running)
    {
	    _IsiMakeEnrollment(FALSE, Assembly);
        sts = IsiApiNoError;
    }
    _IsiAPIDebug("End IsiCreateEnrollment=%d\n", sts);

    return sts;
}

//	end of CreateE.c
