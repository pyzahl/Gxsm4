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
/* pci32.h 
 * Driver
 */

#define MODNAME "pci32: "
#define MODID   PCDSP_MODID_PCI32

/* PCI Device Identification Numbers */

#define PCI_VENDOR_ID_INNOVATIVE  0x10e8
#define PCI_DEVICE_ID_PCI32DSP    0x807c

/*
 * PCI32 IO Mappings 
 */

#define PCI32_BASE  pcdsp_iobase
#define PCI32_CTRL  PCI32_BASE+0x00 /* PCI32 Controlregister: Write: 1 Reset, 0 Run; Read: - */
#define PCI32_IRQ0  PCI32_BASE+0x04 /* PCI32 Control Interrupt Signal (PCINT0) : 1 asserts (JP17 low), 0 deasserts (JP17 high); Read: - */
#define PCI32_IRQ1  PCI32_BASE+0x08 /* PCI32 Control Interrupt Signal (PCINT1) : 1 asserts (JP17 low), 0 deasserts (JP17 high); Read: - */
/* realtive: */

#define PCI32_SEMANZ   4
#define PCDSP_SEMANZ PCI32_SEMANZ

#define PCI32_SEM0  0x0C /* PCI32 Semaphore 0 : Write 1 request, 0 free; Read 1: Busy, 0 free */
#define PCI32_SEM1  0x10 /* PCI32 Semaphore 1 : Write 1 request, 0 free; Read 1: Busy, 0 free */
#define PCI32_SEM2  0x14 /* PCI32 Semaphore 2 : Write 1 request, 0 free; Read 1: Busy, 0 free */
#define PCI32_SEM3  0x18 /* PCI32 Semaphore 3 : Write 1 request, 0 free; Read 1: Busy, 0 free */

static unsigned long pci32_sems[4] = { PCI32_SEM0, PCI32_SEM1, PCI32_SEM2, PCI32_SEM3 };

/*
 * DSP - Host Communication
 * - low level handshaking via Semaphores for controlling DPRAM access -
 */

/* Request for Semaphore [0..3] */
#define GET_SEM(N)  outw(1, pcdsp_iobase+pci32_sems[N])
/* Release SEM */
#define FREE_SEM(N) outw(0, pcdsp_iobase+pci32_sems[N])
/* Status SEM ? 0: busy, 1: free */
#define SEM(N)      (inw(pcdsp_iobase+pci32_sems[N])&1)


/* DSP IRQ control */
#define CLR_IRQ0  outw(0, PCI32_IRQ0)
#define SET_IRQ0  outw(1, PCI32_IRQ0)
#define CLR_IRQ1  outw(0, PCI32_IRQ1)
#define SET_IRQ1  outw(1, PCI32_IRQ1)

/* DSP control */
#define PCDSP_HALT_X  outw(1, PCI32_CTRL)
#define PCDSP_RUN_X   outw(0, PCI32_CTRL)
