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


/*
The Gtk_EntryControl class is used to create  most of the input fields of GXSM. It handles
the units and supports arrow keys and (optional) adjustments.
Beware: Gtk_EntryControl is derived from Param_Control which contains some of the important
functions and variables. Some of the functions of Param_Control are replaced by functions of
the Gtk_EntryControl class. Look for 'virtual' functions in the header file.

'change' callbacks will be handled by Gtk_EntryControl::Set_Parameter, so this is always a
good starting point.
*/


#ifndef __PCS_H
#define __PCS_H

#include <fstream>

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "gxsm_conf.h"
#include "unit.h"
#include "remoteargs.h"

extern "C++" {

extern gboolean generate_pcs_gschema;
extern gboolean generate_pcs_adj_gschema;
extern gchar *generate_pcs_gschema_path_add;

}
        
extern const gchar* pcs_get_current_gschema_group ();
extern void pcs_set_current_gschema_group (const gchar *group);

class Scale_Ticks {
        Scale_Ticks() {};
        ~Scale_Ticks() {};
private:
        double *ticks;
        gchar **marks;
};


class Param_Control;

#define PARAM_CONTROL_ADJUSTMENT_LINEAR         0  // plain linear scale
#define PARAM_CONTROL_ADJUSTMENT_LOG            1  // log scale, log_min = +lower-limit ... +upper-limit
#define PARAM_CONTROL_ADJUSTMENT_LOG_SYM        2  // log scale, -upper-limit .. +/-log_min=lower_limit (=0) .. +upper_limit
#define PARAM_CONTROL_ADJUSTMENT_DUAL_RANGE     4  // auto dual ranging -- defined by warning limits
#define PARAM_CONTROL_ADJUSTMENT_ADD_MARKS      8  // add tick marks

#define PARAM_CONTROL_ADJUSTMENT_LOG_MODE_MASK  (PARAM_CONTROL_ADJUSTMENT_LOG | PARAM_CONTROL_ADJUSTMENT_LOG_SYM)

// Objekt fuer Parameter Ein/Ausgabeberwachung und Formatierung
class Param_Control{
 public:
	Param_Control(UnitObj *U, const char *W, double *V, double VMi, double VMa, const char *p);
	Param_Control(UnitObj *U, const char *W, unsigned long *V, double VMi, double VMa, const char *p);
	Param_Control(UnitObj *U, const char *W, int *V, double VMi, double VMa, const char *p);
	Param_Control(UnitObj *U, const char *W, int *V, int VMi, int VMa, const char *p);
	Param_Control(UnitObj *U, const char *W, short *V, double VMi, double VMa, const char *p);
	Param_Control(UnitObj *U, const char *W, const gchar *V);
	virtual ~Param_Control();

	void Init();
	void setMax(double VMa, double Vmax_warn =  1e111, const gchar* w_color=NULL);
	void setMin(double VMi, double Vmin_warn = -1e111, const gchar* w_color=NULL);
	void set_exclude(double V_ex_lo = 1e111, double V_ex_hi = 1e111);
	void set_info (const gchar* Info);
	void set_adjustment_mode (int m, int magprefixshift = 0) {
                mag_prefix_shift = magprefixshift;
                // reset limits
                adj_current_limits[0]=0.;
                adj_current_limits[1]=0.;
                // set new mode
                adj_mode = m;
        };

	virtual void update_limits() {};
	void changeUnit(UnitObj *U){ delete unit; unit = U->Copy(); Put_Value(); };
	void changeUnit_hold_usr_value(UnitObj *U){ 
		double valusr = unit->Base2Usr(Get_dValue());
                delete unit;
		unit = U->Copy();
		Set_FromUsrValue(valusr);
	};
        const gchar* get_unit_symbol() { return unit->Symbol(); };
	virtual void Set_NewValue (gboolean set_new_value) {};
	void Val(double *V);
	void Val(unsigned long *V);
	void Val(short *V);
	void Val(int *V);
        void Val(const gchar *V);
	void Prec(const char *p);
	gchar *Get_UsrString();

        virtual void init_pcs_gsettings_path_and_key () { };
	virtual void write_pcs_gschema (int array_flag=false) {};
        virtual void get_init_value_from_settings (int array_flag=false) {};
        virtual void update_value_in_settings (int array_flag=false) {};

	void set_refname(const gchar *ref);
	gchar *get_refname();

	double Get_dValue();
	void Set_dValue(double nVal);
	gboolean Set_FromValue(double nVal);
	void Set_FromUsrValue(double nVal){ Set_FromValue(unit->Usr2Base(nVal)); };
	double Convert2Base (double x) { return unit->Usr2Base(x); };
	double Convert2Usr (double x) { return unit->Base2Usr(x); };

	virtual void ShowInfo (const char *header, const char *txt);
	virtual gboolean ShowVerifySet (const char *header, const char *text);
	virtual void Put_Value();
	virtual void Set_Parameter(double value=0., int flg=FALSE, int usr2base=FALSE);
	virtual gpointer GetEntryData(const gchar *txtid){ return NULL; };

        virtual void Set_Force_ChangeNoticeFkt(gboolean force = false) { force_full_update=force; };
        gboolean force_full_update;
	virtual void Set_ChangeNoticeFkt(void (*NewChangeNoticeFkt)(Param_Control* pcs, gpointer data), gpointer data){
		ChangeNoticeFkt = NewChangeNoticeFkt;
		FktData = data;
	}

	void set_head_iter (Param_Control *iter) { pc_head = iter; };
	void set_count (int n) { pc_count = n; };
	int get_count () { return pc_count; };
	void set_next_iter ( Param_Control *iter) { 
                pc_next = iter;
                pc_next->set_count (pc_count+1);
        };
	Param_Control *get_iter_head () { return (pc_head); };
	Param_Control *get_iter_next () {
                if (pc_head && pc_next)
                        return pc_next;
                else
                        return NULL;
	};

        gboolean check_gsettings_path (){
                if (gsettings_path == NULL)
                        init_pcs_gsettings_path_and_key ();
                                
                return (gsettings_path != NULL);
        }

        const gchar *get_gsettings_path () { return gsettings_path;  };
        const gchar *get_gsettings_key() { return gsettings_key;  };

        void set_suspend_settings_update (gboolean x=true) { suspend_settings_update = x; };
        void set_logscale_min(double eps=1e-4) { log_min = eps; };
        void set_logscale_magshift(int msh=0) { log_mag_shift = msh; };
        
 protected:
	UnitObj *unit;
	gchar *warning;
	gchar *warn_color[2];
	gchar *color;
	void (*ChangeNoticeFkt)(Param_Control* pcs, gpointer data);
	gpointer FktData;
	double vMin, vMax;
	double vMin_warn, vMax_warn;
	double v_ex_lo, v_ex_hi;
	gchar *refname;
	gchar *prec;
	gchar *info;
	int   mag_prefix_shift;
	int   adj_mode;
        int log_mag_shift;
        double log_min;
        double adj_current_limits[2];
	//Warning types: 1 = max/min limit
	//               2 = exclude range
	double new_value;	//in the case of a warning this value keeps the current input
	int ShowMessage_flag;	//indicates if a message dialog is active

 private:
	//one of these holds the value of the object. Use Set_dValue() or Get_dValue()
	double Current_Dval;
	double *Dval;
	unsigned long *ULval;
	int    *Ival;
	short  *Sval;
        
protected:
        const gchar *StringVal;
	Param_Control *pc_head;
	Param_Control *pc_next;
        int pc_count;
        gchar *gsettings_adj_path;
        gchar *gsettings_adj_path_dir;
        gchar *gsettings_path;
        gchar *gsettings_path_dir;
        gchar *gsettings_key;
        GVariant *g_variant_var;
        GSettings *pcs_settings;
        gboolean suspend_settings_update;
};

class Gtk_EntryControl : public Param_Control{
 public:
	Gtk_EntryControl(UnitObj *U, const char *W, double *V, double VMi, double VMa, const char *p, GtkWidget *w,
			 double AdjStep=0., double AdjPage=0., double AdjProg=0., gint index=0):
		Param_Control(U, W, V, VMi, VMa, p){
		entry=w;
		extra=NULL;
                opt_scale = NULL;
                set_count (index);
		InitRegisterCb(AdjStep, AdjPage, AdjProg);
	};
	Gtk_EntryControl(UnitObj *U, const char *W, unsigned long *V, double VMi, double VMa, const char *p, GtkWidget *w,
			 double AdjStep=0., double AdjPage=0., double AdjProg=0., gint index=0):
		Param_Control(U, W, V, VMi, VMa, p){
		entry=w;
		extra=NULL;
                opt_scale = NULL;
                set_count (index);
		InitRegisterCb(AdjStep, AdjPage, AdjProg);
	};
	Gtk_EntryControl(UnitObj *U, const char *W, int *V, double VMi, double VMa, const char *p, GtkWidget *w,
			 double AdjStep=0., double AdjPage=0., double AdjProg=0., gint index=0):
		Param_Control(U, W, V, VMi, VMa, p){
		entry=w;
		extra=NULL;
                opt_scale = NULL;
                set_count (index);
		InitRegisterCb(AdjStep, AdjPage, AdjProg);
	};
	Gtk_EntryControl(UnitObj *U, const char *W, int *V, int VMi, int VMa, const char *p, GtkWidget *w,
			 double AdjStep=0., double AdjPage=0., double AdjProg=0., gint index=0):
		Param_Control(U, W, V, (double)VMi, (double)VMa, p){
		entry=w;
		extra=NULL;
                opt_scale = NULL;
                set_count (index);
		InitRegisterCb(AdjStep, AdjPage, AdjProg);
	};
	Gtk_EntryControl(UnitObj *U, const char *W, short *V, double VMi, double VMa, const char *p, GtkWidget *w,
			 double AdjStep=0., double AdjPage=0., double AdjProg=0., gint index=0):
		Param_Control(U, W, V, VMi, VMa, p){
		entry=w;
		extra=NULL;
                opt_scale = NULL;
                set_count (index);
		InitRegisterCb(AdjStep, AdjPage, AdjProg);
	};
	Gtk_EntryControl(UnitObj *U, const char *W, const gchar *V,  GtkWidget *w, gint index=0):
		Param_Control(U, W, V){
		entry=w;
		extra=NULL;
                opt_scale = NULL;
                set_count (index);
		InitRegisterCb(0.,0.,0.);
	};
	virtual ~Gtk_EntryControl(){
		//    gtk_signal_handlers_destroy( G_OBJECT( entry ) );
		adjustment_callback (NULL, NULL); // cleanup static timer if allocated
	};

	static gint update_callback(GtkEditable *editable, void *data);
        static void entry_focus_leave_callback (GtkEventController *controller, gpointer self);

	static void adjustment_callback (GtkAdjustment *adj, Gtk_EntryControl *gpcs);
        static void pcs_adjustment_configure_response_callback (GtkDialog *dialog, int response, gpointer user_data);
	void pcs_adjustment_configure ();
	void get_pcs_configuartion ();
	void put_pcs_configuartion ();

	virtual void ShowInfo (const char *header, const char *text);
	virtual gboolean ShowVerifySet (const char *header, const char *text);
	static int force_button_callback(gpointer ec_object, gpointer dialog);
	static int cancel_button_callback(gpointer ec_object, gpointer dialog);
	static int quit_callback(gpointer ec_object, gpointer dialog);
	virtual void Set_NewValue (gboolean set_new_value);

        void calc_adj_log_gain (){
                if (adj){
                        double dec_l = log10 (fabs (gtk_adjustment_get_lower (adj)/log_min));
                        double dec_r = log10 (fabs (gtk_adjustment_get_upper (adj)/log_min));
                        double dec   = dec_l > dec_r ? dec_l : dec_r;
                        // log_min=0.1 .. 1 .. 10 .. 100=upper
                        //         10^0   ^1   ^2    ^3        ~> 0 ... log10(upper/log_min)
                        gain  = dec / (dec_l > dec_r ? fabs (gtk_adjustment_get_lower (adj)) : fabs (gtk_adjustment_get_upper (adj)));
                }
        };        
        double adj_to_value (double av) {
                if ((adj_mode & PARAM_CONTROL_ADJUSTMENT_LOG_MODE_MASK) && adj){
                        double signum = av >= 0.? 1.:-1.;
                        return signum * log_min * pow (10., fabs (gain*av));
                }
                return (av);
        };

        double value_to_adj (double v) {
                if ((adj_mode & PARAM_CONTROL_ADJUSTMENT_LOG_MODE_MASK) && adj){
                        double signum = v >= 0.? 1.:-1.;
                        if (fabs (v) < log_min)
                                return  0.;
                        return signum * log10 (fabs (v/log_min)) / gain;
                }
                return (v);
        };
        
	GtkAdjustment* GetAdjustment(){ return adj; };
	void SetExtraWidget(GtkWidget *e){ extra = e; };

        void SetScaleWidget(GtkWidget *s, gint mode){ opt_scale=s; ticks_scale_mode=mode; };
        
	void Show(gint flg){ 
		if(flg){ gtk_widget_show(entry); enable_client=TRUE; }
		else{ enable_client=FALSE; gtk_widget_hide(entry); }
	};
	void Thaw(){ 
		gtk_editable_set_editable (GTK_EDITABLE (entry), TRUE); 
		gtk_widget_set_sensitive (entry, TRUE);
		if (extra)
			gtk_widget_set_sensitive (extra, TRUE);
	};
	void Freeze(){ 
		gtk_widget_set_sensitive (entry, FALSE);
		gtk_editable_set_editable (GTK_EDITABLE (entry), FALSE); 
		if (extra)
			gtk_widget_set_sensitive (extra, FALSE);
	};
	virtual gpointer GetEntryData(const gchar *txtid){ 
		return g_object_get_data( G_OBJECT (entry), txtid);
	};

	GSList *AddEntry2RemoteList(const gchar *RefName, GSList *remotelist);

	int CheckRemoteCmd(remote_args* data){
		if (data) 
			if (data->arglist[0] && data->arglist[1]){
				if(! strcmp(data->arglist[0], "get")){ // query entry value, return in data->qvalue
					if(refname && data->arglist[1])
						if(! strcmp(data->arglist[1], refname)){
                                                        if (StringVal){
                                                                data->qvalue = 0.;
                                                                data->qstr = StringVal;
                                                        }else{
                                                                data->qvalue = Convert2Usr (Get_dValue ()); // return value is in user units
                                                                data->qstr = NULL;
                                                        }
							return TRUE;
						}
				} else if(refname && data->arglist[1] && data->arglist[2])
					if(! strcmp(data->arglist[1], refname)){ // set entry action
						Set_Parameter(atof(data->arglist[2]), TRUE, TRUE);
						return TRUE;
					}
			}
		return FALSE;
	};

	virtual void Put_Value();
	virtual void Set_Parameter(double value=0., int flg=FALSE, int usr2base=FALSE);

        virtual void init_pcs_gsettings_path_and_key ();
	virtual void write_pcs_gschema (int array_flag=false);
        void init_pcs_array (){
               if (generate_pcs_gschema)
                       write_pcs_gschema_array ();
               else
                       init_pcs_via_list ();
        };
        void write_pcs_gschema_array (){
                for (Param_Control *pc = pc_head; pc; pc = pc->get_iter_next ())
                        pc->write_pcs_gschema (true);
        };
        void init_pcs_via_list ();

        virtual void get_init_value_from_settings (int array_flag=false);
        virtual void update_value_in_settings (int array_flag=false);

private:
	void InitRegisterCb(double AdjStep, double AdjPage, double AdjProg);

	GtkWidget *entry, *extra, *opt_scale;
        Scale_Ticks *ticks;
	GtkAdjustment *adj;
        gulong adjcb_handler_id, ec_io_handler_id[2], af_update_handler_id[2];
	gint enable_client;
        
	double page10, page, step, progressive;
	gint show_adj:1;
	gint show_scale:1;
	gint show_extra:1;

        gint ticks_scale_mode;

        double gain;
};


#endif




