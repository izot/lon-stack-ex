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

#ifndef FTXLSTACK_H
#define FTXLSTACK_H

#include "LtStackInternal.h"
#include "LtStart.h"
#include "FtxlApiInternal.h"

#define NVD_SEG_VER_APPL_DATA      0 

#define NUM_FTXL_XCVR_TYPES (FtxlTransceiverType40MHz+1)
class FtxlAppPersitencClient : LtPersistenceClient
{
public:
	FtxlAppPersitencClient(LonNvdSegmentType type, int version);
    virtual ~FtxlAppPersitencClient() {};
	void serialize(unsigned char* &pBuffer, int &len);
	LtPersistenceLossReason deserialize(unsigned char* pBuffer, int len, int nVersion);

	LonApiError segmentHasBeenUpdated();
	LonApiError restore(boolean &persistenceRestored);

#if PERSISTENCE_TYPE_IS(FTXL)
    void setAppSignature(unsigned appSignature) { m_persistence.setAppSignature(appSignature); }
#endif

    void setPeristenceGaurdBand(int flushGuardTimeout) { m_persistence.setHoldTime(flushGuardTimeout); }
    void sync() { m_persistence.sync(); }

    void setNvdFsPath(const char *pNvdFsPath) { m_persistence.setNvdFsPath(pNvdFsPath);  }
    const char* getNvdFsPath() { return m_persistence.getNvdFsPath();  }
private:
	LtPersistence	  m_persistence;
	LonNvdSegmentType m_segType;
};

#if PERSISTENCE_TYPE_IS(FTXL)
class  FtxlStack : public LtAppNodeStack, public LtApplication, public LtPersistenceMonitor
#else
class  FtxlStack : public LtAppNodeStack, public LtApplication
#endif
{
public:
    FtxlStack(LtLogicalChannel* pChannel, 
              const LonControlData * const pControlData);
    virtual ~FtxlStack();

    LonApiError createStack(const LonStackInterfaceData* const pInterface,
                            const LonControlData       * const pControlData,
                            const char* pNvdFsPath);

    LonApiError startStack(void);

    void stopApp();

    void eventPump(void);

	LonApiError applSegmentHasBeenUpdated(void);
    LonApiError flushNvd(void);
    int nvdGetMaxSize(LonNvdSegmentType segmentType);

    static LonApiError lt2LonError(LtErrorType ltSts);
    static LonApiError nd2LonError(NdErrorType ndSts);
    
    LonApiError GetUniqueId(LonUniqueId* const pId);

    void GetRecieveAddress(LonReceiveAddress &receiveAddress, LtIncomingAddress &addr);

    boolean isConfiguredAndOnline() 
    {
        return getNetworkImage()->getState() == LT_CONFIGURED && !getOffline();
    }

    void setNvdFsPath(const char *pNvdFsPath);
    const char* getNvdFsPath();

    //-----------------------------------------------------------------------
    //                       Configuration access functions                           
    //-----------------------------------------------------------------------
    const LonApiError queryNvType(LtNetworkVariable *pNv, int arrayIndex, 
                                  LonNvDefinition* const pNvDef);

    static void freeNvType(LonNvDefinition* const pNvDef);

    const LonApiError queryNvConfig(const unsigned index,
                                    LonNvEcsConfig* const pNvConfig);

    const LonApiError updateNvConfig(const unsigned index,
									 const LonNvEcsConfig* const pNvConfig);


    const LonApiError queryAliasConfig(const unsigned index,
                                       LonAliasEcsConfig* const pAlias);

    const LonApiError updateAliasConfig(const unsigned index,
										const LonAliasEcsConfig* const pAlias);

    const LonApiError queryAddressConfig(const unsigned index,
                                         LonAddress* const pAddress);

    const LonApiError updateAddressConfig(const unsigned index,
										  const LonAddress* const pAddress);

    const LonApiError updateConfigData(const LonConfigData* const pConfig);


    //=======================================================================
    //                        LtApplication callbacks                        
    //=======================================================================

    //-----------------------------------------------------------------------
    //                            Stack callbacks                           
    //-----------------------------------------------------------------------
    /**
     * This method is invoked when the LonTalk Stack resets.  A reset could result
     * from a network management command or via the reset pin of the LON-C block.
     */
    void reset(); 

    /**
     * This method is invoked when a wink request is received over the network.
     */
    void wink(); 

    /**
     * This method is invoked when an offline request is received over the network.
     */
    void offline();  

    /**
     * This method is invoked when an online request is received over the network.
     */
    void online();

	/**
	 * This method is invoked when an initialize command is received (indicating 
	 * the start of a full commission.
	 */
    void initializing() {}

    //-----------------------------------------------------------------------
    //                       Network Variable callbacks                           
    //-----------------------------------------------------------------------

    /**
     * This method is invoked when a network variable update arrives either as
     * an update message or as a response to a poll.
     * @param index
     *              The network variable index.  For network variable arrays, this
     *              is the NV base index plus the array element index.
     * @param address
     *              The source address of the network variable update.
     */
    void nvUpdateOccurs(LtNetworkVariable* pNv, int arrayIndex,
		LtNetworkVariable* pSourceNv, int sourceArrayIndex,
        LtIncomingAddress* address);

    /**
     * This method is invoked when a network variable update completes.  For unackd
     * or unackd repeat updates, successful completion means the message(s) was queued
     * to be sent.  For acknowledged updates, successful completion means all
     * acknowledgments were received.  For polls, successful completion means all
     * responses were received and at least one had valid data (i.e., target was
     * online, had matching selector and passed authentication).
     * @param index
     *              The network variable index.  For network variable arrays, this
     *              is the NV base index plus the array element index.
     * @param success
     *              true if the transaction completed successfully.
     */
    void nvUpdateCompletes(LtNetworkVariable* pNv, int arrayIndex, boolean success);

	/**
	 * This method is invoked when an NV is added.  This occurs when dynamic NVs are
	 * added.  The application may choose to create a new version of the NV object
	 * which will be used in place of the one provided.  The stack will delete the
	 * old version in this case.  Return NULL or the old version to indicate no
	 * replacement.  Replacement might be used to derive from LtNetworkVariable.
	 */

    LtNetworkVariable* nvAdded(LtNetworkVariable* pNv);

    /**
	 * This method is called when an NV attribute changes.  It is also called during
	 * initialization to reflect any stored changes from the initial values.
	 */
	void nvChanged(LtNetworkVariable* pNv, NvChangeType type);

	/**
	 * This method is called when a dynamic NV is deleted.  It is the application's
	 * responsibility to delete the object.
	 */    
	void nvDeleted(LtNetworkVariable* pNv); 

	/**
	 * This method is called to find the current length of a network variable
     * with changeable length from the application. If the application does not
     * support this functionality, the callback should return false, and the
     * stack will make a best guess at the length based on the last time
     * the NV was updated, either from the network or from the application.
	 */
	boolean getCurrentNvLength(LtNetworkVariable* pNv, int &length);

    //-----------------------------------------------------------------------
    //                       Explicit Messaging callbacks                           
    //-----------------------------------------------------------------------

    /**
     * This method is invoked when an application message is received.  Such a message
     * will have a code in the range of 0x00 to 0x4F.
     * @param msg
     *              The incoming message.
     */
    void msgArrives(LtMsgIn* msg);

    /**
     * This method is invoked when a message completes.  For unackd
     * or unackd repeat updates, successful completion means the message(s) was queued
     * to be sent.  For acknowledged and request/response messages, successful
     * completion means all acknowledgments/responses were received.
     * @param tag
     *              The message tag provided in the original outgoing message.
     * @param success
     *              true if the transaction completed successfully.
     */
    void msgCompletes(LtMsgTag* tag, boolean success);

    /**
     * This method is invoked when a response arrives.
     * @param response
     *              The incoming response.
     */
    void respArrives(LtMsgTag* tag, LtRespIn* response);

    //-----------------------------------------------------------------------
    //                            Misc. callbacks                           
    //-----------------------------------------------------------------------

    /**
     * This method informs the application when the service pin has been depressed.  The
     * protocol stack automatically sends a service pin message as a result of service pin 
     * depression.  This callback allows the application to do additional actions if so
     * desired.
     */
    void servicePinPushed();

   /**
     * This method informs the application when the service pin has been released. 
     */
    virtual void servicePinHasBeenReleased();

    /**
     * This method informs the application when a flush request has completed.
     */
    void flushCompletes() {};

    /**
     * This method is invoked in non-direct callback mode (see Layer7Api.java)
     * to cause the application thread to run.  It is up to the application whether
     * this is effected by suspend/resume, wait/notify or sleep/interrupt.  Once 
     * running, it is up to the application to invoke processApplicationEvents() to handle
     * the event.
     */
    void applicationEventIsPending();
    
    /**
     * This method is used to read memory from a specified LonTalk address.
     * Only addresses registered with Layer7Api.registerMemory() are read.
     * return true if read is successful, false otherwise
     * @param address
     *          Address to read
     * @param data
     *          Byte array for data (read data.length bytes).
     */
    boolean readMemory(int address, byte* data, int length);
    
    /**
     * This method is used to write memory at a specified LonTalk address.
     * Only addresses registered with Layer7Api.registerMemory() are written.
     * return true if write is successful, false otherwise
     * @param address
     *          Address to write
     * @param data
     *          Byte array of data to write
     */
    boolean writeMemory(int address, byte* data, int length);

	/**
	 * This method is called when an application's persistent data is lost.
	 * @param reason
	 *			Reason for data loss.
	 */
    void persistenceLost(LtPersistenceLossReason reason) {};

    /**
	 * This method is called when the service led status is changed 
	 */
    void setServiceLedStatus(LtServicePinState state);

    byte* getSiData(int* pLength);  // returns byte array containing SI data
    int getSiDataLength() { return NodeDefMaker::getSiDataLength(); } 

    unsigned getCurrentNvSize(const unsigned index);
    unsigned getDefaultApplicationSegmentSize();
    void     defaultSerializeSegment(boolean toNvMemory, void* const pData, const size_t size);

    void notifyNvdScheduled(int timeTillUpdate);
    void notifyNvdComplete(void);

    /* Number of seconds for NVD to be stored before assuming starvation. */
    static const unsigned starvationTimeout = 60;

private:
    static int servicePinHeldTimeout(int pFtxlStack);
    static int nvdStarvationTimeout(int pFtxlStack);

    LonApiError storNetworkImage(void);

    WDOG_ID m_servicePinHoldTimer;
    int     m_servicePinHoldTime;
    boolean m_bServicePinHeld;

    WDOG_ID m_starvationTimer;
    int     m_starvationTimeout;        // Number of ticks for starvation timer
    ULONG   m_expectedNvdStart;         // Tickcount of expected time of NVD write
    boolean m_bNvdStarvedOut;
    int     m_avgDynNvSdLength;

    static const byte m_commTabTable[NUM_FTXL_XCVR_TYPES][FTXL_NUM_COMM_BYTES];

    boolean             m_isOpen;

	FtxlAppPersitencClient m_nvdSegApplicationData;

    LtServicePinState	m_nServiceLedState; 
    LtServicePinState   m_nPrevServiceLedState;

    byte *m_siData;     // SI data array
};
#endif
