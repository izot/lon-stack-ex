//
// LtPersistenceServer.h
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

#ifndef LtPersistenceServer_h
#define LtPersistenceServer_h

// The following are possible reasons for a node losing its persistent data
typedef enum
{
	LT_CORRUPTION				= 0x00,		// Image checksum invalid.
	LT_PROGRAM_ID_CHANGE		= 0x01,		// Program ID changed
	LT_SIGNATURE_MISMATCH		= 0x02,		// Image signature mismtach.  Could be corruption
											// or change to the persistent data format.
	LT_PROGRAM_ATTRIBUTE_CHANGE = 0x03,		// Number of NVs, aliases, address or domain
											// entries changed.
	LT_PERSISTENT_WRITE_FAILURE = 0x04,		// Could not write the persistence file.
	LT_NO_PERSISTENCE			= 0x05,		// No persistence found.
	LT_RESET_DURING_UPDATE		= 0x06,		// Reset or power cycle occurred while
											// configuration changes were in progress.
	LT_VERSION_NOT_SUPPORTED	= 0x07,		// Version number not supported

	LT_PERSISTENCE_OK			= -1
} LtPersistenceLossReason;

class LtPersistenceClient
{
public:
	virtual ~LtPersistenceClient() {}

	virtual void serialize(unsigned char* &pBuffer, int &len) = 0;
	virtual LtPersistenceLossReason deserialize(unsigned char* pBuffer, int len, int nVersion) = 0;
	virtual void notifyPersistenceLost(LtPersistenceLossReason reason) {}
};

#if PERSISTENCE_TYPE_IS(FTXL)
class LtPersistenceMonitor
{
public:
	virtual ~LtPersistenceMonitor() {}

    virtual void notifyNvdScheduled(int timeTillUpdate) = 0;
    virtual void notifyNvdComplete(void) = 0;
};
#endif

class LtPersistenceServer
{
public:
	virtual ~LtPersistenceServer() {}

	virtual boolean schedule() = 0;			// Return true if scheduled OK
											// Allows for returning of false if update is 
											// expected to fail (e.g., due to lack of disk
											// space).
	virtual void commit() = 0;
	virtual void start() = 0;
	virtual void sync() = 0;
	virtual LtPersistenceLossReason restore() = 0;
	virtual void registerPersistenceClient(LtPersistenceClient* pClient) = 0;
	virtual void setHoldTime(int nTime) = 0;
#if PERSISTENCE_TYPE_IS(STANDARD)
	virtual void setPath(const char* szPath) = 0;	// Path of file, eg, "c:\lonworks\vni\images"
	virtual void setName(const char* szName) = 0;	// Name of image file, eg, "img"
	virtual void setIndex(int nIndex) = 0;	// Index of node (0..N-1) in box
											// Result image example is "c:\lonworks\vni\images\img0"
#endif
};

#endif
