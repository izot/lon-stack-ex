/****************************************************************
 *  Filename: iLonSntp.h
 *
 *  Description:  SNTP client support.
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
 *	F Bainbridge July 1999
 *
 ****************************************************************
 *
 * $Log: /Dev/ShareIp/include/iLonSntp.h $
 *
 * 16    1/03/06 5:05p Fremont
 * rename constant to eliminate warning
 *
 * 15    12/08/05 2:05p Fremont
 * exclude ipUtil.h from WIN32 builds
 *
 * 14    10/19/05 5:04p Fremont
 * Port SNTP to support IPv6 -- this is for the i.LON platform only
 *
 * 13    7/01/05 3:26p Fremont
 * EPR 37421 - add support for setting/getting a server by host name
 *
 * 12    5/25/05 6:28p Fremont
 * EPRS FIXED: 32239 Added update interval control
 *
 * 11    4/01/05 10:08a Fremont
 * Add iLonSntpUpdateNow() for SNTP over short-lived PPP connections
 *
 * 10    3/26/02 6:53p Fremont
 * add last-synched prototype
 *
 * 9     3/15/02 4:29p Fremont
 * rename pnc to iLon
 *
 * 7     4/18/00 3:15p Darrelld
 * Fix to work on Windows
 *
 * 5     11/16/99 1:40p Fremont
 * move task priority define
 *
 * 4     9/21/99 5:20p Fremont
 * Add access func
 *
 * 3     9/20/99 1:33p Darrelld
 * Update for 50K max size log and persistent log state
 *
 * 2     9/03/99 12:23p Fremont
 * add logging protos
 *
 * 1     7/29/99 5:15p Fremont
 */

#ifndef __ILONSNTP_H
#define __ILONSNTP_H

// EPANG
/*
#if !defined(WIN32) && defined(__VXWORKS__)
#include "ipUtil.h"
#endif
*/

#ifdef __cplusplus
extern "C" {
#endif

	/* Export / import definitions */
#include <LtaDefine.h>
#include <sys/types.h>
#if defined(linux)
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <netdb.h>
#endif

/* This is the number defined in the RFC */
#define NUM_SNTP_SERVERS 2

/* The server info */
#ifdef	SNTP_SERVER_NAME_LENGTH
// Copy value from BSP
#define SNTP_SERVER_NAME_LEN SNTP_SERVER_NAME_LENGTH
#else
#define SNTP_SERVER_NAME_LEN 64
#endif

typedef struct
{
#ifdef INET6
	struct sockaddr_in6 ipAddr;
#else
	ulong_t ipAddr;
#endif
    ushort_t ipPort;
	char host[SNTP_SERVER_NAME_LEN+1]; /* Name or IP addr string */

} ILonSntpServer;

/* This is the standard SNTP port number */
#define	SNTP_PORT 123

/* Special values for update interval */
#define ILON_SNTP_UPDATE_INTERVAL_AUTO 0	/* Use adaptive algorithm */
#define ILON_SNTP_UPDATE_INTERVAL_NONE -1	/* No automatic update, must be done manually */

/* Delay limits for SNTP adaptive auto-update interval, in seconds */
#define ILON_SNTP_MIN_ADAPTIVE_TIMEOUT 60	/* 1 minute */
#define ILON_SNTP_MAX_ADAPTIVE_TIMEOUT 900 /* 15 minutes */
#define ILON_SNTP_MAX_ADAPTIVE_TIMEOUT_INCREASE 60 /* 1 minute */

#define ILON_SNTP_ERROR_RETRY_TIMEOUT 20

/* Wait this long for a server to respond, in seconds */
#define ILON_SNTP_SERVER_TIMEOUT 2

/* Window of acceptable correction values, in msecs */
#define ILON_SNTP_MSEC_DIFF_LOW	50
#define ILON_SNTP_MSEC_DIFF_HIGH 100
#define ILON_SNTP_MSEC_DIFF_TARGET ((ILON_SNTP_MSEC_DIFF_HIGH + ILON_SNTP_MSEC_DIFF_LOW) / 2)

/* Update the RTC this often, in seconds */
#define ILON_SNTP_RTC_UPDATE_INTERVAL (60 * 30) /* 1/2 hour */

#ifdef __VXWORKS__
#define ILON_SNTP_LOG_FILE_PATH FILEPATH_ROOT "sntp.log"
#elif defined(linux)
#define ILON_SNTP_LOG_FILE_FOLDER FILEPATH_ROOT "log"
#define ILON_SNTP_LOG_FILE_PATH FILEPATH_ROOT "log/sntp.log"
#else
/* This is just so it can compile */
#define ILON_SNTP_LOG_FILE_PATH "sntp.log"
#endif
/* maximum log file size */
#define ILON_SNTP_LOG_FILE_MAXSIZE (50*1024)

/* Start the client task */
LTA_EXTERNAL_FN STATUS iLonSntpInit(void);
/* Shut down the client task */
LTA_EXTERNAL_FN void iLonSntpDestroy(void);
/* Set/get time server values */

#ifdef WIN32
/* Reset the time correction.  This can be used after disabling SNTP.  For example,
 * if the user had been using the VNI SNTP, and then disables it in favor of
 * a Windows SNTP, you would want to clear the offset, since the windows system time
 * is presumably going to be kept in synch.
 */
LTA_EXTERNAL_FN void ilonSntpResetOffset(void);

LTA_EXTERNAL_FN void ilonSntpShutdown(void);
#endif

// EPANG
/*
#if defined(__VXWORKS__)
//#ifndef WIN32
LTA_EXTERNAL_FN STATUS iLonSntpSetServer(int index, IpV4V6Addr *ipAddr, ushort_t ipPort, uchar_t wakeup);
LTA_EXTERNAL_FN STATUS iLonSntpGetServer(int index, IpV4V6Addr *ipAddr, ushort_t *ipPort);
#else
// Windows code not ready for IPv6
LTA_EXTERNAL_FN STATUS iLonSntpSetServer(int index, ulong_t ipAddr, ushort_t ipPort, uchar_t wakeup);
LTA_EXTERNAL_FN STATUS iLonSntpGetServer(int index, ulong_t *ipAddr, ushort_t *ipPort);
#endif
*/
#ifdef INET6
LTA_EXTERNAL_FN STATUS iLonSntpSetServer(int index, struct sockaddr_in6 * ipAddr, ushort_t ipPort, uchar_t wakeup);
LTA_EXTERNAL_FN STATUS iLonSntpGetServer(int index, struct sockaddr_in6 * ipAddr, ushort_t *ipPort);
#else
LTA_EXTERNAL_FN STATUS iLonSntpSetServer(int index, ulong_t ipAddr, ushort_t ipPort, uchar_t wakeup);
LTA_EXTERNAL_FN STATUS iLonSntpGetServer(int index, ulong_t *ipAddr, ushort_t *ipPort);
#endif

LTA_EXTERNAL_FN STATUS iLonSntpSetServerName(int index, const char *host, ushort_t ipPort, uchar_t wakeup);
LTA_EXTERNAL_FN STATUS iLonSntpGetServerName(int index, char **host, ushort_t *ipPort);
/* Determine if time has been synched */
LTA_EXTERNAL_FN uchar_t iLonSntpTimeSynched(void);
LTA_EXTERNAL_FN ulong_t iLonSntpTimeLastSynched(void);
/* Update time immediately */
LTA_EXTERNAL_FN STATUS iLonSntpUpdateNow();
/* Update interval control. Interval in seconds, see special values defined above. */
LTA_EXTERNAL_FN void iLonSntpSetUpdateInterval(ulong_t interval);
LTA_EXTERNAL_FN ulong_t iLonSntpGetUpdateInterval();
/* Logging */
LTA_EXTERNAL_FN void iLonSntpEnableLog(void);
LTA_EXTERNAL_FN void iLonSntpDisableLog(void);
LTA_EXTERNAL_FN int iLonSntpLoggingEnabled(void);

LTA_EXTERNAL_FN void iLonSntpSetDefault(void);

#ifdef __cplusplus
}
#endif

#endif /* __ILONSNTP_H */
