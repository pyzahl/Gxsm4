/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: DSPProbe.C
 * ========================================
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

#ifndef __APP_PROBE_H
#define __APP_PROBE_H

#include "probe_scan.h"

class SPM_Probe_p;
class ProbeScan;

class DSPProbeControl : public AppBase{
public:
  DSPProbeControl(XSM_Hardware *Hard, GSList **RemoteEntryList, int InWindow=TRUE);
  virtual ~DSPProbeControl();

  void update();

  // Probe
  static void ExecCmd(int cmd);
  static void ChangedNotify(Param_Control* pcs, gpointer data);
  static void CmdStartAction(GtkWidget *widget, DSPProbeControl *pc);
  static void CmdStopAction(GtkWidget *widget, DSPProbeControl *pc);

  static void delete_prb_cb(SPM_Probe_p *prb, DSPProbeControl *pc);

private:
  GtkWidget *notebook;
  int itab;
  int datamode;

  ProbeScan *prbscan;

  GSList *PrbList;

  UnitObj *Unity, *Volt, *Current, *Force, *UGapAdj, *Deg;
  UnitObj *TimeUnitms, *TimeUnit, *FrqUnit;
  XSM_Hardware *hard;
};

#endif
