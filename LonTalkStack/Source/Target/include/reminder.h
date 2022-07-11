//
// reminder.h
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
#if 0
  // These are not comments so they don't show up in the Tasks list in Eclipse.
   This file contains definition for the REMINDER and NOTE macros.
#endif
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Target/include/reminder.h#1 $

#ifdef SUPPORT_PRAGMA_ONCE
#pragma once
#endif

#ifndef __reminder_h
#define __reminder_h


// Compile-time reminder macro.
// Formed like other VC compiler messages -- double click to 
// jump to the source location.
#if 0
   // These are not comments so they don't show up in the Tasks list in Eclipse.
   Usage: REMINDER("Some message")
  		use it to tag an issue to be addressed later
   Usage: NOTE("Some message")
  		use it to highlight some characteristic about the build
#endif
#define _QUOTE(x) # x
#define QUOTE(x) _QUOTE(x)
#define __FILE__LINE__ __FILE__ "(" QUOTE(__LINE__) ") : "
#define _REMINDER_ __FILE__LINE__ "Reminder: "
#define _NOTE_ __FILE__LINE__ "Note: "

#ifdef NO_REMINDERS
#define REMINDER(text)
#define NOTE(text)
#else
#define REMINDER(text) message (_REMINDER_ #text)
#define NOTE(text) message (_NOTE_ #text)
#endif

// this is useful for tracing the path of execution in the target;
// it is enabled only when "pcons" is enabled.
#define CHECKPOINT ConPrintf("Debug checkpoint hit at %s\n", __FILE__LINE__)

#endif		// __reminder_h
