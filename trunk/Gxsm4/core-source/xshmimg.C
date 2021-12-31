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

#include <iostream>
#include <fstream>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "xshmimg.h"
#include "util.h"

#include "glbvars.h"

ShmImage2D::ShmImage2D(GtkWidget* area, 
		       int Width, int Height, 
		       int xorigin, int yorigin,
                       int QuechFactor
		       ){
        tr_xy[0] = tr_xy[1] = 0.;
        
        x0 = xorigin; 
        y0 = yorigin;
        QuenchFac = QuechFactor;

        maxcol  = 0;
        ZoomFac = 1;
        imgarea = area;

        MkPalette();

        gdk_pixbuf     = NULL;
        cs_image_layer = NULL;

        Resize(Width, Height);
}

void ShmImage2D::Resize(int Width, int Height, int QuechFactor){
        XSM_DEBUG(DBG_L3, "ShmImage2D::Resize" << Width << "x" << Height );

        width  = Width;
        height = Height;
        QuenchFac = QuechFactor;
        red_line_points[2] = x0;
        red_line_points[0] = x0 + (width-1)*ZoomFac;
        red_line_points[1] = red_line_points[3] = y0;

        // Setup Image
        GdkPixbuf *gdk_pixbufold=gdk_pixbuf;

        gdk_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, width, height);
        bpp        = gdk_pixbuf_get_bits_per_sample(gdk_pixbuf) * gdk_pixbuf_get_n_channels(gdk_pixbuf) / 8;
        rowstride  = gdk_pixbuf_get_rowstride(gdk_pixbuf);
        pixels     = gdk_pixbuf_get_pixels(gdk_pixbuf);

        if (gdk_pixbufold)
                g_object_unref (G_OBJECT (gdk_pixbufold));

        XSM_DEBUG(DBG_L3, "ShmImage2D::Resize done" );
}

ShmImage2D::~ShmImage2D(){
        g_object_unref (G_OBJECT (gdk_pixbuf));
}


void ShmImage2D::MkPalette(const char *name){
	// Make Color Look Up Table fpr Grey-Scale -> TrueColor
	unsigned long r, g, b, cval;

	if (name){
		if (strncmp (name, "NATIVE_L4_RGBA", 14) == 0){
			maxcol = 256;
			return;
		}

		std::ifstream cpal;
		char pline[256];
		int nx,ny;
		cpal.open(name, std::ios::in);
		if(cpal.good()){
			cpal.getline(pline, 255);
			cpal.getline(pline, 255);
			cpal >> nx >> ny;
			cpal.getline(pline, 255);
			cpal.getline(pline, 255);
			
			for(maxcol = MIN(nx, IMGMAXCOLORENTRYS), cval=0; cval<maxcol; ++cval){
				cpal >> r >> g >> b;
				if (WORDS_BIGENDIAN)
					ULColorTable[cval] = (r << 16) | (g << 8) | b ;
				else
					ULColorTable[cval] = (b << 16) | (g << 8) | r ;
			}
			return;
		}
		XSM_SHOW_ALERT(ERR_SORRY, ERR_PALFILEREAD, name, 1);
	}
	// default grey and fallback mode:
	for (maxcol=IMGMAXCOLORENTRYS, cval=0; cval<maxcol; ++cval){
		r = b = g = cval * 255 / maxcol; 
		ULColorTable[cval] = (b << 16) | (g << 8) | r ;
	}
	
	// red/blue entries at end
	if (WORDS_BIGENDIAN){
		ULColorTable[IMGMAXCOLORENTRYS  ] = (255 << 16) | (0 << 8) | 0 ;
		ULColorTable[IMGMAXCOLORENTRYS+1] = (0 << 16) | (0 << 8) | 255 ;
	} else {
		ULColorTable[IMGMAXCOLORENTRYS  ] = (0 << 16) | (0 << 8) | 255 ;
		ULColorTable[IMGMAXCOLORENTRYS+1] = (255 << 16) | (0 << 8) | 0 ;
	}

	for(cval=IMGMAXCOLORENTRYS+2; cval<IMGMAXCOLORENTRYS+2+HILITCOLORENTRYS; ++cval){
                g=b=cval-(IMGMAXCOLORENTRYS+2); r=255;
                if (WORDS_BIGENDIAN)
                        ULColorTable[cval] = (r << 16) | (g << 8) | b ;
                else
                        ULColorTable[cval] = (b << 16) | (g << 8) | r ;
	}
}

void ShmImage2D::saveimage(gchar *name){
}

void ShmImage2D::draw_callback (cairo_t *cr, gboolean draw_red_line, gboolean draw_sls_box, gboolean draw_tip){
	gdk_cairo_set_source_pixbuf (cr, gdk_pixbuf, 0., 0.);
	cairo_paint(cr);
        
        if (draw_red_line){
                cairo_set_line_width (cr, ZoomFac);
                cairo_set_source_rgb (cr, 1.0, 0.0, 0.0); // red
                cairo_move_to (cr, red_line_points[0], red_line_points[1]);
                cairo_line_to (cr, red_line_points[2], red_line_points[3]);
                cairo_stroke (cr);
        }

        if (draw_sls_box){
                if (red_box_extends[3] > 0){
                        cairo_set_line_width (cr, 1.5*ZoomFac);
                        cairo_set_source_rgba (cr, 1.0, 1.0, 0.0, 0.5); // yellow alpha 50%
                        cairo_move_to (cr, red_box_extends[0], red_box_extends[2]);
                        cairo_line_to (cr, red_box_extends[0]+red_box_extends[1], red_box_extends[2]);
                        cairo_line_to (cr, red_box_extends[0]+red_box_extends[1], red_box_extends[2]+red_box_extends[3]);
                        cairo_line_to (cr, red_box_extends[0], red_box_extends[2]+red_box_extends[3]);
                        cairo_line_to (cr, red_box_extends[0], red_box_extends[2]);
                        cairo_stroke (cr);
                }
        }
        if (draw_tip){
                double x,y,z;
                gapp->xsm->hardware->RTQuery ("P", x,y,z);
                //g_message ("PXYZ: %g %g %g", x,y,z);

                x *= ZoomFac;
                y *= ZoomFac;
                x /= QuenchFac;
                y /= QuenchFac;
                
                cairo_save (cr);
                cairo_translate (cr, x, y-2*14.);
                cairo_scale (cr, 2.,2.);

                cairo_move_to (cr, 0, 0);
                cairo_rel_curve_to (cr,
                                    -2.76142,0,
                                    -5,2.23858,
                                    -5,5);
                cairo_rel_curve_to (cr,
                                    0,0.17259,
                                    0.0142,0.33191,
                                    0.0312,0.5);
                cairo_rel_curve_to (cr,
                                    0.0137,0.16725,
                                    0.0358,0.33617,
                                    0.0625,0.5);
                cairo_rel_curve_to (cr,
                                    0.57248,3.51444,
                                    2.9063,6.00336,
                                    4.9063,8.00336);
                cairo_rel_curve_to (cr,
                                    2,-2,
                                    4.33372,-4.48892,
                                    4.9062,-8.00336);
                cairo_rel_curve_to (cr,
                                    0.0267,-0.16383,
                                    0.0488,-0.33275,
                                    0.0625,-0.5);
                cairo_rel_curve_to (cr,
                                    0.0171,-0.16809,
                                    0.0312,-0.32741,
                                    0.0312,-0.5);
                cairo_rel_curve_to (cr,
                                    0,-2.76142,
                                    -2.23858,-5,
                                    -5,-5);
                cairo_close_path (cr);
                cairo_move_to (cr, 0.,3.);
                cairo_rel_curve_to (cr,
                                    1.10457,0,
                                    2,0.89543,
                                    2,2);
                cairo_rel_curve_to (cr,
                                    0,1.10457,
                                    -0.89543,2,
                                    -2,2);
                cairo_rel_curve_to (cr,
                                    -1.10457,0,
                                    -2,-0.89543,
                                    -2,-2);
                cairo_rel_curve_to (cr,
                                    0,-1.10457,
                                    0.89543,-2,
                                    2,-2); 
                cairo_close_path (cr);
                cairo_set_source_rgba (cr, 1.0, 1.0, 0.2, 0.6);
                cairo_fill (cr);
                cairo_stroke (cr);

                cairo_restore (cr);
        }
}

void ShmImage2D::ShowPic (){ 
	red_line_points[1] = red_line_points[3] = y0;
        gtk_widget_queue_draw (imgarea);
}

void ShmImage2D::ShowSubPic (int xs, int ys, int w, int h, int ytop, int yn){ 
	xs*=ZoomFac; ys*=ZoomFac;
	w*=ZoomFac; h*=ZoomFac;
	if (xs >= width || ys >= height || w > width || h > height)
		return;

	red_line_points[0] = xs; red_line_points[2] = xs+w;
	red_line_points[1] = red_line_points[3] = ys+y0 + .5*ZoomFac;
        red_box_extends[0]=xs;
        red_box_extends[1]=w;
        red_box_extends[2]=ytop*ZoomFac;
        red_box_extends[3]=yn*ZoomFac;
        
	// gtk_widget_queue_draw_area (imgarea, tr_xy[0], tr_xy[1]+ys*ZoomFac-1, tr_xy[0]+width-1, tr_xy[1]+(ys+1)*ZoomFac+1);
        gtk_widget_queue_draw (imgarea);
}

void ShmImage2D::update_bbox(int y1, int y2){
        
	// gtk_widget_queue_draw_area (imgarea, tr_xy[0], tr_xy[1]+y1*ZoomFac-1, tr_xy[0]+(width-1)*ZoomFac, tr_xy[1]+y2*ZoomFac+1);
        gtk_widget_queue_draw (imgarea);
}

void ShmImage2D::PutPixel(unsigned long x, unsigned long y, unsigned long val){
#if 0
	if (x >= width || y >= height) std::cout << "Invalid PPix x,y: " << x << "," << y << std::endl;
	else 
#endif
                ((U24*)(pixels + rowstride*y + bpp*x))->rgb = ULColorTable[val];
}


void ShmImage2D::PutPixel_RGB(unsigned long x, unsigned long y, unsigned long r, unsigned long g, unsigned long b){
#if 0
	if (x >= width || y >= height) std::cout << "Invalid PPix x,y: " << x << "," << y << std::endl;
	else 
#endif
                ((U24*)(pixels + rowstride*y + bpp*x))->rgb = (WORDS_BIGENDIAN) ? (r << 16) | (g << 8) | b : (b << 16) | (g << 8) | r ;
}


