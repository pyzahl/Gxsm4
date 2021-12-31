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
/*
 * DSP TALKER DPRAM Definition - higher level communication via DPRAM
 */

/*
 * Mailbox after Start of Monitor:
 * 1FC0: 1f 00 00 00 ff ff ff ff 00 00 00 00 00 00 00 00   ................
 */

#define PCDSP_TALK_MAILBOX (4*PCDSP_MBOX)      /* DSP Talker's Mailbox Offset in DPRAM (1FC0) */

static unsigned long mbox_location=PCDSP_TALK_MAILBOX;

#define PCDSP_TALK_RCV     (mbox_location + 0) /* DSP Talker's XMT - transmit */
#define PCDSP_TALK_ACK     (mbox_location + 4) /* DSP Talker's REQ - request */
#define PCDSP_TALK_XMT     (mbox_location + 8) /* DSP Talker's RCV - receive */
#define PCDSP_TALK_REQ     (mbox_location +12) /* DSP Talker's REQ - acknowledge */

/*
 * Write to Box:
 * REQD_DSP ?  ->  XMT_DSP(value), REQ_DSP  ->  [ REQD_DSP ? ]
 */

#define XMT_DSP(X)  writel((X), pcdsp_dprambaseptr+PCDSP_TALK_XMT)  /* transmit data from driver to DSP */
#define REQD_DSP    readl(pcdsp_dprambaseptr+PCDSP_TALK_REQ)        /* Data receiced by DSP ?           */
#define REQ_DSP     writel(-1, pcdsp_dprambaseptr+PCDSP_TALK_REQ)   /* request DSP to read data         */

/*
 * Read from Box:
 * ACKD_DSP ?  ->  value = RCV_DSP, ACK_DSP  ->  [ ACKD_DSP ? ]
 */

/*
 * DSP Talker Communication
 */
#define RCV_DSP     readl(pcdsp_dprambaseptr+PCDSP_TALK_RCV)        /* recieve Data from DSP               */
#define ACK_DSP     writel(  0, pcdsp_dprambaseptr+PCDSP_TALK_ACK)  /* acknowledged data recieved from DSP */
#define ACKD_DSP    readl(pcdsp_dprambaseptr+PCDSP_TALK_ACK)        /* data acknowledged by DSP            */

