#ifndef	__PNCLTLINK_H
#define	__PNCLTLINK_H
/*
 * Pentagon LON-C Device Driver -- Lower Layer
 *
 * Copyright © 1998,1999 Toshiba Corporation, Fuchu Works
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
 * Release Date: JAN 6 1999
 * Modification Date: JAN 19 1999
 *  -- Moved constructor to private; get it with getInstance()
 *     to avoid creating multiple instances
 */
#include "LtDriver.h"	/* Interfaces */

#include <semLib.h>
#include <taskLib.h>
#include "lonc.h"	/* Desc */
#include "defs.h"

/* Debug flags */
#define	DEBUG_BASIC	1
#define	DEBUG_SEND	2
#define	DEBUG_RECV	4
#define	DEBUG_EXREG	8
#define	DEBUG_INIT	16
#define	DEBUG_INTR	32
#define	DEBUG_RECV_OC	64

extern int	debugFlag;

class pncLtLink: public LtLink {

private:
/* Handles */
  static void interruptEntry(pncLtLink *me);
  static int taskEntry(pncLtLink *me);

#define	L2_RESET	1	/* Reset In Progress */
#define	L2_SCP		2	/* Setting Comm Params */
#define	L2_UP		4	/* LON-C is up */

#define	L2_CTORFAIL	0x2000	/* Constructor Fails -- UNUSABLE AT ALL */
#define	L2_INITFAIL	0x1000	/* CommPort Init Failure */
  int state;

/* Modes */
#define	MODE_LOOPBACK	1	/* Loopback Mode */
#define	MODE_ANALYZER	2	/* Loopback Mode */
#define	MODE_FLUSH	4	/* Flush */
#define	MODE_TERMINATE	8	/* Terminate */
  int mode;

/* Common data */
  byte rccmode;
  byte rcwbase;
  volatile byte irenb;
  volatile byte irreg;
  volatile byte pseudoIntr;
  int  txslot;
  volatile int  l2int_working;
  int  opened;

  int resetCount;

  int revision;	// of LON-C
#define	REVISION_0	0
#define	REVISION_3	3

/* LSI Bugs */
  int rcprlen_keep;

/* IDLE */
#define	IDLERUN_RCCMODE		1
#define	IDLERUN_RCWBASE		2
#define	IDLERUN_TERMINATE	4
#define	IDLERUN_FLUSH		8
#define	IDLERUN_COMMPORTINIT	16
  byte idle_change;

/* CLevel Link Stats */
  struct {
    int transmissionErrors;
    int missedPackets;
    int collisions;
    int backlogOverflows;
    int transmittedPackets;
    int receivedPackets;
    int receivedPriorityPackets;
    int backoffs;
  } stats;

/* External Registers */
#define	ERSTAT_FREE		0
#define	ERSTAT_WRITE		0x2000
#define	ERSTAT_READREQ(n)	(1<<(n))
  int extreg_stat;

/* Upper layer support */
  LtNetwork	*network;

/* Queue structures */
#define	LRCMD_DEAD	1
  typedef struct _rxq {
    Desc d[LTLINK_MAXQUEUEDEPTH];	/* Descriptor Array */
    void *r[LTLINK_MAXQUEUEDEPTH];	/* Reference ID Array */
    byte *h[LTLINK_MAXQUEUEDEPTH];	/* Data Address Array */
    int  t;	/* Top */
    int  b;	/* Bottom: [To queue] */
    int  s;	/* Queued packets */
#ifdef	TEST_KEEP_DESC
    Desc backup[DEBUG_MAX_DESC_KEEP];
    int  bt;
    int  bt_wrap;
#endif
  } rxq_t;
  int	rxq_size;
  int	rxq_size_new;
  rxq_t	rxq_p, rxq_n;

#define	LSCMD_DEAD	1
  typedef struct _txq {
    Desc d[LTLINK_MAXQUEUEDEPTH];	/* Descriptor Array */
    void *r[LTLINK_MAXQUEUEDEPTH];	/* Reference ID Array */
    int  t;	/* Top */
    int  b;	/* Bottom: [To queue] */
    int  s;	/* Queued packets */
#ifdef	TEST_KEEP_DESC
    Desc backup[DEBUG_MAX_DESC_KEEP];
    int  bt;
    int  bt_wrap;
#endif
  } txq_t;
  int	txq_size;
  int	txq_size_new;
  txq_t	txq_p, txq_n;

/* Communication Parameters */
  byte commParams[16];

/* Internal Function Declarations */

/* OS calls */
  int os_init();

  static SEM_ID os_createLock();
  static int os_lock(SEM_ID);
  static int os_unlock(SEM_ID);
  static int os_destroyLock(SEM_ID);

  static SEM_ID os_createCondVar();
  static int os_notify(SEM_ID);
  static int os_sleep(SEM_ID);
  static int os_sleep(SEM_ID, long);
  static int os_destroyCondVar(SEM_ID);

  int os_registerInterrupt();
  int os_unregisterInterrupt();
  int os_createTask(char *name, int (*func)(pncLtLink *me));
  int os_setTaskPriority(int prio);

/* Internal */
  void cleanup();
  void reset2();
  void freePointers();
  void idle();
  void idle_do();
  void idle_dmaEnd();
  void idle_restartDma();
  void idle_scp();

  void enable_interrupt(int lit);
  void disable_interrupt(int lit);

/* Rx Queue */
  void newRxQueue();
  void doRxQueue();
  void doRxQueueForEachQ(rxq_t *q, int prio);
  void disposeRxQueue(LtSts notify);
  void disposeRxQueueForEachQ(rxq_t *q, LtSts note, int prio);

/* Tx Queue */
  void newTxQueue();
  void doTxQueue();
  void doTxQueueForEachQ(txq_t *q, int prio);
  void disposeTxQueue(LtSts notify);
  void disposeTxQueueForEachQ(txq_t *q, LtSts note, int prio);

/* External Register Access */
  LtSts extreg_write();
  LtSts extreg_read(int reg);
  void extreg_done(void);
  void extreg_kick(void);
  void extreg_dispose(void);

  int  intrTask;
  void pseudoInterrupt(int cause);

/* Locks */
  SEM_ID lockRxFree;
  SEM_ID lockTxReq;

/* Condition Variables */
  SEM_ID cvIntr;
  SEM_ID cvExreg;
  SEM_ID cvSCP;

  byte uniqueId[6];
  boolean uniqueIdAvailable;

  pncLtLink();

#ifdef	DEBUG
public:
#endif
  /* Regs */
  Regs	*lonControllerP;

/* State Info */
#ifndef	DEBUG
public:
#endif
  ~pncLtLink();

  static pncLtLink *createInstance();
  virtual void destroyInstance() {}

/* LtLink */
  LtSts inline registerNetwork(LtNetwork &net) {
    network = &net;
    printf("L2: Network layer registered\n");
    return LTSTS_OK;
  }
  boolean enumInterfaces(int idx, char *name, int size);
  LtSts open(const char* *name);
  LtSts close();
  inline boolean isOpen();
  LtSts setReceivePriority(int priority);
  void getMaxQueueDepth(int *pnReceiveQueueDepth, int *pnTransmitQueueDepth);
  LtSts setQueueDepths(int nR, int nT);
  LtSts sendPacket(void *refId, int prioSlot, byte *data, int len, boolean prio);
  LtSts queueReceive(void *lpdu, boolean priority, byte *data, int maxLen);
  void reset(void);
  int  getStandardTransceiverId(void);
  boolean getUniqueId(LtUniqueId &uniqueId);
  void flush(boolean on);
  void terminate(void);
  LtSts setCommParams(const LtCommParams &commParams);
  LtSts getCommParams(LtCommParams &commParams);
  LtSts getTransceiverRegister(int n);
  void getStatistics(LtLinkStats &outstat);
  void clearStatistics();
  void setServicePinState(LtServicePinState spstate);
  void setProtocolAnalyzerMode(boolean on);
  boolean getProtocolAnalyzerMode();
  void setLoopbackMode(boolean on);
  boolean getLoopbackMode(void);
  LtSts  selfTest(void);
  LtSts  reportPowerSelfTest();
  void setCurrentBacklog(int backlog);
  int  getCurrentBacklog();
  void setWindowSize(int windowSize);
  int  getWindowSize();
  void setTransmitSlot(int transmitSlot);
  int  getTransmitSlot();

  void interruptHandler();
  int taskInterrupt();

#ifdef	TEST_STUB
/* Pseudo DMA control */
  void pseudoTx(int prio, int sts);
  void pseudoRx(int prio, int sts, int nbytes);
#endif

/* Info */
  char *getFacility();
/* Dumper */
  void debugDump(int flag);
  char *printState();
  char *printMode();
#define	DEBUGDUMP_RECVQ			1
#define	DEBUGDUMP_SENDQ			2
#define	DEBUGDUMP_VAR			4
#define	DEBUGDUMP_SEND_DESC_BACKUP	8
#define	DEBUGDUMP_RECV_DESC_BACKUP	16
#define	DEBUGDUMP_STATES		32

private:
  void inline flushCompleted() {
    if(network != NULL)
      network->flushCompleted();
    else
      printf("L3 error: flushCompleted invoked w/o registering LtNetwork\n");
  }
  void inline terminateCompleted() {
    if(network != NULL)
      network->terminateCompleted();
    else
      printf("L3 error: terminateCompleted invoked w/o registering LtNetwork\n");
  }
  void inline packetReceived(
    void *refId, int len, boolean prio, int rSlot, LtSts sts ) {
    if(network != NULL)
      network->packetReceived(refId, len, prio, rSlot, sts);
    else
      printf("L3 error: packetReceived invoked w/o registering LtNetwork\n");
  }
  void inline packetComplete(void *refId, LtSts sts) {
    if(network != NULL)
      network->packetComplete(refId, sts);
    else
      printf("L3 error: packetComplete invoked w/o registering LtNetwork\n");
  }
  void inline reportTransceiverRegister(int n, int val, LtSts status) {
    if(network != NULL)
      network->reportTransceiverRegister(n, val, status);
    else
      printf("L3 error: reportTransceiverRegister invoked w/o registering LtNetwork\n");
  }
  void inline servicePinDepressed() {
    if(network != NULL)
      network->servicePinDepressed();
    else
      printf("L3 error: servicePinDepressed invoked w/o registering LtNetwork\n");
  }
  void inline resetRequested() {
    if(network != NULL)
      network->resetRequested();
    else
      printf("L3 error: resetRequested invoked w/o registering LtNetwork\n");
  }
};

#endif	/* __PNCLTLINK_H */
