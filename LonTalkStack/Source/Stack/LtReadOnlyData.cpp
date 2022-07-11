//
// LtReadOnlyData.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtReadOnlyData.cpp#1 $
//

#include "LtStackInternal.h"

//
// Private Member Functions
//

// Not that NV_processing_off is set to 1 because LNS assumes that a node with NV_processing_on can
// have its tables updated using write memory!!!!
const byte LtReadOnlyData::lonTalkInit[] =
{
	0,0,0,0,0,0,				// 0x00 Neuron ID
	LT_MODEL_NUMBER,			// 0x06 Major model
	LT_MINOR_MODEL_NUMBER,		// 0x07 Minor model
	00,00,02,0xff,0xff,			// 0x08 NV fixed, nv count, SI data
	0,0,0,0,0,0,0,0,            // 0x0d Program ID
	0xC0,0xf0,00,				// 0x15 NV_processing_off|Two domains|state, address count,
	0xff,0xff,0x33,0x33,0x33,	// 0x18 Buffer config (don't support changing now)
	00,00,00,00,00,00,00,		// 0x1d EE, MC(2), IO(4)
	00,							// 0x24	Alias count
	0x08,						// 0x25 MT / supports NM_GET_CAPABILITY_INFO
	00,00,00					// 0x26 LARVA
};

const byte LtReadOnlyData::sizeTable[] = 
{
    /* Code  0- 1 */  255,   0, /* Not used ... */
    /* Code  2- 9 */   20,  21,  22,  24,  26,  30,  34,  42,
    /* Code 10-15 */   50,  66,  82, 114, 146, 210
};

// TBD....
const byte LtReadOnlyData::countTable[] = 
{
    /* Code  0- 1 */  0,   0, /* Not used ... */
    /* Code  2- 9 */   1, 2, 3, 5, 7, 11, 15, 23,
    /* Code 10-15 */   31, 47, 63, 95, 127, 191 
};
//
// Protected Member Functions
//


//
// Public Member Functions
//


LtReadOnlyData::LtReadOnlyData(LtDeviceStack* pStack, int dc, int ac, int mt) 
{
	memcpy(lonTalk, lonTalkInit, sizeof(lonTalk));
    m_pNetworkImage = pStack->getNetworkImage();
    if (pStack->getPlatform()->getUniqueId(&uniqueId))
	{
		uniqueId.getData(&lonTalk[LT_ROD_UNIQUEID_OFFSET]);
	}
	setProgramId();

	// Watch for extended capabilities.
	ac = min(ac, MAX_LEGACY_ADDRESS_TABLE_ENTRIES);
	mt = min(mt, MAX_LEGACY_ADDRESS_TABLE_ENTRIES);

    lonTalk[LT_ROD_DOMAIN_OFFSET] |= (byte)((dc > 0)? 0x40 : 0x00);
    lonTalk[LT_ROD_ADDRESS_OFFSET] |= (byte)(ac << 4);
	lonTalk[LT_ROD_MTAG_OFFSET] |= (byte)(ac << 4);

    setBuffers();
	m_pendingUpdate = FALSE;
}

LtUniqueId* LtReadOnlyData::getUniqueId() 
{
    return &uniqueId;
}

int LtReadOnlyData::getModel() 
{
    return lonTalk[LT_ROD_MODEL_OFFSET];
}

LtProgramId* LtReadOnlyData::getProgramId() 
{
    return &programId;
}

void LtReadOnlyData::setUniqueId()
{
	uniqueId.set(&lonTalk[LT_ROD_UNIQUEID_OFFSET]);
}

void LtReadOnlyData::setProgramId() 
{
    memcpy(programId.getData(), &lonTalk[LT_ROD_PROGRAMID_OFFSET], programId.getLength());
}

void LtReadOnlyData::setProgramId(LtProgramId& programId) 
{
    memcpy(&lonTalk[LT_ROD_PROGRAMID_OFFSET], programId.getData(), programId.getLength());
    setProgramId();
}

void LtReadOnlyData::setBuffers()
{
    byte *pLonTalk = &lonTalk[LT_READ_ONLY_DATA_BUFFER_OFFSET];
    m_appOutBufSize     = sizeTable[(*pLonTalk & 0xf0) >> 4];
    m_appInBufSize      = sizeTable[*pLonTalk++ & 0x0f];

    m_netOutBufSize     = sizeTable[(*pLonTalk & 0xf0) >> 4];
    m_netInBufSize      = sizeTable[*pLonTalk++ & 0x0f];

    m_numNetOutBufs[1]  = countTable[(*pLonTalk & 0xf0) >> 4];  // priority
    m_numAppOutBufs[1]  = countTable[*pLonTalk++ & 0x0f];       // priority
        
    m_numAppOutBufs[0]  = countTable[(*pLonTalk & 0xf0) >> 4];  // non-priority
    m_numAppInBufs      = countTable[*pLonTalk++ & 0x0f];     
    
    m_numNetOutBufs[0]  = countTable[(*pLonTalk & 0xf0) >> 4];  // non-priority
    m_numNetInBufs      = countTable[*pLonTalk++ & 0x0f];       
}

void LtReadOnlyData::setNetworkBuffers(const LtReadOnlyData &source)
{
    byte *pLonTalk = &lonTalk[LT_READ_ONLY_DATA_BUFFER_OFFSET];
    const byte *pSourceLonTalk = &source.lonTalk[LT_READ_ONLY_DATA_BUFFER_OFFSET];

    pLonTalk++;         // Skip App Buffers
    pSourceLonTalk++;
    
    *pLonTalk++ = *pSourceLonTalk++;    // netOutput and netInput buf sizes;

    // Set num priority NetOut, preserve num priority app buffs
    *pLonTalk = (*pSourceLonTalk++ & 0xf0) | (*pLonTalk & 0x0f);  
	pLonTalk++;

    pLonTalk++;         // Skip App Buffers
    pSourceLonTalk++;

    *pLonTalk++ = *pSourceLonTalk++;    // num non-priority net out, and num net in;

    setBuffers();
}

const boolean LtReadOnlyData::netBuffersMatch(const LtReadOnlyData &ro)
{
    return (m_netOutBufSize     == ro.m_netOutBufSize &&
            m_netInBufSize      == ro.m_netInBufSize &&
            m_numNetInBufs      == ro.m_numNetInBufs &&  
            m_numNetOutBufs[0]  == ro.m_numNetOutBufs[0] &&
            m_numNetOutBufs[1]  == ro.m_numNetOutBufs[1]);
}

void LtReadOnlyData::setBuffers(const LtReadOnlyData &source)
{
    memcpy(&lonTalk[LT_READ_ONLY_DATA_BUFFER_OFFSET], 
           &source.lonTalk[LT_READ_ONLY_DATA_BUFFER_OFFSET], 
           LT_READ_ONLY_DATA_BUFFER_NUM_BYTES);

    setBuffers();
}

const boolean LtReadOnlyData::buffersMatch(const LtReadOnlyData &ro)
{
    return memcmp(&lonTalk[LT_READ_ONLY_DATA_BUFFER_OFFSET], 
                  &ro.lonTalk[LT_READ_ONLY_DATA_BUFFER_OFFSET], 
                  LT_READ_ONLY_DATA_BUFFER_NUM_BYTES) == 0;
}

int LtReadOnlyData::getBufferSize()
{
    int size = 0;
    size += m_appInBufSize * m_numAppInBufs;
    size += m_appOutBufSize * (m_numAppOutBufs[0] + m_numAppOutBufs[1]);
    size += m_netInBufSize  * m_numNetInBufs;
    size += m_netOutBufSize * (m_numNetOutBufs[0] + m_numNetOutBufs[1]);

    return size;
}

int LtReadOnlyData::getLength() 
{
    return sizeof(lonTalk);
}

LtErrorType LtReadOnlyData::fromLonTalk(int offset, int length, byte* pData)
{
	LtErrorType err = LT_NO_ERROR;
	if (offset+length <= (int)sizeof(lonTalk))
	{
		memcpy(&lonTalk[offset], pData, length);
		setProgramId();
		setUniqueId();
        setBuffers();
	}
	else
	{
		err = LT_INVALID_PARAMETER;
	}
	return err;
}

LtErrorType LtReadOnlyData::toLonTalk(int offset, int length, byte** ppData) 
{
	LtErrorType err = LT_NO_ERROR;
    byte* data = new byte[length];
    if (offset+length <= (int)sizeof(lonTalk)) 
	{
        if (offset <= LT_ROD_STATE_OFFSET && LT_ROD_STATE_OFFSET < (offset+length))
        {
            // Make sure state byte is up-to-date.
            if (m_pNetworkImage == NULL)
            {
		        err = LT_INVALID_PARAMETER;
            }
            else
            {
                int state = m_pNetworkImage->getState();

                lonTalk[LT_ROD_STATE_OFFSET] &= (byte) 0xf8;
                lonTalk[LT_ROD_STATE_OFFSET] |= (byte)(state&0x7);
            }
        }

        if (err == LT_NO_ERROR)
        {
            memcpy(data, &lonTalk[offset], length);
        }
	} 
	else 
	{
		err = LT_INVALID_PARAMETER;
    }
	*ppData = data;
    return err;
}

LtErrorType LtReadOnlyData::getEncodedSize(byte size, byte &encodedValue)
{
    LtErrorType sts = LT_INVALID_PARAMETER;
    for (byte i = 0; i < sizeof(sizeTable) && sts != LT_NO_ERROR; i++)
    {
        if (sizeTable[i] == size)
        {
            sts = LT_NO_ERROR;
            encodedValue = i;
        }
    }
    return sts;
}

LtErrorType LtReadOnlyData::getEncodedCount(byte count, byte &encodedValue)
{
    LtErrorType sts = LT_INVALID_PARAMETER;
    for (byte i = 0; i < sizeof(countTable) && sts != LT_NO_ERROR; i++)
    {
        if (countTable[i] == count)
        {
            sts = LT_NO_ERROR;
            encodedValue = i;
        }
    }
    return sts;
}

/* Return the largest count that can be encoded that is <= count */
int LtReadOnlyData::getApproximateCount(byte count)
{
    byte i;
    for (i = 1; i < sizeof(countTable) && countTable[i] <= count; i++);

    return countTable[i-1];
}


LtErrorType LtReadOnlyData::setLonTalkBuffers(void)
{
    LtErrorType sts = LT_INVALID_PARAMETER;

    byte encodedNetOutBufSize;
    byte encodedNetInBufSize;
    byte encodedAppOutBufSize;
    byte encodedAppInBufSize;

    byte encodedNumAppOutBufs[2];
    byte encodedNumAppInBufs;
    byte encodedNumNetOutBufs[2];
    byte encodedNumNetInBufs;

    if (getEncodedSize(m_netOutBufSize,          encodedNetOutBufSize) == LT_NO_ERROR && 
        getEncodedSize(m_netInBufSize,           encodedNetInBufSize) == LT_NO_ERROR && 
        getEncodedSize(m_appOutBufSize,          encodedAppOutBufSize) == LT_NO_ERROR && 
        getEncodedSize(m_appInBufSize,           encodedAppInBufSize) == LT_NO_ERROR && 
        getEncodedCount(m_numAppOutBufs[0],      encodedNumAppOutBufs[0]) == LT_NO_ERROR && 
        getEncodedCount(m_numAppOutBufs[1],      encodedNumAppOutBufs[1]) == LT_NO_ERROR && 
        getEncodedCount(m_numAppInBufs,          encodedNumAppInBufs) == LT_NO_ERROR &&
        getEncodedCount(m_numNetOutBufs[0],      encodedNumNetOutBufs[0]) == LT_NO_ERROR && 
        getEncodedCount(m_numNetOutBufs[1],      encodedNumNetOutBufs[1]) == LT_NO_ERROR && 
        getEncodedCount(m_numNetInBufs,          encodedNumNetInBufs) == LT_NO_ERROR)
    {
        byte *pLonTalk = &lonTalk[LT_READ_ONLY_DATA_BUFFER_OFFSET];
        *pLonTalk++ = (encodedAppOutBufSize << 4)    | encodedAppInBufSize;
        *pLonTalk++ = (encodedNetOutBufSize << 4)    | encodedNetInBufSize;
        *pLonTalk++ = (encodedNumNetOutBufs[1] << 4) | encodedNumAppOutBufs[1];
        *pLonTalk++ = (encodedNumAppOutBufs[0] << 4) | encodedNumAppInBufs;
        *pLonTalk++ = (encodedNumNetOutBufs[0] << 4) | encodedNumNetInBufs;
        
        setPendingUpdate(true);

        sts = LT_NO_ERROR;
    }     
    return sts;
}

LtErrorType LtReadOnlyData::setNetworkInputBuffers(byte netInBufSize, byte numNetInBufs)
{
    LtErrorType sts;

    m_netInBufSize = netInBufSize;
    m_numNetInBufs = numNetInBufs;

    sts = setLonTalkBuffers();
    if (sts != LT_NO_ERROR)
    {   // Restore buffers from LONTalk
        setBuffers();
    };
    return sts;
}

LtErrorType LtReadOnlyData::setNetworkOutputBuffers(byte netOutBufSize, 
                                                    byte numNetOutBufs, 
                                                    byte numNetPriorityOutBufs) 
{
    LtErrorType sts;

    m_netOutBufSize = netOutBufSize;
    m_numNetOutBufs[0] = numNetOutBufs;
    m_numNetOutBufs[1] = numNetPriorityOutBufs;
    sts = setLonTalkBuffers();
    if (sts != LT_NO_ERROR)
    {   // Restore buffers from LONTalk
        setBuffers();
    };
    return sts;
}


LtErrorType LtReadOnlyData::setAppInputBuffers(byte appInBufSize, byte numAppInBufs)
{
    LtErrorType sts;

    m_appInBufSize = appInBufSize;
    m_numAppInBufs = numAppInBufs;

    sts = setLonTalkBuffers();
    if (sts != LT_NO_ERROR)
    {   // Restore buffers from LONTalk
        setBuffers();
    };
    return sts;
}

LtErrorType LtReadOnlyData::setAppOutputBuffers(byte appOutBufSize, 
                                                byte numAppOutBufs, 
                                                byte numAppPriorityOutBufs) 
{
    LtErrorType sts;

    m_appOutBufSize = appOutBufSize;
    m_numAppOutBufs[0] = numAppOutBufs;
    m_numAppOutBufs[1] = numAppPriorityOutBufs;
    sts = setLonTalkBuffers();
    if (sts != LT_NO_ERROR)
    {   // Restore buffers from LONTalk
        setBuffers();
    };
    return sts;
}
