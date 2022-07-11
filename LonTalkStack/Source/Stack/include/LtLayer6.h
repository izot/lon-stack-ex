#ifndef _LTLAYER6_H
#define _LTLAYER6_H

//
// LtLayer6.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtLayer6.h#1 $
//


class LtLayer6 
{

private:
    LtDeviceStack* m_pStack;

protected:
	boolean			m_bReceiveAllBroadcasts;
	boolean			m_bDciRouting;

public:
	LtLayer6() 
	{
		m_pStack = NULL;
		m_bReceiveAllBroadcasts = false;
		m_bDciRouting = true;
	}
    LtDeviceStack* getStack() { return m_pStack; }
    void setStack(LtDeviceStack* pStack) { m_pStack = pStack; }

    void nvFailure(LtApduIn* pApdu);
    LtErrorType incomingNetworkVariable(LtApduIn* pApdu, boolean& valid);
	LtErrorType privateNvFetchResponse(LtApduIn* pApdu, boolean& valid);
    LtErrorType receive(LtApduIn* pApdu, LtApduOut* pApduOut);
    void completionEvent(LtApduOut* pApdu, boolean success);
	void send(LtApduOut* pApdu, boolean wait = true, boolean throttle = true);
	boolean sourceAddressMatch(LtNetworkVariableConfiguration* pNvc, LtApduIn* pApdu);
	// These two routines are also in LtDeviceStack, but they are independent, not virtual
	void setReceiveAllBroadcasts(boolean bValue) { m_bReceiveAllBroadcasts = bValue; } 
	boolean getReceiveAllBroadcasts() { return m_bReceiveAllBroadcasts; }
	// These routines control whether a L6 client has its traffic routed through the DCI
	void setDciRouting(boolean bValue) { m_bDciRouting = bValue; }
	boolean getDciRouting() { return m_bDciRouting; }
};

#endif
