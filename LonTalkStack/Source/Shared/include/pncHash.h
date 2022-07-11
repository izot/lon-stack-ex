/* HASHTABLE
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
#ifndef _PNCHASH_H
#define _PNCHASH_H

#include <stdio.h>
#include <semLib.h>
#include "pncSysHash.h"

#define	MAX_DATA_ENTRY	256

class pncHashTable {
  static pncHashTable *me;
public:
  static pncHashTable *getSystemHash() {
    if(me == NULL) {
      me = new pncHashTable(0);
    }
    return me;
  }

  pncHashTable(int arg);
  ~pncHashTable();

  int insert(char *key, void *value, int length);
  int remove(char *key);
  int get(char *key, void *value, int length);
  int getSizeOfValue(char *key);

// Debug Feature
  int dump();
};

#endif // _PNCHASH_H
