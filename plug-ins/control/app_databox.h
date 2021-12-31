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

#ifndef APP_DATABOX__H
#define APP_DATABOX__H

#include <gtkdatabox.h>
#include <iostream.h>
#include <iomanip.h>

#define NUMCHANNELS 8

typedef float PLOTTYP;

class DataBox{
public:
  DataBox(gchar *title, gint ux=0, gint uy=0);
  DataBox(GtkWidget *vbox);
  virtual ~DataBox();

  void SetHeading(gchar *title, gchar *tx, gchar *ty);
  void Init();

  void setTyp( GtkDataboxDataType typ){ BoxType=typ; };
  void setWidth( gint w){ BoxWidth=w; };
  void removeData ();
  void setSetType ( gint idx, GtkDataboxDataType typ=(GtkDataboxDataType)(-1), gint dotsize=-1);
  void setSetColor ( gint idx, GdkColor color );
  void setxyData    (PLOTTYP *SetX, PLOTTYP *SetY, int n, int cpy=TRUE);
  void setxyData    (PLOTTYP *SetX, PLOTTYP *SetY[NUMCHANNELS], int n, int cpy=TRUE);
  void updatexyData (PLOTTYP *SetX, PLOTTYP *SetY);
  void updatexyData (PLOTTYP *SetX, PLOTTYP *SetY[NUMCHANNELS]);
  void GetVisible( GtkDataboxValue *vmin, GtkDataboxValue *vmax);
  void print(gchar *name);
  void load(gchar *name);
  virtual void checkheader(gchar *line){};

  void save(gchar *name);

  virtual void addheader(std::ofstream &out){};
  virtual int Xprec(){ return 6; };
  virtual int Yprec(int n){ return 6; };

  void cut();
  void copy();
  void paste();

  void rerun(){ if(CbId){ stop(); run(); } };
  void run();
  void stop();
  virtual gint atrun(){ return FALSE; };
  virtual void atstop(){};
  void draw(int autoskl=FALSE, GtkDataboxValue *vmin=NULL, GtkDataboxValue *vmax=NULL);
  virtual void getdata(){};

  static gint loop(DataBox *db);
  
  GtkWidget *vbox;
  double delay;

  GtkWidget *Screen;

private:
  void make_copy();

  GtkWidget *hbox_t;
  GtkWidget *label_t;
  GtkWidget *label_x;
  GtkWidget *label_y;
  GtkWidget *Window;
  GtkDataboxDataType BoxType;
  gint BoxWidth;
  gint CbId, LoopActive;
  gint CpyMode;

  PLOTTYP *Xpoints, *Ypoints[NUMCHANNELS];
  int nPoints;

  PLOTTYP *CXpoints, *CYpoints[NUMCHANNELS];
  int CnPoints;

};

#endif
