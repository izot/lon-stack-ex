//	Start.c	implementing IsiStart
//
// Copyright © 2005-2022 Dialog Semiconductor
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
//	Revision 0, February 2005, Bernd Gauweiler

#include <stdio.h>
#include <stdarg.h>
#include "isi_int.h"
#include <time.h>

static FILE *fpIsiTracefile = NULL;
LonBool printTimeStamp = TRUE;

/*
 *  Function: IsiSetTracefile
 *  Sets the filename to use for the trace logging.  When the pFilename is NULL, trace logging stops and a previously opened 
 *  tracefile is closed. 
 *  
 *  Parameters:
 *  pFilename - the filename for the trace logging.  Sets to NULL to stop the trace logging.
 *  append    - if it is set to true, it means to open file for writing at the end of the file (appending) without removing the EOF marker
 *              before writing new data to the file; creates the file first if it doesn't exist.
 *              if it is set to false, it means to open an empty trace file for writing. If the given file exists, 
 *              its contents are destroyed.
 *
 *  Returns:     
 *  none 
 *
 */
void IsiSetTracefile(const char* pFilename, LonBool append)
{
    // close the previous trace log (if exists)
    if (fpIsiTracefile != NULL)
    {
        fclose(fpIsiTracefile);
        fpIsiTracefile = NULL;
    }
    if (pFilename != NULL)
    {
        const char *openStr = append ? "a" : "w";
        fpIsiTracefile = fopen(pFilename, openStr);
        if (fpIsiTracefile != NULL && append)
        {	// insert a separator
        	fprintf(fpIsiTracefile, "\n\n\t===\n\n");
        }
    }
}

/*=============================================================================
 *                           SUPPORT FUNCTIONS                                *
 *===========================================================================*/
void _IsiAPIDebug(const char * fmt, ...)
{
    va_list args;

    if (fpIsiTracefile != NULL)
    {
        if (printTimeStamp)
        {
            // Print time stamp
            char *pAsctimeStr;  
            char *pos;

            time_t dateTime;
            time(&dateTime);  // Get time in seconds
            pAsctimeStr = asctime(localtime(&dateTime));  // get local time as string
            // Remove the \n 
            pos = strchr(pAsctimeStr, '\n');
            *pos = '\0';
            fprintf(fpIsiTracefile, "[%s]", pAsctimeStr);
        }

        va_start(args, fmt);
        vfprintf(fpIsiTracefile, fmt, args);
        va_end(args);
        fflush(fpIsiTracefile);
    }
}

void _IsiAPIDump(const char* prefix, void* me, unsigned bytes, const char* postfix)
{
	unsigned char* data = (unsigned char *)me;
	char* buffer = (char*)malloc(strlen(prefix) + bytes * 3 + strlen(postfix) + 5);
	LonBool timestamp = printTimeStamp;

	printTimeStamp = FALSE;

	strcpy(buffer, prefix);
	while (bytes--) {
		sprintf(buffer + strlen(buffer), "%02X.", (unsigned)*data++);
	}
	sprintf(buffer + strlen(buffer) - 1, "%s", postfix);
	_IsiAPIDebug(buffer);

	printTimeStamp = timestamp;

	free(buffer);
}


//	end of trace.c
