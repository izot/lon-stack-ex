#ifndef LonTalkStack_h
#define LonTalkStack_h
//
// LonTalkStack.h
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

#define NM_STACK_SIZE (16*1024)

class LonTalkStack: public LonTalkNode, public LtNetwork {
private:
    boolean terminatePending;
    boolean m_bFlushPending;
    boolean resetRequest;
    
    boolean regValueReceived;
	boolean regValueFailure;
    int regValue;
    boolean specialPurpose;
    boolean m_bXcvrIdUnknown;
	int m_nXcvrId;
    
    LtApplication* lonApp;
	LtDeviceStack* m_pStack;
	LtIpPortClient* m_pLtpc;
	LtLinkStub m_linkStub;
    static const boolean m_xidKeyContainsPath;
public:
	LonTalkStack(LtLogicalChannel* pChannel, LtLreServer* pLre);
	virtual ~LonTalkStack();

	LtDeviceStack* getStack() { return m_pStack; }
	void setStack(LtDeviceStack* pStack)
	{
		m_pStack = pStack;
	}

	LtLink* getLink(LtLogicalChannel* pChannel);

    void halt();
    
    void resume();
    void readyToReceive();

	void setComm();

    LtErrorType fetchXcvrReg(byte* data, int offset);
    void reset(boolean bBoot=false);
    
    int getModeAndState();
    LtErrorType clearStatus();
    void flush(boolean commIgnore);
    void flushCancel();
	boolean flushPending();
    void terminate();
    void sendServicePinMessage();
    LtApdu* getApdu();
    void sendMessage(LtApduOut* apdu);
    void setCommParams(LtCommParams* pCps);
    boolean isSpecialPurpose();
    boolean getEepromLock();
	int getXcvrId() { return m_nXcvrId; }
    void setXcvrId(int xcvrId);
    void setXcvrReg(byte* xcvrReg);
    void setMem(int addr, int size);
    void shutDown();
	void setDriverComm(LtCommParams* pCps);

	// LtNetwork functions
	void registerLink(LtLink& link) {}
	void packetReceived(void* referenceId,
								int nLengthReceived,
								boolean bPriority,
								int receivedSlot,
								boolean isValidLtPacket, 
								byte    l2PacketType,
								LtSts sts,
								byte	ssiReg1=0, 
								byte	ssiReg2=0) {}
	void packetComplete(void* referenceId, LtSts sts ) {}

	void resetRequested();
	void terminateCompleted();
	void reportTransceiverRegister(int n, int value, LtSts sts);

};

#endif
