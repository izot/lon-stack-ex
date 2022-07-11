/* semSmLib.h - shared semaphore library header file */

/* 
 * Copyright 1984-1992 Wind River Systems, Inc. 
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
modification history
--------------------
01d,29sep92,pme  changed semSm[BC]Create to sem[BC]SmCreate.
01c,22sep92,rrr  added support for c++
01b,28jul92,pme  added #include "semLib.h".
01a,19jul92,pme  written.
*/

#ifndef __INCsemSmLibh
#define __INCsemSmLibh

#ifdef __cplusplus
extern "C" {
#endif

#include "vxWorks.h"
#include "semLib.h"
#include "smDllLib.h"


typedef struct sm_semaphore * SM_SEM_ID;

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

extern    SEM_ID       semBSmCreate (int options, SEM_B_STATE initialState);
extern    SEM_ID       semCSmCreate (int options, int initialCount);
extern    void         semSmShowInit ();

#else   /* __STDC__ */

extern    SEM_ID       semBSmCreate ();
extern    SEM_ID       semCSmCreate ();
extern    void         semSmShowInit ();

#endif	/* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif /* __INCsemSmLibh */
