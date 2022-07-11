//////////////////////////////////////////////////////////////////////////////
//
// Max.h
//
// Copyright © 2010-2022 Dialog Semiconductor
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
// Echelon MAX Top-Level Include File
//
// This file must be included after other include files that declare
// the platform (Windows, etc.)
//
/////////////////////////////////////////////////////////////////////////////

#ifndef MAX_H
#define MAX_H

// Linux definitions here
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
//#include <ctype.h>
#ifndef WIN32
	#include <sys/syslog.h>
#endif

#include "echstd.h"
#if !(defined(ILON_PLATFORM) || defined(LONTALK_IP852_STACK_PLATFORM) || defined(IZOT_IP852_PLATFORM))
#include "ech_logger.h"
#endif

#include "Osal.h"

// We don't actually have DcxErrors or LDVCode so map to EchErr for the time being
#define DcxError EchErr
#define LDVCode EchErr

// Essentially extensions to string.h
void safe_strncpy(char *target, const char *source, size_t maxLength);
void safe_strncat(char *target, const char *source, size_t maxLength);


// Following are not defined in linux headers, Need to decide how we will resolve them.
C_API_START
#		ifndef _INC_MINMAX
#			define _INC_MINMAX

#			ifndef MAX
#				define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#			endif

#			ifndef MIN
#				define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#			endif
#		endif

		clock_t msec_clock();	// millisecond version of clock()

		const char *strsigname(int signum);
C_API_END

void		OsPrintError(long osError, const char *szContext);

#include "EchCompat.h"

#if defined(WIN32) || defined(TARGET_PLATFORM)
	#define DISCARD			1 ? (void)0 : printf
	#define DISCARD_ARGS	DISCARD
#else
	#define TRACE			printf
	#define DISCARD(...)
	#define DISCARD_ARGS(...)
#endif

#define SZ_DEFAULT_LON 		"/dev/abandplc"	// smp
#define DCMP_NEURON_CHANNEL 0

#endif		// MAX_H
