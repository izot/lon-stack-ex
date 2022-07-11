#ifndef _LTMISC_H
#define _LTMISC_H

//
// LtMisc.h
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

#define DefineField(type, prefix, name) \
private:\
    type	        prefix##name;\
public:\
    inline type		get##name() { return prefix##name; } \
    inline void     set##name(type bValue) { prefix##name = bValue; }

#define DefineBool(name) DefineField(boolean, m_b, name)
#define DefineInt(name) DefineField(int, m_n, name)

#define trace(s)
//#define trace(s) traceIt(s)
#define traceConditionally(s) 

class LtMisc {
private:
	/*
    static char traceBuf[60][50];
    static int traceBufIndex;
    static boolean traceOn;
    static int traceConditionally;
	*/

protected:    
	/*
    static void bumpTrace() {
        if (++traceBufIndex == LENGTH(traceBuf)) traceBufIndex = 0;
    }
	*/
    
public:
    static void initTrace() {
		/*
        traceBufIndex = 0;
        traceOn = false;
        traceConditionally = 0;
		*/
    }
    
    static int getPriorityType(boolean priority) {
        return priority ? LT_PRIORITY_BUFFER : LT_NON_PRIORITY_BUFFER;
    }

    static boolean getPriority(int bufferType) {
        return bufferType == LT_PRIORITY_BUFFER ? true : false;
    }
    
    static int toTimer(int base, int t) {
        // Compute timer duration based on magic number.
        int val = base << (t / 2);
        if ((t & 1) == 1) val += val/2;
        return val;
    }

    static int toTxTimer(int t) {
        return toTimer(16, t);
    }

    static int toRcvTimer(int t) {
        return toTimer(128, t);
    }

    static int fromTxTimer(int t) {
		if (t <= 16) return 0;
		if (t <= 24) return 1;
		if (t <= 32) return 2;
		if (t <= 48) return 3;
		if (t <= 64) return 4;
		if (t <= 96) return 5;
		if (t <= 128) return 6;
		if (t <= 192) return 7;
		if (t <= 256) return 8;
		if (t <= 384) return 9;
		if (t <= 512) return 10;
		if (t <= 768) return 11;
		if (t <= 1024) return 12;
		if (t <= 1536) return 13;
		if (t <= 2048) return 14;
		return 15;
    }

    static int fromRcvTimer(int t) {
		if (t <= 128) return 0;
		if (t <= 192) return 1;
		if (t <= 256) return 2;
		if (t <= 384) return 3;
		if (t <= 512) return 4;
		if (t <= 768) return 5;
		if (t <= 1024) return 6;
		if (t <= 1536) return 7;
		if (t <= 2048) return 8;
		if (t <= 3072) return 9;
		if (t <= 4096) return 10;
		if (t <= 6144) return 11;
		if (t <= 8192) return 12;
		if (t <= 12288) return 13;
		if (t <= 16384) return 14;
		return 15;
    }

    static int makeint(byte hi, byte lo) {
        return (int)((((int) hi << 8) & 0xff00) | (lo & 0xff));
    }

    static int makeint32(byte hi, byte himi, byte lomi, byte lo) {
        return (int)((((int) hi << 24) & 0xff000000) |
			(((int) himi << 16) & 0xff0000) |
			(((int) lomi << 8) & 0xff00) |
				 (lo & 0xff));
    }

    static int makeint(int hi, int lo) {
        return (int) (((hi << 8) & 0xff00) | (lo & 0xff));
    }
    
    static int makeuint(byte a) {
        return ((int) a) & 0xff;
    }

    static boolean getBool(int a, int mask) {
        return ((a & mask) == mask);
    }
    
    static void dump(byte data[], int offset, int length, int misc) {
        /*
        trace(Integer.toHexString(misc));
        if (false) {

            System.out.print(Integer.toHexString(misc));
            System.out.print(" (len=" + length + ") = \t");
            for (int i = 0; i < length; i++) {
                int val = (data[offset + i] & 0xff);
                if (val < 16)
                    System.out.print("0" + Integer.toHexString(val) + " ");
                else                
                    System.out.print(Integer.toHexString(val) + " ");
            }
            System.out.println();
        }
        */
    }

	/*
    static void traceIt(LPSTR s) {
        traceUnconditional(s);
        traceConditionally = 0;
    }
    
    static void traceUnconditional(LPSTR s) {
        if (traceOn) {
            sprintf(traceBuf[traceBufIndex], "%d: %s", 
                (System::currentTimeMillis()&0xffff), s);
            bumpTrace();
        }
    }
    
    static void traceConditional(LPSTR s, int n) {
        // Trace up to N of these before quitting
        if (traceConditionally != 0) {
            if (traceConditionally == 1) {
                return;
            }
            traceConditionally--;
        } else {
            traceConditionally = n + 1;
        }
        traceUnconditional(s);
    }
    
    static void dumpTrace() {
        int i=0;
        for (i = 0; i < LENGTH(traceBuf); i++) {
            printf("%d. %s", (i + 1), traceBuf[traceBufIndex]);
            bumpTrace();
        }
    }
    
    static void setTrace(boolean which) {
        traceOn = which;
        if (!which) dumpTrace();
    }
	*/
};

#endif
