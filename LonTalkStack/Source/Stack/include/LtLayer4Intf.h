#ifndef LTLAYER4INTF_H
#define LTLAYER4INTF_H
//
// LtLayer4Intf.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtLayer4Intf.h#2 $
//

// 
// Define an abstract class which serves as the interface to either the
// VNI layer 4 or a L5 MIP.
//
class LtLayer4Intf
{
private:
	LtErrorType		m_startError;

public:
	LtLayer4Intf()
	{
		m_startError = LT_NO_ERROR;
	}

	LtErrorType		getStartError() { return m_startError; }
	void			setStartError(LtErrorType err) { m_startError = err; }

	virtual ~LtLayer4Intf() {}

	virtual void halt() {}
	virtual void resume() {}

	virtual void shutdown() {}

	virtual void apduInAvailable() {}
    virtual void send(LtApduOut* pApdu, boolean wait = true, boolean throttle = true) = 0;
	virtual void queuePacket(LtPktInfo* pPkt) = 0;
	virtual void reset() = 0;
	virtual void resetTx() = 0;
	virtual void terminate() = 0;
	virtual void setStack(LtDeviceStack* pStack) = 0;
	virtual void setTxIdLifetime(int duration) = 0;
	virtual void setResetRequested() = 0;
	virtual void setErrorLog(int err) = 0;
	virtual void setOffline(boolean bOffline) {}
	virtual boolean getCommIgnore() = 0;
	virtual void setCommIgnore(boolean value) = 0;
	virtual int getXcvrId() = 0;
	virtual void setXcvrId(int id) = 0;
    virtual void getNmVersion(int &nmVersion, int &nmCapabilities) = 0;

	// Configuration API
	virtual LtErrorType getDomainConfiguration(int nIndex, LtDomainConfiguration** ppDc) = 0;
	virtual LtErrorType getConfigurationData(byte* pData, int offset, int length) = 0;
	virtual LtErrorType updateConfigurationData(byte* pData, int offset, int length) = 0;
	virtual LtErrorType getDomainConfiguration(int nIndex, LtDomainConfiguration* pDc) = 0;
	virtual LtErrorType updateSubnetNode(int domainIndex, int subnetNodeIndex, const LtSubnetNode &subnetNode) = 0;
    virtual LtErrorType updateDomainConfiguration(int nIndex, LtDomainConfiguration* pDomain, boolean bStore = false, boolean bRestore = false) = 0;
    virtual LtErrorType getAddressConfiguration(int nIndex, LtAddressConfiguration* pAc) = 0;
    virtual LtErrorType updateAddressConfiguration(int nIndex, LtAddressConfiguration* pAddress, boolean bRestore = false) = 0;
	virtual LtErrorType changeState(int newState, boolean bClearKeys) = 0;
    virtual LtErrorType getReadOnlyData(byte* readOnlyData) = 0;
	virtual LtErrorType initProgram(LtProgramId &pid) = 0;
	virtual void sendServicePinMessage() = 0;
	virtual void setReceiveAllBroadcasts(boolean bValue) = 0;
	virtual boolean getReceiveAllBroadcasts() = 0;

    virtual LtErrorType sendToXdriver(byte xDriverCommand, void *pData, int len) { return LT_NOT_IMPLEMENTED; }


    virtual boolean getTransmitTxStats(int &nMax, int &nFree, int &nPendingFree, 
                                       int &nMaxAllocated, int &nInstantiated, 
                                       int &searchRatio, int &txAllocationFailures) { return false; }

    virtual boolean getReceiveTxStats(int &nMax, int &nFree, int &nPendingFree, 
                                      int &nMaxAllocated, int &nInstantiated, 
                                      int &searchRatio, int &txAllocationFailures)  { return false; }

    virtual void clearTransmitTxStats(void) {}
    virtual void clearReceiveTxStats(void)  {}

    virtual LtErrorType disableOma() { return LT_NOT_IMPLEMENTED; }
    virtual boolean omaSupported() = 0;

	virtual LtErrorType waitForPendingInterfaceUpdates(void) { return LT_NO_ERROR; }
        // Used to reduce reported capabilities for node simulation.
    virtual LtErrorType setNmVersionOverride(int nmVersion, int nmCapabilities) { return LT_NOT_IMPLEMENTED; }

};

#endif
