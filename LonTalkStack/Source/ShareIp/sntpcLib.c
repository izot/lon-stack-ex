/* sntpcLib.c - Simple Network Time Protocol (SNTP) client library
 *
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
modification history 
--------------------
01j,16mar99,spm  doc: removed references to configAll.h (SPR #25663)
01e,14dec97,jdi  doc: cleanup.
01d,10dec97,kbw  making man page changes
01c,27aug97,spm  corrections for man page generation
01b,15jul97,spm  code cleanup, documentation, and integration; entered in
                 source code control
01a,24may97,kyc  written
*/

/* 
DESCRIPTION
This library implements the client side of the Simple Network Time 
Protocol (SNTP), a protocol that allows a system to maintain the 
accuracy of its internal clock based on time values reported by one 
or more remote sources.  The library is included in the VxWorks image 
if INCLUDE_SNTPC is defined at the time the image is built.

USER INTERFACE
The sntpcTimeGet() routine retrieves the time reported by a remote source and
converts that value for POSIX-compliant clocks.  The routine will either send a 
request and extract the time from the reply, or it will wait until a message is
received from an SNTP/NTP server executing in broadcast mode.

INCLUDE FILES: sntpcLib.h

SEE ALSO: clockLib, RFC 1769
*/

/* includes */

#ifdef WIN32
#include <windows.h>
#include <sys/timeb.h>
#include <VxWorks.h>
#include "sysLib.h"
#include <VxLayer.h>
#include <VxSockets.h>
#elif defined(__VXWORKS__)
#include "vxWorks.h"
#include "sysLib.h"
#include "ioLib.h"
#include "inetLib.h"
#include "hostLib.h"
#include "sockLib.h"
#include "errnoLib.h"
#include "string.h"
#include "selectLib.h"
#include <time.h>
#else
#include "vxWorks.h"
#include "sysLib.h"
#include "string.h"
#include "VxSockets.h"

#include    <sys/socket.h>
#include    <netdb.h>
#include 	<unistd.h>
#include 	<netinet/in.h>
#include 	<arpa/inet.h>
#include 	<net/if.h>
#include 	<sys/ioctl.h>
#include 	<net/if_arp.h>
#include 	<sys/timeb.h>
#include <time.h>
#endif

#include 	<stdio.h>
#if defined(linux)
#include    "nsplatform.h"
#endif

#if !defined(M_sntpcLib)
#define M_sntpcLib     (113 << 16)
#endif

/* Define this here to get external debug defs in sntpcLib.h */
#define DEBUG

#include "sntpcLib.h"
/* There is a potential for getting the wrong copy of the above file,
   since it exists in two places, so check for signature */
#ifndef __INCsntpch_iLON
#error Picked up the wrong copy of sntpcLib.h! Check include path order.
#endif

/* globals */

u_short sntpcPort;

/* forward declarations */

LOCAL STATUS sntpcListen (u_int, struct timespec *);
#ifdef INET6
LOCAL STATUS sntpcFetch (struct sockaddr_in6 *, u_int, struct timespec *);
#else
LOCAL STATUS sntpcFetch (struct in_addr *, u_int, struct timespec *);
#endif

/*******************************************************************************
*
* sntpcInit - set up the SNTP client
*
* This routine is called to link the SNTP client module into the VxWorks
* image. It assigns the UDP source and destination port according to the
* corresponding SNTP_PORT setting.
* 
* RETURNS: OK, always.
*
* ERRNO: N/A
*
* NOMANUAL
*/

STATUS sntpcInit
    (
    u_short 	port 	/* UDP source/destination port */
    )
    {
    sntpcPort = port;

    return (OK);
    }

/*******************************************************************************
*
* sntpcFractionToNsec - convert time from the NTP format to POSIX time format
*
* This routine converts the fractional part of the NTP timestamp format to a 
* value in nanoseconds compliant with the POSIX clock.  While the NTP time 
* format provides a precision of about 200 pico-seconds, rounding error in the 
* conversion routine reduces the precision to tenths of a micro-second.
* 
* RETURNS: Value for struct timespec corresponding to NTP fractional part
*
* ERRNO:   N/A
*
* INTERNAL
*
* Floating-point calculations can't be used because some boards (notably
* the SPARC architectures) disable software floating point by default to 
* speed up context switching. These boards abort with an exception when
* floating point operations are encountered.
*
* NOMANUAL
*/

LOCAL ULONG sntpcFractionToNsec
    (
    ULONG sntpFraction      /* base 2 fractional part of the NTP timestamp */
    )
    {
    ULONG factor = 0x8AC72305; /* Conversion factor from base 2 to base 10 */
    ULONG divisor = 10;        /* Initial exponent for mantissa. */
    ULONG mask = 1000000000;   /* Pulls digits of factor from left to right. */
    int loop;
    ULONG nsec = 0;
    BOOL shift = FALSE;        /* Shifted to avoid overflow? */


    /* 
     * Adjust large values so that no intermediate calculation exceeds 
     * 32 bits. (This test is overkill, since the fourth MSB can be set 
     * sometimes, but it's fast).
     */
 
    if (sntpFraction & 0xF0000000)
        {
        sntpFraction /= 10;
        shift = TRUE;
        }

    /* 
     * In order to increase portability, the following conversion avoids
     * floating point operations, so it is somewhat obscure.
     *
     * Incrementing the NTP fractional part increases the corresponding
     * decimal value by 2^(-32). By interpreting the fractional part as an
     * integer representing the number of increments, the equivalent decimal
     * value is equal to the product of the fractional part and 0.2328306437.
     * That value is the mantissa for 2^(-32). Multiplying by 2.328306437E-10
     * would convert the NTP fractional part into the equivalent in seconds.
     *
     * The mask variable selects each digit from the factor sequentially, and
     * the divisor shifts the digit the appropriate number of decimal places. 
     * The initial value of the divisor is 10 instead of 1E10 so that the 
     * conversion produces results in nanoseconds, as required by POSIX clocks.
     */

    for (loop = 0; loop < 10; loop++)    /* Ten digits in mantissa */
        {
	nsec += sntpFraction * (factor/mask)/divisor;  /* Use current digit. */
	factor %= mask;    /* Remove most significant digit from the factor. */
	mask /= 10;        /* Reduce length of mask by one. */
	divisor *= 10;     /* Increase preceding zeroes by one. */
        }

    /* Scale result upwards if value was adjusted before processing. */

    if (shift)
        nsec *= 10;

    return (nsec);
    }

/*******************************************************************************
*
* sntpcTimeGet - retrieve the current time from a remote source
*
* This routine stores the current time as reported by an SNTP/NTP server in
* the location indicated by <pCurrTime>.  The reported time is first converted
* to the elapsed time since January 1, 1970, 00:00, GMT, which is the base value
* used by UNIX systems.  If <pServerAddr> is NULL, the routine listens for 
* messages sent by an SNTP/NTP server in broadcast mode.  Otherwise, this
* routine sends a request to the specified SNTP/NTP server and extracts the
* reported time from the reply.  In either case, an error is returned if no 
* message is received within the interval specified by <timeout>.  Typically, 
* SNTP/NTP servers operating in broadcast mode send update messages every 64 
* to 1024 seconds.  An infinite timeout value is specified by WAIT_FOREVER.
* 
* RETURNS: OK, or ERROR if unsuccessful.
*
* ERRNO:
*  S_sntpcLib_INVALID_PARAMETER
*  S_sntpcLib_INVALID_ADDRESS
*/

STATUS sntpcTimeGet
    (
#ifdef INET6
	struct sockaddr_in6* pAddr6, 	/* server IP address or hostname */
#else
	char * 		pServerAddr, 	/* server IP address or hostname */
#endif
    u_int 		timeout,	/* timeout interval in ticks */
    struct timespec * 	pCurrTime	/* storage for retrieved time value */
    )
    {
    STATUS result;
#ifndef INET6
    struct in_addr 	target;
#endif

    if (pCurrTime == NULL || (timeout < 0 && timeout != WAIT_FOREVER))
        {
        errnoSet (S_sntpcLib_INVALID_PARAMETER);
        return (ERROR);
        }
#ifdef INET6
    result = sntpcFetch (pAddr6, timeout, pCurrTime);
#else
    if (pServerAddr == NULL)
#ifdef WIN32
        return (ERROR);
#else
        result = sntpcListen (timeout, pCurrTime);
#endif
    else
        {
        VXSOCKADDR pHostAddr;

        /*target.s_addr = hostGetByName (pServerAddr);
        if (target.s_addr == ERROR)*/
        pHostAddr = vxsAddrName(pServerAddr);
        if (pHostAddr != NULL)
        {
            target.s_addr = vxsAddrGetAddr(pHostAddr);
            vxsFreeSockaddr(pHostAddr);
        }
        else
        {
            target.s_addr = vxsInetAddr(pServerAddr);
        }

        if (target.s_addr == ERROR)
            {
            errnoSet (S_sntpcLib_INVALID_ADDRESS);
            return (ERROR);
            }
        result = sntpcFetch (&target, timeout, pCurrTime);
        }
#endif
    return (result);
    }

/* Return the current UTC time as a timespec */
void getCurTimespec(struct timespec *t)
{
    OsalClockGetTime(CLOCK_REALTIME, t);
}

/* Timespec math routines. */
/* A non-normalized timespec may have either a nsec value */
/* who's magnituded is between one and two seconds (1sec <= val < 2sec) */
/* and/or a sign difference between the sec and nsec values */

/* Normalize nsecs for magnitude, i.e. (-1 sec) < nsecs < (1 sec) */
LOCAL void normalizeTimespecMagnitude(timespecDiff *pTime)
{
    if (pTime->tv_nsec > 1000000000)
    {
        pTime->tv_sec++;
        pTime->tv_nsec -= 1000000000;
    }
    else if (pTime->tv_nsec < -1000000000)
    {
        pTime->tv_sec--;
        pTime->tv_nsec += 1000000000;
    }
}

/* Normalize nsecs for sign, i.e. same sign as secs (exempting 0) */
LOCAL void normalizeTimespecSign(timespecDiff *pTime)
{
	if (((pTime->tv_sec < 0) != (pTime->tv_nsec < 0)) && 
        (pTime->tv_sec != 0) && (pTime->tv_nsec != 0))
	{
		if (pTime->tv_sec < 0)
		{
			pTime->tv_sec++;
			pTime->tv_nsec -= 1000000000;
		}
		else
		{
			pTime->tv_sec--;
			pTime->tv_nsec += 1000000000;
		}
	}
}
/* t3 = t1 - t2 */
/* Subtract two times. Inputs must be normalized. Output is normalized */
LOCAL void diffTimespecs(struct timespec *t1, struct timespec *t2, timespecDiff *t3)
{
    t3->tv_sec = t1->tv_sec - t2->tv_sec;
    t3->tv_nsec = t1->tv_nsec - t2->tv_nsec;
    /* The signs may differ, do normalize */
	normalizeTimespecSign(t3);
}

#ifdef DEBUG
/* expose some internals for debug */
timespecDiff TDiff1, TDiff2, TSum1; 
struct timespec T1, T2, T3, T4;
#endif

/* Compute the adjusted time */
/* t1 : originate (client transmit) */
/* t2 : receive (server) */
/* t3 : transmit (server) */
/* t4 : destination (client receive) */
LOCAL void computeTime(struct timespec *t1, struct timespec *t2, 
                        struct timespec *t3, struct timespec *t4, 
                        struct timespec *pCurrTime)
{
    timespecDiff diff1, diff2, sum1, sum2;
    
    /* If we cannot detect any message delay (due to clock resolution) */
    /* just use the transmit timestamp (T3) */
    if ((t1->tv_sec == t4->tv_sec) && (t1->tv_nsec == t4->tv_nsec))
    {
        *pCurrTime = *t3;
    }
    else
    {
        /* Compute the adjustment */
        /* delta = ((T2 - T1) + (T3 - T4)) / 2 */
        diffTimespecs(t2, t1, &diff1);
        diffTimespecs(t3, t4, &diff2);
        sum1.tv_sec = diff1.tv_sec + diff2.tv_sec;
        sum1.tv_nsec = diff1.tv_nsec + diff2.tv_nsec;
        if (sum1.tv_sec & 1)
        {
            /* seconds not even, move one to nsecs */
            if (sum1.tv_sec > 0)
            {
                sum1.tv_sec--;
                sum1.tv_nsec += 1000000000;
            }
            else
            {
                sum1.tv_sec++;
                sum1.tv_nsec -= 1000000000;
            }
        }
        sum1.tv_sec /= 2;
        sum1.tv_nsec /= 2;
        normalizeTimespecMagnitude(&sum1);

#ifdef DEBUG
        /* expose some internals for debug */
        TDiff1 = diff1;
        TDiff2 = diff2;
        TSum1 = sum1;
        T1 = *t1;
        T2 = *t2;
        T3 = *t3;
        T4 = *t4;
#endif
    
        /* Add adjustment to current (destination) time */    
        sum2.tv_sec = t4->tv_sec + sum1.tv_sec;
        sum2.tv_nsec = t4->tv_nsec + sum1.tv_nsec;
        
        /* Normalize */
        normalizeTimespecMagnitude(&sum2);
        normalizeTimespecSign(&sum2);
    
        pCurrTime->tv_sec = sum2.tv_sec;
        pCurrTime->tv_nsec = sum2.tv_nsec;
    }
}

/*******************************************************************************
*
* sntpcFetch - send an SNTP request and retrieve the time from the reply
*
* This routine sends an SNTP request to the IP address specified by
* <pTargetAddr>, converts the returned NTP timestamp to the POSIX-compliant 
* clock format with the UNIX base value (elapsed time since 00:00 GMT on 
* Jan. 1, 1970), and stores the result in the location indicated by <pTime>.
* 
* RETURNS: OK, or ERROR if unsuccessful.
*
* ERRNO:
*  S_sntpcLib_SERVER_UNSYNC
*  S_sntpcLib_VERSION_UNSUPPORTED
*
* NOMANUAL
*/

LOCAL STATUS sntpcFetch
    (
#ifdef INET6
    struct sockaddr_in6 * 	pTargetAddr6, 	/* SNTP/NTP server IP address */
#else
    struct in_addr * 	pTargetAddr, 	/* SNTP/NTP server IP address */
#endif
    u_int 		timeout,	/* timeout in ticks */
    struct timespec * 	pCurrTime	/* storage for retrieved time value */
    )
    {
    SNTP_PACKET sntpRequest;     /* sntp request packet for */
                                 /* transmission to server */
    SNTP_PACKET sntpReply;       /* buffer for server reply */
    /*struct sockaddr_in dstAddr;*/
    /*struct sockaddr_in servAddr;*/
#ifdef INET6
    struct sockaddr_in6 dstAddr;
    struct sockaddr_in6 servAddr;
    fd_set readFds;
    socklen_t servAddrLen;
#else
    VXSOCKADDR pDstAddr = NULL;
    VXSOCKADDR pServAddr = NULL;
    VXSOCKET sntpSocket;
#endif
    struct timeval sockTimeout;
#ifdef SET_SNTP_BROADCAST
    int optval;
#endif
    int clockRate;
    int result;
    struct timespec t1, t2, t3, t4;

#ifdef INET6
    /* Set destination for request. */
    memcpy ( &dstAddr, pTargetAddr6, sizeof (dstAddr));
    dstAddr.sin6_port = htons(sntpcPort);

    /* Create socket for transmission. */
    int sntpSocket = socket (AF_INET6, SOCK_DGRAM, 0);
    if (sntpSocket < 0)
    {
        return (ERROR);
    }
#else
    /* Set destination for request. */
    pDstAddr = vxsAddrValue(pTargetAddr->s_addr);
    if (pDstAddr == NULL)
        goto returnError;

    vxsSetPort(pDstAddr, sntpcPort);

    /* Create socket for transmission. */
    /*sntpSocket = socket (AF_INET, SOCK_DGRAM, 0);*/
    sntpSocket = vxsSocket(SOCK_DGRAM);
    if (sntpSocket == INVALID_SOCKET)
        /*return (ERROR);*/
        goto returnError;

    /*
     * Enable broadcast option for socket in case that address is given.
     * This use of the SNTP client is not likely, so ignore errors. If
     * the broadcast address is used, and this call fails, the error will
     * be caught by sendto() below.
     */

#ifdef SET_SNTP_BROADCAST
/* These settings are not right for the Cisco stack, or for Windows */
    optval = 1;
    result = setsockopt (sntpSocket, SOL_SOCKET, SO_BROADCAST,
                         (char *)&optval, sizeof (optval));
#endif

#endif // INET6

    /* Initialize SNTP message buffers. */

    bzero ( (char *)&sntpRequest, sizeof (sntpRequest));
    bzero ( (char *)&sntpReply, sizeof (sntpReply));

    sntpRequest.leapVerMode = SNTP_CLIENT_REQUEST;

#ifdef INET6
    bzero ( (char *) &servAddr, sizeof (servAddr));
    servAddrLen = sizeof (servAddr);
#else
    pServAddr = vxsGetSockaddr();
    if (pServAddr == NULL)
        goto returnError;
    vxsSetPort(pDstAddr, sntpcPort);
#endif

    /* Set the transmit timestamp */
    getCurTimespec(&t1);

    /* Transmit SNTP request. */
#ifdef INET6
    if (sendto (sntpSocket, (caddr_t)&sntpRequest, sizeof(sntpRequest), 0,
                (struct sockaddr *)&dstAddr, sizeof (dstAddr)) == -1)
        {
        close (sntpSocket);
        return (ERROR);
        }
#else
    if (vxsSendTo(sntpSocket, (char *)&sntpRequest, sizeof(sntpRequest), 0, pDstAddr) == ERROR)
        {
        /*close (sntpSocket);*/
        vxsCloseSocket(sntpSocket);
        /*return (ERROR);*/
        goto returnError;
        }
#endif
    /* Convert timeout value to format needed by select() call. */

    if (timeout != WAIT_FOREVER)
        {
        clockRate = sysClkRateGet ();
        sockTimeout.tv_sec = timeout / clockRate;
        sockTimeout.tv_usec = (1000000 * timeout % clockRate) / clockRate;
        }

    /* Wait for reply at the ephemeral port selected by the sendto () call. */

#ifdef INET6
    FD_ZERO (&readFds);
    FD_SET (sntpSocket, &readFds);
#endif

#ifdef INET6
    if (timeout == WAIT_FOREVER)
        result = select (FD_SETSIZE, &readFds, NULL, NULL, NULL);
    else
        result = select (FD_SETSIZE, &readFds, NULL, NULL, &sockTimeout);
#else
    if (timeout == WAIT_FOREVER)
        /*result = select (FD_SETSIZE, &readFds, NULL, NULL, NULL);*/
        result = vxsSelectRead(sntpSocket, NULL);
    else
        /*result = select (FD_SETSIZE, &readFds, NULL, NULL, &sockTimeout);*/
        result = vxsSelectRead(sntpSocket, &sockTimeout);
#endif

    if (result == ERROR)
        {
#ifdef INET6
        close (sntpSocket);
#else
        vxsCloseSocket(sntpSocket);
#endif
        goto returnError;
        }

    if (result == 0)    /* Timeout interval expired. */
        {
#ifdef INET6
        close (sntpSocket);
#else
        vxsCloseSocket(sntpSocket);
#endif
        errnoSet (S_sntpcLib_TIMEOUT);
        goto returnError;
        }

#ifdef INET6
    result = recvfrom (sntpSocket, (caddr_t)&sntpReply, sizeof (sntpReply),
                       0, (struct sockaddr *)&servAddr, &servAddrLen);
#else
    result = vxsRecvFrom(sntpSocket, (char *)&sntpReply, sizeof (sntpReply), 0, pServAddr);
#endif

    if (result == ERROR)
        {
#ifdef INET6
        close (sntpSocket);
#else
        vxsCloseSocket(sntpSocket);
#endif
        goto returnError;
        }

#ifdef INET6
    close (sntpSocket);
#else
    vxsCloseSocket(sntpSocket);
#endif

    /*
     * Return error if the server clock is unsynchronized, or the version is
     * not supported.
     */

    if ( (sntpReply.leapVerMode & SNTP_LI_MASK) == SNTP_LI_3 ||
        sntpReply.transmitTimestampSec == 0)
        {
        errnoSet (S_sntpcLib_SERVER_UNSYNC);
        /*return (ERROR);*/
        goto returnError;
        }

    if ( (sntpReply.leapVerMode & SNTP_VN_MASK) == SNTP_VN_0 ||
        (sntpReply.leapVerMode & SNTP_VN_MASK) > SNTP_VN_3)
        {
        errnoSet (S_sntpcLib_VERSION_UNSUPPORTED);
        /*return (ERROR);*/
        goto returnError;
        }

    /* Convert the NTP timestamp to the correct format and store in clock. */

    /* Add test for 2036 base value here! */

    /*sntpReply.transmitTimestampSec = ntohl (sntpReply.transmitTimestampSec) -
                                     SNTP_UNIX_OFFSET;*/
    t2.tv_sec = ntohl (sntpReply.receiveTimestampSec) - SNTP_UNIX_OFFSET;
    t2.tv_nsec = sntpcFractionToNsec (ntohl(sntpReply.receiveTimestampFrac));

    t3.tv_sec = ntohl (sntpReply.transmitTimestampSec) - SNTP_UNIX_OFFSET;
    t3.tv_nsec = sntpcFractionToNsec (ntohl(sntpReply.transmitTimestampFrac));

    /*
     * Adjust returned value if leap seconds are present.
     * This needs work!
     */

    /* if ( (sntpReply.leapVerMode & SNTP_LI_MASK) == SNTP_LI_1)
            sntpReply.transmitTimestampSec += 1;
     else if ((sntpReply.leapVerMode & SNTP_LI_MASK) == SNTP_LI_2)
              sntpReply.transmitTimestampSec -= 1;
    */

    getCurTimespec(&t4);

    /* The old way...
    sntpReply.transmitTimestampFrac = ntohl (sntpReply.transmitTimestampFrac);
    pCurrTime->tv_sec = sntpReply.transmitTimestampSec;
    pCurrTime->tv_nsec = sntpcFractionToNsec (sntpReply.transmitTimestampFrac);
    */

    /* Compute the time, accounting for network delays */
    computeTime(&t1, &t2, &t3, &t4, pCurrTime);
#ifndef INET6
    vxsFreeSockaddr(pDstAddr);
    vxsFreeSockaddr(pServAddr);
#endif
    return (OK);

returnError:
#ifndef INET6
    if (pDstAddr)
        vxsFreeSockaddr(pDstAddr);
    if (pServAddr)
        vxsFreeSockaddr(pServAddr);
#endif
    return (ERROR);

    }

/*******************************************************************************
*
* sntpcListen - retrieve the time from an SNTP/NTP broadcast
*
* This routine listens to the SNTP/NTP port for a valid message from any 
* SNTP/NTP server executing in broadcast mode, converts the returned NTP 
* timestamp to the POSIX-compliant clock format with the UNIX base value
* (elapsed time since 00:00 GMT on January 1, 1970), and stores the result in 
* the location indicated by <pTime>.
* 
* RETURNS: OK, or ERROR if unsuccessful.
*
* ERRNO:
*  S_sntpcLib_TIMEOUT
*
* NOMANUAL
*/

#ifndef WIN32
/* This is not ported for Windows */
LOCAL STATUS sntpcListen
    (
    u_int 		timeout,	/* timeout in ticks */
    struct timespec * 	pCurrTime	/* storage for retrieved time value */
    )
    {
    SNTP_PACKET sntpMessage;    /* buffer for message from server */
    struct sockaddr_in srcAddr;
    int sntpSocket;
    struct timeval sockTimeout;
    int clockRate;
    fd_set readFds;
    int result;
    socklen_t srcAddrLen;
 
    /* Initialize source address. */

    bzero ( (char *)&srcAddr, sizeof (srcAddr));
    srcAddr.sin_addr.s_addr = INADDR_ANY;
    srcAddr.sin_family = AF_INET;
    srcAddr.sin_port = htons(sntpcPort);

    /* Create socket for listening. */
  
    sntpSocket = socket (AF_INET, SOCK_DGRAM, 0);
    if (sntpSocket == -1) 
        return (ERROR);
      
    result = bind (sntpSocket, (struct sockaddr *)&srcAddr, sizeof (srcAddr));
    if (result == -1) 
        {
        close (sntpSocket);
        return (ERROR);
        }

    /* Convert timeout value to format needed by select() call. */

    if (timeout != WAIT_FOREVER)
        {
        clockRate = sysClkRateGet ();
        sockTimeout.tv_sec = timeout / clockRate;
        sockTimeout.tv_usec = (1000000 * timeout % clockRate) / clockRate;
        }

    /* Wait for broadcast message from server. */

    FD_ZERO (&readFds);
    FD_SET (sntpSocket, &readFds);
      
    if (timeout == WAIT_FOREVER)
        result = select (FD_SETSIZE, &readFds, NULL, NULL, NULL);
    else
        result = select (FD_SETSIZE, &readFds, NULL, NULL, &sockTimeout);

    if (result == -1)
        {
        close (sntpSocket);
        errnoSet (S_sntpcLib_TIMEOUT);
        return (ERROR);
        }

    if (result == 0)    /* Timeout interval expired. */
        {
        close (sntpSocket);
        errnoSet (S_sntpcLib_TIMEOUT);
        return (ERROR);
        }

    result = recvfrom (sntpSocket, (caddr_t) &sntpMessage, sizeof(sntpMessage),
                       0, (struct sockaddr *) &srcAddr, &srcAddrLen);
    if (result == -1) 
        {
        close (sntpSocket);
        return (ERROR);
        }

    close (sntpSocket);

    /*
     * Return error if the server clock is unsynchronized, or the version is 
     * not supported.
     */

    if ( (sntpMessage.leapVerMode & SNTP_LI_MASK) == SNTP_LI_3 ||
        sntpMessage.transmitTimestampSec == 0)
        {
        errnoSet (S_sntpcLib_SERVER_UNSYNC);
        return (ERROR);
        }

    if ( (sntpMessage.leapVerMode & SNTP_VN_MASK) == SNTP_VN_0 ||
        (sntpMessage.leapVerMode & SNTP_VN_MASK) > SNTP_VN_3)
        {
        errnoSet (S_sntpcLib_VERSION_UNSUPPORTED);
        return (ERROR);
        }

    /* Convert the NTP timestamp to the correct format and store in clock. */

    /* Add test for 2036 base value here! */

    sntpMessage.transmitTimestampSec = 
                                     ntohl (sntpMessage.transmitTimestampSec) -
                                     SNTP_UNIX_OFFSET;

    /*
     * Adjust returned value if leap seconds are present.
     * This needs work!
     */

    /* if ( (sntpReply.leapVerMode & SNTP_LI_MASK) == SNTP_LI_1)
            sntpReply.transmitTimestampSec += 1;
     else if ((sntpReply.leapVerMode & SNTP_LI_MASK) == SNTP_LI_2)
              sntpReply.transmitTimestampSec -= 1;
    */

    sntpMessage.transmitTimestampFrac = 
                                     ntohl (sntpMessage.transmitTimestampFrac);

    pCurrTime->tv_sec = sntpMessage.transmitTimestampSec;
    pCurrTime->tv_nsec =
                       sntpcFractionToNsec (sntpMessage.transmitTimestampFrac);

    return (OK);
    }
#endif /* WIN32 */

