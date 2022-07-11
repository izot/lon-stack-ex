#ifndef _NS_OPTS_H
#define _NS_OPTS_H
/***************************************************************
 *  Filename: ns_opts.h
 *
 * Copyright © 1996-2022 Dialog Semiconductor
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
 *  Description:  This file specifies the literals which control
 *                compile time options in the Network Services API.
 *
 ****************************************************************/


/*****************************************************************
 *                      Bit and Byte order literals              *
 *****************************************************************
 *
 *  Because LNS messages are processed on various host processors, which
 *  may have different bit and byte ordering schemes, byte swapping must be
 *  performed on some machines to guarantee a consistent message format.
 *
 *  All LNS messages sent on the LonTalk Network assume big-endian bit and
 *  byte ordering. Thus processors with little-endian byte ordering must
 *  swap multi-byte integer values before sending messages on the network
 *  and after receiving messages from the network. Differences in bit field
 *  ordering are handled at compile time by reversing the sequence in which
 *  the bit fields are declared.
 *
 *  In general, compilers on processors which have big-endian byte ordering
 *  also use big-endian bit field ordering (likewise for little-endian).
 *  However, this may not always be the case. Thus, separate literals are
 *  provided for controlling byte ordering and bitfield ordering.
 *  BYTE_BIG_ENDIAN/BYTE_LITTLE_ENDIAN control byte ordering;
 *  BITF_BIG_ENDIAN/BITF_LITTLE_ENDIAN control bit field ordering.
 *
 *
 *  If none of the literals are defined, the following settings are assumed:
 *
 *      Compiler             Byte Fields			Bit Fields
 *
 *      Neuron C             BYTE_BIG_ENDIAN		BITF_BIG_ENDIAN
 *      Microsoft C(*)       BYTE_LITTLE_ENDIAN		BITF_LITTLE_ENDIAN
 *      Default              BYTE_LITTLE_ENDIAN		BITF_LITTLE_ENDIAN
 *
 *  (*) - Literal WIN32 defined.
 *
 *  If your compiler/processor do not match the default assumptions,
 *  uncomment the following literal definitions which match your
 *  environment.
 *
 *  (For more details on Byte ordering, see the file platform.h)
 */

/* #define BYTE_BIG_ENDIAN		*/
/* #define BYTE_LITTLE_ENDIAN	*/

/* #define BITF_BIG_ENDIAN      */
/* #define BITF_LITTLE_ENDIAN   */

/*****************************************************************
 *                     C/Pascal Calling Convention               *
 *****************************************************************
 *
 * The Microsoft C Compiler defaults to the Pascal calling convention
 * (arguments pushed on the stack first to last) for all functions
 * exported from a DLL. Functions which support a variable argument
 * list (notably printf() type functions), require that the C calling
 * convention be used. The Microsoft (and most Windows platform)
 * compilers provide the keyword 'cdecl' to indicate that a specific
 * function is to be invoked using the C calling convention.
 *
 * The NSS API includes the literal NS_CDECL to specify the value
 * of this keyword in various environments.
 *
 * If your compiler generates function calls using the C calling
 * convention in all cases, this macro may be defined with a NULL
 * value; otherwise, define it using the appropriate keyword for
 * your environment.
 *
 * If the literal is not defined, the following setting is assumed:
 *
 *      Compiler             NS_CDECL
 *
 *      Neuron C             (NULL)
 *      Microsoft C(*)       cdecl
 *      Default              (NULL)
 *
 *  (*) - Literal WIN32 defined.
 *  (NULL) indicates that the literal evaluates to nothing.
 *
 */

/* #define NS_CDECL         */

/*****************************************************************
 *                           8 bit Compiler                      *
 *****************************************************************
 *
 * The literal COMPILE_8 indicates that the default integer (type
 * int) is an 8-bit value, which is the case with the Neuron Chip.
 *
 * If your target hardware uses 8-bit integers, then define this
 * literal; otherwise, if indefined, a 16-bit integer is assumed.
 *
 * If the literal is not defined, the following setting is assumed:
 *
 *      Compiler             COMPILE_8
 *
 *      Neuron C             defined
 *      Default              not defined
 *
 */

/*  #define COMPILE_8       */

/*****************************************************************
 *                 Event support (NSE*.C source files)           *
 *****************************************************************
 *
 * Part of the processing of LNS event subscriptions must be handled
 * by the LNS host application. The NSE*.C source files provided with
 * LNS implement this processing. Because of different requirements for
 * different host applications, literals have been defined to control
 * which portions of the NSE implementation to include. In particular:
 *
 * If your application GENERATES LNS event messages, then you must include
 * the NSE SEND_SIDE support. This support includes:
 *      - tracking what applications have subscribed to your app's events.
 *      - notifying your event generator when it is or is no longer
 *        necessary to generate event messages.
 *      - Routing events locally and/or remotely
 *      - Providing event sequence numbers
 *      - Regenerating events when an event receiver misses one or more
 *        event message.
 * => If your application generates LNS event messages, then you must
 *    define the literal INCLUDE_SEND_SIDE_SUPPORT.
 *
 * If your application does not generate events, but does SUBSCRIBE to
 * events (from either the NSS or other LNS host applications), then it
 * must include the NSE RECEIVE_SIDE support. This support includes:
 *      - Tracking the events to which your application has subscribed.
 *      - Correlating incoming event messages to the specified event
 *        tag (used to route the event message to the event handler function)
 *      - Tracking incoming event sequence numbers; if a missed event is
 *        detected, and attempt is made to re-aquire the event from the
 *        generator, if that fails a MISSED_EVENT event is reported.
 * => The receive side database may be stored either in memory or on
 *    disk (default). To specify that the receive side database be
 *    stored in memory, define the literal RCV_SIDE_DB_IN_MEMORY. This
 *    literal MAY NOT BE USED if the literal INCLUDE_SEND_SIDE_SUPPORT is
 *    also defined (the send side database must be non-volatile).
 *
 * The database implemenation provided is for example purposes only and
 * should be replaced with your own database implementation. In particular,
 * this database implementation DOES NOT PROVIDE:
 *      - Multi-user access. Access it limited to a single application.
 *      - Sorted key access. Keyed fields are not sorted and the entire
 *        database must be scanned to locate a record with a specific
 *        key value.
 *
 * If your database implementation provides sorted key access, then the
 * database may be accessed more efficiently. To indicate that your
 * database supports sorted keys, define the literal DB_SUPPORTS_SORTED_KEYS
 *
 *
 * By default, none of the following literals are defined; thus
 *      - Send Side support is NOT included
 *      - The Receive Side database is assumed to be on disk
 *      - Sorted database keys are not used.
 */


/* #define RCV_SIDE_DB_IN_MEMORY        */
/* #define INCLUDE_SEND_SIDE_SUPPORT    */
/* #define DB_SUPPORTS_SORTED_KEYS      */


/*****************************************************************
 *               Memory management (NSE*.C source files)         *
 *****************************************************************
 *
 * The NSS Event support modules require the ability to dynamically
 * allocate (and free) memory. By default, these modules call the
 * standard C runtime functions for this purpose. If your implementation
 * has different requirements, change the following literal
 * definitions.
 *
 */

/* #define MALLOC      malloc   */
/* #define REALLOC     realloc  */
/* #define FREE        free     */
/* #define MEMCPY      memcpy   */
/* #define MEMCMP      memcmp   */
/* #define MEMSET      memset   */

#endif

