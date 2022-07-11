#ifndef LTXCVRID_H
#define LTXCVRID_H

//
// LtXcvrId.h
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

#include "LonTalk.h"
#include "LtXcvrIdDefs.h"

#ifdef m_data
#undef m_data /* mbuf.h defines this as m_un.m_dat */
#endif

#if FEATURE_INCLUDED(STDXCVR_FILE_SUPPORT)
    // Define DEBUG_STDXCVR_FILE to enable addtional debugging.
    #define DEBUG_STDXCVR_FILE
#endif

typedef struct 
{
	int  m_xid;
	const char *m_szName;
	byte m_data[16];
} LtXcvrIdData;

#define COMM_1250KB		0x00
#define COMM_625KB		0x08
#define COMM_312KB		0x10
#define COMM_156KB		0x18
#define COMM_78KB		0x20
#define COMM_39KB		0x28
#define COMM_19KB		0x30
#define COMM_9KB		0x38

#define CLOCK_FULL		5
#define CLOCK_HALF		4
#define CLOCK_QTR		3
#define CLOCK_8TH		2
#define CLOCK_16TH		1

#define COMM_SINGLE		0x2e
#define COMM_DIFF		0xac
#define COMM_SPPUR		0x5e
#define COMM_SPPURALT	0x5f
#define COMM_GENERIC	0xa0
// In a meter, the comm type is 0xDB.  We use 0x5B in the DCM in order to make the OAOO bit work right.
// For more details, see EPR 28694.
// And, it shouldn't really matter anyway.  0x80 bit is used to enable attenuation control which was used
// in the Enel system but not in NES.
#define COMM_SPPUR_NES	0x5b
#define COMM_UNKNOWN	0xff

class LtCommParams;

class LtXcvrId
{
public:
	static LtXcvrIdData* getXcvrIdData(int xid);
	static LtErrorType getCommParams(int xid, LtCommParams* pCps);
	static const char* getXcvrName(int xid);

    // This function should be called prior to shutdown to release memory used when reading
    // XID data.
    static void freeXidData(void);

#if FEATURE_INCLUDED(STDXCVR_FILE_SUPPORT)

    private:
        static void initXidDataFromXml(void);
        static LtXcvrIdData* getXcvrIdDataFromXml(int xid);

        #if defined(DEBUG_STDXCVR_FILE)
            static void displayXidCommParms(const char *title, const LtXcvrIdData *pXidData, const LtXcvrIdData *pXidDataCompare = NULL);
            static void reportStdXcvFileEntry(const LtXcvrIdData *pXidData);
        #endif

        // Global values used to store std xcvr information read from stdxcvr.xml file.
        static bool m_xmlXidTableInitialized; // True if we have read, or attempted to read stdxcvrXml
        static int m_numXmlXidEntries;        // Number of entries read from stdXcvrXml.  
        static LtXcvrIdData **m_xmlXidTable;  // Table of XID pointers.  Bounded by numXmlXidEntries

#endif
    static LtXcvrIdData* getHardCodedXcvrIdData(int xid);
};

#endif
