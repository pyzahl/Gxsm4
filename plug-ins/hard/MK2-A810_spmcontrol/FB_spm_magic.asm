;; /* SRanger and Gxsm - Gnome X Scanning Microscopy Project
;;  * universal STM/AFM/SARLS/SPALEED/... controlling and
;;  * data analysis software
;;  *
;;  * DSP tools for Linux
;;  *
;;  * Copyright (C) 1999,2000,2001,2002 Percy Zahl
;;  *
;;  * Authors: Percy Zahl <zahl@users.sf.net>
;;  * WWW Home:
;;  * DSP part:  http://sranger.sf.net
;;  * Gxsm part: http://gxsm.sf.net
;;  *
;;  * This program is free software; you can redistribute it and/or modify
;;  * it under the terms of the GNU General Public License as published by
;;  * the Free Software Foundation; either version 2 of the License, or
;;  * (at your option) any later version.
;;  *
;;  * This program is distributed in the hope that it will be useful,
;;  * but WITHOUT ANY WARRANTY; without even the implied warranty of
;;  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;  * GNU General Public License for more details.
;;  *
;;  * You should have received a copy of the GNU General Public License
;;  * along with this program; if not, write to the Free Software
;;  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
;;  */

;; /* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

MAGICMAXSIZE	.set	0x0100
	.global _magic
_magic	.usect	"MAGIC",MAGICMAXSIZE

;; the following two defionitions must match with C definition in FB_spm_dataexchange.h
PRBVECTORSIZE	.set	0x0700
	.global _prbvh
_prbvh	.usect	"PRBVH",PRBVECTORSIZE

;; this buffer is shared with wave form generation and optional 2nd trace mode 0x3800
PRBDATAFIFOSIZE	.set	0x2800
	.global _prbdf
_prbdf	.usect	"PRBDF",PRBDATAFIFOSIZE

