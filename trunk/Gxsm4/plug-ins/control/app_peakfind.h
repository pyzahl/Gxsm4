/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: DSPProbe.C
 * ========================================
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

#ifndef APP_PEAKFIND__H
#define APP_PEAKFIND__H

#include "peakfind_scan.h"
#include "app_databox.h"
#include "gxsm4/remote.h"

enum SKL_MODE { SKL_LIN, SKL_LOG, SKL_SQRT };
#define FCOLMAX 256

class PeakFindScan;

class DSPPeakFindControl : public AppBase{
public:
  DSPPeakFindControl(XSM_Hardware *Hard, GSList **RemoteEntryList, int InWindow=TRUE);
  virtual ~DSPPeakFindControl();

  void update();

  // Probe
  static void ExecCmd(int cmd);
  static void ChangedNotify(Param_Control* pcs, gpointer data);
  static void CmdAction(GtkWidget *widget, gpointer pc);

  // PF Probe control
  static void delete_pfp_cb(SPA_PeakFind_p *pfp, gpointer pc) { delete pfp; };
  static void PFenable(GtkWidget *widget, gpointer pc);
  static void PFremove(GtkWidget *widget, gpointer pc);
  static void PFadd(GtkWidget *widget, gpointer pc);
  static void PFExpandView(GtkWidget *widget, gpointer pc);
  static void PFcpyorg(GtkWidget *widget, gpointer pc);
  static void PFcpyFromM(GtkWidget *widget, gpointer pc);
  static void PFcpyEfromM(GtkWidget *widget, gpointer pc);
  static void PFcpyToM(GtkWidget *widget, gpointer pc);
  static void PFfollow(GtkButton *button, gpointer pc);
  static void PFrunI0pfp(SPA_PeakFind_p *pfp, gpointer pc);
  static void PFreset(SPA_PeakFind_p *pfp, gpointer pc);

  static void PFsetmode(GtkWidget *widget, SPA_PeakFind_p *pfp);
  static void on_mX_clicked(GtkWidget *button, SPA_PeakFind_p *pfp);
  static void on_mY_clicked(GtkWidget *button, SPA_PeakFind_p *pfp);

  static gint PFtmoutfkt(DSPPeakFindControl *pc);

  void addPFtmout(){
    if(!IdPFtmout)
      IdPFtmout = gtk_timeout_add(PFtmoutms, (GtkFunction) PFtmoutfkt, this);
  };
  void rmPFtmout(){
    if(IdPFtmout){
      gtk_timeout_remove(IdPFtmout);
      IdPFtmout=0;
    }
  };

  void addPFfolder();
  void addPFcontrol(GtkWidget *box);
  int  PFtmoutms;

  UnitObj *Unity, *Volt;
  UnitObj *TimeUnitms, *TimeUnit, *CPSUnit, *EnergyUnit;
  XSM_Hardware *hard;
  PeakFindScan *pfscan;

private:
  GtkWidget *notebook;
  int itab;
  gint IdPFtmout;

  GSList *PfpList;

};

#define FPIXTYP long

#define FOCUS_MODE_0D  1
#define FOCUS_MODE_1D  2
#define FOCUS_MODE_2D  4

//template <class FPIXTYP> class Focus : public DataBox {
class Focus : public DataBox {
public:
  Focus( DSPPeakFindControl *Pfc, SPA_PeakFind_p *Pfp, 
	 GtkWidget *Vbox, GtkWidget *Hbox);
  virtual ~Focus();

  static void on_rdecads_changed (GtkEditable *spin, Focus *foc);
  static void on_rfirstdecad_changed (GtkEditable *spin, Focus *foc);
  static void on_SetAuto_clicked (GtkButton *button, Focus *foc);
  static void on_LinLog_clicked (GtkButton *button, Focus *foc);
  static void on_Invers_clicked (GtkButton *button, Focus *foc);
  static void on_cps_changed(Param_Control* pcs, gpointer data);
  static void on_0d_clicked (GtkButton *button, Focus *foc);
  static void on_1d_clicked (GtkButton *button, Focus *foc);
  static void on_2d_clicked (GtkButton *button, Focus *foc);
  static void on_EXYprint_clicked (GtkButton *button, Focus *foc);
  static gint item_event(GnomeCanvasItem *item, GdkEvent *event, Focus *foc);
  virtual void addheader(ofstream &out){
      out << "# gatetime [ms]: " << pfp->gate2d << endl;
      out << "# energy [eV]: " << pfp->energy << endl;
      out << "# len [V]: " << (pfp->radius*2.) << endl;
  }

  void ratemeter_changed();

  void setmode(int m, int state){
      if (state)
	  mode = (mode & ~m) | m;
      else
	  mode = (mode & ~m);

      if(mode)
	  run();
      else
	  stop();
  };
  gint atrun();
  void getdata();

  int    xymode, autoskl, mode;
  double  cnthi, cntlo, cpshi, cpslo, cps;

  GtkWidget* RateMeter[5];
  int   rateleds, ratedecades, firstdecade;
  double l10maxrate, l10minrate, ledstep, leds_decade;

  GtkWidget* canvas;
  GdkPixbuf* focuspixbuf;
  GnomeCanvasItem *focuspixbufitem;
  GnomeCanvasItem *hline, *vline;
  gint bpp, rowstride;
  guchar *pixels;

  DataBox *xbox, *ybox;

 private:
  void AutoHiLo();
  void CalcLookUp(int SklMode = SKL_LOG);

  int LookUp(FPIXTYP x){
    int i,j;
    j=(i=FCOLMAX>>1)>>1;
    do{
      if(x < LookupTbl[i])
        i -= j;
      else
        i += j;
    }while(j>>=1);
    return i;
  };
  
  int nnx, nnx2d;
  float *yn, *xn, *xx, *yy;

  FPIXTYP LookupTbl[FCOLMAX];
  int invers, linlog;

  SPA_PeakFind_p *pfp;
  DSPPeakFindControl *pfc;
};

#endif
