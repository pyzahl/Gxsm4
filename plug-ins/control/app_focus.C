/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Percy Zahl <zahl@fkp.uni-hannover.de>
 * additional features: Andreas Klust <klust@fkp.uni-hannover.de>
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

/* ignore tag evaluated by for docuscangxsmplugins.pl -- this is no main plugin file
% PlugInModuleIgnore
*/

#include <gtk/gtk.h>
#include "core-source/glbvars.h"
#include "core-source/util.h"
#include "core-source/pcs.h"
#include "core-source/unit.h"

#include <math.h>

#include "app_peakfind.h"

#include <gtkdatabox.h>
#include "gtkled.h"
#include "gtkledbar.h"

// using old gdk_pixbuf ?? than use:
//#define GDK_PIXBUF_OLD

#define DATAAQREP      200

#define DEFAULTLEN     5
#define DEFAULTGATE    0.2
#define DEFAULTNXY     40
#define MAXNXY         42

#define FOCUSICONSIZE  200
#define FOCUSSCROLLSZ  220

//template <class FPIXTYP> 
//Focus<FPIXTYP>::Focus 
Focus::Focus 
( DSPPeakFindControl *Pfc, SPA_PeakFind_p *Pfp, GtkWidget *Vbox, GtkWidget *Hbox
  ) : DataBox(Vbox){
  double xyl[4];
  GnomeCanvasPoints pnl = { xyl, 2, 0 };
  Gtk_EntryControl *ec;  
  GtkWidget *hbox, *entry, *sw;

  PI_DEBUG (DBG_L2, "Focus::Focus" );

  pfp = Pfp;
  pfc = Pfc;

  cps      = 0.;
  cpshi    = 1e6;
  cpslo    = 0.;

  nnx=nnx2d=0;
  yn=xn=xx=yy=NULL;

  delay = 300.;

  xymode    = TRUE;
  autoskl   = FALSE;
  invers    = FALSE;
  linlog    = SKL_LOG;
  mode      = 0;

  xbox = this;
  ybox = new DataBox(Vbox);

  // ========================================
  // RateMeter Frame, note: uses gtkled, gtkledbar from vumeter of gnome-media package
  GtkWidget *rmframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (rmframe), GTK_SHADOW_IN);
  gtk_container_border_width (GTK_CONTAINER (rmframe), 4);
  gtk_container_add (GTK_CONTAINER (Hbox), rmframe);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);
  gtk_container_add (GTK_CONTAINER (rmframe), hbox);
  
  rateleds    = 45;
  ratedecades = 6;
  firstdecade = 0;
  RateMeter[0] = NULL;
  gtk_widget_show (hbox);
  gtk_widget_show (rmframe);
  g_object_set_data (G_OBJECT (vbox), "RateMeterBox", hbox);
  ratemeter_changed();

  // ========================================

  GtkWidget *vb1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (Hbox), vb1, FALSE, FALSE, GXSM_WIDGET_PAD);
  gtk_widget_show (vb1);

  // ========================================
  // Focus-Icon Box
  GtkWidget* vb2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (vb1), vb2, TRUE, TRUE, GXSM_WIDGET_PAD);
  gtk_widget_show (vb2);

  // ========================================
  // HiLo Setiings, ... Box
  GtkWidget *vvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (vb1), vvbox, TRUE, FALSE, GXSM_WIDGET_PAD);
  gtk_widget_show (vvbox);

  // ========================================

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_show(sw);
  gtk_widget_set_size_request(sw, FOCUSSCROLLSZ, FOCUSSCROLLSZ );
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);

  gtk_widget_push_visual(gdk_rgb_get_visual());
  gtk_widget_push_colormap(gdk_rgb_get_cmap());
  canvas = gnome_canvas_new_aa();
  gtk_widget_pop_colormap();
  gtk_widget_pop_visual();
  /*
  gtk_widget_push_visual(gdk_imlib_get_visual());
  gtk_widget_push_colormap(gdk_imlib_get_colormap());
  canvas = gnome_canvas_new();
  gtk_widget_pop_visual();
  gtk_widget_pop_colormap();
  */

  gtk_container_add(GTK_CONTAINER(sw), canvas);
  gtk_box_pack_start (GTK_BOX (vb2), sw, FALSE, FALSE, 0);
  gtk_widget_show(canvas);

  focuspixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, pfp->npkte2d, pfp->npkte2d);
  bpp         = gdk_pixbuf_get_bits_per_sample ( focuspixbuf ) 
    * gdk_pixbuf_get_n_channels ( focuspixbuf ) / 8;
  rowstride   = gdk_pixbuf_get_rowstride ( focuspixbuf );
  pixels      = gdk_pixbuf_get_pixels ( focuspixbuf );

  memset(pixels, 0, pfp->npkte2d*rowstride*sizeof(guchar));

  focuspixbufitem = gnome_canvas_item_new ( gnome_canvas_root ( GNOME_CANVAS (canvas) ),
					    gnome_canvas_pixbuf_get_type(),
					    "pixbuf", focuspixbuf,
					    "width",(double)FOCUSICONSIZE,
					    "width_set",TRUE,
					    "height",(double)FOCUSICONSIZE,
					    "height_set",TRUE,
					    NULL);

  g_signal_connect(G_OBJECT(focuspixbufitem), "event",
		     (GtkSignalFunc) item_event,
		     this);

  xyl[0]=0.; xyl[2]=FOCUSICONSIZE;
  xyl[1]=xyl[3]=FOCUSICONSIZE/2;
  hline = gnome_canvas_item_new(gnome_canvas_root( GNOME_CANVAS(canvas)),
				gnome_canvas_line_get_type(),
				"points",&pnl,
				"fill_color","red",
				"width_pixels",1,
				NULL);
  xyl[0]=xyl[2]=FOCUSICONSIZE/2;
  xyl[1]=0.; xyl[3]=FOCUSICONSIZE;
  vline = gnome_canvas_item_new(gnome_canvas_root( GNOME_CANVAS(canvas)),
				gnome_canvas_line_get_type(),
				"points",&pnl,
				"fill_color","red",
				"width_pixels",1,
				NULL);

  gnome_canvas_set_scroll_region(GNOME_CANVAS (canvas), 0, 0, FOCUSICONSIZE, FOCUSICONSIZE);
  gnome_canvas_set_pixels_per_unit(GNOME_CANVAS (canvas), 1.);

  // ========================================

  g_object_set_data (G_OBJECT (vbox), "cps00",
		       entry=gapp->mygtk_create_input("Cps00", vvbox, hbox));
  ec = new Gtk_EntryControl(pfc->CPSUnit, "invalid range", &cps, 0, 1e7, ".0f", entry);
  g_object_set_data (G_OBJECT (vbox), "eccps00", ec);
  ec->Set_ChangeNoticeFkt(on_cps_changed, this);

  GtkObject *adj     = gtk_adjustment_new (6, 2, 6, 1, 1, 1);
  GtkWidget *spinb = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 1, 0);
  gtk_widget_show (spinb);
  gtk_box_pack_start (GTK_BOX (hbox), spinb, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (spinb), "changed",
                      G_CALLBACK (on_rdecads_changed),
                      this);
  adj     = gtk_adjustment_new (0, 0, 4, 1, 1, 1);
  spinb = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 1, 0);
  gtk_widget_show (spinb);
  gtk_box_pack_start (GTK_BOX (hbox), spinb, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (spinb), "changed",
                      G_CALLBACK (on_rfirstdecad_changed),
                      this);



  g_object_set_data (G_OBJECT (vbox), "cpshi",
		       entry=gapp->mygtk_create_input("Cps hi", vvbox, hbox));
  ec = new Gtk_EntryControl(pfc->CPSUnit, "invalid range", &cpshi, 1, 1e7, ".0f", entry);
  g_object_set_data (G_OBJECT (vbox), "eccpshi", ec);

  g_object_set_data (G_OBJECT (vbox), "cpslow",
		       entry=gapp->mygtk_create_input("Cps low", vvbox, hbox));
  ec = new Gtk_EntryControl(pfc->CPSUnit, "invalid range", &cpslo, 0, 1e7, ".0f", entry);
  g_object_set_data (G_OBJECT (vbox), "eccpslo", ec);


  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vvbox), hbox, FALSE, FALSE, GXSM_WIDGET_PAD);
  gtk_widget_show (hbox);  

  GtkWidget *SetAuto = gtk_check_button_new_with_label ("AutoSkl");
  gtk_widget_show (SetAuto);
  gtk_box_pack_start (GTK_BOX (hbox), SetAuto, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (SetAuto), "clicked",
                      G_CALLBACK (on_SetAuto_clicked),
                      this);

  GtkWidget *LinLog = gtk_check_button_new_with_label ("Lin.");
  gtk_widget_show (LinLog);
  gtk_box_pack_start (GTK_BOX (hbox), LinLog, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (LinLog), "clicked",
                      G_CALLBACK (on_LinLog_clicked),
                      this);

  GtkWidget *SetInvers = gtk_check_button_new_with_label ("Inv.");
  gtk_widget_show (SetInvers);
  gtk_box_pack_start (GTK_BOX (hbox), SetInvers, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (SetInvers), "clicked",
                      G_CALLBACK (on_Invers_clicked),
                      this);

  GtkWidget *Set = gtk_check_button_new_with_label ("0D");
  gtk_widget_show (Set);
  gtk_box_pack_start (GTK_BOX (hbox), Set, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (Set), "clicked",
                      G_CALLBACK (on_0d_clicked),
                      this);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (Set), mode & FOCUS_MODE_0D);

  Set = gtk_check_button_new_with_label ("1D");
  gtk_widget_show (Set);
  gtk_box_pack_start (GTK_BOX (hbox), Set, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (Set), "clicked",
                      G_CALLBACK (on_1d_clicked),
                      this);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (Set), mode & FOCUS_MODE_1D);

  Set = gtk_check_button_new_with_label ("2D");
  gtk_widget_show (Set);
  gtk_box_pack_start (GTK_BOX (hbox), Set, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (Set), "clicked",
                      G_CALLBACK (on_2d_clicked),
                      this);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (Set), mode & FOCUS_MODE_2D);

  /*
  GtkWidget *EXYprint = gtk_button_new_with_label ("Print");
  gtk_widget_show (EXYprint);
  gtk_box_pack_start (GTK_BOX (hbox), EXYprint, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (EXYprint), "clicked",
                      G_CALLBACK (on_EXYprint_clicked),
                      this);
  */

  // ========================================

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, GXSM_WIDGET_PAD);
  gtk_widget_show (hbox);  

  // ========================================

  PI_DEBUG (DBG_L2, "Focus::Focus init done." );
}

//template <class FPIXTYP> 
Focus::~Focus () {
    PI_DEBUG (DBG_L2, "Focus::~Focus" );

    stop();

    delete ybox;

    if(yn) g_free(yn);
    if(xn) g_free(xn);
    if(xx) g_free(xx);
    if(yy) g_free(yy);
}

void Focus::on_rdecads_changed (GtkEditable *spin, Focus *foc){
    gint n = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
    if( n != foc->ratedecades ){
	foc->ratedecades = n;
	foc->ratemeter_changed();
    }
}

void Focus::on_rfirstdecad_changed (GtkEditable *spin, Focus *foc){
    gint n = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
    if( n != foc->firstdecade ){
	foc->firstdecade = n;
	foc->ratemeter_changed();
    }
}

//template <class FPIXTYP>
void Focus::on_SetAuto_clicked (GtkButton *button, Focus *foc){
  if (GTK_TOGGLE_BUTTON (button)->active)
    foc->autoskl=TRUE;
  else
    foc->autoskl=FALSE;
}

void Focus::on_LinLog_clicked (GtkButton *button, Focus *foc){
  if (GTK_TOGGLE_BUTTON (button)->active)
    foc->linlog=SKL_LIN;
  else
    foc->linlog=SKL_LOG;
}

void Focus::on_Invers_clicked (GtkButton *button, Focus *foc){
  if (GTK_TOGGLE_BUTTON (button)->active)
    foc->invers=TRUE;
  else
    foc->invers=FALSE;
}

void Focus::ratemeter_changed(){
    GtkWidget *OldMeter = RateMeter[0];
    RateMeter[0]=NULL;

    if(OldMeter)
	gtk_widget_hide(OldMeter);

    leds_decade = (double)rateleds/(double)ratedecades;
    RateMeter[0] = led_bar_new_with_decades (rateleds, ratedecades, firstdecade, TRUE);
    l10minrate = firstdecade;
    l10maxrate = ratedecades + firstdecade + leds_decade-floor(leds_decade);
    gtk_container_add (GTK_CONTAINER( (GtkWidget*) g_object_get_data 
				      (G_OBJECT (vbox), "RateMeterBox")),
			RateMeter[0] );
    gtk_widget_show (RateMeter[0]);


    if(OldMeter){
	while(gtk_events_pending()) gtk_main_iteration();
	gtk_widget_destroy(OldMeter);
    }
}

void Focus::on_cps_changed(Param_Control* pcs, gpointer data){
    Focus *foc = (Focus*)data;
    if(!foc->RateMeter[0]) return;
    if(foc->cps < 1.)
	led_bar_light_percent( foc->RateMeter[0], 0. );
    else
      if( foc->linlog == SKL_LOG )
	led_bar_light_percent( foc->RateMeter[0], 
	      1. - (1.-(log(foc->cps)/log(10.)/foc->l10maxrate))
	      * foc->l10maxrate / (foc->l10maxrate - foc->l10minrate)
	      + 0.5/foc->leds_decade
			       );
      else
	led_bar_light_percent( foc->RateMeter[0], 
			       (foc->cps-foc->cpslo)/(foc->cpshi-foc->cpslo)
			       );
}

void Focus::on_0d_clicked (GtkButton *button, Focus *foc){
    foc->setmode (FOCUS_MODE_0D, GTK_TOGGLE_BUTTON (button)->active);
}

void Focus::on_1d_clicked (GtkButton *button, Focus *foc){
    foc->setmode (FOCUS_MODE_1D, GTK_TOGGLE_BUTTON (button)->active);
}

void Focus::on_2d_clicked (GtkButton *button, Focus *foc){
    foc->setmode (FOCUS_MODE_2D, GTK_TOGGLE_BUTTON (button)->active);
}

//template <class FPIXTYP>
void Focus::on_EXYprint_clicked (GtkButton *button, Focus *foc){
    //  double S;
    //  S = res.SampleLayerDist * cos(res.ThetaChGunInt * M_PI/180.) *  sqrt(foc->energy/37.6);
    //  PI_DEBUG (DBG_L2, S
    //       << " " << foc->energy
    //       << " " << foc->x0
    //       << " " << foc->y0
    //       << " " << foc->cpshi
    //       );
}

//template <class FPIXTYP>
gint Focus::item_event(GnomeCanvasItem *item, GdkEvent *event, Focus *foc){
  double item_x, item_y;
  item_x = event->button.x;
  item_y = event->button.y;
  gnome_canvas_item_w2i(item->parent, &item_x, &item_y);

  switch (event->type) 
    {
    case GDK_BUTTON_PRESS:
      switch(event->button.button) 
        {
        case 1:
	  foc->pfp->xorg += (item_x/FOCUSICONSIZE-0.5)*2.*foc->pfp->radius;
	  foc->pfp->yorg -= (item_y/FOCUSICONSIZE-0.5)*2.*foc->pfp->radius;
	  foc->pfc->update();
	  break;
	default: 
	  break;
        }
      break;
    default:
      break;
    }
  return FALSE;
}

//template <class FPIXTYP>
gint Focus::atrun(){
    return 1;
}

//template <class FPIXTYP>
void Focus::getdata(){
#define GETFPIX(X,Y) (*(pfp->buffer + (Y)*n + (X)))
  int n=0;
  int n2=0;
  GtkDataboxValue XYmin, XYmax;
  int i, j;

  if(mode & FOCUS_MODE_0D){
      cps = (double)pfc->pfscan->PFget0d(pfc->hard, pfp)/pfp->gate;
      on_cps_changed(NULL, this);
      ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (vbox), "eccps00") ) -> Put_Value();
      if(autoskl && mode == FOCUS_MODE_0D){
	  cpshi=cps; 
	  cnthi=cpshi*pfp->gate;
	  ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (vbox), "eccpshi") ) -> Put_Value();
      }
  }
  if(mode & FOCUS_MODE_1D){
      n  = pfp->npkte;
      n2 = pfp->npkte/2;
      pfc->pfscan->PFhwrun(pfc->hard, pfp);
      pfc->update();
      
      if(autoskl){
	  pfp->xyscans->HiLo(&cnthi, &cntlo);
	  cpshi=cnthi/pfp->gate; 
	  cpslo=0.;
	  cntlo=cpslo*pfp->gate;
	  ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (vbox), "eccpshi") ) -> Put_Value();
	  ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (vbox), "eccpslo") ) -> Put_Value();
      }else{
	  cnthi=cpshi*pfp->gate;
	  cntlo=cpslo*pfp->gate;
      }
      CalcLookUp(linlog);

      if( nnx != n ){
	  nnx = n;
	  if(xn) g_free(xn);
	  if(xx) g_free(xx);
	  xn = g_new ( PLOTTYP, n );
	  xx = g_new ( PLOTTYP, n );    
	  for(i=0; i<n; i++){ xn[i]=i-n2; xx[i]=LookUp( (long)pfp->xyscans->GetDataPkt(i,0)); }
	  xbox -> setxyData(xn, xx, n, FALSE);
	  
	  if(yn) g_free(yn);
	  if(yy) g_free(yy);
	  yn = g_new ( PLOTTYP, n );
	  yy = g_new ( PLOTTYP, n );
	  for(i=0; i<n; i++){ yn[i]=i-n2; yy[i]=LookUp( (long)pfp->xyscans->GetDataPkt(i,1)); }
	  ybox -> setxyData(yn, yy, n, FALSE);
      }else{
	  for(i=0; i<n; i++){ xn[i]=i-n2; xx[i]=LookUp( (long)pfp->xyscans->GetDataPkt(i,0)); }
	  for(i=0; i<n; i++){ yn[i]=i-n2; yy[i]=LookUp( (long)pfp->xyscans->GetDataPkt(i,1)); }
      }

      XYmin.x = -n2; XYmax.x = n2; 
      XYmin.y = 0; XYmax.y = FCOLMAX; 
      xbox -> draw(FALSE, &XYmin, &XYmax);
      XYmin.x = -n2; XYmax.x = n2; 
      XYmin.y = 0; XYmax.y = FCOLMAX; 
      ybox -> draw(FALSE, &XYmin, &XYmax);
  }
  if(mode & FOCUS_MODE_2D){
      n  = pfp->npkte2d;
      n2 = pfp->npkte2d/2;
      pfc->pfscan->PFget2d(pfc->hard, pfp);
      if(autoskl){
	AutoHiLo();
	cpslo=0.;
	cntlo=cpslo*pfp->gate2d;
	((Gtk_EntryControl*) g_object_get_data (G_OBJECT (vbox), "eccpshi") ) -> Put_Value();
	((Gtk_EntryControl*) g_object_get_data (G_OBJECT (vbox), "eccpslo") ) -> Put_Value();
      }else{
	cnthi=cpshi*pfp->gate2d; 
	cntlo=cpslo*pfp->gate2d;
      }
      CalcLookUp(linlog);

      GdkPixbuf *oldpixbuf = focuspixbuf;
      
      if(!(mode & FOCUS_MODE_1D)){ // use same data for 1D if not 1D explicit requested
	  if( nnx != n ){
	      nnx = n;
	      if(xn) g_free(xn);
	      if(xx) g_free(xx);
	      xn = g_new ( PLOTTYP, n );
	      xx = g_new ( PLOTTYP, n );
	      for(i=0; i<n; i++){ xn[i]=i-n2; xx[i]=LookUp((GETFPIX(i,n2-1)+GETFPIX(i,n2)+GETFPIX(i,n2+1))/3); }
	      xbox -> setxyData(xn, xx, n, FALSE);
	      
	      if(yn) g_free(yn);
	      if(yy) g_free(yy);
	      yn = g_new ( PLOTTYP, n );
	      yy = g_new ( PLOTTYP, n );
	      for(i=0; i<n; i++){ yn[i]=i-n2; yy[i]=LookUp((GETFPIX(n2-1,i)+GETFPIX(n2,i)+GETFPIX(n2+1,i))/3); }
	      ybox -> setxyData(yn, yy, n, FALSE);
	  }else{
	      for(i=0; i<n; i++){ xn[i]=i-n2; xx[i]=LookUp((GETFPIX(i,n2-1)+GETFPIX(i,n2)+GETFPIX(i,n2+1))/3); }
	      for(i=0; i<n; i++){ yn[i]=i-n2; yy[i]=LookUp((GETFPIX(n2-1,i)+GETFPIX(n2,i)+GETFPIX(n2+1,i))/3); }
	  }
	  XYmin.x = -n2; XYmax.x = n2; 
	  XYmin.y = 0; XYmax.y = FCOLMAX; 
	  xbox -> draw(FALSE, &XYmin, &XYmax);
	  XYmin.x = -n2; XYmax.x = n2; 
	  XYmin.y = 0; XYmax.y = FCOLMAX; 
	  ybox -> draw(FALSE, &XYmin, &XYmax);
      }
      if( nnx2d != n ){
	nnx2d = n;
	focuspixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, n, n);
	bpp         = gdk_pixbuf_get_bits_per_sample ( focuspixbuf ) 
	  * gdk_pixbuf_get_n_channels ( focuspixbuf ) / 8;
	rowstride   = gdk_pixbuf_get_rowstride ( focuspixbuf );
	pixels      = gdk_pixbuf_get_pixels ( focuspixbuf );
      }

      guchar *p, v;
      for(i=0; i<n; i++)
	  for(j=0, p=pixels+i*rowstride; j<n; j++){
	      v = invers ? 
		  FCOLMAX-1-LookUp(GETFPIX(j,i)) : LookUp(GETFPIX(j,i));
	      *p++ = v;
	      *p++ = v;
	      *p++ = v;
	  }

      gnome_canvas_item_set (focuspixbufitem,
			     "pixbuf", focuspixbuf,
			     NULL);
      
      if ( oldpixbuf != focuspixbuf )
	gdk_pixbuf_unref(oldpixbuf);
  }
}

//template <class FPIXTYP>
void Focus::AutoHiLo(){
    FPIXTYP Hi, Lo;
    FPIXTYP *cnt, *cntend;
    FPIXTYP *cnt_buffer = pfp->buffer;
    Hi=Lo=*cnt_buffer;
    for(cnt = cnt_buffer, cntend = cnt_buffer+pfp->npkte2d*pfp->npkte2d; 
	cnt < cntend; ++cnt){
	if(Hi < *cnt) Hi = *cnt;
	if(Lo > *cnt) Lo = *cnt;
    }
    if(Hi < 1){ Lo = 0; Hi = 1; } // Div Zero prevent....
    cnthi=Hi; cntlo=Lo;
    cpshi=cnthi/pfp->gate2d; 
    cpslo=cntlo/pfp->gate2d;
}

//template <class FPIXTYP>
void Focus::CalcLookUp(int SklMode){
  double fac, logi;
  int i;
  double range = cnthi - cntlo;
  switch(SklMode){
  case SKL_LIN:
    fac = (double)range/(FCOLMAX-1);
    for(i=0; i<FCOLMAX; i++){
      LookupTbl[i] = (CNT)(i*fac + cpslo);
    }
    break;
  case SKL_LOG:
    logi = log(1./(double)FCOLMAX);
    fac  = (double)range/logi;
    for(i=0; i<FCOLMAX; i++){
      logi = log((double)(FCOLMAX-i)/FCOLMAX);
      LookupTbl[i] = (CNT)(fac*logi + cpslo);
    }
    break;
  case SKL_SQRT:
    logi=sqrt(FCOLMAX);
    fac = (double)range/logi;
    for(i=0; i<FCOLMAX; i++){
      logi = sqrt((double)(i+1));
      LookupTbl[i] = (CNT)(fac*logi + cpslo);
    }
    break;
  }
}
