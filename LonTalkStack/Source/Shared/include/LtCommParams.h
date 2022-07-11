#ifndef LTCOMMPARAMS_H
#define LTCOMMPARAMS_H

//
// LtCommParams.h
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

class LtCommParams {
public:
	byte m_nData[16];	// Neuron comm parameter structure

	boolean isCompatible(LtCommParams& cps)
	{
		// Determine compatibility of comm params.  
		return memcmp(m_nData, cps.m_nData, 2) == 0;
	}
	int setNewPriority(LtCommParams& cps)
	{
		int nResult = 0;
		// Cap the priority assignment to be the max of old priority and the new max priority.
		m_nData[7] = cps.m_nData[7];
		if (m_nData[7] && m_nData[8] < m_nData[7])
		{
			m_nData[7] = m_nData[8];
			nResult = m_nData[7];
		}
		return nResult;
	}
};

#endif
