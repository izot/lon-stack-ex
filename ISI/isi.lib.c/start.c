//	Start.c	implementing IsiStart
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
#include <stdio.h>


const IsiApiError IsiStart(unsigned size, IsiType type, IsiFlags flags, unsigned connections, unsigned didLength, 
                           const LonByte* pDid, unsigned repeatCount) 
{
    IsiApiError sts = IsiApiNoError;
    char s[256];
    int i;
    LonBool useIsiAddrMgmt = FALSE;  
    LonByte *pDom;
    char *p = s;
    LonByte tempDomainID[LON_DOMAIN_ID_MAX_LENGTH];
    unsigned int tempDidLen;
    
    memset(&s, 0, sizeof(s));

    _IsiAPIDebug("Start IsiStart\n");
    _IsiSetCurrentType(type);        // current IS type
    gIsiFlags = flags;
    _IsiSetConnectionTableSize(connections);
    IsiSetRepeatCount(repeatCount);

     _IsiAPIDebug("Size=%d Type=%d Flag=%d Connections=%d, RepeatCount=%d\n",
            size, type, flags, connections, repeatCount); 

    if (!(flags & isiFlagDisableAddrMgmt))
    {
        // Assign LS domain ID, subnet and node from the derived IP address.  
        // If the LS address is not derivable the engine will assign a randomly
        // alloacted primary addresss 
        if (_IsiEnableAddrMgmt())
            useIsiAddrMgmt = TRUE;
    }

    if (!useIsiAddrMgmt)
    {
        pDom = (LonByte *)pDid;  // point to the domain ID specified by the caller
        // Check if the domain ID specified is valid
        if (didLength == 3 && (*pDid == 0x00 || *pDid == 0x0A))
        {
            // Not a valid domain.  Try to use domain ID from the IP address
            if (_GetDomainIdFromAddr(tempDomainID, &tempDidLen))
            {
                _IsiAPIDebug("Invalid Domain ID specified.  Use Domain ID from the IP address Length=%d (%2x %2x %2x)\n",
                    tempDidLen, tempDomainID[0], tempDomainID[1], tempDomainID[2]);
                didLength = tempDidLen;  
                pDom = tempDomainID;  // point to the new domain ID 
            }
            else
                sts = IsiInvalidDomain;
        }

        for (i=0; i< (int)didLength; ++i)
        {
            sprintf(p, "%2x ", *(pDom+i));
            p += 3;
        }
        _IsiAPIDebug("DidLength=%d Did=%s\n", didLength, s);
        IsiSetPrimaryDid(pDom, didLength);
    }

    _isiPersist.BootType = isiRestart;

    // Register the message(LonMsgArrived) and response(LonResponseArrived) filtering callbacks
    LonFilterMsgArrivedRegistrar(isiFilterMsgArrived);
    LonFilterResponseArrivedRegistrar(isiFilterResponseArrived);

#ifdef	ISI_SUPPORT_DADAS
	if (type == isiTypeDas)
    {
		IsiStartDas(flags);
	} 
    else if (type == isiTypeDa) 
    {
		IsiStartDa(flags);
	} else
    {
		IsiStartS(flags);
	}
#else
	if (type == isiTypeS)
    {
		IsiStartS(flags);
	}
#endif	//	ISI_SUPPORT_DADAS

    return sts;
}

//	end of Start.c
