//	Csmo.c	implementing isiCreateCsmo
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
//	    Revision 0, February 2005, Bernd Gauweiler
//	    Revision 1, June 2005, Bernd Gauweiler: Adding support for Extended fields

#include "isi_int.h"

// on the caller side and after calling isiCreateCsmo, the following code MUST be added before sending a CSMO or CSMR:
// _IsiCreateCid(&pCsmo->Header.Cid);
// pCsmo->Header.Selector = _IsiGetSelectors(pCsmo->Data.Width)
//	See Host.c
extern unsigned get_nv_si_count(void);

void FWD(IsiCreateCsmo,isiCreateCsmo)(unsigned Assembly, IsiCsmoData* const pCsmoData)
{
	unsigned int NvIdx;

	memset(pCsmoData, 0, sizeof(IsiCsmoData));
	pCsmoData->Group = IsiGetPrimaryGroup(Assembly);
	memcpy(pCsmoData->Extended.Application, read_only_data.ProgramId, ID_STR_LEN-2u);
	LON_SET_ATTRIBUTE_P(pCsmoData,ISI_CSMO_DIR,(LonByte)isiDirectionAny);

	NvIdx = IsiGetNvIndex(Assembly, 0, ISI_NO_INDEX);
	if ((NvIdx < NvCount) && (NvCount == get_nv_si_count()))
    {
		pCsmoData->NvType = (unsigned)get_nv_type(NvIdx);
	}
	LON_SET_ATTRIBUTE_P(pCsmoData,ISI_CSMO_WIDTH,IsiGetWidth(Assembly));
}

//	end of Csmo.c

