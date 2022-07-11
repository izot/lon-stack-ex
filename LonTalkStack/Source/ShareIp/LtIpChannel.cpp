/***************************************************************
 *  Filename: LtIpChannel.cpp
 *
 * Copyright © 2022 Dialog Semiconductor
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in 
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *  Description:  the LTIP channel class.
 *
 ****************************************************************/

#ifdef WIN32
#include <windows.h>
#endif

#include <vxWorks.h>

#include <VxlTypes.h>
#include <vxlTarget.h>

#include <LtRouter.h>
#include <LtDriver.h>
#include <LtIpPackets.h>
#include <LtIpPersist.h>
#include <LtIpMaster.h>
#include <LtLreIpClient.h>
#include <IpLink.h>
#include <LtInit.h>
#include <LtMisc.h>
#include <LtPlatform.h>
#include <LtIpChannel.h>

#include <VxSockets.h>
#include <vxlTarget.h>

#include "routerControl.h"

void LtIpLogicalChannel::init(int index, int ipAddress, int ipPort, LtLonTalkChannel* pLonTalk, LtLreServer* pLreServer, int ipMcastAddress)
{
	boolean		bOk = false;

	m_ipAddress = ipAddress;
    m_localIpAddress = ipAddress;
	m_ipPort = ipPort;
    m_ipMcastAddress = ipMcastAddress;

	setXcvrId(154);	// IP-852, was 25 for IP-10L
	setDisplayName("IP");

#ifdef WIN32
	// Validate that we have the necessary DLLs for this channel.
	// These DLLs are assumed to be declared using the delay load
	// feature of the linker.
	if (LoadLibrary("wsock32.dll") == NULL)
	{
		setStartError( LT_NO_WINSOCK_DLL );
	}
	else
#endif
	{
		// Each LTIP channel needs a master object.  The master object
		// coordinates with the Config Server.
		m_pMaster = new LtIpMaster(getIpAddress(), getIpPort());
		m_pMaster->setIndex(index);
		m_pMaster->registerLink( *new CIpLink() );
		m_pMaster->setChannel(this);

        // Set the MULTICAST address 
        m_pMaster->setSelfInstalledMcastAddr(getIpMcastAddress());

		bOk = m_pMaster->start();
		if ( !bOk )
		{
			// Master may have set an error code
			if ( getStartError() == LT_NO_ERROR )
			{
				setStartError( LT_INVALID_PARAMETER );
			}

			// cleanup the master
			// (probably don't want to call stop() as start() was unsuccessful)
			delete m_pMaster;
			m_pMaster = null;
		}
		else
		{
            m_localIpAddress = m_pMaster->getLocalIpAddress();
			allStacksCreated();	// Even tho no stacks created, this is the only time
								// we can do this.
								// It would be nicer to synch with creation of windows
								// applications to avoid too much traffic with config
								// server at start up.
			registerEventClient( m_pMaster );
		}
	}
}

LtIpLogicalChannel::LtIpLogicalChannel(LtLreServer* pLreServer) :
    LtLogicalChannel(true, null, pLreServer)
{
	setXcvrId(154);	// IP-852, was 25 for IP-10L
	setDisplayName("IP");
	m_ipAddress = 0;
    m_localIpAddress = 0;
	m_ipPort = 0;
    m_ipMcastAddress = 0;
	m_pMaster = null;
}

LtIpLogicalChannel::LtIpLogicalChannel(int ipAddress, int ipPort, LtLonTalkChannel* pLonTalk, LtLreServer* pLreServer, int ipMcastAddress) :
	LtLogicalChannel(true, pLonTalk, pLreServer)
{
	init(0, ipAddress, ipPort, pLonTalk, pLreServer, ipMcastAddress);
}

LtIpLogicalChannel::LtIpLogicalChannel(int index, int ipAddress, int ipPort, LtLonTalkChannel* pLonTalk, LtLreServer* pLreServer, int ipMcastAddress) :
	LtLogicalChannel(true, pLonTalk, pLreServer)
{
	init(index, ipAddress, ipPort, pLonTalk, pLreServer, ipMcastAddress);
}

LtIpLogicalChannel::~LtIpLogicalChannel()
{
	if (m_pMaster != null)
	{
		m_pMaster->stop();
		delete m_pMaster;
	}
}

// Set pointer to native LON link in IP master (router only)
void LtIpLogicalChannel::setMasterLonLink( LtLink* pLink )
{
	if (m_pMaster != null)
	{
		m_pMaster->setLonLink( pLink );
	}
}


// set the secret and return true if master is available or false
// if no master is available to set secret
boolean	LtIpLogicalChannel::setAuthenticSecret( byte* pSecret )
{
	boolean		bOk = false;
	if (m_pMaster != null && m_pMaster->getPersitenceRead())
	{
		m_pMaster->setAuthenticSecret( pSecret );
		bOk = true;
	}
	return bOk;
}

byte* LtIpLogicalChannel::getAuthenticSecret()
{
	if (m_pMaster != null && m_pMaster->getPersitenceRead())
	{
		return m_pMaster->getAuthenticSecret();
	}
    return null;
}

boolean LtIpLogicalChannel::setAuthentication(boolean bEnable)
{
	boolean		bOk = false;
	if (m_pMaster != null && m_pMaster->getPersitenceRead())
	{
		m_pMaster->setAuthentication( bEnable, true );
		bOk = true;
	}
	return bOk;
}
boolean LtIpLogicalChannel::getIsAuthenticating(boolean &bEnabled)
{
	boolean		bOk = false;
	if (m_pMaster != null && m_pMaster->getPersitenceRead())
	{
		bEnabled = m_pMaster->isAuthenticating();
		bOk = true;
	}
	return bOk;
}



boolean LtIpLogicalChannel::startConfigServerCheck( ULONG csIpAddr, word csIpPort )
{
    boolean started = FALSE;
    if (m_pMaster != NULL)
    {
        started = m_pMaster->startConfigServerCheck(csIpAddr, csIpPort);
    }
    return(started);
}

boolean LtIpLogicalChannel::getConfigServerCheckComplete(ULONG &csIpAddr, word &csIpPort)
{
    boolean passed = FALSE;
	csIpAddr = 0;
	csIpPort = 0;
    if (m_pMaster != NULL)
    {
        passed = m_pMaster->getConfigServerCheckComplete(csIpAddr, csIpPort);
    }
    return(passed);
}

CsCommTestSts LtIpLogicalChannel::startConfigServerCheckEx(ULONG csIpAddr, word csIpPort)
{
    CsCommTestSts sts = CsCommTest::Failed;
    if (m_pMaster != NULL)
    {
        sts = m_pMaster->startConfigServerCheckEx(csIpAddr, csIpPort);
    }
    return sts;
}

CsCommTestSts LtIpLogicalChannel::getConfigServerCheckCompleteEx(ULONG &csIpAddr, word &csIpPort)
{
    CsCommTestSts sts = CsCommTest::Failed;
    if (m_pMaster != NULL)
    {
        sts = m_pMaster->getConfigServerCheckCompleteEx(csIpAddr, csIpPort);
    }
    return sts;
}



LtErrorType LtIpLogicalChannel::waitForPendingInterfaceUpdates(void)
{
	LtErrorType err = LT_NO_ERROR;
    if (m_pMaster != NULL)
    {
        err = m_pMaster->waitForPendingInterfaceUpdates();
    }
	return err;
}


boolean LtIpLogicalChannel::dumpLtIpXmlConfig()
{
    boolean succeeded = FALSE;
    if (m_pMaster != NULL)
    {
        succeeded = m_pMaster->dumpXmlConfig();
    }
    return(succeeded);
}
