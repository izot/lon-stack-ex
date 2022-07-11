//
// LtPersistence.h
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
// This class supports persistence with the following properties:
// 
// User creates instance (or derives from) LtPersistence class.  User
// must define (or derive from) a LtPersistenceClient class.
// 
// The persistence class keeps a signature in the persistence file as well
// as a version number.  It also checksums the data.  It returns an error if
// the signature, version or checksum is invalid.  
//
// Version numbers less than the current version are allowed.  It is up to
// the user to parse the data differently.  Version numbers greater than the current
// are not allowed.
//
// Persistence class supports two modes of operation, explicit and implicit
// commit. In explicit commit mode, you explicitly start a transaction and then explicitly
// commit it.  In implicit commit mode, changes are committed after a hold down
// timer expires.
//
// Another control parameter is whether the client is notified of a failure to
// commit pending changes.  This is not done by default since it consumes extra 
// resources.  This is done via setCommitFailureNotify().
//
// Examples:
//  User has "persistence" object.  Wants to schedule an update with implicit commit.
//		LtPersistence persistence(CURRENT_VERSION);
//		persistence.setName("MyPersistence");
//
//		...
//
//		persistence.schedule()
//		<commit occurs upon hold timer expiration>
//
//  User wants to do explicit commit
//		persistence.start()
//		<make changes>
//		persistence.commit()
//
//  Note that start() is not synchronous.  It may return before the transaction start
//  has been recorded persistently (if commit failure notify is on).  The completion
//  of the "start" can be checked with isStartComplete().  Same goes for commit.  It
//  can be checked with isCommitComplete().  
//
//  The "sync()" function synchronously waits for an explicit or implicit commit to 
//  complete.  For example, you might want to call "sync()" on shutdown.
//
//  After reboot, call
//		reason = persistence.read(pImage, len)
//		if (reason == LT_RESET_DURING_UPDATE)
//		{
//			// This can only occur if you did a setCommitFailureNotify(true)
//			// It is up to the caller whether to decide to ignore the data in this
//			// case as it will be returned as long as it is valid.
//		}
// 
// The user must specify the following:
// 1. Image name (".dat" extension is automatically added).
// The user may specify the following:
// 1. Image path (PC default is c:\pentagon\ltConfig; PNC is /root/ltConfig)
// 2. Instance index. (optional, default 0)
// 3. Hold down time (default 1000 msec)
// 4. Commit failure notify mode
//

#ifndef LtPersistence_h
#define LtPersistence_h

#include <LtPersistenceServer.h>

#if PERSISTENCE_TYPE_IS(FTXL)
#include "FtxlApiInternal.h"
#endif  // PERSISTENCE_TYPE_IS(FTXL)

#define nothing
#define LT_SERIALIZABLE_COMMON(fntype)					\
    fntype int getSerialLength();					\
    fntype void serialize(unsigned char* &pBuffer);	\
    fntype void deserialize(unsigned char* &pBuffer, int nVersion);

#define LT_SERIALIZABLE			LT_SERIALIZABLE_COMMON(nothing)
#define LT_SERIALIZABLE_VIRTUAL LT_SERIALIZABLE_COMMON(virtual)

#define LT_IMAGE_SIGNATURE0  0xCE82
#define LT_IMAGE_SIGNATURE1	 0xCE83

#define MAX_IMAGE_NAME_LEN	40

#define IMAGE_PENDING_KEY	"_PENDING"

class LtPersistenceHeader
{
public:
	LtPersistenceHeader(unsigned short nVersion)
	{
		length = 0;
		signature = LT_IMAGE_SIGNATURE1;
		version = nVersion;
		checksum = 0;
#if PERSISTENCE_TYPE_IS(FTXL)
        appSignature = 0;
#endif
	}
	unsigned int length;
	unsigned short signature;
#if PERSISTENCE_TYPE_IS(FTXL)
    unsigned long  appSignature;
#endif
	unsigned short version;			// Major version - mismatch means forget it.
	unsigned short checksum;		// Checksum of length and data

	boolean valid();
	unsigned int getLength();
};

class LtPersistence : public LtPersistenceServer
{
private:
	LtPersistenceClient*	m_pClient;
	boolean					m_bPendingStore;
#if PERSISTENCE_TYPE_IS(FTXL)
    static LtPersistenceMonitor* m_pPersistenceMonitor;    
    static boolean          m_bShutdown;
	static int				m_tid;
	static SEM_ID			m_semPending;
    static SEM_ID           m_taskMutex;
    static boolean          m_bResetFlag;

    static LtPersistence    *m_pPersitenceList[LonNvdSegNumSegmentTypes];

    ULONG                   m_lastUpdate;
    LonNvdSegmentType       m_type;
    ULONG                   m_guardBandDuration;    // In ticks
    boolean                 m_bStoreInProgress;
    unsigned                m_appSignature;                

    void                    setUpdateTime();
    ULONG                   guardBandRemainig();
    ULONG                   store();
#else
	int						m_tid;
	SEM_ID					m_semPending;
	int						m_nHoldTime;
	int						m_nIndex;
	char					m_szImageName[MAX_IMAGE_NAME_LEN];
	char					m_szImagePath[MAX_PATH];
	char					m_szImage[MAX_PATH];
	char					m_szPending[MAX_PATH];

	void					setImageName();
	void					storeWait();
#endif
	SEM_ID					m_semStore;
	boolean					m_bSync;
	boolean					m_bLocked;
	boolean					m_bCommitFailureNotifyMode;
	boolean					m_bChecksum;
    boolean                 m_readyForBackup;
    
    static const boolean    m_pendingPathInKey;

	int						m_nChecksum;
	int						m_nCurrentVersion;

    char                    m_szNvdFsPath[MAX_PATH];

	boolean					validateChecksum(LtPersistenceHeader* pHdr, byte* pImage);
	int						computeChecksum(byte* pImage, int length);
	LtPersistenceLossReason read(byte* &pImage, int &imageLen, int &nVersion);
	void					write(byte* pImage, LtPersistenceHeader* pHdr);
	static int VXLCDECL		storeTask( int obj, ... );
	void					saveConfig();

public:
	LtPersistence(unsigned short nCurrentVersion);
	virtual ~LtPersistence();
	boolean schedule();  
	void commit();
	LtPersistenceLossReason restore();
	void start();
	void sync();
	boolean isCommitComplete();
	boolean isStartComplete();
	void registerPersistenceClient(LtPersistenceClient* pClient);
	void setCommitFailureNotifyMode(boolean bMode);
	void setLocked(boolean bLocked);// For EEPROM lock feature.
	void setHoldTime(int nTime);	// Time to wait before commit.
	void setTransactionMode(boolean bMode);
	void resetPersistence();
#if PERSISTENCE_TYPE_IS(FTXL)
    void setType(LonNvdSegmentType type);
    void setAppSignature(unsigned appSignature) { m_appSignature = appSignature; }
    unsigned getAppSignature() { return m_appSignature; }
    static void registerPersistenceMonitor(LtPersistenceMonitor *pMonitor) { m_pPersistenceMonitor = pMonitor; }
    void setNvdFsPath(const char *pNvdFsPath);
    const char* getNvdFsPath() { return m_szNvdFsPath; }
    void writeUniqueID(LtUniqueId &uid);
    LtPersistenceLossReason readUniqueID(LtUniqueId* pId);
    static boolean getResetFlag() {return m_bResetFlag; }
    static void setResetFlag(boolean bResetFlag) {m_bResetFlag = bResetFlag; }
#else
    void setName(const char* szName);		// Name of image file, eg, "img".  
	void setIndex(int nIndex);		// Index of node (0..N-1) in box
	void setPath(const char* szPath);		// Path of file, eg, "c:\lonworks\vni\images"
#endif									// Result image example is "c:\lonworks\vni\images\img0.dat"

	// Use the following to disable checksum calculation - doesn't suppress checking!!!
	void setSuppressChecksumCalculation(boolean bRecompute);

	void setPending(boolean bPending);
	boolean getPending();

	static const char* getPersistenceLostReason(int reason);

    void prepareForBackup(void);
    void backupComplete(void);
    boolean readyForBackup(void) { return m_readyForBackup; }
};
#endif
