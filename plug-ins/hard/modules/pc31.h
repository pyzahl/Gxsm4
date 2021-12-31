/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * additional features: Andreas Klust <klust@users.sf.net>
 * WWW Home: http://gxsm.sf.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/* pc31.h 
 * Kernel Module
 * (C) 1998 by Percy Zahl, Inst. f. FKP
 */

#define MODNAME "pc31: "
#define MODID   PCDSP_MODID_PC31

/*
 * ============================================================
 * PC31's I/O und Memory (DSP-Karte)
 * ============================================================
 */

/* Default Adressen */
#define BASEPORT 0x280               /* PC31 BASEPORT */
#define RANGE    0x10                /* PC31 IO-SPACESIZE */

/* default is 0xdc000 */
/* #define DPRAMBASE 0xdc000 */           /* DPRAMBASEADRESS */
#define DPRAMBASE 0xd0000            /* DPRAMBASEADRESS */
#define DPRAMSIZE (DSP_DPRAMLEN*4)   /* 8kByte DPRAM */
#define DPLen32   (DSP_DPRAMLEN)     /* 2k Wort a 32bit */

/* CRTL Register
    1 2 4 8 10 20 40 80
Bit 0 1 2 3  4  5  6  7
 */
#define PC31_CRTL_RESET 0x01
#define PC31_CRTL_EI2   0x10
#define PC31_CRTL_HOLD  0x20
#define PC31_CRTL_BL    0x80 

/* IO - Mapping */
#define DSP_BASE        pcdsp_iobase
#define DSP_ADR_H       DSP_BASE+0      /* W:   Adresslatch MSB von 32 Bits */
#define DSP_ADR_L       DSP_BASE+2      /* W:   Adresslatch LSB von 32 Bits */
#define DSP_DAT_H       DSP_BASE+4      /* R/W: Datenlatch MSB von 32 Bits */
#define DSP_DAT_L       DSP_BASE+6      /* R/W: Datenlatch LSB von 32 Bits */
#define DSP_CRTL        DSP_BASE+8      /* W:   Latch 8-bit Bus Control */
                                        /* Bit 0: PC31 Reset, active high */
                                        /* Bit 4: PC31 EI2 */
                                        /* Bit 5: Hold Reqest */
                                        /* Bit 7: Boot loader Control */
#define DSP_DPCTL       DSP_BASE+10     /* DPRAM Control Register 
					 * Bit 0: Read 0: use denied
					 * Bit 0: Read 1: use granted
					 * Bit 0: Write 0: give away
					 * Bit 0: Write 1: use reqired
					 */
#define DSP_SRQ         DSP_BASE+12     /* W: Service Request to PC31, latch bit 0 */
                                        /* R: Return Service Request Status: */
                                        /* Bit 0: PC_AT service request */
                                        /* Bit 1: PC31  service request */
#define DSP_ACK         DSP_BASE+14     /* R/W: Acknowledge, Clear PC31 Service Request */

/* Version Stuff ... */
#define DSPNEW 1 /* PC31 Version 1997/98 mit DPRAM Acesskontrolle */
#define DSPOLD 0 /* PC31 alt, (in Quantum) ohne DPRAM Acesskontrolle */

#define PC31J
#define PC31K

/*
 * DSP - Host Communication
 * - low level handshaking via Semaphores for controlling DPRAM access -
 */

#define PCDSP_SEMANZ 4

/* Request for Semaphore [0..3] */
#define GET_SEM(N)  outw(1, DSP_DPCTL)
/* Release SEM */
#define FREE_SEM(N) outw(0, DSP_DPCTL)
/* Status SEM ? 0: busy, 1: free */
#define SEM(N)      (inw(DSP_DPCTL)&1)


/* DSP IRQ control */
#define CLR_IRQ0
#define SET_IRQ0  
#define CLR_IRQ1  
#define SET_IRQ1  

/* Hardware Mbox control, only PC31 */
#define PC31_SRQ(X)      outw_p((X), DSP_SRQ)
#define PC31_SRQED       (inw_p(DSP_SRQ) & 2)
#define PC31_ACKED       (~inw_p(DSP_SRQ) & 1)
#define PC31_ACK         outw_p(0, DSP_ACK)

/* DSP control */
unsigned short pc31_crtlword=0;

#define PCDSP_PC31CRTL     outw_p(pc31_crtlword, DSP_CRTL)

#define PCDSP_HALT_X       outw_p(0x13, DSP_CRTL); PC31_SRQ(0); PC31_ACK
/* outw(1, DSP_ADR_H); outw(DSP_CRTL, DSP_ADR_L); outw(1, DSP_ADR_L); */
#define PCDSP_RUN_X        outw_p(0x12, DSP_CRTL)
/* outw(1, DSP_ADR_L); */
#define ENABLE_PC31_DPRAM  outw_p(0x00, DSP_CRTL)
#define PC31_CONTROL(X)    outw_p((X), DSP_CRTL)

/* END */
