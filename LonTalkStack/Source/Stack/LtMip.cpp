//
// LtMip.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtMip.cpp#2 $
//

#ifdef WIN32
#include <windows.h>
#endif
#include "LtRouter.h"
#include "LtMip.h"
#include "vxlTarget.h"  // For vxlReportEvent

#ifdef WIN32
// LonTalk network driver on Windows
extern "C" {
#include <tickLib.h>
#include <ldv.h>
}
#endif

//
// This file contains the support for determination of the type of MIP used
// in the system.
//

void reportDeterminMipTypeStatus(boolean done, ULONG &reportTime, ULONG openStartTime, int writeFailures)
{
    if (!done && ((tickGet() - reportTime) > 1000))
    {
        vxlReportEvent("Trying to determine mip type, writeFailures=%d, time=%d Msec\n", 
                       writeFailures, ((tickGet()-openStartTime)*sysClkRateGet()/1000));
        reportTime = tickGet();
    }
}

LtErrorType determineMipType(class LtLtLogicalChannel *pChannel, int &minLayer, int &maxLayer, boolean &bNsaMip, int &xcvrId, int &ldvHandle)
{
#if !FEATURE_INCLUDED(L5MIP) 
	// Only a layer2 interface is supported.
	minLayer = 2;
	maxLayer = 2;
	bNsaMip = false;
	xcvrId = -1;
	return LT_NO_ERROR;
#else
	LtErrorType err = LT_NO_ERROR;
	short handle = -1;
	minLayer = 2;
	maxLayer = 0;
	bNsaMip = false;
    boolean opened = false;
    HANDLE ldvEventHwnd;
    int    ldvEventTag;
    boolean determineMaxLayer = true;
    boolean determineXid = true;
    boolean determineNSA = true;
    boolean turnaround = false;
    LDVCode ldvSts = LDV_OK;

#if !PRODUCT_IS(ILON)
    LDVDeviceInfo updatedDeviceInfo;
    LDVDeviceInfoPtr pDeviceInfo = NULL;
#else
	boolean openFailed = false;
#endif
    
	// Enable LDV32 operations in this process.
	ldv_enable(0x3D85D2);

    // Get LDVX event registration information to be passed to the open routine.  
    pChannel->getLdvEventRegistrationParameters(ldvEventHwnd, ldvEventTag);

	// The upper limit is the hard part.  We either have a VNI MIP or a legacy MIP.
	// To determine this, we need to open the MIP and read its product info.
#if PRODUCT_IS(ILON)
    // ILON simulation uses old LDV32.dll, which does not support ldvx_open...
	if (ldv_open(pChannel->getName(), &handle) != LDV_OK)
	{
		err = LT_CANT_OPEN_PORT;
		openFailed = true;
	}
#else
	const char *pDeviceName = pChannel->getName();
    if (pDeviceName == NULL || *pDeviceName == 0)
    {   // No device specified - this is a simulated channel. 
        turnaround = true;
        maxLayer = 2;
        bNsaMip = true;
    }
    else
    {
        LDVDeviceCaps deviceCaps = LDV_DEVCAP_SICB;
        if (ldv_get_device_info(pDeviceName, &pDeviceInfo) == LDV_OK)
        {

            memcpy(&updatedDeviceInfo, pDeviceInfo, sizeof(updatedDeviceInfo));
            updatedDeviceInfo.size = sizeof(updatedDeviceInfo);

            maxLayer = 0;
            if (pDeviceInfo->caps & (LDV_DEVCAP_CURRENTLY_L2|LDV_DEVCAP_SWITCHABLE))
            {   
                maxLayer = 2;
                determineMaxLayer = false;
                determineNSA = false;
                bNsaMip = true;
                deviceCaps |= LDV_DEVCAP_L2;
                if (pDeviceInfo->caps & LDV_DEVCAP_PA)
                {
                    deviceCaps |= LDV_DEVCAP_PA;
                }
            }
            else if (!(pDeviceInfo->caps & LDV_DEVCAP_L2) && (pDeviceInfo->capsMask & LDV_DEVCAP_L2))
            {   // Does not support L2.  Open as a layer 5
                if (pDeviceInfo->capsMask & LDV_DEVCAP_L5 && 
                    !(pDeviceInfo->caps & LDV_DEVCAP_L5))
                {   // doesnt support layer 5 either - bad news...
                    err = LT_CANT_OPEN_PORT;
                }
                else
                {
		            maxLayer = 5;
                    determineMaxLayer = false;
                    deviceCaps |= LDV_DEVCAP_L5;
                }
            }

            if (pDeviceInfo->transId != (byte)-1)
            {
                determineXid = false;
                xcvrId = pDeviceInfo->transId;
            }
        }
        if (err == LT_NO_ERROR)
        {
            ldvSts = ldv_open_cap(pDeviceName, &handle, deviceCaps, (HWND)ldvEventHwnd, ldvEventTag);
            if (ldvSts == LDV_DEVICE_IN_USE)
            {
                err = LT_NI_IN_USE;
            }
            else if (ldvSts != LDV_OK)
	        {
		        err = LT_CANT_OPEN_PORT;
	        }
        }
    }
#endif
    if (err == LT_NO_ERROR)
    {
        opened = true;

        if (turnaround)
        {
            vxlReportEvent("Open Turnaround\n");
        }
        else if (determineMaxLayer || determineXid || determineNSA)
        {   // Need one of these pecies of information, 
            // so we might as well refresh all of it (since we can get it all with one message)
            
		    char buf[19];
            LtLocal flushCancelCmd;

            vxlReportEvent("Query %s%s%s\n", 
                            determineMaxLayer ? "Mip Type " : "",
                            determineXid ? "XID " : "",
                            determineNSA ? "NSA" : "");

            memset(&flushCancelCmd, 0, sizeof(flushCancelCmd));
            flushCancelCmd.cmd = MI_FLUSH_CANCEL;

            memset(buf, 0, sizeof(buf));

		    minLayer = 2;
		    maxLayer = 5;

            ULONG openTimeout;
            ULONG responseTimeout;
            int openRetries;

            {
                int _openTimeout;
                int _responseTimeout;
                pChannel->getInterfaceTimeouts(_openTimeout, _responseTimeout, openRetries);
                openTimeout = _openTimeout;
                responseTimeout = _responseTimeout;
            }

		    // First, send down a product query.
		    // Construct request in such a way as to work in either explicit address on
		    // or off mode.
		    buf[0] = 0x22;
		    buf[1] = 0x11;
		    buf[2] = 0x70; 
		    buf[3] = 0x00;
		    buf[4] = 0x03;
		    buf[5] = 0x7d;
		    buf[6] = 0x01;
		    buf[7] = 0x01;
		    buf[16]= 0x7d;
		    buf[17]= 0x01;
		    buf[18]= 0x01;

		    boolean bResponse = false;
            boolean bConnectionEstablished = false;
            ULONG openStartTime = tickGet();
            ULONG reportTime = tickGet();
            int writeFailures = 0;

            do
            {
                // Send down flush cancel command first, just in case the device had been left in
                // the flush state.
                ldvSts = ldv_write(handle, &flushCancelCmd, sizeof(flushCancelCmd) -1);
                if (ldvSts == LDV_OK)
                {
                    ldvSts = ldv_write(handle, buf, sizeof(buf));
                }
                if (ldvSts == LDV_OK)              
                {
                    ULONG messageSentTime = tickGet();
                    do
                    {
				        char buf[256];
				        Sleep(50);
				        // Read in all the messages we can looking for the response.  If they are coming
				        // in faster than we can read them, eventually we'll give up.
				        for (int j=0; 
					         !bResponse && j<100 && ldvSts == LDV_OK;
					         j++)					
				        {
                            ldvSts = ldv_read(handle, buf, sizeof(buf));
                            if (ldvSts == LDV_DEVICE_IN_USE)
                            {
                                err = LT_NI_IN_USE;
                            }
                            else if (ldvSts == LDV_OK)
                            {
                                pChannel->connectionEstablished();  // Must have made a connection...
                                bConnectionEstablished = true;
					            if (buf[0] == 0x16)
					            {
	                                int bOffset = 5;
						            if (buf[bOffset] != 0x3d)
						            {
							            bOffset += 11;
						            }
						            if (buf[bOffset] == 0x3d)
						            {
							            // Got the response
							            if (buf[bOffset + 1] == 2 && buf[bOffset + 2] == 3)
							            {
								            // Layer 2 MIP - max layer is 2.
    #if !PRODUCT_IS(ILON)
                                            updatedDeviceInfo.capsMask |= LDV_DEVCAP_L2;
                                            updatedDeviceInfo.caps |= LDV_DEVCAP_L2;
    #endif
								            maxLayer = 2;
                                        }
                                        else
                                        {   // Its a layer 5 mip
    #if !PRODUCT_IS(ILON)
                                            updatedDeviceInfo.capsMask |= LDV_DEVCAP_L5;
                                            updatedDeviceInfo.caps |= LDV_DEVCAP_L5;
    #endif
							                if (buf[bOffset + 1] == 3)
							                {
								                // NSA MIP!
								                bNsaMip = true;
                                            }
							            }

							            xcvrId = buf[bOffset + 5];
                                        bResponse = true;
    #if !PRODUCT_IS(ILON)
                                        updatedDeviceInfo.transId = xcvrId;
                                        if (pDeviceInfo != NULL)
                                        {
                                            if (updatedDeviceInfo.caps != pDeviceInfo->caps ||
                                                updatedDeviceInfo.capsMask != pDeviceInfo->capsMask ||
                                                updatedDeviceInfo.transId != pDeviceInfo->transId)
                                            {
                                                ldv_set_device_info(pDeviceName, &updatedDeviceInfo);
                                            }
                                        }
    #endif        
						            }
					            }
                            }
				        }
                        reportDeterminMipTypeStatus(bResponse, reportTime, openStartTime, writeFailures);

                    } while (!bResponse && 
                             (tickGet() - messageSentTime) < responseTimeout && 
                             !pChannel->getConnectionTerminated());
                }
                else if (ldvSts == LDV_DEVICE_IN_USE)
                {
                    err = LT_NI_IN_USE;
                }
                else
                {
                    ++writeFailures;
                    Sleep(50);
                }
                reportDeterminMipTypeStatus(bResponse, reportTime, openStartTime, writeFailures);
            } while (err == LT_NO_ERROR && 
                     !bResponse && 
                     (tickGet() - openStartTime) < openTimeout && 
                     !pChannel->getConnectionTerminated());

            if (err == LT_NO_ERROR && !bConnectionEstablished)
            {   // If we couldn't even read any kind of message from the interface, assume 
                // that there is no connection.
                err = LT_CANT_OPEN_PORT;    
            }

	    }        
        else 
        {
            vxlReportEvent("Using LDV capability information: MipType = layer %d, xid = %d, NsaMip = %s\n",
                maxLayer, xcvrId, bNsaMip ? "true" : "false");
        }
    }
#if !PRODUCT_IS(ILON)
    if (pDeviceInfo != NULL) 
    {
        ldv_free_device_info(pDeviceInfo);
    }
#endif
    if (opened)
    {
		if (err != LT_NO_ERROR)
		{
			ldv_close(handle);
		}
		else
		{
			ldvHandle = handle;
		}
    }
#if PRODUCT_IS(ILON) && defined(WIN32)
	if (err != LT_NO_ERROR)
	{
		if (openFailed)
			vxlPrintf("\nCan't open port \"%s\"!\n", pChannel->getName());
		else
			vxlPrintf("\nCan't determine the MIP type of port \"%s\"!\n", pChannel->getName());
		vxlPrintf("The port might be in use by another application.\n");
		vxlPrintf("Remember that the iLON simulation requires the old ldv32.dll (no port sharing).\n\n");
        Sleep(500);
	}
	else if (maxLayer == 5)
	{
		vxlPrintf("\nPort \"%s\" may be an L5 MIP, and the simulation requires an L2 MIP!\n\n", pChannel->getName());
	}		 
#endif

	return err;
#endif
}

