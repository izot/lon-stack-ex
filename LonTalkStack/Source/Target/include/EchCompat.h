/////////////////////////////////////////////////////////////////////////////
//
// Standard C Function Compatibility Map
//
// Copyright © 2006-2022 Dialog Semiconductor
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
/////////////////////////////////////////////////////////////////////////////
#ifdef SUPPORT_PRAGMA_ONCE
#pragma once
#endif

#ifndef __EchCompat_h
#define __EchCompat_h

#include "EchelonStandardDefinitions.h"

#ifdef _MSC_VER
	// Microsoft renamed all runtime functions to have a "-" prefix,
	// reportedly for ISO C++ conformance
	#define	strnicmp		_strnicmp	
	#define	chmod			_chmod	
	#define	fileno			_fileno	
//	#define	close			_close	
	#define	read			_read	
	#define	write			_write
	#define	lseek			_lseek
	#define	strdup			_strdup
	#define	stricmp			_stricmp
	#define	snprintf		_snprintf
	#define vsnprintf		_vsnprintf

#elif defined(__VXWORKS__)
	#define stricmp			_stricmp
	#define strnicmp		_strnicmp

#else	// GCC (is there a symbol?) has its own names for case-sensitive strcmp
	#define stricmp			strcasecmp
	#define strnicmp		strncasecmp
#endif


#endif		// __EchCompat_h
