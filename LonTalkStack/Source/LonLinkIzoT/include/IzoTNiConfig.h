
/***************************************************************
 *  Filename: IzoTNiConfig.h
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
 *  Description:  Header file for manage IzoT network interfaces
 *    configuration.
 *
 ****************************************************************/

#if !defined(IZOTNICONFIG_H)
#define IZOTNICONFIG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <VxClass.h>
#include "VxSockets.h"

#ifdef WIN32
#define ETH_ADAPTER_NAME "Local Area Connection"
#define WIFI_ADAPTER_NAME "Wireless Network Connection"
#else
#define ETH_ADAPTER_NAME "eth"
#define WIFI_ADAPTER_NAME "wlan"
#endif

#define IZOT_REG_FORMAT_STRING "Software\\LonWorks\\DeviceDrivers\\%s\\IpAddresses"

// LONLINK_IZOT_MGMNT_OPTION_* option bits
#define LONLINK_IZOT_MGMNT_OPTION_SET_IP_ADDR       0x0001
#define LONLINK_IZOT_MGMNT_OPTION_RETRY_BINDING     0x0002
#define LONLINK_IZOT_MGMNT_OPTION_TRACE_MSGS        0x8000

/******************************************************************************
  Function:  IzoTSetUnicastAddress
   
  Summary:
    Bind the specified socket to the specified IP address.  If 
    ipManagementOptions has the LONLINK_IZOT_MGMNT_OPTION_SET_IP_ADDR option
    set, attempt to add the IP address to the interface.  

    On Windows, adding the IP address is temporary, and disables DHCP for
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
                           const char *ipInterfaceName, const char *szIpAddress);

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
bool IzoTBindToAvailUnicastAddress(VXSOCKET socket, const char *ipInterfaceName,
            VXSOCKADDR *pSocketAddr);

/******************************************************************************
  Function:  IzoTDeleteUnicastReg
   
  Summary:
    On windows, delete all unicast IP addresses from the registry

  Parameters:
    niName:                 The IzoT interface name
*****************************************************************************/
void IzoTDeleteUnicastReg(const char *niName);

/******************************************************************************
  Function:  IzoTSetUnicastReg
   
  Summary:
    On windows, add the IP address to the device driver registery

  Parameters:
    niName:                 The IzoT interface name
    szIpAddress:            The dotted IP address to bind to the socket.
                            using vxsFreeSockaddr.
*****************************************************************************/
void IzoTSetUnicastReg(const char *niName, const char *szIpAddress);

/******************************************************************************
  Function:  IzoTBindToAddress
   
  Summary:
    Utility function on linux to bind a socket to a specified IPV4 address
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
bool IzoTBindToAddress(VXSOCKET socket, ULONG socketAddr, unsigned short port, VXSOCKADDR **pSocketAddr);

#endif	// IZOTNICONFIG_H
