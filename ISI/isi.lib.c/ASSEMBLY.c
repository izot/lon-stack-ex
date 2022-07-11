//	Assembly.c	implementing isiGetAssembly
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
//

#include "isi_int.h"

unsigned FWD(IsiGetAssembly,isiGetAssembly)(const IsiCsmoData* pCsmoData, LonBool Auto, unsigned Assembly)
{
    // Combine the isiGetAssembly and isiGetNextAssembly into one by specifying ISI_NO_ASSEMBLY for the previous assembly in the first call.
    if (Assembly == ISI_NO_ASSEMBLY)
    {
	    // Default implementation returns the assembly number of a compatible NV, if the CSMO relates to a simple
	    // connection with a known NV type.
        LonByte Acknowledged = LON_GET_ATTRIBUTE(pCsmoData->Extended,ISI_CSMO_ACK);
        LonByte Poll = LON_GET_ATTRIBUTE(pCsmoData->Extended,ISI_CSMO_POLL);
        LonByte Width = LON_GET_ATTRIBUTE_P(pCsmoData,ISI_CSMO_WIDTH);
        LonByte Scope = LON_GET_ATTRIBUTE(pCsmoData->Extended,ISI_CSMO_SCOPE);

        if (!Auto						    // don't accept automatic connections be default
    	    && !Acknowledged		        // default implementation doesn't do acknowledged unicasts
	        && !Poll				        // default implementation supports, but does not accept, polled connections
	        && Width == 1u					// only accept simple connections by default
	        && !Scope				        // NV type must be in the standard (i.e. SNVT_*)
	        && !pCsmoData->Variant			// Variant must be 0
	        && pCsmoData->NvType)
        {   // NV Type must be distinct
    		// this could be acceptable. See if we have a matching NV:
            return _IsiFindLocalNvOfType(pCsmoData, 0);
	    }
        return ISI_NO_ASSEMBLY;
    }
    else
    {
        return isiGetNextAssembly(pCsmoData, Auto, Assembly);
    }
}

//	end of Assembly.c
