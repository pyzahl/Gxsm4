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

#include <locale.h>
#include <libintl.h>

#include <fstream>

#include <config.h>
#include <gtk/gtk.h>


#include "xsmdebug.h"
#include "glbvars.h" // because of Inst->... using only dat type.
#include "util.h"
#include "surface.h"


#include "dataio.h"
// #include "plug-ins/control/spm_scancontrol.h" // ????

double Contrast_to_VRangeZ (double contrast, double dz){
	return 64.*dz/contrast;
}
double VRangeZ_to_Contrast (double vrz, double dz){
	return 64.*dz/vrz;
}

/*! 
  BaseClass Dataio
*/
const char* Dataio::ioStatus(){
        static gchar *tmpE=NULL;
        g_free (tmpE); tmpE = g_strdup (error_status.str ().c_str ());
        static gchar *tmpP=NULL;
        
	switch(status){
	case FIO_OK: return 0;
	case FIO_OPEN_ERR: return "file open failed";
	case FIO_WRITE_ERR: return "file write failed";
	case FIO_READ_ERR: return "file read failed";
	case FIO_NSC_ERR: return "file read nsc-type failed, file truncated?";
	case FIO_NO_DATFILE: return "no valid dat file";
	case FIO_NO_NAME: return "no valid filename";
	case FIO_NO_MEM: return "no free memory";
	case FIO_NO_GNUFILE: return "no valid gnu or d2d file";
	case FIO_NO_NETCDFFILE: return "no valid NetCDF file";
	case FIO_NO_NETCDFXSMFILE: return "no valid NetCDF XSM file";
	case FIO_NOT_RESPONSIBLE_FOR_THAT_FILE: return "Handler does not support this filetype";
	case FIO_INVALID_FILE: return "invalid/inconsistent data file";
        case FIO_ERROR_CATCH: return tmpE;
        case FIO_GET_ERROR_STATUS_STRING: return tmpE;
        case FIO_GET_PROGRESS_INFO_STRING:
                g_free (tmpP); tmpP = g_strdup (progress_info.str ().c_str ());
                return tmpP;

        default: return "Dataio: unknown error";
	}
	return 0;
}


// ==============================================================
/*!
  Standard Data Format NetCDF
*/
// ==============================================================

#define NUM(array) (sizeof(array)/sizeof(array[0]))
#define NC_GET_VARIABLE(VNAME, VAR) if( !nc.getVar(VNAME).isNull ()) nc.getVar(VNAME).getVar(VAR)
#define ADD_NC_ATTRIBUTE_ANG(nv, unit, val)         \
	do{\
                gchar *tmp=g_strdup_printf ("NOTE: This length value is always stored in Angstroems.\nThe value for %s is %g %s in the user preferred display unit.", unit->Label(), unit->Base2Usr (val), unit->Symbol()); \
		nv.putAtt("Info", tmp); \
		nv.putAtt("label", unit->Label());	\
		nv.putAtt("var_unit", unit->Symbol());		\
		if (unit->Alias())			\
			nv.putAtt("unit", unit->Alias());	\
		else						\
			nv.putAtt("unitSymbol", unit->Symbol());	\
	}while(0)

#define ADD_NC_ATTRIBUTE_UNIT(nv, unit, val)        \
	do{\
                gchar *tmp=g_strdup_printf ("The value for %s is %g %s.", unit->Label(), unit->Base2Usr (val), unit->Symbol()); \
		nv.putAtt("Info", tmp); g_free(tmp);   \
		nv.putAtt("label", unit->Label());	\
		nv.putAtt("var_unit", unit->Symbol());		\
		if (unit->Alias())			\
			nv.putAtt("unit", unit->Alias());	\
		else						\
			nv.putAtt("unitSymbol", unit->Symbol());	\
	}while(0)

#define ADD_NC_ATTRIBUTE_UNIT_INFO(nv, unit, val, info)      \
	do{\
                gchar *tmp=g_strdup_printf ("The value for %s is %g %s.\n%s", unit->Label(), unit->Base2Usr (val), unit->Symbol(), info); \
		nv.putAtt("Info", tmp); g_free(tmp);   \
		nv.putAtt("label", unit->Label());	\
		nv.putAtt("var_unit", unit->Symbol());		\
		if (unit->Alias())			\
			nv.putAtt("unit", unit->Alias());	\
		else						\
			nv.putAtt("unitSymbol", unit->Symbol());	\
	}while(0)


gchar *NetCDF::get_var_att_as_string (NcFile &nc, NcVar &var, const gchar *att_name){
        netCDF::NcVarAtt att;
        progress_info  << "From " << var.getName()
                       << " reading attribute '" << att_name << "'"
                       << std::endl;
        try {
                att = var.getAtt (att_name);
        } catch (const netCDF::exceptions::NcException& e) {
                progress_info  << "Error: Attribute not found." << std::endl;
                error_status << "Warning: " << var.getName()
                             << " has no attribute '" << att_name << "'." << std::endl;
                return NULL;
        }

        if (att.isNull()) {
                error_status << "Warning: " << var.getName()
                             << " has no attribute '" << att_name << "'." << std::endl;
                return NULL;
        }
        
        // Check if the attribute type is a string
        netCDF::NcType att_type = att.getType();
        if (att_type != netCDF::NcType::nc_CHAR && att_type != netCDF::NcType::nc_STRING) {
                error_status << "Error: " << var.getName()
                             << " has attribute '" << att_name
                             << "' but is not a string type." << std::endl;
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
}

// NetCDF helper
UnitObj *NetCDF::get_gxsm_unit_from_nc(NcFile &nc, const gchar *var_id) {
        gchar *unit = get_att_as_string (nc, var_id, "unit");
        if (unit){
                gchar *label = get_att_as_string (nc, var_id, "label");
                if (label){
                        progress_info  << "Getting unit info for '" << var_id
                                       << "'. Label: '" << label
                                       << "' Unit: " << unit
                                       << std::endl;
                        UnitObj *u =  main_get_gapp ()->xsm->MakeUnit (unit, label);
                        g_free (unit);
                        g_free (label);
                        return u;
                }
                g_free (unit);
        }
        return NULL;
}

// NEW 20180430PY, 20251025PY: major NC4 upgrade
// =====================================================================================
// set scaling, apply load time scale correction -- as set in preferences
// make sure to set/keep this to 1.0,1.0,1.0 if NOT intending to adjust scan XYZ scale!
// applied to XYZ dimension, offset, scale/differentials and XY lookup

FIO_STATUS NetCDF::Read(xsm::open_mode mode){
	int i;
        error_status.str("NetCDF Read Status: "); error_status.clear(); // start clean
        progress_info.str("NetCDF Read Progress Info: "); progress_info.clear(); // start clean

        main_get_gapp ()->progress_info_new ("NetCDF Read Progress", 2);
        main_get_gapp ()->progress_info_set_bar_fraction (0., 1);
        main_get_gapp ()->progress_info_set_bar_text (name, 1);
        main_get_gapp ()->SetStatus (N_("Loading... "), name);
        main_get_gapp ()->monitorcontrol->LogEvent(N_("Loading... "), name);

        //g_message ("NetCDF::Read <%s>", name);

        try {
                ZD_TYPE zdata_typ=ZD_SHORT; // used by "H" -- Topographic STM/AFM data, historic default

                std::string filename(name);
                
                progress_info << "NcFile Open" << std::endl;
                // Open the NetCDF file in read mode
                netCDF::NcFile nc(filename, netCDF::NcFile::read);

                // try Topo Scan
                netCDF::NcVar Data = nc.getVar("H"); // try origial historic image data var name

                if (! Data.isNull ())
                        main_get_gapp ()->progress_info_set_bar_text ("Historic SHORT type data", 2), progress_info << "DataVar: >H<" << std::endl, zdata_typ=ZD_SHORT;
                else if (! (Data = nc.getVar("Intensity")).isNull ()) // try Diffract Scan data, aka D2D
                        main_get_gapp ()->progress_info_set_bar_text ("Type LONG", 2), progress_info << "DataVar: >Intensity<" << std::endl, zdata_typ=ZD_LONG; // used by "Intensity" -- diffraction counts
                else if (! (Data = nc.getVar("FloatField")).isNull ()) // try Float Data, all new hardware uses Float Type
                 	main_get_gapp ()->progress_info_set_bar_text ("Type FLOAT", 2), progress_info << "DataVar: >FloatField<" << std::endl, zdata_typ=ZD_FLOAT;
                else if (! (Data = nc.getVar("DoubleField")).isNull ()) // try Double Data
                 	main_get_gapp ()->progress_info_set_bar_text ("Type DOUBLE", 2), progress_info << "DataVar: >DoubleField<" << std::endl, zdata_typ=ZD_DOUBLE;
                else if (! (Data = nc.getVar("ByteField")).isNull ())        // Try Byte
                        main_get_gapp ()->progress_info_set_bar_text ("Type BYTE", 2), progress_info << "DataVar: >ByteField<" << std::endl, zdata_typ=ZD_BYTE;
                else if (! (Data = nc.getVar("ComplexDoubleField")).isNull ()) // Try Complex
                        main_get_gapp ()->progress_info_set_bar_text ("Type COMPLEX", 2), progress_info << "DataVar: >ComplexDoubleField<" << std::endl, zdata_typ=ZD_COMPLEX;
                else if (! (Data = nc.getVar("RGBA_ByteField")).isNull ()) // Try RGBA Byte / Color Image Type
                        main_get_gapp ()->progress_info_set_bar_text ("Type RGBA", 2), progress_info << "DataVar: >RGBA ByteField<" << std::endl, zdata_typ=ZD_RGBA;
                else { // failed looking for Gxsm data!!
                        std::cerr << "NetCDF file does not contain any Gxsm SPM data fields named: H, Intensity, FloatField, DoubleField, Byte, Compelx nor RGBA."
                                  << std::endl;
                        progress_info << "NetCDF file does not contain any Gxsm SPM data fields named: H, Intensity, FloatField, DoubleField, Byte, Compelx nor RGBA." << std::endl;
                        progress_info << "Trying to find and read any 2d data." << std::endl;
                        
                        netCDF::NcGroup rootGroup = nc.getGroup("/"); // Get the root group
                        std::multimap<std::string, netCDF::NcVar> vars = rootGroup.getVars();
                        std::cout << "Variables found in NetCDF file:" << std::endl;
                        for (auto const& [name, var] : vars) {
                                std::cout << " ** " << var.getName() << std::endl;
                                std::cout << " Type: " << var.getType().getName() << std::endl;
                                std::vector<netCDF::NcDim> dims = var.getDims();
                                std::cout << " Dims:" << std::endl;
                                for (const auto& dim : dims) {
                                        std::cout << " -- Dim-Name: " << dim.getName() << ", Length: " << dim.getSize() << std::endl;
                                }

                                if (dims.size() == 2){
                                        std::cout << " == FOUND IMAGE LIKE 2D VARIABLE == IMPORTING..." << std::endl;

                                        scan->data.s.ntimes  = 1;
                                        scan->data.s.nvalues = 1;
                                        scan->data.s.ny = dims[0].getSize();
                                        scan->data.s.nx = dims[1].getSize();

                                        main_get_gapp ()->progress_info_set_bar_fraction (0., 2);

                                        scan->mem2d->Resize(scan->data.s.nx, scan->data.s.ny, scan->data.s.nvalues, zdata_typ);

                                        for (int y=0; y<scan->data.s.ny; y++){
                                                std::vector<size_t> startp = { y, 0 };
                                                std::vector<size_t> countp = { 1, scan->data.s.nx };
                                                std::vector<double> row (scan->data.s.nx);
                                        
                                                var.getVar(startp, countp, row.data());
                                                for (int x=0; x<scan->data.s.nx; ++x)
                                                        scan->mem2d->PutDataPkt (x,y, (double)row[x]);
                                        }
                                        return status = FIO_OK;
                                }

                        }
                        return FIO_OPEN_ERR;
                }

                switch (mode){
                case xsm::open_mode::replace: scan->free_time_elements (); break;
                case xsm::open_mode::append_time: break;
                case xsm::open_mode::stitch_2d: break;
                }
	
                scan->data.ui.SetName (name);
                progress_info << "--- Data Type Integrity Check for " << Data.getName() << " NC-Type-ID: " << Data.getType().getId()<< std::endl;

                // check integrity
                switch (Data.getType().getId()){
                case NC_SHORT:  if (zdata_typ != ZD_SHORT) { std::cerr << "EE: NetCDF data field type is not matching expected type NC_SHORT."   << std::endl; return FIO_OPEN_ERR; } else break;
                case NC_INT:    std::cerr << "INFO: ?? NetCDF data field type is NC_INT." << std::endl; break;
                case NC_INT64:  if (zdata_typ != ZD_LONG)   { std::cerr << "EE: NetCDF data field type is not matching expected type NC_LONG."   << std::endl; return FIO_OPEN_ERR; } else break;
                case NC_FLOAT:  if (zdata_typ != ZD_FLOAT)  { std::cerr << "EE: NetCDF data field type is not matching expected type NC_FLOAT."  << std::endl; return FIO_OPEN_ERR; } else break;
                case NC_DOUBLE: if (zdata_typ != ZD_DOUBLE) { std::cerr << "EE: NetCDF data field type is not matching expected type NC_DOUBLE." << std::endl; return FIO_OPEN_ERR; } else break;
                case NC_BYTE:   if (zdata_typ != ZD_BYTE)   { std::cerr << "EE: NetCDF data field type is not matching expected type NC_BYTE."   << std::endl; return FIO_OPEN_ERR; } else break;
                case NC_CHAR:   std::cerr << "INFO: ?? NetCDF data field type is NC_CHAR." << std::endl; break;
                default:
                        progress_info << "EE: Data Type Integrity Check failed. Data type mismatch. STOP." << std::endl;
                        return FIO_GET_PROGRESS_INFO_STRING;
                }

                main_get_gapp ()->progress_info_set_bar_text ("Image Data", 2);
                main_get_gapp ()->progress_info_set_bar_fraction (0.25, 1);

                std::vector<netCDF::NcDim> dims = Data.getDims();
                progress_info << "Dimensions of data:" << std::endl;
                progress_info << "  #Dims: " << dims.size()<< std::endl;
                for (const auto& dim : dims) {
                        progress_info << "  Name: '" << dim.getName() << "', Length: " << dim.getSize()<< std::endl;
                }
                
                NcDim timed = dims[0];
                scan->data.s.ntimes  = timed.getSize() > 0 ? dims[0].getSize() : 1; // timed
                NcDim valued = dims[1];
                scan->data.s.nvalues = valued.getSize() > 0 ? dims[1].getSize() : 1; // valued
                NcDim dimyd = dims[2];
                scan->data.s.ny      = dimyd.getSize() > 0 ? dims[2].getSize() : 1; // ny
                NcDim dimxd = dims[3];
                scan->data.s.nx      = dimxd.getSize() > 0 ? dims[3].getSize() : 1; // nx

                main_get_gapp ()->progress_info_set_bar_fraction (0., 2);

                g_message ("scan->mem2d->Resize [%d x %d, #l %d] x %d #te", scan->data.s.nx, scan->data.s.ny, scan->data.s.nvalues, scan->data.s.ntimes);
                progress_info << "Scan Resize: [" << scan->data.s.nx << " x " << scan->data.s.ny << ", #l " << scan->data.s.nvalues << "] x #t " << scan->data.s.ntimes<< std::endl;

                scan->mem2d->Resize (scan->data.s.nx, scan->data.s.ny, scan->data.s.nvalues, zdata_typ);

                int ntimes_tmp = scan->data.s.ntimes;

                NcVar ncv_d;
                NcVar ncv_f;
                NcVar ncv_fo;
                NcVar ncv_v;

                progress_info << "Reading Layer Information along with data slices" << std::endl;
                
                scan->mem2d->start_read_layer_information (nc, ncv_d, ncv_f, ncv_fo, ncv_v);

                for (int time_index=0; time_index < ntimes_tmp; ++time_index){
                        main_get_gapp ()->progress_info_set_bar_fraction ((gdouble)time_index/(gdouble)scan->data.s.ntimes, 2);

                        progress_info  << "Reading data slice for time index " << time_index<< std::endl;
                        scan->mem2d->data->NcGet (Data, time_index);
                        progress_info  << "Reading data time slice completed." << std::endl;
                        scan->data.s.ntimes = ntimes_tmp;

                        if (scan->data.s.nvalues > 1){
                                progress_info  << "Reading Values..." << std::endl;
                                XSM_DEBUG(DBG_L2, "reading valued arr... n=" << scan->data.s.nvalues );
                                float *valuearr = new float[scan->data.s.nvalues];
                                if(!valuearr)
                                        return status = FIO_NO_MEM;
		  
                                XSM_DEBUG(DBG_L2, "getting array..." );
                                nc.getVar("value").getVar ({0},{scan->data.s.nvalues}, valuearr );
                                //double va0=0.;
                                //gboolean make_vai=false;
                                for(i=0; i<scan->data.s.nvalues; i++){
                                        //if (i==1 && va0==valuearr[i])
                                        //        make_vai=true;
                                        //va0=valuearr[i];
                                        //scan->mem2d->data->SetVLookup(i, make_vai ? (double)i : valuearr[i]);
                                        scan->mem2d->data->SetVLookup(i, valuearr[i]);
                                        //XSM_DEBUG_PLAIN (DBG_L3, valuearr[i] << " ");
                                }
		  
                                XSM_DEBUG_PLAIN (DBG_L3, "done." );
                                delete [] valuearr;
                        }

                        progress_info  << "Reading dimension lookups... and coordinates" << std::endl;

                        //--OffRotFix-- -> check for old version and fix offset (not any longer in Lookup)
	
                        double Xoff = 0.;
                        double Yoff = 0.;
                        try {
                                if (!nc.getVar("dimx").getAtt("extra_info").isNull ()){ // if this attribut is present, then no offset correction!
                                        nc.getVar("offsetx").getVar(&Xoff);
                                        nc.getVar("offsety").getVar(&Yoff);
                                }
                        } catch (const netCDF::exceptions::NcException& e) {;} // silent ignore as OK

                        progress_info  << "XY0: " << Xoff << ", " << Yoff<< std::endl;
                        progress_info  << "Lookups..." << std::endl;
                        progress_info  << "dimx: #" << dimxd.getSize()<< std::endl;

                        if (!nc.getVar("dimx").isNull ()){
                                float *dimsx = new float[dimxd.getSize ()];
                                if(!dimsx)
                                        return status = FIO_NO_MEM;
                                nc.getVar("dimx").getVar ({0}, {dimxd.getSize()}, dimsx);
                                progress_info  << "Updating X Lookup" << std::endl;
                                for(i=0; i < dimxd.getSize (); i++)
                                        scan->mem2d->data->SetXLookup(i, (dimsx[i]-Xoff) * xsmres.LoadCorrectXYZ[0]);
                                delete [] dimsx;
                        }else
                                progress_info  << "Missing 'dimx' variable." << std::endl;


                        progress_info  << "dimy: #" << dimyd.getSize()<< std::endl;

                        if (!nc.getVar("dimy").isNull ()){
                                float *dimsy = new float[dimyd.getSize ()];
                                if(!dimsy)
                                        return status = FIO_NO_MEM;
                                nc.getVar("dimy").getVar ({0}, {dimyd.getSize ()}, dimsy);
                                progress_info  << "Updating Y Lookup" << std::endl;
                                for(i=0; i<dimyd.getSize (); i++)
                                        scan->mem2d->data->SetYLookup(i, (dimsy[i]-Yoff) * xsmres.LoadCorrectXYZ[1]);
                                delete [] dimsy;
                        } else
                                progress_info  << "Missing 'dimx' variable." << std::endl;

                        progress_info  << "Checking comming basic paramaters..." << std::endl;

                        // try sranger (old)
                        progress_info  << "  ...checking for sranger_hwi Bias, Current" << std::endl;
                        NC_GET_VARIABLE ("sranger_hwi_bias", &scan->data.s.Bias);
                        NC_GET_VARIABLE ("sranger_hwi_voltage_set_point", &scan->data.s.SetPoint);
                        NC_GET_VARIABLE ("sranger_hwi_current_set_point", &scan->data.s.Current);

                        // read back layer informations
                        progress_info  << "Readback Layer information... auto adding basic params" << std::endl;
                        scan->mem2d->remove_layer_information ();

                        if (ncv_d.isNull () && ncv_f.isNull () && ncv_fo.isNull () && ncv_v.isNull ()){
                                progress_info  << "Readback Layer information again for time index %d" << time_index
                                               << std::endl;
                                scan->mem2d->add_layer_information (ncv_d, ncv_f, ncv_fo, ncv_v, time_index);
                        } else {
                                progress_info  << "Readback Basic Info... Scan ranges: rx,ry." << std::endl;
                                nc.getVar ("rangex").getVar (&scan->data.s.rx); scan->data.s.rx *= xsmres.LoadCorrectXYZ[0];
                                nc.getVar ("rangey").getVar (&scan->data.s.ry); scan->data.s.ry *= xsmres.LoadCorrectXYZ[1];

                                progress_info  << "Checking SR sranger_hwi_bias, ..." << std::endl;
                                NC_GET_VARIABLE ("sranger_hwi_bias", &scan->data.s.Bias);
                                NC_GET_VARIABLE ("sranger_hwi_voltage_set_point", &scan->data.s.SetPoint);
                                NC_GET_VARIABLE ("sranger_hwi_current_set_point", &scan->data.s.Current);

                                // mk2/3 try also
                                progress_info  << "Checking MK2/3: sranger_mk2_hwi_bias, ..." << std::endl;
                                NC_GET_VARIABLE ("sranger_mk2_hwi_bias", &scan->data.s.Bias);
                                NC_GET_VARIABLE ("sranger_mk2_hwi_mix0_set_point", &scan->data.s.Current);
                                NC_GET_VARIABLE ("sranger_mk2_hwi_z_setpoint", &scan->data.s.ZSetPoint);

                                // try RPSPMC
                                progress_info  << "Checking RPSPM JSON VARS for Bias, ..." << std::endl;
                                NC_GET_VARIABLE ("rpspmc_hwi_Bias", &scan->data.s.Bias); // V
                                NC_GET_VARIABLE ("rpspmc_hwi_Z_Servo_SetPoint", &scan->data.s.Current); // nA
                                NC_GET_VARIABLE ("rpspmc_hwi_Z_Servo_CZ_SetPoint", &scan->data.s.ZSetPoint); // Ang
                                //NC_GET_VARIABLE ("rpspmc_hwi_Z_Servo_FLevel", xxxx); // nA

                                
                                progress_info  << "AutoAdding LayerInfo for OSD..." << std::endl;
                                if (fabs(scan->data.s.Bias) >= 1.0)
                                        scan->mem2d->add_layer_information (new LayerInformation ("Bias", scan->data.s.Bias, "%5.2f V"));
                                else
                                        scan->mem2d->add_layer_information (new LayerInformation ("Bias", scan->data.s.Bias*1000, "%5.2f mV"));
                                
                                scan->mem2d->add_layer_information (new LayerInformation (scan->data.ui.dateofscan));
                                scan->mem2d->add_layer_information (new LayerInformation ("X-size", scan->data.s.rx, "Rx: %5.1f \303\205"));
                                scan->mem2d->add_layer_information (new LayerInformation ("Y-size", scan->data.s.ry, "Ry: %5.1f \303\205"));
                                
                                scan->mem2d->add_layer_information (new LayerInformation ("Z-SetPoint", scan->data.s.ZSetPoint, "%5.2f \303\205"));
                                if (IS_AFM_CTRL)
                                        scan->mem2d->add_layer_information (new LayerInformation ("SetPoint", scan->data.s.SetPoint, "%5.2f V"));
                                else{
                                        if (fabs(scan->data.s.Current) >= 0.1)
                                                scan->mem2d->add_layer_information (new LayerInformation ("Current", scan->data.s.Current, "%5.2f nA"));
                                        else
                                                scan->mem2d->add_layer_information (new LayerInformation ("Current", scan->data.s.Current*1000, "%5.1f pA"));
                                }
                        }

                        if (ntimes_tmp > 1){ // do not do this if only one time dim.
                                progress_info  << "Looking up start times for frame # " << time_index<< std::endl;
                                double ref_time=(double)(time_index);
                                if (!nc.getVar("time").isNull ()){
                                        NcVar rt = nc.getVar("time");
                                        rt.getVar({time_index}, {1}, &ref_time);

                                        //rt.set_cur(time_index);
                                        //rt->get(&ref_time, 1);
                                        scan->mem2d->add_layer_information (new LayerInformation ("t", ref_time, "%.2f s"));
                                }
                                progress_info  << "Appending time element." << std::endl;
                                scan->append_current_to_time_elements (time_index, ref_time); // need to add lut
                        }
                }

                progress_info  << "Reading Settings and Variables" << std::endl;

                main_get_gapp ()->progress_info_set_bar_text ("Variables", 2);
                main_get_gapp ()->progress_info_set_bar_fraction (0.25, 1);

                { UnitObj *u = get_gxsm_unit_from_nc(nc, "time"); if (u) { scan->data.SetTimeUnit(u); delete u; } }
                { UnitObj *u = get_gxsm_unit_from_nc(nc, "value"); if (u) { scan->data.SetVUnit(u); delete u; } }
                { UnitObj *u = get_gxsm_unit_from_nc(nc, "rangex"); if (u) { scan->data.SetXUnit(u); delete u; } }
                { UnitObj *u = get_gxsm_unit_from_nc(nc, "rangey"); if (u) { scan->data.SetYUnit(u); delete u; } }

                // Check Z daya type via Z label
                double LoadCorrectZ = 1.0;
                progress_info  << "NetCDF GXSM data type: " << scan->data.ui.type<< std::endl;

                try {
                        gchar *zlabel = get_var_att_as_string (nc, Data, "ZLabel");
                        if (zlabel){
                                progress_info  << "User NetCDF Load Correct Z scaling check type: Data is '" << zlabel << "'" << std::endl;

                                if (!strcmp (zlabel, "Z"))
                                        LoadCorrectZ = xsmres.LoadCorrectXYZ[2];
                                else
                                        LoadCorrectZ = xsmres.LoadCorrectXYZ[3];
                
                                delete zlabel;
                        }
                } catch (const netCDF::exceptions::NcException& e) {;} // silent ignore as OK


                if (fabs (xsmres.LoadCorrectXYZ[0] - 1.0) > 0.001)
                        progress_info  << "User NetCDF Load Correct X scaling is set to: " << xsmres.LoadCorrectXYZ[0]<< std::endl;
                if (fabs (xsmres.LoadCorrectXYZ[1] - 1.0) > 0.001)
                        progress_info  << "User NetCDF Load Correct Y scaling is set to: " << xsmres.LoadCorrectXYZ[1]<< std::endl;
                if (fabs (LoadCorrectZ - 1.0) > 0.001)
                        progress_info  << "User NetCDF Load Correct Z scaling is set to: " << LoadCorrectZ<< std::endl;
        
                nc.getVar("rangez").getVar(&scan->data.s.rz); scan->data.s.rz *= LoadCorrectZ;

                { UnitObj *u = get_gxsm_unit_from_nc(nc, "rangez"); if (u) { scan->data.SetZUnit(u); delete u; } }

                nc.getVar("dx").getVar(&scan->data.s.dx); scan->data.s.dx *= xsmres.LoadCorrectXYZ[0];
                nc.getVar("dy").getVar(&scan->data.s.dy); scan->data.s.dy *= xsmres.LoadCorrectXYZ[1];
                nc.getVar("dz").getVar(&scan->data.s.dz); scan->data.s.dz *= LoadCorrectZ;
                nc.getVar("offsetx").getVar(&scan->data.s.x0); scan->data.s.x0 *= xsmres.LoadCorrectXYZ[0];
                nc.getVar("offsety").getVar(&scan->data.s.y0); scan->data.s.y0 *= xsmres.LoadCorrectXYZ[1];
                nc.getVar("alpha").getVar(&scan->data.s.alpha);

                nc.getVar("contrast").getVar(&scan->data.display.contrast);
                nc.getVar("bright").getVar(&scan->data.display.bright);

                scan->data.display.vrange_z = Contrast_to_VRangeZ (scan->data.display.contrast, scan->data.s.dz);

                //#define MINIMAL_READ_NC
#ifndef MINIMAL_READ_NC
                
                progress_info  << "NetCDF read optional informations..." << std::endl;

                NcVar comment = nc.getVar("comment");
                if (!comment.isNull ()){
                        int clen = comment.getDims ()[0].getSize();
                        G_NEWSIZE(scan->data.ui.comment, 1+clen);
                        memset (scan->data.ui.comment, 0, 1+clen);
                        comment.getVar ({0},{clen}, scan->data.ui.comment);
                }

                NcVar basename = nc.getVar("basename");
                if(!basename.isNull ()){
                        int clen = basename.getDims ()[0].getSize();
                        G_NEWSIZE(scan->data.ui.originalname, clen);
                        basename.getVar ({0}, {clen}, scan->data.ui.originalname);
                        XSM_DEBUG (DBG_L2, "got original name:" << scan->data.ui.originalname);
                }
                else{
                        scan->data.ui.SetOriginalName ("original name is not available");
                        XSM_DEBUG (DBG_L2, "original name is not available");
                }

                NcVar type = nc.getVar("type"); 
                if (!type.isNull ()){
                        int clen = type.getDims ()[0].getSize();
                        G_NEWSIZE(scan->data.ui.type, 1+clen);
                        memset (scan->data.ui.type, 0, 1+clen);
                        type.getVar({0},{clen},scan->data.ui.type);
                }
                NcVar username = nc.getVar("username");
                if (!username.isNull ()){
                        int clen = username.getDims ()[0].getSize();
                        G_NEWSIZE(scan->data.ui.user, 1+clen);
                        memset (scan->data.ui.user, 0, 1+clen);
                        username.getVar ({0}, {clen}, scan->data.ui.user);
                }
                NcVar dateofscan = nc.getVar("dateofscan");
                if(!dateofscan.isNull ()){
                        int clen = dateofscan.getDims ()[0].getSize();
                        G_NEWSIZE(scan->data.ui.dateofscan, 1+clen);
                        memset (scan->data.ui.dateofscan, 0, 1+clen);
                        dateofscan.getVar ({0}, {clen}, scan->data.ui.dateofscan);
                }

                NC_GET_VARIABLE ("t_start", &scan->data.s.tStart);
                NC_GET_VARIABLE ("t_end", &scan->data.s.tEnd);
                NC_GET_VARIABLE ("viewmode", &scan->data.display.ViewFlg); scan->SetVM (scan->data.display.ViewFlg);
                NC_GET_VARIABLE ("vrange_z", &scan->data.display.vrange_z);
                NC_GET_VARIABLE ("voffset_z", &scan->data.display.voffset_z);

                // Count Data, SPA-LEED, etc. only:
                NC_GET_VARIABLE ("energy", &scan->data.s.Energy);
                NC_GET_VARIABLE ("gatetime", &scan->data.s.GateTime);
                NC_GET_VARIABLE ("cnttime", &scan->data.s.GateTime);
                if (!nc.getVar("gatetime").isNull () || !nc.getVar("cnttime").isNull ())
                        scan->data.s.dz = 1./scan->data.s.GateTime;
                
                NC_GET_VARIABLE ("cnt_high", &scan->data.display.z_high);
                NC_GET_VARIABLE ("z_high", &scan->data.display.z_high);
                NC_GET_VARIABLE ("cps_high", &scan->data.display.z_high);
                NC_GET_VARIABLE ("cnt_low", &scan->data.display.z_low);
                NC_GET_VARIABLE ("z_low", &scan->data.display.z_low);
                NC_GET_VARIABLE ("cps_low", &scan->data.display.z_low);
                NC_GET_VARIABLE ("spa_orgx", &scan->data.s.SPA_OrgX);
                NC_GET_VARIABLE ("spa_orgy", &scan->data.s.SPA_OrgY);
                // NC_GET_VARIABLE ("", &scan->data.);


                // load attached Events, if any
                main_get_gapp ()->progress_info_set_bar_fraction (0.5, 1);
                main_get_gapp ()->progress_info_set_bar_text ("Events", 2);
                scan->mem2d->LoadScanEvents (nc);

                // Signal PlugIns
                main_get_gapp ()->progress_info_set_bar_fraction (0.75, 1);
                main_get_gapp ()->progress_info_set_bar_text ("Plugin Data", 2);
                main_get_gapp ()->SignalCDFLoadEventToPlugins (nc);
	
                // try to recover some common defaults data and add to OSD (old sranger)
                NC_GET_VARIABLE ("sranger_hwi_bias", &scan->data.s.Bias);
                NC_GET_VARIABLE ("sranger_hwi_voltage_set_point", &scan->data.s.SetPoint);
                NC_GET_VARIABLE ("sranger_hwi_current_set_point", &scan->data.s.Current);

                // mk2/3 try to recover
                NC_GET_VARIABLE ("sranger_mk2_hwi_bias", &scan->data.s.Bias);
                NC_GET_VARIABLE ("sranger_mk2_hwi_mix0_set_point", &scan->data.s.Current);
                NC_GET_VARIABLE ("sranger_mk2_hwi_z_setpoint", &scan->data.s.ZSetPoint);

                scan->mem2d->add_layer_information (new LayerInformation ("CZ SetPoint", scan->data.s.ZSetPoint, "%5.2f  \303\205"));

                NC_GET_VARIABLE ("sranger_mk2_hwi_motor", &scan->data.s.Motor);
                NC_GET_VARIABLE ("sranger_mk2_hwi_pll_reference", &scan->data.s.pllref);

                //g_message ("NetCDF read: sranger_mk2_hwi_pll_reference = %g Hz", scan->data.s.pllref);
        
#endif
                main_get_gapp ()->progress_info_set_bar_fraction (1., 1);
                main_get_gapp ()->progress_info_set_bar_text ("Finishing", 2);

                progress_info  << " ** DONE READING NCDAT FILE **" << std::endl;
                error_status << "Read Completed." << std::endl;

                // TEST to force exception
                //if (Data.getAtt("test_info").isNull ()){ progress_info  << " ** TEST getAtt is Null **" << std::endl; }
                //else { progress_info  << " ** TEST getAtt is OK **" << std::endl; }
               
                main_get_gapp ()->progress_info_close ();
                return status = FIO_OK; 

        } catch (const netCDF::exceptions::NcException& e) {
                error_status << "EE: NetCDF File Read Error. Catch " << e.what() << std::endl
                             << "************************************************" << std::endl
                             << "PROGRESS REPORT" << std::endl
                             << "************************************************" << std::endl
                             << progress_info.str() << std::endl;
                
                std::cerr << "NetCDF Read Error Status: " << std::endl << error_status.str()
                          << std::endl;
                
                main_get_gapp ()->progress_info_close ();
		return status = FIO_ERROR_CATCH;
        }
}



FIO_STATUS NetCDF::Write(){
	XSM_DEBUG (DBG_L2, "NetCDF::Write");
	// name convention NetCDF: *.nc
        error_status.str("NetCDF Write Status: "); error_status.clear(); // start clear
        progress_info.str("NetCDF Write Progress Info: "); progress_info.clear(); // start clean

        main_get_gapp ()->progress_info_new ("NetCDF Write Progress",2);
        main_get_gapp ()->progress_info_set_bar_fraction (0., 1);
        main_get_gapp ()->progress_info_set_bar_text (name, 1);
        main_get_gapp ()->progress_info_set_bar_fraction (0., 2);

        progress_info  << "NetCDF::Write <" << name << ">" << std::endl;
        
        try {
                std::string filename(name);

                NcFile nc(filename, NcFile::replace); // ,NULL,0,NcFile::Netcdf4); // Create, leave in define mode
                // use 64 byte offset mode if(ntimes > 2) , FileFormat=NcFile::Offset64Bits
   
                scan->data.ui.SetName(name);

                XSM_DEBUG (DBG_L2, "NetCDF::Write-> open OK");
                progress_info  << "Creating Dimensions." << std::endl;

                // Create dimensions
#define NTIMES   1
#define TIMELEN 40
#define VALUEDIM 1
                NcDim timed  = nc.addDim("time", scan->data.s.ntimes);   // Time of Scan
                NcDim valued = nc.addDim("value", scan->data.s.nvalues); // Value of Scan, eg. Volt / Force
                NcDim dimyd  = nc.addDim("dimy", scan->mem2d->GetNy());  // Image Dim. in Y (#Samples)
                NcDim dimxd  = nc.addDim("dimx", scan->mem2d->GetNx());  // Image Dim. in X (#Samples)
  
                NcDim reftimed  = nc.addDim("reftime", TIMELEN);  // Starting time
  
                NcDim commentd    = nc.addDim("comment", strlen(scan->data.ui.comment)+1);       // Comment
                NcDim titled      = nc.addDim("title", strlen(scan->data.ui.title)+1);           // Title
                NcDim typed       = nc.addDim("type", strlen(scan->data.ui.type)+1);           // Type
                NcDim usernamed   = nc.addDim("username", strlen(scan->data.ui.user)+1);         // Username
                NcDim dateofscand = nc.addDim("dateofscan", strlen(scan->data.ui.dateofscan)+1); // Scandate

                XSM_DEBUG (DBG_L2, "NetCDF::Write-> Creating Data Var");
                // Create variables and their attributes
                NcVar Data;

                std::vector<netCDF::NcDim>  data_dims;
                data_dims.push_back (timed);
                data_dims.push_back (valued);
                data_dims.push_back (dimyd);
                data_dims.push_back (dimxd);

                progress_info  << "Preparing Data Field." << std::endl;
                
                switch(scan->mem2d->GetTyp()){
                case ZD_BYTE:
                        Data = nc.addVar("ByteField", ncByte, data_dims); // timed, valued, dimyd, dimxd);
                        Data.putAtt("long_name", "BYTE: 8bit data field");
                        break;
                case ZD_SHORT:
                        Data = nc.addVar("H", ncShort, data_dims);
                        Data.putAtt("long_name", "SHORT: signed 16bit data field)");
                        break;
                case ZD_LONG: 
                        Data = nc.addVar("Intensity", ncInt64, data_dims);
                        Data.putAtt("long_name", "LONG: signed 32bit data field");
                        break;
                case ZD_FLOAT: 
                        Data = nc.addVar("FloatField", ncFloat, data_dims);
                        Data.putAtt("long_name", "FLOAT: single precision floating point data field");
                        break;
                case ZD_DOUBLE: 
                        Data = nc.addVar("DoubleField", ncDouble, data_dims);
                        Data.putAtt("long_name", "DOUBLE: souble precision floating point data field");
                        break;
                case ZD_COMPLEX: 
                        Data = nc.addVar("ComplexDoubleField", ncDouble, data_dims);
                        Data.putAtt("long_name", "complex (abs,re,im) double data field");
                        break;
                case ZD_RGBA: 
                        Data = nc.addVar("RGBA_ByteField", ncDouble, data_dims);
                        Data.putAtt("long_name", "byte RGBA data field");
                        break;
                default:
                        return status = FIO_NO_NETCDFXSMFILE;
                        break;
                }

                progress_info  << "Adding Unit hints, time, ..." << std::endl;

                Data.putAtt("var_units_hint", "raw DAC/Volts/FPGA raw/counter/... values of multi dimensional data field. The data Unit is defined when multipling by dz and using the dz unit.");
                ADD_NC_ATTRIBUTE_UNIT_INFO(Data, scan->data.Zunit, scan->data.s.dz * scan->mem2d->GetDataPkt(0,0), "NOTE: This is the final Unit after scaling by dz. Test/Reference value given here is at index (0,0,0,0) * dz.");

                XSM_DEBUG (DBG_L2, "NetCDF::Write-> Adding Units ZLab");

                Data.putAtt("ZLabel", scan->data.Zunit->Label());
                Data.putAtt("unit_x_dz", scan->data.Zunit->Symbol());
                if (scan->data.Zunit->Alias())
                        Data.putAtt("ZSrcUnit", scan->data.Zunit->Alias());
                else
                        XSM_DEBUG(DBG_L2, "NetCDF::Write-> Warning: No Alias for Z unit, cannot restore unit on load" );
  
                XSM_DEBUG (DBG_L2, "NetCDF::Write-> Adding time, att");

                NcVar time  = nc.addVar("time", ncDouble, timed);
                time.putAtt("long_name", "Time since reftime (actual time scanning) or other virtual in time changeing parameter");
                time.putAtt("short_name", "Scan Time");
                //time.putAtt("var_unit", "s"); // 20170130-PYZ-added-actual-unit-info
                time.putAtt("dimension_unit_info", "actual data dimension for SrcType in time element dimension");
                if (scan->data.TimeUnit){
                        time.putAtt("label", scan->data.TimeUnit->Label());
                        time.putAtt("unit", scan->data.TimeUnit->Symbol());
                }else{
                        time.putAtt("label", "time");
                        time.putAtt("unit", "s");
                }
  
  
                XSM_DEBUG (DBG_L2, "NetCDF::Write-> Adding Units Value");

                NcVar value = nc.addVar("value", ncFloat, valued);
                value.putAtt("long_name", "Image Layers of SrcType at Values");
                if (scan->data.Vunit){
                        value.putAtt("label", scan->data.Vunit->Label());
                        value.putAtt("unit", scan->data.Vunit->Symbol());
                }else{
                        value.putAtt("label", "Value");
                        value.putAtt("unit", "A.U.");
                }
  
                XSM_DEBUG (DBG_L2, "NetCDF::Write-> dim x y reft");

                NcVar dimx  = nc.addVar("dimx", ncFloat, dimxd);
                dimx.putAtt("long_name", "# Pixels in X, contains X-Pos Lookup");
                dimx.putAtt("extra_info", "X-Lookup without offset");
  
                NcVar dimy  = nc.addVar("dimy", ncFloat, dimyd);
                dimy.putAtt("long_name", "# Pixels in Y, contains Y-Pos Lookup");
                dimy.putAtt("extra_info", "Y-Lookup without offset");
  
                NcVar reftime  = nc.addVar("reftime", ncChar, reftimed);
                reftime.putAtt("long_name", "Reference time, i.e. Scan Start");
                reftime.putAtt("unit", "date string");
  
                progress_info  << "Adding Using Infos, Original File Name, ranges, ..." << std::endl;

                XSM_DEBUG (DBG_L2, "NetCDF::Write-> Comment Title ...");

                NcVar comment    = nc.addVar("comment", ncChar, commentd);
                NcVar title      = nc.addVar("title", ncChar, titled);
                NcVar type       = nc.addVar("type", ncChar, typed);
                NcVar username   = nc.addVar("username", ncChar, usernamed);
                NcVar dateofscan = nc.addVar("dateofscan", ncChar, dateofscand);
  
                // This unit and label entries are used to restore the correct unit later!
                NcVar rangex     = nc.addVar("rangex", ncDouble);
                ADD_NC_ATTRIBUTE_ANG(rangex, scan->data.Xunit, scan->data.s.rx);

                NcVar rangey     = nc.addVar("rangey", ncDouble);
                ADD_NC_ATTRIBUTE_ANG(rangey, scan->data.Yunit, scan->data.s.ry);

                NcVar rangez     = nc.addVar("rangez", ncDouble);
                ADD_NC_ATTRIBUTE_UNIT(rangez, scan->data.Zunit, scan->data.s.rz);

                NcVar dx         = nc.addVar("dx", ncDouble);
                ADD_NC_ATTRIBUTE_ANG(dx, scan->data.Xunit, scan->data.s.dz);

                NcVar dy         = nc.addVar("dy", ncDouble);
                ADD_NC_ATTRIBUTE_ANG(dy, scan->data.Yunit, scan->data.s.dy);

                NcVar dz         = nc.addVar("dz", ncDouble);
                ADD_NC_ATTRIBUTE_UNIT(dz, scan->data.Zunit, scan->data.s.dz);

                NcVar opt_xpiezo_AV = nc.addVar("opt_xpiezo_av", ncDouble);
                opt_xpiezo_AV.putAtt("type", "pure optional information, not used by Gxsm for any scaling after the fact. For the records only");
                opt_xpiezo_AV.putAtt("label", "Configured X Piezo Sensitivity");
                opt_xpiezo_AV.putAtt("unit", "Ang/V");

                NcVar opt_ypiezo_AV = nc.addVar("opt_ypiezo_av", ncDouble);
                opt_ypiezo_AV.putAtt("type", "pure optional information, not used by Gxsm for any scaling after the fact. For the records only");
                opt_ypiezo_AV.putAtt("label", "Configured Y Piezo Sensitivity");
                opt_ypiezo_AV.putAtt("unit", "Ang/V");

                NcVar opt_zpiezo_AV = nc.addVar("opt_zpiezo_av", ncDouble);
                opt_zpiezo_AV.putAtt("type", "pure optional information, not used by Gxsm for any scaling after the fact. For the records only");
                opt_zpiezo_AV.putAtt("label", "Configured Z Piezo Sensitivity");
                opt_zpiezo_AV.putAtt("unit", "Ang/V");

                NcVar offsetx    = nc.addVar("offsetx", ncDouble);
                ADD_NC_ATTRIBUTE_ANG(offsetx, scan->data.Xunit, scan->data.s.x0);

                NcVar offsety    = nc.addVar("offsety", ncDouble);
                ADD_NC_ATTRIBUTE_ANG(offsety, scan->data.Yunit, scan->data.s.y0);

                NcVar alpha      = nc.addVar("alpha", ncDouble);
                alpha.putAtt("unit", "Grad");
                alpha.putAtt("label", "Rotation");
  
                NcVar contrast   = nc.addVar("contrast", ncDouble);
                NcVar bright     = nc.addVar("bright", ncDouble);

                progress_info  << "Adding Optional Variables as needed ..." << std::endl;

                XSM_DEBUG (DBG_L2, "NetCDF::Write-> Creating Options Vars");
                // optional...
                NcVar display_vrange_z;
                NcVar display_voffset_z;

                if(!(IS_SPALEED_CTRL)){
                        display_vrange_z = nc.addVar("vrange_z", ncDouble);
                        display_vrange_z.putAtt("long_name", "View Range Z");
                        ADD_NC_ATTRIBUTE_UNIT(display_vrange_z, scan->data.Zunit, scan->data.display.vrange_z);

                        display_voffset_z = nc.addVar("voffset_z", ncDouble);
                        display_voffset_z.putAtt("long_name", "View Offset Z");
                        ADD_NC_ATTRIBUTE_UNIT(display_voffset_z, scan->data.Zunit, scan->data.display.voffset_z);
                }

                NcVar t_start      = nc.addVar("t_start", ncInt64);
                t_start.putAtt("unit", "s");
                t_start.putAtt("label", "Scan Start Time in Unix time (seconds sice 00:00:00 UTC on 1 January 1970)");
                NcVar t_end        = nc.addVar("t_end", ncInt64);
                t_end.putAtt("unit", "s");
                t_end.putAtt("label", "Scan Stop/End Time in Unix time (seconds sice 00:00:00 UTC on 1 January 1970)");

                NcVar viewmode     = nc.addVar("viewmode", ncInt64);
                viewmode.putAtt("long_name", "last viewmode flag");

                // data.ui.basename:
                // filename with the original data (set once and never changed again)
                if ( g_strrstr(scan->data.ui.originalname, "(not saved)") 
                     || g_strrstr(scan->data.ui.originalname, "(in progress)")
                     || g_strrstr(scan->data.ui.originalname, "(new)") ){
                        scan->data.ui.SetOriginalName( name ); // full path/name for storing original position once!
                        XSM_DEBUG (DBG_L2, "got original name:" << scan->data.ui.originalname);
                }else{
                        XSM_DEBUG (DBG_L2, "original left untouched:" << name << " != " << scan->data.ui.originalname);
                }

                // don´t confuse: NC basenase := originalname  --  program: ui.basename is only for automatic namegenerating
                NcDim basenamed   = nc.addDim("basename", strlen(scan->data.ui.originalname)+1);
                NcVar basename    = nc.addVar("basename", ncChar, basenamed);


                // only for LEED like Instruments
                NcVar energy;
                NcVar gatetime;
                NcVar z_high;
                NcVar z_low;
                NcVar spaorgx;
                NcVar spaorgy;
  
                if(IS_SPALEED_CTRL){
                        energy   = nc.addVar("energy", ncDouble);
                        energy.putAtt("unit", "eV");

                        gatetime = nc.addVar("gatetime", ncDouble);
                        gatetime.putAtt("unit", "s");

                        z_high  = nc.addVar("cps_high", ncDouble);
                        z_high.putAtt("unit", "CPS or raw Z");

                        z_low   = nc.addVar("cps_low", ncDouble);
                        z_low.putAtt("unit", "CPS or raw Z");

                        spaorgx  = nc.addVar("spa_orgx", ncDouble);
                        spaorgx.putAtt("unit", "V");

                        spaorgy  = nc.addVar("spa_orgy", ncDouble);
                        spaorgy.putAtt("unit", "V");
                }

                // ....
  
                progress_info  << "Adding System Attributes ..." << std::endl;

                // Global attributes
                nc.putAtt("Creator", PACKAGE);
                nc.putAtt("Version", VERSION);
                nc.putAtt("build_from", COMPILEDBYNAME);
                nc.putAtt("DataIOVer", 
                          "Gxsm4-NetCDF4-V4.3.0ff"
                          );
                nc.putAtt("HardwareCtrlType", xsmres.HardwareType);
                nc.putAtt("HardwareConnectionDev", xsmres.DSPDev);
                nc.putAtt("InstrumentType", xsmres.InstrumentType);
                nc.putAtt("InstrumentName", xsmres.InstrumentName);
  
                // Start writing data, implictly leaves define mode
                progress_info  << "Writing Data ..." << std::endl;

                XSM_DEBUG (DBG_L2, "NetCDF::Write-> NcPut Data");
                XSM_DEBUG (DBG_L2, "NC write: " << timed.getSize () << "x" << valued.getSize () << "x" << dimxd.getSize () << "x" << dimyd.getSize ());
  
                main_get_gapp ()->progress_info_set_bar_text ("Image Data", 2);
                main_get_gapp ()->progress_info_set_bar_fraction (0.25, 1);
                main_get_gapp ()->progress_info_set_bar_fraction (0., 2);
                if (scan->data.s.ntimes == 1){
                        double timearr[] = { (double)(scan->data.s.tEnd-scan->data.s.tStart) };
                        scan->mem2d->data->NcPut (Data);
                        time.putVar ({0}, { NUM(timearr) }, timearr);
                } else
                        for (int time_index=0; time_index<scan->data.s.ntimes; ++time_index){
                                scan->mem2d_time_element(time_index)->data->NcPut (Data, time_index);
                                double ref_time = scan->mem2d_time_element(time_index)->get_frame_time ();
                                time.putVar ({time_index}, {1}, &ref_time);
                                main_get_gapp ()->progress_info_set_bar_fraction ((gdouble)time_index/(gdouble)scan->data.s.ntimes, 2);
                        }

                progress_info  << "Writing Coordinate Lookups ..." << std::endl;

                main_get_gapp ()->progress_info_set_bar_text ("Look-Ups", 2);
                main_get_gapp ()->progress_info_set_bar_fraction (0.5, 1);

                float *valuearr = new float[valued.getSize ()];
                if(!valuearr)
                        return status = FIO_NO_MEM;

                XSM_DEBUG (DBG_L2, "NetCDF::Write-> NcPut Lookups");
                for(int i=0; i < valued.getSize(); i++)
                        valuearr[i] = scan->mem2d->data->GetVLookup(i);

                value.putVar ({0}, { valued.getSize ()}, valuearr);
                delete [] valuearr;
                ADD_NC_ATTRIBUTE_UNIT_INFO(value, scan->data.Vunit, scan->mem2d->data->GetVLookup(0),"Lookup Value Dimension index to Value");

                int i;
                //  float x0=scan->data.s.x0-scan->data.s.rx/2.;
                float *dimsx = new float[dimxd.getSize ()];
                if(!dimsx)
                        return status = FIO_NO_MEM;

                for(i=0; i < dimxd.getSize (); i++)
                        dimsx[i] = scan->mem2d->data->GetXLookup(i);

                dimx.putVar ({0}, { dimxd.getSize () }, dimsx);
                delete [] dimsx;
                ADD_NC_ATTRIBUTE_ANG(dimx, scan->data.Xunit, scan->mem2d->data->GetXLookup(0));

                //  float y0=scan->data.s.y0;
                float *dimsy = new float[dimyd.getSize ()];
                if(!dimsy)
                        return status = FIO_NO_MEM;

                for(i=0; i < dimyd.getSize (); i++)
                        dimsy[i] = scan->mem2d->data->GetYLookup(i);

                dimy.putVar ({0}, { dimyd.getSize () }, dimsy);
                delete [] dimsy;
                ADD_NC_ATTRIBUTE_ANG(dimy, scan->data.Yunit, scan->mem2d->data->GetYLookup(0));

                main_get_gapp ()->progress_info_set_bar_text ("Scan-Parameter", 2);
                main_get_gapp ()->progress_info_set_bar_fraction (0.6, 1);

                progress_info  << "Writing User Info, Dates, Ranges..." << std::endl;

                XSM_DEBUG (DBG_L2, "NetCDF::Write-> NcPut Vars...");
                char* s = ctime(&scan->data.s.tStart);
                reftime.putVar    ({0}, {strlen(s)}, s);
                comment.putVar    ({0}, {commentd.getSize ()}, scan->data.ui.comment);
                title.putVar      ({0}, {titled.getSize()}, scan->data.ui.title);
                type.putVar       ({0}, {typed.getSize ()}, scan->data.ui.type);
                username.putVar   ({0}, {usernamed.getSize ()}, scan->data.ui.user);
                dateofscan.putVar ({0}, {dateofscand.getSize ()}, scan->data.ui.dateofscan);

                rangex .putVar( &scan->data.s.rx);
                rangey .putVar( &scan->data.s.ry);
                rangez .putVar( &scan->data.s.rz); 
                dx     .putVar( &scan->data.s.dx); 
                dy     .putVar( &scan->data.s.dy); 
                dz     .putVar( &scan->data.s.dz); 
                offsetx.putVar( &scan->data.s.x0); 
                offsety.putVar( &scan->data.s.y0); 
                alpha  .putVar( &scan->data.s.alpha);

                opt_xpiezo_AV.putVar( &xsmres.XPiezoAV );
                opt_ypiezo_AV.putVar( &xsmres.YPiezoAV );
                opt_zpiezo_AV.putVar( &xsmres.ZPiezoAV );

                contrast.putVar( &scan->data.display.contrast); 
                bright  .putVar( &scan->data.display.bright); 

                progress_info  << "Completed with minimal data set." << std::endl;

                // Minimalsatz Variablen endet hier =================================

                // Additional Parameters are following ...

                // Display...
                if (!display_vrange_z.isNull ())  display_vrange_z.putVar( &scan->data.display.vrange_z);
                if (!display_voffset_z.isNull ()) display_voffset_z.putVar( &scan->data.display.voffset_z);

                t_start.putVar( &scan->data.s.tStart );
                t_end  .putVar( &scan->data.s.tEnd );

                viewmode.putVar( &scan->data.display.ViewFlg );

                basename.putVar ({0}, {basenamed.getSize ()}, scan->data.ui.originalname);

                // only if needed
                if(!energy.isNull ())   energy  .putVar( &scan->data.s.Energy );
                if(!gatetime.isNull ()) gatetime.putVar( &scan->data.s.GateTime );
                if(!z_high.isNull ())   z_high  .putVar( &scan->data.display.z_high );
                if(!z_low.isNull ())    z_low   .putVar( &scan->data.display.z_low );
                if(!spaorgx.isNull ())  spaorgx .putVar( &scan->data.s.SPA_OrgX );
                if(!spaorgy.isNull ())  spaorgy .putVar( &scan->data.s.SPA_OrgY );

                main_get_gapp ()->progress_info_set_bar_text ("Scan-Events", 2);
                main_get_gapp ()->progress_info_set_bar_fraction (0.7, 1);

                // write attached Events, if any
                progress_info  << "Writing Scan, User & Probe Events." << std::endl;
                scan->mem2d->WriteScanEvents (nc);

                // store LayerInformation / OSD
                {
                        int mdd=1;
                        int mdf=1;
                        int mdfo=1;
                        int mdli=1;
                        int mdliv=1;
                        int nt=1;
                        main_get_gapp ()->progress_info_set_bar_text ("LayerInformation", 2);
                        main_get_gapp ()->progress_info_set_bar_fraction (0.8, 1);
                        main_get_gapp ()->progress_info_set_bar_fraction (0.1, 2);
                        // pass one, eval max dimensions -- NetCDF requires rectangual shape over all dimensions
                        if (scan->data.s.ntimes > 1){
                                nt = scan->data.s.ntimes;
                                for (int time_index=0; time_index<nt; ++time_index){
                                        progress_info  << "Eval Max Dim for Writing Layer Info OSD #ti " << time_index<< std::endl;
                                        scan->mem2d_time_element(time_index)->eval_max_sizes_layer_information (&mdd, &mdf, &mdfo, &mdli, &mdliv);
                                }
                        } else {
                                progress_info  << "Eval Max Dim for Writing Layer Info OSD" << std::endl;
                                scan->mem2d->eval_max_sizes_layer_information (&mdd, &mdf, &mdfo, &mdli, &mdliv);
                        }
                        main_get_gapp ()->progress_info_set_bar_fraction (0.2, 2);
                        if (mdd>1){
                                NcVar ncv_d;
                                NcVar ncv_f;
                                NcVar ncv_fo;
                                NcVar ncv_v;
                                progress_info  << "Storing Layer Info" << std::endl;
                                scan->mem2d->start_store_layer_information (nc, ncv_d, ncv_f, ncv_fo, ncv_v,
                                                                            nt, mdliv, mdli, mdd, mdf, mdfo);
                                if (nt > 1)
                                        for (int time_index=0; time_index<nt; ++time_index){
                                                scan->mem2d_time_element(time_index)->store_layer_information (ncv_d, ncv_f, ncv_fo, ncv_v, time_index);
                                                main_get_gapp ()->progress_info_set_bar_fraction (0.2+0.8*((double)time_index/nt), 2);
                                        }
                                else
                                        scan->mem2d->store_layer_information (ncv_d, ncv_f, ncv_fo, ncv_v, 0);
                        }
                }

                // Signal PlugIns
                progress_info  << "Signaling PlugIns to add Custom/HwI Data." << std::endl;

                main_get_gapp ()->progress_info_set_bar_text ("PlugIn-Data", 2);
                main_get_gapp ()->progress_info_set_bar_fraction (0.9, 1);
                main_get_gapp ()->progress_info_set_bar_fraction (0., 2);

                main_get_gapp ()->SignalCDFSaveEventToPlugins (nc);

                main_get_gapp ()->progress_info_set_bar_fraction (1.0, 1);

                // close of nc takes place in destructor
                progress_info  << " ** COMPLETED WRITING NETCDF DATA **" << std::endl;
                main_get_gapp ()->progress_info_set_bar_text ("Finishing", 2);
                scan->draw ();

                main_get_gapp ()->progress_info_close ();
                error_status << "Write Completed." << std::endl;

                main_get_gapp ()->progress_info_close ();
                return status = FIO_OK;
                
        } catch (const netCDF::exceptions::NcException& e) {
                std::cerr << "EE: NetCDF File Write Error. Catch " << e.what() << std::endl;
                error_status << "EE: NetCDF File Write Error. Catch " << e.what() << std::endl;

                error_status << "************************************************" << std::endl;
                error_status << "PROGRESS REPORT" << std::endl;
                error_status << "************************************************" << std::endl;
                error_status << progress_info.str().c_str() << std::endl;
 
                main_get_gapp ()->progress_info_close ();
		return status = FIO_ERROR_CATCH;
        }
}

