//
// Filename: IsiTypes.h
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
// Description: This file contains type definitions related to 
// Interoperable Self-Installation (ISI).
//

#ifndef __ISI_TYPES_H__
#   define  __ISI_TYPES_H__

#ifndef _LON_PLATFORM_H
#   error You must include LonPlatform.h first 
#endif  /* _LON_PLATFORM_H */
/*
 * *****************************************************************************
 * TITLE: ISI TYPES
 * *****************************************************************************
 *
 * Definitions of the enumerations and data types required by the Interoperable 
 * Self-Installation (ISI) API.
 */

/*
 * Enumeration: IsiApiError
 * IzoT ISI 1.0 API error codes.
 *
 * This enumeration contains all IzoT ISI 1.0 API error codes, including the 
 * code for success _IsiApiNoError_. 
 *
 */
typedef LON_ENUM_BEGIN(IsiApiError) 
{
    /*    0    */    IsiApiNoError = 0,           /* no error.  */
    /*    500  */    IsiNoConnectionSpace = 500,  /* No connection space or no more unused serial */
    /*    501  */    IsiEngineNotRunning = 501, 
    /*    502  */    IsiInvalidDomain = 502 
} LON_ENUM_END(IsiApiError);


#define ID_STR_LEN 8            // program ID length

/* 
 *  Enumeration: IsiMessageCode
 *  Enumeration for different ISI message codes.
 *
 *  This enumeration represents the possible ISI messages.
 */
typedef LON_ENUM_BEGIN(IsiMessageCode) 
{
    isiDrum    = 0x00,    	//  Domain resource usage information
	isiDrumEx  = 0x01,		// 	Extended domain resource usage information (must be isiDrum+1)
    isiCsmo    = 0x02,     	//  Connections: open enrollment
	isiCsmoEx  = 0x03,		// 	Extended connection open enrollment (must be isiCsmo+1)
    isiCsma    = 0x04,     	//  Connections: automatic open enrollment
	isiCsmaEx  = 0x05,		//  Extended automatic open enrollment (must be isiCsma+1)
    isiCsmr    = 0x06,     	//  Connections: automatic enrollment reminder
	isiCsmrEx  = 0x07,		//  Extended automatic open enrollment reminder (must be isiCsmr+1)

	isiLastEx  = isiCsmrEx,	//  Last extended command

    isiDidrq   = 0x08,    	//  Domain ID Request
    isiDidrm   = 0x09,    	//  Domain ID Response
    isiDidcf   = 0x0A,    	//  Domain ID Confirmation
    isiTimg    = 0x0B,     	//  Timing guidance message
    isiCsmx    = 0x0C,     	//  Connections: cancel enrollment
    isiCsmc    = 0x0D,     	//  Connections: close and confirm enrollment
    isiCsme    = 0x0E,     	//  Connections: enrollment acceptance
    isiCsmd    = 0x0F,     	//  Connections: connection deletion
    isiCsmi    = 0x10,     	//  Connections: status and resource info
    isiCtrq    = 0x11,      //  Controlled enrollment control request
    isiCtrp    = 0x12,      //  Controlled enrollment control response
    isiRdct    = 0x13,      //  Controlled enrollment read connection table request
    isiRdcs    = 0x14,      //  Controlled enrollment read connection table success
    isiRdcf    = 0x15,      //  Controlled enrollment read connection table failure    
	//	following are not codes but helpers:
	isiLastCode = isiRdcf,
	isiCodeMask = 0x1F
} LON_ENUM_END(IsiMessageCode);

/*
 *  Typedef: IsiMessageHeader
 *  Structure representing the ISI message header.
 *
 *  This structure contains the header that is sent with all ISI messages.
 */
typedef LON_STRUCT_BEGIN(IsiMessageHeader) 
{
    LON_ENUM(IsiMessageCode)  Code;
} LON_STRUCT_END(IsiMessageHeader);

/*
 *  Typedef: IsiDidrq
 *  Structure representing the domain request message (DIDRQ).
 *
 *  This structure contains the domain request message.
 */
typedef LON_STRUCT_BEGIN(IsiDidrq) 
{
    LonByte  NeuronId[LON_UNIQUE_ID_LENGTH]; // LonUniqueId NeuronId;                  // requestor's neuron id
    LonByte    Nuid;                       // requestor's non-unique id
} LON_STRUCT_END(IsiDidrq);

/*
 *  Typedef: IsiDidrm, IsiDidcf
 *  Structure representing the domain response message.
 *
 *  This structure contains the fields of a domain response message
 */

/*
 * Use the ISI_DID_LENGTH_* macros to access the Length field in IsiDidrm.Attributes1, IsiDidcf.Attributes1
 */
#define ISI_DID_LENGTH_MASK		0xE0
#define ISI_DID_LENGTH_SHIFT	5
#define ISI_DID_LENGTH_FIELD	Attributes1

typedef LON_STRUCT_BEGIN(IsiDidrm) 
{
    //DidLength   :   3;          // domain ID length: 1, 3, or 6
    //            :   5;          // reserved, set to zero
    LonByte     Attributes1;    // contains domain ID length: 1, 3, or 6.  See ISI_DID_LENGTH_* macros 
    LonDomainId DomainId;       // primary domain ID to use
    LonUniqueId NeuronId;       // DAS's neuron id
    LonByte     DeviceCountEstimate;
    LonByte     ChannelType;
} LON_STRUCT_END(IsiDidrm);

typedef IsiDidrm IsiDidcf;

/*
 *  Typedef: IsiDrum
 *  Structure representing the domain resource usage message (DRUM/DRUMEX).
 *
 *  This structure contains the fields of a domain domain resource usage message (DRUM/DRUMEX).
 */

/*
 * Use the ISI_DRUM_DIDLENGTH_* macros to access the DidLength field in IsiDrum.Attributes1.
 */
#define ISI_DRUM_DIDLENGTH_MASK		0xE0
#define ISI_DRUM_DIDLENGTH_SHIFT	5
#define ISI_DRUM_DIDLENGTH_FIELD	Attributes1

/*
 * Use the ISI_DRUM_USER_* macros to access the UserDefined field in IsiDrum.Attributes1.
 */
#define ISI_DRUM_USER_MASK		0x03
#define ISI_DRUM_USER_SHIFT	    0
#define ISI_DRUM_USER_FIELD	    Attributes1

typedef LON_STRUCT_BEGIN(IsiDrum) 
{
	//DidLength   :   3;          // domain ID length: 1, 3, or 6
	//            :   3;          // reserved, set to zero
	//UserDefined :   2;          // user-defined code
    LonByte        Attributes1;             // contains domain ID length: 1, 3, or 6, and used-defined code.  See ISI_DRUM_* macros 
	LonDomainId    DomainId;                // sender's primary domain ID
	LonUniqueId    NeuronId;                // sender's neuron id
	LonSubnetId    SubnetId;
	LonByte    NodeId;
	LonByte    Nuid;
	LonByte    ChannelType;
    LON_STRUCT_NESTED_BEGIN(Extended) 
    {
        LonWord DeviceClass;
	    LonByte    Usage;
    } LON_STRUCT_NESTED_END(Extended);
} LON_STRUCT_END(IsiDrum);

/*
 *  Typedef: IsiTimg
 *  Structure representing the timing guidance message (TIMG).
 *
 *  This structure contains the fields of a timing guidance message (TIMG).
 */

/*
 * Use the ISI_DRUM_DIDLENGTH_* macros to access the DidLength field in IsiDrum.Attributes1.
 */
#define ISI_TIMG_ORIG_MASK		0xF0
#define ISI_TIMG_ORIG_SHIFT     4
#define ISI_TIMG_ORIG_FIELD	    Attributes1

typedef LON_STRUCT_BEGIN(IsiTimg) 
{
    LonByte    Attributes1;                 // contains 8 for DAS.  See ISI_TIMG_ORIG_* macros 
    LonByte    DeviceCountEstimate;
    LonByte    ChannelType;
} LON_STRUCT_END(IsiTimg);

/*
 *  Typedef: HostUniqueId
 *  Holds the host's unique ID (derived from Neuron ID).
 */
typedef LonByte  HostUniqueId[LON_UNIQUE_ID_LENGTH-1];

/*
 *  Typedef: IsiCid
 *  Structure representing the unique connection ID.
 *
 *  This structure contains the unique connection ID for a connection.
 */
typedef LON_STRUCT_BEGIN(IsiCid) 
{
    HostUniqueId UniqueId;    // host's unique ID (derived from Neuron ID)
    LonWord      SerialNumber;
} LON_STRUCT_END(IsiCid);

/*
 *  Typedef: IsiConnectionHeader
 *  Structure representing the connection header.
 *
 *  Following the ISI Message Header, all connection-related messages start with
 *  this structure.
 */
typedef LON_STRUCT_BEGIN(IsiConnectionHeader) 
{
    IsiCid      Cid;
    LonWord     Selector;
} LON_STRUCT_END(IsiConnectionHeader);

/* 
 *  Enumeration: IsiScope
 *  Enumeration for scope of a connection message.
 *
 *	This enumeration represents the different values that can be used in
 *  the scope field of a CSMO message. This value represents the scope of the
 *  resource file containing the functional profile and network variable type
 *  definitions specified by the Profile and NvType fields.
 */
typedef LON_ENUM_BEGIN(IsiScope) 
{
    isiScopeStandard = 0,
    isiScopeManufacturer = 3
} LON_ENUM_END(IsiScope);

/* 
 *  Enumeration: IsiDirection
 *  Enumeration for the direction of a network variable in a connection.
 *
 *	This enumeration represents the direction of the network variable
 *  on offer in a CSMO.
 */
typedef LON_ENUM_BEGIN(IsiDirection) 
{
    isiDirectionOutput = 0,
    isiDirectionInput,
    isiDirectionAny,
    isiDirectionVarious
} LON_ENUM_END(IsiDirection);

typedef LonByte  ApplicationId[ID_STR_LEN-1];

/*
 *  Typedef: IsiCsmoData
 *  Structure representing the CSMO message.
 *
 *  This structure contains the fields of a CSMO message to be sent by the
 *  ISI engine.
 */

/*
 * Use the ISI_CSMO_DIR_* macros to access the Direction field in IsiCsmoData.Attributes1
 */
#define ISI_CSMO_DIR_MASK		0xC0
#define ISI_CSMO_DIR_SHIFT		6
#define ISI_CSMO_DIR_FIELD		Attributes1

/*
 * Use the ISI_CSMO_WIDTH_* macros to access the Width field in IsiCsmoData.Attributes1
 */
#define ISI_CSMO_WIDTH_MASK		0x3F
#define ISI_CSMO_WIDTH_SHIFT	0
#define ISI_CSMO_WIDTH_FIELD	Attributes1

/*
 * Use the ISI_CSMO_ACK_* macros to access the Acknowledged field in IsiCsmoData.Attributes2
 */
#define ISI_CSMO_ACK_MASK		0x80
#define ISI_CSMO_ACK_SHIFT		7
#define ISI_CSMO_ACK_FIELD		Attributes2

/*
 * Use the ISI_CSMO_POLL_* macros to access the Poll field in IsiCsmoData.Attributes2
 */
#define ISI_CSMO_POLL_MASK		0x40
#define ISI_CSMO_POLL_SHIFT		6
#define ISI_CSMO_POLL_FIELD		Attributes2

/*
 * Use the ISI_CSMO_SCOPE_* macros to access the Scope field in IsiCsmoData.Attributes2
 */
#define ISI_CSMO_SCOPE_MASK		0x30
#define ISI_CSMO_SCOPE_SHIFT	4
#define ISI_CSMO_SCOPE_FIELD	Attributes2

typedef LON_STRUCT_BEGIN(IsiCsmoData)
{
    /* The group (or: device category) that this connection applies to. */
    LonByte		Group;
	LonByte		Attributes1;	/* contains Direction, Width. See ISI_CSMO_DIR_* and _WIDTH_* macros */
    /* Functional profile number of the functional profile that defines the 
       functional block containing this input or output, or zero if none. */
	LonWord		Profile;
    /* NV type index of the NV type for the network variable, or zero if none
       specified. The NV type index is an index into resource file that defines the
       network variable type for the network variable on offer. */
    LonByte	    NvType;
    /* Variant number for the offered network variable. Variants can be defined for
       any functional profile/member number pair. */
    LonByte	    Variant;
	LON_STRUCT_NESTED_BEGIN(Extended) 
	{
        LonByte		Attributes2;	/* contains Ack, Poll, Scope. See ISI_CSMO_ACK_*, _POLL_* and _SCOPE_* macros */
        /* The first 6 bytes of the connection host’s standard program ID. 
           The last two standard program ID bytes (channel type and model
           number) are not included. */
		LonByte		Application[LON_PROGRAM_ID_LENGTH - 2];
        /* NV member number within the functional block, or zero if none. */
		LonByte	    Member;
    } LON_STRUCT_NESTED_END(Extended);    
} LON_STRUCT_END(IsiCsmoData);

/*
 *  Typedef: IsiCsmo
 *  Structure representing the manual open enrollment message (CSMO).
 *
 *  This structure contains a manual open enrollment message (CSMO).
 */
typedef LON_STRUCT_BEGIN(IsiCsmo) 
{
    IsiConnectionHeader Header;
    IsiCsmoData         Data;
} LON_STRUCT_END(IsiCsmo);

// Literals for accessing Desc field of IsiCsmi without using bitfields (for efficiency)
#define CsmiOffset_SHIFT	2
#define CsmiOffset_MASK     0xFC
#define CsmiOffset_FIELD    Attributes1
#define CsmiCount_SHIFT	    0
#define CsmiCount_MASK  	0x03
#define CsmiCount_FIELD  	Attributes1


typedef LON_STRUCT_BEGIN(CsmiDesc) 
{
	//unsigned Offset: 6;
	//unsigned Count:  2;
    LonByte Attributes1;  // contains Offset,  Count
} LON_STRUCT_END(CsmiDesc);

/*
 *  Typedef: IsiCsmi
 *  Structure representing the enrollment information message (CSMI) sent by the ISI engine.
 *
 *  This structure contains an enrollment information message (CSMI) sent by the ISI engine.
 */
typedef LON_STRUCT_BEGIN(IsiCsmi) 
{
    IsiConnectionHeader Header;
	LON_UNION_BEGIN(Desc) 
    {        
		CsmiDesc Bf;
		LonByte OffsetCount;
    } LON_UNION_END(Desc);
} LON_STRUCT_END(IsiCsmi);

typedef IsiConnectionHeader IsiCsmx;
typedef IsiConnectionHeader IsiCsmc;
typedef IsiConnectionHeader IsiCsme;
typedef IsiConnectionHeader IsiCsmd;
typedef IsiCsmo    IsiCsma;
typedef IsiCsmo    IsiCsmr;

/* 
 *  Enumeration: IsiControl
 *  Specifies the requested operation for a controlled enrollment request contained in 
 *  a control request (CTRQ) message.
 *
 */
typedef LON_ENUM_BEGIN(IsiControl) 
{
    isiNoop     =   0,
    isiOpen     =   1,
    isiCreate   =   2,
    isiExtend   =   3,
    isiCancel   =   4,
    isiLeave    =   5,
    isiDelete   =   6,
    isiFactory  =   7
} LON_ENUM_END(IsiControl);

typedef LON_STRUCT_BEGIN(IsiCtrq) 
{
    LON_ENUM(IsiControl) Control;
    LonByte    Parameter;
} LON_STRUCT_END(IsiCtrq);

typedef LON_STRUCT_BEGIN(IsiCtrp) 
{
    LonUbits8         Success;
    LonUniqueId     NeuronID;
} LON_STRUCT_END(IsiCtrp);

//  IsiConnectionTable
//  Notice the connection table state values are an ordered enumeration, with
//  isiConnectionStateUnused < isiConnectionStatePending < isiConnectionStateInUse
typedef LON_ENUM_BEGIN(IsiConnectionState) 
{
    isiConnectionStateUnsed = 0,
    isiConnectionStatePending,
    isiConnectionStateInUse,
    isiConnectionStateTcsmr
} LON_ENUM_END(IsiConnectionState);

// Literals for accessing Desc field of IsiConnection without using bitfields (for efficiency)
#define ConnectionOffset_SHIFT	2
#define ConnectionOffset_MASK	0xFC
#define ConnectionOffset_FIELD	Attributes1
#define ConnectionAuto_SHIFT		1
#define ConnectionAuto_MASK		0x02
#define ConnectionAuto_FIELD    Attributes1

typedef LON_STRUCT_BEGIN(ConnDesc) 
{
	//unsigned    Offset : 6;
	//unsigned    Auto  : 1;
	//unsigned          : 1;    // reserved for future use
    LonByte        Attributes1;  // contains Offset, Auto
} LON_STRUCT_END(ConnDesc);

/*
 *  Typedef: IsiConnection
 *  Structure representing a row in the connection table.
 *
 *  This structure is used to represent a row in the connection table that
 *  is returned by <IsiGetConnection> and is used in <IsiSetConnection> to set a
 *  row in the table.
 */

/*
 * Use the ISI_CONN_STATE_* macros to access the State field in IsiConnection.Attributes1
 */
#define ISI_CONN_STATE_MASK		0xC0
#define ISI_CONN_STATE_SHIFT	6
#define ISI_CONN_STATE_FIELD	Attributes1

 /*
 * Use the ISI_CONN_EXTEND_* macros to access the Extend field in IsiConnection.Attributes1
 */
#define ISI_CONN_EXTEND_MASK	0x20
#define ISI_CONN_EXTEND_SHIFT	5
#define ISI_CONN_EXTEND_FIELD	Attributes1

/*
 * Use the ISI_CONN_CSME_* macros to access the State field in IsiConnection.Attributes1
 */
#define ISI_CONN_CSME_MASK		0x10
#define ISI_CONN_CSME_SHIFT		4
#define ISI_CONN_CSME_FIELD	Attributes1

/*
 * Use the ISI_CONN_WIDTH_* macros to access the State field in IsiConnection.Attributes1
 */
#define ISI_CONN_WIDTH_MASK		0x0F
#define ISI_CONN_WIDTH_SHIFT	0
#define ISI_CONN_WIDTH_FIELD	Attributes1

typedef LON_STRUCT_BEGIN(IsiConnection) 
{
    IsiConnectionHeader Header;
    LonByte    Host;           //  local assembly that is hosted here, or ISI_NO_ASSEMBLY if this is not the host for this connection
    LonByte    Member;         //  local assembly that is enrolled in this connection, or ISI_NO_ASSEMBLY if none.
    LonByte		Attributes1;	/* contains state, extend, csme, width. See ISI_CONN_STATE_*, _EXTEND_*, _CSME_* and _WIDTH_* macros */
    LON_UNION_BEGIN(Desc) 
	{
		ConnDesc Bf;
		LonByte OffsetAuto;
    } LON_UNION_END(Desc);
} LON_STRUCT_END(IsiConnection);

typedef LON_STRUCT_BEGIN(IsiRdct) 
{
    LonByte    Index;
    LonByte    Host;
    LonByte    Member;
} LON_STRUCT_END(IsiRdct);

typedef LON_STRUCT_BEGIN(IsiRdcs) 
{
    LonByte    Index;
    IsiConnection Data;
} LON_STRUCT_END(IsiRdcs);

//
//  ISI Message Structure
//
typedef LON_STRUCT_BEGIN(IsiMessage) 
{
    IsiMessageHeader Header;
    LON_UNION_BEGIN(Msg) 
    {
        IsiDidrq   Didrq;
        IsiDidrm   Didrm;
        IsiDidcf   Didcf;
        IsiTimg    Timg;
        IsiDrum    Drum;
        IsiCsmo    Csmo;
        IsiCsmx    Csmx;
        IsiCsmc    Csmc;
        IsiCsmd    Csmd;
        IsiCsme    Csme;
        IsiCsmi    Csmi;
        IsiCsma    Csma;
        IsiCsmr    Csmr;
        IsiCtrq    Ctrq;
        IsiCtrp    Ctrp;
        IsiRdct    Rdct;
        IsiRdcs    Rdcs; 
    } LON_UNION_END(Msg);
} LON_STRUCT_END(IsiMessage);

// The following constant defines the 709.1 (LonTalk) application message code used
// with all ISI management messages. The structure of these messages is IsiMessage,
// as defined above, which contains an ISI specific IsiMessageCode to further 
// distringuish the individual ISI message types. 
// DO NOT CONFUSE THE 709.1 APPLICATION MESSAGE CODE WITH THE IsiMessageCode enumeration!
extern LonByte const isiApplicationMessageCode;

//  Basic ISI API macros
#define ISI_DEFAULT_GROUP   128u
#define ISI_NO_ASSEMBLY     255u
#define ISI_NO_INDEX        255u
#define ISI_TICKS_PER_SECOND   4u
#define ISI_DEFAULT_CONTAB_SIZE     32u		// was 8
#define ISI_DEFAULT_REPEATS			3
#define ISI_DEFAULT_DOMAIN_ID       { 'I', 'S', 'I' }
#define ISI_DEFAULT_DOMAIN_ID_LEN   3u


//  ***********************************************************************
//  Basic ISI entry points
//  ***********************************************************************

//  Starting / Stopping / Running the ISI engine. Use a combination of the
//  IsiFlags with the Start functions.
typedef LON_ENUM_BEGIN(IsiFlags) 
{
    isiFlagNone               =   0x00,   // does nothing
    isiFlagExtended 		  =   0x01,   // enables use of extended DRUM and enrollment messages
    isiFlagHeartbeat          =   0x02,   // enables ISI NV heartbeats
    isiFlagApplicationPeriodic =  0x04,	  // enables IsiApplicationPeriodic()
	isiFlagSupplyDiagnostics  =   0x08,   // enables UpdateDiagnostics callback
    isiControlledEnrollment   =   0x10,   // enables controlled enrollment  
    isiFlagDisableAddrMgmt    =   0x20    // always assign a randomly allocated primary address
} LON_ENUM_END(IsiFlags);

typedef LON_ENUM_BEGIN(IsiType) 
{
    isiTypeS,                       //  use for ISI-S and ISI-S/C
    isiTypeDa,                      //  use for ISI-DA and ISI-DA/C
    isiTypeDas                      //  use for ISI-DAS and ISI-DAS/C
} LON_ENUM_END(IsiType);

typedef LON_ENUM_BEGIN(IsiEvent)
{
    isiNormal    =   0,		// Some code ASSUMES this value.
    isiRun       =   1,
    //  following are events related to connection enrollment:
    isiPending,
    isiApproved,
    isiImplemented,
    isiCancelled,
    isiDeleted,
    isiWarm,
    isiPendingHost,
    isiApprovedHost,
    //  following are events related to domain and device acquisition:
    isiAborted, // see param for IsiAbortReason detail
    isiRetry,   // see param for remaining number of retries
    isiWink,    // device should perform Wink operation
    isiRegistered   // successful start (param 0) or completion (param 0xFF) of device or domain acquisition
} LON_ENUM_END(IsiEvent);

typedef LON_ENUM_BEGIN(IsiAbortReason)
{
    // following are abort reasons from the process that controls the domain acquisition process
    // on a ISI-DA device:
    isiAbortUnsuccessful = 1,        // abort domain acq. after 20 retries
    isiAbortMismatchingDidrm,       // abort domain acq. due to arrival of mismatching DIDRM
    isiAbortMismatchingDidcf,       // abort domain acq. due to arrival of mismatching DIDCF
    isiAbortMismatchService         // abort domain acq. due to mismatching confirmation service msg
} LON_ENUM_END(IsiAbortReason);

extern void IsiUpdateUserInterface(IsiEvent Event, LonByte Parameter);

typedef LON_ENUM_BEGIN(IsiDiagnostic)
{
    isiSubnetNodeAllocation = 1,
    isiSubnetNodeDuplicate,
    isiReceiveDrum = 4,
    isiReceiveTimg,
    isiSendPeriodic,
    isiSelectorDuplicate,
    isiSelectorUpdate,
    isiReallocateSlot
} LON_ENUM_END(IsiDiagnostic);

// Macro to get the bit value
#define GET_BITS_VALUE(field, MASK, SHIFT)          ((field & MASK) >> SHIFT)
#define SET_BITS_VALUE(field, MASK, SHIFT, value)   field = (field & ~MASK) | (value << SHIFT)


/*
 *  Type definitions for callback and event handers
 */
typedef void (*IsiCreateCsmoFunction)(unsigned Assembly, IsiCsmoData* const pCsmoData);
typedef LonBool (*IsiCreatePeriodicMsgFunction) (void);
typedef unsigned (*IsiGetAssemblyFunction)(const IsiCsmoData* pCsmoData, LonBool Auto, unsigned Assembly);
typedef unsigned (*IsiGetNvIndexFunction)(unsigned Assembly, unsigned Offset, unsigned PreviousIndex);
typedef unsigned (*IsiGetPrimaryGroupFunction)(unsigned Assembly);
typedef unsigned (*IsiGetWidthFunction)(unsigned Assembly);
typedef LonBool (*IsiQueryHeartbeatFunction) (unsigned NvIndex);
typedef LonBool (*IsiUpdateDiagnosticsFunction) (IsiDiagnostic Event, LonByte Parameter);
typedef void (*IsiUpdateUserInterfaceFunction) (IsiEvent Event, LonByte Parameter);

typedef struct
{
    IsiCreateCsmoFunction               createCsmo; 
    IsiCreatePeriodicMsgFunction        createPeriodicMsg;
    IsiGetAssemblyFunction              getAssembly;
    IsiGetNvIndexFunction               getNvIndex;
    IsiGetPrimaryGroupFunction          getPrimaryGroup;
    IsiGetWidthFunction                 getWidth;
    IsiQueryHeartbeatFunction           queryHeartbeat;
    IsiUpdateDiagnosticsFunction        updateDiagnostics;
    IsiUpdateUserInterfaceFunction      updateUserInterface;

}   IsiCallbackVectors;



#endif  //  !defined __ISI_TYPES_H__
