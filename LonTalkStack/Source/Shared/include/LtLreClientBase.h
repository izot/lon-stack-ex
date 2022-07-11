/***************************************************************
 *  Filename: LtLreClientBase.h
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
 *  Description:  Header file for base LtLre client.
 *
 *	DJ Duffy Oct 1998
 *
 ****************************************************************/

/*
 * $Log: /Dev/Shared/include/LtLreClientBase.h $
 * 
 * 6     11/01/99 8:02a Glen
 * Reorganize event server base class to fix problem with master getting
 * events before all clients are done.
 * 
 * 5     8/13/99 1:45p Darrelld
 * Notify on register
 * 
 * 4     3/10/99 2:27p Glen
 * Fix memory leak
 * 
 * 3     2/01/99 11:15a Glen
 * Joint Test 3 integration
 * 
 * 2     1/18/99 8:52a Glen
 * LTIP Port Client integration
 * 
 * 1     1/14/99 3:35p Glen
 * 
 * 3     1/11/99 2:38p Darrelld
 * Allow protected access to data by derived classes.
 * 
 * 2     12/15/98 10:25a Darrelld
 * Repeater using LRE works
 * 
 * 1     12/14/98 5:47p Darrelld
 * LtLre test modules
 * 
 */


#ifndef _LTLRECLIENTBASE_H
#define _LTLRECLIENTBASE_H 1

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// LreClientBase class
//

class LtLreClientBase: public VxcLock, public LtLreClient
{
public:
    LtLreClientBase(LreClientType clientType, LtChannel *pChannel) : 
		LtLreClient(clientType, pChannel)
	{
	}
};

#endif // _LTLRECLIENTBASE_H

