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
#include <gtk/gtk.h>

#include <cairo-svg.h>
#include <cairo-pdf.h>
#include <iomanip>

// Gxsm headers 
#include "gxsm_app.h"
#include "gxsm_window.h"

#include "unit.h"
#include "util.h"
#include "pcs.h"
#include "xsmtypes.h"
#include "glbvars.h"
#include "action_id.h"
#include "app_profile.h"

#include "gxsm_monitor_vmemory_and_refcounts.h"

// ============================================================
// Popup Menu and Object Action Map
// ============================================================

static GActionEntry win_profile_popup_entries[] = {
        { "file-open", ProfileControl::file_open_callback, NULL, NULL, NULL },
        { "file-save", ProfileControl::file_save_callback, NULL, NULL, NULL },
        { "file-image-save", ProfileControl::file_save_image_callback, NULL, NULL, NULL },
        { "file-save-data", ProfileControl::file_save_data_callback, NULL, NULL, NULL },
        
        { "print", ProfileControl::file_print5_callback, NULL, NULL, NULL },

        { "skl-Y-auto", ProfileControl::skl_Yauto_callback, NULL, "true", NULL },
        { "skl-Y-log",  ProfileControl::logy_callback, NULL, "false", NULL },
        { "skl-Y-hold",  ProfileControl::yhold_callback, NULL, "false", NULL },
        { "skl-Y-expand",  ProfileControl::yexpand_callback, NULL, "false", NULL },

        { "opt-Y-linreg",  ProfileControl::linreg_callback, NULL, "false", NULL },
        { "opt-Y-diff",  ProfileControl::ydiff_callback, NULL, "false", NULL },
        { "opt-Y-ft",  ProfileControl::psd_callback, NULL, "false", NULL },
        { "opt-decimate",  ProfileControl::decimate_callback, NULL, "true", NULL },
        { "opt-Y-binary",  ProfileControl::skl_Binary_callback, NULL, "false", NULL },
        { "opt-Y-lowpass-cycle",  ProfileControl::ylowpass_cycle_callback, NULL, "false", NULL },
        { "opt-Y-lowpass",  ProfileControl::ylowpass_callback, "s", "'lp-off'", NULL },

        { "cursor-A",  ProfileControl::cur_Ashow_callback, NULL, "false", NULL },
        { "cursor-B",  ProfileControl::cur_Bshow_callback, NULL, "false", NULL },
        { "opt-XR-AB",  ProfileControl::opt_xr_ab_callback, NULL, "false", NULL },

        { "cursor-A-left",  ProfileControl::cur_Aleft_callback, NULL, NULL, NULL },
        { "cursor-A-right",  ProfileControl::cur_Aright_callback, NULL, NULL, NULL }, 
        { "cursor-A-rmin",  ProfileControl::cur_Armin_callback, NULL, NULL, NULL },
        { "cursor-A-rmax",  ProfileControl::cur_Armax_callback, NULL, NULL, NULL },
        { "cursor-A-lmin",  ProfileControl::cur_Almin_callback, NULL, NULL, NULL },
        { "cursor-A-lmax",  ProfileControl::cur_Almax_callback, NULL, NULL, NULL },
        { "cursor-B-left",  ProfileControl::cur_Bleft_callback, NULL, NULL, NULL },
        { "cursor-B-right",  ProfileControl::cur_Bright_callback, NULL, NULL, NULL },
        { "cursor-B-rmin",  ProfileControl::cur_Brmin_callback, NULL, NULL, NULL },
        { "cursor-B-rmax",  ProfileControl::cur_Brmax_callback, NULL, NULL, NULL },
        { "cursor-B-lmin",  ProfileControl::cur_Blmin_callback, NULL, NULL, NULL },
        { "cursor-B-lmax",  ProfileControl::cur_Blmax_callback, NULL, NULL, NULL },

        { "settings-xgrid",   ProfileControl::settings_xgrid_callback, NULL, "true", NULL },
        { "settings-ygrid",   ProfileControl::settings_ygrid_callback, NULL, "true", NULL },
        { "settings-notics",  ProfileControl::settings_notics_callback, NULL, "false", NULL },
        { "settings-pathmode",ProfileControl::settings_pathmode_radio_callback, "s", "'connect'", NULL },
        { "settings-header",  ProfileControl::settings_header_callback, NULL, "false", NULL },
        { "settings-legend",  ProfileControl::settings_legend_callback, NULL, "false", NULL },
        { "settings-series-tics",  ProfileControl::settings_series_tics_callback, NULL, "false", NULL }

};

#define LOG_RESCUE_RATIO 100.

ProfileElement::ProfileElement(Scan *sc, ProfileControl *pc, const gchar *col, int Yy){
	XSM_DEBUG (DBG_L2, "ProfileElement::ProfileElement sc");
	yy = Yy;
	scan = sc;
	psd_scan = NULL;

	if(col)
		color = g_strdup(col);
	else
		color = g_strdup("black");

	pctrl=pc;
	for (int i=0; i<NPI; ++i)
		pathitem[i] = pathitem_draw[i]   = NULL;
	XSM_DEBUG (DBG_L2, "ProfileElement::ProfileElement sc done");
}

ProfileElement::ProfileElement(ProfileElement *pe, const gchar *col, int Yy){
	XSM_DEBUG (DBG_L2, "ProfileElement::ProfileElement pe");
	yy = Yy;
	scan = pe->scan;
	psd_scan = NULL;

	if(col)
		color = g_strdup(col);
	else
		color = g_strdup("black");

	pctrl=pe->pctrl;
	for (int i=0; i<NPI; ++i)
		pathitem[i] = pathitem_draw[i]   = NULL;
	XSM_DEBUG (DBG_L2, "ProfileElement::ProfileElement pe done.");
}

ProfileElement::~ProfileElement(){
	XSM_DEBUG (DBG_L2, "ProfileElement::~ProfileElement");
	for (int i=0; i<NPI; ++i){
		UNREF_DELETE_CAIRO_ITEM_NO_UPDATE (pathitem[i]);
                UNREF_DELETE_CAIRO_ITEM_NO_UPDATE (pathitem_draw[i]);
        }
	g_free(color);
}

void ProfileElement::calc_decimation (gint64 ymode){
        if (ymode & PROFILE_MODE_DECIMATE){
                gint w = (gint) (pctrl->get_drawing_width());
                if (n > 2*w){
                        dec_len = n / w;
                        n_dec   = n / dec_len;
                        // g_message ("ProfileElement::calc_decimation -- decimation on. dec_len=%d n_dec=%d", dec_len, n_dec);
                } else { // no decimating
                        dec_len = 1;
                        n_dec = n;
                        // g_message ("ProfileElement::calc_decimation -- decimation off. dec_len=%d n_dec=%d", dec_len, n_dec);
                }
        } else {
        // no decimating
                dec_len = 1;
                n_dec = n;
        }
}

void ProfileElement::SetMode(long Mode){
	mode=(gint64) Mode;
}

void ProfileElement::SetOptions(long Flg){
	flg=(gint64) Flg;
}

double ProfileElement::calc(gint64 ymode, int id, int binary_mask, double y_offset, GtkWidget *canvas){
	Scan    *s;
	double  dc=0.;
        
        // XSM_DEBUG (DBG_L5, "ProfileElement::calc enter id=" << id);

        int     ix_left=0;
        int     ix_right=scan->mem2d->GetNx()-1;

        if (scan->mem2d->GetNx() < 4)
                return 0.;

	if(ymode & PROFILE_MODE_XR_AB){
                ix_left  = MIN (pctrl->CursorsIdx[0], pctrl->CursorsIdx[1]);
                ix_right = MAX (pctrl->CursorsIdx[0], pctrl->CursorsIdx[1]);
        }
        
 
        /*
         * unref, remove and delete unused paths
         * allocate new as needed
         */
	if(!(ymode & PROFILE_MODE_BINARY8)){
		for (int i=1; i<NPI; ++i){
                        UNREF_DELETE_CAIRO_ITEM_NO_UPDATE (pathitem[i]);
                        UNREF_DELETE_CAIRO_ITEM (pathitem_draw[i], canvas);
                }
        }

       	if(ymode & PROFILE_MODE_YPSD){
		if(psd_scan)
			delete psd_scan;
		psd_scan = new Scan (scan);
		psd_scan->SetView ();
		F1D_PowerSpec(scan, psd_scan);
		n=psd_scan->mem2d->GetNx();
                ix_left=0;
                ix_right=n-1;
                ylocmin = 0.;
		dc=psd_scan->mem2d->GetDataPkt (n-1, 0);
                // g_message ("FT DC=%g  N=%d", dc, n);
		double c=psd_scan->mem2d->GetDataPkt (n-2, 0);
		psd_scan->mem2d->PutDataPkt (c, n-1, 0); // remove DC component
		s = psd_scan;
        } else {
                //n=scan->mem2d->GetNx();
                n=ix_right-ix_left+1;
		if(psd_scan){
                        n=scan->mem2d->GetNx();
			delete psd_scan;
			psd_scan = NULL;
		}
		s = scan;
        }
        
        // n => n_dec, dec_len -- use auto (random pick) decimation if more points than physical x pixels!
        calc_decimation (ymode);

        // g_message ("CiAB: %d..%d, n,ndec: %d, %d", ix_left, ix_right, n, n_dec);


        if (pathitem[id]){
                if (pathitem[id]->get_n_nodes () != n_dec){
                        UNREF_DELETE_CAIRO_ITEM_NO_UPDATE (pathitem[id]);
                        UNREF_DELETE_CAIRO_ITEM (pathitem_draw[id], canvas);
                        pathitem[id] = new cairo_item_path (n_dec);
                }
        } else {
                pathitem[id] = new cairo_item_path (n_dec);
        }

        pathcoords_world = true;
        // ----
        
        // pathitem[id]->set_xy (i, x,y); // manages xy data!

        if (dec_len > 1){
                if(ymode & PROFILE_MODE_BINARY8){
                        ylocmin=-0.5; ylocmax=8.;
                        if(ymode & PROFILE_MODE_BINARY16)
                                ylocmax=16.;
                        for(int k=0, i=ix_left; k < n_dec; ++k, i+=dec_len){
                                int ii = i + (rand() % dec_len);
                                pathitem[id]->set_xy_fast (k,
                                                           s->data.Xunit->Base2Usr(s->mem2d->data->GetXLookup(ii)),
                                                           y_offset + (( (int)(s->mem2d->GetDataPkt(ii,yy)) & binary_mask ) ? 0.7 : 0.));
                        }
                } else if(ymode & PROFILE_MODE_YLINREG){
                        for(int k=0, i=ix_left; k < n_dec; ++k, i+=dec_len){
                                int ii = i + (rand() % dec_len);
                                pathitem[id]->set_xy_test (k,
                                                           s->data.Xunit->Base2Usr(s->mem2d->data->GetXLookup(ii)),
                                                           s->data.Zunit->Base2Usr(s->mem2d->GetDataPktLineReg(ii,yy)*s->data.s.dz));
                        }
                } else if(ymode & PROFILE_MODE_YLOWPASS){
                        ylocmin=ylocmax=0.;
                        double vp = 0.;
                        double v = s->data.Zunit->Base2Usr(s->mem2d->GetDataPkt(0,yy) * s->data.s.dz);
                        double lpn = pctrl->lp_gain;
                        double lpo = 1.0-lpn;
                        vp=v;
                        // for(int i=0, ix=0, iy=1; i < n; ix+=2, iy+=2, ++i){
                        for(int k=0, i=ix_left; k < n_dec; ++k, i+=dec_len){
                                int ii = i + dec_len/2;
                                double y=s->data.Zunit->Base2Usr(s->mem2d->GetDataPkt(i,yy) * s->data.s.dz);
                                for (int m=0; m<dec_len; ++m){
                                        v = y =  lpo*v + lpn * s->data.Zunit->Base2Usr(s->mem2d->GetDataPkt(i+m,yy) * s->data.s.dz);
                                }
                                if(ymode & PROFILE_MODE_YDIFF){
                                        y  = v-vp;
                                        vp = v;
                                }
                                pathitem[id]->set_xy_test (k, s->data.Xunit->Base2Usr(s->mem2d->data->GetXLookup(ii)), y);
                        }
                } else if(ymode & PROFILE_MODE_YDIFF){
                        for(int k=0, i=ix_left; k < n_dec; ++k, i+=dec_len){
                                int ii = i + (rand() % dec_len);
                                pathitem[id]->set_xy_test (k,
                                                           s->data.Xunit->Base2Usr(s->mem2d->data->GetXLookup(ii)),
                                                           s->data.Zunit->Base2Usr(s->mem2d->GetDataPktDiff(ii+1,yy) * s->data.s.dz));
                        }
                }else if(ymode & PROFILE_MODE_YLOG){
                        for(int k=0, i=ix_left; k < n_dec; ++k, i+=dec_len){
                                int ii = i + (rand() % dec_len);
                                pathitem[id]->set_xy_hold_logmin (k,
                                                                  s->data.Xunit->Base2Usr(s->mem2d->data->GetXLookup(ii)),
                                                                  s->data.Zunit->Base2Usr(s->mem2d->GetDataPkt(ii,yy)*s->data.s.dz),
                                                                  xsmres.ProfileLogLimit);
                        }
                }else{
                        for(int k=0, i=ix_left; k < n_dec; ++k, i+=dec_len){
                                int ii = i + (rand() % dec_len);
                                pathitem[id]->set_xy_test (k,
                                                           s->data.Xunit->Base2Usr(s->mem2d->data->GetXLookup(ii)),
                                                           s->data.Zunit->Base2Usr(s->mem2d->GetDataPkt(ii,yy)*s->data.s.dz));
                        }
                }
        } else {
                if(ymode & PROFILE_MODE_BINARY8){
                        ylocmin=-0.5; ylocmax=8.;
                        if(ymode & PROFILE_MODE_BINARY16)
                                ylocmax=16.;
                        for(int k=0, i=ix_left; i <= ix_right; ++i,++k)
                                pathitem[id]->set_xy_fast (k,
                                                           s->data.Xunit->Base2Usr(s->mem2d->data->GetXLookup(i)),
                                                           y_offset + (( (int)(s->mem2d->GetDataPkt(i,yy)) & binary_mask ) ? 0.7 : 0.));

                } else if(ymode & PROFILE_MODE_YLINREG){
                        for(int k=0, i=ix_left; i <= ix_right; ++i,++k)
                                pathitem[id]->set_xy_test (k,
                                                           s->data.Xunit->Base2Usr(s->mem2d->data->GetXLookup(i)),
                                                           s->data.Zunit->Base2Usr(s->mem2d->GetDataPktLineReg(i,yy)*s->data.s.dz));
                } else if(ymode & PROFILE_MODE_YLOWPASS){
                        ylocmin=ylocmax=0.;
                        double vp = 0.;
                        double v = s->data.Zunit->Base2Usr(s->mem2d->GetDataPkt(0,yy) * s->data.s.dz);
                        double lpn = pctrl->lp_gain;
                        double lpo = 1.0-lpn;
                        vp=v;
                        // for(int i=0, ix=0, iy=1; i < n; ix+=2, iy+=2, ++i){
                        for(int k=0, i=ix_left; i <= ix_right; ++i,++k){
                                double y;
                                v = y =  lpo*v + lpn * s->data.Zunit->Base2Usr(s->mem2d->GetDataPkt(i,yy) * s->data.s.dz);

                                if(ymode & PROFILE_MODE_YDIFF){
                                        y  = v-vp;
                                        vp = v;
                                }
                                pathitem[id]->set_xy_test (k, s->data.Xunit->Base2Usr(s->mem2d->data->GetXLookup(i)), y);
                        }
                } else if(ymode & PROFILE_MODE_YDIFF){
                        for(int k=0, i=ix_left; i <= ix_right; ++i,++k)
                                pathitem[id]->set_xy_test (k,
                                                           s->data.Xunit->Base2Usr(s->mem2d->data->GetXLookup(i)),
                                                           s->data.Zunit->Base2Usr(s->mem2d->GetDataPktDiff(i+1,yy) * s->data.s.dz));
                }else if(ymode & PROFILE_MODE_YLOG){
                        for(int k=0, i=ix_left; i <= ix_right; ++i,++k)
                                pathitem[id]->set_xy_hold_logmin (k,
                                                                  s->data.Xunit->Base2Usr(s->mem2d->data->GetXLookup(i)),
                                                                  s->data.Zunit->Base2Usr(s->mem2d->GetDataPkt(i,yy)*s->data.s.dz),
                                                                  xsmres.ProfileLogLimit);
                }else{
                        for(int k=0, i=ix_left; i <= ix_right; ++i,++k)
                                pathitem[id]->set_xy_test (k,
                                                           s->data.Xunit->Base2Usr(s->mem2d->data->GetXLookup(i)),
                                                           s->data.Zunit->Base2Usr(s->mem2d->GetDataPkt(i,yy)*s->data.s.dz));
                }
        }
        // update and get X and Y bounds
        pathitem[id]->update_bbox (false);
        pathitem[id]->get_bb_min (xlocmin, ylocmin);
        pathitem[id]->get_bb_max (xlocmax, ylocmax);

	return dc;
}

void ProfileElement::draw(cairo_t* cr){
        // XSM_DEBUG (DBG_L5, "ProfileElement::draw ******************");

        for (int id=0; id<NPI; ++id)
                if (pathitem_draw[id])
                        pathitem_draw[id]->draw (cr);
                else
                        return;
}

void ProfileElement::stream_set (std::ofstream &ofs, int id){
        for(int i=0; i < n_dec; ++i){
                double x,y;
                pathitem[id]->get_xy (i, x, y);
                ofs << std::setw(6) << i << " "  << std::setw(10) << x << " " << std::setw(10) << y << std::endl;
        }
}
        
void ProfileElement::update(GtkWidget *canvas, int id, int style){
        // XSM_DEBUG (DBG_L5, "ProfileElement::update ******************");
        gint m = pctrl->GetMode ();
        gint lm = 0;        // 0l,1d,2i,3x

        if (m & PROFILE_MODE_POINTS) lm = 1;
        else if (m & PROFILE_MODE_IMPULS) lm = 2;
        else if (m & PROFILE_MODE_SYMBOLS) lm = 3;
        else if (m & PROFILE_MODE_CONNECT) lm = 0;

        if (pathitem[id]){
                UNREF_DELETE_CAIRO_ITEM (pathitem_draw[id], canvas);
                pathitem_draw[id] = new cairo_item_path (pathitem[id]);
        }
        if (pathitem_draw[id]){
                pathitem_draw[id]->set_line_style (style);
                pathitem_draw[id]->set_stroke_rgba (color);
                pathitem_draw[id]->set_line_width (pctrl->get_lw1 (xsmres.ProfileLineWidth));
                pathitem_draw[id]->set_mark_radius (pctrl->get_lw1 (5.));
                pathitem_draw[id]->set_linemode (lm);

                // TDB
                //        pathitem[id]->map_xy ( &pctrl->scan2canvas);
                double yf=0.;
                if (pathitem[id]->get_n_nodes () != n_dec || pathitem_draw[id]->get_n_nodes () != n_dec){
                        g_warning ("ProfileElement::update -- WARNING, abort: pathitem[id]->get_n_nodes () != n_dec || pathitem_draw[id]->get_n_nodes () != n_dec");
                        return;
                }
                for(int i=0; i < n_dec; ++i){
                        double x,y;
                        pathitem[id]->get_xy (i, x, y);
                        pctrl->scan2canvas (x ,y);
                        pathitem_draw[id]->set_xy_fast (i, x, y);
                        if (i==0) yf=y;
                        else if (yf < y) yf=y;
                }
                pathitem_draw[id]->set_impulse_floor (yf);

                pathitem_draw[id]->queue_update (canvas);
        }

}

gchar *ProfileElement::GetDeltaInfo(int i, int j, gint64 ymode){
	double x1,y1, x2,y2, dx,dz, z1, z2, zint12, dxint12, rms12;
	GetCurXYc (&x1, &y1, i);
	GetCurXYc (&x2, &y2, j);
	if(i<n && i>=0 && j<n && j>=0){
		int n=0;
		Scan *sc = ymode & PROFILE_MODE_YPSD ? psd_scan : scan;
		x1 = sc->mem2d->data->GetXLookup(i);
		x2 = sc->mem2d->data->GetXLookup(j);
		zint12 = 0.;
		dxint12 = 0.;
		rms12 = 0.;
		for (int k=i<j?i:j; k<(j>i?j:i); ++k){
			dxint12 = sc->mem2d->data->GetXLookup(k+1) - sc->mem2d->data->GetXLookup (k);
			zint12  += dxint12 * sc->mem2d->GetDataPkt(k,yy);
			double z = ymode & PROFILE_MODE_YLINREG ? sc->mem2d->GetDataPktLineReg (k,yy) : sc->mem2d->GetDataPkt (k,yy);
			rms12 += z*z;
			++n;
		}
		rms12 = sqrt (rms12/n);
		dx = x2-x1;
		z1=sc->mem2d->GetDataPkt(i,yy);
		z2=sc->mem2d->GetDataPkt(j,yy);
		gchar *s1 = sc->data.Xunit->UsrString (dx);
		gchar *s2 = ymode & PROFILE_MODE_YLINREG ?
			sc->data.Zunit->UsrString (dz = (( sc->mem2d->GetDataPktLineReg(i,yy)
							   - sc->mem2d->GetDataPktLineReg(j,yy))
							 *sc->data.s.dz))
			:
			sc->data.Zunit->UsrString (dz = (( sc->mem2d->GetDataPkt(i,yy) 
							   - sc->mem2d->GetDataPkt(j,yy) )
							 *sc->data.s.dz));
		gchar *s3 = g_strdup_printf ("%g" UTF8_DEGREE, 180./M_PI*atan(fabs(dz/dx)));
		gchar *sz1 = g_strdup (sc->data.Zunit->UsrString (z1*sc->data.s.dz));
		gchar *sz2 = g_strdup (sc->data.Zunit->UsrString (z2*sc->data.s.dz));
		gchar *szi12 = g_strdup (sc->data.Zunit->UsrString (zint12/sc->data.s.dx));
		gchar *szrms12 = g_strdup (sc->data.Zunit->UsrString (rms12*sc->data.s.dz));
		gchar *s = g_strconcat ("Cursor Delta (", s1, ", ", s2, ")",
					" Slope: ", s3,
					"  Y1: ", sz1, ", Y2: ", sz2,
					"  Int: ", szi12, 
					"  RMS: ", szrms12, 
					NULL);
		pctrl->UpdateCursorInfo (x1, x2, z1, z2);

		g_free (s1);
		g_free (s2);
		g_free (s3);
		g_free (sz1);
		g_free (sz2);
		g_free (szi12);
		g_free (szrms12);
		return s;
	}
	else
		return g_strdup("out of scan !");
}


// ========================================

gint ProfileControl_auto_update_callback (ProfileControl *pc){
	if (! pc->get_auto_update_id ()) 
		return FALSE;

	if (pc->update_needed () == TRUE) {
		pc->mark_for_update (111); // lock
		pc->UpdateArea ();
//		pc->show ();
		pc->mark_for_update (FALSE); // unlock and done
	}
	return TRUE;
}

ProfileControl::ProfileControl (const gchar *titlestring, int ChNo){
        pc_in_window = NULL;
	Init(titlestring, ChNo);
	ref ();
}

ProfileControl::ProfileControl (const gchar *titlestring, int n, UnitObj *ux, UnitObj *uy, double xmin, double xmax, const gchar *resid,  Gxsm4appWindow *in_external_window) : LineProfile1D(n, ux, uy, xmin, xmax){
        pc_in_window = in_external_window;
	Init(titlestring, -1, resid);
	ref ();
	g_signal_connect(G_OBJECT(window),
                         "delete_event",
                         G_CALLBACK (ProfileControl::file_close_callback),
                         this);
	gchar *tmp;
	SetXlabel (tmp = scan1d->data.Xunit->MakeLongLabel());
	g_free (tmp);
	
	SetYlabel (tmp = scan1d->data.Zunit->MakeLongLabel());
	g_free (tmp);

	AddScan(scan1d);

	UpdateArea ();
	show();
}

ProfileControl::ProfileControl (const gchar *filename, const gchar *resource_id_string){
	XSM_DEBUG (DBG_L3, "ProfileControl::ProfileControl from file" << filename );
        pc_in_window = NULL;

	gchar *t = g_strconcat (filename, " - Profile from File", NULL);
	Init (t, -1, resource_id_string);
	g_free (t);

	g_signal_connect (G_OBJECT (window),
                          "delete_event",
                          G_CALLBACK (ProfileControl::file_close_callback),
                          this);

	XSM_DEBUG (DBG_L3, "ProfileControl::ProfileControl from file - Init done, loading data");

	load (filename);

	XSM_DEBUG (DBG_L3, "ProfileControl::ProfileControl from file - label setup...");

	gchar *tmp;
	SetXlabel (tmp = scan1d->data.Xunit->MakeLongLabel());
	g_free (tmp);
	
	SetYlabel (tmp = scan1d->data.Zunit->MakeLongLabel());
	g_free (tmp);
	
	XSM_DEBUG (DBG_L3, "ProfileControl::ProfileControl from file Add Scan" );

	AddScan(scan1d);

	XSM_DEBUG (DBG_L3, "ProfileControl::ProfileControl from file Update" );

	UpdateArea ();
	show();
	
	XSM_DEBUG (DBG_L3, "ProfileControl::ProfileControl from file done." );
}

void ProfileControl::Init(const gchar *titlestring, int ChNo, const gchar *resid){
	int i;
	GtkWidget *tb;
	char xcline[256];

        GXSM_REF_OBJECT (GXSM_GRC_PROFILECTRL);

        pc_grid = NULL;
        set_pc_matrix_size ();
        window_w = window_h = 0;
        font_size_10 = 10.;
        
	auto_update_id = 0;

	lp_gain = 0.1;

	ref_count = 0;
	lock = FALSE;
	tic_x1=0.,tic_x2=0.,tic_y1=0.,tic_y2=0.;
	tic_ym=0;

	cursor_bound_object = NULL;
        tmp_effected = 0;
        
	xcolors_list = NULL;
	num_xcolors = 0;
	xcolors_list_2 = NULL;
	num_xcolors_2 = 0;

	std::ifstream xcolors;

	// default
	gchar *pname = g_strdup_printf ("%s/%s", xsmres.GxsmPalettePath, xsmres.ProfileXColorsPath);

	xcolors.open (pname, std::ios::in);

	if(!xcolors.good()) // or try this, absolute
		xcolors.open (xsmres.ProfileXColorsPath, std::ios::in);

//	if(!xcolors.good()) // if nothing, try this
//		xcolors.open ("/etc/X11/rgb.txt", std::ios::in);

	if(xcolors.good()){
		gchar **rgbcolor;
		xcolors.getline(xcline, 255);
		if (strstr (xcline, "rgb.txt")){
			int lastcolorsum=0;
			int colorsum=0;
			GString* strbuffer = g_string_new ("");
			while (xcolors.good()){
				xcolors.getline(xcline, 255);
				rgbcolor = g_strsplit (xcline, "\t\t", 2);
//				std::cout << "RGB-Name[" << num_xcolors << "]:" << rgbcolor[0] <<","<< rgbcolor[1] <<"."<< std::endl;
				int r=256,g=256,b=256;
				if (rgbcolor[0]){
					r = atoi (&rgbcolor[0][0]);
					g = atoi (&rgbcolor[0][4]);
					b = atoi (&rgbcolor[0][8]);
				}
				colorsum = r+g+b;
				if (rgbcolor[1] && colorsum<600 && lastcolorsum != colorsum){ // sort out very light colors and remove duplicates
//					std::cout << rgbcolor[1] << std::endl;
					strbuffer = g_string_append (strbuffer, rgbcolor[1]);
					strbuffer = g_string_append (strbuffer, "\n");
					g_strfreev (rgbcolor);
					++num_xcolors;
				}
				lastcolorsum = colorsum;
			}
			xcolors_list = g_strsplit (strbuffer->str, "\n", 0);
			g_string_free (strbuffer, TRUE);
		} else { 
			// get length of file:
			xcolors.seekg (0, std::ios::end);
			int length = xcolors.tellg();
			gchar *buffer = g_strnfill (length+1, '*');
			xcolors.seekg (0, std::ios::beg);
			xcolors.read (buffer, length);
			buffer[length]=0;
			xcolors_list = g_strsplit (buffer, "\n", 0);
			g_free (buffer);
			for (; xcolors_list[num_xcolors]; ++num_xcolors)
				;
//				std::cout << xcolors_list[num_xcolors] << std::endl;
		}
		xcolors.close ();
	}

	// init legend
	for (int ils=0; ils<PC_LEGEND_ITEMS_MAX; ++ils)
		ys_legend_label_list[ils] = NULL;

	// use resif or auto-choose profile type and select resource
	if (resid)
		profile_res_id = g_strconcat ("Profile/", resid, NULL);
	else{
		if(strstr(titlestring, "Red"))
			profile_res_id = g_strdup("Profile/Red-Line");
		else
			if(strstr(titlestring, "from"))
				profile_res_id = g_strdup("Profile/Show-Line");
			else
				if(strstr(titlestring, "Probe"))
					profile_res_id = g_strdup("Profile/Probe-Data");
				else
					if(strstr(titlestring, ".asc"))
						profile_res_id = g_strdup("Profile/File-Data");
					else
						profile_res_id = g_strdup("Profile/Other-Line");
	}
  
	chno=ChNo;

        // setup basic window and header, create and attach (if in own window) pc_grid
        // set or gets app_window, window
        if (titlestring)
                title = g_strdup (titlestring);
        else
                title = g_strdup ("Profile");

        AppWindowInit (title);
        
	Cursor[0][0] = NULL;
	Cursor[0][1] = NULL;
	Cursor[1][0] = NULL;
	Cursor[1][1] = NULL;
	CursorsIdx[0]=0;
	CursorsIdx[1]=0;

        BBox = NULL;
        bbox_mode = 0;
        
	last_pe = NULL;
	scount=0;
	statusheight = 30;
	bbarwidth    = 30;
	border=0.10;
	pasize=1.0;
	aspect=3./2.;
	papixel=400;
        left_border = 3.;
        top_border  = 1./4.;

        
	Yticn=8;
	Xticn=8;

	Xtics   = new cairo_item*[PC_XTN];
	Ytics   = new cairo_item*[PC_YTN];
	Xlabels = new cairo_item_text*[PC_XLN];
	Ylabels = new cairo_item_text*[PC_YLN];
	LegendSym  = new cairo_item*[PC_LEGEND_ITEMS_MAX];
	LegendText = new cairo_item_text*[PC_LEGEND_ITEMS_MAX];
	num_legend=0;
	for(i=0; i<PC_XTN; i++) Xtics[i] = NULL;
	for(i=0; i<PC_YTN; i++) Ytics[i] = NULL;
	for(i=0; i<PC_XLN; i++) Xlabels[i] = NULL;
	for(i=0; i<PC_YLN; i++) Ylabels[i] = NULL;
	for(i=0; i<PC_LEGEND_ITEMS_MAX; i++) LegendSym[i] = NULL, LegendText[i] = NULL;

	ScanList = NULL;
	background = NULL;
	frame = NULL;
	xaxislabel = NULL;
	yaxislabel = NULL;
	saxislabel = NULL;
	titlelabel = NULL;
	infotext = NULL;
        
        
	xlabel0 = NULL;
	ylabel0 = NULL;
	xlabel = g_strdup("x");
	ylabel = g_strdup("y");

	xticfmt = g_strdup("%g");
	yticfmt = g_strdup("%g");

	SetXrange(0., 1.);
	SetYrange(0., 1.);

	SetMode(PROFILE_MODE_XGRID | PROFILE_MODE_YGRID | PROFILE_MODE_CONNECT | PROFILE_MODE_DECIMATE);
	SetScaling(PROFILE_SCALE_XAUTO | PROFILE_SCALE_YAUTO);

	XSM_DEBUG (DBG_L2, "ProfileControl::ProfileControl set_data to widget");

	g_object_set_data  (G_OBJECT (window), "Ch", GINT_TO_POINTER (chno));
	g_object_set_data  (G_OBJECT (window), "ChNo", GINT_TO_POINTER (chno+1));

        //	gapp->configure_drop_on_widget (window);

	if(chno>=0)
		g_signal_connect(G_OBJECT(window),
                                 "delete_event",
                                 G_CALLBACK(App::close_scan_event_cb),
                                 this);

	// New Scrollarea

	XSM_DEBUG (DBG_L2, "ProfileControl::ProfileControl canvas");

        canvas = gtk_drawing_area_new(); // gtk3 cairo drawing-area -> "canvas"

	scrollarea = gtk_scrolled_window_new ();

        gtk_widget_set_hexpand (scrollarea, TRUE);
        gtk_widget_set_vexpand (scrollarea, TRUE);
        
	/* the policy is one of GTK_POLICY AUTOMATIC, or GTK_POLICY_ALWAYS.
	 * GTK_POLICY_AUTOMATIC will automatically decide whether you need
	 * scrollbars, whereas GTK_POLICY_ALWAYS will always leave the scrollbars
	 * there.  The first one is the horizontal scrollbar, the second, 
	 * the vertical. */
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollarea),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_grid_attach (GTK_GRID (pc_grid), scrollarea, 1,1, 9,20);
        gtk_widget_show (scrollarea);
        gtk_widget_show (pc_grid);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrollarea), canvas);

	XSM_DEBUG (DBG_L2, "ProfileControl::ProfileControl hbox");

        if (!pc_in_window){
                // New Statusbar
                statusbar = gtk_statusbar_new ();
                statusid  = gtk_statusbar_get_context_id (GTK_STATUSBAR(statusbar), "drag");
                gtk_grid_attach (GTK_GRID (pc_grid), statusbar, 1,100, 11,1);
        } else {
                // Get Statusbar
                statusbar = (GtkWidget*) g_object_get_data (G_OBJECT (window), "statusbar");
                statusid  = gtk_statusbar_get_context_id (GTK_STATUSBAR(statusbar), "drag");
        }
        gtk_widget_show (statusbar);

        // hookup popup menu
        //gtk_popover_set_default_widget (GTK_POPOVER (p_popup_menu_cv), canvas);
        gtk_widget_set_parent (p_popup_menu_cv, canvas);
        //gtk_popover_set_child (GTK_POPOVER (p_popup_menu_cv), canvas);

        // configure getsure events
        GtkEventController* motion = gtk_event_controller_motion_new ();
        g_signal_connect (motion, "motion", G_CALLBACK (ProfileControl::drag_motion), this);
        gtk_widget_add_controller (canvas, GTK_EVENT_CONTROLLER (motion));
        g_object_set_data (G_OBJECT (canvas), "motion-controller", motion);
        g_object_set_data (G_OBJECT (canvas), "statusbar", statusbar);

        GtkGesture *gesture = gtk_gesture_click_new ();
        gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), 0);
        g_signal_connect (gesture, "pressed", G_CALLBACK (ProfileControl::pressed_cb), this);
        g_signal_connect (gesture, "released", G_CALLBACK (ProfileControl::released_cb), this);
        gtk_widget_add_controller (canvas, GTK_EVENT_CONTROLLER (gesture));

        gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (canvas),
                                        G_CALLBACK (ProfileControl::canvas_draw_function),
                                        this, NULL);
	SetSize();

	XSM_DEBUG (DBG_L2, "ProfileControl::ProfileControl draw!");

	XSM_DEBUG (DBG_L2, "ProfileControl::ProfileControl show!");
	gtk_widget_show( canvas );

        // Tool Button simply attached to grid.

        if (!pc_in_window){
                int k=1;
                tb = gtk_check_button_new_with_label ("LR");
                gtk_actionable_set_action_name (GTK_ACTIONABLE (tb), "profile.opt-Y-linreg");
                gtk_check_button_set_active (GTK_CHECK_BUTTON (tb), (mode & PROFILE_MODE_YLINREG) ? TRUE : FALSE);
                gtk_grid_attach (GTK_GRID (pc_grid), tb, 10,k++, 1,1);
                gtk_widget_show (tb);

                tb = gtk_check_button_new_with_label ("LG");
                gtk_actionable_set_action_name (GTK_ACTIONABLE (tb), "profile.skl-Y-log");
                gtk_check_button_set_active (GTK_CHECK_BUTTON (tb), (mode & PROFILE_MODE_YLOG) ? TRUE : FALSE);
                gtk_grid_attach (GTK_GRID (pc_grid), tb, 10,k++, 1,1);
                gtk_widget_show (tb);

                tb = gtk_check_button_new_with_label ("FT");
                gtk_actionable_set_action_name (GTK_ACTIONABLE (tb), "profile.opt-Y-ft");
                gtk_check_button_set_active (GTK_CHECK_BUTTON (tb), (mode & PROFILE_MODE_YPSD) ? TRUE : FALSE);
                gtk_grid_attach (GTK_GRID (pc_grid), tb, 10,k++, 1,1);
                gtk_widget_show (tb);

                tb = gtk_check_button_new_with_label ("dY");
                gtk_actionable_set_action_name (GTK_ACTIONABLE (tb), "profile.opt-Y-diff");
                gtk_check_button_set_active (GTK_CHECK_BUTTON (tb), (mode & PROFILE_MODE_YDIFF) ? TRUE : FALSE);
                gtk_grid_attach (GTK_GRID (pc_grid), tb, 10,k++, 1,1);
                gtk_widget_show (tb);
        
                tb = gtk_check_button_new_with_label ("LP");
                gtk_actionable_set_action_name (GTK_ACTIONABLE (tb), "profile.opt-Y-lowpass-cycle");
                gtk_check_button_set_active (GTK_CHECK_BUTTON (tb), (mode & PROFILE_MODE_YLOWPASS) ? TRUE : FALSE);
                gtk_grid_attach (GTK_GRID (pc_grid), tb, 10,k++, 1,1);
                gtk_widget_show (tb);
        
                tb = gtk_check_button_new_with_label ("YH");
                gtk_actionable_set_action_name (GTK_ACTIONABLE (tb), "profile.skl-Y-hold");
                gtk_check_button_set_active (GTK_CHECK_BUTTON (tb), (scaleing & PROFILE_SCALE_YHOLD) ? TRUE : FALSE);
                gtk_grid_attach (GTK_GRID (pc_grid), tb, 10,k++, 1,1);
                gtk_widget_show (tb);

                tb = gtk_check_button_new_with_label ("YE");
                gtk_actionable_set_action_name (GTK_ACTIONABLE (tb), "profile.skl-Y-expand");
                gtk_check_button_set_active (GTK_CHECK_BUTTON (tb), (scaleing & PROFILE_SCALE_YEXPAND) ? TRUE : FALSE);
                gtk_grid_attach (GTK_GRID (pc_grid), tb, 10,k++, 1,1);
                gtk_widget_show (tb);

                tb = gtk_check_button_new_with_label ("CA");
                gtk_actionable_set_action_name (GTK_ACTIONABLE (tb), "profile.cursor-A");
                gtk_grid_attach (GTK_GRID (pc_grid), tb, 10,k++, 1,1);
                gtk_widget_show (tb);

                widget_cb = tb = gtk_check_button_new_with_label ("CB");
                gtk_actionable_set_action_name (GTK_ACTIONABLE (tb), "profile.cursor-B");
                gtk_grid_attach (GTK_GRID (pc_grid), tb, 10,k++, 1,1);
                gtk_widget_show (tb);
        
                widget_xr_ab = tb = gtk_check_button_new_with_label ("AB");
                gtk_actionable_set_action_name (GTK_ACTIONABLE (tb), "profile.opt-XR-AB");
                gtk_check_button_set_active (GTK_CHECK_BUTTON (tb), (mode & PROFILE_MODE_XR_AB) ? TRUE : FALSE);
                gtk_grid_attach (GTK_GRID (pc_grid), tb, 10,k++, 1,1);
                gtk_widget_show (tb);
        
                tb = gtk_check_button_new_with_label ("BI");
                gtk_actionable_set_action_name (GTK_ACTIONABLE (tb), "profile.opt-Y-binary");
                gtk_grid_attach (GTK_GRID (pc_grid), tb, 10,k++, 1,1);
                gtk_widget_show (tb);

                gtk_widget_show (pc_grid); // FIX-ME GTK4 SHOWALL
                gtk_widget_hide (widget_cb);
                gtk_widget_hide (widget_xr_ab);
        } else {
                gtk_widget_show (pc_grid); // FIX-ME GTK4 SHOWALL
                widget_cb=NULL;
                widget_xr_ab=NULL;
        }
        if (!pc_in_window){
                if (g_strrstr (title, "Red Line Ch")){ // manage 1st 8 channels for red line type profiles
                        gint i = atoi (&title[11]);
                        if (i>=1 && i <= 8){
                                gchar *id=g_strdup_printf ("view-profile-redline-%d", i);
                                set_window_geometry (id, -1, false);
                                show ();
                        }
                }
        }

        XSM_DEBUG (DBG_L2, "ProfileControl::ProfileControl [" << title << "] done.");
}

ProfileControl::~ProfileControl ()
{
	XsmRescourceManager xrm(profile_res_id);

	XSM_DEBUG (DBG_L2, "ProfileControl::~ProfileControl");

        // must undo
        // resize_cb_handler_id = g_signal_connect (GTK_CONTAINER (window), "check-resize", G_CALLBACK (ProfileControl::resize_drawing), this);
        g_signal_handler_disconnect (GTK_BOX (window), resize_cb_handler_id);

	auto_update (0);

        g_object_unref (pc_action_group);

	RemoveScans();

        UNREF_DELETE_CAIRO_ITEM (frame, canvas);
        UNREF_DELETE_CAIRO_ITEM (xaxislabel, canvas);
        UNREF_DELETE_CAIRO_ITEM (yaxislabel, canvas);
        UNREF_DELETE_CAIRO_ITEM (saxislabel, canvas);
        UNREF_DELETE_CAIRO_ITEM (titlelabel, canvas);
        UNREF_DELETE_CAIRO_ITEM (infotext, canvas);
        for (int id=0; id<2; ++id){
                UNREF_DELETE_CAIRO_ITEM (Cursor[id][0], canvas);
                UNREF_DELETE_CAIRO_ITEM (Cursor[id][1], canvas);
        }
        UNREF_DELETE_CAIRO_ITEM (BBox, canvas);
        for(int i=0; i<PC_XTN; i++) if (Xtics[i]) { UNREF_DELETE_CAIRO_ITEM (Xtics[i], canvas); }
	for(int i=0; i<PC_YTN; i++)  if (Ytics[i]) { UNREF_DELETE_CAIRO_ITEM (Ytics[i], canvas); }
        for(int i=0; i<PC_XLN; i++)  if (Xlabels[i]) { UNREF_DELETE_CAIRO_ITEM (Xlabels[i], canvas); }
        for(int i=0; i<PC_YLN; i++)  if (Ylabels[i]) { UNREF_DELETE_CAIRO_ITEM (Ylabels[i], canvas); }
	for(int i=0; i<PC_LEGEND_ITEMS_MAX; i++) {
                if (LegendSym[i]) { UNREF_DELETE_CAIRO_ITEM (LegendSym[i], canvas); }
                if (LegendText[i]) { UNREF_DELETE_CAIRO_ITEM (LegendText[i], canvas); }
        }
        
	delete[] Xtics;
	delete[] Ytics;
	delete[] Xlabels;
	delete[] Ylabels;
	delete[] LegendSym;
	delete[] LegendText;

	g_free(profile_res_id);
	g_free(xticfmt);
	g_free(yticfmt);
	g_free(xlabel);
	g_free(ylabel);
	g_free(title);

	if (xcolors_list)
		g_strfreev (xcolors_list);

	// delete legend
	for (int ils=0; ils<PC_LEGEND_ITEMS_MAX; ++ils)
		if (ys_legend_label_list[ils])
			g_free (ys_legend_label_list[ils]);

        if (pc_in_window){
                // destroy grid and widgets, unref app_window and window
                if (pc_grid){\
                        // FIX-ME GTK4
                        g_message ("FIX-ME-GTK4 unref grid: ProfileControl::~ProfileControl");
                        //gtk_window_destroy (GTK_WINDOW (pc_grid));
                        pc_grid = NULL;
                }
                app_window = NULL;
                window = NULL;
        }

        GXSM_UNREF_OBJECT (GXSM_GRC_PROFILECTRL);
        XSM_DEBUG (DBG_L2, "done.");
       
}


void ProfileControl::AppWindowInit(const gchar *title, const gchar *sub_title){
	XSM_DEBUG (DBG_L2,  "ViewControl::WidgetInit" );

        // create window PopUp menu  ---------------------------------------------------------------------
        XSM_DEBUG (DBG_L2,  "VC::VC popup" );

        // new action group
        pc_action_group = g_simple_action_group_new ();

        GObject *profile_popup_menu = gapp->get_profile_popup_menu ();
        p_popup_menu = gtk_popover_menu_new_from_model (G_MENU_MODEL (profile_popup_menu));
        p_popup_menu_cv = gtk_popover_menu_new_from_model (G_MENU_MODEL (profile_popup_menu));
        gtk_popover_set_has_arrow (GTK_POPOVER (p_popup_menu_cv), FALSE);
        
        if (pc_in_window){
                // g_message ("ProfileControl::AppWindowInit in external window >%s<", title);
                // get app_window and window
                pc_grid = gtk_grid_new ();
                app_window = pc_in_window;
                window = GTK_WINDOW (app_window);
                header_bar = gtk_window_get_titlebar (GTK_WINDOW (window));
                gtk_window_present (GTK_WINDOW (window));
                // FIX-ME-GTK4
                //g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);

        } else {
                // g_message ("ProfileControl::AppWindowInit create own app_window >%s<", title);
                // create our own app_window
                app_window = gxsm4_app_window_new (GXSM4_APP (gapp->get_application ()));
                window = GTK_WINDOW (app_window);

                header_bar = gtk_header_bar_new ();
                gtk_widget_show (header_bar);
                //gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header_bar), true);

                XSM_DEBUG (DBG_L2,  "VC::VC popup Header Buttons setup. " );

#if 1
                // attach full view popup menu to tool button ----------------------------------------------------
                GtkWidget *header_menu_button = gtk_menu_button_new ();
                //gtk_menu_button_set_icon_name (GTK_MENU_BUTTON (header_menu_button), "emblem-system-symbolic");
                gtk_menu_button_set_popover (GTK_MENU_BUTTON (header_menu_button), p_popup_menu);
                gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), header_menu_button);
                gtk_widget_show (header_menu_button);
#endif

                SetTitle (title, sub_title);
                //gtk_header_bar_set_title ( GTK_HEADER_BAR (header_bar), title);
                //gtk_header_bar_set_subtitle (GTK_HEADER_BAR  (header_bar), title);
                gtk_window_set_titlebar (GTK_WINDOW (window), header_bar);

                v_grid = gtk_grid_new ();
                gtk_window_set_child (GTK_WINDOW (window), v_grid);
                g_object_set_data (G_OBJECT (window), "v_grid", v_grid);
                pc_grid = v_grid;

                // g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);
                // g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (AppBase::window_close_callback), this);
                gtk_widget_show (GTK_WIDGET (window)); // FIX-ME GTK4 SHOWALL
        }
        gtk_widget_show (pc_grid);

        // FIX-ME-GTK4 ?!?!
        // g_signal_connect (G_OBJECT (pc_grid), "destroy", G_CALLBACK (gtk_window_destroy), &pc_grid);
        
        resize_cb_handler_id = g_signal_connect (GTK_BOX (window), "check-resize", G_CALLBACK (ProfileControl::resize_drawing), this);

        gtk_widget_show (GTK_WIDGET (pc_grid)); // FIX-ME GTK4 SHOWALL

        // create action map for canvas/grid to manage "this" profile
        GSimpleActionGroup *gsag = g_simple_action_group_new ();
        gtk_widget_insert_action_group (pc_grid, "profile", G_ACTION_GROUP (gsag));
        g_action_map_add_action_entries (G_ACTION_MAP (gsag),
                                         win_profile_popup_entries, G_N_ELEMENTS (win_profile_popup_entries),
                                         this);
}

gboolean ProfileControl::resize_drawing (GtkWidget *widget, ProfileControl *pc){
        gint w,h;
        // FIX-ME-GTK4
        // gtk_window_get_size (GTK_WINDOW (pc->window), &w, &h);

        // invert: gtk_widget_set_size_request (canvas, (int)(papixel*(aspect+3*border)), (gint)(papixel*(1.+border))+statusheight);
        // w=papixel*(aspect+3*border)       -> papixel = w/(aspect+3*border)
        // h=papixel*(1.+border)+statusheight

        if (pc->window_w != w || pc->window_h != h || pc->w_pc_nrows != pc->pc_nrows || pc->w_pc_ncolumns != pc->pc_ncolumns){
                pc->window_w = w, pc->window_h = h;
                pc->w_pc_nrows = pc->pc_nrows,  pc->w_pc_ncolumns = pc->pc_ncolumns;
                if (pc->ref_count > 0){
                        double ap;
                        if (!pc->pc_in_window){
                                // correct size down to canvas area available
                                w -= pc->bbarwidth;
                                h -= pc->statusheight;
                                //g_print ("Profile Window Corrected: %d, %d\n", w,h);
                                ap = (double)(w*(1.-3.*pc->border))/(double)(h*(1.-pc->border));
                                //g_print ("Profile Drawing Resize: %d, %d, ap=%g\n", w,h, ap);
                                pc->papixel = (int)(w / (ap+3*pc->border));
                        } else {
                                w /= pc->pc_ncolumns;
                                h /= pc->pc_nrows;
                                if (! (pc->mode & PROFILE_MODE_HEADER))
                                        pc->mode |= PROFILE_MODE_HEADER;
                                ap = (double)(w)/(double)(h);
                                pc->papixel = (int)(w/ap);
                        }
                        pc->SetSize (ap);
                        pc->UpdateArea ();
                }
        }
        return FALSE;
}

gboolean ProfileControl::cairo_draw_profile_only_callback (cairo_t *cr, ProfileControl *pc){
        cairo_scale (cr, pc->pixel_size, pc->pixel_size);
        pc->drawScans (cr);
        return FALSE;
}

void ProfileControl::canvas_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                           int             width,
                                           int             height,
                                           ProfileControl *pc){
        cairo_item **c_item;
        cairo_item_text **c_item_t;

        // -- this must be inverted to transform canvas mouse coordinates
        // -- back to world in canvas_event_cb () +++ 
        cairo_scale (cr, pc->pixel_size, pc->pixel_size);
        cairo_translate (cr, pc->left_border*pc->border, pc->top_border*pc->border);
        // +++++++++++++++++++++++++++++++++++++++++++++++++++
        
        pc->frame->draw (cr);
        pc->xaxislabel->draw (cr);
        pc->yaxislabel->draw (cr, -90.);

        if (pc->mode & PROFILE_MODE_HEADER){ // else invisible...
                if (pc->mode & PROFILE_MODE_STICS)
                        if (pc->saxislabel)
                                pc->saxislabel->draw (cr);
                if (pc->titlelabel)
                        pc->titlelabel->draw (cr);
                if (pc->infotext)
                        pc->infotext->draw (cr);
        }

        c_item = &pc->Xtics[0];   
        while (*c_item) (*c_item++)->draw (cr);
        c_item_t = &pc->Xlabels[0]; 
        while (*c_item_t) (*c_item_t++)->draw (cr);

        c_item = &pc->Ytics[0];  
        while (*c_item) (*c_item++)->draw (cr);
        c_item_t = &pc->Ylabels[0]; 
        while (*c_item_t) (*c_item_t++)->draw (cr);

        c_item = &pc->LegendSym[0]; 
        while (*c_item) (*c_item++)->draw (cr);
        c_item_t = &pc->LegendText[0]; 
        while (*c_item_t) (*c_item_t++)->draw (cr);

        pc->drawScans (cr);

	if(!(pc->mode & PROFILE_MODE_XR_AB)){
                for (int id=0; id<2; ++id)
                        if (pc->Cursor[id][0]){
                                pc->Cursor[id][0]->draw (cr);
                                pc->Cursor[id][1]->draw (cr);
                        }
        }
        
        if (pc->BBox)
                pc->BBox->draw (cr);
}


void ProfileControl::pressed_cb (GtkGesture *gesture, int n_press, double x, double y, ProfileControl *pc){
        double mouse_pix_xy[2];
        int button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture));
        g_message ("RELEASED %d #%d at %g %g", button, n_press, x,y);
        cairo_item *items[2] = { pc->Cursor[0][1], pc->Cursor[1][1] };

        // -- this must be the inverted transformation for canvas mouse coordinates into world
        // -- see canvas_draw_callback (), cairo scale/translate +++ 
        // ***** cairo_scale (cr, pc->pixel_size, pc->pixel_size);
        // ***** cairo_translate (cr, 3.*pc->border, 5./4.*pc->border);
        // undo cairo image translation/scale:
        mouse_pix_xy[0] = ((double)x/pc->pixel_size - (double)(pc->left_border*pc->border));
        mouse_pix_xy[1] = ((double)y/pc->pixel_size - (double)(pc->top_border*pc->border));
        // ++++++++++++++++++++++++++++++++++++++++++++++++++
        
        pc->canvas2scan (mouse_pix_xy[0], mouse_pix_xy[1], mouse_pix_xy[0], mouse_pix_xy[1]);
        
        switch(button) {
        case 1:
                {
                        DA_Event event = { EV_BUTTON_1, EV_BUTTON_PRESS, x,y };
                        if (pc->tmp_effected >= 0){
                                //g_message ("CANVAS EVENT cursor grab mode image-pixel XY: %g, %g\n", mouse_pix_xy[0], mouse_pix_xy[1]);
                                pc->cursor_event (items, &event, mouse_pix_xy, pc);
                                break;
                        }
                        pc->cursor_event (items, &event, mouse_pix_xy, pc);

                        if (pc->tmp_effected >= 0) // handled by object, done. no more action here!
                                break;
                }
                break;
        case 2: // Show XYZ display
                {
                        GtkWidget *menu;
                        GtkWidget *box;
                        GtkWidget *item;
                        //g_message ("M BUTTON_PRESS image-pixel XY: %g, %g\n", mouse_pix_xy[0], mouse_pix_xy[1]);

                        menu = gtk_popover_new ();
                        gtk_widget_set_parent (menu, pc->canvas);
                        gtk_popover_set_has_arrow (GTK_POPOVER (menu), TRUE);
                        gtk_popover_set_pointing_to (GTK_POPOVER (menu), &(GdkRectangle){ x, y, 1, 1});
                        box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
                        gtk_popover_set_child (GTK_POPOVER (menu), box);

                        gchar *mxy = g_strdup_printf ("%g, %g", mouse_pix_xy[0], mouse_pix_xy[1]);
                        item = gtk_button_new_with_label (mxy);
                        g_free (mxy);
                        gtk_button_set_has_frame (GTK_BUTTON (item), FALSE);
                        gtk_box_append (GTK_BOX (box), item);

                        gtk_popover_popup (GTK_POPOVER (menu));
                }
                //dragging=TRUE;
                break;
        case 3: // do popup
                // g_print ("RM BUTTON_PRESS (do popup) image-pixel XY: %g, %g\n", mouse_pix_xy[0], mouse_pix_xy[1]);
                // gtk_menu_popup_at_pointer (Menu, Event)

                // do graph window menu popup
                gtk_popover_set_pointing_to (GTK_POPOVER (pc->p_popup_menu_cv), &(GdkRectangle){ x, y, 1, 1});
                gtk_popover_popup (GTK_POPOVER (pc->p_popup_menu_cv));
                break;
        }
}

void ProfileControl::released_cb (GtkGesture *gesture, int n_press, double x, double y, ProfileControl *pc){
        double mouse_pix_xy[2];
        int button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture));
        g_message ("RELEASED %d #%d at %g %g", button, n_press, x,y);

        cairo_item *items[2] = { pc->Cursor[0][1], pc->Cursor[1][1] };

        // -- this must be the inverted transformation for canvas mouse coordinates into world
        // -- see canvas_draw_callback (), cairo scale/translate +++ 
        // ***** cairo_scale (cr, pc->pixel_size, pc->pixel_size);
        // ***** cairo_translate (cr, 3.*pc->border, 5./4.*pc->border);
        // undo cairo image translation/scale:
        mouse_pix_xy[0] = ((double)x/pc->pixel_size - (double)(pc->left_border*pc->border));
        mouse_pix_xy[1] = ((double)y/pc->pixel_size - (double)(pc->top_border*pc->border));
        // ++++++++++++++++++++++++++++++++++++++++++++++++++
        
        pc->canvas2scan (mouse_pix_xy[0], mouse_pix_xy[1], mouse_pix_xy[0], mouse_pix_xy[1]);
        
        switch(button) {
        case 1:
                {
                        DA_Event event = { EV_BUTTON_1, EV_BUTTON_RELEASE, x,y };
                        if (pc->tmp_effected >= 0){
                                //g_message ("CANVAS EVENT cursor grab mode image-pixel XY: %g, %g\n", mouse_pix_xy[0], mouse_pix_xy[1]);
                                pc->cursor_event (items, &event, mouse_pix_xy, pc);
                                break;
                        }
                        pc->cursor_event (items, &event, mouse_pix_xy, pc);

                        if (pc->tmp_effected >= 0) // handled by object, done. no more action here!
                                break;
                }
                break;
        }
}

void ProfileControl::drag_motion (GtkEventControllerMotion *motion, gdouble x, gdouble y, ProfileControl *pc){
        double mouse_pix_xy[2];
        g_message ("DRAGGING %g %g", x,y);
        cairo_item *items[2] = { pc->Cursor[0][1], pc->Cursor[1][1] };

        // -- this must be the inverted transformation for canvas mouse coordinates into world
        // -- see canvas_draw_callback (), cairo scale/translate +++ 
        // ***** cairo_scale (cr, pc->pixel_size, pc->pixel_size);
        // ***** cairo_translate (cr, 3.*pc->border, 5./4.*pc->border);
        // undo cairo image translation/scale:
        mouse_pix_xy[0] = ((double)x/pc->pixel_size - (double)(pc->left_border*pc->border));
        mouse_pix_xy[1] = ((double)y/pc->pixel_size - (double)(pc->top_border*pc->border));
        // ++++++++++++++++++++++++++++++++++++++++++++++++++
        
        pc->canvas2scan (mouse_pix_xy[0], mouse_pix_xy[1], mouse_pix_xy[0], mouse_pix_xy[1]);
        
        DA_Event event = { EV_BUTTON_1, EV_MOTION_NOTIFY,x,y  };
        if (pc->tmp_effected >= 0){
                //g_message ("CANVAS EVENT cursor grab mode image-pixel XY: %g, %g\n", mouse_pix_xy[0], mouse_pix_xy[1]);
                pc->cursor_event (items, &event, mouse_pix_xy, pc);
                return;
        }
        pc->cursor_event (items, &event, mouse_pix_xy, pc);

        if (pc->tmp_effected >= 0) // handled by object, done. no more action here!
                return;
}

   


void ProfileControl::auto_update(int rate){ // auto update, rate in ms. Disable: rate=0
        if (rate > 0 && !auto_update_id)
		auto_update_id = g_timeout_add (rate, (GSourceFunc) ProfileControl_auto_update_callback, this);
  
	if (rate == 0 && auto_update_id){
//	      g_idle_remove_by_data (this);
	      auto_update_id = 0;
	}
}

void ProfileControl::SetSize(double new_aspect){
	if (papixel > 4000) return;

        if (new_aspect > 0.)
                aspect = new_aspect;

        if (aspect > 100. || aspect < 0.01)
                return;
        
	cxwidth=pasize*aspect;
	cywidth=pasize;

        pixel_size = (double)papixel / (3*border+pasize);
        lw_1 = 1. / pixel_size;

        // calculate approx. window size -- need to est. borders. GTK3QQQ: find actual widget (button onr ight) width??
        current_geometry[0] = papixel*(aspect+3*border);
        current_geometry[1] = papixel*(1.+2*border);
        gtk_widget_set_size_request (canvas, (int)current_geometry[0], (gint)current_geometry[1]);
        //        gtk_widget_set_size_request (canvas, (int)(papixel*(aspect+3*border)), (gint)(papixel*(1.+border))+statusheight);

        updateFrame ();
        updateTics  (TRUE);
        updateScans ();
}



gint ProfileControl::cursor_event(cairo_item *items[2], DA_Event *event, double mxy[2], ProfileControl *pc){
        double x, y;
  	static int dragging=FALSE;
        cairo_item *item = NULL;

	x = mxy[0];
	y = mxy[1];
 
        //g_message (" ProfileControl::cursor_event [%s] %d, %d\n", pc->title, pc->tmp_effected, dragging );

        if (pc->tmp_effected >= 0 && dragging)
                item = items[pc->tmp_effected];
        else {
                // check items
                pc->scan2canvas (x ,y);
                for(int i=0; i<2; i++)
                        if (items[i]){
                                //g_message (" ProfileControl::cursor_event check item[%d] [%s] CHK BBOX: %d, %d\n",i,pc->title, pc->tmp_effected, dragging );
                                if (items[i]->check_grab_bbox (x, y)){
                                        //if (items[i]->check_grab_bbox_dxy (mxy[0], mxy[1], pc->cxwidth/66., pc->cywidth/66.)){
                                        item = items[i];
                                        pc->tmp_effected = i;
                                        //g_message (" ProfileControl::cursor_event [%s] CHK BBOX OK: %d, %d\n", pc->title, pc->tmp_effected, dragging );
                                        break;
                                }
                        }
        }

        if (item == NULL)
                return false;

	switch (event->type){
        case EV_BUTTON_PRESS:
                switch(event->button){
                case 1:
                        {
                                // FIX-ME GTK4
                                //GdkWindow* win = gtk_widget_get_parent_window(pc->canvas);
                                //GdkCursor *grab_cursor = gdk_cursor_new_from_name (gdk_display_get_default (), "grab");
                                //gdk_window_set_cursor(win, grab_cursor);
                        }
                        item->grab ();
                        
                        dragging = true;
                        break;

                default:
                        break;
                }
                break;
		
        case EV_MOTION_NOTIFY:
                if (item->is_grabbed () && dragging){ // && (event->motion.state & GDK_BUTTON1_MASK)){
                        //g_message (" ProfileControl::cursor_event MOTION EV [%d] to x=%g", pc->tmp_effected, mxy[0]);
                        if (pc->tmp_effected >= 0){ // make sure
                                //g_message (" ProfileControl::cursor_event MOTION EV => MOVE CURSOR");
                                pc->moveCur (pc->tmp_effected, 0, 2, mxy[0]);
                        }
                }
                break;
		
        case EV_BUTTON_RELEASE:
                if (item->is_grabbed () && dragging){
                        item->ungrab ();
                        pc->tmp_effected = -1;
                        // FIX-ME GTK4
                        //GdkWindow* win = gtk_widget_get_parent_window(pc->canvas);
                        //GdkCursor *default_cursor = gdk_cursor_new_from_name (gdk_display_get_default (), "default");
                        //gdk_window_set_cursor(win, default_cursor);
                }

                dragging = false;
                return false;
                break;

        default:
                break;
        }
        
	return true;
}


const gchar *ProfileControl::get_xcolor (int i){
	const gchar *sccl[] = { "red","orange","yellow",
				"green","ForestGreen",
				"magenta","SteelBlue","blue"
				
	};
	if (xcolors_list && num_xcolors)
		return (const gchar*)xcolors_list[i%num_xcolors];
	else
		return sccl[i&7];
}

void ProfileControl::AddScan(Scan* scan, int line, gchar *col){
	XSM_DEBUG (DBG_L3, "ProfileControl::AddScan");
	if(col)
		last_pe = new ProfileElement(scan, this, col, line);
	else
		last_pe = new ProfileElement(scan, this, get_xcolor (scount), line);
	ScanList = g_slist_prepend( ScanList, last_pe);
	++scount;
	XSM_DEBUG (DBG_L3, "ProfileControl::AddScan done.");
}

void ProfileControl::AddLine(int line, gchar *col){
	if(!last_pe) return;

	if(col)
		last_pe = new ProfileElement(last_pe, col, line);
	else
		last_pe = new ProfileElement(last_pe, get_xcolor (scount), line);
	ScanList = g_slist_prepend( ScanList, last_pe);
	++scount;
}

void ProfileControl::RemoveScans(){
	if(!ScanList || !scount) return;
	
	g_slist_foreach(ScanList, (GFunc) ProfileControl::kill_elem, this);
	g_slist_free(ScanList);
	ScanList = NULL;
	last_pe = NULL;
	scount   = 0;
}

void ProfileControl::save_data (const gchar *fname){
        std::ofstream o;
        GSList *s = ScanList;
        o.open (fname);

        while (s){
                ProfileElement *pe = (ProfileElement*)s->data;
                if (o.good ()){
                        int i=0;
                        o << "# Dataset Start" << std::endl;
                        do {
                                pe->stream_set (o, i++);
                                if (mode & PROFILE_MODE_BINARY8){
                                        if (!(mode & PROFILE_MODE_BINARY16) && i >= 8)
                                                i = NPI;
                                }
                                else
                                        i = NPI;
                        } while (i < NPI);
                        o << "# Dataset End" << std::endl;
                }
                s = g_slist_next (s);
        }
	g_slist_foreach(ScanList, (GFunc) ProfileControl::update_elem, this);

}

void ProfileControl::draw_elem(ProfileElement *pe, cairo_t* cr){
        //	XSM_DEBUG (DBG_L5, "ProfileElement::draw_elem - enter");
        pe->draw (cr);
}

void ProfileControl::update_elem(ProfileElement *pe, ProfileControl *pc){
	int i=0;
	double yboff=0.;
	int bmask = 0x0001;
	XSM_DEBUG (DBG_L5, "ProfileElement::update_elem - enter");
	do {
                XSM_DEBUG (DBG_L5, "ProfileElement::update_elem - calc");
                // calculate and map path to canvas coordinates
		double dc = pe->calc (pc->mode, i, bmask, yboff, pc->canvas);

                XSM_DEBUG (DBG_L5, "ProfileElement::update_elem - calc1 OK");
		if (dc > 0.){
			gchar *p=g_strdup_printf ("Y-DC: %g", dc);
                        gtk_statusbar_remove_all (GTK_STATUSBAR (pc->statusbar), pc->statusid);
			gtk_statusbar_push (GTK_STATUSBAR(pc->statusbar), pc->statusid, p);
			g_free (p);
		}
		pc->SetXrange(pe->xlocmin, pe->xlocmax);
		if (pc->mode & PROFILE_MODE_BINARY16)
			pc->SetYrange(-0.5, 16.);	
		else if (pc->mode & PROFILE_MODE_BINARY8)
			pc->SetYrange(-0.5, 8.);	
		else if(pc->scaleing&PROFILE_SCALE_YEXPAND || (pc->SklOnly && !(pc->scaleing&PROFILE_SCALE_YHOLD)))
			pc->SetYrange(MIN(pe->ylocmin, pc->ymin), MAX(pe->ylocmax, pc->ymax));
		else
			if(!(pc->scaleing & PROFILE_SCALE_YHOLD))
				pc->SetYrange(pe->ylocmin, pe->ylocmax);
		
                XSM_DEBUG (DBG_L5, "ProfileElement::update_elem - skl done");
		if(!pc->SklOnly){
                        XSM_DEBUG (DBG_L5, "ProfileElement::update_elem - draw");
			pe->update (pc->canvas, i++, CAIRO_LINE_SOLID);
		} else
			i = NPI;
                XSM_DEBUG (DBG_L5, "ProfileElement::update_elem - draw passed");
		
		if (pc->mode & PROFILE_MODE_BINARY8){
			bmask <<= 1; 
			yboff += 1.;
			if (!(pc->mode & PROFILE_MODE_BINARY16) && i >= 8)
				i = NPI;
		} else 
			i = NPI;
	} while (i < NPI);
	XSM_DEBUG (DBG_L5, "ProfileElement::update_elem - done");
}

// Add New Data / Change:
// 1) add scan data:
//    sc : Scan to add
// ------------------------------------
// line = -1: Get marked Line from Scan
// line >= 0: use Line line from Scan
// sc==NULL => jump to Line line in last scan
gint ProfileControl::NewData_redprofile(Scan* sc, int redblue){
	if (lock){
                g_message ("ProfileControl::NewData_redprofile *** Object is Locked, skipping.");
		return TRUE; // auto load balancing, low priority
	}
	// lock object
	lock = TRUE;

	SetData_redprofile (sc, redblue);

	int sn2 = scan1d_2 ? scan1d_2->mem2d->GetNy () : 0;

        if (scount != scan1d->mem2d->GetNy () + sn2){
		RemoveScans();
		int n=scan1d->mem2d->GetNy ();
		AddScan (scan1d, 0, xcolors_list[0]);
		for(int i=1; i<n; ++i)
			AddLine(i, xcolors_list[i]);
		if (sn2>0){
			n=scan1d_2->mem2d->GetNy ();
			AddScan (scan1d_2, 0, xcolors_list_2[0]);
			for(int i=1; i<n; ++i)
				AddLine(i, xcolors_list_2[i]);
		}
	}

	gchar *tmp;
	SetXlabel (tmp = scan1d->data.Xunit->MakeLongLabel());
	g_free (tmp);
	
	SetYlabel (tmp = scan1d->data.Zunit->MakeLongLabel());
	g_free (tmp);

	// unlock object
	lock = FALSE;

	return FALSE;
}

gint ProfileControl::NewData(Scan* sc, int line, int cpyscan, VObject *vo, gboolean append){
	gchar *txt;

	if (lock){
		g_message ("ProfileControl::NewData *** Object is Locked, skipping.");
		return TRUE; // auto load balancing, low priority
	}
	// lock object
	lock = TRUE;

	if(sc){
		if(cpyscan){ // make copy
  		        SetData(sc, vo, append);
			if (scount != scan1d->mem2d->GetNy ()){
				RemoveScans();
				int n=scan1d->mem2d->GetNy ();
				AddScan (scan1d);
				for(int i=1; i<n; ++i)
					AddLine(i);
			}
			
		}else{ // use scan
			if(! scount){
				XSM_DEBUG (DBG_L2, "ProfileControl::NewData AddScan, scount>0, SetData");
				SetData(sc, -2); // only ref to scan !!
				XSM_DEBUG (DBG_L2, "ProfileControl::NewData AddScan, sc=" << sc);
				AddScan(sc);
			}
			last_pe->SetY(line);
		}
	}

	if (line >= 0)
		txt=g_strdup_printf("Line: %d", line);
	else {
		if(sc->RedLineActive )
			txt=g_strdup_printf ("Red Line #: %d", sc->Pkt2dScanLine[0].y);
		else{
			txt=g_strdup_printf ("multi dimensional cut %s, # series: %d", vo?vo->obj_text ():"", scan1d->mem2d->GetNy ());
                        if (titlelabel)
                                titlelabel->set_text (txt);
                }
	}

        gtk_statusbar_remove_all (GTK_STATUSBAR (statusbar), statusid);
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), statusid, txt);
	g_free(txt);

	Scan *labsrcsc = NULL;
	if(cpyscan){
		labsrcsc = scan1d;
	} else  labsrcsc = sc;

	if(labsrcsc){
		gchar *tmp;
		SetXlabel (tmp=labsrcsc->data.Xunit->MakeLongLabel());
		g_free (tmp);
	  
		SetYlabel (tmp = labsrcsc->data.Zunit->MakeLongLabel());
		g_free (tmp);
	}

	// unlock object
	lock = FALSE;

	if (vo)
		UpdateArea();


	return FALSE;
}

void ProfileControl::drawScans (cairo_t* cr){
        //	XSM_DEBUG (DBG_L5, "ProfileElement::drawScans - enter");
	g_slist_foreach(ScanList, (GFunc) ProfileControl::draw_elem, cr);
}

void ProfileControl::updateScans (int flg)
{
        //	XSM_DEBUG (DBG_L5, "ProfileElement::updateScans - enter");
	SklOnly=TRUE;
	g_slist_foreach(ScanList, (GFunc) ProfileControl::update_elem, this); // do only scaling
	SklOnly=FALSE;
	g_slist_foreach(ScanList, (GFunc) ProfileControl::update_elem, this);
}

void ProfileControl::updateFrame ()
{
	if(!frame){
		frame = new cairo_item_rectangle (0., 0., cxwidth, cywidth);
                frame->set_fill_rgba (xsmres.ProfileCanvasColor);
                frame->set_stroke_rgba (xsmres.ProfileFrameColor);
                frame->set_line_width (lw_1*1.5*xsmres.ProfileLineWidth);
        } else 
		frame->set_xy (1, cxwidth, cywidth);

	if(!xaxislabel){
		xaxislabel = new cairo_item_text ( cxwidth/2., cywidth+border, "X-axis");
                xaxislabel->set_stroke_rgba (xsmres.ProfileLabelColor);
                font_size_10 = xaxislabel->set_font_face_size_from_string (xsmres.ProfileTicFont, get_lw1 ()); // make reference for tics only
                // font_size_10 = get_lw1 (10.); // set reference
                xaxislabel->set_font_face_size_from_string (xsmres.ProfileLabelFont, get_lw1 ());
                xaxislabel->set_anchor (CAIRO_ANCHOR_CENTER);
        } else  
		xaxislabel->set_position (cxwidth/2., cywidth+border);

	if(!yaxislabel){
		yaxislabel = new cairo_item_text ( -2.*border, cywidth/2., "Y-axis");
                yaxislabel->set_stroke_rgba (xsmres.ProfileLabelColor);
                yaxislabel->set_font_face_size_from_string (xsmres.ProfileLabelFont, get_lw1 ());
                //yaxislabel->set_stroke_rgba (0.,0.,1.,0.5);
                //yaxislabel->set_font_face_size ("Georgia", get_lw1 (12.)); //  "font", xsmres.ProfileLabFont,
                yaxislabel->set_anchor (CAIRO_ANCHOR_CENTER);
        } else 
                yaxislabel->set_position (-2.5*border, cywidth/2.);

#if 1
	if(!saxislabel){
		saxislabel = new cairo_item_text ( cxwidth, -border, "Series");
                saxislabel->set_stroke_rgba (xsmres.ProfileLabelColor);
                saxislabel->set_font_face_size_from_string (xsmres.ProfileLabelFont, get_lw1 ());
                saxislabel->set_anchor (CAIRO_ANCHOR_CENTER);
        } else 
                saxislabel->set_position ( cxwidth+border, -border);
        saxislabel->queue_update (canvas);
#endif
	if(!titlelabel){
		titlelabel = new cairo_item_text ( cxwidth/2., -border, "Title");
                titlelabel->set_stroke_rgba (xsmres.ProfileTitleColor);
                titlelabel->set_font_face_size_from_string (xsmres.ProfileTitleFont, get_lw1 ());
                //titlelabel->set_stroke_rgba (0.,0.,1.,1.);
                //titlelabel->set_font_face_size ("Georgia", get_lw1 (14.)); //  "font", xsmres.ProfileLabFont,
                titlelabel->set_anchor (CAIRO_ANCHOR_CENTER);
        } else {
		titlelabel->set_position (cxwidth/2., -border);
                if (title)
                        titlelabel->set_text (title);
        }

	if(!infotext){
		infotext = new cairo_item_text ( cxwidth, -0.4*border, "dec info");
                infotext->set_stroke_rgba (0.1,0.1,0.1,0.8);
                infotext->set_font_face_size ("Unutu", get_lw1 (10.));
                infotext->set_anchor (CAIRO_ANCHOR_E);
        } else {
		infotext->set_position ( cxwidth, -0.4*border);
                if (last_pe){
                        gchar *info;
                        gint d,n;
                        last_pe->get_decimation (d,n);
                        if (mode & PROFILE_MODE_YLOWPASS)
                                info = g_strdup_printf ("lp=%g dec=%d:%d",lp_gain,d,n);
                        else
                                info = g_strdup_printf ("dec=%d:%d",d,n);
                        infotext->set_text (info);
                        g_free (info);
                }
        }
        
        frame->queue_update (canvas);
        xaxislabel->queue_update (canvas);
        yaxislabel->queue_update (canvas);
        titlelabel->queue_update (canvas);
        infotext->queue_update (canvas);
}

#define TIC_TOP    1
#define TIC_BOTTOM 2
#define TIC_LEFT   3
#define TIC_RIGHT  4
#define TIC_GRID_H 5
#define TIC_GRID_V 6
#define TIC_EMPTY  10
#define LAB_EMPTY  11
#define ADD_LEGEND       20
#define ADD_LEGEND_EMPTY 21


void ProfileControl::addTic(cairo_item **tic, cairo_item_text **lab,
			    double val, double len, int pos, const gchar *fmt, double tval, const gchar *color){
	double tx=0, ty=0;
	int lstyl;
        int txtanchor = CAIRO_ANCHOR_CENTER;
        double coords[4];
  
        //        XSM_DEBUG (DBG_L5, "ProfileElement::addTic");
	
	if (tval != tval || val != val || len != len){ // check for floating infinity "NaN" and abort!
		XSM_DEBUG (DBG_L5, "ProfileElement::addTic out-of-range abort");
		return;
	}

	lstyl=CAIRO_LINE_SOLID;

        //        XSM_DEBUG (DBG_L5, "ProfileElement::addTic - select");

        if(*tic == NULL)
                *tic = new cairo_item_path (2);
        if (color)
                (*tic)->set_stroke_rgba (color);
        else
                (*tic)->set_stroke_rgba ("black");

	switch(pos){
	case TIC_EMPTY: case LAB_EMPTY:
	case TIC_TOP:    tx=coords[0]=coords[2]=scan2canvasX(val); coords[1]=0.; 
		ty=-len+(coords[3]=-len); 
		txtanchor = CAIRO_ANCHOR_N;
		break;
	case TIC_BOTTOM: tx=coords[0]=coords[2]=scan2canvasX(val); coords[1]=cywidth; 
		ty=len+(coords[3]=cywidth+len); 
		txtanchor = CAIRO_ANCHOR_S;
		break;
	case TIC_LEFT:   ty=coords[1]=coords[3]=scan2canvasY(val); coords[0]=0.; 
		tx=-4*len+(coords[2]=-len); 
		txtanchor = CAIRO_ANCHOR_E;
		break;
	case TIC_RIGHT:  ty=coords[1]=coords[3]=scan2canvasY(val); coords[0]=cxwidth; 
		tx=4*len+(coords[2]=cxwidth+len); 
		txtanchor = CAIRO_ANCHOR_W;
		break;
	case TIC_GRID_V:
		coords[0]=coords[2]=scan2canvasX(val);
		coords[1]=0.; coords[3]=cywidth; 
                (*tic)->set_stroke_rgba (xsmres.ProfileGridColor);
		lstyl=CAIRO_LINE_ON_OFF_DASH;
		break;
	case TIC_GRID_H:
		coords[1]=coords[3]=scan2canvasY(val); 
		coords[0]=0.; coords[2]=cxwidth; 
                (*tic)->set_stroke_rgba (xsmres.ProfileGridColor);
		lstyl=CAIRO_LINE_ON_OFF_DASH;
		break;
	case ADD_LEGEND:  ty=coords[1]=coords[3]=scan2canvasY(val); coords[0]=cxwidth+len/10.; 
		tx=len/2.+(coords[2]=cxwidth+len+len/10.); 
		txtanchor = CAIRO_ANCHOR_W;
                if (color)
                        (*tic)->set_stroke_rgba (color);
                //                (*tic)->set_stroke_rgba (xsmres.ProfileGridColor);
		lstyl=CAIRO_LINE_SOLID;
		break;
	default:
		return;
	}

        //	XSM_DEBUG (DBG_L5, "ProfileElement::addTic - format");
        
	if(fmt && ty != NAN && tx != NAN && ty != -NAN && tx != -NAN && tval != NAN && tval != -NAN){
                if (fabs (tval) < 1e-15) tval = 0.; // check for "zero" and eliminate rounding residuals
		gchar *txt=g_strdup_printf (fmt, tval);
		if(*lab == NULL)
			*lab = new cairo_item_text (tx, ty, txt);
                else
                        (*lab)->set_text (tx, ty, txt);
                (*lab)->set_stroke_rgba (xsmres.ProfileTicColor);
                (*lab)->set_font_face_size_from_string (xsmres.ProfileTicFont, get_lw1 ());
                (*lab)->set_font_face_size ("Unutu", font_size_10); // from xsmres.ProfileTicFont, set at initial frame build -- needs to scale now!
                (*lab)->set_anchor (txtanchor); // anchor
                (*lab)->queue_update (canvas);
		g_free(txt);
	}
		
        //	XSM_DEBUG (DBG_L5, "ProfileElement::addTic - item_set ");

        (*tic)->set_xy (0, coords[0], coords[1]);
        (*tic)->set_xy (1, coords[2], coords[3]);
        (*tic)->set_line_width (get_lw1 ());
        (*tic)->set_line_style (lstyl);
        (*tic)->queue_update (canvas);
		
        //	XSM_DEBUG (DBG_L5, "ProfileElement::addTic done");
}

const gchar *ProfileControl::get_ys_label (int i){ if (i < PC_LEGEND_ITEMS_MAX) return ys_legend_label_list[i]; else return NULL; }
void  *ProfileControl::set_ys_label (int i, const gchar *label){
	if (i < PC_LEGEND_ITEMS_MAX){
		if (ys_legend_label_list[i])
			g_free (ys_legend_label_list[i]);
		ys_legend_label_list[i] = g_strdup (label); 
	}
	return NULL;
}

gint ProfileControl::updateTics (gboolean force)
{
	int    i, nTics;
	double l10;
	double Lab,dLab,MaxLab,LabW,Lab0;
	const double TicL1 = 0.01;
	const double TicL2 = 0.015;
	XSM_DEBUG (DBG_L5, "ProfileElement::drawTics X");
 
	// rebuild only if needed !!
	if(tic_x1 != xmin || tic_x2 != xmax || tic_xm != mode || Xtics[0]==NULL || force){ // *VG?uiv
		tic_x1=xmin; tic_x2=xmax; tic_ym = mode;
		nTics  = Xticn;
				
		Lab0   = xmin;
		MaxLab = Lab0+xrange;
		ixt=ixl=0;
#define TICLABSEP 1
		if(Lab0 < MaxLab){
			LabW = MaxLab-Lab0;
			dLab = AutoSkl(LabW/(double)nTics);
			l10 = pow(10.,floor(log10(MaxLab)));
			if(l10 < 1000. && l10 > 0.01) l10=1.;
			if(dLab <= 0.) return(-1);
			for(i=0, Lab=AutoNext(Lab0,dLab); Lab<MaxLab; Lab+=dLab, ++i){
				if (ixt >= PC_XTN-2) break;
				if (ixl >= PC_XLN-2) break;
				if(i%TICLABSEP){ 
					addTic(&Xtics[ixt++], &Xlabels[ixl], Lab, TicL1, TIC_BOTTOM);
				}
				else{
					addTic(&Xtics[ixt++], &Xlabels[ixl++], Lab, TicL2, TIC_BOTTOM, xticfmt, Lab);
					if(mode & PROFILE_MODE_XGRID)
						addTic(&Xtics[ixt++], &Xlabels[ixl], Lab, TicL2, TIC_GRID_V);
				}
			}
		}
		else{
			LabW = Lab0-MaxLab;
			dLab = AutoSkl(LabW/(double)nTics);
			l10 = pow(10.,floor(log10(MaxLab)));
			if(l10 < 1000. && l10 > 0.01) l10=1.;
			if(dLab <= 0.) return(-1);
			for(i=0, Lab=AutoNext(MaxLab,dLab); Lab<Lab0; Lab+=dLab, ++i){
				if (ixt >= PC_XTN-2) break;
				if (ixl >= PC_XLN-2) break;
				if(i%TICLABSEP){ 
					addTic(&Xtics[ixt++], &Xlabels[ixl], Lab, TicL1, TIC_BOTTOM);
				}
				else{
					addTic(&Xtics[ixt++], &Xlabels[ixl++], Lab, TicL2, TIC_BOTTOM, xticfmt, Lab);
					if(mode & PROFILE_MODE_XGRID)
						addTic(&Xtics[ixt++], &Xlabels[ixl], Lab, TicL2, TIC_GRID_V);
				}
			}
		}
		while(Xtics[ixt]){
			UNREF_DELETE_CAIRO_ITEM (Xtics[ixt], canvas);
			ixt++;
		}
		while(Xlabels[ixl]){
			UNREF_DELETE_CAIRO_ITEM (Xlabels[ixl], canvas);
			ixl++;
		}
	}

	XSM_DEBUG (DBG_L5, "ProfileElement::drawTics Y");
	if (tic_y1 != ymin || tic_y2 != ymax || tic_ym != mode || Ytics[0]==NULL || force){
		tic_y1=ymin; tic_y2=ymax; tic_ym=mode;
		nTics  = Yticn;
		Lab0   = ymin;
		MaxLab = Lab0+yrange;
		iyt=iyl=0;
    
		if(mode & PROFILE_MODE_YLOG){
			XSM_DEBUG (DBG_L5, "ProfileElement::drawTics Y-log");
			if (Lab0 <= 0.){
				XSM_DEBUG_WARNING (DBG_L2, "ProfileElement::drawTics Y-log mode: negative Ymin!");
				Lab0 = MaxLab/LOG_RESCUE_RATIO; // FixMe: auto safety hack -- must match!
			}
			if (Lab0 > 0.){
				l10 = pow (10.,floor (log10 (Lab0)));
				for (i=1, Lab=l10; Lab<MaxLab; Lab+=l10, ++i){
					if (iyt >= PC_YTN-2) break;
					if (iyl >= PC_YLN-2) break;
					if (Lab < Lab0) continue;
					if (Lab > 9.999*l10){ l10=Lab=10.*l10; i=1; }
					if (i!=5 && i!=1){ 
						addTic (&Ytics[iyt++], &Ylabels[iyl], Lab, TicL1, TIC_LEFT);
					}
					else{
						addTic (&Ytics[iyt++], &Ylabels[iyl++], Lab, TicL1, TIC_LEFT, yticfmt, Lab);
						if (mode & PROFILE_MODE_YGRID)
							addTic(&Ytics[iyt++], &Ylabels[iyl], Lab, TicL2, TIC_GRID_H);
					}
				}
			}
		}else{
			if(Lab0 < MaxLab){
				LabW = MaxLab-Lab0;
				dLab = AutoSkl(LabW/(double)nTics);
				l10 = pow(10.,floor(log10(MaxLab)));
				if(l10 < 1000. && l10 > 0.01) l10=1.;
				if(dLab <= 0.) return(-1);
				for(i=0, Lab=AutoNext(Lab0,dLab); Lab<MaxLab; Lab+=dLab, ++i){
					if (iyt >= PC_YTN-2) break;
					if (iyl >= PC_YLN-2) break;
					if(i%TICLABSEP){ 
						addTic(&Ytics[iyt++], &Ylabels[iyl], Lab, TicL1, TIC_LEFT);
					}
					else{
						addTic(&Ytics[iyt++], &Ylabels[iyl++], Lab, TicL1, TIC_LEFT, yticfmt, Lab);
						if(mode & PROFILE_MODE_YGRID)
							addTic(&Ytics[iyt++], &Ylabels[iyl], Lab, TicL2, TIC_GRID_H);
					}
				}
			}
			else{
				LabW = Lab0-MaxLab;
				dLab = AutoSkl(LabW/(double)nTics);
				l10 = pow(10.,floor(log10(MaxLab)));
				if(l10 < 1000. && l10 > 0.01) l10=1.;
				if(dLab <= 0.) return(-1);
				for(i=0, Lab=AutoNext(MaxLab,dLab); Lab<Lab0; Lab+=dLab, ++i){
					if (iyt >= PC_YTN-2) break;
					if (iyl >= PC_YLN-2) break;
					if(i%TICLABSEP){ 
						addTic(&Ytics[iyt++], &Ylabels[iyl], Lab, TicL1, TIC_LEFT);
					}
					else{
						addTic(&Ytics[iyt++], &Ylabels[iyl++], Lab, TicL1, TIC_LEFT, yticfmt, Lab);
						if(mode & PROFILE_MODE_YGRID)
							addTic(&Ytics[iyt++], &Ylabels[iyl], Lab, TicL2, TIC_GRID_H);
					}
				}
			}
		}

		XSM_DEBUG (DBG_L5, "ProfileElement::drawTics - garbage collecting");


		while(Ytics[iyt]){
			UNREF_DELETE_CAIRO_ITEM (Ytics[iyt], canvas);
                        iyt++;
		}
		while(Ylabels[iyl]){
			UNREF_DELETE_CAIRO_ITEM (Ylabels[iyl], canvas);
			iyl++;
		}
	}


	if (num_legend != scount || force){
		int k=0;
		Scan *s=NULL;
                if (mode & PROFILE_MODE_STICS){
                        for (i=0; i<scount && i < PC_LEGEND_ITEMS_MAX-2; ++i, ++k){
                                if (get_ys_label (i)){
                                        gchar *tmp = g_strconcat ("%.0f: ", get_ys_label (i), NULL);
                                        addTic(&LegendSym[i], &LegendText[i], ymax-i*(ymax-ymin)/scount, TicL1*10., ADD_LEGEND, tmp, 
                                               (double)i, get_xcolor (i));
                                        g_free (tmp);
                                } else {
                                        ProfileElement *pe = (ProfileElement*)g_slist_nth_data (ScanList, i);
                                        if (s && s != pe->get_scan ())
                                                k=0;
                                        addTic(&LegendSym[i], &LegendText[i], ymax-i*(ymax-ymin)/scount, TicL1*10., ADD_LEGEND, "%.2f", 
                                               (pe->get_scan ())->mem2d->data->GetYLookup (k), pe->get_color ());
                                        s=pe->get_scan ();
                                }
                        }
                } else {
                        i=0;
                }
		while(LegendSym[i]){
                        UNREF_DELETE_CAIRO_ITEM (LegendSym[i], canvas);
                        UNREF_DELETE_CAIRO_ITEM (LegendText[i], canvas);
			++i;
		}
	}

	XSM_DEBUG (DBG_L5, "ProfileElement::drawTics - done");

	return 0;
}

void ProfileControl::UpdateArea ()
{
	if (lock){
		g_message ("ProfileControl::UpdateArea -- scans/profile draw is locked/busy: skipping update.");
		return; // avoid attempts to draw while scan data is updated
	}

        if (mode & PROFILE_MODE_HEADER)
                top_border = 5./4.;
        else
                top_border = 1./4.;
                        

	XSM_DEBUG (DBG_L4,  "ProfileControl::UpdateArea drw scans" );
        updateScans ();
        
	XSM_DEBUG (DBG_L4,  "ProfileControl::UpdateArea ticks?" );
	if(mode & PROFILE_MODE_NOTICS){
		int i=0;
		while(Xtics[i]){
                        UNREF_DELETE_CAIRO_ITEM (Xtics[i], canvas);
			i++;
		}
		i=0;
		while(Xlabels[i]){	
                        UNREF_DELETE_CAIRO_ITEM (Xlabels[i], canvas);
			i++;
		}
		i=0;
		while(Ytics[i]){
                        UNREF_DELETE_CAIRO_ITEM (Ytics[i], canvas);
			i++;
		}
		i=0;
		while(Ylabels[i]){
                        UNREF_DELETE_CAIRO_ITEM (Ylabels[i], canvas);
			i++;
		}
	}
	else{
		XSM_DEBUG (DBG_L4,  "ProfileControl::UpdateArea drawing ticks!" );
                updateTics();
                moveCur (0);
                moveCur (1);
	}
}

void ProfileControl::showCur(int id, int show){
	if(show){
		if(Cursor[id][0]){
			Cursor[id][0]->show ();
                        Cursor[id][0]->update_bbox ();
                        Cursor[id][0]->queue_update (canvas);
			Cursor[id][1]->show ();
                        Cursor[id][1]->update_bbox ();
                        Cursor[id][1]->queue_update (canvas);
		}else{
                        double x,y;
			const char *c[]={"blue","green"};

                        last_pe->GetCurXYc(&x, &y, CursorsIdx[id]);

                        Cursor[id][0] = new cairo_item_segments (4);
                        Cursor[id][0]->set_xy (0, 0., y);
                        Cursor[id][0]->set_xy (1, cxwidth, y);
                        Cursor[id][0]->set_xy (2, x, 0.);
                        Cursor[id][0]->set_xy (3, x, cywidth);
                        Cursor[id][0]->set_stroke_rgba (c[id]);
                        Cursor[id][0]->set_line_width (get_lw1 (1.5*xsmres.ProfileLineWidth));
			Cursor[id][0]->show ();
                        Cursor[id][0]->queue_update (canvas);

                        double rsz=0.05*cxwidth;
                        Cursor[id][1] = new cairo_item_path_closed (3);
                        Cursor[id][1]->set_position (x, cywidth);
                        Cursor[id][1]->set_xy (0, 0., 0.);
                        Cursor[id][1]->set_xy (1, -rsz/3., -rsz);
                        Cursor[id][1]->set_xy (2,  rsz/3., -rsz);
                        Cursor[id][1]->set_stroke_rgba (c[id]);
                        Cursor[id][1]->set_fill_rgba (c[id]);
                        Cursor[id][1]->set_line_width (get_lw1 (1.5*xsmres.ProfileLineWidth));
                        Cursor[id][1]->show ();
                        Cursor[id][1]->update_bbox ();
                        Cursor[id][1]->queue_update (canvas);
		}
	}else{
		if(Cursor[id][0]){
                        UNREF_DELETE_CAIRO_ITEM (Cursor[id][0], canvas);
                        UNREF_DELETE_CAIRO_ITEM (Cursor[id][1], canvas);
		}
        }
        
	if (cursor_bound_object)
		cursor_bound_object->Update ();

        updateFrame ();
	UpdateArea ();
}

void ProfileControl::moveCur(int id, int dir, int search, double cx){
        double x,y;

        if (Cursor[id][0] == NULL) return;
        if (Cursor[id][1] == NULL) return;

        last_pe->GetCurXYc(&x, &y, CursorsIdx[id]);
       
	if(search == 2){
                x = cx;
                double xx,yy;
                xx=x; yy=0.;
                CursorsIdx[id] = last_pe->GetCurX(&xx, &yy); 
        }else{
		if(search){
			if(search>0) CursorsIdx[id] = last_pe->nextMax(CursorsIdx[id], dir);
			if(search<0) CursorsIdx[id] = last_pe->nextMin(CursorsIdx[id], dir);
                        last_pe->GetCurXYc(&x, &y, CursorsIdx[id]);
		}else{
                        last_pe->GetCurXYc(&x, &y, CursorsIdx[id] += dir);
		}
        }

        scan2canvas (x, y);

        //g_message ("ProfileControl::moveCur(%d, %d, %d, %g) x=%g", id,dir,search,cx,x);
        
        Cursor[id][0]->set_xy (0, 0., y);
        Cursor[id][0]->set_xy (1, cxwidth, y);
        Cursor[id][0]->set_xy (2, x, 0.);
        Cursor[id][0]->set_xy (3, x, cywidth);
        Cursor[id][0]->queue_update (canvas);
  
        Cursor[id][1]->set_position (x, cywidth);
        Cursor[id][1]->queue_update (canvas);
  
        gchar *p;
        gtk_statusbar_remove_all (GTK_STATUSBAR (statusbar), statusid);
        if (Cursor[0][0] && Cursor[1][0] )
                gtk_statusbar_push (GTK_STATUSBAR(statusbar), statusid, 
                                    p=last_pe->GetDeltaInfo( CursorsIdx[0], CursorsIdx[1], mode));
        else
                gtk_statusbar_push (GTK_STATUSBAR(statusbar), statusid, 
                                    p=last_pe->GetInfo( CursorsIdx[id], mode));
        g_free (p);

	if (cursor_bound_object)
		cursor_bound_object->Update ();
}

void ProfileControl::SetYrange (double y1, double y2)
{
	if (y1 < y2){
		ymin = y1; ymax = y2; yrange=y2-y1;
	} else if (y1 > y2){
		ymin = y2; ymax = y1; yrange=y1-y2;
	}else{ // fallback
		ymin = y2; ymax = y1+1.; yrange=y1+1.-y2;
	}
	if (ymax > 0. && ymin > 0. && ymin < ymax){
		lmin    = log (ymin);
		lmaxmin = log (ymax) - lmin;
	}else{
		if (ymax > 0.) { // FixMe: must match with / in DrawTicks!!
			lmin    = log (ymax/LOG_RESCUE_RATIO);
			lmaxmin = log (ymax) - lmin;
		} else { // dummy range
			lmaxmin = 1.;
			lmin = 0.;
		}
	}
}

void ProfileControl::SetXrange(double x1, double x2)
{
	if (x1 < x2){
		xmin = x1; xmax = x2; xrange=x2-x1;
	} else if (x1 > x2){
		xmin = x2; xmax = x1; xrange=x1-x2;
	} else{ // fallback
		xmin = x2; xmax = x1+1.; xrange=x1+1.-x2;
	}
}

void ProfileControl::SetXlabel (const gchar *xlab)
{
	if (xlab){
		if (xlabel0) 
			g_free (xlabel0);
		xlabel0 = g_strdup (xlab);
	}

	if (xlabel)
		g_free (xlabel);

	if (!xlabel0) return;

	if(mode & PROFILE_MODE_YPSD)
		xlabel = g_strconcat ("Inverse::", xlabel0, NULL);
	else
		xlabel = g_strdup (xlabel0);

	if (xaxislabel)
                xaxislabel->set_text (xlabel);
}

void ProfileControl::SetYlabel (const gchar *ylab)
{
	if (ylab){
		if (ylabel0)
			g_free (ylabel0); 
		ylabel0 = g_strdup (ylab);
	}

	if (ylabel)
		g_free (ylabel);

	if (!ylabel0) return;

	if(mode & PROFILE_MODE_YDIFF)
		ylabel = g_strconcat ("d ", ylabel0, "/dx", NULL);
	else
		ylabel = g_strdup (ylabel0);


	if (yaxislabel)
                yaxislabel->set_text (ylabel);
}

void ProfileControl::SetActive(int flg){
        gtk_statusbar_remove_all (GTK_STATUSBAR (statusbar), statusid);
	if (flg){
		// gnome_canvas_item_set (frame,
		// 		       "fill_color", "white",
		// 		       NULL);
		gtk_statusbar_push (GTK_STATUSBAR(statusbar), statusid, 
				    "channel is active now");
	}else{
		// gnome_canvas_item_set (frame,
		// 		       "fill_color", "grey80",
		// 		       NULL);
		gtk_statusbar_push(GTK_STATUSBAR(statusbar), statusid, 
				   "inactive");
	}
}

void ProfileControl::SetGraphTitle(const gchar *tit, gboolean append){
	gchar *t;
	if (append){
		t = g_strconcat (title, tit, NULL);
		g_free(title);
		title = t;
	} else {
		g_free(title);
		title = g_strdup (tit);
	}

        // FIX-ME-GTK4
        if (pc_in_window)
                SetTitle (NULL, title);
        else
                SetTitle (title);
}

void ProfileControl::file_open_callback_exec (GtkDialog *dialog,  int response){
        if (response == GTK_RESPONSE_ACCEPT){
                GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
                g_autoptr(GFile) file = gtk_file_chooser_get_file (chooser);
                gchar *tmp=g_file_get_parse_name (file);
		gapp->xsm->load (tmp);
                g_free (tmp);
        }
        gtk_window_destroy (GTK_WINDOW (dialog));
}

void ProfileControl::file_open_callback (GSimpleAction *simple, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;

        GtkWidget *dialog = gtk_file_chooser_dialog_new ("Open vpdata File",
                                                         pc->window, // parent_window
                                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                                         _("_Cancel"),
                                                         GTK_RESPONSE_CANCEL,
                                                         _("_Open"),
                                                         GTK_RESPONSE_ACCEPT,
                                                         NULL);
        gtk_widget_show (dialog);
        g_signal_connect (dialog, "response",
                          G_CALLBACK (ProfileControl::file_open_callback_exec),
                          NULL);
	//gchar *ffname;
	//ffname = gapp->file_dialog_load ("Profile to load", NULL, NULL);
	//if (ffname)
	//	gapp->xsm->load (ffname);
}

void ProfileControl::file_save_callback_exec (GtkDialog *dialog,  int response, gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        if (response == GTK_RESPONSE_ACCEPT){
                GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
                g_autoptr(GFile) file = gtk_file_chooser_get_file (chooser);
                // filename = gtk_file_chooser_get_filename (chooser);
                gchar *tmp=g_file_get_parse_name (file);
		pc->save (tmp);
                g_free (tmp);
        }
        gtk_window_destroy (GTK_WINDOW (dialog));
}

void ProfileControl::file_save_callback (GSimpleAction *simple, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;

	gchar *ffname;
	gchar *mld, *oname;
        GtkWidget *chooser = gtk_file_chooser_dialog_new ("Save Profile",
                                                          pc->window, // parent_window
                                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                                          _("_Cancel"),
                                                          GTK_RESPONSE_CANCEL,
                                                          _("_Open"),
                                                          GTK_RESPONSE_ACCEPT,
                                                          NULL);
	oname = g_strconcat (pc->scan1d->data.ui.originalname, ".asc", NULL);
        GFile *default_file_for_saving = g_file_new_for_path (oname);
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), default_file_for_saving, NULL);
        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser), oname);
        g_object_unref (default_file_for_saving);
        g_free (oname);
        
        gtk_widget_show (chooser);

        g_signal_connect (chooser, "response",
                          G_CALLBACK (ProfileControl::file_save_callback_exec),
                          user_data);

        /*
	oname = g_strconcat (pc->scan1d->data.ui.originalname, ".asc", NULL);
	ffname = gapp->file_dialog_save (N_("Profile to save"), NULL, oname);
	if (gapp->check_file (ffname)){
		pc->save (ffname);
		mld = g_strconcat (N_("profile saved as "), ffname, NULL);
	}else
		mld = g_strdup (N_("save canceld"));
        gtk_statusbar_remove_all (GTK_STATUSBAR (pc->statusbar), pc->statusid);
	gtk_statusbar_push (GTK_STATUSBAR(pc->statusbar), pc->statusid, mld);
	g_free (mld);
        */
}

void ProfileControl::file_save_data_callback_exec (GtkDialog *dialog,  int response, gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        if (response == GTK_RESPONSE_ACCEPT){
                GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
                g_autoptr(GFile) file = gtk_file_chooser_get_file (chooser);
                // filename = gtk_file_chooser_get_filename (chooser);
                gchar *tmp=g_file_get_parse_name (file);
		pc->save_data (tmp);
                g_free (tmp);
        }
        gtk_window_destroy (GTK_WINDOW (dialog));
}

void ProfileControl::file_save_data_callback (GSimpleAction *simple, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;

	gchar *ffname;
	gchar *mld, *oname;
        GtkWidget *chooser = gtk_file_chooser_dialog_new ("Save Data as Viewed",
                                                          pc->window, // parent_window
                                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                                          _("_Cancel"),
                                                          GTK_RESPONSE_CANCEL,
                                                          _("_Open"),
                                                          GTK_RESPONSE_ACCEPT,
                                                          NULL);
	oname = g_strconcat (pc->scan1d->data.ui.originalname, ".asc", NULL);
        GFile *default_file_for_saving = g_file_new_for_path (oname);
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), default_file_for_saving, NULL);
        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser), oname);
        g_object_unref (default_file_for_saving);
        //gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), oname);
        //gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), oname);
        g_free (oname);
        
        gtk_widget_show (chooser);

        g_signal_connect (chooser, "response",
                          G_CALLBACK (ProfileControl::file_save_data_callback_exec),
                          user_data);

        /*		
	oname = g_strconcat (pc->scan1d->data.ui.originalname, ".asc", NULL);
	ffname = gapp->file_dialog_save (N_("Profile data save as viewed"), NULL, oname);
	if (gapp->check_file (ffname)){
		pc->save_data (ffname);
		mld = g_strconcat (N_("profile data saved as "), ffname, NULL);
	}else
		mld = g_strdup (N_("save data canceld"));
        gtk_statusbar_remove_all (GTK_STATUSBAR (pc->statusbar), pc->statusid);
	gtk_statusbar_push (GTK_STATUSBAR(pc->statusbar), pc->statusid, mld);
	g_free (mld);
        */
}

void ProfileControl::file_save_as_callback_exec (GtkDialog *dialog,  int response, gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        if (response == GTK_RESPONSE_ACCEPT){
                GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
                g_autoptr(GFile) file = gtk_file_chooser_get_file (chooser);
                // filename = gtk_file_chooser_get_filename (chooser);
                gchar *tmp=g_file_get_parse_name (file);
		pc->save (tmp);
                g_free (tmp);
        }
        gtk_window_destroy (GTK_WINDOW (dialog));
}

void ProfileControl::file_save_as_callback (GSimpleAction *simple, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;

	gchar *ffname;
	gchar *mld, *oname;
        GtkWidget *chooser = gtk_file_chooser_dialog_new ("Save-as Profile",
                                                          pc->window, // parent_window
                                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                                          _("_Cancel"),
                                                          GTK_RESPONSE_CANCEL,
                                                          _("_Open"),
                                                          GTK_RESPONSE_ACCEPT,
                                                          NULL);
	oname = g_strconcat (pc->scan1d->data.ui.originalname, ".asc", NULL);
        GFile *default_file_for_saving = g_file_new_for_path (oname);
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), default_file_for_saving, NULL);
        gtk_file_chooser_set_current_name ( GTK_FILE_CHOOSER (chooser), oname);
        g_object_unref (default_file_for_saving);
        //gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), oname);
        //gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), oname);
        g_free (oname);
        gtk_widget_show (chooser);

        g_signal_connect (chooser, "response",
                          G_CALLBACK (ProfileControl::file_save_as_callback_exec),
                          user_data);

        /*		
	oname = g_strconcat (pc->scan1d->data.ui.originalname, ".asc", NULL);
	ffname = gapp->file_dialog_save (N_("Profile to save"), NULL, oname);
	if (gapp->check_file (ffname)){
		pc->save (ffname);
		mld = g_strconcat (N_("profile saved as "),ffname,NULL);
	}else
		mld = g_strdup (N_("save canceld"));
        gtk_statusbar_remove_all (GTK_STATUSBAR (pc->statusbar), pc->statusid);
	gtk_statusbar_push (GTK_STATUSBAR(pc->statusbar), pc->statusid, mld);
	g_free (mld);
        */
}

void ProfileControl::file_save_image_callback_exec (GtkDialog *dialog,  int response, gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        if (response == GTK_RESPONSE_ACCEPT){
                cairo_surface_t *surface;
                cairo_t *cr;
                cairo_status_t status;
                
                GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
                g_autoptr(GFile) file = gtk_file_chooser_get_file (chooser);
                // filename = gtk_file_chooser_get_filename (chooser);
                gchar *imgname=g_file_get_parse_name (file);

                if (imgname == NULL || strlen(imgname) < 5){
                        g_free (imgname);
                        return;
                }
                
                int png=0;

                double scaling = 1.;

                if (g_strrstr (imgname,".svg")){
                        surface = cairo_svg_surface_create (imgname, pc->current_geometry[0], pc->current_geometry[1]);
                        cairo_svg_surface_restrict_to_version (surface, CAIRO_SVG_VERSION_1_2);
                } else if (g_strrstr (imgname,".pdf")){
                        surface = cairo_pdf_surface_create (imgname, pc->current_geometry[0], pc->current_geometry[1]); 
                } else {
                        scaling = 3.;
                        surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, scaling * pc->current_geometry[0], scaling * pc->current_geometry[1]);
                        png=1;
                }

                cr = cairo_create (surface);
                //        cairo_scale (cr, IMAGE_DPI/72.0, IMAGE_DPI/72.0);
        
                cairo_scale (cr, scaling, scaling);
                cairo_save (cr);

                // 1) draw Frame -- not here
                // 2) draw Image and red line via ShmImage2D
                pc->canvas_draw_function (NULL, cr, 0,0, pc);

                cairo_restore (cr);

                status = cairo_status(cr);
                if (status)
                        printf("%s\n", cairo_status_to_string (status));
        
                cairo_destroy (cr);

                if (png){
                        g_print ("Cairo save scan view to png: '%s'\n", imgname);
                        status = cairo_surface_write_to_png (surface, imgname);
                        if (status)
                                printf("%s\n", cairo_status_to_string (status));
                } else {
                        cairo_surface_flush (surface);
                        cairo_surface_finish (surface);
                        g_print ("Cairo save scan view to svg/pdf: '%s'\n", imgname);
                }
        
                cairo_surface_destroy (surface);
                g_free (imgname);

        }
        gtk_window_destroy (GTK_WINDOW (dialog));
}

void ProfileControl::file_save_image_callback (GSimpleAction *simple, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	
        if(!pc->last_pe) return;

        GtkWidget *chooser = gtk_file_chooser_dialog_new ("Save Profile as Drawing [.pdf, .svg or .png]",
                                                          pc->window, // parent_window
                                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                                          _("_Cancel"),
                                                          GTK_RESPONSE_CANCEL,
                                                          _("_Open"),
                                                          GTK_RESPONSE_ACCEPT,
                                                          NULL);
        
        GtkFileFilter *f0 = gtk_file_filter_new ();
        gtk_file_filter_set_name (f0, "All");
        gtk_file_filter_add_pattern (f0, "*");
        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), f0);

        GtkFileFilter *fpng = gtk_file_filter_new ();
        gtk_file_filter_add_mime_type (fpng, "image/png");
        gtk_file_filter_add_mime_type (fpng, "image/jpeg");
        gtk_file_filter_add_mime_type (fpng, "image/gif");
        gtk_file_filter_set_name (fpng, "Images");
        gtk_file_filter_add_pattern (fpng, "*.png");
        gtk_file_filter_add_pattern (fpng, "*.jpeg");
        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), fpng);

        GtkFileFilter *fsvg = gtk_file_filter_new ();
        gtk_file_filter_set_name (fsvg, "SVG");
        gtk_file_filter_add_pattern (fsvg, "*.svg");
        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), fsvg);

        GtkFileFilter *fpdf = gtk_file_filter_new ();
        gtk_file_filter_set_name (fpdf, "PDF");
        gtk_file_filter_add_pattern (fpdf, "*.pdf");
        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), fpdf);

        GFile *default_file_for_saving = g_file_new_for_path (g_settings_get_string (gapp->get_as_settings (), "auto-save-folder"));
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), default_file_for_saving, NULL);
        g_object_unref (default_file_for_saving);
        
	gchar *suggested_name = g_strdup_printf ("%s-profile-snap.pdf", pc->last_pe->get_scan ()->data.ui.name);
        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser), suggested_name);
	g_free (suggested_name);
        
        gtk_widget_show (chooser);
        
        g_signal_connect (chooser, "response",
                          G_CALLBACK (ProfileControl::file_save_image_callback_exec),
                          user_data);
}

void ProfileControl::file_close_callback (GSimpleAction *simple, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
		
	if (pc->ref_count == 0)
		gapp->xsm->RemoveProfile (pc);
	else
		XSM_DEBUG_GP_WARNING (DBG_L1, "Sorry, cant't do this, other object depends on me!" );
}

void ProfileControl::file_print1_callback (GSimpleAction *simple, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->file_print_callback (1, pc);
}

void ProfileControl::file_print2_callback (GSimpleAction *simple, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->file_print_callback (2, pc);
}

void ProfileControl::file_print3_callback (GSimpleAction *simple, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->file_print_callback (3, pc);
}

void ProfileControl::file_print4_callback (GSimpleAction *simple, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->file_print_callback (4, pc);
}

// Experimental new print_callback for xmgrace
void ProfileControl::file_print5_callback (GSimpleAction *simple, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->file_print_callback (5, pc);
}

// Experimental new print_callback for matplotlob
void ProfileControl::file_print6_callback (GSimpleAction *simple, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->file_print_callback (6, pc);
}

void ProfileControl::file_print_callback (int index, ProfileControl *pc)
{
	int tmpf;
	gchar *tmp;
	GError *printer_failure;

	--index;
	if ((tmpf=g_file_open_tmp("GXSM_TEMPFILE_XXXXXX", &tmp, &printer_failure)) != -1){
		FILE *gricmd;
		char command[520];
		char retinfo[64];
		int ret;
        		
		close (tmpf);
		pc->save(tmp);
        	
//         		if (setenv("GXSMGRIDATAFILE", tmp, TRUE))
// 						XSM_DEBUG_ERROR (DBG_L1,  "error: can't set GXSMGRIDATAFILE enviromentvar!" );
//         		if (setenv("GXSMGRIXLAB", pc->xlabel, TRUE))
// 						XSM_DEBUG_ERROR (DBG_L1,  "error: can't set GXSMGRIXLAB enviromentvar!" );
//         		if (setenv("GXSMGRIYLAB", pc->ylabel, TRUE))
// 						XSM_DEBUG_ERROR (DBG_L1,  "error: can't set GXSMGRIYLAB enviromentvar!" );
//         		if (setenv("GXSMGRIPLOTTITLE", xsmres.griplottitle, TRUE))
// 						XSM_DEBUG_ERROR (DBG_L1,  "error: can't set GXSMGRIPLOTTITLE enviromentvar!" );
// 				
		if((index >= 0) && (index < GRIMAX)){
			sprintf(command, "%s %s \"%s\"\n",xsmres.gricmd1d[index], tmp, xsmres.griplottitle);
//				sprintf(command,  // xmgrace plots only 1&5nd column!!!
//				"xmgrace  -graph 0 -pexec \"title \\\"%s\\\"\"  -pexec \"xaxis label \\\"%s\\\"\" -pexec \"yaxis label \\\"%s\\\"\" -block %s -bxy %d:%d\n", 
//				tmp, pc->xlabel, pc->ylabel, tmp, 3+scan1d->mem2d->GetNy(), 4+scan1d->mem2d->GetNy());
		}else {
			; // Cannot be reached.
		}
		gricmd=popen(command,"r");
		ret=pclose(gricmd);
		if(ret){
			sprintf(retinfo, "%s", ret ? "Execution Error" : "OK");
			XSM_SHOW_ALERT("PRINT INFO", command, retinfo,1);
		}
// 			else{
// 				gchar *p;
// 				if(strrchr(xsmres.gricmd1d[index], '.') && index < 4){ //do this for gri
// 					*(p = strrchr(xsmres.gricmd1d[index], '.')) = 0;
// 					sprintf(command, "gv %s.ps\n", strrchr(xsmres.gricmd1d[index], '/')+1);
// 					*p = '.';
// 					gricmd=popen(command,"r");
// 			        	pclose(gricmd);
// 				}
//         		}
		// remove tmp file
		sprintf(command, "rm %s\n", tmp);
		gricmd=popen(command,"r");
		pclose(gricmd);
	}
	g_free (tmp);
}

void ProfileControl::file_activate_callback (GSimpleAction *simple, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	if (pc->chno >= 0)
		gapp->xsm->ActivateChannel (pc->chno);
}

void ProfileControl::logy_callback (GSimpleAction *action, GVariant *parameter, 
                                    gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
                

        if (g_variant_get_boolean (new_state))
		pc->mode = (pc->mode & ~PROFILE_MODE_YLOG) | PROFILE_MODE_YLOG;
	else
		pc->mode &= ~PROFILE_MODE_YLOG;

        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        pc->updateFrame ();
	pc->UpdateArea ();
}

void ProfileControl::linreg_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
 
        if (g_variant_get_boolean (new_state))
		pc->mode = (pc->mode & ~PROFILE_MODE_YLINREG) | PROFILE_MODE_YLINREG;
	else
		pc->mode &= ~PROFILE_MODE_YLINREG;

        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        pc->updateFrame ();
	pc->UpdateArea ();
}

void ProfileControl::psd_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
 
        if (g_variant_get_boolean (new_state))
		pc->mode = (pc->mode & ~PROFILE_MODE_YPSD) | PROFILE_MODE_YPSD;
	else
		pc->mode &= ~PROFILE_MODE_YPSD;

        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        pc->updateFrame ();
	pc->SetXlabel ();
	pc->UpdateArea ();
}

void ProfileControl::decimate_callback (GSimpleAction *action, GVariant *parameter, 
                                        gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
 
        if (g_variant_get_boolean (new_state))
		pc->mode = (pc->mode & ~PROFILE_MODE_DECIMATE) | PROFILE_MODE_DECIMATE;
	else
		pc->mode &= ~PROFILE_MODE_DECIMATE;

        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        pc->updateFrame ();
	pc->SetXlabel ();
	pc->UpdateArea ();
}

void ProfileControl::ydiff_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
 
        if (g_variant_get_boolean (new_state))
		pc->mode = (pc->mode & ~PROFILE_MODE_YDIFF) | PROFILE_MODE_YDIFF;
	else
		pc->mode &= ~PROFILE_MODE_YDIFF;

        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        pc->updateFrame ();
	pc->SetYlabel ();
	pc->UpdateArea ();
}

void ProfileControl::ylowpass_cycle_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
 
        if (g_variant_get_boolean (new_state)){
		pc->mode = (pc->mode & ~PROFILE_MODE_YLOWPASS) | PROFILE_MODE_YLOWPASS;
		gchar *p=g_strdup_printf ("LP gain: %g", pc->lp_gain);
		gtk_statusbar_push (GTK_STATUSBAR(pc->statusbar), pc->statusid, p);
		g_free (p);
	} else {
		pc->mode &= ~PROFILE_MODE_YLOWPASS;
		pc->lp_gain *= 0.5;
		if (pc->lp_gain < 0.01)
		        pc->lp_gain = 0.4;
        }
        
        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        pc->updateFrame ();
	pc->SetYlabel ();
	pc->UpdateArea ();
}

void ProfileControl::ylowpass_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_string (g_variant_get_string (parameter, NULL));
                
        XSM_DEBUG_GP (DBG_L1, "LOWPASS Radio action %s activated, state changes from %s to %s\n",
                      g_action_get_name (G_ACTION (action)),
                      g_variant_get_string (old_state, NULL),
                      g_variant_get_string (new_state, NULL));
        
        if (!strcmp (g_variant_get_string (new_state, NULL), "lp-off"))
		pc->mode &= ~PROFILE_MODE_YLOWPASS, pc->lp_gain=1.0;
        else if (!strcmp (g_variant_get_string (new_state, NULL), "lp0-1"))
                pc->mode = (pc->mode & ~PROFILE_MODE_YLOWPASS) | PROFILE_MODE_YLOWPASS, pc->lp_gain = 0.001;
        else if (!strcmp (g_variant_get_string (new_state, NULL), "lp1"))
                pc->mode = (pc->mode & ~PROFILE_MODE_YLOWPASS) | PROFILE_MODE_YLOWPASS, pc->lp_gain = 0.01;
        else if (!strcmp (g_variant_get_string (new_state, NULL), "lp5"))
                pc->mode = (pc->mode & ~PROFILE_MODE_YLOWPASS) | PROFILE_MODE_YLOWPASS, pc->lp_gain = 0.05;
        else if (!strcmp (g_variant_get_string (new_state, NULL), "lp10"))
                pc->mode = (pc->mode & ~PROFILE_MODE_YLOWPASS) | PROFILE_MODE_YLOWPASS, pc->lp_gain = 0.10;
        else if (!strcmp (g_variant_get_string (new_state, NULL), "lp50"))
                pc->mode = (pc->mode & ~PROFILE_MODE_YLOWPASS) | PROFILE_MODE_YLOWPASS, pc->lp_gain = 0.50;
        else if (!strcmp (g_variant_get_string (new_state, NULL), "lp90"))
                pc->mode = (pc->mode & ~PROFILE_MODE_YLOWPASS) | PROFILE_MODE_YLOWPASS, pc->lp_gain = 0.90;
        else pc->mode &= ~PROFILE_MODE_YLOWPASS, pc->lp_gain=1.0;
        
        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        pc->updateFrame ();
	pc->SetYlabel ();
	pc->UpdateArea ();
}

void ProfileControl::yhold_callback (GSimpleAction *action, GVariant *parameter, 
                                     gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
 
        if (g_variant_get_boolean (new_state))
		pc->scaleing = (pc->scaleing & ~PROFILE_SCALE_YHOLD) | PROFILE_SCALE_YHOLD;
	else
		pc->scaleing &= ~PROFILE_SCALE_YHOLD;

        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        pc->updateFrame ();
	pc->UpdateArea ();
}

void ProfileControl::yexpand_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
 
        if (g_variant_get_boolean (new_state))
		pc->scaleing = (pc->scaleing& ~PROFILE_SCALE_YEXPAND) | PROFILE_SCALE_YEXPAND;
	else
		pc->scaleing &= ~PROFILE_SCALE_YEXPAND;

        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        pc->updateFrame ();
	pc->UpdateArea ();
}

void ProfileControl::skl_Yauto_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->scaleing &= ~PROFILE_SCALE_YHOLD;
        pc->updateFrame ();
	pc->scaleing = (pc->scaleing & ~PROFILE_SCALE_YHOLD) | PROFILE_SCALE_YHOLD;
	pc->UpdateArea ();
}

void ProfileControl::skl_Yupperup_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->scaleing = (pc->scaleing & ~PROFILE_SCALE_YHOLD) | PROFILE_SCALE_YHOLD;
	if(pc->mode & PROFILE_MODE_YLOG)
		pc->SetYrange(pc->ymin, pc->ymax*2.);
	else{
		double yrange = pc->ymax - pc->ymin;
		pc->SetYrange(pc->ymin, pc->ymax+0.1*yrange);
	}

        pc->updateFrame ();
	pc->UpdateArea ();
}

void ProfileControl::skl_Yupperdn_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->scaleing = (pc->scaleing & ~PROFILE_SCALE_YHOLD) | PROFILE_SCALE_YHOLD;
	if(pc->mode & PROFILE_MODE_YLOG)
		pc->SetYrange(pc->ymin, pc->ymax/2.);
	else{
		double yrange = pc->ymax - pc->ymin;
		pc->SetYrange(pc->ymin, pc->ymax-0.1*yrange);
	}

        pc->updateFrame ();
	pc->UpdateArea ();
}

void ProfileControl::skl_Ylowerup_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->scaleing = (pc->scaleing & ~PROFILE_SCALE_YHOLD) | PROFILE_SCALE_YHOLD;
	if(pc->mode & PROFILE_MODE_YLOG)
		pc->SetYrange(pc->ymin*2., pc->ymax);
	else{
		double yrange = pc->ymax - pc->ymin;
		pc->SetYrange(pc->ymin+0.1*yrange, pc->ymax);
	}
        pc->updateFrame ();
	pc->UpdateArea ();
}

void ProfileControl::skl_Ylowerdn_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->scaleing = (pc->scaleing & ~PROFILE_SCALE_YHOLD) | PROFILE_SCALE_YHOLD;
	if(pc->mode & PROFILE_MODE_YLOG)
		pc->SetYrange(pc->ymin/2., pc->ymax);
	else{
		double yrange = pc->ymax - pc->ymin;
		pc->SetYrange(pc->ymin-0.1*yrange, pc->ymax);
	}
        pc->updateFrame ();
	pc->UpdateArea ();
}

void ProfileControl::skl_Yzoomin_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->scaleing = (pc->scaleing & ~PROFILE_SCALE_YHOLD) | PROFILE_SCALE_YHOLD;
	double yrange = pc->ymax - pc->ymin;
	if (pc->mode & PROFILE_MODE_YLOG){
		if (yrange > 50.)
			pc->SetYrange (pc->ymin*2., pc->ymax/2.);
	}
	else
		pc->SetYrange (pc->ymin+0.1*yrange, pc->ymax-0.1*yrange);
        pc->updateFrame ();
	pc->UpdateArea ();
}

void ProfileControl::skl_Yzoomout_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->scaleing = (pc->scaleing & ~PROFILE_SCALE_YHOLD) | PROFILE_SCALE_YHOLD;
	if (pc->mode & PROFILE_MODE_YLOG)
		pc->SetYrange (pc->ymin/2., pc->ymax*2.);
	else{
		double yrange = pc->ymax - pc->ymin;
		pc->SetYrange (pc->ymin-0.1*yrange, pc->ymax+0.1*yrange);
	}
        pc->updateFrame ();
	pc->UpdateArea ();
}

void ProfileControl::skl_Yset_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->scaleing = (pc->scaleing & ~PROFILE_SCALE_YHOLD) | PROFILE_SCALE_YHOLD;
	pc->SetYrange (10., 1e6);
	pc->UpdateArea ();
}

void ProfileControl::skl_Xauto_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        pc->updateFrame ();
	pc->UpdateArea();
}

void ProfileControl::skl_Xset_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        pc->updateFrame ();
	pc->UpdateArea ();
}

void ProfileControl::opt_xr_ab_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
 
        if (g_variant_get_boolean (new_state))
                pc->mode = (pc->mode & ~PROFILE_MODE_XR_AB) | PROFILE_MODE_XR_AB;
	else
		pc->mode &= ~PROFILE_MODE_XR_AB;

        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        pc->updateFrame ();
	pc->SetYlabel ();
	pc->UpdateArea ();
}

void ProfileControl::skl_Binary_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
	static int b2=0;
        ProfileControl *pc = (ProfileControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
 
        if (g_variant_get_boolean (new_state)){
		if (b2==0)
			pc->mode = (pc->mode & ~(PROFILE_MODE_BINARY8 | PROFILE_MODE_BINARY16)) | PROFILE_MODE_BINARY8;
		else
			pc->mode = (pc->mode & ~PROFILE_MODE_BINARY8) | (PROFILE_MODE_BINARY8 | PROFILE_MODE_BINARY16);
		++b2; b2 &= 1;
	} else {
		pc->mode &= ~(PROFILE_MODE_BINARY8 | PROFILE_MODE_BINARY16);
	}

        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        pc->updateFrame ();
	pc->SetYlabel ();
	pc->UpdateArea ();
}

void ProfileControl::settings_adjust_mode_callback (GSimpleAction *action, GVariant *parameter, gint64 flg){
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
 
        if (g_variant_get_boolean (new_state))
		mode = (mode & ~flg) | flg;
	else
		mode &= ~flg;

        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        updateFrame ();
	UpdateArea();
}

void ProfileControl::settings_xgrid_callback (GSimpleAction *action, GVariant *parameter, 
                                              gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        pc->settings_adjust_mode_callback (action, parameter, PROFILE_MODE_XGRID);
}

void ProfileControl::settings_ygrid_callback (GSimpleAction *action, GVariant *parameter, 
                                              gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        pc->settings_adjust_mode_callback (action, parameter, PROFILE_MODE_YGRID);
}
void ProfileControl::settings_notics_callback (GSimpleAction *action, GVariant *parameter, 
                                               gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        pc->settings_adjust_mode_callback (action, parameter, PROFILE_MODE_NOTICS);
}
void ProfileControl::settings_header_callback (GSimpleAction *action, GVariant *parameter, 
                                               gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        pc->settings_adjust_mode_callback (action, parameter, PROFILE_MODE_HEADER);
}
void ProfileControl::settings_legend_callback (GSimpleAction *action, GVariant *parameter, 
                                               gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        pc->settings_adjust_mode_callback (action, parameter, PROFILE_MODE_LEGEND);
}
void ProfileControl::settings_series_tics_callback (GSimpleAction *action, GVariant *parameter, 
                                                    gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        pc->settings_adjust_mode_callback (action, parameter, PROFILE_MODE_STICS);
}

void ProfileControl::settings_pathmode_radio_callback (GSimpleAction *action, GVariant *parameter, 
                                                       gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_string (g_variant_get_string (parameter, NULL));
                
        XSM_DEBUG_GP (DBG_L1, "PATHMODE Radio action %s activated, state changes from %s to %s\n",
                      g_action_get_name (G_ACTION (action)),
                      g_variant_get_string (old_state, NULL),
                      g_variant_get_string (new_state, NULL));
        
        // clear all
        pc->mode &= ~(PROFILE_MODE_POINTS|PROFILE_MODE_CONNECT|PROFILE_MODE_SYMBOLS|PROFILE_MODE_IMPULS);

        if (!strcmp (g_variant_get_string (new_state, NULL), "connect"))
                pc->mode |= PROFILE_MODE_CONNECT;
        else if (!strcmp (g_variant_get_string (new_state, NULL), "points"))
                pc->mode |= PROFILE_MODE_POINTS;
        else if (!strcmp (g_variant_get_string (new_state, NULL), "impulses"))
                pc->mode |= PROFILE_MODE_IMPULS;
        else if (!strcmp (g_variant_get_string (new_state, NULL), "symbols"))
                pc->mode |= PROFILE_MODE_SYMBOLS;
        else // fallback
                pc->mode |= PROFILE_MODE_CONNECT;
        
        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        pc->updateFrame ();
	pc->UpdateArea();
}

void ProfileControl::zoom_out_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	if (pc->papixel < 100) return;
	pc->papixel *= 2;
	pc->papixel /= 3;
	pc->SetSize();
	pc->UpdateArea();
}

void ProfileControl::zoom_in_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	if (pc->papixel > 2000) return;
	pc->papixel /= 2;
	pc->papixel *= 3;
	pc->SetSize();
	pc->UpdateArea();
}

void ProfileControl::aspect_inc_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	if (pc->aspect > 5.) return;

	if (pc->aspect > 2.)
		pc->aspect += 1.;
	else if (pc->aspect > 0.5)
		pc->aspect += 0.25;
	else
		pc->aspect += 0.1;

	pc->SetSize();
	pc->UpdateArea();
        pc->updateFrame ();
}

void ProfileControl::aspect_dec_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;

	if (pc->aspect < 0.2) return;

	if (pc->aspect > 2.)
		pc->aspect -= 1.;
	else if (pc->aspect > 0.5)
		pc->aspect -= 0.25;
	else
		pc->aspect -= 0.1;

	pc->SetSize();
	pc->UpdateArea();
}

void ProfileControl::canvas_size_store_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;

	XsmRescourceManager xrm (pc->profile_res_id);
	xrm.Put("XYaspectratio", pc->aspect);
	xrm.Put("Xsize", pc->papixel);
}

void ProfileControl::cur_Ashow_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));

        if (g_variant_get_boolean (new_state)){
		pc->showCur(0,TRUE);
                gtk_widget_show (pc->widget_cb);
	} else {
		pc->showCur(0,FALSE);
                gtk_widget_hide (pc->widget_cb);
                gtk_widget_hide (pc->widget_xr_ab);
        }
        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);
}

void ProfileControl::cur_Bshow_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
 
        if (g_variant_get_boolean (new_state)){
		pc->showCur(1,TRUE);
                gtk_widget_show (pc->widget_xr_ab);
	} else {
		pc->showCur(1,FALSE);
                gtk_widget_hide (pc->widget_xr_ab);
        }
        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);
}

void ProfileControl::cur_Aleft_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->moveCur(0,-1);
}

void ProfileControl::cur_Aright_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->moveCur(0,1);
}

void ProfileControl::cur_Bleft_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->moveCur(1,-1);
}

void ProfileControl::cur_Bright_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->moveCur(1,1);
}

// Min Max search
void ProfileControl::cur_Almax_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->moveCur(0,-1,1);
}
void ProfileControl::cur_Almin_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->moveCur(0,-1,-1);
}
void ProfileControl::cur_Armax_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->moveCur(0,1,1);
}
void ProfileControl::cur_Armin_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->moveCur(0,1,-1);
}

void ProfileControl::cur_Blmax_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->moveCur(1,-1,1);
}
void ProfileControl::cur_Blmin_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->moveCur(1,-1,-1);
}
void ProfileControl::cur_Brmax_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->moveCur(1,1,1);
}
void ProfileControl::cur_Brmin_callback (GSimpleAction *action, GVariant *parameter, 
                                 gpointer user_data){
        ProfileControl *pc = (ProfileControl *) user_data;
	pc->moveCur(1,1,-1);
}

void ProfileControl::UpdateCursorInfo (double x1, double x2, double z1, double z2){
	if (gapp->xsm->GetActiveScan ()){
		if (strncmp (xlabel, "Z", 1) == 0){
			double dz = gapp->xsm->GetActiveScan()->data.s.dz;
			gapp->xsm->GetActiveScan()->mem2d->SetZHiLitRange (x1/dz, x2/dz);
		} else
			gapp->xsm->GetActiveScan()->mem2d->SetZHiLitRange (z1, z2);
	}
}

double ProfileControl::getCursorPos(int id) { 
	double ret = -1.;
	if (id>=0 && id <2 && last_pe)
		if (Cursor[id][0])
			ret = last_pe->GetXPos(CursorsIdx[id]);
	return ret; 
}
