#ifndef _LTMD5_H
#define _LTMD5_H
/***************************************************************
 *  Filename: LtMD5.h
 *
 * Copyright © 2022 Dialog Semiconductor
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in 
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *  Description:  definition of the LtMD5 class.
 *				  Also some C callable interfaces
 *
 *	DJ Duffy July 1999
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/ShareIp/include/LtMD5.h#1 $
//
/*
 * $Log: /Dev/ShareIp/include/LtMD5.h $
 *
 * 7     9/16/03 5:35p Fremont
 * allow retreiving the secret, let user check auth with 'other' algorithm
 *
 * 6     6/10/03 5:37p Fremont
 * changes for EIA0852 auth, remove global auth settings
 *
 * 5     3/14/02 3:07p Fremont
 * Make the simple digest routine calleable from C
 *
 * 4     2/25/00 5:46p Darrelld
 * Fixes for PCLIPS operation
 *
 * 2     11/12/99 3:19p Darrelld
 * Updates for static secret
 *
 *
 */

// WARNING -- This file must be safe to include from C code

#include <assert.h>
#include <string.h>

// Define some C callable interfaces
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void md5Digest( UCHAR* pSecret, int lenSecret,
					  UCHAR* pData, int len, UCHAR* pDigest );

#ifdef __cplusplus
}
#endif

// Exclude the rest of this file if not compiling C++
#ifdef __cplusplus

class LtMD5
{
public:
	LtMD5()
	{
		m_bLocalSecretSet = false;
		m_bAuthenticating = false;
		m_bEIA852Auth	  = false;
		memset(m_LocalSecret, 0, sizeof(m_LocalSecret));
	}

	enum
	{
		LTMD5_DIGEST_LEN = 16,
		LTMD5_SECRET_LEN = 16
	};
	boolean isAuthenticating()
	{	return m_bAuthenticating;
	}
	void setAuthenticating( boolean bEnable )
	{
		m_bAuthenticating = bEnable;
	}
	void setEIA852Auth(boolean bEnable)
	{
		m_bEIA852Auth = bEnable;
	}
	boolean isEIA852Auth()
	{
		return m_bEIA852Auth;
	}

	static void	digest(byte* pSecret, int lenSecret, byte* pData, int len, byte* pDigest, boolean bEIA852Auth);
	static boolean checkDigest(byte* pSecret, int lenSecret, byte* pData, int len, byte* pDigest, boolean bEIA852Auth);

	void setSecret( byte* pSecret )
	{
		memcpy(m_LocalSecret, pSecret, LTMD5_SECRET_LEN);
		m_bLocalSecretSet = true;
	}
	void getSecret(byte* pSecret)
	{
		memcpy(pSecret, m_LocalSecret, LTMD5_SECRET_LEN);
	}

	void digestWithSecret( byte* pData, int len, byte* pDigest )
	{
		assert(m_bLocalSecretSet);
		digest( m_LocalSecret, LTMD5_SECRET_LEN, pData, len, pDigest, m_bEIA852Auth);
	}
	boolean checkDigestWithSecret( byte* pData, int len, byte* pDigest,
									boolean otherAlgorithm = FALSE )
	{	boolean	bOk;

		assert(m_bLocalSecretSet);
		bOk = checkDigest(m_LocalSecret, LTMD5_SECRET_LEN, pData, len, pDigest,
			(otherAlgorithm ? !m_bEIA852Auth : m_bEIA852Auth));

		return bOk;
	}
protected:
	byte		m_LocalSecret[LTMD5_SECRET_LEN];
	boolean		m_bLocalSecretSet;
	boolean		m_bAuthenticating;
	boolean		m_bEIA852Auth;
};

#endif // __cplusplus

#endif // _LTMD5_H
