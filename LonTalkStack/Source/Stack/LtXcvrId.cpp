//
// LtXcvrId.cpp
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

#ifdef WIN32
#include <windows.h>
#endif

#include <string.h>
#include "LtDriver.h"
#include "LtXcvrId.h"

#if FEATURE_INCLUDED(STDXCVR_FILE_SUPPORT)
    #include "LtXcvrIdXml.h"
    #include "vxlTarget.h"  // For vxlReportEvent

#endif

static LtXcvrIdData xidDefs[] =
{
	{	
		1,
		"TP/XF-78",
		{
			COMM_78KB|CLOCK_FULL,
			COMM_DIFF,
			0x07, 0x04, 0x04, 0x0E, 0x0F,
			0x00, 0x04,			// 4 pri slots
			0x2A, 0xA4,
			0x00, 0x00, 0x00, 0x00, 0x00
		}
	},

	{	
		3,
		"TP/XF-1250",
		{
			COMM_1250KB|CLOCK_FULL,
			COMM_DIFF,
			0x01, 0x04, 0x00, 0x00, 0x00,
			0x00, 0x10,			// 16 pri slots
			0x60, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00
		}
	},

	{	
		4,
		"TP/FT-10",
		{
			COMM_78KB|CLOCK_FULL,
			COMM_SINGLE,
			0x08, 0x05, 0x0C, 0x0E, 0x0F,
			0x00, 0x04,			// 4 pri slots
			0x00, 0xA4,
			0x00, 0x00, 0x00, 0x00, 0x00
		}
	},

	{	
		5,
		"TP/RS485-39",
		{
			COMM_39KB|CLOCK_FULL,
			COMM_SINGLE,
			0x04, 0x05, 0x06, 0x0E, 0x10,
			0x00, 0x04,			// 4 pri slots
			0x00, 0x4C,
			0x00, 0x00, 0x00, 0x00, 0x00
		}
	},

	{	
		7,
		"RF-10",
		{
			COMM_9KB|CLOCK_HALF,
			COMM_SINGLE,
			0x4F, 0x13, 0x18, 0x22, 0x27,
			0x00, 0x04,			// 4 pri slots
			0x60, 0x4C,
			0x00, 0x00, 0x00, 0x00, 0x00
		}
	},

	{
		9,
		"PL-10",
		{
			COMM_625KB|CLOCK_FULL,
			COMM_SPPUR,
			0x00, 0x10, 0x0C, 0x3B, 0x0F,
			0x00, 0x08,			// 8 pri slots
			0x00, 0x0A,	
			0x00, 0x00, 0x00, 0x00, 0x00
		}
	},

	{
		10,
		"TP/RS485-625",
		{
			COMM_625KB|CLOCK_FULL,
			COMM_SINGLE,
			0x08, 0x04, 0x02, 0x0E, 0x0F,
			0x00, 0x04,			// 4 pri slots
			0x00, 0x0C,
			0x00, 0x00, 0x00, 0x00, 0x00
		}
	},

	{
		11,
		"TP/RS485-1250",
		{
			COMM_1250KB|CLOCK_FULL,
			COMM_SINGLE,
			0x01, 0x04, 0x00, 0x00, 0x00,
			0x00, 0x10,			// 16 pri slots
			0x00, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00
		}
	},

	{
		12,
		"TP/RS485-78",
		{
			COMM_78KB|CLOCK_FULL,
			COMM_SINGLE,
			0x06, 0x04, 0x04, 0x0E, 0x0F,
			0x00, 0x04,			// 4 pri slots
			0x00, 0xA4,
			0x00, 0x00, 0x00, 0x00, 0x00
		}
	},

	{
		14,
		"PL-20A-LOW",
		{
			COMM_1250KB|CLOCK_FULL,
			COMM_SPPUR,
			0x00, 0x5A, 0xE6, 0xA9, 0x78,
			0x00, 0x08,			// 8 pri slots
			0x0E, 0x01,
			0x00, 0x12, 0x00, 0x01, 0x00
		}		
	},

	{
		15,
		"PL-20A",
		{
			COMM_1250KB|CLOCK_FULL,
			COMM_SPPUR,
			0x00, 0x5A, 0xE6, 0xA9, 0x78,
			0x00, 0x08,			// 8 pri slots
			0x0E, 0x01,
			0x00, 0x10, 0x00, 0x01, 0x00
		}
	},

	{
		16,
		"PL-20C",
		{
			COMM_1250KB|CLOCK_FULL,
			COMM_SPPUR,
			0x00, 0x3F, 0xA6, 0x77, 0x67,
			0x00, 0x08,			// 8 pri slots
			0x4A, 0x00,
			0x00, 0x10, 0x00, 0x00, 0x00
		}
	},

	{
		17,
		"PL-20N",
		{
			COMM_1250KB|CLOCK_FULL,
			COMM_SPPUR,
			0x00, 0x3F, 0xA6, 0x77, 0x67,
			0x00, 0x08,			// 8 pri slots
			0x0E, 0x01,
			0x00, 0x10, 0x00, 0x00, 0x00,
		}
	},

	{
		18,
		"PL-30",
		{
			COMM_625KB|CLOCK_FULL,
			COMM_SPPUR,
			0x00, 0x4D, 0x30, 0xC4, 0x0F,
			0x00, 0x0C,			// 12 pri slots
			0x00,				// 0x7F for comm anzr
			0x8A, 0x00, 0x00, 0x00, 0x00, 0x00 // 0x8A auto threshold
		}
	},

    {
		19,
		"PL-20C-LOW",
		{
			COMM_1250KB|CLOCK_FULL,
			COMM_SPPUR,
			0x00, 0x3F, 0xA6, 0x77, 0x67,
			0x00, 0x08,			// 8 pri slots
			0x4A, 0x00,
			0x00, 0x12, 0x00, 0x00, 0x00
		}
	},

	{
		20,
		"PL-20N-LOW",
		{
			COMM_1250KB|CLOCK_FULL,
			COMM_SPPUR,
			0x00, 0x3F, 0xA6, 0x77, 0x67,
			0x00, 0x08,			// 8 pri slots
			0x0E, 0x01,
			0x00, 0x12, 0x00, 0x00, 0x00,
		}
	},

	{
		21,
		"Mot-RF450",
		{
			COMM_9KB|CLOCK_HALF,
			COMM_SINGLE,
			0xFD, 0x17, 0x18, 0x22, 0x27,
			0x00, 0x04,			// 4 pri slots
			0x60, 0x4C,
			0x00, 0x00, 0x00, 0x00, 0x00
		}
	},

	{
		22,
		"IR-10",
		{
			COMM_39KB|CLOCK_FULL,
			COMM_SINGLE,
			0x03, 0x05, 0x04, 0x0E, 0x0F,
			0x00, 0x04,			// 4 pri slots
			0x00, 0x4C,
			0x00, 0x00, 0x00, 0x00, 0x00
		}
	},

	{
		23,
		"IR-20",
		{
			COMM_78KB|CLOCK_FULL,
			COMM_SINGLE,
			0x05, 0x04, 0x02, 0x0E, 0x0F,
			0x00, 0x04,			// 4 pri slots
			0x00, 0xA4,
			0x00, 0x00, 0x00, 0x00, 0x00
		}
	},

	{
		24,
		"FO-10",
		{
			COMM_1250KB|CLOCK_FULL,
			COMM_SINGLE,
			0x05, 0x04, 0x00, 0x00, 0x00,
			0x00, 0x10,			// 16 pri slots
			0x80, 0x07,
			0x00, 0x00, 0x00, 0x00, 0x00
		}
	},

	{
		25,
		"IP-10L",
		{	// bogus values, mostly from TP-1250
			COMM_1250KB|CLOCK_FULL,
			COMM_DIFF,
			0x01, 0x04, 0x00, 0x00, 0x00,
			0x00, 0x00,			// no pri slots
			0x60, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00
		}
	},

	{	
		30,
		"Custom",
		{
			COMM_1250KB|CLOCK_FULL,
			COMM_DIFF,
			0x01, 0x04, 0x00, 0x00, 0x00,
			0x00, 0x00,			// no pri slots
			0x00,
			0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
		}
	},

	{
		154,		// the LonMark approved TXID for IP
		"IP-852",	
		{	// bogus values, mostly from TP-1250
			COMM_1250KB|CLOCK_FULL,
			COMM_DIFF,
			0x01, 0x04, 0x00, 0x00, 0x00,
			0x00, 0x00,			// no pri slots
			0x60, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00
		}
	},

    {
	    207,
        "PL-20A (alternate clock)",
	    {
		    COMM_1250KB|CLOCK_HALF,
		    COMM_SPPURALT,
		    0x00, 0x3B, 0x96, 0x5A, 0x4A,
		    0x00, 0x08,			// 8 pri slots
		    0x0E, 0x01,
		    0x00, 0x10, 0x00, 0x01, 0x00
	    }
    },

    {
    	XID_NES,
         "PL-20A (alternate clock, NES)",
 	    {
 		    COMM_1250KB|CLOCK_HALF,
 		    COMM_SPPUR_NES,
 		    0x00, 0x3B, 0x96, 0x5A, 0x4A,
 		    0x00, 0x00,			// 0 pri slots
 		    0x0E, 0x01,
 		    0x00, 0x12, 0x00, 0x01, 0x00
 	    }
     },

     {
     	XID_NES_INTERNAL,
          "PL-20A (alternate clock, NES, AGC off, no atten)",
  	    {
  		    COMM_1250KB|CLOCK_HALF,
  		    COMM_SPPUR_NES,
  		    0x00, 0x3B, 0x96, 0x5A, 0x4A,
  		    0x00, 0x00,			// 0 pri slots
  		    0x1E, 0x01,
  		    0x00, 0x12, 0x00, 0x01, 0x00
  	    }
      },
	{	
		-1,
		"<not defined>",
		{
			COMM_1250KB|CLOCK_FULL,
			COMM_DIFF,
			0x01, 0x04, 0x00, 0x00, 0x00,
			0x00, 0x10,			// 16 pri slots
			0x60, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00
		}
	},
};

#if FEATURE_INCLUDED(STDXCVR_FILE_SUPPORT)

// Global values used to store std xcvr information read from stdxcvr.xml file.
bool LtXcvrId::m_xmlXidTableInitialized = FALSE;
int LtXcvrId::m_numXmlXidEntries = 0;
LtXcvrIdData **LtXcvrId::m_xmlXidTable = NULL; // Table of XID pointers...

#if defined(DEBUG_STDXCVR_FILE)
void LtXcvrId::displayXidCommParms(const char *title, const LtXcvrIdData *pXidData, const LtXcvrIdData *pXidDataCompare)
{
    int i;
    char stringBuf[100];
    char *s = stringBuf;
    for (i = 0; i < 16; i++)
    {
        char buf[10];
        if (pXidDataCompare == NULL || pXidData->m_data[i] == pXidDataCompare->m_data[i])
        {
            *s++ = ' ';
        }
        else
        {
            *s++ = '*';
        }
        sprintf(buf, "%.2x", pXidData->m_data[i]);
        strcpy(s, buf);
        s += 2;
    }
    vxlReportEvent("%s: %s\n", title, stringBuf);
}

void LtXcvrId::reportStdXcvFileEntry(const LtXcvrIdData *pXidData)
{
    const LtXcvrIdData *pHardCodedXidData = getHardCodedXcvrIdData(pXidData->m_xid);
    if (pHardCodedXidData != NULL && pHardCodedXidData->m_xid != -1)
    {
        vxlReportEvent("XID %d (%s) overriden by XML entry (%s)\n",  
                       pHardCodedXidData->m_xid, pHardCodedXidData->m_szName, pXidData->m_szName);
        if (memcmp(pHardCodedXidData->m_data, pXidData->m_data, sizeof(pHardCodedXidData->m_data)) != 0)
        {                
            vxlReportEvent("  Comm parms do not match\n");                            
            displayXidCommParms("    XMLCommParms", pXidData);
            displayXidCommParms("    VNICommParms", pHardCodedXidData, pXidData);
        }
    }
    else
    {
        vxlReportEvent("XID %d (%s) added from XML\n", pXidData->m_xid, pXidData->m_szName);
        displayXidCommParms("    XMLCommParms", pXidData);
    }
}
#endif
    
void LtXcvrId::initXidDataFromXml(void)
{
    if (!m_xmlXidTableInitialized)
    {
        LtStdXcvrXmlConfig xmlConfig;
        if (xmlConfig.LoadFromFile())
        {
            int maxEntries = xmlConfig.transceivers.Count();
            int allocSize = sizeof(LtXcvrIdData*)*maxEntries;
            m_xmlXidTable = (LtXcvrIdData **)malloc(allocSize);

            memset(m_xmlXidTable, 0, allocSize);
            m_numXmlXidEntries = 0;

            CXMLTransceiver *pTransceiver;
            for (int i = 0; i < maxEntries; i++)
            {
                pTransceiver = xmlConfig.transceivers[i];
                if (pTransceiver != NULL)
                {
                    if (pTransceiver->stdXcvrData.stdXcvrCommParms.isValid())
                    {
                        m_xmlXidTable[m_numXmlXidEntries] = new LtXcvrIdData;
                        pTransceiver->stdXcvrData.stdXcvrCommParms.getData(m_xmlXidTable[m_numXmlXidEntries]->m_data);
                        m_xmlXidTable[m_numXmlXidEntries]->m_xid = pTransceiver->xid;
                        stdString name = pTransceiver->name.m_StrValue;
                        char *pName = new char[name.length()+1];
                        strcpy(pName,name.c_str());
                        m_xmlXidTable[m_numXmlXidEntries]->m_szName = pName;
#if defined(DEBUG_STDXCVR_FILE)
                        reportStdXcvFileEntry(m_xmlXidTable[m_numXmlXidEntries]);
#endif
                        m_numXmlXidEntries++;
                    }
                    else
                    {
                        const char *pName = pTransceiver->name.m_StrValue.c_str();
                        int xid = pTransceiver->xid;
#if defined(DEBUG_STDXCVR_FILE)
                        vxlReportEvent("XID %d (%s) in standard transciever file does not have any comm parms\n",  
                                        xid, pName);
#endif

                    }
                }
            }
        }
        else
        {
            vxlReportEvent("Failed to load stdxcvr.xml\n");
        }
        m_xmlXidTableInitialized = true;

#if defined(DEBUG_STDXCVR_FILE)
        {
           // Report if there are any hard-coded entries that are not in the XML file
            int i;
            i = 0;
            for (i = 0; xidDefs[i].m_xid != -1; i++)
            {
                if (getXcvrIdDataFromXml(xidDefs[i].m_xid) == NULL)
                {
                    vxlReportEvent("XID %d (%s) missing from XML\n", xidDefs[i].m_xid, xidDefs[i].m_szName);
                    displayXidCommParms("    VNICommParms", &xidDefs[i]);
               }
            }
        }
#endif
    }
}

LtXcvrIdData* LtXcvrId::getXcvrIdDataFromXml(int xid)
{
    int i;
    LtXcvrIdData *pXidData = NULL;
    initXidDataFromXml();
    for (i = 0; pXidData == NULL && i < m_numXmlXidEntries; i++)
    {
        if (m_xmlXidTable[i] != NULL && m_xmlXidTable[i]->m_xid == xid)
        {
            pXidData = m_xmlXidTable[i];
        }
    }
    return pXidData;
}
#endif

void LtXcvrId::freeXidData(void)
{
#if FEATURE_INCLUDED(STDXCVR_FILE_SUPPORT)
    int i;
    if (m_xmlXidTable != NULL)
    {
        for (i = 0; i < m_numXmlXidEntries; i++)
        {
            if (m_xmlXidTable[i] != NULL)
            {
                delete m_xmlXidTable[i]->m_szName;
                delete m_xmlXidTable[i];
            }
        }
    }

    free(m_xmlXidTable);
    m_xmlXidTableInitialized = FALSE;
    m_numXmlXidEntries = 0;
    m_xmlXidTable = NULL;
#endif
}

LtXcvrIdData* LtXcvrId::getHardCodedXcvrIdData(int xid)
{
	int i = 0;

	while (xidDefs[i].m_xid != -1)
	{
		if (xidDefs[i].m_xid == xid) break;
		i++;
	}
	return &xidDefs[i];
}


LtXcvrIdData* LtXcvrId::getXcvrIdData(int xid)
{
#if FEATURE_INCLUDED(STDXCVR_FILE_SUPPORT)
    LtXcvrIdData *pXidData = getXcvrIdDataFromXml(xid);

    if (pXidData == NULL)
    {
        pXidData = getHardCodedXcvrIdData(xid);
    }
	return pXidData;
#else
    return getHardCodedXcvrIdData(xid);
#endif
}

LtErrorType LtXcvrId::getCommParams(int xid, LtCommParams* pCps)
{
	LtErrorType err = LT_NOT_FOUND;
	LtXcvrIdData* pData = getXcvrIdData(xid);
	if (pData->m_xid != -1)
	{
		memcpy(pCps->m_nData, pData->m_data, sizeof(pCps->m_nData));
		err = LT_NO_ERROR;
	}
	else
	{
		memset(pCps->m_nData, COMM_UNKNOWN, sizeof(pCps->m_nData));
	}
	return err;
}

const char* LtXcvrId::getXcvrName(int xid)
{
	return getXcvrIdData(xid)->m_szName;
}

