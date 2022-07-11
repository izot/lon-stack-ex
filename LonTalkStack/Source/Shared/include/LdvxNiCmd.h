//////////////////////////////////////////////////////////////////////
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
// LdvxNiCmd.h: Declaration of xDriver Network Interface local
//              command types and constants.
//
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// This file is also used in the i.LON100 RNI code and so has to
// be ported on the VxWorks platform. Therefore, before checking-in any 
// changes, make sure that all the Windows specific code is compilable
// only when _WIN32 is defined - Ritesh
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


/**
 * @file
 *
 * xDriver Local Network Interface Commands
 *
 * This file defines types and constants for the
 * xDriver local Network Interface commands.
 */

#ifndef _LDVXNICMD_H
#define _LDVXNICMD_H


// xDriver NI escape command code
const BYTE LDVX_NICMD_CODE              = 0xEF;

// xDriver NI escape sub-commands
enum LDVX_NICMD
{
    LDVX_NICMD_SET_KEY                  = 0x00,
    LDVX_NICMD_SEND_WITH_EXPLICIT_KEY   = 0x01,
    LDVX_NICMD_ENCRYPTION_ON_SEND       = 0x02,
    LDVX_NICMD_ENCRYPTION_OFF_SEND      = 0x03,
    LDVX_NICMD_ENCRYPTION_ON_RECEIVE    = 0x04,
    LDVX_NICMD_ENCRYPTION_OFF_RECEIVE   = 0x05,

    LDVX_NICMD_DEVICE_INFO              = 0x20,

    LDVX_NICMD_RETRIEVE_COUNTERS        = 0x30,
    LDVX_NICMD_RETRIEVE_COUNTERS_RESET  = 0x31,

    LDVX_NICMD_HOLDOFF_ENABLE           = 0x40, // queues MP updates
    LDVX_NICMD_HOLDOFF_DISABLE          = 0x41, // delivers queued MP updates
};

// sub-command data definitions
typedef union LDVX_NICMD_DATA {
    // e.g. sub-command = LDVX_NICMD_SUBCOMMAND
    //struct {
    //    bool bEnable;
    //    int  nType;
    //}                                 SUBCOMMAND;
} LDVX_NICMD_DATA;

#endif // _LDVXNICMD_H
