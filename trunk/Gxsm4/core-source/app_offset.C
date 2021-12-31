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

/*
 * Project: Gxsm
 */

#include "gxsm_app.h"

GrOffsetControl::GrOffsetControl(int Size){
  size = Size;
  AppWindowInit("Offset");
  gtk_window_set_default_size(GTK_WINDOW(widget), size, size);
  
  canvas = gnome_canvas_new();
  
  gtk_widget_pop_visual();
  gtk_widget_pop_colormap();
  
  gnome_canvas_set_pixels_per_unit( GNOME_CANVAS(canvas), (double)(size/2.));
  gnome_canvas_set_scroll_region( GNOME_CANVAS(canvas), -1., -1., 1., 1.);
  
  ScanAnim = gnome_canvas_item_new(gnome_canvas_root( GNOME_CANVAS(canvas)),
				   gnome_canvas_rect_get_type(),
				   "x1",-.5,
				   "y1", .2,
				   "x2", .5,
				   "y2", .5,
				   "fill_color","grey80",
				   "width_pixels",0,
				   NULL);
  
  ScanArea = gnome_canvas_item_new(gnome_canvas_root( GNOME_CANVAS(canvas)),
				   gnome_canvas_rect_get_type(),
				   "x1",-.5,
				   "y1",-.5,
				   "x2", .5,
				   "y2", .5,
				   "fill_color", "grey30",
				   "outline_color", "yellow",
				   "width_pixels", 2,
				   NULL);
  
  double xy[] = { -1.,0., 0.,0., 0.,1., 0.,-1., 0.,0., 1.,0. };
  GnomeCanvasPoints *pn =  gnome_canvas_points_new (6);
  for(int i=0; i<2*6; ++i) pn->coords[i] = xy[i];
  Cross = gnome_canvas_item_new(gnome_canvas_root( GNOME_CANVAS(canvas)),
				gnome_canvas_line_get_type(),
				"points", pn,
				"fill_color", "blue",
				"width_pixels", 2,
				NULL);
  
  gnome_canvas_points_unref(pn);
  gtk_box_pack_start (GTK_BOX (vbox), canvas, TRUE, TRUE, 0);
  gtk_widget_show( canvas );
}

GrOffsetControl::~GrOffsetControl(){
}


void GrOffsetControl::Offset(double x0, double y0){
}
