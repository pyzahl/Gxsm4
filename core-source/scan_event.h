/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyight (C) 1999,2000,2001,2002,2003 Percy Zahl
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

#ifndef __SCAN_EVENT_H
#define __SCAN_EVENT_H

#ifndef __XSMTYPES_H
#include "xsmtypes.h"
#endif

#ifndef __XSMMASKS_H
#include "xsmmasks.h"
#endif

#ifndef __XSMDEBUG_H
#include "xsmdebug.h"
#endif

#include <config.h>
#include <gtk/gtk.h>

#include <fstream>

#include <netcdf.hh>
//#include <netcdf>
//using namespace netCDF;


class Mem2d;
class ScanEvent;

/**
 * EventData:
 *
 * Lists of arbitrary data, these are attached to any position withing the 2D field
 *
 */

class EventEntry{
public:
	EventEntry (const gchar *Name, time_t Time, const gchar *file_name=NULL);
	virtual ~EventEntry ();

	virtual void add (double *value) {};
	virtual void add (gchar *message) {};
	virtual double get (int n, int j) { return 0.; };
	virtual time_t get_time ();
	virtual gchar* get_str_time ();
	virtual gchar* get (int n) { return NULL; };
	virtual void print () {};
	virtual void save (std::ofstream *file) {};

	gchar* file_source_name () { return fsrc_name; };
	gchar* description () { return name; };
	gchar description_id () { return name[0]; };
	gint description_id_match (const gchar *filter) { 
		gint match = 0;
		for (gint i=strlen(filter)-1; i>=0; --i)
			if (name[0] == filter[i]) ++match;
		return match;
	};

protected:
	gchar *name;
	time_t time;
        gchar *fsrc_name;
private:
};

class ProbeEntry : public EventEntry{
public:
	ProbeEntry (const gchar *Name, time_t Time, GPtrArray *Labels, GPtrArray *Unitssymbols, int Chunk_size, const gchar *file_name=NULL);
	ProbeEntry (const gchar *Name, NcFile *ncf, int p_index, NcVar* &evdata_var, NcVar* &evcoords_var, ScanEvent* &se, int num, int &count, ProbeEntry *pe=NULL);
	virtual ~ProbeEntry ();
	virtual void add (double *value){ g_array_append_vals (data, (gconstpointer)value, chunk_size); ++num_sets; };
	virtual double get (int n, int j){ 
		if (j < 0)
			return (double)n;
		if (j >= chunk_size)
                        return 0.;
                if (n >= num_sets)
                        return 0.;
		return g_array_index (data, double, n*chunk_size+j); 
	};
	virtual double get_iir (int n, int j, int iirl=20, double iirf=0.05){ 
		if (j < 0)
			return (double)n;
		if (j >= chunk_size)
                        return 0.;
                if (n >= num_sets)
                        return 0.;
                double z=0.;
#if 0
                int cnt=0;
                // average around start
                for (int i=MAX(0,n-iirl/2); i<=MIN(n+iirl/2,num_sets-1); i++, cnt++)
                        z+=g_array_index (data, double, i*chunk_size+j);
                z /= cnt;
#else
                z=g_array_index (data, double, (num_sets-1)*chunk_size+j);
                
#endif
                double q=1.-iirf;
                // IIR filter to j
                //for (int i=MIN(0, n-iirl); i<=n; i++)
                for (int i=num_sets-1; i>=n; i--)
                        z = q*z + iirf*g_array_index (data, double, i*chunk_size+j);
		return z;
	};
	virtual void print ();
	virtual void save (std::ofstream *file);

	int get_num_sets () { return num_sets; };
	int get_chunk_size () { return chunk_size; };
	const gchar *get_label (int j);
	const gchar *get_unit_symbol (int j);

	int write_nc_variable_set (NcFile* ncf, int p_index, NcVar* &evdata_var, NcVar* &evcoords_var, int total_count);
	int write_nc_data (NcVar* evdata_var, NcVar* evcoords_var, ScanEvent *se, int count);

        ScanEvent *get_parent_scan_event () { return parent_scan_event; }; 
        
private:
	GPtrArray *labels;
	GPtrArray *unitssymbols;
	int chunk_size;
	int num_sets;
	GArray *data;

        ScanEvent *parent_scan_event; 
};

typedef struct { double vp, vn; double coords[5]; } UE_set;

class UserEntry : public EventEntry{
public:
	UserEntry (UserEntry *u, int n, ScanEvent *s);
	UserEntry (const gchar *Name, const gchar *Message_id, const gchar *Info, time_t Etime, GPtrArray *Messages=NULL, int num=0);
	UserEntry (const gchar *Name, time_t Time, GPtrArray *Messages=NULL, int num=0);
	UserEntry (const gchar *Name, NcFile *ncf, int u_index, ScanEvent* &se);
	UserEntry (const gchar *Name, NcFile *ncf, const gchar *Message_id, ScanEvent* &se, gint count=0, Mem2d *m=NULL);
	~UserEntry ();
	virtual void add (double val_pre, double val_now, double c[5]=NULL){ 
		UE_set *ue = new (UE_set);
		ue->vp = val_pre; ue->vn = val_now;
		if (c)
			memcpy (ue->coords, c, sizeof(ue->coords));
		else
			memset (ue->coords, 0, sizeof(ue->coords));
		g_ptr_array_add (data, (gpointer) ue); ++num_sets;
	};
	virtual gchar* get (int n){ 
		UE_set *ue = (UE_set*) g_ptr_array_index (data, n);
		return (g_strdup_printf ("%s :: %s: %g -> %g", message_id, info, ue->vp, ue->vn));
	};
	gint same (UserEntry *a) { return !g_strcmp0 (a->message_id, message_id) ? TRUE:FALSE; };
	int get_num_sets () { return num_sets; };
	virtual void print ();

	int store_event_to_nc (NcFile* ncf, GSList *uel, int count);

	gchar *message_id;
private:
	gchar *info;
	time_t etime;
	int num_sets;
	GPtrArray *data;
};

class ScanEvent{
public:
	ScanEvent (double xPos, double yPos, int iVal=0, int iTime=0, double val=0., gboolean Sort=TRUE);
	~ScanEvent ();
	static void eeremove(EventEntry *entry, gpointer from);
	static void eeprint(EventEntry *entry, int *w);
	static void eesave(EventEntry *entry, std::ofstream *data);
	void add_event (EventEntry *ee);
	double get_position (double &x, double &y) { x=xpos; y=ypos; return val; };
	double get_position (double &x, double &y, int &l, int &t) { x=xpos; y=ypos; l=ival; t=itime; return val; };
	double get_position (double &x, double &y, double &l, double &t) { x=xpos; y=ypos; l=(double)ival; t=(double)itime; return val; };
	double distance (double *xy) { if (sort){ double dx=xpos-xy[0]; double dy=ypos-xy[1]; return sqrt(dx*dx+dy*dy); } else return 0.; };
	double distance2 (double *xy) { if (sort){ double dx=xpos-xy[0]; double dy=ypos-xy[1]; return (dx*dx+dy*dy); } else return 0.; };
	guint get_event_count() { return g_slist_length (event_list); };
		
	void save ();
	void print () {
		std::cout << "ScanEvent at (X,Y, v-,t-index): (" 
			  << xpos << "Ang, " 
			  << ypos << "Ang," 
			  << ival << ", " 
			  << itime << ")"
			  << std::endl;
		g_slist_foreach(event_list, (GFunc) ScanEvent::eeprint, NULL); 
	};


	GSList *event_list;
	gint flag;
	gchar *saveto;

private:
	double xpos, ypos, val;
	int    ival, itime;
	gboolean sort;
};


#endif


	
