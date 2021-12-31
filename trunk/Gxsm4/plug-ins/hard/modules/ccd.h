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
/* ccd.h 
 * Kernel Module
 * (C) 2000 by Percy Zahl
 */

#ifndef __CCDMOD_H
#define __CCDMOD_H


#define CCD_MAJOR       240
#define CCD_DEVFS_DIR   "pcdsps"
#define CCD_DEVICE_NAME "ccd"
#define MODNAME "ccd: "
#define MODID   CCD_MODID_01

#define CCD_DEVICE_NAME "ccd"

/*
 * ============================================================
 * PARPORT I/O used for CCD control
 * ============================================================
 */
#define	LPT1_BASE	0x378		/* LPT1 Basis Portnummer */
#define	LPT2_BASE	0x278		/* LPT2 Basis Portnummer */
#define	LPT3_BASE	0x3bc		/* LPT3 Basis Portnummer */

typedef struct {
    unsigned long base;     /* base address */
    unsigned long basehi;   /* basehi address (ECP), usually base+0x400 */
    short save_status;
    char  save_mode;
    char  save_ecmode; 
} ccd_parport;

#define PARP_DATA(p)         (p->base+0x0)
#define PARP_STATUS(p)       (p->base+0x1)
#define PARP_CONTROL(p)      (p->base+0x2)
#define PARP_ECPCONTROL(p)   (p->basehi+0x2)

/*  Steuerleitungsbelegung:
 **********************************
 *  CCD  Pin  =   LPT:     Pin Bit
 *  MD   1    =   -Strobe  1   0
 *  GO   2    =   -AutoFD  14  1
 *  PIC  3    =   Init     16  2
 *  HRD  4    =   -SLCT_IN 17  3   
 **********************************
 */

#define CCD_MinWait 40

#define LPT_XOR (1 | 2 | 8)
/*                                  1     2     4     8    0x20 
 *                                 /MD    GO  /PIC   HRD   READ 
 */
#define CCD_Monitoring(p)  outb_p((  1  |  2  |  4  |  8  | 0x20  )^LPT_XOR, PARP_CONTROL(p))
#define CCD_Move2Mem(p)    outb_p((  0  |  0  |  4  |  8  | 0x20  )^LPT_XOR, PARP_CONTROL(p))
#define CCD_Sammeln(p)     outb_p((  0  |  2  |  4  |  8  | 0x20  )^LPT_XOR, PARP_CONTROL(p))
#define CCD_Lesen(p)       outb_p((  1  |  0  |  0  |  8  | 0x20  )^LPT_XOR, PARP_CONTROL(p))
#define CCD_Next(p)        outb_p((  1  |  0  |  0  |  0  | 0x20  )^LPT_XOR, PARP_CONTROL(p))

#define CCD_PixWert(p)     inb_p(PARP_DATA(p))

/* CCD Daten */
#define CCD_Xpixel 193  // Anzahl Pixel in X
#define CCD_Ypixel 167  // Anzahl Pixel in Y
#define CCD_dT 20       // 20ms Zeiteinheit

/* CCD Cmds */

#define CCD_CMD_MONITORENABLE  1  // Monitor enable
#define CCD_CMD_CLEAR          2  // clear ccd
#define CCD_CMD_EXPOSURE       3  // do exposure cycle
#define CCD_CMD_INITLESEN      4  // read pixelvalue and skip to next
#define CCD_CMD_GETPIXEL       5  // read pixelvalue and skip to next


#endif
