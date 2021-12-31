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

#ifndef __PROBE_BASE_H
#define __PROBE_BASE_H

/* SPM Hardware auf /dev/dspdev -- xafm.out running on dsp 
 * used by AFM, STM, SARLS
 */

class ProbeBase{
 public:
  ProbeBase(UnitObj *x=NULL, UnitObj *z=NULL){ 
    XUnit = x?x : new UnitObj(" "," ");
    ZUnit = z?z : new UnitObj(" "," ");
  };
  ~ProbeBase(){ if(XUnit) delete XUnit; if(ZUnit) delete ZUnit; };

  void SetUnits(UnitObj *x=NULL, UnitObj *z=NULL){ 
    if(XUnit) delete XUnit; 
    if(ZUnit) delete ZUnit;
    XUnit=x; ZUnit=z; 
  };

  UnitObj *XUnit;
  UnitObj *ZUnit;
};

#endif
