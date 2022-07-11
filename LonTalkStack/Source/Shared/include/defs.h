/*
 * Definitions and Limits
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
 */
/*
 Rev.0: 98/10/15
   Releases
 */
#define	TXQ_DEFAULT_SIZE	4
#define	RXQ_DEFAULT_SIZE	4
#define	MAX_STACKS		10000

/* Test Facilities */
#define	TEST_NORXSLOT	// Define if RXSLOT fails
#define	TEST_NOTXSLOT	// Define if TXSLOT fails
#undef	TEST_STUB	// Define if no LON-C is available
#define	TEST_KEEP_DESC	// Define if you'd like to keep descriptors
#define	TEST_CANCELBUG	// Define if DMA priority problem
#define	DEBUG	0
#ifdef	TEST_KEEP_DESC
#define	DEBUG_MAX_DESC_KEEP	2000
#endif
#undef	TEST_RCWBASE_RUNNING_CHANGE	// Define it if you want to change RCWBASE on running status

/* Initial CommParams: \ is vital to be a macro. */
#define	LONC3_INITIAL_COMMPARAMS	{		\
  0x25, 0x2E, 0x03, 0x02, 0x02, 0x0B, 0x0B, 0x00,	\
  0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	\
} 
