/*
 * Defs for LON-C
 *
 * Copyright(C) 1998 Toshiba Corporation, ALL RIGHTS RESERVED.
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
 */
#ifndef	___LONC_H
#define	___LONC_H

#ifdef	__cplusplus
extern "C" {
#endif
/*
 Rev.1 98/10/15
   Register&Bitmaps.
 Rev.2 98/10/16
   Added #D9C -- detect IDLE state of DMA
 */

typedef unsigned long txword;
typedef unsigned short half;

typedef struct _Desc {
  volatile txword cmd;
  volatile txword adr;
  volatile txword siz;
  volatile txword sts;
} Desc;

/* LSCMD */
#define	LSCMD_TX_OK	0x8000
#define LSCMD_SOP	0x4000
#define	LSCMD_EOP	0x2000

/* LxSIZ */
#define	LSSIZ_REQ_MASK	0x0000FFFF
#define	LSSIZ_FIN_MASK	0xFFFF0000

/* LSSTS */
#define	LSSTS_DMA_COMP		0x80000000
#define	LSSTS_DMA_ACT		0x20000000
#define	LSSTS_PKT_COMP		0x10000000
#define	LSSTS_TRANS_ERR		0x02000000
#define	LSSTS_ERR_UDRUN		0x00008000
#define	LSSTS_ERR_CCOL		0x00002000
#define	LSSTS_ERR_LCOL		0x00001000
#define	LSSTS_ERR_CANCELLED	0x00000800
#define	LSSTS_ERR_LONGFRAME	0x00000400
#define	LSSTS_ERR_SHORTFRAME	0x00000200

#define	LSSTS_FAIL_INV	(LSSTS_DMA_COMP|LSSTS_PKT_COMP)
#define	LSSTS_FAIL	(LSSTS_DMA_COMP|LSSTS_PKT_COMP|	\
	LSSTS_DMA_ACT|LSSTS_TRANS_ERR|LSSTS_ERR_UDRUN|	\
	LSSTS_ERR_CCOL|LSSTS_ERR_LCOL|			\
	LSSTS_ERR_LONGFRAME|LSSTS_ERR_SHORTFRAME)

/* LSDCNT */
#define	LSDCNT_DSC_ACTIVE	0x80000000
#define	LSDCNT_DSC_PRIO_ACTIVE	0x40000000
#define	LSDCNT_DSC_STOP		0x20000000

/* LRCMD */
#define	LRCMD_RX_OK	0x8000

/* LRSTS */
#define	LRSTS_DMA_COMP		0x80000000
#define	LRSTS_DMA_ACT		0x20000000
#define	LRSTS_PKT_COMP		0x10000000
#define	LRSTS_SOP		0x08000000
#define	LRSTS_EOP		0x04000000
#define	LRSTS_ERR_OVRUN		0x00008000
#define	LRSTS_ERR_OVERFILL	0x00004000
#define	LRSTS_ERR_CRC		0x00002000
#define	LRSTS_ERR_DCOL		0x00001000
#define	LRSTS_ERR_FRAGBITS	0x00000800
#define	LRSTS_ERR_LONGFRAME	0x00000400
#define	LRSTS_ERR_SHORTFRAME	0x00000200
#define	LRSTS_ERR_RXTIMEOUT	0x00000100
#define	LRSTS_ERR_LONGPREAMBLE	0x00000080
#define	LRSTS_ERR_SHORTPREAMBLE	0x00000040
#define	LRSTS_RXSLOT		0x00000020	/* not an error */

#define	LRSTS_FAIL_INV	(LRSTS_DMA_COMP|LRSTS_PKT_COMP|	\
	LRSTS_SOP|LRSTS_EOP)
#define	LRSTS_FAIL	(LRSTS_DMA_COMP|LRSTS_PKT_COMP|	\
	LRSTS_SOP|LRSTS_EOP|LRSTS_ERR_OVRUN|LRSTS_ERR_OVERFILL|	\
	LRSTS_ERR_CRC|LRSTS_ERR_DCOL|LRSTS_ERR_FRAGBITS|	\
	LRSTS_ERR_LONGFRAME|LRSTS_ERR_SHORTFRAME|LRSTS_ERR_RXTIMEOUT|	\
	LRSTS_ERR_LONGPREAMBLE|LRSTS_ERR_SHORTPREAMBLE)
/* LRDCNT */
#define	LRDCNT_DSC_ACTIVE	0x80000000
#define	LRDCNT_DSC_PRIO_ACTIVE	0x40000000
#define	LRDCNT_DSC_STOP		0x20000000

/* Interrupt bits */
#define	LIT_HW_IDLE		0x80
#define	LIT_ERRCNT_OVERFLOW	0x40
#define	LIT_EXTREG_ACC		0x20
#define	LIT_SERVICEPIN		0x10
#define	LIT_LON_WAKEUP		0x08
#define	LIT_LON_RESET		0x04
#define	LIT_DMA_RX_COMP		0x02
#define	LIT_DMA_TX_COMP		0x01

/* RCCMODE */
#define	RCCMODE_ANALYZER	0x40
#define	RCCMODE_RXSLOT		0x20
#define	RCCMODE_TXSLOT		0x10
#define	RCCMODE_FIX_RANDOM	0x08
#define	RCCMODE_COMMTYPE_MASK	0x07
#define   RCCMODE_BLANK		0
#define	  RCCMODE_SINGLE	1
#define   RCCMODE_SPECIAL	2
#define	  RCCMODE_DIFFER	5
#define	  RCCMODE_LOOPBACK	7

/* RCPDIR */
#define	RCPDIR_MASK		0x1F
#define	RCPDIR_DIRECT		0x0E
#define	RCPDIR_SLEEP		0x1E
#define	RCPDIR_WAKEUP		0x16

/* RCCLKR */
#define	RCCLKR_MASK		0x1F
#define	RCCLKR_8		0
#define	RCCLKR_16		1
#define	RCCLKR_32		2
#define	RCCLKR_64		3
#define	RCCLKR_128		4
#define	RCCLKR_256		5
#define	RCCLKR_512		6
#define	RCCLKR_1024		7
#define	RCCLKR_2048		8
#define	RCCLKR_4096		9
#define	RCCLKR_8192		10
#define	RCCLKR_16384		11
#define	RCCLKR_32768		12
#define	RCCLKR_65536		13
#define	RCCLKR_131072		14

/* RCBSYNC */
#define	RCBSYNC_MASK		3
#define	RCBSYNC_4		0
#define	RCBSYNC_5		1
#define	RCBSYNC_6		2
#define	RCBSYNC_7		3

/* RCSVC */
#define	RCSVC_ON		0
#define	RCSVC_BLINK		1
#define	RCSVC_OFF		2

/* RTXRQTH */
#define	RTXRQTH_MASK		0x0f

/* RCEXREG */
#define	RCEXREG_WRITEBUSY	0x2000
#define	RCEXREG_VALIDDATA	0x1000
#define	RCEXREG_READWRITE	0x0800
#define	RCEXREG_REG_ADDR	0x0700
#define	RCEXREG_REG_SHIFT	8
#define	RCEXREG_WRITEDATA	0x00FF

/* RCBLOG */
#define	RCBLOG_MASK		0x3F

/* RCWBASE */
#define	RCWBASE_MASK		3
#define	RCWBASE_16		0
#define	RCWBASE_32		1
#define	RCWBASE_63		2

/* RCPOVRD */
#define	RCPOVRD_MASK		0xFF
#define	RCPOVRF_CP3		0x80
#define	RCPOVRF_CP3_CLK_MASK	0x70
#define	RCPOVRF_CP3_CLK_SHIFT	4
#define	RCPOVRF_CP2		0x08
#define	RCPOVRF_CP2_CLK_MASK	0x07
#define	RCPOVRF_CP2_CLK_SHIFT	0

#define	RCPOVRF_UNDEF		0
#define	RCPOVRF_BCLK		1
#define	RCPOVRF_B2LK		2
#define	RCPOVRF_DMRCK		3
#define	RCPOVRF_LONCK		4
#define	RCPOVRF_SELIO		5
#define	RCPOVRF_SELIO2		6
#define	RCPOVRF_EBCK		7

/* RCLCKR */
#define	RCLCKR_MASK		3
#define	RCLCKR_1		0
#define	RCLCKR_2		1
#define	RCLCKR_4		2
#define	RCLCKR_8		3

/* RCINCK */
#define	RCINCK_MASK		7
#define	RCINCK_00313		0
#define	RCINCK_00625		1
#define	RCINCK_01250		2
#define	RCINCK_02500		3
#define	RCINCK_05000		4
#define	RCINCK_10000		5
#define	RCINCK_20000		6
#define	RCINCK_40000		7

typedef struct _Regs {
/* D00 */
  volatile byte _FCOPR;
#define FCOPR (lonControllerP->_FCOPR)
/* D01 */
  volatile byte _RCCMODE;
#define RCCMODE (lonControllerP->_RCCMODE)
/* D02 */
  volatile byte _RCPDIR;
#define RCPDIR (lonControllerP->_RCPDIR)
/* D03 */
  volatile byte _RCCLKR;
#define RCCLKR (lonControllerP->_RCCLKR)
/* D04 */
  volatile txword _RCPRLEN;
#define RCPRLEN (lonControllerP->_RCPRLEN)
/* D08 */
  volatile txword _RCPCYL;
#define RCPCYL (lonControllerP->_RCPCYL)
/* D0C */
  volatile txword _RCBETA2;
#define RCBETA2 (lonControllerP->_RCBETA2)
/* D10 */
  volatile txword _RCBT1RX;
#define RCBT1RX (lonControllerP->_RCBT1RX)
/* D14 */
  volatile txword _RCBT1TX;
#define RCBT1TX (lonControllerP->_RCBT1TX)
/* D18 */
  volatile byte _RCNODEP;
#define RCNODEP (lonControllerP->_RCNODEP)
/* D19 */
  volatile byte _RCCHANP;
#define RCCHANP (lonControllerP->_RCCHANP)
/* D1A */
  volatile byte _FCCOLDEN;
#define FCCOLDEN (lonControllerP->_FCCOLDEN)
/* D1B */
  volatile byte _RCBSYNC;
#define RCBSYNC (lonControllerP->_RCBSYNC)
/* D1C */
  volatile byte _RCSVC;
#define RCSVC (lonControllerP->_RCSVC)
/* D1D */
  volatile byte _RTXRQTH;
#define RTXRQTH (lonControllerP->_RTXRQTH)
/* D1E */
  volatile byte _FIDLE;
#define FIDLE (lonControllerP->_FIDLE)
/* D1F */
  volatile byte _LONC_1F;
#define LONC_1F (lonControllerP->_LONC_1F)
/* D20 */
  volatile byte _FCCDTL;
#define FCCDTL (lonControllerP->_FCCDTL)
/* D21 */
  volatile byte _FCCDPRB;
#define FCCDPRB (lonControllerP->_FCCDPRB)
/* D22 */
  volatile half _RCEXREG;
#define RCEXREG (lonControllerP->_RCEXREG)
/* D24 */
  volatile byte _FPCNCL;
#define FPCNCL (lonControllerP->_FPCNCL)
/* D25 */
  volatile byte _FTXTERM;
#define FTXTERM (lonControllerP->_FTXTERM)
/* D26 */
  volatile byte _RCBLOG;
#define RCBLOG (lonControllerP->_RCBLOG)
/* D27 */
  volatile byte _RCWBASE;
#define RCWBASE (lonControllerP->_RCWBASE)
/* D28 */
  volatile byte _RDMARTH;
#define RDMARTH (lonControllerP->_RDMARTH)
/* D29 */
  volatile byte _RDMASTH;
#define RDMASTH (lonControllerP->_RDMASTH)
/* D2A */
  volatile byte _FSLEEP;
#define FSLEEP (lonControllerP->_FSLEEP)
/* D2B */
  volatile byte _RCPOVRD;
#define RCPOVRD (lonControllerP->_RCPOVRD)
/* D2C */
  volatile half _RCDBGRN;
#define RCDBGRN (lonControllerP->_RCDBGRN)
/* D2E */
  volatile byte _RCLCKR;
#define RCLCKR (lonControllerP->_RCLCKR)
/* D2F */
  volatile byte _RCINCK;
#define RCINCK (lonControllerP->_RCINCK)
/* D30 */
  volatile half _RXMTERC;
#define RXMTERC (lonControllerP->_RXMTERC)
/* D32 */
  volatile half _RMISPC;
#define RMISPC (lonControllerP->_RMISPC)
/* D34 */
  volatile half _RCOLLC;
#define RCOLLC (lonControllerP->_RCOLLC)
/* D36 */
  volatile half _RBLOVFC;
#define RBLOVFC (lonControllerP->_RBLOVFC)
/* D38 */
  volatile half _RBKOFFC;
#define RBKOFFC (lonControllerP->_RBKOFFC)
/* D3A */
  volatile half _LONC_3A;
#define LONC_3A (lonControllerP->_LONC_3A)
/* D3C */
  volatile txword _RSEED;
#define RSEED (lonControllerP->_RSEED)
/* D40 */
  volatile txword _LSCMD;
#define LSCMD (lonControllerP->_LSCMD)
/* D44 */
  volatile txword _LSADR;
#define LSADR (lonControllerP->_LSADR)
/* D48 */
  volatile txword _LSSIZ;
#define LSSIZ (lonControllerP->_LSSIZ)
/* D4C */
  volatile txword _LSSTS;
#define LSSTS (lonControllerP->_LSSTS)
/* D50 */
  volatile txword _LSPCL;
#define LSPCL (lonControllerP->_LSPCL)
/* D54 */
  volatile txword _LSDCNT;
#define LSDCNT (lonControllerP->_LSDCNT)
/* D58 */
  volatile txword _LONC_58;
#define LONC_58 (lonControllerP->_LONC_58)
/* D5C */
  volatile txword _LONC_5C;
#define LONC_5C (lonControllerP->_LONC_5C)
/* D60 */
  volatile txword _LRCMD;
#define LRCMD (lonControllerP->_LRCMD)
/* D64 */
  volatile txword _LRADR;
#define LRADR (lonControllerP->_LRADR)
/* D68 */
  volatile txword _LRSIZ;
#define LRSIZ (lonControllerP->_LRSIZ)
/* D6C */
  volatile txword _LRSTS;
#define LRSTS (lonControllerP->_LRSTS)
/* D70 */
  volatile txword _LRPCL;
#define LRPCL (lonControllerP->_LRPCL)
/* D74 */
  volatile txword _LRDCNT;
#define LRDCNT (lonControllerP->_LRDCNT)
/* D78 */
  volatile txword _LONC_78;
#define LONC_78 (lonControllerP->_LONC_78)
/* D7C */
  volatile txword _LONC_7C;
#define LONC_7C (lonControllerP->_LONC_7C)
/* D80 */
  volatile txword _LSDST;
#define LSDST (lonControllerP->_LSDST)
/* D84 */
  volatile txword _LSDEND;
#define LSDEND (lonControllerP->_LSDEND)
/* D88 */
  volatile txword _LSDAD;
#define LSDAD (lonControllerP->_LSDAD)
/* D8C */
  volatile txword _LRDST;
#define LRDST (lonControllerP->_LRDST)
/* D90 */
  volatile txword _LRDEND;
#define LRDEND (lonControllerP->_LRDEND)
/* D94 */
  volatile txword _LRDAD;
#define LRDAD (lonControllerP->_LRDAD)
/* D98 */
  volatile byte _LONENB;
#define LONENB (lonControllerP->_LONENB)
/* D99 */
  volatile byte _DRAMA_B;
#define DRAMA_B (lonControllerP->_DRAMA_B)
/* D9A */
  volatile byte _IRENB;
#define IRENB (lonControllerP->_IRENB)
/* D9B */
  volatile byte _IRREG;
#define IRREG (lonControllerP->_IRREG)
/* D9C */
  volatile txword _LONC_D9C;
#define LONC_D9C   (lonControllerP->_LONC_D9C)
/* DA0 */
  volatile txword _LPSDST;
#define LPSDST (lonControllerP->_LPSDST)
/* DA4 */
  volatile txword _LPSDEND;
#define LPSDEND (lonControllerP->_LPSDEND)
/* DA8 */
  volatile txword _LPSDAD;
#define LPSDAD (lonControllerP->_LPSDAD)
/* DAC */
  volatile txword _LPRDST;
#define LPRDST (lonControllerP->_LPRDST)
/* DB0 */
  volatile txword _LPRDEND;
#define LPRDEND (lonControllerP->_LPRDEND)
/* DB4 */
  volatile txword _LPRDAD;
#define LPRDAD (lonControllerP->_LPRDAD)
/* DB8 */
  volatile byte _DO;
#define DO (lonControllerP->_DO)
} Regs;

#ifdef	__cplusplus
};
#endif

#endif	/* ___LONC_H */
