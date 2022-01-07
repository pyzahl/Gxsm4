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


#ifndef __SCAN_H
#define __SCAN_H

#ifndef __MELDUNGEN_H
#include "meldungen.h"
#endif

#ifndef __XSMTYPES_H
#include "xsmtypes.h"
#endif

#ifndef __XSMHARD_H
#include "xsmhard.h"
#endif

#ifndef __VIEW_H
//#include "view.h"
#endif

#ifndef __MEM2D_H
#include "mem2d.h"
#endif

#ifndef __SCAN_OBJECT_DATA_H
#include "scan_object_data.h"
#endif

#include "gnome-res.h"
#include "scan_types.h"
#include "gxsm_window.h"

/*
 * XSM Surface Scan Basis-Objekt:
 * vereint alle Scan-Methoden
 * ============================================================
 */

#define NOT_SAVED 0
#define IS_SAVED  1
#define IS_NEW    2
#define IS_FRESH  3

class StorageManager{
public:
        StorageManager(){
                dataset_counter=-1;
                storage_path=NULL;
                storage_basename=NULL;
                storage_type=NULL;
                storage_name=NULL;
        };
        ~StorageManager(){
                g_free (storage_path);
                g_free (storage_basename);
                g_free (storage_type);
                g_free (storage_name);
        };
        void set_dataset_counter(int n){ dataset_counter=n; };
        void set_path(const gchar *path){ g_free (storage_path); storage_path = g_strdup(path); };
        void set_basename(const gchar *basename){ g_free (storage_basename); storage_basename = g_strdup(basename); };
        void set_type(const gchar *type){
                g_free (storage_type); storage_type = g_strdup(type);
                g_strdelimit (storage_type, "+",'p');
                g_strdelimit (storage_type, "-",'m');
                g_strdelimit (storage_type, " ",'-');
                gchar *p = strchr (storage_type, ',');
                if(p) *p=0; // cut off
        };
        const gchar *get_filename(){
                g_free (storage_name); storage_name = NULL;

                /*
	// Automatic file counter --> DIGITS string
	if( IS_FILENAME_CONVENTION_ALPHA ) {
		int cbase = (int)'z'-(int)'a'+1;
		int tmp = counter/cbase/cbase;
		digits = g_strdup_printf ("%c%c%c",
					  (char)('a'+ tmp % cbase), 
					  (char)('a'+ ((counter - tmp*cbase*cbase) / cbase) % cbase), 
					  (char)('a'+ counter % cbase ));
	} else if( IS_FILENAME_CONVENTION_DIGIT ){
		digits = g_strdup_printf ("%03d",counter+1);
	} else {
		for(si=0; si<CHMAX; si++) if(Ch[si] == -1) continue; else break;
		struct tm *ts = localtime (&scan[Ch[si]]->data.s.tStart);
		digits = g_strdup_printf ("%04d%02d%02d%02d%02d%02d", 
					  1900+ts->tm_year, ts->tm_mon, ts->tm_mday, 
					  ts->tm_hour, ts->tm_min, ts->tm_sec);
	}
                */
                
                if (dataset_counter >= 0 && storage_path && storage_basename && storage_type){
                        g_free (storage_name);
                        storage_name=g_strdup_printf ("%s/%s%03d-%s.nc", storage_path, storage_basename, dataset_counter, storage_type);
                }
                return storage_name;
        };
        const gchar *get_name(const gchar *append=NULL){
                g_free (storage_name); storage_name = NULL;
                if (dataset_counter >= 0 && storage_basename && storage_type){
                        g_free (storage_name);
                        if (append)
                                storage_name=g_strdup_printf ("%s%03d-%s.nc %s", storage_basename, dataset_counter, storage_type, append);
                        else
                                storage_name=g_strdup_printf ("%s%03d-%s.nc", storage_basename, dataset_counter, storage_type);
                }
                return storage_name;
        };
private:
        int dataset_counter;
        gchar *storage_path;
        gchar *storage_basename;
        gchar *storage_type;
        gchar *storage_name;
};


class Scan{
public:
	Scan(Scan *scanmaster);
	Scan(int vtype=0, int vflg=0, int ChNo=-1, SCAN_DATA *vd=NULL, ZD_TYPE mtyp=ZD_SHORT, Gxsm4app *app=NULL);
	virtual ~Scan();
	
	virtual void hide();
	virtual int draw(int y1=-1, int y2=-2);
	void update_world_map (Scan *src=NULL);
	void clear_world_map ();
      
	virtual int create(gboolean RoundFlg=FALSE, gboolean subgrid=FALSE, gdouble direction=1., gint fast_scan=0,
                           ZD_TYPE ztype=ZD_IDENT,
                           gboolean keep_layer_info=true, gboolean remap=false, gboolean keep_nv=false);
	void Saved(){ State = IS_SAVED; };

        int Save (gboolean overwrite=false, gboolean check_file_exist=false);
        int Update_ZData_NcFile ();
  
	virtual void start(int l=0, double lv=0.);
	virtual void stop(int StopFlg=FALSE, int line=0);
	gboolean is_scanning() { return Running ? true : false; };
	
	void inc_refcount() { ++refcount; };
	void dec_refcount() { --refcount; };
	int get_refcount() { return refcount; };
	
	void CpyUserEntries(SCAN_DATA &src);
	void CpyDataSet(SCAN_DATA &src);
	void GetDataSet(SCAN_DATA &dst);
	
	int SetView(uint vtype=0);
	//void AutoDisplay(double hi=0., double lo=0., int Delta=4, double sm_eps=0.05);
        void set_main_app (Gxsm4app *app=NULL) { gxsm4app = app; g_message ("Scan:: got main app ref.");};
        Gxsm4app* get_app () { return gxsm4app; };

	void determine_display(int Delta=4, double sm_eps=0.05);
	void auto_display();
	void set_display (gint lock=0);
	void set_display_hl (gint lock=0);
	void set_display_shift();
	int SetVM(int vflg=0, SCAN_DATA *src=NULL, int Delta=4, double sm_eps=0.05);
	int GetVM(){ return VFlg; };
	void Activate();

	//void realloc_pkt2d(int n);
	
	/* Data */
	GList *TimeList; /* List multiple of Scan-Data elements (always a copy) */
	Mem2d *mem2d;    /* 2d Daten */
	int   mem2d_refcount;
	SCAN_DATA data;  /* Daten des letzten Scans - Scanbezogen*/
	SCAN_DATA *vdata; /* ever valid Pointer to XSM-(User)-Data (may be manipulated) */
        StorageManager storage_manager;
        
	View  *view;     /* View Objekt */
	
	Point2D Pkt2dScanLine[2];
	int     RedLineActive;
	int     BlueLineActive;
	
	/* Time dimension handling */
	int free_time_elements (); /* free all time elements -- keeps the current, destroys all others! */
	int append_current_to_time_elements (int index=-1, double t=0., Mem2d* other=NULL); /* append current (or other if given) dataset to time elemets */
	int prepend_current_to_time_elements (int index=-1, double t=0., Mem2d* other=NULL); /* prepend current (or other if given) dataset to time elemets */

	void sort_time_elements_by_index ();
	void sort_time_elements_by_time ();
	void sort_time_elements_by_bias ();
	void sort_time_elements_by_zsetpoint ();
	
	int remove_time_element (int index); /* time elemet */
	int number_of_time_elements (); /* find and return number of time elements in list */
	void reindex_time_elements (); /* reindex time elements in list */
	double retrieve_time_element (int index); /* retrieve time element "index" and revert to it, returns time */
	Mem2d* mem2d_time_element (int index); /* retrieve mem2d ptr from time element "index" */
	int get_current_time_element (); /* find index of current time element, -1 if not in list */
	static void free_time_element (TimeElementOfScan *tes, Scan *s){ delete tes->mem2d; delete tes->sdata; delete tes; };

	/* Scan Coodinate System Handling */
	int Pixel2World (int ix, int iy, double &wx, double &wy, SCAN_COORD_MODE scm = SCAN_COORD_ABSOLUTE);
	int Pixel2World (double ix, double iy, double &wx, double &wy, SCAN_COORD_MODE scm = SCAN_COORD_ABSOLUTE);
	int World2Pixel (double wx, double wy, int &ix, int &iy, SCAN_COORD_MODE scm = SCAN_COORD_ABSOLUTE);
	int World2Pixel (double wx, double wy, double &ix, double &iy, SCAN_COORD_MODE scm = SCAN_COORD_ABSOLUTE);
	/* Conveniance wrappers */
	double GetWorldX (int ix){
		double x,y;
		Pixel2World (ix,0, x, y, SCAN_COORD_RELATIVE);
		return x;
	};
	double GetWorldY (int ix){
		double x,y;
		Pixel2World (ix,0, x, y, SCAN_COORD_RELATIVE);
		return y;
	};

	// Point2D *Pkt2d;
	// int     PktVal;
	
	int add_object (scan_object_data *sod);
	int del_object (scan_object_data *sod);
	void destroy_all_objects ();

	unsigned int number_of_object () { return g_slist_length(objects_list); };
	scan_object_data* get_object_data (int i) { return (scan_object_data*) g_slist_nth_data(objects_list, i); };
	// do not modify data!!!!!!
	void dump_object_data (int i) { ((scan_object_data*) g_slist_nth_data(objects_list, i)) -> dump (); };
	int get_channel_id () { return ChanNo; };

	void x_linearize (int f) { X_linearize=f; };
	int x_linearize () { return X_linearize; };

	void set_subscan_information (int ssi[4]) { for (int i=0; i<4; ++i) ixy_sub[i]=ssi[i]; };
	void get_subscan_information (int ssi[4]) { for (int i=0; i<4; ++i) ssi[i]=ixy_sub[i]; };
	
	int ixy_sub[4]; // sub scan information [int xs, int xn, int ys, int yn]
	gboolean show_world_map (gboolean flg);
	
 private:
	double t0_ref;
	
	int VFlg;
	int ChanNo;
	int State;
	int Running;
	//int numpkt2d;
	int refcount;
	int X_linearize;
	gdouble scan_direction;
	ZD_TYPE scan_ztype;
	int     objects_id;
	GSList  *objects_list;
	Scan *world_map;

        // passed at scan-setup time
        int dataset_counter;
        gchar *storage_path;
        gchar *storage_basename;
        gchar *storage_name;

        Gxsm4app *gxsm4app;
};

/*
 * TopoGraphic Ableitung
 * ============================================================
 */

class TopoGraphicScan : public Scan{
public:
	TopoGraphicScan(int vtype=0, int vflg=0, int ChNo=-1, SCAN_DATA *vd=0, Gxsm4app *app=NULL);
	virtual ~TopoGraphicScan();
  
private:  

};


/*
 * SpaScan Ableitung
 * ============================================================
 */

class SpaScan : public Scan{
public:
	SpaScan(int vtype=0, int vflg=0, int ChNo=-1, SCAN_DATA *vd=0, Gxsm4app *app=NULL);
	virtual ~SpaScan();
  
private:  

};


#endif
