//	IsConn.c	implementing IsiIsConnected and IsiIsAutomaticallyEnrolled
//
// Copyright © 2005-2022 Dialog Semiconductor
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
//	Revision 0, February 2005, Bernd Gauweiler

#include "isi_int.h"
#include <stddef.h>

/*
 * Function: IsiIsConnected
 * Returns the status of the specified assembly whether is currently enrolled in a connection or not. 
 *
 * Parameters:
 * assembly - assembly for which the connection status needs to be queried.
 *
 * Returns:
 * True - if the specified assembly is enrolled in a connection.
 * False - if the ISI engine is stopped.
 *
 * Remarks:
 * The function operates whether the ISI engine is running or not. 
 */
LonBool IsiIsConnected(unsigned Assembly)
{
	unsigned Index;
	IsiConnection ConnectionData;

	for(Index=0; _IsiNextConnection(Index, &ConnectionData); ++Index)
    {
        if (LON_GET_ATTRIBUTE(ConnectionData,ISI_CONN_STATE) >= isiConnectionStateInUse &&
            (ConnectionData.Host == Assembly || ConnectionData.Member == Assembly))
        {
            return TRUE;
        }
	}
	return FALSE;
}

/*
 * Function: IsiIsAutomaticallyEnrolled
 * Returns the status of the specified assembly whether is currently connected and if that
 * connection was made with automatic enrollment.
 *
 * Parameters:
 * assembly - assembly for which the connection status needs to be queried.
 *
 * Returns:
 * True - if the specified assembly is enrolled in a connection and the connection was made with 
 *        automatic enrollment.
 * False - if the ISI engine is stopped or if the connection was not made with automatic enrollment.
 *
 * Remarks:
 * The function operates whether the ISI engine is running or not. 
 */
LonBool IsiIsAutomaticallyEnrolled(unsigned Assembly)
{
	unsigned Index;
	IsiConnection ConnectionData;

	for(Index=0; _IsiNextConnection(Index, &ConnectionData); ++Index)
    {
        // Finds out if the assembly is enrolled in a connection and the connection was made
        // with automatic enrollment. We'd be looking for a connection table record with offset
		// zero though; higher order records are irrelevant (and can't appear in automatic
		// connections - see rcvcsmo.c for details about that).
        if (LON_GET_ATTRIBUTE(ConnectionData,ISI_CONN_STATE) >= isiConnectionStateInUse &&
            (ConnectionData.Host == Assembly || ConnectionData.Member == Assembly) &&
            ConnectionData.Desc.OffsetAuto == ConnectionAuto_MASK)
        {
        	return TRUE;
        }
	}
	return FALSE;
}    

//	end of IsConn.c
