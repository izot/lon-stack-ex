/**
 * LonTalk UniqueId List
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
#include "LtDriver.h"

class LtUniqueIdList {
public:
	virtual ~LtUniqueIdList() {}

/**
 * Get number of UniqueIds holded in router/server
 *
 * It will return -1 if anything goes wrong -- "can't happen" happens.
 */
  virtual int getCount() = 0;

/**
 * Get an UniqueID by index.
 * the index should be from 0 to (getCount() - 1).
 *
 * It will return LTSTS_OK if LtUniqueId is filled, or LTSTS_ERROR
 * if anything goes wrong.
 */
  virtual LtSts get(int index, LtUniqueId *id) = 0;
};
