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

#ifndef __SCAN_OBJECT_DATA_H
#define __SCAN_OBJECT_DATA_H

#include "gxsm_monitor_vmemory_and_refcounts.h"

/**
 *  scan_object_ixy_func:
 *
 *  Coordinate access function type.
 */

// this is the type of the current active object...
// we need to get rid of this...
typedef enum { 
        O_NONE,
        O_POINT,
        O_LINE,
        O_RECTANGLE,
        O_POLYLINE,
        O_PARABEL,
        O_CIRCLE,
        O_TRACE,
        O_EVENT,
        O_KSYS
} V_OBJECT_TYPE;

/**
 *  scan_object_data:
 *  
 *  @_id: object id.
 *  @_name: object name.
 *  @_text: object text.
 *  @_np: number of points used by this object - can not be changed later.
 *  @f_ixy: Pointer to function #scan_object_ixy_func, which provides access 
 *  to the object coodrinates.
 *
 *  Constructor of class scan_object_data. Pure virtual base class.
 */

class scan_object_data{
public:
	scan_object_data (){
                GXSM_REF_OBJECT (GXSM_GRC_MEM2D_SCO);
	};
	virtual ~scan_object_data (){
                GXSM_UNREF_OBJECT (GXSM_GRC_MEM2D_SCO);
	};

        /**
         *  dump:
         *
 *  Dumps object information to stdout.
 */
	void dump(){
		XSM_DEBUG (DBG_L2, 
                           "Objectid = " << get_id () << std::endl
                           << "Name = " << get_name () << std::endl
                           << "Text = " << get_text () << std::endl
                           << "NumP = " << get_num_points () );

		for (int i=0; i < get_num_points (); ++i){
                        Point2D p2d;
                        double xy[2];
                        get_xy_i (i, xy[0], xy[1]);
                        get_xy_i_pixel2d (i, &p2d);
                        g_message ("P[%d] = [%g, %g]A = [%d, %d]px", i, xy[0], xy[1], p2d.x, p2d.y);
                }
	};


/**
 *  get_xy:
 *  @i: index of coordinates to get (in Ang).
 *  @x: variable x to get.
 *  @y: variable y to get.
 *
 *  Gets the coordinates of the @i-th point.
 */

	double distance (scan_object_data *other, int i=0) {
                double xy[2];
                double oxy[2];
                get_xy_i (i, xy[0], xy[1]);
                other->get_xy_i (i, oxy[0], oxy[1]);

		double dx = oxy[0] - xy[0];
		double dy = oxy[1] - xy[1];
                
		return sqrt (dx*dx+dy*dy); 
	};

 
	void get_xy_i_pixel (int i, double &x, double &y) { 
                Point2D p2d;
                get_xy_i_pixel2d (i, &p2d);
		x=p2d.x; y=p2d.y;
	}; 

 /**
 *  get_name:
 *
 *  Get the objects name.
 *
 *  Returns: pointer to name, do not modifiy!
 */
	virtual gchar *get_name ()=0;

/**
 *  get_text:
 *
 *  Get the objects text.
 *
 *  Returns: pointer to text, do not modifiy!
 */
	virtual gchar *get_text ()=0;

/**
 *  get_num_points:
 *
 *  Get the number of points, used by this object.
 *
 *  Returns: number of points.
 */
	virtual gint get_num_points ()=0;

/**
 *  get_id:
 *
 *  Get the objects id.
 *
 *  Returns: object id.
 */
	virtual gint get_id ()=0;


/**
 *  get_xy_i:
 *
 *  Get the objects i-th point xy coordinate in absolute Angstroems (xy base unit).
 */
	virtual void get_xy_i (int i, double &x, double &y)=0;

/**
 *  get_id:
 *
 *  Get the objects i-th point xy coordinate in pixels in Point2D struct.
 */
        virtual void get_xy_i_pixel2d (int i, Point2D *p)=0;

 	virtual void SetUpScan()=0;
 	virtual void set_offset()=0;
 	virtual void show_label (gboolean flg=true)=0;
 	virtual void set_object_label(const gchar *lab)=0;
};

#endif
