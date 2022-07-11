#ifndef LTEVENTSERVERBASE_H
#define LTEVENTSERVERBASE_H

//
// LtEventServerBase.h
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
// Base class for implementing LtEvents.h
//

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// Event Client Vector
//
#ifdef WIN32
// Explicitly instantiate this template class so we can attach the
// __declspec() attribute to it for the DLL implementation.
// The Tornado compiler (GNU) cannot handle this.
template class LTA_EXTERNAL_CLASS LtTypedVector<LtEventClient>;
#endif

class LTA_EXTERNAL_CLASS LtEventClientV : public LtTypedVector<LtEventClient>
{
};

class LTA_EXTERNAL_CLASS LtEventServerBase : public LtEventServer
{
protected:
    LtEventClientV	m_vClients;
	boolean			m_bNotificationOccurred;

public:
    LtEventServerBase()
	{
		m_bNotificationOccurred = false;
	}
	// Declare a virtual destructor to eliminate warnings
	virtual ~LtEventServerBase() {}

    void registerEventClient(LtEventClient* pEventClient)
    {
		if (!m_vClients.isElement(pEventClient))
		{
	        m_vClients.addElement(pEventClient);
			if (m_bNotificationOccurred)
			{
				// Have already notified a client.  New client
				// should get immediate notification to establish
				// baseline.
				pEventClient->eventNotification(this);
			}
		}
    }

    void deregisterEventClient(LtEventClient* pEventClient)
    {
        m_vClients.removeElement(pEventClient);
    }

    void notify()
    {
        LtVectorPos pos;
        LtEventClient* pClient;
        while (m_vClients.getElement(pos, &pClient))
        {
            pClient->eventNotification(this);
        }
		m_bNotificationOccurred = true;
    }
};

#endif
