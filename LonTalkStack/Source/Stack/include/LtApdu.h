#ifndef LTAPDU_H
#define LTAPDU_H

//
// LtApdu.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtApdu.h#2 $
//

#include <RefQues.h>
#if PRODUCT_IS(VNISTACK)
#include  <sys\timeb.h>
#endif


#define LT_APDU_NETWORK_VARIABLE        0x80
#define LT_APDU_NETWORK_VARIABLE_MASK   0x80
#define LT_APDU_NETWORK_MANAGEMENT      0x60
#define LT_APDU_NETWORK_DIAGNOSTIC      0x50
#define LT_APDU_FOREIGN                 0x40
#define LT_APDU_FOREIGN_MASK            0xf0
#define LT_APDU_MESSAGE                 0x00
#define LT_APDU_MESSAGE_MASK            0xc0

#define LT_APDU_ENHANCED_PROXY			0x4D
#define LT_ENHANCED_PROXY_SUCCESS		0x4D
#define LT_ENHANCED_PROXY_FAILURE		0x4C
#define LT_ADDP_INBOUND					0x4A	// ATM message inbound
#define LT_ADDP_OUTBOUND				0x45	// ATM message outbound

#define LT_IS_ATM(c)					((c) == LT_ADDP_INBOUND || (c) == LT_ADDP_OUTBOUND)

#define LT_APDU_NORMAL   0
#define LT_APDU_RESPONSE   1
#define LT_APDU_SUCCESS   2
#define LT_APDU_FAILURE   3

// This is used for the address index to indicate an explicit address
#define LT_EXPLICIT_ADDRESS		-1

class LtMsgIn;
class LtDeviceStack;
class LtPktInfo;
class LtNetworkVariableConfiguration;

class LTA_EXTERNAL_CLASS LtApdu: public LtPacketControl, public LtQue {

private:
	boolean		m_bOutput;
	byte		m_data[MAX_LPDU_SIZE+1];// Use MAX_LPDU_SIZE rather than 
                                        // MAX_APDU_SIZE to allow layer 2 stacks to receieve
                                        // full packets in the data portion of the APDU.
    WORD		m_len;
	byte		m_nNvDataOffset;

protected:
    LtApdu(LtBlob &blob);
    void package(LtBlob *pBlob);
    void packageMyData(LtBlob *pBlob);

protected:
    void setOutgoingAddress(LtOutgoingAddress& outgoingAddress);

public:
    LtApdu(boolean bOutput);
    LtApdu(boolean bOutput, LtApdu* request);

	virtual ~LtApdu() {}

    void reinit();

    boolean valid() { return m_len > 0; }
    int getLength() { return m_len; }

	void resetLength() { m_len = 0; }

	void flipNvCode();
    int getFlippedNvCode();
    boolean isRequest();
    int getDataLength();

    byte* getCodeAndData() { return m_data; }
    byte* getData() { return &m_data[1]; }

    void getData(byte data[], int offset, int length);
    byte getData(int i);
	void setLength(int length) { m_len = length + 1; }
    void setData(int i, int value);
    LtErrorType setNvData(byte data[], int len);
    LtErrorType setData(byte data[], int offset, int len);
    void setCodeAndData(byte data[], int len);
    void setCodeAndData(LtApdu* pApdu);
    int getCode();
    void setTxFailure();
    void setValidResponse();
    void setCode(int newCode);
    LtRefId& getTag();
    int getNvDataOffset() { return m_nNvDataOffset; }
	void setNvDataOffset(int offset) { m_nNvDataOffset = offset; }
    LtErrorType getNvData(byte** ppData, int &len, int lengthFromDevice, boolean bChangeableLength);
    boolean isNetworkVariable();
    boolean isNetworkVariableMsg();
	boolean isNetworkDiagnostic();
    boolean isApplMsg();
    boolean isForeignMsg();
    LtErrorType getSelector(int& selector, boolean& bOutput);
    boolean forNetworkManager();
	boolean forProxyHandler();
    int getNmCode(boolean success);
	static int getNmCode(int code, boolean success);
	boolean getNmPass(int code);
	boolean getProxyRelay();
	void setUpProxy();

	boolean getOutput() { return m_bOutput; }
	boolean getResponse() { return getServiceType() == LT_RESPONSE; }
};

class LtNvId : public LtObject
{
    DefineInt(NvIndex)
    DefineInt(NvIncarnationNumber)
public:
    LtNvId(int nvIndex, int nvIncarnationNumber) 
    {
        setNvIndex(nvIndex);
        setNvIncarnationNumber(nvIncarnationNumber);
    }
    ~LtNvId() {}
};

class LTA_EXTERNAL_CLASS LtApduIn : public LtApdu, public LtIncomingAddress
{
private:
	LtTypedVector<LtNvId>*m_pNvIds;
	bool	m_bViaLtNet;
	int		m_phaseReading;
	bool	m_bSsiValid;
	byte	m_nSsiReg1;
	byte	m_nSsiReg2;

#if PRODUCT_IS(VNISTACK)
	_timeb  m_timeOfDay;
	DWORD	m_highResTimeStamp;	
#endif
	
	// The following are valid only for L2 packets.
	byte			m_nL2PacketType;
	boolean			m_bIsValidL2Packet;		

protected:
    LtApduIn(LtBlob &blob) : LtApdu(blob), LtIncomingAddress(blob) 
	{
		m_pNvIds = NULL;
		m_bViaLtNet = false;
		m_bSsiValid = false;
	};
    void package(LtBlob *pBlob);

public:
	LtApduIn() : LtApdu(false) 
	{
		m_pNvIds = NULL;
		m_nL2PacketType = 0;
		m_bIsValidL2Packet = false;
		m_bSsiValid = false;
	}
	~LtApduIn()
	{
        if (m_pNvIds != NULL)
        {
            m_pNvIds->removeAllElements(TRUE);
		    delete m_pNvIds;
        }
	}
    void init(LtDeviceStack* pStack, LtPktInfo* pPkt, LtRefId& refId);
	void addNvIndex(int nvIndex, int nvIncarnationNumber);
	boolean getNextNvIndex(LtVectorPos &pos, int &nvIndex, int &nvIncarnationNumber);

	bool	getViaLtNet()			{ return m_bViaLtNet; }
	void	setViaLtNet(bool b)		{ m_bViaLtNet = b; }

	int		getPhaseReading()		{ return m_phaseReading; }
	void    setPhaseReading(int v)	{ m_phaseReading = v; }

	bool 	getSsi(byte& reg1, byte& reg2);
	void 	setSsi(bool isValid, byte reg1=0, byte reg2=0);

	boolean					getIsValidL2Packet()			{ return m_bIsValidL2Packet; }
	byte					getL2PacketType()				{ return m_nL2PacketType; }
#if PRODUCT_IS(VNISTACK)
	void					getTimeStamp(_timeb &timeOfDay, DWORD &highResTimeStamp)
	{
		timeOfDay = m_timeOfDay;
		highResTimeStamp = m_highResTimeStamp;	
	}
#endif


	void setLayer2Info(class LtPktInfo *pL2Pkt);
};

class LTA_EXTERNAL_CLASS LtApduOut : public LtApdu, public LtOutgoingAddress
{
private:
	LtNetworkVariableConfiguration* m_pNvc;
	LtMsgOverride				    m_override;
    boolean                         m_downlinkEncryption;   // Encrypt this message
    boolean                         m_uplinkEncryption;     // Encrypt response to this message
protected:
    LtApduOut(LtBlob &blob);
    void package(LtBlob *pBlob);

public:
	LtApduOut();
    LtApduOut(LtApduIn* pRequest);
    boolean getTurnaroundOnly() { return getTurnaround() && !isBoundExternally(); }
	LtNetworkVariableConfiguration* getAliasNvc() { return m_pNvc; }
	void setAliasNvc(LtNetworkVariableConfiguration* pNvc) { m_pNvc = pNvc; }
	void setOverride(LtMsgOverride* pOverride);
	void applyOverride(boolean bLayer6);
	LtMsgOverride* getOverride();

        // Encryption values are initially FALSE, and can be set at any layer, but can never
        // be cleared again.
    void setDownlinkEncryption()    { m_downlinkEncryption = TRUE; }    
    boolean getDownlinkEncryption() { return m_downlinkEncryption; }
    void setUplinkEncryption()      { m_uplinkEncryption = TRUE; }
    boolean getUplinkEncryption()   { return m_uplinkEncryption; }
};

#endif
