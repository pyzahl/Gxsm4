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

to clean dconf database and reset run as user:
dconf reset -f /org/gnome/gxsm4/

The Gtk_EntryControl class is used to create  most of the input fields of GXSM. It handles
the units and supports arrow keys and (optional) adjustments.
Beware: Gtk_EntryControl is derived from Param_Control which contains some of the important
functions and variables. Some of the functions of Param_Control are replaced by functions of
the Gtk_EntryControl class. Look for 'virtual' functions in the header file.

'change' callbacks will be handled by Gtk_EntryControl::Set_Parameter, so this is always a
good starting point.
*/

#include <stdlib.h>
#include <config.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <iostream>

#include "xsmdebug.h"
#include "glbvars.h"
#include "gapp_service.h"
#include "pcs.h"
#include "util.h"
#include "plugin.h"
#include "gnome-res.h"

#define EC_INF 1e133


// PCS debugging/watching
// #define DEBUG_PCS_LOG


int total_message_count = 0;
int total_message_count_int = 0;

gint global_pcs_count = 0;
gboolean pcs_schema_writer_first_entry = true;
gboolean pcs_adj_schema_writer_first_entry = true;

// used for PCS VALUE GSCHEMA GENERATION
gchar *current_value_path = NULL;
gchar *generate_pcs_value_gschema_path_add_prev = NULL; // prev. schema name

// used for PCS ADJUSTEMNENTS GSCHEMA GENERATION
gchar *current_pcs_path = NULL;
gchar *generate_pcs_adjustment_gschema_path_add_prev = NULL; // prev. schema name

gboolean pcs_adj_write_missing_schema_entry = false;

gchar *current_pcs_gschema_path_group = NULL;


// global group control atg build time

const gchar* pcs_get_current_gschema_group (){
        return current_pcs_gschema_path_group ?
                strlen (current_pcs_gschema_path_group) > 0 ?
                current_pcs_gschema_path_group : "core-x" : "core-y";
}

void pcs_set_current_gschema_group (const gchar *group){
        if (current_pcs_gschema_path_group)
                g_free (current_pcs_gschema_path_group);
        current_pcs_gschema_path_group=g_strdup (group);
}

        
Param_Control::Param_Control(UnitObj *U, const char *W, double *V, double VMi, double VMa, const char *p){
	XSM_DEBUG (DBG_L8, "PCS-double:: " << *V << ", " << VMi << ", " << VMa << ", " << p);
	unit = U->Copy ();
	warning = W ? g_strdup(W) : NULL;
	Dval = V; Ival=0; ULval=0; Sval=0; StringVal=NULL;
	Init();
	setMin (VMi);
	setMax (VMa);
	prec = g_strdup(p);
}

Param_Control::Param_Control(UnitObj *U, const char *W, unsigned long *V, double VMi, double VMa, const char *p){
	XSM_DEBUG (DBG_L8, "PCS-ulong:: " << *V << ", " << VMi << ", " << VMa << ", " << p);
	unit = U->Copy ();
	warning = W ? g_strdup(W) : NULL;
	ULval = V; Ival=0; Dval=0; Sval=0; StringVal=NULL;
	Init();
	setMin (VMi);
	setMax (VMa);
	prec = g_strdup(p);
}

Param_Control::Param_Control(UnitObj *U, const char *W, int *V, double VMi, double VMa, const char *p){
	XSM_DEBUG (DBG_L8, "PCS-int:: " << *V << ", " << VMi << ", " << VMa << ", " << p);
	unit = U->Copy ();
	warning = W ? g_strdup(W) : NULL;
	Ival = V; ULval=0; Dval=0; Sval=0; StringVal=NULL;
	Init();
	setMin (VMi);
	setMax (VMa);
	prec = g_strdup(p);
}

Param_Control::Param_Control(UnitObj *U, const char *W, short *V, double VMi, double VMa, const char *p){
	XSM_DEBUG (DBG_L8, "PCS-short:: " << *V << ", " << VMi << ", " << VMa << ", " << p);
	unit = U->Copy ();
	warning = W ? g_strdup(W) : NULL;
	Sval = V; ULval=0; Ival=0; Dval=0; StringVal=NULL;
	Init();
	setMin (VMi);
	setMax (VMa);
	prec = g_strdup(p);
}

Param_Control::Param_Control(UnitObj *U, const char *W, const gchar *SV){
	XSM_DEBUG (DBG_L8, "PCS-string:: " << W);
	unit = U->Copy ();
	warning = W ? g_strdup(W) : NULL;
	Sval = 0; ULval=0; Ival=0; Dval=0; StringVal=W;
	Init();
	setMin (0.);
	setMax (0.);
	prec = g_strdup("");
}

Param_Control::~Param_Control(){
	if (warning)
		g_free (warning);
	if (color)
		g_free (color);
	if (warn_color[0])
		g_free (warn_color[0]);
	if (warn_color[1])
		g_free (warn_color[1]);
	g_free(prec);
	if (refname)
		g_free (refname);
	if (info)
		g_free (info);

        if (gsettings_adj_path)
                g_free (gsettings_adj_path);
        if (gsettings_path)
                g_free (gsettings_path);
        if (gsettings_adj_path_dir)
                g_free (gsettings_adj_path_dir);
        if (gsettings_path_dir)
                g_free (gsettings_path_dir);
        if (gsettings_key)
                g_free (gsettings_key);

        if (pcs_settings)
                g_clear_object (&pcs_settings);

        delete (unit);
}

void Param_Control::Init(){
	Current_Dval = -9e999; // internal buffer

	set_exclude ();
	color = NULL;
	warn_color[0] = NULL;
	warn_color[1] = NULL;
	ChangeNoticeFkt = NULL;
	FktData = NULL;
	refname = NULL; // g_strdup_printf ("pcs%04d", ++global_pcs_count);
	info=NULL;
	ShowMessage_flag=0;
        suspend_settings_update = false;
	set_adjustment_mode (PARAM_CONTROL_ADJUSTMENT_LINEAR);
        set_logscale_min (); // set default
        set_logscale_magshift (); // set default
        
        // GSettings 
        pc_head = NULL; 
        pc_next = NULL;
        pc_count = 0;
        gsettings_adj_path_dir = NULL;
        gsettings_path_dir = NULL;
        gsettings_adj_path = NULL;
        gsettings_path = NULL;
        gsettings_key = NULL;
        g_variant_var = NULL;
        pcs_settings = NULL;
}

void Param_Control::Val(double *V){
	Dval = V; Ival=0; Sval=0; StringVal=NULL;
}

void Param_Control::Val(int *V){
	Ival = V; Dval=0; Sval=0; StringVal=NULL;
}

void Param_Control::Val(unsigned long *V){
	ULval=V; Ival = 0; Dval=0; Sval=0; StringVal=NULL;
}

void Param_Control::Val(short *V){
	Sval = V; Ival=0; Dval=0; StringVal=NULL;
}

void Param_Control::Val(const gchar *V){
	Sval = 0; Ival=0; Dval=0; StringVal=V;
}

void Param_Control::setMax(double VMa, double Vmax_warn, const gchar* w_color){
	vMax = VMa;
	vMax_warn = Vmax_warn;
	if (warn_color[1]){
		g_free (warn_color[1]);
		warn_color[1] = NULL;
	}
	if (w_color) warn_color[1] = g_strdup (w_color);
	update_limits ();
}
  
void Param_Control::setMin(double VMi, double Vmin_warn, const gchar* w_color){
	vMin = VMi;
	vMin_warn = Vmin_warn;
	if (warn_color[0]){
		g_free (warn_color[0]);
		warn_color[0] = NULL;
	}
	if (w_color) warn_color[0] = g_strdup (w_color);
	update_limits ();
}

void Param_Control::set_exclude(double V_ex_lo, double V_ex_hi){
	v_ex_lo = V_ex_lo;
	v_ex_hi = V_ex_hi;
	update_limits ();
}

void Param_Control::set_info (const gchar* Info){
	if (info)
		g_free (info);
	info = NULL;
	if (Info)
		info = g_strdup (Info);	
}

void Param_Control::Prec(const char *p){
	g_free(prec);
	prec = g_strdup(p);
}

void Param_Control::ShowInfo (const char *header, const char *text){
        g_message ("%s\n%s", header, text);
}

gboolean Param_Control::ShowVerifySet (const char *header, const char *text){
        g_message ("%s\n%s", header, text);
        return true; // no interaction in vitual base
}

void Param_Control::Put_Value(){
	gchar *txt = Get_UsrString ();
	XSM_DEBUG(DBG_L2, txt);
        g_free (txt);
}

double Param_Control::Get_dValue(){
	if(Dval)
		return *Dval;
	else
		if(Ival)
			return (double)*Ival;
		else
			if(Sval)
				return (double)*Sval;
			else
				if(ULval)
					return (double)*ULval;
	return 0.;
}

void Param_Control::Set_dValue(double nVal){
	Current_Dval = nVal; // internal buffer

	if(Dval)
		*Dval = nVal;
	else
		if(Ival)
			*Ival = (int)nVal;
		else
			if(Sval)
				*Sval = (short)nVal;
			else
				if(ULval)
					*ULval = (unsigned long)nVal;
}

void Param_Control::set_refname(const gchar *ref){
        if (refname) g_free (refname);
        refname = g_strdup(ref);

        if (pc_count == 0){
                if (main_get_generate_pcs_gschema ())
                        write_pcs_gschema ();
                else
                        get_init_value_from_settings ();
        }
}

gchar *Param_Control::get_refname(){
	gchar *txt;	
        // XSM_DEBUG (DBG_L5, "Refname=" << refname );
	txt = g_strdup(refname);
	return txt;
}


gchar *Param_Control::Get_UsrString(){
	gchar *warn;
	if (color) g_free (color);
	color = NULL;

        if (StringVal)
                return g_strdup (StringVal);
        
	if (Get_dValue() <= vMin_warn && warn_color[0]){
		color = g_strdup (warn_color[0]);
		warn = g_strdup_printf ("low (<%g)", vMin_warn);
	}else if (Get_dValue() >= vMax_warn && warn_color[1]){
		color = g_strdup (warn_color[1]);	
		warn = g_strdup_printf ("hi (>%g)", vMax_warn);
	}else
		warn = g_strdup (" ");

	gchar *txt;	
	if (strncmp (prec, "04X", 3) == 0){
                gchar *fmt = g_strdup_printf("%%%s %s %s %s", prec, unit->Symbol(), info?info:" ", warn);
		int h = (int)unit->Base2Usr (Get_dValue ());
		txt = g_strdup_printf(fmt, (int)unit->Base2Usr(Get_dValue()));
                g_free(fmt);
		XSM_DEBUG (DBG_L5, "Param_Control::Get_UsrString -- PCS H Usr out: " << h << " ==> " << txt);
	}
	else{
                txt = g_strdup_printf("%s %s %s", unit->UsrString (Get_dValue(), UNIT_SM_NORMAL, prec) , info? info:" ", warn);
        }
        g_free(warn);

	return txt;
}


gboolean Param_Control::Set_FromValue(double nVal){
#ifdef DEBUG_PCS_LOG
        g_message ("PCS Set_FromValue newVal: %g   CurdVal: %g dVal: %g", nVal, Current_Dval, Get_dValue() );
#endif
	if (ShowMessage_flag)
                return false;	//do nothing if a message dialog is active
        if (StringVal)
                return false;

        if (nVal == Current_Dval){
#if 0
                if (strncmp(refname, "dsp-gvp", 7)==0) // weird patch
                        g_message ("Param_Control::Set_FromValue: same value[%s]: %g", refname, Current_Dval); // TEST
                else
                        return false;
#else
                return false; // no actual value change, done.
#endif
        }
        
#ifdef DEBUG_PCS_LOG
        g_message ("PCS Set_FromValue %g   dValue: %g", nVal, Get_dValue() );
#endif
        
	new_value = nVal;
	if(nVal <= vMax && nVal >= vMin){
		if(nVal >= vMax_warn || nVal <= vMin_warn){
			//check if the current value is already inside the warning range -- then OK
			if (Get_dValue() >= vMax_warn || Get_dValue() <= vMin_warn){
				Set_dValue (nVal); // set managed value variable
                                Put_Value (); // update/auto reformat entry
                                return true;
			}
			else{
				if (warn_color[0] || warn_color[1]){
					; // use warn colors, port to PangoAttribute done.
				} else {
					//The input exceeds the warning limits
					//Put_Value will reset the entry to the current valid value
					//Changes will eventually be done by the CB of the ShowMessage dialog
					gchar *ref = g_strconcat("[",refname ? refname:" ","]",NULL);
					gchar *txt = g_strdup_printf("Do you really want to\nenter the WARNING range?\n\n%s = %g", ref, new_value);
					gboolean success = ShowVerifySet ("Warning Limit reached!", txt);
                                        Set_NewValue (success); // set managed value variable on success of user confirmation, else restore previous
                                        g_free (ref);
                                        g_free (txt);
                                        Put_Value (); // update/auto reformat entry
                                        return success;
				}
			}
                        Put_Value (); // update/restore
                        return false;
 		}

		//check if the current value is already exclude range -- then OK
		if(nVal >= v_ex_lo && nVal <= v_ex_hi){
			if (Get_dValue() >= v_ex_lo && Get_dValue() <= v_ex_hi){
				Set_dValue (nVal); // set managed value variable
                                Put_Value (); // update/auto reformat entry
                                return true;
			}
			else{
				gchar *ref = g_strconcat("[",refname ? refname:" ","]",NULL);
				gchar *txt = g_strdup_printf("Do you really want to\nenter the EXCLUDE range?\n\n%s = %g", ref, new_value);
				gboolean success = ShowVerifySet ("Exclude Limit reached!", (const gchar*)txt);
                                Set_NewValue (success); // set managed value variable on success of user confirmation, else restore previous
                                g_free (ref);
                                g_free (txt);
                                Put_Value (); // update/auto reformat entry
                                return success;
			}
		}

                // regular valid range, set managed value variable
                Set_dValue (nVal);
                Put_Value (); // set managed value variable
                return true;
	}
	else{ // out if absolute bounds, do nothing but show valid range information and restore previous value
		gchar *ref = g_strconcat("[",refname ? refname:" ","]",NULL);
		gchar *txt = g_strdup_printf("for %s: %s ... %s\nrequested: %s", 
					     ref,
					     unit->UsrString (vMin), 
					     unit->UsrString (vMax),
                                             unit->UsrString (nVal));

		if (warning)
			ShowInfo (MLD_VALID_RANGE, txt);
		else
			g_print ("%s", txt);

		g_free (txt);
		g_free (ref);
                Put_Value(); // set managed value variable

                // test
                // pcs_adjustment_configure ();

                return false;
	}
}

void Param_Control::Set_Parameter(double value, int flg, int usr2base){
	if(flg){
		if(usr2base)
			Set_FromValue(unit->Usr2Base(value));
		else
			Set_FromValue(value);
	}else{
		gchar *ctxt = Get_UsrString ();
		XSM_DEBUG(DBG_L2, "Set Value [" << ctxt << "] = " );
		std::cin >> ctxt;
		Set_FromValue(unit->Usr2Base(ctxt));
                g_free (ctxt);
	}
	if(ChangeNoticeFkt)
		(*ChangeNoticeFkt)(this, FktData);
}

// make valid gschema key
gchar *key_assure (const gchar *key){
        gchar *t;
        gchar *p, *k;
        int nup=0;
        for (p=(gchar*)key; *p; ++p)
                if (g_ascii_isupper(*p))
                        ++nup;

        if (g_ascii_isalpha (*key))
                p = t = g_strndup (key, nup + strlen (key));
        else {
                p = t = g_strndup (key, 1 + nup + strlen (key));
                *p = 'x'; ++p;
        }
        for (k=(gchar*)key; *k; ++p, ++k){
                *p = *k; // copy
                if (*p == '/') *p = '-';
                if (*p == '.') *p = '-';
                if (*p == ' ') *p = '-';
                if (*p == '_') *p = '-';
                if (g_ascii_isupper (*p)){
                        if (k == key)
                                *p = g_ascii_tolower (*p);
                        else{
                                if (*(p-1) != '-' && (g_ascii_islower(*(k-1)) || g_ascii_islower(*(k+1)))) { *p = '-'; ++p; }
                                *p = g_ascii_tolower (*k);
                        }
                }
        }
        return t;
}

        
void Gtk_EntryControl::init_pcs_gsettings_path_and_key (){
        if (refname && gsettings_path == NULL){
                
                gsettings_path_dir = g_strdup_printf ("%s/pcs/%s", GXSM_RES_BASE_PATH, pcs_get_current_gschema_group ());
                gsettings_path = g_strdup_printf ("%s.pcs.%s", GXSM_RES_BASE_PATH_DOT, pcs_get_current_gschema_group ());
                gsettings_key = key_assure (refname);
        }
        gsettings_adj_path_dir = g_strdup_printf ("%s/pcsadjustments/%s", GXSM_RES_BASE_PATH, pcs_get_current_gschema_group ());
        gsettings_adj_path = g_strdup_printf ("%s.pcsadjustments.%s", GXSM_RES_BASE_PATH_DOT, pcs_get_current_gschema_group ());
}

void Gtk_EntryControl::write_pcs_gschema (int array_flag){

        if (main_get_generate_pcs_gschema () && refname) {
                std::ofstream logf; logf.open ("gxsm-pcs-writer.log",  std::ios::app);
                std::ofstream f;
                std::ifstream fi;

                if (!check_gsettings_path ()){
                        logf << "Error: GSettings Path not set for '" << refname << "'. [" << array_flag << "], " << std::endl;
                        return;
                }

                gchar *tmppathname = g_strdup_printf ("%s.gschema.xml", gsettings_path);
                
                if (current_value_path)
                        g_free (current_value_path);
                current_value_path = tmppathname;
                //                                            x
                logf << "opening pcs-value xml file new for:  " << current_value_path
                     << "  [" << refname << "] array_flag=" << array_flag << " count=" << get_count () << std::endl;

                if (! g_file_test (tmppathname, G_FILE_TEST_EXISTS)){
                        f.open (current_value_path, std::ios::out);
                        f << "<schemalist>" << std::endl;
                        f.close ();
                }

                // -- identify end tag and truncate if exists for continuation
                //  </schema> 
                //</schemalist>
                fi.open (current_value_path, std::ios::in);
                fi.seekg (-28, std::ios_base::end);
                if (!fi.fail ()){
                        gchar x[1024]; x[0]=0;
                        fi.getline (x, 1024);
                        //                        logf << "pcs-end-read: [" << x << "]" << std::endl;
                        if (strcmp (x, "  </schema>") == 0){ // overwrite from here!
                                fi.seekg (-28, std::ios_base::end);
                                off_t pos = fi.tellg ();
                                fi.close ();
                                truncate(current_value_path, pos);
                                //      logf << "  ---> ov from eof-26 at " << pos << std::endl;
                        } else
                                fi.close ();
                } else
                        fi.close ();
                
                logf.close ();

                f.open (current_value_path, std::ios::app);

                
                if (!generate_pcs_value_gschema_path_add_prev)
                        generate_pcs_value_gschema_path_add_prev = g_strdup("dummy");

                if (strcmp (pcs_get_current_gschema_group (), generate_pcs_value_gschema_path_add_prev)){
                        g_free (generate_pcs_value_gschema_path_add_prev);
                        generate_pcs_value_gschema_path_add_prev = g_strdup (pcs_get_current_gschema_group ());

                        f << "  <schema id=\"" << gsettings_path
                          << "\" path=\"/" << gsettings_path_dir << "/\">" 
                          << std::endl;
                }

                gchar *us = Get_UsrString();
                if (!us) us = g_strdup ("N/A");
                const gchar *cpn = ((const gchar*)g_object_get_data( G_OBJECT (entry), "Adjustment_PCS_Name"));
                gchar *pn;
                if (!cpn) pn = g_strdup ("PCS has no adjustment");
                else pn = g_strdup_printf ("Adjustment PCS Name: '%s'", cpn);

                gchar *pcs_id=g_strdup_printf ("pcs%04d", global_pcs_count);

                if (array_flag && pc_count == 1){

                        f << std::endl
                          << "    <key name=\"" << gsettings_key << "\" type=\"ad\">" << std::endl
                          << "      <default>[" << Get_dValue() ;

                } else if (array_flag && pc_next == NULL ){

                        f << "," << Get_dValue() << "]</default>" << std::endl
                          << "      <summary>PCS Remote-ID: '" << refname << "', Default/U: " << us << " </summary>" << std::endl
                          << "      <description>" << pcs_id << ", Array-N: " << pc_count << ", PCS Remote-ID: '" << refname << "', Default/U: " << us << ", " << pn << " </description>" << std::endl
                          << "    </key>"  << std::endl;
                        
                } else if (array_flag){
                        f << "," << Get_dValue() ;
                } else {
                        // Add: HANDLE StringVal!!
                        f << std::endl
                          << "    <key name=\"" << gsettings_key << "\" type=\"d\">" << std::endl
                          << "      <default>" << Get_dValue() << "</default>" << std::endl
                          << "      <summary>PCS Remote-ID: '" << refname << "', Default/U: " << us << " </summary>" << std::endl
                          << "      <description>" << pcs_id << ", PCS Remote-ID: '" << refname << "', Default/U: " << us << ", " << pn << " </description>" << std::endl
                          << "    </key>"  << std::endl;
                }

                g_free (pcs_id);
                g_free (us);
                g_free (pn);

                f << "  </schema>"  << std::endl << std::endl;
                f << "</schemalist>"  << std::endl << std::endl;
                
                f.close ();
        }
}

void Gtk_EntryControl::get_init_value_from_settings (int array_flag){
        if (check_gsettings_path ()){
                XSM_DEBUG_GP (DBG_L2, "PCS init: looking for %s.%s array_flag=%d pcs_count=%d\n", gsettings_path, gsettings_key, array_flag, get_count ());
                
                if (!array_flag && get_count () == 0){
                        if (pcs_settings == NULL)
                                pcs_settings = g_settings_new (gsettings_path);
                        if (!StringVal)
                                Set_FromValue (g_settings_get_double (pcs_settings, gsettings_key));
                } else
                        init_pcs_via_list ();
                XSM_DEBUG_GP (DBG_L2, "PCS init done.\n");
        }
}
 
void Gtk_EntryControl::update_value_in_settings (int array_flag){
        if (suspend_settings_update)
                return;

        if (check_gsettings_path ()){
                
                XSM_DEBUG_GP (DBG_L2, "PCS update value for %s.%s[%d] = %g\n", gsettings_path, gsettings_key, get_count (), Get_dValue () );
                
                if (!array_flag && get_count () == 0){
                        if (pcs_settings == NULL)
                                pcs_settings = g_settings_new (gsettings_path);
                        g_settings_set_double (pcs_settings, gsettings_key, Get_dValue());
                } else { 
                        if (pc_head && get_count () > 0){
                                unsigned int i=0;
                                Param_Control *pc_last;
                                for (Param_Control *pc = pc_head; pc; pc = pc->get_iter_next ())
                                     pc_last = pc;
                                gsize gn = pc_last->get_count ();
                                gdouble *arr = g_new (gdouble, gn);
      
                                for (Param_Control *pc = pc_head; pc && i<gn; pc = pc->get_iter_next (), ++i)
                                        arr[i] = pc->Get_dValue();

                                if (pc_head->check_gsettings_path ()){
                                        if (pcs_settings == NULL)
                                                pcs_settings = g_settings_new (pc_head->get_gsettings_path ());
                                        
                                        g_variant_var = g_variant_new_fixed_array (G_VARIANT_TYPE_DOUBLE, (gconstpointer) arr, gn, sizeof (gdouble));
                                        g_settings_set_value (pcs_settings, pc_head->get_gsettings_key (), g_variant_var);

                                        //if (g_variant_var)
                                        // g_variant_unref (g_variant_var);  // not good to do here? -- settings is holing a  ref?
                                }
                        }
                }
        }
}

void  Gtk_EntryControl::init_pcs_via_list (){
       // must be last element
       if (pc_head && pc_next == NULL){
               if (pc_head->check_gsettings_path ()){
                       XSM_DEBUG_GP (DBG_L2, "PCS init list for %s.%s [%d]\n", pc_head->get_gsettings_path (),  pc_head->get_gsettings_key (),  pc_head->get_count () );
                       gsize gn;
                       if (pcs_settings == NULL)
                               pcs_settings = g_settings_new (pc_head->get_gsettings_path ());

                       g_variant_var = g_settings_get_value (pcs_settings, pc_head->get_gsettings_key ()) ;
                       gdouble *arr = (gdouble*) g_variant_get_fixed_array (g_variant_var, &gn, sizeof (gdouble));

                       guint i=0;
                       for (Param_Control *pc = pc_head; pc && i<gn; pc = pc->get_iter_next (), ++i)
                               pc->Set_FromValue (arr[i]);

                       // g_variant_unref (g_variant_var); // not good to do here? -- settings is holing a  ref?
               }
       }
}

double pcs_mag_base (double v, const gchar** pfx, double fac=10., int magshift=0) {
        int mi;
        static const gchar  *prefix[]    = { "a",  "f",   "p",   "n", UTF8_MU,   "m", "", "k", "M", "G", "T"  };
        //                            a:0    f:1    p:2    n:3    mu:4    m:5   1:6 k:7  M:8  G:9  T:10
        const double magnitude[]  = { 1e-18, 1e-15, 1e-12, 1e-9,  1e-6,   1e-3, 1., 1e3, 1e6, 1e9, 1e12 };
        double x = fac*v;
        if (fabs (x) < 1e-22){
                mi=6;
        } else {
                double m = fabs (x*1e-3);
                for (mi=0; mi<=10; ++mi)
                        if (m < magnitude[mi])
                                break;
                if (mi>10)
                        mi=10;
        }
        // g_print ("set_mag_get_base:: %g [x=%g]:[mi=%d]{%g}\n", v, x, mi, magnitude[mi]);
        mi +=  magshift;
        if (mi >= 0 && mi <= 10)
                *pfx = prefix[mi];
        else
                *pfx = "?";
        
        return x/magnitude[mi-magshift]/fac;
}

void Gtk_EntryControl::adjustment_callback(GtkAdjustment *adj, Gtk_EntryControl *gpcs){
	if (!adj) return;


        double  v=gtk_adjustment_get_value (adj);
#ifdef DEBUG_PCS_LOG
        g_message ("PCS:adj-callback get_vale (adj)  : %g --> v=%g", gtk_adjustment_get_value (adj), v);
#endif
        
        if (gpcs->adj_mode & PARAM_CONTROL_ADJUSTMENT_DUAL_RANGE){
                double al=gtk_adjustment_get_lower (adj);
                double au=gtk_adjustment_get_upper (adj);

                // value inside "Warn" Range, auto adjust
                if (al < gpcs->vMin_warn && au > gpcs->vMax_warn && v > gpcs->vMin_warn && v < gpcs->vMax_warn)
                        gtk_adjustment_set_upper (adj, gpcs->vMax_warn), gtk_adjustment_set_lower (adj, gpcs->vMin_warn);

                // value at edge(s) and Warn Range active, switch back to full scale
                if (fabs (al-gpcs->vMin_warn) < 1e-6 && fabs (au-gpcs->vMax_warn) < 1e-6
                    && (fabs (v-gpcs->vMin_warn) < 1e-6 || fabs (v-gpcs->vMax_warn) < 1e-6))
                        gtk_adjustment_set_upper (adj, gpcs->vMax), gtk_adjustment_set_lower (adj, gpcs->vMin);
        }

	if (gpcs->adj_mode & PARAM_CONTROL_ADJUSTMENT_LOG_MODE_MASK){
                // LOG ADJUSTMENT MODE
                gpcs->calc_adj_log_gain ();
                gpcs->Set_Parameter (gpcs->adj_to_value (v), TRUE, FALSE);
#ifdef DEBUG_PCS_LOG
                g_message ("PCS:adj-callback log_mode_auto_dual set paramerter %g -> val=%g", v, gpcs->adj_to_value (v));
#endif
                
                if (gpcs->adj_mode & PARAM_CONTROL_ADJUSTMENT_ADD_MARKS){
                        double l=gtk_adjustment_get_lower (adj);
                        double r=gtk_adjustment_get_upper (adj);

                        if (gpcs->adj_current_limits[0] != l || gpcs->adj_current_limits[1] != r){
                                gpcs->adj_current_limits[0] = l;
                                gpcs->adj_current_limits[1] = r;

                                if (gpcs->opt_scale){
                                        gtk_scale_clear_marks (GTK_SCALE (gpcs->opt_scale));
                                        gtk_scale_add_mark (GTK_SCALE (gpcs->opt_scale), 0, GTK_POS_BOTTOM, "0");

                                        double Lab0 = pow (10., floor (log10 (gpcs->log_min)));
                                        double MaxLab = fabs(r);
                                        for (int s = (gpcs->adj_mode & PARAM_CONTROL_ADJUSTMENT_LOG_SYM) ? -1 : 1; s<=1; s+=2) {
                                                double l10 = Lab0;
                                                double Lab = l10;
                                                double signum = s;
                                                for (int i=1; Lab<MaxLab; Lab+=l10, ++i){
                                                        if (Lab > 9.999*l10){ l10=Lab=10.*l10; i=1; }
                                                        double v = gpcs->value_to_adj (signum * Lab);
                                                        if (v>0 || v<0){
                                                                if (i!=5 && i!=1){ 
                                                                        gtk_scale_add_mark (GTK_SCALE (gpcs->opt_scale), v, GTK_POS_BOTTOM, NULL);
                                                                }
                                                                else{
                                                                        const gchar *pfx=NULL;
                                                                        double lp = pcs_mag_base (Lab, &pfx, 10., gpcs->log_mag_shift);
                                                                        gchar *tmp = g_strdup_printf ("<span size=\"x-small\">%g%s</span>", signum*lp, pfx);
                                                                        //g_message ("%d: %s %g %g @adv= %g", i, tmp, signum, Lab, v);
                                                                        gtk_scale_add_mark (GTK_SCALE (gpcs->opt_scale), v, GTK_POS_BOTTOM, tmp);
                                                                        g_free (tmp);
                                                                }
                                                        }
                                                }
                                                MaxLab = fabs(r);
                                        }
                                }
                        }
                }
        } else {
                // LINEAR ADJUSTMENT MODE
                //g_message ("Gtk_EntryControl::adjustment_callback ->  gpcs->Set_Parameter (gtk_adjustment_get_value (adj), TRUE, FALSE) val=%g new_adj_val=%g [%s]", gpcs->Get_dValue (), gtk_adjustment_get_value (adj), gpcs->get_refname ());

                gpcs->Set_Parameter (v, TRUE, FALSE);

                if (gpcs->adj_mode & PARAM_CONTROL_ADJUSTMENT_ADD_MARKS){
                        double l=gtk_adjustment_get_lower (adj);
                        double r=gtk_adjustment_get_upper (adj);

                        if (gpcs->adj_current_limits[0] != l ||  gpcs->adj_current_limits[1] != r){
                                gpcs->adj_current_limits[0] = l;
                                gpcs->adj_current_limits[1] = r;

                                if (gpcs->opt_scale){
                                        gtk_scale_clear_marks (GTK_SCALE (gpcs->opt_scale));
                                        double tic_w = r-l;
                                        double d_tic = AutoSkl(tic_w/11);
                                        double tic_0 = l;
                                        for(double x=AutoNext (tic_0, d_tic); x < r; x += d_tic){
                                                if (fabs(x/d_tic) < 1e-3)
                                                        x=0.;
                                                const gchar *pfx=NULL;
                                                double lp = pcs_mag_base (x, &pfx, 10., gpcs->log_mag_shift);
                                                gchar *tmp = g_strdup_printf ("<span size=\"x-small\">%g%s</span>", lp, pfx);
                                                gtk_scale_add_mark (GTK_SCALE (gpcs->opt_scale), x, GTK_POS_BOTTOM, tmp);
                                                g_free (tmp);
                                                if (x+d_tic/2 < r)
                                                        gtk_scale_add_mark (GTK_SCALE (gpcs->opt_scale), x+d_tic/2, GTK_POS_BOTTOM, NULL);
                                        }
                                }
                        }
                }
        }

        //g_message ("PCS:adj-cb.... end.");
}


void Gtk_EntryControl::Put_Value(){
        static time_t t0, t; // Scan - Startzeit eintragen 
	gchar *txt = Get_UsrString ();

        // #define DEBUG_PCS_PERF
#ifdef DEBUG_PCS_PERF
        static gint64 task_t_last=g_get_monotonic_time ();
        {
                gint64 tmp = g_get_monotonic_time ();
                XSM_DEBUG_GM (DBG_L1, "START ** PCS::Put_Value [%s] {%s} = %g, dt=%d, time=%d", refname?refname:"--", txt,  Get_dValue(), tmp-task_t_last, tmp);
                task_t_last = tmp;
        } 
#endif


        if (af_update_handler_id[0]){
                g_signal_handler_block (G_OBJECT (entry), af_update_handler_id[0]);
                //g_signal_handler_block (G_OBJECT (entry), af_update_handler_id[1]);
        }
        if (ec_io_handler_id[0]){
                g_signal_handler_block (G_OBJECT (entry), ec_io_handler_id[0]);
                //g_signal_handler_block (G_OBJECT (entry), ec_io_handler_id[1]);
        }

        if (GTK_IS_SPIN_BUTTON (entry)){
                gtk_spin_button_set_value (GTK_SPIN_BUTTON (entry), Get_dValue());
#ifdef DEBUG_PCS_LOG
                g_message ("PCS:put_value SPIN_BUTTON [%s] usrs='%s' v=%g", refname?refname:"--", txt,  Get_dValue());
#endif
        }
        else
                gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (entry))), txt, -1);
        
#ifdef DEBUG_PCS_LOG
        g_message ("PCS:put_value [%s] usrs='%s'", refname?refname:"--", txt);
#endif
        g_free (txt);


#ifdef DEBUG_PCS_PERF
        {
                gint64 tmp = g_get_monotonic_time ();
                XSM_DEBUG_GM (DBG_L1, "111 **** PCS::Put_Value [%s] dt=%d, time=%d", refname?refname:"--", tmp-task_t_last, tmp);
        }
#endif

        
        if (GTK_IS_ENTRY (entry)){
                if (color) {
                        // g_message ("Entry %s has warning color set.", refname);
                        PangoAttrList *prev_attrs = gtk_entry_get_attributes (GTK_ENTRY (entry));
                        GdkRGBA bgc;
                        gdk_rgba_parse (&bgc,color);
                        PangoAttribute *pa_fg = pango_attr_foreground_new ((guint16) (65535.*bgc.red),
                                                                           (guint16) (65535.*bgc.green),
                                                                           (guint16) (65535.*bgc.blue));
                        PangoAttrList *attrs = pango_attr_list_new (); // ref count 1
                        pango_attr_list_insert (attrs, pa_fg); // the attribute to insert. Ownership of this value is assumed by the list.
                        gtk_entry_set_attributes (GTK_ENTRY (entry), attrs);
                        if (prev_attrs){
                                // g_message ("Entry had Pango Attribs, replacing");
                                pango_attr_list_unref (prev_attrs);
                        }
                } else {
                        PangoAttrList *prev_attrs = gtk_entry_get_attributes (GTK_ENTRY (entry));
                        gtk_entry_set_attributes (GTK_ENTRY (entry), NULL);
                        if (prev_attrs){
                                // g_message ("Entry had Pango Attribs, set to none");
                                pango_attr_list_unref (prev_attrs);
                        }
                }
        }
        
#ifdef DEBUG_PCS_PERF
        {
                gint64 tmp = g_get_monotonic_time ();
                XSM_DEBUG_GM (DBG_L1, "111 **** PCS::Put_Value [%s] dt=%d, time=%d", refname?refname:"--", tmp-task_t_last, tmp);
        }
#endif

#if 0 // old GTK2 -- this was easier I have to say
	if (color) {
		GdkRGBA bgc;
		gdk_rgba_parse (&bgc,color);
                gtk_widget_override_color(GTK_WIDGET (entry), GTK_STATE_FLAG_NORMAL, &bgc); 		
	} else {
                gtk_widget_override_color( GTK_WIDGET(entry), GTK_STATE_FLAG_NORMAL, NULL); 		
	}
#endif
        
	if(adj){
                // skip self callbacks while reconfiguring
                g_signal_handler_block (G_OBJECT (adj), adjcb_handler_id);

                if (adj_mode & PARAM_CONTROL_ADJUSTMENT_LOG_MODE_MASK){
                        calc_adj_log_gain ();
                        gtk_adjustment_set_value (GTK_ADJUSTMENT(adj), value_to_adj (unit->Usr2Base (Get_dValue ())));
#ifdef DEBUG_PCS_LOG
                        g_message ("PCS:put-value to adj... log uv:%g -> adjv:%g", unit->Usr2Base (Get_dValue ()),value_to_adj (unit->Usr2Base (Get_dValue ())));
#endif
                } else {
                        gtk_adjustment_set_value (GTK_ADJUSTMENT(adj), unit->Usr2Base (Get_dValue()));
                }

                g_signal_handler_unblock (G_OBJECT (adj), adjcb_handler_id);
	}

        if (ec_io_handler_id[0]){
                //g_signal_handler_unblock (G_OBJECT (entry), ec_io_handler_id[1]);
                g_signal_handler_unblock (G_OBJECT (entry), ec_io_handler_id[0]);
        }

        if (af_update_handler_id[0]){
                g_signal_handler_unblock (G_OBJECT (entry), af_update_handler_id[0]);
                //g_signal_handler_unblock (G_OBJECT (entry), af_update_handler_id[1]);
        }

#ifdef DEBUG_PCS_PERF
        {
                gint64 tmp = g_get_monotonic_time ();
                XSM_DEBUG_GM (DBG_L1, "END **** PCS::Put_Value [%s] dt=%d, time=%d", refname?refname:"--", tmp-task_t_last, tmp);
        }
#endif
}

void Gtk_EntryControl::Set_Parameter(double Value=0., int flg=FALSE, int usr2base=FALSE){
	if (ShowMessage_flag) return;	//do nothing if a message dialog is active
	double value;
	GtkWidget *c;
	if(flg){
		if(usr2base)
			value=unit->Usr2Base(Value);
		else
			value=Value;
	}
	else{
                if (GTK_IS_SPIN_BUTTON (entry)){
                        value=unit->Usr2Base (gtk_spin_button_get_value (GTK_SPIN_BUTTON (entry)));
#ifdef DEBUG_PCS_LOG
                        g_message ("PCS:set_param SPIN_BUTTON [%s] v=%g", refname?refname:"--", value);
#endif
                }
                else{
                        const gchar *ctxt = gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (entry))));
                        if (strncmp (prec, "04X", 3) == 0){
                                int h;
                                sscanf (ctxt, "%x", &h);
                                value=(double)h;
                                XSM_DEBUG (DBG_L1, "Gtk_EntryControl::Set_Parameter -- PCS H: " << ctxt << " ==> " << value);
                        } else
                                value=unit->Usr2Base (ctxt);
                }
	}

#ifdef DEBUG_PCS_LOG
        g_message ("Gtk_EntryControl::Set_Parameter value=%g [%s] flg=%d ?-> usr2base=%d", value, refname, flg, usr2base);
#endif
        
	if (Set_FromValue (value)){ // preoceed only with updated and potential client if new value was accepted
                update_value_in_settings ();

                if ((c=(GtkWidget*)g_object_get_data( G_OBJECT (entry), "HasMaster"))){
                        Gtk_EntryControl *master = (Gtk_EntryControl *)g_object_get_data( G_OBJECT (c), "Gtk_EntryControl");
                        g_object_set_data( G_OBJECT (c), "HasRatio", GUINT_TO_POINTER((guint)round(1000.*new_value/master->Get_dValue())));
                }
                
                if((c=(GtkWidget*)g_object_get_data( G_OBJECT (entry), "HasClient")) && enable_client){
                        Gtk_EntryControl *cec = (Gtk_EntryControl *) g_object_get_data( G_OBJECT (c), "Gtk_EntryControl");

                        gpointer data = g_object_get_data( G_OBJECT (entry), "HasRatio");
                        double ratio = data ? (double)(GPOINTER_TO_UINT (data))/1000. : 1.0;
#ifdef DEBUG_PCS_LOG
                        g_message ("Gtk_EntryControl::Set_Parameter update client with value=%g [%s]", value, cec->refname);
#endif
                        if (cec->Set_FromValue (ratio * new_value)) // only if also this succeeded
                                cec->update_value_in_settings ();
                }
                if(ChangeNoticeFkt)
                        (*ChangeNoticeFkt)(this, FktData);
        }

}

void Gtk_EntryControl::Set_NewValue (gboolean set_new_value){
	GtkWidget *c;
	if (!set_new_value){
		ShowMessage_flag=0;
		return;
	}

	Set_dValue(new_value);
	Put_Value ();

        if ((c=(GtkWidget*)g_object_get_data( G_OBJECT (entry), "HasMaster"))){
                Gtk_EntryControl *master = (Gtk_EntryControl *)g_object_get_data( G_OBJECT (c), "Gtk_EntryControl");
                g_object_set_data( G_OBJECT (c), "HasRatio", GUINT_TO_POINTER((guint)round(1000.*new_value/master->Get_dValue())));
        }
        
        gpointer data = g_object_get_data( G_OBJECT (entry), "HasRatio");
        double ratio = data ? (double)(GPOINTER_TO_UINT (data))/1000. : 1.0;
        
	if((c=(GtkWidget*)g_object_get_data( G_OBJECT (entry), "HasClient")) && enable_client)
		((Gtk_EntryControl *)g_object_get_data( G_OBJECT (c), "Gtk_EntryControl"))->Set_FromValue (ratio * new_value);

	// The ChangeNoticeFkt commits the current value to the DSP
	if(ChangeNoticeFkt) (*ChangeNoticeFkt)(this, FktData);
	ShowMessage_flag=0;
}

void Gtk_EntryControl::ShowInfo (const char *header, const char *text){
	if (!text || !header) 
		return;

	++total_message_count_int;
	if (++total_message_count > 6){
		XSM_DEBUG_GW (-1, "Gtk_EntryControl::ShowInfo *** Repeting Messag(es) '%s' suppressed. count: %d", text, total_message_count);
		--total_message_count;
		return;
	}

	if (xsmres.gxsm4_ready){
        
                GtkWidget *dialog = gtk_message_dialog_new_with_markup
                        (GTK_WINDOW (gtk_widget_get_root (entry)),
                         GTK_DIALOG_DESTROY_WITH_PARENT,
                         GTK_MESSAGE_INFO,
                         GTK_BUTTONS_CLOSE,
                         "<span foreground='green' size='large' weight='bold'>%s</span>\n%s", header, text);
	
                g_signal_connect_swapped (G_OBJECT (dialog), "response",
                                          G_CALLBACK (gtk_window_destroy),
                                          G_OBJECT (dialog));
                gtk_widget_show (dialog);
        } else {
                XSM_DEBUG_GW (-1, "Gtk_EntryControl::ShowInfo *** WHILE GXSM4 STARTUP: %s %s ... auto continue.", header, text);
        }
        
        --total_message_count;
}

gboolean Gtk_EntryControl::ShowVerifySet (const char *header, const char *text){
	if (!text || !header) 
		return false;

        XSM_DEBUG_GP (DBG_L2, "ShowVerifySet Dialog: [%s] => %s: %s", refname, header, text);

	++total_message_count_int;
	if (++total_message_count > 6){
		g_warning ("Repeting Messag(es) '%s' suppressed. count: %d", text, total_message_count);
		--total_message_count;
		return false;
	}

        gint result = GTK_RESPONSE_NO; 
	if (xsmres.gxsm4_ready){
        
                GtkWidget *dialog = gtk_message_dialog_new_with_markup
                        (GTK_WINDOW (gtk_widget_get_root (entry)),
                         GTK_DIALOG_DESTROY_WITH_PARENT,
                         GTK_MESSAGE_INFO,
                         GTK_BUTTONS_YES_NO,
                         "<span foreground='red' size='large' weight='bold'>%s</span>", header);
                
                gtk_message_dialog_format_secondary_markup
                        (GTK_MESSAGE_DIALOG (dialog),
                         "%s for %s", text, refname);

		g_signal_connect_swapped (G_OBJECT (dialog), "response",
					  G_CALLBACK (gtk_window_destroy),
					  G_OBJECT (dialog));
		gtk_widget_show (dialog);
                // FIX-ME-GTK4
                //result = gtk_dialog_run (GTK_DIALOG (dialog));

        } else {
                g_warning ("WHILE GXSM4 STARTUP:  %s %s --> auto answer with 'NO'.", header, text);
        }
        --total_message_count;

        switch (result){
        case GTK_RESPONSE_YES:
                return true;
        default:
                return false;
        }
}


gint Gtk_EntryControl::update_callback(GtkEditable *editable, void *data){
	Gtk_EntryControl *current_object = ((Gtk_EntryControl *)g_object_get_data( G_OBJECT (editable), "Gtk_EntryControl"));
        if (editable && current_object){
                gchar *p = gtk_editable_get_chars (editable, 0 , -1);
                if (p){
                        if (current_object->StringVal){
                                ;
                        } else {
#ifdef DEBUG_PCS_LOG
                                gchar *tmp;
                                g_free (tmp);
                                double x=atof (p);
                                g_message ("Gtk_EntryControl::update_callback txt={%s} pcs=%s  [%g]", p, tmp=current_object->get_refname (), x);
                                current_object->Set_Parameter (x, FALSE, FALSE);
#else
                                current_object->Set_Parameter (atof (p), FALSE, FALSE);
#endif
                        }
                        g_free (p);
                } else {
                        gchar *tmp;
                        g_warning ("Gtk_EntryControl::update_callback -- empty editable text. {%s}", tmp=current_object->get_refname ());
                        if (tmp)
                                g_free (tmp);
                }
                        
        } else {
                g_warning ("Gtk_EntryControl::update_callback -- no current object.");
        }
	return FALSE;
}

void Gtk_EntryControl::entry_focus_leave_callback (GtkEventController *controller, gpointer self)
{
        //g_message ("ENTRY OUT-OF-FOCUS, buffer: %s", gtk_entry_buffer_get_text (gtk_entry_get_buffer (GTK_ENTRY (self))));
        update_callback (GTK_EDITABLE (self), NULL);
}


void Gtk_EntryControl::pcs_adjustment_configure_response_callback (GtkDialog *dialog, int response, gpointer user_data){
        Gtk_EntryControl *ec = (Gtk_EntryControl *) user_data;
        {
                gint mode_tmp = PARAM_CONTROL_ADJUSTMENT_LINEAR; // = 0
        
                if (g_object_get_data  (G_OBJECT (dialog), "CB-LOG-SCALE"))
                        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (g_object_get_data  (G_OBJECT (dialog), "CB-LOG-SCALE"))))
                                mode_tmp |= PARAM_CONTROL_ADJUSTMENT_LOG;
                if (g_object_get_data  (G_OBJECT (dialog), "CB-LOG-SYM"))
                        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (g_object_get_data  (G_OBJECT (dialog), "CB-LOG-SYM"))))
                                mode_tmp |= PARAM_CONTROL_ADJUSTMENT_LOG_SYM;
                if (g_object_get_data  (G_OBJECT (dialog), "CB-DUAL-RANGE"))
                        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (g_object_get_data  (G_OBJECT (dialog), "CB-DUAL-RANGE"))))
                                mode_tmp |= PARAM_CONTROL_ADJUSTMENT_DUAL_RANGE;
                if (g_object_get_data  (G_OBJECT (dialog), "CB-TICKS"))
                        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (g_object_get_data  (G_OBJECT (dialog), "CB-TICKS"))))
                                mode_tmp |= PARAM_CONTROL_ADJUSTMENT_ADD_MARKS;
                       
                if (response == GTK_RESPONSE_ACCEPT){
                        ec->set_adjustment_mode (mode_tmp);
                        ec->put_pcs_configuartion ();
                }
        }
        gtk_window_destroy (GTK_WINDOW (dialog));
}

void Gtk_EntryControl::pcs_adjustment_configure (){
        static UnitObj *unity = new UnitObj(" "," ");
        gchar *tmp = NULL;

	if (! (tmp = (gchar*)g_object_get_data( G_OBJECT (entry), "Adjustment_PCS_Name")))
		return;

	get_pcs_configuartion ();

	tmp = g_strconcat (N_("Configure"), 
			   " ",
			   (gchar*) g_object_get_data( G_OBJECT (entry), 
							 "Adjustment_PCS_Name"),
			   NULL);

	GtkWidget *dialog = gtk_dialog_new_with_buttons (tmp,
							 GTK_WINDOW (gtk_widget_get_root (entry)), // get underlying window as parent
                                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							 _("_OK"), GTK_RESPONSE_ACCEPT,
							 _("_Cancel"), GTK_RESPONSE_REJECT,
							 NULL);
	g_free (tmp);

        BuildParam bp;

        bp.set_error_text (N_("Value not allowed."));

        gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);

	tmp = g_strdup_printf (N_("Warning: know what you are doing here!" \
                                  "\nInfo: You may use the dconf-editor." \
                                  "\nD-Bus path is:"                    \
                                  "\n %s/%s [%d]"),
                               gsettings_path, gsettings_key, get_count () 
                               );
        bp.grid_add_label (tmp, NULL, 2); bp.new_line ();
	g_free (tmp);	
	bp.grid_add_ec ("Upper Limit", unit, &vMax, -EC_INF, EC_INF, "8g"); bp.new_line ();
	bp.grid_add_ec ("Upper Warn",  unit, &vMax_warn, -EC_INF, EC_INF, "8g"); bp.new_line ();
	bp.grid_add_ec ("Lower Warn",  unit, &vMin_warn, -EC_INF, EC_INF, "8g"); bp.new_line ();
	bp.grid_add_ec ("Lower Limit", unit, &vMin, -EC_INF, EC_INF, "8g"); bp.new_line ();

        bp.grid_add_widget (gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 2); bp.new_line ();
	bp.grid_add_ec ("Exclude Hi", unit, &v_ex_hi, -EC_INF, EC_INF, "8g"); bp.new_line ();
	bp.grid_add_ec ("Exclude Lo", unit, &v_ex_lo, -EC_INF, EC_INF, "8g"); bp.new_line ();

        if (GTK_IS_SPIN_BUTTON (entry)){
                bp.grid_add_widget (gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 2); bp.new_line ();
                bp.grid_add_ec ("Step [B1]", unit, &step, -EC_INF, EC_INF, "8g"); bp.new_line ();
                bp.grid_add_ec ("Page [B2]", unit, &page, -EC_INF, EC_INF, "8g"); bp.new_line ();
                bp.grid_add_ec ("Pg10 [B3]", unit, &page10, -EC_INF, EC_INF, "8g"); bp.new_line ();
                bp.grid_add_ec ("Progressive", unity, &progressive, 0., 10., "g"); bp.new_line ();
#if 0
                g_object_set_data  (G_OBJECT (dialog), "CB-LOG-SCALE",
                                    bp.grid_add_check_button ("Log-Scale", "use slider in log scale mode.\n WARING: EXPERIMENTAL",
                                                              1, NULL, NULL, (adj_mode & PARAM_CONTROL_ADJUSTMENT_LOG)?1:0
                                                              ));
                

                bp.new_line ();
                g_object_set_data  (G_OBJECT (dialog), "CB-LOG-SYM",
                                    bp.grid_add_check_button ("Log-Sym", "use slider in log scale mode with zero at center. Left: log, neg val.\n WARING: EXPERIMENTAL",
                                                              1, NULL, NULL, adj_mode & PARAM_CONTROL_ADJUSTMENT_LOG?1:0
                                                              ));
                bp.new_line ();
                g_object_set_data  (G_OBJECT (dialog), "CB-DUAL-RANGE",
                                    bp.grid_add_check_button ("Dual-Range", "use slider in log scale mode with zero at center. Left: log, neg val.\n WARING: EXPERIMENTAL",
                                                              1, NULL, NULL, adj_mode & PARAM_CONTROL_ADJUSTMENT_LOG?1:0
                                                              ));
                bp.new_line ();
                g_object_set_data  (G_OBJECT (dialog), "CB-TICKS",
                                    bp.grid_add_check_button ("Add-Ticks", "add tick marks w. snapping to slider",
                                                              1, NULL, NULL, adj_mode & PARAM_CONTROL_ADJUSTMENT_LOG?1:0
                                                              ));
#endif
                bp.new_line ();
        }

        // FIX-ME GTK4 show all
        gtk_widget_show (dialog);
        g_signal_connect (dialog, "response",
                          G_CALLBACK (Gtk_EntryControl::pcs_adjustment_configure_response_callback),
                          this);
}

#define XRM_GET_WD(L, V) tdv = g_strdup_printf ("%g", V); xrm.Get (L, &V, tdv); g_free (tdv)

void Gtk_EntryControl::get_pcs_configuartion (){
	XSM_DEBUG (DBG_L9, "GET-PCS-ADJ:\n");
        gchar *name = (gchar*) g_object_get_data( G_OBJECT (entry), "Adjustment_PCS_Name");

        if (name == NULL){
                XSM_DEBUG (DBG_L8, "GET-PCS-ADJ: return OK. No adjustment/remote requested.");
                return;
        }
        
	XSM_DEBUG (DBG_L8, "GET-PCS-ADJ: " << name << " count=" << get_count ());

        check_gsettings_path ();

        name = g_ascii_strdown (name, -1);
        // name = key_assure (name);
        
        gchar *dotpath =  g_strdup_printf ("%s", gsettings_adj_path);

	XSM_DEBUG (DBG_L8, "GET-PCS-ADJ: " << dotpath << "." << name);

        if (main_get_generate_pcs_adj_gschema () || pcs_adj_write_missing_schema_entry){

                std::ofstream logf; logf.open ("gxsm-pcs-writer.log",  std::ios::app);
                std::ofstream f;
                std::ifstream fi;
                
                gchar *path    = g_strdup_printf ("/%s/",
                                                  gsettings_adj_path_dir
                                                  );
                
                gchar *tmppathname = g_strdup_printf ("%s.gschema%sxml",
                                                      gsettings_adj_path,
                                                      pcs_adj_write_missing_schema_entry ? ".missing." : "."
                                                      );

                if (current_pcs_path)
                        g_free (current_pcs_path);

                current_pcs_path = tmppathname;

                //                                            x
                logf << "opening pcs-adj xml file new for:    " << current_pcs_path << "[" << name << "] count=" << get_count () << std::endl;
                                
                if (! g_file_test (tmppathname, G_FILE_TEST_EXISTS)){
                        f.open (current_pcs_path,  std::ios::out);
                        f << "<schemalist>" << std::endl;
                        f.close ();
                        pcs_adj_schema_writer_first_entry = false;
                }

                // -- identify end tag and truncate if exists for continuation
                //  </schema> 
                //</schemalist>
                fi.open (current_pcs_path, std::ios::in);
                fi.seekg (-28, std::ios_base::end);
                if (!fi.fail ()){
                        gchar x[1024]; x[0]=0;
                        fi.getline (x, 1024);
                        //                        logf << "pcs-end-read: [" << x << "]" << std::endl;
                        if (strcmp (x, "  </schema>") == 0){ // overwrite from here!
                                fi.seekg (-28, std::ios_base::end);
                                off_t pos = fi.tellg ();
                                fi.close ();
                                truncate(current_pcs_path, pos);
                                //      logf << "  ---> ov from eof-26 at " << pos << std::endl;
                        } else
                                fi.close ();
                } else
                        fi.close ();
                
                logf.close ();

                //======
                
                f.open (current_pcs_path, std::ios::app); // continue writing

                if (!generate_pcs_adjustment_gschema_path_add_prev)
                        generate_pcs_adjustment_gschema_path_add_prev = g_strdup("dummy");

                if (strcmp (pcs_get_current_gschema_group (), generate_pcs_adjustment_gschema_path_add_prev)){
                        g_free (generate_pcs_adjustment_gschema_path_add_prev);
                        generate_pcs_adjustment_gschema_path_add_prev = g_strdup (pcs_get_current_gschema_group ());

                        f << "  <schema id=\"" << dotpath
                          << "\" path=\"" << path << "\">" 
                          << std::endl;
                }

                

                XSM_DEBUG_GP (DBG_L1, "Generating PCS Adjustment Schema for [%s.%s] at %s\n", dotpath, name, current_pcs_path);

                f << std::endl
                  << "    <key name=\"" << name << "\" type=\"ad\">" << std::endl
                  << "      <default>["
                  << vMax << ", "
                  << vMin << ", "
                  << vMax_warn << ", "
                  << vMin_warn << ", "
                  << v_ex_hi << ", "
                  << v_ex_lo << ", "
                  << step << ", "
                  << page << ", "
                  << page10 << ", "
                  << progressive << ", "
                  << " 0, 0"
                  << "      ]"
                  << "      </default>"
                  << std::endl
                  << "      <summary>Array of [Adjustments Absolute Limits: Max, -Min,  Warning Bounds: Max, -Min,  Exclude Range: Lo, -Hi, Spin: Step, Page, Page10, Progressive-mode, 0,0] for '" << name << "' </summary>"
                  << std::endl
                  << "      <description>Configuration for GXSM Entry with ID '" << name << "'. Setup of Min/Max Range (warnign: not checked for validity), Warning Bonds, Exclude Range and Spin Step/Page behavior. </description>"
                  << std::endl
                  << "    </key>"  << std::endl;

                f << "  </schema>"  << std::endl << std::endl;
                f << "</schemalist>" << std::endl << std::endl;

                f.close ();
 
                g_free (path);
                
                pcs_adj_write_missing_schema_entry = false;

        } else {

                // verify schema presence to avoid forced program termination
                GSettingsSchema *gs_schema_source = g_settings_schema_source_lookup (g_settings_schema_source_get_default (), dotpath, FALSE);
                if (!gs_schema_source){
                        g_print ("PCS: g_settings_schema_source_lookup () retured NULL while checking for '%s' Adjustment safed/default value, no schema installed!\n"
                                 "Please run gxsm4 --write-gxsm-pcs-adj-gschema, and copy the schema file(s) to the proper folder(s) and run make install.\n"
                                 "Ignoring problem and continuing with build in default mode.\n"
                                 "==> FYI: Writing missing schema to '%s.gschema%sxml' now.\n",
                                 dotpath,
                                 gsettings_adj_path,
                                 ".missing."
                                 );
                        
                        pcs_adj_write_missing_schema_entry = true;

                        // auto retry/write schema to tmp file
                        get_pcs_configuartion ();

                } else {
                        gdouble *array;
                        gsize n_stores;
                        GSettings *tmp_settings = g_settings_new (dotpath);
                        GVariant *storage = g_settings_get_value (tmp_settings, name);
                        array = (gdouble*) g_variant_get_fixed_array (storage, &n_stores, sizeof (gdouble));
                        if (n_stores < 10)
                                g_print ("PCS GSETTINGS DATA ERROR: g_settings_get_fixed_array returned the wrong number %d of elements:\n"
                                         " ===> Need at a minimum of 10 double values for key %s.%s.\nConfiguration",
                                         (int)n_stores,
                                         dotpath,
                                         name);
                        else {
                                vMax = array[0];
                                vMin = array[1];
                                vMax_warn = array[2];
                                vMin_warn = array[3];
                                v_ex_lo = array[4];
                                v_ex_hi = array[5];
                                step = array[6];
                                page = array[7];
                                page10 = array[8];
                                progressive = array[9];
                        }  

                        g_clear_object (&tmp_settings);
                }
        }       
        g_free (name);
        g_free (dotpath);
}

#define XRM_PUT_WD(L, V) xrm.Put (L, V);

void Gtk_EntryControl::put_pcs_configuartion (){
	XSM_DEBUG(DBG_L2, "PUT-PCS-ADJ:\n");

        gchar *name = (gchar*) g_object_get_data( G_OBJECT (entry), "Adjustment_PCS_Name");

        if (name == NULL)
		return;
        
        if (main_get_generate_pcs_adj_gschema ())
                return;
     
        name = g_ascii_strdown (name, -1);

        // gchar *dotpath =  g_strdup_printf ("%s.%s", gsettings_adj_path, name);
        gchar *dotpath =  g_strdup_printf ("%s", gsettings_adj_path);

        XSM_DEBUG(DBG_L2, "PCS: storing adjustment configuration [" << dotpath << "." << name << "]\n");

        // verify schema presence to avoid forced program termination
        GSettingsSchema *gs_schema_source = g_settings_schema_source_lookup (g_settings_schema_source_get_default (), dotpath, FALSE);
        if (!gs_schema_source){
                g_print ("PCS: g_settings_schema_source_lookup () retured NULL while checking for '%s' Adjustment safed/default value, no schema installed!\n"
                         "Please update/add schema via gxsm4 --write-gxsm-pcs-adj-gschema, edit file termination tag and run a make install.\n"
                         "Ignoring problem and continuing -- new settings will not be remembered but take effect for this session.\n",
                         dotpath);
        }  else {
        
                gsize n_stores = 10;
                gdouble *array = g_new (gdouble, n_stores);

                array[0] = vMax;
                array[1] = vMin;
                array[2] = vMax_warn;
                array[3] = vMin_warn;
                array[4] = v_ex_lo;
                array[5] = v_ex_hi;
                array[6] = step;
                array[7] = page;
                array[8] = page10;
                array[9] = progressive;

                GSettings *tmp_settings = g_settings_new (dotpath);
                GVariant *storage = g_variant_new_fixed_array (g_variant_type_new ("d"), array, n_stores, sizeof (gdouble));
                g_settings_set_value (tmp_settings, name, storage);

                g_clear_object (&tmp_settings);
        }

        g_free (name);
        g_free (dotpath);

	if (GTK_IS_SPIN_BUTTON (entry)){
		gtk_spin_button_configure (GTK_SPIN_BUTTON (entry), 
                                           GTK_ADJUSTMENT (adj), progressive, step < 0.1 ? (int)(floor(-log(step))+1) : 0);	
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON (entry),
                                                step, page);
	}
	if (adj){
	        gtk_adjustment_set_upper (GTK_ADJUSTMENT (adj), vMax);
		gtk_adjustment_set_lower (GTK_ADJUSTMENT (adj), vMin);
	}
}


static gint
ec_gtk_spin_button_sci_output (GtkSpinButton *spin_button,
                              gpointer       user_data)
{
        if (!G_IS_OBJECT(spin_button) ) return FALSE;
        Gtk_EntryControl *ec = (Gtk_EntryControl *)g_object_get_data( G_OBJECT (spin_button), "Gtk_EntryControl");
        if (!ec) return FALSE;
        GtkAdjustment *adjustment = gtk_spin_button_get_adjustment (spin_button);
        double value = gtk_adjustment_get_value (adjustment);
        gchar *buf = ec->Get_UsrString();
        if (strcmp (buf, gtk_editable_get_text (GTK_EDITABLE (spin_button))))
                gtk_editable_set_text (GTK_EDITABLE (spin_button), buf);
#ifdef DEBUG_PCS_LOG
        g_message ("spin_sci output.. : %g --> %s", value, buf);
#endif
        g_free (buf);
        return TRUE;
}

static gint
ec_gtk_spin_button_sci_input (GtkSpinButton *spin_button,
                              gdouble       *new_val,
                              gpointer       user_data)
{
        if (!G_IS_OBJECT(spin_button) ) return FALSE;
        Gtk_EntryControl *ec = (Gtk_EntryControl *)g_object_get_data( G_OBJECT (spin_button), "Gtk_EntryControl");
        if (!ec) return FALSE;
        const char *text;
        gchar *err = NULL;
        gdouble input_value = g_strtod (text=gtk_editable_get_text (GTK_EDITABLE (spin_button)), &err);
        *new_val = (gdouble)ec->Convert2Base (input_value);
#ifdef DEBUG_PCS_LOG
        g_message ("spin_sci input.. : %s -> %g --> %g", text, input_value, *new_val);
#endif
        if (*err)
                return GTK_INPUT_ERROR;
	else
                return TRUE;
}

static void 
ec_pcs_adjustment_configure (GtkWidget *menuitem, Gtk_EntryControl *gpcs){
	gpcs->pcs_adjustment_configure ();
}

void Gtk_EntryControl::InitRegisterCb(double AdjStep, double AdjPage, double AdjProg){
        af_update_handler_id[0] = af_update_handler_id[1] = 0;
        ec_io_handler_id[0] = ec_io_handler_id[1] = 0;
        adjcb_handler_id = 0;
	adj=NULL;
	enable_client = TRUE;
        
	page10 = 10.*AdjPage;
	page   = AdjPage;
	step   = AdjStep;
	progressive = AdjProg;

        XSM_DEBUG (DBG_L8, "InitRegisterCb -- enter");
        // only for head EC if EC is part of an iter/array
        if (get_count () <= 1){
                get_pcs_configuartion ();
	}
        XSM_DEBUG (DBG_L8, "InitRegisterCb -- connect...");
        
	g_object_set_data( G_OBJECT (entry), "Gtk_EntryControl", this);
      
        XSM_DEBUG (DBG_L8, "InitRegisterCb -- AdjSetup?");
	if(fabs (AdjStep) > 1e-22 && get_count () <= 1){ // only master
                XSM_DEBUG (DBG_L8, "InitRegisterCb -- hookup config menuitem");

                // FIX-ME-GTK4 -- TESTING
                gchar *cfg_label = g_strconcat ("Configure",
                                                " ",
                                                g_object_get_data( G_OBJECT (entry), 
                                                                   "Adjustment_PCS_Name"),
                                                NULL);
                GMenu *menu = g_menu_new ();
                GMenuItem *menu_item_config = g_menu_item_new (cfg_label, NULL);
                // *** HOOK CONFIGURE MENUITEM TO ENTRY *** -- issue w gtk4, how???
                //g_signal_connect (menu_item_config, "activate", G_CALLBACK (ec_pcs_adjustment_configure), this);
                g_menu_append_item (menu, menu_item_config);
		if (GTK_IS_SPIN_BUTTON (entry)){
                        ;// on_set_extra_menu (GTK_ENTRY (entry), G_MENU_MODEL (menu)); // need equivalent function for spin button!!
                } else
                        gtk_entry_set_extra_menu (GTK_ENTRY (entry), G_MENU_MODEL (menu));
                g_object_unref (menu_item_config);
                // TESTING
                
                adj = gtk_adjustment_new( Get_dValue (), vMin, vMax, step, page, 0);

                adjcb_handler_id = g_signal_connect (G_OBJECT (adj), "value_changed",
                                                     G_CALLBACK (Gtk_EntryControl::adjustment_callback), this);
		if (GTK_IS_SPIN_BUTTON (entry)){
			ec_io_handler_id[0] = g_signal_connect (G_OBJECT (entry), "output",
                                                                G_CALLBACK (&ec_gtk_spin_button_sci_output),
                                                                (gpointer) NULL);
			ec_io_handler_id[1] = g_signal_connect (G_OBJECT (entry), "input",
                                                                G_CALLBACK (&ec_gtk_spin_button_sci_input),
                                                                (gpointer) NULL);
                        gtk_spin_button_configure (GTK_SPIN_BUTTON (entry), 
                                                   GTK_ADJUSTMENT (adj), progressive, step < 1.0 ? (int)(floor(-log(step))+1) : 1);
		}
	}
        
        if (GTK_IS_ENTRY (entry)){
                af_update_handler_id[0] = g_signal_connect (G_OBJECT (entry), "activate",
                                                            G_CALLBACK (&Gtk_EntryControl::update_callback),
                                                            (gpointer) NULL);
                GtkEventController *focus = gtk_event_controller_focus_new ();
                af_update_handler_id[1] = g_signal_connect (focus, "leave", G_CALLBACK (&Gtk_EntryControl::entry_focus_leave_callback), entry);
                gtk_widget_add_controller (entry, GTK_EVENT_CONTROLLER (focus));
        }
        
        XSM_DEBUG (DBG_L8, "InitRegisterCb -- put value");
	Put_Value();

        XSM_DEBUG (DBG_L8, "InitRegisterCb -- done.");
}


GSList *Gtk_EntryControl::AddEntry2RemoteList(const gchar *RefName, GSList *remotelist){
	if(refname) 
		g_free (refname);
	refname = g_strdup(RefName);
	
	gtk_widget_set_tooltip_text (entry, RefName);

        if (pc_count == 0){
                if (main_get_generate_pcs_gschema ())
                        write_pcs_gschema ();
                else
                        get_init_value_from_settings ();
        }

	return g_slist_prepend(remotelist, this);
}

