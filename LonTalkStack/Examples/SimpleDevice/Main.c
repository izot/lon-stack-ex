/*
 * Main.c
 * $Revision: #8 $
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
 * Description: This file contains a simple IzoT application example.
 *
 *  The example implements a single functional block described by the table below:
 * 
 *  +==========================================================================+ 
 *  |    |    Name       | Type                   |       Description          |
 *  |====|===============|========================|============================|
 *  | FB | voltActuator  | SFPTclosedLoopActuator |                            |
 *  |----|---------------|------------------------|----------------------------|
 *  | NV | nviVolt       | SNVT_volt              | Input implementing nviValue|
 *  |    |               |                        | of voltActuator            |
 *  |----|---------------|------------------------|----------------------------|
 *  | NV | nvoVoltFb     | SNVT_volt              | Output implementing        |
 *  |    |               |                        | nvoValueFb of voltActuator |
 *  +==========================================================================+ 
 * 
 *  The voltActuator functional block simulates a voltage actuator with a 
 *  built-in gain of 2. Whenever an update to nviVolt is received, the 
 *  feedback variable nvoVoltFb is set to 2*nviVolt, and is propagated on the 
 *  network.  
 *
 *  For a real actuator, the value of nviVolt would be used to set a voltage
 *  level.  After updating the voltage level, the application would read back 
 *  the actual resulting level, and use that value to set the feedback variable 
 *  nvoVoltFb.  
 * 
 */
 
#include "FtxlDev.h"
#ifdef ISI
#include "IsiApi.h"
#endif
#include "Osal.h"

#include <stdio.h>
#include <ctype.h>

#if defined WIN32
    #include <Windows.h>
    #if FEATURE_INCLUDED(IP852)       
        #include <WinSock.h>
    #endif
    #include <memory.h>
#else
#define _stricmp(s1,s2) strcasecmp(s1,s2)
#endif

/* Limits for nviVolt */
#define MIN_VOLT                (-32768/2)
#define MAX_VOLT                (32767/2)

/* Handle to eventReady event used to signal the appTask when an IzoT event has 
 * occurred. 
 */
OsalHandle eventReadyHandle;
LonBool appTaskRunning;
LonBool shutdownApp = FALSE; 
LonBool sendServicPin = FALSE; 

/******************************************************************************
 * Forward references 
 ******************************************************************************/

    /* Task implementing the application control loop. */
void appTask(int taskIndex);
    /* Process inputs when the device goes online. */
void ProcessOnlineEvent(void);
    /* Process a change to nviVolt. */
void ProcessNviVoltUpdate(void);


#if FEATURE_INCLUDED(IP852)
    static int myIpAddress;
    static int myPort;
    static char szMyDeviceURI[100];
    void GetMyIpAddress(int *pAddress, int *pPort)
    {
        *pAddress = myIpAddress;
        *pPort = myPort;
    }
    LonBool SetMyIpAddress(const char *szAddress, const char *szPort, const char *szProtType,
            const char *szInterface)
    {
        LonBool ok = FALSE;
        if (sscanf(szPort, "%d", &myPort) == 1)
        {
            myIpAddress = htonl(inet_addr(szAddress));
            ok = TRUE;
            if (!strcmp(szProtType, "") || !_stricmp(szProtType, "uc"))
                sprintf(szMyDeviceURI, "uc://%s:%s",szAddress,szPort);
            else
            if (!_stricmp(szProtType, "ldv"))
                // ldv does not need to specify a port numberr (ignored)
                sprintf(szMyDeviceURI, "%s://%s",szProtType,szAddress);
            else
                sprintf(szMyDeviceURI, "%s://%s,%s:%s",szProtType,szAddress,szInterface,szPort);
        }
        return ok;
    }
    const char *GetMyDeviceURI()
    {
        return szMyDeviceURI;
    }
#else
    static const char *myNetworkInterface = "LON1";
    const char *GetMyNetworkInterface()
    {
        return myNetworkInterface;
    }

    void SetMyNetworkInterface(const char *networkInterface)
    {
        myNetworkInterface = networkInterface;
    }
#endif

static const char *pMyNvdFolder = "izot-nvd";
const char *GetMyNvdFolder()
{
	return pMyNvdFolder;
}

void SetMyNvdFolder(const char *folder)
{
	pMyNvdFolder = folder;
}

/******************************************************************************
 * Functions 
 ******************************************************************************/

void printHelp()
{
    printf("Commands:\n\n");
    printf("    S - Send service Pin\n");
    printf("    E - Exit (quit)\n");
    printf("    Q - Quit (exit)\n");
    printf("    ? - Print this screen\n");
    printf("\n");
}

/* The main function processes command parameters, creates the application 
 * task to the main stack loopand then runs a simple console to 
 * allow sending a service pin messages and shutting down the application
 */
int main(int argc, char* argv[])
{
#if FEATURE_INCLUDED(IP852)
#if PRODUCT_IS(IZOT)
    if (argc < 4 || !SetMyIpAddress(argv[1], argv[2], argv[3], (argc > 4) ? argv[4] : ""))
#else
    if (argc < 3 || !SetMyIpAddress(argv[1], argv[2], "", ""))
#endif
    {
    printf("Run a simple IzoT Device using an IP interface\n\n"
           "Syntax:\n"
#if PRODUCT_IS(IZOT)
           "    %-19s <ipAddress> <ipPort> <protocolType> <interface> [nvdFolder][ltsLogFile][ISILogFile]\n"
#else
           "    %-19s <ipAddress> <ipPort>\n"
#endif
           "\n"
           "        <ipAddress>     is the IPv4 dotted-decimal address to use\n"
           "        <port>          is a decimal port number to use.  Specify 0 if ldv.\n"
#if PRODUCT_IS(IZOT)
           "        <protocolType>  is ldv(Windows LON device), uc(unicast),mc(multicast), izot(IzoT IP) or\n"
           "                        izot-e(IzoT IP Enhanced Mode)\n"
           "        <interface>     network interface name (for izot/izot-e only), e.g. eth1(for ethernet 1),\n"
           "                        wlan0(for wifi 0), or lon[X](for non-Windows LON X device).\n"
           "                        Specify 0 for others.\n"
           "        [nvdFolder]     folder to store nvd files\n"
           "        [ltsLogFile]    Stack trace log file\n"
           "        [ISILogFile]    ISI trace log file\n"
#endif
           "\n", argv[0]);
        return 1;
    }
#else
    if (argc < 2)
    {
        printf("Run a simple IzoT Device using a standard IzoT network interface\n\n"
               "Syntax:\n"
               "    SimpleLtDevice <niName>"
               "\n"
               "        <niName>    is the name of the native IzoT network interface\n"
               "\n");
        return 1;
    }
    else
    {
        SetMyNetworkInterface(argv[1]);
    }
#endif

#if PRODUCT_IS(IZOT)
    if (argc > 5)
    	SetMyNvdFolder(argv[5]);
    if (argc > 6)
    {
        LonSetTracefile(argv[6], FALSE);	 /* Setting up a LTS trace file */
#ifdef ISI
        if (argc > 7)
            IsiSetTracefile(argv[7], FALSE);	 /* Setting up a ISI trace file */
        else
            IsiSetTracefile("ISITrace.log", FALSE);	 /* Setting up a ISI trace file */
#endif
    }
    else
    {
        LonSetTracefile("LTSTrace.log", FALSE);	 /* Setting up a LTS trace file */
#ifdef ISI
        IsiSetTracefile("ISITrace.log", FALSE);	 /* Setting up a ISI trace file */
#endif
    }
#endif

    if (OsalCreateEvent(&eventReadyHandle) == OSALSTS_SUCCESS)
    {
        OsalHandle taskHandle;
        OsalTaskId taskId;
        appTaskRunning = TRUE;
        if (OsalCreateTask(appTask, 0 /* Not used */, 64*1024, 11, &taskHandle, &taskId) == OSALSTS_SUCCESS)
        {

            printHelp();
            printf("> ");
            while (appTaskRunning)
            {
                char c = toupper(getchar());
                if (c == 'S')
                {
                    sendServicPin = TRUE;
                    printf("Sending service pin...\n");
                    OsalSetEvent(eventReadyHandle);
                }
                else if (c == 'Q' || c == 'E')
                {
                    shutdownApp = TRUE;
                    printf("Exiting...\n");
                    OsalSetEvent(eventReadyHandle);
                    break;
                }
                else if (c == 0x0a)
                {
                    printf("> ");
                    ;
                }
                else
                {
                    if (c != '?')
                    {
                        printf("Unrecognized command\n");
                    }
                    printHelp();
                }
            }
            while (appTaskRunning)
            {
            	OsalSleep(10);
            }
            OsalCloseTaskHandle(taskHandle);
        }
        OsalDeleteEvent(&eventReadyHandle);           
    }

    return 0;
}

/* The application task initializes the IzoT protocol stack and 
 * implements the main control loop.  The bulk of the application processing 
 * is performed in the myNvUpdateOccurred event handler.
 */
void appTask(int taskIndex)
{
    /* Create the "event ready" event, which is signaled by the myEventReady 
     * callback to wake this task up to process IzoT Device Stack events.
     */
    /* Initialize the IzoT Device Stack */
    LonApiError sts;
#if FEATURE_INCLUDED(IP852)
    #pragma message ("Warning:  TBD - Must set a valid UniqueID for IP-852 interface!!!")
#if !PRODUCT_IS(IZOT)
    // TBD - Set the unique ID.  
    // You may want to use a new API LonGenerateUniqueId to generate a new unique ID
    // For the IzoT case, the unique ID is specified when you call LonLidCreateStackEx
    LonUniqueId uid = { 0xBA, 0xDB, 0xAD, 0xBA, 0xDB, 0xAD };
    printf("UniqueID = %02x %02x %02x %02x %x\n", uid[0], uid[1], uid[2], uid[3], uid[4], uid[5]);
    LonRegisterUniqueId(&uid);
#endif
#endif

    sts = LonInit();
    if (sts == LonApiNoError)
    {
        /* This is the main control loop, which runs forever. */

        while (!shutdownApp)
        {
            /* Whenever the ready event is fired, process events by calling 
             * LonEventPump.  The ready event is fired by the myEventReady 
             * callback. 
             */
            if (OsalWaitForEvent(eventReadyHandle, OSAL_WAIT_FOREVER) == OSALSTS_SUCCESS)
            {
                LonEventPump();
            }
            if (sendServicPin)
            {
                LonSendServicePin();
                sendServicPin = FALSE;
            }
        }
    }
    else
    {
        printf("Error: LonInit failed with error #%d\n", sts);
    }
    LonExit();
    appTaskRunning = FALSE;
}

/******************************************************************************
 * Callback Handler Implementation Functions
 ******************************************************************************/

/*
 * This function is called by the IzoT LonEventReady callback, signaling that
 * an event is ready to be processed. 
 */
void myEventReady(void)
{
    /* Signal application task so that it can process the event. */
    OsalSetEvent(eventReadyHandle);
}

/******************************************************************************
 * Event Handler Implementation Functions
 ******************************************************************************/

/*
 *  This function is called by the IzoT LonReset event handler, 
 *  indicating that the IzoT protocol stack has been reset.
 *
 *  Parameters:
 *  pResetNotification - <LonResetNotification> structure with capabilities 
 *      and identifying data
 *
 *  Remarks:
 *  The pointer to <LonResetNotification> is provided for call compatibility with 
 *  the ShortStack LonTalk Compact API.  For IzoT, the value of 
 *  pResetNotification is always NULL.
 * 
 *  Whenever the IzoT device has been reset, the mode of the device is changed 
 *  to *online*, but no LonOnline() event is generated.
 *  
 *  Note that resetting the IzoT device only affects the IzoT protocol 
 *  stack and does not cause a processor or application software reset.
 */
void myReset(const LonResetNotification* const pResetNotification)
{
    LonStatus status;
    /* Check to see if the device is online. */
    if (LonQueryStatus(&status) == LonApiNoError && 
        status.NodeState == LonConfigOnLine)
    {
        /* Process inputs that may have changed while the device was offline. */
        ProcessOnlineEvent();
    }
}

/*
 *  This function is called by the IzoT LonGetCurrentNvSize event handler, 
 *  which gets the current size of a network variable.
 *
 *  Parameters:
 *  index - the local index of the network variable
 *
 *  Returns:     
 *  Current size of the network variable. Zero if the index is invalid.
 */
unsigned myGetCurrentNvSize(const unsigned index)
{
    return LonGetDeclaredNvSize(index);
}


/*
 *  This function is called by the IzoT LonOnline event handler, 
 *  indicating that the IzoT protocol stack has been set online.
 */
void myOnline(void)
{
    /* Process inputs that may have changed while the device was offline. */
    ProcessOnlineEvent();
}

/*
 *  This function is called by the IzoT LonNvUpdateOccurred event handler, 
 *  indicating that a network variable input has arrived.
 *
 *  Parameters:
 *  index - global index (local to the device) of the network variable in question 
 *  pNvInAddr - pointer to source address description 
 *
 *  Remarks:
 *  The network variable with local index given in this event handler has been 
 *  updated with a new value. The new value is already stored in the network 
 *  variable's location; access the value through the global variable 
 *  representing the network variable, or obtain the pointer to the network 
 *  variable's value from the <LonGetNvValue> function. The pNvInAddr 
 *  pointer is only valid for the duration of this event handler.
 * 
 *  For an element of a network variable array, the index is the global network 
 *  variable index plus the array-element index. For example, if nviVolt[0] has
 *  global network variable index 4, then nviVolt[1] has global network variable 
 *  index 5.
 */
void myNvUpdateOccurred(const unsigned nvIndex, 
                        const LonReceiveAddress* const pNvInAddr)
{
    switch (nvIndex)
    {
        case LonNvIndexNviVolt:
        {
            /* process update to nviVolt. */
            ProcessNviVoltUpdate();
            break;
        }
        /* Add more input NVs here, if any */
    
        default:
            break;
    }
}

/*
 *  This function is called by the IzoT LonServiceLedStatus event handler, 
 *  indicating that the service led status has changed.
 *
 *  Parameters:
 *  state - determines the state of the service pin.  "state" values are  
 *          defined as per the LON-C specification.
 *
 *  Remarks:
 *  Note that the IzoT protocol stack calls <LonServiceLedStatus>
 *  when the status of service pin is changed. 
 */
void myServiceLedStatus(LtServicePinState state)
{
    printf("myServiceLedStatus = %d\n", state);
}

/*
 *  This function is called by the IzoT LonMsgArrived event handler, 
 *  indicating that the application message has arrived.
 *
 *  Parameters:
 *  pAddress - source and destination address (see <LonReceiveAddress>)
 *  correlator - correlator to be used with <LonSendResponse>
 *  authenticated - TRUE if the message was (successfully) authenticated
 *  code - message code
 *  pData - pointer to message data bytes, might be NULL if dataLength is zero
 *  dataLength - length of bytes pointed to by pData
 *
 *  Remarks:
 *  This event handler reports the arrival of a message that is neither a network 
 *  variable message or a non-Nv message that is otherwise processed by the 
 *  IzoT device (such as a network management command). Typically, this is used 
 *  with application message codes in the value range indicated by the 
 *  <LonApplicationMessageCode> enumeration. All pointers are only valid for 
 *  the duration of this event handler. 
 *
 *  If the message is a request message, then the function must deliver a 
 *  response using <LonSendResponse> passing the provided *correlator*.  
 *  Alternatively, if for any reason the application chooses not to respond to 
 *  a request, it must explicitly release the correlator by calling 
 *  <LonReleaseCorrelator>.
 *
 *  Application messages are always delivered to the application, regardless 
 *  of whether the message passed authentication or not. It is up to the 
 *  application to decide whether authentication is required for any given 
 *  message and compare that fact with the authenticated flag. The 
 *  authenticated flag is clear (FALSE) for non-authenticated messages and for
 *  authenticated messages that do not pass authentication. The authenticated 
 *  flag is set only for correctly authenticated messages. 
 */
void myMsgArrived(const LonReceiveAddress* const pAddress, 
                   const LonCorrelator correlator,
                   const LonBool priority, 
                   const LonServiceType serviceType, 
                   const LonBool authenticated, 
                   const LonByte code, 
                   const LonByte* const pData, const unsigned dataLength)
{
    /*
     * TBD
     */

    if (serviceType == LonServiceRequest)
    {
        /* Because this example implementation does not send responses to 
         * request messages, it must release the correlator to prevent memory
         * leaks.  Your implementation should either call LonSendResponse or
         * LonReleaseCorrelator for each request message that arrives.
         */
        LonReleaseCorrelator(correlator);
    }
}


/******************************************************************************
 *                              Utility functions 
 ******************************************************************************/

/* Process input network variables and configuration property values when 
 * the device goes online. 
 */
void ProcessOnlineEvent(void)
{
    /* If a network variable is updated while the IzoT device is offline, the
     * value of the variable will be changed, but the application will not be 
     * notified.  This function processes the current network variable input
     * and configuration property values. 
     */

    ProcessNviVoltUpdate();     /* Process changes to nviVolt. */

    /* Add other online processing here. */
}

/* Process a change to nviVolt */
void ProcessNviVoltUpdate(void)
{
    /* Whenever nviVolt is updated, set nvoVoltFb to twice the value of 
     * nviVolt. 
     */
    int value = LON_GET_SIGNED_WORD(nviVolt);
    if (value > MAX_VOLT) 
    {
        /* Input value is out of range.  Set it to the maximum */
        value = MAX_VOLT;
        LON_SET_SIGNED_WORD(nviVolt, value);
    }
    else if (value < MIN_VOLT) 
    {
        /* Input value is out of range.  Set it to the minimum */
        value = MIN_VOLT;
        LON_SET_SIGNED_WORD(nviVolt, value);
    }

    /* Set nvoVoltFb to 2*nviVolt to simulate a built-in gain of 2. In a real 
     * actuator, nviVolt would be used to set a physical output to adjust the 
     * voltage level.  The resulting voltage output would then be read and 
     * reported using the nvoVoltFb. 
     */
    LON_SET_SIGNED_WORD(nvoVoltFb, value * 2);

    /* Propagate the NV onto the network. */
    if (LonPropagateNv(LonNvIndexNvoVoltFb) != LonApiNoError)
    {
        /* Handle error here, if desired. */
    }
}
