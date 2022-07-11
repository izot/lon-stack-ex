#ifndef LTROUTER_H
#define LTROUTER_H
//
// LtRouter.h
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

#include <stdio.h>
#include <time.h>
#ifdef WIN32
#include <malloc.h>
#include <memory.h>
#endif
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include <vxWorks.h>
#include <msgQLib.h>
#include <taskLib.h>
#include <wdLib.h>
#include <sysLib.h>

#include "LtaDefine.h"

#include "LtVxWorks.h"
#include "LtCUtil.h"
#include "RefQues.h"

#include "System.h"

#include "LonTalk.h"

#include "taskPriority.h"

#include "LtObject.h"
#include "LtHashTable.h"
#include "LtVector.h"
 
#include "LtUniqueId.h"
#include "LtProgramId.h"
#include "LtDomain.h"

#include "LtChannel.h"
#include "LtPktInfo.h"

#include "LtEvents.h"
#include "LtRouteMap.h"
#include "LtLre.h"

#include "LtLreClientBase.h"
#include "LtIpPortClient.h"

#endif
