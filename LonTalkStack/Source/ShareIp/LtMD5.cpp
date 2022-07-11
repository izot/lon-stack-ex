/***************************************************************
 *  Filename: LtMD5.cpp
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
 *  Description:  implementation of the LtMD5 class.
 *
 *	DJ Duffy July 1999
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/ShareIp/LtMD5.cpp#1 $
//
/*
 * $Log: /Dev/ShareIp/LtMD5.cpp $
 *
 * 7     7/27/04 12:52p Fremont
 * EPRS FIXED: 34053 - md5 hash of text string doesn't work.
 * NULL first parameter (for secret) was halting hashing.
 *
 * 6     6/10/03 5:37p Fremont
 * changes for EIA0852 auth, remove global auth settings
 *
 * 5     3/14/02 3:05p Fremont
 * Make the simple digest routine calleable from C
 *
 * 4     2/05/02 5:25p Fremont
 * clean up MD5 includes
 *
 * 2     11/12/99 3:19p Darrelld
 * Updates for static secret
 *
 *
 */
#include <vxWorks.h>
#include <VxlTypes.h>
#include <md5.h>
#include <LtMD5.h>
#include <string.h>

struct md5DigestDataListElement
{
	const UCHAR* pData;
	UINT len;
};

// Zero-filled space the size of the digest
static const UCHAR md5EIA852ZeroDigest[LtMD5::LTMD5_DIGEST_LEN] = {0};

void md5CustomDigest(md5DigestDataListElement *pDataList, UCHAR* pDigest)
{
	MD5_CTX		ctx;

	MD5Init( &ctx );
	while ((pDataList != NULL) && (pDataList->pData != NULL))
	{
		MD5Update( &ctx, const_cast<unsigned char *>(pDataList->pData), pDataList->len);
		pDataList++;
	}
	MD5Final( pDigest, &ctx );
}

// C callable routine, also used by LtMD5::digest
// Original authentication order
void	md5Digest( UCHAR* pSecret, int lenSecret,
					  UCHAR* pData, int len, UCHAR* pDigest )
{
	md5DigestDataListElement dataList[3];

	// If user passes a NULL pSecret, fake a valid zero-length element
	if (pSecret == NULL)
	{
		pSecret = (UCHAR*)"";
		lenSecret = 0;
	}
	dataList[0].pData = pSecret;
	dataList[0].len = lenSecret;
	dataList[1].pData = pData;
	dataList[1].len = len;
	dataList[2].pData = NULL;	// terminator

	md5CustomDigest(dataList, pDigest);
}

// C callable routine, also used by LtMD5::digest
// Original authentication order
void	md5DigestEIA852( UCHAR* pSecret, int lenSecret,
					  UCHAR* pData, int len, UCHAR* pDigest )
{
	md5DigestDataListElement dataList[4];

	// If user passes a NULL pData, fake a valid zero-length element
	if (pData == NULL)
	{
		pData = (UCHAR*)"";
		len = 0;
	}
	dataList[0].pData = pData;
	dataList[0].len = len;
	dataList[1].pData = md5EIA852ZeroDigest;
	dataList[1].len = LtMD5::LTMD5_DIGEST_LEN;
	dataList[2].pData = pSecret;
	dataList[2].len = lenSecret;
	dataList[3].pData = NULL;	// terminator

	md5CustomDigest(dataList, pDigest);
}

//
// digest
//
// create an MD5 digest of a secret and some data
//
void LtMD5::digest( byte* pSecret, int lenSecret, byte* pData, int len, byte* pDigest, boolean bEIA852Auth)
{
	if (bEIA852Auth)
		md5DigestEIA852(pSecret, lenSecret, pData, len, pDigest);
	else
		md5Digest(pSecret, lenSecret, pData, len, pDigest);
}

//
// checkDigest
//
// Check a digest of a secret and a packet
//
boolean	LtMD5::checkDigest( byte* pSecret, int lenSecret,
						   byte* pData, int len, byte* pDigest, boolean bEIA852Auth)
{
	byte	aNewDigest[LTMD5_DIGEST_LEN];

	digest( pSecret, lenSecret, pData, len, aNewDigest, bEIA852Auth);
	return 0 == memcmp( aNewDigest, pDigest, LTMD5_DIGEST_LEN );
}
