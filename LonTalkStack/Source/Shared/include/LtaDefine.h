#ifndef _LTADEFINE_H
#define _LTADEFINE_H

// LtaDefine.h
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
#ifdef WIN32
#ifdef LTA_EXPORTS
#define LTA_EXTERNAL_CLASS		__declspec(dllexport)
#define LTA_EXTERNAL_FN			__declspec(dllexport) 
#else
#ifdef LTA_LIBRARY
#define LTA_EXTERNAL_CLASS
#define LTA_EXTERNAL_FN
#else
#define LTA_EXTERNAL_CLASS		__declspec(dllimport)
#define LTA_EXTERNAL_FN			__declspec(dllimport)
#endif
#endif
#else /* WIN32 */
/* VXWORKS ignores this stuff */
#define LTA_EXTERNAL_CLASS
#define LTA_EXTERNAL_FN
#endif /* WIN32 */

/* 
 * The following definitions are used to identify product lines. These 
 * are used for conditional compilation, such as
 * #if PRODUCT_IS(ILON)
 *   ...
 * #endif.
 */
#define PRODUCT_ID_ILON             0  /* ILON-100, 600, etc. */
#define PRODUCT_ID_VNISTACK         1  /* PC based app used by LNS, nodesim, etc. */
#define PRODUCT_ID_FTXL             2  /* FT XL based product */
#define PRODUCT_ID_DCX		        3  /* NES Data Concentrator */
#define PRODUCT_ID_LONTALK_STACK    4  /* LonTalk Stack software distributable */
#define PRODUCT_ID_LONTALK_ROUTER   5  /* LonTalk Router software distributable */
#define PRODUCT_ID_IZOT            6  /* Pylon software distributable */
#define PRODUCT_ID_LIFTBR           7  /* Lift Border Router */

#define PRODUCT_IS(pid) (PRODUCT_ID == PRODUCT_ID_ ## pid) 

/* 
 * The following macro is used to test a feature.  Macros are used 
 * rather than just using '#if <FEATURE_NAME>' so that a compiler 
 * error is generated if this file is not included.
 */
#define FEATURE_INCLUDED(feature) (INCLUDE_ ## feature)

    /* Standard persistence, uses a file system. */
#define PERSISTENCE_TYPE_STANDARD 0 
    /* FTXL persistence differs from standard persistence in a number of wasys:
     *   1) Does not depend on a file system.  Instead uses application defined 
     *      callbacks to perform I/O.
     *   2) Identifies segments using an enumeration, rather than file name.
     *   3) Supports only one stack - no app index is used.
     *   4) Performs all writes within a single task, rather than spawning
     *      a task for each persistence object.
     *   5) Does not depend on NVRAM.
     */
#define PERSISTENCE_TYPE_FTXL     1 

/* 
 * The following macro is used to test the persistence style
 * defined for the platform.
 */
#define PERSISTENCE_TYPE_IS(type) (PERSISTENCE_TYPE == PERSISTENCE_TYPE_ ## type)

/* Product IDs and features for each platform. */
#if defined(ILON_PLATFORM)
    #define PRODUCT_ID              PRODUCT_ID_ILON
    #define INCLUDE_IP852           1   
    #define INCLUDE_L5MIP           0   /* Implement L6 on top of a L5 MIP. */
    #define INCLUDE_MONITOR_SETS    1
    #define INCLUDE_STANDALONE_MGMT 1   /* Includes lontalk repeating */
    #define PERSISTENCE_TYPE        PERSISTENCE_TYPE_STANDARD
    #define PERSISTENCE_LOCATION    NULL
    #define INCLUDE_APP_CONTROL     1	/* 1 => You get all the LtStart.cpp fun stuff */
    #define INCLUDE_MULTI_APP	    1	/* 1 => You could have more than one stack app. */
    #define INCLUDE_NVRAM           1
    #define INCLUDE_LTEP            0 	/* LonTalk Enhanced Proxy, not needed because i.LON is not a repeating relay */
    #define INCLUDE_EMBEDDED        1	/* Embedded platform */
    #define INCLUDE_APP_INDEX_IN_PERSISTENCE_FILES 1 
    #define INCLUDE_INDEPENDENTLY_POWERED_INTERFACE 1
    #define LNI_CARD_NAME           CARD_TYPE_PLCC		// ECN card type name
#ifdef linux
#define LON_INTERFACE_USB 1
#endif
    #define INCLUDE_STDXCVR_FILE_SUPPORT 0 /* Extend support of XID interfaces using stdxcvr.xml */
    #define INCLUDE_LONLINK         1
    #define INCLUDE_XNVT_SUPPORT    1   /* 0 => Not included.  Workaround for issue where the LNS incorrectly swaps the array length for */
                                        /*      the newer APP_QUERY_NV_INFO:NV_INFO_DESCNV_INFO_DESC2, introduced for 2 byte SNVT IDs. */
                                        /* 1 => Support commands to query and define NVs with 2 byte SNVTIDs. Needed to support the ability to have SNVT IDs > 254 */
    #define INCLUDE_IZOT            0   /* LS/IP (or IzoT) protocol */

#elif defined(FTXL_PLATFORM)

    #define PRODUCT_ID              PRODUCT_ID_FTXL
    #define INCLUDE_IP852           0
    #define INCLUDE_L5MIP           0
    #define INCLUDE_MONITOR_SETS    0
    #define INCLUDE_STANDALONE_MGMT 0
    #define PERSISTENCE_TYPE        PERSISTENCE_TYPE_FTXL
    #define PERSISTENCE_LOCATION    NULL
    #define INCLUDE_APP_CONTROL     0
    #define INCLUDE_MULTI_APP	    0
    #define INCLUDE_NVRAM           0
    #define INCLUDE_LTEP            0
    #define INCLUDE_EMBEDDED        1
    #define INCLUDE_APP_INDEX_IN_PERSISTENCE_FILES 0 
    #define INCLUDE_INDEPENDENTLY_POWERED_INTERFACE 0
    #define INCLUDE_STDXCVR_FILE_SUPPORT 0 /* Extend support of XID interfaces using stdxcvr.xml */
    #define INCLUDE_LONLINK         1
    #define INCLUDE_XNVT_SUPPORT    1   /* 0 => Not included.  Workaround for issue where the LNS incorrectly swaps the array length for */
                                        /*      the newer APP_QUERY_NV_INFO:NV_INFO_DESCNV_INFO_DESC2, introduced for 2 byte SNVT IDs. */
                                        /* 1 => Support commands to query and define NVs with 2 byte SNVTIDs. Needed to support the ability to have SNVT IDs > 254 */
    #define INCLUDE_IZOT            0   /* LS/IP (or IzoT) protocol */

#elif defined(DCX_PLATFORM)

    #define PRODUCT_ID              PRODUCT_ID_DCX
    #define INCLUDE_IP852           0
    #define INCLUDE_L5MIP           0
    #define INCLUDE_MONITOR_SETS    0
    #define INCLUDE_STANDALONE_MGMT 0	
    #define PERSISTENCE_TYPE        PERSISTENCE_TYPE_STANDARD
    #define PERSISTENCE_LOCATION    NULL
    #define INCLUDE_APP_CONTROL     0
    #define INCLUDE_MULTI_APP	    1
    #define INCLUDE_NVRAM           1
    #define INCLUDE_LTEP            1
    #define INCLUDE_EMBEDDED        1
    #define INCLUDE_APP_INDEX_IN_PERSISTENCE_FILES 1 
    #define INCLUDE_ENCRYPTED_PASSWORDS 1
    #define INCLUDE_INDEPENDENTLY_POWERED_INTERFACE 1
    #define LNI_CARD_NAME           CARD_TYPE_PLCA			// ECN card type name
    #define INCLUDE_STDXCVR_FILE_SUPPORT 0 /* Extend support of XID interfaces using stdxcvr.xml */
    #define INCLUDE_LONLINK         1
    #define INCLUDE_XNVT_SUPPORT    1   /* 0 => Not included.  Workaround for issue where the LNS incorrectly swaps the array length for */
                                        /*      the newer APP_QUERY_NV_INFO:NV_INFO_DESCNV_INFO_DESC2, introduced for 2 byte SNVT IDs. */
                                        /* 1 => Support commands to query and define NVs with 2 byte SNVTIDs. Needed to support the ability to have SNVT IDs > 254 */
    #define INCLUDE_IZOT            0   /* LS/IP (or IzoT) protocol */


#elif defined(LONTALK_STACK_PLATFORM)

    #define PRODUCT_ID              PRODUCT_ID_LONTALK_STACK
    #define INCLUDE_IP852           0
    #define INCLUDE_L5MIP           0
    #define INCLUDE_MONITOR_SETS    0
    #define INCLUDE_STANDALONE_MGMT 0
    #define PERSISTENCE_TYPE        PERSISTENCE_TYPE_FTXL
    #define PERSISTENCE_LOCATION    NULL
    #define INCLUDE_APP_CONTROL     0
    #define INCLUDE_MULTI_APP	    0
    #define INCLUDE_NVRAM           0
    #define INCLUDE_LTEP            0
    #define INCLUDE_EMBEDDED        1
    #define INCLUDE_APP_INDEX_IN_PERSISTENCE_FILES 0 
    #define INCLUDE_INDEPENDENTLY_POWERED_INTERFACE 0
    #define INCLUDE_STDXCVR_FILE_SUPPORT 0 /* Extend support of XID interfaces using stdxcvr.xml */
    #define INCLUDE_LONLINK         1
    #define INCLUDE_XNVT_SUPPORT    1   /* 0 => Not included.  Workaround for issue where the LNS incorrectly swaps the array length for */
                                        /*      the newer APP_QUERY_NV_INFO:NV_INFO_DESCNV_INFO_DESC2, introduced for 2 byte SNVT IDs. */
                                        /* 1 => Support commands to query and define NVs with 2 byte SNVTIDs. Needed to support the ability to have SNVT IDs > 254 */
    #define INCLUDE_IZOT            0   /* LS/IP (or IzoT) protocol */


#elif defined(LONTALK_IP852_STACK_PLATFORM)

    #define PRODUCT_ID              PRODUCT_ID_LONTALK_STACK
    #define INCLUDE_IP852           1
    #define INCLUDE_L5MIP           0
    #define INCLUDE_MONITOR_SETS    0
    #define INCLUDE_STANDALONE_MGMT 0
    #define PERSISTENCE_TYPE        PERSISTENCE_TYPE_FTXL       
    #define PERSISTENCE_LOCATION    NULL
    #define INCLUDE_APP_CONTROL     0
    #define INCLUDE_MULTI_APP	    0
    #define INCLUDE_NVRAM           1
    #define INCLUDE_LTEP            0
    #define INCLUDE_EMBEDDED        1
    #define INCLUDE_APP_INDEX_IN_PERSISTENCE_FILES 0 
    #define INCLUDE_INDEPENDENTLY_POWERED_INTERFACE 0
    #define INCLUDE_STDXCVR_FILE_SUPPORT 0 /* Extend support of XID interfaces using stdxcvr.xml */
    #define INCLUDE_LONLINK         0
    #define INCLUDE_XNVT_SUPPORT    1   /* 0 => Not included.  Workaround for issue where the LNS incorrectly swaps the array length for */
                                        /*      the newer APP_QUERY_NV_INFO:NV_INFO_DESCNV_INFO_DESC2, introduced for 2 byte SNVT IDs. */
                                        /* 1 => Support commands to query and define NVs with 2 byte SNVTIDs. Needed to support the ability to have SNVT IDs > 254 */
    #define INCLUDE_IZOT            0   /* LS/IP (or IzoT) protocol */


#elif defined(LONTALK_ROUTER_PLATFORM)

    #define PRODUCT_ID              PRODUCT_ID_LONTALK_ROUTER
    #define INCLUDE_IP852           1
    #define INCLUDE_L5MIP           0
    #define INCLUDE_MONITOR_SETS    0
    #define INCLUDE_STANDALONE_MGMT 0
    #define PERSISTENCE_TYPE        PERSISTENCE_TYPE_STANDARD
    #define PERSISTENCE_LOCATION    NULL
    #define INCLUDE_APP_CONTROL     0
    #define INCLUDE_MULTI_APP	    1
    #define INCLUDE_NVRAM           1
    #define INCLUDE_LTEP            0
    #define INCLUDE_EMBEDDED        1
    #define INCLUDE_APP_INDEX_IN_PERSISTENCE_FILES 1 
    #define INCLUDE_INDEPENDENTLY_POWERED_INTERFACE 0
    #define INCLUDE_STDXCVR_FILE_SUPPORT 0 /* Extend support of XID interfaces using stdxcvr.xml */
    #define INCLUDE_LONLINK         1
    #define INCLUDE_XNVT_SUPPORT    1   /* 0 => Not included.  Workaround for issue where the LNS incorrectly swaps the array length for */
                                        /*      the newer APP_QUERY_NV_INFO:NV_INFO_DESCNV_INFO_DESC2, introduced for 2 byte SNVT IDs. */
                                        /* 1 => Support commands to query and define NVs with 2 byte SNVTIDs. Needed to support the ability to have SNVT IDs > 254 */
    #define INCLUDE_IZOT            0   /* LS/IP (or IzoT) protocol */


#elif defined(IZOT_IP852_PLATFORM)

    #define PRODUCT_ID              PRODUCT_ID_IZOT
    #define INCLUDE_IP852           1
    #define INCLUDE_L5MIP           0
    #define INCLUDE_MONITOR_SETS    0
    #define INCLUDE_STANDALONE_MGMT 0
    #define PERSISTENCE_TYPE        PERSISTENCE_TYPE_FTXL
    #define PERSISTENCE_LOCATION    NULL
    #define INCLUDE_APP_CONTROL     0
    #define INCLUDE_MULTI_APP	    0
    #define INCLUDE_NVRAM           1
    #define INCLUDE_LTEP            0
    #define INCLUDE_EMBEDDED        1
    #define INCLUDE_APP_INDEX_IN_PERSISTENCE_FILES 0 
    #define INCLUDE_INDEPENDENTLY_POWERED_INTERFACE 0
    #define INCLUDE_STDXCVR_FILE_SUPPORT 0 /* Extend support of XID interfaces using stdxcvr.xml */
    #define INCLUDE_LONLINK         0
    #define INCLUDE_XNVT_SUPPORT    1   /* 0 => Not included.  Workaround for issue where the LNS incorrectly swaps the array length for */
                                        /*      the newer APP_QUERY_NV_INFO:NV_INFO_DESCNV_INFO_DESC2, introduced for 2 byte SNVT IDs. */
                                        /* 1 => Support commands to query and define NVs with 2 byte SNVTIDs. Needed to support the ability to have SNVT IDs > 254 */
    #define INCLUDE_IZOT            0   /* LS/IP (or IzoT) protocol */


#elif defined(IZOT_PLATFORM)

    #define PRODUCT_ID              PRODUCT_ID_IZOT
    #define INCLUDE_IP852           1
    #define INCLUDE_L5MIP           0
    #define INCLUDE_MONITOR_SETS    0
    #define INCLUDE_STANDALONE_MGMT 0
    #define PERSISTENCE_TYPE        PERSISTENCE_TYPE_FTXL
    #define PERSISTENCE_LOCATION    NULL
    #define INCLUDE_APP_CONTROL     0
    #define INCLUDE_MULTI_APP	    0
    #define INCLUDE_NVRAM           1
    #define INCLUDE_LTEP            0
    #define INCLUDE_EMBEDDED        1
    #define INCLUDE_APP_INDEX_IN_PERSISTENCE_FILES 0 
    #define INCLUDE_INDEPENDENTLY_POWERED_INTERFACE 0
    #define INCLUDE_STDXCVR_FILE_SUPPORT 0 /* Extend support of XID interfaces using stdxcvr.xml */
    #define INCLUDE_LONLINK         1
    #define INCLUDE_XNVT_SUPPORT    1   /* 0 => Not included.  Workaround for issue where the LNS incorrectly swaps the array length for */
                                        /*      the newer APP_QUERY_NV_INFO:NV_INFO_DESCNV_INFO_DESC2, introduced for 2 byte SNVT IDs. */
                                        /* 1 => Support commands to query and define NVs with 2 byte SNVTIDs. Needed to support the ability to have SNVT IDs > 254 */
    #define INCLUDE_IZOT            1   /* LS/IP (or IzoT) protocol */
#elif defined(LIFTBR_PLATFORM)

    #define PRODUCT_ID              PRODUCT_ID_LIFTBR
    #define INCLUDE_IP852           0
    #define INCLUDE_L5MIP           0
    #define INCLUDE_MONITOR_SETS    0
    #define INCLUDE_STANDALONE_MGMT 0
    #define PERSISTENCE_TYPE        PERSISTENCE_TYPE_STANDARD
	#define PERSISTENCE_LOCATION	NULL
    #define INCLUDE_APP_CONTROL     1
    #define INCLUDE_MULTI_APP	    1
    #define INCLUDE_NVRAM           0
	#define INCLUDE_LTEP			0
	#define INCLUDE_EMBEDDED		1
    #define INCLUDE_APP_INDEX_IN_PERSISTENCE_FILES 1
	#define INCLUDE_INDEPENDENTLY_POWERED_INTERFACE 0
    #define INCLUDE_STDXCVR_FILE_SUPPORT 0 /* Extend support of XID interfaces using stdxcvr.xml */
    #define INCLUDE_LONLINK         1
    #define INCLUDE_XNVT_SUPPORT    1   /* 0 => Not included.  Workaround for issue where the LNS incorrectly swaps the array length for */
                                        /*      the newer APP_QUERY_NV_INFO:NV_INFO_DESCNV_INFO_DESC2, introduced for 2 byte SNVT IDs. */
                                        /* 1 => Support commands to query and define NVs with 2 byte SNVTIDs. Needed to support the ability to have SNVT IDs > 254 */
    #define INCLUDE_IZOT            1   /* LS/IP (or IzoT) protocol */
#else   /* VNISTACK */

    #define PRODUCT_ID              PRODUCT_ID_VNISTACK
    #define INCLUDE_IP852           1
    #define INCLUDE_L5MIP           1
    #define INCLUDE_MONITOR_SETS    1
    #define INCLUDE_STANDALONE_MGMT 0
    #define PERSISTENCE_TYPE        PERSISTENCE_TYPE_STANDARD
    #define PERSISTENCE_LOCATION    NULL
    #define INCLUDE_APP_CONTROL     1
    #define INCLUDE_MULTI_APP	    1
    #define INCLUDE_NVRAM           1
    #define INCLUDE_LTEP            0
    #define INCLUDE_EMBEDDED        0
    #define INCLUDE_APP_INDEX_IN_PERSISTENCE_FILES 0 
    #define INCLUDE_INDEPENDENTLY_POWERED_INTERFACE 0
    #define INCLUDE_STDXCVR_FILE_SUPPORT 1 /* Extend support of XID interfaces using stdxcvr.xml */
    #define INCLUDE_LONLINK         1
    #define INCLUDE_XNVT_SUPPORT    1   /* 0 => Not included.  Workaround for issue where the LNS incorrectly swaps the array length for */
                                        /*      the newer APP_QUERY_NV_INFO:NV_INFO_DESCNV_INFO_DESC2, introduced for 2 byte SNVT IDs. */
                                        /* 1 => Support commands to query and define NVs with 2 byte SNVTIDs. Needed to support the ability to have SNVT IDs > 254 */
    #define INCLUDE_IZOT            1   /* LS/IP (or IzoT) protocol */


#endif

#endif


