#ifndef LTLINKDEFAULT_H
#define LTLINKDEFAULT_H

//
// LtLinkDefault.h
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

#include "LtDriver.h"

#if PRODUCT_IS(DCX)
#ifdef WIN32
#include "LonLinkVni.h"
#define LtLinkDefault	LonLinkVni
#else
#include "LonLinkDcx.h"
#define LtLinkDefault	LonLinkDcx
#endif

#elif PRODUCT_IS(LONTALK_STACK) || PRODUCT_IS(LONTALK_ROUTER)
#if defined(linux) 
#include "LonLinkDcx.h"
#define LtLinkDefault	LonLinkDcx
#elif defined(WIN32)
#include "LonLinkWin.h"
#define LtLinkDefault	LonLinkWin
#else
#error You must define LtLinkDefault
#endif

#elif PRODUCT_IS(IZOT)
#if defined (linux)
#define LtLinkDefault	LonLink
#elif defined(WIN32)
#include "LonLinkWin.h"
#define LtLinkDefault	LonLinkWin
#else
#error You must define LtLinkDefault
#endif

#elif PRODUCT_IS(ILON)
#if defined(linux) // for now, use USB for all linux ilons
#include "LonLinkUsb.h"
#define LtLinkDefault	LonLinkUsb
#else
#include "defs.h"
#include "lonc.h"
#include "LonLinkILon.h"
#define LtLinkDefault	LonLinkILon
#endif

#elif PRODUCT_IS(FTXL)
#include "LonLinkFtxl.h"
#define LtLinkDefault	LonLinkFtxl
#elif PRODUCT_IS(LIFTBR)
#include "LonLinkIzoTRouter.h"
#define LtLinkDefault LonLinkIzoTRouter
#else  // VniStack
#include "LonLinkWin.h"
#define LtLinkDefault	LonLinkWin
#endif

#endif

