//
// LtNetworkManager2.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtNetworkManager2.cpp#1 $
//

#include "LtStackInternal.h"

//
// This code is based on code in the Neuron.  See Neuron source n_phase.ns for more details.
//

//
// Phase computation literals
//
// Numbers for frequency 50Hz with period 20000 usec
#define		PHASE_TICKS_50				98
#define		PHASE_CONVERSION_A_50		112
#define		PHASE_CONVERSION_A_ALT_50	128
#define		PHASE_CONVERSION_B_50		16
// Time that zero crossing is low: 9223 usec
#define		DEAD_ZONE_50				45
// Numbers for frequency 60Hz with period 16666 usec
#define		PHASE_TICKS_60				81
#define		PHASE_CONVERSION_A_60		107
#define		PHASE_CONVERSION_A_ALT_60	134
#define		PHASE_CONVERSION_B_60		14
// Time that zero crossing is low: 7660 usec
#define		DEAD_ZONE_60				38

//
// Phase computation data
//
typedef struct
{
	int		ticks;
	int		conversion_a[2];
	int		conversion_b;
	int		deadZone;
} PhaseComputationData;

static PhaseComputationData pcd[2] =
{
	{ 
		PHASE_TICKS_50,
		{PHASE_CONVERSION_A_50,	PHASE_CONVERSION_A_ALT_50},
		PHASE_CONVERSION_B_50,
		DEAD_ZONE_50
	},
	{
		PHASE_TICKS_60,
		{PHASE_CONVERSION_A_60,	PHASE_CONVERSION_A_ALT_60},
		PHASE_CONVERSION_B_60,
		DEAD_ZONE_60
	}
};

//
// getNeuronPhaseData
//
// This method extracts the data from the Neuron Chip that is needed to do a phase
// computation.  Unfortunately, some of this data is accessible not via standard API
// but only through direct memory read.  This will need to suffice for the short term.
// Eventually we should add an API to the MIP for this.
//
LtErrorType LtNetworkManager::getNeuronPhaseData(int& freq, int&clockFactor)
{
	LtErrorType err = LT_INVALID_STATE;

	if (m_freq == -1 || m_freq == 0)
	{
		LtCommParams cp;
		// Information is not known.  We must fetch it.
		m_clockFactor = 100;
		if (getStack()->getDriver()->getCommParams(cp) == LTSTS_OK)
		{
			if (cp.m_nData[1] & 0x01)
			{
				m_clockFactor = 131;
			}
			// Get the frequency 
			if (getStack()->getDriver()->getFrequency(m_freq) == LTSTS_OK)
			{
				err = LT_NO_ERROR;
			}
		}
	}
	else
	{
		err = LT_NO_ERROR;
	}
	freq = m_freq;
	clockFactor = m_clockFactor;

	return err;
}

//
// processComputePhase
//
// This method is called by the network manager when a Compute Phase network management
// function is received.  This function extracts the reading made by the MAC layer from
// the packet and determines the phase. 
//
// It generates a phase value as follows:
//
// 0: unknown
// 1: 0 degrees
// 2: -120 inverse
// 3: +120
// 4: +180
// 5: -120
// 6: +120 inverse
//

LtErrorType LtNetworkManager::processComputePhase(LtApduIn& apdu, LtApdu& response)
{
	int freq;
	int clockFactor;
	int phase = 0;
	int error = 0;	// init all these to eliminate warnings
	int result = 0;
	byte reading = 0;
	bool bOk = true;

	LtErrorType err = getNeuronPhaseData(freq, clockFactor);

	if (!apdu.getViaLtNet())
	{
		// Mimic fact that in phase meters read +180 due to measurement on falling edge.
		result = 3;
		error = 0;
		reading = 0;
	}
	else if (err == LT_NO_ERROR && freq != 0)
	{
		int index = freq==50 ? 0 : 1;
		PhaseComputationData myPcd = pcd[index];
		PhaseComputationData* pData = &myPcd;

		reading = (byte)apdu.getPhaseReading();

		// Scale the PCD numbers.  Note that we don't precompute clockFactor/100 to avoid using "float".
		pData->conversion_a[0] = pData->conversion_a[0] * clockFactor / 100;
		pData->conversion_a[1] = pData->conversion_a[1] * clockFactor / 100;
		pData->conversion_b = pData->conversion_b * clockFactor / 100;
		pData->deadZone = pData->deadZone * clockFactor / 100;
		pData->ticks = pData->ticks * clockFactor / 100;

		// Extract the zero crossing relative reading and add in the offset
		// to move signal to middle of cell.  A phase is divided into 6
		// cells and the phase shift is determined by which of the 6 cells
		// the packet arrived in.

		// We do phase measurement based on the falling edge.  This is because that
		// is the way the timer counter works.  If we invert, then we have a problem
		// with the asymmetry of the edges.
		//
		// Now do phase adjustment that is necessary to account for false falling
		// edges on the rising edge.  When the phase value is latched, we also save
		// the IO state in the MSB.  This allows us to detect an out of whack reading
		// and adjust it as follows.  If the reading is less than the half way point 
		// but the I/O state at the time was high, then add in the dead zone time.  Note that
		// this logic doesn't handle the case where the reading occurs right at
		// the falling edge after a false falling edge at the rising edge.  We can't
		// distinguish this case from a legitimate reading at the rising edge.  For this reason
		// the sender attempts to align the edge so as to avoid this zone.

		// Note that the phase won't be less than the dead zone point if the high bit
		// is set due to the I/O sampling of low at latch time (see m_rcv.ns)
		phase = reading;
		if (phase < pData->deadZone)
		{
			// Sample was bogus due to mistaken detection of falling edge at rising edge.
			// Adjust by adding in the delta between the falling and rising edges (== dead zone time)
			phase += pData->deadZone;
		}
		// Mask out MSB which might have been set by MAC layer (see m_rcv.ns)
		phase &= 0x7f;

		// Subtract out delay from transmit start to packet detect (depends on alternate
		// path setting).
		phase = phase - pData->conversion_a[apdu.getAlternatePath() == LT_ALT_PATH ? 1 : 0];

		while (phase < 0)
		{
			// Went negative.  Add in phase ticks until we're back in positive territory
			phase += pData->ticks;
		}

		result = phase / pData->conversion_b;

		// Get remainder to determine error
		error = phase % pData->conversion_b - (pData->conversion_b/2);

		if (result)
		{
			// Because the phase reading is done by this node relative to its view
			// of the world, the answer derived to this point is flipped relative
			// to the sender.  So, we flip it back to give a sender relative answer.
			result = 6 - result;
			if (result < 0)
			{
				// Out of range, return 0
				result = 0;
				bOk = false;
			}
		}
	}

	if (err == LT_NO_ERROR)
	{
		if (bOk)
		{
			// Incorporate agent's phase into answer
			byte agentPhase = 0;
			if (apdu.getLength())
			{
				agentPhase = apdu.getData(0)-1;
			}

			result += agentPhase;
			// Zero crossing circuit measures falling edge so answer is off by 180 degrees
			result += 3;
			// If >=6, subtract 6
			result %= 6;
			// Reported phase is value from 1 to 6.
			result++;
		}

		// Set the response data
		response.setData(0, (byte)result);
		response.setData(1, (byte)freq);
		response.setData(2, reading);
		response.setData(3, (byte)error);
	}

	return err;
}

