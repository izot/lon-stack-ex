//	SendMsg.c	implementing IsiMsgSend
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
//	Revision 0, July 2005, Bernd Gauweiler

#include "isi_int.h"

//  IsiMsgSend simply calls _IsiSendIsi with the parameters in the right stack order. The IsiMsgSend API may be used by
//  those applications that extened ISI in some useful way, but it is not used by the library internally. Internally,
//  we use the _IsiSendIsi function or anyhthing that layers on top of that (see send.ns). The fact that IsiMsgSend() was
//  defined, implemented and documented with an inconvenient parameter order was discovered late in the game; changing
//  the API a few days short of version 1 RTM didn't seem a good choice.
//  This function used to be documented because we had it anyway (on the rather off chance that someone might find it
//  useful), and now we don't use it anymore internally and provide it because it's documented.
void IsiMsgSend(const IsiMessage* pMsg, unsigned IsiMessageLength, LonServiceType Service, LonSendAddress* pDestination)
{
    _IsiSendIsi(Service, pMsg, IsiMessageLength, pDestination);
}

//	end of SendMsg.c
