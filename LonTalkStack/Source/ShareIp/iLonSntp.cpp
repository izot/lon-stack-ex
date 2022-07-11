/****************************************************************
 *  Filename: iLonSntp.c
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
 *  Description:  SNTP support.
 *
 *	F Bainbridge July 1999
 *
 ****************************************************************
*/

/*
 *
 * $Log: /Dev/ShareIp/iLonSntp.c $
 * 
 * 41    5/23/06 1:33p Fremont
 * Remove dependency on network interface name
 * 
 * 40    12/08/05 2:04p Fremont
 * Fix Windows compile, fix character arrays to hold IPv6 addresses
 * 
 * 39    10/25/05 11:19a Fremont
 * Fix compilation error on sntpcInit()
 * 
 * 38    10/19/05 5:04p Fremont
 * Port SNTP to support IPv6 -- this is for the i.LON platform only
 * 
 * 37    9/07/05 8:42p Fremont
 * EPR 38344 - SNTP not working with host name over dialup link
 * 
 * 36    7/05/05 4:31p Fremont
 * fix compilation for VNI
 * 
 * 35    7/01/05 3:26p Fremont
 * EPR 37421 - add support for setting/getting a server by host name
 * 
 * 34    5/25/05 6:28p Fremont
 * EPRS FIXED: 32239 Added update interval control
 * 
 * 33    4/01/05 10:08a Fremont
 * Add iLonSntpUpdateNow() for SNTP over short-lived PPP connections
 * 
 * 32    9/09/03 6:57p Iphan
 * Fixed EPR: 29510 - Unable to set time after factorydefault
 * 
 * 31    7/25/03 2:17p Iphan
 * EPRS FIXED: Supress subsequent error reporting to the eventlog until it
 * is able to establish communication with the server.
 * 
 * 30    7/22/03 4:24p Iphan
 * EPRS FIXED: 29205 - Received "Time synchronization failed".  Make the
 * SNTP client to fail 3 times consecutively before reporting an error.
 * 
 * 29    10/24/02 11:43a Hho
 * changes for VS.NET compilation.
 * 
 * 28    5/09/02 10:12a Fremont
 * Change stack to 8KB
 * 
 * 27    5/03/02 4:20p Fremont
 * Don't timeout from semaphore if no servers configured
 * 
 * 26    3/26/02 6:50p Fremont
 * Add routine for time-last-synched
 * 
 * 25    3/15/02 4:29p Fremont
 * rename pnc to iLon, hook new non-volatile data routines, set timeout to
 * max if no servers
 * 
 * 23    1/22/02 5:24p Fremont
 * change RTC interface, remove some dead code
 * 
 * 22    10/18/01 4:23p Fremont
 * For now comment out old hash table access. Should move this to EEPROM
 * or ltConfig
 * 
 * 21    9/15/00 1:37p Fremont
 * Fix vxWorks compile errors from Darrel's WIN32 port -- NOT TESTED
 * 
 * 20    4/18/00 3:15p Darrelld
 * Fix to work on Windows
 * 
 * 18    11/16/99 1:41p Fremont
 * literal for task priority
 * 
 * 17    11/15/99 3:13p Fremont
 * EPR 15689 Time sync tracing
 * 
 * 16    10/26/99 2:51p Fremont
 * Add log messages, fix sign display
 * 
 * 15    9/23/99 4:52p Fremont
 * Win32 fix
 * 
 * 14    9/21/99 5:20p Fremont
 * Add access func, fix auto log disable
 * 
 * 13    9/20/99 1:33p Darrelld
 * Update for 50K max size log and persistent log state
 * 
 * 12    9/03/99 12:23p Fremont
 * tweaks for logging
 * 
 * 11    8/18/99 5:46p Fremont
 * change task name
 * 
 * 10    8/17/99 10:50a Fremont
 * 
 * 9     8/12/99 6:58p Fremont
 * Unsync as appropriate
 * 
 * 8     8/12/99 9:47a Fremont
 * Make logging a non-debug feature
 * 
 * 7     8/04/99 11:39a Darrelld
 * Fix WIN32 conditionals
 * 
 * 6     8/03/99 2:42p Darrelld
 * Increase stack
 * 
 * 5     8/02/99 3:03p Fremont
 * Fix logging, add show command
 * 
 * 4     8/02/99 2:19p Fremont
 * Protect against NULL semaphore
 * 
 * 3     8/02/99 12:48p Fremont
 * Make init/destroy safe for multiple calls
 * 
 * 2     7/30/99 5:34p Fremont
 * fix file path
 * 
 * 1     7/29/99 5:10p Fremont
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

#ifdef __VXWORKS__
#include <sys/times.h>
#include <inetLib.h>
#include <echelon/ilon.h>
#include "iLonNvram.h"
#include <vxSockets.h>
#include <sysRtc.h>
#elif defined(linux)
#include <time.h>
#include <sys/timeb.h>
#include <stdio.h>
#else
#include <time.h>
#include <sys/timeb.h>
#include <winsock2.h>
#include <vxWorks.h>
#include <vxSockets.h>
#endif

#include <vxWorks.h>
#include <semLib.h>
#include <taskLib.h>
#include <iLonSntp.h>
#include <sntpcLib.h>
#include <taskPriority.h>
#include <vxlTarget.h>
#include <LtCUtil.h>
#include <VxlTypes.h>
#include <LtRouter.h>
#include <LtMisc.h>
#include <LtPlatform.h>
#include <Osal.h>
#include <VxSockets.h>

#ifdef WIN32
#include "LtIpMaster.h"
#ifdef  __cplusplus
extern "C" {
#endif
#ifdef  __cplusplus
}
#endif
#endif

/* forward declarations
 */
static void logSntpMsg(struct tm *pDateTime, const char *format, ...);
static void getTM( ULONG secs, struct tm* pdateTime );

#ifdef __VXWORKS__ /* VXWORKS DEFINITIONS */
// REMINDER - FB: I put this definition in target/include/sysLib.h,
// but that is the same name as a vxWorks header file, so it doesn't
// pick it up. Our header file name should be changed, and both these
// prototypes should be removed.
//#include <sysLib.h>
extern STATUS setRtcFromOsTime(); /* really lives in sysRtc.h in the BSP */
extern int sysClkRateGet(void);
#define STATCALL stat

static void getSystemDateTime( struct tm* pdateTime )
{
    struct timespec systemTime;

	OsalClockGetTime(CLOCK_REALTIME, &systemTime);
	gmtime_r(&systemTime.tv_sec, pdateTime);
}

static void getTM( ULONG secs, struct tm* pdateTime )
{
	time_t		tt;

	tt = (time_t)secs;

	gmtime_r(&tt, pdateTime);
}


/* Linux DEFINITIONS */
#elif defined(linux)
#define STATCALL stat
#define INET6_ADDR_LEN 46

#ifdef ILON_PLATFORM
extern "C" void sysGetNonvolSntpLogEnable(BOOL* sntpLoggingEnabled);
extern "C" void sysPutNonvolSntpLogEnable(BOOL sntpLoggingEnabled);
extern "C" void sysSetSystemClock(struct tm *tmPtr);
#endif

#ifndef strncpy2
// strncpy2 get defined elsewhere for iLON and Windows simulation, but not for VNI
#define strncpy2 strncpy
#endif

static void getSystemDateTime( struct tm* pdateTime )
{
	struct tm* pTm;
	time_t		tt;

	time( &tt );

	pTm = gmtime( &tt );
	*pdateTime = *pTm;
}

static void getTM( ULONG secs, struct tm* pdateTime )
{
	struct tm* pTm;
	time_t		tt;

	tt = (time_t)secs;

	pTm = gmtime( &tt );
    if (pTm != NULL)
    {
	    *pdateTime = *pTm;
    }
    else
    {
        memset(pdateTime, 0, sizeof(*pdateTime));
    }
}

#else /* WINDOWS DEFINITIONS */
#define STATCALL _stat
#define INET6_ADDR_LEN 46

#ifndef strncpy2
// strncpy2 get defined elsewhere for iLON and Windows simulation, but not for VNI
#define strncpy2 strncpy
#endif

static void getSystemDateTime( struct tm* pdateTime )
{
    timespec t;
    getCurTimespec(&t);
    getTM((time_t)t.tv_sec, pdateTime);
}

static void getTM( ULONG secs, struct tm* pdateTime )
{
	struct tm* pTm;
	time_t		tt;

	tt = (time_t)secs;

	pTm = gmtime( &tt );
    if (pTm != NULL)
    {
	    *pdateTime = *pTm;
    }
    else
    {
        memset(pdateTime, 0, sizeof(*pdateTime));
    }
}

/* Change form of network address string to vxWorks form
 */
#define INET6_ADDR_LEN 46

#endif /* END OF DEFINITIONS */

#ifdef INET6
static BOOL IsSockAddrEmpty(struct sockaddr_in6* ipAddr)
{
	struct sockaddr_in6 emptySockAddr;

	memset(&emptySockAddr, 0, sizeof(struct sockaddr_in6));
	return (memcmp(ipAddr, &emptySockAddr, sizeof(struct sockaddr_in6)) == 0);
}

static void SetSockAddrEmpty(struct sockaddr_in6* ipAddr)
{
	memset(ipAddr, 0, sizeof(struct sockaddr_in6));
}
#else
static BOOL IsSockAddrEmpty(unsigned long *pIpAddr)
{
	return *pIpAddr == 0;
}

static BOOL IsSockAddrEmpty(int *pIpAddr)
{
	return *pIpAddr <= 0;
}

static void SetSockAddrEmpty(unsigned long *pIpAddr)
{
	*pIpAddr = 0;
}
#endif

#ifndef INET6
static void inet_ntoa_b( struct in_addr ina, char* szStr )
{
	char* pDotted;
	struct in_addr inh;
	inh.s_addr = ntohl( ina.s_addr );

    pDotted = inet_ntoa( inh );
	if ( pDotted )
	{	strcpy( szStr, pDotted );
	}
	else
	{	szStr[0] = 0;
	}
}
#endif

#if !defined(NDEBUG) && !defined(DEBUG)
#define DEBUG
#endif


SEM_ID iLonSntpSem = NULL;		/* semaphore for SNTP client task */
int iLonSntpTid = -1;
int iLonSntpUpdateIntervalSecs = ILON_SNTP_UPDATE_INTERVAL_AUTO;

/* The SNTP target server list */
/* Initially all zeros = no servers configured */
ILonSntpServer iLonSntpServer[NUM_SNTP_SERVERS];
static int curServerIndex = 0;
static BOOL sntpLoggingEnabled = FALSE;
static int sntpDisablingLog = FALSE;    /* to stop infinite recursion */
static uchar_t sntpSynched = FALSE;
static time_t sntpLastSynchedTime = 0; 
#define MAX_SYNC_FAILED_FOR_REPORTING	4
static int iSyncFailedCount = 0;
static int wakeupForIntervalChange = FALSE;
static BOOL sntpTaskRunning = FALSE;
static BOOL exitSntpTask = FALSE;

/* Forward declarations */
int iLonSntpTask(...);

#if defined(ILON_PLATFORM)

static void iLonSntpWriteNVRam()
{
	sysPutNonvolSntpLogEnable(sntpLoggingEnabled);
}

static void iLonSntpReadNVRam()
{
	sysGetNonvolSntpLogEnable(&sntpLoggingEnabled);
}
#else
/* No such thing as NV ram on Windows */
static void iLonSntpReadNVRam()
{	/* logging always enabled for windows */
	sntpLoggingEnabled = TRUE;
}

#endif /* WIN32 */


int IncrementSyncFailedCount()
{
	if (++iSyncFailedCount >= MAX_SYNC_FAILED_FOR_REPORTING)
		sntpSynched = FALSE;
	return iSyncFailedCount;
}

int GetSyncFailedCount()
{
	return iSyncFailedCount;
}

void SetSyncFailedCount(int iCount)
{
	iSyncFailedCount = iCount;
	if (iCount >= MAX_SYNC_FAILED_FOR_REPORTING)
		sntpSynched = FALSE;
	else
		sntpSynched = TRUE;
}

void SyncAndClearFailedCount()
{
	iSyncFailedCount = 0;
	sntpSynched = TRUE;
}

void NotSyncAndClearFailedCount()
{
	iSyncFailedCount = 0;
	sntpSynched = FALSE;
}

static void formatTm(tm *pDateTime, char *buf)
{
    sprintf(buf, "%d/%d/%d %02d:%02d:%02d", 
			pDateTime->tm_mon+1, pDateTime->tm_mday, pDateTime->tm_year+1900,
			pDateTime->tm_hour, pDateTime->tm_min, pDateTime->tm_sec);
}

static void formatSeconds(time_t sec, char *buf)
{
    tm dateTime;
    getTM(sec, &dateTime);
    formatTm(&dateTime, buf);
}

/* Log a message, optionally prefixed with date/time
 * 
 * On windows, just printf the log entry
 */
static void logSntpMsg(struct tm *pDateTime, const char *format, ...)
{
    va_list argInfo;
	char buf1[150];
	char buf2[200];
#ifndef WIN32
	FILE *fp;
	struct stat		fst;
#endif /* WIN32 */

	if (sntpLoggingEnabled)
	{
		buf2[0] = 0;
		if (pDateTime != NULL)
		{
            formatTm(pDateTime, buf2);
            strcat(buf2, " - ");
		}
		va_start(argInfo, format);
		vsprintf(buf1, format, argInfo);
		va_end(argInfo);
		strcat(buf2, buf1);
#ifndef WIN32
#ifdef linux
		CreateDirectoryTree(ILON_SNTP_LOG_FILE_FOLDER);
#endif
		if ((fp = fopen(ILON_SNTP_LOG_FILE_PATH, "a")) != NULL)
		{
			fprintf(fp, "%s", buf2);
			fclose(fp);
			if (!sntpDisablingLog && (OK == STATCALL( ILON_SNTP_LOG_FILE_PATH, &fst ) ))
			{
				if ( fst.st_size > ILON_SNTP_LOG_FILE_MAXSIZE )
				{   
                    /* protect against infinite recursion */
                    sntpDisablingLog = TRUE;
                    printf("SNTP log file %s has reached the maximum size\n", ILON_SNTP_LOG_FILE_PATH);
                    logSntpMsg(NULL, "Log file has reached the maximum size\n");
                    iLonSntpDisableLog();
                    sntpDisablingLog = FALSE;
				}
			}
		}
//#ifdef DEBUG
		printf("%s", buf2);
//#endif /* DEBUG */
#else /* WIN32 */
		vxlReportEvent(buf2);
#endif /* WIN32 */
	}
}

void iLonSntpEnableLog(void)
{
#ifdef ILON_PLATFORM
    struct tm dateTime;

	sntpLoggingEnabled = TRUE;
    sntpDisablingLog = FALSE;
	iLonSntpWriteNVRam();
	printf("Enabling SNTP logging\n");
    printf("Log file is %s\n", ILON_SNTP_LOG_FILE_PATH);
	getSystemDateTime( &dateTime );
	logSntpMsg(&dateTime, "Enabling SNTP logging\n");
    logSntpMsg(NULL, "NOTE: Times shown are UTC, not local time\n");
#endif
}

void iLonSntpDisableLog(void)
{
#ifdef ILON_PLATFORM
    struct tm dateTime;

    printf("Disabling SNTP logging\n");
    printf("Log file is %s\n", ILON_SNTP_LOG_FILE_PATH);
	getSystemDateTime( &dateTime );
	logSntpMsg(&dateTime, "Disabling SNTP logging\n");
	sntpLoggingEnabled = FALSE;
	iLonSntpWriteNVRam();
#endif
}

int iLonSntpLoggingEnabled(void)
{
    return(sntpLoggingEnabled);
}

void sntpShow(void)
{
	int i;
	char addrStr[INET6_ADDR_LEN];
	char buf[64];

	i = GetSyncFailedCount();
	sprintf(buf, " (current SyncFailedCount: %d)", i);
	printf("SNTP synched: %s%s\n", (iLonSntpTimeSynched() ? "Yes" : "No"), (i == 0) ? "" : buf );
	printf("Current server index: %d\n", curServerIndex);
	printf("Server Table:\n");
	printf("Index\tPort\tAddress\t\tHost\n");
	for (i = 0; i < NUM_SNTP_SERVERS; i++)
	{
#ifdef INET6
		inet_ntop(AF_INET6, &(iLonSntpServer[i].ipAddr.sin6_addr), addrStr, INET6_ADDR_LEN);
#else
		struct in_addr inAddr;
		inAddr.s_addr = iLonSntpServer[i].ipAddr;
		inet_ntoa_b(inAddr, addrStr);
#endif
		printf("%d\t%d\t%-15s\t%s\n",i, iLonSntpServer[i].ipPort, addrStr, iLonSntpServer[i].host);
	}
}


/* Start up the SNTP client task */
STATUS iLonSntpInit(void)
{
	STATUS sts = OK;

	// initialize array
	memset(iLonSntpServer, 0, sizeof(iLonSntpServer));

	/* restore logging setting */
	iLonSntpReadNVRam();
	if (iLonSntpSem == NULL)
	{
		sts = ERROR;
		iLonSntpSem = semBCreate( SEM_Q_FIFO, SEM_EMPTY );
		if (iLonSntpSem != NULL)
		{
#ifdef __VXWORKS__
			sts = sntpcInit (SNTP_PORT, NULL); // don't bother specifying the network interface, we fix it up in the lib call
#else
			sts = sntpcInit (SNTP_PORT);
#endif
			if ((sts == OK) && (iLonSntpTid == -1))
			{
				/* Start the task */
                sntpTaskRunning = TRUE;
                exitSntpTask = FALSE;
				iLonSntpTid = taskSpawn( "tIlonSntp", ILON_SNTP_TASK_PRIORITY, 0, 8*1024,
										iLonSntpTask, 0,0,0,0,0,0,0,0,0,0);
				if (iLonSntpTid == ERROR)
				{
					sts = ERROR;
				}
			}
		}
	}

	if (sts != OK)
	{
		/* Clean up after failure */
		iLonSntpDestroy();
		logSntpMsg(NULL, "SNTP init failed\n");
		printf("SNTP init failed\n");
	}

	return(sts);
}


/* Shut down the SNTP client task. */
void iLonSntpDestroy(void)
{
	if (iLonSntpTid != -1)
	{
        int i;
        exitSntpTask = TRUE;
        for (i = 0; sntpTaskRunning && i < 10; i++)
        {
	        if (iLonSntpSem != NULL)
            {
                semGive(iLonSntpSem);
            }
            taskDelay(msToTicks(100));
        }
        if (sntpTaskRunning)
        {   /* Timed out - just delete the task and hope for the best. */
#if !PRODUCT_IS(FTXL) && !PRODUCT_IS(LONTALK_STACK) && !PRODUCT_IS(IZOT)
            taskDelete(	iLonSntpTid );
#endif
            sntpTaskRunning = FALSE;
        }
		iLonSntpTid = -1;
	}

	if (iLonSntpSem != NULL)
	{
		semDelete( iLonSntpSem );
		iLonSntpSem = NULL;
	}
    NotSyncAndClearFailedCount();
}

#ifdef WIN32
LTA_EXTERNAL_FN void ilonSntpShutdown(void)
{
    iLonSntpDestroy();
    SntpServerList::shutdown();
}
#endif

/* Set the update interval. Allow any value. Special values disable it or use adaptive algorithm */
void iLonSntpSetUpdateInterval(ulong_t interval)
{
    struct tm dateTime;

	getSystemDateTime( &dateTime );
	logSntpMsg(&dateTime, "Set SNTP update interval to %ld\n", interval);
	iLonSntpUpdateIntervalSecs = interval;
	wakeupForIntervalChange = TRUE;
	/* Wake up the task to reset the interval */
	if (iLonSntpSem != NULL)
	{
		semGive(iLonSntpSem);
	}

}
ulong_t iLonSntpGetUpdateInterval()
{
	return(iLonSntpUpdateIntervalSecs);
}

/* Update the time immediately */
STATUS iLonSntpUpdateNow()
{
    STATUS sts = ERROR;
    struct tm dateTime;

	getSystemDateTime( &dateTime );
	logSntpMsg(&dateTime, "Triggered update\n");
	wakeupForIntervalChange = FALSE;
	/* Wake up the task */
	if ((iLonSntpSem != NULL) && (semGive( iLonSntpSem ) == OK))
	{
		sts = OK;
	}
	return(sts);
}

/* Set the values for one of the target SNTP servers. */
/* Optionally, wake up the SNTP client task. */
#ifdef INET6
STATUS iLonSntpSetServer(int index, struct sockaddr_in6* ipAddr, ushort_t ipPort, uchar_t wakeup)
#else
STATUS iLonSntpSetServer(int index, ulong_t ipAddr, ushort_t ipPort, uchar_t wakeup)
#endif
{
    STATUS sts = OK;
    struct tm dateTime;

	if (index < NUM_SNTP_SERVERS)
    {
#ifdef INET6
		iLonSntpServer[index].ipAddr = *ipAddr;
		inet_ntop(AF_INET6, &(ipAddr->sin6_addr),
				iLonSntpServer[index].host, sizeof(iLonSntpServer[index].host));
#else
		struct in_addr inetAddr;
		iLonSntpServer[index].ipAddr = ipAddr;
		inetAddr.s_addr = ipAddr;
		inet_ntoa_b(inetAddr, iLonSntpServer[index].host);
#endif
		iLonSntpServer[index].ipPort = ipPort;
		getSystemDateTime( &dateTime );
		logSntpMsg(&dateTime, "Set server %d to %s\n", index+1, iLonSntpServer[index].host);
		if (wakeup)
		{
			sts = iLonSntpUpdateNow();
		}
	}
	else
	{
		sts = ERROR;
	}
	return(sts);
}

#ifdef INET6
static BOOL iLonSntpGetHostAddr(char *host, struct sockaddr_in6* ipAddr)
{
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_V4MAPPED;
    hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;

	if (inet_pton(AF_INET6, host, ipAddr) == 0)
	{
		if (getaddrinfo(host, NULL, &hints, &res) == 0)
		{
			do
			{
				if (res->ai_family == AF_INET6)
				{
					memcpy(ipAddr, (struct sockaddr_in6*) res->ai_addr, sizeof(struct sockaddr_in6));
					// No interface scope, currently
					//ifNdx = (((struct sockaddr_in6 *) (res->ai_addr))->sin6_scope_id);
					return TRUE;
				}
				res = res->ai_next;
			} while (res);
		}
	}
	else
	{
		return TRUE;
	}

	return FALSE;
}
#else
static int iLonSntpGetHostAddr(char *host)
{
	int ipAddr;

	ipAddr = inet_addr(host);
	if ((STATUS)ipAddr == ERROR)
	{
	    ipAddr = hostGetByName(host);
	}
    // Swap from little endian to big endian and return...
	return htonl(ipAddr);
}
#endif

/* Set a server using a host name */
STATUS iLonSntpSetServerName(int index, const char *host, ushort_t ipPort, uchar_t wakeup)
{
    STATUS sts = OK;
    struct tm dateTime;

	if (index < NUM_SNTP_SERVERS)
    {
		/* Don't reset translation if the name is the same */
		if (strcmp(iLonSntpServer[index].host, host) != 0)
		{
			strncpy2(iLonSntpServer[index].host, host, sizeof(iLonSntpServer[index].host));
			/* Don't translate host name (right now, at least) */
			SetSockAddrEmpty(&iLonSntpServer[index].ipAddr);
		}
		iLonSntpServer[index].ipPort = ipPort;

		getSystemDateTime( &dateTime );
		logSntpMsg(&dateTime, "Set server %d to %s\n", index+1, iLonSntpServer[index].host);
		if (wakeup)
		{
			sts = iLonSntpUpdateNow();
		}
	}
	else
	{
		sts = ERROR;
	}
	return(sts);
}

#ifdef INET6
STATUS iLonSntpGetServer(int index, struct sockaddr_in6 *ipAddr, ushort_t *ipPort)
#else
STATUS iLonSntpGetServer(int index, ulong_t *ipAddr, ushort_t *ipPort)
#endif
{
    STATUS sts = ERROR;

	if (index < NUM_SNTP_SERVERS)
    {
		*ipAddr = iLonSntpServer[index].ipAddr;
		*ipPort = iLonSntpServer[index].ipPort;
        sts = OK;
    }
    return(sts);
}

STATUS iLonSntpGetServerName(int index, char **host, ushort_t *ipPort)
{
    STATUS sts = ERROR;

	if (index < NUM_SNTP_SERVERS)
    {
		*host = iLonSntpServer[index].host;
		*ipPort = iLonSntpServer[index].ipPort;
        sts = OK;
    }
    return(sts);
}

/* Determine if time has been synched */
uchar_t iLonSntpTimeSynched(void)
{
	return sntpSynched;
}

/* Return the last time SNTP synched (in local time) */
ulong_t iLonSntpTimeLastSynched(void)
{
	return(sntpLastSynchedTime);
}

int iLonSntpTask(...)
{
	time_t adaptiveTimeout = 0; /* in seconds */
	int timeoutTicks;
	struct timespec sntpTime;
    struct timespec systemTime;
	int secDiff;
	int msecDiff;
    unsigned char diffSign;
	int updateRtc = TRUE;
	time_t lastRtcUpdate = 0;
	char serverString[INET6_ADDR_LEN];
	STATUS sts;
	struct tm dateTime;
    int startingUp = TRUE;
	int i;
	int hostTranslationErrors = 0;
	BOOL translateErrorThisLoop;
#ifdef INET6
	struct sockaddr_in6 ipAddr;
#else
	struct in_addr inetAddr;
	int ipAddr;
#endif
	
	curServerIndex = 0;
	while (!exitSntpTask)
	{
		do
		{
			wakeupForIntervalChange = FALSE;
			/* Decide on the appropriate timeout */
			if (((GetSyncFailedCount() > 0) && (GetSyncFailedCount() < MAX_SYNC_FAILED_FOR_REPORTING)) ||
				(hostTranslationErrors > 0))
			{
				/* Rapid retry for the first few failures */
				timeoutTicks = ILON_SNTP_ERROR_RETRY_TIMEOUT * sysClkRateGet();
			}
			else if (iLonSntpUpdateIntervalSecs == ILON_SNTP_UPDATE_INTERVAL_AUTO)
			{
				timeoutTicks = adaptiveTimeout * sysClkRateGet();
			}
			else if (iLonSntpUpdateIntervalSecs == ILON_SNTP_UPDATE_INTERVAL_NONE)
			{
				timeoutTicks = WAIT_FOREVER;
			}
			else
			{
				timeoutTicks = iLonSntpUpdateIntervalSecs * sysClkRateGet();
			}
			/* See if any servers are configured */
			for	(i = 0; i < NUM_SNTP_SERVERS; i++)
			{
				if (!IsSockAddrEmpty(&iLonSntpServer[i].ipAddr))
				{
					break;
				}

				if((iLonSntpServer[i].host[0] != 0) && (strcmp(iLonSntpServer[i].host, "0.0.0.0") != 0))
				{
					break;
				}
			}
			if (i >= NUM_SNTP_SERVERS)
			{
				/* No servers, wait until one is configured */
				timeoutTicks = 60 * 60 * sysClkRateGet(); // One hour
			}
			/* Try to take the semaphore. This will normally just time out, */
			/* unless the semaphore is given by iLonSntpSetServer(). */
			/* If no servers are configured, it will not time out. */
			sts = semTake(iLonSntpSem, timeoutTicks);

// todo - comment out for e5, as we don't know how to access the RTC yet.
/*
			if (i >= NUM_SNTP_SERVERS)
			{
			  if(S_objLib_OBJ_TIMEOUT  == errno)
			  {
			    // Syncs RTC -> System clock
				getSystemDateTime( &dateTime );
				logSntpMsg(&dateTime, "Sync RTC to system clock\n");
                setSystemClock( NULL );
			  }
            }
*/
		} while (wakeupForIntervalChange && !exitSntpTask);

        if (exitSntpTask)
        {
            sntpTaskRunning = FALSE;
        	return(OK);
        }

		/* If the current server is empty, try the next one */
		translateErrorThisLoop = FALSE;
		for	(i = 0; i < NUM_SNTP_SERVERS; i++)
		{
			/* First try translating the host name, even if it's just numeric */
#ifdef INET6
			if (!iLonSntpGetHostAddr(iLonSntpServer[curServerIndex].host, &ipAddr))
#else
			ipAddr = iLonSntpGetHostAddr(iLonSntpServer[curServerIndex].host);
			if (ipAddr == ERROR)
#endif
			{
				getSystemDateTime( &dateTime );
				logSntpMsg(&dateTime, "Failed to get address for server %s\n", iLonSntpServer[curServerIndex].host);

				if (!translateErrorThisLoop)	// only bump this once per wakeup loop
				{
					++hostTranslationErrors;
					translateErrorThisLoop = TRUE;
				}
				if (hostTranslationErrors > 2)
				{
					SetSockAddrEmpty(&iLonSntpServer[curServerIndex].ipAddr);
				}
			}
			else
			{
				/* assign even if just a numeric host */
				iLonSntpServer[curServerIndex].ipAddr = ipAddr;
				if (!IsSockAddrEmpty(&ipAddr))
				{
					hostTranslationErrors = 0;
				}
			}
			if (IsSockAddrEmpty(&iLonSntpServer[curServerIndex].ipAddr) ||
					((i == 0) && ((hostTranslationErrors > 0) )))
				{
				curServerIndex++;
				if (curServerIndex >= NUM_SNTP_SERVERS)
				{
					curServerIndex = 0;
				}
			}
			else
				break;
		}
		if (hostTranslationErrors > 2)
		{
			hostTranslationErrors = 0;	// Don't trigger fast retries again because of this
		}

		/* If we have a server, query it for the time */
		if (IsSockAddrEmpty(&iLonSntpServer[curServerIndex].ipAddr))
        {
            /* No server, no sync */
            if (sntpSynched && !startingUp)
            {
                /* We are losing sync, report it */
                vxlReportUrgent("Time synchronization disabled, no server\n");
				SetSyncFailedCount(MAX_SYNC_FAILED_FOR_REPORTING);
            }
        }
        else
		{
			/* Get a numeric string for this address. */
			/* It still may not be valid. */
#ifdef INET6
			inet_ntop(AF_INET6, &(iLonSntpServer[curServerIndex].ipAddr.sin6_addr), serverString, sizeof(serverString));
#else
			inetAddr.s_addr = iLonSntpServer[curServerIndex].ipAddr;
			inet_ntoa_b(inetAddr, serverString);
#endif

			#define MAX_SNTP_DELTA_TIME 3600*24 /* 1 day */
			#define MAX_DELTA_TIME_NUMBER 4

            struct timespec l_Now;

            if(OK == OsalClockGetTime(CLOCK_REALTIME, &l_Now))
			{
			  int l_iCounter = 0;
		      for(l_iCounter = 0; MAX_DELTA_TIME_NUMBER > l_iCounter; l_iCounter++)
			  {
#ifdef INET6
				inet_ntop(AF_INET6, &(iLonSntpServer[curServerIndex].ipAddr.sin6_addr), serverString, sizeof(serverString));
				sts = sntpcTimeGet (&(iLonSntpServer[curServerIndex].ipAddr), (ILON_SNTP_SERVER_TIMEOUT * sysClkRateGet()), &sntpTime);
#else
			    sts = sntpcTimeGet (serverString, (ILON_SNTP_SERVER_TIMEOUT * sysClkRateGet()), &sntpTime);
#endif
				if(ERROR == sts)
				{
					break;
			    }

				int l_iDeltaTime = l_Now.tv_sec - sntpTime.tv_sec;

				if(MAX_SNTP_DELTA_TIME > abs(l_iDeltaTime))
				{
				  break;
				}
			  }
			}
			else
			{
			  sts = ERROR;
			}

			if (sts != OK)
			{
				/* We don't care why it failed */
				int tempServerIndex;
				getSystemDateTime( &dateTime );
				logSntpMsg(&dateTime, "server %s (index %d), failed to get time\n",
								serverString, curServerIndex);
				IncrementSyncFailedCount();
                if (GetSyncFailedCount() == MAX_SYNC_FAILED_FOR_REPORTING)
                {
                    /* We are losing/failing sync, report it (even on startup) */
	                    vxlReportUrgent("Time synchronization failed, server: %s\n", serverString);
                }
				adaptiveTimeout = ILON_SNTP_MIN_ADAPTIVE_TIMEOUT;
				/* Switch server, if possible */
				tempServerIndex = curServerIndex + 1;
				if (tempServerIndex >= NUM_SNTP_SERVERS)
				{
					tempServerIndex = 0;
				}
				if (!IsSockAddrEmpty(&iLonSntpServer[tempServerIndex].ipAddr))
				{
				  curServerIndex = tempServerIndex;
				}
				else
				{
#ifdef INET6
					if (iLonSntpGetHostAddr(iLonSntpServer[tempServerIndex].host, &ipAddr))
#else
					ipAddr = iLonSntpGetHostAddr(iLonSntpServer[tempServerIndex].host);
					if (ipAddr != ERROR)
#endif
					{
						curServerIndex = tempServerIndex;
					}
				}
			}
			else
			{
				/* First, get the current system time */
				sts = OsalClockGetTime(CLOCK_REALTIME, &systemTime);
				/* Next, adjust the system time */
				sts = OsalClockSetTime(CLOCK_REALTIME, &sntpTime);

				if ( sts == OK)
				{
					/* If we are synching up after being out-of-sync, update the RTC */
					if (!sntpSynched)
					{
						updateRtc = TRUE;
                        if (!startingUp)
                        {
                            /* We are regaining sync, report it */
			                vxlReportUrgent("Time synchronization established, server: %s\n",
                                            serverString);
                        }
					}
					SyncAndClearFailedCount();
					sntpLastSynchedTime = time(NULL);
				}
				else
				{
					logSntpMsg(NULL, "Setting system time failed [%d]\n", sts);
				}

				if (sts)
				{
					getTM(sntpTime.tv_sec, &dateTime);
				}
				else
				{	/* getTM(&systemTime.tv_sec, &dateTime); */
					getSystemDateTime( &dateTime );
				}


				/* Next, determine the correction factor. */
				/* Use addaptive algorithm to adjust the query delay. */
				if (sts == OK)
				{
					secDiff = sntpTime.tv_sec - systemTime.tv_sec;
					msecDiff = (sntpTime.tv_nsec - systemTime.tv_nsec) / 1000000;
					/* If the signedness is different, normalize by one second */
					if (((secDiff < 0) != (msecDiff < 0)) && (secDiff != 0) && (msecDiff != 0))
					{
						if (secDiff < 0)
						{
							secDiff++;
							msecDiff -= 1000;
						}
						else
						{
							secDiff--;
							msecDiff += 1000;
						}
					}
					/* Get the sign of the total difference */
                    diffSign = ((secDiff < 0) || (msecDiff < 0));
                    /* Get the magnitudes of the difference values */
					if (secDiff < 0) secDiff *= -1;
					if (msecDiff < 0) msecDiff *= -1;

					logSntpMsg(&dateTime, "server %s, adjust by %c%d.%03d sec\n",
								serverString,
								(diffSign ? '-' : '+'), secDiff, msecDiff);
#ifdef WIN32

	                struct _timeb windowsTmb;
		            _ftime( &windowsTmb );
                    char windowsTime[100];
                    formatSeconds(windowsTmb.time, windowsTime);
                    __int64 absSysOffset;
                    char *sign;
                    if (systemTimeOffset < 0)
                    {
                        sign = "-";
                        absSysOffset = -systemTimeOffset;
                    }
                    else
                    {
                        sign = "";
                        absSysOffset = systemTimeOffset;
                    }
                    logSntpMsg(&dateTime, "Windows time is %s, offset is %s%I64d.%.3d seconds\n", windowsTime, sign, absSysOffset/1000, absSysOffset%1000);

#endif

#ifdef SNTP_LIB_DEBUG
                    logSntpMsg(NULL, "sysTime: %d sec %d nsec, sntpTime: %d sec %d nsec\n",
                                 systemTime.tv_sec, systemTime.tv_nsec, sntpTime.tv_sec, sntpTime.tv_nsec);
                    logSntpMsg(NULL, "T1: %d.%d, T4: %d.%d\n", T1.tv_sec, T1.tv_nsec, T4.tv_sec, T4.tv_nsec);
                    logSntpMsg(NULL, "T2: %d.%d, T3: %d.%d\n", T2.tv_sec, T2.tv_nsec, T3.tv_sec, T3.tv_nsec);
                    logSntpMsg(NULL, "TDiff1: %d.%d, TDiff2: %d.%d\n", TDiff1.tv_sec, TDiff1.tv_nsec, TDiff2.tv_sec, TDiff2.tv_nsec);
                    logSntpMsg(NULL, "TSum1: %d.%d\n", TSum1.tv_sec, TSum1.tv_nsec);
#endif

					/* Now get the magnitude of the sec difference */
					if (secDiff < 0) secDiff *= -1;
					/* Just in case it's large, scale it down */
					if (secDiff > 1000) secDiff = 1000;
					/* Now get the absolute msec difference */
					msecDiff += secDiff * 1000;
					/* Calculate new timeout based on the drift */
					/* Make sure we don't divide by zero */
					if (msecDiff == 0)
					{
						msecDiff = 1;
					}
					/* Limit the increase to one minute */
					adaptiveTimeout = min((adaptiveTimeout * ILON_SNTP_MSEC_DIFF_TARGET / msecDiff),
									(adaptiveTimeout + ILON_SNTP_MAX_ADAPTIVE_TIMEOUT_INCREASE));
				}
				else
				{
					logSntpMsg(&dateTime, "server %s, unable to determine correction\n", serverString );
				}

				/* Finally, update the RTC, if appropriate */
				if ((sntpTime.tv_sec - lastRtcUpdate) > ILON_SNTP_RTC_UPDATE_INTERVAL)
				{
					updateRtc = TRUE;
				}
#if defined(ILON_PLATFORM) /* No separate update of RTC on windows */
				if (updateRtc)
				{
					setRtcFromOsTime();
					lastRtcUpdate = sntpTime.tv_sec;
					updateRtc = FALSE;
					getTM(sntpTime.tv_sec, &dateTime);
					logSntpMsg(&dateTime, "updated the RTC\n");
				}
#endif /* WIN32 */
			}
		}

		/* Enforce limits on the delay */
		if (adaptiveTimeout < ILON_SNTP_MIN_ADAPTIVE_TIMEOUT)
		{
			adaptiveTimeout = ILON_SNTP_MIN_ADAPTIVE_TIMEOUT;
		}
		else if (adaptiveTimeout > ILON_SNTP_MAX_ADAPTIVE_TIMEOUT)
		{
			adaptiveTimeout = ILON_SNTP_MAX_ADAPTIVE_TIMEOUT;
		}

        startingUp = FALSE;
	}
    sntpTaskRunning = FALSE;
	return(OK);
}


void iLonSntpSetDefault(void)
{
#ifdef INET6
	iLonSntpSetServerName(0, "91.103.24.10", 123, 1);
#else
	iLonSntpSetServerName(0, "91.103.24.10", 123, 1);
	//	iLonSntpSetServerName(0, "tock.usask.ca", 123, 1);
//	iLonSntpSetServer(0,0x0a010015,123,1);
#endif
}

#ifdef WIN32
/* Reset the time correction.  This can be used after disabling SNTP.  For example,
 * if the user had been using the VNI SNTP, and then disables it in favor of
 * a Windows SNTP, you would want to clear the offset, since the windows system time
 * is presumably going to be kept in synch.
 */
void ilonSntpResetOffset(void)
{
    systemTimeOffset = 0;
}
#endif
