/***************************************************************
 *  Filename: LtIpPersist.cpp
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
 *  Description:  implementation of the LtIpPersist class.
 *
 *	DJ Duffy Feb 1999
 *
 ****************************************************************/
//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/ShareIp/LtIpPersist.cpp#1 $
//
/*
 * $Log: /Dev/ShareIp/LtIpPersist.cpp $
 * 
 * 15    12/12/03 11:24a Fremont
 * EPR 31227 - Use the fail-safe file mechanism to guarantee modifications
 * to disk will not corrupt or completely lose the file if a power-fail or
 * reset occurs in the middle.
 * 
 * 14    8/19/03 5:06p Fremont
 * add 'morePath' param to makePath(), reorder params, rename some vars
 * 
 * 13    7/11/03 3:32p Karlo
 * makePath is more generalized.  It can take an index and append it to
 * the passed-in filename.  Only filename that come in without an
 * extension will ge the default .dat appended.
 * 
 * 12    7/10/03 6:11p Karlo
 * makePath does not append .dat extension if passed in name already has
 * one.  This make it usable for XML filenames
 * 
 * 10    1/07/00 3:37p Darrelld
 * Fix includes for PC
 * 
 * 9     1/06/00 2:05p Darrelld
 * Make the dir before writing persistence
 * 
 * 8     9/09/99 12:25p Darrelld
 * Remove some obsolete code
 * 
 * 7     7/30/99 5:03p Darrelld
 * Fix includes
 * 
 * 6     7/30/99 4:47p Fremont
 * Change file path root
 * 
 * 5     7/14/99 11:02a Darrelld
 * Use platform persistance path
 * 
 * 4     7/06/99 1:05p Darrelld
 * Fix path for vxWorks
 * 
 * 3     7/02/99 8:45a Darrelld
 * Target specific files
 * 
 * 2     2/22/99 10:57a Darrelld
 * Checkin after testing
 * 
 * 1     2/22/99 9:26a Darrelld
 * 
 * 
 */
#include <LtRouter.h>
#include <LtStackInternal.h>
#include "LtIpPersist.h"
#include "LtIpPlatform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LtFailSafeFile.h"



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

LtIpPersist::LtIpPersist()
{

}

LtIpPersist::~LtIpPersist()
{

}

#define PATHSIZE 300

//
// makePath
//
void		LtIpPersist::makePath( char* pPath, const char* pName, int index, const char *morePath)
{
	// Split the extension off and save it
	char pad[MAX_PATH];
	char incomingExt[MAX_PATH];
	strncpy(pad,pName,sizeof(pad));
	char * pDot = strrchr(pad,'.');
	if(pDot)
	{
		strcpy(incomingExt,pDot);
		*pDot = '\0';
	} else
		incomingExt[0] = '\0';

	LtPlatform::getPersistPath( pPath, MAX_PATH );
	if (morePath != NULL)
		strcat(pPath, morePath);
	if (pPath[strlen(pPath)-1] != DIR_SEPARATOR_CHAR)
        strcat(pPath, DIR_SEPARATOR_STRING);

	strcat( pPath, "LTIP_" );
	strcat( pPath, pad );
	// KO. If not instance 0, append the instance number to the filename
	if (index != 0)
	{	
		char indexStr[MAX_PATH];
		sprintf(indexStr, "%d", index );
		strcat(pPath,indexStr);
	}
	// KO. If the filename came in with an extension put it back
	if (incomingExt[0] != '\0')
	{
		strcat( pPath,incomingExt);
	} else
		strcat( pPath, ".dat");
}


//
// readFile
//
boolean		LtIpPersist::readFile( char* pName, byte** ppData, int* pNbytes )
{
	// just use fopen to store stuff on windows right now
	FILE*		fp = NULL;
	char		path[PATHSIZE];
	ULONG		nSize;
	ULONG		nBytes;
	ULONG		nSizeOnDisk = 0;
	boolean		bOk = false;
	byte*		pData = NULL;
	LtFailSafeFile fsf;

	*ppData = NULL;
	*pNbytes = 0;

	makePath( path, pName );

	// First, run the "fail-safe" algorithm to get a safe copy of the file
	fsf.GetSafeFile(path);

	LtIpFileSize(path, &nSizeOnDisk);	

	// read binay data
	fp = fopen( path, "rb" );
	while ( fp )
	{
		nBytes = fread( &nSize, sizeof(ULONG), 1, fp );
		if ( nBytes != 1 )	break;
		if ( nSize > nSizeOnDisk) break;
		pData = (byte*)malloc( nSize );
		if ( pData == NULL ) break;
		nBytes = fread( pData, 1, nSize, fp );
		if ( nBytes != nSize )	break;
		*ppData = pData;
		pData = NULL;
		*pNbytes = nBytes;
		bOk = true;
		break;
	}
	if ( pData )
	{	free( pData );
	}
	if ( fp )
	{	fclose( fp );
	}
	return bOk;
}



//
// writeFile
//
boolean		LtIpPersist::writeFile( char* pName, byte* pData, int nDataSize )
{

	// just use fopen to store stuff on windows right now
	char		path[PATHSIZE];
	ULONG		nSize = nDataSize;
	boolean		bOk = false;
	LtFailSafeFile fsf;
	LtFailSafeFile::FileParts fileParts[3];

	// Create the directory in case some fool deleted the directory.
	LtPlatform::getPersistPath( path, PATHSIZE );
	LtIpMakeDir(path);

	makePath( path, pName );

	// Write binay data, using the "fail-safe" mechanism
	// The file is written in two parts
	memset(fileParts, 0, sizeof(fileParts));	// for termination of list
	fileParts[0].pData = (byte*)(&nSize);
	fileParts[0].nSize = sizeof(nSize);
	if (nDataSize != 0)
	{
		fileParts[1].pData = pData;
		fileParts[1].nSize = nSize;
	}

	bOk = fsf.WriteSafeFile(path, fileParts);

	return bOk;
}


//
// deleteFile
//
boolean		LtIpPersist::deleteFile( char* pName )
{

	char		path[PATHSIZE];
	int			sts;
	
	makePath( path, pName );
	sts = remove( path );
	return sts != -1;
}




// end
