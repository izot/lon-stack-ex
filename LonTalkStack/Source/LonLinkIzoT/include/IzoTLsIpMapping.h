/***************************************************************
 *  Filename: IzoTLsIpMapping.h
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
 *  Description:  Header file for utilities used to facilitate
 *    mapping LS address to arbitrary IP addresses.
 *      
 *    The mapping structure follows the domain/subnet/node 
 *    heirarchy using arrays to efficiently index by subnet ID and
 *    node ID. Contents of the arrays are allocated as needed since 
 *    we expect the information to be sparse.
 *
 *    Arbitrary IP addresses are stored as byte arrays in network 
 *    order.
 *
 ****************************************************************/

// Ls/IP mapping utilities
#include "LtDomain.h"

#define IZOT_MAX_NODE_ID   127
#define IZOT_MAX_SUBNET_ID 255

#define IZOT_ARB_ADDRESS_AGE_LIMIT 2    // After two ticks, we should have seen an announcement

#define IP_ADDR_SIZE 4  // REMINDER:  Need to support both IPV4 and IPV6

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   IzoTLsArbitraryIpAddr
//  Summary:
//      This utilty class is used to store an arbitrary IP address.
//
///////////////////////////////////////////////////////////////////////////////
class IzoTLsArbitraryIpAddr
{
public:
        // Construct an IzoTLsArbitraryIpAddr with the specified IP address
        // in network order.
    IzoTLsArbitraryIpAddr(const void *pIpAddress);
    ~IzoTLsArbitraryIpAddr();
    
        // Set the arbitrary address in network order, or NULL if none
    void setArbitraryIpAddr(const void *pIpAddress);

        // Get the arbitrary address in network order, or NULL if none
    void *getArbitraryIpAddr(void) { return m_pAddr; }

    void resetAge(void) { m_age = 0; }
    int incAge(void) { return ++m_age; }
    bool matches(const void *pIpAddress);
    
private:
    // REMINDER:  Need to include IP address len to support IPV6
    void *m_pAddr;

        // The number of times the ageing timer has expired
        // since the last time the address was updated.
    uint8_t m_age;  
};

///////////////////////////////////////////////////////////////////////////////
// 
//  Class:   IzoTLsArbitraryIpAddrs
//  Summary:
//      This utilty class is used to store a collection of arbitrary IP 
//      addresses on a specific LS subnet.  The addresses are accessed
//      by LS nodeID.
//
///////////////////////////////////////////////////////////////////////////////
class IzoTLsArbitraryIpAddrs
{
public:
    IzoTLsArbitraryIpAddrs();
    ~IzoTLsArbitraryIpAddrs();

        // Return the arbitrary IP address in network order) or NULL if none.
    void *getArbitraryIpAddr(int nodeId);
        // Set or refresh the arbitrary IP address (which may be NULL)
    void setArbitraryIpAddr(int nodeId, const void *pIpAddress);
        // Process a tick of the aging timer
    void agingTimerExpired(void);
private:
    // An array of pointers to arbitrary addresses, indexed by LS node ID.
    IzoTLsArbitraryIpAddr *m_pAddrs[IZOT_MAX_NODE_ID+1];
};

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
class IzoTLsIpMappingNodeInfo
{
public:
    IzoTLsIpMappingNodeInfo();
    ~IzoTLsIpMappingNodeInfo();
    
        // Return true if we know that the node uses a derived IP address.
    boolean getLsDerivedIpAddr(int nodeId);
        // Set or clear the isLsDerived bit for the node
    void setLsDerivedIpAddr(int nodeId, boolean isLsDerived);
        // Set all LS addresses as derived
    void setAllLsDerivedIpAddr(void);

        // Get the arbitrary IP address in network order) for the node, or NULL if not known.
    void *getArbitraryIpAddr(int nodeId);
        // Set or refresh the arbitrary IP address (which may be NULL)
    void setArbitraryIpAddr(int nodeId, const void *pIpAddress);
        // Process a tick of the aging timer
    void agingTimerExpired(void);

private:
        // Bit map of devices known to be using LS derived IP addresses
    byte m_lsDerivedIpAddrs[(IZOT_MAX_NODE_ID+7)/8];  
        // Reference to arbitrary IP addresses on this subnet.  NULL until
        // we discover one, since we expect them to be rare.
    IzoTLsArbitraryIpAddrs *m_pLsArbitraryIpAddrs;
};

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
class IzoTLsIpMappingSubnetInfo
{
public:
    IzoTLsIpMappingSubnetInfo(LtDomain domainId);
    ~IzoTLsIpMappingSubnetInfo();

        // Return true if we know that the node uses a derived IP address.
    boolean getLsDerivedIpAddr(int subnetId, int nodeId);
        // Set or clear the isLsDerived bit for the node
    void setLsDerivedIpAddr(int subnetId, int nodeId, boolean isLsDerived);
        // Set or clear the isLsDerived bit for all addresses with the specified subnets.
    void setLsDerivedIpSubnets(boolean set, const uint8_t *pSubnets);

        // Get the arbitrary IP address (in network order) for the node, or NULL if not known.
    void *getArbitraryIpAddr(int subnetId, int nodeId);
        // Set or refresh the arbitrary IP address (which may be NULL)
    void setArbitraryIpAddr(int subnetId, int nodeId, const void *pIpAddress);
        // Process a tick of the aging timer
    void agingTimerExpired(void);

        // Get the domain ID
    LtDomain& GetDomain() { return m_domainId; }
        // Get the IzoTLsIpMappingSubnetInfo for the next domain.
    IzoTLsIpMappingSubnetInfo *getNext(void) { return m_pNext; }
        // Link pNext after this object.
    void link(IzoTLsIpMappingSubnetInfo *pNext);
        // Unlink this object from pPrev
    void unlink(IzoTLsIpMappingSubnetInfo *pPrev);
private:
        // The LS domain ID
    LtDomain m_domainId;
        // Pointers to subnet mapping information. Entries are allocated as
        // needed.
    IzoTLsIpMappingNodeInfo *m_subnets[IZOT_MAX_SUBNET_ID+1];  
        // Link to the next domain
    IzoTLsIpMappingSubnetInfo *m_pNext;
};
