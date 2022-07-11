/* sntpcLib.h - Simple Network Time Protocol client include file */

/* 
 * Copyright 1984-1997 Wind River Systems, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in 
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
Modification history 
--------------------
01b,15jul97,spm  code cleanup, documentation, and integration; entered in
                 source code control
01a,20apr97,kyc  written

*/

#ifndef __INCsntpclib2h
#define __INCsntpclib2h

#ifdef WIN32

#include "sntpcLib.h"

#else  /* WIN32 */

#include 
#ifdef __cplusplus
extern "C"
{
#endif

/* A version of timespec that guarantees that the seconds value is signed */
/* so we can compute differences in timespecs */
struct timespecDiff
    {
    					/* interval = +/- (tv_sec*10**9 + tv_nsec) */
    long tv_sec;			/* seconds */
    long tv_nsec;			/* nanoseconds (0 - 1,000,000,000) */
    };
typedef struct timespecDiff timespecDiff;

/* includes */

#include "sntp.h"
#include "sntpcLib.h"

void getCurTimespec(struct timespec *t);

/* Debug */
#ifdef DEBUG
/* This is used to make normally static routines global */
#undef LOCAL
#define LOCAL
#endif

/* expose some internals that only exist if compiled for debug */
extern timespecDiff TDiff1, TDiff2, TSum1; 
extern struct timespec T1, T2, T3, T4;


#ifdef __cplusplus
}
#endif

#endif /* WIN32 */

#endif /* __INCsntpclib2h */

