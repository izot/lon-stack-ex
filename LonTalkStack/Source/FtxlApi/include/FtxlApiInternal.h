/*
 * File: FtxlApiInternal.h
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
 * Title: FTXL LonTalk 1.0 API Reference wrapper (this version is for internal use only)
 * It includes some of the internal files. 
 */

#ifndef _FTXL_API_INTERNAL_H
#define _FTXL_API_INTERNAL_H

#include <stdlib.h>
#ifdef  __cplusplus
extern "C" {
#endif
#include "FtxlApi.h"
#include "LtaDefine.h"

#define FTXL_EXTERNAL_FN extern

#if PRODUCT_IS(IZOT)
#define DEFAULT_NVD_FOLDER  ".izot-nvd"
#else
#define DEFAULT_NVD_FOLDER  "nvd-files"
#endif

// Debug statement
FTXL_EXTERNAL_FN void APIDebug(const char * fmt, ...);
#define CALLBACK_EXEC(str)                  APIDebug("%s = executing registered callback vector\n", str)
#define CALLBACK_EXCEPTION(str)             APIDebug("%s = ** an exception occurred when executing the callback vector **\n", str)
#define CALLBACK_NOT_REGISTERED(str)        APIDebug("%s = callback vector not registered\n", str)
#define CALLBACK_NOT_REGISTERED_DEF(str)    APIDebug("%s = callback vector not registered, executing default\n", str)
#define CALLBACK_EXEC_IDX(str,idx)                  APIDebug("%s(Index %d) = executing registered callback vector\n", str, idx)
#define CALLBACK_EXCEPTION_IDX(str,idx)             APIDebug("%s(%d) = ** an exception occurred when executing the callback vector **\n", str, idx)
#define CALLBACK_NOT_REGISTERED_IDX(str,idx)        APIDebug("%s(%d) = callback vector not registered\n", str, idx)
#define CALLBACK_NOT_REGISTERED_DEF_IDX(str,idx)    APIDebug("%s(%d) = callback vector not registered, executing default\n", str, idx)
#define CALLBACK_REGISTER(str, sts)         APIDebug("%sRegistrar = %d\n", str, sts)

const unsigned GetDefaultCurrentNvSize(const unsigned index);
const unsigned GetDefaultApplicationSegmentSize();
const LonApiError DefaultSerializeSegment(LonBool toNvMemory, void* const pData, const size_t size);

#ifdef  __cplusplus
}
#endif




#endif /* _FTXL_API_INTERNAL_H */
