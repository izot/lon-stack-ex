#ifndef LTAPPLICATIONCONTROL_H
#define LTAPPLICATIONCONTROL_H
//
// LtApplicationControl.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtApplicationControl.h#1 $
//

class LtStack;
class LtApplication;

class LtApplicationControl : public LtAppControl
{
private:
	LtApplication* m_pApp;
	LtStack* m_pStack;

public:
	LtApplicationControl(int nIndex, LtLogicalChannel* pChannel) : LtAppControl(nIndex, pChannel) 
	{
		m_pApp = null;
		m_pStack = null;
	}
	~LtApplicationControl();
	void deactivate();
	void sendServicePinMsg();
	int getState();

	LtStack* getStack(int index) 
	{ 
		if (index == 0)	return m_pStack; 
		return null;
	}
	void setStack(LtStack* pStack) { m_pStack = pStack; }

	LtApplication* getApp() { return m_pApp; }
	void setApp(LtApplication* pApp) { m_pApp = pApp; }
};

#endif
