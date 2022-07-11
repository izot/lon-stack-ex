//
// BuildOptions.h
//
// Copyright © 2003-2022 Dialog Semiconductor
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
// This file contains options for builds that are local to a given
// machine.
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Target/include/BuildOptions.h#1 $
//

#ifndef BUILDOPTIONS_H
#	define BUILDOPTIONS_H

#if (_MSC_VER > 1000) || (__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4))
#define SUPPORT_PRAGMA_ONCE
#endif

//
// Turn this on to disable reminders
//
//#define NO_REMINDERS 1

//
// Turn this on to make it really hard to go offline due to multiple reboots (since this can be a hassle
// during debug).
//
//#define PROC_MAX_REBOOT 200


//
// Enable more warnings.  Disable it only temporarily.
//
#define STRICTER_WARNINGS

//
// Use this to run without a shell
//
#define DISABLE_SHELL

// Set this to 1 to automatically get detailed TIPC tracing in MAX without having to use the
// magic command line switch
#ifdef NDEBUG
#define TRACE_AUTO_DETAIL 0
#else
#define TRACE_AUTO_DETAIL 1
#endif

#endif
