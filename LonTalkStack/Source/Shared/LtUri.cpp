/* 
 * LtUri.cpp
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

#include <stdio.h>
#include <string.h>
#include "LtUri.h"

#ifdef WIN32
#undef ERROR
#undef INVALID_SOCKET
#include <WinSock.h>
#else
#include <arpa/inet.h>
#endif  // WIN32

#include "vxWorks.h"
#include "VxSockets.h"


/**
 * This class defines the LTS device URI 
 *
 */

static const char* LontalkStackUriAliasString[] =
{
    "unicast",
    "multicast",
    "ldv",
	"izot",
    "izot-enhanced"
};

static const char* LontalkStackUriSchemeString[] =
{
    "uc",       // unicast IP-852
    "mc",       // multicast
    "ldv",  
	"izot",     // using native IP
    "izot-e"    // native IP running enhanced mode
};

LtUri::LtUri()        
{
    init();
}

LtUri::LtUri(const char* pDeviceURI)
{
    init();
    setData(pDeviceURI);
}

LtUri::~LtUri()
{
	if (m_szNiName != NULL)
        delete[] m_szNiName;
    if (m_szIPInterface != NULL)
        delete[] m_szIPInterface;
}

static char toLowerCase(char c)
{
    if (c >= 'A' && c <= 'Z')
        c = c - 'A' + 'a';
    return c;
}

// Convert asci string into unsigned long
static unsigned long hatol(char *astring)
{
    unsigned long result = 0;
    unsigned val;

    if (astring == 0)
        return 0;

    while(*astring)
    {
        char c = toLowerCase(*astring);
        if (c >= '0' && c <= '9')
            val = c - '0';
        else if (c >= 'a' && c <= 'f')
            val = c - 'a' + 10;
        else
            break;  // not a valid hex string
        result = (result << 4) + val;
        ++astring;
    }
    return result;
}


// remove spaces/blanks from a string 
static void removeSpace(char *str)
{
    char *p1 = str;
    char *p2 = str;
  
    do
    {
        while (*p2 == ' ')
            p2++; 
    } while ((*p1++ = *p2++));
}

// Trim the leading and trailing spaces/blanks
static char* trimSpace(char *str)
{
    int len = 0;
    char *frontp = str - 1;
    char *endp = NULL;

    if( str == NULL )
        return NULL;

    if( str[0] == '\0' )
        return str;

    len = strlen(str);
    endp = str + len;

    while(*(++frontp) == ' ');
    while( *(--endp) == ' ' && endp != frontp );

    if( str + len - 1 != endp )
        *(endp + 1) = '\0';
    else if( frontp != str &&  endp == frontp )
        *str = '\0';

    endp = str;
    if( frontp != str )
    {
        while( *frontp ) *endp++ = *frontp++;
        *endp = '\0';
    }
    return str;
}

void LtUri::init()
{
    m_szNiName = NULL;
    m_szIPInterface = NULL;
    // set the default URI
    setDefault();
}

// Set to default URI - multicast with default group (MC_ADDR1_DEFAULT) and port (MC_PORT_DEFAULT)
void LtUri::setDefault(LontalkStackUriScheme scheme)
{
    if (scheme == IPUnicast)
    {
        m_IPAddress1 = 0;  
        m_IPAddress2 = 0;
        m_port = UC_PORT_DEFAULT;
        m_Scheme = scheme;
    }
    else if (scheme == Ldv)
    {
        m_IPAddress1 = 0;  
        m_IPAddress2 = 0;
        m_port = 0;
        m_Scheme = scheme;
        delete[] m_szNiName;
        m_szNiName = new char[strlen(LDV_ADDR1_DEFAULT) + 1];
        strcpy(m_szNiName,LDV_ADDR1_DEFAULT);
    }
#if FEATURE_INCLUDED(IZOT)
	else if (scheme == IPNative || scheme == IPNativeEnhanced)
    {
        m_IPAddress1 = 0;  
        m_IPAddress2 = 0;
        m_port = IZOT_PORT_DEFAULT;  
        m_IPManagement = IZOT_MGMT_DEFAULT;
        m_Scheme = scheme;
		delete[] m_szNiName;
		delete[] m_szIPInterface;
        m_szNiName = new char[strlen(IZOT_ADDR1_DEFAULT) + 1];
        strcpy(m_szNiName,IZOT_ADDR1_DEFAULT);

        // Default is to use any interface available
        m_szIPInterface = NULL;
    }
#endif
    else
    {
        // multicast
        m_IPAddress1 = 0;  
        m_IPAddress2 = htonl(inet_addr(MC_ADDR1_DEFAULT));
        m_port = MC_PORT_DEFAULT;
        m_Scheme = IPMulticast;
    }
}

// compose the URI base on the current property values  
bool LtUri::getURI(char *s, int maxLen)
{
    if (s == NULL)
        return false;

    char *szURI = (char *)malloc(MAX_URI_LEN);
    int len;

    if (m_Scheme == Ldv)
    {
        sprintf(szURI, "%s://%s", LontalkStackUriSchemeString[m_Scheme], m_szNiName == NULL ? "" : m_szNiName);
    }
    else if (m_Scheme == IPNative || m_Scheme == IPNativeEnhanced)
    {
        sprintf(szURI, "%s://%s,%s,%x:%d", LontalkStackUriSchemeString[m_Scheme],
        		m_szNiName == NULL ? "" : m_szNiName,
        		m_szIPInterface  == NULL ? "" : m_szIPInterface,
                m_IPManagement,
                m_port);
    }
    else
    {
        char szIPAddress1[MAX_ADDR_LEN], szIPAddress2[MAX_ADDR_LEN];

        vxsMakeDottedAddr(szIPAddress1, m_IPAddress1);
        if (m_Scheme == IPUnicast)
        {
            sprintf(szURI, "%s://%s:%d", LontalkStackUriSchemeString[m_Scheme], szIPAddress1, m_port);
        }
        else
        {
            vxsMakeDottedAddr(szIPAddress2, m_IPAddress2);
            sprintf(szURI, "%s://%s,%s:%d", LontalkStackUriSchemeString[m_Scheme], szIPAddress1, szIPAddress2, m_port);
        }
    }
    len = (int)strlen(szURI) + 1;
    strncpy(s,szURI, maxLen > len ? len : maxLen); 
    free(szURI);
    return (maxLen >= len);     // returns false if the buffer is too short to hold the URI
}

// check if the scheme name is a valid LTS URI scheme
LtUriError LtUri::checkSchemeName(char *pSchemeName)
{
    int i;

    for (i = 0; i < SchemeCount; ++i)
    {
        if (!strcmp(LontalkStackUriSchemeString[i], pSchemeName))
        {
            m_Scheme = (LontalkStackUriScheme)i;
#if !FEATURE_INCLUDED(IZOT)
            if (m_Scheme == IPNative || m_Scheme == IPNativeEnhanced)
                return LtUri_unknownScheme;
#endif
            return LtUri_success;
        }
    }
    for (i = 0; i < SchemeCount; ++i)
    {
        if (!strcmp(LontalkStackUriAliasString[i], pSchemeName))
        {
            m_Scheme = (LontalkStackUriScheme)i;
            return LtUri_success;
        }
    }
    return LtUri_unknownScheme;
}


// Parse URI addresses and port. Returns false if invalid URI. Otherwise, it returns true.
//
LtUriError LtUri::parseURIAddr(const char *pDeviceUri)
{
    const char *pAddr;
    const char *pStr;
    char *str = NULL;
    LtUriError retval = LtUri_success;
    bool isParsingPort = false;
    unsigned long lAddr;
    int nItems = 1;
    int nPorts = 0;

    // Initialize to use default URI
    setDefault(m_Scheme);

    if (!pDeviceUri || !*pDeviceUri)
    {
        return retval;
    }

    pAddr = pDeviceUri;
    while (*pAddr)
    {
        isParsingPort = false;
        if (*pAddr == ',')
        {
            // Parse the next item
            ++nItems;
            pAddr++;    // skip the ','
        }
        else if (*pAddr == ':')
        {
            // Parse the udp port
            ++nPorts;
            pAddr++;    // skip the ':'
            isParsingPort = true;
        }

        pStr = pAddr;
        while (*pAddr)
        {
            if ((*pAddr == ':') || (*pAddr == ','))
                break;
            pAddr++;
        }
            
        if (str != NULL)
            free(str);
        str = (char *)malloc((pAddr-pStr) + 1);
        memset(str,0,(pAddr-pStr)+1);
        strncpy(str, pStr, pAddr-pStr);
        str = trimSpace(str); 
        if (str[0])
        {
            if (isParsingPort)
            {
                if (nPorts > 1)
                {
                    retval = LtUri_duplicatePort;
                    break;   // port is already specified
                }
                if (m_Scheme == Ldv)
                {
                    retval = LtUri_extraPort;
                    break;  // LDV is not using port
                }
                m_port = atoi(str);

                // validate UDP port range
                if (
#if FEATURE_INCLUDED(IZOT)
                    ((m_Scheme == IPNative || m_Scheme == IPNativeEnhanced) && m_port != IZOT_PORT_DEFAULT) ||
#endif
                    (m_port <= 0 || m_port >= 65535))
                {
                    retval = LtUri_portOutOfRange;
                    break;   // port is out of range
                }
            }
            else
            {
                if (nItems == 1)
                {
                    if (m_Scheme == Ldv || m_Scheme == IPNative || m_Scheme == IPNativeEnhanced)
                    {
                        delete[] m_szNiName;
                        m_szNiName = new char[strlen(str) + 1];
                        strcpy(m_szNiName,str);
                    }
                    else
                    {
                        // Parse the IP Address
                        // Converts the Internet host address cp from IPv4 
                        // numbers-and-dots notation into binary data in network byte order
                        removeSpace(str); 
                        lAddr = inet_addr(str);
                        if (lAddr == INADDR_NONE)
                        {
                            retval = LtUri_invalidIPAddress;
                            break;
                        }
                        else
                            m_IPAddress1 = htonl(lAddr); 
                    }
                }
                else if (nItems == 2)
                {
                    if (m_Scheme == IPUnicast || m_Scheme == Ldv)
                    {
                        retval = LtUri_extraIPAddress;
                        break;    // Unicast and LDV are not using Addr2
                    }
                    else
                    if (m_Scheme == IPNative || m_Scheme == IPNativeEnhanced)
                    {
                        // Parse the IP Interface
                        if (m_szIPInterface != NULL)
                            delete[] m_szIPInterface;
               	        m_szIPInterface = new char[strlen(str) + 1];
                        strcpy(m_szIPInterface,str);
                    }
                    else
                    {
                        // Parse the multicast address
                        // Converts the Internet host address cp from IPv4 numbers-and-dots 
                        // notation into binary data in network byte order
                        removeSpace(str); 
                 	    lAddr = inet_addr(str);
           	            if (lAddr == INADDR_NONE)
           	            {
                   		    retval = LtUri_invalidIPAddress;
           		            break;
           	            }
          	            else
                   		    m_IPAddress2 = htonl(lAddr);
                    }
                }
                else if (nItems == 3)
                {
                    if (m_Scheme == IPNative || m_Scheme == IPNativeEnhanced)
                    {
                        // Parse IP Management option
                        m_IPManagement = hatol(str) & 0xFFFF;
                    }
                    else if (m_Scheme == IPMulticast)
                    {
                        retval = LtUri_duplicateMCastIPAddress;
                        break;   // multicast addr is already defined
                    }
                    else
                    {
                        retval = LtUri_invalidFormat;
                        break;   // invalid format
                    }
                }
                else
                {   
                    // Too many items
                    retval = LtUri_invalidFormat;
                    break;   // invalid format
                }
            }
        }
    }

    // clean up resources
    if (str != NULL)
        free(str);

    return retval;
}

// Parse device URI. Returns false if invalid URI. Otherwise, it returns true.
//
// * This class defines the LTS device URI 
// *
// * scheme://[addr1][,addr2][,options][:port]
// * scheme = {ldv|unicast|multicast|izot} (default: multicast)  
// *                                       (aliases: uc=unicast, mc=multicast, 
// *                                        izot= IzoT IP, izot-e= IzoT IP in enhanced mode)
// *
// * unicast:
// *  addr1 = local listening address (default: 0.0.0.0 = ANY_ADDR)
// *  addr2 = not used (error)
// *  options = not used
// *  port  = unicast port (default: 1628)
// * 
// * multicast:
// *  addr1 = local listening address (default: 0.0.0.0 = ANY_ADDR)
// *  addr2 =  multicast group address (default: 239.192.84.76)
// *  options = not used
// *  port  = multicast group port (default: 1628)
// * 
// * ldv:
// *  addr1 = LDV device name (default: "LON1")
// *  addr2 = not used (error)
// *  options = not used
// *  port  = not used (error)
// *
// * izot/izot-e:
// *  addr1 = device name (only used in Windows environment)
// *  addr2 = interface name, linux: "eth" or "wlan" for wifi (default: eth)
// *                          windows: "Local Area Connection" or "Wireless Network Connection"
// *  options = in hex (see LtUriOptions in LtUri.h)
// *  port  = only 2541 is a valid port
// *
//
LtUriError LtUri::parseURI(const char* pDeviceURI)
{
    char *pPath;
    char *scheme;
    
    LtUriError result = LtUri_invalidFormat;

    setDefault();           // Default to multicast with default group and port

    if (!pDeviceURI || !*pDeviceURI)
    {
        result = LtUri_success;
    }
    else
    {
        /* Find schema name */
        if ((pPath = strstr((char *)pDeviceURI, "://")) != NULL)
        {	
            scheme = (char *)malloc((pPath-pDeviceURI)+1);
            memset(scheme, 0, (pPath-pDeviceURI)+1);
            strncpy(scheme, pDeviceURI, pPath-pDeviceURI);
            removeSpace(scheme);
            char *pScheme = scheme;
            while (*pScheme)
            {
                *pScheme = toLowerCase(*pScheme);
                pScheme++;
            }
            result = checkSchemeName(scheme);
            if (scheme != NULL) free(scheme);
            if (result == LtUri_success)
                result = parseURIAddr(pPath+3);
        } 
        else
        if (*pDeviceURI == '/' && *(pDeviceURI+1)=='/')
        {
            // no scheme.  URI starts with "//"
            result = parseURIAddr(pDeviceURI+2);
        }
    }

    return result;
}

bool LtUri::setData(const char* pDeviceURI)
{
    return (parseURI(pDeviceURI) == LtUri_success);
}

bool LtUri::getData(char* data, int maxLen)
{
    return getURI(data, maxLen);
}

const char* LtUri::getSchemeName()
{
    return LontalkStackUriSchemeString[m_Scheme]; 
};

