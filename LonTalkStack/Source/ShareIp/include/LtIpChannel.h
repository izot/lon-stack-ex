#ifndef LTIPCHANNEL_H
#define LTIPCHANNEL_H

/***************************************************************
 *  Filename: LtIpChannel.h
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
 *  Description:  interface for the LTIP channel class.
 *
 ****************************************************************/


// Declare these classes to decouple this file from other header files.
class LtIpMaster;
class LtInit;
class LtLonTalkChannel;
class LtLreServer;
class LtLink;

// Return codes used by the IP-852 config server communications test.
// Moved here from LtIpMaster.h to keep that header from being exported for LNS.
// Declare a namespace to give the enum values scope
namespace CsCommTest
{
typedef enum 
{
	Incomplete,		// Test not complete
	OkActive,		// CS is responding, device is in channel
	OkNotActive,	// CS is responding, but device is not in channel
	DnsFailed,		// DNS failed to get host address, no internal address available
	NoAddr,			// No address configured
	Failed,			// CS not responding
} eCsCommTestSts;
} // namespace
typedef CsCommTest::eCsCommTestSts CsCommTestSts;

class LTA_EXTERNAL_CLASS LtIpLogicalChannel : public LtLogicalChannel
{
private:
	int			m_ipAddress;
	int			m_ipPort;
    int         m_ipMcastAddress;
    int			m_localIpAddress;
	LtIpMaster* m_pMaster;
	LtInit*		m_pInit;

	void init(int index, int ipAddress, int ipPort, LtLonTalkChannel* pLonTalk=null, LtLreServer* pLreServer=null,
            int ipMcastAddress=0);

public:
	// This form does not create the master.  This is used when creating a master explicitly
	// (old style init).
	LtIpLogicalChannel(LtLreServer* pLreServer=null);
	// This constructor creates a master automatically.
	LtIpLogicalChannel(int ipAddress, int ipPort, LtLonTalkChannel* pLonTalk=null, LtLreServer* pLreServer=null, int ipMcastAddress=0);
	// This is the same but it allows for an "index" so that you can have multiple masters.
	// Masters should be numbered 0..n-1.
	LtIpLogicalChannel(int index, int ipAddress, int ipPort, LtLonTalkChannel* pLonTalk=null, LtLreServer* pLreServer=null, int ipMcastAddress=0);
	~LtIpLogicalChannel();

	int getIpAddress() { return m_ipAddress; }
	int getIpPort() { return m_ipPort; }
    int getIpMcastAddress() { return m_ipMcastAddress; }
    int getLocalIpAddress() { return m_localIpAddress; }

	// Set pointer to native LON link in IP master (router only)
	void setMasterLonLink( LtLink* pLink );

	// set the secret and return true if master is available or false
	// if no master is available to set secret
	boolean	setAuthenticSecret( byte* pSecret );
    byte*   getAuthenticSecret();

    boolean setAuthentication(boolean bEnable);
	boolean getIsAuthenticating(boolean &bEnabled);


    // Start a check to see if we are in communication with the config server.
    // Optionally set the config server IP address and port (use current
    // if csIpAddr address is 0).  Use getConfigServerCheckPassed() to see
    // if the check has passed.
    boolean startConfigServerCheck( ULONG csIpAddr, word csIpPort );
    boolean getConfigServerCheckComplete(ULONG &csIpAddr, word &csIpPort);
	CsCommTestSts startConfigServerCheckEx(ULONG csIpAddr, word csIpPort);
	CsCommTestSts getConfigServerCheckCompleteEx(ULONG &csIpAddr, word &csIpPort);

	LtErrorType waitForPendingInterfaceUpdates(void);

    // Dump the LTIP XML configuration ;
    boolean dumpLtIpXmlConfig();


};

#endif
