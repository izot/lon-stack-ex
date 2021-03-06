/*  
 *   File: FtxlDev.c
 *   Generated by LonTalk Interface Developer 3.00.36
 *
 *   Created on Fri Jul 31 11:04:39 2009
 *   This file applies to the device with program ID 9F:FF:FF:06:00:0A:04:11
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
 */
#if !defined(DEFINED_FTXLDEV_C)
#define DEFINED_FTXLDEV_C

#include <string.h>
#include "FtxlDev.h"
#ifdef ISI
#include "IsiApi.h"
#endif
#include <stdio.h>

#if PRODUCT_IS(IZOT)
extern void RegisterMyCallbackEvents(void);
extern void RegisterMyNvdFlashFs(void);
extern const char *GetMyDeviceURI();
extern const char *GetMyNvdFolder();
#endif

/*
 *  Network Variables
 */
volatile SNVT_volt nviVolt;   /* sd_string("@0|1") */

SNVT_volt nvoVoltFb;          /* sd_string("@0|2") */


static const LonStackInterfaceData interfaceData = {
    LON_STACK_INTERFACE_CURRENT_VERSION,
    LON_APP_SIGNATURE,
    {0x9Fu, 0xFFu, 0xFFu, 0x06u, 0x00u, 0x0Au, 0x04u, 0x11u},
    LON_STATIC_NV_COUNT,
    LON_STATIC_NV_COUNT+LON_DYNAMIC_NV_COUNT,
    LON_DOMAIN_COUNT,
    LON_ADDRESS_COUNT,
    LON_ALIAS_COUNT,
    LON_BINDABLE_MT_COUNT,
    LON_NODE_SD_STRING,
    LON_AVG_DYN_NV_SD_LENGTH
};

static const LonControlData controlData = {
    LON_CONTROL_DATA_CURRENT_VERSION,
    LON_SERVICE_PIN_TIMER,
    LON_NVD_FLUSH_TIMEOUT,
    {   /* communication parameters: */
        LON_TRANSCEIVER,
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    },
    {
        {   /* application buffers: */
            LON_BUFFER_APP_PRIO_OUT_COUNT,
            LON_BUFFER_APP_OUT_COUNT,
            LON_BUFFER_APP_IN_COUNT
        },
        {   /* link-layer buffers: */
            LON_BUFFER_LINK_LAYER_COUNT
        },
        {   /* transceiver buffers: */
            LON_BUFFER_XCVR_IN_SIZE,
            LON_BUFFER_XCVR_OUT_SIZE,
            LON_BUFFER_XCVR_PRIO_OUT_COUNT,
            LON_BUFFER_XCVR_OUT_COUNT,
            LON_BUFFER_XCVR_IN_COUNT
        },
    },
    LON_RCV_TRANSACTIONS,
    LON_TX_TRANSACTIONS,
    LON_TX_TRANSACTION_TTL
};


/*
 * Constant: nvTable
 * Network variables' definitions.
 */
#define VERSION      LON_NV_DEFINITION_CURRENT_VERSION           /* abbreviation for IzoT NV definition format version   */
#define UNRESTRICTED LON_NV_SERVICE_CONFIG+LON_NV_PRIORITY_CONFIG+LON_NV_AUTH_CONFIG    /* abbreviation, common flags   */
#define DEFAULT      LON_NV_ACKD+UNRESTRICTED                    /* abbreviation for common network variable attributes */
#define RATE_UNKNOWN 0,0                                         /* abbreviation for undefined max and mean NV rates    */
#define NVPTR        void* const                                 /* abbreviation for value pointer type cast            */

static const LidNvDefinition nvTable[] = {
    {{VERSION, (NVPTR)&nviVolt,  (LonByte)sizeof(SNVT_volt),  44, 0, DEFAULT, "nviVolt", "@0|1", RATE_UNKNOWN}, LON_NV_NONE},
    {{VERSION, (NVPTR)&nvoVoltFb,(LonByte)sizeof(SNVT_volt),  44, 0, LON_NV_IS_OUTPUT+UNRESTRICTED+LON_NV_UNACKD, "nvoVoltFb", "@0|2", RATE_UNKNOWN}, LON_NV_NONE}
};

/*
 * Function: LonGetNvTable
 * Returns the base address of the (static) network variable table
 */
const LidNvDefinition* const LonGetNvTable(void)
{
    return nvTable;
}

/*
 * Function: LonGetMtTable
 * Returns the base address of the message tag table
 */
const LonMtDescription* const LonGetMtTable(void)
{
    return NULL;
}

/*
 * Function: LonInit
 * Initializes the IzoT stack and this application framework

 */
static size_t nonVolatileNvData = 0;

const LonApiError LonInit(void)
{
    const LidNvDefinition* pNv = nvTable;
    size_t definitions = 0;
    LonApiError result = LonApiNoError;
   
#if PRODUCT_IS(IZOT)
    char szTemp[200];
    LonUniqueId tempUid;
#endif

    definitions = sizeof(nvTable)/sizeof(LidNvDefinition);

#if PRODUCT_IS(IZOT)
    /* create the stack */
    LonSetNvdFsPath(GetMyNvdFolder());
    result = LonSetDeviceUri(GetMyDeviceURI());
    if (result == LonApiNoError)
        result = LonLidCreateStack(&interfaceData, &controlData);
    if (result == LonApiNoError)
    {
        /*  Register of all the callback events for the IzoT Device Stack API callback functions. */
        RegisterMyCallbackEvents();
        // Commented out the following, just use the default
        // RegisterMyNvdFlashFs();  

        LonGetDeviceUri(szTemp, sizeof(szTemp));
        printf("Device URI = %s\n", szTemp);
        LonGetUniqueId(&tempUid);
        printf("UniqueID = %02x %02x %02x %02x %02x %02x\n", tempUid[0], tempUid[1], tempUid[2], tempUid[3], tempUid[4], tempUid[5]);
        LonGetNvdFsPath(szTemp, sizeof(szTemp));
        printf("NVD Path = %s\n", szTemp);
    }
#else
    /* create the stack */
    result = LonLidCreateStack(&interfaceData, &controlData);
#endif

    /* register all static network variables */
    nonVolatileNvData = 0;
    while (result == LonApiNoError && definitions--) 
    {
        result = LonLidRegisterStaticNv(&pNv->Definition);
        if (pNv->Attributes & LID_NVDESC_PERSISTENT_MASK)
        {
            nonVolatileNvData +=  pNv->Definition.DeclaredSize;
        }  
        ++pNv;
    }
#if  LON_DMF_ENABLED
    if (result == LonApiNoError)
    {
        LonLidRegisterMemoryWindow(LON_DMF_WINDOW_START, LON_DMF_WINDOW_USAGE);
    }
#endif  /* LON_DMF_ENABLED */
    /* get going: */
    if (result == LonApiNoError)
    {
        LonLidStartStack();
#if 0
        // set online
        result = LonSetNodeMode(LonApplicationOnLine, LonStateInvalid);
        if (result == LonApiNoError)
            // change the state to configure
            LonSetNodeMode(LonChangeState, LonConfigOnLine);
#endif
    } 
    else 
    {
         LonExit();
    }

    return result;
}

/*
 * Function: LonExit
 * Terminates the IzoT stack.

 */
void LonExit(void)
{
    /*
     * TODO: Add application-specific termination code here
     */



    /*
     * Shut down:
     */
#ifdef ISI
    IsiStop();
#endif
    LonLidDestroyStack();         
}

/*
 * Function: serialize
 * This is an internal function, used by the <LonNvdSerializeSegment>
 * and <LonNvdDeserializeSegment> callback functions.
 */
static const LonApiError serialize(LonBool toNvMemory, void* const pData, const size_t size)
{
    LonApiError result = LonApiNoError;
    const LidNvDefinition* pNv = nvTable;
    size_t index;
    size_t offset = 0;
    size_t definitions = 0;
    char* const pNvd = (char* const)pData;

    if (sizeof(nvTable) != 0)
    {
        definitions = sizeof(nvTable)/sizeof(LidNvDefinition);
    }

    for (index=0; index < definitions; ++index, ++pNv)
    {
        if (pNv->Attributes & LID_NVDESC_PERSISTENT_MASK)
        {
            if (toNvMemory)
            {
                (void)memcpy(pNvd+offset, (void* const)LonGetNvValue((unsigned) index), pNv->Definition.DeclaredSize);
            }
            else
            {
                (void)memcpy((void* const)LonGetNvValue((unsigned) index), pNvd+offset, pNv->Definition.DeclaredSize);
            }
            offset += pNv->Definition.DeclaredSize;
        }
    }

    return result;
}

/******************************************************************************
 * Event Handler Implementation Functions
 ******************************************************************************/

/*
 *  This function is called by the IzoT LonNvdGetApplicationSegmentSize event handler, 
 *  which returns the size of all persistent application data.
 */
const unsigned myNvdGetApplicationSegmentSizea(void)
{
    return (unsigned)nonVolatileNvData;
}

/*
 *  This function is called by the IzoT LonNvdSerializeSegment event handler, 
 *  when application data needs transferring to non-volatile storage
 */
const LonApiError myNvdSerializeSegment(void* const pData, const size_t size)
{
    return serialize(TRUE, pData, size);
}

/*
 *  This function is called by the IzoT LonNvdDeserializeSegment event handler, 
 *  when application data needs loading from non-volatile storage
 */
const LonApiError myNvdDeserializeSegment(const void* const pData, const size_t size)
{
    return serialize(FALSE, (void* const)pData, size);
}

#if !PRODUCT_IS(IZOT)
/*
 * Callback: LonNvdGetApplicationSegmentSize
 * Returns the size of all persistent application data.
 */
const unsigned LonNvdGetApplicationSegmentSize(void)
{
    return myNvdGetApplicationSegmentSizea();
}

/*
 * Callback: LonNvdSerializeSegment
 * Called by the API when application data needs transferring to non-volatile storage
 */
const LonApiError LonNvdSerializeSegment(void* const pData, const size_t size)
{
    return myNvdSerializeSegment(pData, size);
}
#endif

/*
 * Callback: LonNvdDeserializeSegment
 * Called by the API when application data needs loading from non-volatile storage
 */
#if !PRODUCT_IS(IZOT)
const LonApiError LonNvdDeserializeSegment(const void* const pData, const size_t size)
{
    return myNvdDeserializeSegment(pData, size);
}
#endif

#endif	/* defined(DEFINED_FTXLDEV_C) */
/* end of file. */
