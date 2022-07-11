//	Csmi.c	implementing _IsiCreateCsmi
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

void _IsiCreateCsmi(const IsiConnection* pConnection, IsiCsmi* pCsmi)
{
    LonByte Offset = LON_GET_ATTRIBUTE(pConnection->Desc.Bf,ConnectionOffset);
    LonByte Width = LON_GET_ATTRIBUTE_P(pConnection,ISI_CONN_WIDTH);
	memcpy(&pCsmi->Header, &pConnection->Header, (unsigned)sizeof(pConnection->Header));
	//pCsmi->Desc.OffsetCount = (pConnection->Desc.Bf.Offset<<CsmiOffsetShift) | ((pConnection->Width - 1u)<<CsmiCountShift);
    pCsmi->Desc.OffsetCount = (Offset<<CsmiOffset_SHIFT) | ((Width - 1u)<<CsmiCount_SHIFT);
}

//	end of Csmi.c
