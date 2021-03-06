/*  
 *   File: FtxlDev.h
 *   Generated by LonTalk Interface Developer 3.00.35
 *
 *   Created on Thu Nov 08 13:07:53 2012
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
#if !defined(DEFINED_FTXLDEV_H)
#define DEFINED_FTXLDEV_H

#include <stddef.h>
#include "LtaDefine.h"
#include "LonPlatform.h"
#include "LonNvTypes.h"
#include "LonCpTypes.h"
#include "FtxlApi.h"

/*
 *  Enumeration: LonNvIndex
 */
typedef LON_ENUM_BEGIN(LonNvIndex)
{
    LonNvIndexNviVolt = 0,
    LonNvIndexNvoVoltFb = 1,
    LonNvCount = 2
} LON_ENUM_END(LonNvIndex);

#define LON_PERSISTENT_NVS  0

/*
 *  Enumeration: LonMtIndex
 */
typedef LON_ENUM_BEGIN(LonMtIndex)
{
    LonMtCount = 0
} LON_ENUM_END(LonMtIndex);

/*
 * Global parameters and preferences
 */
#define LON_TRANSCEIVER                FtxlTransceiverType20MHz
#define LON_SERVICE_PIN_EVENT          1   /* (event enabled) */
#define LON_SERVICE_PIN_TIMER          10   /* 10 seconds */
#define LON_DOMAIN_COUNT               2
#define LON_ADDRESS_COUNT              15
#define LON_ALIAS_COUNT                0
#define LON_RCV_TRANSACTIONS           20
#define LON_TX_TRANSACTIONS            15
#define LON_TX_TRANSACTION_TTL         24576   /* 24.576 seconds */
#define LON_STATIC_NV_COUNT            2
#define LON_BINDABLE_MT_COUNT          0
#define LON_DYNAMIC_NV_COUNT           0
#define LON_FB_COUNT                   1
#define LON_AVG_DYN_NV_SD_LENGTH       0
#define LON_DMF_WINDOW_START           0xA100
#define LON_DMF_WINDOW_SIZE            11776
#define LON_DMF_ENABLED                0
#define LON_APP_SIGNATURE              0x73AC6410ul
#define LON_NVD_FLUSH_TIMEOUT          1
#define LON_NVD_MODEL_USER_DEFINED     0
#define LON_NVD_MODEL_FLASH_DIRECT     0
#define LON_NVD_MODEL_FILE_SYSTEM      1
#define LON_NVD_ROOT_NAME              "."
#define LON_NODE_SD_STRING             "&3.3@4VoltActuator"
#define LON_PROGRAM_ID                 "9F:FF:FF:06:00:0A:04:11"
#define LON_MODEL_FILE                 "C:\LonWorks\LonTalkStack\Examples\SimpleDevice\Simple Example.nc"
#define LON_DMF_WINDOW_USAGE           0   /* (0%) */
#define LON_BUFFER_APP_PRIO_OUT_COUNT  1
#define LON_BUFFER_APP_OUT_COUNT       5
#define LON_BUFFER_APP_IN_COUNT        5
#define LON_BUFFER_LINK_LAYER_COUNT    2
#define LON_BUFFER_XCVR_IN_SIZE        66
#define LON_BUFFER_XCVR_OUT_SIZE       66
#define LON_BUFFER_XCVR_PRIO_OUT_COUNT 3
#define LON_BUFFER_XCVR_OUT_COUNT      3
#define LON_BUFFER_XCVR_IN_COUNT       11

/*
 *  Type: LidNvDefinition
 *  Defines the structure of the nvTable variable.
 *
 *  The LidNvDefinition defines the structure of the nvTable, which is
 *  implemented in FtxlDev.c. This is an extension to the IzoT API
 *  type LonNvDefinition, adding details used by the LID-generated 
 *  skeleton application framework.
 *  The nvTable is used to register static network variables with 
 *  the IzoT stack during startup, and to manage persistent values 
 *  for 'eeprom' and 'cp' network variables.
 */
#define LID_NVDESC_PERSISTENT_MASK    0x01u
#define LID_NVDESC_PERSISTENT_SHIFT   0
#define LID_NVDESC_PERSISTENT_FIELD   Attributes

typedef struct 
{
    LonNvDefinition Definition;      /* the IzoT NV Description data */
    LonByte         Attributes;      /* additional attributes        */
} LidNvDefinition;

/*
 *  Message Tag Table Type Definition
 */
typedef LonBool LonMtDescription;

/*
 * Prototypes for access functions:
 */
extern const LidNvDefinition* const LonGetNvTable(void);
extern const LonMtDescription* const LonGetMtTable(void);

extern const LonApiError LonInit(void);
extern void LonExit(void);

/*
 *  Network Variables
 */
extern volatile SNVT_volt nviVolt;
extern SNVT_volt nvoVoltFb;
#endif	/* defined(DEFINED_FTXLDEV_H) */
/* end of file. */
