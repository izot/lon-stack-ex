//	Subnet.c	implementing _IsiAllocSubnet
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

//	Note that this function is only used by a single caller. A byte ot two can be saved
//	by embedding the function body in the caller directly; however, it is important that
//	this function remains a separate function.
//	When testing aspects of the ISI implementation, testers can override this function by
//	providing their own definition, which might produce a deterministic, or less varied,
//	set of results, allowing to increase probability of duplicates of node IDs, subnet
//	IDs, or selector values.

static unsigned customSubnetId = 0;
static LonBool useCustomSubnetId = FALSE;

// Sets the subnet Id. By default the ISI engine will use the random subnet Id.  
void _IsiSetSubnet(unsigned subnetId)
{
    if (subnetId == 0)
        // Invalid subnet.  Assign to use randomly allocated subnet
        useCustomSubnetId = FALSE;
    else
    {
        useCustomSubnetId = TRUE;
        customSubnetId = subnetId;
    }
}

unsigned _IsiAllocSubnet(void)
{
    return _IsiRand(ISI_SUBNET_BUCKET_SIZE, _isiVolatile.Transport.BaseSubnet);
}

// Returns the subnet to use
// Use the random value if the application does not specified one
unsigned _GetIsiSubnet(void)
{
    if (useCustomSubnetId)
        return customSubnetId;
    return _IsiAllocSubnet();
}

//	end of Subnet.c
