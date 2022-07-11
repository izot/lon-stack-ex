//
#ifndef LTINIT_H
#define LTINIT_H
// LtInit.h
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


// Init  flags
#define LT_INIT_START_ROUTER	0x0001
#define LT_INIT_FIND_LRE		0x0002
#define LT_INIT_START_ENGINE	0x0004

class LtRouterApp;
class LtLreServer;
class LtIpLogicalChannel;

class LTA_EXTERNAL_CLASS LtInit
{
protected:
	LtLtLogicalChannel* m_pLtChannel;
	LtIpLogicalChannel* m_pIpChannel;
	LtRouterApp*		m_pRouterNear;
	LtRouterApp*		m_pRouterFar;
	LtLreServer*		m_pLreServer;
	boolean				m_bCreatedRouter;

public:
	LtInit();
	virtual ~LtInit();

	virtual LtLreServer* initAll(int &appIndex,
						 LtLogicalChannel** ppLtChannel,
						 int nMulticastTargets,
						 int nInitFlags = 0,
						 boolean bLayer2 = false,
    					 const char* pLtPortName = null);

	// create a router for testing
	virtual LtLreServer* initTest( int& index, 
						LtLink* pLtLink,
						int nMulticastTargets );
	virtual LtLink*		getLonLink()			{ return m_pLtChannel ? m_pLtChannel->getLonLink() : null; }
	virtual LtLtLogicalChannel* getLtChannel()	{ return m_pLtChannel; }
	virtual LtIpLogicalChannel* getIpChannel()	{ return m_pIpChannel; }
	virtual void setLtChannel(LtLtLogicalChannel* pChannel) { m_pLtChannel = pChannel; }
	virtual void setIpChannel(LtIpLogicalChannel* pChannel) { m_pIpChannel = pChannel; }
	virtual LtLreServer* getLreServer();
	virtual LtRouterApp* getRouterFar()			{ return m_pRouterFar; }
	// enumerate clients on a specific channel
	virtual boolean		enumChannelClients( LtLogicalChannel* pChan,
									int& idx,
									LtLreClient** ppClient );

protected:
	// common code to complete initialization
	virtual void	createRouter(int& index);
};

#endif
