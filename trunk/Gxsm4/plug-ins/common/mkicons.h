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
This file belongs to mkicons.C and helps the mkicons plugin of GXSM.
*/

//#ifndef __XSM_MKICONS_H
//#define __XSM_MKICONS_H

struct MKICONSPI_OPTIONS{
  const gchar *name;
  const gchar **list;
  const gchar *id;
  int  init;
};

class MkIconsPIData{
 public:
  MkIconsPIData(const gchar *InPath=NULL, const gchar *OutPath=NULL, const gchar *InMask=NULL, 
		const gchar *Opt=NULL, const gchar *IconName=NULL);
  ~MkIconsPIData();

  gchar *pathname;
  gchar *outputname;
  gchar *name;
  gchar *mask;
  gchar *options;
  int   redres;
  int   nix;
};

//#endif

void MkIconsPI(MkIconsPIData *mid);



typedef enum { 
	MkIconOpt_Paper, MkIconOpt_Resolution, MkIconOpt_EReg, MkIconOpt_LReg, 
	MkIconOpt_ViewMode, MkIconOpt_AutoSkl, MkIconOpt_Scaling 
} MkIconOptions ;

class MkIconsPIControl : public DlgBase{
public:
  MkIconsPIControl();
  ~MkIconsPIControl();

  void run();

  static void option_choice_callback(GtkWidget *widget, MkIconsPIControl *mki);
  void dlg_clicked(gint button_number);

  MkIconsPIData *icondata;

  GtkWidget *SrcPath, *SrcMask, *IconName;
  GtkWidget **Options;

private:
};
