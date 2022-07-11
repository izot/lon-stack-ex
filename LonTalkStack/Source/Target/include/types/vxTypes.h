/* vxTypes.h - POSIX compliant (_t) types header file */

/* Copyright 1991-1992 Wind River Systems, Inc. 
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
$Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Target/include/types/vxTypes.h#1 $
 * 
 * 1     12/05/07 11:19p Mwang
 * 
 * 1     11/04/98 8:42a Darrelld
 * Windows Target header files


01f,13nov92,dnw  added include of vxANSI.h
		 pruned some non-POSIX stuff.
01e,22sep92,rrr  added support for c++
01d,08sep92,smb  made some MIPS specific additions.
01c,07sep92,smb  added __STDC__ conditional and some documentation.
01b,29jul92,smb  added fpos_t type for stdio.h
01a,03jul92,smb  written.
*/

/*
DESCRIPTION
This file actually typedef's the system types defined in vxArch.h or
vxTypesBase.h.  

Architecture specific values for the standard types are defined in vxArch.h
as follows:

	#ifndef _TYPE_sometype_t
	#define _TYPE_sometype_t 	typedef unsigned int sometype_t
	#endif

Defaults for each type are provided in vxTypesBase.h as follows:

	#ifndef _TYPE_sometype_t
	#define _TYPE_sometype_t 	typedef int sometype_t
	#endif

When vxTypesBase.h is included following the include of vxArch.h,
_TYPE_sometype_t will already be defined and so will not be redefined
in vxTypesBase.h.

Alternatively, if it not defined in vxArch.h, it will be defined in
vxTypesBase.h.  So after vxArch.h and vxTypesBase.h are included all ANSI
and POSIX types will be defined but they will not have been typedef'ed yet.

The typedef happens in this file, vxTypes.h

	#ifdef _TYPE_sometype_t
	_TYPE_sometype_t
	#undef _TYPE_sometype_t
	#endif

The '#undef _TYPE_sometype_t' is necessary because a type may be defined in
different header files. For example, ANSI says that size_t must be defined
in time.h and stddef.h

*/

#ifndef __INCvxTypesh
#define __INCvxTypesh

#ifdef __cplusplus
extern "C" {
#endif

#include "types/vxANSI.h"

#if 0 // ECH
#ifndef ptrdiff_t
#ifdef _TYPE_ptrdiff_t
_TYPE_ptrdiff_t;
#undef _TYPE_ptrdiff_t
#endif
#endif

#ifndef size_t
#ifdef _TYPE_size_t
_TYPE_size_t;
#undef _TYPE_size_t
#endif
#endif

#ifdef _TYPE_wchar_t
_TYPE_wchar_t;
#undef _TYPE_wchar_t
#endif

#ifdef _TYPE_ssize_t
_TYPE_ssize_t;
#undef _TYPE_ssize_t
#endif

#ifdef _TYPE_time_t
_TYPE_time_t;
#undef _TYPE_time_t
#endif

#ifdef _TYPE_fpos_t
_TYPE_fpos_t;
#undef _TYPE_fpos_t
#endif

#endif // ECH

#ifdef _TYPE_int8_t
_TYPE_int8_t;
#undef _TYPE_int8_t
#endif

#ifdef _TYPE_uint8_t
_TYPE_uint8_t;
#undef _TYPE_uint8_t
#endif

#ifdef _TYPE_int16_t
_TYPE_int16_t;
#undef _TYPE_int16_t
#endif

#ifdef _TYPE_uint16_t
_TYPE_uint16_t;
#undef _TYPE_uint16_t
#endif

#ifdef _TYPE_int32_t
_TYPE_int32_t;
#undef _TYPE_int32_t
#endif

#ifdef _TYPE_uint32_t
_TYPE_uint32_t;
#undef _TYPE_uint32_t
#endif

/* old Berkeley definitions */

typedef unsigned char	uchar_t;
typedef unsigned short	ushort_t;
typedef unsigned int	uint_t;
typedef unsigned long	ulong_t;

#ifndef linux
typedef	struct	_quad { long val[2]; } quad;
typedef	long	daddr_t;
typedef	char *	caddr_t;
typedef	char *	addr_t;
typedef	long	swblk_t;
#endif

/* POSIX required */
#if 0 // ECH
typedef short		dev_t;
typedef unsigned short	gid_t;
typedef	unsigned long	ino_t;
typedef int		mode_t;
typedef unsigned long	nlink_t;
typedef long		off_t;
typedef int		pid_t;
typedef unsigned short	uid_t;
#endif // ECH

#ifdef __cplusplus
}
#endif

#endif /* __INCvxTypesh */
