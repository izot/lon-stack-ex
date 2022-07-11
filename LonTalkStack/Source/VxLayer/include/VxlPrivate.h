#ifndef _VXLPRIVATE_H
#define _VXLPRIVATE_H
/***************************************************************
 *  Filename: VxlPrivate.h
 *
 * Copyright © 1998-2022 Dialog Semiconductor
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
 *  Description:  Private Header file for VxWorks emulation layer.
 *
 *	DJ Duffy Oct 1998
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/VxLayer/include/VxlPrivate.h#1 $
//
/*
 * $Log: /VNIstack/Dcx_Dev/VxLayer/include/VxlPrivate.h $
 * 
 * 7     6/24/08 7:56a Glen
 * Support for EVNI running on the DCM
 * 1. Fix GCC problems such as requirement for virtual destructor and
 * prohibition of <class>::<method> syntax in class definitions
 * 2. Fix up case of include files since Linux is case sensitive
 * 3. Add LonTalk Enhanced Proxy to the stack
 * 4. Changed how certain optional features are controlled at compile time
 * 5. Platform specific stuff for the DC such as file system support
 * 
 * 1     12/05/07 11:19p Mwang
 * 
 * 6     3/26/07 2:36p Bobw
 * 
 * 5     4/09/04 1:57p Rjain
 * Put some function declarations in the right place.
 * 
 * 4     2/12/02 9:54a Fremont
 * remove extra 'extern "C" ' block
 * 
 * 3     12/21/99 3:34p Glen
 * Protect queue count within lock
 * 
 * 1     11/04/98 8:37a Darrelld
 * header files
 * 
 * 
 */



#include <VxlTypes.h>
#include "Osal.h"

#ifdef  __cplusplus
extern "C" {
#endif

//
// DoubleLinked Queue services
// CAUTION *** NOT INTENDED TO BE PORTABLE TO VXWORKS ***
// Use RefQues.h queues for portability.
// These queues are used to implement msgQ Services etc.
// in Windows VxLayer.
//


//
// DoubleLinked Queue services
//
typedef struct _Que
{
	struct _Que*		pNxt;
	struct _Que*		pPrv;
	int					elementCount;
	BOOL				bLock;
	OsalHandle          csLock;
} Que;


// Init the queue, and maybe a critical section
void	QueInit( Que* pHead, BOOL bLock );
// Zap the queue, and maybe delete the critical section
// No items touched.
void	QueDelete( Que* pHead );
void	QueFreeAll( Que* pHead );
void	QueLock( Que* pHead );
void	QueUnlock( Que* pHead );
void	QueInsert( Que* pPrev, Que* pItem );
void	QueRemove( Que* pItem );
BOOL	QueIsEmpty( Que* pHead );
int		QueElementCount( Que* pHead );

BOOL	QueInsertTail( Que* pHead, Que* pItem );
BOOL	QueInsertHead( Que* pHead, Que* pItem );
BOOL	QueRemoveTail( Que* pHead, Que** ppItem );
BOOL	QueRemoveHead( Que* pHead, Que** ppItem );
BOOL	QueCheck( Que* pHead, int maxItems );

#ifdef __cplusplus
}
#endif

// end
#endif // _VXLPRIVATE_H
