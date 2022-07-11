//
// LtIpPlatform.cpp
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
// This file contains platform specific code that is of a general nature with
// a leaning towards IP related stuff although not necessarily limited to that.
//

#ifdef WIN32
#include <windows.h>
#include <direct.h>
#include <sys\types.h>
#include <stdio.h>
#else
// change case
#include <VxWorks.h>
#include <LtaDefine.h>
#if defined(__VXWORKS__)
#include <usrLib.h>
#endif
#include "LtRouter.h"
#include "LtStart.h"
//#include "httpControl.h"
#endif

#include "LtIpPlatform.h"
#include "vxlTarget.h"

#ifndef WIN32
#if defined(__VXWORKS__)
#include "echelon\ilon.h"
#endif
#include "md5.h"

#if not (defined(WIN32) || PRODUCT_IS(LONTALK_STACK) || PRODUCT_IS(LONTALK_ROUTER) || PRODUCT_IS(IZOT) || defined(BOOTROM))
#include "LicenseMgr.h"
#include "LicenseMgrP.h"
#include "LicMgrMd5.c"
#include "LicMgrHmacMd5.c"
#endif

extern "C" void iLonEventLog(const char* szEvent);
// From syseeprom.h - don't include
extern "C" unsigned int sysGetHwConfiguration(void);
#endif

#include "LtRouter.h"
#include "LtPlatform.h"

void LtIpEventLog(const char* pMsg)
{
// EPANG TODO - event log not supported in i.LON Linux yet
#if defined(__VXWORKS__)
	iLonEventLog(pMsg);
#endif
}

// This function is used only for reboot requests coming from
// the LonWorks/IP config server
boolean LtIpReboot()
{
	// EPANG TODO - event log not supported in i.LON Linux yet
#if !defined(__VXWORKS__)
	// Nothing implemented here.
	return false;
#else
	// This call checks if remote reboot is allowed
	return sysDoRemoteReboot("Configuration Server Initiated Reboot.\n", TRUE);

#endif
}

boolean LtIpRebootAllowed()
{
// EPANG TODO - configuration data not supported in i.LON Linux yet
#if !defined(__VXWORKS__)
	// Nothing implemented here.
	return false;
#else
	return sysRemoteRebootAllowed();
#endif
}

boolean LtIpStartWebServer()
{
	// Start/stop of webserver/dataserver no longer supported
	return false;
}

boolean LtIpStopWebServer()
{
	// Start/stop of webserver/dataserver no longer supported
	return false;
}


void LtGetTimeZone( char* szTimeZone )
{
// EPANG TODO - time zone string not supported in i.LON Linux yet
#if !defined(__VXWORKS__)
	// no string returned
	szTimeZone[0] = 0;
#else
	getTimezoneString( szTimeZone );
#endif
}


void LtSetTimeZone( char* szTimeZone )
{
// EPANG TODO - time zone string not supported in i.LON Linux yet
#if !defined(__VXWORKS__)
#else
	setTimezoneFromString( szTimeZone );
#endif
}

void LtIpApplicationsStarted()
{
}

#if defined(WIN32) || PRODUCT_IS(LONTALK_STACK) || PRODUCT_IS(LONTALK_ROUTER) || PRODUCT_IS(IZOT)
// For real iLONs, these routines are defined in the Tornado project code

#ifdef ILON_PLATFORM
// Pick whatever iLON type you wish to emulate.
// Make sure these macros match.
#define EMULATED_ILON_PLATFORM ILON_100_PLATFORM
#define ILON100
#endif
// Include this after setting the platform type above
#include "PlatformVersion.h"

// This indicates the type of hardware the core software was built for
// Note: this name is obscured for security
int GET_SW_PLATFORM_TYPE()
{
#if defined(ILON_PLATFORM)
	return(EMULATED_ILON_PLATFORM);

#elif defined(CNFGSRVR_BUILD)
	return(CNFG_SRVR_PLATFORM);
#else
	return(VNI_PLATFORM);
#endif
}

// This indicates the type of hardware we are actually running on
// Note: this name is obscured for security
int GET_HW_PLATFORM_TYPE()
{
#if defined(ILON_PLATFORM)
	return(EMULATED_ILON_PLATFORM);

#elif defined(CNFGSRVR_BUILD)
	return(CNFG_SRVR_PLATFORM);
#else
	return(VNI_PLATFORM);
#endif
}

void getSoftwareVersionNum(int *major, int *minor, int *build)
{
	*major = VER_MAJOR_D;
	*minor = VER_MINOR_D;
	*build = VER_BUILD_D;
}

const char *getSoftwareVersionNumStr()
{
	// Return string constant in form of "M.mm.bb"
	return(VER_RES2);
}

const char *getSoftwareProductStr()
{
	// Return string constant of product name ("i.LON xxx" or "VNISTACK")
	return(PRODUCT_NAME);
}

#endif // WIN32

// this routine can be used for both hardware and software platform determinations
static boolean LtSwHwPlatformIsType(int platform, int type)
{
	boolean match;

	if (type < PLATFORM_FAMILIES)
	{
		match = (type == platform);
	}
	else
	{
		// Platform family determination
		match = FALSE;
		if ((type == ILON_ANY_PLATFORM) &&
			(platform >= ILON_PLATFORMS_START) &&
			(platform <= ILON_PLATFORMS_END))
		{
			// Any type of iLON
			match = TRUE;
		}
		else if ((type == ILON_100ANY_PLATFORM) &&
					((platform == ILON_100_PLATFORM) ||
					(platform == ILON_100DP_PLATFORM)))
		{
			// Any variety of iLON 100
			match = TRUE;
		}
		else if ((type == ILON_XX00_PLATFORM) &&
					((platform == ILON_1000_PLATFORM) ||
					(platform == ILON_1500_PLATFORM) ||
					(platform == ILON_2000_PLATFORM)))
		{
			// Any of the xx00 model iLONs
			match = TRUE;
		}
	}
	return(match);
}

// Note: this name is obscured for security
boolean SW_PLATFORM_IS_TYPE(int type)
{
	return(LtSwHwPlatformIsType(GET_SW_PLATFORM_TYPE(), type));
}

// Note: this name is obscured for security
boolean HW_PLATFORM_IS_TYPE(int type)
{
	return(LtSwHwPlatformIsType(GET_HW_PLATFORM_TYPE(), type));
}

// Note: this name is obscured for security
int GET_HW_PLATFORM_VERSION()
{
#if defined(ILON_PLATFORM) && !defined(WIN32)
	return sysGetHwConfiguration();
#else
	return ILON_HW_VERSION_E4;
#endif
}


// Holds router enable value
// Note: these names are obscured for security
static int ROUTER_LICENSED_FLAG;
static boolean ROUTER_LICENSE_CHECKED = FALSE;
#ifndef WIN32
#define MD5_DIGEST_LEN 16

// The secret license key for the router - 16 hex bytes
// 2AA7856DD160EF969B49BEC5B9C8CE3D
static unsigned char ROUTER_KEY_PART_1[MD5_DIGEST_LEN/2] = {0x9B,0x49,0xBE,0xC5,0xB9,0xC8,0xCE,0x3D};
#endif

#if defined(WIN32) || PRODUCT_IS(LONTALK_STACK) || PRODUCT_IS(LONTALK_ROUTER) || PRODUCT_IS(IZOT)
// Override for Windows simulations (e.g. iLON 100)
static boolean routerLicenseSuppressed = FALSE;
void suppressRouterLicense()
{
	routerLicenseSuppressed = TRUE;
}
#endif

// Note: this name is obscured for security
void CHECK_ROUTER_LICENSE()
{
	ROUTER_LICENSED_FLAG = 0;
	ROUTER_LICENSE_CHECKED = TRUE;

#if defined(WIN32) || PRODUCT_IS(LONTALK_STACK) || PRODUCT_IS(LONTALK_ROUTER) || PRODUCT_IS(IZOT)
      // Hard code router licensing on Windows, LONTALK STACK and LONTALK ROUTER, to avoid calling the licensing code.
      // For iLON simulations on Windows this can be changed for testing.
      // Router is licensed by default, unless suppressRouterLicense() is called first.
      ROUTER_LICENSED_FLAG = routerLicenseSuppressed ? 0 : ROUTER_LICENSED_MAGIC_NUM;
#elif defined(BOOTROM)
	// Treat the router as never licensed in the bootrom to avoid including licensing code.
	ROUTER_LICENSED_FLAG = 0;
#else
	LicMgrTaskCallBlock taskCallBlock;
	LicMgrLicenseId licenseId;
	LicMgrLicenseData *pLic;
	unsigned char secretKey[MD5_DIGEST_LEN];
	unsigned char digest[MD5_DIGEST_LEN];
	unsigned char ROUTER_KEY_PART_0[MD5_DIGEST_LEN/2] = {0x2A,0xA7,0x85,0x6D,0xD1,0x60,0xEF,0x96};

	if (SW_PLATFORM_IS_TYPE(ILON_600_PLATFORM))
		return;	// Don't check for a license on the 600

	ROUTER_LICENSED_FLAG = 0;
	// Check for a valid license.
	licenseId.szCompanyName = ROUTER_LICENSE_COMPANY_NAME;
	licenseId.szFeatureName = ROUTER_LICENSE_FEATURE_NAME;
	// Use dedicated license file
	memset(&taskCallBlock, 0, sizeof(taskCallBlock));
	taskCallBlock.pFilePath	= ROUTER_LICENSE_FILE_NAME;
	taskCallBlock.pLicenseId = &licenseId;
	// Macro for indirect task call.
	LICMGR_TaskCall_FindLicense(&taskCallBlock);

#ifdef DEBUGBUILD
	// For development, allow the router if ANY matching well-formed license is found,
	// regardless of node lock or license key
	if ((taskCallBlock.sts == LicMgrStsOK) || (taskCallBlock.sts == LicMgrStsLockMismatch))
	{
		ROUTER_LICENSED_FLAG = ROUTER_LICENSED_MAGIC_NUM;
	}
	else
#endif
	if (taskCallBlock.sts == LicMgrStsOK)
	{
		pLic = taskCallBlock.pLicense;

		// Copy the key in parts
		memcpy(secretKey, ROUTER_KEY_PART_0, sizeof(ROUTER_KEY_PART_0));
		memcpy(&secretKey[sizeof(ROUTER_KEY_PART_0)], ROUTER_KEY_PART_1, sizeof(ROUTER_KEY_PART_1));

		LICMGR_hmac_md5((unsigned char*)pLic->szHashText, strlen(pLic->szHashText),
					secretKey, 16, digest);
		if ((pLic->licenseKeyLen == MD5_DIGEST_LEN) &&
			(memcmp(pLic->licenseKey, digest, MD5_DIGEST_LEN) == 0))
		{
			ROUTER_LICENSED_FLAG = ROUTER_LICENSED_MAGIC_NUM;
		}
		else
		{
			taskCallBlock.pGeneric = (void*)"Router license is invalid: file " ROUTER_LICENSE_FILE_NAME;
			LICMGR_TaskCall_LogLicenseError(&taskCallBlock);
		}
	}
	// free the license data, if any
	LICMGR_TaskCall_FreeLicenseData(&taskCallBlock);

#endif // !WIN32 && !BOOTROM
}

// Note: this name is obscured for security
// This routine only shows whether the router is licensed, not whether it is
// actually allowed to run.
boolean LT_ROUTER_IS_LICENSED()
{
	if (!ROUTER_LICENSE_CHECKED)
	{
		CHECK_ROUTER_LICENSE();
	}

	// Only the xx00 and 600 model iLONs have routers apps
	return ((ROUTER_LICENSED_FLAG == ROUTER_LICENSED_MAGIC_NUM) ||
			SW_PLATFORM_IS_TYPE(ILON_600_PLATFORM));
	// Any Windows-based platforms will automatically show the router
	// licensed, unless suppressRouterLicense() is called before checking.
}

// Note: this name is obscured for security
// This routine indicates the router is licensed and allowed to run.
boolean LT_ROUTER_IS_PRESENT()
{
	if (!ROUTER_LICENSE_CHECKED)
	{
		CHECK_ROUTER_LICENSE();
	}

	// Only the xx00 and 600 model iLONs have routers apps
	return (((ROUTER_LICENSED_FLAG == ROUTER_LICENSED_MAGIC_NUM)
// EPANG2 - no nvram for now
#if defined(__VXWORKS__)
	// Don't enable the router in standalone mode
		&& !sysStandaloneModeEnabled()
#endif
										) ||
			SW_PLATFORM_IS_TYPE(ILON_600_PLATFORM));
	// Any Windows-based platforms will automatically show the router
	// licensed, unless suppressRouterLicense() is called before checking.
}

// Note: this name is obscured for security
boolean IP_852_CHANNEL_IS_PRESENT()
{
	// For now, this is the same as having a router present
	return LT_ROUTER_IS_PRESENT();
}


