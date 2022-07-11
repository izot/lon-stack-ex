/***************************************************************
 *  vldv.h
 *
 * Copyright © 2006-2022 Dialog Semiconductor
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
 *  This file contains the, typedefs, and literals for the
 *  LON Applications Programming Interface Driver LDV level
 *  interface.
 *
 *  Date Created:  August 8th, 1990
 ****************************************************************/

#ifndef __VLDV_H
#define __VLDV_H

#include "EchelonStandardDefinitions.h"

// This is a non-standard form of LDV.  This is because we use
// the LDV API to map to VNI calls.  This provides the flexibility
// of VNI on the simulators.  We also rename the symbols from
// standard LDV to avoid conflicts with software that uses the
// actual LDV (such as the i.lon).

#define LDV_EXTERNAL_FN

typedef void            * pVoid;
typedef short           * pShort;
typedef int				* pInt;
typedef unsigned int	* pUInt;
typedef uint32_t        * pUInt32;
typedef char            * pStr;
typedef void 			*(*PFUNC) (void *);

#define LDVCODES \
    LDVCODE(OK,					0)	\
    LDVCODE(NOT_FOUND,			1)	\
    LDVCODE(ALREADY_OPEN,		2)	\
    LDVCODE(NOT_OPEN,			3)	\
    LDVCODE(DEVICE_ERR,			4)	\
    LDVCODE(INVALID_DEVICE_ID,	5)	\
    LDVCODE(NO_MSG_AVAIL,		6)	\
    LDVCODE(NO_BUFF_AVAIL,		7)	\
    LDVCODE(NO_RESOURCES,		8)	\
    LDVCODE(INVALID_BUF_LEN,	9)	\
	LDVCODE(NOT_ENABLED,		10) \
	LDVCODE(INITIALIZATION_FAILED, 11) \
	LDVCODE(OPEN_FAILURE,		   12) 	\
	LDVCODE(INVALID_HANDLE,		   13)

#undef LDVCODE
#define LDVCODE(sym,val) LDV_##sym = val,

enum
{
	LDVCODES
};

#ifndef LDVCode
typedef short LDVCode;
#endif

#ifdef __cplusplus
#ifdef USING_LAYER2 
namespace VldvLayer2 {
#else
#define USING_LAYER5
#ifdef unix
namespace VldvLayer5 {
#else
C_API_START
#endif
#endif
#endif

LDVCode LDV_EXTERNAL_FN vldv_open(const char* pName, pShort handle, int xcvrId);
LDVCode LDV_EXTERNAL_FN vldv_close(short handle);
LDVCode LDV_EXTERNAL_FN vldv_read(short handle, pVoid msg_p, short len);
LDVCode LDV_EXTERNAL_FN vldv_write(short handle, pVoid msg_p, short len);
LDVCode LDV_EXTERNAL_FN vldv_register(short handle, OsalHandle event);
LDVCode LDV_EXTERNAL_FN vldv_setTxidLifetime(short handle, int lifetime);
LDVCode LDV_EXTERNAL_FN vldv_setExclusionUid(short handle, const unsigned char* pUid);
void 	LDV_EXTERNAL_FN vldv_setClientTipcDumpEnabled(bool value);

#ifdef __cplusplus
#ifdef USING_LAYER2
} using namespace VldvLayer2;
#else
#ifdef unix
} using namespace VldvLayer5;
#else
C_API_END
#endif
#endif
#endif	

#endif	// __VLDV_H

