/*
 * ech_logger.h
 *
 * Copyright © 2010-2022 Dialog Semiconductor
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
 * This header file contains Echelon Logger definitions/API.
 */

#ifndef ECH_LOGGER_H_
#define ECH_LOGGER_H_

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>

#include "EchelonStandardDefinitions.h"

// #define USE_NON_OSAL_TICK_COUNT
#ifdef USE_NON_OSAL_TICK_COUNT
  typedef struct timespec			LOG_TIME_SPEC;
#else
# include "Osal.h"
  typedef OsalTickCount				LOG_TIME_SPEC;
#endif

C_API_START

#define NO_FLAG			0
#define DEBUG_FLAG		1
#define PRINT_FLAG		2

#ifdef NDEBUG

#define	LogError(fmt, ...) 		_EchLog	(NO_FLAG, LOG_ERR,     fmt,__FUNCTION__,##__VA_ARGS__)
#define	LogWarning(fmt, ...)	_EchLog	(NO_FLAG, LOG_WARNING, fmt,__FUNCTION__,##__VA_ARGS__)
#define	LogInfo(fmt, ...)		_EchLog	(NO_FLAG, LOG_INFO,    fmt,__FUNCTION__,##__VA_ARGS__)
#define	LogCritical(fmt, ...)	_EchLog	(NO_FLAG, LOG_CRIT,    fmt,__FUNCTION__,##__VA_ARGS__)
#define	LogEmergency(fmt, ...)	_EchLog	(NO_FLAG, LOG_EMERG,   fmt,__FUNCTION__,##__VA_ARGS__)
#define	LogDebug(fmt, ...)

#define	LogPError(fmt, ...) 	_EchLog	(PRINT_FLAG, LOG_ERR,     fmt,__FUNCTION__,##__VA_ARGS__)
#define	LogPWarning(fmt, ...)	_EchLog	(PRINT_FLAG, LOG_WARNING, fmt,__FUNCTION__,##__VA_ARGS__)
#define	LogPInfo(fmt, ...)		_EchLog	(PRINT_FLAG, LOG_INFO,    fmt,__FUNCTION__,##__VA_ARGS__)
#define	LogPCritical(fmt, ...)	_EchLog	(PRINT_FLAG, LOG_CRIT,    fmt,__FUNCTION__,##__VA_ARGS__)
#define	LogPEmergency(fmt, ...)	_EchLog	(PRINT_FLAG, LOG_EMERG,   fmt,__FUNCTION__,##__VA_ARGS__)
#define	LogPDebug(fmt, ...)

#else

#define	LogError(fmt, ...) 		_EchLog	(DEBUG_FLAG, LOG_ERR,     fmt,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)
#define	LogWarning(fmt, ...)	_EchLog	(DEBUG_FLAG, LOG_WARNING, fmt,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)
#define	LogInfo(fmt, ...)		_EchLog	(DEBUG_FLAG, LOG_INFO,    fmt,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)
#define	LogCritical(fmt, ...)	_EchLog (DEBUG_FLAG, LOG_CRIT,    fmt,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)
#define	LogEmergency(fmt, ...)	_EchLog	(DEBUG_FLAG, LOG_EMERG,   fmt,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)
#define	LogDebug(fmt, ...)		_EchLog	(DEBUG_FLAG, LOG_DEBUG,   fmt,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)

#define	LogPError(fmt, ...) 	_EchLog	(DEBUG_FLAG|PRINT_FLAG, LOG_ERR,     fmt,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)
#define	LogPWarning(fmt, ...)	_EchLog	(DEBUG_FLAG|PRINT_FLAG, LOG_WARNING, fmt,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)
#define	LogPInfo(fmt, ...)		_EchLog	(DEBUG_FLAG|PRINT_FLAG, LOG_INFO,    fmt,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)
#define	LogPCritical(fmt, ...)	_EchLog (DEBUG_FLAG|PRINT_FLAG, LOG_CRIT,    fmt,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)
#define	LogPEmergency(fmt, ...)	_EchLog	(DEBUG_FLAG|PRINT_FLAG, LOG_EMERG,   fmt,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)
#define	LogPDebug(fmt, ...)		_EchLog	(DEBUG_FLAG|PRINT_FLAG, LOG_DEBUG,   fmt,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)

#endif

// The following macros are used for throttling error, warning and info logs within a function.
// NO throttling macros are defined for debug, critical and emergency logs.
// InitThrottleLog must be declared at the beginning of a local function scope
// The log throttling is meant to throttle logging on a per function basis.
// All ThrottleLogXxx calls (even in a loop) within the local function will count as 1 log entry.
// Throttling (suspend logging) begins after the 10 log entries within 1 minutes but logging will occur
// on every 50th entries there after.  Throttling will end when log entry interval is longer than 1 minute.
//
// In the following Example, if foo() is called repeatedly more than 10 times per minute (i.e. 65 times),
// foo will only log the first 10 INFO, 10 ERR and 10 WARNING entries and every 50th entries there after.
// When foo is called with interval > 1 minute, the logging will resume normally.
//
//   foo()
//   {
//		InitThrottleLog();
//      ...
//		ThrottleLogInfo("Entering foo():");
//
//		ThrottleLogError("Failed to close IPC socket - %s\n", IPC_GetErrorString(retVal));
//
//		ThrottleLogWarning("Memory leak: unable to close IPC socket");
//   }
//


#ifdef USE_NON_OSAL_TICK_COUNT
#define InitThrottleLog()						static throttle_log throttle = {{0,0},0,0,0,0,0};		\
												throttle.skipCheck = FALSE;		 						\
												throttle.doLog = FALSE
#else
#define InitThrottleLog()						static throttle_log throttle = {0,0,0,0,0,0};			\
												throttle.skipCheck = FALSE;		 						\
												throttle.doLog = FALSE
#endif

#define	ThrottleLogError(fmt, ...)				{														\
													if (!throttle.skipCheck)							\
													{													\
														throttle.skipCheck = TRUE;						\
														if (!ThrottleLogging(&throttle))				\
														{												\
															if (throttle.transitionFlag)				\
															{											\
	LogError("Stop throttling log entries (missed a total of %d log entries)", throttle.missedCount); 	\
															}											\
															throttle.doLog = TRUE;						\
															LogError(fmt, ##__VA_ARGS__);				\
														}												\
														else if (throttle.transitionFlag)				\
														{												\
															LogError("Start throttling log entries"); 	\
														}												\
													}													\
													else												\
													{													\
														if (throttle.doLog)								\
															LogError(fmt, ##__VA_ARGS__);				\
													}													\
												}

#define	ThrottleLogWarning(fmt, ...)			{														\
													if (!throttle.skipCheck)							\
													{													\
														throttle.skipCheck = TRUE;						\
														if (!ThrottleLogging(&throttle))				\
														{												\
															if (throttle.transitionFlag)				\
															{											\
	LogError("Stop throttling log entries (missed a total of %d log entries)", throttle.missedCount); 	\
															}											\
															throttle.doLog = TRUE;						\
															LogWarning(fmt, ##__VA_ARGS__);				\
														}												\
														else if (throttle.transitionFlag)				\
														{												\
															LogInfo("Start throttling log entries"); 	\
														}												\
													}													\
													else												\
													{													\
														if (throttle.doLog)								\
															LogWarning(fmt, ##__VA_ARGS__);				\
													}													\
												}

#define	ThrottleLogInfo(fmt, ...)				{														\
													if (!throttle.skipCheck)							\
													{													\
														throttle.skipCheck = TRUE;						\
														if (!ThrottleLogging(&throttle))				\
														{												\
															if (throttle.transitionFlag)				\
															{											\
	LogError("Stop throttling log entries (missed a total of %d log entries)", throttle.missedCount); 	\
															}											\
															throttle.doLog = TRUE;						\
															LogInfo(fmt, ##__VA_ARGS__);				\
														}												\
														else if (throttle.transitionFlag)				\
														{												\
															LogInfo("Start throttling log entries"); 	\
														}												\
													}													\
													else												\
													{													\
														if (throttle.doLog)								\
															LogInfo(fmt, ##__VA_ARGS__);				\
													}													\
												}


typedef struct {
	LOG_TIME_SPEC prevTimeStamp;
	int    count;
	int    missedCount;				// maintain missed count before count is cleared
	int    transitionFlag;			// transition from throttling to not throttling and vice versa
	// The following two fields must be initialized using IPC_INIT_THROTTLE_LOG upon entrance of a function
	int    skipCheck;				// used to skipCheck throttling when IPC_THROTTLE_LOG_ERR is called more than once in a routine
	int    doLog;					// used in conjunction with skipCheck to call _EchLogError
} throttle_log;


/* Internal APIs, use macro definitions above instead */
void	_EchLog(unsigned int flag, int pri, const char *pFormat, ...);
int 	ThrottleLogging(throttle_log * pThrottle);

C_API_END

#endif /* ECH_LOGGER_H_ */
