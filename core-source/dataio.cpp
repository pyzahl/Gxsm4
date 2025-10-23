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

// NEW 20180430PY
// =====================================================================================
// set scaling, apply load time scale correction -- as set in preferences
// make sure to set/keep this to 1.0,1.0,1.0 if NOT intending to adjust scan XYZ scale!
// applied to XYZ dimension, offset, scale/differentials and XY lookup

FIO_STATUS NetCDF::Read(xsm::open_mode mode){
	int i;

        try {
                std::string filename = name;
                // Open the NetCDF file in read mode
                netCDF::NcFile nc(filename, netCDF::NcFile::read);
                
                // try Topo Scan
                netCDF::NcVar Data; 

                main_get_gapp ()->progress_info_new ("NetCDF Read Progress", 2);
                main_get_gapp ()->progress_info_set_bar_fraction (0., 1);
                main_get_gapp ()->progress_info_set_bar_text (name, 1);
                main_get_gapp ()->SetStatus (N_("Loading... "), name);
                main_get_gapp ()->monitorcontrol->LogEvent(N_("Loading... "), name);

                ZD_TYPE zdata_typ=ZD_SHORT; // used by "H" -- Topographic STM/AFM data, historic default

                if (! (Data = nc.getVar("H")).isNull ())
                        main_get_gapp ()->progress_info_set_bar_text ("H: Historic SHORT type data", 2), zdata_typ=ZD_SHORT;
                else if (! (Data = nc.getVar("Intensity")).isNull ()) // try Diffract Scan data, aka D2D
                        main_get_gapp ()->progress_info_set_bar_text ("H: Type LONG", 2), zdata_typ=ZD_LONG; // used by "Intensity" -- diffraction counts
                else if ((Data = nc.getVar("FloatField")).isNull ()) // try Float Data, all new hardware uses Float Type
                 	main_get_gapp ()->progress_info_set_bar_text ("H: Type FLOAT", 2), zdata_typ=ZD_FLOAT;
                else if ((Data = nc.getVar("DoubleField")).isNull ()) // try Double Data
                 	main_get_gapp ()->progress_info_set_bar_text ("H: Type DOUBLE", 2), zdata_typ=ZD_DOUBLE;
                else if ((Data = nc.getVar("ByteField")).isNull ())        // Try Byte
                        main_get_gapp ()->progress_info_set_bar_text ("H: Type BYTE", 2), zdata_typ=ZD_BYTE;
                else if ((Data = nc.getVar("ComplexDoubleField")).isNull ()) // Try Complex
                        main_get_gapp ()->progress_info_set_bar_text ("H: Type COMPLEX", 2), zdata_typ=ZD_COMPLEX;
                else if ((Data = nc.getVar("RGBA_ByteField")).isNull ()) // Try RGBA Byte / Color Image Type
                        main_get_gapp ()->progress_info_set_bar_text ("H: Type RGBA", 2), zdata_typ=ZD_RGBA;
                else { // failed looking for Gxsm data!!
                        std::cerr << "NetCDF file does not contain any Gxsm SPM data fields named: H, Intensity, FloatField, DoubleField, Byte, Compelx nor RGBA."
                                  << std::endl;

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

                // check integrity
                switch (Data.getType().getId()){
                case NC_SHORT: if (zdata_typ != ZD_SHORT) { std::cerr << "EE: NetCDF data field type is not matching expected type NC_SHORT."   << std::endl; return FIO_OPEN_ERR; } else break;
                case NC_INT:   std::cerr << "INFO: ?? NetCDF data field type is NC_INT." << std::endl; break;
                case NC_INT64: if (zdata_typ != ZD_LONG)   { std::cerr << "EE: NetCDF data field type is not matching expected type NC_LONG."   << std::endl; return FIO_OPEN_ERR; } else break;
                case NC_FLOAT: if (zdata_typ != ZD_FLOAT)  { std::cerr << "EE: NetCDF data field type is not matching expected type NC_FLOAT."  << std::endl; return FIO_OPEN_ERR; } else break;
                case NC_DOUBLE:if (zdata_typ != ZD_DOUBLE) { std::cerr << "EE: NetCDF data field type is not matching expected type NC_DOUBLE." << std::endl; return FIO_OPEN_ERR; } else break;
                case NC_BYTE:  if (zdata_typ != ZD_BYTE)   { std::cerr << "EE: NetCDF data field type is not matching expected type NC_BYTE."   << std::endl; return FIO_OPEN_ERR; } else break;
                case NC_CHAR:  std::cerr << "INFO: ?? NetCDF data field type is NC_CHAR." << std::endl; break;
                default:
                        std::cerr << "NetCDF data field type is invald/not supported." << std::endl;
                        return FIO_OPEN_ERR;
                }

                main_get_gapp ()->progress_info_set_bar_text ("Image Data", 2);
                main_get_gapp ()->progress_info_set_bar_fraction (0.25, 1);

                std::vector<netCDF::NcDim> dims = Data.getDims();
                std::cout << "Dimensions of data:" << std::endl;
                std::cout << "  #Dims: " << dims.size() << std::endl;
                for (const auto& dim : dims) {
                        std::cout << "  Name: " << dim.getName() << ", Length: " << dim.getSize() << std::endl;
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

                scan->mem2d->Resize(scan->data.s.nx, scan->data.s.ny, scan->data.s.nvalues, zdata_typ);

                int ntimes_tmp = scan->data.s.ntimes;

                NcVar ncv_d;
                NcVar ncv_f;
                NcVar ncv_fo;
                NcVar ncv_v;
                
                scan->mem2d->start_read_layer_information (nc, ncv_d, ncv_f, ncv_fo, ncv_v);

                for (int time_index=0; time_index < ntimes_tmp; ++time_index){
                        main_get_gapp ()->progress_info_set_bar_fraction ((gdouble)time_index/(gdouble)scan->data.s.ntimes, 2);

                        scan->mem2d->data->NcGet (Data, time_index);
                        scan->data.s.ntimes = ntimes_tmp;

                        if (scan->data.s.nvalues > 1){
                                XSM_DEBUG(DBG_L2, "reading valued arr... n=" << scan->data.s.nvalues );
                                float *valuearr = new float[scan->data.s.nvalues];
                                if(!valuearr)
                                        return status = FIO_NO_MEM;
		  
                                XSM_DEBUG(DBG_L2, "getting array..." );
                                nc.getVar("value").getVar ({scan->data.s.nvalues}, valuearr );
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

                        float *dimsx = new float[dimxd.getSize ()];
                        if(!dimsx)
                                return status = FIO_NO_MEM;

                        //--OffRotFix-- -> check for old version and fix offset (not any longer in Lookup)
	
                        double Xoff = 0.;
                        double Yoff = 0.;
                        if (!nc.getVar("dimx").getAtt("extra_info").isNull ()){ // if this attribut is present, then no offset correction!
                                nc.getVar("offsetx").getVar(&Xoff);
                                nc.getVar("offsety").getVar(&Yoff);
                        }

                        nc.getVar("dimx").getVar({dimxd.getSize()}, dimsx);
                        for(i=0; i<dimxd.getSize (); i++)
                                scan->mem2d->data->SetXLookup(i, (dimsx[i]-Xoff) * xsmres.LoadCorrectXYZ[0]);

                        delete [] dimsx;

                        float *dimsy = new float[dimyd.getSize ()];
                        if(!dimsy)
                                return status = FIO_NO_MEM;

                        nc.getVar("dimy").getVar({dimyd.getSize ()}, dimsy);
                        for(i=0; i<dimyd.getSize (); i++)
                                scan->mem2d->data->SetYLookup(i, (dimsy[i]-Yoff) * xsmres.LoadCorrectXYZ[1]);

                        delete [] dimsy;

                        // try sranger (old)
                        NC_GET_VARIABLE ("sranger_hwi_bias", &scan->data.s.Bias);
                        NC_GET_VARIABLE ("sranger_hwi_voltage_set_point", &scan->data.s.SetPoint);
                        NC_GET_VARIABLE ("sranger_hwi_current_set_point", &scan->data.s.Current);

                        // read back layer informations
                        scan->mem2d->remove_layer_information ();

                        if (!ncv_d.isNull () && !ncv_f.isNull () && !ncv_fo.isNull () && !ncv_v.isNull ())
                                scan->mem2d->add_layer_information (ncv_d, ncv_f, ncv_fo, ncv_v, time_index);
                        else {
                                nc.getVar ("rangex").getVar (&scan->data.s.rx); scan->data.s.rx *= xsmres.LoadCorrectXYZ[0];
                                nc.getVar ("rangey").getVar (&scan->data.s.ry); scan->data.s.ry *= xsmres.LoadCorrectXYZ[1];
                                NC_GET_VARIABLE ("sranger_hwi_bias", &scan->data.s.Bias);
                                NC_GET_VARIABLE ("sranger_hwi_voltage_set_point", &scan->data.s.SetPoint);
                                NC_GET_VARIABLE ("sranger_hwi_current_set_point", &scan->data.s.Current);
                                // mk2/3 try also
                                NC_GET_VARIABLE ("sranger_mk2_hwi_bias", &scan->data.s.Bias);
                                NC_GET_VARIABLE ("sranger_mk2_hwi_mix0_set_point", &scan->data.s.Current);
                                NC_GET_VARIABLE ("sranger_mk2_hwi_z_setpoint", &scan->data.s.ZSetPoint);
                                scan->mem2d->add_layer_information (new LayerInformation ("Bias", scan->data.s.Bias, "%5.2f V"));
                                scan->mem2d->add_layer_information (new LayerInformation (scan->data.ui.dateofscan));
                                scan->mem2d->add_layer_information (new LayerInformation ("X-size", scan->data.s.rx, "Rx: %5.1f \303\205"));
                                scan->mem2d->add_layer_information (new LayerInformation ("Y-size", scan->data.s.ry, "Ry: %5.1f \303\205"));
                                if (IS_AFM_CTRL)
                                        scan->mem2d->add_layer_information (new LayerInformation ("SetPoint", scan->data.s.SetPoint, "%5.2f V"));
                                else{
                                        scan->mem2d->add_layer_information (new LayerInformation ("Current", scan->data.s.Current, "%5.2f nA"));
                                        scan->mem2d->add_layer_information (new LayerInformation ("Current", main_get_gapp ()->xsm->data.s.Current*1000, "%5.1f pA"));
                                }
                        }

                        if (ntimes_tmp > 1){ // do not do this if only one time dim.
                                double ref_time=(double)(time_index);
                                if (!nc.getVar("time").isNull ()){
                                        NcVar rt = nc.getVar("time");
                                        rt.getVar({time_index}, {1}, &ref_time);

                                        //rt.set_cur(time_index);
                                        //rt->get(&ref_time, 1);
                                        scan->mem2d->add_layer_information (new LayerInformation ("t", ref_time, "%.2f s"));
                                }
                                scan->append_current_to_time_elements (time_index, ref_time); // need to add lut
                        }
                }

                main_get_gapp ()->progress_info_set_bar_text ("Variables", 2);
                main_get_gapp ()->progress_info_set_bar_fraction (0.25, 1);

                {
                        NcVarAtt unit_att = nc.getVar("time").getAtt("unit");
                        if (!unit_att.isNull()){
                                NcVarAtt label_att = nc.getVar("time").getAtt("label");
                                if (!label_att.isNull ()){
                                        NcValues unit  = unit_att.values();
                                        NcValues label = label_att.values();
                                        UnitObj *u = main_get_gapp ()->xsm->MakeUnit (unit->as_string(0), label->as_string(0));
                                        scan->data.SetTimeUnit(u);
                                        delete u;
                                }
                        }
                }

                if ((unit_att = nc.getVar("value")->get_att("unit"))){
                        if ((label_att = nc.getVar("value").getAtt("label"))){
                                NcValues *unit  = unit_att->values();
                                NcValues *label = label_att->values();
                                UnitObj *u = main_get_gapp ()->xsm->MakeUnit (unit->as_string(0), label->as_string(0));
                                scan->data.SetVUnit(u);
                                delete unit;
                                delete label;
                                delete u;
                                delete label_att;
                                label_att = NULL;
                        }
                        delete unit_att;
                        unit_att = NULL;
                }

                nc.getVar("rangex")->get(&scan->data.s.rx); scan->data.s.rx *= xsmres.LoadCorrectXYZ[0];
                if ((unit_att = nc.getVar("rangex").getAtt("unit"))){
                        if ((label_att = nc.getVar("rangex").getAtt("label"))){
                                NcValues *unit  = unit_att->values();
                                NcValues *label = label_att->values();
                                UnitObj *u = main_get_gapp ()->xsm->MakeUnit (unit->as_string(0), label->as_string(0));
                                scan->data.SetXUnit(u);
                                delete unit;
                                delete label;
                                delete u;
                                delete label_att;
                                label_att = NULL;
                        }
                        delete unit_att;
                        unit_att = NULL;
                }
                nc.getVar("rangey")->get(&scan->data.s.ry); scan->data.s.ry *= xsmres.LoadCorrectXYZ[1];
                if ((unit_att = nc.getVar("rangey").getAtt("unit"))){
                        if ((label_att = nc.getVar("rangey").getAtt("label"))){
                                NcValues *unit  = unit_att->values();
                                NcValues *label = label_att->values();
                                UnitObj *u = main_get_gapp ()->xsm->MakeUnit (unit->as_string(0), label->as_string(0));
                                scan->data.SetYUnit(u);
                                delete unit;
                                delete label;
                                delete u;
                                delete label_att;
                                label_att = NULL;
                        }
                        delete unit_att;
                        unit_att = NULL;
                }

                // Check Z daya type via Z label
                double LoadCorrectZ = 1.0;
                g_message ("NetCDF GXSM data type: %s", scan->data.ui.type);
                if ((unit_att = Data.getAtt("ZLabel"))){
                        NcValues *label = unit_att->values();

                        g_message ("User NetCDF Load Correct Z scaling check type: Data is '%s'.", label->as_string(0));

                        if (!strcmp (label->as_string(0), "Z"))
                                LoadCorrectZ = xsmres.LoadCorrectXYZ[2];
                        else
                                LoadCorrectZ = xsmres.LoadCorrectXYZ[3];
                
                        delete label;
                        delete unit_att;
                        unit_att = NULL;
                }


                if (fabs (xsmres.LoadCorrectXYZ[0] - 1.0) > 0.001)
                        g_message ("User NetCDF Load Correct X scaling is set to %g.", xsmres.LoadCorrectXYZ[0]);
                if (fabs (xsmres.LoadCorrectXYZ[1] - 1.0) > 0.001)
                        g_message ("User NetCDF Load Correct Y scaling is set to %g.", xsmres.LoadCorrectXYZ[1]);
                if (fabs (LoadCorrectZ - 1.0) > 0.001)
                        g_message ("User NetCDF Load Correct Z scaling is set to %g.", LoadCorrectZ);
        
                nc.getVar("rangez")->get(&scan->data.s.rz); scan->data.s.rz *= LoadCorrectZ;
                if ((unit_att = nc.getVar("rangez").getAtt("unit"))){
                        if ((label_att = nc.getVar("rangez").getAtt("label"))){
                                NcValues *unit  = unit_att->values();
                                NcValues *label = label_att->values();
                                UnitObj *u = main_get_gapp ()->xsm->MakeUnit (unit->as_string(0), label->as_string(0));
                                scan->data.SetZUnit(u);
                                delete unit;
                                delete label;
                                delete u;
                                delete label_att;
                                label_att = NULL;
                        }
                        delete unit_att;
                        unit_att = NULL;
                }

                nc.getVar("dx")->get(&scan->data.s.dx); scan->data.s.dx *= xsmres.LoadCorrectXYZ[0];
                nc.getVar("dy")->get(&scan->data.s.dy); scan->data.s.dy *= xsmres.LoadCorrectXYZ[1];
                nc.getVar("dz")->get(&scan->data.s.dz); scan->data.s.dz *= LoadCorrectZ;
                nc.getVar("offsetx")->get(&scan->data.s.x0); scan->data.s.x0 *= xsmres.LoadCorrectXYZ[0];
                nc.getVar("offsety")->get(&scan->data.s.y0); scan->data.s.y0 *= xsmres.LoadCorrectXYZ[1];
                nc.getVar("alpha")->get(&scan->data.s.alpha);

                nc.getVar("contrast")->get(&scan->data.display.contrast);
                nc.getVar("bright")->get(&scan->data.display.bright);

                scan->data.display.vrange_z = Contrast_to_VRangeZ (scan->data.display.contrast, scan->data.s.dz);

                //#define MINIMAL_READ_NC
#ifndef MINIMAL_READ_NC
                g_message ("NetCDF read optional informations");

                NcVar *comment = nc.getVar("comment");
                if (comment){
                        G_NEWSIZE(scan->data.ui.comment, 1+comment->get_dim(0)->size());
                        memset (scan->data.ui.comment, 0, 1+comment->get_dim(0)->size());
                        comment->get(scan->data.ui.comment, comment->get_dim(0)->size());
                }

                NcVar *type = nc.getVar("type"); 
                if (type){
                        G_NEWSIZE(scan->data.ui.type, 1+type->get_dim(0)->size());
                        memset (scan->data.ui.type, 0, 1+type->get_dim(0)->size());
                        type->get(scan->data.ui.type, type->get_dim(0)->size());
                }
                NcVar *username = nc.getVar("username");
                if (username){
                        G_NEWSIZE(scan->data.ui.user, username->get_dim(0)->size());
                        username->get(scan->data.ui.user, username->get_dim(0)->size());
                }
                NcVar *dateofscan = nc.getVar("dateofscan");
                if(dateofscan){
                        G_NEWSIZE(scan->data.ui.dateofscan, dateofscan->get_dim(0)->size());
                        dateofscan->get(scan->data.ui.dateofscan, dateofscan->get_dim(0)->size());
                }

                if(nc.getVar("t_start")) nc.getVar("t_start")->get(&scan->data.s.tStart);
                if(nc.getVar("t_end")) nc.getVar("t_end")->get(&scan->data.s.tEnd);
                // restore last view mode
                if(nc.getVar("viewmode")){ 
                        nc.getVar("viewmode")->get(&scan->data.display.ViewFlg); 
                        scan->SetVM(scan->data.display.ViewFlg);
                } 
                if(nc.getVar("vrange_z")) nc.getVar("vrange_z")->get(&scan->data.display.vrange_z);
                if(nc.getVar("voffset_z")) nc.getVar("voffset_z")->get(&scan->data.display.voffset_z);

                if(nc.getVar("basename")){
                        int len;
                        G_NEWSIZE(scan->data.ui.originalname, len=nc.getVar("basename")->get_dim(0)->size());
                        nc.getVar("basename")->get(scan->data.ui.originalname, len);
                        XSM_DEBUG (DBG_L2, "got original name:" << scan->data.ui.originalname);
                }
                else{
                        scan->data.ui.SetOriginalName ("original name is not available");
                        XSM_DEBUG (DBG_L2, "original name is not available");
                }
                if(nc.getVar("energy"  )) nc.getVar("energy"  )->get(&scan->data.s.Energy);
                if(nc.getVar("gatetime")){
                        nc.getVar("gatetime")->get(&scan->data.s.GateTime );
                        scan->data.s.dz = 1./scan->data.s.GateTime;
                }
                if(nc.getVar("cnttime")){ // overrides above
                        nc.getVar("cnttime")->get(&scan->data.s.GateTime);
                        scan->data.s.dz = 1./scan->data.s.GateTime;
                }
                if(nc.getVar("cnt_high" )){
                        nc.getVar("cnt_high" )->get(&scan->data.display.z_high);
                }
                if(nc.getVar("z_high" )){
                        nc.getVar("z_high" )->get(&scan->data.display.z_high);
                }
                if(nc.getVar("cps_high" )) nc.getVar("cps_high" )->get(&scan->data.display.z_high);

                if(nc.getVar("cnt_low"  )){
                        nc.getVar("cnt_low"  )->get(&scan->data.display.z_low);
                }
                if(nc.getVar("z_low"  )){
                        nc.getVar("z_low"  )->get(&scan->data.display.z_low);
                }
                if(nc.getVar("cps_low" )) nc.getVar("cps_low" )->get(&scan->data.display.z_low);

                if(nc.getVar("spa_orgx"  )) nc.getVar("spa_orgx"  )->get(&scan->data.s.SPA_OrgX);
                if(nc.getVar("spa_orgy"  )) nc.getVar("spa_orgy"  )->get(&scan->data.s.SPA_OrgY);

                //  if(nc.getVar("")) nc.getVar("")->get(&scan->data.hardpars.);

                // load attached Events, if any
                main_get_gapp ()->progress_info_set_bar_fraction (0.5, 1);
                main_get_gapp ()->progress_info_set_bar_text ("Events", 2);
                scan->mem2d->LoadScanEvents (&nc);

                // Signal PlugIns
                main_get_gapp ()->progress_info_set_bar_fraction (0.75, 1);
                main_get_gapp ()->progress_info_set_bar_text ("Plugin Data", 2);
                main_get_gapp ()->SignalCDFLoadEventToPlugins (&nc);
	
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
                main_get_gapp ()->progress_info_set_bar_text ("Finishing", 2);
                main_get_gapp ()->progress_info_set_bar_fraction (1., 1);
                main_get_gapp ()->progress_info_close ();
                return status = FIO_OK; 

                

                return NC_READ_OK;
        } catch (const netCDF::exceptions::NcException& e) {
                std::cerr << "EE: NetCDF File Error: " << e.what() << std::endl;
		return status = FIO_OPEN_ERR;
        }
}


#define ADD_NC_ATTRIBUTE_ANG(nv, unit)\
	do{\
		nv.putAtt("Info", "This number is alwalys stored in Angstroem. Unit is used for user display only.");\
		nv.putAtt("label", unit->Label());	\
		nv.putAtt("var_unit", "Ang");		\
		if (unit->Alias())			\
			nv->putAtt("unit", unit->Alias());	\
		else						\
			nv->putAtt("unitSymbol", unit->Symbol());	\
	}while(0)


FIO_STATUS NetCDF::Write(){
	XSM_DEBUG (DBG_L2, "NetCDF::Write");
	// name convention NetCDF: *.nc

	NcError ncerr(NcError::verbose_nonfatal);
	NcFile nc(name, NcFile::Replace); // ,NULL,0,NcFile::Netcdf4); // Create, leave in define mode
        // use 64 byte offset mode if(ntimes > 2) , FileFormat=NcFile::Offset64Bits
   
	scan->data.ui.SetName(name);

	// Check if the file was opened successfully
	if (! nc.is_valid())
		return status = FIO_OPEN_ERR;

	main_get_gapp ()->progress_info_new ("NetCDF Write Progress",2);
	main_get_gapp ()->progress_info_set_bar_fraction (0., 1);
	main_get_gapp ()->progress_info_set_bar_text (name, 1);
	main_get_gapp ()->progress_info_set_bar_fraction (0., 2);

	XSM_DEBUG (DBG_L2, "NetCDF::Write-> open OK");

	// Create dimensions
#define NTIMES   1
#define TIMELEN 40
#define VALUEDIM 1
	NcDim timed  = nc.addDim("time", scan->data.s.ntimes);        // Time of Scan
	NcDim valued = nc.addDim("value", scan->data.s.nvalues);     // Value of Scan, eg. Volt / Force
	NcDim dimxd  = nc.addDim("dimx", scan->mem2d->GetNx()); // Image Dim. in X (#Samples)
	NcDim dimyd  = nc.addDim("dimy", scan->mem2d->GetNy()); // Image Dim. in Y (#Samples)
  
	NcDim reftimed  = nc.addDim("reftime", TIMELEN);  // Starting time
  
	NcDim commentd    = nc.addDim("comment", strlen(scan->data.ui.comment)+1);       // Comment
	NcDim titled      = nc.addDim("title", strlen(scan->data.ui.title)+1);           // Title
	NcDim typed       = nc.addDim("type", strlen(scan->data.ui.type)+1);           // Type
	NcDim usernamed   = nc.addDim("username", strlen(scan->data.ui.user)+1);         // Username
	NcDim dateofscand = nc.addDim("dateofscan", strlen(scan->data.ui.dateofscan)+1); // Scandate

	XSM_DEBUG (DBG_L2, "NetCDF::Write-> Creating Data Var");
	// Create variables and their attributes
	NcVar* Data;
	switch(scan->mem2d->GetTyp()){
	case ZD_BYTE:
		Data = nc.addVar("ByteField", ncByte, timed, valued, dimyd, dimxd);
		Data->putAtt("long_name", "BYTE: 8bit data field");
		break;
	case ZD_SHORT:
		Data = nc.addVar("H", ncShort, timed, valued, dimyd, dimxd);
		Data->putAtt("long_name", "SHORT: signed 16bit data field)");
		break;
	case ZD_LONG: 
		Data = nc.addVar("Intensity", ncLong, timed, valued, dimyd, dimxd);
		Data->putAtt("long_name", "LONG: signed 32bit data field");
		break;
	case ZD_FLOAT: 
		Data = nc.addVar("FloatField", ncFloat, timed, valued, dimyd, dimxd);
		Data->putAtt("long_name", "FLOAT: single precision floating point data field");
		break;
	case ZD_DOUBLE: 
		Data = nc.addVar("DoubleField", ncDouble, timed, valued, dimyd, dimxd);
		Data->putAtt("long_name", "DOUBLE: souble precision floating point data field");
		break;
	case ZD_COMPLEX: 
		Data = nc.addVar("ComplexDoubleField", ncDouble, timed, valued, dimyd, dimxd);
		Data->putAtt("long_name", "complex (abs,re,im) double data field");
		break;
	case ZD_RGBA: 
		Data = nc.addVar("RGBA_ByteField", ncDouble, timed, valued, dimyd, dimxd);
		Data->putAtt("long_name", "byte RGBA data field");
		break;
	default:
		return status = FIO_NO_NETCDFXSMFILE;
		break;
	}
	Data->putAtt("var_units_hint", "raw DAC/counter data. Unit is not defined here: multiply by dz-unit");

	XSM_DEBUG (DBG_L2, "NetCDF::Write-> Adding Units ZLab");

	Data->putAtt("ZLabel", scan->data.Zunit->Label());
	Data->putAtt("unit", scan->data.Zunit->Symbol());
	if (scan->data.Zunit->Alias())
		Data->putAtt("ZSrcUnit", scan->data.Zunit->Alias());
	else
		XSM_DEBUG(DBG_L2, "NetCDF::Write-> Warning: No Alias for Z unit, cannot restore unit on load" );
  
	XSM_DEBUG (DBG_L2, "NetCDF::Write-> Adding time, att");

	NcVar* time  = nc.addVar("time", ncDouble, timed);
	time->putAtt("long_name", "Time since reftime (actual time scanning) or other virtual in time changeing parameter");
	time->putAtt("short_name", "Scan Time");
	//time->putAtt("var_unit", "s"); // 20170130-PYZ-added-actual-unit-info
	time->putAtt("dimension_unit_info", "actual data dimension for SrcType in time element dimension");
	if (scan->data.TimeUnit){
		time->putAtt("label", scan->data.TimeUnit->Label());
		time->putAtt("unit", scan->data.TimeUnit->Symbol());
	}else{
		time->putAtt("label", "time");
		time->putAtt("unit", "s");
	}
  
  
	XSM_DEBUG (DBG_L2, "NetCDF::Write-> Adding Units Value");

	NcVar* value = nc.addVar("value", ncFloat, valued);
	value->putAtt("long_name", "Image Layers of SrcType at Values");
	if (scan->data.Vunit){
		value->putAtt("label", scan->data.Vunit->Label());
		value->putAtt("unit", scan->data.Vunit->Symbol());
	}else{
		value->putAtt("label", "Value");
		value->putAtt("unit", "A.U.");
	}
  
	XSM_DEBUG (DBG_L2, "NetCDF::Write-> dim x y reft");

	NcVar* dimx  = nc.addVar("dimx", ncFloat, dimxd);
	dimx->putAtt("long_name", "# Pixels in X, contains X-Pos Lookup");
	dimx->putAtt("extra_info", "X-Lookup without offset");
  
	NcVar* dimy  = nc.addVar("dimy", ncFloat, dimyd);
	dimy->putAtt("long_name", "# Pixels in Y, contains Y-Pos Lookup");
	dimy->putAtt("extra_info", "Y-Lookup without offset");
  
	NcVar* reftime  = nc.addVar("reftime", ncChar, reftimed);
	reftime->putAtt("long_name", "Reference time, i.e. Scan Start");
	reftime->putAtt("unit", "date string");
  

	XSM_DEBUG (DBG_L2, "NetCDF::Write-> Comment Title ...");

	NcVar* comment    = nc.addVar("comment", ncChar, commentd);
	NcVar* title      = nc.addVar("title", ncChar, titled);
	NcVar* type       = nc.addVar("type", ncChar, typed);
	NcVar* username   = nc.addVar("username", ncChar, usernamed);
	NcVar* dateofscan = nc.addVar("dateofscan", ncChar, dateofscand);
  
// This unit and label entries are used to restore the correct unit later!
	NcVar* rangex     = nc.addVar("rangex", ncDouble);
	ADD_NC_ATTRIBUTE_ANG(rangex, scan->data.Xunit);

	NcVar* rangey     = nc.addVar("rangey", ncDouble);
	ADD_NC_ATTRIBUTE_ANG(rangey, scan->data.Yunit);

	NcVar* rangez     = nc.addVar("rangez", ncDouble);
	ADD_NC_ATTRIBUTE_ANG(rangez, scan->data.Zunit);

	NcVar* dx         = nc.addVar("dx", ncDouble);
	ADD_NC_ATTRIBUTE_ANG(dx, scan->data.Xunit);

	NcVar* dy         = nc.addVar("dy", ncDouble);
	ADD_NC_ATTRIBUTE_ANG(dy, scan->data.Yunit);

	NcVar* dz         = nc.addVar("dz", ncDouble);
	ADD_NC_ATTRIBUTE_ANG(dz, scan->data.Zunit);

	NcVar* opt_xpiezo_AV = nc.addVar("opt_xpiezo_av", ncDouble);
	opt_xpiezo_AV->putAtt("type", "pure optional information, not used by Gxsm for any scaling after the fact. For the records only");
	opt_xpiezo_AV->putAtt("label", "Configured X Piezo Sensitivity");
	opt_xpiezo_AV->putAtt("unit", "Ang/V");

	NcVar* opt_ypiezo_AV = nc.addVar("opt_ypiezo_av", ncDouble);
	opt_ypiezo_AV->putAtt("type", "pure optional information, not used by Gxsm for any scaling after the fact. For the records only");
	opt_ypiezo_AV->putAtt("label", "Configured Y Piezo Sensitivity");
	opt_ypiezo_AV->putAtt("unit", "Ang/V");

	NcVar* opt_zpiezo_AV = nc.addVar("opt_zpiezo_av", ncDouble);
	opt_zpiezo_AV->putAtt("type", "pure optional information, not used by Gxsm for any scaling after the fact. For the records only");
	opt_zpiezo_AV->putAtt("label", "Configured Z Piezo Sensitivity");
	opt_zpiezo_AV->putAtt("unit", "Ang/V");

	NcVar* offsetx    = nc.addVar("offsetx", ncDouble);
	ADD_NC_ATTRIBUTE_ANG(offsetx, scan->data.Xunit);

	NcVar* offsety    = nc.addVar("offsety", ncDouble);
	ADD_NC_ATTRIBUTE_ANG(offsety, scan->data.Yunit);

	NcVar* alpha      = nc.addVar("alpha", ncDouble);
	alpha->putAtt("unit", "Grad");
	alpha->putAtt("label", "Rotation");
  
	NcVar* contrast   = nc.addVar("contrast", ncDouble);
	NcVar* bright     = nc.addVar("bright", ncDouble);

	XSM_DEBUG (DBG_L2, "NetCDF::Write-> Creating Options Vars");
	// optional...
	NcVar* display_vrange_z  = NULL;
	NcVar* display_voffset_z = NULL;

	if(!(IS_SPALEED_CTRL)){
		display_vrange_z = nc.addVar("vrange_z", ncDouble);
		display_vrange_z->putAtt("long_name", "View Range Z");
		ADD_NC_ATTRIBUTE_ANG(display_vrange_z, scan->data.Zunit);

		display_voffset_z = nc.addVar("voffset_z", ncDouble);
		display_voffset_z->putAtt("long_name", "View Offset Z");
		ADD_NC_ATTRIBUTE_ANG(display_voffset_z, scan->data.Zunit);
	}

	NcVar* t_start      = nc.addVar("t_start", ncLong);
	NcVar* t_end        = nc.addVar("t_end", ncLong);

	NcVar* viewmode     = nc.addVar("viewmode", ncLong);
	viewmode->putAtt("long_name", "last viewmode flag");

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
	NcDim* basenamed   = nc.add_dim("basename", strlen(scan->data.ui.originalname)+1);
	NcVar* basename    = nc.addVar("basename", ncChar, basenamed);


	// only for LEED like Instruments
	NcVar* energy   = NULL;
	NcVar* gatetime = NULL;
	NcVar* z_high  = NULL;
	NcVar* z_low   = NULL;
	NcVar* spaorgx  = NULL;
	NcVar* spaorgy  = NULL;
  
	if(IS_SPALEED_CTRL){
		energy   = nc.addVar("energy", ncDouble);
		energy->putAtt("unit", "eV");

		gatetime = nc.addVar("gatetime", ncDouble);
		gatetime->putAtt("unit", "s");

		z_high  = nc.addVar("cps_high", ncDouble);
		z_high->putAtt("unit", "CPS or raw Z");

		z_low   = nc.addVar("cps_low", ncDouble);
		z_low->putAtt("unit", "CPS or raw Z");

		spaorgx  = nc.addVar("spa_orgx", ncDouble);
		spaorgx->putAtt("unit", "V");

		spaorgy  = nc.addVar("spa_orgy", ncDouble);
		spaorgy->putAtt("unit", "V");
	}

	// ....
  
	// Global attributes
	nc.putAtt("Creator", PACKAGE);
	nc.putAtt("Version", VERSION);
	nc.putAtt("build_from", COMPILEDBYNAME);
	nc.putAtt("DataIOVer", 
		   "$Header: /home/ventiotec/gxsm-cvs/Gxsm-2.0/src/dataio.C,v 1.46 2013-02-04 19:19:36 zahl Exp $"
		);
	nc.putAtt("HardwareCtrlType", xsmres.HardwareType);
	nc.putAtt("HardwareConnectionDev", xsmres.DSPDev);
	nc.putAtt("InstrumentType", xsmres.InstrumentType);
	nc.putAtt("InstrumentName", xsmres.InstrumentName);
  
	// Start writing data, implictly leaves define mode

	XSM_DEBUG (DBG_L2, "NetCDF::Write-> NcPut Data");
	XSM_DEBUG (DBG_L2, "NC write: " << timed->size() << "x" << valued->size() << "x" << dimxd->size() << "x" << dimyd->size());
  
	main_get_gapp ()->progress_info_set_bar_text ("Image Data", 2);
	main_get_gapp ()->progress_info_set_bar_fraction (0.25, 1);
	main_get_gapp ()->progress_info_set_bar_fraction (0., 2);
	if (scan->data.s.ntimes == 1){
		double timearr[] = { (double)(scan->data.s.tEnd-scan->data.s.tStart) };
		scan->mem2d->data->NcPut (Data);
		time->put(timearr, NUM(timearr));
	} else
		for (int time_index=0; time_index<scan->data.s.ntimes; ++time_index){
			scan->mem2d_time_element(time_index)->data->NcPut (Data, time_index);
			time->set_cur (time_index);
			double ref_time = scan->mem2d_time_element(time_index)->get_frame_time ();
			time->put (&ref_time, 1);
			main_get_gapp ()->progress_info_set_bar_fraction ((gdouble)time_index/(gdouble)scan->data.s.ntimes, 2);
		}


	main_get_gapp ()->progress_info_set_bar_text ("Look-Ups", 2);
	main_get_gapp ()->progress_info_set_bar_fraction (0.5, 1);

	float *valuearr = new float[valued->size()];
	if(!valuearr)
		return status = FIO_NO_MEM;

	XSM_DEBUG (DBG_L2, "NetCDF::Write-> NcPut Lookups");
	for(int i=0; i<valued->size(); i++)
		valuearr[i] = scan->mem2d->data->GetVLookup(i);

	value->put(valuearr, valued->size());
	delete [] valuearr;

	int i;
	//  float x0=scan->data.s.x0-scan->data.s.rx/2.;
	float *dimsx = new float[dimxd->size()];
	if(!dimsx)
		return status = FIO_NO_MEM;

	for(i=0; i<dimxd->size(); i++)
		dimsx[i] = scan->mem2d->data->GetXLookup(i);

	dimx->put(dimsx, dimxd->size());
	delete [] dimsx;

	//  float y0=scan->data.s.y0;
	float *dimsy = new float[dimyd->size()];
	if(!dimsy)
		return status = FIO_NO_MEM;

	for(i=0; i<dimyd->size(); i++)
		dimsy[i] = scan->mem2d->data->GetYLookup(i);

	dimy->put(dimsy, dimyd->size());
	delete [] dimsy;

	main_get_gapp ()->progress_info_set_bar_text ("Scan-Parameter", 2);
	main_get_gapp ()->progress_info_set_bar_fraction (0.6, 1);

	XSM_DEBUG (DBG_L2, "NetCDF::Write-> NcPut Vars...");
	char* s = ctime(&scan->data.s.tStart);
	reftime->put(s, strlen(s));
  
	comment->put(scan->data.ui.comment, commentd->size());


	title->put(scan->data.ui.title, titled->size());
	type->put(scan->data.ui.type, typed->size());
	username->put(scan->data.ui.user, usernamed->size());
	dateofscan->put(scan->data.ui.dateofscan, dateofscand->size());

	rangex ->put( &scan->data.s.rx);
	rangey ->put( &scan->data.s.ry);
	rangez ->put( &scan->data.s.rz); 
	dx     ->put( &scan->data.s.dx); 
	dy     ->put( &scan->data.s.dy); 
	dz     ->put( &scan->data.s.dz); 
	offsetx->put( &scan->data.s.x0); 
	offsety->put( &scan->data.s.y0); 
	alpha  ->put( &scan->data.s.alpha);

	opt_xpiezo_AV->put( &xsmres.XPiezoAV );
	opt_ypiezo_AV->put( &xsmres.YPiezoAV );
	opt_zpiezo_AV->put( &xsmres.ZPiezoAV );

	contrast->put( &scan->data.display.contrast); 
	bright  ->put( &scan->data.display.bright); 

	// Minimalsatz Variablen endet hier =================================

	// Additional Parameters are following ...

	// Display...
	if(display_vrange_z) display_vrange_z->put( &scan->data.display.vrange_z);
	if(display_voffset_z) display_voffset_z->put( &scan->data.display.voffset_z);

	t_start->put( &scan->data.s.tStart );
	t_end  ->put( &scan->data.s.tEnd );

	viewmode->put( &scan->data.display.ViewFlg );

	basename->put(scan->data.ui.originalname, basenamed->size());

	// only if needed
	if(energy   ) energy  ->put( &scan->data.s.Energy );
	if(gatetime ) gatetime->put( &scan->data.s.GateTime );
	if(z_high  ) z_high ->put( &scan->data.display.z_high );
	if(z_low   ) z_low  ->put( &scan->data.display.z_low );
	if(spaorgx  ) spaorgx ->put( &scan->data.s.SPA_OrgX );
	if(spaorgy  ) spaorgy ->put( &scan->data.s.SPA_OrgY );

	main_get_gapp ()->progress_info_set_bar_text ("Scan-Events", 2);
	main_get_gapp ()->progress_info_set_bar_fraction (0.7, 1);

	// write attached Events, if any
	scan->mem2d->WriteScanEvents (&nc);

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
			for (int time_index=0; time_index<nt; ++time_index)
				scan->mem2d_time_element(time_index)->eval_max_sizes_layer_information (&mdd, &mdf, &mdfo, &mdli, &mdliv);
		} else
			scan->mem2d->eval_max_sizes_layer_information (&mdd, &mdf, &mdfo, &mdli, &mdliv);

		main_get_gapp ()->progress_info_set_bar_fraction (0.2, 2);
		if (mdd>1){
			NcVar* ncv_d=NULL;
			NcVar* ncv_f=NULL;
			NcVar* ncv_fo=NULL;
			NcVar* ncv_v=NULL;
			scan->mem2d->start_store_layer_information (&nc, &ncv_d, &ncv_f, &ncv_fo, &ncv_v,
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
	main_get_gapp ()->progress_info_set_bar_text ("PlugIn-Data", 2);
	main_get_gapp ()->progress_info_set_bar_fraction (0.9, 1);
	main_get_gapp ()->progress_info_set_bar_fraction (0., 2);

	main_get_gapp ()->SignalCDFSaveEventToPlugins (&nc);

	main_get_gapp ()->progress_info_set_bar_text ("Finishing", 2);
	main_get_gapp ()->progress_info_set_bar_fraction (1.0, 1);

	// close of nc takes place in destructor
	scan->draw ();

	// Check if the file was written successfully
	if ( ncerr.get_err() )
		return status = FIO_WRITE_ERR;


	main_get_gapp ()->progress_info_close ();

	return status = FIO_OK; 
}

