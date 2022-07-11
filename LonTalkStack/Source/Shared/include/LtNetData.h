#ifndef _LTNETDATA_H
#define _LTNETDATA_H

/***************************************************************
 *  Filename: LtNetData.h
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
 *  Description:  Header file containing macros for moving
 *				  data from network to host and vice versa.
 *
 *	DJ Duffy Feb 1999
 *
 ****************************************************************/

//
// These macros move data from network format to host format.
// The input is a "network byte ordered" value pointed to by p
// and the result is a "host byte ordered" value in a
// whose type is given by the macro name as follows:
//		HL: host long (32 bits)
//		H3: host 3 byte item (24 bits)
//      HS: host short (16 bits)
//		HB: host byte (8 bits)
// Following the move, "p" points just past the item moved.
// This works without causing alignment traps.
// (unlike constructs such as a = *(int*)&p[2])
#define PTOHL(p,a) { a = (((((p[0]<<8)+p[1])<<8)+p[2])<<8)+p[3]; p+=4; }
#define PTOH3(p,a) { a = ((((p[0]<<8)+p[1])<<8)+p[2]); p+=3; }
#define PTOHS(p,a) { a = (p[0]<<8)+p[1]; p+=2; }
#define PTOHB(p,a) { a = *p++; }

#define PTOHBN(n,p,a) \
{ byte* q = (byte*)(a); for(int i=0; i<n;i++) { *q++ = *p++; } }

//
// These macros move data from host format to network format.
// The result is a "network byte ordered" value pointed to by p
// and the input is a "host byte ordered" value in a
// whose type is given by the macro name as follows:
//		NL: host long (32 bits)
//		N3: host 3 byte item (24 bits)
//      NS: host short (16 bits)
//		NB: host byte (8 bits)
// Following the move, "p" points just past the item moved.
#define PTONL(p,a) *p++ = (byte)(a>>24); PTON3(p,a)
#define PTON3(p,a) *p++ = (byte)(a>>16); PTONS(p,a)
#define PTONS(p,a) *p++ = (byte)(a>>8);  PTONB(p,a)
#define PTONB(p,a) *p++ = (byte)a;

#define PTONBN(n,p,a) \
{ byte* q = (byte*)(a); for(int i=0; i<n;i++) { *p++ = *q++; } }

// Host (Variable) TO Network (Address) macros
#define HVTONAL(v,a) { byte* _np = (byte*)a; PTONL(_np,v) }	/* long */
#define HVTONAS(v,a) { byte* _np = (byte*)a; PTONS(_np,v) }	/* short */

// Network (Address) TO Host (Variable) macros
#define NATOHVL(a,v) { byte* _np = (byte*)a; PTOHL(_np,v) }	/* long */ 
#define NATOHVS(a,v) { byte* _np = (byte*)a; PTOHS(_np,v) }	/* short */
															

// straight copies avoid alignment issues
#define PTOXL(p,a) { byte* q = (byte*)&(a); p[0]=q[0];p[1]=q[1];p[2]=q[2];p[3]=q[3]; p+=4; }
#define PTOXS(p,a) { byte* q = (byte*)&(a); p[0]=q[0];p[1]=q[1]; p+=2; }
#define PFMXL(p,a) { byte* q = (byte*)&(a); q[0]=p[0];q[1]=p[1];q[2]=p[2];q[3]=p[3]; p+=4; }
#define PFMXS(p,a) { byte* q = (byte*)&(a); q[0]=p[0];q[1]=p[1]; p+=2; }

#if 0
// 
// These version are deprecated.  The above do the same thing without concern
// for endian-ness.
//
#if defined(_WIN) || defined(WIN32)
#define PTONL_OLD(p,a) { byte* q = (byte*)&(a); p[0]=q[3];p[1]=q[2];p[2]=q[1];p[3]=q[0]; p+=4; }
#define PTONS_OLD(p,a) { byte* q = (byte*)&(a); p[0]=q[1];p[1]=q[0]; p+=2; }
#define PTON3_OLD(p,a) { byte* q = (byte*)&(a); p[0]=q[2];p[1]=q[1];p[2]=q[0]; p+=3; }
#else // endian
#define PTONL_OLD(p,a) { byte* q = (byte*)&(a); p[0]=q[0];p[1]=q[1];p[2]=q[2];p[3]=q[3]; p+=4; }
#define PTONS_OLD(p,a) { byte* q = (byte*)&(a); p[0]=q[0];p[1]=q[1]; p+=2; }
#define PTON3_OLD(p,a) { byte* q = (byte*)&(a); p[0]=q[0];p[1]=q[1];p[2]=q[2]; p+=3; }
#endif // endian

#endif

#endif


