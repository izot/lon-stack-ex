/*
 * ErrorTypes.h
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
 * Error enums taken from the EDC global header file (global.h/GlobalApplDef.h)
 */

#ifndef _ERRORTYPES_H
#define	_ERRORTYPES_H

typedef enum EStatus
{
  eError=-1,
  eOK=0
}
EStatus;

typedef enum EFunctionError
{
  eFENone=0,              //0
  eFEUnknownFunctionCall, //1
  eFEParameterError,      //2
  eFEParseXMLError,       //3
  eFETagMissing,          //4
  eFEIndexMissing,        //5
  eFEIndexNotFound,       //6
  eFEIndexInvalid,        //7
  eFECanNotCreate,        //8      
  eFECanNotDelete,        //9
  eFECanNotModify,        //10
  eFEFormatError,         //11
  eFECommandFailed,       //12
  eFEWrongDataPointIndex, //13
  eFENameNotFound,        //14
  eFENoData,              //15
  eFEFieldNameNotFound,   //16
  eFEMallocFailed = 30,   // 30
  eFECannotOpenFile,      // 31
  eFECannotWriteFile,     // 32
  eFECannotReadFile,      // 33
  eFECannotDeleteFile,    // 34
  eFEDivisionByZero,      // 35
  eFEFatalError,          // 36
  eFEInvalidOrOfflineDP,  // 37        
  eFEVersionError,        // 38

  eFENotConnected=50,     // 50
  eFEFinished,            // 51

  eFENULL=99999
}
EFunctionError;


#endif	// _ERRORTYPES_H
