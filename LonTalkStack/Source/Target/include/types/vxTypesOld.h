/* vxTypesOld.h - old VxWorks type definition header */

/* 
 * Copyright 1984-1994 Wind River Systems, Inc. 
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
$Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Target/include/types/vxTypesOld.h#1 $
 * 
 * 8     6/24/08 7:56a Glen
 * Support for EVNI running on the DCM
 * 1. Fix GCC problems such as requirement for virtual destructor and
 * prohibition of <class>::<method> syntax in class definitions
 * 2. Fix up case of include files since Linux is case sensitive
 * 3. Add LonTalk Enhanced Proxy to the stack
 * 4. Changed how certain optional features are controlled at compile time
 * 5. Platform specific stuff for the DC such as file system support
 * 
 * 1     12/05/07 11:19p Mwang
 * 
 * 7     11/17/03 4:24p Garyb
 * Added another test case (_STLP_NEW_PLATFORM_SDK) for Platform SDK.
 * 
 * 6     1/27/03 10:53a Garyb
 * Added mechanism to auto-detect and/or override signedness of basic
 * types (SIGNED macro) to avoid compile errors with different versions of
 * compiler/headers.
 * 
 * 5     10/24/02 11:43a Hho
 * changes for VS.NET compilation.
 * 
 * 3     2/16/99 11:10a Glen
 * Remove FUNCPTR redefine
 * 
 * 2     11/09/98 11:56a Darrelld
 * Updates after native trial
 * 
 * 1     11/04/98 8:42a Darrelld
 * Windows Target header files

02b,28sep95,ms	 removed "static __inline__" (SPR #4500)
02b,12jul95,ism  added simsolaris support
02a,19mar95,dvs  removed tron references.
01z,01sep94,ism  fixed comment as per SPR# 1512.
01y,02dec93,pme  added Am29K family support.
01x,12jun93,rrr  vxsim.
02a,26may94,yao  added PPC support.
01w,09jun93,hdn  added support for I80X86
01v,12feb93,srh  added C++ versions of FUNCPTR, et al.
01u,13nov92,dnw  added definition of VOID (SPR #1781)
01t,02oct92,srh  replaced conditional around volatile, const, and signed so
                 they won't be elided when __STDC__ is defined.
                 added __cplusplus to __STDC__ condition.
01s,22sep92,rrr  added support for c++
01r,08sep92,smb  made some additions for the MIPS.
01q,07sep92,smb  added __STDC__ and modes to maintain compatibility with 5.0
01p,07jul92,rrr  moved STACK_GROW and ENDIAN to vxArch.h
01o,03jul92,smb  changed name from vxTypes.h.
01n,26may92,rrr  the tree shuffle
01m,25nov91,llk  included sys/types.h.
01l,04oct91,rrr  passed through the ansification filter
		  -fixed #else and #endif
		  -removed TINY and UTINY
		  -changed VOID to void
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01k,01oct91,jpb  fixed MIPS conditional for undefined CPU_FAMILY.
01j,20sep91,wmd  conditionalized out defines for const, unsigned and volatile
		 for the MIPS architecture.
01i,02aug91,ajm  added support for MIPS_R3k.
01h,15may91,gae  added define for "signed" when not available for pre-ANSI.
01g,29apr91,hdn  added defines and macros for TRON architecture.
01f,28apr91,del  added defines of __volatile__ and __const__ if !_STDC_
		 && _GNUC__
01f,24mar91,del  added INSTR * define for I960.
01e,28jan91,kdl  added DBLFUNCPTR and FLTFUNCPTR.
01d,25oct90,dnw  changed void to void except when linting.
01c,05oct90,shl  added copyright notice.
                 made #endif ANSI style.
01b,10aug90,dnw  added VOIDFUNCPTR
01a,29may90,del  written.
*/

/*
DESCRIPTION
This header file contains a mixture of stuff.
1) the old style typedefs (ie. POSIX now says they must end with _t).
   These will be phased out gradually.
2) a mechanism for getting rid of const warning which are produced by the
   GNU C compiler. Hopefully, this will be removed in the future.
3) macros that are so longer needed for vxWorks source code but maybe needed
   by some customer applications and are therefore provided for backward
   compatability.
4) system III typedefs (used by netinet) which do not fit in anywhere else.

*/

#ifndef __INCvxTypesOldh
#define __INCvxTypesOldh

#ifdef __cplusplus
extern "C" {
#endif

#include "sys/types.h"

/* vxWorks types */

// [GaryB 1/24/2003]
// Microsoft started including explicit 'signed' typedefs
// in "basetsd.h" in one of their newer Platform SDK releases
// (and also in the version that ships with Visual Studio .NET).
// Unfortunately, an explicitly 'signed' typedef is treated as
// a completely different type than an implicitly 'signed' one
// (i.e. no signed keyword at all) despite actually being the same
// underlying type.
// To avoid conflicts with VxWorks vs Windows (pre- and post- new "basetsd.h")
// pre-define the SIGNED macro to match your build environment.

// Visual Studio .NET uses newer Platform SDK with explicitly signed types:
#if _MSC_VER >= 1300
#  ifndef SIGNED
#    define SIGNED      signed
#  endif

// VxWorks uses implicitly signed types:
#elif !defined(_MSC_VER)
#  ifndef SIGNED
#    define SIGNED      /* implicit signed */
#  endif

// STLport requires a macro to be set to use the new Platform SDK,
// so check for that macro too
#elif defined(_STLP_NEW_PLATFORM_SDK)
#  ifndef SIGNED
#    define SIGNED      signed
#  endif

// Otherwise it depends what version of Platform SDK is installed
// (don't know how to auto-detect, so must pre-define SIGNED accordingly).
// Default to implicitly signed types to match Visual Studio 6:
#elif !defined(SIGNED)
#  define SIGNED        /* implicit signed */
#endif

typedef	SIGNED   char	INT8;
typedef	SIGNED   short	INT16;
typedef	SIGNED   int    INT32;

typedef	unsigned char	UINT8;
typedef	unsigned short	UINT16;
typedef	unsigned int	UINT32;

typedef	unsigned char	UCHAR;
typedef unsigned short	USHORT;
typedef	unsigned int	UINT;
typedef unsigned long	ULONG;

typedef	int		BOOL;
typedef	int		STATUS;
typedef int 		ARGINT;
#if 0 // ECH
typedef void		VOID;
#endif

#ifdef __cplusplus
typedef int     	(*FUNCPTR) (...);     /* ptr to function returning int */
typedef void 		(*VOIDFUNCPTR) (...); /* ptr to function returning void */
typedef double 		(*DBLFUNCPTR) (...);  /* ptr to function returning double*/
typedef float 		(*FLTFUNCPTR) (...);  /* ptr to function returning float */
#else
typedef int 		(*FUNCPTR) ();	   /* ptr to function returning int */
typedef void 		(*VOIDFUNCPTR) (); /* ptr to function returning void */
typedef double 		(*DBLFUNCPTR) ();  /* ptr to function returning double*/
typedef float 		(*FLTFUNCPTR) ();  /* ptr to function returning float */
#endif			/* _cplusplus */


/* This structure and the following definitions are needed to get rid
   of const warning produced by the GNU C compiler.
 */

#ifndef unix
#if defined(__STDC__) || defined(__cplusplus)
typedef union
    {
    long pm_int;
    void *pm_v;
    const void *pm_cv;
    char *pm_c;
    unsigned char *pm_uc;

    signed char *pm_sc;
    const char *pm_cc;
    const unsigned char *pm_cuc;
    const signed char *pm_csc;
    short *pm_s;
    ushort_t *pm_us;
    const short *pm_cs;
    const ushort_t *pm_cus;
    int *pm_i;
    uint_t *pm_ui;
    const int *pm_ci;
    const uint_t *pm_cui;
    long *pm_l;
    ulong_t *pm_ul;
    const long *pm_cl;
    const ulong_t *pm_cul;

    int8_t *pm_i8;
    uint8_t *pm_ui8;
    const int8_t *pm_ci8;
    const uint8_t *pm_cui8;
    int16_t *pm_i16;
    uint16_t *pm_ui16;
    const int16_t *pm_ci16;
    const uint16_t *pm_cui16;
    int32_t *pm_i32;
    uint32_t *pm_ui32;
    const int32_t *pm_ci32;
    const uint32_t *pm_cui32;
#if _ARCH_MOVE_SIZE > 4
    int64_t *pm_i64;
    const int64_t *pm_ci64;
#if _ARCH_MOVE_SIZE > 8
    int128_t *pm_i128;
    const int128_t *pm_ci128;
#endif
#endif
    } pointer_mix_t;

#define CHAR_FROM_CONST(x)	(char *)(x)
#define VOID_FROM_CONST(x)	(void *)(x)

#endif /* __STDC__ */
#endif

#define STACK_DIR		_ARCH_STACK_DIR
#define ALIGN_MEMORY		_ARCH_ALIGN_MEMORY
#define ALIGN_STACK		_ARCH_ALIGN_STACK
#define ALIGN_REGS		_ARCH_ALIGN_REGS

#define	NBBY	8		/* number of bits in a byte */

/* modes - must match O_RDONLY/O_WRONLY/O_RDWR in ioLib.h! */

#define READ            0
#define WRITE           1
#define UPDATE          2

#if 0 // ECH OUT conflicts with WinSock
/* Select uses bit masks of file descriptors in longs.
 * These macros manipulate such bit fields (the filesystem macros use chars).
 * FD_SETSIZE may be defined by the user, but the default here
 * should be >= maxFiles parameter in iosInit call found in usrConfig.c.
 * If this define is changed, recompile the source, or else select() will
 * not work.
 */

#ifndef	FD_SETSIZE
#define	FD_SETSIZE	256
#endif	/* FD_SETSIZE */

typedef long	fd_mask;
#define NFDBITS	(sizeof(fd_mask) * NBBY)	/* bits per mask */
#ifndef howmany
#define	howmany(x, y)	((unsigned int)(((x)+((y)-1)))/(unsigned int)(y))
#endif	/* howmany */

typedef	struct fd_set
    {
    fd_mask	fds_bits[howmany(FD_SETSIZE, NFDBITS)];
    } fd_set;

#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))
#endif // ECH OUT

/* system III  typedefs (used by netinet) */

typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
typedef	unsigned short	ushort;


/* historical definitions - now obsolete */

typedef char		TBOOL;		/* obsolete */


/* architecture dependent typedefs */

#ifdef	CPU_FAMILY

#if	CPU_FAMILY==MC680X0 
typedef unsigned short INSTR;		/* word-aligned instructions */
#endif	/* CPU_FAMILY==MC680X0 */

#if	CPU_FAMILY==SPARC || CPU_FAMILY==MIPS || CPU_FAMILY==SIMSPARCSUNOS || CPU_FAMILY==SIMHPPA || CPU_FAMILY==SIMSPARCSOLARIS
typedef unsigned long INSTR;		/* 32 bit word-aligned instructions */
#endif	/* CPU_FAMILY==SPARC || CPU_FAMILY==MIPS || CPU_FAMILY==SIMSPARCSUNOS || CPU_FAMILY==SIMHPPA || CPU_FAMILY==SIMSPARCSOLARIS */

#if	CPU_FAMILY==I960
typedef	unsigned long INSTR;		/* 32 bit word-aligned instructions */
#endif	/* CPU_FAMILY==I960 */

#if	CPU_FAMILY==I80X86
typedef	unsigned char INSTR;		/* char instructions */
#endif	/* CPU_FAMILY==I80X86 */

#if	CPU_FAMILY==AM29XXX
typedef	unsigned long INSTR;		/* 32 bit word-aligned instructions */
#endif	/* CPU_FAMILY==AM29XXX */

#if     (CPU_FAMILY==PPC)
typedef unsigned long INSTR;            /* 32 bit word-aligned instructions */
#endif  /* (CPU_FAMILY==PPC) */

#endif	/* CPU_FAMILY */

/* ANSI type qualifiers */

#if !defined(__STDC__) && !defined(__cplusplus)

#ifdef  __GNUC__
#define	volatile	__volatile__
#define	const		__const__
#define	signed		__signed__
#else
#if     !(defined(CPU_FAMILY) && CPU_FAMILY==MIPS)
#define	volatile
#define	const
#define	signed
#endif	/* !(defined(CPU_FAMILY) && CPU_FAMILY==MIPS) */
#endif	/* __GNUC__ */

#endif	/* !defined(__STDC__) && !defined(__cplusplus) */

#if     CPU_FAMILY==MIPS
#define CHAR_FROM_CONST(x)	(char *)(x)
#define VOID_FROM_CONST(x)	(void *)(x)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __INCvxTypesOldh */
