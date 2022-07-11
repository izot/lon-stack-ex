#ifndef _LTIPPLATFORM_H
#define _LTIPPLATFORM_H

//
// LtIpPlatform.h
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
// Routines for platform specific control
//

#ifdef __cplusplus

//
// Extension to "new" to prevent it throwing exceptions.
// 
// For example,
//   void* p = new NOTHROW LtObject;
//
#ifdef WIN32
// Not necessary.
#define NOTHROW
#else
#include <new>
#define NOTHROW (std::nothrow)
#endif

#endif	// __cplusplus

#ifdef __cplusplus
extern "C"
{
#endif

// Each of these routines returns true if successful, else false with the
// notable exception of reboot which never returns if successful.
boolean LtIpReboot();
boolean LtIpRebootAllowed();
boolean LtIpStartWebServer();
boolean LtIpStopWebServer();
void LtIpApplicationsStarted();
void LtIpEventLog(const char* szMsg);

/* Platform determination definitions */
/* These are specific platform types */
#define VNI_PLATFORM			1
#define CNFG_SRVR_PLATFORM		2
#define ILON_PLATFORMS_START	10	/* lowest iLON ID */
									/* Keep this list packed. Don't change IDs */
#define ILON_100_PLATFORM		10
#define ILON_100DP_PLATFORM		11
#define ILON_1000_PLATFORM		12	/* not really supported anymore */
#define ILON_1500_PLATFORM		13
#define ILON_2000_PLATFORM		14
#define ILON_600_PLATFORM		15
#define ILON_PLATFORMS_END		15	/* highest iLON ID */
/* These are composite types (platform families) */
#define PLATFORM_FAMILIES		100	/* the start of the families */
#define ILON_ANY_PLATFORM		101	/* any iLON */
#define ILON_100ANY_PLATFORM	102	/* iLON 100 or 100DP */
#define ILON_XX00_PLATFORM		103	/* iLON 1000, 1500, 2000... */

// IKP05202003: added support for HW and SW platform compatibility
#define ILON_100_PLATFORM_NAME		"i.LON 100"
#define ILON_100DP_PLATFORM_NAME	"i.LON 100DP"
#define ILON_600_PLATFORM_NAME		"i.LON 600"
#define INVALID_PLATFORM_NAME		"Invalid"

// **** SECURITY DEFINITIONS ****
// The following macros are name translations to obscure the real routines for security reasons
// The names should look sort of real, with some random letters added
// These all start with "LtIp" to make it easier to strip them out of the iLonSystem (with WDB)
// object image, since not all symbols can be stripped from that image.
#define GET_PLATFORM_NAME_BY_TYPE	LtIpDevmodeaqxp
#define SW_PLATFORM_IS_TYPE			LtIpDevmodeaqxl
#define HW_PLATFORM_IS_TYPE			LtIpframecpgl
#define GET_SW_PLATFORM_TYPE		LtIpframepglq
#define GET_HW_PLATFORM_TYPE		LtIppglxframe
#define GET_HW_PLATFORM_VERSION     LtIpXdrvModeVal
#define IP_852_CHANNEL_IS_PRESENT	LtIpqcximode
#define LT_ROUTER_IS_PRESENT		LtIpqcxitype
#define LT_ROUTER_IS_LICENSED		LtIpXpktque2
#define	CHECK_ROUTER_LICENSE		LtIpGetSystMode0Sem
#define	ROUTER_LICENSE_CHECKED		LtIpSystMode0SemA
#define	ROUTER_LICENSED_FLAG		LtIpSystMode0Sem
#define ROUTER_LICENSED_MAGIC_NUM	42
#define ROUTER_KEY_PART_0			LtIpSystMode0SemB
#define ROUTER_KEY_PART_1			LtIpSystMode1
#define ROUTER_LICENSE_COMPANY_NAME "Echelon Corporation"
#define ROUTER_LICENSE_FEATURE_NAME "IP-852 Router"
#define ROUTER_LICENSE_FILE_NAME	"Echelon1IP852RtrLic.xml"

// These are the actual routine declarations
boolean GET_PLATFORM_NAME_BY_TYPE(int nPlatformType, char *pszPlatformName, int len); 
boolean SW_PLATFORM_IS_TYPE(int type);
boolean HW_PLATFORM_IS_TYPE(int type);
int GET_SW_PLATFORM_TYPE();
int GET_HW_PLATFORM_TYPE();
int GET_HW_PLATFORM_VERSION();
boolean IP_852_CHANNEL_IS_PRESENT();
boolean LT_ROUTER_IS_PRESENT();
void CHECK_ROUTER_LICENSE();

// Definitions for GET_HW_PLATFORM_VERSION()
#define ILON_HW_VERSION_E3 2
#define ILON_HW_VERSION_E4 3

#ifdef WIN32
// For Windows simulations
void suppressRouterLicense();
#endif

// These are the old names of the above routines.

//boolean getPlatformNameByType(int nPlatformType, char *pszPlatformName, int len); 
//boolean LtSwPlatformIsType(int type);
//boolean LtHwPlatformIsType(int type);
//int getSwPlatformType();
//int getHwPlatformType();
//boolean LtSwPlatformHasRouterApp();


void getSoftwareVersionNum(int *major, int *minor, int *build);
const char *getSoftwareVersionNumStr();
const char *getSoftwareProductStr();

// max chars in timezone = 128
void	LtSetTimeZone( char* szTimeZone );
void	LtGetTimeZone( char* szTimeZone );

#ifdef __cplusplus
}
#endif

#endif

