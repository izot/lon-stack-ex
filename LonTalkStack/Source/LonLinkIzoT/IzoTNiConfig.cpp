
/***************************************************************
 *  Filename: IzoTNiConfig.cpp
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
 *  Description:  Utilities to IzoT configure network interfaces.
 *
 ****************************************************************/

#include "LtaDefine.h"
#if FEATURE_INCLUDED(IZOT)
#include <stdio.h>
#ifdef WIN32
#include <winsock2.h>
#include <iphlpapi.h>

#else

#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include "LonPlatform.h"
#include <sys/types.h>
#include <ifaddrs.h>
#include <unistd.h>		// sleep()
#endif


#include "IzoTNiConfig.h"

#include "vxlTarget.h"
#include "ipv6_ls_to_udp.h"

#define IZOT_BINDING_TIMEOUT 5000   // 5 seconds
#define IZOT_BINDING_SLEEP_TIME 100
#define MAX_IZOT_BINDING_RETRIES (IZOT_BINDING_TIMEOUT/IZOT_BINDING_SLEEP_TIME)

#ifdef WIN32
#define REG_CLASS       "Application Global Data"

#pragma comment(lib, "Iphlpapi.lib")

/******************************************************************************
  Function:  openAdapter
   
  Summary:
    Open an adaptor configuration and return a pointer to it.  This function
    allocates a buffer containing the information which must be freed by the
    caller using "free".

  Parameters:
    ipIfName:   The IP Interface Name
    ppBuffer:   Set to the address of the buffer to be freed when completed.

  Return:
    A pointer to the adapter configuration.
*****************************************************************************/
static PIP_ADAPTER_ADDRESSES openAdapter(const char *ipIfName, void **ppBuffer)
{
    PIP_ADAPTER_ADDRESSES pMyAdapter = NULL, pAddresses = NULL;
    *ppBuffer = NULL;
    // Currently only IPV4 is supported..
    DWORD dwSize = 0;

    // Determine the size of the adapters configuration.
    DWORD dwRetVal = GetAdaptersAddresses(AF_INET /* IPV4 */, 0, NULL, pAddresses, &dwSize);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) 
    {
        // Allocate a buffer for the configuration of all adapters
        pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(dwSize);

        // Get the configuration for all of the adapters.  This copies the address information
        // for all adapters arranged as a linked list of adapters
        dwRetVal = GetAdaptersAddresses(AF_INET /* IPV4 */, 0, NULL, pAddresses, &dwSize);
        if (dwRetVal == ERROR_SUCCESS)
        {
            PIP_ADAPTER_ADDRESSES pAdapter;
            // Scan the linked list of adapters to find the one we are interested in.
            for (pAdapter = pAddresses; pMyAdapter == NULL && pAdapter != NULL; pAdapter = pAdapter->Next)
            {
                char *friendlyName = NULL;
                int friendlyNameLen = WideCharToMultiByte(CP_UTF8, 0, pAdapter->FriendlyName, 2*wcslen(pAdapter->FriendlyName), NULL, 0, NULL, NULL);
                if (friendlyNameLen != 0)
                {
                    friendlyNameLen++;
                    friendlyName = (char *)malloc(friendlyNameLen);
                    if (WideCharToMultiByte(CP_UTF8, 0, pAdapter->FriendlyName, 2*wcslen(pAdapter->FriendlyName), friendlyName, friendlyNameLen, NULL, NULL) != 0)
                    {
                        if (strcmp(ipIfName, friendlyName) == 0)
                        {
                            // Got a match.  We are done.
                            pMyAdapter = pAdapter;
                        }
                    }
                    free(friendlyName);
                }
            }
        }
    }
    if (pMyAdapter == NULL)
    {
        // Couldn't find it.  Free the buffer.
        free(pAddresses);
    }
    else
    {
        // Pass the buffer back to the user so they can free it when done.
        *ppBuffer = pAddresses;
    }
    return pMyAdapter;
}

/******************************************************************************
  Function:  findAddress
   
  Summary:
    Find the configuration of a specific unicast address on the specified
    adapter.

  Parameters:
    pAdapter:       Pointer to the adapter address configuration returned by 
                    openAdapter.
    szLocalIpAddr:  The unicast address in dotted format.

  Return:
    A pointer to the unicast address information or NULL
*****************************************************************************/
static PIP_ADAPTER_UNICAST_ADDRESS findAddress(PIP_ADAPTER_ADDRESSES pAdapter, const char *szLocalIpAddr)
{
    PIP_ADAPTER_UNICAST_ADDRESS pUnicastAddr;  
    ULONG localIpAddr = inet_addr(szLocalIpAddr);

    // The address are linked in a link list.  Scan until we find the one we
    // are looking for.
    for (pUnicastAddr = pAdapter->FirstUnicastAddress; pUnicastAddr != NULL; pUnicastAddr = pUnicastAddr->Next)
    {
        sockaddr_in *pInAddr = (sockaddr_in *)pUnicastAddr->Address.lpSockaddr;
        if (memcmp(&pInAddr->sin_addr, &localIpAddr, sizeof(localIpAddr)) == 0)
        {
            // Found it.
            break;
        }
    }

    // Return the pointer to the unicast information if found, NULL otherwise.
    return pUnicastAddr;
}

/******************************************************************************
  Function:  addLocalIpAddr
   
  Summary:
    Add a unicast IP address to the specified interface.

    Note that this is not persistent.  The configuration is machine wide and
    outlives the process, but does not survive reboots.  It also disables DHCP
    until the configuration is restored (by a reboot for example).

    This operation requires elevation on Windows. Since it disables DHCP it
    is considered disruptive.  For example, the PC will no longer receive
    DHCP gateway address assignments.

  Parameters:
    szLocalIpAddr:  The unicast address in dotted format.
    szSubnetMask:   The subnet mask in dotted format
    interfaceName:  The interface name

  Return:
    True if successful, false otherwise.
*****************************************************************************/
static bool addLocalIpAddr(const char *szLocalIpAddr, const char *szSubnetMask, const char *interfaceName)
{
    bool success = false;
    void *pBuffer;

    // Get the address configuration of the adaptor
    PIP_ADAPTER_ADDRESSES pAdapter = openAdapter(interfaceName, &pBuffer);
    if (pAdapter != NULL)
    {
        // See if the unicast address we want to add already exists
        PIP_ADAPTER_UNICAST_ADDRESS pAddr = findAddress(pAdapter, szLocalIpAddr);
        if (pAddr == NULL)
        {
            int err;
            ULONG  NTEContext;
            ULONG  NTEInstance;
            ULONG subnetMask = inet_addr(szSubnetMask);
            ULONG localIpAddr = inet_addr(szLocalIpAddr);

            // The address doesn't exist, so we can try to add it.
            vxlReportEvent("Address %s not found, adding it...\n", szLocalIpAddr);

            // Try adding
            err = AddIPAddress(localIpAddr, subnetMask, pAdapter->IfIndex, &NTEContext, &NTEInstance);
            if (err != NO_ERROR)
            {
                /* Variables used to return error message */
                LPVOID lpMsgBuf;
                vxlReportEvent("Failed to add address, err = %d\n", err);
                if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),       // Default language
                                  (LPTSTR) & lpMsgBuf, 0, NULL)) {
                    vxlReportEvent("\tError: %s\n", lpMsgBuf);
                    LocalFree(lpMsgBuf);
                }
            }
            else
            {
                vxlReportEvent("Successfully added Address %s subnet %s to interface %s\n", szLocalIpAddr, szSubnetMask, interfaceName);
                success = TRUE;
            }
        }
        else
        {
            // Already exists - OK..
            success = TRUE;
        }

        // Done with the address information - so free it.
        free(pBuffer);
    }
    return success;
}

/******************************************************************************
  Function:  IzoTBindToAvailUnicastAddress
   
  Summary:
    Bind the specified socket to any available IP address on the 
    specified interface.

  Parameters:
    socket:             The open socket bind
    ipInterfaceName:    The name of the IP interface that the socket uses
    pSocketAddr:        Allocated socket address.  Must be freed by caller
                        using vxsFreeSockaddr.

  Return:
    True if successful, false otherwise.
*****************************************************************************/
bool IzoTBindToAvailUnicastAddress(VXSOCKET socket, const char *ipInterfaceName, VXSOCKADDR *pSocketAddr)
{
    bool success = false;
    void *pBuffer;
    *pSocketAddr = NULL;
    VXSOCKADDR socketAddr;

    // Find the addresses supported by this IP interface
    PIP_ADAPTER_ADDRESSES pAdapter = openAdapter(ipInterfaceName, &pBuffer);
    if (pAdapter != NULL)
    {
        VXSOCKADDR localAddr = NULL;

        // Try to bind to the first address found.  If that fails, go on to the
        // next until this succeeds or there are no more left.
        for (PIP_ADAPTER_UNICAST_ADDRESS pUnicastAddr = pAdapter->FirstUnicastAddress; 
            !success && pUnicastAddr != NULL; 
            pUnicastAddr = pUnicastAddr->Next)
        {
            // Get the IP address
            sockaddr_in *pInAddr = (sockaddr_in *)pUnicastAddr->Address.lpSockaddr;

            // Convert to VXSOCKADDR format
            socketAddr = vxsAddrValue(ntohl(pInAddr->sin_addr.S_un.S_addr));

            if (socketAddr != NULL)
            {
                STATUS vxsts;

                // Set the port to the LS UDP port
                vxsSetPort(socketAddr, gLsUdpPort);
                
                // Bind it.
			    vxsts = vxsBind(socket, socketAddr);

				if (vxsts == OK)
                {
                    char szIpAddress[100];
                    vxsMakeDottedAddr(szIpAddress, ntohl(pInAddr->sin_addr.S_un.S_addr));
                    vxlReportEvent("IzoTBindToAvailUnicastAddress succeeded, interface = '%s', address = '%s'\n", ipInterfaceName, szIpAddress);

                    // Success.  We are done.  Return the socket address to the caller
                    *pSocketAddr = socketAddr;
                    success = true;
                }
                else
                {
                    vxsFreeSockaddr(socketAddr);
                }
            }
        }

        // Free the address information
        free(pBuffer);
    }
    if (!success)
    {
        vxlReportUrgent("ERROR: IzoTBindToAvailUnicastAddress failed, interface = '%s'\n", ipInterfaceName);
    }
    return success;
}

#else

/******************************************************************************
  Function:  openAdapter
   
  Summary:
    Utility function on linux to open the network interface adapter configuration 
    of the local system and return a pointer to it. 
    This function allocates a buffer containing the information which must be freed by the
    caller using "freeifaddrs".

  Parameters:
    ipIfName:   The IP Interface Name
    ppBuffer:   Set to the address of the buffer to be freed when completed.

  Return:
    A pointer to the adapter configuration.
*****************************************************************************/
static struct ifaddrs * openAdapter(const char *ipIfName, void **ppBuffer, int *ifIndex)
{
    struct ifaddrs *pAddresses = NULL, *ifa = NULL, *pMyAdapter = NULL;
    bool bFound = FALSE;
    int index = -1;

    *ppBuffer = NULL;
	*ifIndex = 0;

    // get the list of structures describing the network interfaces of the local system
    if (getifaddrs(&pAddresses) == -1)
    {
    	vxlReportEvent("openAdapter: getifaddrs returns -1\n");
     	return pMyAdapter;
    }

	// Walk through linked list, maintaining head pointer so we can free list later
    for (ifa = pAddresses; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }

		vxlReportEvent("Name: %s Family = %d\n", ifa->ifa_name, ifa->ifa_addr->sa_family);
       	if ((ifa->ifa_addr->sa_family == AF_INET) &&
       			(!strncmp(ifa->ifa_name, ipIfName, strlen(ipIfName))))
    	{
			// is a valid IP4 Address
       		vxlReportEvent("Found it.  Name = %s\n", ifa->ifa_name);
       		if (!bFound)
       		{
       		    pMyAdapter = ifa;
			    bFound = TRUE;
       		}
            ++index;
     	}
    }

	if (pMyAdapter == NULL)
	{
		if (pAddresses != NULL)
    		// Free the data which dynamically allocated by the getifaddrs()
    		freeifaddrs(pAddresses);
	}
    else
    {
        *ppBuffer = pAddresses;
    }
	*ifIndex = index;
    return pMyAdapter;
}

/******************************************************************************
  Function:  findAddress
   
  Summary:
    Utility function on linux to find the configuration of a specific unicast 
    address on the specified network interface.

  Parameters:
    pAdapter:       Pointer to the network interface adapter address configuration
                    returned by openAdapter.
    ipIfName:       The IP Interface Name
    szLocalIpAddr:  The unicast address in dotted format.

  Return:
    A pointer to the unicast address information or NULL
*****************************************************************************/
static struct ifaddrs* findAddress(struct ifaddrs *pAdapter, const char *ipIfName, const char *szLocalIpAddr)
{
    struct ifaddrs * pUnicastAddr;  
    bool found = false;
    ULONG localIpAddr = inet_addr(szLocalIpAddr);

    vxlReportEvent("Find Address: %s (%ld)\n", szLocalIpAddr, localIpAddr);
    for (pUnicastAddr = pAdapter; 
        !found && pUnicastAddr != NULL; 
        pUnicastAddr = pUnicastAddr->ifa_next)
    {
       	char szIpAddress[100];
       	vxsMakeDottedAddr(szIpAddress, ntohl(((struct sockaddr_in *) pUnicastAddr->ifa_addr)->sin_addr.s_addr));

        // is a valid IP4 Address
        sockaddr_in *pInAddr = (sockaddr_in *)pUnicastAddr->ifa_addr;
        vxlReportEvent("Name = %s Addr = %s (%ld) Family=%d\n",
    			pUnicastAddr->ifa_name,
    			szIpAddress, pInAddr->sin_addr,
    			pUnicastAddr->ifa_addr->sa_family);

        if ( (pUnicastAddr->ifa_addr->sa_family == AF_INET) &&
    	     (!strncmp(ipIfName, pUnicastAddr->ifa_name, strlen(ipIfName))) &&
			 (!memcmp(&pInAddr->sin_addr, &localIpAddr, sizeof(localIpAddr))) )
		{
			// Found it
            break;
        }
    }
    return pUnicastAddr;
}

/******************************************************************************
  Function:  ExecSystemCommand
   
  Summary:
    Utility function on linux to execute a system command specified in cmd

  Parameters:
    cmd:       Pointer to the network interface adapter address configuration
               returned by openAdapter.
    bTrace:    Flag to enable/disable tracing.

  Return:
    The value returned is 0 if successful.  Otherwise, it returns the status
    of the command.
*****************************************************************************/
int ExecSystemCommand(const char *cmd, bool bTrace)
{
	vxlReportEvent("ExecSystemCommand\n");
    if (bTrace)
    {
    	vxlReportEvent("%s\n", cmd);
    }

    int   	status = system(cmd);

	if (status == -1)
	{
		vxlReportEvent("system(%s) error = %d (%s) \n", cmd, errno, strerror(errno));
		if (errno)
		{
			status = errno;
		}
	}
	else if (status)
	{
		vxlReportEvent("'%s' error = %d\n", cmd, WEXITSTATUS(status));
	}

	return status;
}

/******************************************************************************
  Function:  AddIPAddress
   
  Summary:
    Utility function on linux to add the specified IPv4 address to the specified 
    IP Interface adapter.
    In Windows, this function is part of the Iphlpapi.lib.
  Parameters:
    szLocalIpAddr:      The IPv4 address to add to the adapter in dotted format
    szSubnetMask:       The IPv4 subnet mask to use for adding an IP address in dotted format
    ifIndex:            The interface index to use for adding an IP address 
    ifName:             The IP Interface Name   

  Return:
    The value returned is 0 if successful.  Otherwise, it returns the status
    of the command.
*****************************************************************************/
static int AddIPAddress(const char *szLocalIpAddr, const char *szSubnetMask, int ifIndex, char *ifName)
{
    char cmd[200];

    // Add IP Address to the adapter specified in ifName
    // Format: ifconfig eth0: [ifIndex] netmask [IPAddress[ [netmask]
    sprintf(cmd, "ifconfig %s:%d %s netmask %s", ifName, ifIndex, szLocalIpAddr, szSubnetMask);
    vxlReportEvent("AddIPAddress: %s\n", cmd);
    if (ExecSystemCommand(cmd, true) == OK)
    	return true;
    else
    	return false;
}

/******************************************************************************
  Function:  addLocalIpAddr
   
  Summary:
    Utility function on linux to add a unicast IP address to the specified interface.

    Note that this is not persistent.  The configuration is machine wide and
    outlives the process, but does not survive reboots.  It also disables DHCP
    until the configuration is restored (by a reboot for example).

    This operation requires elevation on Linux. Since it disables DHCP it
    is considered disruptive.  For example, the system will no longer receive
    DHCP gateway address assignments.

  Parameters:
    szLocalIpAddr:  The unicast address in dotted format.
    szSubnetMask:   The subnet mask in dotted format
    interfaceName:  The interface name

  Return:
    True if successful, false otherwise.
*****************************************************************************/
static bool addLocalIpAddr(const char *szLocalIpAddr, const char *szSubnetMask, const char *interfaceName)
{
    bool success = false;
    int ifIndex = -1;
    void *pBuffer;

    vxlReportEvent("addLocalIpAddr\n");
    struct ifaddrs *pAdapter = openAdapter(interfaceName, &pBuffer, &ifIndex);
    if (pAdapter != NULL)
    {
    	char szIpAddress[100];
    	vxsMakeDottedAddr(szIpAddress, ntohl(((struct sockaddr_in *) pAdapter->ifa_addr)->sin_addr.s_addr));

        vxlReportEvent("Open Adapter interfaceName=%s Retval name = %s Addr=%s Idx=%d\n",
    	        		interfaceName, pAdapter->ifa_name,
    	        		szIpAddress, ifIndex);
        struct ifaddrs *pAddr = findAddress((struct ifaddrs *)pBuffer,
        		pAdapter->ifa_name, szLocalIpAddr);
        if (pAddr == NULL)
        {
            vxlReportEvent("Address %s not found in %s, adding it at index: %d\n",
            		szLocalIpAddr, pAdapter->ifa_name, ifIndex);
            if (!AddIPAddress(szLocalIpAddr, szSubnetMask, ifIndex, pAdapter->ifa_name))
            {
                /* Variables used to return error message */
                vxlReportEvent("Failed to add Address %s subnetMask %s to interface %s\n",
                		szLocalIpAddr, szSubnetMask, interfaceName);
             }
            else
            {
                vxlReportEvent("Successfully added Address %s subnetMask %s to interface %s\n",
                		szLocalIpAddr, szSubnetMask, interfaceName);
                success = TRUE;
            }
        }
        else
        {
            // Already exists - OK..
            vxlReportEvent("Address %s already exists\n", szLocalIpAddr);
            success = TRUE;
        }
        // Free the data which dynamically allocated by the openAdapter() via getifaddrs()
        freeifaddrs((struct ifaddrs *)pBuffer);
    }
    return success;
}

/******************************************************************************
  Function:  IzoTBindToAvailUnicastAddress
   
  Summary:
    Utility function on linux to bind the specified socket to any available 
    IP address on the specified interface.

  Parameters:
    socket:             The open socket bind
    ipInterfaceName:    The name of the IP interface that the socket uses
    pSocketAddr:        Allocated socket address.  Must be freed by caller
                        using vxsFreeSockaddr.

  Return:
    True if successful, false otherwise.
*****************************************************************************/
bool IzoTBindToAvailUnicastAddress(VXSOCKET socket, const char *ipInterfaceName, VXSOCKADDR *pSocketAddr)
{
    bool success = false;
    void *pBuffer;
    int ifIndex;    
    
    *pSocketAddr = NULL;

    vxlReportEvent("IzoTBindToAvailUnicastAddress\n");
    struct ifaddrs *pAdapter = openAdapter(ipInterfaceName, &pBuffer, &ifIndex);
    if (pAdapter != NULL)
    {
        if (IzoTBindToAddress(socket, ntohl(((struct sockaddr_in *) pAdapter->ifa_addr)->sin_addr.s_addr),
            		gLsUdpPort, &pSocketAddr))
        {
		    success = true;
        }
	}

    if (!success)
    {
        vxlReportUrgent("ERROR: IzoTBindToAvailUnicastAddress failed, interface = '%s socket = %d'\n",
        		ipInterfaceName, socket);
    }
    return success;
}

#endif  // #ifdef WIN32

/******************************************************************************
  Function:  IzoTBindToAddress
   
  Summary:
    Bind a socket to a specified IPV4 address
    and port and return socket address.

  Parameters:
    socket:                 The open socket bind
    socketInAddr:           The IP address the socket should be bound to
    port:                   The port to bind the socket to.
    pSocketAddr:            Allocated socket address.  Must be freed by caller
                            using vxsFreeSockaddr.
  Return:
    True if successful, false otherwise.
*****************************************************************************/
bool IzoTBindToAddress(VXSOCKET socket, ULONG socketInAddr, unsigned short port, VXSOCKADDR **pSocketAddr)
{
    bool success = false;
    VXSOCKADDR socketAddr;
    char szTempIpAddress[100];

    vxsMakeDottedAddr(szTempIpAddress, socketInAddr);
    vxlReportEvent("IzoTBindToAddress: Socket=%d Addr=%s Port=%d\n",
    		socket, szTempIpAddress, port);

    socketAddr = vxsAddrValue( socketInAddr );
    if (socketAddr != NULL)
    {
        STATUS vxsts = 0;
        vxsSetPort(socketAddr, port);

#ifndef WIN32
        /*  Need to do this, otherwise no other sockets can be bound using different address with the same port */
        /*  REMINDER:  In Windows this doesn't seem to work - it just allows stealing the port from someone else */
        vxsts = vxsAllowUsingSamePort(socket);
#endif
        if (vxsts == 0)
            vxsts = vxsBind(socket, socketAddr);
        else
    		vxlReportEvent("IzoTBindToAddress: Failed to set socket: %d option to allow multiple sockets to use the same PORT number\n",
    					socket);

        if (vxsts == 0)
        {
            vxlReportEvent("IzoTBindToAddress: succeeded, socket = '%d\n", socket);
            **pSocketAddr = socketAddr;
            success = true;
        }
        else
        {
            vxsFreeSockaddr(socketAddr);
        }
    }
    return success;
}

/******************************************************************************
  Function:  IzoTSetUnicastAddress
   
  Summary:
    Bind the specified socket to the specified IP address.
    If ipManagementOptions has the LONLINK_IZOT_MGMNT_OPTION_SET_IP_ADDR option
    set, attempt to add the IP address to the interface.  

    Adding the IP address is temporary, and disables DHCP for
    the interface until the PC reboots or DHCP is restored some other way.
    
    On Windows, this function also adds the IP address to the device driver
    registry so that the OpenLDV control panel can see which IP addresses
    should be added to the interface.

  Parameters:
    niName:                 The IzoT interface name
    ipManagementOptions:    A bit mask of LONLINK_IZOT_MGMNT_OPTION_* options
    socket:                 The open socket bind.
    ipInterfaceName:        The name of the IP interface that the socket uses
    szIpAddress:            The dotted IP address to bind to the socket.

  Return:
    True if successful, false otherwise.
*****************************************************************************/
bool IzoTSetUnicastAddress(const char *niName, int ipManagementOptions, VXSOCKET socket,
                           const char *ipInterfaceName, const char *szIpAddress)
{
    bool success = false;
    VXSOCKADDR localAddr = NULL;

    vxlReportEvent("IzoTSetUnicastAddress\n");

#ifdef WIN32
    // Update the registry to indicate that the IzoT interface would *LIKE* to use this
    // IP address.
    IzoTSetUnicastReg(niName, szIpAddress);
#endif

    // Convert string version of IP address to VXSOCKADDR
    localAddr = vxsAddrValue(vxsInetAddr((LPSTR)szIpAddress));
    if (localAddr != NULL)
    {
        STATUS vxsts;

        // Set the port to the LS UDP port
        vxsSetPort(localAddr, gLsUdpPort);
        
#ifndef WIN32
        // allow multiple sockets to use the same PORT number
        // Otherwise we won't be able to bind the socket
        // REMINDER:  In Windows this doesn't seem to work - it just allows stealing the port from someone else
        vxsAllowUsingSamePort(socket);  
#endif

        vxlReportEvent("Try to bind socket %d to %s\n", socket, szIpAddress);

        // Try to bind
        vxsts = vxsBind(socket, localAddr);
        if (vxsts != OK)
        {
            // Failed
        	vxlReportEvent("Not successful, need to add local IP Addr\n");

            // If the LONLINK_IZOT_MGMNT_OPTION_SET_IP_ADDR option is set, 
            // try adding the IP interface
            if ((ipManagementOptions&LONLINK_IZOT_MGMNT_OPTION_SET_IP_ADDR) &&
            		addLocalIpAddr(szIpAddress, "255.255.255.0", ipInterfaceName))
            {
                int i;
                // Try binding several times, as it may take a while before the IP address
                // for binding, while Windows is doing Duplicate Address Detection
                for (i = 0; vxsts != OK && i < MAX_IZOT_BINDING_RETRIES; i++)
                {
                    vxsts = vxsBind(socket, localAddr);
                    if (vxsts != OK)
                    {
#ifdef WIN32
						Sleep(IZOT_BINDING_SLEEP_TIME);
#else
                        sleep(IZOT_BINDING_SLEEP_TIME);
#endif
                    }
                }
            }
        }

        // Free the address
        vxsFreeSockaddr(localAddr);
        if (vxsts == 0)
        {
            success = true;
        }
    }

    if (success)
    {
    	vxlReportEvent("Successfully bound IzoT socket to '%s'\n", szIpAddress);
    }
    else
    {
    	vxlReportEvent("Failed to bind IzoT socket to '%s'\n", szIpAddress);
    }
    return success;
}


#ifdef WIN32

/******************************************************************************
  Function:  IzoTDeleteUnicastReg
   
  Summary:
    On windows, delete all unicast IP addresses from the registry

  Parameters:
    niName:                 The IzoT interface name
*****************************************************************************/
void IzoTDeleteUnicastReg(const char *niName)
{
    HKEY key;
    char keyName[1000];

    // Key name include the niName
    sprintf(keyName,  IZOT_REG_FORMAT_STRING, niName);

    // Open the registry for delete and enumerate.  
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyName, 0, DELETE|KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
    {
        LONG sts = ERROR_SUCCESS;
        int i = 0;

        // Enumerate each of the keys and delete them. 
        // Note that Vista and later have a RegDeleteTree, but XP does not.
        // Limit of 1000 just in case this process never completes.
        for (i = 0; sts == ERROR_SUCCESS && i < 1000; i++)
        {
            char keyName[1000];
            sts = RegEnumKey(key, 0, keyName, sizeof(keyName));
            if (sts == ERROR_SUCCESS)
            {
                sts = RegDeleteKey(key, keyName);
            }
        }
        RegCloseKey(key);
    }
}

/******************************************************************************
  Function:  IzoTSetUnicastReg
   
  Summary:
    On windows, add the IP address to the device driver registry

  Parameters:
    niName:                 The IzoT interface name
    szIpAddress:            The dotted IP address to bind to the socket.
                            using vxsFreeSockaddr.
*****************************************************************************/
void IzoTSetUnicastReg(const char *niName, const char *szIpAddress)
{
    DWORD   disposition;

    HKEY key;
    char keyName[1000];

    // Form the key name using the niName and the IP address
    sprintf(keyName,  IZOT_REG_FORMAT_STRING "\\%s", niName, szIpAddress);

    // See if it already exists
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyName, 0, KEY_READ|KEY_WRITE, &key) != ERROR_SUCCESS)
    {
        // Doesn't already exist.  Create it.
        RegCreateKeyEx(HKEY_LOCAL_MACHINE, keyName, 0, REG_CLASS, REG_OPTION_NON_VOLATILE,
                KEY_READ|KEY_WRITE, NULL, &key, &disposition);
    }
    RegCloseKey(key);
 }
#endif

#endif  // #if FEATURE_INCLUDED(IZOT)
