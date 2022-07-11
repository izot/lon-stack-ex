#ifndef LTPKTINFO_H
#define LTPKTINFO_H
//
// LtPktInfo.h
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
//
#if PRODUCT_IS(VNISTACK)
#include  <sys\timeb.h>
#endif

typedef struct  
{
	LtPduType		m_nEnclPdu;
	int				m_nType;
} LtTypeConversion;

#define LT_NUM_PDU_TYPE 6

#define LT_CHALLENGE_LENGTH 8

class LtLreClient;


// These literals are used for version numbers in the second byte of the NPDU
#define LT_L2_PKT_VER_LS_LEGACY_MODE    0   // LS Legacy mode.  Uses 4 bit transaction numbers
#define LT_L2_PKT_VER_ENCAPSULATED_IP   1   // Encapsulated IP message.  Not used for Lontalk Services.
#define LT_L2_PKT_VER_LS_ENHANCED_MODE  2   // LS Enhaced mode.  Uses 12 bit transaction numbers
class LtPktData
{
protected:
#if PRODUCT_IS(VNISTACK)
	_timeb  m_timeOfDay;
	DWORD	m_highResTimeStamp;	
#endif

	// Layer 2 Information
	boolean			m_bPriority;
	boolean			m_bAltPath;
	byte			m_nDeltaBacklog;
	byte			m_nPhaseReading;
	byte			m_nL2PacketType;	    // Incoming L2 packet type (first byte of SICB).
	bool			m_bSsiValid;
	byte			m_nSsiReg1;
	byte			m_nSsiReg2;
#if PRODUCT_IS(ILON)
	boolean			m_bRouteMultipleNoXmit;	// If true, indicates that we should not restrict routing
											// of a packet to a single client on a channel (like m_bRouteMultiple).
											// However, this flag instructs the normally excluded port client(s) to
											// not actually transmit the packet, where m_bRouteMultiple does not.
											// This is to allow the iLON to collect ALL packets to send to LonScanner,
											// which is done in the port driver on the iLON.
#endif

	// Layer 2/3 Information
	boolean			m_bIsValidL2L3Packet;	// A valid LONTalk packet (as opposed to a CRC error, for example)
											// Can be used to indicate addressing (L3) parsed OK (or not)
	// Layer 3 Information
	LtDomain		m_domain;
	LtSubnetNode	m_subnetNode;
	LtAddressFormat m_nAddressFormat;
	LtUniqueId		m_uniqueId;
	byte			m_nDestSubnet;
	byte			m_nDestNode;			
	byte			m_nGroup;
	byte			m_nMember;
	byte			m_nVersion;
	LtPduType		m_nEnclPdu;

	// Layer 4/5 Information
	boolean			m_bAuth;
	LtServiceType	m_nServiceType;
	word            m_nTxNumber;

    LtAddressFormat	m_origFmt;
	boolean			m_bCrcFixup;
	boolean			m_bRouteMultiple;		// If true, indicates that we should not restrict routing
											// of a packet to a single client on a channel.

	enum { LT_NONE, LT_L2, LT_L4 } m_parseState;

	// Ip information [ intentionally not cleared in constructor ]
	ULONG			m_ipSrcAddr;
	word			m_ipSrcPort;
	ULONG			m_timestamp;
	ULONG			m_sequence;
	const char*		m_pCrumb;		// debugging cell
	boolean			m_bIgnoreAuthentication;

	byte			m_pktFlags;		// Optional flags for transmission
									// WARNING! We can't use 'm_flags' because
									// a stupid Tornado include file (mbuf.h) does
									// a #define of m_flags to something else!
									// It also #define's several other common names,
									// like m_next, m_len, m_data, and m_type!
									// Someone should be shot for this.

public:
	LtPktData() 
	{
		m_nL2PacketType = 0;
		m_bIsValidL2L3Packet = false;
	}

	virtual ~LtPktData() {}

	void setIncomingSicbData(boolean bIsValidL2Packet, byte l2PacketType);

	// Layer 2 Information
	inline boolean			getAltPath()					{ return m_bAltPath; }
	inline void				setPriority(boolean bPri)		{ m_bPriority = bPri; }
	inline void				setAltPath(boolean bAltPath)	{ m_bAltPath = bAltPath; }
	inline void				setDeltaBacklog(int nBl)		{ m_nDeltaBacklog = nBl; }
	inline int				getPhaseReading()				{ return m_nPhaseReading; }
	inline void				setPhaseReading(int value)		{ m_nPhaseReading = value; }
	bool					getSsi(byte& reg1, byte& reg2);
	void					setSsi(bool isValid, byte reg1, byte reg2);

	// Layer 3 Information
	inline byte				getVersion(void)		        { return m_nVersion; }
	inline void				setVersion(int nVersion)		{ m_nVersion = nVersion; }
	inline void				setEnclPdu(LtPduType type)		{ m_nEnclPdu = type; }
	inline void				setDomain(LtDomain& domain)		{ m_domain = domain; }
	inline void				setSubnetNode(LtSubnetNode& sn) { m_subnetNode.set(sn); }
	inline void				setAddressFormat(LtAddressFormat fmt) { m_nAddressFormat = fmt; }
	inline void				setDestSubnet(int nSubnet)		{ m_nDestSubnet = nSubnet; }
	inline void				setDestNode(int nNode)			{ m_nDestNode = nNode; }
	inline void				setDestGroup(int nGroup)		{ m_nGroup = nGroup; }
	inline void				setDestMember(int nMember)		{ m_nMember = nMember; }
	inline void				setUniqueId(LtUniqueId& id)		{ m_uniqueId = id; }

	// Layer 4 Information
	inline boolean			getAuth()						{ return m_bAuth; }
	inline LtAddressFormat	getOrigFmt()					{ return m_origFmt; }
	inline int				getTxNumber()					{ return m_nTxNumber; }
	inline int				getOrigAddress()				{ return m_nGroup; }
	inline void				setAuth(boolean bAuth)			{ m_bAuth = bAuth; }
	inline void				setTxNumber(int nTxNumber)		{ m_nTxNumber = nTxNumber; }
	inline void				setOrigFmt(LtAddressFormat fmt)	{ m_origFmt = fmt; }
	inline void				setServiceType(LtServiceType t) { m_nServiceType = t; }
	inline void				setOrigAddress(int nAddress)	{ m_nGroup = nAddress; }
	inline void				encrypt(byte* pMsg, int nMsgLen, byte* pKey);

	// Control info
	inline boolean			getCrcFixup()					{ return m_bCrcFixup; }
	inline void				setCrcFixup(boolean bValue)		{ m_bCrcFixup = bValue; }
	inline boolean			getRouteMultiple()				{ return m_bRouteMultiple; }
	inline void				setRouteMultiple(boolean bValue){ m_bRouteMultiple = bValue; }
#if PRODUCT_IS(ILON)
	inline boolean			getRouteMultipleNoXmit()		{ return m_bRouteMultipleNoXmit; }
	inline void				setRouteMultipleNoXmit(boolean bValue){ m_bRouteMultipleNoXmit = bValue; }
#endif

	// IP info
	inline ULONG			getIpSrcAddr()					{ return m_ipSrcAddr; }
	inline word				getIpSrcPort()					{ return m_ipSrcPort; }
	inline void				setIpSrcAddr( ULONG ipAddr )	{ m_ipSrcAddr = ipAddr; }
	inline void				setIpSrcPort( word ipPort )		{ m_ipSrcPort = ipPort; }
	inline void				setTimestamp( ULONG ts )		{ m_timestamp = ts; }
	inline ULONG			getTimestamp()					{ return m_timestamp; }
	inline void				setSequence( ULONG sn )			{ m_sequence = sn; }
	inline ULONG			getSequence()					{ return m_sequence; }
	inline void				setCrumb( const char* cmb )			{ m_pCrumb = cmb; }
	inline const char*			getCrumb()						{ return m_pCrumb; }
	inline void				setIgnoreAuthentication( boolean bIgnore=true)
	{	m_bIgnoreAuthentication = bIgnore;
	}
	inline boolean			ignoreAuthentication()
	{	return m_bIgnoreAuthentication;
	}
	void					setCrc();

public:
	// Layer 2 Information
	inline boolean			getPriority()					{ return m_bPriority; }

	// Layer 3 Information
	inline boolean			isValidVersion()				
    { 
        return m_nVersion == LT_L2_PKT_VER_LS_LEGACY_MODE ||
               m_nVersion == LT_L2_PKT_VER_LS_ENHANCED_MODE;
    }
	inline LtPduType		getEnclPdu()					{ return m_nEnclPdu; }

	// Source address information
	inline LtDomain&		getDomain()						{ return m_domain; }
	inline LtSubnetNode& 	getSourceNode()					{ return m_subnetNode; }
	
	// Destination address information
	inline LtAddressFormat	getAddressFormat()				{ return m_nAddressFormat; }

	// LT_BROADCAST, LT_SUBNET_NODE, LT_GROUP_ACK
	inline int	 			getDestSubnet()					{ return m_nDestSubnet; }
	// LT_SUBNET_NODE
	inline int				getDestNode()					{ return m_nDestNode; }
	// LT_GROUP, LT_GROUP_ACK
	inline int				getDestGroup()					{ return m_nGroup; }
	// LT_UNIQUE_ID
	inline LtUniqueId&		getUniqueId()					{ return m_uniqueId; }
	// LT_GROUP_ACK
	inline int				getDestMember()					{ return m_nMember; }

	// Layer 4 Information
	inline LtServiceType	getServiceType()				{ return m_nServiceType; }
	inline bool				isUnackd()						{ return getServiceType() == LT_UNACKD; }

	int hashCode();

	boolean					getIsValidL2L3Packet()		    { return m_bIsValidL2L3Packet; }
	boolean					getL2PacketType()				{ return m_nL2PacketType; }
#if PRODUCT_IS(VNISTACK)
	void					getTimeStamp(_timeb &timeOfDay, DWORD &highResTimeStamp)
	{
		timeOfDay = m_timeOfDay;
		highResTimeStamp = m_highResTimeStamp;	
	}
#endif
		// Functions to get/set optional flags.  These flags control transmission attributes such as
	// zero crossing synchronization and attenuation.
	inline byte				getFlags()						{ return m_pktFlags; }
	inline void				setFlags(byte flags)			{ m_pktFlags = flags; }
};

class LtPktInfo : public LtPktData, public LtMsgRef
{
private:
    LtLreClient*    m_pSourceClient;
	LtLreClient*	m_pDestClient;

	byte*			m_pData;		// Pointer to current parse/build position
	WORD			m_nLen;			// Length of data to be parsed or length of data built.

protected:
	static LtServiceType nToServiceType[LT_AUTHPDU+1][LT_NUM_PDU_TYPE];
	static LtTypeConversion nFromServiceType[LT_SERVICE_TYPES];

public:
    inline                  LtPktInfo() { setFlags(0); }
	virtual				   ~LtPktInfo() {}

	// Upper layer information
	// These routines can be used to access the remaining packet data after
	// the packet has been parsed.
	byte*		getData()						{ return m_pData; }
	int			getLen()						{ return m_nLen; }
	void		initPacket();
	void		reset()							{ m_parseState = LT_NONE; }

	// First pass parse packet function.  This function establishes
	// the layer 2 and layer 3 information.  If the packet is to
	// actually be routed, then parsePktL4 must be called to set
	// the layer 4 information.
	LtErrorType parsePktLayers2and3();
	LtErrorType parsePktLayer4();
	LtErrorType buildPkt(boolean bLayer2 = false);

	void domainFixup(LtDomain& domain, int nSubnet);
	void subnetFixup(int nSubnet);
	void setCrc();

    void resetApdu();
    LtErrorType setData(byte* pData, int nLen);
	LtErrorType addData(byte* pData, int nLen);
    void removeData(byte* pData, int nLen);
    int getLength() { return m_nLen; }
	void getChallengeReply(byte* pData);
	void setChallengeReply(byte* pData);

    LtLreClient* getSourceClient()              { return m_pSourceClient; }
    void setSourceClient(LtLreClient* pClient)  { m_pSourceClient = pClient; }

    LtLreClient* getDestClient()                { return m_pDestClient; }
    void setDestClient(LtLreClient* pClient)    { m_pDestClient = pClient; }

	void copyMessage(LtPktInfo* pPkt, boolean bDeepClone)
	{
		*(LtPktData*)this = *(LtPktData*)pPkt;
		m_pSourceClient = pPkt->m_pSourceClient;
		m_nLen = pPkt->m_nLen;
		if (bDeepClone)
		{
			// Deep clone - need to set pointer relative to new data block.
			m_pData = getDataPtr() + (pPkt->m_pData - pPkt->getDataPtr());
		}
		else
		{
			m_pData = pPkt->m_pData;
		}
	}

	// always a deep copy of a packet moving the data portion to the beginning
	// of the destination block.
	void copyMessage2( LtPktInfo* pPkt )
	{
		*(LtPktData*)this = *(LtPktData*)pPkt;
		LtMsgRef::copyMessage( pPkt );
		m_pSourceClient = pPkt->m_pSourceClient;
		m_nLen = pPkt->m_nLen;
		// need to set pointer relative to new data block.
		m_pData = getBlock() + (pPkt->m_pData - pPkt->getDataPtr());
	}

	void dump();
	
    // Returns the destination address data in the form used by OMA authentication
    void getOmaDestData(byte *pData);

    void setUseLsEnhancedMode(boolean bUseLsEnhancedMode) 
    {
        setVersion(bUseLsEnhancedMode ? LT_L2_PKT_VER_LS_ENHANCED_MODE : LT_L2_PKT_VER_LS_LEGACY_MODE);
    }

    boolean getUseEnhancedMode(void)
    {
        return getVersion() == LT_L2_PKT_VER_LS_ENHANCED_MODE;
    }
};
#endif
