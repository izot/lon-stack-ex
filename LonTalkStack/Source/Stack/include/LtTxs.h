#ifndef _LTTXS_H
#define _LTTXS_H

//
// LtTxs.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtTxs.h#2 $
//

// To enable tracing, uncomment #define below
// #define TRACE_TX_ALLOCATION

class LtLayer4;

#ifdef TRACE_TX_ALLOCATION
#include "vxlTarget.h"  // For vxlReportEvent
#endif
#include <assert.h>

template<class TxType> class LtTypedTxs 
{
private:
    TxType**    m_ppTxs;
    int         m_nSize;
	int			m_ctFreePending;
	int			m_ctFree;
    LtLayer4   *m_pStack;
    int         m_nMaxAllocated;
    int         m_nInstantiated;
    int         m_nTxAllocationFailures;
    unsigned int m_nSearches;
    unsigned int m_nProbes;

public:
	LtTypedTxs(int nSize) : m_ppTxs(new TxType*[nSize])
	{
		m_nSize = nSize;
		m_ctFree = m_nSize;
		m_ctFreePending = 0;
        m_pStack = NULL;
        m_nMaxAllocated = 0;
        m_nInstantiated = 0;
        m_nSearches = 0;
        m_nProbes = 0;
        m_nTxAllocationFailures = 0;
        memset(m_ppTxs, 0, sizeof(TxType *) * nSize);
	}

	~LtTypedTxs()
	{
		for (int i = 0; i < m_nSize; i++)
		{
			delete m_ppTxs[i];
		}
		delete m_ppTxs;
	}

	void setOwner(LtLayer4 *pStack)
	{
        m_pStack = pStack;
		for (int i = 0; i < m_nSize; i++)
		{
            if (m_ppTxs[i] != NULL)
            {
			    m_ppTxs[i]->setOwner(pStack);
            }
		}
	}

    TxType* get(int nIndex)
    {
        if (nIndex >= m_nSize) return null;
        return m_ppTxs[nIndex];
    }

    TxType* find(LtPktData* pKey, TxType** ppAvail)
    {
        TxType* pTx = null;
		// Use a prime multiplier to get better distribution in case user
		// is communicating with a consequtive series of addresses.
		int slot = ((unsigned int)pKey->hashCode()*17) % m_nSize;
        if (m_nProbes < 0xffffffff)
        {
            m_nSearches++;
        }
        for (int i = 0; i < m_nSize; i++)
        {
            TxType* pTry = m_ppTxs[slot];
            if (m_nProbes < 0xffffffff)
            {
                m_nProbes++;
            }
            if (pTry == NULL || pTry->isFree())
			{   // Free (or NULL) entry means search is terminated
				if (ppAvail != null)
				{
                    if (*ppAvail == NULL)
                    {   // Haven't already found a better one...
                        if (pTry == NULL)
                        {
#ifdef TRACE_TX_ALLOCATION
                            vxlReportEvent("Instantiat (new) TX for %x at %d, collisions=%d\n", pKey->hashCode(), slot, i);
#endif
		                    pTry = new TxType(slot);
		                    assert(pTry);
                            m_ppTxs[slot] = pTry;
                            m_nInstantiated++; 
                            if (pTry != NULL && m_pStack != NULL)
                            {
                                pTry->setOwner(m_pStack);
                            }
                        }
					    *ppAvail = pTry;
#ifdef TRACE_TX_ALLOCATION
                        vxlReportEvent("Allocate TX for %x at %d, collisions=%d\n", pKey->hashCode(), slot, i);
#endif
                    }
#ifdef TRACE_TX_ALLOCATION
                    else
                    {
                        vxlReportEvent("Allocated elected freePendingTx for %x, collisions=%d\n", pKey->hashCode(), i);
                    }
#endif
				}
				break;
			}
            else if (!pTry->freePending() && pTry->match(pKey))
            {   // Found a match - this is the one to use...
                pTx = pTry;
#ifdef TRACE_TX_ALLOCATION
                vxlReportEvent("Found matching tx for %x at %d, collisions=%d\n", pKey->hashCode(), slot,i);
#endif
                break;
            }
			else if (ppAvail != null && *ppAvail == NULL && pTry->freePending())
            {   // Trying to allocate one, and this is free, its a candidate,
                // assuming that a match doesn't come along.
                *ppAvail = pTry;
#ifdef TRACE_TX_ALLOCATION
                vxlReportEvent("Elect freePending TX for %x at %d, collisions=%d\n", pKey->hashCode(), slot, i);
#endif
            }

			if (++slot == m_nSize) slot = 0;
        }

        return pTx;
    }

	TxType*	alloc(LtPktData* pData)
	{
		TxType* pAvail = null;
		TxType* pTx = find(pData, &pAvail);
		if (pTx == null && pAvail != null)
		{
			pTx = pAvail;
		}
		if (pTx != null)
		{
			*(LtPktData*)pTx = *pData;
			if (pTx->freePending())
			{
				m_ctFreePending--;
				pTx->makeFree();
			}
			else
			{
				m_ctFree--;
			}
            m_nMaxAllocated = max(m_nMaxAllocated, m_nSize-(m_ctFreePending+m_ctFree));
		}
        else
        {
            m_nTxAllocationFailures++;
#ifdef TRACE_TX_ALLOCATION
            vxlReportEvent("Cannot allocate TX for %x\n", pData->hashCode());
#endif
        }
		return pTx;
	}

	TxType* get(LtPktData* pKey)
	{
		TxType* pTx = find(pKey, null);
		return pTx;
	}

	void release(TxType* pTx)
	{
		// When we release, we don't release if next entry is in use since
		// searches rely on empty entry to terminate.  Once subsequent entry
		// is freed, it will free this one.  Edge condition - if all tx get
        // allocated, the "next" entry for all of them will be either allocated
        // or pending free.  When they all become pending free, free them all.
		int nIndex = pTx->getRefId().getIndex();
		int next = nIndex + 1;
		if (next == m_nSize) next = 0;

		if (m_ppTxs[next] == NULL || m_ppTxs[next]->isFree() ||
            ((m_ctFreePending == m_nSize-1) && !pTx->freePending()))
		{
#ifdef TRACE_TX_ALLOCATION
            vxlReportEvent("Release TX at %d\n", nIndex);
            if ((m_ctFreePending == m_nSize-1) && !pTx->freePending())
            {
                vxlReportEvent("All tx are pending free\n");
            }
#endif
			if (pTx->freePending()) m_ctFreePending--;
			pTx->makeFree();
			m_ctFree++;
			// Free all entries before me which are in pending state.
			for (int i = 0; i < m_nSize; i++)
			{
				if (--nIndex < 0) nIndex = m_nSize-1;
				if (m_ppTxs[nIndex] != NULL && m_ppTxs[nIndex]->freePending())
				{
#ifdef TRACE_TX_ALLOCATION
                    vxlReportEvent("Release pending TX at %d\n", nIndex);
#endif
					m_ppTxs[nIndex]->makeFree();
					m_ctFree++;
					m_ctFreePending--;
				}
				else
				{
					break;
				}
			}
		}
		else if (!pTx->freePending())
		{
#ifdef TRACE_TX_ALLOCATION
            vxlReportEvent("Set TX freePending at %d\n", nIndex);
#endif
			m_ctFreePending++;
			pTx->setPendingFree();
		}
	}

    void reset()
    {
        for (int i = 0; i < m_nSize; i++)
        {
            if (m_ppTxs[i] != NULL)
            {
			    m_ppTxs[i]->makeFree();
            }
        }
		m_ctFree = m_nSize;
		m_ctFreePending = 0;
    }

	void stats()
	{
		printf(" ctFree: %d; ctFreePending: %d\n", m_ctFree, m_ctFreePending);
	}

    void getStats(int &nMax, int &nFree, int &nPendingFree, 
                  int &nMaxAllocated, int &nInstantiated, int &searchRatio,
                  int &txAllocationFailures)
    {
        nMax = m_nSize;
        nFree = m_ctFree;
        nPendingFree = m_ctFreePending;
        nMaxAllocated = m_nMaxAllocated;
        nInstantiated = m_nInstantiated;
        txAllocationFailures = m_nTxAllocationFailures;
        if (m_nSearches != 0)
        {
            searchRatio = m_nProbes/m_nSearches;
        }
        else
        {
            searchRatio = 0;
        }
    }

    void clearStats(void)
    {
        m_nProbes = 0;
        m_nSearches = 0;
        m_nMaxAllocated = 0;
        m_nTxAllocationFailures = 0;
    }
};

#endif
