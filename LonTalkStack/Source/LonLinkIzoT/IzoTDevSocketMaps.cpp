/*****************************************************************************
 *  Filename: IzoTDevSocketMaps.cpp
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
 *  Description:  Implementation for a collection of classes used to manage 
 *                IzoT sockets for multiple devices on a given link:
 *
 *      IzotUnicastAddress:	    Holds a unicast address.
 *  
 *      IzotUnicastAddresses:   A map of unicast addresses. The LonLinkIzoTDev
 *                              class uses this map to keep track of the 
 *                              unicast addresses bound to each socket. The map
 *                              index is the same as the index to the LonLinkIzot
 *                              socket array. 
 *
 *      IzoTDevSubnetNodeConfig: Maps the stack/domain/subenetNode indicies to
 *                                a socketIndex
 *  
 *      IzoTDevDomainConfig:    A list of IzoTDevSubnetNodeConfig objects used
 *                              by a paricular devices domain configuration.  Also
 *                              contains a bitmap of groups the domain is a member of.
 *
 *      IzoTDevConfig:          A list of IzoTDevDomainConfig objects used by a device
 *
 *      IzoTDevConfigMap:       A list of IzoTDevConfig for all devices using the link
 *
 ****************************************************************************/
#include "LtaDefine.h"
#if FEATURE_INCLUDED(IZOT)

#ifdef WIN32
#include <windows.h>
#else
#include <arpa/inet.h>
#endif
#include <string.h>

#include "IzoTDevSocketMaps.h"
#include "ipv6_ls_to_udp.h"

//////////////////////////////////////////////////////////////////////////////
//                      IzotUnicastAddress
//////////////////////////////////////////////////////////////////////////////

IzotUnicastAddress::IzotUnicastAddress()
{
    close();
}

IzotUnicastAddress::~IzotUnicastAddress()
{
    // No cleanup here.  Note that a IzotUnicastAddressMaps may
    // copy and delete IzotUnicastAddress as they are resized.
}

// Clear the contents of this object.
void IzotUnicastAddress::close(void)
{
    m_useCount = 0;
    m_ipAddrLen = 0;
    m_isBound = false;
    memset(m_ipAddress, 0, sizeof(m_ipAddress));
    memset(m_szIpAddress, 0, sizeof(m_szIpAddress));
}

// Set the address in network byte order.
void IzotUnicastAddress::set(const byte *ipAddress, int ipAddrLen)
{
    m_useCount++;   // Increment the use count
    m_ipAddrLen = ipAddrLen;
    memcpy(m_ipAddress, ipAddress, ipAddrLen);
    // Create the string version of the address
    vxsMakeDottedAddr(m_szIpAddress, ntohl(getIpv4Address()));
}

// Increment the use count and return the count
int IzotUnicastAddress::increment(void)
{
    m_useCount++;
    return m_useCount;
}

// Decrement the use count and return the count.  If the use count
// reaches 0, clear contents.
int IzotUnicastAddress::decrement(void)
{
    if (m_useCount)
    {
        m_useCount--;
        if (m_useCount == 0)
        {
            close();
        }
    }
    return m_useCount;
}

// compare the specified ipAddress with this one, and return true if they 
// match.
boolean IzotUnicastAddress::match(const byte *ipAddress, int ipAddrLen)
{
    return m_useCount && (m_ipAddrLen == ipAddrLen) && (memcmp(ipAddress, m_ipAddress, m_ipAddrLen) == 0);
}

// Return IPV4 address as a ULONG, or 0.
ULONG IzotUnicastAddress::getIpv4Address(void)
{
    if (m_ipAddrLen == IPV4_ADDRESS_LEN)
    {
        return *((ULONG *)m_ipAddress);
    }
    return 0;
}

// Copy the ip address in network byte order to a buffer. Return true if successful
bool IzotUnicastAddress::getIpAddress(void *pAddr, int bufferSize)
{
    bool ok = false;
    if (m_useCount && bufferSize >= m_ipAddrLen)
    {
        ok = true;
        memcpy(pAddr, m_ipAddress, m_ipAddrLen);
    }
    else
    {
        memset(pAddr, 0, bufferSize);
    }
    return ok;
}

//////////////////////////////////////////////////////////////////////////////
//                      IzotUnicastAddresses
//////////////////////////////////////////////////////////////////////////////

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
IzotUnicastAddresses::IzotUnicastAddresses(SEM_ID lock, int count, int reallocSize)
{
    m_lock = lock;
    m_numEntries = count;
    m_map = new IzotUnicastAddress[count];
    m_reallocSize = reallocSize;
}

IzotUnicastAddresses::~IzotUnicastAddresses(void)
{
    delete[] m_map;
}

// Return the socket index corresponding to the specified ipAddress
// or IZOT_NULL_SOCKET_INDEX.
int IzotUnicastAddresses::find(const byte *ipAddress, int ipAddrLen)
{
    LonLinkIzoTLock lock(m_lock);

    int i; 
    // Scan the map for a match
    for (i = 0; i < m_numEntries; i++)
    {
        if (m_map[i].match(ipAddress, ipAddrLen))
        {
            // Got it
            return i;
        }
    }
    return IZOT_NULL_SOCKET_INDEX;
}

IzotUnicastAddress *IzotUnicastAddresses::get(int socketIndex)
{
    LonLinkIzoTLock lock(m_lock);

    IzotUnicastAddress *p = NULL;
    if (validSocketIndex(socketIndex))
    {
        p = &m_map[socketIndex];
    }
    return p;
}

// Set the IP address in network byte order for the specified socket index.  
// The array may grow as a result.
void IzotUnicastAddresses::set(int index, const byte *ipAddress, int ipAddrLen)
{        
    LonLinkIzoTLock lock(m_lock);

    if (index >= m_numEntries)
    {
        // Need to grow. Make sure there is enough room for index plus some more
        IzotUnicastAddress *pTempMap = new IzotUnicastAddress[index + m_reallocSize];

        // Copy the old entries
        for (int i = 0; i < m_numEntries; i++)
        {
            pTempMap[i] = m_map[i];
        }
        delete[] m_map; // Done with the old map

        m_map = pTempMap;
        m_numEntries = index + m_reallocSize;
    }
    
    // At this point index must be in range.
    m_map[index].set(ipAddress, ipAddrLen);
}

void IzotUnicastAddresses::set(int socketIndex, VXSOCKADDR psad)
{
    LonLinkIzoTLock lock(m_lock);

    // REMINDER: This only works for IPV4
    ULONG ipv4Addr = htonl(vxsAddrGetAddr(psad));
    set(socketIndex, (byte *)&ipv4Addr, sizeof(ipv4Addr));
}

int IzotUnicastAddresses::remove(const byte *ipAddress, int ipAddrLen)
{
    LonLinkIzoTLock lock(m_lock);

    int index = find(ipAddress, ipAddrLen);
    if (index != IZOT_NULL_SOCKET_INDEX)
    {
        m_map[index].decrement();
    }
    return index;
}

void IzotUnicastAddresses::close(void)
{
    LonLinkIzoTLock lock(m_lock);

    for (int i = 0; i < m_numEntries; i++)
    {
        m_map[i].close();
    }
}

bool IzotUnicastAddresses::getIsBound(int socketIndex)
{
    LonLinkIzoTLock lock(m_lock);

    bool isBound = false;
    if (validSocketIndex(socketIndex))
    {
        isBound = m_map[socketIndex].getIsBound();
    }
    return isBound;
}

bool IzotUnicastAddresses::getRebind(int socketIndex)
{
    LonLinkIzoTLock lock(m_lock);

    bool rebind = false;
    if (validSocketIndex(socketIndex))
    {
        rebind = m_map[socketIndex].getRebind();
    }
    return rebind;
}

void IzotUnicastAddresses::setIsBound(int socketIndex, bool isBound)
{
    LonLinkIzoTLock lock(m_lock);

    if (validSocketIndex(socketIndex))
    {
        m_map[socketIndex].setIsBound(isBound);
    }
}

// Get the IP address in network byte order at socketIndex
const byte *IzotUnicastAddresses::getIpAddress(int socketIndex)
{
    const byte *pIpAddress = NULL;
    if (validSocketIndex(socketIndex))
    {
        // The index is in range - return the address
        pIpAddress = m_map[socketIndex].getIpAddress();
    }
    return pIpAddress;
}

// Copy the ip address at socketIndex in network byte order to a buffer. Return true if successful
bool IzotUnicastAddresses::getIpAddress(int socketIndex, void *pAddr, int bufferSize)
{
    LonLinkIzoTLock lock(m_lock);

    bool ok = false;
    memset(pAddr, 0, bufferSize);
    if (validSocketIndex(socketIndex))
    {
        // The index is in range - copy the address
        ok = m_map[socketIndex].getIpAddress(pAddr, bufferSize);
    }
    return ok;
}


// Get the IP address at socket index in dotted format
const char *IzotUnicastAddresses::getSzIpAddress(int socketIndex)
{
    const char *pSzIpAddress = NULL;
    if (validSocketIndex(socketIndex))
    {
        // The index is in range - return the address
        pSzIpAddress = m_map[socketIndex].getSzIpAddress();
    }
    return pSzIpAddress;
}

// Copy the ip address at socket index in dotted format into a buffer. Return true if successful
bool IzotUnicastAddresses::getSzIpAddress(int socketIndex, char *pSzAddr, unsigned int bufferSize)
{
    LonLinkIzoTLock lock(m_lock);

    const char *p = getSzIpAddress(socketIndex);
    if (p != NULL && bufferSize > strlen(p))
    {
        // The index is in range - copy the address
        strcpy(pSzAddr, p);
        return true;
    }
    return false;
}

// Return the IPV4 address at socketIndex as a ULONG, or 0.
ULONG IzotUnicastAddresses::getIpv4Address(int socketIndex)
{
    LonLinkIzoTLock lock(m_lock);

    ULONG ipAddress = 0;
    if (validSocketIndex(socketIndex))
    {
        // The index is in range
        ipAddress = m_map[socketIndex].getIpv4Address();
    }
    return ipAddress;
}

// Return true if the socketIndex falls within the map.
bool IzotUnicastAddresses::validSocketIndex(int socketIndex)
{
    return socketIndex != IZOT_NULL_SOCKET_INDEX && socketIndex < m_numEntries;
}

//////////////////////////////////////////////////////////////////////////////
//                      IzoTConfigListEntry
//////////////////////////////////////////////////////////////////////////////

// Utitility base class to hold an IzoT config list entry.
IzoTConfigListEntry::IzoTConfigListEntry(int id, IzoTConfigListEntry *pNext)
{
    m_id = id;
    m_pNext = pNext;
}

///////////////////////////////////////////////////////////////////////////////
//                      IzoTConfigListEntries
//////////////////////////////////////////////////////////////////////////////

// Utitility base class to maintain a list of IzoTConfigListEntrys
IzoTConfigListEntries::IzoTConfigListEntries(SEM_ID lock)
{
    m_pHead = NULL;
    m_lock = lock;
}

IzoTConfigListEntries::~IzoTConfigListEntries()
{
    close();
}

// Close the list, freeing all entries
void IzoTConfigListEntries::close(void)
{
    LonLinkIzoTLock lock(m_lock);

    // Delete all entries
    while (m_pHead)
    {
        // Get the one after the head
        IzoTConfigListEntry *pNext = m_pHead->next();
        // Delete the head
        delete m_pHead;
        // Set the head to the next one.
        m_pHead = pNext;
    }
}

// Find and return the config entry with the specified ID, or NULL;
IzoTConfigListEntry *IzoTConfigListEntries::find(int id)
{
    LonLinkIzoTLock lock(m_lock);

    IzoTConfigListEntry *p;

    // Search the linked list
    for (p = m_pHead; p != NULL && !p->match(id); p = p->next())
    {
    }
    // p either matches or is NULL
    return p;
}

// Add an entry to the list and return a pointer to it
IzoTConfigListEntry *IzoTConfigListEntries::add(int id)
{
    LonLinkIzoTLock lock(m_lock);

    // Create an entry and update the links. Note that the
    // createEntry inserts the new entry before the head, and
    // this function updates the head.
    createEntry(id);
    return m_pHead;
}

// Remove the entry with the specified ID from the list
void IzoTConfigListEntries::remove(int id)
{
    LonLinkIzoTLock lock(m_lock);

    IzoTConfigListEntry *p;
    IzoTConfigListEntry *pPrev = NULL;
    // Find the entry
    for (p = m_pHead; p != NULL && !p->match(id); p = p->next())
    {
        // Remember the previous one
        pPrev = p;
    }
    remove(p, pPrev);
}

void IzoTConfigListEntries::remove(IzoTConfigListEntry *pEntry)
{
    LonLinkIzoTLock lock(m_lock);
    
    IzoTConfigListEntry *p;
    IzoTConfigListEntry *pPrev = NULL;
    for (p = m_pHead; p != NULL && p != pEntry; p = p->next())
    {
        pPrev = p;
    }
    // Remove the entry.  Note that this method verifies that p is not NULL.
    remove(p, pPrev);
}

// Remove the specified entry from the list, wher pPrev is the previous
// in the list or NULL;
void IzoTConfigListEntries::remove(IzoTConfigListEntry *p, IzoTConfigListEntry *pPrev)
{
    LonLinkIzoTLock lock(m_lock);

    if (p != NULL)
    {
        // The entry exists
        if (pPrev == NULL)
        {
            // But no previous one - must be at the head
            m_pHead = p->next();
        }
        else
        {
            // Not at the head, just link the previous to this one's next
            pPrev->setNext(p->next());
        }
        // Done with p.
        delete p;
    }
}

//////////////////////////////////////////////////////////////////////////////
//                      IzoTDevSubnetNodeConfig
//////////////////////////////////////////////////////////////////////////////

// The unicast address for a particular devices address configuration
// identified by stackIndex, domainIndex and subnetNodeIndex (the subnetNodeIndex is 
// required to support alternate subnets on a given domain).
IzoTDevSubnetNodeConfig::IzoTDevSubnetNodeConfig(SEM_ID dummy, int subnetNodeIndex, IzoTDevSubnetNodeConfig *pNext) : 
    IzoTConfigListEntry(subnetNodeIndex, pNext)
{
    // Note that the lock is not used because this object does not include a list.  
    // However the parameter is required by the base class to support the pure virtual function
    // createEntry.

    // Not associated with any socket yet.
    m_socketIndex = IZOT_NULL_SOCKET_INDEX;
}

//////////////////////////////////////////////////////////////////////////////
//                      IzoTDevDomainConfig
//////////////////////////////////////////////////////////////////////////////
IzoTDevDomainConfig::IzoTDevDomainConfig(SEM_ID lock, int domainIndex, IzoTDevDomainConfig *pNext) :
    IzoTConfigListEntry(domainIndex, pNext), m_subnetNodes(lock)
{
}

//////////////////////////////////////////////////////////////////////////////
//                           IzoTDevConfig
//////////////////////////////////////////////////////////////////////////////


// The configuration for a given IzoT Device using the link
IzoTDevConfig::IzoTDevConfig(SEM_ID lock, int stackIndex, IzoTDevConfig *pNext) : 
    IzoTConfigListEntry(stackIndex, pNext), m_domains(lock)
{
    m_lsAddrMappingAnnounceFreq = 0;
    m_lsAddrMappingAnnounceThrottle = 0;
    m_lsAddrMappingAgeLimit = 0;
}

//////////////////////////////////////////////////////////////////////////////
//                           IzoTDevConfigMap
//////////////////////////////////////////////////////////////////////////////
IzoTDevConfigMap::IzoTDevConfigMap(SEM_ID lock) : m_devices(lock)
{
}

// Find the domain config for a given device and domain index.
IzoTDevDomainConfig *IzoTDevConfigMap::find(int stackIndex, int domainIndex)
{
    IzoTDevDomainConfig *pDomainConfig = NULL;

    // Find the device config
    IzoTDevConfig *pDevConfig = find(stackIndex);
    if (pDevConfig != NULL)
    {
        // It exists, so find the domain config
        pDomainConfig = pDevConfig->getDomains().find(domainIndex);
    }
    return pDomainConfig;
}

// Add domain config for a given device and domain index
IzoTDevDomainConfig *IzoTDevConfigMap::add(int stackIndex, int domainIndex)
{
    // Find the device config
    IzoTDevConfig *pDevConfig = find(stackIndex);
    if (pDevConfig == NULL)
    {
        // Doesn't exists, need to add it.
        pDevConfig = add(stackIndex);
    }

    // Now add the domain config to the domain list
    return pDevConfig->getDomains().add(domainIndex);
}

// Remove the domain config for a given device and domain index
void IzoTDevConfigMap::remove(int stackIndex, int domainIndex)
{
    // Find the device config
    IzoTDevConfig *pDevConfig = find(stackIndex);
    if (pDevConfig != NULL)
    {
        // It exists, so remove the domain config from the domain list
        pDevConfig->getDomains().remove(domainIndex);
    }
}

// Find the subnet node config for a given device and domain index, and subnet/node index
IzoTDevSubnetNodeConfig *IzoTDevConfigMap::find(int stackIndex, int domainIndex, int subnetNodeIndex)
{
    IzoTDevSubnetNodeConfig *pSubnetNodeConfig = NULL;

    // Find the domain config
    IzoTDevDomainConfig *pDomainConfig = find(stackIndex, domainIndex);
    if (pDomainConfig != NULL)
    {
        // It exists, so find the subnet/node config
        pSubnetNodeConfig = pDomainConfig->getSubnetNodes().find(subnetNodeIndex);
    }
    return pSubnetNodeConfig;
}

// Add  subnet node config for a given device and domain index, and subnet/node index
IzoTDevSubnetNodeConfig *IzoTDevConfigMap::add(int stackIndex, int domainIndex, int subnetNodeIndex)
{
    IzoTDevSubnetNodeConfig *pSubnetNodeConfig = NULL;

    // Find the domain config
    IzoTDevDomainConfig *pDomainConfig = find(stackIndex, domainIndex);
    if (pDomainConfig == NULL)
    {
        // Doesn't exist, so add it
        pDomainConfig = add(stackIndex, domainIndex);
    }

    // Now we can add the subnet/node config to the list of subnet nodes
    pSubnetNodeConfig = pDomainConfig->getSubnetNodes().add(subnetNodeIndex);
    return pSubnetNodeConfig;
}

// Remove subnet node config for a given device and domain index, and subnet/node index
void IzoTDevConfigMap::remove(int stackIndex, int domainIndex, int subnetNodeIndex)
{
    // find the domain config
    IzoTDevDomainConfig *pDomainConfig = find(stackIndex, domainIndex);
    if (pDomainConfig != NULL)
    {
        // It exists, so remove the subnet/node info from the subnetNode list
        pDomainConfig->getSubnetNodes().remove(subnetNodeIndex);
    }
}

#endif // #if FEATURE_INCLUDED(IZOT)
