/* vxANSI.h - ANSI header file */

/* 
 * Copyright 1992 Wind River Systems, Inc. 
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
$Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Target/include/types/vxANSI.h#1 $
$Log: /Target/include/types/vxANSI.h $
 * 
 * 1     11/04/98 8:42a Darrelld
 * Windows Target header files


01d,13nov92,dnw  added define for _EXTENSION_POSIX_1003
01c,22sep92,rrr  added support for c++
01b,07sep92,smb  added documentation.
01a,03jul92,smb  written.
*/

/*
DESCRIPTION
This header file includes the minimum configuration necessary for
an ANSI compatible file.

vxCpu.h       contains CPU_FAMILY definition
vxArch.h      contains architecture specific definitions
vxParams.h    contains system parameters specified by ANSI or POSIX standards
vxTypesBase.h contains type definitions for ALL types specified by ANSI or
	      POSIX not already defined in vxArch.h
*/

#ifndef __INCvxANSIh
#define __INCvxANSIh

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The following defines enable various extensions to strict ANSI definitions.
 * To achieve strict ANSI compliance without extensions, define these
 * to be 0.
 */

#define _EXTENSION_POSIX_1003		1    /* include POSIX extensions */
#define _EXTENSION_POSIX_REENTRANT	1    /* include POSIX reentrant ext. */
#define _EXTENSION_WRS			1    /* include WRS extensions */

#if 0 // ECH
#include "types/vxCpu.h"		/* CPU family definition */
#include "types/vxArch.h"		/* architecture definitions */
#include "types/vxParams.h"		/* system defaults */
#include "types/vxTypesBase.h"	 	/* defaults for machine dependant */
#endif // ECH
#ifdef __cplusplus
}
#endif

#endif /* __INCvxANSIh */
