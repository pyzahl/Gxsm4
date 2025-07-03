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

#include <stdlib.h>
#include "lineprofile.h"
#include "mem2d.h"
#include "glbvars.h"
#include "action_id.h"
#include "gxsm_monitor_vmemory_and_refcounts.h"
#include "surface.h"
#include "scan.h"

LineProfile1D::LineProfile1D(){
        GXSM_REF_OBJECT (GXSM_GRC_PROFILEOBJ);
	scan1d = NULL;
	scan1d_2 = NULL;
	private_scan1d = NULL;
	private_scan1d_2 = NULL;
}

LineProfile1D::LineProfile1D(int n, UnitObj *ux, UnitObj *uy, double xmin, double xmax, int ns){
        GXSM_REF_OBJECT (GXSM_GRC_PROFILEOBJ);
	scan1d_2 = NULL;
	private_scan1d_2 = NULL;

	private_scan1d   = new Scan(0, 0, -1, NULL, ZD_DOUBLE); // new private scan

	scan1d = private_scan1d; // never delete scan1d!!! -- only a internal (or external) reference!
	
	scan1d->data.s.nvalues=1;
	scan1d->data.s.ntimes=1;

	scan1d->data.s.ny=ns;
	scan1d->data.s.nx=n;

	scan1d->data.s.rx=xmax-xmin;
	scan1d->data.s.ry=0.;
	scan1d->data.s.rz=0.;
	scan1d->data.s.dl=scan1d->data.s.rx;

	scan1d->data.s.dx=scan1d->data.s.rx/(n-1);
	scan1d->data.s.dy=1.;
	scan1d->data.s.dz=1.;

	scan1d->data.s.x0=xmin;
	scan1d->data.s.y0=0.;
	scan1d->data.s.alpha=0.;

	scan1d->create();
	scan1d->mem2d->data->MkXLookup(xmin, xmax);

	UnitObj Unity(" "," ","g","arb. units");
	scan1d->data.SetXUnit (ux);
	scan1d->data.SetYUnit (&Unity);
	scan1d->data.SetZUnit (uy);

}

LineProfile1D::~LineProfile1D(){
	if(private_scan1d)
		delete private_scan1d;
	if(private_scan1d_2)
		delete private_scan1d_2;
        GXSM_UNREF_OBJECT (GXSM_GRC_PROFILEOBJ);
}

// Lesen und schreiben von spa4 .d1d Daten

int LineProfile1D::load(const gchar *fname){
	XSM_DEBUG (DBG_L3, "LineProfile1D::load - file to new scan: " << fname);
	std::ifstream f;

	if (fname == NULL) return 1;
	
	f.open(fname, std::ios::in);
	if(!f.good()){
		//    fl_show_alert(ERR_SORRY, ERR_FILEREAD,fname,1);
		return 1;
	}

	XSM_DEBUG (DBG_L3, "LineProfile1D::load - file check OK");

	if(private_scan1d)
		delete private_scan1d;

	XSM_DEBUG (DBG_L3, "LineProfile1D::load - new Scan");
	private_scan1d = new Scan(0, 0, -1, NULL, ZD_DOUBLE); // new private scan
	scan1d = private_scan1d; // never delete scan1d!!! -- only a internal (or external) reference!
	
	scan1d->data.s.nvalues=1;
	scan1d->data.s.ntimes=1;

	scan1d->data.s.ny=1;
	scan1d->data.s.nx=0;

	scan1d->data.s.rx=1.;
	scan1d->data.s.ry=0.;
	scan1d->data.s.rz=0.;
	scan1d->data.s.dl=scan1d->data.s.rx;

	scan1d->data.s.dx=1.;
	scan1d->data.s.dy=1.;
	scan1d->data.s.dz=1.;

	scan1d->data.s.x0=0.;
	scan1d->data.s.y0=0.;
	scan1d->data.s.alpha=0.;

	UnitObj Unity(" "," ","g","arb. units");
	scan1d->data.SetXUnit (&Unity);
	scan1d->data.SetYUnit (&Unity);
	scan1d->data.SetZUnit (&Unity);

	gchar l[1024];

	// Check for VectorProbe file type
	if (strstr(fname, ".vpdata")){

//		std::cout << "VP DATA READ" << std::endl;
		
		if (f.good())
			f.getline(l, 1024);
		else
			return 1;
		if (f.good())
			f.getline(l, 1024);
		else
			return 1;
		if (strncmp(l, "# GXSM Vector Probe Data :: VPVersion=00.02", 43) == 0 ||
                    strncmp(l, "# GXSM Vector Probe Data :: VPVersion=00.03", 43) == 0){
			// vpdata file type/version double check OK :-)
			// skip fwd to Vector Position Table
			while (f.good()){
				f.getline(l, 1024);
				if (strncmp(l, "#C Full Position Vector List", 28) == 0)
					break;
			}


//			std::cout << "VP DATA FOUND IS VALID" << std::endl;

			// scan vector table
                        int nvsec = 0;
			gchar sec_position_vector[64][1024];
			double svXS[64], svYS[64], svZS[64], svtime[64];
			int svindexi[64];
			int i=0;
			while (i < 64 && f.good()){
				svXS[i] = svYS[i] = svZS[i] = svtime[i] = 0.;
				svindexi[0]=-1;
				f.getline (sec_position_vector[i], 1024);
				if (strncmp(sec_position_vector[i], "# S[", 4) == 0){
					gchar **secvec = g_strsplit (sec_position_vector[i], " ", 30);
					gchar **svtoc = secvec;
					for (; *svtoc; ++svtoc){
						if (!strncmp (*svtoc, "VP[=", 4))
							svindexi[i] = atoi (*svtoc+4);
						if (!strncmp (*svtoc, "\"Time\"=", 7))
							svtime[i] = atof (*svtoc+7);
						if (!strncmp (*svtoc, "\"XS\"=", 5))
							svXS[i] = atof (*svtoc+5);
						if (!strncmp (*svtoc, "\"YS\"=", 5))
							svYS[i] = atof (*svtoc+5);
						if (!strncmp (*svtoc, "\"ZS\"=", 5))
							svZS[i] = atof (*svtoc+5);
					}
					if (secvec)
                                                g_strfreev (secvec);
					nvsec = ++i;
				} else break;
                                g_print ("SecVecTable: %d [#%d t=%g XYZ=(%g %g %g)\n]", nvsec, svindexi[i], svtime[i], svXS[i], svYS[i], svZS[i]);
			}
                        
//			std::cout << "VP DATA -- SECTAB OK: nvsec=" << nvsec << std::endl;

			while (f.good()){
				f.getline(l, 1024);
				if (strncmp(l, "#C Data Table", 13) == 0)
					break;
			}

//			std::cout << "VP DATA -- DATA TABLE FOUND" << std::endl;

			gchar column_header_labels[1024];
			f.getline(column_header_labels, 1024);

//			std::cout << "VP DATA -- READING HEADERS" << std::endl;

			gchar **Hrecord = g_strsplit(column_header_labels, "\t", 100);
			gchar **token = Hrecord;
			int j;
			for (j=0; *token; ++token){
				if (! strcmp (*token, "#C Index"))
					continue;
				if (! strcmp (*token, "Block-Start-Index"))
					break;

//				std::cout << "VP DATA -- H:" << j << " = " << *token << std::endl;

				UnitObj *uobj=NULL;
				g_strdelimit (*token, "\")", ' ');
				gchar **lu = g_strsplit(*token, "(", 2);

//				if (lu[1])
//					std::cout << "VP DATA -- HTOK:" << lu[0] << " [" << lu[1] << "]" << std::endl;
//				else
//					std::cout << "VP DATA -- HTOK:" << lu[0] << " [??]" << std::endl;

				uobj = new LinUnit (lu[1]?g_strstrip(lu[1]):"??", lu[1]?g_strstrip(lu[1]):"??",
						    g_strstrip(lu[0]), 1.0);

				if (j==0)
					scan1d->data.SetXUnit (uobj);
				
				if (j==1) // this is ref. -- possibly not rigth unit for j>1!!!!!!
					scan1d->data.SetZUnit (uobj);

				if (lu)
					g_strfreev (lu);

				++j;
			}
			g_strfreev (Hrecord);
			scan1d->data.s.ny = j-1;

//			int data_pos = f.tellg ();

			scan1d->data.s.nx = 0; 
			while (f.good()){
				f.getline(l, 1024);
				if (strncmp(l, "#C", 2) == 0) continue;
				if (strncmp(l, "#C END.", 7) == 0) break; // EOF mark OK.
				scan1d->data.s.nx++; 
			}

//			std::cout << "VP DATA -- FOUND SETS #" << scan1d->data.s.nx << std::endl;
			scan1d->data.s.rx=1.; 
			scan1d->data.s.dl=1.; 
			scan1d->data.s.dx=1.; 
			scan1d->data.s.dy=1.; 
			scan1d->data.s.dz=1.; 
			scan1d->data.s.x0=0.; 
			scan1d->data.s.y0=0.; 

			scan1d->data.ui.SetTitle(fname); 
	
			scan1d->create();

//			f.seekg (data_pos, std::ios::beg);
			f.close (); // reopen, reseek
			f.open(fname, std::ios::in);
			while (f.good()){
				f.getline(l, 1024);
				if (strncmp(l, "#C Data Table", 13) == 0){
					f.getline(l, 1024); // skip one more (headers
					break;
				}
			}

//			std::cout << l << std::endl;
//			std::cout << "VP DATA -- RESEEK, READING DATA" << std::endl;

			// put data
			for(int i=0; i<scan1d->data.s.nx && f.good(); ++i){
				double x=0., y=0.;
				f.getline (l, 1024);
//				std::cout << l << std::endl;
				g_strdelimit (l, "\t", ' ');
				gchar **VPrecord = g_strsplit(l, " ", 100);
				
				gchar **token = VPrecord;
				for (int j=-1; *token && j <= scan1d->mem2d->GetNy (); ++token, ++j){
					if (j==0){
						x = atof (*token);
						scan1d->mem2d->data->SetXLookup (i, x);
//						std::cout << "X=" << x << std::endl;
						continue;
					}
					if (j>0){
						y = atof (*token);
						if (j <= scan1d->mem2d->GetNy ())
							scan1d->mem2d->PutDataPkt (y, i, j-1);
//						std::cout << "Y[" << i << "]_" << j << "=" << y << std::endl;
						continue;
					}
				}
				g_strfreev (VPrecord);
			}

//			std::cout << "VP DATA -- READING DATA OK" << std::endl;

			f.close();
			XSM_DEBUG (DBG_L3, "LineProfile1D::load [VP Data Mode] - Done.");
			return 0;
		}else{
			XSM_DEBUG (DBG_L3, "VP file type check failed or not VPVer=00.02");
			return 1;
		}
	}

	XSM_DEBUG (DBG_L3, "LineProfile1D::load - reading asc Header");
	while (f.good()){
		f.getline(l, 1024);
		XSM_DEBUG(DBG_L2, l );

		if (strncmp (l, "# Len   = ", 10) == 0){
			scan1d->data.s.rx=atof(l+10); 
			scan1d->data.s.dl=scan1d->data.s.rx;
			continue;
		}
		if (strncmp (l, "# dX    = ", 10) == 0){
			scan1d->data.s.dx=atof(l+10); 
			continue;
		}
		if (strncmp (l, "# dY    = ", 10) == 0){
			scan1d->data.s.dy=atof(l+10); 
			continue;
		}
		if (strncmp (l, "# dZ    = ", 10) == 0){
			scan1d->data.s.dz=1.; // reading world data now!!
			continue;
		}
		if (strncmp (l, "# X0    = ", 10) == 0){
			scan1d->data.s.x0=atof(l+10); 
			continue;
		}
		if (strncmp (l, "# Y0    = ", 10) == 0){
			scan1d->data.s.y0=atof(l+10); 
			continue;
		}
		if (strncmp (l, "# Anz   = ", 10) == 0){
			scan1d->data.s.nx=atoi(l+10); 
			continue;
		}
		if (strncmp (l, "# NSets = ", 10) == 0){
			scan1d->data.s.ny=atoi(l+10); 
			continue;
		}
		if (strncmp (l, "# Xunit = ", 10) == 0){
			gchar *u = g_strdup(l+10);
			gchar *ua=NULL;
			gchar *ul=NULL;
			f.getline(l, 1024); // expecting label in next line!
			if (strncmp (l, "# Xalias= ", 10) == 0){
				ua = g_strdup(l+10);
				f.getline(l, 1024); // expecting label in next line!
				if (strncmp (l, "# Xlabel= ", 10) == 0)
					ul = g_strdup(l+10);
				UnitObj *uobj=NULL;
				if (ua)
					uobj = main_get_gapp ()->xsm->MakeUnit (ua, ul?ul:ua);
				else
					uobj = main_get_gapp ()->xsm->MakeUnit (u, ul?ul:u);

				scan1d->data.SetXUnit (uobj);
				delete uobj;
					
				if (u) g_free(u);
				if (ua) g_free(ua);
				if (ul) g_free(ul);
				continue;
			}
		}
		if (strncmp (l, "# Yunit = ", 10) == 0){
			gchar *u = g_strdup(l+10);
			gchar *ua=NULL;
			gchar *ul=NULL;
			f.getline(l, 1024); // expecting label in next line!
			if (strncmp (l, "# Yalias= ", 10) == 0){
				ua = g_strdup(l+10);
				f.getline(l, 1024); // expecting label in next line!
				if (strncmp (l, "# Ylabel= ", 10) == 0)
					ul = g_strdup(l+10);
				UnitObj *uobj=NULL;
				if (ua)
					uobj = main_get_gapp ()->xsm->MakeUnit (ua, ul?ul:ua);
				else
					uobj = main_get_gapp ()->xsm->MakeUnit (u, ul?ul:u);

				scan1d->data.SetZUnit (uobj);
				delete uobj;
					
				if (u) g_free(u);
				if (ua) g_free(ua);
				if (ul) g_free(ul);
				continue;
			}
		}
		if (strncmp (l, "# Title = ", 10) == 0){
			scan1d->data.ui.SetTitle(l+10); 
			continue;
		}
		if (strncmp (l, "# Format= ", 10) == 0){
			break;
		}
		if (strncmp (l, "# Fromat= ", 10) == 0){ // bugfix
			break;
		}
	}
	XSM_DEBUG (DBG_L3, "LineProfile1D::load - new Scan->create()");
	XSM_DEBUG (DBG_L3, "LineProfile1D::load NX: "<<scan1d->data.s.nx); 
	XSM_DEBUG (DBG_L3, "LineProfile1D::load NY: "<<scan1d->data.s.ny); 
	scan1d->create();

	XSM_DEBUG (DBG_L3, "LineProfile1D::load - reading data");
	for(int i=0; i<scan1d->data.s.nx && f.good(); ++i){
		double x=0., y=0.;
		f.getline (l, 1024);
		g_strdelimit (l, "\t", ' ');
		gchar **record = g_strsplit(l, " ", 100);

		gchar **token = record;
		for (int j=-1; *token; ++token){
			if (! strcmp (*token, "InUnits:")){
				++j;
				continue;
			}
			if (j==0){
				x = atof (*token);
				scan1d->mem2d->data->SetXLookup (i, x);
				XSM_DEBUG(DBG_L2, i << " " << x );
				++j;
				continue;
			}
			if (j>0){
				y = atof (*token);
				if (j <= scan1d->mem2d->GetNy ())
					scan1d->mem2d->PutDataPkt (y, i, j-1);
				++j;
				continue;
			}
		}
		g_strfreev (record);
	}
    
	f.close();
	XSM_DEBUG (DBG_L3, "LineProfile1D::load - Done.");
	return 0;
}

int LineProfile1D::save(const gchar *fname){
	std::ofstream f;
	const gchar *separator = "\t";
	int   i,j;
	time_t t;
	time(&t); 
	f.open(fname, std::ios::out | std::ios::trunc);
	if(!f.good()){
		//    fl_show_alert(ERR_SORRY, ERR_FILEWRITE,fname,1);
	return 1;
	}
	f << "# ASC LineProfile Data V2" << std::endl
	  << "# Date: " << ctime(&t)
	  << "# FileName: " << fname << std::endl
	  << "# Len   = " << scan1d->data.s.rx << "A" << std::endl
	  << "# tStart= " << scan1d->data.s.tStart << "s" << std::endl
	  << "# tEnd  = " << scan1d->data.s.tEnd << "s" << std::endl
	  << "# phi   = " << scan1d->data.s.alpha << "degree" << std::endl
	  << "# dX    = " << scan1d->data.s.dx << "A" << std::endl
	  << "# dY    = " << scan1d->data.s.dy << "A" << std::endl
	  << "# dZ    = " << scan1d->data.s.dz << "A" << std::endl
	  << "# Xo    = " << scan1d->data.s.x0 << "A" << std::endl
	  << "# Yo    = " << scan1d->data.s.y0 << "A" << std::endl
	  << "# Anz   = " << scan1d->data.s.nx << std::endl
	  << "# NSets = " << scan1d->data.s.ny << std::endl
	  << "# Xunit = " << (scan1d->data.Xunit->Symbol()? scan1d->data.Xunit->Symbol():"N/A") << std::endl
	  << "# Xalias= " << (scan1d->data.Xunit->Alias()? scan1d->data.Xunit->Alias():"N/A") << std::endl
	  << "# Xlabel= " << scan1d->data.Xunit->Label() << std::endl
	  << "# Yunit = " << (scan1d->data.Zunit->Symbol()? scan1d->data.Zunit->Symbol():"N/A")  << std::endl
	  << "# Yalias= " << (scan1d->data.Zunit->Alias()? scan1d->data.Zunit->Alias():"N/A") << std::endl
	  << "# Ylabel= " << scan1d->data.Zunit -> Label() << std::endl
	  << "# Title = " << scan1d->data.ui.title << std::endl
	  << "# Format= X[index*LineLen/Nx] Z[DA]... InUnit: X[Xunit] Z[Zunit]..." << std::endl
		;

	for(i=0; i<scan1d->mem2d->GetNx(); i++){
		f << i*scan1d->data.s.dl/(double)scan1d->mem2d->GetNx();
		for(j=0; j<scan1d->mem2d->GetNy(); j++)
			f << separator << (double)scan1d->mem2d->GetDataPkt(i,j);

		f << separator << "InUnits:" << separator  
		  << ( scan1d->data.Xunit ? scan1d->data.Xunit : scan1d->data.Xunit) -> Base2Usr(scan1d->mem2d->data->GetXLookup(i));
		for(j=0; j<scan1d->mem2d->GetNy(); j++)
			f << separator << scan1d->data.Zunit->Base2Usr(scan1d->mem2d->GetDataPkt(i,j)*scan1d->data.s.dz);

		f << std::endl;
	}
  
	f.close();

	return 0;
}

// for single "red line"
int LineProfile1D::SetData(Scan *sc, int line){
	//  XSMDEBUG("LineProfile1D::SetData");

	if(line == -2){ // mirror mode -- red line
		if(private_scan1d){
			delete private_scan1d;
			private_scan1d = NULL;
		}
		scan1d = sc;
		return 0; // done.
	}

	if(! private_scan1d){ // need new scan?
		private_scan1d = new Scan(sc); // new private scan based on sc
		scan1d = private_scan1d; // never delete scan1d!!! -- only a internal (or external) reference!
	}

	scan1d->CpyDataSet (sc->data);
	scan1d->data.CpUnits (sc->data);
	scan1d->data.s.ny=1;
	scan1d->data.s.nvalues=1;
	scan1d->data.s.ntimes=1;
	scan1d->create();

	if(line >= 0)
		scan1d->mem2d->CopyFrom(sc->mem2d, 0,line, 0,0, scan1d->mem2d->GetNx()); 

	return 0;
}

int LineProfile1D::SetData_redprofile(Scan *sc, int redblue){
	Point2D a;
	int hist_len=1;
	Scan *s;
	//  XSMDEBUG("LineProfile1D::SetData_redprofile");

	if (sc->Pkt2dScanLine[0].y >= sc->mem2d->GetNy() || sc->Pkt2dScanLine[0].y < 0) {
	        g_warning ("LineProfile1D::SetData_redprofile invalid data request");
  	        return 0;
	}

	if(redblue == 'r' && ! private_scan1d){ // need new scan?
		private_scan1d = new Scan(sc); // new private scan based on sc
		scan1d = private_scan1d; // never delete scan1d!!! -- only a internal (or external) reference!
		scan1d->CpyDataSet (sc->data);
		scan1d->data.CpUnits (sc->data);
		scan1d->data.s.ny=1;
		scan1d->data.s.x0=0.;
		scan1d->data.s.y0=0.;
		scan1d->data.s.dz=sc->data.s.dz;
		scan1d->data.s.nvalues=1;
		scan1d->data.s.ntimes=1;
		scan1d->create();
		scan1d->mem2d->data->CopyXLookup (sc->mem2d->data);
	}

	if(redblue == 'b' && ! private_scan1d_2){ // need new 2nd scan?
		private_scan1d_2 = new Scan(sc); // new private scan based on sc
		scan1d_2 = private_scan1d_2; // never delete scan1d_2!!! -- only a internal (or external) reference!
		scan1d_2->CpyDataSet (sc->data);
		scan1d_2->data.CpUnits (sc->data);
		scan1d_2->data.s.ny=1;
		scan1d_2->data.s.x0=0.;
		scan1d_2->data.s.y0=0.;
		scan1d_2->data.s.dz=sc->data.s.dz;
		scan1d_2->data.s.nvalues=1;
		scan1d_2->data.s.ntimes=1;
		scan1d_2->create();
		scan1d_2->mem2d->data->CopyXLookup (sc->mem2d->data);
	}
	if(redblue == 'b' && scan1d_2)
		s = scan1d_2;
	else
		s = scan1d;

	// RedLine special data copy mode for last [#xcolors set,...] lines
	// make sure it's up-to-date
	s->CpyDataSet (sc->data);
	s->mem2d->data->CopyXLookup (sc->mem2d->data);

	if (xcolors_list)
		g_strfreev (xcolors_list);
	if (xcolors_list_2)
		g_strfreev (xcolors_list_2);

	num_xcolors=0;
	num_xcolors_2=0;
	xcolors_list   = g_strsplit (xsmres.RedLineHistoryColors, ",", 0);
	xcolors_list_2 = g_strsplit (xsmres.BlueLineHistoryColors, ",", 0);
	for (; xcolors_list[num_xcolors]; ++num_xcolors);
	for (; xcolors_list_2[num_xcolors_2]; ++num_xcolors_2);

	if(redblue == 'b')	
		hist_len = num_xcolors_2-1;
	else
		hist_len = num_xcolors-1;
	
	a.x = sc->Pkt2dScanLine[0].x;
	s->data.s.nx = sc->mem2d->GetNx ();
	
        //        gchar trb[2] = {redblue, 0};

	gint sydir = sc->data.s.ydir;
	if (sydir > 0){ 
	        a.y = sc->Pkt2dScanLine[0].y-hist_len;
		while (a.y < 0){
                        a.y++;
                        hist_len--;
		}

#if 0
		if (sc->mem2d->data->Li[sc->Pkt2dScanLine[0].y].IsNew() != 1 || hist_len < 0){
                        g_warning ("LineProfile1D::SetData_redprofile dir>0 -- already processed line %d or history length %d invalid.", a.y, hist_len);
                        //std::cout << __func__ << trb << " top-dn[" << sydir << "] - not valid new data for line: a.y=" << a.y << ", with histlen=" << hist_len << std::endl;
                        return 0;
		}
#endif
		s->data.s.ny = 1 + sc->Pkt2dScanLine[0].y - a.y;
		//std::cout << __func__ << " " << trb << " top-dn[" << sydir << "] [#" << hist_len << ", Nx=" << s->mem2d->GetNx () << ", Ny=" << s->mem2d->GetNy () << "] s->data.s.ny=" << s->data.s.ny << " a.y=" << a.y << "  with histlen=" << hist_len << std::endl;
		s->mem2d->Resize (s->data.s.nx, s->data.s.ny);
		s->mem2d->ConvertFrom (sc->mem2d, 0, a.y, 0,0,  s->data.s.nx, s->data.s.ny, 1);
			
		int i_i = a.y + s->mem2d->GetNy ();
		for (int i=0; i < s->mem2d->GetNy (); ++i, --i_i)
			s->mem2d->data->SetYLookup (i, i_i);
		sc->mem2d->data->Li[a.y].SetNew(2); // now processed/displayed
	} else {
	        a.y = sc->Pkt2dScanLine[0].y+hist_len;
		while (a.y >= sc->mem2d->GetNy ()){
                        a.y--;
                        hist_len--;
		}

#if 0
		if (sc->mem2d->data->Li[a.y].IsNew() != 1 || hist_len < 0){
                        g_warning ("LineProfile1D::SetData_redprofile dir<0 -- already processed line %d or history length %d invalid.", a.y, hist_len);
                        //std::cout << __func__ << trb  << " bot-up[" << sydir << "] - not valid new data for line a.y=" << a.y << ", with histlen=" << hist_len << std::endl;
                        return 0;
		}
#endif

                s->data.s.ny = 1 + a.y - sc->Pkt2dScanLine[0].y;
		//std::cout << __func__ << " " << trb << " bot-up[" << sydir << "] [#" << hist_len << ", Nx=" << s->mem2d->GetNx () << ", Ny=" << s->mem2d->GetNy () << "] s->data.s.ny=" << s->data.s.ny << " a.y=" << a.y << "  with histlen=" << hist_len << std::endl;
		s->mem2d->Resize (s->data.s.nx, s->data.s.ny);
		s->mem2d->ConvertFrom (sc->mem2d, 0,  sc->Pkt2dScanLine[0].y, 0,0,  s->data.s.nx, s->data.s.ny, 0);

		for (int i=0; i < s->mem2d->GetNy (); ++i)
			s->mem2d->data->SetYLookup (i, sc->Pkt2dScanLine[0].y+i);
		sc->mem2d->data->Li[a.y].SetNew(2); // now processed/displayed
	}

	return 0;
}

// need to copy lines for multiple "red lines" with history
int LineProfile1D::SetData(Scan *sc, VObject *vo, gboolean append){
        Point2D p2d[2];
        vo->get_xy_i_pixel2d (0, &p2d[0]);
        vo->get_xy_i_pixel2d (1, &p2d[1]);

	//  XSMDEBUG("LineProfile1D::SetData");

	if(! private_scan1d){ // need new scan?
		private_scan1d = new Scan(sc); // new private scan based on sc
		scan1d = private_scan1d; // never delete scan1d!!! -- only a internal (or external) reference!
		scan1d->CpyDataSet (sc->data);
		scan1d->data.CpUnits (sc->data);
		scan1d->data.s.ny=1;
		scan1d->data.s.x0=0.;
		scan1d->data.s.y0=0.;
		scan1d->data.s.dz=sc->data.s.dz;
		scan1d->data.s.nvalues=1;
		scan1d->data.s.ntimes=1;
		scan1d->create();
	}

	switch (vo ? vo->obj_type_id () : 0){
	case O_POINT:
		if (vo->get_num_points () == 1){
			if (vo->get_profile_path_dimension () == MEM2D_DIM_LAYER){ // Plot Data over Value (Layer dim is 3) as Series in times
				int nt = sc->number_of_time_elements ();
				if (vo->get_profile_series_all () && nt > 1)
					for (int t=0; t<nt; ++t)
						scan1d->mem2d->GetLayerDataLineFrom (&p2d[0], sc->mem2d_time_element (t), &sc->data, &scan1d->data, vo->get_profile_path_width()/2., t);
				else
					scan1d->mem2d->GetLayerDataLineFrom (&p2d[0], sc->mem2d, &sc->data, &scan1d->data, vo->get_profile_path_width()/2.);

			} else if (vo->get_profile_series_dimension () == MEM2D_DIM_TIME){ // Plot Data over Time (Time dim is 4) as Series in Values (layer)
				int nt = sc->number_of_time_elements ();
                                int tend = vo->get_profile_series_limit (1);
                                if (tend < 1 || tend > nt)
                                        tend = nt;
				if (vo->get_profile_series_all () && nt > 1)
                                        for (int t=vo->get_profile_series_limit (0); t<tend; ++t){
                                                sc->mem2d_time_element (t) -> SetLayer (sc->mem2d->GetLayer ());
                                                scan1d->mem2d->GetLayerDataLineFrom (&p2d[0], sc->mem2d_time_element (t), &sc->data,  &scan1d->data, 
                                                                                     vo->get_profile_path_width()/2., t, TRUE);
                                        }
                                else{
                                        if (nt > 1)
                                                for (int t=vo->get_profile_series_limit (0); t<tend; ++t){
                                                        sc->mem2d_time_element (t) -> SetLayer (sc->mem2d->GetLayer ());
                                                        scan1d->mem2d->GetLayerDataLineFrom (&p2d[0], sc->mem2d_time_element(t), &sc->data,  &scan1d->data, 
                                                                                             vo->get_profile_path_width()/2., t, TRUE, sc->mem2d_time_element (t)->GetLayer());
                                                }
                                }
			}
		}
		if (scan1d->mem2d->GetNy() > 1 && vo->get_profile_series_pg2d ()){
			if (!scan1d->view){
				scan1d->view = new Grey2D (scan1d);
				scan1d->SetVM (SCAN_V_DIRECT);
			}
			scan1d->SetVM ();
			scan1d->auto_display ();
		} else {
			if (scan1d->view){
				delete scan1d->view;
				scan1d->view = NULL;
				
			}
		}
		return 0;

	case O_CIRCLE:
		if (vo->get_num_points () == 2)
			scan1d->mem2d->GetArcDataLineFrom (&p2d[0], &p2d[1], sc->mem2d, &scan1d->data, vo->get_profile_path_width());
		return 0;

	case O_LINE:
		if (vo->get_num_points () >= 2){
                        int sections = vo->get_num_points ()-1;
                        int section = 0;
			int nt = sc->number_of_time_elements ();
                        int tstart = vo->get_profile_series_limit (0);
                        int tend   = vo->get_profile_series_limit (1);
                        if (tend < 1 || tend > nt)
                                tend = nt;
                        while (sections--){
                                vo->get_xy_i_pixel2d (section++, &p2d[0]);
                                vo->get_xy_i_pixel2d (section,   &p2d[1]);
                                if (vo->get_profile_series_all () && nt > 1
                                    && (vo->get_profile_series_dimension () == MEM2D_DIM_TIME || vo->get_profile_path_dimension () == MEM2D_DIM_TIME))
                                        for (int t=tstart; t<tend; ++t){
                                                sc->mem2d_time_element (t) -> SetLayer (sc->mem2d->GetLayer ());
                                                scan1d->mem2d->GetDataLineFrom (&p2d[0], &p2d[1], sc->mem2d_time_element (t), &sc->data, &scan1d->data,
                                                                                (GETLINEORGMODE)xsmres.LineProfileOrgMode, 
                                                                                vo->get_profile_path_width(),
                                                                                vo->get_profile_path_step(),
                                                                                vo->get_profile_path_dimension (),
                                                                                vo->get_profile_series_dimension (),
                                                                                vo->get_profile_series_all (),
                                                                                t, tend-tstart, append || section>1);
                                        }
                                else
                                        scan1d->mem2d->GetDataLineFrom (&p2d[0], &p2d[1], sc->mem2d, &sc->data,  &scan1d->data,
                                                                        (GETLINEORGMODE)xsmres.LineProfileOrgMode,
                                                                        vo->get_profile_path_width(),
                                                                        vo->get_profile_path_step(),
                                                                        vo->get_profile_path_dimension (),
                                                                        vo->get_profile_series_dimension (),
                                                                        vo->get_profile_series_all (), 0,0, append || section>1);
                        }
		}


		if (scan1d->mem2d->GetNy() > 1 && vo->get_profile_series_pg2d ()){
			if (!scan1d->view){
				scan1d->view = new Grey2D (scan1d);
				scan1d->SetVM (SCAN_V_DIRECT);
			}
			scan1d->SetVM ();
			scan1d->auto_display ();
		} else {
			if (scan1d->view){
				delete scan1d->view;
				scan1d->view = NULL;
				
			}
		}

		return 0;

	default: return -1;
	}
}


double LineProfile1D::GetPoint(int n, int s) {
	if (scan1d) return scan1d->mem2d->GetDataPkt (n,s); 
	else return 0.; 
}
void LineProfile1D::SetPoint(int n, double y, int s) { if (scan1d) scan1d->mem2d->data->ZSave (y, n, s); }
void LineProfile1D::SetPoint(int n, double x, double y, int s) { 
	if (scan1d) { 
		scan1d->mem2d->PutDataPkt (y, n, s);
		scan1d->mem2d->data->SetXLookup (n, x);   
	} 
}
void LineProfile1D::AddPoint(int n, double a, int s) { if (scan1d) scan1d->mem2d->data->ZaddSave (a, n, s); }
void LineProfile1D::MulPoint(int n, double f, int s) { if (scan1d) scan1d->mem2d->data->ZmulSave (f, n, s); }


// END
