IzoT(tm) SDK Change History
===========================
*June 2015*

Following is a summary of the major changes implemented in each release of the IzoT SDK and IzoT Router software.  The following releases are described:

- 2.00.28 2015-07-16 Release 2
- 1.02.28 2014-10-27 Release 1
- 1.01.25 2014-03-28 Beta 2
- 1.00.00 2013-10-28 Beta 1

Starting with Release 2, the editions and software that each change applies to is identified.  They are the IzoT SDK Standard Edition (SE), IzoT SDK Premium Edition (PE), and the IzoT Router software.

# 2.00.28 2015-07-16 Release 2 #

- Support for IP-852 and Remote Network Interfaces (RNIs—PE and IzoT Router)  
- Support for multi-channel routing, with support for up to 6 routers, each routing
from a LAN IP-70 channel to up to five LON channels and one IP-852 channel (PE and
IzoT Router)  
- Simplified custom Web page development with a new IzoT Vision tool (SE, PE, and IzoT
Router)  
- Device interface (XIF) file generation for IzoT Python (SE, PE, and IzoT Router)
- IzoT Server support for SNVT_str_ascii and other string types (SE, PE, and IzoT Router)
- New server device for CPU utilization, memory usage, and disk usage (SE, PE, and
IzoT Router)  
- Device browser updates to optionally show unique ID, type, and program ID in the
device tabs (SE. PE, and IzoT Router)  
- Sorting options and manual ordering support in the device browser (SE, PE, and IzoT
Router)  
- Data export from the device browser (SE, PE, and IzoT Router)  
- Enhanced multi-browser support (SE, PE, and IzoT Router)

# 1.02.28 2014-10-27 Release 1 #

- First public production release of the IzoT SDK

## IzoT SDK Installation ##

- Created ready-to-run Raspberry Pi image
- Created BeagleBone Black flasher image
- Added Premium Edition with support for custom routers and network
interfaces

## IzoT SDK Tools ##

- Added version number output to the izot_stat command

## IzoT SDK Documentation ##

- Updated the documentation

## IzoT Device Stack ##

- Added support for LonTalk(R)/IP Enhanced Mode--this mode increases
the transaction ID size to support high-transaction-rate peer-to-peer
connections
- Improved integration with DHCP servers
- Added automatic IP interface selection
- Added support for ISI manual connections
- Added missing iotAlarm to the iotNodeObject profile
- Fixed problem with property persistence

## IzoT Server Stack ##

- Fixed the channel type reported by the network server
- Improved marginal and offline state reporting
- Added read-only datapoint support
- Added configurable poll rate multipliers
- Increased maximum device class file size
- Changed type name for the Environment Sensor and Keypad device class
files
- Added device class file for routers
- Added local host restriction for the network server for improved
security

## IzoT Server REST API ##

- Added support for categories, including support for AND, OR, and NOT
- Added After filter

## IzoT Server Web Pages ##

- Fixed IE9 compatibility problem for Charts page

## IzoT Router ##

- Added IzoT Router to the IzoT SDK Premium Edition
- Enable IzoT Router and IzoT Server to run on the same device

## IzoT Network Interface ##
- Added SMIP Serial Interface code and driver to the IzoT SDK
Premium Edition

## Examples ##

- Added Wink support to Keypad example
- Fixed device naming support in the example device class files
- Added eight configurable scenes to the LED Controller device class 
file
- Added command line flag to set the number of scenes in the LED 
Controller
- Added legacy SNVT_switch input to the LED Controller
- Added occupancy mode setting to the Environment Sensor
- Fixed example crashes in the Environment Sensor
- Fixed Connect button polarity for the example device applications
- Fixed DIO device class file problem


# 1.01.25 2014-03-28 Beta 2  #

## IzoT Device Stack ##

- Added support for the LonTalk/IP protocol--Beta 1 used an enhanced
version of IP-852 (ISO/IEC 14908-4); LonTalk/IP uses UDP/IP as the 
Transport Layer and has a protocol limit of 32,385 devices per network; 
IP-852 tunnels ISO/IEC 14908-1 packets over UPD/IP with a protocol limit 
of 255 devices per channel

## IzoT Server Stack ##

- Added capability to fetch device names from devices
- Added support for event-driven updates for datapoint updates using
domain and group broadcast service

## IzoT Server REST API ##

- Added batch command support
- Added time-based filters

## IzoT Server Web Pages ##

- Added general-purpose charting page
- Enhanced cross browser compatibility

## Examples ##

- Added alarm detection and output to the Environment Sensor
- Added multi-keypad synchronization to the Keypad
- Added Service button support to the Keypad
- Enhanced reliability of key press detection for the Keypad
- Added new Multi DIO example to provide a simplified example that uses
commercially available hardware (the PiFace Digital I/O Board)


# 1.00.00 2013-10-28 Beta 1 #

- First public beta release of the IzoT SDK


# Copyright #

Use of this code is subject to your compliance with the terms of the
Echelon IzoT(tm) Software Developer's Kit License Agreement which is
available in the **LICENSE.md** file in your **izot_sdk** directory.

Copyright (C) 2013-2014 Echelon Corporation. The Echelon logo, Echelon, 
LON, LONWORKS, Digital Home, i.LON, LNS, LONMARK, LonMaker, LonPoint, 
LonTalk, Neuron, NodeBuilder, Pyxos, and ShortStack, are trademarks 
of Echelon Corporation registered in the United States and other 
countries. A full list of Echelon trademarks is available at 
<http://www.echelon.com>. All other trademarks are owned by their 
respective owners. All rights reserved.
