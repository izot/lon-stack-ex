//	MsgS.c	implementing IsiProcessMsgS
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
//	Revision 0, February 2005, Bernd Gauweiler
//	Revision 1, June 2005, Bernd Gauweiler: Added support for DRUMEX, CSMOEX, CSMAEX, and CSMREX messages


#include "isi_int.h"

LonBool IsiProcessMsgS(const LonByte code, const IsiMessage* msgIn, const unsigned dataLength)
{
#ifdef	ISI_SUPPORT_AUTOMATIC_CONNECTIONS
	LonBool Auto, ShortForm;
	Auto = ShortForm = FALSE;
#else
	LonBool ShortForm;
	ShortForm = FALSE;
#endif	//	ISI_SUPPORT_AUTOMATIC_CONNECTIONS

	// reset the spreading timer:
	_isiVolatile.Spreading = 0;

	switch(msgIn->Header.Code)
    {
		case isiDrumEx:
			// ASSUMES: fall-through because ISI ignores the "Extended" DRUM fields.
		case isiDrum:
			//
			// Receive DRUM	-	Domain Resource Usage Message
			//
			_IsiReceiveDrumS(msgIn);
			break;
#ifdef	ISI_SUPPORT_TIMG
		case isiTimg:
			//
			// Receive TIMG	-	Timing Guideance Message
			//
			_IsiReceiveTimg(msgIn->Msg.Timg.DeviceCountEstimate, msgIn->Msg.Timg.ChannelType);
			break;
#endif	//	ISI_SUPPORT_TIMG
#ifdef	ISI_SUPPORT_AUTOMATIC_CONNECTIONS
		case isiCsmo:
			ShortForm = TRUE;
			goto HandleCsmoX;
		case isiCsma:
		case isiCsmr:
			ShortForm = TRUE;
			// FALL-THROUGH:
		case isiCsmaEx:
		case isiCsmrEx:
			//
			// Receive CSMA + CSMR:	Automatic enrollment + reminder
			//
			Auto = TRUE;
			// FALL THROUGH:
		case isiCsmoEx:
			//
			// Receive CSMO	-	Open Enrollment
			//
HandleCsmoX:
			_IsiReceiveCsmo(Auto, ShortForm, &msgIn->Msg.Csmo);
			break;
#else
		case isiCsmo:
			ShortForm = TRUE;
			// FALL-THROUGH:
		case isiCsmoEx:
			_IsiReceiveCsmo(FALSE, ShortForm, &msgIn->Msg.Csmo);
			break;
#endif	//	ISI_SUPPORT_AUTOMATIC_CONNECTIONS
#ifdef	ISI_SUPPORT_MANUAL_CONNECTIONS
		case isiCsmx:
			//
			// Receive CSMX	-	Cancel pending enrollment
			//
			_IsiReceiveCsmx(&msgIn->Msg.Csmx);
			break;
		case isiCsmc:
			//
			// Receive CSMC	-	Confirm pending enrollment
			//
			_IsiReceiveCsmc(&msgIn->Msg.Csmc);
			break;
		case isiCsme:
			//
			// Receive CSME	-	Accept pending enrollment
			//
			_IsiReceiveCsme(&msgIn->Msg.Csme);
			break;
#endif	//	ISI_SUPPORT_MANUAL_CONNECTIONS
#ifdef ISI_SUPPORT_CONNECTION_REMOVAL
		case isiCsmd:
			//
			// Receive CSMD	-	Delete enrollment
			//
			_IsiReceiveCsmd(&msgIn->Msg.Csmd);
			break;
#endif	//	ISI_SUPPORT_CONNECTION_REMOVAL
		case isiCsmi:
			//
			// Receive CSMI	-	Connection Status Information
			//
			_IsiReceivePtrCsmi(&msgIn->Msg.Csmi);
			break;
		default:
			// message not recognized; return TRUE to indicate a caller may post-process the message (it is still valid)
			return TRUE;
	}
	return FALSE;
}

//	end of MsgS.c
