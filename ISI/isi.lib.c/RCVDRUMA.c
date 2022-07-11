//	RcvDrumA.c	implementing _IsiReceiveDrumDas
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

#include "isi_int.h"
//#include <mem.h>

static /*far */
LonByte Lives[256];

#define	DAS_INITIAL_CREDIT	3u
#define	DAS_STANDARD_CREDIT	5u
#define	DAS_MINIMUM_ESTIMATE 8u
#define	DAS_STANDARD_DEBIT	1u
#define DAS_MAXIMUM_CREDITS	128u

#define DAS_FAST_CHANNEL    4

unsigned _IsiGetCurrentDeviceEst(void)
{
	LonByte Count, Index;
	unsigned Value;
	Count = Index = 0;
	// we use a rather unusual do-while construct here, instead of a for-loop. The for-loop would
	// require a 16 bit comparison (for(..., Index < 256,  ...)), whereas this construct can do the
	// same based on 8 bit operations.
	do {
		if (Lives[Index])
        {
			++Count;
		}
		++Index;
	} while (Index);

	// The live counter tends to underestimate with growing actual device counts. We
	// compensate by applying a progressive fudge factor using this formula:
	// D = max(4, min(255, D' + (D' * D')/256))

	Value = high_byte(Count*Count);	        // Count * Count / 256
	Value += (1l + Count);	                //	1 + count + count * count / 256;

	return (unsigned)max(DAS_MINIMUM_ESTIMATE, min(255ul, Value));
}

void _IsiInitDeviceCountEstimation(void)
{
	unsigned Live, Credit;

	// clear everything:
	memset(Lives, 0u, (unsigned)sizeof(Lives));

	// now set up the first N lives, where N is the most recent known
	// device count estimate _isiPersist.Devices. Set up each live with
	// a value between DAS_INITIAL_CREDIT and DAS_STANDARD_CREDIT.
	// Lives[] fills up like so: 2, 3, 4, 5, 2, 3, 4, 5, 2, 3, ...
	// This allows a new DAS, which defaults to 32 devices, to reach
	// the true number within 5 DRUM cycles.
	Credit = DAS_INITIAL_CREDIT;
#ifdef	ISI_SUPPORT_TIMG
	for (Live = 0; Live < _isiPersist.Devices; ++Live) {
#else
	for (Live = 0; Live < ISI_DEFAULT_DEVICECOUNT; ++Live) {
#endif	//	ISI_SUPPORT_TIMG
		Lives[Live] = Credit++;
		if (Credit > DAS_STANDARD_CREDIT) {
			Credit = DAS_INITIAL_CREDIT;
		}
	}
}

void _IsiDecrementLiveCounters(void)
{
	LonByte Number;
	Number = 0;

	// name "Number" is not ideal, but note the variable is used twice, once as an index, and once as a temporary unsigned variable.

	// we use a rather unusual do-while construct here, instead of a for-loop. The for-loop would
	// require a 16 bit comparison (for(..., Local < 256, ...)), whereas this construct can do the
	// same based on 8 bit operations.
	do {
		if (Lives[Number] > DAS_STANDARD_DEBIT) {
			Lives[Number] -= DAS_STANDARD_DEBIT;
		} else if (Lives[Number] <= DAS_STANDARD_DEBIT) {
			Lives[Number] = 0;
		}
		++Number;
	} while(Number);

#ifdef	ISI_SUPPORT_TIMG
	// update the counts (limit writes to EEPROM)
	Number = _IsiGetCurrentDeviceEst();
	if (_isiPersist.Devices != Number)
    {
		_IsiSetDasDeviceCountEst(Number);
	}
#endif	//	ISI_SUPPORT_TIMG
}

void _IsiReceiveDrumDas(const IsiMessage* msgIn)
{
	const IsiDrum* pDrum;
	pDrum = &((const IsiMessage*)msgIn)->Msg.Drum;

	//	First, in the domain address server specific part,
	//	maintain the device counts and channel type
    Lives[pDrum->Nuid] = min(DAS_MAXIMUM_CREDITS, Lives[pDrum->Nuid] + DAS_STANDARD_CREDIT);

    //  If we see a channel type that performs lower than our current "worst case" channel type assumption,
    //  switch to the lower channel.
    //
    //  ASSUMES: channel type numbers for supported channel types are in ascending order, lower numbers indicating faster channels
	if (pDrum->ChannelType > _isiVolatile.ChannelType)
    {
		_IsiSelectTransportProps(_isiVolatile.ChannelType = pDrum->ChannelType);
	}

	// 	Now, do what each ISI device must do: Handle incoming DRUM
	//	messages and re-allocate local network address in case of a conflict:
	_IsiReceiveDrumS(msgIn);

}

//	end of RcvDrumA.c
