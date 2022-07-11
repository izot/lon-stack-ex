/***************************************************************
 *  Filename: LonLinkWin.h
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
 ****************************************************************/

#if !defined(LONLINKWIN_H)
#define LONLINKWIN_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "LonLink.h"

extern "C"
STATUS sendServicePinMessageAll();

class LonLinkWin : public LonLink
{
public:
	friend int lonLinkWinTimer( int a1 );

	LonLinkWin();
	~LonLinkWin();

	static LtLink* createInstance();
	static LonLinkWin* getInstance();

	// Enumerate Interfaces supported by this link
	// idx values start with zero. For each valid interface the name
	// is returned in the buffer and true is returned.
	// False is returned if there is no interface with that index.
	boolean enumInterfaces( int idx, LPSTR pNameRtn, int nMaxSize );

    void sendServicePinMessageAll();

    void registerLdvHandle(int ldvHandle);

    boolean getUniqueId(LtUniqueId& uniqueId);

protected:
	enum{		INVALID_LDV_CHAN = -1 };


	virtual boolean		isOpen()
	{	return m_hLDV != INVALID_LDV_CHAN;
	}

	LtSts				MapLdv2LtStatus( int ldvSts );

static LonLinkWin *_instance;

	short				m_hLDV;				// "handle" to lon device

	// Platform-specific driver function overrides
	virtual LtSts driverOpen(const char* pName);
	virtual void driverClose();
	virtual LtSts driverRead(void *pData, short len);
	virtual LtSts driverWrite(void *pData, short len);
	virtual LtSts driverRegisterEvent();
	virtual void driverReceiveEvent();
	virtual void sendToProtocolAnalyser(byte* pData, bool crcIncluded = true);

	// Retransmit timer routines (Win32 specific)
	void transmitTimerRoutine();
	virtual void startDelayedRetransmit();
	
	WDOG_ID				m_wdTimer;			// timer for transmits

private:
    // The VniServer "pre-opens" the link to determine the mip type.  The
    // LDV handle is stored in m_hPreOpenedLDV.  Opening the driver then
    // uses this handle rather than opening LDV directly.
    short				m_hPreOpenedLDV;	
};


#endif	// LONLINKWIN_H
