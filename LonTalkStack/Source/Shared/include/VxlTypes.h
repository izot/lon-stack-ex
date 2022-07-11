#ifndef _VXLTYPES_H
#define _VXLTYPES_H
/***************************************************************
 *  Filename: VxlTypes.h
 *
 * Copyright © 2022 Dialog Semiconductor
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
 *
 *  Description:  Definitions for use with VxWorks modules.
 *
 *	DJ Duffy Oct 1998
 *
 ****************************************************************/

/*
 * $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Shared/include/VxlTypes.h#2 $
 *
 *
 * $Log: /VNIstack/FtXl_Dev/FtXlStack/Shared/include/VxlTypes.h $
 * 
 * 24    3/22/07 2:05p Bobw
 * 
 * 23    5/18/06 1:03p Fremont
 * Porting to vxworks 6.2. Make HANDLE Win32 only
 * 
 * 22    6/13/05 11:21a Garyb
 * Added conditional inclusions for SUCCEEDED and FAILED macros to
 * eliminate redefinition warnings.
 * 
 * 18    3/06/03 6:01p Vprashant
 * EPRS FIXED: 
 * 
 * 17    1/07/03 11:14p Vprashant
 * added definition for LPTSTR
 * 
 * 16    10/24/02 11:43a Hho
 * changes for VS.NET compilation.
 * 
 * 15    5/12/02 3:58p Vprashant
 * added more types
 * 
 * 14    3/22/02 4:23p Vprashant
 * added upLink related files
 * 
 * 13    2/25/02 11:50a Vprashant
 * removed definition of __int64 after it was added into the xDriver code.
 * 
 * 12    2/12/02 9:53a Fremont
 * fix comment typo to allow brace matching
 * 
 * 11    2/01/02 2:49p Vprashant
 * added some more defines
 * 
 * 10    10/17/01 4:25p Fremont
 * add MAX_PATH
 * 
 * 8     8/18/99 2:43p Fremont
 * Make ANSI C safe
 * 
 * 7     3/04/99 1:41p Glen
 * Disable LONGLONG for non win only
 * 
 * 6     3/02/99 4:02p Darrelld
 * long long causes C file warning
 * 
 * 4     12/02/98 12:36p Glen
 * 
 * 3     11/24/98 11:29a Darrelld
 * Define byte type
 * 
 * 2     11/09/98 11:58a Darrelld
 * Updates after native trial
 * 
 * 1     11/04/98 8:37a Darrelld
 * header files
 * 
 * 3     10/27/98 4:49p Darrelld
 * Time client / server work
 * 
 * 2     10/23/98 5:05p Darrelld
 * Enhancements and socket testing
 * 
 * 1     10/19/98 2:31p Darrelld
 * Vx Layer Sources
 * 
 * 1     10/16/98 1:43p Darrelld
 * Vx Layer Sources
 */

/* KEEP THIS FILE ANSI C COMPATIBLE (no // comments) */

/* This stuff was shamelessly ripped off from windef.h */

#include "LtaDefine.h"

#if defined(__GNUC__)
#define __UNUSED__ __attribute__((unused))
#else
#define __UNUSED__
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifndef null
#define null	NULL
#endif

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

#ifndef __cplusplus
/* These are predefined in C++ */
#ifndef false
#define false               0
#endif

#ifndef true
#define true                1
#endif
#endif /* __cplusplus */

typedef unsigned long       DWORD;
#ifndef __INCvxTypesOldh
typedef	unsigned char	    UCHAR;
typedef unsigned long       ULONG;
typedef int                 BOOL;
#endif
typedef unsigned char       BYTE;
typedef unsigned char       byte;

#ifndef WIN32
// (taken from WINBASE.H and WINNT.H)
#define INFINITE            0xFFFFFFFF  // Infinite timeout
#define MAXLONG    			0x7fffffff 
#define STATUS_WAIT_0                    ((DWORD   )0x00000000L)
#define WAIT_OBJECT_0       ((STATUS_WAIT_0 ) + 0 )
#define STATUS_TIMEOUT                   ((DWORD   )0x00000102L)

#ifndef WAIT_TIMEOUT
#define WAIT_TIMEOUT                        STATUS_TIMEOUT
#endif

#ifndef WAIT_FAILED
#define WAIT_FAILED (DWORD)0xFFFFFFFF
#endif

#ifndef SUCCEEDED
#   define SUCCEEDED(Status) ((HRESULT)(Status) >= 0)
#endif /*SUCCEEDED*/
#ifndef FAILED
#   define FAILED(Status)    ((HRESULT)(Status) < 0)
#endif /*FAILED*/

#endif

#if !defined(__VXWORKS__)
// Conflicts with new vxworks definition
#ifndef _LB_ISI_MGMT_
typedef void *  	HANDLE;
#else
#undef _LB_ISI_MGMT_
#endif
#endif
typedef long		HRESULT;

typedef unsigned short      WORD;
typedef long     			LONG;
typedef unsigned short		word;
typedef unsigned char		boolean;
#ifndef _TCHAR_DEFINED
typedef char*				LPSTR;
typedef const char *		LPCTSTR;
typedef const char *		LPCSTR;
typedef char *				LPTSTR;
#endif
#ifdef WIN32
typedef __int64				LONGLONG;
typedef unsigned __int64	ULONGLONG;
#else
typedef long long 			__int64;
#ifdef __cplusplus
typedef long long			LONGLONG;
typedef unsigned long long	ULONGLONG;
#endif /* __cplusplus */
#endif /* WIN32 */

/* Convert a ULONGLONG to a ULONG without warnings */
#define ULL2UL(a) (*(ULONG*)&(a))

#ifndef _INC_MINMAX
#	define _INC_MINMAX

#	ifndef MAX
#		define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#	endif

#	ifndef MIN
#		define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#	endif
#endif



/* Microsoft specific declaration to declare a "C" called routine */
/* in a CPP module. */
/* Must be used in a task declaration as follows: */
/*

	int VXLCDECL taskName( int a1, ... )
	{
		body of the task
		return status;
	}

 */
#ifdef WIN32
#define VXLCDECL __cdecl
#else
#define VXLCDECL
#endif


#ifndef MAX_PATH
// Adopt the Windows definition
#define MAX_PATH 260
#endif


#ifdef _WIN32
    typedef struct _SECURITY_ATTRIBUTES /*{
        DWORD nLength;
        LPVOID lpSecurityDescriptor;
        BOOL bInheritHandle;
    }*/ SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;
#endif


#ifdef __cplusplus
}
#endif


#endif /* _VXLTYPES_H */
