/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001,2002,2003 Percy Zahl
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

#ifndef APP_VINFO_H
#define APP_VINFO_H

#include <config.h>

#include <gtk/gtk.h>

#include "xsmtypes.h"
#include "gapp_service.h"
#include "scan_types.h"

class Scan;

class ViewInfo{
public:
        ViewInfo(Scan *Sc, int qf, int zf);
        virtual ~ViewInfo(){};
  
        void SetQfZf (int qf, int zf){
                Qfac = qf; Zfac=zf;
        };
  
        void EnableTimeDisplay(int flag=TRUE){ showtime=flag; };
        void SetPixelUnit(int flag=TRUE){ pixelmode=flag; };
        void SetCoordMode(SCAN_COORD_MODE scm=SCAN_COORD_ABSOLUTE){ sc_mode=scm; };
        void ChangeXYUnit(UnitObj *u){ uy=ux=u; };
        void ChangeXUnit(UnitObj *u){ ux=u; };
        void ChangeYUnit(UnitObj *u){ uy=u; };
        void ChangeZUnit(UnitObj *u){ uz=u; };
        
        
        gchar *makeXinfo(double x); // X only
        gchar *makeDXYinfo(double xy1[2], double xy2[2], double factor=1.); // Delta X, w clipping, opt. scaleing w factor of user number
        gchar *makeDnXYinfo(double *xy, int n); // length of polyline, n nodes
        gchar *makeA2info(double xy1[2], double xy2[2]); // Area XY1-XY2
        gchar *makeXYinfo(double x, double y); // X,Y
        gchar *makedXdYinfo(double xy1[2], double xy2[2]); // dX, dY
        gchar *makeXYZinfo(double x, double y); // Point: X,Y,Z
        gchar *makeZinfo(double data_z, const gchar *new_prec=NULL, double sub=0.);
        double getZ(double x, double y);
      
        // convert X,Y (view, mouse) to image index
        void XYview2pixel(double x, double y, Point2D *p);

        void Angstroem2W(double &x, double &y);
        void W2Angstroem(double &x, double &y);
        double AngstroemXRel2W(double x);

        int GetQfac() { return Qfac; };
        int GetZfac() { return Zfac; };

        UnitObj *Ux();
        UnitObj *Uy();
        UnitObj *Uz();

        Scan *sc;
private:
        UnitObj *ux;
        UnitObj *uy;
        UnitObj *uz;
        int Qfac, Zfac;
        int pixelmode;
        int showtime;
        SCAN_COORD_MODE sc_mode;
};

#endif
