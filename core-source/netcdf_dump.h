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


#include <stdlib.h>
#include <locale.h>
#include <libintl.h>

#include "unit.h"

#include <sstream>
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

                g_message ("** %s **", var.getName().data());
                for (int d=0; d<var.getDims().size(); ++d){
                        varSize *= var.getDims()[d].getSize();
                        g_message ("%s[%d]: %d => %d", var.getName().data(), d, var.getDims()[d].getSize(), varSize);
                }
                gboolean elips=false;
                // limit for preview
                if (varSize > 10){
                        varSize = 10;
                        elips=true;
                }

                std::vector<T> data(varSize);

                switch (var.getDims().size()){
                case 1: var.getVar( {0}, {varSize}, data.data ()); break;
                case 2: var.getVar( {0,0}, {0,varSize}, data.data ()); break;
                case 3: var.getVar( {0,0,0}, {0,0,varSize}, data.data ()); break;
                case 4: var.getVar( {0,0,0,0}, {0,0,0,varSize}, data.data ()); break;
                }
                //s << var.getName() << ": ";
                
                for (size_t i = 0; i < varSize; ++i)
                        s << data[i] << ", ";

                if (elips) s << " ...";
        };

#if 0
        // Overload for string type
        template <>
        void readVarWriteToStream<std::string>(const NcVar& var, ostringstream &s) {
                std::vector<std::string> data = var.asString("");
                //s << var.getName() << ": ";
                for (const auto& strdata : data)
                        s << strdata;
        };
#endif


#if 0
        void findVariables(const NcGroup& group, std::multimap<std::string, NcVar>& variableMap) {
                // Get all variables in the current group

                std::multimap<std::string, NcVar> vars = group.getVars(NcGroup::Current);
                for (auto const& pair : vars)
                        variableMap.insert(pair);

                // Get all subgroups
                std::multimap<std::string, NcGroup> subgroups = group.getGroups(NcGroup::ChildrenGrps);

                // Recursively call for each subgroup
                for (auto const& subgroup_pair : subgroups) {
                        findVariables(subgroup_pair.second, variableMap);
                }
        };

        // A generic function to read and print data from a variable.
        // Because the data type is not known at compile time, we use a template and a pointer.
        template <typename T>
        void readAndStoreVariable(const NcVar& var, std::multimap<std::string, std::string>& dataMap) {
                // Get the size of the variable.
                std::vector<size_t> varShape = var.getShape();
                size_t varSize = 1;
                for (size_t dimSize : varShape) {
                        varSize *= dimSize;
                }

                // Read the data into a buffer.
                std::vector<T> data(varSize);
                var.getVar(data.data());

                // Store and print the variable name and all its values.
                for (size_t i = 0; i < varSize; ++i) {
                        dataMap.insert({var.getName(), std::to_string(data[i])});
                }
        }

        // Overload for string type
        template <>
        void readAndStoreVariable<std::string>(const NcVar& var, std::multimap<std::string, std::string>& dataMap) {
                std::vector<std::string> data = var.asString("");
                for (const auto& s : data) {
                        dataMap.insert({var.getName(), s});
                }
        }

        int run() {
                // Specify the NetCDF file name.
                const std::string FILE_NAME("mydata.nc");

                try {
                        // Open the file.
                        NcFile dataFile(FILE_NAME, NcFile::read);

                        // A multimap to store variable names and their values as strings.
                        std::multimap<std::string, std::string> allVariableData;

                        // Get all variables in the file.
                        std::map<std::string, NcVar> varMap = dataFile.getVars();

                        // Iterate through all variables.
                        for (auto const& pair : varMap) {
                                NcVar var = pair.second;
                                NcType type = var.getType();

                                // Handle different NetCDF types.
                                if (type.getId() == nc_BYTE || type.getId() == nc_UBYTE) {
                                        readAndStoreVariable<signed char>(var, allVariableData);
                                } else if (type.getId() == nc_SHORT || type.getId() == nc_USHORT) {
                                        readAndStoreVariable<short>(var, allVariableData);
                                } else if (type.getId() == nc_INT || type.getId() == nc_UINT) {
                                        readAndStoreVariable<int>(var, allVariableData);
                                } else if (type.getId() == nc_INT64 || type.getId() == nc_UINT64) {
                                        readAndStoreVariable<long long>(var, allVariableData);
                                } else if (type.getId() == nc_FLOAT) {
                                        readAndStoreVariable<float>(var, allVariableData);
                                } else if (type.getId() == nc_DOUBLE) {
                                        readAndStoreVariable<double>(var, allVariableData);
                                } else if (type.getId() == nc_CHAR) {
                                        readAndStoreVariable<char>(var, allVariableData);
                                } else if (type.getId() == nc_STRING) {
                                        readAndStoreVariable<std::string>(var, allVariableData);
                                } else {
                                        std::cerr << "Warning: Skipping variable '" << var.getName() << "' with unknown type." << std::endl;
                                }
                        }

                        // Now print all the data from the multimap.
                        std::cout << "Printing all variable data from the multimap:" << std::endl;
                        std::string currentKey = "";
                        for (const auto& entry : allVariableData) {
                                if (entry.first != currentKey) {
                                        if (!currentKey.empty()) {
                                                std::cout << std::endl; // Add a newline for the next variable.
                                        }
                                        currentKey = entry.first;
                                        std::cout << "Variable: " << currentKey << " -> ";
                                }
                                std::cout << entry.second << " ";
                        }
                        std::cout << std::endl;

                } catch (const NcException& e) {
                        e.what();
                        return 1;
                }

                return 0;
        }
#endif

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

                gtk_grid_attach (GTK_GRID (grid), lab=gtk_label_new("NC Data"), 0, grid_row++, 10, 1);
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

                        g_message ("NC DUMP: VARS %s", pair.first.data() );

                        int unlimited_flag = FALSE;
                        gchar *vdims = g_strdup(" ");

                        if (var.getDims().size() > 0) {
                                std::vector<netCDF::NcDim> dims = var.getDims();
                                std::cout << " Dims:" << std::endl;
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
                        }
                        vdims[strlen(vdims)-1]=')';
                        gchar *vardef = g_strconcat((gchar*)var.getName().data(), vdims, NULL);
                        g_free(vdims);

                        XSM_DEBUG_GM (DBG_L3, "NcDumpToWidge::dump ** variable: %s", vardef);

                        VarName = gtk_check_button_new_with_label (vardef);
                        setup_toggle (VarName, (gchar*)var.getName().data());
                        //		std::cout << vardef << std::endl;
                        g_free(vardef);

                        gtk_grid_attach (GTK_GRID (grid), VarName, 0, grid_row, 2, 1);
                        gtk_widget_show (VarName);

                        variable = gtk_entry_new ();
                        ostringstream ostr_val;

                        
                        if (unlimited_flag){
                                ostr_val << "** Unlimited Data Set, data suppressed **";
                        } else {
                                if(type == ncChar){
                                        gchar* value_c_str = new gchar[var.getDims()[0].getSize() + 1]; // +1 for null terminator
                                        var.getVar ({0}, {var.getDims()[0].getSize()}, value_c_str);
                                        ostr_val << value_c_str;
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
                                } else if (type == ncChar) {
                                        readVarWriteToStream<char>(var, ostr_val);
                                } else if (type == ncString) {
                                        readVarWriteToStream<std::string>(var, ostr_val);
                                } else {
                                        std::cerr << "Warning: Skipping variable '" << var.getName() << "' with unknown type." << std::endl;
                                        ostr_val << "** work in progress **";
                                }
                        }
                        SETUP_ENTRY(variable, (const gchar*)ostr_val.str().c_str());
                        
                        gtk_grid_attach (GTK_GRID (grid), variable, 2, grid_row, 1, 1);
                        gtk_widget_show (variable);
                        
                        //multimap<string, NcAtt> varAttributes = var.getAtts();
                        
                        ostringstream  ostr_att;
                        map< std::string, NcVarAtt > attributes = var.getAtts ();
                        //multimap< std::string, NcAtt > attributes = var.getAtts ();
                        for (auto const& [name, att] : attributes){
                                NcVarAtt a = att;
                                ostr_att << name << ":" << att.getName() << " = " << get_att_as_string (a) << endl;
                        }
                        //else
                        //        ostr_att << "No attributes available for \"" << vname << "\" !";
                        
                        ostr_att << "\nValue(s):\n" << ostr_val.str().c_str(); 
                        ostr_att << ends;
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
                                gchar *tmp=get_att_as_string (a);
                                if (tmp) VarName_i = gtk_label_new (tmp);
                                else	 VarName_i = gtk_label_new (var.getName().data());
                                delete tmp;
                                SETUP_LABEL(VarName_i);

                                a = var.getAtt("unit");
                                gchar *unit = get_att_as_string (a);
                                gchar* value_str=NULL;
                                if (unit){
                                        NcVarAtt a = var.getAtt("label");
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
                                variable_i = gtk_entry_new ();
                                if (value_str){
                                        SETUP_ENTRY(variable_i, value_str);
                                        g_free (value_str);
                                }else{
                                        gchar *tmp=NULL;
                                        NcVarAtt a = var.getAtt("var_unit");
                                        tmp = get_att_as_string (a);
                                        if (tmp) ostr_val << " [" << tmp << "]"; // << " [vu]";
                                        g_free (tmp);
                                        a = var.getAtt("unit");
                                        tmp = get_att_as_string (a);
                                        if (tmp) ostr_val << " [" << tmp << "]"; // << " [u]";
                                        g_free (tmp);

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
