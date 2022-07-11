/*****************************************************************************
 *  Filename: IzoTDevSocketMaps.h
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
 *  Description:  Header file for a collection of classes
 *                used to manage IzoT sockets for multiple devices
 *                on a given link.  Each domain/subnet/node address
 *                of device is associated with a socket.  Ideally 
 *                the socket is bound to an IP address derived from
 *                the LS address.  If that is not possible the
 *                socket will remain unbound, and when sending 
 *                a message with that LS source an arbitrary
 *                socket bound to an aribitrary IP address will
 *                be used.
 *                
 *                The first socket is special, and is bound to
 *                group and broadcast LS addresses, based on the
 *                LS subnet and group membership of the devices 
 *                using the link.
 *
 ****************************************************************************/

#if !defined(IZOTDEVSOCKETMAPSS_H)
#define IZOTDEVSOCKETMAPSS_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "LonLinkIzoT.h"
#include <assert.h>
#include "LtDomain.h"
#include "LtRouteMap.h"

#define IZOT_NULL_SOCKET_INDEX -1

#define IZOT_MAX_IP_ADDR_SIZE 4 // REMINDER: IPV6 must be 16.

// 3 chars for each byte, plus one for a 'dot' for max bytes-1, plus a terminator
#define IZOT_MAX_SZ_IP_ADDR_SIZE (4*IZOT_MAX_IP_ADDR_SIZE)  

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   IzotUnicastAddress
//  Summary:
//      A class to hold a unicast address. This address may be used by multiple
//      devices (for example, the 0/0 address), so so a use count is maintained.
//
///////////////////////////////////////////////////////////////////////////////
class IzotUnicastAddress
{
public:
    IzotUnicastAddress();
    ~IzotUnicastAddress();

    // Set the address in network byte order.
    void set(const byte *ipAddress, int ipAddrLen);

    // Increment the use count and return the count
    int increment(void);

    // Decrement the use count and return the count.  If the use count
    // reaches 0, clear contents.
    int decrement(void);

    // Clear the contents of this object.
    void close(void);

    // compare the specified ipAddress with this one, and return true if they 
    // match.
    boolean match(const byte *ipAddress, int ipAddrLen);
    int getUseCount() { return m_useCount; }
    bool getIsBound() { return m_isBound; }
    // See if the address needs rebinding - that is, its in use and is not
    // currently bound.
    bool getRebind() { return m_useCount !=0 && !m_isBound; }
    void setIsBound(bool isBound) { m_isBound = isBound; }

    // Get the IP address in dotted format
    const char* getSzIpAddress(void) { return m_szIpAddress; }

    // Get the IP address in network byte order
    const byte *getIpAddress(void)   { return m_ipAddress; }

    // Copy the ip address in network byte order to a buffer. Return true if successful
    bool getIpAddress(void *pAddr, int bufferSize);

    // Return IPV4 address as a ULONG, or 0.
    ULONG getIpv4Address(void);

private:
    bool m_isBound;     // IP address is currently bound to a socket
    int m_useCount;     // Number of devices using this address.
    int m_ipAddrLen;    // length of address.
    byte m_ipAddress[IZOT_MAX_IP_ADDR_SIZE];        // IP address in network order
    char m_szIpAddress[IZOT_MAX_SZ_IP_ADDR_SIZE];   // dotted string format of IP address
};

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   IzotUnicastAddresses
//  Summary:
//      A map of unicast addresses. The LonLinkIzoTDev class uses this map to 
//      keep track of the unicast addresses associated with each socket. The 
//      index is the same as the index to the LonLinkIzot socket array.  If
//      possible the address will be bound to the socket, but if the bind fails
//      the address still refers to that socket - but the socket will not be
//      useable.
///////////////////////////////////////////////////////////////////////////////
class IzotUnicastAddresses
{
public:

    /******************************************************************************
      Method:  IzotUnicastAddresses
       
      Summary:
        IzotUnicastAddresses constructor. Allocates an array of 
        count IzotUnicastAddress objects.  This array can grow dynamically as
        needed.

      Parameters:
        lock:        A lock to protect this data.
        count:       The initial number of entries in the IzotUnicastAddress array
        reallocSize: The number of entries to increase the array by if it needs 
                     to grow.
    *****************************************************************************/

    IzotUnicastAddresses(SEM_ID lock, int count=4, int reallocSize = 4);
    ~IzotUnicastAddresses(void);

    // Return the socket index corresponding to the specified ipAddress
    // or IZOT_NULL_SOCKET_INDEX.
    int find(const byte *ipAddress, int ipAddrLen);

    // Set the IP address in network byte order for the specified socket index.  
    // The array may grow as a result.
    void set(int socketIndex, const byte *ipAddress, int ipAddrLen);
    // Set the IP address in VXSOCKADDR format for the specified socket.  
    // The array may grow as a result.
    void set(int socketIndex, VXSOCKADDR psad);

    // Get the unicast address for the specified socket index, or NULL.
    IzotUnicastAddress *get(int socketIndex);

    // Remove the ipAddress from the map.
    int remove(const byte *ipAddress, int ipAddrLen);

    // Close the map
    void close(void);

    // Return true if the address at socketIndex is properly bound to the 
    // socket at socketIndex
    bool getIsBound(int socketIndex);

    // Return true if the address at socketIndex is in use but is NOT
    // properly bound to the socket at socketIndex
    bool getRebind(int socketIndex);

    // Indicate wheter the address at socketIndex is properly bound to 
    // the socket at socketIndex
    void setIsBound(int socketIndex, bool isBound);


    // Get the IP address in network byte order at socketIndex
    const byte *getIpAddress(int socketIndex);
    // Copy the ip address at socketIndex in network byte order to a buffer. Return true if successful
    bool getIpAddress(int socketIndex, void *pAddr, int bufferSize);

    // Get the IP address at socket index in dotted format
    const char *getSzIpAddress(int socketIndex);
    // Copy the ip address at socket index in dotted format into a buffer. Return true if successful
    bool getSzIpAddress(int socketIndex, char *pSzAddr, unsigned int bufferSize);

    // Return the IPV4 address at socketIndex as a ULONG, or 0.
    ULONG getIpv4Address(int socketIndex);

    // Get array size of the map
    int getNumEntries(void) { return m_numEntries; }

    // Return true if the socketIndex falls within the map.
    bool validSocketIndex(int socketIndex);

private:
    int                 m_numEntries;   // Number of elements in the array map
    int                 m_reallocSize;  // Number of elmenets to increase the array by
    IzotUnicastAddress *m_map;          // The map
    SEM_ID              m_lock;         // The lock to protect everything.

};

// 
///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   IzoTConfigListEntry
//  Summary:
//      Utitility base class to hold an IzoT config list entry.  Each entry
//      is a member of a list and is furthur identified by an integer ID.
//
///////////////////////////////////////////////////////////////////////////////
class IzoTConfigListEntry
{
public:
    // Create an entry with the specified IP and link it to pNext.
    IzoTConfigListEntry(int id, IzoTConfigListEntry *pNext);
    virtual ~IzoTConfigListEntry() {};

    // return TRUE if the specified ID matches the ID of the config entry
    boolean match(int id) { return id == m_id; }

    // Get the next config entry.
    IzoTConfigListEntry *next(void) { return m_pNext; }

    // Insert p after this entry
    void setNext(IzoTConfigListEntry *p) { m_pNext = p; }

private:
    int m_id;                       // The configuration identifier
    IzoTConfigListEntry *m_pNext;   // The next entry in the list.
};

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   IzoTConfigListEntries
//  Summary:
//      Utitility base class to maintain a list of IzoTConfigListEntrys
//
///////////////////////////////////////////////////////////////////////////////
class IzoTConfigListEntries
{
public:
    IzoTConfigListEntries(SEM_ID lock);
    virtual ~IzoTConfigListEntries();

    // Find and return the config entry with the specified ID, or NULL;
    IzoTConfigListEntry *find(int id);
    // Find the first config entry in the list
    IzoTConfigListEntry *findFirst(void)  { return m_pHead; }
    // Add an entry to the list and return a pointer to it
    IzoTConfigListEntry *add(int id);

    // Remove the entry with the specified ID from the list
    void remove(int id);

    // Remove the specified entry from the list
    void remove(IzoTConfigListEntry *pEntry);
    
    // Close the list, freeing all entries
    void close(void);
protected:
    // Pure virtual function to create IzoTConfigListEntry and insert it at the head of the list
    // Classes derived from IzoTConfigListEntry must implement this, create the appropriate
    // class derived from a IzoTConfigListEntry and return a point to it.
    virtual void createEntry(int id) = 0;

    // Remove the specified entry from the list, wher pPrev is the previous
    // in the list or NULL;
    void remove(IzoTConfigListEntry *p, IzoTConfigListEntry *pPrev);

    // A lock to maintain the integrity of this object
    SEM_ID              m_lock;

    // The head of the list
    IzoTConfigListEntry *m_pHead;
};


///////////////////////////////////////////////////////////////////////////////
// 
//  Template:   template<class EntryType> class IzoTConfigList
//  Summary:
//      A template for an configuration list type EntryType.  EntryType must
//      be derived from IzoTConfigListEntry
//
///////////////////////////////////////////////////////////////////////////////
template<class EntryType> class IzoTConfigList : public IzoTConfigListEntries
{
public:
    IzoTConfigList(SEM_ID lock) : IzoTConfigListEntries(lock) {};
    ~IzoTConfigList() {}

    // Method to create a configuration list entry of type EntryType and insert it
    // in the list
    virtual void createEntry(int id) 
    {
        m_pHead = new EntryType(m_lock, id, static_cast<EntryType *>(m_pHead));
    }

    // Find the config entry with that matches the specified ID an return  a pointer to it
    EntryType *find(int id) { return static_cast<EntryType *>(IzoTConfigListEntries::find(id)); }

    // Find the first config entry in the list.
    EntryType *findFirst(void)  { return static_cast<EntryType *>(m_pHead); }

    // Add a config entry with the specified id and return a pointer to the entry.
    EntryType *add(int id) { return static_cast<EntryType *>(IzoTConfigListEntries::add(id)); }
}; 

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   IzoTDevSubnetNodeConfig
//  Summary:
//      Class used to map the configuration for a particular subnet/node of
//      a devices domain to a socketIndex.  The configuration is identified
//      by a subnetNodeIndex, and is a member of a list owned by a
//      IzoTDevDomainConfig.  Note that typically devices only have one
//      subnetNode per domain, but the list is required to support alternate
//      subnetNode configuration used by LNS NSDs.
///////////////////////////////////////////////////////////////////////////////
class IzoTDevSubnetNodeConfig : public IzoTConfigListEntry
{
public:
    // Note that the lock is unused but required by the IzoTConfigListEntry base class
    IzoTDevSubnetNodeConfig(SEM_ID dummy, int subnetNodeIndex, IzoTDevSubnetNodeConfig *pNext);
    ~IzoTDevSubnetNodeConfig() {};

    // set the socketIndex used for this configuration
    void setSocketIndex(int index) { m_socketIndex = index; }

    // get the socket index used for this configuration
    int getSocketIndex(void) { return m_socketIndex; }

    // get the next one in the list
    IzoTDevSubnetNodeConfig *next(void) { return static_cast<IzoTDevSubnetNodeConfig *>(IzoTConfigListEntry::next()); }

private:
    int m_socketIndex;  // The sockeIndex used to map this to a unicast address and socket.
};

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   IzoTDevDomainConfig
//  Summary:
//      Class used to contain a list of IzoTDevSubnetNodeConfig objects for
//      a specified device and domain configuration, and the LS groups that
//      this domain is a member of.
//
//      The configuration is identifed by a domainIndex, and is a member of a 
//      list owned by a IzoTDevConfig.
///////////////////////////////////////////////////////////////////////////////
class IzoTDevDomainConfig : public IzoTConfigListEntry
{
public:
    IzoTDevDomainConfig(SEM_ID lock, int domainIndex, IzoTDevDomainConfig *pNext);
    ~IzoTDevDomainConfig() {};

    // Get the list of subnet/node configuration used by this domain
    IzoTConfigList<IzoTDevSubnetNodeConfig> &getSubnetNodes(void) { return m_subnetNodes; }

    // Get the next domain config in the list
    IzoTDevDomainConfig *next(void) { return static_cast<IzoTDevDomainConfig *>(IzoTConfigListEntry::next()); }

    // Get the groups used by this domain
    LtGroups &getGroups(void) { return m_groups; }

private:
    // A list of subnet/node configuration used by this domain
    IzoTConfigList<IzoTDevSubnetNodeConfig> m_subnetNodes; 
    // A bitmap of groups used by this domain
    LtGroups m_groups;
};

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   IzoTDevConfig
//  Summary:
//      The configuration for a given IzoT Device using the link.
//
//      The configuration is identifed by a stack index, and is a member of a 
//      list owned by a IzoTDevConfigMap.
///////////////////////////////////////////////////////////////////////////////
class IzoTDevConfig : public IzoTConfigListEntry
{
public:
    IzoTDevConfig(SEM_ID lock, int stackIndex, IzoTDevConfig *pNext);
    ~IzoTDevConfig() {}

    // Get the list of domain configuration used by this device
    IzoTConfigList<IzoTDevDomainConfig> &getDomains(void) { return m_domains; }

    // Get the next device configuration in the IzoTDevConfigMap.
    IzoTDevConfig *next(void) { return static_cast<IzoTDevConfig *>(IzoTConfigListEntry::next()); }
    
    void setLsAddrMappingConfig(ULONG lsAddrMappingAnnounceFreq, 
                                WORD lsAddrMappingAnnounceThrottle, 
                                ULONG lsAddrMappingAgeLimit)
    {
        m_lsAddrMappingAnnounceFreq = lsAddrMappingAnnounceFreq;
        m_lsAddrMappingAnnounceThrottle = lsAddrMappingAnnounceThrottle;
        m_lsAddrMappingAgeLimit = lsAddrMappingAgeLimit;
    }

    ULONG getLsAddrMappingAnnounceFreq(void) { return m_lsAddrMappingAnnounceFreq; }
    WORD  getLsAddrMappingAnnounceThrottle(void) { return m_lsAddrMappingAnnounceThrottle; }
    ULONG getLsAddrMappingAgeLimit(void) { return m_lsAddrMappingAgeLimit; }

private:
    // A list of domain configuration used by this device
    IzoTConfigList<IzoTDevDomainConfig> m_domains;
    ULONG m_lsAddrMappingAnnounceFreq;
    ULONG m_lsAddrMappingAgeLimit;
    WORD m_lsAddrMappingAnnounceThrottle;
};

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   IzoTDevConfigMap
//  Summary:
//      A map (or list) of the device configuration for all devices using
//      this link.
///////////////////////////////////////////////////////////////////////////////
class IzoTDevConfigMap
{
public:
    IzoTDevConfigMap(SEM_ID lock);
    ~IzoTDevConfigMap() {};

    // Get the first device config in the list.
    IzoTDevConfig *findFirst(void)       { return m_devices.findFirst(); }

    // Find the device config for the specified stack index
    IzoTDevConfig *find(int stackIndex) { return m_devices.find(stackIndex); }

    // Add a device config to the map and return a pointer to it.
    IzoTDevConfig *add(int stackIndex)  { return m_devices.add(stackIndex); }
    // Remove a device config frome the map.
    void remove(int stackIndex)         { m_devices.remove(stackIndex); }

    // Find the domain config for a given device and domain index.
    IzoTDevDomainConfig *find(int stackIndex, int domainIndex);
    // Add domain config for a given device and domain index
    IzoTDevDomainConfig *add(int stackIndex, int domainIndex);
    // Remove the domain config for a given device and domain index
    void remove(int stackIndex, int domainIndex);

    // Find the subnet node config for a given device and domain index, and subnet/node index
    IzoTDevSubnetNodeConfig *find(int stackIndex, int domainIndex, int subnetNodeIndex);
    // Add  subnet node config for a given device and domain index, and subnet/node index
    IzoTDevSubnetNodeConfig *add(int stackIndex, int domainIndex, int subnetNodeIndex);
    // Remove subnet node config for a given device and domain index, and subnet/node index
    void remove(int stackIndex, int domainIndex, int subnetNodeIndex);

    // Close the configuration
    void close(void) { m_devices.close(); }
private:

    // A list of device configuration
    IzoTConfigList<IzoTDevConfig> m_devices;
};

#endif