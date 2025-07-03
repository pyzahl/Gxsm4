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

// Global set of shared PCs for preview/readback

#include <stdlib.h>
#include "gxsm_window.h"
#include "app_vpdata_view.h"
#include "view.h"

#define VPDATA_SKIP_LAST_TWO

ProfileControl *tmp_pc[32] = { 
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL
};


app_vpdata_view *vpdata_graph_view = NULL;


int vpdata_read (Gxsm4app *app, const gchar *fname, Scan *active_scan){
	gchar l[1100];
	std::ifstream f;

	if (fname == NULL){
		//clean up PCs
		for (int i=0; i<32; ++i){
			if (tmp_pc[i])
				delete tmp_pc[i];
			tmp_pc[i] = NULL;
		}
		return 1;
	}
	
	f.open(fname, std::ios::in);
	if(!f.good()){
		//    fl_show_alert(ERR_SORRY, ERR_FILEREAD,fname,1);
		return 1;
	}

	// Check for VectorProbe file type
	if (strstr(fname, ".vpdata")){

		std::cout << "VP DATA READ" << std::endl;
		
		if (f.good())
			f.getline(l, 1024);
		else
			return 1;
		if (f.good())
			f.getline(l, 1024);
		else
			return 1;
		if (strncmp(l, "# GXSM Vector Probe Data :: VPVersion=00.02", 43) == 0 ||
                    strncmp(l, "# GXSM Vector Probe Data :: VPVersion=00.03", 43) == 0) {
			double MainOffsetX0=0., MainOffsetY0=0.;
			// vpdata file type/version double check OK :-)

// # GXSM-Main-Offset       :: X0=-528.716 Ang  Y0=1165.1 Ang, iX0=75 Pix iX0=75 Pix
// # GXSM-DSP-Control-FB    :: Bias=-0.7 V, Current=0.5 nA

			// find GXSM Main info and skip fwd to Vector Position Table
			while (f.good()){
				f.getline(l, 1024);
				if (strncmp(l, "# GXSM-Main-Offset", 18) == 0){
					gchar **GXSMmainData = g_strsplit (l, " ", 20);
					gchar **toc = GXSMmainData;
					for (; *toc; toc++){
						if (strncmp(*toc, "X0=", 3) == 0)
							MainOffsetX0 = atof (*toc+3);
						if (strncmp(*toc, "Y0=", 3) == 0)
							MainOffsetY0 = atof (*toc+3);
					}
				}

				if (strncmp(l, "#C Full Position Vector List", 28) == 0)
					break;
			}


			std::cout << "VP DATA FOUND IS VALID" << std::endl;

			// scan vector table
			int nvsec = 0;
			gchar sec_position_vector[64][1024];
			double svXS[64], svYS[64], svZS[64], svtime[64];
			int svindexi[64];
			int i=0;
			while (i < 64 && f.good()){
                                if (i >= 63){
                                        g_message ("WARNING: GVP MAX GVP POSITON VECTOR LIMIT of 64 FOR READ BACK EXCEEDED.");
                                        return 1;
                                }
				svXS[i] = svYS[i] = svZS[i] = svtime[i] = 0.;
				svindexi[0]=-1;
				f.getline (sec_position_vector[i], 1024);
				if (strncmp(sec_position_vector[i], "# S[", 4) == 0){
					gchar **secvec = g_strsplit (sec_position_vector[i], " ", 30);
					gchar **svtoc = secvec;
					for (; *svtoc; svtoc++){
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
                                g_print ("SecVecTable[%d]: %d [#%d t=%g XYZ=(%g %g %g)\n]", i, nvsec, svindexi[i], svtime[i], svXS[i], svYS[i], svZS[i]);
			}

			std::cout << "VP DATA -- SECTAB OK: nvsec=" << nvsec << std::endl;

			while (f.good()){
				f.getline(l, 1024);
				if (strncmp(l, "#C Data Table", 13) == 0)
					break;
			}

			std::cout << "VP DATA -- DATA TABLE FOUND" << std::endl;

			gchar column_header_labels[1024];
			f.getline(column_header_labels, 1024);

			std::cout << "VP DATA -- READ HEADERS" << std::endl;

			std::cout << "READING DATA PASS 1 -- COUNTING, FINDING X RANGE" << std::endl;

			int numx = 0;
			double x, xmin=0., xmax=0.;
			while (f.good()){
				f.getline(l, 1024);
				if (strncmp(l, "#C END.", 7) == 0) break; // EOF mark OK.
				if (strncmp(l, "#C", 2) == 0) continue;

				g_strdelimit (l, "\t", ' ');
				gchar **VPrecord = g_strsplit(l, " ", 2);
				gchar **token = VPrecord;
				x = atof (*++token);
				if (numx == 0)
					xmin = xmax = x;
				else{
					if (x > xmax) xmax = x;
					if (x < xmin) xmin = x;
				}

				numx++; 
			}

			std::cout << "VP DATA -- FOUND SETS #" << numx << " : " << xmin << " .. " << xmax << std::endl;

			std::cout << "SETTING UP PROFILE AND ATTACHIONG SEs" << std::endl;

			f.close (); // reopen, reseek

			GPtrArray *glabarray = g_ptr_array_new ();
			GPtrArray *gsymarray = g_ptr_array_new ();

			gchar **Hrecord = g_strsplit(column_header_labels, "\t", 100);
			gchar **token = Hrecord;
			int j;
			UnitObj *xuobj = NULL;
			int join_same_x = FALSE;
			int num_sets = 0;

                        for (token=Hrecord; *token; ++token){
				if (! strcmp (*token, "#C Index"))
					continue;
				if (! strcmp (*token, "Block-Start-Index"))
					break;
#ifdef VPDATA_SKIP_LAST_TWO
                                if (token[1]){
                                        if (token[2]){
                                                if (! strcmp (token[2], "Block-Start-Index")){
                                                        g_print ("num_sets=%d, [2]=Block-Start-Index [0]=%s [1]=%s\n", num_sets, token[0], token[1]);
                                                        // [2]=Block-Start-Index [0]="Time (ms)" [1]="Bias (V)"
                                                        if (! strcmp (token[0], "Time (ms)") && ! strcmp (token[1], "Bias (V)")){
                                                                g_print ("skipping copies of \"%s\" and \"%s\".\n", token[0], token[1]);
                                                                break;
                                                        }
                                                }
                                        }
                                }
#endif
                                ++num_sets;
                        }
                        g_print("num_sets=%d\n", num_sets);

                        g_message("** new app vpdata view");
                        if (!vpdata_graph_view)
                                vpdata_graph_view = new app_vpdata_view (app, 1, num_sets-1);
                        else
                                vpdata_graph_view->init_vpdata_view (1, num_sets-1);

			for (j=0, token=Hrecord; *token && j < num_sets; ++token){
                                g_message("** read token %d ** %s", j, token);
				if (! strcmp (*token, "#C Index"))
					continue;
				if (! strcmp (*token, "Block-Start-Index"))
					break;

				std::cout << "VP DATA -- H:" << j << " = " << *token << std::endl;

				UnitObj *uobj=NULL;
				g_strdelimit (*token, "\")", ' ');
				gchar **lu = g_strsplit(*token, "(", 2);

				gchar *lu0 = g_strescape (g_strstrip(lu[0]), "");
				for(char *c=lu0; *c; ++c)
					if (*c == ' ')
						*c = '_';
				if (lu[1]){
					std::cout << "VP DATA -- HTOK:" << lu0 << "<=" << lu[0] << " [" << lu[1] << "]" << std::endl;
				}else
					std::cout << "VP DATA -- HTOK:" << lu[0] << " [??]" << std::endl;

				uobj = new LinUnit (lu[1]?g_strstrip(lu[1]):"??", lu[1]?g_strstrip(lu[1]):"??", lu0, 1.0);

				std::cout << "VP DATA -- UOK." << std::endl;

				int nas = 1;

				if (j==0)
					xuobj = uobj;
				else if (j < 32){
					int jj = join_same_x ? 0:j-1;
					if (!tmp_pc[jj]){
						gchar   *title  = g_strdup_printf ("VP File: %s : %s (%g, %.2f A, %.2f A, %.2f A)", fname, lu[0], svtime[0], svXS[0], svYS[0], svZS[0] );
						std::cout << "VP DATA -- t:" << title << std::endl;

                                                vpdata_graph_view->vpdata_add_pc (tmp_pc[jj], title, numx,
                                                               lu0, xuobj,
                                                               lu[0], uobj,
                                                               xmin, xmax,
                                                               0, nas, join_same_x, // si, nas, join_same_x
                                                               1, j, 1, num_sets-1);
						g_free (title);

					} else {	
		
						gchar   *title  = g_strdup_printf ("VP File: %s : %s (%g, %.2f A, %.2f A, %.2f A)", fname, lu[0], svtime[0], svXS[0], svYS[0], svZS[0] );
                                                vpdata_graph_view->vpdata_add_pc (tmp_pc[jj], title, numx,
                                                               lu0, xuobj,
                                                               lu[0], uobj,
                                                               xmin, xmax,
                                                               0, nas, join_same_x, // si, nas, join_same_x
                                                               1, j, 1, num_sets-1);
						g_free (title);
					}
				}

				g_ptr_array_add (glabarray, (gpointer) g_strdup (lu0));
				if (lu[1])
					g_ptr_array_add (gsymarray, (gpointer) g_strdup (lu[1]));
				else
					g_ptr_array_add (gsymarray, (gpointer) g_strdup ("??"));

				g_free (lu0);

				if (lu)
					g_strfreev (lu);

				++j;
			}
			g_strfreev (Hrecord);

			std::cout << l << std::endl;
			std::cout << "VP DATA -- RESEEK" << std::endl;

			f.open(fname, std::ios::in);
			while (f.good()){
				f.getline(l, 1024);
				if (strncmp(l, "#C Data Table", 13) == 0){
					f.getline(l, 1024); // skip one more (headers
					break;
				}
			}

			std::cout << "VP DATA -- PE,SE SETUP*" << std::endl;

			double dataset[32];
			ScanEvent *se = NULL;
			ProbeEntry *pe = NULL;
			if (active_scan){
				se = new ScanEvent (MainOffsetX0 + svXS[0], MainOffsetY0 + svYS[0], 0,0, svZS[0]);
				pe = new ProbeEntry ("Probe", svtime[0], glabarray, gsymarray, num_sets, fname);
				active_scan->mem2d->AttachScanEvent (se);
			}

			std::cout << "VP DATA -- READING DATA: numx= " << numx << std::endl;
			// put data
			for(int i=0; i<numx && f.good(); ++i){
				double x=0., y=0.;
				f.getline (l, 1024);
//				std::cout << i << ":" << l << std::endl;
				g_strdelimit (l, "\t", ' ');
				gchar **VPrecord = g_strsplit_set(l, " \t", 100);
				gchar **token = VPrecord;
//				std::cout << "VP DATA[" << i << "]: " << l << std::endl;
//				std::cout << l << " => token0=" << VPrecord[0] << "," << VPrecord[1] << "..::" << **VPrecord << std::endl;
				for (int j=-1; *token && j <= 32; ++token, ++j){
//					std::cout << "A" << std::endl;
					if (j<0) continue;
					if (j==0){
//						std::cout << "Aj0" << std::endl;
						x = atof (*token);
						dataset[0] = x;
//						std::cout << "X=" << x << std::endl;
					} else {
//						std::cout << "Aelse" << std::endl;
						y = atof (*token);
//						std::cout << "Aelse y=" << y << std::endl;
						if (tmp_pc[j-1])
							tmp_pc[j-1]->SetPoint (i, x, y, join_same_x ? (j-1):0);
						dataset[j] = y;
//						std::cout << "Y[" << i << "]_" << j << "=" << y << std::endl;					}
//						std::cout << "Aforend" << std::endl;
					}
				}
//				std::cout << "Bpe" << std::endl;
				if (pe)
					pe->add ((double*)&dataset);
				g_strfreev (VPrecord);
                                if (j>0)
                                        if (tmp_pc[j-1])
                                                tmp_pc[j-1]->UpdateArea ();
			}
			std::cout << "VP DATA -- DATA OK" << std::endl;
			if (se)
				se->add_event (pe);

			if (active_scan)
				if (active_scan->view)
					active_scan->view->update_events ();

			std::cout << "VP DATA -- UPDATE VIEWS" << std::endl;
#if 0
			for (int j=0; j <= 32; ++j){
				if (tmp_pc[j]){
					tmp_pc[j]->drawScans ();
					tmp_pc[j]->UpdateArea ();
					tmp_pc[j]->show ();
				}
			}
#endif
			std::cout << "VP DATA -- READING DATA OK" << std::endl;

			f.close();
			XSM_DEBUG (DBG_L3, "LineProfile1D::load [VP Data Mode] - Done.");
			return 0;
		}else{
			XSM_DEBUG (DBG_L3, "VP file type check failed or not VPVer=00.02");
			f.close();
			return 1;
		}
	}
	f.close();
	return 1;
}
