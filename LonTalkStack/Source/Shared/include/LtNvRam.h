//
// LtNvRam.h
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

#ifndef LTNVRAM_H
#define LTNVRAM_H

#define NVRAMDIR	"nvram"

//
// Store non-volatile ram items
// These are written often, and are small items
// Key values must be unique across all users
// Someday, if multiple processes are used, then
// this class should add a qualifier to the ram dir for the process
// Nobody outside of this class needs to set the path since
// NV ram items are not saved or backed up beyond a reboot.
// NV ram items should be lost on change of machine identity or
// ever restored from a backup. Better to have none, than have stale values.
//
class LtNvRam 
{
private:
	static char m_szPath[MAX_PATH];

public:
	LtNvRam();
					// return true if all is ok
	static boolean	set(const char* pKey, const byte* pValue, int nSize, boolean pathInKey);
					// return bytes obtained
	static int		get(const char* pKey, byte* pValue, int nSize, boolean pathInKey);
    static void deletePersistence(const char *pKey, boolean pathInKey);
protected:
	static void setPath(char* pFile, const char* pKey, boolean pathInKey);
};

// please use the class form of the members
// LtNvRam::set()
//extern LtNvRam ltNvRam;

#endif
