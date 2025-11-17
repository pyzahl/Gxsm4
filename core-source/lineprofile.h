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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

#ifndef __LINEPROFILE_H
#define __LINEPROFILE_H

#include <iostream>
#include <fstream>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>

#include <gtk/gtk.h>


class Scan;
class UnitObj;
class VObject;

/*
 * LineProfile Class
 */
class LineProfile1D{
public:
        LineProfile1D();
        LineProfile1D(int n, UnitObj *ux, UnitObj *uy, double xmin=0., double xmax=1., int ns=1);
        virtual ~LineProfile1D();

        double GetPoint(int n, int s=0);
        void SetPoint(int n, double y, int s=0);
        void SetPoint(int n, double x, double y, int s=0);
        void AddPoint(int n, double a, int s=0);
        void MulPoint(int n, double f, int s=0);

        void AddNextSectionIndex (int si) { SectionIndexList = g_slist_append (SectionIndexList, GINT_TO_POINTER(si)); };
        int  GetNextSectionIndex (int i) { return GPOINTER_TO_INT (g_slist_nth_data (SectionIndexList, i)); };
        void ClearSectionIndexList () { if (SectionIndexList) { g_slist_free (SectionIndexList); SectionIndexList = NULL; }};

        int SetData_redprofile(Scan *sc, int redblue='r');
        int SetData(Scan *sc, int line=-1);
        int SetData(Scan *sc, VObject *vo, gboolean append=FALSE);
        int load(const gchar *fname);
        int save(const gchar *fname);
  
        Scan *scan1d;
        Scan *scan1d_2;

        GSList *SectionIndexList;

protected:
        gchar **xcolors_list;
        gchar **xcolors_list_2;
        int   num_xcolors;
        int   num_xcolors_2;

private:
        Scan *private_scan1d;
        Scan *private_scan1d_2;
        int ChanNo;
};

#endif



