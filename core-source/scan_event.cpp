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

#include "scan_event.h"
#include "mem2d.h"

#include <netcdf>
using namespace netCDF;

/*
 * Base Class for Event Data Storage
 * 
 *
 * Class: EventEntry
 */

EventEntry::EventEntry (const gchar *Name, time_t Time, const gchar *file_name){ 
	XSM_DEBUG (DBG_L6, "EventEntry::EventEntry");
	fsrc_name = NULL;
	name = g_strdup (Name);
	time = Time;
	if (file_name)
                fsrc_name = g_strdup (file_name);
}
 
EventEntry::~EventEntry (){ 
	XSM_DEBUG (DBG_L6, "EventEntry::~EventEntry");
	g_free (name);
        if (fsrc_name)
                g_free (fsrc_name);
}

time_t EventEntry::get_time () { return time; };

gchar* EventEntry::get_str_time () { return NULL; };

ProbeEntry::ProbeEntry (const gchar *Name, time_t Time, GPtrArray *Labels, GPtrArray *Unitssymbols, int Chunk_size, const gchar *file_name) : EventEntry (Name, Time, file_name){ 
	XSM_DEBUG (DBG_L6, "ProbeEntry::ProbeEntry");
	chunk_size = Chunk_size;
	data = g_array_new (FALSE, TRUE, sizeof (double));
	labels = Labels;
	unitssymbols =Unitssymbols;
	num_sets=0;
        parent_scan_event = NULL;
}
 
ProbeEntry::ProbeEntry (const gchar *Name, NcFile &ncf, int p_index, NcVar &evdata_var, NcVar &evcoords_var, ScanEvent* &se, int num, int &count, ProbeEntry *pe) : EventEntry (Name, 0){ 
	gchar *tmp;

	XSM_DEBUG_GP (DBG_L4, "ProbeEntry::ProbeEntry ** Scan NC-File for Entry [%d] #%d", p_index, num);

	se = NULL;

	if (!pe){

		chunk_size = 0;
		num_sets=0;
		data = NULL;
		labels = NULL;
		unitssymbols = NULL;

		tmp = g_strdup_printf ("Event_Probe_%05d_coords", p_index);
		XSM_DEBUG_GP (DBG_L4, "ProbeEntry::ProbeEntry ... scanning for ev set [%s]", tmp);
		evcoords_var = ncf.getVar(tmp); 
		g_free (tmp);

		// found that Event_Probe, else return?
		if (evcoords_var.isNull ()){ // not OK?
                        XSM_DEBUG_GP (DBG_L4, "ProbeEntry::ProbeEntry ... not present, done.");
			return;
                }

		tmp = g_strdup_printf ("Event_Probe_%05d_data", p_index); 
		XSM_DEBUG_GP (DBG_L1, "ProbeEntry::ProbeEntry ... scanning for ev set [%s]", tmp);
		evdata_var = ncf.getVar (tmp); 
		g_free (tmp);

		if (evdata_var.isNull ()){ // not OK?
                        XSM_DEBUG_GP (DBG_L4, "ProbeEntry::ProbeEntry ... not present, done.");
			return;
                }

		// got it now.
		count      = evdata_var.getDims()[0].getSize ();
		chunk_size = evdata_var.getDims()[1].getSize ();
		num_sets   = evdata_var.getDims()[2].getSize ();
//		std::cout << "Count:" << count << " Chunk:" << chunk_size << " NumSets:" << num_sets << std::endl;

		labels = g_ptr_array_new ();
		NcVarAtt data_labels = evdata_var.getAtt ("Labels");
		if (!data_labels.isNull ()){
                        std::string str;
                        data_labels.getValues (str);
			gchar **ss = g_strsplit (str.data(), ",", chunk_size);
			for (gchar **s = ss; *s; ++s)
				g_ptr_array_add (labels, (gpointer) *s);
                }

		unitssymbols = g_ptr_array_new ();
                NcVarAtt data_units = evdata_var.getAtt ("Units");
                if (!data_units.isNull ()){
                        std::string str;
                        data_units.getValues (str);
			gchar **ss = g_strsplit (str.data(), ",", chunk_size);
			for (gchar **s = ss; *s; ++s)
				g_ptr_array_add (unitssymbols, (gpointer) *s);
		}
		
//		std::cout << " ProbeEntry::ProbeEntry Data:" << num_sets << "," << chunk_size << std::endl;
		data = g_array_sized_new (FALSE, TRUE, sizeof (double), num_sets*chunk_size);

	} else {

		chunk_size = pe->get_chunk_size ();
		num_sets = pe->get_num_sets ();
//		std::cout << " ProbeEntry::ProbeEntry DataCPY:" << num_sets << "," << chunk_size << std::endl;
		data = g_array_sized_new (FALSE, TRUE, sizeof (double), num_sets*chunk_size);
		labels = g_ptr_array_new ();
		unitssymbols =  g_ptr_array_new ();
		for (int i=0; i<chunk_size; ++i){
			g_ptr_array_add (labels, (gpointer)pe->get_label (i));
			g_ptr_array_add (unitssymbols, (gpointer)pe->get_unit_symbol (i));
		}
	}

	double coord[4];

        std::vector<size_t> startp = { num, 0 };
        std::vector<size_t> countp = { 1,4 };
        evcoords_var.getVar(startp, countp, coord);

        //evcoords_var->set_cur (num,0);
	//evcoords_var->get (coord, 1,4);

	se = new ScanEvent (coord[0], coord[1], coord[2], TRUE);
	time = (time_t) coord[3];

	float f[1];
	double vals[20];
	for (int n=0; n<num_sets; ++n){
		for (int j=0; j<chunk_size; ++j){
                        std::vector<size_t> startp = { num, j,n };
                        std::vector<size_t> countp = { 1,1,1 };
                        evdata_var.getVar(startp, countp, f);
			//evdata_var->set_cur (num,j,n);
			//evdata_var->get (&f, 1, 1, 1);
			vals[j] = (double)f[0];
 		}
		g_array_append_vals (data, (gconstpointer)vals, chunk_size);
	}

        parent_scan_event = se;
}

ProbeEntry::~ProbeEntry (){ 
	XSM_DEBUG (DBG_L1, "ProbeEntry::~ProbeEntry");
        if (data){
                g_array_unref (data);
                g_ptr_array_unref (labels);
                g_ptr_array_unref (unitssymbols);
        }
}

const gchar *ProbeEntry::get_label (int j){ 
	if (j < 0)
		return "#";
	return (const gchar*)g_ptr_array_index (labels,  j); 
}
	
const gchar *ProbeEntry::get_unit_symbol (int j){ 
	if (j < 0)
		return " ";
	return (const gchar*)g_ptr_array_index (unitssymbols,  j); 
}

void ProbeEntry::find_and_reuse_or_auto_add (NcFile &ncf, const gchar *name, int p_index, int &dim_size, NcDim &nc_dim_x){
	gchar *tmp = NULL;
	for (int k=0; k<p_index; ++k){
                // std::cout << " ProbeEntry::write_nc_variable_set dim -- looking for reusabel " << name << " with size " << dim_size << std::endl;
		tmp = g_strdup_printf (name, k); 
                try {
                        if (!nc_dim_x.isNull()){
                                nc_dim_x = ncf.getDim (tmp);
                                if (nc_dim_x.getSize () == dim_size){
                                        //std::cout << " ProbeEntry::write_nc_variable_set dim #" << p_index << " is dup, re-using #" << k << " is " << tmp << " = " << dim_size << std::endl;
                                        g_free (tmp);
                                        return;
                                }
                        }
                } catch (const netCDF::exceptions::NcException& e) {;} // dim not existing most likely or other issue, try creating
	}
	
	if (tmp) g_free (tmp);
        if (p_index > 255){
                std::cout << " ProbeEntry::write_nc_variable_set -- FAILED, sorry exeeding max dimension variables per NC file." << std::endl;
                return;
        }
        tmp = g_strdup_printf (name, p_index); 
        // std::cout << " ProbeEntry::write_nc_variable_set dim #" << p_index << " :" << tmp << " = " << dim_size << std::endl;
        nc_dim_x  = ncf.addDim (tmp, dim_size);
        g_free (tmp);
}


int ProbeEntry::write_nc_variable_set (NcFile &ncf, int p_index, NcVar &evdata_var, NcVar &evcoords_var, int total_count){
	static NcVar ev_dv;
	static NcVar ev_cv;
	static int ev_dims[4] = { 0,0,0,0 };
	gchar *tmp = NULL;

//	std::cout << " *** write_nc_variable_set #" << p_index << std::endl;

	if (num_sets < 1 || chunk_size < 1){
//		std::cout << " ProbeEntry::write_nc_variable_set dim ** ERROR: num_sets = " << num_sets << " ** skipping this." << std::endl;
		return -1;
	}

	NcDim ev_dim_events;
        find_and_reuse_or_auto_add (ncf, "Event_Probe_%05d_dim_events", p_index, total_count, ev_dim_events);
	if (ev_dim_events.isNull ())
		return -1; 

	int four=4;
	NcDim ev_dim_coords;
        find_and_reuse_or_auto_add (ncf, "Event_Probe_%05d_dim_coords", p_index, four, ev_dim_coords);
	if (ev_dim_coords.isNull ())
		return -1;

	NcDim ev_dim_chunks;
        find_and_reuse_or_auto_add (ncf, "Event_Probe_%05d_dim_chunks", p_index, chunk_size, ev_dim_chunks);
	if (ev_dim_chunks.isNull ())
		return -1;

	NcDim ev_dim_samples;
        find_and_reuse_or_auto_add (ncf, "Event_Probe_%05d_dim_samples", p_index, num_sets, ev_dim_samples);
	if (ev_dim_samples.isNull ())
		return -1;

	if (p_index > 0 && !ev_dv.isNull () && !ev_cv.isNull () && ev_dims[0] == total_count && ev_dims[1] == 4 && ev_dims[2] == chunk_size && ev_dims[3] == num_sets) {
		evcoords_var = ev_cv;
		evdata_var = ev_dv;
	} else {
		tmp = g_strdup_printf ("Event_Probe_%05d_coords", p_index); 
//		std::cout << " ProbeEntry::write_nc_variable_set var[events][coords] #" << p_index << " :" << tmp << std::endl;
                std::vector<netCDF::NcDim> probe_dims;
                probe_dims.push_back(ev_dim_events);
                probe_dims.push_back(ev_dim_coords);
		evcoords_var = ncf.addVar (tmp, ncDouble, probe_dims); //  ev_dim_events, ev_dim_coords);
		evcoords_var.putAtt ("long_name", "Probe Event coordinates list");
		evcoords_var.putAtt ("Units", "X/Y/V/t ==> Ang/Ang/Volt/Unixtime");
		g_free (tmp);
		evcoords_var.putAtt ("ID", ncInt, p_index);

// COMPRESSION
//		evcoords_var->??? for  nc_def_var_deflate(int ncid, int varid, int shuffle=1, int deflate=1, int deflate_level=9);
		GString* all_labels=NULL;
		GString* all_units=NULL;
		for (int j=0; j<chunk_size; ++j){
			if (!j){
				all_labels = g_string_new ((const gchar*)g_ptr_array_index(labels,j));
				all_units  = g_string_new ((const gchar*)g_ptr_array_index(unitssymbols,j));
			} else {
				all_labels = g_string_append (all_labels, ",");
				all_labels = g_string_append (all_labels, (const gchar*)g_ptr_array_index(labels,j));
				all_units  = g_string_append (all_units, ",");
				all_units  = g_string_append (all_units, (const gchar*)g_ptr_array_index(unitssymbols,j));
			}
		}
		tmp = g_strdup_printf ("Event_Probe_%05d_data", p_index); 
//		std::cout << " ProbeEntry::write_nc_variable_set var[events][chunks][samples] #" << p_index << " :" << tmp << std::endl;
                std::vector<netCDF::NcDim> event_dims;
                event_dims.push_back(ev_dim_events);
                event_dims.push_back(ev_dim_chunks);
                event_dims.push_back(ev_dim_samples);
		evdata_var = ncf.addVar (tmp, ncFloat, event_dims);
		g_free (tmp);
		evdata_var.putAtt ("long_name", "Probe Event Data Set");
		evdata_var.putAtt ("Name", name);
		evdata_var.putAtt ("Labels", all_labels->str);
		evdata_var.putAtt ("Units", all_units->str);
		
		g_string_free (all_labels, TRUE);
		g_string_free (all_units, TRUE);
	}

//	std::cout << " ProbeEntry::write_nc_variable_set done." << std::endl;
	return 0;
}

int ProbeEntry::write_nc_data (NcVar &evdata_var, NcVar &evcoords_var, ScanEvent *se, int count){
	double coord[4];

	if (num_sets < 1 || chunk_size < 1){
		std::cout << " ProbeEntry::write_nc_variable_set dim ** ERROR: num_sets = " << num_sets << " ** skipping this." << std::endl;
		return -1;
	}
        // X,Y,Z,time
	coord[2] = se->get_position (coord[0], coord[1]);
	coord[3] = (double)time;
	std::cout << "Write, Coord, Data #" << count << " :XY" << coord[0] << "," << coord[1]
                  << " => " << evdata_var.getName()
                  << evdata_var.getDims()[0].getName() << ": " << evdata_var.getDims()[0].getSize() << ", "
                  << evdata_var.getDims()[1].getName() << ": " << evdata_var.getDims()[1].getSize() << ", "
                  << evdata_var.getDims()[2].getName() << ": " << evdata_var.getDims()[2].getSize() << "]"
                  << " DIMS required: " << chunk_size << ", " << num_sets
                  << std::endl;
        std::vector<size_t> startp = { count,0 };
        std::vector<size_t> countp = { 1,4 };
        evcoords_var.putVar(startp, countp, coord);
	//evcoords_var->set_cur (count,0);
	//evcoords_var->put (coord, 1,4);

	float f[1];
 	for (int j=0; j<chunk_size; ++j)
 		for (int n=0; n<num_sets; ++n){
                        std::vector<size_t> startp = { count,j,n };
                        std::vector<size_t> countp = { 1,1,1 };
 			f[0] = (float)get (n, j);
                        evdata_var.putVar(startp, countp, f);

                        //evdata_var->set_cur (count,j,n);
			//evdata_var->put (&f, 1, 1, 1);
 		}

	return 0;
}


void ProbeEntry::print (){
	std::cout << "ScanEvent::ProbeEntry " << name << " [" << chunk_size << ":" << num_sets << "] time: " << time << std::endl;
	std::cout << "### ";
	for (int j=0; j<chunk_size; ++j)
		std::cout << (gchar*)g_ptr_array_index(labels,j) << "[" << (gchar*)g_ptr_array_index(unitssymbols,j) << "], ";
	std::cout << std::endl;
	for (int i=0; i<num_sets; ++i){
		std::cout << i << " ";
		for (int j=0; j<chunk_size; ++j)
			std::cout << get (i, j) << " ";
		std::cout << std::endl;
	}
}

void ProbeEntry::save (std::ofstream *file){
	*file << "ScanEvent::ProbeEntry " << name << " [" << chunk_size << ":" << num_sets << "] time: " << time << std::endl;
	*file << "### ";
	for (int j=0; j<chunk_size; ++j)
		*file << (gchar*)g_ptr_array_index(labels,j) << "[" << (gchar*)g_ptr_array_index(unitssymbols,j) << "], ";
	*file << std::endl;
	for (int i=0; i<num_sets; ++i){
		*file << i << " ";
		for (int j=0; j<chunk_size; ++j)
			*file << get (i, j) << " ";
		*file << std::endl;
	}
}


UserEntry::UserEntry (UserEntry *u, int n, ScanEvent *s) : EventEntry (u->name, u->time){
//	std::cout << "UserEntry(u,n,s) : " << u->name << ", " << u->message_id << std::endl;
	num_sets=0;
	data = g_ptr_array_new ();
	message_id = g_strdup (u->message_id);
	info = g_strdup (u->info);
	UE_set *ue = (UE_set*) g_ptr_array_index (u->data, n);
	if (ue){
		ue->coords[0] = s->get_position (ue->coords[1], ue->coords[2], ue->coords[3], ue->coords[4]);
		add (ue->vp, ue->vn, ue->coords);
	}
}

UserEntry::UserEntry (const gchar *Name, const gchar *Message_id, const gchar *Info, time_t Time, GPtrArray *Messages, int num) : EventEntry (Name, Time){ 
	XSM_DEBUG (DBG_L6, "UserEntry::UserEntry");
	num_sets=0;
	data = NULL;
	message_id = g_strdup (Message_id);
	info = g_strdup (Info);

	if (Messages && num>0){
		data = Messages;
		num_sets=num;
	} else
		data = g_ptr_array_new ();
}
 
UserEntry::UserEntry (const gchar *Name, NcFile &ncf, int u_index, ScanEvent* &se) : EventEntry (Name, 0){ 
	gchar *tmp;

	XSM_DEBUG (DBG_L6, "UserEntry::UserEntry -- Scan NC-File for User Entry (old)" << u_index);

	se = NULL;
	num_sets=0;
	data = NULL;
	message_id = NULL;
	info = NULL;

	tmp = g_strdup_printf ("Event_User_%05d_coords", u_index);
	std::cout << " UserEntry::UserEntry (old) scanning for #" << u_index << " :" << tmp << std::endl;

	NcVar evcoords_var = ncf.getVar(tmp); 
	g_free (tmp);

	// found that Event_User, else return?
	if (evcoords_var.isNull ()) // not OK?
		return;

	NcVarAtt data_message = evcoords_var.getAtt ("Message");
	if (!data_message.isNull ()){
                data = g_ptr_array_new ();
                std::string str;
                data_message.getValues (str);
		message_id = g_strdup (str.data());
	}

	double coord[4];
        evcoords_var.getVar ({0}, {4}, coord);
 
	se = new ScanEvent (coord[0], coord[1], coord[2], FALSE);
	time = (time_t) coord[3];
}


UserEntry::UserEntry (const gchar *Name, NcFile &ncf, const gchar *Message_id, ScanEvent* &se, gint count, Mem2d *m) : EventEntry (Name, 0){
	gchar *tmp;

	XSM_DEBUG (DBG_L6, "UserEntry::UserEntry -- Scan NC-File for User Entry" << Message_id);
//	std::cout << "UserEntry::UserEntry -- Scan NC-File for User Entry: " << Message_id << " Count=" << count << std::endl;

	se = NULL;
	num_sets=0;
	data = NULL;
	message_id = g_strdup (Message_id);
	info = NULL;

	gchar *ue_data_dim_name = g_strdup_printf ("Event_User_Adjust_Data_Dim");
//	std::cout << " UE NCF dim checking >" << ue_data_dim_name << std::endl;

        NcDim ue_data_dim;
        try {
                ue_data_dim = ncf.getDim (ue_data_dim_name);
        } catch (const netCDF::exceptions::NcException& e) {;}
	g_free (ue_data_dim_name);
	if (ue_data_dim.isNull ())
		return;
	// int dim_coord_data  = (gint) ue_data_dim.getSize ();

	gchar *ue_entities_dim_name = g_strdup_printf ("Event_User_%s_Dim", message_id);
//	std::cout << " UE NCF dim entities checking >" << ue_entities_dim_name << std::endl;
        NcDim ue_entities_dim;
        try {
                ue_entities_dim = ncf.getDim (ue_entities_dim_name);
        } catch (const netCDF::exceptions::NcException& e) {;}
	g_free (ue_entities_dim_name);
	if (ue_entities_dim.isNull ())
		return;
	gint dim_adjustments = (gint) ue_entities_dim.getSize ();

	if (count >= dim_adjustments)
		return; // done with this, stop recursion

	tmp = g_strdup_printf ("Event_User_%s", message_id); 
	NcVar ue_data_var = ncf.getVar (tmp);
//	std::cout << " UE NCF get NCVar >" << tmp << std::endl;
	g_free (tmp);
	if (ue_data_var.isNull ())
		return;

	NcVarAtt att_info = ue_data_var.getAtt ("Info");
	if (!att_info.isNull ()){
                std::string str;
                att_info.getValues (str);
		info = g_strdup (str.data());
        }
        
//	std::cout << " UE getting data, adding.. " << std::endl;

	double ue_data[20];
	double coords[5];
	//ue_data_var.set_cur (count);
	ue_data_var.getVar ({count,0}, {1,20}, ue_data); //, 1, 20);

	data = g_ptr_array_new ();
	for (int i=0; i<5; ++i)
		coords[i] = ue_data[i];
	add (ue_data[5], ue_data[6], coords);
       
	se = new ScanEvent (coords[1], coords[2], coords[3], coords[4]);
	se->add_event (this);
	m->AttachScanEvent (se);
	ScanEvent *se_=NULL;

	// recursive add
//	std::cout << " UE recursive adding.. " << std::endl;
	UserEntry *ue = new UserEntry ("User", ncf, message_id, se_, count+1, m);
	if (!se_)
		delete ue;
}


UserEntry::~UserEntry (){ 
	XSM_DEBUG (DBG_L6, "UserEntry::~UserEntry");
	if (data)
		g_ptr_array_unref (data);
	if (message_id)
		g_free (message_id);
	if (info)
		g_free (info);
}

void UserEntry::print (){
	std::cout << "ScanEvent::UserEntry " << name << " time: " << time 
		  << " Messages: " << std::endl;
	for (int i=0; i<num_sets; ++i)
		std::cout << i << ": " << get (i) << std::endl;
}

int UserEntry::store_event_to_nc (NcFile &ncf, GSList *uel, int count){
	GSList *ue_list = uel;
	UserEntry *ue = (UserEntry*) uel->data;
	gchar *tmp;

	if (!count) 
		return 0;

	// count x [5 coords + 15 data]
	int dim_coord_data  = 20;
	int dim_adjustments = count;
	
//	std::cout << " UE >" << ue->message_id << "< to NCF, count=" << count << std::endl;

	gchar *ue_data_dim_name = g_strdup_printf ("Event_User_Adjust_Data_Dim");
        NcDim ue_data_dim;
        try {
                ue_data_dim = ncf.getDim (ue_data_dim_name);
        } catch (const netCDF::exceptions::NcException& e) {;}
//	std::cout << " UE NCF dim checking/creating >" << ue_data_dim_name << std::endl;
	if (ue_data_dim.isNull ())
		ue_data_dim = ncf.addDim (ue_data_dim_name, dim_coord_data);
	g_free (ue_data_dim_name);
	if (ue_data_dim.isNull ())
		return 0;
;
	gchar *ue_entities_dim_name = g_strdup_printf ("Event_User_%s_Dim", ue->message_id);
//	std::cout << " UE NCF dim creating >" << ue_entities_dim_name << std::endl;
	NcDim ue_entities_dim = ncf.addDim (ue_entities_dim_name, dim_adjustments);
	g_free (ue_entities_dim_name);
	if (ue_entities_dim.isNull ())
		return 0;

	tmp = g_strdup_printf ("Event_User_%s", ue->message_id); 

        std::vector<netCDF::NcDim> ue_dims;
        ue_dims.push_back (ue_entities_dim);
        ue_dims.push_back (ue_data_dim);
	NcVar ue_data_var = ncf.addVar (tmp, ncDouble, ue_dims); //ue_entities_dim, ue_data_dim);
//	std::cout << " UE NCF NCVar creating >" << tmp << std::endl;
	g_free (tmp);

	tmp = g_strdup_printf ("User Event for %s change. Coordinates and Data included", ue->message_id); 
	ue_data_var.putAtt ("long_name", tmp);
	ue_data_var.putAtt ("Units", "{{[Z], X,Y,iL,it, data[16]}, {..}, ..} ==> Ang/Ang/#/#/ ..");
	ue_data_var.putAtt ("Name", name);
	ue_data_var.putAtt ("Entities", ncInt, count);
	ue_data_var.putAtt ("Info", ue->info);
	g_free (tmp);

//	std::cout << " UE adding data.. " << std::endl;
	for (int i=0; ue_list && i<count; ++i){
		double data[20];
//		std::cout << i << std::endl;
		UE_set *ue_set = (UE_set*) g_ptr_array_index (((UserEntry*)(ue_list->data))->data, 0); // here!! in reorganized/flattened list of UE always only one entry

		memset (data, 0, sizeof(data));
		memcpy (data, ue_set->coords, sizeof(ue_set->coords));
		data[5] = ue_set->vp; data[6] = ue_set->vn;

#if 0
		std::cout << " UE adding data.. [" << i << "] = {"
			  << data[0] << ", "
			  << data[1] << ", "
			  << data[2] << ", "
			  << data[3] << ", "
			  << data[4] << ", vp="
			  << data[5] << ", vn="
			  << data[6] << ", "
			  << data[7] << ", ..[19]="
			  << data[19] << ", .."
			  << "}" << std::endl;
#endif
                std::vector<size_t> startp = { i, 0 };
                std::vector<size_t> countp = { 1,20 };
                ue_data_var.putVar(startp, countp, data);
		//ue_data_var->set_cur (i);
		//ue_data_var->put (data, 1, 20);

		ue_list = g_slist_next (ue_list);
	}

	std::cout << " done." << std::endl;
	return 0;
}

/*
 * Base Class implementation for one Scan Event at one Time/Location
 *
 * Class: ScanEvent
 */

void ScanEvent::eeremove (EventEntry *entry, gpointer from){
	delete entry;
}

void ScanEvent::eeprint (EventEntry *entry, int *w){
	entry->print ();
}

void ScanEvent::eesave (EventEntry *entry, std::ofstream *data){
	entry->save (data);
}

void ScanEvent::save (){
	printf("Saving to: %s\n",saveto);
	std::ofstream file;
	file.open(saveto,std::ios::out);
	file << "ScanEvent at " << xpos << "Ang, " << ypos << "Ang" << std::endl;
	g_slist_foreach(event_list, (GFunc) ScanEvent::eesave, &file);  
	file.close();
}

ScanEvent::ScanEvent (double xPos, double yPos, int iVal, int iTime, double Val, gboolean Sort){
	XSM_DEBUG (DBG_L6, "ScanEvent::ScanEvent");
	event_list = NULL;
	xpos = xPos;
	ypos = yPos;
	ival = iVal;
	itime= iTime;
	val = Val;
	flag = FALSE;
	sort = Sort;
}


ScanEvent::~ScanEvent (){
	XSM_DEBUG (DBG_L6, "ScanEvent::~ScanEvent");
	g_slist_foreach(event_list, (GFunc) ScanEvent::eeremove, event_list);
	g_slist_free (event_list);
}

void ScanEvent::add_event (EventEntry *ee){
	event_list = g_slist_prepend (event_list, ee);
}

