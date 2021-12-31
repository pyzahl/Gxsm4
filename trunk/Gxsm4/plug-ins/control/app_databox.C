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

#include "core-source/unit.h"
#include "core-source/pcs.h"
#include "app_databox.h"

#define DEFAULT_BOXWIDTH 2

// ============================================================
// class DataBox 

DataBox::DataBox(gchar *title, gint ux, gint uy){
  cout << "*** DataBox::DataBox Txy *** : Init" << endl;

  Init();

  Window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  if(ux>0)
    gtk_widget_set_size_request (Window, ux, uy);  
  gtk_widget_show (Window);  

  g_signal_connect (G_OBJECT (Window), "destroy",
		      G_CALLBACK(gtk_widget_destroyed),
		      &Window);

  gtk_window_set_title (GTK_WINDOW (Window), title);
  gtk_container_border_width (GTK_CONTAINER (Window), 0);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (Window), vbox);
  gtk_widget_show (vbox);  

  hbox_t = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show (hbox_t);  
  gtk_box_pack_start (GTK_BOX (vbox), hbox_t, FALSE, FALSE, 0);

  Screen = gtk_databox_new();
  g_signal_connect (G_OBJECT (Screen), "destroy",
		      G_CALLBACK(gtk_databox_data_destroy_all),
		      NULL);
  gtk_box_pack_start (GTK_BOX (vbox), Screen, TRUE, TRUE, 0);
  gtk_widget_show(Screen);
}

DataBox::DataBox(GtkWidget *Vbox){
  cout << "*** DataBox::DataBox Vb *** : Init" << endl;

  Init();

  vbox = Vbox;

  hbox_t = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show (hbox_t);  
  gtk_box_pack_start (GTK_BOX (vbox), hbox_t, FALSE, FALSE, 0);

  Screen = gtk_databox_new();
  g_signal_connect (G_OBJECT (Screen), "destroy",
		      G_CALLBACK(gtk_databox_data_destroy_all),
		      NULL);
  gtk_box_pack_start (GTK_BOX (vbox), Screen, TRUE, TRUE, 0);
  gtk_widget_show(Screen);
}

void DataBox::Init(){
  CpyMode=FALSE;
  CbId=0;
  delay=200.;
  LoopActive=FALSE;
  nPoints=0;
  CXpoints=Xpoints=NULL; 
  for(int i=0; i<NUMCHANNELS; ++i) CYpoints[i]=Ypoints[i]=NULL;
  BoxType=GTK_DATABOX_LINES;
  BoxWidth=DEFAULT_BOXWIDTH;
  Window=NULL; Screen=NULL;
  label_t = label_x = label_y = NULL;
}

DataBox::~DataBox(){
  stop();
  removeData ();
  if(Window)
    gtk_widget_destroy(Window);
}

void DataBox::SetHeading(gchar *title, gchar *tx, gchar *ty){
  if(label_t && title)
    gtk_label_set_text( GTK_LABEL (label_t), title );
  else
    if(title){
      label_t = gtk_label_new( title );
      gtk_widget_show (label_t);  
      gtk_box_pack_start (GTK_BOX (hbox_t), label_t, TRUE, TRUE, 0);
    }

  if(label_x && tx)
    gtk_label_set_text( GTK_LABEL (label_x), tx );
  else
    if(tx){
      label_x = gtk_label_new( tx );
      gtk_widget_show (label_x);  
      gtk_box_pack_start (GTK_BOX (hbox_t), label_x, TRUE, TRUE, 0);
    }

  if(label_y && ty)
    gtk_label_set_text( GTK_LABEL (label_y), ty );
  else
    if(ty){
      label_y = gtk_label_new( ty );
      gtk_widget_show (label_y);  
      gtk_box_pack_start (GTK_BOX (hbox_t), label_y, TRUE, TRUE, 0);
    }
}

void DataBox::setSetType ( gint idx, GtkDataboxDataType typ, gint dotsize){
  if(Ypoints[idx])
    gtk_databox_data_set_type(GTK_DATABOX(Screen), idx, 
			      typ>=0 ? typ:BoxType,
			      dotsize>=0 ? dotsize:BoxWidth);
}

void DataBox::setSetColor ( gint idx, GdkColor color ){
  if(Ypoints[idx])
    gtk_databox_data_set_color(GTK_DATABOX(Screen), idx, color );
}

void DataBox::removeData (){
  gtk_databox_data_remove_all(GTK_DATABOX(Screen));
  if(!CpyMode){
    Xpoints=NULL;
    for(int i=0; i<NUMCHANNELS; ++i) Ypoints[i]=NULL;
  }else{
    if(Xpoints)
      delete Xpoints;
    Xpoints=NULL;
    for(int i=0; i<NUMCHANNELS; ++i) 
      if(Ypoints[i]){
	delete Ypoints[i];
	Ypoints[i]=NULL;
      }
  }
}

void DataBox::setxyData (PLOTTYP *SetX, PLOTTYP *SetY, int n, int cpy){
  PLOTTYP *y[NUMCHANNELS]; 
  y[0]=SetY;
  for(int i=1; i<NUMCHANNELS; ++i) y[i]=NULL;
  setxyData(SetX, y, n, cpy);
}

void DataBox::setxyData (PLOTTYP *SetX, PLOTTYP *SetY[NUMCHANNELS], int n, int cpy){
  GdkColor color[NUMCHANNELS];
  int j;

  removeData ();

  nPoints=n;

  for(j=0; j<NUMCHANNELS; j++){
    color[j].red   = (j+1)&1 ? 65535:0;
    color[j].green = (j+1)&4 ? 65535:0;
    color[j].blue  = (j+1)&2 ? 65535:0;
    if(SetY[j]){
      if(cpy){
	Ypoints[j] = new PLOTTYP[nPoints];
	memcpy(Ypoints[j], SetY[j], nPoints*sizeof(PLOTTYP));
	if(!Xpoints){
	  Xpoints = new PLOTTYP[nPoints];
	  memcpy(Xpoints, SetX, nPoints*sizeof(PLOTTYP));
	  gtk_databox_data_add_x_y(GTK_DATABOX(Screen), nPoints,
				   Xpoints, Ypoints[j], color[j],
				   BoxType, BoxWidth);
	}else
	  gtk_databox_data_add_y(GTK_DATABOX(Screen), nPoints,
				 Ypoints[j], 0, color[j],
				 BoxType, BoxWidth);
      }else{
	Ypoints[j] = SetY[j];
	if(!Xpoints){
	  Xpoints = SetX;
	  gtk_databox_data_add_x_y(GTK_DATABOX(Screen), nPoints,
				   Xpoints, Ypoints[j], color[j],
				   BoxType, BoxWidth);
	}else
	  gtk_databox_data_add_y(GTK_DATABOX(Screen), nPoints,
				 Ypoints[j], 0, color[j],
				 BoxType, BoxWidth);
      }
    }
  }
  CpyMode=cpy;
}

void DataBox::updatexyData (PLOTTYP *SetX, PLOTTYP *SetY){
  if(Xpoints && SetX)
    memcpy(Xpoints, SetX, nPoints*sizeof(PLOTTYP));
  if(SetY && Ypoints[0])
    memcpy(Ypoints[0], SetY, nPoints*sizeof(PLOTTYP));
}

void DataBox::updatexyData (PLOTTYP *SetX, PLOTTYP *SetY[NUMCHANNELS]){
  if(Xpoints && SetX)
    memcpy(Xpoints, SetX, nPoints*sizeof(PLOTTYP));
  for(int j=0; j<NUMCHANNELS; j++)
    if(SetY[j] && Ypoints[j])
      memcpy(Ypoints[j], SetY[j], nPoints*sizeof(PLOTTYP));
}

void DataBox::print(gchar *name){
}

void DataBox::load(gchar *name){
  gchar line[256];
  int np=0, nch=0;
  int ii;
  ifstream in;
  in.open(name, ios::in);
  do{
    in.getline(line, 256);
//    PI_DEBUG (DBG_L2, line );
    if(! strncmp(line, "# nPoints:", 10))
      np = atoi(line+11);
    else if(! strncmp(line, "# nChannels:", 12))
      nch = atoi(line+13);
    else checkheader(line);
  }while(strcmp(line, "# DataStart"));

//  PI_DEBUG (DBG_L2, "Open: " << np << ": " << nch );
  if(np && nch){
    if(CXpoints)
      delete CXpoints;
    CXpoints=NULL;
    for(int i=0; i<NUMCHANNELS; ++i) 
      if(CYpoints[i]){
	delete CYpoints[i];
	CYpoints[i]=NULL;
      }
    CnPoints=np;
    CXpoints = new PLOTTYP[CnPoints];
    for(int i=0; i<nch; ++i)
      CYpoints[i] = new PLOTTYP[CnPoints];

    for(int i=0; i<CnPoints; i++){
      in >> ii >> CXpoints[i];
      for(int j=0; j<nch; j++)
	in >> CYpoints[j][i];
    }
    setxyData (CXpoints, CYpoints, np, TRUE);
    draw(TRUE);
    if(CXpoints)
      delete CXpoints;
    CXpoints=NULL;
    for(int i=0; i<NUMCHANNELS; ++i) 
      if(CYpoints[i]){
	delete CYpoints[i];
	CYpoints[i]=NULL;
      }
    CnPoints=0;
  }
  in.close();
}

void DataBox::save(gchar *name){
  int  nch;
  char hn[256];
  time_t t;
  ofstream out;
  gethostname(hn, 256);
  t=time(NULL);

  for(nch=0; nch<NUMCHANNELS; nch++)
    if(!Ypoints[nch])
      break;

  out.open(name, ios::out);

  out << "# goszi: DataAq mode" << endl;
  out << "# date: " << ctime(&t);
  out << "# user: " << getlogin() << "@" << hn << endl;
  out << "# nPoints: " << nPoints << endl;
  out << "# nChannels: " << nch << endl;
  out << "# --- Options Start ---" << endl;
  addheader(out);
  out << "# --- Options End ---" << endl;
  out << "# Format: index x y1 y2 ... y" << nch << endl;
  out << "# DataStart" << endl;

  for(int i=0; i<nPoints; i++){
    out << i << " " << setprecision(Xprec()) << Xpoints[i];
    for(int j=0; j<nch; j++)
      if(Ypoints[j])
	out << " " << setprecision(Yprec(j)) << Ypoints[j][i];
    out << endl;
  }
  out.close();
}

void DataBox::cut(){
  int i1, i2;
  GtkDataboxValue min, max;
  gtk_databox_data_get_visible_extrema(GTK_DATABOX(Screen), &min, &max);
  for(i1= 0; i1<nPoints && Xpoints[i1]<min.x; ++i1);
  for(i2=i1; i2<nPoints && Xpoints[i2]<max.x; ++i2);
  // ...
}

void DataBox::copy(){
  int i1, i2;
  GtkDataboxValue min, max;
  if(!Xpoints) return;
  if(CXpoints)
    delete CXpoints;
  CXpoints=NULL;
  for(int i=0; i<NUMCHANNELS; ++i) 
    if(CYpoints[i]){
      delete CYpoints[i];
      CYpoints[i]=NULL;
    }
  gtk_databox_data_get_visible_extrema(GTK_DATABOX(Screen), &min, &max);
  for(i1= 0; i1<nPoints && Xpoints[i1]<min.x; ++i1);
  for(i2=i1; i2<nPoints && Xpoints[i2]<max.x; ++i2);
  CnPoints = i2-i1+1;
  CXpoints = new PLOTTYP[CnPoints];
  memcpy(CXpoints, Xpoints+i1, sizeof(CXpoints));
  for(int i=0; i<NUMCHANNELS; ++i)
    if(Ypoints[i]){
      CYpoints[i] = new PLOTTYP[CnPoints];
      memcpy(CYpoints[i], Ypoints[i]+i1, sizeof(CYpoints[i]));
    }
}

void DataBox::GetVisible( GtkDataboxValue *vmin, GtkDataboxValue *vmax){
  gtk_databox_data_get_visible_extrema(GTK_DATABOX(Screen), vmin, vmax);
}

void DataBox::paste(){
  int i1;
  GtkDataboxValue min, max;
  gtk_databox_data_get_visible_extrema(GTK_DATABOX(Screen), &min, &max);
  for(i1= 0; i1<nPoints && Xpoints[i1]<min.x; ++i1);
  // ....
}

void DataBox::run(){
//  PI_DEBUG (DBG_L2, "DataBox::run" );
  if(atrun())
    if(!CbId)
      CbId = gtk_timeout_add((int)delay, (GtkFunction) loop, this); 
}
void DataBox::stop(){
//  PI_DEBUG (DBG_L2, "DataBox::stop" );
  if(CbId)
    gtk_timeout_remove(CbId);
  CbId=0;
  atstop();
}

void DataBox::draw(int autoskl, GtkDataboxValue *vmin, GtkDataboxValue *vmax){
  if(autoskl)
    gtk_databox_rescale(GTK_DATABOX(Screen));
  else
    if(vmin && vmax)
      gtk_databox_rescale_with_values(GTK_DATABOX(Screen), *vmin, *vmax);

  gtk_databox_redraw(GTK_DATABOX(Screen));
}

gint DataBox::loop(DataBox *db){
  if( db->LoopActive ) return FALSE;
  db->LoopActive=TRUE;

  if(db->CbId){
    gtk_timeout_remove(db->CbId);
    db->CbId=0;
  }
  db->getdata();
  // rerun in delay ms!
  db->CbId = gtk_timeout_add((int)db->delay, (GtkFunction) DataBox::loop, db);

  db->LoopActive=FALSE;
  return TRUE;
}

void DataBox::make_copy(){
  PLOTTYP *cx, *cy[NUMCHANNELS];
  int n;

  if(Xpoints && !CpyMode){
    cx = Xpoints;
    n = nPoints;
    for(int i=0; i<NUMCHANNELS; ++i) 
      cy[i] = Ypoints[i];
    setxyData(cx, cy, n, TRUE);
  }
}

