// util.c - implementing the ISI utility routines
//
// Copyright © 2005-2022 Dialog Semiconductor
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

#include <stdio.h>
#include <stdarg.h>
#include "isi_int.h"
#include <time.h>

//============== LOCAL VARIABLES ===========================================


/*=============================================================================
 *                           SUPPORT FUNCTIONS                                *
 *===========================================================================*/

/***************************************************************************
 *  Function: access_domain
 *
 *  Parameters: int index
 *
 *  Operation: Call LonQueryDomainConfig() LTS API function to get domain 
 *  configuration of index.
 *
 ***************************************************************************/
const LonDomain* access_domain(int index)
{
    LonDomain* pDomain = NULL;

    _IsiAPIDebug("access_domain = %d\n", index);
    if (index <= LON_GET_ATTRIBUTE(read_only_data, LON_READONLY_TWO_DOMAINS))
    {
        pDomain = &domainTable[index];
        if (LonQueryDomainConfig(index, pDomain) == (LonApiError)IsiApiNoError)
        {
            _IsiAPIDebug("DomainID  = %x %x %x %x %x %x, Subnet=%d, NonClone=%d Node=%d Invalid=%d Length=%d Key=%x\n", 
                pDomain->Id[0],pDomain->Id[1],pDomain->Id[2],pDomain->Id[3],pDomain->Id[4],pDomain->Id[5],pDomain->Subnet, 
                LON_GET_ATTRIBUTE_P(pDomain,LON_DOMAIN_NONCLONE),
                LON_GET_ATTRIBUTE_P(pDomain,LON_DOMAIN_NODE),
                LON_GET_ATTRIBUTE_P(pDomain,LON_DOMAIN_INVALID),
                LON_GET_ATTRIBUTE_P(pDomain,LON_DOMAIN_ID_LENGTH), pDomain->Key[0]);
        }
    }

    return pDomain;
};

/***************************************************************************
 *  Function: update_domain_address
 *
 *  Parameters: const DomainStruct* pDomain, int index
 *
 *  Operation: ISI Update Domain 
 ***************************************************************************/
IsiApiError update_domain_address(const LonDomain* pDomain, int index, int nonCloneValue, LonBool bUpdateID)
{
    int i;
    LonDomain* temp = NULL;
    IsiApiError sts = IsiApiNoError;

    _IsiAPIDebug("Start update_domain_address = %d\n", index);
    if (index <= LON_GET_ATTRIBUTE(read_only_data, LON_READONLY_TWO_DOMAINS))
    {
        temp = &domainTable[index];
        if (bUpdateID)
        {
            for(i=0; i<DOMAIN_ID_LEN; i++)
            {
		        temp->Id[i] = pDomain->Id[i];
	        };
        }
	    temp->Subnet = pDomain->Subnet;
	    LON_SET_ATTRIBUTE_P(temp,LON_DOMAIN_NONCLONE,nonCloneValue);  // 0 = if it's a clone domain, 1= otherwise
	    LON_SET_ATTRIBUTE_P(temp,LON_DOMAIN_NODE, LON_GET_ATTRIBUTE_P(pDomain,LON_DOMAIN_NODE));
	    temp->InvalidIdLength = pDomain->InvalidIdLength;
        LON_SET_ATTRIBUTE_P(temp,LON_DOMAIN_INVALID, 0);    // set the domain to be valid.  Otherwise, the LTS will reset the length to 7
        sts = LonUpdateDomainConfig(index, temp);
        _IsiAPIDebug("DomainID  = %x %x %x %x %x %x, Subnet=%d, NonClone=%d Node=%d Invalid=%d Length=%d Key=%x\n", 
                temp->Id[0],temp->Id[1],temp->Id[2],temp->Id[3],temp->Id[4],temp->Id[5],temp->Subnet, 
                LON_GET_ATTRIBUTE_P(temp,LON_DOMAIN_NONCLONE),
                LON_GET_ATTRIBUTE_P(temp,LON_DOMAIN_NODE),
                LON_GET_ATTRIBUTE_P(temp,LON_DOMAIN_INVALID),
                LON_GET_ATTRIBUTE_P(temp,LON_DOMAIN_ID_LENGTH), temp->Key[0]);
    }
    _IsiAPIDebug("End update_domain_address = %d\n", sts);
    return sts;
};


/***************************************************************************
 *  Function: IsiSetDomain
 *
 *  Parameters: DomainStruct* pDomain, unsigned Index
 *
 *  Operation: Using LTS LonUpdateDomainConfig API function to update domain of index
 ***************************************************************************/
IsiApiError IsiSetDomain(const LonDomain* pDomain, unsigned index)
{
    IsiApiError sts;

    _IsiAPIDebug("IsiSetDomain - index = %d\n", index);
    sts = update_domain_address(pDomain, index, 1, TRUE); // it's not a clone domain

    return sts;
};

/***************************************************************************
 *  Function: _nv_count
 *
 *  Parameters: void
 *
 *  Operation: Return Number of static NVs
 ***************************************************************************/
unsigned _nv_count(void)
{
    unsigned int nvCount = LonGetStaticNVCount();
    // Check against the ISI limit
    if (nvCount > ISI_MAX_NV_COUNT)
        nvCount = ISI_MAX_NV_COUNT;
    return nvCount;
};

/***************************************************************************
 *  Function: IsiSetNv
 *
 *  Parameters: NVStruct* pNv, unsigned Index
 *
 *  Operation: ISI Set NV
 ***************************************************************************/
void IsiSetNv(LonNvEcsConfig* pNv, unsigned Index)
{
	update_nv(pNv, Index);
};

/***************************************************************************
 *  Function: update_nv
 *
 *  Parameters: const NVStruct * nv_entry, int index
 *
 *  Operation: Update NV
 ***************************************************************************/
void update_nv(const LonNvEcsConfig * nv_entry, unsigned index)
{
	// UpdateNV(nv_entry, index);
    if (nv_entry && index < _nv_count())
    {
        // nvConfigTable[index] = *nv_entry;
        IsiApiError sts = LonUpdateNvConfig(index, nv_entry);

        _IsiAPIDebug("update_nv index %u sts %i ", index, sts);
        _IsiAPIDump("data = 0x", (void *)nv_entry, sizeof(LonNvEcsConfig), "\n");
    }
};

/***************************************************************************
 *  Function: IsiGetNv
 *
 *  Parameters: unsigned Index
 *
 *  Operation: Using LTS LonQueryNvConfig function to return nv ptr of Index
 ***************************************************************************/
const LonNvEcsConfig* IsiGetNv(unsigned Index)
{
    memset(&nv_config, 0, sizeof(LonNvEcsConfig));
    LonQueryNvConfig(Index, &nv_config);
    return (LonNvEcsConfig* )&nv_config;
};

/***************************************************************************
 *  Function: high_byte
 *
 *  Parameters: LonUbits16 a
 *
 *  Operation: Return high byte of a
 ***************************************************************************/
LonByte high_byte (LonUbits16 a)
{
	return ((unsigned char)(a>>8));
};

/***************************************************************************
 *  Function: low_byte
 *
 *  Parameters: LonUbits16 a
 *
 *  Operation: Return low byte of a
 ***************************************************************************/
LonByte low_byte (LonUbits16 a)
{
	return ((unsigned char)(a&(0x00FF)));
};

/***************************************************************************
 *  Function: make_long
 *
 *  Parameters: LonByte low_byte, LonByte high_byte
 *
 *  Operation: Return combined of high_byte and low_byte
 ***************************************************************************/
LonWord make_long(LonByte low_byte, LonByte high_byte)
{
    LonWord w;
    LON_SET_UNSIGNED_WORD(w, (LonUbits16)((high_byte<<8)|low_byte));
	return w;
};

/***************************************************************************
 *  Function: getRandom
 *
 *  Parameters: void
 *
 *  Operation: return a random number based on time and the 
 *  least significant 4 bytes of NeuronID.
 *
 ***************************************************************************/
unsigned int getRandom(void)
{
    /* Simple "srand()" seed: just use "time()" */
    unsigned int nuid = (unsigned int)((read_only_data.UniqueNodeId[2] << 24) | (read_only_data.UniqueNodeId[3] << 16) |
        (read_only_data.UniqueNodeId[4] << 8) | (read_only_data.UniqueNodeId[5]));
    unsigned int seedtime = (unsigned int)time(NULL);
    unsigned int iseed = seedtime ^ nuid;
//    unsigned int iseed = (unsigned int)time(NULL) ^ (unsigned int)((read_only_data.UniqueNodeId[2] << 24) | (read_only_data.UniqueNodeId[3] << 16) |
        //(read_only_data.UniqueNodeId[4] << 8) | (read_only_data.UniqueNodeId[5]));
    
    srand (iseed);
    nuid = rand();
    return (nuid & 0xff);
	// return (rand() >> 17) & 0xff;
};

/***************************************************************************
 *  Function: access_address
 *
 *  Parameters: int index
 *
 *  Operation: Using LTS LonQueryAddressConfig function to return ptr of index
 ***************************************************************************/
const LonAddress*	access_address	(int index)
{
    LonAddress* pAddress = (LonAddress*)&addrTable;

    if (LonQueryAddressConfig(index, pAddress) == (LonApiError)IsiApiNoError)
        return pAddress;
    else
        return (LonAddress*)NULL;            
};

/***************************************************************************
 *  Function: update_address
 *
 *  Parameters: const AddrTableEntry * address, int index
 *
 *  Operation: Using LTS LonUpdateAddressConfig function to update address of index
 ***************************************************************************/
IsiApiError update_address(const LonAddress* address, int index)
{
    IsiApiError sts = LonUpdateAddressConfig(index, address);
	if (sts != IsiApiNoError)
    {
		_IsiAPIDebug("update_address failed (entry %d)\n", index);
	}
    return sts;
};

/***************************************************************************
 *  Function: watchdog_update
 *
 *  Parameters: void
 *
 *  Operation: 
 ***************************************************************************/
void watchdog_update(void)
{
    // do nothing
};


/***************************************************************************
 *  Function: IsiGetAlias
 *
 *  Parameters: unsigned Index
 *
 *  Operation: using LTS LonQueryAliasConfig to return ptr to Index
 ***************************************************************************/
const LonAliasEcsConfig* IsiGetAlias(unsigned Index)
{
    memset(&alias_config, 0, sizeof(LonAliasEcsConfig));
    if (LonQueryAliasConfig(Index, &alias_config) != (LonApiError)IsiApiNoError)
    {
        _IsiAPIDebug("Error - IsiGetAlias(%d)\n", Index); 
    }
	return (LonAliasEcsConfig*)&alias_config;
};

/***************************************************************************
 *  Function: IsiSetAlias
 *
 *  Parameters: LonAliasEcsConfig* pAlias, unsigned Index
 *
 *  Operation: using LTS LonUpdateAliasConfig to update alias
 ***************************************************************************/
IsiApiError IsiSetAlias(LonAliasEcsConfig* pAlias, unsigned Index)
{
    return LonUpdateAliasConfig(Index, pAlias); 
};

IsiApiError update_config_data(const LonConfigData *config_data1)
{
    // Update the global copy of config data
    memcpy(&config_data, config_data1, sizeof(LonConfigData));
    return LonUpdateConfigData(config_data1);  // call LTS LonUpdateConfigData to update the config data 
};

LonConfigData* get_config_data()
{
    // Get the fresh config data from the stack
    LonQueryConfigData(&config_data);
    return (LonConfigData*)&config_data;
}

unsigned int _alias_count(void)
{
    unsigned int aliasCount = LonGetAliasCount();
    // Check against the ISI limit
    if (aliasCount > ISI_MAX_ALIAS_COUNT)
        aliasCount = ISI_MAX_ALIAS_COUNT;
    return aliasCount;
};

unsigned int _address_table_count(void)
{
    unsigned int addressTableCount = LonGetAddressTableCount();
    // Check against the ISI limit
    if (addressTableCount > ISI_MAX_ADDRESS_TABLE_SIZE)
        addressTableCount = ISI_MAX_ADDRESS_TABLE_SIZE;
    return addressTableCount;
};

unsigned get_nv_si_count(void)
{
	return _nv_count(); // same with the nv count
};

/***************************************************************************
 *  Function: get_nv_type
 *
 *  Parameters: unsigned index
 *
 *  Operation: Returns the type of NV  if it is a SNVT (1-250). A non-standard network 
 *  variable type will have SnvtId = 0.  
 ***************************************************************************/
unsigned get_nv_type (unsigned index)
{
    IsiApiError sts;
    LonNvDefinition NvDef;
    unsigned nvType = 0;

    sts = LonQueryNvType(index, &NvDef);
    if (sts == IsiApiNoError)
    {
        nvType = NvDef.SnvtId;
        LonFreeNvTypeData(&NvDef);      // free the internally allocated string buffers
    }
    return nvType;
};

LonWord _IsiAddSelector(LonWord Selector, unsigned Increment)
{
    LonWord result;
	LON_SET_UNSIGNED_WORD(result, LON_GET_UNSIGNED_WORD(Selector) + Increment);
    return result;
};

LonWord _IsiIncrementSelector(LonWord Selector)
{
    return _IsiAddSelector(Selector, 1);
};

IsiType _IsiGetCurrentType()
{
    return gIsiType;
};

void _IsiSetCurrentType(IsiType type)
{
    gIsiType = type;
};

const unsigned get_nv_length(const unsigned index)
{
    return LonGetCurrentNvSize(index);
}

LonByte* get_nv_value(const unsigned index)
{
	return (LonByte *)LonGetNvValue(index);
}

IsiApiError service_pin_msg_send()
{
    return LonSendServicePin();
}

void node_reset()
{
    LonReset(NULL);     
}    

IsiApiError retrieve_status(LonStatus* status)
{
    return LonQueryStatus(status);       
}

// Initialize the persitent data, connection table, config_data and read_only_data
IsiApiError initializeData(IsiBootType bootType)
{
    IsiApiError sts = LonQueryConfigData(&config_data);
   
    if (sts == IsiApiNoError)
        sts = LonQueryReadOnlyData(&read_only_data);

    _IsiSetAppSignature(LonGetAppSignature());

    if (restorePersistentData(IsiNvdSegPersistent) != LT_PERSISTENCE_OK)
    {
        // First time run.  Signal the engine to clear-out NV, alias, connection table and address tables.
        if (bootType != isiReboot)
            _isiPersist.BootType = isiReset;
    }
    else
    if (restorePersistentData(IsiNvdSegConnectionTable) != LT_PERSISTENCE_OK) 
        _IsiInitConnectionTable();

    return sts;
}

IsiApiError set_node_mode(unsigned mode, unsigned state)
{
	return LonSetNodeMode(mode, state);
}
