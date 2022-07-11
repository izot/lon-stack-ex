//
// nodedef.h
//
// Copyright © 2022 Dialog Semiconductor
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

#ifndef _NODEDEF_H
#define _NODEDEF_H

#include <LtaDefine.h>

#include <stdio.h>
#include <time.h>
#ifdef WIN32
#include <malloc.h>
#include <memory.h>
#endif
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

typedef unsigned char byte;

typedef unsigned char boolean;

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#include <LtNetData.h>

#include <LtPersistence.h>
#include <VxClass.h>

//
// This file contains the class for handling LonWorks node definitions.
// The goal of this class is provide a single location which supports
// all of the following node definition formats:
// 1. XIF files
// 2. XFB files
// 3. SI data
//
// The class provides APIs for specifying node definition either via the node
// itself (read only data and SI data), a XIF file, or an XFB file.  Alternatively, 
// node definition can be specified programmatically.  Node definition can then 
// be retrieved programmatically or in any of the above formats.  The class also 
// provides a text dump version of the node definition as well.
//
// Other possible enhancements to this class include:
// 1. Methods to compare two node definitions to determine compatibility.
//    For example, does a new program definition contain only added NVs or
//    did some NVs change or otherwise move?
// 2. Methods to spot check a node's read only data and SI data relative to
//    the node definition.
// 3. Methods to handle LonMark.
//
// This class is intended to be platform independent and thus avoids use
// of bit fields and structures to define network packed data.  It does assume
// presence of a standard C library (fopen, printf, mem and string routines).
//
// This class provides methods for serializing objects but does not actually store them.
// It is the responsibility of the owning class to serialize the objects and store
// them.  Note that with lots of NV definitions, storing all NVs in one flat might
// be problematic, both from a performance perspective and a flash perspective.  On the
// other hand, it is probably the case that implementations with thousands of dynamic 
// NVs are limited to PCs which don't have a flash lifetime problem and are less likely
// to have performance problems.
//
// Because of its lack of platform assumptions, multiple thread access to a single
// instance of this class is not guaranteed to work (multiple threads can simultaneously
// operate on independent instances).  To guarantee thread safety, wrap any calls into
// this class with a lock/unlock pair.
//
// Objects stored by this class (such as NdNetworkVariable objects) must be allcoated
// and freed by the caller.  The exception to this is if you do a "reset" operation
// in which case all objects are freed.
//

#include <nodedef1.h>

class NdMessageTag : public NdCollectionElement
{
};

typedef NdCollection<NdMessageTag> NdMessageTags;

class NdConfigProp : public NdCollectionElement
{
};

typedef NdCollection<NdConfigProp> NdConfigProps;

//
// Note that network variable collections are tricky because of NV arrays.  We could
// just explode arrays but this is very inefficient in some cases.  So instead, each
// NV tracks its base NV index.  We thus need to map from NV index to NdNetworkVariable
// index.

#include "NdNetworkVariable.h"

typedef NdCollection<NdNetworkVariable> NdNetworkVariables;

typedef NdNetworkVariable NdDeviceNetworkVariable	;

typedef NdNetworkVariables NdDeviceNetworkVariables;

class NdLonMarkObject : public NdCollectionElement
{
	DEFINE_COLLECTION(NetworkVariable)
	DEFINE_COLLECTION(ConfigProp)

private:

public:
};

typedef NdCollection<NdLonMarkObject> NdLonMarkObjects;

// This class is only currently used for storin static NVs.  It does
// not have a removal mechanism.  The only reason for this collection
// is to support SI version 1 format which numbers NV arrays from 
// 0..N-1.  This collection makes for easy mapping of NV arrays in
// this form.
class NdNvArrays : public NdNetworkVariables
{
private:
	int m_nStaticNvArrayCount;

public:
	NdNvArrays()
	{
		reset();
	}

	NdErrorType add(NdNetworkVariable* pNv)
	{
		return add(getCount(), pNv);
	}

	NdErrorType add(int nIndex, NdNetworkVariable* pNv)
	{
		if (pNv->getStatic())
		{
			m_nStaticNvArrayCount++;
		}
		return NdNetworkVariables::add(nIndex, pNv);
	}

	void reset()
	{
		m_nStaticNvArrayCount = 0;
		setDeleteOnReset(false);
		NdNetworkVariables::reset();
	}

	int getStaticNvCount() { return m_nStaticNvArrayCount; }
};

class NdNvArraysNamed : public NdNvArrays
{
private:
	int m_bNetworkVariableNamesUnique;
	int getHashCode(char* szName, int& arrayIndex);

public:
	// The key to this class is a fast look-up by name.
	NdNvArraysNamed()
	{
		setDeleteOnReset(false);
		m_bNetworkVariableNamesUnique = true;
	}
	NdNetworkVariable* get(char* szName, int &arrayIndex, int &nIndex);
	NdErrorType add(NdNetworkVariable* pNv);
	NdErrorType remove(NdNetworkVariable* pNv);
	int getNetworkVariableNamesUnique() { return m_bNetworkVariableNamesUnique; }
	NdErrorType setNetworkVariableNamesUnique(int bValue) { m_bNetworkVariableNamesUnique = bValue; return ND_OK; }
};

class NdNvs : public NdNetworkVariables
{
private:
	int m_nStaticNvCount;

public:
	NdNvs()
	{
		m_nStaticNvCount = 0;
		setDeleteOnReset(false);
		NdNetworkVariables::reset();
	}

	NdErrorType add(int nIndex, NdNetworkVariable* pNv)
	{
		NdErrorType err = ND_OK;

		// If the caller is not exploding the array, for each NV array 
		// element, add to the collection.
		int length = pNv->getArrayLength();
		if (pNv->getStatic() && pNv->getChangeable() && length)
		{
			// To support static arrays with changeable types means we would
			// 1. need to accept an array object from the app and return it individual
			//   objects.
			// OR
			// 2. support semantics of types by array index.
			// Neither exists now.  (TBD)
			err = ND_NOT_IMPLEMENTED;
		}
		else
		{
			if (length == 0 || pNv->getForked()) length = 1;
			for (int i = 0; i < length; i++)
			{
				NdNetworkVariables::add(nIndex++, pNv);
				if (pNv->getStatic())
				{
					m_nStaticNvCount++;
				}
			}
		}
		return err;
	}

	NdErrorType add(NdNetworkVariable* pNv)
	{
		NdErrorType err;
		if (pNv->getDynamic())
		{
			err = ND_INVALID_PARAMETER;
		}
		else
		{
			pNv->setNvIndex(m_nStaticNvCount);
			err = add(m_nStaticNvCount, pNv);
		}
		return err;
	}

	void reset()
	{
		// Dynamic NVs live here but not in a LonMark collection so they
		// are deleted when this collection is reset.
		while (getCount() > m_nStaticNvCount)
		{
			deleteLast();
		}
		m_nStaticNvCount = 0;
		setDeleteOnReset(false);
		NdNetworkVariables::reset();
	}

	int getStaticNvCount() { return m_nStaticNvCount; }
	int getDynamicNvCount() { return getCount() - getStaticNvCount(); }
};

class NodeDefClient
{
public:
	virtual ~NodeDefClient() {}

	virtual NdNetworkVariable* nvAdded(NdNetworkVariable* pNd)
	{
		return NULL;
	}
	virtual void nvChanged(NdNetworkVariable* pNd, NvChangeType type) {}
	virtual void nvDeleted(NdNetworkVariable* pNd)
	{
		delete pNd;
	}
	virtual void persistenceLost(LtPersistenceLossReason reason) {}
};

#define ND_CAPABILITY_LENGTH 30

#define ND_CAPABILITY_DYN_FB_COUNT_OFFSET (ND_CAPABILITY_LENGTH)
#define ND_CAPABILITY_EAT_ADDRESS_CAPACITY (ND_CAPABILITY_DYN_FB_COUNT_OFFSET + 2)

#define ND_NUM_EXTCAP_BYTES 6
#define ND_MAX_NM_VER 1

// Size to reserve for ND persistence header info.
#define ND_PERSISTENCE_HEADER_LEN 100

class NodeDef : public LtPersistenceClient
{
	DEFINE_COLLECTION(ConfigProp)
	// Message tag collections not supported yet.  Note conflict between registerApplication
	// message tag count (getMessageTagCount()) and getMessageTagCount function resulting
	// from this macro.
	// DEFINE_COLLECTION(MessageTag)
	DEFINE_COLLECTION(DeviceNetworkVariable)
	DEFINE_COLLECTION(LonMarkObject)

friend class NodeDefMaker;

private:
	LtProgramId m_programId;
	int m_nAddressTableCount;
	int m_nAliasCount;
	int m_bBinding2;
	int m_bBinding3;
	int m_nDomainCount;
	int	m_bNvArrayExplosion;
	int m_bExplicitMessageProcessing;
	int m_nMaximumDynamicNetworkVariableCount;
	int m_nMaximumPrivateNetworkVariableCount;
	int m_nMaximumMonitorNetworkVariableCount;
	int m_nMaximumMonitorPointCount;
	int m_nMaximumMonitorSetCount;
	int m_nMaxMemWriteSize;
	int m_nReceiveTransactionCount;
	int m_nStaticNetworkVariableCount;
	int m_bStaticNvChangeable;
	int m_nMessageTagCount;
	int m_bEndDefDone;
	int m_bLayer5Mip;
    int m_bOma; // Supports OMA.

	int m_bOneTimeCreation;

	int m_siVersion;
	int m_siHdrLen;
	int m_siLength;

	char* m_szDescription;// User description
	int m_nDescLen;

	NdNvs	m_nvs;	// All network NVs, network based index.

	NdNvArrays m_nvArrays;

	NdNvArraysNamed m_nvArraysNamed;

	NdProgramType m_programType;

	NdBufferConfiguration m_buffers;

    // The following set of attributes are used for node simulation, to better control the 
    // exposed capabilities of the device.  They are not stored persistently, and
    // if not set, use defaults based on actual capabilities of the stack.
    int m_bOverrideExtendedCapabilityFlags; 
    byte m_extendedCapabilityFlagOverrides[ND_NUM_EXTCAP_BYTES];
    int m_numOptionalCapabilitiesBytes;    
    int m_dynamicFbCapacity;  // Only used if m_optionalCapabilitiesBytes >= 2.
    int m_maxNmVer;

	// Limit set to 65535 by default.  Generally, index 0xffff is reserved.
	NdErrorType set(int& value, int nCount, int limit=65535);

	NodeDefClient* m_pClient;

	LtPersistenceServer* m_pPersistenceServer;
	LtPersistenceServer* getPersistenceServer() { return m_pPersistenceServer; }

protected:
	VxcLock m_lock;

public:
	NodeDef();
	virtual ~NodeDef();

	void lock() { m_lock.lock(); }
	void unlock() { m_lock.unlock(); }

	void reset();
	LtPersistenceLossReason restore();	// Restore definitions from persistent storage.

	void registerNodeDefClient(NodeDefClient* pClient) { m_pClient = pClient; }
	NodeDefClient* getClient() { return m_pClient; }

	void registerPersistence(LtPersistenceServer* pServer) { m_pPersistenceServer = pServer; }

	// Program ID - array of 6 bytes
	unsigned char* getProgramId();
	LtProgramId& getProgramIdRef();
	NdErrorType setProgramId(unsigned char* pProgramId);
	NdErrorType setProgramId(LtProgramId& programId);

	// Program type
	NdProgramType getProgramType();
	NdErrorType setProgramType(NdProgramType type);

	// Node self doc string.
	const char* getNodeSd();
	int getNodeSdLen();
	NdErrorType setNodeSd(const char* szNodeSd);

	// The total number of domains.
	int getDomainCount();
	NdErrorType setDomainCount(int nCount);

	// The total number of address table entries
	int getAddressTableCount();
	NdErrorType setAddressTableCount(int nCount);

	// The total number of aliases.  
	int getAliasCount();
	NdErrorType setAliasCount(int nCount);

	// Number of message tags
	int getMessageTagCount();
	NdErrorType setMessageTagCount(int nCount);

	// true if application handles explicit messages
	int getExplicitMessageProcessing();
	NdErrorType setExplicitMessageProcessing(int bExpMsg);

	// true if this is a layer 5 mip node
	int getLayer5Mip();
	NdErrorType setLayer5Mip(int bLayer5Mip);

	// true if this supports OMA
	int getOma();
	NdErrorType setOma(int bOma);

	// Buffer configuration - values are not encoded.
	void getBufferConfiguration(NdBufferConfiguration& buffers);
	NdErrorType setBufferConfiguration(NdBufferConfiguration& buffers);

	// Number of receive transactions
	int getReceiveTransactionCount();
	NdErrorType setReceiveTransactionCount(int nCount);

	// true if binding 2 constraints supported
	int getBinding2();
	NdErrorType setBinding2(int bBinding2);

	// true if binding 3 constraints supported
	int getBinding3();
	NdErrorType setBinding3(int bBinding3);

	// true if network variable names must be unique (default is yes)
	int getNetworkVariableNamesUnique();
	NdErrorType setNetworkVariableNamesUnique(int bValue);

	// max write memory size (bytes)
	int getMaxMemWriteSize();
	NdErrorType setMaxMemWriteSize(int nSize);

	// Set one time creation to true if you want to create
	// static NV definitions once (e.g., from a XIF)
	NdErrorType setOneTimeCreation(int bOneTimeCreation);

	NdErrorType setStaticNetworkVariableCount(int nCount);

	void ndInitDone();

	//
	// This section defines special Network Variable definitions
	//

	NdErrorType addNetworkVariable(NdNetworkVariable* pNv);

	void getNvStats(int start, int end, int &count, int &maxIndex);
	void getMcNvStats(int &count, int &maxIndex);
	void getStaticDynamicNvStats(int &count, int &maxIndex);

	int getTotalNvCapacity();
	int getNvCount();
	NdNetworkVariable* getNv(int nIndex);
	NdNvs* getNvs() { return &m_nvs; }

	NdNvArrays* getNvArrays() { return &m_nvArrays; }

	NdNvArraysNamed* getNvArraysNamed() { return &m_nvArraysNamed; }

	NdNetworkVariable* get(char* szName, int& arrayIndex);
	NdNetworkVariable* get(int nvIndex, int& arrayIndex);

	// The total number of static network variables.  
	int getStaticNetworkVariableCount();

	// The total number of dynamic network variables currently defined.
	int getDynamicNetworkVariableCount();

	// The maximum number of dynamic network variables that can be defined.
	int getMaximumDynamicNetworkVariableCount();
	NdErrorType setMaximumDynamicNetworkVariableCount(int nCount);

	// The maximum NV index currently in use.  
	int getMaximumNetworkVariableIndex();

	int getMaximumPrivateNetworkVariableCount();
	NdErrorType setMaximumPrivateNetworkVariableCount(int nCount);

	int getMaximumMonitorNetworkVariableCount();
	NdErrorType	setMaximumMonitorNetworkVariableCount(int numMonitorNvEntries);

	int getMaximumMonitorPointCount();
	NdErrorType setMaximumMonitorPointCount(int numMonitorPointEntries);

	int getMaximumMonitorSetCount();
	NdErrorType	setMaximumMonitorSetCount(int numMonitorSetEntries);

	// Once all static NVs have been defined, this must be called.
	NdErrorType endDefinition();

	NdErrorType addNonStaticNv(int nvIndex, NdNetworkVariable* pNv, boolean bSetName);

	NdErrorType modifyNv(int nvIndex, NdNvType nvType, const char* szDesc, char* szName, int rateEst, int maxRateEst);
	NdErrorType createNvs(boolean legacyNvType, int nvIndex, boolean bMultiType, byte* pNativeOrderNvTypes, int nLength, int nFlags, int nArrayLength,
		const char* pSdString, char* pNvName, int rateEstimate, int maxRateEstimate);

	// Method to serialize the node definition.  Buffer is allocated and 
	// returned by the serialize method.  It's length is also returned
	void serialize(byte* &pBuffer, int &len);
	LtPersistenceLossReason deserialize(byte* pBuffer, int imageLen, int nVersion);

	void persistenceUpdate();

#if PERSISTENCE_TYPE_IS(FTXL)
    // Return the the maximum number of bytes that will be consumed for
    // serialized data given the current static interface and an estitmated 
    // NV SD length for dynamic NVs.
    int  getMaxSerialLength(int averageDynamicNvdSdLength);
#endif

	// Miscellaneous
	int getNvArrayExplosion();
	NdErrorType setNvArrayExplosion(int bNvArrayExplosion);

	int getStaticNvChangeable() { return m_bStaticNvChangeable; }
	void setStaticNvChangeable(int bStaticNvChangeable) { m_bStaticNvChangeable = bStaticNvChangeable; }

	NdErrorType removeNvs(int nvIndex, int nCount);

    // The following routines are used to better control self documentation for the purposes
    // of node simulation.  If not called, the defaults will be based on the actual abilities
    // of the stack.
    void setExtendedCapabilityFlagOverrides(const byte *pExtendedCapabilityFlags, int length);
    void setMaxNmVer(int maxNmVer);
    void setDynamicFbCapacity(int dynamicFbCapacity);

    int getNumOptionalCapabilitiesBytes() { return m_numOptionalCapabilitiesBytes; }
    int getCapabilitiesLength() { return m_numOptionalCapabilitiesBytes + ND_CAPABILITY_LENGTH; }
    int getDynamicFbCapacity() { return m_dynamicFbCapacity; }
    boolean getExtendedCapabilityFlagOverrides(byte *pExtendedCapabilityFlags);
    int getMaxNmVer() { return m_maxNmVer; }

private:
    int getEcsAddressCapacity();
    int getEatAddressCapacity();
    boolean isEatAddressCountRequired();
    void setIncludeEatAddressCount(boolean unconditional);
    void addOptionalCapability(int offset, int size);
};


typedef enum
{
    NodeDefVer_original = 1,         // 1 - Original
    NodeDefVer_ECS,                  // 2 - ECS commands   
    NodeDefVer_BigNvTypes,           // 3 - 2 byte NV types
} LtNodeDefVersion;
#define CURRENT_NODEDEF_VER NodeDefVer_BigNvTypes

class NodeDefMaker : public NodeDef
{
private:
	int m_nLastNvIndex;
	int m_nLastNvOffset;
    int m_numXnvtRecords; 
    boolean m_includeXnvt;

	NdErrorType makeSiDataHeader(unsigned char*& pSiData, int &offset, int &len);
	NdErrorType makeSiNvs(unsigned char*& pSiData, int &offset, int &len);
	NdErrorType makeSiSdString(unsigned char*& pSiData, int& offset, int& len);
	NdErrorType makeSiNvSummary(unsigned char*& pSiData, int &offset, int &len);
	NdErrorType makeSiTrailer(unsigned char* &pSiData, int &offset, int &len);
	NdErrorType makeSiXnvt(unsigned char* &pSiData, int &offset, int &len);

	NdErrorType processXifFile(char* szXifFile, int bExplodeNvArrays);

public:
	NodeDefMaker();
	virtual ~NodeDefMaker();

	// Reset the object.  Allows one object to be used to process multiple definitions.
	void reset();

	// Process a XIF or XFB file.  
	NdErrorType processXif(char *szXifFile, int bExplodeNvArrays);

	// Process an SI data array and read only data.  If you opt not to read the 
	// entire SI data array but only a portion, NodeDef will end up with a partial 
	// definition.  It returns ND_PARTIAL_DEFINITION if more SI data is needed to 
	// complete the definition.  To complete the definition, call this function 
	// again and it will resume where is left off.
	NdErrorType processReadOnlyData(unsigned char* pReadOnlyData, int len);
	NdErrorType processSiData(unsigned char* pSiData, int len);

	// Create XIF or XFB
	NdErrorType makeXif(char *szXifFile);
	NdErrorType makeXfb(char *szXfbFile);

	// Create SI data.  Pointer to read only data block and SI data block is 
		// allocated by this routine and must be freed by caller.
	NdErrorType makeSiData(unsigned char*& pReadOnlyData, int& roLen, 
						   unsigned char*& pSiData, int& siLen);
	NdErrorType makeSiData(unsigned char* pSiData, int offset, int len);
	NdErrorType makeSiDataAliasRelative(unsigned char* pSiData, int offset, int len);
	NdErrorType makeReadOnlyData(unsigned char*& pReadOnlyData, int& len);
	NdErrorType makeSiDataArray(unsigned char*& pSiData, int& len);
	int siComputeLength();
    int getSiDataLength() { return (m_siVersion == 1) ? siComputeLength() : (VER2_HDR_LEN + getCapabilitiesLength()); }  // returns the length of SI data
	void getVersion(int& nMajor, int& nMinor);
};

#endif
