//
// LtStatus.cpp
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

#include "LtStackInternal.h"

LtStatus::LtStatus()
{
	transmissionErrors = 0;
	transmitTimeouts = 0;
	receiveTransactionFulls = 0;
	lostMessages = 0;
	missedMessages = 0;
	resetCause = 0;
	state = 0;
	version = 0;
	error = 0;
	model = 0;
}

void LtStatus::package(LtBlob *pBlob)
{
    pBlob->package(&transmissionErrors);
    pBlob->package(&transmitTimeouts);
    pBlob->package(&receiveTransactionFulls);
    pBlob->package(&lostMessages);
    pBlob->package(&missedMessages);
    pBlob->package(&resetCause);
    pBlob->package(&state);
    pBlob->package(&version);
    pBlob->package(&error);
    pBlob->package(&model);
}

void LtStatus::fromLonTalk(byte* pData)
{
	PTOHS(pData, transmissionErrors);
	PTOHS(pData, transmitTimeouts);
	PTOHS(pData, receiveTransactionFulls);
	PTOHS(pData, lostMessages);
	PTOHS(pData, missedMessages);
	PTOHB(pData, resetCause);
	PTOHB(pData, state);
	PTOHB(pData, version);
	PTOHB(pData, error);
	PTOHB(pData, model);
}
