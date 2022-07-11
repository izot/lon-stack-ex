/****************************************************************
 *  Filename: pnc.h
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
 *  Description:  Global PNC definitions.
 *
 *	F Bainbridge July 1999
 *
 ****************************************************************
 *
 * $Log: /Shared/include/pnc.h $
 *
 * 4     9/13/99 10:44a Darrelld
 * Reference _ASMLANGUAGE
 *
 * 3     7/30/99 5:38p Fremont
 * fix name
 *
 * 2     7/30/99 5:00p Darrelld
 * Toshiba root
 *
 * 1     7/30/99 4:49p Fremont
 * Created, defined file path root
 */

#ifndef __PNC_H
#define __PNC_H

#ifdef __cplusplus
extern "C" {
#endif

/* The flash disk device name */
#ifdef TOSHIBA
#define FLASH_DISK_DEVICE_NAME "/ffs0"
#else
#define FLASH_DISK_DEVICE_NAME "/root"
#endif

/* Tack on the trailing slash to the filepath root, */
/* just in case we are forced to make the flash disk */
/* device name include it. */
/* Use this macro to construct file path names */
#define FILEPATH_ROOT FLASH_DISK_DEVICE_NAME "/"

#ifndef _ASMLANGUAGE

/* Place any "extern" declarations here since the
 * assembler will not understand them.
 */
extern int exampleOfFunctionDefinition();

#endif

#ifdef __cplusplus
}
#endif

#endif /* __PNC_H */
