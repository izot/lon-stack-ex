/***************************************************************
 *  Filename: LonLinkWin.cpp
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
 *  Description:  Implementation of the LonLinkWin class.
 *		This is the Windows-specific derivation of the LonLink class
 *
 ****************************************************************/


#include <windows.h>
#include <string.h>
#include <assert.h>

#include "LonTalk.h"
#include "LonLinkWin.h"
#include "LtMip.h"
#include "LtNetworkManagerLit.h"

// LonTalk network driver on Windows
extern "C" {
#include <ldv.h>
}


/*
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
*/

// Table of lon talk devices and whether we gathered them

#define MAXLONDEV 4
#define MAXLONNAMESIZE 32
static boolean	bEnumDevicesDone = false;
static CHAR	aszLonDevices[MAXLONDEV][MAXLONNAMESIZE];

//////////////////////////////////////////////////////////////////////
// Local Routines
//////////////////////////////////////////////////////////////////////

//
// EnumLonDevices
//
// Enumerate the local devices into a table from the registry once
// the first time we are asked for them.
//
static void	EnumLonDevices()
{
	HKEY	hkDDs;
	LONG	sts;
	int		i;

	if ( bEnumDevicesDone )
	{	return;
	}
	bEnumDevicesDone = true;
	// zero the list of devices
	for ( i=0; i< MAXLONDEV; i++ )
	{
		aszLonDevices[i][0] = 0;
	}

	sts = RegOpenKey( HKEY_LOCAL_MACHINE, "Software\\LonWorks\\DeviceDrivers",
					&hkDDs );
	if ( sts != ERROR_SUCCESS )
	{
		//MBfromError(sts);
		return;
	}

	// winerror.h

	// Obtain a new list from the registry
	for ( i=0, sts=ERROR_SUCCESS; (i<MAXLONDEV) && (sts == ERROR_SUCCESS); i++ )
	{
		sts = RegEnumKey(hkDDs, i, &aszLonDevices[i][0], MAXLONNAMESIZE);
	}

	if ( sts != ERROR_SUCCESS && sts != ERROR_NO_MORE_ITEMS )
	{	//MBfromError(sts);
	}
	RegCloseKey( hkDDs );
}

//
// Map Ldv status to LtSts
//
struct	LdvLtStsMap
{
	int		ldvsts;
	LtSts	ltsts;
};

static LdvLtStsMap  stsMap[] =
{
	{ LDV_OK, LTSTS_OK },
	{ LDV_NOT_FOUND, LTSTS_ERROR },
	{ LDV_ALREADY_OPEN, LTSTS_OPENFAILURE },
    { LDV_NOT_OPEN, LTSTS_INVALIDSTATE },
    { LDV_DEVICE_ERR, LTSTS_ERROR },
    { LDV_INVALID_DEVICE_ID, LTSTS_OPENFAILURE },
    { LDV_NO_MSG_AVAIL, LTSTS_ERROR },
    { LDV_NO_BUFF_AVAIL, LTSTS_QUEUEFULL },
    { LDV_NO_RESOURCES, LTSTS_ERROR },
    { LDV_INVALID_BUF_LEN, LTSTS_ERROR },
	{ -1, LTSTS_END }
};

LtSts LonLinkWin::MapLdv2LtStatus( int ldvSts )
{
	int		i=0;

	for (i =0; stsMap[i].ltsts > 0; i++ )
	{
		if ( stsMap[i].ldvsts == ldvSts )
		{
			return stsMap[i].ltsts;
		}
	}
	return LTSTS_ERROR;
}



// LonLinkWin class

LonLinkWin::LonLinkWin()
{
	m_hLDV = INVALID_LDV_CHAN;
    m_hPreOpenedLDV = INVALID_LDV_CHAN;
	m_wdTimer = NULL;
	m_hReceiveEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
}

LonLinkWin::~LonLinkWin()
{
	stopReceiveTask();

	if ( m_hReceiveEvent )
	{	CloseHandle( (HANDLE)m_hReceiveEvent );
		m_hReceiveEvent = NULL;
	}

	if ( m_wdTimer )
	{
		wdCancel( m_wdTimer );
		wdDelete( m_wdTimer );
		m_wdTimer = NULL;
	}
}

void
LonLinkWin::sendServicePinMessageAll()
{
	if (m_pNet)
	{
		m_pNet->servicePinDepressed();
	}
}

extern "C"
STATUS sendServicePinMessageAll()
{
	LonLinkWin* ptr = LonLinkWin::getInstance();
	if (ptr)
	{
		ptr->sendServicePinMessageAll();
		return(OK);
	}
	return(ERROR);
}

LonLinkWin* LonLinkWin::_instance = NULL;
LonLinkWin* LonLinkWin::getInstance()
{
	return _instance;
}

LtLink* LonLinkWin::createInstance()
{
	_instance =  new LonLinkWin();
	return (LtLink*) _instance;
}

//
// enumInterfaces
//
// Enumerate Interfaces supported by this link
// idx values start with zero. For each valid interface the name
// is returned in the buffer and true is returned.
// False is returned if there is no interface with that index.
//
boolean LonLinkWin::enumInterfaces( int idx, LPSTR pNameRtn, int nMaxSize )
{
	boolean		bOk = false;

	// Load up our static list if we haven't already
	EnumLonDevices();

	if ( idx < MAXLONDEV && strlen(&aszLonDevices[idx][0]) &&
		nMaxSize >= MAXLONNAMESIZE )
	{
		strcpy(pNameRtn, &aszLonDevices[idx][0] );
		bOk = true;
	}
	else if ( idx > 0 && idx < MAXLONDEV && strlen(&aszLonDevices[idx-1][0]) &&
		nMaxSize >= MAXLONNAMESIZE )
	{
		// always add a blank line at the end of the list
		strcpy( pNameRtn, "" );
		bOk = true;
	}
	else if ( idx == 0 )
	{	// we don't have any devices here
		// let's just do something so we can test
		strcpy( pNameRtn, "" );
		bOk = true;
	}

	return bOk;
}


LtSts LonLinkWin::driverOpen(const char* pName )
{
	LtSts	sts = LTSTS_OPENFAILURE;
	int		rc;

#if PRODUCT_IS(ILON)
    // ILON simulation uses old LDV32.dll, which does not support ldvx_open...
	rc = ldv_open(pName, &m_hLDV);
#else
    if (m_hPreOpenedLDV != INVALID_LDV_CHAN)
    {   // already opened - OK...
        m_hLDV = m_hPreOpenedLDV;
        m_hPreOpenedLDV = INVALID_LDV_CHAN;
        rc = LDV_OK;
    }
    else
    {
#if !FEATURE_INCLUDED(L5MIP) 
        rc = ldv_open_cap(pName, &m_hLDV, LDV_DEVCAP_L2, NULL, 0);
#else
        rc = ldv_open(pName, &m_hLDV);
#endif
    }
#endif

	if (rc == LDV_OK)
	{
		sts = LTSTS_OK;
	}
	return(sts);
}

void LonLinkWin::driverClose()
{
	ldv_close(m_hLDV);
	m_hLDV = INVALID_LDV_CHAN;
}

LtSts LonLinkWin::driverRead(void *pData, short len)
{
	LtSts	sts;
	int		rc;

	rc = ldv_read(m_hLDV, pData, len);
	sts = MapLdv2LtStatus(rc);
	return(sts);
}

LtSts LonLinkWin::driverWrite(void *pData, short len)
{
	LtSts	sts;
	int		rc;

	rc = ldv_write(m_hLDV, pData, len);
	sts = MapLdv2LtStatus(rc);
	return(sts);
}

LtSts LonLinkWin::driverRegisterEvent()
{
	LtSts	sts;
	int		rc;

	rc = ldv_register_event( m_hLDV, m_hReceiveEvent );
	sts = MapLdv2LtStatus(rc);
	return(sts);
}

void LonLinkWin::driverReceiveEvent()
{
	// For unknown reasons, we don't always get an event.  This
	// should be pursued at some point.  Anyway, for this reason
	// we use a 50 msec timeout rather than INFINITE one.

	// IMPORTANT - the receive task now does periodic clean-up of the PL31x0 transceiver
	// register.  So, if you change this back to infinite, you must come up with
	// an alternate scheme for doing this periodic clean-up (e.g., create a timer and another task).
	WaitForSingleObject( m_hReceiveEvent, 50 );
}

//
// lonLinkWinTimer
//
// Friendly timer task
//
static int lonLinkWinTimer( int a1 )
{
	LonLinkWin*	pLink = (LonLinkWin*) a1;
	pLink->transmitTimerRoutine();
	return 0;
}

//
// transmitTimerRoutine
//
// Try repeatedly to transmit something if we get a transmit full condition.
//
void	LonLinkWin::transmitTimerRoutine()
{
	LLPktQue*	pPkt;
	LtQue*		pItem;
	LtSts		sts = LTSTS_OK;

	lock();

	while ( m_qTransmit.removeHead( &pItem ) )
	{
		pPkt = (LLPktQue*)pItem;

		sts = tryTransmit( pPkt->m_refId, pPkt->m_pktFlags, pPkt->m_pData, pPkt->m_nDataLength );
		if ( sts == LTSTS_QUEUEFULL )
		{
			m_qTransmit.insertHead( pPkt );
			break;
		}
		else
		{
			// Done with this packet - return it to the owner.
			unlock();
			m_pNet->packetComplete( pPkt->m_refId, sts );
			lock();
		}

		freeLLPkt(pPkt);
	}

	m_bDelayedRetransmitPending = false;

	if ( sts == LTSTS_QUEUEFULL || !m_qTransmit.isEmpty() )
	{	startDelayedRetransmit();
	}

	unlock();
}

//
// startDelayedRetransmit
//
// Start the timer to try again on a transmit
//
void	LonLinkWin::startDelayedRetransmit()
{
	STATUS	vxSts;
	if ( ! m_bDelayedRetransmitPending )
	{
		if ( m_wdTimer == NULL )
		{
			m_wdTimer = wdCreate();
			assert( m_wdTimer != NULL );
		}
		vxSts = wdStart( m_wdTimer, 20, lonLinkWinTimer, (int)this );
		m_bDelayedRetransmitPending = true;
	}
}

void LonLinkWin::sendToProtocolAnalyser(byte *pData, bool crcIncluded)
{
	// This needs an implementation because it is pure virtual in LonLink
	// Add hook here if packet needs to be send to a protocol analyser data sink
}

void LonLinkWin::registerLdvHandle(int ldvHandle)
{
    m_hPreOpenedLDV = ldvHandle;
}

boolean LonLinkWin::getUniqueId(LtUniqueId& uniqueId)
{
	LtSts sts = LTSTS_OK;
	LtReadMemory rmem;
    byte uniqueIdValue[LT_UNIQUE_ID_LENGTH];
	int respLen = sizeof(uniqueIdValue);

	rmem.mode = LT_READ_ONLY_RELATIVE;
	rmem.addresshi = 0;
	rmem.addresslo = LT_ROD_UNIQUEID_OFFSET;
	rmem.len = LT_UNIQUE_ID_LENGTH;
	sts = localCommand(LT_READ_MEMORY, (byte*) &rmem, sizeof(rmem),
		               uniqueId.getData(), respLen);

    return sts == LTSTS_OK;
}
