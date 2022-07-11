//	ConnTab.c	implementing Connection Table stuff
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

// eeprom fastaccess 
IsiConnection * _isiConnectionTable = NULL;     // malloc(ISI_DEFAULT_CONTAB_SIZE * sizeof(IsiConnection));
unsigned _connectionsTableSize = 0; // ISI_DEFAULT_CONTAB_SIZE;

unsigned IsiGetConnectionTableSize(void)
{
	return _connectionsTableSize;
}

void _IsiFreeConnectionTableSize()
{
    if (_isiConnectionTable != NULL)
        free(_isiConnectionTable);
    _isiConnectionTable = NULL;
}

void _IsiSetConnectionTableSize(unsigned sz)
{
    // limits max number of connections to 256; we will document the workaround to exceed 256
    if (sz > ISI_MAX_CONNECTION_COUNT)
        sz = ISI_MAX_CONNECTION_COUNT;
    if (sz != _connectionsTableSize)
    {
        _IsiFreeConnectionTableSize();
        _isiConnectionTable = malloc(sz * sizeof(IsiConnection));
        _connectionsTableSize = sz;
       _IsiInitConnectionTable();
    }
}

void _IsiInitConnectionTable()
{
    memset(_isiConnectionTable, 0, IsiGetConnectionTableSize() * sizeof(IsiConnection));    
}

const IsiConnection* IsiGetConnection(unsigned Index) 
{
	return _isiConnectionTable + Index;
}

void IsiSetConnection(const IsiConnection* pConnection, unsigned Index)
{
#if 0
#pragma relaxed_casting_on
#pragma warnings_off
#endif
    memcpy((IsiConnection*)IsiGetConnection(Index), pConnection, (unsigned)sizeof(IsiConnection));
#if 0
#pragma relaxed_casting_off
#pragma warnings_on
#endif
}

void DumpConnectionTable()
{
    unsigned i;
    IsiUniqueCid uCid;

    _IsiAPIDebug ("DumpConnectionTable\n");
    for (i = 0; i < IsiGetConnectionTableSize(); ++i)
    {
        const IsiConnection* pCon = (IsiConnection *)IsiGetConnection(i);
        _IsiAPIDebug("ConnTable Index = %d\n", i);
        memcpy(&uCid, &pCon->Header.Cid, sizeof(pCon->Header.Cid));
        _IsiAPIDebug("\tFmt1:Serial=%d ConnID=%02x %02x %02x %02x %02x Selector=%d\n", 
            LON_GET_UNSIGNED_WORD(pCon->Header.Cid.SerialNumber), 
            pCon->Header.Cid.UniqueId[0], pCon->Header.Cid.UniqueId[1], pCon->Header.Cid.UniqueId[2],
            pCon->Header.Cid.UniqueId[3], pCon->Header.Cid.UniqueId[4],
            LON_GET_UNSIGNED_WORD(pCon->Header.Selector));
        _IsiAPIDebug("\tFmt2:Serial=%d ConnID=%02x %02x %02x %02x %02x %02x Selector=%d\n", 
            uCid.rev2Cid.SerialNumber,  
            uCid.rev2Cid.UniqueId[0], uCid.rev2Cid.UniqueId[1], uCid.rev2Cid.UniqueId[2],
            uCid.rev2Cid.UniqueId[3], uCid.rev2Cid.UniqueId[4], uCid.rev2Cid.UniqueId[5],
            LON_GET_UNSIGNED_WORD(pCon->Header.Selector));
        _IsiAPIDebug("\tHost=%d Member=%d State=%d Extend=%d CSME=%d Width=%d\n", 
            pCon->Host, pCon->Member, LON_GET_ATTRIBUTE_P(pCon,ISI_CONN_STATE),
            LON_GET_ATTRIBUTE_P(pCon,ISI_CONN_EXTEND), LON_GET_ATTRIBUTE_P(pCon,ISI_CONN_CSME),
            LON_GET_ATTRIBUTE_P(pCon,ISI_CONN_WIDTH));
        _IsiAPIDebug("\tConnOffset=%d ConnAuto=%d\n",
            LON_GET_ATTRIBUTE(pCon->Desc.Bf, ConnectionOffset),
            LON_GET_ATTRIBUTE(pCon->Desc.Bf, ConnectionAuto));
    }
}


//	end of ConnTab.c
