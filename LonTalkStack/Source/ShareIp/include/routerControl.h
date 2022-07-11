#ifndef ROUTERCONTROL_H
#define ROUTERCONTROL_H

/***************************************************************
 *  Filename:routerControl.h
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
 *  Description:  target Control routines for Router
 *
 *	DJ Duffy Mar 1999
 * From /LTIPTest/IpBench.cpp 18    5/14/99 2:44p
 *
 ****************************************************************/

/*
 * $Log: /Dev/ShareIp/include/routerControl.h $
 * 
 * 21    7/21/05 10:27a Fremont
 * Remove some header dependencies
 * 
 * 20    11/05/04 3:40p Fremont
 * iLON 100 PPP router demo
 * 
 * 19    9/19/03 11:29a Fremont
 * removed rtrReportStatus, added param to rtrReportStatistics
 * 
 * 18    9/16/03 5:35p Fremont
 * add auth diagnostic
 * 
 * 17    7/28/03 2:01p Fremont
 * add rtrGetPersistIndex()
 * 
 * 16    7/22/03 4:24p Fremont
 * add rtrReportXcvrParams()
 * 
 * 15    7/08/03 2:34p Fremont
 * rename stats time cleared field and convert to regular time_t
 * 
 * 14    6/04/03 8:04p Iphan
 * IKP06042003: Fixed statistics shown in console & ConfigServer
 * IKP06042003: Support for LT & LTIP statistics in System Info
 * 
 * 13    3/27/03 2:33p Fremont
 * updated obsolete rtrSetConfigServer() to be useful
 * 
 * 12    3/20/03 11:51a Fremont
 * Allow setting startup values for IP addr and port and persist index
 * (for simulation)
 * 
 * 11    9/26/01 3:00p Fremont
 * change interface to rtrStartup()
 * 
 * 10    3/15/00 5:22p Darrelld
 * add rtrGetInit to obtain init object for testing
 * 
 * 9     3/13/00 5:28p Darrelld
 * Segmentation work
 * 
 * 8     2/25/00 5:46p Darrelld
 * Fixes for PCLIPS operation
 * 
 * 7     12/08/99 5:21p Glen
 * Support for vnistack.dll
 * 
 * 5     10/22/99 1:13p Darrelld
 * rtrstat command
 * 
 * 4     10/06/99 4:09p Darrelld
 * Reporting link statistics
 * 
 * 3     9/20/99 10:53a Bobw
 * Allow caller to specify LT channel name (for PC).
 * 
 * 2     8/05/99 6:02p Glen
 * Tweak entry points for LtStart's benefit
 * 
 * 1     7/01/99 5:50p Darrelld
 * Target specific files
 * 
 * 4     6/30/99 4:46p Darrelld
 * Add statistics
 * 
 * 2     6/22/99 4:59p Darrelld
 * Intermediate debugging of interface
 * 
 * 1     6/22/99 9:49a Darrelld
 * The Echelon Router running on a PC
 * 

 * 
 */

#include <LtaDefine.h>
#include <LonLink.h>

#ifdef __cplusplus
extern "C" 
{
#endif

// Declare these classes to decouple this file from other header files.
class LtIpMaster;
class LonLink;
class LtInit;

	void	rtrSetConfigServer( ULONG ipAddr, word port );
	void	rtrSetPersistIndex(int index);
	void	rtrGetPersistIndex(int *pIndex);
	void	rtrSetDefaultIpAddr(ULONG ipAddr);
	void	rtrSetDefaultIpPort(int port);
	void	rtrStartNoisy();
	// after calling one of rtrStartAll, rtrStartup, then you must call
	// rtrStartComplete to complete the startup process
	// REMINDER - you can't call rtrStartComplete after rtrStartAll (NULL ptr error)
	boolean LTA_EXTERNAL_FN rtrStartAll(LtInit* pInit, boolean bStartLonTalkRouter, const char* pLtPortName = null);
	// if the portname is null, the first port is selected from the available list.
	// if the portname is "", the empty string, then no port is used.
	boolean	LTA_EXTERNAL_FN rtrStartup( boolean bStartLonTalkRouter=TRUE, const char* szPortName= null );
	boolean	LTA_EXTERNAL_FN rtrStartComplete();
	boolean	LTA_EXTERNAL_FN rtrShutdown();
	void	rtrSetPktHandling( boolean bAgg, int nAggMs, 
								   boolean bBwLimit, int gnBwKB );
	void	LTA_EXTERNAL_FN rtrReportStatistics(boolean bAllClients = TRUE);
	void    LTA_EXTERNAL_FN rtrClearStats();
	void	rtrSetPacketDump( boolean bDump, boolean bHeadersOnly );
	void	rtrSendServicePin( );
	LtIpMaster*	rtrGetMaster();
	LonLink*	rtrGetLonLink();
	LtInit*		rtrGetInit();

	void	rtrReportXcvrParams();
	void	rtrShowAuthParams();
	void	rtrSetPaMode(boolean enable);

#ifdef __cplusplus
}
#endif

struct	rtrStats
{
	time_t	timeLastCleared;
	ULONG	nSecSinceLastClear;
	ULONG	nLtPktsRcv;
	ULONG	nLtPktsSnt;
	ULONG	nLtPktsLost;
	ULONG	nMsPktsRcv;
	ULONG	nMsPktsSnt;
	ULONG	nClPktsRcv;
	ULONG	nClPktsSnt;
	ULONG	nClPktsStale;

	rtrStats()
	{
		timeLastCleared  = 0;
		nSecSinceLastClear = 0;
		nLtPktsRcv   = 0;
		nLtPktsSnt   = 0;
		nLtPktsLost  = 0;
		nMsPktsRcv   = 0;
		nMsPktsSnt   = 0;
		nClPktsRcv   = 0;
		nClPktsSnt   = 0;
		nClPktsStale = 0;
	}
};
void	rtrGetStats( rtrStats& stats );

#endif // ROUTERCONTROL_H
