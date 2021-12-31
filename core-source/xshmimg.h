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


#ifndef __XSHMIMG_H
#define __XSHMIMG_H

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#define IMGMAXCOLORENTRYS 8192
#define HILITCOLORENTRYS  32

/*
 * XShm Image Object
 * ============================================================
 */

#define UUS24(N) ((U24*)((char*)uus+(3*N)))->rgb

typedef union { unsigned long rgb:24; } U24; /* Achtung sizof( U24 ) == 4 !!! */

class ShmImage2D{
public:
        ShmImage2D(GtkWidget *area, 
                   int Width, int Height, 
                   int xorigin, int yorigin,
                   int QuechFactor=1
                   );
        ~ShmImage2D();

        void Resize(int Width, int Height, int QuechFactor=1);

        unsigned long GetMaxCol(){ return maxcol; };
        void MkPalette(const char *name=NULL);

        void saveimage(gchar *name);

        void SetZoomFac(int zf){ ZoomFac = zf; };

        void PutPixel(unsigned long x, unsigned long y, unsigned long val);
        void PutPixel_RGB(unsigned long x, unsigned long y, unsigned long r, unsigned long g, unsigned long b);

        void draw_callback (cairo_t *cr, gboolean draw_red_line=true, gboolean draw_sls_box=true, gboolean draw_tip=false);
        void ShowPic ();
        void ShowSubPic (int xs, int ys, int w, int h, int ytop=0, int yn=0);
        void update_bbox (int y1, int y2);
        void set_translate_offset (gint x, gint y) { tr_xy[0]=x+x0; tr_xy[1]=y+y0; };

        void get_rgb_from_colortable (unsigned long val, double &r, double &g, double &b){
                r = (double)( ULColorTable[val]      & 0xff)/256.;
                g = (double)((ULColorTable[val]>> 8) & 0xff)/256.;
                b = (double)((ULColorTable[val]>>16) & 0xff)/256.;
        };
        
private:

        /*my palette of greys / LookUp Table*/
        unsigned long ULColorTable[IMGMAXCOLORENTRYS+2+HILITCOLORENTRYS];

        double red_line_points[4];
        double red_box_extends[4];
        GdkPixbuf *gdk_pixbuf;
        GtkWidget *imgarea;
        cairo_surface_t *cs_image_layer;

        unsigned int maxcol;
        int rowstride, bpp, width, height, x0, y0;
        guchar *pixels;

        int QuenchFac;
        int ZoomFac;
        gint tr_xy[2];
};

#endif
