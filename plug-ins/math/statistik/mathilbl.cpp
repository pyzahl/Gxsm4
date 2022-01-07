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

/*@
 *@	File:	mathilbl.C
 *@     $Header: /home/ventiotec/gxsm-cvs/Gxsm-2.0/plug-ins/math/statistik/mathilbl.C,v 1.5 2003-08-23 15:45:49 zahl Exp $
 *@	Datum:	25.10.1995
 *@	Author:	M.Bierkandt
 *@	Zweck:	higher Math-Routines
 *@
 *@
 *@
 *@	Funktionen:
 *@
 *@
 */

/* irnore this module for docuscan
% PlugInModuleIgnore
 */

#include "xsmmath.h"
#include "xsmtypes.h"
#include "glbvars.h"
#include "stdio.h"

#include "bench.h"
#include "regress.h"

#include "mathilbl.h"

#include <math.h>


// Inselstatistiken.... MB
// ============================================================

double d_sa=0.;
double d_pr=0.;
double d_lbn=5.;
int inter=1;
gchar *filename=NULL;
gchar *simname=NULL;
int save=0;

long proper(long cl_link[], long label){
  if (cl_link[label]==label) 
    return label;
  else 
    return proper(cl_link, cl_link[label]);
}

int SF(int line, int n_l, int sa){
  if (sa <=0) return 0;
  if (sa >0){
    int TW= n_l / sa;
    for (int s=0; s<sa; s++){
    if ( line==(s+1)*TW ) return -1;
    if ( line==s*TW+1 ) return 1;
    }
  }
  return 0;
}


void labeling(ZData *ZData_Ptr, int n_c, int n_l, long Nmax, long cl_link[], int sa, int step_cb){

  long n_cluster=0 , site, siteL, siteT, siteLT, siteRT,  min1, min2;
 
  for (int line=1; line < n_l+1; line++) {
    ZData_Ptr->SetPtrT(1,line); // Setzt Pointer in Zeile und in darüberliegenden
    for (int col = 1; col < n_c+1; col++){
      site   =  (long)ZData_Ptr->GetThis();
      siteL  =  (long)ZData_Ptr->GetThisL();
      siteT  =  (long)ZData_Ptr->GetThisT();
      siteRT =  (long)ZData_Ptr->GetThisRT();
      if ( (SF(line, n_l, sa)==1) && (step_cb==0) ){         // keine chemische Bindung vom unteren zum
      	siteT=0;                                              // oberen Stufenplatz        
      	siteRT=0;
      }
      if (site < 0)  // Platz ist besetzt
        if ( (siteL==0) && (siteT==0) && (siteRT==0) ) {
          n_cluster++;             // neuer Cluster
          ZData_Ptr->SetThis(n_cluster);
          cl_link[n_cluster]=n_cluster;
        }
        else {    // neighbors checken
	  if (siteT==0) siteT=Nmax; else siteT=proper(cl_link, siteT);
          if (siteL==0) siteL=Nmax; else siteL=proper(cl_link, siteL);
	  if (siteRT==0) siteRT=Nmax; else siteRT=proper(cl_link, siteRT);
	  min1=MIN(siteT, siteL);
	  min2=MIN(siteL, siteRT);
          ZData_Ptr->SetThis(MIN(min1, min2));
	  site   =  (long)ZData_Ptr->GetThis();
	  if ((siteT < Nmax) && (siteT > site)) cl_link[(long) ZData_Ptr->GetThisT()]=site; 
          if ((siteL<Nmax)   && (siteL > site)) cl_link[(long) ZData_Ptr->GetThisL()]=site;
	  if ((siteRT<Nmax)  && (siteRT> site)) cl_link[(long) ZData_Ptr->GetThisRT()]=site; 
        }
      ZData_Ptr->IncPtrT();   // Inkrementiert Pointer in Zeile und der darüberliegenden  
    }
  }
}

void proper_labeling(ZData *ZData_Ptr, int n_c, int n_l, long cl_link[]){
  
    for (int line=1; line < n_l +1; line++) {
    ZData_Ptr->SetPtr(1,line);
    for (int col = 1; col < n_c +1; col++)
            ZData_Ptr->SetNext( proper(cl_link, (long)ZData_Ptr->GetThis()) );
  }
}


double deltaz(int sa, int n_l, int line){
  
  int TW;
  if (sa <= 0) return 0;
  TW=(int) (n_l/sa);
  for (int s=0; s<sa; s++) 
    if ((line > s*TW) && (line <= (s+1)*TW))
      return (double) (s*256);
}

void sub_stepfunc(ZData *ZData_Ptr, int n_c, int n_l, int sa){
  
  double dz;
  for (int line=1; line <= n_l; line++){
    dz=deltaz(sa, n_l, line);
    ZData_Ptr->SetPtr(1,line);
    for (int col=1; col <= n_c; col++)
      ZData_Ptr->SetNext(ZData_Ptr->GetThis()-dz);
  } 
}

void rescale(ZData *ZData_Ptr, double low, double high, int n_c, int n_l){

  double skl;

  // skl=high-low;
  skl=256.;   // für Mats KMC-Simulationen
  for (int line=1; line <= n_l; line++){
    ZData_Ptr->SetPtr(1,line);
    for (int col=1; col <= n_c; col++)
      ZData_Ptr->SetNext( (ZData_Ptr->GetThis()-low)/skl*(-1) );
  } 
}

void cluster_link_col(ZData *ZData_Ptr, int n_c, int n_l, long cl_link[]){

  double zl, zr;
  for (int line=1; line <= n_l; line++){
  ZData_Ptr->SetPtr(1,line);
  zl=ZData_Ptr->GetThis();
  ZData_Ptr->SetPtr(n_c+1,line);
  zr=ZData_Ptr->GetThis();
  if (zl < zr) cl_link[(long)zr]=(long)zl;
  if (zl > zr) cl_link[(long)zl]=(long)zr;
  }
}

void cluster_link_line(ZData *ZData_Ptr, int n_c, int n_l, long cl_link[]){

  double zo, zu;
  for (int col=1; col <= n_c; col++){
  ZData_Ptr->SetPtr(col,1);
  zo=ZData_Ptr->GetThis();
  ZData_Ptr->SetPtr(col, n_l+1);
  zu=ZData_Ptr->GetThis();
  if (zo < zu) cl_link[(long)zu]=(long)zo;
  if (zo > zu) cl_link[(long)zo]=(long)zu;
  }
}

void LoHi(ZData *ZData_Ptr, int n_c, int n_l, double *p_low, double *p_high){
  
    ZData_Ptr->SetPtr(1,1);
    *p_low=ZData_Ptr->GetThis();
    *p_high=ZData_Ptr->GetThis();
    for (int line=1; line <= n_l; line++){
      ZData_Ptr->SetPtr(1,line);
      for (int col=1; col <= n_c; col++){
	if (ZData_Ptr->GetThis() < *p_low) *p_low=ZData_Ptr->GetThis();
	if (ZData_Ptr->GetThis() > *p_high) *p_high=ZData_Ptr->GetThis();
	++(*ZData_Ptr);
      }
    } 
}


gboolean IslandLabl(MATHOPPARAMS){
  
  double high, low, skl, z, site;
  int n_lines=Src->mem2d->GetNy(), n_cols=Src->mem2d->GetNx();
  long Nmax=n_lines*n_cols/3;
  int sa=0, pr, sf, sz[Nmax], lbn, sim_nr; 
  long size[Nmax], n_stable=0 , n_monomer=0, amount=0;
  long cl_link[Nmax];
  ZData  *SrcZ, *HIdx, *DstZ;
  FILE *filePtr;
  gchar *simfile;

  Mem2d HIndex(n_cols+3, n_lines+3, ZD_SHORT);  // Height Index Field
  
  // Initialisierung der Variablen
  
    
  for (long j=0; j < Nmax+1; j++){
    cl_link[j]=0; sz[j]=0; size[j]=0;
  }
  SrcZ  = Src->mem2d->data;
  DstZ  = Dest->mem2d->data;
  HIdx  = HIndex.data;

  Dest->data.s.nx=Dest->data.s.nx +3;  //Dest-Feld wird um 3 lines und colums groesser
  Dest->data.s.ny=Dest->data.s.ny +3;
  Dest->mem2d->Resize(Dest->data.s.nx, Dest->data.s.ny);
  

  
  PI_DEBUG (DBG_L2, "F2D_IslandLabl");

  if (inter==1){
    // Eingabe der Randbedingungen
    gchar *txt1 = g_strdup_printf("Periodische RB: ohne(-1), x+y(0), x(1)");
    main_get_gapp()->ValueRequest("Enter Value", "RB", txt1, main_get_gapp()->xsm->Unity, -1., 1., "g", &d_pr);
    g_free(txt1); 
    // Eingabe der Stepamounts (wegen Unterbindung Bindung über Stufen)
    gchar *txt2 = g_strdup_printf("Stufenanzahl des Orginal-Substrates");
    main_get_gapp()->ValueRequest("Enter Value", "sa", txt2, main_get_gapp()->xsm->Unity, 0.,512., "g", &d_sa);
    g_free(txt2); 
  }
  sa= (int) d_sa;
  pr= (int) d_pr;

  
  // Copy der Daten aus Src in HIdx
  for (int line=0; line < Src->mem2d->GetNy(); line++) {
    SrcZ->SetPtr(0,line);
    HIdx->SetPtr(1,line+1);
    for (int col = 0; col < Src->mem2d->GetNx(); col++)
      HIdx->SetNext( SrcZ->GetNext() );
  }
  
  
  // Reskalierung
  LoHi(HIdx, n_cols, n_lines, &low, &high);
  skl = high-low;
  PI_DEBUG (DBG_L2, "F2D_IslandLabl" << std::endl << high << " " << low);
  rescale(HIdx, low, high, n_cols, n_lines);
  
  // Periodische Randbedingungen 
    // 1.Spalte in (n_cols+1) -te kopieren
  for (int line=1; line <= n_lines; line++){
    HIdx->SetPtr(1,line);
    z=HIdx->GetThis();
    HIdx->SetPtr(n_cols+1,line);
    HIdx->SetThis(z);
  }
    // 1.Zeile in (n_lines+1) -te kopieren
  for (int col=1; col <= n_cols; col++){
    HIdx->SetPtr(col, 1);
    z=HIdx->GetThis();
    HIdx->SetPtr(col, n_lines+1);
    HIdx->SetThis(z);
  }

  // erstes labeling
  labeling(HIdx, n_cols+1, n_lines+1, Nmax, cl_link, sa, 0);
  
  
  // zusätzliche links für periodischen RB
  if (pr > -1){
    cluster_link_col(HIdx, n_cols, n_lines, cl_link);
    if (pr == 0) 
      cluster_link_line(HIdx, n_cols, n_lines, cl_link);
  }

  // proper labeling
  proper_labeling(HIdx, n_cols+1, n_lines+1, cl_link);
  

  // **********************
  // Anfang Auswertung
  // **********************

  // Clustergrößen ermitteln, liefert clustersize[clusternumber]
  for (int line=1; line <= n_lines; line++){
      HIdx->SetPtr(1,line);
      for (int col=1; col <= n_cols; col++){
	if (HIdx->GetThis()>0) size[(int)HIdx->GetThis()]++;
	++(*HIdx);
      }
   } 

  // Anzahl der Inseln ermitteln
  for (int j=1; j<=Nmax; j++){
    if (size[j]>1)  n_stable++;
    if (size[j]==1) n_monomer++; 
  }

  // Gesamtmasse berechnen
  for (int j=1; j<=Nmax; j++)
     amount=amount+size[j]; 

  double d_n_stable= (double) n_stable;
  double d_n_monomer= (double) n_monomer;
  double d_amount= (double) amount;
  double d_n_stable_tr=(double) n_stable /  ( (double) (n_lines * n_cols) )  *1000000;
 
  std::cout << "\n\n************************Statistik************************";
  std::cout << "\n# stabile Inseln: " << n_stable << " rel. Fehler: " << 1/sqrt(n_stable)  <<"\n";
  printf("\t entspricht  (%.2f" , (double) n_stable /  ( (double) (n_lines * n_cols) )  *1000000);
  printf(" +- %.2f) / 10^6 sites" , (double) n_stable /  ( (double) (n_lines * n_cols) )*1000000 * 1/sqrt(n_stable));
  std::cout << "\n# Monomere: " << n_monomer << " rel. Fehler: " << 1/sqrt(n_monomer); 
  std::cout << "\n Bedeckung: " << amount << " rel. Fehler: " << 1/sqrt(amount)  << "\n";
  std::cout << "*********************************************************\n\n"; 
  std::cout << "Save: " << save << "\n";

  // Ausgabe der Statistik in Datei
  
  if (save==1){
    lbn= (int) d_lbn;
    simfile = g_strdup(strrchr(Src->data.ui.name,'/')+1);
    std::cout << "\n" << simfile << "\n";
    std::cout << "\n" << g_strndup(simfile+lbn, strlen(simfile)-lbn-4) << "\n";
    sim_nr=atoi( (const char *)  g_strndup(simfile+lbn, strlen(simfile)-lbn-4));    
      if ( (filePtr=fopen( (const char *) filename, "a+")) == NULL)
	  printf("File could not be opened\n");
      else {
	  fprintf(filePtr, "%.0f\t%.3f\t", d_n_stable, 1/sqrt(d_n_stable) );
          fprintf(filePtr, "%.1f\t", d_n_stable_tr);
          fprintf(filePtr, "%.1f\t", d_n_stable_tr * 1/sqrt(d_n_stable) );
          fprintf(filePtr, "%.0f\t", d_n_monomer);
          fprintf(filePtr, "%.3f\t", 1/sqrt(d_n_monomer) );
          fprintf(filePtr, "%.0f\t", d_amount);
          fprintf(filePtr, "%.5f\t", 1/sqrt(d_amount) );
	  fprintf(filePtr, "%d\n", sim_nr );
          printf("saving data\n");
          fclose(filePtr);
          printf("closing file\n");
      } 
  }
     
  // **********************
  // Ende Auswertung
  // **********************

  Dest->mem2d->ConvertFrom(&HIndex, 0, 0, 0, 0, Dest->mem2d->GetNx(),Dest->mem2d->GetNy());
     
return MATH_OK;
}



gboolean StepFlaten(MATHOPPARAMS){
  
  int  n_lines=Src->mem2d->GetNy(), n_cols=Src->mem2d->GetNx(), sa;
  ZData  *SrcZ, *HIdx, *DstZ;
  
  Mem2d HIndex(n_cols+1, n_lines+1, ZD_SHORT);  // Height Index Field
 
  // Initialisierung der Variablen
  SrcZ  = Src->mem2d->data;
  DstZ  = Dest->mem2d->data;
  HIdx  = HIndex.data;


  // Copy der Daten aus Src in HIdx
  for (int line=0; line < Src->mem2d->GetNy(); line++) {
    SrcZ->SetPtr(0,line);
    HIdx->SetPtr(1,line+1);
    for (int col = 0; col < Src->mem2d->GetNx(); col++)
      HIdx->SetNext( SrcZ->GetNext() );
  }

  if (inter==1){
  // Eingabe der Stepamounts
  gchar *txt = g_strdup_printf("Stepamount: ");
  main_get_gapp()->ValueRequest("Enter Value", "Stepamount", txt, main_get_gapp()->xsm->Unity, 1., 512., "g", &d_sa);
  g_free(txt); 
  }
  sa= (int) d_sa;

  // Stufenfunktion abziehen, falls sa>0
  if (sa > 0) sub_stepfunc(HIdx, n_cols, n_lines, sa);

  
  Dest->mem2d->ConvertFrom(&HIndex, 1, 1, 0, 0, Dest->mem2d->GetNx(),Dest->mem2d->GetNy());

  return MATH_OK;
}

gboolean KillStepIslands(MATHOPPARAMS){
  
  double high, low, skl, z, site;
  int n_lines=Src->mem2d->GetNy(), n_cols=Src->mem2d->GetNx();
  long Nmax=n_lines*n_cols/3; 
  int pr=1, sa, sf, sz[Nmax], amount=0;
  long cl_link[Nmax];
  ZData  *SrcZ, *HIdx, *DstZ, *HIdxTemp;
  FILE *filePtr;
  
  Mem2d HIndex(n_cols+1, n_lines+1, ZD_SHORT);  // Height Index Field
  Mem2d HIndexTemp(n_cols+3, n_lines+3, ZD_SHORT);  // Height Index Field temporär

  // Initialisierung der Variablen
  for (long j=0; j < Nmax+1; j++){
    cl_link[j]=0; sz[j]=0; 
  }
  SrcZ  = Src->mem2d->data;
  DstZ  = Dest->mem2d->data;
  HIdx  = HIndex.data;
  HIdxTemp  = HIndexTemp.data;


  // Copy der Daten aus Src in HIdxTemp
  for (int line=0; line < Src->mem2d->GetNy(); line++) {
    SrcZ->SetPtr(0,line);
    HIdxTemp->SetPtr(1,line+1);
    for (int col = 0; col < Src->mem2d->GetNx(); col++)
      HIdxTemp->SetNext( SrcZ->GetNext() );
  }

  // Copy der Daten aus Src in HIdx
  for (int line=0; line < Src->mem2d->GetNy(); line++) {
    SrcZ->SetPtr(0,line);
    HIdx->SetPtr(1,line+1);
    for (int col = 0; col < Src->mem2d->GetNx(); col++)
      HIdx->SetNext( SrcZ->GetNext() );
  }
  
  if (inter==1){
  // Eingabe der Randbedingungen
  gchar *txt1 = g_strdup_printf("Periodische RB: ohne(-1), x+y(0), x(1)");
  main_get_gapp()->ValueRequest("Enter Value", "RB", txt1, main_get_gapp()->xsm->Unity, -1., 1., "g", &d_pr);
  g_free(txt1); 
  // Eingabe der Stepamounts
  gchar *txt2 = g_strdup_printf("Stepamount: ");
  main_get_gapp()->ValueRequest("Enter Value", "Stepamount", txt2, main_get_gapp()->xsm->Unity, 1., 256., "g", &d_sa);
  g_free(txt2); 
  }
  sa= (int) d_sa;
  pr= (int) d_pr;

    // Reskalierung
  LoHi(HIdxTemp, n_cols, n_lines, &low, &high);
  skl = high-low;
  std::cout << "F2D_IslandLabl" << std::endl << high << " " << low << std::endl;
  rescale(HIdxTemp, low, high, n_cols, n_lines);
  
  // Periodische Randbedingungen 
    // 1.Spalte in (n_cols+1) -te kopieren
  for (int line=1; line <= n_lines; line++){
    HIdxTemp->SetPtr(1,line);
    z=HIdxTemp->GetThis();
    HIdxTemp->SetPtr(n_cols+1,line);
    HIdxTemp->SetThis(z);
  }
    // 1.Zeile in (n_lines+1) -te kopieren
  for (int col=1; col <= n_cols; col++){
    HIdxTemp->SetPtr(col, 1);
    z=HIdxTemp->GetThis();
    HIdxTemp->SetPtr(col, n_lines+1);
    HIdxTemp->SetThis(z);
  }

  // erstes labeling
  labeling(HIdxTemp, n_cols+1, n_lines+1, Nmax, cl_link, sa, 0);
  
  
  // zusätzliche links für periodischen RB (temporär ist pr noch fest 0)
  if (pr > -1){
    cluster_link_col(HIdxTemp, n_cols, n_lines, cl_link);
    if (pr == 0) 
      cluster_link_line(HIdxTemp, n_cols, n_lines, cl_link);
  }

  // proper labeling
  proper_labeling(HIdxTemp, n_cols+1, n_lines+1, cl_link);
  
   // für Inselnummer die Stufenzugehörigkeit checken
  if (sa>0){  
    for (int line=1; line <= n_lines; line++){
      HIdxTemp->SetPtr(1,line);
      sf=SF(line, n_lines, sa);
      for (int col=1; col <= n_cols; col++){
	if (sf==-1) sz[(int)HIdxTemp->GetThis()]=1; 
	HIdxTemp->IncPtrT();
      }
    }
    sz[0]=0;
  } 

 // step islands in Hidx eliminieren und zählen

  for (int line=1; line <= n_lines; line++){
    HIdxTemp->SetPtr(1,line);
    HIdx->SetPtr(1,line);
    for (int col=1; col <= n_cols; col++){
      z=HIdx->GetThis();
      if (sz[(int)HIdxTemp->GetThis()]==1){
	amount++; 
	HIdx->SetThis(z-256.);
      }
      ++(*HIdxTemp);
      ++(*HIdx);
    }
  }
  

  double d_amount= (double) amount;
  std::cout << "\n\n************************Statistik************************";
  std::cout << "\nBedeckung an Stufen: " << amount << " rel. Fehler: " << 1/sqrt(amount)  <<"\n";
  std::cout << "*********************************************************\n\n";
  std::cout << "Save: " << save << "\n";

   // Ausgabe der Statistik in Datei
  
  if (save==1){
      if ( (filePtr=fopen( (const char *) filename, "a+")) == NULL)
	  printf("File could not be opened\n");
      else {
	  fprintf(filePtr, "%.0f\t", d_amount);
          fprintf(filePtr, "%.5f\t", 1/sqrt(d_amount) );
          printf("saving data\n");
          fclose(filePtr);
          printf("closing file\n");
      } 
  }

  Dest->mem2d->ConvertFrom(&HIndex, 1, 1, 0, 0, Dest->mem2d->GetNx(),Dest->mem2d->GetNy());

  return MATH_OK;
}
