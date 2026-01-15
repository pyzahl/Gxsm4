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


#include "gxsm_app.h"

#include "unit.h"
#include "pcs.h"
#include "xsmtypes.h"
#include <gtk/gtk.h>
#include "glbvars.h"
#include "surface.h"

void callback_view( GtkWidget *widget, gpointer data ){
        XSM_DEBUG(DBG_L3, "cb_view");
        if (gapp && gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) 
                main_get_gapp ()->xsm->SetVM((long)data);
}

void cbtext( GtkEditable *widget, App *ap ){
        XSM_DEBUG(DBG_L3, "ui.comment changed!!");
        ap->ui_setcomment();
}

void cbbasename( GtkWidget *widget, App *ap ){
        //g_print ("xxxxxxxxxxxxxxxxxxx cbbasename new  %s\n",gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (widget)))));
        //g_print ("xxxxxxxxxxxxxxxxxxx cbbasename old %s\n",main_get_gapp ()->xsm->data.ui.basename );
        main_get_gapp ()->xsm->data.ui.SetBaseName (gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (widget)))));
        // ap->as_setdata();
}


void make_freeze_selected (){
        if (G_IS_OBJECT (main_get_gapp ()->spm_control)){
                if (main_get_gapp ()->xsm->IsMode (MODE_SETRANGE)){
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Rx"))->Freeze ();
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Rx"))->Freeze ();
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Ry"))->Freeze ();
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Stx"))->Thaw ();
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Sty"))->Thaw ();
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Pnx"))->Thaw ();
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Pny"))->Thaw ();
                } else if (main_get_gapp ()->xsm->IsMode (MODE_SETSTEPS)){
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Rx"))->Thaw ();
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Ry"))->Thaw ();
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Stx"))->Freeze ();
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Sty"))->Freeze ();
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Pnx"))->Thaw ();
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Pny"))->Thaw ();
                } else if (main_get_gapp ()->xsm->IsMode (MODE_SETPOINTS)){
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Rx"))->Thaw ();
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Ry"))->Thaw ();
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Stx"))->Thaw ();
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Sty"))->Thaw ();
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Pnx"))->Freeze ();
                        ((Gtk_EntryControl*)g_object_get_data (G_OBJECT (main_get_gapp ()->spm_control), "EC_Pny"))->Freeze ();
                }
        }
}

void cb_setmode( GtkWidget *widget, int data ){
        XSM_DEBUG(DBG_L3, "cb_setmode");
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) 
                main_get_gapp ()->xsm->SetModeFlg(data);
        else
                main_get_gapp ()->xsm->ClrModeFlg(data);
	
        if(IS_SPALEED_CTRL){
                main_get_gapp ()->spa_mode_switch_check();
                main_get_gapp ()->spa_SeV_unit_switch_check();
                main_get_gapp ()->spm_update_all();
        }

        make_freeze_selected ();
}

void App::spm_range_check(Param_Control* pcs, gpointer app){
        App* a = (App*)app;
        XSM_DEBUG(DBG_L3,  "range check" );
        XSM_Instrument *Inst = a->xsm->Inst;
        SCAN_DATA *data = &(a->xsm->data);
        int analysis_mode = 0;
        
        // check if we run in analysis mode, then do not check.
        if (!((App*)app)->xsm->hardware)
                analysis_mode = 1;
        else {
                const gchar *tmp = ((App*)app)->xsm->hardware->Info (0);
                if (strncmp (tmp, "Data Analysis mode", 18) == 0)
                        analysis_mode = 1;
        }

        if (analysis_mode){
                // g_print (">%s<=>%d", tmp, strncmp (tmp, "Data Analysis Mode", 18));
                // g_message ("Skipping range check. Analysis Mode.");
                if (a->xsm->IsMode(MODE_SETPOINTS)){
                        // Points from Range & dx
                        data->s.nx = 1 + R2INT(data->s.rx/data->s.dx);
                        data->s.ny = 1 + R2INT(data->s.ry/data->s.dy);
                } else if (a->xsm->IsMode(MODE_SETSTEPS)){
                        // dx from Range & Points
                        data->s.dx = data->s.rx/(data->s.nx-1);
                        data->s.dy = data->s.ry/(data->s.ny-1);
                } else {
                        // rx from dx & Points
                        data->s.rx = (data->s.nx-1) * data->s.dx;
                        data->s.ry = (data->s.ny-1) * data->s.dy;
                }
                data->s.dz = Inst->ZResolution();
                a->spm_update_all();
                return;
        }

        //        else g_message ("Checking ranges. HwI Mode.");
        
        if(IS_SPALEED_CTRL){
                // always calc steps
                data->s.dx = data->s.rx/(data->s.nx-1);
                // force dx==dy
                if(0){
                        data->s.dy = data->s.ry/(data->s.ny-1);
                        data->s.ry = data->s.dy*(data->s.ny-1);
                }else{
                        // always range from points and dx if ry!=0
                        if(data->s.ry > 0.){
                                data->s.dy = data->s.dx;
                                data->s.ry = (data->s.ny-1)*data->s.dy;
                        }else{
                                data->s.dy = 0.;
                                data->s.ry = 0.;
                        }
			
                }
        }else{
                if(a->xsm->IsMode(MODE_SETPOINTS)){
                        // Points from Range & dx
                        data->s.nx = 1 + R2INT(data->s.rx/data->s.dx);
                        data->s.dx = MAX(1,R2INT(data->s.dx/Inst->XResolution()))*Inst->XResolution();
                        data->s.rx = (data->s.nx-1) * data->s.dx;
			
                        if(data->s.dy>=Inst->YResolution()/2.){ // allow dy = 0 for special purposes ...
                                data->s.ny = 1 + R2INT(data->s.ry/data->s.dy);
                                data->s.dy = MAX(1,R2INT(data->s.dy/Inst->YResolution()))*Inst->YResolution();
                                data->s.ry = (data->s.ny-1) * data->s.dy;
                        }
                }else
                        if(a->xsm->IsMode(MODE_SETSTEPS)){
                                // dx from Range & Points
                                XSM_DEBUG(DBG_L3,  "fixpoints" );
                                data->s.dx = data->s.rx/(data->s.nx-1);
                                if (data->s.dx < (Inst->XResolution () / 16.)){
                                        data->s.dx = Inst->XResolution()/16.;
                                        data->s.rx = (data->s.nx-1) * data->s.dx;
                                }
                                //				data->s.dx = MAX(1,R2INT(data->s.dx/Inst->XResolution()))*Inst->XResolution();
                                //				data->s.rx = (data->s.nx-1) * data->s.dx;
				
                                data->s.dy = data->s.ry/(data->s.ny-1);
                                //				data->s.dy = MAX(1,R2INT(data->s.dy/Inst->YResolution()))*Inst->YResolution();
                                //				data->s.ry = (data->s.ny-1) * data->s.dy;
                        }else{
                                // rx from dx & Points
                                XSM_DEBUG(DBG_L3,  "fixsteps"  );
                                data->s.dx = MAX(1,R2INT(data->s.dx/Inst->XResolution()))*Inst->XResolution();
                                data->s.rx = (data->s.nx-1) * data->s.dx;
				
                                data->s.dy = MAX(1,R2INT(data->s.dy/Inst->YResolution()))*Inst->YResolution();
                                data->s.ry = (data->s.ny-1) * data->s.dy;
                        }
        }
        if(g_object_get_data ( G_OBJECT (a->spm_control), "EC_Rx")){
                ((Gtk_EntryControl*)g_object_get_data ( G_OBJECT (a->spm_control), "EC_Rx"))
                        ->setMax(a->xsm->XRangeMax());
                XSM_DEBUG(DBG_L3,  "Rx Max: " << a->xsm->XRangeMax() );
        }
        if(g_object_get_data ( G_OBJECT (a->spm_control), "EC_Ry")){
                ((Gtk_EntryControl*)g_object_get_data ( G_OBJECT (a->spm_control), "EC_Ry"))
                        ->setMax(a->xsm->YRangeMax());
                XSM_DEBUG(DBG_L3,  "Ry Max: " << a->xsm->YRangeMax() );
        }
        if(g_object_get_data ( G_OBJECT (a->spm_control), "EC_Stx"))
                ((Gtk_EntryControl*)g_object_get_data ( G_OBJECT (a->spm_control), "EC_Stx"))
                        ->setMax(a->xsm->XStepMax());
        if(g_object_get_data ( G_OBJECT (a->spm_control), "EC_Stx"))
                ((Gtk_EntryControl*)g_object_get_data ( G_OBJECT (a->spm_control), "EC_Stx"))
                        ->setMin(0., a->xsm->XStepMin(), "red");
        if(g_object_get_data ( G_OBJECT (a->spm_control), "EC_Sty"))
                ((Gtk_EntryControl*)g_object_get_data ( G_OBJECT (a->spm_control), "EC_Sty"))
                        ->setMax(a->xsm->YStepMax());
        if(g_object_get_data ( G_OBJECT (a->spm_control), "EC_Sty"))
                ((Gtk_EntryControl*)g_object_get_data ( G_OBJECT (a->spm_control), "EC_Sty"))
                        ->setMin(0., a->xsm->YStepMin(), "red");
	
        if(g_object_get_data ( G_OBJECT (a->spm_control), "EC_Offx")){
                ((Gtk_EntryControl*)g_object_get_data ( G_OBJECT (a->spm_control), "EC_Offx"))
                        ->setMax(a->xsm->XOffsetMax());
                ((Gtk_EntryControl*)g_object_get_data ( G_OBJECT (a->spm_control), "EC_Offx"))
                        ->setMin(a->xsm->XOffsetMin());
        }
        if(g_object_get_data ( G_OBJECT (a->spm_control), "EC_Offy")){
                ((Gtk_EntryControl*)g_object_get_data ( G_OBJECT (a->spm_control), "EC_Offy"))
                        ->setMax(a->xsm->YOffsetMax());
                ((Gtk_EntryControl*)g_object_get_data ( G_OBJECT (a->spm_control), "EC_Offy"))
                        ->setMin(a->xsm->YOffsetMin());
        }
	
        a->spm_update_all();
	
        // **=	data->hardpars.LS_dnx = R2INT(Inst->XA2Dig(data->s.dx));
        // **=	data->hardpars.LS_nx2scan = data->s.nx;
        data->s.dz = Inst->ZResolution();

        a->SignalSPMRangeEventToPlugins();
		
        if(IS_SPALEED_CTRL)
                a->spa_show_scan_time();
}

void App::spm_offset_check(Param_Control* pcs, gpointer app){
        App* a = (App*)app;
        XSM_DEBUG(DBG_L3,  "offset check"  );

        if (((App*)app)->xsm->hardware == NULL){
                XSM_DEBUG(DBG_L3,  "offset check -- no HwI yet set up -- no action."  );
                return;
        }

        XSM_Instrument *Inst = ((App*)app)->xsm->Inst;
        SCAN_DATA *data = &((App*)app)->xsm->data;
	
        // move to 0,0 in offset coordinate system (never rotated)
        if(IS_SPALEED_CTRL) // add additional offset
                ((App*)app)->xsm->hardware->SetOffset 
                        (R2INT(Inst->X0A2Dig(data->s.x0) 
                               + data->s.SPA_OrgX/Inst->X0Resolution()),
                         R2INT(Inst->Y0A2Dig(data->s.y0) 
                               + data->s.SPA_OrgY/Inst->Y0Resolution()));
        else
                ((App*)app)->xsm->hardware->SetOffset
                        (R2INT(Inst->X0A2Dig(data->s.x0)),
                         R2INT(Inst->Y0A2Dig(data->s.y0)));
	
        XSM_DEBUG(DBG_L3,  "offset check -- signal plugins"  );

        a->SignalSPMRangeEventToPlugins();

        XSM_DEBUG(DBG_L3,  "offset check -- moveto 0,0"  );

        if (!pcs){
                // move tip to center of scan ( 0,0 ) now also
                //data->s.sx = data->s.sy = 0.;
                while (((App*)app)->xsm->hardware->MovetoXY(0, 0)); // G-IDLE ME
        }

        ((App*)app)->xsm->hardware->SetAlpha(data->s.alpha);
        
        XSM_DEBUG(DBG_L3,  "offset check -- update all"  );
        
        ((App*)app)->spm_update_all();
        
        XSM_DEBUG(DBG_L3,  "offset check -- done."  );
}

void App::offset_to_preset_callback(GtkWidget* w, gpointer app){
         SCAN_DATA *data = &((App*)app)->xsm->data;

         if (!((App*)app)->spm_control)
                 return;
 
         if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (g_object_get_data (G_OBJECT((((App*)app)->spm_control)), "offset_lock_button"))))
                 return;

         double *xy = (double*)g_object_get_data (G_OBJECT(w), "preset_xy"); // [ preset box ix, iy (0,0=center), +/-# presets ]
         if (xy){
                 data->s.x0 = xy[0] * round(((App*)app)->xsm->XOffsetMax() / xy[2] / 10)*10;
                 data->s.y0 = xy[1] * round(((App*)app)->xsm->YOffsetMax() / xy[2] / 10)*10;
         } else {
                 data->s.x0 = data->s.y0 = 0.;
         }
         App::spm_offset_check(NULL, app);
         // ((App*)app)->spm_update_all();
}

void App::spm_scanpos_check(Param_Control* pcs, gpointer app){
        XSM_DEBUG(DBG_L3,  "offset check"  );
        XSM_Instrument *Inst = ((App*)app)->xsm->Inst;
        SCAN_DATA *data = &((App*)app)->xsm->data;
	
        // move to scan position relative to offset in scan coordinate system (may be rotated)
        while (((App*)app)->xsm->hardware->MovetoXY(R2INT(Inst->XA2Dig(data->s.sx)),
                                             R2INT(Inst->YA2Dig(data->s.sy)))
               ); // G-IDLE ME

        ((App*)app)->spm_update_all();
}

void App::spm_nlayer_update(Param_Control* pcs, gpointer app){
        //	XSM_DEBUG(DBG_L3,  "nlayer check"  );
        //	SCAN_DATA *data = &((App*)app)->xsm->data;
        //	XSM_DEBUG(DBG_L3, "nlayer check: " << data->lp.nvalues);

        //	GTK_ADJUSTMENT (((Gtk_EntryControl*)pcs)->GetAdjustment ())->upper = (gfloat)(data->lp.nvalues-1);
}

void App::spm_select_layer(Param_Control* pcs, gpointer app){
        gchar *valtxt = NULL;
        int i3d = main_get_gapp ()->xsm->data.display.vlayer;
        if (i3d < 0) 
                i3d = 0;

        //	XSM_DEBUG(DBG_L3, "Select Layer: " << i3d);

        if (((App*)app)->xsm->ActiveScan){
                double lastlayer = (double)(((App*)app)->xsm->ActiveScan->mem2d->GetNv ()-1);
                if (lastlayer > 1.){
                        if (pcs){
                                pcs->setMax (lastlayer);
                                gtk_adjustment_set_upper (GTK_ADJUSTMENT (((Gtk_EntryControl*)pcs)->GetAdjustment ()), (gfloat)lastlayer);
                        }
                }
                if (i3d < ((App*)app)->xsm->ActiveScan->mem2d->GetNv ()){
                        ((App*)app)->xsm->ActiveScan->mem2d->SetLayer (i3d);
                        ((App*)app)->xsm->SetRedraw();
                        if (((App*)app)->xsm->ActiveScan->data.Vunit)
                                valtxt = ((App*)app)->xsm->ActiveScan->data.Vunit->UsrString (
                                                                                              ((App*)app)->xsm->ActiveScan->mem2d->data->GetVLookup (i3d));
                        else
                                valtxt = g_strdup ("-U-");
                }else{
                        XSM_DEBUG(DBG_L3, "Sorry, requested layer is out of range!" );
                        valtxt = g_strdup("--");
                }


        }else{
                valtxt = g_strdup("*");
        }

        gtk_label_set_text
                ( GTK_LABEL ((GtkWidget*) g_object_get_data
                             ( G_OBJECT (((App*)app)->spm_control), "LayerSelectValue")),
                  valtxt
                  );
        g_free (valtxt);

        ((App*)app)->xsm->SetVM();
}

void App::spm_select_time(Param_Control* pcs, gpointer app){
        gchar *valtxt = NULL;
        int i4d = main_get_gapp ()->xsm->data.display.vframe;
        if (i4d < 0) 
                i4d = 0;


        if (((App*)app)->xsm->ActiveScan){
                int n_frames = ((App*)app)->xsm->ActiveScan->number_of_time_elements ();
                double lastframe = (double)(n_frames-1);
                if (lastframe > 1.){
                        if (pcs){
                                pcs->setMax (lastframe);
                                gtk_adjustment_set_upper (GTK_ADJUSTMENT (((Gtk_EntryControl*)pcs)->GetAdjustment ()), (gfloat)lastframe);
                        }
                }
                if (i4d < n_frames){
                        double t = ((App*)app)->xsm->ActiveScan->retrieve_time_element (i4d);
                        ((App*)app)->xsm->SetRedraw();
                        if (((App*)app)->xsm->ActiveScan->data.TimeUnit)
                                valtxt = ((App*)app)->xsm->ActiveScan->data.TimeUnit->UsrString (t);
                        else
                                valtxt = g_strdup ("???");
                }else{
                        valtxt = g_strdup("--");
                }


        }else{
                valtxt = g_strdup ("--");
        }

        gtk_label_set_text
                ( GTK_LABEL ((GtkWidget*) g_object_get_data
                             ( G_OBJECT (((App*)app)->spm_control), "TimeSelectValue")),
                  valtxt
                  );
        g_free (valtxt);
        ((App*)app)->xsm->SetVM();
}

void App::spa_energy_check(Param_Control* pcs, gpointer app){
        XSM_Instrument *Inst = ((App*)app)->xsm->Inst;
        SCAN_DATA *data = &((App*)app)->xsm->data;
	
        XSM_DEBUG(DBG_L3,  "energy changed !"  );

        // to be changed/removed in future of *-HwI-* redesign	
        // **=	data->hardpars.SPA_EnergyVolt = Inst->eV2V(data->hardpars.SPA_Energy); 
        data->s.Bias = Inst->eV2V(data->s.Energy); 
        ((App*)app)->xsm->hardware->SetUserParam (6, "ENERGY", data->s.Bias);
        // **=	((App*)app)->xsm->hardware->PutParameter(&data->hardpars);
	
        // update all Energy dependent Voltages if in BZ Mode:
        if(((App*)app)->xsm->IsMode(MODE_BZUNIT)){
                // **==		double etmp=data->hardpars.SPA_Energy;
                double etmp=Inst->eV2V(data->s.Energy); 
                XSM_DEBUG(DBG_L3,  "energy check: recalc new Volt from BZ"  );
                g_slist_foreach (
                                 (GSList*)g_object_get_data (
                                                             G_OBJECT (((App*)app)->spm_control), 
                                                             "SPA_EnergyDepend_list"),
                                 (GFunc) App::recalc_volt_from_new_Energy, &etmp);
        }
        // **==	((App*)app)->xsm->BZ_Unit->SetE(data->hardpars.SPA_Energy);
        ((App*)app)->xsm->BZ_Unit->SetE(data->s.Energy);
        ((App*)app)->spm_update_all();
}

void App::spa_show_scan_time(){
        double s,m,h;
        gchar *t;
        s = xsm->data.s.GateTime * xsm->data.s.nx*xsm->data.s.ny;
        h = floor(s/3600.);
        m = floor((s-h*3600.)/60.);
        s-= h*3600.+m*60.;
        SetStatus("approx Scantime", 
                  t=g_strdup_printf("%.0f:%02.0f:%02.0f",h,m,s));
        g_free(t);
}

void App::spa_gate_check(Param_Control* pcs, gpointer app){
        App *a = (App*)app;
        ((App*)app)->xsm->hardware->SetUserParam (10, "GATETIME", a->xsm->data.s.GateTime);

        //  a->xsm->CPSHiLoUnit->Change(a->xsm->data.display.cnttime); // only for hi/lo
        a->xsm->CPSHiLoUnit->Change(1.); // only for hi/lo
        a->spa_show_scan_time();
}

void App::spa_SeV_unit_switch_check(){
        static int lastmode=MODE_ENERGY_EV;
        if(main_get_gapp ()->xsm->IsMode(MODE_ENERGY_EV)){
                if(lastmode == MODE_ENERGY_EV) return;
                lastmode = MODE_ENERGY_EV;
                ((Gtk_EntryControl *) g_object_get_data( G_OBJECT (spm_control), "SPA_Energy"))->changeUnit(main_get_gapp ()->xsm->EnergyUnit);
        }else{
                if(lastmode == MODE_ENERGY_S) return;
                lastmode = MODE_ENERGY_S;
                ((Gtk_EntryControl *) g_object_get_data( G_OBJECT (spm_control), "SPA_Energy"))->changeUnit(main_get_gapp ()->xsm->YSUnit);
        }
}

void App::recalc_volt_from_new_Energy(double* x, double *Eneu){
        BZUnit BZEnew(*main_get_gapp ()->xsm->BZ_Unit);
        BZEnew.SetE(*Eneu);
        // convert: from Volt(Eold) to BZ to Volt(Enew):
        *x = BZEnew.Usr2Base(main_get_gapp ()->xsm->BZ_Unit->Base2Usr(*x));
}


void App::spa_switch_unit(Param_Control* pcs, gpointer data){
        pcs->changeUnit((UnitObj*)data);
}

void App::spa_mode_switch_check(){
        static int lastmode=MODE_VOLTUNIT;
	
        if(main_get_gapp ()->xsm->IsMode(MODE_BZUNIT)){
                if(lastmode == MODE_BZUNIT) return;
                lastmode = MODE_BZUNIT;
                //  main_get_gapp ()->xsm->BZ_Unit->SetE(data->hardpars.SPA_Energy);
                XSM_DEBUG(DBG_L3,  "mode switch check: recalc BZ from Volt"  );
                g_slist_foreach (
                                 (GSList*)g_object_get_data (
                                                             G_OBJECT (main_get_gapp ()->spm_control), 
                                                             "SPA_EnergyDependEC_list"),
                                 (GFunc) App::spa_switch_unit, main_get_gapp ()->xsm->BZ_Unit);
                main_get_gapp ()->spm_update_all();
        }else{
                if(lastmode == MODE_VOLTUNIT) return;
                lastmode = MODE_VOLTUNIT;
                XSM_DEBUG(DBG_L3,  "mode switch check: recalc Volt from BZ" );
                g_slist_foreach (
                                 (GSList*)g_object_get_data (
                                                             G_OBJECT (main_get_gapp ()->spm_control), 
                                                             "SPA_EnergyDependEC_list"),
                                 (GFunc) App::spa_switch_unit, main_get_gapp ()->xsm->VoltUnit);
        }
        spm_range_check(NULL, gapp);
}

// Controll Fields used by STM and AFM
// ========================================
GtkWidget* App::create_spm_control (){
        BuildParam *spm_bp = new BuildParam ();
        GtkWidget *grid = spm_bp->grid; // base grid, holds several references
        spm_bp->remote_list_ec = RemoteEntryList;
        
        GSList *EC_ScanFix_list=NULL;
        GSList *EC_OffsetFix_list=NULL;
	
        GtkWidget *inputMR, *inputMS, *inputMN; // M wie Master, R,S,N wie Range, Step, NPoints
        GtkWidget *inputCR, *inputCS, *inputCN; // C wie Client

        XSM_DEBUG(DBG_L3, "creating SPM control");

        spm_bp->new_grid_with_frame ("Scan Parameter", 1, 1);
        spm_bp->set_no_spin ();
        spm_bp->set_input_nx (3);
        #define SPM_MAIN_LWLC 10
        #define SPM_MAIN_LWIC 10
        spm_bp->set_label_width_chars (SPM_MAIN_LWLC);
        spm_bp->set_input_width_chars (SPM_MAIN_LWIC);
        spm_bp->set_default_ec_change_notice_fkt (App::spm_range_check, this);
        
        // Range controls
        XSM_DEBUG(DBG_L3, "creating SPM control -- Range Entries");
        inputMR = spm_bp->grid_add_ec ("Range XY", xsm->X_Unit, &xsm->data.s.rx,
                                       0., // xsm->XRangeMin(),
                                       xsm->XRangeMax(), 
                                       xsm->AktUnit->prec1,
                                       0.,0.,
                                       "RangeX");
        gtk_widget_set_hexpand (spm_bp->input, TRUE);
        EC_ScanFix_list = g_slist_prepend (EC_ScanFix_list, spm_bp->ec);
        g_object_set_data( G_OBJECT (grid), "EC_Rx", spm_bp->ec);

        spm_bp->set_input_nx (2);
        inputCR = spm_bp->grid_add_ec (NULL, xsm->Y_Unit, &xsm->data.s.ry,
                                       0., // xsm->XRangeMin(),
                                       xsm->YRangeMax(), 
                                       xsm->AktUnit->prec1,
                                       0.,0.,
                                       "RangeY");
        gtk_widget_set_hexpand (spm_bp->input, TRUE);
        EC_ScanFix_list = g_slist_prepend (EC_ScanFix_list, spm_bp->ec);
        g_object_set_data( G_OBJECT (grid), "EC_Ry", spm_bp->ec);
	spm_bp->grid_add_radio_button ("", "Calculate Range from Steps and Points",
                                       G_CALLBACK (cb_setmode), (gpointer)MODE_SETRANGE, true);
        spm_bp->new_line ();
        
        spm_bp->set_input_nx (3);
        inputMS = spm_bp->grid_add_ec ("Steps XY", xsm->X_Unit, &xsm->data.s.dx,
                                       0., // xsm->XstepsMin(),
                                       xsm->XStepMax(), 
                                       xsm->AktUnit->prec2,
                                       0.,0.,
                                       "StepsX");
        EC_ScanFix_list = g_slist_prepend (EC_ScanFix_list, spm_bp->ec);
        g_object_set_data( G_OBJECT (grid), "EC_Stx", spm_bp->ec);
        spm_bp->ec->setMin (0., xsm->XStepMin(), "red"); 

        spm_bp->set_input_nx (2);
        inputCS = spm_bp->grid_add_ec (NULL, xsm->Y_Unit, &xsm->data.s.dy,
                                       0., // xsm->YStepsMin(),
                                       xsm->YStepMax(), 
                                       xsm->AktUnit->prec2,
                                       0.,0.,
                                       "StepsY");
        EC_ScanFix_list = g_slist_prepend (EC_ScanFix_list, spm_bp->ec);
        g_object_set_data( G_OBJECT (grid), "EC_Sty", spm_bp->ec);
        spm_bp->ec->setMin (0., xsm->YStepMin(), "red"); 
	spm_bp->grid_add_radio_button ("", "Calculate Step Size from Range and Points",
                                       G_CALLBACK (cb_setmode), (gpointer)MODE_SETSTEPS);
        gtk_check_button_set_active(GTK_CHECK_BUTTON (spm_bp->radiobutton), true);
        
        spm_bp->new_line ();

        spm_bp->set_input_nx (3);
        inputMN = spm_bp->grid_add_ec ("Points XY", xsm->Unity, &xsm->data.s.nx,
                                       xsm->XMinPoints ()<2.? 2.:xsm->XMinPoints (), xsm->XMaxPoints (), 
                                       ".0f",
                                       "PointsX");
        EC_ScanFix_list = g_slist_prepend (EC_ScanFix_list, spm_bp->ec);
        g_object_set_data( G_OBJECT (grid), "EC_Pnx", spm_bp->ec);
        
        spm_bp->set_input_nx (2);
        inputCN = spm_bp->grid_add_ec (NULL, xsm->Unity, &xsm->data.s.ny,
                                       xsm->YMinPoints()<2.? 2.:xsm->YMinPoints (), xsm->YMaxPoints(), 
                                       ".0f",
                                       "PointsY");
        EC_ScanFix_list = g_slist_prepend (EC_ScanFix_list, spm_bp->ec);
        g_object_set_data( G_OBJECT (grid), "EC_Pny", spm_bp->ec);
	spm_bp->grid_add_radio_button ("", "Calculate Points from Steps and Range",
                                       G_CALLBACK (cb_setmode), (gpointer)MODE_SETPOINTS);
	
        XSM_DEBUG(DBG_L5, "setting up interconnects for range fields");
	
        // Setup Interconnections: for Range Fields
        // ========================================
        // Master Range X has client Range Y:
        g_object_set_data( G_OBJECT (inputMR), "HasClient", inputCR);
	
        // Master Steps X has client Steps Y:
        g_object_set_data( G_OBJECT (inputMS), "HasClient", inputCS);
	
        // Master Points X has client Points Y:
        g_object_set_data( G_OBJECT (inputMN), "HasClient", inputCN);


        spm_bp->new_line ();

        // Scan Offset -- always in non rotated coordinate system
        spm_bp->set_input_nx (3);
        spm_bp->set_default_ec_change_notice_fkt  (App::spm_offset_check, this);
        spm_bp->grid_add_ec ("Offset XY", xsm->X_Unit, &xsm->data.s.x0,
                             xsm->XOffsetMin(), xsm->XOffsetMax(),
                             xsm->AktUnit->prec1,
                             0.,0.,
                             "OffsetX");
        EC_OffsetFix_list = g_slist_prepend( EC_OffsetFix_list, spm_bp->ec);
        g_object_set_data( G_OBJECT (grid), "EC_Offx", spm_bp->ec);
	
        spm_bp->grid_add_ec (NULL, xsm->Y_Unit, &xsm->data.s.y0,
                             xsm->YOffsetMin(), xsm->YOffsetMax(),
                             xsm->AktUnit->prec1,
                             0.,0.,
                             "OffsetY");
        EC_OffsetFix_list = g_slist_prepend( EC_OffsetFix_list, spm_bp->ec);
        g_object_set_data( G_OBJECT (grid), "EC_Offy", spm_bp->ec);

        spm_bp->new_line ();

        // Scan Position: Tip position in scan coordinate system rel to scan center, using scan XY positions!
        xsm->data.s.sx = xsm->data.s.sy = 0.;
        spm_bp->set_default_ec_change_notice_fkt  (App::spm_scanpos_check, this);
        spm_bp->grid_add_ec("Scan XY", xsm->X_Unit, &xsm->data.s.sx,
                            -xsm->XRangeMax()/2., xsm->XRangeMax()/2.,
                            xsm->AktUnit->prec1,
                            0.,0.,
                            "ScanX");
        EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, spm_bp->ec);
        g_object_set_data( G_OBJECT (grid), "EC_ScanX", spm_bp->ec);
        
        spm_bp->grid_add_ec (NULL, xsm->Y_Unit, &xsm->data.s.sy,
                             -xsm->YRangeMax()/2., xsm->YRangeMax()/2.,
                             xsm->AktUnit->prec1,
                             0.,0.,
                             "ScanY");
        EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, spm_bp->ec);
        g_object_set_data( G_OBJECT (grid), "EC_ScanY", spm_bp->ec);

        spm_bp->new_line ();

        // Rotation
        spm_bp->set_no_spin (false);
        spm_bp->set_input_width_chars (5);
        spm_bp->set_default_ec_change_notice_fkt  (App::spm_offset_check, this);
        //        g_object_set_data( G_OBJECT (input), "Adjustment_PCS_Name", (void*)("SPM-Rotation"));
        spm_bp->grid_add_ec ("Rotation", xsm->ArcUnit, &xsm->data.s.alpha,
                             -360., 360., "6g", 1., 15.,
                             "Rotation");

        if (main_get_gapp ()->xsm->hardware)
                if (!main_get_gapp ()->xsm->hardware->RotateStepwise (0)) // test for delta rotation availability
                        EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, spm_bp->ec); // no life/while scanning rotation!

        GtkWidget *preset_button = spm_bp->grid_add_button
                ("Move to Origin", "Move to Origin (Zero Offset)", 1,
                 G_CALLBACK (App::offset_to_preset_callback), this);
        g_object_set_data (G_OBJECT(preset_button), "preset_xy", NULL);
        GtkWidget *offset_lock = spm_bp->grid_add_lock_button ("Lock/Unlock Move to Origin Button\nand PanView Preset Move to Actions", false, 2);
        g_object_set_data (G_OBJECT(grid), "offset_lock_button", offset_lock);
        
        spm_bp->new_line ();

        // Layer Dimension Control
        spm_bp->set_input_nx (1);
        spm_bp->set_input_width_chars (5);
        //        spm_bp->set_default_ec_change_notice_fkt  (App::spm_nlayer_update, this);
        spm_bp->set_default_ec_change_notice_fkt  (NULL, this);
        spm_bp->set_no_spin ();
        spm_bp->grid_add_ec("Layers", xsm->Unity, &xsm->data.s.nvalues,
                            1., xsm->MaxValues(), ".0f",
                            "Layers");
        // EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, spm_bp->ec);
        g_object_set_data( G_OBJECT (grid), "EC_Layers", spm_bp->ec);
        spm_bp->ec->Freeze ();

        spm_bp->set_no_spin (false);
        spm_bp->set_input_nx (3);
        spm_bp->set_default_ec_change_notice_fkt  (App::spm_select_layer, this);
        spm_bp->grid_add_ec (NULL,  xsm->Unity, &xsm->data.display.vlayer, 
                             0., 10., ".0f",
                             "LayerSelect");
        g_object_set_data( G_OBJECT (grid), "LayerSelectSpin", spm_bp->input);
        // EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, spm_bp->ec); // Allow

        spm_bp->set_label_width_chars (SPM_MAIN_LWIC);
        spm_bp->grid_add_label ("-.--#", NULL, 2);
        g_object_set_data (G_OBJECT (grid), "LayerSelectValue", spm_bp->label);
        spm_bp->set_label_width_chars (SPM_MAIN_LWLC);

        spm_bp->new_line ();

        // Time Dimension Control...
        spm_bp->set_no_spin ();
        spm_bp->set_input_nx (1);
        spm_bp->set_default_ec_change_notice_fkt  (NULL, this);
        spm_bp->grid_add_ec ("Time", xsm->Unity, &xsm->data.s.ntimes,
                             0., 1e6, ".0f",
                             "Time");
        EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, spm_bp->ec);
        //	RemoteEntryList = ec->AddEntry2RemoteList("Time", RemoteEntryList);
        //	ec->Set_ChangeNoticeFkt(App::spm_ntimes_update, this);
        g_object_set_data( G_OBJECT (grid), "EC_Time", spm_bp->ec);
        //	EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, ec);
        spm_bp->ec->Freeze ();

        spm_bp->set_no_spin (false);
        spm_bp->set_input_nx (3);
        spm_bp->set_default_ec_change_notice_fkt  (App::spm_select_time, this);
        spm_bp->grid_add_ec (NULL, xsm->Unity, &xsm->data.display.vframe, 
                             0., 1e6, ".0f",
                             "TimeSelect");
        g_object_set_data( G_OBJECT (grid), "TimeSelectSpin", spm_bp->input);
        EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, spm_bp->ec);

        spm_bp->set_label_width_chars (SPM_MAIN_LWIC);
        spm_bp->grid_add_label ("-.--#", NULL, 2);
        g_object_set_data( G_OBJECT (grid), "TimeSelectValue", spm_bp->label);
        spm_bp->set_label_width_chars (SPM_MAIN_LWLC);
	
        spm_bp->set_default_ec_change_notice_fkt  (NULL, this);
	
        spm_bp->set_input_nx (3);
        spm_bp->new_line ();

        spm_bp->set_no_spin (false);

        
        #if 0
        // Display scaling controls
        if (!strncasecmp(xsmres.InstrumentType, "CCD",3)){
                // Display -- Hi-Low
                spm_bp->set_default_ec_change_notice_fkt  (cb_display_changed_hilo, this);

                // g_object_set_data( G_OBJECT (grid), "SPM_EC_VRangeZ", spm_bp->ec);
                spm_bp->grid_add_ec ("CCD high", xsm->CPSHiLoUnit, &xsm->data.display.cpshigh,
                                     0., 5000., ".0f", 1., 100.,
                                     "CPShigh");

                spm_bp->new_line ();
                
                // g_object_set_data( G_OBJECT (input), "Adjustment_PCS_Name", (void*)("SPA-CPS-Low"));
                spm_bp->grid_add_ec ("CCD low", xsm->CPSHiLoUnit, &xsm->data.display.cpslow,
                                     0., 5000., ".0f", 1., 100.,
                                     "CPSlow");
        } else {
                // Display -- Range auto center
                spm_bp->set_default_ec_change_notice_fkt  (cb_display_changed, this);
                //                g_object_set_data( G_OBJECT (input), "Adjustment_PCS_Name", (void*)("SPM-VRangeZ"));
                spm_bp->grid_add_ec ("V Range Z", xsm->data.Zunit, &xsm->data.display.vrange_z,
                                     -5000., 5000., ".3g", 0.1, 5.,
                                     "VRangeZ");
                g_object_set_data( G_OBJECT (grid), "SPM_EC_VRangeZ", spm_bp->ec);

                spm_bp->new_line ();

                // g_object_set_data( G_OBJECT (input), "Adjustment_PCS_Name", (void*)("SPM-VOffsetZ"));
                spm_bp->grid_add_ec ("V Offset Z", xsm->data.Zunit, &xsm->data.display.voffset_z,
                                     -5000., 5000., ".3g", 0.2, 2.,
                                     "VOffsetZ");
                g_object_set_data( G_OBJECT (grid), "SPM_EC_VOffsetZ", spm_bp->ec);
        }
        #endif

        spm_bp->pop_grid ();
#if 0
        
        // --------------------------------------------------
        // Range automatism control... Calc: Range,Points,Steps
        spm_bp->set_xy (10,1);
        spm_bp->new_grid_with_frame ("Calculate", 1,1);
	spm_bp->grid_add_radio_button ("Steps", "Calculate Step Size from Range and Points",
                                       G_CALLBACK (cb_setmode), (gpointer)MODE_SETSTEPS, true);
        spm_bp->new_line ();
	spm_bp->grid_add_radio_button ("Range", "Calculate Range from Steps and Points",
                                       G_CALLBACK (cb_setmode), (gpointer)MODE_SETRANGE);
        spm_bp->new_line ();
	spm_bp->grid_add_radio_button ("Points", "Calculate Points from Steps and Range",
                                       G_CALLBACK (cb_setmode), (gpointer)MODE_SETPOINTS);
	
        spm_bp->pop_grid ();

        // View Mode Radiobuttons
        // --------------------------------------------------
        spm_bp->set_xy (10,2);
        spm_bp->new_grid_with_frame ("View Mode", 1,1);
        // gtk_widget_set_vexpand (frame, TRUE);

	spm_bp->grid_add_radio_button ("Quick", "Quick View (line regression)",
                                       G_CALLBACK (callback_view), (gpointer)SCAN_V_QUICK, true);
        g_object_set_data( G_OBJECT (grid), "quickbutton", spm_bp->radiobutton);
        spm_bp->new_line ();

	spm_bp->grid_add_radio_button ("Direct", "Direct View (only range scale)",
                                       G_CALLBACK (callback_view), (gpointer)SCAN_V_DIRECT);
        g_object_set_data( G_OBJECT (grid), "directbutton", spm_bp->radiobutton);
        spm_bp->new_line ();

	spm_bp->grid_add_radio_button ("Plane", "Plane View (background plane correct)",
                                       G_CALLBACK (callback_view), (gpointer)SCAN_V_PLANESUB);
        g_object_set_data( G_OBJECT (grid), "planesubbutton", spm_bp->radiobutton);
        spm_bp->new_line ();

	spm_bp->grid_add_radio_button ("Logarith.", "Logarithm transform, direct",
                                       G_CALLBACK (callback_view), (gpointer)SCAN_V_LOG);
        g_object_set_data( G_OBJECT (grid), "logbutton", spm_bp->radiobutton);
        spm_bp->new_line ();

	spm_bp->grid_add_radio_button ("Horizont", "Horizontal (line by line offset)",
                                       G_CALLBACK (callback_view), (gpointer)SCAN_V_HORIZONTAL);
        g_object_set_data( G_OBJECT (grid), "horizontalbutton", spm_bp->radiobutton);
        spm_bp->new_line ();

	spm_bp->grid_add_radio_button ("Diff.", "Differential (differential filter)",
                                       G_CALLBACK (callback_view), (gpointer)SCAN_V_DIFFERENTIAL);
        g_object_set_data( G_OBJECT (grid), "differentialbutton", spm_bp->radiobutton);
        spm_bp->new_line ();

	spm_bp->grid_add_radio_button ("Periodic", "Periodic/Modulo color repeat",
                                       G_CALLBACK (callback_view), (gpointer)SCAN_V_PERIODIC);
        g_object_set_data( G_OBJECT (grid), "periodicbutton", spm_bp->radiobutton);

        spm_bp->pop_grid ();
#endif
        
        // save lists away...
        g_object_set_data( G_OBJECT (grid), "SPM_EC_list", spm_bp->get_ec_list_head ());
        g_object_set_data( G_OBJECT (grid), "SPM_SCANFIX_EC_list", EC_ScanFix_list);
        g_object_set_data( G_OBJECT (grid), "SPM_OFFSETFIX_EC_list", EC_OffsetFix_list);

        RemoteEntryList = spm_bp->get_remote_list_head ();
        gtk_widget_show (grid); // FIX-ME GTK4 SHOWALL

        return (grid);
}

// Updates all SPM Entry Fields in List !!!
void App::spm_update_all(int Vflg){
        if(!spm_control) return;
        XSM_DEBUG(DBG_L3, "------ App::spm_update_all -------");
        if(gapp){
                main_get_gapp ()->xsm->Inst->update (); // piezo, current, ... conversion are buffered, must update

                main_get_gapp ()->xsm->SetRedraw(FALSE); // suppress redraw on each displayrelevat field, do redraw only once at end !
		
                main_get_gapp ()->xsm->data.UpdateUnits();

                g_slist_foreach (
                                 (GSList*)g_object_get_data (
                                                             G_OBJECT (spm_control), "SPM_EC_list"),
                                 (GFunc) App::update_ec, NULL);
        }

        if(Vflg<0) Vflg=-Vflg;
        XSM_DEBUG(DBG_L3, "App::spm_update_all - Vflg=" << Vflg);
        
        GtkCheckButton *tb = NULL;
        switch(Vflg & SCAN_V_MASK){
        case 0: break;
        case SCAN_V_QUICK:
                tb = (GtkCheckButton*)g_object_get_data( G_OBJECT (spm_control), "quickbutton");
                break;
        case SCAN_V_DIRECT:
                tb = (GtkCheckButton*)g_object_get_data( G_OBJECT (spm_control), "directbutton");
                break;
        case SCAN_V_PLANESUB:
                tb = (GtkCheckButton*)g_object_get_data( G_OBJECT (spm_control), "planesubbutton");
                break;
        case SCAN_V_LOG:
                tb = (GtkCheckButton*)g_object_get_data( G_OBJECT (spm_control), "logbutton");
                break;
        case SCAN_V_HORIZONTAL:
                tb = (GtkCheckButton*)g_object_get_data( G_OBJECT (spm_control), "horziontalbutton");
                break;
        case SCAN_V_DIFFERENTIAL:
                tb = (GtkCheckButton*)g_object_get_data( G_OBJECT (spm_control), "differentialbutton");
                break;
        case SCAN_V_PERIODIC:
                tb = (GtkCheckButton*)g_object_get_data( G_OBJECT (spm_control), "periodicbutton");
                break;
        default: break;
        }
        if (tb)
                gtk_check_button_set_active (tb, TRUE);
        
        if(gapp)
                main_get_gapp ()->xsm->SetRedraw(); // Enable redraw and do one redraw !
	
        ui_update();
        as_update();
        
        XSM_DEBUG(DBG_L3, "App::spm_update_all done");
}

void App::spm_freeze_scanparam(int subset){
        g_slist_foreach (
                         (GSList*)g_object_get_data (
                                                     G_OBJECT (spm_control), 
                                                     subset == 0?"SPM_SCANFIX_EC_list":"SPM_OFFSETFIX_EC_list"),
                         (GFunc) App::freeze_ec, NULL);
}

void App::spm_thaw_scanparam(int subset){
        g_slist_foreach (
                         (GSList*) g_object_get_data (
                                                      G_OBJECT (spm_control), 
                                                      subset == 0?"SPM_SCANFIX_EC_list":"SPM_OFFSETFIX_EC_list"),
                         (GFunc) App::thaw_ec, NULL);
        make_freeze_selected ();
}

GtkWidget* App::create_spa_control (){
        GtkWidget *grid=NULL;
#if 0
        GSList *EC_list=NULL;
        GSList *EC_ScanFix_list=NULL;
        GSList *EC_EnergyDep_list=NULL;
        GSList *EC_EnergyDepEC_list=NULL;
        GSList *EC_Ysize_list=NULL;
	
        Gtk_EntryControl *ec;

        GtkWidget *grid, *f_grid;
        GtkWidget *frame;
        
        GtkWidget *input, *inputM, *inputC;
        GtkWidget *inputMR, *inputMN; // M wie Master, R,S,N wie Range, Step, NPoints
        GtkWidget *inputCR, *inputCN; // C wie Client
	
        GtkWidget *radiobutton;
        int x,y, xg, yg;

        xg=yg=1;
        
        XSM_DEBUG(DBG_L3, "creating SPA control");

        grid = gtk_grid_new ();
	
        frame = gtk_frame_new ("Scan Parameter");
        gtk_grid_attach (GTK_GRID (grid), frame, xg, yg, 3,4);
	
        f_grid = gtk_grid_new ();
        x=y=1;
        gtk_frame_set_child (GTK_FRAME (frame), f_grid);
	
        // Range controls
        XSM_DEBUG(DBG_L3, "creating range controls");
        inputMR = spm_bp->grid_add_ec("Length XY", f_grid, x,y);
        gtk_label_set_width_chars (GTK_LABEL (gtk_grid_get_child_at (GTK_GRID (f_grid), 1,1)), 13);
        ec = new Gtk_EntryControl (xsm->X_Unit, MLD_WERT_NICHT_OK, &xsm->data.s.rx,
                                   0.,
                                   //			     xsm->XRangeMin(),
                                   xsm->XRangeMax(), 
                                   xsm->AktUnit->prec1, inputMR);
        ec->Set_ChangeNoticeFkt(App::spm_range_check, this);
        EC_list = g_slist_prepend( EC_list, ec);
        EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, ec);
        EC_EnergyDep_list = g_slist_prepend( EC_EnergyDep_list, &xsm->data.s.rx);
        EC_EnergyDepEC_list = g_slist_prepend( EC_EnergyDepEC_list, ec);
        RemoteEntryList = ec->AddEntry2RemoteList("LengthX", RemoteEntryList);
        g_object_set_data( G_OBJECT (grid), "EC_Rx", ec);
	
        inputCR= spm_bp->grid_add_ec (NULL, f_grid, x,y);
        ec = new Gtk_EntryControl (xsm->Y_Unit, MLD_WERT_NICHT_OK, &xsm->data.s.ry,
                                   0.,
                                   //			     xsm->YRangeMin(),
                                   xsm->YRangeMax(), 
                                   xsm->AktUnit->prec1, inputCR);
        ec->Set_ChangeNoticeFkt(App::spm_range_check, this);
        EC_list = g_slist_prepend( EC_list, ec);
        EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, ec);
        EC_EnergyDep_list = g_slist_prepend( EC_EnergyDep_list, &xsm->data.s.ry);
        EC_EnergyDepEC_list = g_slist_prepend( EC_EnergyDepEC_list, ec);
        EC_Ysize_list = g_slist_prepend( EC_Ysize_list, inputCR);
        RemoteEntryList = ec->AddEntry2RemoteList("LengthY", RemoteEntryList);
        g_object_set_data( G_OBJECT (grid), "EC_Rlisty", ec);
        
        x=1,++y;
        inputMN = spm_bp->grid_add_ec("Points XY", f_grid, x,y);
        ec = new Gtk_EntryControl (xsm->Unity, MLD_WERT_NICHT_OK,&xsm->data.s.nx,
                                   xsm->XMinPoints(), xsm->XMaxPoints(), 
                                   "4.0f", inputMN);
        ec->Set_ChangeNoticeFkt(App::spm_range_check, this);
        EC_list = g_slist_prepend( EC_list, ec);
        EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, ec);
        RemoteEntryList = ec->AddEntry2RemoteList("PointsX", RemoteEntryList);
        g_object_set_data( G_OBJECT (grid), "EC_Pnx", ec);
        inputCN = spm_bp->grid_add_ec (NULL, f_grid, x,y);
        ec = new Gtk_EntryControl (xsm->Unity, MLD_WERT_NICHT_OK,&xsm->data.s.ny,
                                   xsm->YMinPoints(), xsm->YMaxPoints(), 
                                   "4.0f", inputCN);
        ec->Set_ChangeNoticeFkt(App::spm_range_check, this);
        EC_list = g_slist_prepend( EC_list, ec);
        EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, ec);
        EC_Ysize_list = g_slist_prepend( EC_Ysize_list, inputCN);
        RemoteEntryList = ec->AddEntry2RemoteList("PointsY", RemoteEntryList);
        g_object_set_data( G_OBJECT (grid), "EC_Pny", ec);
	
        // Setup Interconnections:
        // Master Length X has client Length Y
        g_object_set_data( G_OBJECT (inputMR), "HasClient", inputCR);
        // Master Points X has client Points Y
        g_object_set_data( G_OBJECT (inputMN), "HasClient", inputCN);
	
        x=1,++y;
        // Offset realtiv to (0,0) Offset, used as invariant Point of Rotation
        XSM_DEBUG(DBG_L3, "creating Offset controls");
        inputM = spm_bp->grid_add_ec("Offset XY", f_grid, x,y);
        ec = new Gtk_EntryControl (xsm->X_Unit, MLD_WERT_NICHT_OK, &xsm->data.s.x0,
                                   xsm->XOffsetMin(), xsm->XOffsetMax(), xsm->AktUnit->prec1, inputM);
        EC_list = g_slist_prepend( EC_list, ec);
        EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, ec);
        EC_EnergyDep_list = g_slist_prepend( EC_EnergyDep_list, &xsm->data.s.x0);
        EC_EnergyDepEC_list = g_slist_prepend( EC_EnergyDepEC_list, ec);
        RemoteEntryList = ec->AddEntry2RemoteList("OffsetX", RemoteEntryList);
        ec->Set_ChangeNoticeFkt(App::spm_offset_check, this);
	
        inputC = spm_bp->grid_add_ec (NULL, f_grid, x,y);
        ec = new Gtk_EntryControl (xsm->Y_Unit, MLD_WERT_NICHT_OK, &xsm->data.s.y0,
                                   xsm->YOffsetMin(), xsm->YOffsetMax(), xsm->AktUnit->prec1, inputC);
        EC_list = g_slist_prepend( EC_list, ec);
        //  g_object_set_data( G_OBJECT (inputM), "HasClient", inputC);
        EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, ec);
        EC_EnergyDep_list = g_slist_prepend( EC_EnergyDep_list, &xsm->data.s.y0);
        EC_EnergyDepEC_list = g_slist_prepend( EC_EnergyDepEC_list, ec);
        RemoteEntryList = ec->AddEntry2RemoteList("OffsetY", RemoteEntryList);
        ec->Set_ChangeNoticeFkt(App::spm_offset_check, this);
	
        x=1,++y;
        // Rotation
        XSM_DEBUG(DBG_L3, "creating Rotation controls");
        input = mygtk_grid_add_spin_input("Rotation", f_grid, x,y);
        g_object_set_data( G_OBJECT (input), "Adjustment_PCS_Name", (void*)("SPA-Rotation"));
        ec = new Gtk_EntryControl (xsm->ArcUnit, MLD_WERT_NICHT_OK, &xsm->data.s.alpha,
                                   -360., 360., "6g", input, 1., 15.);
        EC_list = g_slist_prepend( EC_list, ec);
        EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, ec);
        RemoteEntryList = ec->AddEntry2RemoteList("Rotation", RemoteEntryList);

        x=1,++y;
        // Gatetime
        input = spm_bp->grid_add_ec("Gatetime", f_grid, x,y);
        ec = new Gtk_EntryControl (xsm->TimeUnitms, MLD_WERT_NICHT_OK, &xsm->data.s.GateTime,
                                   1e-4, 60., ".1f", input);
        ec->Set_ChangeNoticeFkt(App::spa_gate_check, this);
        EC_list = g_slist_prepend( EC_list, ec);
        EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, ec);
        RemoteEntryList = ec->AddEntry2RemoteList("Gatetime", RemoteEntryList);

        x=1,++y;
        // Energy
        input = mygtk_grid_add_spin_input("Energy", f_grid, x,y);
        g_object_set_data( G_OBJECT (input), "Adjustment_PCS_Name", (void*)("SPA-Energy"));
        ec = new Gtk_EntryControl (xsm->EnergyUnit, MLD_WERT_NICHT_OK, &xsm->data.s.Energy,
                                   0., 500., ".2f", input, 0.1, 1.);
        ec->Set_ChangeNoticeFkt(App::spa_energy_check, this);
        EC_list = g_slist_prepend( EC_list, ec);
        EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, ec);
        spa_energy_check(NULL, this); // First call initializes internal Eold
        RemoteEntryList = ec->AddEntry2RemoteList("Energy", RemoteEntryList);
        g_object_set_data( G_OBJECT (grid), "SPA_Energy", ec);
	
	
        x=1,++y;
        // Layer Dimension Control
        input = spm_bp->grid_add_ec("Layers", f_grid, x,y);
        gtk_editable_set_width_chars (GTK_EDITABLE (input), 5);
        ec = new Gtk_EntryControl (xsm->Unity, MLD_WERT_NICHT_OK, &xsm->data.s.nvalues,
                                   1., xsm->MaxValues(), ".0f", input);
        EC_list = g_slist_prepend( EC_list, ec);
        RemoteEntryList = ec->AddEntry2RemoteList("Layers", RemoteEntryList);
        //	ec->Set_ChangeNoticeFkt(App::spm_nlayer_update, this);
        g_object_set_data( G_OBJECT (grid), "EC_Layers", ec);
        //	EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, ec);
        ec->Freeze ();

        input = mygtk_grid_add_spin_input (NULL, f_grid, x,y, 3);
        g_object_set_data( G_OBJECT (grid), "LayerSelectSpin", input);
        ec = new Gtk_EntryControl (xsm->Unity, N_("Layer out of range"), &xsm->data.display.vlayer, 
                                   0., 10., ".0f", input, 1., 10.);
        ec->Set_ChangeNoticeFkt(App::spm_select_layer, this);
        SetupScale (ec->GetAdjustment(), f_grid, x,y);
        EC_list = g_slist_prepend( EC_list, ec);
        RemoteEntryList = ec->AddEntry2RemoteList("LayerSelect", RemoteEntryList);

        input = gtk_label_new ("-.--#");
        gtk_label_set_width_chars (GTK_LABEL (input), 10);
        gtk_widget_show (input);
        gtk_grid_attach (GTK_GRID (f_grid), input, x,y, 1,1);
        g_object_set_data( G_OBJECT (grid), "LayerSelectValue", input);

        x=1,++y;
        // Time Dimension Control...
        input = spm_bp->grid_add_ec("Time", f_grid, x,y);
        gtk_editable_set_width_chars (GTK_EDITABLE (input), 5);
        ec = new Gtk_EntryControl (xsm->Unity, MLD_WERT_NICHT_OK, &xsm->data.s.ntimes,
                                   0., 1e6, ".0f", input);
        EC_list = g_slist_prepend( EC_list, ec);
        RemoteEntryList = ec->AddEntry2RemoteList("Time", RemoteEntryList);
        //	ec->Set_ChangeNoticeFkt(App::spm_ntimes_update, this);
        g_object_set_data( G_OBJECT (grid), "EC_Time", ec);
        //	EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, ec);
        ec->Freeze ();

        input = mygtk_grid_add_spin_input (NULL, f_grid, x,y, 3);
        g_object_set_data( G_OBJECT (grid), "TimeSelectSpin", input);
        ec = new Gtk_EntryControl (xsm->Unity, N_("Time out of range"), &xsm->data.display.vframe,
                                   0., 10., ".0f", input, 1., 10.);
        ec->Set_ChangeNoticeFkt(App::spm_select_time, this);
        SetupScale (ec->GetAdjustment(), f_grid, x,y);
        EC_list = g_slist_prepend( EC_list, ec);
        RemoteEntryList = ec->AddEntry2RemoteList("TimeSelect", RemoteEntryList);

        input = gtk_label_new ("-.--#");
        gtk_label_set_width_chars (GTK_LABEL (input), 10);
        gtk_widget_show (input);
        gtk_grid_attach (GTK_GRID (f_grid), input, x,y, 1,1);
        g_object_set_data( G_OBJECT (grid), "TimeSelectValue", input);
	
        //????? g_object_set_data( G_OBJECT (vbox), "TimeSelectHbox", hbox_param);	

        x=1,++y;
        // Display
        XSM_DEBUG(DBG_L3, "creating Display controls");
        input = mygtk_grid_add_spin_input("CPS high", f_grid, x,y);
        g_object_set_data( G_OBJECT (input), "Adjustment_PCS_Name", (void*)("SPA-CPS-High"));
        ec = new Gtk_EntryControl (xsm->CPSHiLoUnit, MLD_WERT_NICHT_OK, &xsm->data.display.cpshigh,
                                   10., 10e6, ".0f", input, 1., 100.);
        SetupScale(ec->GetAdjustment(), f_grid, x,y);
        ec->Set_ChangeNoticeFkt(cb_display_changed_hilo, this);
        EC_list = g_slist_prepend( EC_list, ec);
        RemoteEntryList = ec->AddEntry2RemoteList("CPShigh", RemoteEntryList);
	
        x=1,++y;
        input = mygtk_grid_add_spin_input("CPS low", f_grid, x,y);
        g_object_set_data( G_OBJECT (input), "Adjustment_PCS_Name", (void*)("SPA-CPS-Low"));
        ec = new Gtk_EntryControl (xsm->CPSHiLoUnit, MLD_WERT_NICHT_OK, &xsm->data.display.cpslow,
                                   0., 10e6,".0f", input, 1., 100.);
        SetupScale(ec->GetAdjustment(), f_grid, x,y);
        ec->Set_ChangeNoticeFkt(cb_display_changed_hilo, this);
        EC_list = g_slist_prepend( EC_list, ec);
        RemoteEntryList = ec->AddEntry2RemoteList("CPSlow", RemoteEntryList);
	
	
#define SPA_CONTRAST_CONTROLS
#ifdef SPA_CONTRAST_CONTROLS
        x=1,++y;
        // Display
        input = mygtk_grid_add_spin_input("VRange Z", f_grid, x,y);
        g_object_set_data( G_OBJECT (input), "Adjustment_PCS_Name", (void*)("SPA-VRangeZ"));
        ec = new Gtk_EntryControl (xsm->data.Zunit, MLD_WERT_NICHT_OK, &xsm->data.display.vrange_z,
                                   -1e6, 1e6, ".3g", input, 1., 100. );
        SetupScale(ec->GetAdjustment(), f_grid, x,y);
        ec->Set_ChangeNoticeFkt(cb_display_changed, this);
        EC_list = g_slist_prepend( EC_list, ec);
        RemoteEntryList = ec->AddEntry2RemoteList("VRangeZ", RemoteEntryList);
        g_object_set_data( G_OBJECT (grid), "SPM_EC_VRangeZ", ec);

        x=1,++y;
        input = mygtk_grid_add_spin_input("VOffset Z", f_grid, x,y);
        g_object_set_data( G_OBJECT (input), "Adjustment_PCS_Name", (void*)("SPA-VOffsetZ"));
        ec = new Gtk_EntryControl (xsm->data.Zunit, MLD_WERT_NICHT_OK, &xsm->data.display.voffset_z,
                                   -1e6, 1e6, ".2g", input, 1., 100.);
        SetupScale(ec->GetAdjustment(), f_grid ,x,y);
        ec->Set_ChangeNoticeFkt(cb_display_changed, this);
        EC_list = g_slist_prepend( EC_list, ec);
        RemoteEntryList = ec->AddEntry2RemoteList("VOffsetZ", RemoteEntryList);
        g_object_set_data( G_OBJECT (grid), "SPM_EC_VOffsetZ", ec);
#endif

        x=1,++y;
        // SPA Offset to (0,0) - for more comfort...
        XSM_DEBUG(DBG_L3, "creating Offset controls");
        inputM = spm_bp->grid_add_ec("Offset (0,0)", f_grid, x,y);
        ec = new Gtk_EntryControl (xsm->VoltUnit, MLD_WERT_NICHT_OK, &xsm->data.s.SPA_OrgX,
                                   xsm->XOffsetMin(), xsm->XOffsetMax(), xsm->AktUnit->prec2, inputM);
        EC_list = g_slist_prepend( EC_list, ec);
        EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, ec);
        RemoteEntryList = ec->AddEntry2RemoteList("Offset00X", RemoteEntryList);
        ec->Set_ChangeNoticeFkt(App::spm_offset_check, this);
	
        inputC = spm_bp->grid_add_ec (NULL, f_grid, x,y);
        ec = new Gtk_EntryControl (xsm->VoltUnit, MLD_WERT_NICHT_OK, &xsm->data.s.SPA_OrgY,
                                   xsm->YOffsetMin(), xsm->YOffsetMax(), xsm->AktUnit->prec2, inputC);
        EC_list = g_slist_prepend( EC_list, ec);
        //  g_object_set_data( G_OBJECT (inputM), "HasClient", inputC);
        EC_ScanFix_list = g_slist_prepend( EC_ScanFix_list, ec);
        RemoteEntryList = ec->AddEntry2RemoteList("Offset00Y", RemoteEntryList);
        ec->Set_ChangeNoticeFkt(App::spm_offset_check, this);
	
	
        // --------------------------------------------------
        // Mode Select
	
        frame = gtk_frame_new ("Modes");
        gtk_grid_attach (GTK_GRID (grid), frame, xg+4, yg++, 1,1);
        f_grid = gtk_grid_new();
        gtk_frame_set_child (GTK_FRAME (frame), f_grid);
        x=y=1;
        
        // --------------------------------------------------
        // Units Mode: %BZ / Volt
        radiobutton = gtk_radio_button_new_with_label (NULL, "Volt");
        gtk_grid_attach (GTK_GRID (f_grid), radiobutton, x,y++, 1,1);
        g_signal_connect (G_OBJECT (radiobutton), "toggled",
                          G_CALLBACK (cb_setmode), (void*)MODE_VOLTUNIT);
	
        radiobutton = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radiobutton), "%BZ");
        gtk_grid_attach (GTK_GRID (f_grid), radiobutton, x,y++, 1,1);
        g_signal_connect (G_OBJECT (radiobutton), "toggled",
                          G_CALLBACK (cb_setmode), (void*)MODE_BZUNIT);
	
        // --------------------------------------------------
	
        gtk_grid_attach (GTK_GRID (f_grid), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), x,y++,1,1);
	
        // --------------------------------------------------
        // Energy Mode: Energy [eV] / Phase [1]
        radiobutton = gtk_radio_button_new_with_label (NULL, "Energy");
        gtk_grid_attach (GTK_GRID (f_grid), radiobutton, x,y++, 1,1);
        g_signal_connect (G_OBJECT (radiobutton), "toggled",
                          G_CALLBACK (cb_setmode), (void*)MODE_ENERGY_EV);
	
        radiobutton = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radiobutton), "Phase");
        gtk_grid_attach (GTK_GRID (f_grid), radiobutton, x,y++, 1,1);
        g_signal_connect (G_OBJECT (radiobutton), "toggled",
                          G_CALLBACK (cb_setmode), (void*)MODE_ENERGY_S);
	
        /*
        // --------------------------------------------------
        // Scan Mode: 2D / 1D
        radiobutton = gtk_radio_button_new_with_label (NULL, "2D Scan");
        gtk_box_pack_start (GTK_BOX (vbox_view), radiobutton, TRUE, TRUE, 0);
        gtk_widget_show (radiobutton);
        g_signal_connect (G_OBJECT (radiobutton), "toggled",
        G_CALLBACK (cb_setmode), (void*)MODE_2DIMSCAN);
	
        radiobutton = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radiobutton), "1D Scan");
        gtk_box_pack_start (GTK_BOX (vbox_view), radiobutton, TRUE, TRUE, 0);
        gtk_widget_show (radiobutton);
        g_signal_connect (G_OBJECT (radiobutton), "toggled",
        G_CALLBACK (cb_setmode), (void*)MODE_1DIMSCAN);
	
        */

        // View Mode Radiobuttons
        // --------------------------------------------------
        frame = gtk_frame_new ("View");
        gtk_grid_attach (GTK_GRID (grid), frame, xg+4, yg++, 1,1);
        f_grid = gtk_grid_new();
        gtk_frame_set_child (GTK_FRAME (frame), f_grid);
        x=y=1;
	
        radiobutton = gtk_radio_button_new_with_label (NULL, "Quick");
        gtk_grid_attach (GTK_GRID (f_grid), radiobutton, x,y++, 1,1);
        g_object_set_data( G_OBJECT (grid), "quickbutton", radiobutton);

        // Connect the "toggled" signal of the button to our callback
        g_signal_connect (G_OBJECT (radiobutton), "toggled",
                          G_CALLBACK (callback_view), (gpointer) SCAN_V_QUICK);
	
        radiobutton = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radiobutton), "Direct");
        gtk_grid_attach (GTK_GRID (f_grid), radiobutton, x,y++, 1,1);
        g_object_set_data( G_OBJECT (grid), "directbutton", radiobutton);
        g_signal_connect (G_OBJECT (radiobutton), "toggled",
                          G_CALLBACK (callback_view), (gpointer) SCAN_V_DIRECT);
	
        radiobutton = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radiobutton), "Plane");
        gtk_grid_attach (GTK_GRID (f_grid), radiobutton, x,y++, 1,1);
        g_object_set_data( G_OBJECT (grid), "planesubbutton", radiobutton);
        g_signal_connect (G_OBJECT (radiobutton), "toggled",
                          G_CALLBACK (callback_view), (gpointer) SCAN_V_PLANESUB);
	
        radiobutton = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radiobutton), "Logarith.");
        gtk_grid_attach (GTK_GRID (f_grid), radiobutton, x,y++, 1,1);
        g_object_set_data( G_OBJECT (grid), "logbutton", radiobutton);
        g_signal_connect (G_OBJECT (radiobutton), "toggled",
                          G_CALLBACK (callback_view), (gpointer) SCAN_V_LOG);
	
        radiobutton = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radiobutton), "Horizont");
        gtk_grid_attach (GTK_GRID (f_grid), radiobutton, x,y++, 1,1);
        g_object_set_data( G_OBJECT (grid), "horizontbutton", radiobutton);
        g_signal_connect (G_OBJECT (radiobutton), "toggled",
                          G_CALLBACK (callback_view), (gpointer) SCAN_V_HORIZONTAL);
	
        radiobutton = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radiobutton),  "Diff.");
        gtk_grid_attach (GTK_GRID (f_grid), radiobutton, x,y++, 1,1);
        g_object_set_data( G_OBJECT (grid), "differentialbutton", radiobutton);
        g_signal_connect (G_OBJECT (radiobutton), "toggled",
                          G_CALLBACK (callback_view), (gpointer) SCAN_V_DIFFERENTIAL);
		
        radiobutton = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radiobutton), "Periodic");
        gtk_grid_attach (GTK_GRID (f_grid), radiobutton, x,y++, 1,1);
        g_object_set_data( G_OBJECT (grid), "periodicbutton", radiobutton);
	
        // save lists away
        g_object_set_data( G_OBJECT (grid), "SPM_EC_list", EC_list);
        g_object_set_data( G_OBJECT (grid), "SPM_SCANFIX_EC_list", EC_ScanFix_list);
        g_object_set_data( G_OBJECT (grid), "SPA_EnergyDepend_list", EC_EnergyDep_list);
        g_object_set_data( G_OBJECT (grid), "SPA_EnergyDependEC_list", EC_EnergyDepEC_list);
        g_object_set_data( G_OBJECT (grid), "SPA_Ysize_entrys", EC_Ysize_list);
	
#endif
        return grid;
}


void App::spa_update(){
        g_slist_foreach (
                         (GSList*) g_object_get_data (
                                                      G_OBJECT (spm_control), "SPM_EC_list"),
                         (GFunc) App::update_ec, NULL);
        ui_update();
        as_update();
}

GtkWidget* App::create_as_control (){
        BuildParam *as_bp = new BuildParam ();
        GObject *refob = G_OBJECT (as_bp->grid);
        as_bp->new_grid_with_frame ("File/Autosave", 3, 4);
        as_bp->set_no_spin ();
        as_bp->grid_add_input(N_("Basename"));
        as_bp->set_input_width_chars (10);
        gtk_widget_set_hexpand (as_bp->input, TRUE);
        g_object_set_data( refob, "basename", as_bp->input);

        g_signal_connect (G_OBJECT (as_bp->input), "changed",
                          G_CALLBACK (cbbasename),
                          as_bp->input);

        g_settings_bind (as_settings, "auto-save-basename",
                         G_OBJECT (as_bp->input), "text",
                         G_SETTINGS_BIND_DEFAULT);

        
        as_bp->set_input_width_chars (6);
        as_bp->grid_add_ec ("#", xsm->Unity, &xsm->file_counter,
                            0, 99999, "05.0f", "auto-save-counter-i");

        as_bp->grid_add_button ("...", "Set file storage folder.",1,
                                G_CALLBACK (App::file_set_datapath_callback));


        GtkWidget *checkbutton = as_bp->grid_add_check_button (N_("Auto Save"), "Auto Save" PYREMOTE_CHECK_HOOK_KEY("MainAutoSave"), 1,
                                                                cb_setmode, (void*)MODE_AUTOSAVE);
        // GtkWidget *checkbutton = gtk_check_button_new_with_label( N_("Auto Save"));
        // as_bp->grid_add_widget (checkbutton);
        //g_signal_connect (G_OBJECT (checkbutton), "toggled",
        //                  G_CALLBACK (cb_setmode), (void*)MODE_AUTOSAVE);
        g_settings_bind (as_settings, "auto-save",
                         G_OBJECT (checkbutton), "active",
                         G_SETTINGS_BIND_DEFAULT);

        as_bp->new_line ();

        as_bp->grid_add_input (N_("Original"), 5);
        gtk_widget_set_hexpand (as_bp->input, TRUE);
        g_object_set_data (refob, "originalname", as_bp->input);
        gtk_editable_set_editable (GTK_EDITABLE (as_bp->input), FALSE);

        
        // save List away...
        g_object_set_data (refob, "AS_EC_list", as_bp->get_ec_list_head ());
        as_bp->show_all ();
        g_object_set_data (refob, "AS_BP", as_bp);

        as_bp->pop_grid ();

        return as_bp->grid;
}


void App::as_update(){
        g_slist_foreach
                ((GSList*) g_object_get_data
                 (G_OBJECT (as_control), "AS_EC_list"),
                 (GFunc) App::update_ec, NULL);
        
        // now automatic via as_settings bind, so get it !
        as_setdata();
        if (xsm->GetActiveScan())
                gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (g_object_get_data( G_OBJECT (as_control), "originalname")))),
                                           xsm->GetActiveScan()->data.ui.originalname, -1);
        else
                gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (g_object_get_data( G_OBJECT (as_control), "originalname")))),
                                           xsm->data.ui.originalname, -1);
}

void App::as_setdata(){
        xsm->data.ui.SetBaseName (gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (g_object_get_data (G_OBJECT (as_control), "basename"))))));
}

GtkWidget* App::create_ui_control (){
        BuildParam *ui_bp = new BuildParam ();
        GObject *refob = G_OBJECT (ui_bp->grid);
        ui_bp->new_grid_with_frame ("User Info & Comment", 3, 4);
        ui_bp->set_no_spin ();

        GtkWidget *text = gtk_text_view_new ();
        GtkWidget *scrolled_window = gtk_scrolled_window_new ();
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_widget_set_hexpand (scrolled_window, TRUE);
        gtk_widget_set_vexpand (scrolled_window, TRUE);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled_window), text);
        ui_bp->grid_add_widget (scrolled_window, 2,1);

#if 0
        g_settings_bind (as_settings, "user-info-comment",
                         G_OBJECT (text), "text",
                         G_SETTINGS_BIND_SET);
#endif

        GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
        gtk_text_buffer_set_text (buffer, N_("Hey here is no comment yet!"), -1);
        g_object_set_data (refob, "comment", buffer);
        //	g_signal_connect (G_OBJECT (buffer), "changed",
        //			    G_CALLBACK (cbtext), this);

        ui_bp->new_line ();

        ui_bp->grid_add_input(N_("Date of Scan"));
        ui_bp->set_input_width_chars (24);
        gtk_widget_set_hexpand (ui_bp->input, TRUE);
        g_object_set_data (refob, "dateofscan", ui_bp->input);
        gtk_editable_set_editable(GTK_EDITABLE (ui_bp->input), FALSE);

        ui_bp->show_all ();
        g_object_set_data (refob, "UI_BP", ui_bp);
        ui_bp->pop_grid ();

        return (ui_bp->grid);
}

void App::ui_update(){
        static gchar* comment_tmp = NULL;
        if(xsm->data.ui.comment){
#if 1
                if (comment_tmp){ // signal changed did not work, workaround here
                        if (!strcmp (comment_tmp, xsm->data.ui.comment))
                                ui_setcomment ();
                        g_free (comment_tmp);
                        comment_tmp = g_strdup (xsm->data.ui.comment);
                } else 
                        comment_tmp = g_strdup (xsm->data.ui.comment);
#endif
                GtkTextBuffer *buffer = (GtkTextBuffer*)g_object_get_data( G_OBJECT (ui_control), "comment");

                const gchar *ve=NULL;
                if (!g_utf8_validate ((const gchar*) xsm->data.ui.comment, -1, &ve)){
                        gsize br, bw;
                        g_print ("\nERROR at:\n");
                        g_print ("%s", (const gchar*) ve);
			
                        g_print ("%s", (const gchar*) xsm->data.ui.comment);
			
                        gchar *tmp = g_convert_with_fallback ((const gchar*) xsm->data.ui.comment,
                                                              strlen ((const gchar*) xsm->data.ui.comment),
                                                              "UTF-8",
                                                              "G_LOCALE",
                                                              NULL,
                                                              &br, &bw,
                                                              NULL);
                        if (!tmp)
                                tmp = g_strdup_printf ("Error while converting to UTF8. <%2x> r%d, w%d, l%d", 
                                                       *ve, (int)br, (int)bw, (int)strlen ((const gchar*) xsm->data.ui.comment));
                        g_free(xsm->data.ui.comment);
                        xsm->data.ui.comment = g_strdup (tmp);
                        g_free (tmp);
                }
                gtk_text_buffer_set_text ( buffer, (const gchar*) xsm->data.ui.comment, -1);
        }
        if(xsm->data.ui.dateofscan){
                gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (g_object_get_data( G_OBJECT (ui_control), "dateofscan")))),
                                           xsm->data.ui.dateofscan,
                                           -1);
        }
                            //--------------------------------------------------------------------------------
        XSM_DEBUG_GM(DBG_L1, "App::ui_update** ui.comment updated from data ==================================\n%s", xsm->data.ui.comment);
        //  g_slist_foreach(g_object_get_data( G_OBJECT (ui_control), "UI_EC_list"),
        //		  (GFunc) App::update_ec, NULL);
}

void App::ui_setcomment(){
        GtkTextIter s,e;
        GtkTextBuffer *buffer = (GtkTextBuffer*)g_object_get_data( G_OBJECT (ui_control), "comment");
        gtk_text_buffer_get_bounds (buffer, &s, &e);

        gchar *txt = gtk_text_buffer_get_text (buffer, &s, &e, FALSE);

        g_free(xsm->data.ui.comment);
        xsm->data.ui.comment = g_strdup (txt);
        XSM_DEBUG(0, "ui.comment changed: " << xsm->data.ui.comment);
}
