/***************************************************************
 *  Filename: PlatformVersion.h
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
 *  Description:  Conditionally include correct version file for  
 *					specific platform.
 *
 ****************************************************************/

#ifndef _PLATFORMVERSION_H
#define _PLATFORMVERSION_H


#if defined(ILON_PLATFORM)

/* Add other mechanisms for determining which file to include, as necessary. */
/* Any product with its own version number should have its own version file. */


#if defined(ILON600)
#include "ilon600version.h"

#elif defined(ILON100)
#include "ilon100version.h"

#else
#error  Target iLON platform not defined! Must define ILON100 or ILON600
#endif

#elif defined(CNFGSRVR_BUILD)
#include "cnfgsrvrVersion.h"

#elif PRODUCT_IS(LONTALK_STACK) || PRODUCT_IS(LONTALK_ROUTER) || PRODUCT_IS(IZOT)
#include "version.h"
#else
/* Old name, for VNI */
#include "rcversion.h"		/* pull in version numbers for single file include */
#endif


#endif // _PLATFORMVERSION_H 
