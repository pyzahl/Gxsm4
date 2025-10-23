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

#include <locale.h>
#include <libintl.h>

#include <config.h>

#include <time.h>
#include "surface.h"
#include "xsmmasks.h"
#include "glbvars.h"
#include "action_id.h"
#include "xsm.h"
#include "dataio.h"

#include "gxsm_monitor_vmemory_and_refcounts.h"

int scandatacount=0;

Scan::Scan(Scan *scanmaster){
        gxsm4app = scanmaster->gxsm4app;
        GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_SCANOBJ, "constructor");
        GXSM_REF_OBJECT (GXSM_GRC_SCANOBJ);
	TimeList = NULL;
	refcount=0;
	Running = 0;
	objects_id = 0;
	objects_list = NULL;
        world_map = NULL;
	vdata = scanmaster->vdata;;
	mem2d = new Mem2d(data.s.nx=scanmaster->mem2d->GetNx(),data.s.ny=scanmaster->mem2d->GetNy(),data.s.nvalues=scanmaster->mem2d->GetNv(),scanmaster->mem2d->GetTyp()); // MemObj. anlegen
	if (vdata)
		data.CpUnits(*vdata);
	data.s.iyEnd=0;
	data.s.xdir=data.s.ydir=0;
	data.s.pixeltime=0.;
	data.s.tStart=data.s.tEnd=(time_t)0;
	data.s.ntimes=1;
	//data.s.nvalues=1;
	view=NULL; // noch klein View
	VFlg  = scanmaster->VFlg;
	ChanNo= scanmaster->ChanNo;
	X_linearize=scanmaster->X_linearize;
	view_asp_range=scanmaster->view_asp_range;
        for(int i=0; i<4; ++i) ixy_sub[i]=0;
	State = IS_FRESH;
        last_line_updated = -1;
        last_line_updated_time = 0;
}


Scan::Scan(int vtype, int vflg, int ChNo, SCAN_DATA *vd, ZD_TYPE mtyp, Gxsm4app *app){
        gxsm4app = app;
        GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_SCANOBJ, "constructor");
        GXSM_REF_OBJECT (GXSM_GRC_SCANOBJ);
	TimeList = NULL;
	mem2d_refcount = 0;
	refcount = 0;
	Running  = 0;
	objects_id = 0;
	objects_list = NULL;
        world_map = NULL;
	vdata = vd ? vd : &main_get_gapp ()->xsm->data;
	if(vd)
		data.CpUnits(*vdata);
	data.s.iyEnd=0;
	data.s.xdir=data.s.ydir=0;
	data.s.pixeltime=0.;
	data.s.tStart=data.s.tEnd=(time_t)0;
	data.s.ntimes=1;
	//data.s.nvalues=1;
	mem2d = new Mem2d(data.s.nx=vdata->s.nx,data.s.ny=vdata->s.ny,data.s.nvalues=vdata->s.nvalues,mtyp); // MemObj. anlegen
	view=NULL; // noch klein View
	VFlg  = SCAN_V_QUICK;
	ChanNo= ChNo;
	State = IS_FRESH;
	if(vflg)
		VFlg = vflg;
	if(vtype)
		SetView(vtype);
	SetVM(vflg, vdata);
	X_linearize=FALSE;
        view_asp_range=FALSE;
        for(int i=0; i<4; ++i) ixy_sub[i]=0;
}

Scan::~Scan(){
        XSM_DEBUG (DBG_L2, "Scan::~Scan ** closing...");
	if(State==NOT_SAVED)
		;
  	if(view) delete view; // ggf. alten View löschen
	view=NULL;

	free_time_elements ();

        XSM_DEBUG (DBG_L2, "Scan::~Scan ** delete Mem2D");
	delete mem2d; // MemObj. löschen

        XSM_DEBUG (DBG_L2, "Scan::~Scan ** unref all objects");
	destroy_all_objects ();
        GXSM_UNREF_OBJECT (GXSM_GRC_SCANOBJ);
        GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_SCANOBJ, "destructor");

        if (world_map)
                delete world_map;

        XSM_DEBUG (DBG_L2, "Scan::~Scan ** completed.");
}

int Scan::free_time_elements (){
	if (TimeList){
		g_list_foreach (TimeList, (GFunc) Scan::free_time_element, this);
		g_list_free (TimeList);
	}
	TimeList = NULL;
	return 0;
}

int Scan::append_current_to_time_elements (int index, double t, Mem2d* other){
	TimeElementOfScan *tes = new TimeElementOfScan;
	if (!other)
		mem2d->set_frame_time (t);
	tes->index = 0;
	if (index >= 0)
		tes->index = index;
	else
		if (TimeList){
			TimeElementOfScan *tes_last = (TimeElementOfScan*) g_list_last (TimeList);
			index = tes->index = tes_last->index + 1;
		} else {
			index = 0;
			tes->index = 0;
                        t0_ref = t; // set ref time
		}

	if (other)
		tes->mem2d = new Mem2d (other, M2D_COPY);
	else
		tes->mem2d = new Mem2d (mem2d, M2D_COPY);
	tes->mem2d->set_frame_time (t);
	tes->mem2d->set_frame_time (t0_ref, 1);
	tes->mem2d->set_t_index(index); // only here and only!
	tes->refcount = 0;
	tes->sdata = new SCAN_DATA;
	tes->sdata->copy (data);

	TimeList = g_list_append (TimeList, tes);

	data.s.ntimes = g_list_length (TimeList);
	return data.s.ntimes;
}

int Scan::prepend_current_to_time_elements (int index, double t, Mem2d* other){
	TimeElementOfScan *tes = new TimeElementOfScan;
	if (!other)
		mem2d->set_frame_time (t);
	tes->index = 0;
	if (index >= 0)
		tes->index = index;
	else
		if (TimeList){
			TimeElementOfScan *tes_last = (TimeElementOfScan*) g_list_last (TimeList);
			index = tes->index = tes_last->index + 1;
		} else {
			index = 0;
			tes->index = 0;
		}

	if (other)
		tes->mem2d = new Mem2d (other, M2D_COPY);
	else
		tes->mem2d = new Mem2d (mem2d, M2D_COPY);
	tes->mem2d->set_frame_time (t);
	tes->mem2d->set_t_index(index); // set only here and only and by reindex!
	tes->refcount = 0;
	tes->sdata = new SCAN_DATA;
	tes->sdata->copy (data);

	TimeList = g_list_prepend (TimeList, tes);

	data.s.ntimes = g_list_length (TimeList);

	reindex_time_elements ();

	return data.s.ntimes;
}

void Scan::reindex_time_elements (){
	if (! TimeList) return;

	int nte = number_of_time_elements ();
	for (int i=0; i<nte; ++i){
		TimeElementOfScan *tes = (TimeElementOfScan*) g_list_nth_data (TimeList, i);
		tes->index = i;
		tes->mem2d->set_t_index(i); // only here and at app/prepend!
	}
}

// sorting tools
gint compare_time_list_elements_by_index (TimeElementOfScan *a, TimeElementOfScan *b) {
	if (a->mem2d->get_t_index () < b->mem2d->get_t_index ()) return -1;
	if (a->mem2d->get_t_index () > b->mem2d->get_t_index ()) return 1;
        return 0;
}
void Scan::sort_time_elements_by_index (){
 	if (TimeList)
                TimeList = g_list_sort (TimeList,  GCompareFunc (compare_time_list_elements_by_index)); 
}
                        
gint compare_time_list_elements_by_time (TimeElementOfScan *a, TimeElementOfScan *b) {
	if (a->mem2d->get_frame_time () < b->mem2d->get_frame_time ()) return -1;
	if (a->mem2d->get_frame_time () > b->mem2d->get_frame_time ()) return 1;
        return 0;
}
void Scan::sort_time_elements_by_time (){
 	if (TimeList)
                TimeList = g_list_sort (TimeList,  GCompareFunc (compare_time_list_elements_by_time)); 
}
                        
gint compare_time_list_elements_by_bias (TimeElementOfScan *a, TimeElementOfScan *b) {
        // g_message ("Sorting: %s %g <> %s %g", a->sdata->ui.name, a->sdata->s.Bias, b->sdata->ui.name, b->sdata->s.Bias);
	if (a->sdata->s.Bias < b->sdata->s.Bias) return -1;
	if (a->sdata->s.Bias > b->sdata->s.Bias) return 1;
        return 0;
}
void Scan::sort_time_elements_by_bias (){
 	if (TimeList)
                TimeList = g_list_sort (TimeList,  GCompareFunc (compare_time_list_elements_by_bias));
}

gint compare_time_list_elements_by_zsetpoint (TimeElementOfScan *a, TimeElementOfScan *b) {
	if (a->sdata->s.ZSetPoint < b->sdata->s.ZSetPoint) return -1;
	if (a->sdata->s.ZSetPoint > b->sdata->s.ZSetPoint) return 1;
        return 0;
}
void Scan::sort_time_elements_by_zsetpoint (){
 	if (TimeList)
                TimeList = g_list_sort (TimeList,  GCompareFunc (compare_time_list_elements_by_zsetpoint));
}

// list management
int Scan::remove_time_element (int index){
	return 0; // to be completed.
}

int Scan::number_of_time_elements (){
	if (! TimeList)
		return 1; // we always have this !!

	return g_list_length (TimeList);
}

double Scan::retrieve_time_element (int index){
	if (! TimeList) 
		return 0.;

        update_tes ();
        
	TimeElementOfScan *tes = (TimeElementOfScan*) g_list_nth_data (TimeList, index);
	if (tes){
		int v=mem2d->data->GetLayer();
//		std::cout << "Scan::retrieve_time_element index=" << index << std::endl;
		mem2d->copy (tes->mem2d);
		mem2d->data->SetLayer(v);
		mem2d->set_frame_time (tes->mem2d->get_frame_time ());
                mem2d->set_t_index(index);

                if (data.display.px_shift_xy[2] >= 0){
                        mem2d->set_px_shift (data.display.px_shift_xy[0]/data.s.dx, data.display.px_shift_xy[1]/data.s.dy, data.display.px_shift_xy[2]);
                        // g_print ("TES [%d] retrieve Scan::set_display_shift  TC %g %g %g\n", index, tes->mem2d->data->pixshift_dt[0], tes->mem2d->data->pixshift_dt[1], tes->mem2d->data->creepfactor);
                } else {
                        mem2d->data->pixshift_dt[0] = data.display.px_shift_xy[0] = tes->mem2d->data->pixshift_dt[0];
                        mem2d->data->pixshift_dt[1] = data.display.px_shift_xy[1] = tes->mem2d->data->pixshift_dt[1];
                        mem2d->data->creepfactor    = data.display.px_shift_xy[2];
                        // g_print ("TES [%d] retrieve Scan::set_display_shift M    %g %g %g\n", index, data.display.px_shift_xy[0], data.display.px_shift_xy[1], data.display.px_shift_xy[2]);
                        // g_print ("TES [%d] retrieve Scan::set_display_shift MTC= %g %g %g\n", index, tes->mem2d->data->pixshift_dt[0], tes->mem2d->data->pixshift_dt[1], tes->mem2d->data->creepfactor);
                }                
		return tes->mem2d->get_frame_time ();
	}
	return 0.;
}

Mem2d* Scan::mem2d_time_element (int index){
	if (! TimeList) 
		return mem2d;

	TimeElementOfScan *tes = (TimeElementOfScan*) g_list_nth_data (TimeList, index);
	if (tes)
		return tes->mem2d;
	else
		return mem2d;
}


int Scan::get_current_time_element (){
//	g_list_find_custom (TimeList, ...);
//	tes->index = (TimeElementOfScan*)( g_list_last (TimeList)->data)->index + 1;
	return -1;
}


int Scan::add_object (scan_object_data *sod){
	//scan_object_data *sod = new scan_object_data (++objects_id, name, text, np, f_xyi); 
	objects_list = g_slist_prepend (objects_list, sod);
	return sod->get_id ();
}

int Scan::del_object (scan_object_data *sod){
        objects_list = g_slist_remove((GSList*) objects_list, sod);
        delete sod;
	return -1;
}

void delete_sod (scan_object_data *sod, gpointer data){ delete sod; }

void Scan::destroy_all_objects (){
        g_slist_foreach (objects_list, (GFunc) delete_sod, NULL);
        g_slist_free (objects_list);
        //for (int i=0; i<=objects_id; ++i) del_object (i);
}

void Scan::determine_display (int Delta, double sm_eps){
        double hi,lo;
        gboolean success = false;
        int n_obj = number_of_object ();
        Point2D Pkt2d[2];
        hi=lo=0.;

        //g_message ("--> Scan::determin_display");

	if (view)
		view->setup_data_transformation();

        while (n_obj--){
                scan_object_data *scan_obj = get_object_data (n_obj);
		
                if (strncmp (scan_obj->get_name (), "Rectangle", 9) )
                        continue; // only points are used!

                if (scan_obj->get_num_points () != 2) 
                        continue; // sth. is weired!

		//objects[i]->get_xy (0, x, y);
                //r = objects[i]->distance(objects[i-1]);

                
                scan_obj->get_xy_i_pixel2d (0, &Pkt2d[0]);
                scan_obj->get_xy_i_pixel2d (1, &Pkt2d[1]);

                if(data.display.ViewFlg & SCAN_V_SCALE_SMART){
                        mem2d->AutoHistogrammEvalMode (&Pkt2d[0], &Pkt2d[1], Delta, sm_eps);
                }else{
                        if (data.display.ViewFlg & SCAN_V_LOG){
                                mem2d->HiLo (&hi, &lo, FALSE, &Pkt2d[0], &Pkt2d[1], Delta);
                                mem2d->SetHiLo (hi, lo, success);
                        } else
                                mem2d->HiLoMod (&Pkt2d[0], &Pkt2d[1], Delta, success);
                }

                success = true;
        }

        if (!success){
                if(data.display.ViewFlg & SCAN_V_SCALE_SMART)
                        mem2d->AutoHistogrammEvalMode (NULL, NULL, Delta, sm_eps);
                else{
                        if (data.display.ViewFlg & SCAN_V_LOG){
                                mem2d->HiLo (&hi, &lo, FALSE, NULL, NULL, Delta);
                                mem2d->SetHiLo (hi, lo);
                        } else
                                mem2d->HiLoMod (NULL, NULL, Delta);
                }
        }
}

void Scan::auto_display (){
        double hi,lo;

        //g_message ("--> Scan::auto_display");

        // determine ranges on selection(s)
        determine_display (xsmres.HiLoDelta, (double)xsmres.SmartHistEpsilon);

        // update view parameters automatically from selected range(s)
        mem2d->GetZHiLo (&hi, &lo);
        data.display.vrange_z = (hi-lo)*data.s.dz;
        data.display.voffset_z = 0.;
        data.display.z_low  = 0.5*((hi+lo)*data.s.dz - data.display.vrange_z);
        data.display.z_high = data.display.z_low + data.display.vrange_z;
        
        // calculate contrast and bright
        mem2d->AutoDataSkl (&data.display.contrast, &data.display.bright);

        draw ();
}

// from VRange centered on high..low
void Scan::set_display_shift (){
        //g_message ("--> Scan::set_display_shift");

        if (data.display.px_shift_xy[2] > 0.){
                if (data.display.px_shift_xy[0] == 0. && data.display.px_shift_xy[1] == 0.)
                        mem2d->set_px_shift ();
                else
                        mem2d->set_px_shift (data.display.px_shift_xy[0]/data.s.dx, data.display.px_shift_xy[1]/data.s.dy, data.display.px_shift_xy[2]);

                int nte = number_of_time_elements ();
                for (int i=0; i<nte; ++i){
                        TimeElementOfScan *tes = (TimeElementOfScan*) g_list_nth_data (TimeList, i);
                        if (tes)
                                tes->mem2d->set_px_shift (data.display.px_shift_xy[0]/data.s.dx, data.display.px_shift_xy[1]/data.s.dy, data.display.px_shift_xy[2]);
                        g_print ("Scan::set_display_shift TES %d tau %g %g %g\n", i, data.display.px_shift_xy[0], data.display.px_shift_xy[1], data.display.px_shift_xy[2]);
                }
        } else {
                // current only (manaual) for tau < 0
                mem2d->data->pixshift_dt[0] = data.display.px_shift_xy[0];
                mem2d->data->pixshift_dt[1] = data.display.px_shift_xy[1];
                mem2d->data->creepfactor    = data.display.px_shift_xy[2];
                g_print ("M2D [%d] Scan::set_display_shift M %g %g %g\n", mem2d->get_t_index (), data.display.px_shift_xy[0], data.display.px_shift_xy[1], data.display.px_shift_xy[2]);
                if (mem2d->get_t_index () >= 0){
                        TimeElementOfScan *tes = (TimeElementOfScan*) g_list_nth_data (TimeList, mem2d->get_t_index ());
                        if (tes){
                                tes->mem2d->data->pixshift_dt[0] = data.display.px_shift_xy[0];
                                tes->mem2d->data->pixshift_dt[1] = data.display.px_shift_xy[1];
                                tes->mem2d->data->creepfactor    = data.display.px_shift_xy[2];
                                g_print ("TES[%d] Scan::set_display_shift M %g %g %g\n", mem2d->get_t_index (), data.display.px_shift_xy[0], data.display.px_shift_xy[1], data.display.px_shift_xy[2]);
                        }
                }
        }
        draw ();
}

// from VRange centered on high..low
void Scan::set_display (gint lock){
        if (lock)
                data.display.use_high_low = lock;

        if (data.display.use_high_low > 0)
                set_display_hl ();
        else {
                //g_message ("--> Scan::set_display, determine");
                double hi,lo;
                // determine ranges on selection(s)
                determine_display (xsmres.HiLoDelta, (double)xsmres.SmartHistEpsilon);

                // recompute from vrange/offset
                mem2d->SetDataVRangeZ (data.display.vrange_z, 
                                       data.display.voffset_z,
                                       data.s.dz);

                mem2d->GetZHiLo (&hi, &lo);
                data.display.z_low  = 0.5*((hi+lo)*data.s.dz - data.display.vrange_z);
                data.display.z_high = data.display.z_low + data.display.vrange_z;

                // calculate contrast and bright
                mem2d->AutoDataSkl (&data.display.contrast, &data.display.bright);

                draw ();
        }
}

// from High..Low
void Scan::set_display_hl (gint lock){
        double hi,lo;

        if (lock)
                data.display.use_high_low = lock;

        //g_message ("--> Scan::set_display_hl, determine");
        
        // determine ranges on selection(s)
        determine_display (xsmres.HiLoDelta, (double)xsmres.SmartHistEpsilon);
        mem2d->GetZHiLo (&hi, &lo);
        
        // set hi low
        mem2d->SetHiLo (data.display.z_high/data.s.dz, data.display.z_low/data.s.dz);

        // update view parameters
        data.display.vrange_z = (data.display.z_high - data.display.z_low);
        // data.display.voffset_z = 0.5*(hi + lo)*data.s.dz - data.display.z_low + 0.5*data.display.vrange_z;
        data.display.voffset_z = data.display.z_low + 0.5*data.display.vrange_z - 0.5*(hi + lo)*data.s.dz;

        // calculate contrast and bright
        mem2d->AutoDataSkl (&data.display.contrast, &data.display.bright);

        draw ();
}

int Scan::SetVM(int vflg, SCAN_DATA *src, int Delta, double sm_eps){
	if (vflg > 0)
		data.display.ViewFlg=vflg;

        set_display ();
	return 0;
}

void Scan::hide(){;}

void Scan::Activate(){
	XSM_DEBUG (DBG_L2, "Scan::Activate VFlg=" << VFlg);
	
	GetDataSet(*vdata);
	VFlg = data.display.ViewFlg;
	main_get_gapp ()->spm_update_all(VFlg);
}

int Scan::SetView(uint vtype){ 
	if(view) delete view; // ggf. alten View löschen
	view = NULL;
	switch(vtype){
	case ID_CH_V_NO:
		return ID_CH_V_NO;
		break;
	case ID_CH_V_GREY: 
		view = new Grey2D(this, ChanNo);
		SetVM();
		draw();
		return ID_CH_V_GREY;
		break;
	case ID_CH_V_SURFACE:
		//    return ID_CH_V_NO; // just disable
#if ENABLE_3DVIEW_HAVE_GL_GLEW
		view = new Surf3d(this, ChanNo);
		SetVM();
		draw();
#endif
		return ID_CH_V_SURFACE;
		break;
	default:    
		return ID_CH_V_NO;
	}

}

int Scan::draw(int y1, int y2){
        //g_message ("--> Scan::draw");
	if (view){
		mem2d->SetDataSkl (data.display.contrast, data.display.bright);
		//    XSMDEBUGLVL(8,"Scan::draw");
		if(y1>=0) // was >0
			view->update(y1,y2);
		else
			view->draw ();
	}
	//  else XSM_DEBUG (DBG_L2, "no view !");
	return 0;
}

gboolean Scan::show_world_map (gboolean flg){
        if (!world_map)
                return false;
        
        if (flg){
                world_map->SetView(ID_CH_V_GREY);
                world_map->draw();
        } else 
                world_map->SetView(ID_CH_V_NO);

        return true;
}

double arrmin(double *x, int n){
        double m = *x;
        for (int i=0; i<n; ++i, ++x)
                if (*x < m)
                        m = *x;
        return m;
}
double arrmax(double *x, int n){
        double m = *x;
        for (int i=0; i<n; ++i, ++x)
                if (*x > m)
                        m = *x;
        return m;
}

void Scan::update_world_map(Scan *src){
        XSM_DEBUG_GP (DBG_L1, "Scan update world map.");
        if (!world_map){
                world_map = new Scan(ID_CH_V_NO, false, ChanNo, NULL, scan_ztype);
                world_map -> set_main_app (get_app());
                //Scan::Scan(int vtype, int vflg, int ChNo, SCAN_DATA *vd, ZD_TYPE mtyp){
                world_map->data.copy(data);
                world_map->data.s.alpha = 0.;
                world_map->data.s.x0 = 0.;
                world_map->data.s.y0 = 0.;
                world_map->data.s.rx = 2*main_get_gapp ()->xsm->XOffsetMax(); // full accessible world range (offset)
                world_map->data.s.ry = 2*main_get_gapp ()->xsm->YOffsetMax(); // full accessible world range (offset)
                world_map->data.s.nx = xsmres.world_size_nxy; // set in preferences
                world_map->data.s.ny = xsmres.world_size_nxy;
                world_map->data.s.dx = world_map->data.s.rx/(world_map->data.s.nx-1);
                world_map->data.s.dy = world_map->data.s.ry/(world_map->data.s.ny-1);
                world_map->data.s.dz = 1.0;
                world_map->data.s.ntimes  = 1;
                world_map->data.s.nvalues = 1;
                gchar *tmp = g_strdup_printf("World Map for %s", data.ui.title);
                world_map->data.ui.SetTitle (tmp);
                g_free(tmp);
                world_map->data.ui.SetName ("World Map");
                if(vdata) world_map->data.CpUnits (*vdata);
                world_map->mem2d->Resize( world_map->data.s.nx,  world_map->data.s.ny,  world_map->data.s.nvalues, scan_ztype, false);
                world_map->mem2d->data->MkXLookup (-scan_direction*world_map->data.s.rx/2, scan_direction*world_map->data.s.rx/2);
                world_map->mem2d->data->MkYLookup (world_map->data.s.ry/2, -world_map->data.s.ry/2);
        }

        Scan *tmps = src ? src : this;
        double wxy[2][4];
        double rxy[2][4];
        tmps->Pixel2World (0,0, wxy[0][0], wxy[1][0]);
        world_map->World2Pixel (wxy[0][0], wxy[1][0], rxy[0][0], rxy[1][0]);
                
        tmps->Pixel2World (tmps->mem2d->GetNx()-1, tmps->mem2d->GetNy()-1, wxy[0][1], wxy[1][1]);
        world_map->World2Pixel (wxy[0][1], wxy[1][1], rxy[0][1], rxy[1][1]);
                
        tmps->Pixel2World (tmps->mem2d->GetNx()-1,0, wxy[0][2], wxy[1][2]);
        world_map->World2Pixel (wxy[0][2], wxy[1][2], rxy[0][2], rxy[1][2]);
                
        tmps->Pixel2World (0,tmps->mem2d->GetNy()-1, wxy[0][3], wxy[1][3]);
        world_map->World2Pixel (wxy[0][3], wxy[1][3], rxy[0][3], rxy[1][3]);

        int iix[2] = { (int)arrmin(rxy[0],4), (int)arrmax(rxy[0],4)  };
        int iiy[2] = { (int)arrmin(rxy[1],4), (int)arrmax(rxy[1],4)  };
        // transfer / remap data
        for (int iy=iiy[0]; iy < iiy[1]; ++iy)
                for (int ix=iix[0]; ix < iix[1]; ++ix){
                        double wx,wy, rix, riy;
                        world_map->Pixel2World (ix, iy, wx, wy);
                        tmps->World2Pixel (wx, wy, rix,riy);
                        double z = tmps->mem2d->GetDataPktInterpol (rix,riy);
                        if (z != 0.0) // crude simple test
                                world_map->mem2d->PutDataPkt (data.s.dz*z, ix, iy, 0); // scale to actual z, dz_world=1
                }

        gchar *tmp = g_strdup_printf("World Map for %s", world_map->data.ui.title);
        world_map->data.ui.SetTitle (tmp);
        world_map->draw();

}

void Scan::clear_world_map(){
        if (world_map){
                world_map->mem2d->SetData (0., 0,0, world_map->mem2d->GetNx(),world_map->mem2d->GetNy());
                world_map->draw();
        }
}
int Scan::create(gboolean RoundFlg, gboolean subgrid, gdouble direction, gint fast_scan,
                 ZD_TYPE ztype,
                 gboolean keep_layer_info, gboolean remap, gboolean keep_nv){
        Scan *tmp=NULL;
        scan_ztype = ztype;
        scan_direction = direction;
        const gboolean transferdata = (remap && mem2d &&  mem2d->GetNx() > 2 && mem2d->GetNy() > 2 && data.s.nx > 2 && data.s.ny > 2 ) ? true : false;
        if (transferdata){
                tmp = new Scan;
                g_message ("Scan create -- tmp copy from current %d x %d", mem2d->GetNx(), mem2d->GetNy());
                tmp->data.copy (data);
                tmp->data.s.nx = mem2d->GetNx();
                tmp->data.s.ny = mem2d->GetNy();
                tmp->mem2d->copy(mem2d);
                tmp->data.s.ntimes  = 1;
		tmp->data.s.nvalues = mem2d->GetNv(); //1;

                update_world_map ();
        }
        
	if(vdata){
		Display_Param disp_tmp;
		// jetzt alle Geometrie Werte übernehmen
		data.GetScan_Param(*vdata);
		data.GetUser_Info(*vdata);
                data.CpUnits (*vdata);
		data.UpdateUnits();
	}

        mem2d->Resize(data.s.nx, data.s.ny, keep_nv ? -1 : data.s.nvalues, ztype, keep_layer_info);
        //mem2d->Resize(data.s.nx, data.s.ny, -1, ztype, keep_layer_info);

	// check if non linear sine X scale (fast scan set) 
	if (fast_scan){
		double rx2 = data.s.rx/2.;
		for (int i=0; i<data.s.nx; ++i)
			mem2d->data->SetXLookup(i, -rx2 * direction * cos (M_PI*(double)i / (double)(data.s.nx)) );
	} 
	else
		mem2d->data->MkXLookup (-direction*data.s.rx/2, direction*data.s.rx/2);

	switch(data.orgmode){
	case SCAN_ORG_MIDDLETOP:
		mem2d->data->MkYLookup (0., -data.s.ry); break;
	case SCAN_ORG_CENTER:
		mem2d->data->MkYLookup (data.s.ry/2, -data.s.ry/2); break;
	}
	
	// "Anzeige-Daten" aktualisieren
	if (vdata){
		vdata->GetScan_Param(data);
		vdata->GetUser_Info(data);
	} else {
		data.s.nvalues = 1;
		data.s.ntimes = 1;
	}

	data.ui.SetName("noname");
	
        if (transferdata){
                XSM_DEBUG_GP (DBG_L1, "Scan create/transfer -- resizing. %d %d @ %g  -> %d %d @ %g", tmp->data.s.nx, tmp->data.s.ny, tmp->data.s.rx, data.s.nx, data.s.ny, data.s.rx);

                // transfer / remap data
                for (int iy=0; iy < data.s.ny; ++iy)
                        for (int ix=0; ix < data.s.nx; ++ix){
                                double wx,wy, rix, riy;
                                Pixel2World (ix, iy, wx, wy);
                                tmp->World2Pixel (wx, wy, rix,riy);
                                //if ((ix == 0 || ix ==  (data.s.nx-1)) && (iy == 0 || iy ==  (data.s.ny-1)))
                                //        g_message ("T %g %g -[%g]-> %d %d [%g %g]", rix,riy, tmp->mem2d->GetDataPktInterpol (rix,riy), ix, iy, wx, wy);
                                double z = tmp->mem2d->GetDataPktInterpol (rix,riy);
                                if (z == 0.0 && world_map){ // try remapping from world_map!
                                        world_map->World2Pixel (wx, wy, rix,riy);
                                        // update from world map
                                        z = world_map->mem2d->GetDataPktInterpol (rix,riy)/data.s.dz;
                                }
                                mem2d->PutDataPkt (z, ix, iy, 0);
                        }
                delete tmp;
        }
        
	XSM_DEBUG (DBG_L2, "Scan::create done");
	State= IS_NEW;
	Running = 0;
	
	return 0;
}

void Scan::start(int l, double lv){	
	time_t t; // Frame Startzeit
	time(&t);

	inc_refcount ();
        g_message ("SCAN[%02d]::START. INC_REFCOUNT = %d",ChanNo,get_refcount());

	if (l==0){
		data.UpdateUnits ();
		data.s.tStart = time (0);
		data.s.ntimes = 1;
		data.ui.SetOriginalName (storage_manager.get_name ("(not saved)")); // ("unknown (not saved)");
#if 0
		if (vdata){
			vdata->ui.SetOriginalName (storage_manager.get_name ("(not saved)"));
			main_get_gapp ()->ui_update();
		}
#endif
		// clean up LayerInformation and auto add basic set
		mem2d->remove_layer_information ();
	}

	mem2d->SetLayer (l);
	mem2d->SetLayerDataPut (l);
	mem2d->data->SetVLookup (l, lv);
        // mem2d->data->ResetLineInfo ();
        if (fabs (main_get_gapp ()->xsm->data.s.Bias) >= 0.2) // auto V / mV 
                mem2d->add_layer_information (new LayerInformation ("Bias", main_get_gapp ()->xsm->data.s.Bias, "%5.3f V"));
        else
                mem2d->add_layer_information (new LayerInformation ("Bias", 1e3*main_get_gapp ()->xsm->data.s.Bias, "%5.3f mV"));
	mem2d->add_layer_information (new LayerInformation ("Layer", l, "%03.0f"));
	mem2d->add_layer_information (new LayerInformation ("Name", data.ui.name));
	mem2d->add_layer_information (new LayerInformation (data.ui.dateofscan)); // Date of scan (series) start
        if (main_get_gapp ()->xsm->data.s.Current >= 0.5) // < 0.5nA? // auto nA / pA
                mem2d->add_layer_information (new LayerInformation ("Current", main_get_gapp ()->xsm->data.s.Current, "%5.2f nA"));
        else
                mem2d->add_layer_information (new LayerInformation ("Current", 1e3*main_get_gapp ()->xsm->data.s.Current, "%5.2f pA"));
	mem2d->add_layer_information (new LayerInformation ("Layer-Param", lv, "%5.3f [V]"));
	mem2d->add_layer_information (new LayerInformation ("Frame-Start",ctime(&t))); // Date for frame start (now)
	mem2d->add_layer_information (new LayerInformation ("X-size original", data.s.rx, "Rx: %5.1f \303\205"));
	mem2d->add_layer_information (new LayerInformation ("Y-size original", data.s.ry, "Ry: %5.1f \303\205"));
	mem2d->add_layer_information (new LayerInformation ("SetPoint", data.s.SetPoint, "%5.2f V"));
	mem2d->add_layer_information (new LayerInformation ("ZSetPoint", data.s.ZSetPoint, "%5.2f  \303\205"));

        Running = 1;
}

void Scan::stop(int StopFlg, int line){
	data.s.tEnd = time (0);
#if 0 // this is saving memory, but causes a lot of trouble elsewhere...
	if (StopFlg && main_get_gapp ()->xsm->hardware->FreeOldData ()){
		mem2d->Resize (data.s.nx, data.s.ny=line);
		data.s.ry = data.s.ny*data.s.dy;
		draw ();
	}
#endif
	data.s.iyEnd=line;
	mem2d->SetLayer (0);
	Running = 0;
	dec_refcount ();
        g_message ("SCAN[%02d]::STOP. DEC_REFCOUNT = %d",ChanNo,get_refcount());
#if 0
	if (vdata){
		vdata->ui.SetOriginalName ("unknown (not saved)");
		main_get_gapp ()->ui_update(); // hack as long signal "changed" did not work
		data.ui.SetComment (vdata->ui.comment);
	}
#endif
}

// get updated copy of user fields comment, ...
void Scan::CpyUserEntries(SCAN_DATA &src){
	data.GetUser_Info (src);
}

// get complete data set
void Scan::CpyDataSet(SCAN_DATA &src){
	data.GetScan_Param (src);
// **	data.GetLayer_Param (src);
	data.GetUser_Info (src);
// **	data.GetDSP_Param (src);
	data.GetDisplay_Param (src);
	data.UpdateUnits ();
}

// put back dataset without dsp-params
void Scan::GetDataSet(SCAN_DATA &dst){
	double x0,y0;
	x0 = dst.s.x0; // keep for restore if running scan
	y0 = dst.s.y0;

	dst.GetScan_Param (data);

	if (Running){ // keep, i.e. restore User-Scan-Offset if scan is running -- NEW 20060915-PZ-fix !!
		dst.s.x0 = x0;
		dst.s.y0 = y0;
	}

	if (!Running) // do not put back if scan is running!!
		dst.GetUser_Info (data);

	dst.GetDisplay_Param (data);
// **	if (IS_SPALEED_CTRL) // need to get Energy and gatettime !
// **		dst.GetDSP_Param (data);
	dst.UpdateUnits ();
// **	dst.GetLayer_Param (data);
	dst.SetZUnit (data.Zunit);

//	std::cout << "Scan::GetDataSet  s.ntimes=" << dst.s.ntimes << std::endl;
//	std::cout << "...               s.values=" << dst.s.nvalues << std::endl;

}

int Scan::Pixel2World (int ix, int iy, double &wx, double &wy, SCAN_COORD_MODE scm){
	switch (scm){
	case SCAN_COORD_ABSOLUTE:
	{
		double s = sin (data.s.alpha*M_PI/180.);
		double c = cos (data.s.alpha*M_PI/180.);
		double rx = mem2d->data->GetXLookup (ix);
		double ry = mem2d->data->GetYLookup (iy);
		// offset + rotation
		wx = data.s.x0 + c*rx + s*ry;
		wy = data.s.y0 - s*rx + c*ry;
	}
	break;
	case SCAN_COORD_RELATIVE:
		wx = mem2d->data->GetXLookup (ix);
		wy = mem2d->data->GetYLookup (iy);
		break;
	}
	return 0;
}

int Scan::Pixel2World (double ix, double iy, double &wx, double &wy, SCAN_COORD_MODE scm){
	double rx  = mem2d->data->GetXLookup ((int)round(ix));
	double rx2 = mem2d->data->GetXLookup ((int)round(ix)+1);
	rx += (rx2-rx)*(ix-round(ix));
	double ry  = mem2d->data->GetYLookup ((int)round(iy));
	double ry2 = mem2d->data->GetYLookup ((int)round(iy)+1);
	ry += (ry2-ry)*(iy-round(iy));
	switch (scm){
	case SCAN_COORD_ABSOLUTE:
	{
		double s = sin (data.s.alpha*M_PI/180.);
		double c = cos (data.s.alpha*M_PI/180.);
		// offset + rotation
		wx = data.s.x0 + c*rx + s*ry;
		wy = data.s.y0 - s*rx + c*ry;
	}
	break;
	case SCAN_COORD_RELATIVE:
		wx = rx;
		wy = ry;
		break;
	}
	return 0;
}

int Scan::World2Pixel (double wx, double wy, int &ix, int &iy, SCAN_COORD_MODE scm){
	double rx=0.,ry=0.;

	World2Pixel (wx, wy, rx, ry, scm);

	ix = (int)round(rx);
	iy = (int)round(ry);

	return 0;
}

int Scan::World2Pixel (double wx, double wy, double &ix, double &iy, SCAN_COORD_MODE scm){
	double rx=0.,ry=0.;

	switch (scm){
	case SCAN_COORD_ABSOLUTE:
	{
		double s = sin (data.s.alpha*M_PI/180.);
		double c = cos (data.s.alpha*M_PI/180.);
		wx -= data.s.x0; // remove offset
		wy -= data.s.y0;
		rx = c*wx - s*wy; // inverse rotation
		ry = s*wx + c*wy;
	}
	break;
	case SCAN_COORD_RELATIVE:
		rx = wx;
		ry = wy;
		break;
	}

	ix = rx / data.s.dx + (double)data.s.nx/2.;
	iy = -ry / data.s.dy + ((data.orgmode == SCAN_ORG_CENTER)? (double)data.s.ny/2. : 0.); // was - (minus) testing if + work now...

	return 0;
}

int Scan::Save(gboolean overwrite, gboolean check_file_exist){
        Dataio *Dio = NULL;

        if(g_strrstr (data.ui.originalname,"(in progress)"))
                overwrite = true;

        CpyUserEntries(main_get_gapp ()->xsm->data);
        
        if (check_file_exist){
                if (g_file_test (storage_manager.get_filename(), G_FILE_TEST_EXISTS)) // | G_FILE_TEST_IS_DIR)))
                        return -1;
                else
                        return 0;
        }
        if (!overwrite){
                if (g_file_test (storage_manager.get_filename(), G_FILE_TEST_EXISTS))
                        return -1;
        }
        Dio = new NetCDF(this, storage_manager.get_filename());

        if (Dio){
                Dio->Write();
                if (Dio->ioStatus()){
                        main_get_gapp ()->SetStatus(N_("Error"), Dio->ioStatus());
                        main_get_gapp ()->monitorcontrol->LogEvent("*Error Saving", storage_manager.get_filename());
                        XSM_SHOW_ALERT(ERR_SORRY, Dio->ioStatus(), storage_manager.get_filename(),1);
                }
                else{
                        Saved();
                        main_get_gapp ()->monitorcontrol->LogEvent("*Save", storage_manager.get_filename());
                        main_get_gapp ()->SetStatus(N_("Saving done "), storage_manager.get_name());
                        data.ui.SetOriginalName (storage_manager.get_name ("(*saved)")); // update without path
                }

                delete Dio;
        } else
                return -1;

        return 0;
}

int Scan::Update_ZData_NcFile(){

        // check for file name
	if (!storage_manager.get_filename()){
                g_warning("No file name yet, please save first to update later!");
                return -1;
        }
        
        try {
                // open in write mode
                std::string filename = storage_manager.get_filename();
                NcFile nc(filename, netCDF::NcFile::write);
                
                // read Data variable
                NcVar Data;
                
                switch (mem2d->GetTyp()){
                case ZD_DOUBLE:  Data = nc.getVar("DoubleField"); break; // Double data field ?
                case ZD_FLOAT:   Data = nc.getVar("FloatField"); break;  // Float data field ?
                case ZD_SHORT:   Data = nc.getVar("H"); break;           // standart "SHORT" Topo Scan ?
                case ZD_LONG:    Data = nc.getVar("Intensity"); break;   // used by "Intensity" -- diffraction counts
                case ZD_BYTE:    Data = nc.getVar("ByteField"); break;   // Byte ?
                case ZD_COMPLEX: Data = nc.getVar("ComplexDoubleField"); break; // Complex ?
                case ZD_RGBA:    Data = nc.getVar("RGBA_ByteField"); break;     // RGBA Byte ?
                default:    g_critical("Sorry, compatible NetCDF Data field variable not found."); return -1;
                }
                
                if (!Data.isNull()){
                        g_critical("Sorry, NetCDF file ZData type is incompatible. Please save this scan first to update later!");
                        return -1;
                }

                data.ui.SetOriginalName (storage_manager.get_name ("(in progress)"));
                
                g_message("Data in NetCDF file >%s< is now updated!", storage_manager.get_filename());
                main_get_gapp ()->monitorcontrol->LogEvent("*Update", storage_manager.get_filename());
                main_get_gapp ()->SetStatus(N_("Update done "), storage_manager.get_name());

                if (data.s.ntimes == 1){
                        mem2d->data->NcPut (Data, 0, true);
                } else {
                        for (int time_index=0; time_index<data.s.ntimes; ++time_index)
                                mem2d_time_element(time_index)->data->NcPut (Data, time_index, true);
                }

                return 0;
                
        } catch (const netCDF::exceptions::NcException& e) {
                std::cerr << "EE: NetCDF File Error: " << e.what() << std::endl;
                g_warning("NetCDF file is not valid, please save first to update later! Trying Auto Save now...");
                if (Save ()){ // try auto save, do not overwrite automatically
                        GtkWidget *dialog = gtk_message_dialog_new (NULL,
                                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                                    GTK_MESSAGE_WARNING,
                                                                    GTK_BUTTONS_YES_NO,
                                                                    N_("File '%s' exists or can't be written, try overwrite?"),
                                                                    storage_manager.get_filename());
                        // FIX-ME-GTK4
                        int overwrite = GTK_RESPONSE_YES;
                        g_signal_connect (dialog, "response", G_CALLBACK (gtk_window_destroy), NULL);
                        gtk_widget_show (dialog);
                        //int overwrite = gtk_dialog_run (GTK_DIALOG (dialog));
                        //gtk_widget_destroy (dialog);
                        if (overwrite != GTK_RESPONSE_YES){
                                main_get_gapp ()->SetStatus(N_("File exists, save aborted by user."));
                                return -1; // abort
                        } else {
                                Save (true); // force overwrite as requested
                                return 0;
                        }
                }
                data.ui.SetOriginalName (storage_manager.get_name ("*(in progress)"));

                return 0; // saved full
        }
}
