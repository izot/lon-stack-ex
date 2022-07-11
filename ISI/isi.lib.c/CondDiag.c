//	CondDiag.c	implementing _IsiConditionalDiagnostics
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

//	Note that calls to IsiUpdateDiagnostics are not only made through this
//	conditional forwarder (in order to control overhead uplink traffic for
//	shortstack, or general overhead burden in all cases), but that this is
//	also a feature controlled via the ISI_SUPPORT_DIAGNOSTICS macro, defined in
//	isi_int.h
//	This allows to create a library version with smaller footprint (and without
//	support for IsiUpdateDiagnostics), and a (slightly) larger one that
//	supports this callback.
//	Whether we're going to use this remains to be seen (subject to resulting ISI
//	footprint), but at least it's in place.

void _IsiConditionalDiagnostics(IsiDiagnostic Event, unsigned Parameter) {
	if (_isiVolatile.Flags & isiFlagSupplyDiagnostics) {
		IsiUpdateDiagnostics(Event, Parameter);
	}
}

//	end of CondDiag.c
