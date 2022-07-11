/***************************************************************
 *  Filename: IzoTLsIpMapping.cpp
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
 *  Description:  Utilities used to facilitate
 *  mapping LS address to arbitrary IP addresses.
 *
 ****************************************************************/
#include "LtaDefine.h"
#if FEATURE_INCLUDED(IZOT)

#include <string.h>
#include <assert.h>

#include "LonLinkIzoT.h"	
#include "IzoTLsIpMapping.h"
#include "vxlTarget.h"
#include "LtRouteMap.h"

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   IzoTLsArbitraryIpAddr
//  Summary:
//      This utilty class is used to store an arbitrary IP address.
//
///////////////////////////////////////////////////////////////////////////////

// Construct an IzoTLsArbitraryIpAddr with the specified IP address
IzoTLsArbitraryIpAddr::IzoTLsArbitraryIpAddr(const void *pIpAddress)
{
    m_pAddr = NULL;
    setArbitraryIpAddr(pIpAddress);    
}

IzoTLsArbitraryIpAddr::~IzoTLsArbitraryIpAddr()
{
    delete[] m_pAddr;
}

    // Set or refresh the arbitrary IP address (which may be NULL
void IzoTLsArbitraryIpAddr::setArbitraryIpAddr(const void *pIpAddress)
{
    if (pIpAddress == NULL)
    {
        // Not using an arbitrary address (or the arbitrary address is unknown).
        // Delete it.
        delete[] m_pAddr;
        m_pAddr = NULL;
    }
    else if (m_pAddr == NULL || memcmp(m_pAddr, pIpAddress, IP_ADDR_SIZE) != 0)
    {
        // The address has changed.
        if (m_pAddr == NULL)
        {
            // Get a buffer
            m_pAddr = new unsigned char[IP_ADDR_SIZE];
        }
        if (m_pAddr != NULL)
        {
            // Copy the new address
            memcpy(m_pAddr, pIpAddress, IP_ADDR_SIZE);
        }
    }
    // Whether its been changed or not, its been refreshed.
    resetAge();
}

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   IzoTLsArbitraryIpAddrs
//  Summary:
//      This utilty class is used to store a collection of arbitrary IP 
//      addresses on a specific LS subnet.  The addresses are accessed
//      by LS nodeID.
//
///////////////////////////////////////////////////////////////////////////////
IzoTLsArbitraryIpAddrs::IzoTLsArbitraryIpAddrs()
{
    memset(m_pAddrs, 0, sizeof(m_pAddrs));
}

IzoTLsArbitraryIpAddrs::~IzoTLsArbitraryIpAddrs()
{
    for (int i = 0; i <= IZOT_MAX_NODE_ID; i++)
    {
        delete m_pAddrs[i];
    }
}

// Return the arbitrary IP address in network order) or NULL if none.
void *IzoTLsArbitraryIpAddrs::getArbitraryIpAddr(int nodeId)
{
    if (nodeId <= IZOT_MAX_NODE_ID && m_pAddrs[nodeId] != NULL)
    {
        return m_pAddrs[nodeId]->getArbitraryIpAddr();
    }
    return NULL;
}

    // Set or refresh the arbitrary IP address (which may be NULL
void IzoTLsArbitraryIpAddrs::setArbitraryIpAddr(int nodeId, const void *pIpAddress)
{
    if (nodeId <= IZOT_MAX_NODE_ID)
    {
        if (pIpAddress == NULL)
        {
            // Not using an arbitrary address (or the arbitrary address is unknown).
            // Delete it.
            delete m_pAddrs[nodeId];
            m_pAddrs[nodeId] = NULL;
        }
        else if (m_pAddrs[nodeId] == NULL)
        {
            // Its new, so create one
            m_pAddrs[nodeId] = new IzoTLsArbitraryIpAddr(pIpAddress);
        }
        else
        {
            // Update or refresh it
            m_pAddrs[nodeId]->setArbitraryIpAddr(pIpAddress);
        }
    }
}

// Process a tick of the aging timer
void IzoTLsArbitraryIpAddrs::agingTimerExpired(void)
{
    // Process a tick of the aging timer for every node with an arbitrary 
    // IP address,
    for (int nodeId = 0; nodeId < IZOT_MAX_NODE_ID; nodeId++)
    {
        if (m_pAddrs[nodeId] != NULL)
        {
            // Has an arbitrary address.  Increment the age
            if (m_pAddrs[nodeId]->incAge() >= IZOT_ARB_ADDRESS_AGE_LIMIT)
            {
                // The address has expired.  Delete it.

                // vxlReportEvent("Arbitrary address for node %d aged out - deleting\n", nodeId);
                delete m_pAddrs[nodeId];
                m_pAddrs[nodeId] = NULL;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   IzoTLsIpMappingNodeInfo
//  Summary:
//      This utilty class is used to store LS mapping information for
//      all nodes on a subnet. It includes a map of devices that use
//      LS derived IP addresses and a pointer to reference to a map
//      of arbitrary addresses (if any).
//
///////////////////////////////////////////////////////////////////////////////
IzoTLsIpMappingNodeInfo::IzoTLsIpMappingNodeInfo()
{
    // Initially we don't know if any devices use derived addresses
    // or arbitrary addresses.
    memset(m_lsDerivedIpAddrs, 0, sizeof(m_lsDerivedIpAddrs));
    m_pLsArbitraryIpAddrs = NULL;
}

IzoTLsIpMappingNodeInfo::~IzoTLsIpMappingNodeInfo()
{
    delete m_pLsArbitraryIpAddrs;
}
    
// Return true if we know that the node uses a derived IP address.
boolean IzoTLsIpMappingNodeInfo::getLsDerivedIpAddr(int nodeId)
{
    if (nodeId <= IZOT_MAX_NODE_ID)
    {
        return m_lsDerivedIpAddrs[nodeId/8] & (1 << (nodeId%8)) ? true : false;
    }
    return false;
}

// Set or clear the isLsDerived bit for the node
void IzoTLsIpMappingNodeInfo::setLsDerivedIpAddr(int nodeId, boolean isLsDerived)
{
    if (nodeId <= IZOT_MAX_NODE_ID)
    {
        byte mask = (1 << (nodeId%8));
        byte *pEntry = &m_lsDerivedIpAddrs[nodeId/8];
        if (isLsDerived)
        {
            *pEntry |= mask;
            setArbitraryIpAddr(nodeId, NULL);   // Can't be arbitrary if its derived.
        }
        else
        {
            *pEntry &= ~mask;
        }
    }
}

void IzoTLsIpMappingNodeInfo::setAllLsDerivedIpAddr(void)
{
    for (int i = 0; i < (IZOT_MAX_NODE_ID+1)/8; i++)
    {
        m_lsDerivedIpAddrs[i] = 0xff;
    }
    delete m_pLsArbitraryIpAddrs;
    m_pLsArbitraryIpAddrs = NULL;
}


// Get the arbitrary IP address in network order) for the node, or NULL if not known.
void *IzoTLsIpMappingNodeInfo::getArbitraryIpAddr(int nodeId)
{
    if (m_pLsArbitraryIpAddrs != NULL)
    {
        return m_pLsArbitraryIpAddrs->getArbitraryIpAddr(nodeId);
    }
    return NULL;
}

// Set the arbitrarty IP address (in network order) for the node.  If pIpAddress is NULL,
// this clears the arbitrary IP address.
// Also used to reset the age
void IzoTLsIpMappingNodeInfo::setArbitraryIpAddr(int nodeId, const void *pIpAddress)
{
    if (m_pLsArbitraryIpAddrs == NULL && pIpAddress != NULL)
    {
        // Don't have any yet, so we need to allocate the collection
        m_pLsArbitraryIpAddrs = new IzoTLsArbitraryIpAddrs;
    }
    if (m_pLsArbitraryIpAddrs != NULL)
    {
        if (pIpAddress != NULL)
        {
            setLsDerivedIpAddr(nodeId, FALSE);   // Can't be derived if it is arbitrary
        }

        // Now set or refresh the arbitrary address.
        m_pLsArbitraryIpAddrs->setArbitraryIpAddr(nodeId, pIpAddress);
    }
}

// Process a tick of the aging timer
void IzoTLsIpMappingNodeInfo::agingTimerExpired(void)
{
    if (m_pLsArbitraryIpAddrs != NULL)
    {
        m_pLsArbitraryIpAddrs->agingTimerExpired();
    }
}

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   IzoTLsIpMappingNodeInfo
//  Summary:
//      This utilty class is used to store LS mapping information for
//      all subnets on a given domain. Mapping structures for a specific subnet
//      are allocated dynamically, since it is unlikely that all subnets will
//      be in use.
//
//      Objects of this type can be linked together in order to support 
//      multiple domains.
///////////////////////////////////////////////////////////////////////////////
IzoTLsIpMappingSubnetInfo::IzoTLsIpMappingSubnetInfo(LtDomain domainId)
{
    m_pNext = NULL;
    m_domainId.set(domainId);
    memset(m_subnets, 0, sizeof(m_subnets));
}

IzoTLsIpMappingSubnetInfo::~IzoTLsIpMappingSubnetInfo()
{
    for (int i = 0; i <= IZOT_MAX_SUBNET_ID; i++)
    {
        delete m_subnets[i];
    }
}

// Return true if we know that the node uses a derived IP address.
boolean IzoTLsIpMappingSubnetInfo::getLsDerivedIpAddr(int subnetId, int nodeId)
{
    if (m_subnets[subnetId] != NULL)
    {
        return m_subnets[subnetId]->getLsDerivedIpAddr(nodeId);
    }
    return false;
}

// Set or clear the isLsDerived bit for the node
void IzoTLsIpMappingSubnetInfo::setLsDerivedIpAddr(int subnetId, int nodeId, boolean isLsDerived)
{
    if (m_subnets[subnetId] == NULL && isLsDerived)
    {
        // First one for this subnet.  Allocate map
        m_subnets[subnetId] = new IzoTLsIpMappingNodeInfo;
    }
    if (m_subnets[subnetId] != NULL)
    {
        // Set or clear the bit
        m_subnets[subnetId]->setLsDerivedIpAddr(nodeId, isLsDerived);
    }
}

// Set or clear the isLsDerived bit for all addresses with the specified subnets.
void IzoTLsIpMappingSubnetInfo::setLsDerivedIpSubnets(boolean set, const uint8_t *pSubnets)
{
    int subnetId = 0;
    for (int i = 0; i < (IZOT_MAX_SUBNET_ID+1)/8; i++)
    {
        if (pSubnets[i])
        {
            int mask = 1;
            for (int j = 0; j < 8; j++)
            {
                if (pSubnets[i] & mask)
                {
                    if (set)
                    {
                        if (m_subnets[subnetId] == NULL)
                        {
                            m_subnets[subnetId] = new IzoTLsIpMappingNodeInfo;
                        }
                        m_subnets[subnetId]->setAllLsDerivedIpAddr();
                    }
                    else if (m_subnets[subnetId] != NULL)
                    {
                        delete m_subnets[subnetId];
                        m_subnets[subnetId] = NULL;
                    }
                }
                mask <<= 1;
                subnetId++;
            }
        }
        else
        {
            subnetId += BITS_PER_UNIT;
        }
    }
}

// Get the arbitrary IP address (in network order) for the node, or NULL if not known.
void *IzoTLsIpMappingSubnetInfo::getArbitraryIpAddr(int subnetId, int nodeId)
{
    if (m_subnets[subnetId] != NULL)
    {
        return m_subnets[subnetId]->getArbitraryIpAddr(nodeId);
    }
    return NULL;
}

// Set or refresh the arbitrary IP address (which may be NULL)
void IzoTLsIpMappingSubnetInfo::setArbitraryIpAddr(int subnetId, int nodeId, const void *pIpAddress)
{
    if (m_subnets[subnetId] == NULL && pIpAddress != NULL)
    {
        // Don't have one for this subnet yet, so allocate the collection
        m_subnets[subnetId] = new IzoTLsIpMappingNodeInfo;
    }
    if (m_subnets[subnetId] != NULL)
    {
        m_subnets[subnetId]->setArbitraryIpAddr(nodeId, pIpAddress);
    }
}

// Link pNext after this object.
void IzoTLsIpMappingSubnetInfo::link(IzoTLsIpMappingSubnetInfo *pNext)
{
    m_pNext = pNext;
}

// Unlink this object from pPrev
void IzoTLsIpMappingSubnetInfo::unlink(IzoTLsIpMappingSubnetInfo *pPrev)
{
    if (pPrev != NULL)
    {
        pPrev->m_pNext = m_pNext;
    }
    m_pNext = NULL;
}

// Process a tick of the aging timer
void IzoTLsIpMappingSubnetInfo::agingTimerExpired(void)
{
    // Process a tick of the aging timer for every subnet that we have
    // mapping  info for
    for (int subnetId = 0; subnetId < IZOT_MAX_SUBNET_ID; subnetId++)
    {
        if (m_subnets[subnetId] != NULL)
        {
            // Have mapping info for this one.
            m_subnets[subnetId]->agingTimerExpired();
        }
    }
}

#endif  // #if FEATURE_INCLUDED(IZOT)
