/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Percy Zahl <zahl@fkp.uni-hannover.de>
 * additional features: Andreas Klust <klust@fkp.uni-hannover.de>
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

#ifndef __PROBE_SCAN_H
#define __PROBE_SCAN_H

#include <iostream>

#include <gtk/gtk.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <asm/page.h>
#include <fcntl.h>

#include "config.h"
#include "gxsm4/mem2d.h"
#include "gxsm4/scan.h"
#include "gxsm4/xsmhard.h"
#include "gxsm4/glbvars.h"
#include "gxsm4/plugin.h"
#include "app_probe.h"
#include "include/dsp-pci32/xsm/xsmcmd.h"

#include "probe_base.h"

extern GxsmPlugin DSPProbe_pi;

// storage class for probe control parameters
class SPM_Probe_p : public ProbeBase {
 public:
  SPM_Probe_p(gchar *SetName = "ProbeCntrl", gchar *SetTitle = "Probe Data");
  ~SPM_Probe_p();
  int Resize(){
    scan->Resize(nx, nsrcs);
    return 0;
  };
  
  double xS, xE;   // start and end point
  double GapAdj;   // Gap Adjustment, Ang/Volt from current Setpoit
  double ACAmp;    // AC out Anteil
  double ACFrq;    // AC Modulations Frq.
  double ACPhase;  // Phase for LockIn
  double ACMultiplier;  // Multiplier for AC data (1, >1 make used of increased resolution)
  int nx;          // #points
  int delay;       // delay between data aquisition
  int channels;    // channels to aquire (>=1Value, >=2Amp, >=3Z)
  int nAve;        // averaging
  int nRep;        // # Repitions
  double CIval;    // CP=CS=0.; CI=this while Probe
  int srcs;        // input channel mask
  int nsrcs;       // number of input sources
  int outp;        // output channel
  gchar *resName;  // resource name
  gchar *resTitle;    // title / tab label
  Mem2d *scan;     // data
};

/*
 * Probe (Force/Dist, STS, Oszi...)) Ableitung
 * this "scan" is handled seperatly by ProbeControl
 * e.g. is not managed by the "surface-channel-system"
 * ============================================================
 */

class ProbeScan : public Scan{
public:
  ProbeScan();
  ~ProbeScan();

  int Save(gchar *fname);
  int Load(gchar *fname);

  int Probe(XSM_Hardware *hw, SPM_Probe_p *prbp);
  int Stop();

private:
  int StopProbeFlg;
};

#endif

