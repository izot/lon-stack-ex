//	ApprDas.c	implementing IsiApproveMsgDas
//
// Copyright © 2005-2022 Dialog Semiconductor
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
// IsiApproveMsgDas() must be used with DAS devices - at least if the IsiFetchDomain() function is used
// (i.e. the domain sniffing process is supported). Technically a DAS may chose not to support sniffing
// and can safe a few bytes by using IsiApproveMsg() therefore, but it is recommended that DAS should
// support sniffing to support DAS replacement and redundancy.
//
// Revision 0, July 2005, Bernd Gauweiler
//

#include "isi_int.h"

LonBool IsiApproveMsgDas(const LonByte code, const LonByte* msgIn, const unsigned dataLength)
{
    return (IsiApproveMsg(code, msgIn, dataLength) || (_isiVolatile.Running && (isiDasExtState & isiDasCollect) && (code == LONTALK_SERVICE_PIN_MESSAGE)));
}

//	end of ApprDas.c
