//	AutoEnro.c	implementing IsiInitiateAutoEnrollment
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

#ifdef	ISI_SUPPORT_AUTOMATIC_CONNECTIONS
IsiApiError IsiInitiateAutoEnrollment(const IsiCsmoData* pCsmaData, unsigned Assembly)
{
    IsiApiError sts = IsiEngineNotRunning;

    if (_isiVolatile.Running)
    {
        sts = IsiApiNoError;
	    if (_isiVolatile.Startup >= ISI_T_AUTO
	        && !IsiIsAutomaticallyEnrolled(Assembly))
        {
            if (_IsiBecomeHost(Assembly, TRUE, pCsmaData))
            {
			    _isiVolatile.State = isiStatePlannedParty;
			    IsiCreateEnrollment(Assembly);
		    }
            else
                sts = IsiNoConnectionSpace;
    	}
    }
    return sts;
}
#endif	//	ISI_SUPPORT_AUTOMATIC_CONNECTIONS

//	end of AutoEnro.c
