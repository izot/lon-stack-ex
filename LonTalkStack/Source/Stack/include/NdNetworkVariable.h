//
// NdNetworkVariable.h
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

#ifndef _NDNETWORKVARIABLE_H
#define _NDNETWORKVARIABLE_H

#include <nodedef1.h>

// NV definition flags - useable by applications.  First 7 flags values match
// SI data definitions.  Last 3 flags also are related to SI data definitions.
#define NV_SD_CONFIG_CLASS			0x00000001	// Config class NV - keep persistently
#define NV_SD_AUTH_CONFIG			0x00000002	// Authentication attribute configurable
#define NV_SD_PRIORITY_CONFIG		0x00000004	// Priority attribute configurable
#define NV_SD_SERVICE_CONFIG		0x00000008	// Service type configurable
#define NV_SD_OFFLINE				0x00000010	// Only change when offline
#define NV_SD_POLLED				0x00000020	// Polled output or polling input
#define NV_SD_SYNC					0x00000040	// Sync NV

#define NV_SD_CHANGEABLE			0x00000080	// Causes NV arrays to be exploded in SI data.
#define	NV_SD_PRIORITY				0x00000100	// Default to priority
#define NV_SD_AUTHENTICATED			0x00000200	// Default to authenticated
#define NV_SD_ACKD					0x00000400	// Default to ackd service
#define NV_SD_UNACKD_RPT			0x00000800	// Default to unackd rpt service
#define NV_SD_UNACKD				0x00001000	// Default to unackd service

#define NV_SD_AUTH_NONCONFIG		0x00002000	// Authentication attribute non-configurable
#define NV_SD_PRIORITY_NONCONFIG	0x00004000	// Priority attribute non-configurable
#define NV_SD_SERVICE_NONCONFIG		0x00008000	// Service type non-configurable

#define NV_SD_TEMPORARY				0x00010000	// NV is temporary and no persistence should be kept.
#define NV_SD_SOURCE_SELECTION		0x00020000	// Use source selection (for private NVs)
#define NV_SD_READ_BY_SELECTOR		0x00040000  // NV is to be read by selector
#define NV_SD_WRITE_BY_INDEX		0x00080000	// NV is to be written by index
#define NV_SD_NM_AUTH				0x00100000	// Target node has NM auth turned on
#define NV_SD_CHANGEABLE_LENGTH		0x00200000	// NV length may be changed

// Flag values for internal use only

#define NV_SD_NONCONFIG_SHIFT		12			// Delta of config mask and nonconfig masks
#define NV_SD_NONCONFIG_MASK		(NV_SD_AUTH_CONFIG|NV_SD_PRIORITY_CONFIG|NV_SD_SERVICE_CONFIG)
#define NV_SD_FLAGS_MASK			(NV_SD_CONFIG_CLASS|NV_SD_OFFLINE|NV_SD_POLLED|NV_SD_SYNC)
#define NV_SD_PUBLIC_MASK			0x0000FFFF

#define NV_SD_PRIVATE				0x00400000	// Private NV flag
#define NV_SD_MASTER_DELETE			0x00800000	// Indicates owner of master
#define NV_SD_DESCRIPTION_CHANGE	0x01000000	// Indicates non-static description
#define NV_SD_FORKED				0x02000000	// Forked NV array elements
#define NV_SD_MAX_RATE				0x04000000	// Has max rate estimate
#define NV_SD_RATE					0x08000000	// Has rate estimate
#define NV_SD_DESCRIPTION			0x10000000	// Has description
#define NV_SD_NAME					0x20000000	// Has name
#define NV_SD_DYNAMIC				0x40000000	// Dynamic NV
#define NV_SD_OUTPUT				0x80000000	// Output NV

// NV type (SNVT unless one of values below)
typedef unsigned short NdNvType;
typedef unsigned char NdLegacyNvType;
#define NV_TYPE_USER				0
#define NV_TYPE_LENGTH_CHANGEABLE	0xff

#define NO_RATE_ESTIMATE			65535

class NdCollectionElement;
class NodeDef;

// Base length of NV serial data
#define ND_BASE_SERIAL_LENGTH (18 + ND_NAME_BASE_LEN + 4)

class LTA_EXTERNAL_CLASS NdNetworkVariable : public NdCollectionElement
{
	// When LonMark stuff is added, this collection needs to be turned on.
	//DEFINE_COLLECTION(ConfigProp)

private:
	int			m_nNvIndex;		// NV index
	int			m_nFlags;
	int			m_nLength;		// Length of each NV element
	int			m_nArrayLength;	// Length of the array (0 if scalar)
	NdNvType	m_nvType;
	char*		m_szDescription;// User description (allocated separately since could 
								// be quite long).
	int			m_nRateEstimate;
	int			m_nMaxRateEstimate;
	NdNetworkVariable* m_pMaster;

	NodeDef*	m_pNodeDef;

	int			m_nLengthFromDevice;	// Network variable length as determined from device.
	int			m_nLengthForUpdate;		// Network variable length as determined by application (specified in
										// last setNvData call).  For non-changeable length NVs, this is always
										// the same as m_nLength;

protected:
	int			getIntFlags() { return m_nFlags; }

	void		setLengthFromDevice(int l) { m_nLengthFromDevice = l; }

	// NV length as determined by last application call to setNvData.  This is the 
	// length used for updates.
	// FB: These need to be made public. See below for 'CurLength' calls (a better public name).
	int			getLengthForUpdate()	{ return m_nLengthForUpdate; }
	void		setLengthForUpdate(int l);

public:
	//
	// pSdString - pointer to SD string copied into object.
	// pNvName - name contents copied into object.
	NdNetworkVariable(NdNvType type, int nLength, int nFlags=0, int nArrayLength=0,
		const char* pSdString=NULL, const char* pNvName=NULL, int rateEstimate=NO_RATE_ESTIMATE, int maxRateEstimate=NO_RATE_ESTIMATE);

	NdNetworkVariable(NdNetworkVariable* pNv);

	virtual ~NdNetworkVariable();

	void strip();

	boolean hasExtensionData();

	void setParent(NodeDef* pNodeDef) { m_pNodeDef = pNodeDef; }
	NodeDef* getParent() { return m_pNodeDef; }
	void lock();
	void unlock();

	int getFlags();

	int getFlag(int flag) { return (getFlags() & flag) ? true : false; }

	int getOutput() { return getFlag(NV_SD_OUTPUT); }
	int getChangeable() { return getFlag(NV_SD_CHANGEABLE); }
	int getForked() { return getFlag(NV_SD_FORKED); }
	int getDynamic() { return getFlag(NV_SD_DYNAMIC); }
	int getStatic() { return !getDynamic() && !getPrivate(); }
	int getHasName() { return getFlag(NV_SD_NAME); }
	int getHasSd() { return getFlag(NV_SD_DESCRIPTION); }
	int getSync() { return getFlag(NV_SD_SYNC); }
	int getPrivate() { return getFlag(NV_SD_PRIVATE); }
	int getAuth() { return getFlag(NV_SD_AUTHENTICATED); }
	int getPriority() { return getFlag(NV_SD_PRIORITY); }
	int getTemporary() { return getFlag(NV_SD_TEMPORARY); }
	int getSourceSelection() { return getFlag(NV_SD_SOURCE_SELECTION); }
	int getReadBySelector() { return getFlag(NV_SD_READ_BY_SELECTOR); }
	int getWriteByIndex() { return getFlag(NV_SD_WRITE_BY_INDEX); }
	int getNmAuth() { return getFlag(NV_SD_NM_AUTH); }

#if PRODUCT_IS(ILON) || PRODUCT_IS(VNISTACK)
    // Currently the i.lon and VNIStack use promiscuous NV lengths, which will
    // adjust thier size based on the last update (not to exceed the initial size.
	int getChangeableLength() { return TRUE; }
#else
	int getChangeableLength() { return getFlag(NV_SD_CHANGEABLE_LENGTH); }
#endif

	NdNvType getType() { return m_nvType; }
	void setType(int nvType) { m_nvType = nvType; }

	int isFirst();
	int isMaster();

	// Bad name! This is deprecated; use getMaxLength()
	int getLength() { return m_nLength; }	// m_nLength is a bad name too
	// New length calls
	int getMaxLength() { return m_nLength; }	// m_nLength is a bad name too. How about m_nMaxLength?
	// These replace the 'LengthForUpdate' protected calls above.
	int getCurLength() { return m_nLengthForUpdate; } // How about m_nCurLength?
	void setCurLength(int len) { if (getChangeableLength()) m_nLengthForUpdate = len; }


	int getRateEstimate();
	int getMaxRateEstimate();
	void setRateEstimate(int re);
	void setMaxRateEstimate(int mre);

	int getArrayLength();		// Network array length: 
								//  0 for non-array.
								//  0 for forked array element.
								//  count for non-forked array or master.
	int getElementCount();		// Same as array length but returns 1 rather than 0.
	int getAppInstanceCount();	// Number of NV instances given to app.
								//  1 for non-array
								//  1 for non-forked array.
								//  count for forked array.
	int isArrayElement();		// 1 for array element, else 0.

	void setName(const char* szName);

	// Not thread-safe
	const char* getDescription();
	
	void getDescription(char* szDesc, int maxLength);
	void setDescription(const char* szDesc, int offset, int length);

	int getNvArrayIndex() { return getMaster()->getNvIndex(); }
	int getNvIndex() { return m_nNvIndex; }
	void setNvIndex(int nIndex) { m_nNvIndex = nIndex; }

	NdNetworkVariable* getMaster() { return m_pMaster; }
	void setMaster(NdNetworkVariable* pMaster) { m_pMaster = pMaster; }

	NdErrorType makeSiExtensionData(unsigned char* pBuf, int len, int& resultLen);

	char* getName() { return NdCollectionElement::getName(); }
	void getName(char* szName, int nLength=ND_NAME_LEN);
	void getArrayName(char* szName);
	char* getArrayName() { return getMaster()->NdCollectionElement::getName(); }

	// Serialize the object.  When done, pBuffer points to just past data.
	void serialize(byte* &pBuffer, int &len);
	int getSerialLength();

	// NV length as determined by the device (for array, length of each element)
	// - uses length of NV data from last NV poll or fetch so will return 0 if 
	// no NV poll or fetch has occurred.	
	int	getLengthFromDevice() { return m_nLengthFromDevice; }
};

#endif
