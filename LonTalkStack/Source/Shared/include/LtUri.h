/*
 * LtUri.h
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
 */

#ifndef _LTURI_H
#define _LTURI_H

#include "LonPlatform.h"
#include "FtxlApiInternal.h"
#include "FtxlTypes.h"

#if FEATURE_INCLUDED(IZOT)
#ifndef USE_UIP
#include "types/vxTypesBase.h"		/* must come between vxArch/vxTypes */
#include "types/vxTypes.h"
#endif 
#include "ipv6_ls_to_udp.h"
#endif

/*
 *
 * This class defines the LTS device URI 
 *
 * scheme://[addr1][,addr2][,options][:port]
 * scheme = {ldv|unicast|multicast|izot} (default: multicast)  
 *                                       (aliases: uc=unicast, mc=multicast, 
 *                                        izot= IzoT IP, izot-e=Enhanced Mode IzoT IP) 
 *
 * unicast:
 *  addr1 = local listening address (default: 0.0.0.0 = ANY_ADDR)
 *  addr2 = not used (error)
 *  options = not used
 *  port  = unicast port (default: 1628)
 * 
 * multicast:
 *  addr1 = local listening address (default: 0.0.0.0 = ANY_ADDR)
 *  addr2 =  multicast group address (default: 239.192.84.76)
 *  options = not used
 *  port  = multicast group port (default: 1628)
 * 
 * ldv:
 *  addr1 = LDV device name (default: "LON1")
 *  addr2 = not used (error)
 *  options = not used
 *  port  = not used (error)
 *
 * izot/izot-e:
 *  addr1 = device name (only used in Windows environment)
 *  addr2 = interface name, linux: "eth" or "wlan" for wifi (default: eth)
 *                          windows: "Local Area Connection" or "Wireless Network Connection"
 *  options = in hex (see LtUriOptions in LtUri.h)
 *  port  = only 2541 is a valid port
 *
 */

#if FEATURE_INCLUDED(IZOT)
#include "IzoTNiConfig.h"
#endif

#if FEATURE_INCLUDED(IZOT)
#define SCHEME_DEFAULT      IPNative
#else
#define SCHEME_DEFAULT      IPMulticast
#endif

#define UC_ADDR1_DEFAULT    "0.0.0.0"       // default address for unicast
#define MC_ADDR1_DEFAULT    "239.192.84.76" // default group address for multicast
#define LDV_ADDR1_DEFAULT   "LON1"          // default LDV device name
#define IZOT_ADDR1_DEFAULT  "IZOT"          // default IZOT device name

#define MC_ADDR2_DEFAULT    "0.0.0.0"       	// default internal address for multicast

#define UC_PORT_DEFAULT    	1628             	// default unicast port
#define MC_PORT_DEFAULT		1628            	// default multicast port
#define IZOT_PORT_DEFAULT	IPV6_LS_UDP_PORT	// default IzoT IP port
#define IZOT_MGMT_DEFAULT   LtUriOption_None    // set back to 0 for now (instead of LONLINK_IZOT_MGMNT_OPTION_SET_IP_ADDR)

#define MAX_URI_LEN         256
#define MAX_ADDR_LEN        64

#if FEATURE_INCLUDED(IZOT)
typedef enum
{
    LtUriOption_None = 0,
    LtUriOption_IzotMgmntSetIPAddr = LONLINK_IZOT_MGMNT_OPTION_SET_IP_ADDR,  // 0x0001
    LtUriOption_IzotMgmntRetryBinding = LONLINK_IZOT_MGMNT_OPTION_RETRY_BINDING, // 0x0002
} LtUriOptions;
#endif


// The following are the enumerations that indicate the error status of the device URI
// required. 
typedef	enum
{
    LtUri_success = 0,
    LtUri_invalidFormat = 1,        // if it's not empty, must contains "://" or "//"
	LtUri_invalidIPAddress = 2,     // 
    LtUri_invalidMcastIPAddress = 3,
	LtUri_portOutOfRange = 4,
    LtUri_duplicatePort = 5,
    LtUri_duplicateMCastIPAddress = 6,
    LtUri_unknownScheme = 7,
    LtUri_extraIPAddress = 8,        // second IP address (Mcast address) is defined for Unicast/Ldv
    LtUri_extraPort = 9              // port is defined for Ldv
} LtUriError;

class LTA_EXTERNAL_CLASS LtUri
{
private:
    int m_port;                 // e.g. "1628" 
    int m_IPAddress1;
    int m_IPAddress2;           // multicast address in the case of IP852 multicast    
    int m_IPManagement;         // IP Management option for IzoT IP
    char* m_szNiName;
    char* m_szIPInterface;		// IP network interface eth (Ethernet) or wlan (Wifi)
    LontalkStackUriScheme m_Scheme;  // "mc" for IP852 multicast, "uc" for IP852 unicast or "ldv" for OpenLDV 
	
private:
    void init();
	void setDefault(LontalkStackUriScheme scheme=(LontalkStackUriScheme)SCHEME_DEFAULT);          // set to default URI
    LtUriError checkSchemeName(char *pSchemeName);    // check if the scheme name is a valid LTS URI scheme
    bool getURI(char *s, int maxLen);
    LtUriError parseURI(const char *pDeviceUri);
    LtUriError parseURIAddr(const char *pDeviceUri);
public:
    LtUri();     
    ~LtUri();
    LtUri(const char* pDeviceURI);
    bool setData(const char* pDeviceURI);
    int getPort() { return m_port; } 
    int getIPAddress() { return m_IPAddress1; }
    int getMulticastIPAddress() { return m_IPAddress2; }
    bool getData(char* data, int maxLen);
    const char* getNiName() { return m_szNiName; }
    const char* getIPInterfaceName() { return m_szIPInterface; }
    LontalkStackUriScheme getScheme() { return m_Scheme; }
    const char* getSchemeName();
    int getIPManagementOption() { return m_IPManagement; }
};

#endif
