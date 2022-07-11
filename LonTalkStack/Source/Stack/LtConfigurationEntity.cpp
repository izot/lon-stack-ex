//
// LtConfigurationEntity.cpp
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
// $Header $
//

#include <LtStackInternal.h>

LtErrorType LtConfigurationEntity::checkLimits(int cmd, byte* pData, int len)
{
	boolean ok = false;
	// Default implementation of check limits.
	switch (cmd)
	{
		case LT_NM_INITIALIZE:
			ok = len == 4;
			break;
		case LT_NM_ENUMERATE:
			ok = len == 2;
			break;
		case LT_NM_REMOVE:
			ok = len == 4;
			break;
		case LT_NM_SET_DESC:
		{
			int descLen = pData[5];
			ok = len == 6 + descLen;
			break;
		}
		case LT_NM_GET_DESC:
			ok = len == 5;
			break;
	}
	return ok ? LT_NO_ERROR : LT_INVALID_PARAMETER;
}
