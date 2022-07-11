//
// EchelonStandardDefinitions.h
//
// Copyright © 2022 Dialog Semiconductor
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
// This file contains standard definitions for Echelon C++ 
// software.
//

#ifdef SUPPORT_PRAGMA_ONCE
#pragma once
#endif

#ifndef __EchelonStandardDefinitions_h
#define __EchelonStandardDefinitions_h

#ifndef BOOL
/* changed the definition of BOOL from an unsigned char to an int in order to match vxTypesOld.h & vxlTypes.h */
typedef int  BOOL;
#endif
typedef unsigned char  BOOLEAN;
typedef unsigned char  BYTE;
typedef unsigned char  OCTET;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef char CHAR;
#ifdef __VXWORKS__
typedef unsigned char UCHAR;
#endif

#ifdef _MSC_VER
typedef __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;
#else
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
#endif

typedef unsigned long  ULONG;
typedef unsigned short USHORT;

typedef uint8_t		UInt8;
typedef uint16_t	UInt16;
typedef uint32_t	UInt32;

typedef int8_t		Int8;
typedef int16_t		Int16;
typedef int32_t		Int32;

#ifndef FALSE
#define FALSE		0
#endif

#ifndef TRUE
#define TRUE		1 
#endif

#ifndef OK
#define OK      0
#endif

#ifndef FAIL
#define FAIL   -1
#endif

#ifdef NDEBUG
# define STATIC static
#else
# define STATIC
#endif

#ifndef EXTERN 
#define EXTERN extern
#endif

//
// Standard error returns.  Convention is that error returns contain
// an optional "area" in the high byte.  Error codes between 1..127
// are valid for every area.  Error codes between 128 and 255 are area
// specific.  Error code 0 is reserved for the generic area only.
// In particular, OK should always be 0x0000 regardless of the area.
//
typedef uint16_t					EchErr;

#define ECHERRS \
	ECHERR(ECHERR_OK, 						0) \
	ECHERR(ECHERR_OUT_OF_RANGE,				1) \
	ECHERR(ECHERR_TIMEOUT,					2) \
	ECHERR(ECHERR_INVALID_PARAM,			3) \
	ECHERR(ECHERR_NO_MEMORY,				4) \
	ECHERR(ECHERR_UNDERFLOW,				5) \
	ECHERR(ECHERR_OVERFLOW,					6) \
	ECHERR(ECHERR_DATA_INTEGRITY,			7) \
	ECHERR(ECHERR_NOT_FOUND,				8) \
	ECHERR(ECHERR_ALREADY_OPEN,				9) \
	ECHERR(ECHERR_NOT_OPEN,					10) \
	ECHERR(ECHERR_DEVICE_ERR,				11) \
	ECHERR(ECHERR_INVALID_DEVICE_ID,		12) \
	ECHERR(ECHERR_NO_MSG_AVAILABLE,			13) \
	ECHERR(ECHERR_NO_BUFFER_AVAILABLE,		14) \
	ECHERR(ECHERR_NO_RESOURCES,				15) \
	ECHERR(ECHERR_INVALID_LENGTH,			16) \
	ECHERR(ECHERR_OPEN_FAILURE,				17) \
	ECHERR(ECHERR_SECURITY_VIOLATION,		18) \
	ECHERR(ECHERR_CREATE_FAILURE,			19) \
	ECHERR(ECHERR_REMOVE_FAILURE,			20) \
	ECHERR(ECHERR_INVALID_OPERATION,		21) \
	ECHERR(ECHERR_SEND_FAILURE,				22) \
	ECHERR(ECHERR_RECV_FAILURE,				23) \
	ECHERR(ECHERR_INVALID_HANDLE,			24) \
	ECHERR(ECHERR_BIND_FAILURE,				25) \
	ECHERR(ECHERR_CONNECT_FAILURE,			26) \
	ECHERR(ECHERR_NO_SERVICE,				27) \
	ECHERR(ECHERR_BUSY,						28) \
	ECHERR(ECHERR_NOT_INITIALIZED,          30) \
	ECHERR(ECHERR_NOT_POSSIBLE,				31) \
	ECHERR(ECHERR_NOT_INSTALLED,			32) \
	ECHERR(ECHERR_REBOOT_REQUIRED,          33) \
	ECHERR(ECHERR_PLATFORM_NOT_APPLICABLE,	34) \
	ECHERR(ECHERR_RESOURCE_NOT_AVAILABLE,	35) \

#undef ECHERR
#define ECHERR(l, v)	l = v,

typedef enum
{
	ECHERRS
} _EchErr;

#define ECHERR_END_GLOBAL_ERRORS		127
#define ECHERR_START_AREA_ERRORS		128

// Echelon Error Areas
#define ECHERR_AREA_GLOBAL				0   // use global error codes above
#define ECHERR_AREA_SMPL				1   // simplicity error codes
#define ECHERR_AREA_PAL					2   // see pal.h
#define ECHERR_AREA_RTP                 3   // see rtp.h
#define ECHERR_AREA_SLBM                4   // see slbm.h
#define ECHERR_AREA_UPGRADE				5	// see upgrd.h
#define ECHERR_AREA_RFM                 6   // see rfm.h
#define ECHERR_AREA_RAL					7   // see ral.h
#define ECHERR_AREA_AES					8	// see aes.h

#define ECHERR_GET_ERROR(e)				((e) & 0xFF)
#define ECHERR_GET_AREA(e)				(((e)>>8) & 0xFF)
#define ECHERR_SET_AREA(e,a)			((e) ? ((e) | ((a)<<8)) : ECHERR_OK)

#define ECHERR_IS_OK(e)                 ((e) == ECHERR_OK)

// Use these macros to bracket a collection of C routines.
#ifdef __cplusplus
#define C_API_START	extern "C" {
#define C_API_END	}
#else
#define C_API_START
#define C_API_END
#define false FALSE
#define true TRUE
typedef unsigned char bool;
#endif

#ifdef _MSC_VER
#	define C_DECL	/* __cdecl */
#else
#	define C_DECL
#endif

#if defined(TARGET_PLATFORM) && !defined(NDEBUG) && defined(LEAK_CHECK)
// some files use this so map them too after we do it
#define MALLOC(n)	Malloc_debug(n, __FILE__, __LINE__)
#define FREE(p)		Free_debug(p)
#define STRDUP(s)   Strdup_debug(s, __FILE__, __LINE__)
#else
#define MALLOC(n)	malloc(n)
#define FREE(p)		free(p)
#define STRDUP(s)   strdup(s)
#endif

// rand() only returns a value in the range 0-0x7fff; following generates 32 bits.
#define RAND32()		((rand()<<17) | (rand()<<2) | (rand() & 0x3))

// These are used for accessing bits in a bit array that is made up of an array of bytes.
#define BITS_PER_BYTE				8
#define BITS_ARRAY_SIZE(bits)		(((bits)+BITS_PER_BYTE-1)/BITS_PER_BYTE)
#define BITS_BYTE_OFFSET(bitIndex)	((bitIndex)/BITS_PER_BYTE)
#define BITS_MASK(bitIndex)			(1<<((bitIndex)%BITS_PER_BYTE))

// These are used to access the high and low bytes of a 16-bit value
#define LO_BYTE(w)					((BYTE)((w) & 0xff))
#define HI_BYTE(w)					(LO_BYTE((w)>>8))

#define INFINITE_LOOP               1

#ifdef __GNUC__
#define __UNUSED__ __attribute__((unused))
#else
#define __UNUSED__
#endif

#endif		// __EchelonStandardDefinitions_h
