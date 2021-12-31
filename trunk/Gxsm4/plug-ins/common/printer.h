/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 2003 Percy Zahl 
 *
 * Authors: Stefan Schroeder and Percy Zahl 
 *                                                                                
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
 *  * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

/*
This file belongs to printer.C and contains information about the printer plugin of GXSM.
*/


class PIPrintPSData{
 public:
  PIPrintPSData();
  ~PIPrintPSData();

  gchar *title;
  gchar *fname;
  gchar *prtcmd;
  gchar *previewcmd;
  int  mode;
  int  typ;
  int  info;
  
  double fontsize;
  double figwidth;
  UnitObj *ptUnit;
  UnitObj *mmUnit;
};
void PIPrintPS(PIPrintPSData *ppsd);


class PIPrintControl : public DlgBase{
public:
  PIPrintControl();
  ~PIPrintControl();

  void run();

  static void option_choice_callback(GtkWidget *widget, PIPrintControl *pc);
  static void dlg_clicked(GnomeDialog * dialog, gint button_number, PIPrintControl *pc);

  PIPrintPSData *pdata;

  GtkWidget *title, *fname, *prtcmd, *previewcmd;
  GtkWidget *framebutton, *scalebutton, *regionbutton;
  GtkWidget *filebutton, *printbutton, *previewbutton;
  Gtk_EntryControl *Fs, *Fw;
private:
  void cleanup();
};
