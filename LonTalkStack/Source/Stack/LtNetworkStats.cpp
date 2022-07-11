//
// LtNetworkStats.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtNetworkStats.cpp#1 $
//

#include "LtStackInternal.h"

//
// Private Member Functions
//


//
// Protected Member Functions
//


//
// Public Member Functions
//


LtNetworkStats::LtNetworkStats(LtLink* d, LtLreServer* pLre) {
	m_pLre = pLre;
    drv = d;
    reset();
}

void LtNetworkStats::bump(int index) {
    if (netStats[index] < 0xffff) {
        netStats[index]++;
    }
}

int LtNetworkStats::get(int index) {
    return netStats[index];
}

boolean LtNetworkStats::getEepromLock() {
    return (netStats[LT_EEPROM_LOCK] & 0xff00) != 0;
}

void LtNetworkStats::setEepromLock(boolean l) {
    netStats[LT_EEPROM_LOCK] = l ? 0xffff : 0;
}

void LtNetworkStats::reset() {
    for (int i=0; i<LT_NUM_STATS; i++) {
        netStats[i] = 0;
    }
	if (drv != null)
	{
		// If the stats are shadowed, clear only the shadowed part (leave real link stats alone)
		LtLinkStats *pStats;
		drv->getStatistics(pStats);
		if ((pStats != NULL) && (pStats->m_shadowed))
		{
			((LtLinkStatsShadow*)pStats)->clearShadowStats();
		}
		else
		{
			drv->clearStatistics();
		}
	}
}

LtErrorType LtNetworkStats::validate(int offset, int length) 
{
	LtErrorType err = LT_NO_ERROR;
    if ((offset+length)/2 > LT_NUM_STATS) 
	{
        err = LT_INVALID_PARAMETER;
    }
	return err;
}

#define CAPSTAT(stat) min(0xffff,stat)
void LtNetworkStats::refresh()
{
	// Refresh beginning portion of stats from driver.
	LtLinkStats *pLS;
	drv->getStatistics(pLS);
	if ((pLS != NULL) && (pLS->m_shadowed))
	{
		LtLinkStatsShadow *pShadow = static_cast<LtLinkStatsShadow *>(pLS);
		netStats[LT_TRANSMISSION_ERRORS] = CAPSTAT(pShadow->m_nTransmissionErrorsShadow);
		netStats[LT_MISSED_MESSAGES] = CAPSTAT(pShadow->m_nMissedPacketsShadow);
		netStats[LT_BACKLOG_OVERFLOW] = CAPSTAT(pShadow->m_nBacklogOverflowsShadow);
		netStats[LT_COLLISIONS] = CAPSTAT(pShadow->m_nCollisionsShadow);
	}
	else
	{
		LtLinkStats ls;
		drv->getStatistics(ls);
		netStats[LT_TRANSMISSION_ERRORS] = CAPSTAT(ls.m_nTransmissionErrors);
		netStats[LT_MISSED_MESSAGES] = CAPSTAT(ls.m_nMissedPackets);
		netStats[LT_BACKLOG_OVERFLOW] = CAPSTAT(ls.m_nBacklogOverflows);
		netStats[LT_COLLISIONS] = CAPSTAT(ls.m_nCollisions);
	}

}

LtErrorType LtNetworkStats::fromLonTalk(byte data[], int offset, int length) {
    LtErrorType err = validate(offset, length);
    
	if (err == LT_NO_ERROR)
	{
		for (int i = offset; i < offset + length; i++) {
			int value = netStats[i / 2];
			if ((i&1)==1) {
				value = (value & 0xff) | ((int) data[i] << 8);
			} else {
				value = (value & 0xff00) | (((int) data[i]) & 0xff);
			}
			netStats[i/2] = value & 0xffff;
		}  
	}
	return err;
}

LtErrorType LtNetworkStats::toLonTalk(byte** ppData, int offset, int length) {
    LtErrorType err = validate(offset, length);
	if (err == LT_NO_ERROR)
	{
		byte* data = new byte[length];

		refresh();

		// Note that the L2 number represents the total packets routed by the
		// engine not from us.  This includes packets routed on other channels!
		// To really meet the spirit of the LonTalk definition the counter should
		// be maintained by LonTalk channel.  This could be done but would require
		// that the engine on each packet bump a per channel counter up to once 
		// per packet per channel (even if multiple clients on a channel get the
		// packet).  This is non-trivial and not worth the cycles.
		int index = 0;
		for (int i = offset; i < offset + length; i++) {
			int value = netStats[i / 2];
			data[index++] = (byte) (((i & 1) == 1) ? value : (value >> 8));
		}  
		*ppData = data;
	}
    return err;
}
