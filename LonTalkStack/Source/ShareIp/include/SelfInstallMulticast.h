/***************************************************************
 *  Filename: SelfInstallMulticast.h
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
 *  Description:  compile control for the self-installed multicast IP channel feature
 *
 *	Fremont Bainbridge
 *  December 9, 2004
 *
 ****************************************************************/

#ifndef SELFINSTALLMULTICAST_H


// Special multicast channel mode
// Always define this literal, to either 0 (false) or 1 (true).
// Files that uses it should use #if rather than #ifdef
// and they should generate an error if it is not defined
#ifdef WIN32
// This does not work on Windows yet (header file conflicts for IP stuff)
#define INCLUDE_SELF_INSTALLED_MULTICAST 1
#else
// Enabled on the iLON
#define INCLUDE_SELF_INSTALLED_MULTICAST 1
#endif


#endif // SELFINSTALLMULTICAST_H
