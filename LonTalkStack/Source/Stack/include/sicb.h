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

#pragma once

//typedef bool boolean;

#define SICB_LONG_TIMER	0x80 //MS ?

#define GetSicbLen(pSicb) ((pSicb)->getLength())

#define proxy_ccresp_free(pSicb) 

typedef enum
{
	MMT_PROXY,
	MMT_PROXY_RPT_AGNT,
	MMT_APP,
	MMT_ATM,
} MmtType;

class CMeter;
class CMeterMsgTag : public VniMsgTag
{
public:
	CMeterMsgTag(MmtType mtType) : VniMsgTag(), m_mtType(mtType), m_pProxyIn(NULL), 
		m_proxyCount(-1), m_meterRefId(m_nextRefId++), m_pMeter(NULL), m_request(true) {}

	MmtType		GetType()	{ return m_mtType; }
	bool		IsProxy()	{ return m_mtType==MMT_PROXY || m_mtType==MMT_PROXY_RPT_AGNT; }
	bool		IsRptAgent() { return m_mtType==MMT_PROXY_RPT_AGNT; }
	bool		IsRequest() { return m_request; }

	VniMsgIn	*GetProxyIn()			{ return m_pProxyIn; }
	void		SetProxyIn(VniMsgIn *p, CMeter *pMeter);
	void		ReleaseProxyIn();

	int			GetProxyCount()			{ return m_proxyCount; }
	void		SetProxyCount(int c)	{ m_proxyCount = c; }


	int			GetMeterRefId()			{ return m_meterRefId; }

private:
	MmtType		m_mtType;
	VniMsgIn	*m_pProxyIn;
	CMeter		*m_pMeter;
	int			m_proxyCount;
	int			m_meterRefId;
	bool		m_request;
	static int	m_nextRefId;
};
