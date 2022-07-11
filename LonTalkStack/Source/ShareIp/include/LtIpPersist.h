#if !defined(LTIPPERSIST_H__68FEF4F1_C68E_11D2_A837_00104B9F34CA__INCLUDED_)
#define LTIPPERSIST_H__68FEF4F1_C68E_11D2_A837_00104B9F34CA__INCLUDED_
/***************************************************************
 *  Filename: LtIpPersist.h
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
 *  Description:  interface for the LtIpPersist class.
 *
 *	DJ Duffy Feb 1999
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/ShareIp/include/LtIpPersist.h#1 $
//
/*
 * $Log: /Dev/ShareIp/include/LtIpPersist.h $
 * 
 * 6     8/19/03 5:06p Fremont
 * add 'morePath' param to makePath()
 * 
 * 5     7/11/03 3:33p Karlo
 * Allow an index to be passed to makePath
 * 
 * 4     7/10/03 6:10p Karlo
 * makePath is now a static method
 * 
 * 3     7/10/03 4:17p Fremont
 * make makePath public
 * 
 * 1     2/22/99 9:26a Darrelld
 * 
 * 
 */

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// IP Persistence Class
//
class LtIpPersist
{
protected:

public:
	LtIpPersist();
	virtual ~LtIpPersist();

	boolean		readFile( char*	pName, byte** ppData, int* pNbytes );
	boolean		writeFile( char* pName, byte* pData, int nBytes );
	boolean		deleteFile( char* pName );
	static void		makePath( char* pPath, const char* pName, int index = 0, const char *morePath = NULL);

};

#endif // !defined(LTIPPERSIST_H__68FEF4F1_C68E_11D2_A837_00104B9F34CA__INCLUDED_)
