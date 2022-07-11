//	FndLocNv.c	implementing _IsiFindLocalNvOfType
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
#if 0
#include <modnvlen.h>
#endif

//	see extensive comment, below, about this function. Defined in l_nv4.ns
extern      // system far 
unsigned get_nv_si_count(void);

//	This function is used by the default implementation of isiGetAssembly alone, and might become obsolete as soon as the application
//	overrides IsiGetAssembly (unless of course it continues calling the isiGetAssembly forwardee).
//	In the default implementation, assemblies are always assumed to have a wdith of one, and the assembly number conventiently equals
//	the NV index.
//	This function here finds a NV that matches the CSMO given, starting with (PreviousAssembly+1). First-time callers should set the
//	PreviousAssembly parameter to ISI_NO_ASSEMBLY

unsigned _IsiFindLocalNvOfType(const IsiCsmoData* pCsmoData, unsigned Assembly)
{
	IsiDirection Direction;
	const LonNvEcsConfig* pNv;

	// the get_nv_type function expects the "SI index", not the NV's "global index". This is a problem, because the SI index is not available
	// anywhere (expect when parsing the entire SI data). With normal, simple, NVs, SI index and global index are the same. With NV arrays,
	// however, these are only the same when declared with the bind_info(expand_array_info) modifier (and when linked with some very old
	// system image).
	// For ISI enabled devices, it is therefore recommended to use the bind_info(expand_array_info) modifier with the declaration of NV arrays,
	// or not to declare NV arrays, or to override the IsiGetAssembly() function.
	//
	// This function, _IsiFindLocalNvOfType, is used internally by the default implementations of IsiGetAssembly.
	// This function cannot solve the problem of compacted NV array SI data, but it can detect it and avoid misinterpretation of random data
	// therefore:
	// NvCount is the number of actual network variables, with each array element counting as one.
	// The get_nv_si_count() function returns the number of network variable entries in SI
	// If the two numbers are the same, then get_nv_type() can be used with the NV index as the argument. In this case, the _IsiFindLocalNvOfType()
	// function does what it is supposed to do. When in doubt (the two NV counts won't match), the function says ISI_NO_ASSEMBLY.
	// Thus, if arrays are used without bind_info(expand_array_info), the function will at least not misinterpret arbitrary data.
#define ISI_CSMO_DIR_MASK		0xC0
#define ISI_CSMO_DIR_SHIFT		6
#define ISI_CSMO_DIR_FIELD		Attributes1

	Direction = LON_GET_ATTRIBUTE_P(pCsmoData,ISI_CSMO_DIR);     //     ->Direction;

    if (NvCount == get_nv_si_count())
    {
        while(Assembly < NvCount)
        {
            if (pCsmoData->NvType == get_nv_type(Assembly))
            {
				pNv = IsiGetNv(Assembly);
                // the following returns any assembly, if the enrollment is not direction-specific:
				if (Direction == isiDirectionAny) return Assembly;
                // the following returns the assembly if the enrollment is direction-specific with either isiDirectionInput or isiDirectionOutput,
                // and if the local NV has the opposite direction. Note EPR 37820, which explains that IsiDirection is unfortunately defined so that
                // isiDirectionOutput has a value of 0 (zero), wheras the direction flag in the NV config table is cleared (zero) for input NVs.
                // Admittedly this is confusing, but by the time this was detected we didn't feel like we could change this any more, as the product
                // was well into regression test already, and re-defining the enumeration would have broken example applications that had already been
                // programmed into the evaluation boards. So, we live with this discrepancy. Note the test for equality of the direction information
                // in the following line actually approves complimentary network variables, therefore.
				if ((Direction != isiDirectionVarious) && 
                        (Direction == LON_GET_ATTRIBUTE_P(pNv,LON_NV_ECS_DIRECTION)))
                    return Assembly;
			}
			++Assembly;
		}
	}
	return ISI_NO_ASSEMBLY;
}

//	end of FndLocNv.c
