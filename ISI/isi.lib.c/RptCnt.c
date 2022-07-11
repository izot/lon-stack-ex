//	RptCnt.c	implementing IsiGetRepeatCount
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
//	Revision 0, June 2005, Bernd Gauweiler

#include "isi_int.h"

unsigned RepeatCount = ISI_NV_UPDATE_RETRIES;

//	The IsiGetRepeatCount callback gets called whenever the connection table needs initializing: at the first power-up with a new
//	application, or when returning to factory defaults. Applications may override this callback, but only repeat count values 1, 
//	2, or 3 are supported. See _IsiInitialize() for a detailed discussion.

unsigned IsiGetRepeatCount(void)
{
	return RepeatCount;
}
void IsiSetRepeatCount(unsigned Count)
{
    RepeatCount = Count;
}

//	end of RptCnt.c
