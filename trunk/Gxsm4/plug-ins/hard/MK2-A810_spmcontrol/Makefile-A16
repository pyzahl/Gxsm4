#/* SRanger and Gxsm - Gnome X Scanning Microscopy Project
# * universal STM/AFM/SARLS/SPALEED/... controlling and
# * data analysis software
# *
# * DSP tools for Linux
# *
# * Copyright (C) 1999-2008 Percy Zahl
# *
# * Authors: Percy Zahl <zahl@users.sf.net>
# * WWW Home:
# * DSP part:  http://sranger.sf.net
# * Gxsm part: http://gxsm.sf.net
# *
# * This program is free software; you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation; either version 2 of the License, or
# * (at your option) any later version.
# *
# * This program is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
# *
# * You should have received a copy of the GNU General Public License
# * along with this program; if not, write to the Free Software
# * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
# */
#
# --------------------------------------------------------------------------------
# Makefile to build the FB_spmcontrol.out DSP binary (COFF2 code), 
# you need to install the TI-C55x CC or newer to /opt.
# Get it from here: 
#
# Linux hosted 55xx Code Generation Tools Linux-DSP-Tools-1.00.00.06 
# https://www-a.ti.com/downloads/sds_support/targetcontent/LinuxDspTools/download.html 
#
# --------------------------------------------------------------------------------

C55X_C_DIR   = /opt/TI/cgtools-c5500/include
C55X_LIB_DIR = /opt/TI/cgtools-c5500/lib
C55X_BIN_DIR = /opt/TI/cgtools-c5500/bin

C            = $(C55X_BIN_DIR)/./cl55
CL           = $(C55X_BIN_DIR)/./lnk55
CS           = $(C55X_BIN_DIR)/./strip55
CFLAGS       = -O3 -v5502 -i $(C55X_C_DIR) -c -DUSE_ANALOG_16 -DDSP_CC
CLFLAGS      = -i $(C55X_LIB_DIR) -c
LIBS         = FB_spm_lnk_A16.cmd

C_OBJECTS = dataprocess.obj \
	FB_spm.obj FB_spm_statemaschine.obj \
	FB_spm_areascan.obj FB_spm_offsetmove.obj \
	FB_spm_autoapproach.obj FB_spm_probe.obj \
	FB_spm_CoolRunner_puls.obj FB_spm_dsoszi.obj

ASM_OBJECTS = FB_spm_magic.obj spm_log.obj feedback.obj \
        dsp_clock.obj vectors_A16.obj ReadWrite_GPIO.obj mul32.obj record.obj

FB_spmcontrol_MK2_A16.out: $(C_OBJECTS) $(ASM_OBJECTS) $(LIBS)
	@echo
	@echo binding objects...
	$(CL) $(CLFLAGS) $(C_OBJECTS) $(ASM_OBJECTS) $(LIBS) -m FB_spmcontrol_MK2_A16.map -o FB_spmcontrol_MK2_A16.out
#	@echo
#	@echo stripping debug info from FB_spmcontrol_MK2_A16.out...
#	@echo $(CS) FB_spmcontrol_MK2_A16.out
	@echo
	@echo FB_spmcontrol_MK2_A16.out is ready to load into the DSP.
	@echo

$(C_OBJECTS) : dataprocess.h  FB_spm_analog.h  FB_spm_dataexchange.h  FB_spm.h  FB_spm_statemaschine.h 

$(C_OBJECTS): %.obj: %.c
	$(C) -c $(CFLAGS) $<

$(ASM_OBJECTS): %.obj: %.asm
	$(C) -c $(CFLAGS) $<

.PHONY : load
load: FB_spmcontrol_MK2_A16.out
	ln -sf ../../loadusb64/kernel.out .
	../../loadusb64/loadusb FB_spmcontrol_MK2_A16.out

.PHONY : clean
clean :
	rm -f ${C_OBJECTS} ${ASM_OBJECTS} FB_spmcontrol*.out FB_spmcontrol*.map
