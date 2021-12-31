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

/* pcsim.h 
 * Driver
 */

#define PCDSP_MAJOR       240
#define PCDSP_DEVFS_DIR   "pcdsps"
#define PCDSP_DEVICE_NAME "emu"

/* MODID's zur Moduletyp Identifizierung */
#define PCDSP_MODID_SIM   0x1001

#define MODNAME "pcsim: "
#define MODID   PCDSP_MODID_SIM

/* DPRAM [in 32bit-Words, e.g. total 8kBytes] */
#define PCDSP_DPRAM_WIDTH       0x0032 /* bits per cell */
#define PCDSP_DPRAM_CELLSIZE    4
#define PCDSP_DPRAM_ADRSHR      2
#define PCDSP_DPRAM_WLEN        0x0800 /* in Words...  8kB */
#define PCDSP_DPRAM_SIZE        0x2000 /* in Bytes...  8kB */

#define BYTSIZE(X) ((X)<<PCDSP_DPRAM_ADRSHR)

/*
 * IOCTL cmds
 */

#define PCDSP_MBOX_FULL         0x0001
#define PCDSP_MBOX_READ_WAIT    0x0002
#define PCDSP_MBOX_READ_NOWAIT  0x0003
#define PCDSP_MBOX_EMPTY        0x0004
#define PCDSP_MBOX_WRITE_WAIT   0x0005
#define PCDSP_MBOX_WRITE_NOWAIT 0x0006
#define PCDSP_SET_MBOX_LOCATION 0x0007

#define PCDSP_PUT_SPEED    0x0009

#define PCDSP_PUTMEM32     0x0010
#define PCDSP_PUTMEM32INC  0x0011
#define PCDSP_GETTMEM32    0x0012
#define PCDSP_GETMEM32INC  0x0013
#define PCDSP_ADDRESS      0x0014

#define PCDSP_RESET        0x0020
#define PCDSP_RUN          0x0021
#define PCDSP_HALT         0x0022

#define PCDSP_SEM0START    0x0100
#define PCDSP_SEM0LEN      0x0101
#define PCDSP_SEM1START    0x0102
#define PCDSP_SEM1LEN      0x0103
#define PCDSP_SEM2START    0x0104
#define PCDSP_SEM2LEN      0x0105
#define PCDSP_SEM3START    0x0106
#define PCDSP_SEM3LEN      0x0107

/* identify module type */
#define PCDSP_GETMODID     0x0900

/*
 * DPRAM Sem Bereiche
 */

#define PCDSP_MBOX       0x07f0  /* Controlled by SEM0 */
#define PCDSP_MBOX_SIZE  0x0004  /* MBOX Size */

/* DATAN <=> SEM(N) */
#define PCDSP_DATA1      0x0000  /* Controlled by SEM1 */
#define PCDSP_DATA1_SIZE 0x0030  /* Data0 Size */

#define PCDSP_DATA2      0x0030  /* Controlled by SEM2 */
#define PCDSP_DATA2_SIZE 0x0500  /* Data1 Size */

#define PCDSP_DATA3      0x0700  /* Controlled by SEM3 */
#define PCDSP_DATA3_SIZE 0x00c0  /* Data1 Size */

