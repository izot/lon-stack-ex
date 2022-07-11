//	did.c	implementing IsiGetPrimaryDid
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
//  Revision 0, January 2005, Bernd Gauweiler

#include "isi_int.h"
#include <stdio.h>

static LonByte DomainID[LON_DOMAIN_ID_MAX_LENGTH] = ISI_DEFAULT_DOMAIN_ID;
static unsigned DIDLen = ISI_DEFAULT_DOMAIN_ID_LEN;

const LonByte* IsiGetPrimaryDid(LonByte* pLength)
{
    *pLength = DIDLen;
    return DomainID;
}

void IsiSetPrimaryDid(const LonByte* pDid, unsigned len)
{
    if (len <= LON_DOMAIN_ID_MAX_LENGTH)
    {
        memset(DomainID, 0, sizeof(DomainID));
        DIDLen = len;
        memcpy(DomainID, pDid, len);
    }
}

// Set the Primary Did based on the local IP address
// The first two bytes of the IP addess represnt the LONTalk services domain ID
// the third byte represents the LONTalk services subnet and the fourth byte 
// represents the LONTalk services node ID
// If the address is not derivabled, ISI will assign a randomly allocated
// subnet and node ID, and set the flag to indicate that that the address is not derivabled
LonBool _IsiEnableAddrMgmt(void)
{
#if defined(IZOT_PLATFORM)
    LonByte localDomainID[LON_DOMAIN_ID_MAX_LENGTH];
    unsigned int localDidLen;
    unsigned int subnetId, nodeId;
#endif

    _IsiSetSubnet(0);
    _IsiSetNode(0);
    gIsiDerivableAddr = 0;

#if defined(IZOT_PLATFORM)
    // Gets the domain, subnet and node Id from the local IP address
    if (LonGetDidFromLocalAddress(localDomainID, &localDidLen, &subnetId, &nodeId) == (LonApiError)IsiApiNoError)
    {
        char s[256];
        int i;
        char *p = s;

        IsiSetPrimaryDid(localDomainID, localDidLen);

        // Check to make sure if the address is derivabled 
        if ((subnetId >= 1 && subnetId <= 255) && 
            (nodeId >= 1 && nodeId <= 127))
        {
            _IsiSetSubnet(subnetId);
            _IsiSetNode(nodeId);
            gIsiDerivableAddr = 1;
        }
        memset(&s, 0, sizeof(s));
        for (i=0; i< (int)localDidLen; ++i)
        {
            sprintf(p, "%2x ", localDomainID[i]);
            p += 3;
        }
        _IsiAPIDebug("IP Address is %s DidLength=%d Did=%s\n",
            gIsiDerivableAddr ? "derivabled" : "not derivabled", localDidLen, s);
        if (gIsiDerivableAddr)
            _IsiAPIDebug("Subnet=%d Node=%d\n", subnetId, nodeId); 
        return TRUE;
    }
#endif
    return FALSE;
}

// Get the DomainId and length from the IP address
// The first two bytes of the IP addess represnt the LONTalk services domain ID
LonBool _GetDomainIdFromAddr(LonByte* pDom, unsigned int* didLen)
{
#if defined(IZOT_PLATFORM)
    unsigned int subnetId, nodeId;

    if (LonGetDidFromLocalAddress(pDom, didLen, &subnetId, &nodeId) == (LonApiError)IsiApiNoError)
        return TRUE;
#endif
    return FALSE;
}
