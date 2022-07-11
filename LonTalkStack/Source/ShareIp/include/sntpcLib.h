/* sntpcLib.h - Simple Network Time Protocol client include file */

/* Copyright 1984-1997 Wind River Systems, Inc.
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
01f,23aug04,rp   merged from COMP_WN_IPV6_BASE6_ITER5_TO_UNIFIED_PRE_MERGE
01e,04nov03,rlm  Ran batch header path update for header re-org.
01d,03nov03,rlm  Removed wrn/coreip/ prefix from #includes for header re-org.
01c,25aug99,cno  Add extern "C" definition (SPR21825)
01b,15jul97,spm  code cleanup, documentation, and integration; entered in
                 source code control
01a,20apr97,kyc  written

*/

#ifndef __INCsntpch_iLON
#define __INCsntpch_iLON

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef WIN32
#define IMPORT
#define LOCAL static
#define bzero(addr,size) memset((addr), 0, (size))
#define errnoSet(err) /* nothing */

#elif defined(linux)
#include <errno.h>
#define errnoSet(err) errno=err
#define IMPORT
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

#include <sntp.h>
#include <Osal.h>

/* defines */

#define S_sntpcLib_INVALID_PARAMETER         (M_sntpcLib | 1)
#define S_sntpcLib_INVALID_ADDRESS           (M_sntpcLib | 2)
#define S_sntpcLib_TIMEOUT                   (M_sntpcLib | 3)
#define S_sntpcLib_VERSION_UNSUPPORTED       (M_sntpcLib | 4)
#define S_sntpcLib_SERVER_UNSYNC             (M_sntpcLib | 5)

#define SNTP_CLIENT_REQUEST 0x0B             /* standard SNTP client request */

#ifndef INET6
/* SNTP data retrived from SNTP IPv4 protocol message in mSntpcTimeGet */
typedef struct sntpData
    {
    unsigned char     stratum;                 
    char              poll;
    char              precision;
    } SNTP_DATA;
    
IMPORT STATUS mSntpcTimeGet (char *, u_int, struct timespec *, char *,
                             SNTP_DATA *);
                             
#endif

#ifdef __VXWORKS__
IMPORT STATUS sntpcInit (u_short, char *);
#else
IMPORT STATUS sntpcInit (u_short);
#endif // WIN32

#ifdef INET6
IMPORT STATUS sntpcTimeGet (struct sockaddr_in6*, u_int, struct timespec *);
#else
IMPORT STATUS sntpcTimeGet (char * pServerAddr, u_int, struct timespec *);
#endif

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

#endif /* __INCsntpch_iLON */

