// IsiVectorHandlers .cpp
// Registration and implementation of the callbacks vectors 
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

#include <stdio.h>
#include <string.h>
#include "isi_int.h"

#ifdef WIN32
#include <memory.h>
#endif

// all the callback vectors
IsiCallbackVectors theIsiCallbackVectors;

#ifndef MAX_PATH
#define MAX_PATH 260
#endif


/*
 * ******************************************************************************
 * SECTION: Support functions
 * ******************************************************************************
 *
 * This section contains the support functions used to support the NVD vector handlers functions
 * defined in this file.
 */

/*
 *  Function: IsiDeregisterAllCallbacks
 *  Deregister all callbackGet functions.   
 *  It is not an error to deregister a callback twice, but only an
 *  unclaimed callback can be registered.

 *  Returns:
 *  void.
 * 
 *  Remarks:
 *  Returns the IP address and port used for an IP852 interface.  
 */
ISI_EXTERNAL_FN void IsiDeregisterAllCallbacks(void)
{
    _IsiAPIDebug("IsiDeregisterAllCallbacks\n");
    memset((void *)&theIsiCallbackVectors, 0, sizeof(theIsiCallbackVectors));
    _IsiAPIDebug("IsiDeregisterAllCallbacks - %d\n", IsiApiNoError);
}

/*
 *  Callback: IsiCreateCsmo
 *
 * Remarks:
 *
 */
void IsiCreateCsmo(unsigned Assembly, IsiCsmoData* const pCsmoData)
{
    if (theIsiCallbackVectors.createCsmo) 
    {
        try
        {
            ISI_CALLBACK_EXEC("IsiCreateCsmo");
            _IsiAPIDebug("IsiCreateCsmo OUT %d: %p\n", Assembly, pCsmoData);
            theIsiCallbackVectors.createCsmo(Assembly, pCsmoData);
            _IsiAPIDebug("IsiCreateCsmo IN  %d: %p: ", Assembly, pCsmoData);
            _IsiAPIDump("0x", pCsmoData, sizeof(IsiCsmoData), "\n");
        }
        catch (...)
        {
            ISI_CALLBACK_EXCEPTION("IsiCreateCsmo");
        }
    }
    else
    {
        ISI_CALLBACK_NOT_REGISTERED_DEF("IsiCreateCsmo");
        isiCreateCsmo(Assembly, pCsmoData);
    }
}

ISI_EXTERNAL_FN const IsiApiError
IsiCreateCsmoRegistrar(IsiCreateCsmoFunction handler)
{
    IsiApiError sts = IsiApiNoError;
    theIsiCallbackVectors.createCsmo = handler;
    ISI_CALLBACK_REGISTER("IsiCreateCsmo", sts);
    return sts;
}

/*
 *  Callback: IsiCreatePeriodicMsg
 *
 * Remarks:
 *
 */
LonBool IsiCreatePeriodicMsg(void)
{
    if (theIsiCallbackVectors.createPeriodicMsg) 
    {
        try
        {
            ISI_CALLBACK_EXEC("IsiCreatePeriodicMsg");
            return theIsiCallbackVectors.createPeriodicMsg();
        }
        catch (...)
        {
            ISI_CALLBACK_EXCEPTION("IsiCreatePeriodicMsg");
            return FALSE;
        }
    }
    else
    {
        ISI_CALLBACK_NOT_REGISTERED_DEF("IsiCreatePeriodicMsg");
        return isiCreatePeriodicMsg();
    }
}

ISI_EXTERNAL_FN const IsiApiError
IsiCreatePeriodicMsgRegistrar(IsiCreatePeriodicMsgFunction handler)
{
    IsiApiError sts = IsiApiNoError;
    theIsiCallbackVectors.createPeriodicMsg = handler;
    ISI_CALLBACK_REGISTER("IsiCreatePeriodicMsg", sts);
    return sts;
}

/*
 *  Callback: IsiGetAssembly
 *
 * Remarks:
 *
 */
unsigned IsiGetAssembly(const IsiCsmoData* pCsmoData, LonBool Auto, unsigned Assembly)
{
    if (theIsiCallbackVectors.getAssembly) 
    {
        try
        {
            _IsiAPIDebug(
				"IsiGetAssembly: executing registered callback, assembly %u, auto %s ",
				Assembly,
				Auto ? "Yes" : "No"
            );
            _IsiAPIDump("csmo 0x", (void *)pCsmoData, sizeof(IsiCsmoData), "\n");
            return theIsiCallbackVectors.getAssembly(pCsmoData, Auto, Assembly);
        }
        catch (...)
        {
            ISI_CALLBACK_EXCEPTION("IsiGetAssembly");
            return ISI_NO_ASSEMBLY;
        }
    }
    else
    {
        ISI_CALLBACK_NOT_REGISTERED_DEF("IsiGetAssembly");
        return isiGetAssembly(pCsmoData, Auto, Assembly);
    }
}

ISI_EXTERNAL_FN const IsiApiError
IsiGetAssemblyRegistrar(IsiGetAssemblyFunction handler)
{
    IsiApiError sts = IsiApiNoError;
    theIsiCallbackVectors.getAssembly = handler;
    ISI_CALLBACK_REGISTER("IsiGetAssembly", sts);
    return sts;
}

/*
 *  Callback: IsiGetNvIndex
 *
 * Remarks:
 *
 */
unsigned IsiGetNvIndex(unsigned Assembly, unsigned Offset, unsigned PreviousIndex)
{
    if (theIsiCallbackVectors.getNvIndex) 
    {
        try
        {
        	_IsiAPIDebug(
				"IsiGetNvIndex: executing registered callback, assembly %u, offset %u, prev %u\n",
				Assembly,
				Offset,
				PreviousIndex
        	);
            return theIsiCallbackVectors.getNvIndex(Assembly, Offset, PreviousIndex);
        }
        catch (...)
        {
            ISI_CALLBACK_EXCEPTION("IsiGetNvIndex");
            return ISI_NO_INDEX; 
        }
    }
    else
    {
        ISI_CALLBACK_NOT_REGISTERED_DEF("IsiGetAssembly");
        return isiGetNvIndex(Assembly, Offset, PreviousIndex);
    }
}

ISI_EXTERNAL_FN const IsiApiError
IsiGetNvIndexRegistrar(IsiGetNvIndexFunction handler)
{
    IsiApiError sts = IsiApiNoError;
    theIsiCallbackVectors.getNvIndex = handler;
    ISI_CALLBACK_REGISTER("IsiGetNvIndex", sts);
    return sts;
}

/*
 *  Callback: IsiGetPrimaryGroup
 *
 * Remarks:
 *
 */
unsigned IsiGetPrimaryGroup(unsigned Assembly)
{
    if (theIsiCallbackVectors.getPrimaryGroup) 
    {
        try
        {
        	_IsiAPIDebug(
				"IsiGetPrimaryGroup: executing registered callback, assembly %u\n",
				Assembly
        	);
            return theIsiCallbackVectors.getPrimaryGroup(Assembly);
        }
        catch (...)
        {
            ISI_CALLBACK_EXCEPTION("IsiGetPrimaryGroup");
            return ISI_DEFAULT_GROUP;
        }
    }
    else
    {
        ISI_CALLBACK_NOT_REGISTERED_DEF("IsiGetPrimaryGroup");
        return isiGetPrimaryGroup(Assembly);
    }
}

ISI_EXTERNAL_FN const IsiApiError
IsiGetPrimaryGroupRegistrar(IsiGetPrimaryGroupFunction handler)
{
    IsiApiError sts = IsiApiNoError;
    theIsiCallbackVectors.getPrimaryGroup = handler;
    ISI_CALLBACK_REGISTER("IsiGetPrimaryGroup", sts);
    return sts;
}

/*
 *  Callback: IsiGetWidth
 *
 * Remarks:
 *
 */
unsigned IsiGetWidth(unsigned Assembly)
{
    if (theIsiCallbackVectors.getWidth) 
    {
        try
        {
        	_IsiAPIDebug(
        		"IsiGetWidth: executing registered callback, assembly %u\n",
        		Assembly
        	);
            return theIsiCallbackVectors.getWidth(Assembly);
        }
        catch (...)
        {
            ISI_CALLBACK_EXCEPTION("IsiGetWidth");
            return 1u;
        }
    }
    else
    {
        ISI_CALLBACK_NOT_REGISTERED_DEF("IsiGetWidth");
        return isiGetWidth(Assembly);
    }
}

ISI_EXTERNAL_FN const IsiApiError
IsiGetWidthRegistrar(IsiGetWidthFunction handler)
{
    IsiApiError sts = IsiApiNoError;
    theIsiCallbackVectors.getWidth = handler;
    ISI_CALLBACK_REGISTER("IsiGetWidth", sts);
    return sts;
}

/*
 *  Callback: IsiQueryHeartbeat
 *
 * Remarks:
 *
 */
LonBool IsiQueryHeartbeat(unsigned NvIndex)
{
    if (theIsiCallbackVectors.queryHeartbeat) 
    {
        try
        {
            ISI_CALLBACK_EXEC("IsiQueryHeartbeat");
            return theIsiCallbackVectors.queryHeartbeat(NvIndex);
        }
        catch (...)
        {
            ISI_CALLBACK_EXCEPTION("IsiQueryHeartbeat");
            return FALSE;
        }
    }
    else
    {
        ISI_CALLBACK_NOT_REGISTERED_DEF("IsiQueryHeartbeat");
        return isiQueryHeartbeat(NvIndex);
    }
}


ISI_EXTERNAL_FN const IsiApiError
IsiQueryHeartbeatRegistrar(IsiQueryHeartbeatFunction handler)
{
    IsiApiError sts = IsiApiNoError;
    theIsiCallbackVectors.queryHeartbeat = handler;
    ISI_CALLBACK_REGISTER("IsiQueryHeartbeat", sts);
    return sts;
}

/*
 *  Callback: IsiUpdateDiagnostics
 *
 * Remarks:
 *
 */
void IsiUpdateDiagnostics(IsiDiagnostic Event, LonByte Parameter)
{
    if (theIsiCallbackVectors.updateDiagnostics) 
    {
        try
        {
            ISI_CALLBACK_EXEC("IsiUpdateDiagnostics");
            theIsiCallbackVectors.updateDiagnostics(Event, Parameter);
        }
        catch (...)
        {
            ISI_CALLBACK_EXCEPTION("IsiUpdateDiagnostics");
        }
    }
    else
    {
        ISI_CALLBACK_NOT_REGISTERED_DEF("IsiUpdateDiagnostics");
        isiUpdateDiagnostics(Event, Parameter);
    }
}

ISI_EXTERNAL_FN const IsiApiError
IsiUpdateDiagnosticsRegistrar(IsiUpdateDiagnosticsFunction handler)
{
    IsiApiError sts = IsiApiNoError;
    theIsiCallbackVectors.updateDiagnostics = handler;
    ISI_CALLBACK_REGISTER("IsiUpdateDiagnostics", sts);
    return sts;
}

/*
 *  Callback: IsiUpdateUserInterface
 *
 * Remarks:
 *
 */
void IsiUpdateUserInterface(IsiEvent Event, LonByte Parameter)
{
	_IsiAPIDebug(
		"IsiUpdateUserInterface: event %d, parameter %u\n",
		(int)Event,
		(unsigned)Parameter
	);

    if (theIsiCallbackVectors.updateUserInterface) 
    {
        try
        {
            ISI_CALLBACK_EXEC("IsiUpdateUserInterface");
            theIsiCallbackVectors.updateUserInterface(Event, Parameter);
        }
        catch (...)
        {
            ISI_CALLBACK_EXCEPTION("IsiUpdateUserInterface");
            isiUpdateUserInterface(Event, Parameter);       // call the default
        }
    }
    else
    {
        ISI_CALLBACK_NOT_REGISTERED_DEF("IsiUpdateUserInterface");
        isiUpdateUserInterface(Event, Parameter);
    }
}


ISI_EXTERNAL_FN const IsiApiError
IsiUpdateUserInterfaceRegistrar(IsiUpdateUserInterfaceFunction handler)
{
    IsiApiError sts = IsiApiNoError;
    theIsiCallbackVectors.updateUserInterface = handler;
    ISI_CALLBACK_REGISTER("IsiUpdateUserInterface", sts);
    return sts;
}
