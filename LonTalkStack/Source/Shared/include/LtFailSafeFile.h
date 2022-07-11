/*
 * File: LtFailSafeFile.h
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
 * Description:
 * Read or write a persistent data file using a multi-step process to guarantee that
 * changes are atomic. This is to make sure the file remains consistent in the event 
 * of power-failures or other asynchronous resets in the middle of these operations. 
 */


#ifndef LTFAILSAFEFILE_H
#define	LTFAILSAFEFILE_H

#ifndef boolean
typedef unsigned char boolean;
#endif
#ifndef byte
typedef unsigned char byte;
#endif

class LtFailSafeFile
{
public:
	LtFailSafeFile();
	virtual ~LtFailSafeFile();

	struct FileParts
	{
		const byte* pData;
		int nSize;
	};
	boolean GetSafeFile(const char* path);
	int ReadSafeFile(const char* path, byte* pData, int nSize);
	boolean WriteSafeFile(const char* path, const byte* pData, int nSize);
	boolean WriteSafeFile(const char* path, const FileParts* parts);
	const char *GetTempFileEnding();
	int GetTempFileEndingLength() { return m_tempFileEndingLength; }
	void GetTempFilePath(const char* basePath, char* tempPath) { strcpy(tempPath, basePath); strcat(tempPath, m_tempFileEnding); }
	
	// Updates "path" to reflect final official file name
	boolean MakeTempFileOfficialCopy(char* path);
	void Trace(boolean bTrace) { m_bTrace = bTrace; }

	static void Shutdown(void);

protected:
	boolean WriteSafeFileParts(const char* path, const FileParts* parts);

	boolean m_bTrace;

	static const char * const m_newFileEnding;
	static const char * const m_oldFileEnding;
	static const char * const m_tempFileEnding;
	static const int m_tempFileEndingLength;
};

#endif	// LTFAILSAFEFILE_H
