#ifndef _LTVXWORKS_H
#define _LTVXWORKS_H

//
// LtVxWorks.h
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

#include <taskLib.h>

#ifdef __cplusplus

#define wdStart(a,b,c,d) wdStart(a,b,(FUNCPTR)c,d)

#ifndef taskSpawn
#define taskSpawn(a,b,c,d,e,p0,p1,p2,p3,p4,p5,p6,p7,p8,p9) \
	taskSpawn(a,b,c,d,(FUNCPTR)e,p0,p1,p2,p3,p4,p5,p6,p7,p8,p9)
#endif

#endif

#endif
