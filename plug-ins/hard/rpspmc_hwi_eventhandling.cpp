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

/* ignore this module for docuscan
% PlugInModuleIgnore
*/



#include <locale.h>
#include <libintl.h>
#include <time.h>
#include <iomanip>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "gxsm_app.h"
#include "gxsm_window.h"

#include "glbvars.h"
#include "app_view.h"
#include "app_vobj.h"
#include "action_id.h"

#include "surface.h"

#include "rpspmc_hwi_structs.h"
#include "rpspmc_pacpll.h"

#define UTF8_DEGREE    "\302\260"
#define UTF8_MU        "\302\265"
#define UTF8_ANGSTROEM "\303\205"

extern GxsmPlugin rpspmc_pacpll_hwi_pi;
extern rpspmc_hwi_dev *rpspmc_hwi; // instance of the HwI derived XSM_Hardware class

gfloat color_gray[4]    = { .3, .3, .3, 0.8 };
gfloat color_red[4]     = { 1., 0., 0., 1.0 };
gfloat color_yellow[4]  = { 1., 1., 0., 1.0 };

// SR specific conversions and lookups

#define DSP_FRQ_REF 75000.0

#define SRV2     (2.05/32767.)
#define SRV10    (10.0/32767.)
#define PhaseFac (1./16.)
#define BiasFac  (main_get_gapp()->xsm->Inst->Dig2VoltOut (1.) * main_get_gapp()->xsm->Inst->BiasGainV2V ())
#define BiasOffset (main_get_gapp()->xsm->Inst->Dig2VoltOut (1.) * main_get_gapp()->xsm->Inst->BiasV2V (0.))
#define ZAngFac  (main_get_gapp()->xsm->Inst->Dig2ZA (1))
#define XAngFac  (main_get_gapp()->xsm->Inst->Dig2XA (1))
#define YAngFac  (main_get_gapp()->xsm->Inst->Dig2YA (1))

extern SOURCE_SIGNAL_DEF source_signals[];

//#define XSM_DEBUG_PG(X)  std::cout << X << std::endl;
#define XSM_DEBUG_PG(X) ;

const gchar *err_unknown_l = "L? (index)";
const gchar *err_unknown_u = "U?";

const gchar *str_pA = "pA";
const gchar *str_nA = "nA";


void RPSPMC_Control::init_vp_signal_info_lookup_cache(){
        for (int i=0; i<NUM_PROBEDATA_ARRAYS; ++i){
                msklookup[i]   = 0;
                lablookup[i]   = err_unknown_l;
                unitlookup[i]  = err_unknown_u;
                expdi_lookup[i] = PROBEDATA_ARRAY_INDEX; // safety/error fallback
                for (int k=0; source_signals[k].mask; ++k)
                        if (source_signals[k].garr_index == i){
                               msklookup[i]   = source_signals[k].mask;
                               lablookup[i]   = source_signals[k].label;
                               unitlookup[i]  = source_signals[k].unit_sym;
                               expdi_lookup[i] = source_signals[k].garr_index;
                        }
                g_print ("Mask[%02d] 0x%08x => %s\n",i,msklookup[i],lablookup[i]);
        }
        // END MARK:
        msklookup[NUM_PROBEDATA_ARRAYS] = -1;
        lablookup[NUM_PROBEDATA_ARRAYS] = NULL;
        unitlookup[NUM_PROBEDATA_ARRAYS] = NULL;
        expdi_lookup[NUM_PROBEDATA_ARRAYS] = 0;
}

const char* RPSPMC_Control::vp_label_lookup(int i){
        for (int k=0; source_signals[k].mask; ++k)
                if (source_signals[k].garr_index == i)
                        return  source_signals[k].label;
        return err_unknown_l;
}

const char* RPSPMC_Control::vp_unit_lookup(int i){
        for (int k=0; source_signals[k].mask; ++k)
                if (source_signals[k].garr_index == i)
                        if (source_signals[k].mask == 0x000010){ // CUSTOM auto
                                if (main_get_gapp()->xsm->Inst->nAmpere2V (1.) > 1.)
                                        return str_pA;
                                else
                                        return str_nA;
                        } else
                                return  source_signals[k].unit_sym;
        return err_unknown_u;
}

double RPSPMC_Control::vp_scale_lookup(int i){
        for (int k=0; source_signals[k].mask; ++k){
                if (source_signals[k].garr_index == i){
                        if (source_signals[k].mask == 0x000010){ // CUSTOM auto
                                if (main_get_gapp()->xsm->Inst->nAmpere2V (1.) > 1.)
                                        return source_signals[k].scale_factor/main_get_gapp()->xsm->Inst->nAmpere2V (1e-3); // choose pA
                                else
                                        return source_signals[k].scale_factor/main_get_gapp()->xsm->Inst->nAmpere2V (1.); // nA
                        } else
                                return  source_signals[k].scale_factor;
                }
        }
        g_warning ("vp_scale_lookup -- failed to find scale for garr index %d", i);
        return 1.;
}


int RPSPMC_Control::Probing_event_setup_scan (int ch, 
                                          const gchar *titleprefix, 
                                          const gchar *name,
                                          const gchar *unit,
                                          const gchar *label,
                                          double d2u,
                                          int nvalues
	){
	Mem2d *m=main_get_gapp()->xsm->scan[ch]->mem2d;
        g_message ("0 ch[%d] Nxy: %d, %d Nv:%d sls[%d,%d, %d,%d]",
                   ch,
                   m->data->GetNx (),
                   m->data->GetNy (),
                   m->data->GetNv (),
                   m->data->GetX0Sub(), m->data->GetNxSub(),
                   m->data->GetY0Sub(), m->data->GetNySub()
                   );
        //m->Resize (m->GetNx (), m->GetNy (), nvalues, ZD_DOUBLE, false); // multilayerinfo=clean
	
        main_get_gapp()->xsm->scan[ch]->create (TRUE, FALSE, strchr (titleprefix, '-') ? -1.:1., main_get_gapp()->xsm->hardware->IsFastScan (), ZD_DOUBLE, true, false, true );
        
        g_message ("C ch[%d] Nxy: %d, %d Nv:%d sls[%d,%d, %d,%d]",
                   ch,
                   m->data->GetNx (),
                   m->data->GetNy (),
                   m->data->GetNv (),
                   m->data->GetX0Sub(), m->data->GetNxSub(),
                   m->data->GetY0Sub(), m->data->GetNySub()
                   );
 
        main_get_gapp()->xsm->scan[ch]->data.s.nvalues = nvalues;
        m->Resize (m->GetNx (), m->GetNy (), nvalues, ZD_IDENT, false); // multilayerinfo=clean
        m->data->MkVLookup(0, nvalues-1);

        g_message ("R ch[%d] Nxy: %d, %d Nv:%d sls[%d,%d, %d,%d]",
                   ch,
                   m->data->GetNx (),
                   m->data->GetNy (),
                   m->data->GetNv (),
                   m->data->GetX0Sub(), m->data->GetNxSub(),
                   m->data->GetY0Sub(), m->data->GetNySub()
                   );

       // Setup correct Z unit
	UnitObj *u = main_get_gapp()->xsm->MakeUnit (unit, label);
	main_get_gapp()->xsm->scan[ch]->data.SetZUnit (u);
	delete u;

	// setup dz from instrument definition or propagated via signal definition
        main_get_gapp()->xsm->scan[ch]->data.s.dz = d2u;
	
	// set scan title, name, ... and draw it!

	gchar *scantitle = NULL;
        scantitle = g_strdup_printf ("%s %s", titleprefix, name);
	
	main_get_gapp()->xsm->scan[ch]->data.ui.SetName (scantitle);
	main_get_gapp()->xsm->scan[ch]->data.ui.SetTitle (scantitle);
	main_get_gapp()->xsm->scan[ch]->data.ui.SetType (scantitle);
	main_get_gapp()->xsm->scan[ch]->data.s.xdir = strchr (titleprefix, '-') ? -1.:1.;
	main_get_gapp()->xsm->scan[ch]->data.s.ydir = main_get_gapp()->xsm->data.s.ydir;

        main_get_gapp()->xsm->scan[ch]->storage_manager.set_type (scantitle);
        main_get_gapp()->xsm->scan[ch]->storage_manager.set_basename (main_get_gapp()->xsm->data.ui.basename); // from GXSM Main GUI
        main_get_gapp()->xsm->scan[ch]->storage_manager.set_dataset_counter (main_get_gapp()->xsm->GetFileCounter ());   // from GXSM Main GUI
        main_get_gapp()->xsm->scan[ch]->storage_manager.set_path (g_settings_get_string (main_get_gapp()->get_as_settings (), "auto-save-folder"));   // from GXSM Main GUI
        main_get_gapp()->xsm->scan[ch]->data.ui.SetOriginalName (main_get_gapp()->xsm->scan[ch]->storage_manager.get_name ("(not saved)"));
        
	PI_DEBUG (DBG_L2, "setup_scan[" << ch << " ]: scantitle done: " << main_get_gapp()->xsm->scan[ch]->data.ui.type ); 

        main_get_gapp()->channelselector->SetInfo (ch, scantitle);
        //main_get_gapp()->channelselector->SetInfo (ch, main_get_gapp()->xsm->scan[ch]->storage_manager.get_name()); // testing
	main_get_gapp()->xsm->scan[ch]->draw ();

	g_free (scantitle);

        return 0;
}

// DSP Raster Probe MAP handling:
int RPSPMC_Control::Probing_eventcheck_callback( GtkWidget *widget, RPSPMC_Control *dspc){
	//static ProfileControl *pc[MAX_NUM_CHANNELS][MAX_NUM_CHANNELS];
        static int xlast=0;
        static int xdelta=1;
        static int xiDD=0;
        static int xipD=0;
        static int yipD=0;
        static int Xsrc_lookup_end=-1;
	int popped=0;
	GArray **garr;
	GArray **garr_hdr;
        int xip=-1, yip=-1;
	XSM_DEBUG_PG ("RPSPMC_Control::Probing_eventcheck_callback -- enter");
        
	// pop off all available data from stack
	while ((garr = dspc->pop_probedata_arrays ()) != NULL){
                garr_hdr = dspc->pop_probehdr_arrays ();
                ++popped;
                
                if (main_get_gapp()->xsm->FindChan(xsmres.extchno[0], ID_CH_D_P) >= 0){ // mapi=0 must be selected!
                        // find chunksize (total # data sources)
                        int chunksize = 0;
                        GPtrArray *glabarray = g_ptr_array_new ();
                        GPtrArray *gsymarray = g_ptr_array_new ();

                        int Xsrc=-1;
                        
                        for (int src=0; dspc->msklookup[src]>=0 && src < MAX_NUM_CHANNELS; ++src){
                                if (dspc->vis_PSource & dspc->msklookup[src] || dspc->vis_XSource & dspc->msklookup[src]){
                                        g_ptr_array_add (glabarray, (gpointer) dspc->vp_label_lookup (src));
                                        g_ptr_array_add (gsymarray, (gpointer) dspc->vp_unit_lookup (src));
                                        ++chunksize;
                                        //                                g_message ("PEV: %i #%i Lab:%s USym:%s <%c>", chunksize, src,
                                        //                                           (gpointer) dspc->vp_label_lookup (src), (gpointer) dspc->vp_unit_lookup (src),
                                        //                                           dspc->vis_PSource & dspc->msklookup[src]? 'Y' : dspc->vis_XSource & dspc->msklookup[src] ? 'X' : '?');
                                        if (Xsrc<0 && dspc->vis_XSource & dspc->msklookup[src]){
                                                Xsrc = src; // used to map layer index to value
                                        }
                                }
                        }

#if 0  //** DATAMAP0...7 to scan image disabled
                        
                        // auto decide on mapping mode -- direct data map to Scan-Channel or to Scan-Event
                
                        // *****************************************************************************************************************
                        // remap probe data to scan channel(s) DataMap0..7
                        // *****************************************************************************************************************
                        int src=-1;
                        int xiD=-1, yiD=-1;
                        for(int mapi=0; mapi < EXTCHMAX; ++mapi){
                                int map=0;
                                int chmap=main_get_gapp()->xsm->FindChan(xsmres.extchno[mapi], ID_CH_D_P);
                                if (chmap < 0) // check if any DataMap channel is setup
                                        continue;
                        
                                int nx=main_get_gapp()->xsm->scan[chmap]->mem2d->GetNx ();
                                int ny=main_get_gapp()->xsm->scan[chmap]->mem2d->GetNy ();
                        
                                // locate 1st,... probe src# to map
                                ++src;
                                // look for mapping sources and assign
                                while (map == 0){
                                        if (src < MAX_NUM_CHANNELS){
                                                if (dspc->vis_PSource & dspc->msklookup[src] || dspc->vis_XSource & dspc->msklookup[src])
                                                        if (dspc->vis_PSource & dspc->msklookup[src]){
                                                                map = 1;
                                                                break;
                                                        }
                                                ++src;
                                        } else
                                                break; // bail
                                }
                                if (map){
                                        if (xip<0){ // only once per coordinate!
                                                if (garr_hdr) {
                                                        int i=0;

                                                        // get probe coordinates from DSP probe HDR
#if 1
                                                        g_message ("P SEC HDR #%d :: i:%g, t:%g, ix:%g, iy:%g, PHI:%g, XS:%g, YS:%g, ZS:%g, U:%g, S:%g",
                                                                   i,
                                                                   g_array_index (garr_hdr[PROBEDATA_ARRAY_INDEX], double, i),
                                                                   g_array_index (garr_hdr[PROBEDATA_ARRAY_TIME], double, i),
                                                                   g_array_index (garr_hdr[PROBEDATA_ARRAY_X0], double, i),
                                                                   g_array_index (garr_hdr[PROBEDATA_ARRAY_Y0], double, i),
                                                                   g_array_index (garr_hdr[PROBEDATA_ARRAY_PHI], double, i),
                                                                   g_array_index (garr_hdr[PROBEDATA_ARRAY_XS], double, i),
                                                                   g_array_index (garr_hdr[PROBEDATA_ARRAY_YS], double, i),
                                                                   g_array_index (garr_hdr[PROBEDATA_ARRAY_ZS], double, i),
                                                                   g_array_index (garr_hdr[PROBEDATA_ARRAY_U], double, i),
                                                                   g_array_index (garr_hdr[PROBEDATA_ARRAY_SEC], double, i));
#endif
                                                        if (main_get_gapp()->xsm->scan[chmap]->mem2d->data->GetNxSub()){
#if 1
                                                                g_message ("CH[%d] HDR ixy: %d, %d   sls[%d,%d, %d,%d]", chmap,
                                                                           (int)g_array_index (garr_hdr[PROBEDATA_ARRAY_X0], double, i),
                                                                           (int)g_array_index (garr_hdr[PROBEDATA_ARRAY_Y0], double, i),
                                                                           main_get_gapp()->xsm->scan[chmap]->mem2d->data->GetX0Sub(), main_get_gapp()->xsm->scan[chmap]->mem2d->data->GetNxSub(),
                                                                           main_get_gapp()->xsm->scan[chmap]->mem2d->data->GetY0Sub(), main_get_gapp()->xsm->scan[chmap]->mem2d->data->GetNySub()
                                                                           );
#endif
                                                                xip = main_get_gapp()->xsm->scan[chmap]->mem2d->data->GetNxSub()-1 - g_array_index (garr_hdr[PROBEDATA_ARRAY_X0], double, i);
                                                                yip = main_get_gapp()->xsm->scan[chmap]->mem2d->data->GetNySub()-1 - g_array_index (garr_hdr[PROBEDATA_ARRAY_Y0], double, i);
                                                        } else {
                                                                xip = nx-1 - g_array_index (garr_hdr[PROBEDATA_ARRAY_X0], double, i);
                                                                yip = ny-1 - g_array_index (garr_hdr[PROBEDATA_ARRAY_Y0], double, i);
                                                        }

                                                        xiD = ((int)g_array_index (garr_hdr[PROBEDATA_ARRAY_XS], double, i)<<16)/dspc->mirror_dsp_scan_dx32 + nx/2 - 1;
                                                        yiD = (nx/2 - 1) - ((int)g_array_index (garr_hdr[PROBEDATA_ARRAY_YS], double, i)<<16)/dspc->mirror_dsp_scan_dy32;
                                        
                                                        g_message ("DSP-areascan.ixy: %04d, %04d   index from XY ScanVector dsp: %04d, %04d  Deltas: %04d, %04d **DX %04d %04d",
                                                                   xip,yip,
                                                                   xiD, yiD,
                                                                   xip-xiD, yip-yiD, xiD-xiDD, xip-xipD
                                                                   );
                                                        xdelta=xip-xipD;
                                                        xlast=xip;
                                                        xiDD=xiD; xipD=xip; yipD=yip;
                                                } else {
                                                        xip = xlast+xdelta; yip=yiD;
                                                        g_message ("XYdsp: probe HDR N/A -- projecting x to %d", xip);
                                                        if (xip >= main_get_gapp()->xsm->scan[chmap]->mem2d->GetNx ()){
                                                                g_message ("XYdsp: probe HDR N/A, project out of range -- dropping point");
                                                                xip=-1;
                                                                mapi = EXTCHMAX;
                                                                continue;
                                                        }
                                                }
                                        
                                                // check and limit ranges
                                                if (xip < 0 || yip < 0 || xip >= main_get_gapp()->xsm->scan[chmap]->mem2d->GetNx () || yip >= main_get_gapp()->xsm->scan[chmap]->mem2d->GetNy ()){
                                                        g_message ("Warning: Coordinates (%d, %d) out of scan range. Dropping point.", xip, yip);
                                                        xip=-1;
                                                        mapi = EXTCHMAX;
                                                        continue;
                                                }
                                        }
                                        
                                        // sanity check adn trigger initial final setup -- need to resize scan map?
                                        if (main_get_gapp()->xsm->scan[chmap]->data.s.dz < 0.){
                                                gchar *id = g_strconcat ("Map-", (const gchar*)g_ptr_array_index (glabarray,  mapi),
                                                                         "(", Xsrc<0?"index":(gpointer) dspc->vp_label_lookup (Xsrc),
                                                                         Xsrc<0?"i":(gpointer) dspc->vp_unit_lookup (Xsrc), ")",
                                                                         NULL);
                                                Probing_event_setup_scan (chmap, "X+", id,
                                                                          (const gchar*)g_ptr_array_index (gsymarray,  mapi),
                                                                          (const gchar*)g_ptr_array_index (glabarray,  mapi),
                                                                          1.0, dspc->last_probe_data_index);
                                                g_message ("MAPI: %i CH%i Lab:%s USym:%s LayerLookup:%s(%s)", mapi, chmap,
                                                           (const gchar*)g_ptr_array_index (gsymarray,  mapi),
                                                           (const gchar*)g_ptr_array_index (glabarray,  mapi),
                                                           Xsrc<0?"index":(const gchar*)((gpointer) dspc->vp_label_lookup (Xsrc)), Xsrc<0?"N/A":(const gchar*)((gpointer) dspc->vp_unit_lookup (Xsrc))
                                                           );
                                                Xsrc_lookup_end = -1;
                                                g_free (id);
                                                //main_get_gapp()->xsm->scan[chmap]->mem2d->add_layer_information (new LayerInformation ("Bias", main_get_gapp()->xsm->data.s.Bias, "%5.3f V"));
                                                //main_get_gapp()->xsm->scan[chmap]->mem2d->add_layer_information (new LayerInformation ("Layer", l, "%03.0f"));

                                                
                                        }
                      
                                        if (dspc->last_probe_data_index !=  main_get_gapp()->xsm->scan[chmap]->mem2d->GetNv ()){ // auto n-values range adjust
                                                main_get_gapp()->xsm->scan[chmap]->mem2d->Resize (main_get_gapp()->xsm->scan[chmap]->mem2d->GetNx (), main_get_gapp()->xsm->scan[chmap]->mem2d->GetNy (),
                                                                                       dspc->last_probe_data_index, ZD_DOUBLE, false);
                                                g_message ("Resize was required ** ch[%d] Nxy: %d, %d Nv:%d sls[%d,%d, %d,%d]",
                                                           chmap,
                                                           main_get_gapp()->xsm->scan[chmap]->mem2d->GetNx (),
                                                           main_get_gapp()->xsm->scan[chmap]->mem2d->GetNy (),
                                                           main_get_gapp()->xsm->scan[chmap]->mem2d->GetNv (),
                                                           main_get_gapp()->xsm->scan[chmap]->mem2d->data->GetX0Sub(), main_get_gapp()->xsm->scan[chmap]->mem2d->data->GetNxSub(),
                                                           main_get_gapp()->xsm->scan[chmap]->mem2d->data->GetY0Sub(), main_get_gapp()->xsm->scan[chmap]->mem2d->data->GetNySub()
                                                           );
                                        }
                                        // remap probe data data to scan mem2d buffer
                                        int rf=0;
                                        int tf=0;
                                        int nk=1;
                                        int nl=1;
                                        if (g_settings_get_boolean (dspc->hwi_settings, "probe-graph-enable-map-fill")){
                                                rf = -dspc->probe_trigger_raster_points/2;
                                                tf = -dspc->probe_trigger_raster_points_b/2;
                                                nk = dspc->probe_trigger_raster_points;
                                                nl = dspc->probe_trigger_raster_points_b;
                                        }
                                        for (int k=0; k<nk; ++k){
                                                int x=xip+rf+k;
                                                for (int l=0; l<nl; ++l){
                                                        int y=yip+tf+l;
                                                        if (x < 0 || y < 0 || x >= main_get_gapp()->xsm->scan[chmap]->mem2d->GetNx () || y >= main_get_gapp()->xsm->scan[chmap]->mem2d->GetNy ())
                                                                continue;
                                                        for (int i = 0; i < dspc->last_probe_data_index; i++){
                                                                main_get_gapp()->xsm->scan[chmap]->mem2d->PutDataPkt_ixy_sub (dspc->vp_scale_lookup (src) * g_array_index (garr [expdi_lookup[src]], double, i), x,y,i);
                                                                if (Xsrc >= 0 && Xsrc_lookup_end < i){ // update with X lookup
                                                                        main_get_gapp()->xsm->scan[chmap]->mem2d->SetLayer (i);
                                                                        main_get_gapp()->xsm->scan[chmap]->mem2d->data->SetVLookup(i, dspc->vp_scale_lookup (Xsrc) * g_array_index (garr [expdi_lookup[Xsrc]], double, i)); // update X lookup
                                                                        gchar *lpl = g_strdup_printf ("Layer-Param %s", (const gchar*)dspc->vp_label_lookup (Xsrc));
                                                                        gchar *lpu = g_strdup_printf ("%%5.3f %s", (const gchar*)dspc->vp_unit_lookup (Xsrc));
                                                                        main_get_gapp()->xsm->scan[chmap]->mem2d->add_layer_information (new LayerInformation (lpl,
                                                                                                                                                    dspc->vp_scale_lookup (Xsrc) * g_array_index (garr [expdi_lookup[Xsrc]], double, i),
                                                                                                                                                    lpu));
                                                                        g_free (lpl);
                                                                        g_free (lpu);
                                                                        Xsrc_lookup_end=i;
                                                                }

                                                        }
                                                }
                                        }
                                        if (Xsrc < 0) // no X lookup, make index matching lookup
                                                main_get_gapp()->xsm->scan[chmap]->mem2d->data->MkVLookup(0, dspc->last_probe_data_index-1); // use index
                                
                                        main_get_gapp()->xsm->scan[chmap]->draw ();
                                } // endif map                              
                        } // for mappi
#endif //** DATAMAP0...7 to scan image disabled
                        
                        // unref lable and symbols
                        g_ptr_array_free (glabarray, TRUE);
                        g_ptr_array_free (gsymarray, TRUE);
                        
                } // endif DataMap Scan Channel mapping

                // *****************************************************************************************************************
                // attach event to active channel, if one exists -- not for DSP raster mode only manual/script mode ----------------
                // *****************************************************************************************************************
                if ( g_settings_get_boolean (dspc->hwi_settings, "probe-graph-enable-add-events")
                     && main_get_gapp()->xsm->MasterScan){
                        ScanEvent *se = NULL;
                        double wx, wy;
                        main_get_gapp()->xsm->MasterScan->Pixel2World (xip+main_get_gapp()->xsm->MasterScan->mem2d->data->GetX0Sub(),
                                                            yip+main_get_gapp()->xsm->MasterScan->mem2d->data->GetY0Sub(),
                                                            wx,wy); // with SLS offset
                        se = new ScanEvent (wx,wy,
                                            main_get_gapp()->xsm->Inst->Dig2ZA ((long) round (g_array_index (garr [PROBEDATA_ARRAY_ZS], double, 0)))
                                            );
                        main_get_gapp()->xsm->MasterScan->mem2d->AttachScanEvent (se);
                        
                        // if we have attached an scan event, so fill it with data now
                        if (se){
                                // find chunksize (total # data sources)
                                int chunksize = 0;
                                GPtrArray *glabarray = g_ptr_array_new ();
                                GPtrArray *gsymarray = g_ptr_array_new ();
                                //g_message ("Looking up data chunks...");
                                // find chunksize (total # data sources)

                                for (int src=0; dspc->msklookup[src]>=0 && src < MAX_NUM_CHANNELS; ++src)
                                        if (dspc->vis_Source & dspc->msklookup[src]){
                                                g_ptr_array_add (glabarray, (gpointer) dspc->vp_label_lookup (src));
                                                g_ptr_array_add (gsymarray, (gpointer) dspc->vp_unit_lookup (src));
                                                ++chunksize;
                                        }
                        
                                if (chunksize > 0 && chunksize < MAX_NUM_CHANNELS){
                                        double dataset[MAX_NUM_CHANNELS];
                                        //g_message ("Creating PE");
                                        ProbeEntry *pe = new ProbeEntry ("Probe", time(0), glabarray, gsymarray, chunksize);
                                        for (int i = 0; i < dspc->last_probe_data_index; i++){
                                                int j=0;
                                                for (int src=0; dspc->msklookup[src]>=0 && src < MAX_NUM_CHANNELS; ++src){
                                                        if (dspc->vis_Source & dspc->msklookup[src]){
                                                                dataset[j++] = dspc->vp_scale_lookup (src) * g_array_index (garr [dspc->expdi_lookup[src]], double, i);
                                                                // g_message ("data for <%s> : %d %g", (gpointer) dspc->vp_label_lookup (src), i,dataset[j-1]);
                                                        }
                                                }
                                                pe->add ((double*)&dataset);
                                        }
                                        se->add_event (pe);
                                }
                                //g_ptr_array_free (glabarray, TRUE); // passed to PE!
                                //g_ptr_array_free (gsymarray, TRUE);
                        }
                } // end if attach SE, PE

                // free popped arrays
                free_probehdr_array_set (garr_hdr, dspc);
                free_probedata_array_set (garr, dspc);
	} // while pop...
        
	XSM_DEBUG_PG("DBG-M4");

	if (popped > 0)
		if (main_get_gapp()->xsm->MasterScan){
                        main_get_gapp()->xsm->MasterScan->draw ();
			main_get_gapp()->xsm->MasterScan->view->update_events ();
                }

        XSM_DEBUG_PG("DBG-M5");
	XSM_DEBUG_PG ("RPSPMC_Control::Probing_eventcheck_callback -- exit");
	return 0;
}

void RPSPMC_Control::probedata_visualize (GArray *probedata_x, GArray *probedata_y, GArray *probedata_sec,
				      ProfileControl* &pc, ProfileControl* &pc_av, int plot_msk,
				      const gchar *xlab, const gchar *xua, double xmult,
				      const gchar *ylab, const gchar *yua, double ymult,
				      int current_i, int si, int nas, gboolean join_same_x,
                                      gint xmap, gint src, gint num_active_xmaps, gint num_active_sources){

        static gint last_current_i=0;
	UnitObj *UXaxis = new UnitObj(xua, " ", "g", xlab);
	UnitObj *UYaxis = new UnitObj(yua,  " ", "g", ylab);
	double xmin, xmax, x;

        // force rebuild if view/window arrangement changed?
        if (pc){
                if ((GrMatWin && !pc->is_external_window_set ()) || (!GrMatWin && pc->is_external_window_set ())){
                        delete pc;
                        pc = NULL;
                }
        }
        
        if (!GrMatWin && vpg_app_window){
                // g_message ("PGCV destroy vpg_window");
                gtk_window_destroy (GTK_WINDOW (vpg_app_window));
                vpg_app_window = NULL;
                vpg_window = NULL;
                vpg_grid = NULL;
        }
        
        if (GrMatWin && !vpg_window){
                vpg_app_window =  gxsm4_app_window_new (GXSM4_APP (main_get_gapp()->get_application ()));
                vpg_window = GTK_WINDOW (vpg_app_window);
                GtkWidget *header_bar = gtk_header_bar_new ();
                gtk_widget_show (header_bar);
                //gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header_bar), true);
                // FIX-ME GTK4
                //gtk_header_bar_set_subtitle (GTK_HEADER_BAR  (header_bar), "last plot");
                gtk_window_set_titlebar (GTK_WINDOW (vpg_window), header_bar);

                //gtk_window_resize (GTK_WINDOW (vpg_window),
                //                   400*num_active_xmaps > 1100? 1100:400*num_active_xmaps,
                //                   200*num_active_sources > 800? 800:200*num_active_sources);

                vpg_grid = gtk_grid_new ();
                g_object_set_data (G_OBJECT (vpg_window), "v_grid", vpg_grid);
                gtk_window_set_child (GTK_WINDOW (vpg_window), vpg_grid);
                gtk_widget_show (GTK_WIDGET (vpg_window));
                GtkWidget *statusbar = gtk_statusbar_new ();
                g_object_set_data (G_OBJECT (vpg_window), "statusbar", statusbar);
                gtk_grid_attach (GTK_GRID (vpg_grid), statusbar, 1,100, 100,1);
                gtk_widget_show (GTK_WIDGET (statusbar));

        } 
        if (GrMatWin){
                if (vp_exec_mode_name)
                        SetTitle (vp_exec_mode_name);
                else
                        SetTitle ("Probe Graphing Matrix Display");
        }
            
	XSM_DEBUG_PG("DBG-M VIS  " << xlab << " : " << ylab);

	XSM_DEBUG_PG ("RPSPMC_Control::probedata_visualize -- enter");

        if (!probedata_x){
                g_warning ("RPSPMC_Control::probedata_visualize called with probedata_x undefined (=NULL)");
                return;
        }
        if (!probedata_y){
                g_warning ("RPSPMC_Control::probedata_visualize called with probedata_y undefined (=NULL)");
                return;
        }

	// find min and max X limit
	xmin = xmax = g_array_index (probedata_x, double, 0);
	for(int i = 1; i < current_i; i++){
		x = g_array_index (probedata_x, double, i);
		if (x > xmax) xmax = x;
		if (x < xmin) xmin = x;
	}
	xmax *= xmult;
	xmin *= xmult;

	XSM_DEBUG_PG("DBG-M VIS 1");

	XSM_DEBUG_PG("Probing_graph_callback Visualization U&T/Rz" );
	UXaxis->SetAlias (xlab);
	UYaxis->SetAlias (ylab);
	if (!pc){
		XSM_DEBUG_PG("DBG-M VIS c1  ci=" << current_i);
		XSM_DEBUG_PG ("Probing_graph_callback Visualization -- new pc" );
		XSM_DEBUG_PG (ylab << " i:" << current_i);
		gchar   *title  = g_strdup_printf ("Vector Probe, Channel: %s", ylab);
		XSM_DEBUG_PG("DBG-M VIS c2  " << title << " xr= " << xmin << " .. " << xmax << " " << ylab);
                gchar *resid = g_strdelimit (g_strconcat (xlab,ylab,NULL), " ;:()[],./?!@#$%^&*()+-=<>", '_');

		pc = new ProfileControl (main_get_gapp() -> get_app (),
                                         title, current_i, 
                                         UXaxis, UYaxis, 
                                         xmin, xmax,
                                         resid,
                                         GrMatWin ? vpg_app_window : NULL);

                if (GrMatWin){
                        pc->set_pc_matrix_size (num_active_xmaps, num_active_sources);
                        gtk_grid_attach (GTK_GRID (vpg_grid), pc->get_pc_grid (), xmap,src, 1,1);
                }
                
		g_free (resid);
		XSM_DEBUG_PG("DBG-M VIS c3");
		pc->scan1d->mem2d->Resize (current_i, join_same_x ? nas:1);
		XSM_DEBUG_PG("DBG-M VIS c4");
		pc->SetTitle (title);
		XSM_DEBUG_PG("DBG-M VIS c5");
		pc->set_ys_label (0, ylab);
		XSM_DEBUG_PG("DBG-M VIS c6");
		g_free (title);
		XSM_DEBUG_PG("DBG-M VIS cx");

                pc->SetData_dz (1.); // force dz=1.
                // g_message ("PC data.s.dz[%s]=%g", ylab, pc->GetData_dz ());
                
	} else {	
		XSM_DEBUG_PG("DBG-M VIS u1");
		XSM_DEBUG_PG ("Probing_graph_callback Visualization -- add/update pc" );

                if (GrMatWin)
                        pc->set_pc_matrix_size (num_active_xmaps, num_active_sources);

		if(!join_same_x || nas < pc->get_scount ()){
			pc->RemoveScans ();
			pc->scan1d->mem2d->Resize (current_i, join_same_x ? nas:1);
			pc->AddScan (pc->scan1d, 0);
		} else
			pc->scan1d->mem2d->Resize (current_i, join_same_x ? nas:1);

		pc->scan1d->data.s.ny = join_same_x ? nas:1;
		pc->scan1d->data.s.x0=0.;
		pc->scan1d->data.s.y0=0.;
		pc->scan1d->data.s.nvalues=1;
		pc->scan1d->data.s.ntimes=1;
		pc->SetXrange (xmin, xmax);

		if (join_same_x && si >= pc->get_scount ()){
			pc->AddLine (si);
			pc->SetGraphTitle (", ", TRUE);
			pc->SetGraphTitle (ylab, TRUE);
			pc->set_ys_label (si, ylab);
		}
		XSM_DEBUG_PG("DBG-M VIS ux");
	}

	XSM_DEBUG_PG("DBG-M VIS av");
	XSM_DEBUG_PG ("Probing_graph_callback Visualization -- setup data" );

	gint spectra=0; // used to count spectra
	gint spectra_section=0; // used to count spectra
	gint spectra_index=0; // used to count data points within spectra
	gdouble spectra_average[current_i][2]; // holds averaged spectra; ..[0] source ..[1] value
	for(int i = 0; i < current_i; i++){
		spectra_average[i][0]=0;
		spectra_average[i][1]=0;
	}
	for(int i = 0; i < current_i; i++){
		if (g_array_index (probedata_sec, double, i) < spectra_section)
		{
			spectra++;	
			spectra_section = 0;
			spectra_index = 0;
		} else {
			spectra_index++;
			spectra_section = (int) g_array_index (probedata_sec, double, i);
		}
                //g_print ("Ptk[%04d] %g %g\n",i, g_array_index (probedata_x, double, i), g_array_index (probedata_y, double, i));
		pc->SetPoint (i,
			      xmult * g_array_index (probedata_x, double, i),
			      ymult * g_array_index (probedata_y, double, i),
			      join_same_x ? si:0);
		if (vis_PlotAvg & plot_msk){
			spectra_average[spectra_index][0] = spectra_average[spectra_index][0] + g_array_index (probedata_x, double, i);
			spectra_average[spectra_index][1] = spectra_average[spectra_index][1] + g_array_index (probedata_y, double, i);
		}
		if (vis_PlotSec & plot_msk){
			spectra_average[spectra_index][0] = g_array_index (probedata_x, double, i);
			spectra_average[spectra_index][1] = g_array_index (probedata_y, double, i);
		}
	}
	XSM_DEBUG_PG("DBG-M VIS avx");
	XSM_DEBUG_PG ("**Update: #spectra:  " << spectra 
		      << " #points: " << spectra_index 
		      << " si: " << si 
		      << " nas:  " << nas );

	if (current_i > 2){
                if (last_current_i >= current_i){
                        pc->SetScaling (PROFILE_SCALE_XAUTO | PROFILE_SCALE_YAUTO);
                } else {
                        pc->SetScaling (PROFILE_SCALE_XAUTO | PROFILE_SCALE_YEXPAND);
                }
                last_current_i = current_i;
		pc->UpdateArea ();
	}

	// Create graph for averaged data; you will find them in the pc-array above 
	if (spectra>0 && (vis_PlotAvg & plot_msk || vis_PlotSec & plot_msk)){
		XSM_DEBUG_PG("DBG-M VIS avg1");
		XSM_DEBUG_PG ("Probing_graph_callback Visualization new pc -- put Av/Sec data" );
		if (!pc_av){
			XSM_DEBUG_PG ("Probing_graph_callback Visualization -- new pc_av" );
			gchar   *title  = g_strdup_printf ("Vector Probe, Channel: %s %s", ylab, (vis_PlotAvg & plot_msk)?"averaged":"cur. section");
			pc_av = new ProfileControl (main_get_gapp() -> get_app (),
                                                    title, spectra_index+1, 
						    UXaxis, UYaxis, xmin, xmax, ylab);
			pc_av->scan1d->mem2d->Resize (spectra_index+1, join_same_x ? nas:1);
			pc_av->SetTitle (title);
			pc_av->set_ys_label (0, ylab);
			g_free (title);
		} else {
			XSM_DEBUG_PG ("Probing_graph_callback Visualization -- add/update pc_av" );
			if(!join_same_x || nas < pc_av->get_scount ()){
				pc_av->RemoveScans ();
				pc_av->scan1d->mem2d->Resize (spectra_index+1, join_same_x ? nas:1);
				pc_av->AddScan (pc_av->scan1d, 0);
			} else
				pc_av->scan1d->mem2d->Resize (spectra_index+1, join_same_x ? nas:1);
			pc_av->scan1d->data.s.x0=0.;
			pc_av->scan1d->data.s.y0=0.;
			pc_av->scan1d->data.s.nvalues=1;
			pc_av->scan1d->data.s.ntimes=1;
			pc_av->SetXrange (xmin, xmax);
			if (join_same_x && si >= pc_av->get_scount ()){
				pc_av->AddLine (si);
				pc_av->SetGraphTitle (", ", TRUE);
				pc_av->SetGraphTitle (ylab, TRUE);
				pc_av->set_ys_label (si, ylab);
			}
		}
		double norm = 1.;
		if (vis_PlotAvg & plot_msk)
			norm = 1./(spectra+1.);
		for(int i = 0; i < spectra_index+1; i++)
			pc_av->SetPoint (i, 
					 xmult * norm * spectra_average[i][0], 
					 ymult * norm * spectra_average[i][1]/(spectra+1),
					 join_same_x ? si:0);
						
		XSM_DEBUG_PG ("**Update_AV");
		if (current_i > 2){
//			pc_av->mark_for_update();
//			pc_av->auto_update ();
			pc_av->UpdateArea ();
//			pc_av->show ();
		}
		XSM_DEBUG_PG("DBG-M VIS avgx");
	} 
	delete UXaxis;
	delete UYaxis;
	XSM_DEBUG_PG ("RPSPMC_Control::probedata_visualize -- exit");
}


int RPSPMC_Control::Probing_graph_callback( GtkWidget *widget, RPSPMC_Control *dspc, int finish_flag){
// show and update data pv graph
	static ProfileControl *pc[2*MAX_NUM_CHANNELS][2*MAX_NUM_CHANNELS]; // The factor of 2 is to have it big enough to hold also the averaged data

	XSM_DEBUG_PG ("RPSPMC_Control::Probing_graph_callback -- enter");

        dspc->dump_probe_hdr (); // TESTING TRAIL 

//xxxxxxxxxxxxx attach event to active channel, if one exists -- manual mode xxxxxxxxxxxxxxxxxxxxx

	XSM_DEBUG_PG ("Probing_graph_callback MasterScan? Add Ev." );

	ScanEvent *se = NULL;
	if (main_get_gapp()->xsm->MasterScan && finish_flag){
		// find first section header and take this X,Y coordinates as reference -- start of probe event
		XSM_DEBUG_PG ("Probing_graph_callback MasterScan: adding Ev." );
		int sec = 0;
                int j=0;
                //g_message ("Creating SE *** %d %d %d", j, sec, dspc->current_probe_data_index);
                se = new ScanEvent (
                                    main_get_gapp()->xsm->Inst->Dig2X0A ((long) round (g_array_index (dspc->garray_probedata [PROBEDATA_ARRAY_X0], double, j)))
                                    + main_get_gapp()->xsm->Inst->Dig2XA ((long) round (g_array_index (dspc->garray_probedata [PROBEDATA_ARRAY_XS], double, j))),
                                    main_get_gapp()->xsm->Inst->Dig2Y0A ((long) round (g_array_index (dspc->garray_probedata [PROBEDATA_ARRAY_Y0], double, j)))
                                    + main_get_gapp()->xsm->Inst->Dig2YA ((long) round (g_array_index (dspc->garray_probedata [PROBEDATA_ARRAY_YS], double, j))),
                                    main_get_gapp()->xsm->Inst->Dig2ZA ((long) round ( g_array_index (dspc->garray_probedata [PROBEDATA_ARRAY_ZS], double, j)))
                                    );
                main_get_gapp()->xsm->MasterScan->mem2d->AttachScanEvent (se);

                // for all sections add probe event!!!
                XSM_DEBUG_PG ("Probing_graph_callback ScanEvent-update/add" );
                // find chunksize (total # data sources)
                int chunksize = 0;
                GPtrArray *glabarray = g_ptr_array_new ();
                GPtrArray *gsymarray = g_ptr_array_new ();
			
                for (int src=0; dspc->msklookup[src]>=0 && src < MAX_NUM_CHANNELS; ++src)
                        if (dspc->vis_Source & dspc->msklookup[src]){
                                g_ptr_array_add (glabarray, (gpointer) dspc->vp_label_lookup (src));
                                g_ptr_array_add (gsymarray, (gpointer) dspc->vp_unit_lookup (src));
                                ++chunksize;
                                //g_message ("SRC: %s %s", dspc->vp_label_lookup (src), dspc->vp_unit_lookup (src));
                        }
                XSM_DEBUG_PG("DBG-Mb1");
			
                if (chunksize > 0 && chunksize < MAX_NUM_CHANNELS){
                        double dataset[MAX_NUM_CHANNELS];
                        //g_message ("Creating PE j=%d", j);
                        ProbeEntry *pe = new ProbeEntry ("Probe", time(0), glabarray, gsymarray, chunksize);
                        for (int i = 0; i < dspc->current_probe_data_index; i++){
                                //for (int i = j0; i < j; i++){
                                int k=0;
                                XSM_DEBUG_PG("DBG-Mb2");
                                for (int src=0; dspc->msklookup[src]>=0 && src < MAX_NUM_CHANNELS; ++src){
                                        if (dspc->vis_Source & dspc->msklookup[src]){
                                                dataset[k++] = dspc->vp_scale_lookup (src) * g_array_index (dspc->garray_probedata [dspc->expdi_lookup[src]], double, i);
                                                // g_message ("Adding Dataset to PE i%d k%d %g", i, k, dspc->vp_scale_lookup (src) * g_array_index (dspc->garray_probedata [dspc->expdi_lookup[src]], double, i));
                                        }
                                }
                                pe->add ((double*)&dataset);
                                XSM_DEBUG_PG("DBG-Mb3");
                        }
                        se->add_event (pe);
                }
                XSM_DEBUG_PG("DBG-Mb4");
                //g_ptr_array_free (glabarray, TRUE); // passed to PE
                //g_ptr_array_free (gsymarray, TRUE); // passed to PE

                main_get_gapp()->xsm->MasterScan->view->update_events ();
                XSM_DEBUG_PG ("Probing_graph_callback have ScanEvent?" );
	}

        int num_active_xmaps = 0;
	for (int xmap=0; dspc->msklookup[xmap]>=0; ++xmap)
		if (dspc->vis_XSource & dspc->msklookup[xmap])
                        ++num_active_xmaps; 

	int num_active_sources = 0;
	int source_index;
	int src0 = -1;
	for (int src=0; dspc->msklookup[src]>=0; ++src)
		if ((dspc->vis_PSource & dspc->msklookup[src]) && (dspc->vis_Source & dspc->msklookup[src])){
			if (src0 < 0)
				src0 = src;
			++num_active_sources;
		}

        // g_message ("PGV %d x %d",num_active_xmaps,num_active_sources);
        

// on-the-fly visualisation graphs update
	for (int xmap=0; dspc->msklookup[xmap]>=0 && xmap < MAX_NUM_CHANNELS; ++xmap){
		if ((dspc->vis_XSource & dspc->msklookup[xmap]) && (dspc->vis_Source & dspc->msklookup[xmap])){
			for (int src=0, source_index=0; dspc->msklookup[src]>=0 && src < MAX_NUM_CHANNELS; ++src){
				//XSM_DEBUG_PG("DBG-Mbb0a " << dspc->vp_label_lookup (src));
				if (xmap == src) continue;
				XSM_DEBUG_PG("DBG-Mbb0b " << dspc->vp_label_lookup (src));
				if ((dspc->vis_PSource & dspc->msklookup[src]) && (dspc->vis_Source & dspc->msklookup[src])){
					XSM_DEBUG_PG("DBG-Mbb1 " << dspc->vp_label_lookup (src));
					XSM_DEBUG_PG ("Probing_graph_callback Visualisation xmap=" << xmap << " src=" << src << " ** " << dspc->vp_label_lookup (src) << " ( " << dspc->vp_label_lookup (xmap) << " )");
					dspc->probedata_visualize (
						dspc->garray_probedata [dspc->expdi_lookup[xmap]], 
						dspc->garray_probedata [dspc->expdi_lookup[src]], 
						dspc->garray_probedata [PROBEDATA_ARRAY_SEC], 
						dspc->probe_pc_matrix[xmap][dspc->vis_XJoin ? src0:src], 
						dspc->probe_pc_matrix[MAX_NUM_CHANNELS+xmap][MAX_NUM_CHANNELS+dspc->vis_XJoin ? src0:src],
						dspc->msklookup[src],
						dspc->vp_label_lookup (xmap), dspc->vp_unit_lookup (xmap), dspc->vp_scale_lookup (xmap),
						dspc->vp_label_lookup (src), dspc->vp_unit_lookup (src), dspc->vp_scale_lookup (src),
						dspc->current_probe_data_index, source_index++, num_active_sources, dspc->vis_XJoin,
                                                xmap, src, num_active_xmaps, dspc->vis_XJoin ? 1 : num_active_sources);
					XSM_DEBUG_PG("DBG-Mbb1x");
				} else { // clean up unused windows...
					XSM_DEBUG_PG("DBG-Mbb1e cleanup xmap=" << xmap << " src=" << src << " ** " << dspc->vp_label_lookup (src));
					if (dspc->probe_pc_matrix[xmap][src]){
						delete dspc->probe_pc_matrix[xmap][src]; // get rid of it now
						dspc->probe_pc_matrix[xmap][src] = NULL;
					}
					XSM_DEBUG_PG("DBG-Mbb1eX cleanup AV xmap=" << (MAX_NUM_CHANNELS+xmap) << " src=" << (MAX_NUM_CHANNELS+src) << " ** " << dspc->vp_label_lookup (src));
					if (dspc->probe_pc_matrix[MAX_NUM_CHANNELS+xmap][MAX_NUM_CHANNELS+src]){
						delete dspc->probe_pc_matrix[MAX_NUM_CHANNELS+xmap][MAX_NUM_CHANNELS+src]; // get rid of it now
						dspc->probe_pc_matrix[MAX_NUM_CHANNELS+xmap][MAX_NUM_CHANNELS+src] = NULL;
					}
				}
				XSM_DEBUG_PG("DBG-Mbb1ex");
			}
		}else{   // remove not used ones now! pc[2*MAX_NUM_CHANNELS][2*MAX_NUM_CHANNELS]
			XSM_DEBUG_PG("DBG-Mbb2");
			for (int src=0; src<MAX_NUM_CHANNELS; ++src){
				if (dspc->probe_pc_matrix[xmap][src]){
					XSM_DEBUG_PG ("Probing_graph_callback cleanup xmap=" << xmap << " src=" << src  << " ** " << dspc->vp_label_lookup (src));
					delete dspc->probe_pc_matrix[xmap][src]; // get rid of it now
					dspc->probe_pc_matrix[xmap][src] = NULL;
				}
				if (dspc->probe_pc_matrix[MAX_NUM_CHANNELS+xmap][MAX_NUM_CHANNELS+src]){
					XSM_DEBUG_PG ("Probing_graph_callback cleanup AV xmap=" << xmap << " src=" << src << " ** " << dspc->vp_label_lookup (src) );
					delete dspc->probe_pc_matrix[MAX_NUM_CHANNELS+xmap][MAX_NUM_CHANNELS+src]; // get rid of it now
                                        dspc->probe_pc_matrix[MAX_NUM_CHANNELS+xmap][MAX_NUM_CHANNELS+src] = NULL;
				}
			}
		}
	}
	XSM_DEBUG_PG ("RPSPMC_Control::Probing_graph_callback -- done");
	return 0;
}

// abort probe and stop fifo read, plot data until then
int RPSPMC_Control::Probing_abort_callback( GtkWidget *widget, RPSPMC_Control *dspc){
        rpspmc_hwi->GVP_abort_vector_program ();

	dspc->Probing_graph_callback (widget, dspc);
        
	// can not simply cancel a DSP vector program in progress -- well can, but: this leaves it in an undefined state of all effected outputs incl.
	// ==> feedback state ON or OFF. SO AFTER THAT -- CHEC and eventually manually recover settings!
	// but aborting on your request

	// **** dspc->Probing_exec_ABORT_callback (widget, dspc);
        
        return 0;
}



int RPSPMC_Control::Probing_save_callback( GtkWidget *widget, RPSPMC_Control *dspc){
	int sec, bi;
	double t0=0.;

	if (!dspc->current_probe_data_index && !dspc->nun_valid_hdr) 
		return 0;
	
	const gchar *separator = "\t";

	std::ofstream f;

	// XsmRescourceManager xrm("FilingPathMemory");
	// gchar *path = xrm.GetStr ("Probe_DataSavePath", xsmres.DataPath);

	gchar *fntmp = g_strdup_printf ("%s/%s%03d-VP%03d-%s.vpdata", 
					// path, 
					g_settings_get_string (main_get_gapp()->get_as_settings (), "auto-save-folder-probe"), 
					main_get_gapp()->xsm->data.ui.basename, main_get_gapp()->xsm->GetFileCounter(), main_get_gapp()->xsm->GetNextVPFileCounter(), "VP");
	// g_free (path);

	time_t t;
	time(&t);

	f.open (fntmp);

	int ix=-999999, iy=-999999;
	if (main_get_gapp()->xsm->MasterScan){
		main_get_gapp()->xsm->MasterScan->World2Pixel (main_get_gapp()->xsm->data.s.x0, main_get_gapp()->xsm->data.s.y0, ix, iy, SCAN_COORD_ABSOLUTE);
	}
// better, get realtime DSP position readings for XY0 now if available:

	double x0=0.;
	double y0=0.;
	double z0=0.;
	if (main_get_gapp()->xsm->hardware->RTQuery ("O", z0, x0, y0)){ // get HR Offset
//		gchar *tmp = NULL;
//		tmp = g_strdup_printf ("Offset Z0: %7.3f "UTF8_ANGSTROEM"\nXY0: %7.3f "UTF8_ANGSTROEM", %7.3f "UTF8_ANGSTROEM
//				       "\nXYs: %7.3f "UTF8_ANGSTROEM", %7.3f "UTF8_ANGSTROEM,
//				       main_get_gapp()->xsm->Inst->V2ZAng(z0),
//				       main_get_gapp()->xsm->Inst->V2XAng(x0),
//				       main_get_gapp()->xsm->Inst->V2YAng(y0),
//				       main_get_gapp()->xsm->Inst->V2XAng(x),
//				       main_get_gapp()->xsm->Inst->V2YAng(y));
		x0 = main_get_gapp()->xsm->Inst->V2XAng(x0);
		y0 = main_get_gapp()->xsm->Inst->V2YAng(y0);
		if (main_get_gapp()->xsm->MasterScan){
			main_get_gapp()->xsm->MasterScan->World2Pixel (x0, y0, ix, iy, SCAN_COORD_ABSOLUTE);
		}
	} else {
		if (main_get_gapp()->xsm->MasterScan){
			x0 = main_get_gapp()->xsm->data.s.x0;
			y0 = main_get_gapp()->xsm->data.s.y0;
		}
	}
        f.precision (12);
	f << "# view via: xmgrace -graph 0 -pexec 'title \"GXSM Vector Probe Data: " << fntmp << "\"' -block " << fntmp  << " -bxy 2:4 ..." << std::endl;
	f << "# GXSM Vector Probe Data :: VPVersion=00.02 vdate=20070227" << std::endl;
	f << "# Date                   :: date=" << ctime(&t) << "#" << std::endl;
	f << "# FileName               :: name=" << fntmp << std::endl;
	f << "# GXSM-Main-Offset       :: X0=" <<  x0 << " Ang" <<  "  Y0=" << y0 << " Ang" 
	  << ", iX0=" << ix << " Pix iX0=" << iy << " Pix"
	  << std::endl;
        if (main_get_gapp()->xsm->MasterScan)
                f << "# DSP SCANCOORD POSITION :: DSP-XSpos=" 
                  << (((int)g_array_index (dspc->garray_probe_hdrlist[PROBEDATA_ARRAY_XS], double, 0)<<16)/dspc->mirror_dsp_scan_dx32 + main_get_gapp()->xsm->MasterScan->data.s.nx/2 - 1)
                  << " DSP-YSpos=" 
                  << ((main_get_gapp()->xsm->MasterScan->data.s.nx/2 - 1) - ((int)g_array_index (dspc->garray_probe_hdrlist[PROBEDATA_ARRAY_YS], double, 0)<<16)/dspc->mirror_dsp_scan_dy32)
                  << " CENTER-DSP-XSpos=" 
                  << (((int)g_array_index (dspc->garray_probe_hdrlist[PROBEDATA_ARRAY_XS], double, 0)<<16)/dspc->mirror_dsp_scan_dx32)
                  << " CENTER-DSP-YSpos=" 
                  << (((int)g_array_index (dspc->garray_probe_hdrlist[PROBEDATA_ARRAY_YS], double, 0)<<16)/dspc->mirror_dsp_scan_dy32)
                  << std::endl;
        else
                f << "# DSP SCANCOORD POSITION :: NO MASTERSCAN SCAN COORDINATES N/A" << std::endl;
	f << "# GXSM-DSP-Control-FB    :: Bias=" << dspc->bias << " V" <<  ", Current=" << dspc->mix_set_point[0]  << " nA" << std::endl; 
	f << "# GXSM-DSP-Control-STS   :: #IV=" << dspc->IV_repetitions << " " << std::endl; 
	f << "# GXSM-DSP-Control-LOCKIN:: AC_amp=[ " 
	  << dspc->AC_amp[0] << " V, " << dspc->AC_amp[1] << ", " << dspc->AC_amp[2] << ", " << dspc->AC_amp[3] << "], "
	  << " AC_frq=" << dspc->AC_frq << " Hz, "
                //<< " AC_phaseA=" << dspc->AC_phaseA << " deg, "
                //<< " AC_phaseB=" << dspc->AC_phaseB << " deg, "
                //<< " AC_avg_cycles=" << dspc->AC_lockin_avg_cycels
	  << " " << std::endl; 

	gchar *tmp = g_strdup(main_get_gapp()->xsm->data.ui.comment);
	gchar *cr;
	while (cr=strchr (tmp, 0x0d))
		*cr = ' ';
	while (cr=strchr (tmp, 0x0a))
		*cr = ' ';
	
	f << "# GXSM-Main-Comment      :: comment=\"" << tmp << "\"" << std::endl;
	g_free (tmp);
	f << "# Probe Data Number      :: N=" << dspc->current_probe_data_index << std::endl;
	f << "# Data Sources Mask      :: Source=" << dspc->vis_Source << std::endl;
	f << "# X-map Sources Mask     :: XSource=" << dspc->vis_XSource << std::endl;
	f << "#C " << std::endl;
	f << "#C VP Channel Map and Units lookup table used:=table [## msk expdi, lab, DAC2U, unit/DAC, Active]" << std::endl;

	for (int i=0; dspc->msklookup[i] >= 0; ++i)
		f << "# Cmap[" << i << "]" << separator 
		  << dspc->msklookup[i] << separator 
		  << dspc->expdi_lookup[i] << separator 
		  << dspc->vp_label_lookup (i) << separator 
		  << dspc->vp_scale_lookup (i) << separator 
		  << dspc->vp_unit_lookup (i) << "/DAC" << separator 
		  << (dspc->vis_Source & dspc->msklookup[i] ? "Yes":"No") << std::endl;

	f << "#C " << std::endl;
	f << "#C Full Position Vector List at Section boundaries follows :: PositionVectorList" << std::endl;
	sec = 0; bi = -1;
	for (int j=0; j<dspc->current_probe_data_index; ++j){
		int bsi = g_array_index (dspc->garray_probedata [PROBEDATA_ARRAY_BLOCK], double, j);
		int s = (int) g_array_index (dspc->garray_probedata [PROBEDATA_ARRAY_SEC], double, j);
		if (bsi != bi){
			bi = bsi;
			f << "# S[" << s << "]  :: VP[" << j <<"]=(";
		} else
			continue;

		for (int i=13; dspc->msklookup[i]>=0; ++i){
			double ymult = dspc->vp_scale_lookup (i);
			if (i>13) 
				f << ", ";
			f << " \"" << dspc->vp_label_lookup (i) << "\"="
			  << (ymult * g_array_index (dspc->garray_probedata [dspc->expdi_lookup[i]], double, j))
			  << " " << dspc->vp_unit_lookup (i);
		}
		f << ")" << std::endl;
	}

	f << "#C " << std::endl;
	f << "#C Data Table             :: data=" << std::endl;
        f.precision (12);
        f << std::scientific;
	for (int i = -1; i < dspc->current_probe_data_index; i++){
		if (i == -1)
			f << "#C Index" << separator;
		else
			f << i << separator;

		for (int xmap=0; dspc->msklookup[xmap]>=0; ++xmap)
			if ((dspc->vis_XSource & dspc->msklookup[xmap]) && (dspc->vis_Source & dspc->msklookup[xmap])){
				double xmult = dspc->vp_scale_lookup (xmap);
				if (i == -1)
					f << "\"" << dspc->vp_label_lookup (xmap) << " (" << dspc->vp_unit_lookup (xmap) << ")\"" << separator;
				else
					f << (xmult * g_array_index (dspc->garray_probedata [dspc->expdi_lookup[xmap]], double, i)) << separator;
			}

		for (int src=0; dspc->msklookup[src]>=0; ++src)
			if (dspc->vis_Source & dspc->msklookup[src]){
				double ymult = dspc->vp_scale_lookup (src);
				if (i == -1)
					f << "\"" << dspc->vp_label_lookup (src) << " (" << dspc->vp_unit_lookup (src) << ")\"" << separator;
				else
					f << (ymult * g_array_index (dspc->garray_probedata [dspc->expdi_lookup[src]], double, i)) << separator;
			}

		if (i == -1)
			f << "Block-Start-Index";
		else
			f << g_array_index (dspc->garray_probedata [PROBEDATA_ARRAY_BLOCK], double, i);
		f << std::endl;
	}
	f << "#C " << std::endl;
	f << "#C END." << std::endl;

	f << "#C START OF HEADER LIST APPENDIX" << std::endl;

	f << "#C Vector Probe Header List -----------------" << std::endl;
	f << "#C # ####\t time[ms]  \t dt[ms]    \t X[Ang]   \t Y[Ang]   \t Z[Ang]    \t Sec" << std::endl;
	for (int i=0; i<dspc->nun_valid_hdr; ++i){
		double val[10];
		val[0] = g_array_index (dspc->garray_probe_hdrlist[PROBEDATA_ARRAY_TIME], double, i);
		val[1] = g_array_index (dspc->garray_probe_hdrlist[PROBEDATA_ARRAY_XS], double, i);
		val[2] = g_array_index (dspc->garray_probe_hdrlist[PROBEDATA_ARRAY_YS], double, i);
		val[3] = g_array_index (dspc->garray_probe_hdrlist[PROBEDATA_ARRAY_ZS], double, i);
		val[4] = g_array_index (dspc->garray_probe_hdrlist[PROBEDATA_ARRAY_SEC], double, i);

// 1e3/DSP_FRQ_REF, XAngFac, YAngFac, ZAngFac,
                
		f << "#  " << std::setw(6) << i << "\t "
		  << std::setw(10) << val[0] * 1e3/DSP_FRQ_REF << "\t " << std::setw(10) << (val[0]-t0) * 1e3/DSP_FRQ_REF << "\t "
		  << std::setw(10) << val[1] * XAngFac << "\t "
		  << std::setw(10) << val[2] * YAngFac << "\t "
		  << std::setw(10) << val[3] * ZAngFac << "\t "
		  << std::setw(2) << val[4] << "\t "
		  << std::endl;
		t0 = val[0];
	}
	f << "#C END OF HEADER LIST APPENDIX." << std::endl;

	f.close ();

	// update counter on main window!
	dspc->update_gui_thread_safe (fntmp);
	g_free (fntmp);

	return 0;
}



#define DEFAULT_PROBE_LEN 256 // can increase automatically, just more efficient

void RPSPMC_Control::push_probedata_arrays (){
	GArray **garrp = new GArray*[NUM_PROBEDATA_ARRAYS];
	GArray **garrh = new GArray*[NUM_PROBEDATA_ARRAYS];

        GXSM_REF_OBJECT(GXSM_GRC_PRBHDR);
        GXSM_REF_OBJECT(GXSM_GRC_PRBVEC);

        pv_lock = TRUE;
	for (int i=0; i<NUM_PROBEDATA_ARRAYS; ++i){
		garrp [i] = garray_probedata [i];
		garray_probedata [i] = g_array_sized_new (FALSE, TRUE, sizeof (double), DEFAULT_PROBE_LEN); // preallocated, can increase
	}
	probedata_list =  g_slist_prepend (probedata_list, garrp); // push probe data on list
	last_probe_data_index = current_probe_data_index;

	for (int i=0; i<NUM_PROBEDATA_ARRAYS; ++i){
		garrh [i] = garray_probe_hdrlist [i];
		garray_probe_hdrlist [i] = g_array_sized_new (FALSE, TRUE, sizeof (double), DEFAULT_PROBE_LEN); // preallocated, can increase
	}
	probehdr_list =  g_slist_prepend (probehdr_list, garrh); // push section headers on list
      
	++num_probe_events;
	pv_lock = FALSE;
}

// return last dataset and removed it from list, does not touch data itself
GArray** RPSPMC_Control::pop_probedata_arrays (){
	pv_lock = TRUE;
	if (probedata_list){
		GSList *last = g_slist_last (probedata_list);
		if (last){
			GArray **garr = (GArray **) (last->data);
			probedata_list = g_slist_delete_link (probedata_list, last);
                        // probedata_list = g_slist_remove_link (probehdr_list, last);
                        pv_lock = FALSE;
			return garr;
		}
	}
	pv_lock = FALSE;
	return NULL;
}

GArray** RPSPMC_Control::pop_probehdr_arrays (){
	pv_lock = TRUE;
	if (probehdr_list){
		GSList *last = g_slist_last (probehdr_list);
		if (last){
			GArray **garr = (GArray **) (last->data);
			probehdr_list = g_slist_delete_link (probehdr_list, last);
                        // probehdr_list = g_slist_remove_link (probehdr_list, last);
                        pv_lock = FALSE;
			return garr;
		}
	}
	pv_lock = FALSE;
	return NULL;
}

void RPSPMC_Control::free_probedata_array_set (GArray** garr, RPSPMC_Control *dc){
        if (!garr) return;
	dc->pv_lock = TRUE;
	for (int i=0; i<NUM_PROBEDATA_ARRAYS; ++i)
		g_array_unref (garr[i]);
	dc->pv_lock = FALSE;

        GXSM_UNREF_OBJECT(GXSM_GRC_PRBVEC);      
}

void RPSPMC_Control::free_probehdr_array_set (GArray** garr, RPSPMC_Control *dc){
        if (!garr) return;
	dc->pv_lock = TRUE;
 	for (int i=0; i<NUM_PROBEDATA_ARRAYS; ++i)
                g_array_unref (garr[i]);
	dc->pv_lock = FALSE;

        GXSM_UNREF_OBJECT(GXSM_GRC_PRBHDR);
}

void RPSPMC_Control::free_probedata_arrays (){
	pv_lock = TRUE;
	if (probedata_list){
                g_slist_foreach (probedata_list, (GFunc) RPSPMC_Control::free_probedata_array_set, this);
                g_slist_free (probedata_list);
                probedata_list = NULL;	
        }

	if (probehdr_list){
                g_slist_foreach (probehdr_list, (GFunc) RPSPMC_Control::free_probehdr_array_set, this);
                g_slist_free (probehdr_list);
                probehdr_list = NULL;	
        }

        num_probe_events = 0;
        pv_lock = FALSE;
}

void RPSPMC_Control::init_probedata_arrays (){
	for(int i=0; i<4; ++i)
		vp_input_id_cache[i]=-1;
	for (int i=0; i<NUM_PROBEDATA_ARRAYS; ++i){
		if (!garray_probedata[i])
			garray_probedata [i] = g_array_sized_new (FALSE, TRUE, sizeof (double), DEFAULT_PROBE_LEN); // preallocated, can increase
		else
			g_array_set_size (garray_probedata [i], 0);

		if (!garray_probe_hdrlist[i])
			garray_probe_hdrlist [i] = g_array_sized_new (FALSE, TRUE, sizeof (double), DEFAULT_PROBE_LEN); // preallocated, can increase
                else
			g_array_set_size (garray_probe_hdrlist [i], 0);
	}

	for (int i=0; i<NUM_PV_HEADER_SIGNALS; ++i)
                pv_tmp[i] = 0.0;
        
	current_probe_data_index = 0;
        current_probe_section = 0;
	nun_valid_data_sections = 0;
	nun_valid_hdr = 0;
	last_nun_hdr_dumped = 0;
}

//#define TTY_DEBUG
void RPSPMC_Control::add_probe_hdr(double pv[NUM_PV_HEADER_SIGNALS]){ 
	int i;
        // append header
        double dind = (double)current_probe_data_index;
	g_array_append_val (garray_probe_hdrlist [PROBEDATA_ARRAY_INDEX], dind);
	for (i = PROBEDATA_ARRAY_TIME; i <= PROBEDATA_ARRAY_SEC; ++i)
		g_array_append_val (garray_probe_hdrlist[i], pv[i]);

	++nun_valid_hdr;
}

// set section start reference position/values for vector generation
void RPSPMC_Control::set_probevector(double pv[NUM_PV_HEADER_SIGNALS]){ 
	int i,j;

#ifdef TTY_DEBUG
        g_print ("***************** SET_PV [%d] section = %d",  current_probe_data_index, (int)pv[PROBEDATA_ARRAY_SEC]);
#endif
        current_probe_section = (int)pv[PROBEDATA_ARRAY_SEC];

        double dind = (double)current_probe_data_index;
	g_array_append_val (garray_probedata [PROBEDATA_ARRAY_INDEX], dind);
        g_array_append_val (garray_probedata [PROBEDATA_ARRAY_BLOCK], dind);

        if (!nun_valid_data_sections)
                for (int i=0; i<NUM_PV_HEADER_SIGNALS; ++i)
                        pv_tmp[i] = pv[i];

        if (!nun_valid_data_sections){
                for (i = PROBEDATA_ARRAY_TIME, j=1; i <= PROBEDATA_ARRAY_SEC; ++i, ++j)
                        g_array_append_val (garray_probedata [i], pv_tmp[j]);

                ++nun_valid_data_sections;
        }

#ifdef TTY_DEBUG
	g_print ("### SET_PV [%04d] {sec=%d}= (", current_probe_data_index, current_probe_section);
	for (i = PROBEDATA_ARRAY_INDEX, j=0; i <= PROBEDATA_ARRAY_SEC; ++i, ++j)
		g_print("%g_[%d], ", pv[j],i);
        g_print("\n");
#endif
}

// add probe vector to generate ramp reference signals (same as on hardware, but this data is not streamed as redundent)
void RPSPMC_Control::add_probevector(){ 
	int i,j;
	double val, multi, fixptm;
        if (current_probe_data_index < 1){ // must have previous data point
                g_warning ("### *** add_probevector() call at first point is invalid, skipping. point index=%d*** ", current_probe_data_index);
                return;
        }
        
        double dsec = (double)current_probe_section;
        double dind = (double)current_probe_data_index;
	g_array_append_val (garray_probedata [PROBEDATA_ARRAY_SEC], dsec);
	g_array_append_val (garray_probedata [PROBEDATA_ARRAY_INDEX], dind);

	multi = 1. + program_vector_list[current_probe_section].dnx;

	// copy Block Start Index
	val = g_array_index (garray_probedata [PROBEDATA_ARRAY_BLOCK], double, current_probe_data_index-1);
	g_array_append_val (garray_probedata [PROBEDATA_ARRAY_BLOCK], val);

        g_print ("###ADD_PV[%04d] sec=%d blk=%d (du %d  dxyz %d %d %d) ",
                 current_probe_data_index, current_probe_section, (int)val,
                 program_vector_list[current_probe_section].f_du,
                 program_vector_list[current_probe_section].f_dx, program_vector_list[current_probe_section].f_dy, program_vector_list[current_probe_section].f_dz
                 );
#ifdef TTY_DEBUG
	g_print(" <+=> (multi=%d) PV=( ", (int)multi);
#endif
	for (i = PROBEDATA_ARRAY_TIME; i < PROBEDATA_ARRAY_SEC; ++i){
		// val = g_array_index (garray_probedata[i], double, current_probe_data_index-1); // get previous, then add delta
                val = pv_tmp[i];
#ifdef TTY_DEBUG_PREV
		g_print("%d => ", (int)val);
#endif
		switch (i){
		case PROBEDATA_ARRAY_TIME:
			val += multi;
			break;
		case PROBEDATA_ARRAY_X0:
			val += program_vector_list[current_probe_section].f_dx0*multi;
			break;
		case PROBEDATA_ARRAY_Y0:
			val += program_vector_list[current_probe_section].f_dy0*multi;
			break;
		case PROBEDATA_ARRAY_PHI: // *** FIX ME ***
			val += program_vector_list[current_probe_section].f_dz0*multi;
			break;
		case PROBEDATA_ARRAY_XS:
			val -= program_vector_list[current_probe_section].f_dx*multi;
			break;
		case PROBEDATA_ARRAY_YS:
			val -= program_vector_list[current_probe_section].f_dy*multi;
			break;
		case PROBEDATA_ARRAY_ZS:
			val -= program_vector_list[current_probe_section].f_dz*multi;
			break;
		case PROBEDATA_ARRAY_U:
			val += program_vector_list[current_probe_section].f_du*multi;
			break;
		default:
			break; // error!!!!
		}
                pv_tmp[i] = val;
		g_array_append_val (garray_probedata[i], val);
#ifdef TTY_DEBUG
		g_print("%d_[%d], ", (int)val,i);
#endif
	}
	++nun_valid_data_sections;

#ifdef TTY_DEBUG
        g_print(" ) #{%d : %d} \n", nun_valid_data_sections, current_probe_data_index);
#endif
}

// CALL ONLY THIS EXTERNALLY TO ADD DATA
void RPSPMC_Control::add_probedata(double data[NUM_PV_DATA_SIGNALS], double pv[NUM_PV_HEADER_SIGNALS], gboolean set_pv){ 
	int i,j;
        
        pv_lock = TRUE;
        // create and add vector generated signals
        if (set_pv){
                g_print ("+++>>>> add_probedata add_hdr sec=%d and set probe vector\n", (int)pv[PROBEDATA_ARRAY_SEC]);
                // add probe section header info
                add_probe_hdr (pv);
                // add (set) section start reference position/values for vector signal generation
                set_probevector (pv);
        }
        g_print ("+++>>>> add_probedata add_vec sec=%d\n", (int)pv[PROBEDATA_ARRAY_SEC]);
        add_probevector();

        // add data channels
        g_print ("+++>>>> add_probedata add_data sec=%d\n", (int)pv[PROBEDATA_ARRAY_SEC]);
	for (i = PROBEDATA_ARRAY_S1, j=0; i <= PROBEDATA_ARRAY_END; ++i, ++j)
		g_array_append_val (garray_probedata[i], data[j]);

#ifdef TTY_DEBUG
	std::cout << "###ADD_PDATA[" << current_probe_data_index << "]: ";
	for (i = PROBEDATA_ARRAY_S1, j=0; i <= PROBEDATA_ARRAY_END; ++i, ++j)
		std::cout << ((int)(data[j])) << ", ";
	std::cout << std::endl;
#endif

	current_probe_data_index++;	
	pv_lock = FALSE;
}



void RPSPMC_Control::dump_probe_hdr(){ 
#define VP_TRAIL_LEN 256
	static VObEvent *vp_trail[VP_TRAIL_LEN];
	static int vp_trail_i = -1;
	static int vp_trail_n = -1;
	static double x=0.;
	static double y=0.;
	double s2=0.;
	double val[10];
	double t0=0.;

        // #define DUMP_TERM
#ifdef DUMP_TERM
	std::cout << "Vector Probe Header List -----------------" << std::endl;
	std::cout << "# ####\t time[ms]  \t dt[ms]    \t X[Ang]   \t Y[Ang]   \t Z[Ang]    \t Sec" << std::endl;
#endif

	for (int i=last_nun_hdr_dumped; i<nun_valid_hdr; ++i){
		val[0] = g_array_index (garray_probe_hdrlist[PROBEDATA_ARRAY_TIME], double, i);
		val[1] = g_array_index (garray_probe_hdrlist[PROBEDATA_ARRAY_XS], double, i);
		val[2] = g_array_index (garray_probe_hdrlist[PROBEDATA_ARRAY_YS], double, i);
		val[3] = g_array_index (garray_probe_hdrlist[PROBEDATA_ARRAY_ZS], double, i);
		val[4] = g_array_index (garray_probe_hdrlist[PROBEDATA_ARRAY_SEC], double, i);

#ifdef DUMP_TERM
// 1e3/DSP_FRQ_REF, XAngFac, YAngFac, ZAngFac,
		std::cout << std::setw(6) << i << "\t "
			  << std::setw(10) << val[0] * 1e3/DSP_FRQ_REF << "\t " << std::setw(10) << (val[0]-t0) * 1e3/DSP_FRQ_REF << "\t "
			  << std::setw(10) << val[1] * XAngFac << "\t "
			  << std::setw(10) << val[2] * YAngFac << "\t "
			  << std::setw(10) << val[3] * ZAngFac << "\t "
			  << std::setw(2) << val[4] << "\t "
			  << std::endl;
#endif
		t0 = val[0];

		if (val[4] == 0 && main_get_gapp()->xsm->MasterScan){
			if (val[0] < 1.){ // TIME = 0 at start, initialize
				for(int i=0; i<VP_TRAIL_LEN; ++i)
					vp_trail[i]=NULL;
				vp_trail_i=0;
				vp_trail_n=0;
				x=val[1];
				y=val[2];
			}
			s2 = (x-val[1])*(x-val[1]) + (y-val[2])*(y-val[2]);
			if (i==0)
				s2 = 999999.;

			// only put new marker/update/rotate if more than a pixel moved!
			if (s2 > 2.*main_get_gapp()->xsm->MasterScan->data.s.dx*main_get_gapp()->xsm->MasterScan->data.s.dx+main_get_gapp()->xsm->MasterScan->data.s.dy*main_get_gapp()->xsm->MasterScan->data.s.dy){
				gchar *info = g_strdup_printf ("TrkPt# %d %.1fms", i, val[0]*1e3/DSP_FRQ_REF);
				double xyz[3] = {
					main_get_gapp()->xsm->Inst->Dig2XA ((long) round (val[1]))
					+ main_get_gapp()->xsm->Inst->Dig2X0A ((long) round ( g_array_index (garray_probe_hdrlist[PROBEDATA_ARRAY_X0], double, i))),
					main_get_gapp()->xsm->Inst->Dig2YA ((long) round (val[2]))
					+ main_get_gapp()->xsm->Inst->Dig2Y0A ((long) round ( g_array_index (garray_probe_hdrlist[PROBEDATA_ARRAY_Y0], double, i))),
					main_get_gapp()->xsm->Inst->Dig2ZA ((long) round (val[3]))
				};
				
				x=val[1];
				y=val[2];

				// add simple Marker ------------------------------------- moving trail/tail mode
				if (main_get_gapp()->xsm->MasterScan->view->Get_ViewControl()){ // if view available, add markers
					double xy[2] = {xyz[0], xyz[1]};
					if (vp_trail_n < VP_TRAIL_LEN){
						ViewControl *vc = main_get_gapp()->xsm->MasterScan->view->Get_ViewControl();
						VObEvent *vp;
						if (vp_trail_n == 0)
							vc->RemoveIndicators ();
						else
							vp_trail[vp_trail_n-1]->set_color_to_custom (color_gray, color_yellow);
						vc->AddIndicator (vp = new VObEvent (vc->GetCanvas (), xy, FALSE, VOBJ_COORD_ABSOLUT, info, 0.25));
						vp->set_marker_scale (0.25);
						vp->set_obj_name ("*Trailpoint");
						vp_trail[vp_trail_n] = vp;

						++vp_trail_n;
						++vp_trail_i;
					} else { // PLEASE DO NOT DELETED OBJECTs WHILE TK IS IN ACTION!!!!
						if (vp_trail[(vp_trail_i-1) %  VP_TRAIL_LEN])
							vp_trail[(vp_trail_i-1) %  VP_TRAIL_LEN]->set_color_to_custom (color_gray, color_yellow);
						vp_trail_i %= VP_TRAIL_LEN;
						if (vp_trail[vp_trail_i]){
							vp_trail[vp_trail_i]->set_xy_node (xy, VOBJ_COORD_ABSOLUT);
							vp_trail[vp_trail_i]->set_marker_scale (0.25);
							vp_trail[vp_trail_i]->set_object_label (info);
							vp_trail[vp_trail_i]->set_color_to_custom (color_red, color_red);
						}
						++vp_trail_i;
					}
//				std::cout << "Marker at " << xy[0] << ", " << xy[1] << " " << info << std::endl;
				}
				main_get_gapp()->xsm->MasterScan->view->update_events ();
			}
		}
	}
	last_nun_hdr_dumped = nun_valid_hdr;
}
