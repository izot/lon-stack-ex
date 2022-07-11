//////////////////////////////////////////////////////////////////////
//
// Copyright © 2001-2022 Dialog Semiconductor
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
// VxLayerDll.h: defines export/import symbols for this dll
//
//////////////////////////////////////////////////////////////////////

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the VXLAYER_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// VXLAYER_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifndef _VXLAYERDLL_H
#define _VXLAYERDLL_H

    #ifdef WIN32
        #ifdef VXLAYER_EXPORTS
            #define VXLAYER_API		__declspec(dllexport)
        #else
            #ifdef LTA_LIBRARY
                #define VXLAYER_API
            #else
                #define VXLAYER_API		__declspec(dllimport)
            #endif
        #endif
    #else // WIN32
        // VXWORKS ignores this stuff
        #define VXLAYER_API
    #endif // WIN32

#endif // _VXLAYERDLL_H
