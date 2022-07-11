/****************************************************************************
 *
 * Copyright © 1994-2022 Dialog Semiconductor
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
 *  FILE DESCRIPTION:  This file contains platform dependant flags
 *  and basic data types.  All data types of NI, NS, as well as internal
 *  data types used by the NSS core code are derived from the basic
 *  data types of this file.
 *
 *  If you are porting the NI or NS layer to a non-Win32, non-DOS platform,
 *  please examine the data types defined in this file, and modify them
 *  as appropriate to your OS, compiler, and CPU.
 *
 *  -------- Note on Byte-Ordering -----
 *  The defines BYTE_BIG_ENDIAN and BYTE_LITTLE_ENDIAN are used in these files to
 *  indicate the byte ordering.
 *
 *  BYTE_BIG_ENDIAN means the high byte value is located in low memory address,
 *  and bits are numbered from left to right. The Motorola 68k family and
 *  the Neuron Chip are examples of BYTE_BIG_ENDIAN processors.
 *
 *  BYTE_LITTLE_ENDIAN means the high byte value is located in high memory address,
 *  and bits are numbered from right to left. The Intel 80x86 processors are
 *  an example of BYTE_LITTLE_ENDIAN processors.
 *
 *  The following description of these terms from "C, A Reference Manual"
 *  by Samuel P. Harbison and Guy L. Steele Jr. may refresh your memory:
 *
 *  "Computers differ in their storage byte order; that is, they differ in
 *  which byte of storage they consider to be first in a larger piece.  In
 *  "right-to-left" or "little endian" architectures, which include the
 *  DEC VAX computers and the Intel 80x86 microprocessors, the address of
 *  a 32-bit integer is the address of the low-order byte of the integer.
 *  In "left-to-right" or "big endian" architectures, which include the
 *  Motorola 680x0 microprocessor family and the Zilog Z8000, the address
 *  of a 32-bit integer is the address of the high-order byte of the
 *  integer."
 *
 *  "Components of a structure type are allocated in the order of increasing
 *  addresses, that is, either left to right or right to left depending
 *  on the byte order of the computer.  Because bit fields are also packed
 *  following the byte order, it is natural to number the bits in a
 *  piece of storage following the same convention.  Thus, in a left-to-right
 *  computer the most significant (leftmost) bit of a 32-bit integer would
 *  be bit number 0 and the least significant bit would be bit number 31.
 *  In right-to-left computers the least significant (rightmost) bit would
 *  be bit 0, and so forth".
 *
 ***************************************************************************/
#ifndef _NSPLATFORM_H
#define _NSPLATFORM_H

#if defined(linux) || defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__)
#include <endian.h>
#endif

/* The following symbols define the platform.
 * You may change them if they are not correct for your environment.
 * See the file ns_opts.h
 */
#include "ns_opts.h"


//////////////////// Start of Compiler-Dependent Symbols ////////////////////

/*
 *	The use of BIG_ENDIAN and LITTLE_ENDIAN as mutually exclusive
 *	symbol is now deprecated.  They are renamed to BYTE_BIG_ENDIAN
 *	and BYTE_LITTLE_ENDIAN, respectively. 
 *
 *	The reason for the deprecation is that the usage conflicts with 
 *	recent platforms such as Linux, in which both symbols are defined
 *	but with different values.
 *
 *	Although BIG_ENDIAN and LITTLE_ENDIAN should continue to work
 *	on your current platform, they could become unreliable when your
 *	change platforms.  Thus we strongely recommend converting your code
 *	to use the BYTE_xxx version of the symbol.
 *
 *	The bit-order symbols BITF_BIG_ENDIAN and BITF_LITTLE_ENDIAN are not
 *	affected.
 */

#ifndef BYTE_BIG_ENDIAN
	#ifndef BYTE_LITTLE_ENDIAN
		/* ENDIAN not specified, use default settings
		*/

		/* Neuron-C programs.  The neuron-C compiler automatically defines
		   _ECHELON as the result of including echelon.h */
		#ifdef _ECHELON
			#define NEURON_HOSTED               /* runs on a Neuron Chip */
			#define BYTE_BIG_ENDIAN             /* neuron is always BYTE_BIG_ENDIAN */
			#define COMPILE_8                   /* 8-bit compiler */
			/* Use this to substitute an expression based on platform */
			#define NEURON_OR_WIN32(neuron_exp,win32_exp) neuron_exp

		#else
			/*	If our legacy. mutually exclusive BIG_ENDIAN or LITTLE_ENDIAN
				is already defined explicitly, convert them to BYTE_BIG_ENDIAN
				and BYTE_LITTLE_ENDIAN respectively.
			*/
			#if defined(BIG_ENDIAN) && !defined(LITTLE_ENDIAN)
				#define BYTE_BIG_ENDIAN
			#elif !defined(BIG_ENDIAN) && defined(LITTLE_ENDIAN)
				#define BYTE_LITTLE_ENDIAN

			/*	Win32 is assumed to run on x86 and is little-endian.
				If it is not the case, define BYTE_BIG_ENDIAN and BITF_BIG_ENDIAN 
				in ns_opts.h to override this.
			*/
			#elif defined(WIN32)
				#define BYTE_LITTLE_ENDIAN

			/* BSD style (also used in Linux) */
			#elif defined(BYTE_ORDER)
				#if BYTE_ORDER == BIG_ENDIAN && defined(BIG_ENDIAN)
					#define BYTE_BIG_ENDIAN
				#elif BYTE_ORDER == LITTLE_ENDIAN && defined(LITTLE_ENDIAN)
					#define BYTE_LITTLE_ENDIAN
				#else
					#error "Unknown BYTE_ORDER"
				#endif

			/* VxWorks style */
			#elif defined(_BYTE_ORDER)
				#if _BYTE_ORDER == _BIG_ENDIAN && defined(_BIG_ENDIAN)
					#define BYTE_BIG_ENDIAN
				#elif _BYTE_ORDER == _LITTLE_ENDIAN && defined(_LITTLE_ENDIAN)
					#define BYTE_LITTLE_ENDIAN
				#else
					#error "Unknown _BYTE_ORDER"
				#endif

			/* Non-BSD style Linux */
			#elif defined(__BYTE_ORDER)
				#if __BYTE_ORDER == __BIG_ENDIAN && defined(__BIG_ENDIAN)
					#define BYTE_BIG_ENDIAN
				#elif __BYTE_ORDER == __LITTLE_ENDIAN && defined(__LITTLE_ENDIAN)
					#define BYTE_LITTLE_ENDIAN
				#else
					#error "Unknown __BYTE_ORDER"
				#endif

			/* iLON MIPS architecture (VxWorks headers not included to define _BYTE_ORDER) */
			#elif defined(__VXWORKS__)
				#define BYTE_BIG_ENDIAN

			/*	Other environment -- assume BYTE_LITTLE_ENDIAN byte and bit orders.  
				If it is not the case, define BYTE_BIG_ENDIAN and BITF_BIG_ENDIAN 
				in ns_opts.h to override this.
			 */
			#else
				/*
					To resolve, define BYTE_LITTLE_ENDIAN or BYTE_BIG_ENDIAN
					in project, or include an include file that defines it.  
					If bit endian doesn't follow byte endian, also define it
					in project or in ns_opts.h.
				*/
				#error "Undefined BYTE_ORDER, _BYTE_ORDER, and __BYTE_ORDER"
			#endif	/* various platforms */
		#endif  /* ifndef BYTE_LITTLE_ENDIAN */
	#endif  /* ifndef BYTE_BIG_ENDIAN */

	/* If bitfield ENDIAN not specified, base it on byte field ENDIAN setting
	*/
	#ifndef BITF_BIG_ENDIAN
		#ifndef BITF_LITTLE_ENDIAN

			#ifdef BYTE_BIG_ENDIAN
				#define BITF_BIG_ENDIAN
			#else
				#define BITF_LITTLE_ENDIAN
			#endif

		#endif  /* ifndef BITF_LITTLE_ENDIAN */
	#endif  /* ifndef BITF_BIG_ENDIAN */
#endif						// BYTE_BIG_ENDIAN

/* Win32 programs.  By default, Microsoft Visual C++ includes WIN32 as the
   project setting.  If your compiler doesn't do that, you should manually
   specify it to the compiler.
 */
#ifdef WIN32
	#define NS_CDECL        cdecl
	#define WIN32_HOSTED                /* runs in Win32 OS environment */
	/* Use this to substitute an expression based on platform */
	#define NEURON_OR_WIN32(neuron_exp,win32_exp) win32_exp
#endif

#ifndef NS_CDECL
	#define NS_CDECL
#endif	/* ifndef NS_CDECL */

#ifndef _ECHELON
	/* Default packing alignment for NSS structures is single byte boundaries */
	#pragma pack(1)		// _NSS_PACKING
#endif

//////////////////// End of Compiler-Dependent Symbols ////////////////////


/* Basic data types for 8-bit compilers such as the neuron-C compiler.
 * "short is assumed to be 8-bit, "long" is assumed to be 16-bit.
 * If that is not the case for your compiler,
 * replace them with another 8 and 16 bit types.
 */
#ifdef COMPILE_8
typedef unsigned short Byte;        /* 8-bits */
typedef unsigned long Word;         /* 16-bits */
#define NetEnum(enumType) enumType  /* 8-bit enums */
#define MAX_NET_ENUM (-1)           /* maximum value for enum */
typedef boolean Bool;               /* 8-bit boolean */
typedef unsigned int BitField;      /* bit field */
typedef struct {int bytes[4];} Long;/* 32-bits */
typedef struct {int bytes[4];} SignedLong;/* 32-bits signed */

/* Basic data types for 16 and 32 bit compilers.
 * "short" is assumed to be 16-bit.
 * If that is not the case for your compiler,
 * replace it with another 16-bit type.
 */
#else
typedef unsigned char Byte;         /* 8-bits */
typedef unsigned short Word;        /* 16-bits (assumes short to be 16-bits) */
typedef unsigned long Long;			/* 32-bits unsigned long */
typedef signed long SignedLong;     /* 32-bits signed long */
#define NetEnum(enumType) Byte      /* 8-bit enums */
#define MAX_NET_ENUM (255)          /* maximum value for enum */
#define NetEnumWord(enumType) NetWord /* 16-bit enums */
typedef Byte Bool;                  /* 8-bit boolean */
typedef unsigned char BitField;		/* bit field, bit aligned */
#endif

/* true and false */
#ifndef _ECHELON
#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif
#endif


/* 16-bit word (BYTE_BIG_ENDIAN ordering if exposed, native ordering if internal) */
typedef Word NetWord;
typedef NetWord niWord;
typedef NetWord NeuronMemAddr;              /* neuron memory address */

/* 32-bit word */
typedef Long NetLong;
typedef SignedLong NetSignedLong;

/* The following section contains short-hands for some frequently-used types.
   They are provided as a convenience.
 */

typedef unsigned int Uint;          /* unsigned integer, native size and byte order */

typedef unsigned long Ulong;        /* unsigned long, native size and byte order */

typedef unsigned short Ushort;      /* unsigned short, native size and byte order */

typedef unsigned char Uchar;        /* unsigned char */

/* swap bytes if native byte-ordering is different from NSS */
#ifdef BYTE_LITTLE_ENDIAN
#define NET_SWAB(aWord)      ( (((unsigned)(aWord))>>8) | (((aWord)&0xff)<<8) )
#define NET_SWAB_LONG(aLong) ( (((unsigned)(aLong))>>24) | (((aLong)>>8)&0xff00) | \
                               (((aLong)<<8)&0xff0000UL) | ((aLong)<<24) )
#define NET_SWAP_WORD(aWord) aWord = NET_SWAB(aWord)
/*_swab((char *)&(aWord), (char *)&(aWord), 2) */
#define NET_SWAP_LONG(aLong) aLong = NET_SWAB_LONG(aLong)
#else
#define NET_SWAB(aWord) 		(aWord)
#define NET_SWAB_LONG(aLong)	(aLong)
#define NET_SWAP_WORD(aWord)
#define NET_SWAP_LONG(aWord)
#endif

#define SET_STRUCT(structName,byteValue) memset(&(structName),byteValue,sizeof(structName))
#define ZERO_STRUCT(structName) SET_STRUCT(structName,0)
#define SET_PSTRUCT(pStruct,byteValue) memset(pStruct,byteValue,sizeof(*(pStruct)))
#define ZERO_PSTRUCT(pStruct) SET_PSTRUCT(pStruct,0)

// Bit Field macros for hiding endian differences
// NOTES:
// 1. There are 8 BITS<n>() macros, one for each of 1..8 bitfields.
// 2. BITS<n>() arguments are listed in little endian order (LSB first)
// 3. If you include a ';' after the macro, it will compile in C++ but not C so it's best not to.
// 4. Unnamed bitfields are not possible.  So use names like "rsvd".  They must be unique with a structure.
// 5. The recommended format of invoking BITS<n> is:
/*
	typedef struct 
	{
		BITS4	   (field1,			2,		// Comment
					field2,			2,		// Comment
					field3,			2,		// Comment
					field4,			2)		// Comment
	} Example; 
*/
#define BITFIELD(name, length)	BitField name:length;
#ifdef  BITF_LITTLE_ENDIAN
#define BFLE(name, length)	BITFIELD(name, length)
#define BFBE(name, length)
#else
#define BFLE(name, length)
#define BFBE(name, length)	BITFIELD(name, length)
#endif
#define BITS1(f1,l1)\
		BFBE(f1,l1)															BFLE(f1,l1)
#define BITS2(f1,l1,f2,l2)\
		BFBE(f2,l2)		BITS1(f1,l1)										BFLE(f2,l2)
#define BITS3(f1,l1,f2,l2,f3,l3)\
		BFBE(f3,l3)		BITS2(f1,l1,f2,l2)									BFLE(f3,l3)
#define BITS4(f1,l1,f2,l2,f3,l3,f4,l4)\
		BFBE(f4,l4)		BITS3(f1,l1,f2,l2,f3,l3)							BFLE(f4,l4)
#define BITS5(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5)\
		BFBE(f5,l5)		BITS4(f1,l1,f2,l2,f3,l3,f4,l4)						BFLE(f5,l5)
#define BITS6(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5,f6,l6)\
		BFBE(f6,l6)		BITS5(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5)				BFLE(f6,l6)
#define BITS7(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5,f6,l6,f7,l7)\
		BFBE(f7,l7)		BITS6(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5,f6,l6)			BFLE(f7,l7)
#define BITS8(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5,f6,l6,f7,l7,f8,l8)\
		BFBE(f8,l8)		BITS7(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5,f6,l6,f7,l7)	BFLE(f8,l8)

#ifndef _ECHELON
#pragma pack()
#endif

/*	Path delimiter character macro.

	In a backslash system, we may also recognize a forward slash to be friendly.
	But in a forward slash system, a backward slash may be an escape character
	and so should not be treated as a separator.
*/
// Directory separators
#ifdef WIN32
#define DIR_SEPARATOR_CHAR		'\\'
#define DIR_SEPARATOR_STRING	"\\"
#define ALT_DIR_SEPARATOR_CHAR	'/'
#define ALT_DIR_SEPARATOR_STRING "/"
#else
#define DIR_SEPARATOR_CHAR		'/'
#define DIR_SEPARATOR_STRING	"/"
#define ALT_DIR_SEPARATOR_CHAR	'\\'
#define ALT_DIR_SEPARATOR_STRING "\\"
#endif

#if (DIR_SEPARATOR_CHAR == '\\')
#define IS_DIR_SEPARATOR_CHAR(ch) ((ch)==DIR_SEPARATOR_CHAR || (ch)==ALT_DIR_SEPARATOR_CHAR)
#else
#define IS_DIR_SEPARATOR_CHAR(ch) ((ch)==DIR_SEPARATOR_CHAR)
#endif


#endif	// _NSPLATFORM_H
