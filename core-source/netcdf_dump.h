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

//
// 20251025PY: major NC4 upgrade
//

#include <stdlib.h>
#include <locale.h>
#include <libintl.h>

#include "unit.h"

#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm> // For std::copy
#include <iterator>  // For std::ostream_iterator
using namespace std;

#include <netcdf>
using namespace netCDF;



#define SETUP_ENTRY(VAR, TXT)			\
	do {\
                gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (VAR))), TXT, -1); \
		gtk_editable_set_editable (GTK_EDITABLE (VAR), FALSE); \
		gtk_widget_set_sensitive (VAR, TRUE); \
		gtk_widget_show (VAR); \
	}while(0)


#define SETUP_LABEL(LAB) \
	do {\
                gtk_label_set_xalign (GTK_LABEL (LAB), 1.0); \
		gtk_widget_show (LAB); \
	}while(0)



class NcDumpToWidget : public NcFile{
public:
	NcDumpToWidget (std::string filename, NcFile::FileMode mode = netCDF::NcFile::read) 
		: NcFile (filename, mode) { 
		maxvals = 10; 
	} ;
	~NcDumpToWidget (){ };
	static void show_info_callback (GtkWidget *widget, gchar *message){
                GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
                GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_root (widget)), // parent window
                                                            flags,
                                                            GTK_MESSAGE_INFO,
                                                            GTK_BUTTONS_CLOSE,
                                                            "%s",
                                                            message
                                                            );
                g_signal_connect (dialog, "response", G_CALLBACK (gtk_window_destroy), NULL);
                gtk_widget_show (dialog);
	};
	void cleanup (GtkWidget *box){
                XSM_DEBUG_GM (DBG_L1, "NcDumpToWidge::cleanup **");
                if (!box) return;
		g_slist_foreach (
                                 (GSList*) g_object_get_data (
                                                              G_OBJECT (box), "info_list" ), 
                                 (GFunc) free_info_elem, 
                                 NULL
                                 );
                // FIX-ME-GTK4 or OK ???
                XSM_DEBUG_GM (DBG_L1, "NcDumpToWidge::cleanup FIX-ME-GTK4 >>detach grid, wipe contents<<");
#if 0
		g_list_foreach (
                                gtk_container_get_children ( GTK_CONTAINER (box)),
                                (GFunc) gtk_widget_destroy, 
                                NULL
			);
#endif
	};
	static void varshow_toggel_cb (GtkWidget *widget, gchar *varname){
                g_warning ("varshow_toggel_cb -- missing port.");
		// XsmRescourceManager xrm("App_View_NCraw");
                // gsettings!!!
                // ==> <key name="view-nc-raw" type="b">
		// xrm.PutBool (varname, gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON (widget)));
	};
	void setup_toggle (GtkWidget *tog, gchar *varname){
                // gsettings!!!
                // ==> <key name="view-nc-raw" type="b">
                // GTK3QQQ GSettings!!!!!!!! Tuple List ???
                // XsmRescourceManager xrm("App_View_NCraw");
                //gtk_check_button_set_active (GTK_CHECK_BUTTON (tog), xrm.GetBool (varname, FALSE) ? 1:0);

                // to be ported
		//g_signal_connect (tog, "toggled", G_CALLBACK(NcDumpToWidget::varshow_toggel_cb), g_strdup(varname));
                GtkLabel *l=GTK_LABEL (gtk_widget_get_last_child (tog));
                if (l){
                        //XSM_DEBUG_GM (DBG_L1, "NcDumpToWidge::setup_toggle... %s", varname);
                        gtk_label_set_ellipsize (l, PANGO_ELLIPSIZE_START); // or.._END
                        gtk_label_set_width_chars (l, 20);
                        gtk_label_set_xalign (l, 1.0);
                        
                        gtk_widget_set_tooltip_text (tog, varname);
                }
		gtk_widget_show (tog);
	};

        gchar *get_gatt_as_string (netCDF::NcGroupAtt &att){
                if (att.isNull()) {
                        std::cerr << "Error: Attribute '" << att.getName() << "' not found." << std::endl;
                        return NULL;
                }
        
                // Check if the attribute type is a string
                netCDF::NcType att_type = att.getType();
                if (att_type != netCDF::NcType::nc_CHAR && att_type != netCDF::NcType::nc_STRING) {
                        std::cerr << "Error: Attribute is not a string type." << std::endl;
                        return NULL;
                }
        
                // Get the length of the attribute value
                size_t len = att.getAttLength();
        
                // Read the attribute value into a character buffer
                if (att_type == netCDF::NcType::nc_CHAR) {
                        char* att_value_c_str = new char[len + 1]; // +1 for null terminator
                        att.getValues(att_value_c_str);
                        att_value_c_str[len] = '\0'; // Ensure null-termination
                        gchar *ret = g_strdup (att_value_c_str);
                        delete[] att_value_c_str;
                        //std::cout << "*** nc_CHAR Attribute '" << att_name << "' value: " << ret << " ***" << std::endl;
                        return ret;
                } else if (att_type == netCDF::NcType::nc_STRING) {
                        // For NC_STRING, getValues returns an array of char*
                        char** str_array_c = new char*[len];
                        att.getValues(str_array_c);
            
                        // Convert to a vector of std::string
                        std::vector<std::string> str_vector;
                        for (size_t i = 0; i < len; ++i) {
                                str_vector.push_back(std::string(str_array_c[i]));
                        }
            
                        // Print the values
                        gchar *att_as_string=g_strdup("");
                        //std::cout << "*** string Attribute '" << att_name << "' value: ";
                        for (const auto& s : str_vector) {
                                //std::cout << "  " << s << std::endl;
                                gchar *tmp = g_strconcat (att_as_string, s.data(), NULL);
                                g_free (att_as_string);
                                att_as_string = tmp;
                        }
                        //std::cout << " ***" << std::endl;
                        return (att_as_string);
                }
        };
        gchar *get_att_as_string (netCDF::NcVarAtt &att){
                if (att.isNull()) {
                        std::cerr << "Error: Attribute '" << att.getName() << "' not found." << std::endl;
                        return NULL;
                }
        
                // Check if the attribute type is a string
                netCDF::NcType att_type = att.getType();
                if (att_type != netCDF::NcType::nc_CHAR && att_type != netCDF::NcType::nc_STRING) {
                        std::cerr << "Error: Attribute is not a string type." << std::endl;
                        return NULL;
                }
        
                // Get the length of the attribute value
                size_t len = att.getAttLength();
        
                // Read the attribute value into a character buffer
                if (att_type == netCDF::NcType::nc_CHAR) {
                        char* att_value_c_str = new char[len + 1]; // +1 for null terminator
                        att.getValues(att_value_c_str);
                        att_value_c_str[len] = '\0'; // Ensure null-termination
                        gchar *ret = g_strdup (att_value_c_str);
                        delete[] att_value_c_str;
                        //std::cout << "*** nc_CHAR Attribute '" << att_name << "' value: " << ret << " ***" << std::endl;
                        return ret;
                } else if (att_type == netCDF::NcType::nc_STRING) {
                        // For NC_STRING, getValues returns an array of char*
                        char** str_array_c = new char*[len];
                        att.getValues(str_array_c);
            
                        // Convert to a vector of std::string
                        std::vector<std::string> str_vector;
                        for (size_t i = 0; i < len; ++i) {
                                str_vector.push_back(std::string(str_array_c[i]));
                        }
            
                        // Print the values
                        gchar *att_as_string=g_strdup("");
                        //std::cout << "*** string Attribute '" << att_name << "' value: ";
                        for (const auto& s : str_vector) {
                                //std::cout << "  " << s << std::endl;
                                gchar *tmp = g_strconcat (att_as_string, s.data(), NULL);
                                g_free (att_as_string);
                                att_as_string = tmp;
                        }
                        //std::cout << " ***" << std::endl;
                        return (att_as_string);
                }
        };


        template <typename T>
        void readVarWriteToStream(const NcVar& var, ostringstream &s) {
                size_t varSize = 1;

                g_message ("PRINT %s", var.getName().data());
                
                if (var.getDims().size() == 0){
                        T x;
                        var.getVar(&x);

                        try {
                                NcVarAtt a = var.getAtt("unit");
                                gchar *unit = get_att_as_string (a);
                                gchar *value_str=NULL;
                                if (unit){
                                        NcVarAtt a = var.getAtt("label");
                                        gchar *label = get_att_as_string (a);
                                        if (label){
                                                UnitObj *u = main_get_gapp ()->xsm->MakeUnit (unit, label);
                                                gchar *tmp = u->UsrString (x);
                                                s << tmp;
                                                g_free (tmp);
                                                delete unit;
                                                delete label;
                                                delete u;
                                                return;
                                        }
                                        delete unit;
                                }
                        } catch (const netCDF::exceptions::NcException& e) {;}
                        try {
                                NcVarAtt a = var.getAtt("var_unit");
                                gchar *unit = get_att_as_string (a);
                                gchar *value_str=NULL;
                                if (unit){
                                        UnitObj *u = main_get_gapp ()->xsm->MakeUnit (unit, "var.getName().data()");
                                        gchar *tmp = u->UsrString (x);
                                        s << tmp;
                                        g_free (tmp);
                                        delete unit;
                                        delete u;
                                        return;
                                }
                        } catch (const netCDF::exceptions::NcException& e) {;}
                        s << x;
                        return;
                }

                s << "S";
                for (int d=0; d<var.getDims().size(); ++d){
                        varSize *= var.getDims()[d].getSize();
                        s << "[" << var.getDims()[d].getSize() << "]";
                        std::cout << "[" << var.getDims()[d].getSize() << "]";
                        //g_message ("%s[%d]: %d => %d", var.getName().data(), d, var.getDims()[d].getSize(), varSize);
                }

                gboolean elips=false;
                // limit for preview
                if (varSize > 100){
                        varSize = 100;
                        elips=true;
                }

                s << ":=";
                
                std::vector<size_t> startp(var.getDims().size());
                std::vector<size_t> countp(var.getDims().size());
                
                for (int d=0; d<var.getDims().size(); ++d){
                        startp[d] = 0;
                        countp[d]  = 1;
                        s << '[';
                }
                int d=var.getDims().size()-1;
                for (size_t count=0; count < varSize && d >= 0; ){
                        size_t dim_len = var.getDims()[d].getSize();
                        size_t len = varSize <= dim_len ? varSize : dim_len;
                        std::vector<T> data(len);
                        countp[d] = len;
                        count += len;
                        std::cout << "### S=[";
                        std::copy(startp.begin(), startp.end(), std::ostream_iterator<double>(std::cout, " "));
                        std::cout << "], C=[";
                        std::copy(countp.begin(), countp.end(), std::ostream_iterator<double>(std::cout, " "));
                        std::cout << "]###" << std::endl;
                        var.getVar( startp, countp, data.data ());
                        for (size_t i = 0; i < len; ++i){
                                s << data[i] << ", ";
                                std::cout << data[i] << ", ";
                        }
                        startp[d]+=len;
                        if (startp[d] >= var.getDims()[d].getSize() && d >= 1){
                                startp[d] = 0;
                                startp[d-1]++;
                                s << "],[";
                                if (startp[d-1] >= var.getDims()[d-1].getSize() && d >= 2){
                                        startp[d-1]=0;
                                        startp[d-2]++;
                                        s << "[";
                                        if (startp[d-2] >= var.getDims()[d-2].getSize() && d >= 3){
                                                startp[d-2]=0;
                                                startp[d-3]++;
                                                s << "[";
                                                if (startp[d-3] >= var.getDims()[d-3].getSize()){
                                                        s << "E ..";
                                                        break;
                                                }
                                        }
                                }
                        }
                }

                if (elips) s << " ...]";
                else s << "]";
        };

        // Overload for char type
        //template <char>
        void readVarWriteToStreamC (const NcVar& var, ostringstream &s) {
                size_t varSize = 1;

                g_message ("PRINT <CHAR> %s", var.getName().data());
                
                if (var.getDims().size() == 0){
                        char x;
                        var.getVar(&x);
                        s << x;
                        return;
                }

                for (int d=0; d<var.getDims().size(); ++d){
                        varSize *= var.getDims()[d].getSize();
                        //s << "[" << var.getDims()[d].getSize() << "]";
                        //std::cout << "[" << var.getDims()[d].getSize() << "]";
                        //g_message ("%s[%d]: %d => %d", var.getName().data(), d, var.getDims()[d].getSize(), varSize);
                }

                
                gboolean elips=false;
                // limit for preview
                if (varSize > 100){
                        varSize = 100;
                        elips=true;
                }

                std::vector<size_t> startp(var.getDims().size());
                std::vector<size_t> countp(var.getDims().size());
                for (int d=0; d<var.getDims().size(); ++d){
                        startp[d] = 0;
                        countp[d]  = 1;
                }
                int d=var.getDims().size()-1;
                for (size_t count=0; count < varSize && d >= 0; ){
                        size_t dim_len = var.getDims()[d].getSize();
                        size_t len = varSize <= dim_len ? varSize : dim_len;
                        gchar data[len+1];
                        countp[d] = len;
                        count += len;
                        //std::cout << "### S=[";
                        //std::copy(startp.begin(), startp.end(), std::ostream_iterator<double>(std::cout, " "));
                        //std::cout << "], C=[";
                        //std::copy(countp.begin(), countp.end(), std::ostream_iterator<double>(std::cout, " "));
                        //std::cout << "]###" << std::endl;
                        var.getVar( startp, countp, data);
                        data[len]=0;
                        std::cout << data;
                        s << data;
                        startp[d]+=len;
                        if (startp[d] >= var.getDims()[d].getSize() && d > 1){
                                startp[d] = 0;
                                startp[d-1]++;
                                if (startp[d-1] >= var.getDims()[d-1].getSize() && d > 2){
                                        startp[d-1]=0;
                                        startp[d-2]++;
                                        if (startp[d-2] >= var.getDims()[d-2].getSize() && d > 3){
                                                startp[d-2]=0;
                                                startp[d-3]++;
                                                if (startp[d-3] >= var.getDims()[d-3].getSize()){
                                                        s << "...";
                                                        break;
                                                }
                                        } else break;
                                        s << ", ";
                                } else break;
                                s << ", ";
                        } else break;
                }

        };

	static void free_info_elem(gpointer txt, gpointer data){ g_free((gchar*) txt); };

        // General NC Formatting Dumpingutil, Output into GTK-Window
        void dump ( GtkWidget *box, GtkWidget *box_selected ){
                GSList *infolist=NULL;
                GtkWidget *sep;
                GtkWidget *lab;
                GtkWidget *grid, *grid_selected;
                GtkWidget *VarName, *VarName_i;
                GtkWidget *variable, *variable_i;
                GtkWidget *info;
                int grid_row=0;
                int grid_row_s=0;
        
                XSM_DEBUG_GM (DBG_L3, "NcDumpToWidge::dump **");
                // cleanup old contents if exists
                cleanup (box_selected);
                cleanup (box);

                XSM_DEBUG_GM (DBG_L3, "NcDumpToWidge::dump ** new grid...");
                grid = gtk_grid_new ();
                gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (box), grid);
                gtk_widget_show (box);
                gtk_widget_show (grid);

                grid_selected = gtk_grid_new ();
                gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (box_selected), grid_selected);
                gtk_widget_show (grid_selected);
                gtk_widget_show (box_selected);

                gtk_grid_attach (GTK_GRID (grid), lab=gtk_label_new("Selected NetCDF values"), 0, grid_row++, 10, 1);
                gtk_grid_attach (GTK_GRID (grid), sep=gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 0, grid_row++, 10, 1);
                gtk_widget_show (lab);
                gtk_widget_show (sep);
    
                // ===============================================================================
                // DUMP:  global attributes
                // ===============================================================================
                gtk_grid_attach (GTK_GRID (grid), lab=gtk_label_new("Global Attributes"), 0, grid_row++, 10, 1);
                gtk_grid_attach (GTK_GRID (grid), sep=gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 0, grid_row++, 10, 1);
                gtk_widget_show (lab);
                gtk_widget_show (sep);

                g_message ("NC DUMP: ATTS");
                
                //netCDF::NcGroup rootGroup = getGroup("/");
                //multimap< std::string, NcGroupAtt > attributes = rootGroup.getAtts ();

                multimap< std::string, NcGroupAtt > attributes = getAtts ();
                for (auto const& [name, att] : attributes) {
                        gchar *tmp;
                        VarName = gtk_label_new (name.data());
                        SETUP_LABEL (VarName);
                        gtk_grid_attach (GTK_GRID (grid), VarName, 0, grid_row, 2, 1);
                        gtk_widget_show (VarName);

                        variable = gtk_entry_new ();
                        //NcGroupAtt att = pair.second;

                        NcGroupAtt ga = att;
                        tmp=get_gatt_as_string (ga);
                        if (tmp) SETUP_ENTRY (variable, tmp);
                        else SETUP_ENTRY (variable, "*NULL*");
                        g_free (tmp);
                        gtk_grid_attach (GTK_GRID (grid), variable, 2, grid_row++, 1, 1);
                        gtk_widget_show (variable);
                }

                //	static gchar *types[] = {"","byte","char","short","long","float","double"};

                // ===============================================================================

                gtk_grid_attach (GTK_GRID (grid), sep=gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 0, grid_row++, 10, 1);
                gtk_widget_show (sep);
                gtk_grid_attach (GTK_GRID (grid), sep=gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 0, grid_row++, 10, 1);
                gtk_widget_show (sep);

                // ===============================================================================
                // DUMP:  dimension value
                // ===============================================================================
                g_message ("NC DUMP: DIMS");
  
                gtk_grid_attach (GTK_GRID (grid), lab=gtk_label_new("NC Dimensions"), 0, grid_row++, 10, 1);
                gtk_widget_show (lab);
                gtk_grid_attach (GTK_GRID (grid), sep=gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 0, grid_row++, 10, 1);
                gtk_widget_show (sep);

                std::multimap<std::string, netCDF::NcDim> dims = getDims();
                for (auto const& [name, dim] : dims) {
                        VarName = gtk_check_button_new_with_label (name.data());
                        setup_toggle (VarName, name.data());
                        gtk_grid_attach (GTK_GRID (grid), VarName, 0, grid_row, 2, 1);
                        gtk_widget_show (VarName);

                        variable = gtk_entry_new ();
                        gchar *dimval=NULL;
                        if (dim.isUnlimited())
                                dimval = g_strdup_printf("UNLIMITED, %d currently", (int)dim.getSize ());
                        else
                                dimval = g_strdup_printf("%d", (int)dim.getSize ());
    
                        SETUP_ENTRY(variable, dimval);
                        gtk_grid_attach (GTK_GRID (grid), variable, 2, grid_row++, 1, 1);
                        gtk_widget_show (variable);

                        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (VarName))){
                                VarName_i = gtk_label_new (name.data());
                                SETUP_LABEL(VarName_i);
                                variable_i = gtk_entry_new ();
                                SETUP_ENTRY(variable_i, dimval);

                                gtk_grid_attach (GTK_GRID (grid_selected), VarName_i, 0, grid_row_s, 2, 1);
                                gtk_widget_show (VarName_i);
                                gtk_grid_attach (GTK_GRID (grid_selected), variable_i, 2, grid_row_s++, 1, 1);
                                gtk_widget_show (variable_i);
                        }
                }
        
                // ===============================================================================

                gtk_grid_attach (GTK_GRID (grid), sep=gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 0, grid_row++, 10, 1);
                gtk_widget_show (sep);
                gtk_grid_attach (GTK_GRID (grid), sep=gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 0, grid_row++, 10, 1);
                gtk_widget_show (sep);

                // ===============================================================================
                // DUMP:  vartype varname(dims)   data
                // ===============================================================================

                gtk_grid_attach (GTK_GRID (grid), lab=gtk_label_new("NC Data, Variables"), 0, grid_row++, 10, 1);
                gtk_widget_show (lab);
                gtk_grid_attach (GTK_GRID (grid), sep=gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 0, grid_row++, 10, 1);
                gtk_widget_show (sep);
        
                // Define a multimap to hold the variable names and objects
                std::multimap<std::string, NcVar> allVariables;

                g_message ("NC DUMP: VARS");
                
                std::multimap<std::string, NcVar> vars = getVars(NcGroup::Current);
                for (auto const& pair : vars){
                        NcVar var = pair.second;
                        NcType type = var.getType();

                        std::cout << "VAR: " << pair.first << std::endl;

                        int unlimited_flag = FALSE;
                        gchar *vdims = g_strdup(" ");

                        if (var.getDims().size() > 0) {
                                //g_message ("VAR has dimensions > 0: #dims = %d", var.getDims().size());
                                std::vector<netCDF::NcDim> dims = var.getDims();
                                //std::cout << " Dims:" << std::endl;
                                g_free(vdims);
                                vdims = g_strdup("(");
                                for (const auto& dim : dims) {
                                        gchar *tmp = g_strconcat(vdims, (gchar*)dim.getName().data(), ", ", NULL);
                                        if (dim.isUnlimited())
                                                unlimited_flag = TRUE;
                                        g_free(vdims);
                                        vdims = g_strdup(tmp);
                                        g_free(tmp);
                                }
                                vdims[strlen(vdims)-2]=')';
                        }
                        gchar *vardef = g_strconcat((gchar*)var.getName().data(), vdims, NULL);
                        g_free(vdims);

                        XSM_DEBUG_GM (DBG_L3, "NcDumpToWidge::dump ** variable: %s", vardef);

                        VarName = gtk_check_button_new_with_label (vardef);
                        setup_toggle (VarName, (gchar*)var.getName().data());

                        std::cout << "VarDef: " << vardef << std::endl;

                        g_free(vardef);

                        gtk_grid_attach (GTK_GRID (grid), VarName, 0, grid_row, 2, 1);
                        gtk_widget_show (VarName);

                        variable = gtk_entry_new ();
                        ostringstream ostr_val;

                        
                        std::cout << type.getName() << std::endl;
                        //ostr_val << "<" << type.getName() << "> ";
                                
                        if (unlimited_flag)
                                ostr_val << "** Unlimited Dim **";

                        if (type == ncChar) {
                                readVarWriteToStreamC(var, ostr_val);
                        } else if (type == ncByte || type == ncUbyte) {
                                readVarWriteToStream<signed char>(var, ostr_val);
                        } else if (type == ncShort || type == ncUshort) {
                                readVarWriteToStream<short>(var, ostr_val);
                        } else if (type == ncInt || type == ncUint) {
                                readVarWriteToStream<int>(var, ostr_val);
                        } else if (type == ncInt64 || type == ncUint64) {
                                readVarWriteToStream<long long>(var, ostr_val);
                        } else if (type == ncFloat) {
                                readVarWriteToStream<float>(var, ostr_val);
                        } else if (type == ncDouble) {
                                readVarWriteToStream<double>(var, ostr_val);
                        } else if (type == ncString) {
                                ostr_val << "** NEW STRING TYPE * work in progress **";
                                // readVarWriteToStream<std::string>(var, ostr_val);
                        } else {
                                std::cerr << "Warning: Skipping variable '" << var.getName() << "' with unknown type." << std::endl;
                                ostr_val << "** work in progress **";
                        }
                        SETUP_ENTRY(variable, (const gchar*)ostr_val.str().c_str());
                        
                        gtk_grid_attach (GTK_GRID (grid), variable, 2, grid_row, 1, 1);
                        gtk_widget_show (variable);

                        std::cout << "pre varAttributes info dlg data" << std::endl;
                        
                        //multimap<string, NcAtt> varAttributes = var.getAtts();
                        
                        ostringstream  ostr_att;
                        ostr_att << "Variable '" << var.getName() << "'" << endl
                                 << "  Var. Type <" << type.getName() << "> " << endl << endl
                                 << "--- Attributes List ---" << endl;
                        map< std::string, NcVarAtt > attributes = var.getAtts ();
                        //multimap< std::string, NcAtt > attributes = var.getAtts ();
                        for (auto const& [name, att] : attributes){
                                NcVarAtt a = att;
                                ostr_att << name << "\t\t: " << get_att_as_string (a) << endl;
                        }
                        
                        ostr_att << endl
                                 << " --- Data ---" << endl
                                 << " Value(s): "
                                 << ostr_val.str().c_str()
                                 << ends;
                        
                        gchar *infotxt = g_strdup( (const gchar*)ostr_att.str().c_str() );
                        infolist = g_slist_prepend( infolist, infotxt);
                        info = gtk_button_new_with_label (" Details ");
                        
                        gtk_grid_attach (GTK_GRID (grid), info, 3, grid_row++, 1, 1);
                        gtk_widget_show (info);

                        g_signal_connect (G_OBJECT (info), "clicked",
                                          G_CALLBACK (show_info_callback),
                                          infotxt);

                        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (VarName))){
                                NcVarAtt a = var.getAtt ("short_name");
                                gchar *tmp = get_att_as_string (a);
                                if (tmp) VarName_i = gtk_label_new (tmp);
                                else	 VarName_i = gtk_label_new (var.getName().data());
                                delete tmp;
                                SETUP_LABEL(VarName_i);

                                gchar *unit = NULL;
                                gchar *value_str = NULL;
                                try {
                                        a = var.getAtt ("unit");
                                        unit = get_att_as_string (a);
                                        if (unit){
                                                NcVarAtt a = var.getAtt ("label");
                                                gchar *label = get_att_as_string (a);
                                                if (label){
                                                        UnitObj *u = main_get_gapp ()->xsm->MakeUnit (unit, label);
                                                        double tmp;
                                                        var.getVar (&tmp);
                                                        value_str = u->UsrString (tmp);
                                                        delete unit;
                                                        delete label;
                                                        delete u;
                                                }
                                                delete unit;
                                        }
                                } catch (const netCDF::exceptions::NcException& e) { if (unit) delete unit; }

                                variable_i = gtk_entry_new ();

                                if (value_str){
                                        SETUP_ENTRY(variable_i, value_str);
                                        g_free (value_str);
                                }else{
                                        try {
                                                gchar *tmp=NULL;
                                                NcVarAtt a = var.getAtt ("var_unit");
                                                tmp = get_att_as_string (a);
                                                if (tmp) ostr_val << " [" << tmp << "]"; // << " [vu]";
                                                g_free (tmp);
                                                a = var.getAtt("unit");
                                                tmp = get_att_as_string (a);
                                                if (tmp) ostr_val << " [" << tmp << "]"; // << " [u]";
                                                g_free (tmp);
                                        } catch (const netCDF::exceptions::NcException& e) {;}

                                        SETUP_ENTRY(variable_i, (const gchar*)ostr_val.str().c_str());
                                }
                                gtk_grid_attach (GTK_GRID (grid_selected), VarName_i, 0, grid_row_s, 2, 1);
                                gtk_widget_show (VarName_i);
                                gtk_grid_attach (GTK_GRID (grid_selected), variable_i, 2, grid_row_s++, 1, 1);
                                gtk_widget_show (variable_i);
                        }
                }
                g_object_set_data (G_OBJECT (box), "info_list", infolist);
                
                gtk_widget_show (grid); // FIX-ME GTK4 SHOWALL
                gtk_widget_show (grid_selected); // FIX-ME GTK4 SHOWALL
        };
        
        int maxvals;
};
