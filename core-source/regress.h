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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/********************************************************
 *							*
 *		regress.h :   				*
 *		Regression				*
 *		Für Polynome nten Grades		*
 *		by Percy Zahl, Version 1.0, 18.2.1994	*
 *
 *
 *
 ********************************************************/

#ifndef __REGRESS__H
#define __REGRESS__H

#include <iostream>
#include <fstream>

#include <cstring>
#include <cmath>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>

#include "mem2d.h"


#define		MAXGL	20
#define		MAXVAR	50
#define		Delta	1E-5
#define		NEG		-1

#define		NO_SOLUTION	0
#define		R_IS_SOLUTION	1
#define		SPEC_SOLUTION	2

#define		FREE	0
#define		UNFREE	1
#define		ONE	2

// #define         TALK

double	gpow(double X, int n);

// Acquiring data and preparing Linear Equation System 
class StatInp;

// LGS - Behandlung

// line contains a solution

class Zeile{
private:
	int		n,m;
	double* gl;	// pointer to coefficients and b : a1 x1 + .. + an xn = b
public:
	double Wert(int i) { return(gl[i]); };
	int Test(int);
	void Mult(double);
	void Add(class Zeile*);
	void P_Zeile();
	Zeile(int, int, StatInp*);
	Zeile(int, int);
	~Zeile() { delete [] gl; };
};

class LGSys{
public:
	int		n,m;			// n x m coefficient matrix
	Zeile*	gln[MAXGL];		// pointer to equation
	int		st_fkt[MAXGL];	// pointer to step function
	int		r;				// rank
	int*	aus;			// selection list
	double* pn[MAXGL];		// pointer to solution

	void Eingabe();
	void Eingabe(StatInp*);
	int SucheGl(int);
	int Umformen();
	int Untersuche(int);
	void Loese(int);
	void Berechne(int);
	void Ausgabe_Loesung(int);
	double CalcPoly(double, int);
	double GetPolyK(int);
	void Ausgabe_Glsys();
	void Tausche(int, int);
	int DoIt() { return Untersuche(Umformen()); };
	void Init(){
	  aus=NULL;
	  for(int i=0; i<MAXGL; ++i){
	    gln[i]=NULL;
	    pn[i]=NULL;
	  }
	};
	LGSys() {  Init(); Eingabe(); Ausgabe_Loesung(Untersuche(Umformen())); };
	LGSys(int N) {  Init(); n = m = N>=MAXGL? MAXGL:N; };
	~LGSys() { 
	  for(int i=0; i<MAXGL; i++) {
	    if(gln[i]) delete gln[i]; 
	    if(pn[i]) delete [] pn[i]; 
	  }
	  if(aus) delete [] aus;
	};
};

class StatInp {
public:
  StatInp(){
    gs=NULL;
    x=y=NULL;
    std::cout << "calculate regression polynom  V1.00 (C) 1994 by PZ\n";
    do{ 
      std::cout << "\nUp to degree?   => "; std::cin >> PGrad; PGrad++;
    }while(PGrad > MAXGL || PGrad < 0);
    PGrad++;
    State=0;
    N=0;
    Read(NULL);
			Optimize();
  };
  StatInp(char *Grad, char *FName, int yx){
    gs=NULL;
    x=y=NULL;
    State=0;
    std::cout << "calculate regression polynom  V1.00 (C) 1994 by PZ\n";
    PGrad=atoi(Grad)+1;
    if(PGrad >= MAXGL || PGrad <= 0)
      State=1;
    else{
      N=0;
      Read(FName, yx);
      Optimize();
    }
  };
  StatInp(int Grad, double *xp, double *yp, int n){
    gs=NULL;
    x=y=NULL;
    State=0;
    PGrad=Grad+1;
    if(PGrad >= MAXGL || PGrad <= 0)
      State=1;
    else{
      N=n;
      x=new double[N];
      y=new double[N];
      memcpy(x, xp, sizeof(double)*N);
      memcpy(y, yp, sizeof(double)*N);
      Optimize();
    }
  }
  StatInp(int Grad, Mem2d *ysrc, int line, 
	  double (*transfkt)(double,double)=NULL, double a0=0.){
    gs=NULL;
    x=y=NULL;
    State=0;
    PGrad=Grad+1;
    if(PGrad >= MAXGL || PGrad <= 0)
      State=1;
    else{
      N=ysrc->GetNx();
      x=new double[N];
      y=new double[N];
      ysrc->data->SetPtr(0,line);
      if(transfkt)
	for(int i=0; i<N; ++i){
	  x[i] = (double)i;
	  y[i] = (*transfkt)(ysrc->data->GetNext(),a0);
	}
      else
	for(int i=0; i<N; ++i){
	  x[i] = (double)i;
	  y[i] = ysrc->data->GetNext();
	}
      Optimize();
    }
  }
  ~StatInp(){ if(x) delete [] x; if(y) delete [] y; if(gs) delete gs; };
  int Result() { return(State); };
  void Read(char *Fn, int yx=0){
    double	X,Y;
    int	i;
    char	DatOutFile[100];
    double	Xfac, Xof,Yfac,Yof,Err;
    char FileName[100], l[1024], *tok;
    Xfac=Yfac=1.; Xof=Yof=0.;
    Err=0.;
    DatOutFile[0]=0;
    std::ifstream IDat;
    std::ofstream ODat;
    if(Fn)
      strcpy(FileName, Fn);
    else{
      std::cout << "Data file ? => "; std::cin >> FileName;
    }
    IDat.open(FileName , std::ios::in);
    if(!IDat.good()){
      std::cout << "\n Cannot find file >>" << FileName << "<< !\n";
      State=2;
      return;
    }
    while(!IDat.eof()){
      IDat.getline(l,1024);
      tok=strtok(l," ;,:|&!");
      if(*tok == '%' || *tok == '*' || *tok == '#') continue;
      if(*tok == '$') break;
      if(*tok == 'A') { Xof=atof(tok+2); continue; }
      if(*tok == 'B') { Xfac=atof(tok+2); continue; }
      if(*tok == 'C') { Yof=atof(tok+2); continue; }
      if(*tok == 'D') { Yfac=atof(tok+2); continue; }
      if(*tok == 'F') { strcpy(DatOutFile,tok+2); continue; }
      if(*tok == 'E') { Err=atof(tok+2); continue; }
      N++;	// Eintraege Zaehlen
    }
    std::cout << "A= " << Xof << " B= " << Xfac << " C= " << Yof << " D= " << Yfac << "\n";
    std::cout << "value count: " << N << "\n\n";
    IDat.close();
    x=new double[N];
    y=new double[N];
    i=0;
    IDat.open(FileName , std::ios::in);
    if(DatOutFile[0]) ODat.open(DatOutFile, std::ios::out);
    if(!IDat.good()){
      std::cout << "\n Cannot find file >>" << FileName << "<<  !\n";
      State=2;
      return;
    }
    std::cout << "\\begin{tabular}{|r|r|}\\hline \\\\\n X   &   Y\\hline\\\\\n";
    if(DatOutFile[0])
      ODat << "# Regres Daten\n"
	   << "#\\begin{tabular}{|r|r|}\\hline \\\\\n# X   &   Y  &  Err\\hline\\\\\n";
    while(IDat.good() && i < N){
      IDat.getline(l,1024);
      tok=strtok(l," ;,:&|!");
      if(*tok == '%' || *tok == '*' || *tok == '#') continue;
      if(*tok == 'A' || *tok == 'B' || *tok == 'C' || *tok== 'D') continue;
      if(*tok == 'F' || *tok == 'E' || *tok == 'G' || *tok== 'H') continue;
      if(*tok == '$') break;
      if(tok){
	X = atof(tok)*Xfac+Xof;
	tok=strtok(0," ;,&:|!");
	if(tok)
	  Y = atof(tok)*Yfac+Yof;
	else{
	  Y = 0.;
	  std::cout << "Y-error in data line: " << i << " !\n";
	  State=3;
	}
	if(yx){
	  y[i]=X; x[i]=Y;
	}else{
	  x[i]=X; y[i]=Y;
	}
      }
      else{
	x[i]=y[i]=0.;
	std::cout << "X-error in data line: " << i << " !\n";
	State=3;
      }
      std::cout << " " << x[i] << "  &   " << y[i] << " \\\\\n";
      if(DatOutFile[0]) 
	ODat << x[i] << " " << y[i] << " " << Err << " \\\\\n";
      
      ++i;
    }
    std::cout << " & \\hline\\end{tabular}\n";
    if(DatOutFile[0]){ 
      ODat << "# & \\hline\\end{tabular}\n";
      ODat.close();
    }
    IDat.close();
  }
  void Optimize();
  double CalcKoef(int k, int j){	// Br.: S.787
/*
			   _N_
			   \
	[ f(x) ] :=         )   f(x )		// Gauss'ian-brace
			   /       i
			   ~~~
			   i=1
	LGS:

general:
	f(x;a ,a ,...,a   ) = a f (x) + a f (x) + ... + a   f   (x)
	     0  1      n-1     0 0       1 1             n-1 n-1

	n-1
	___
	\
	 )	a  [ f (x) f (x) ]  =  [ y f (x) ]		(j=0,1,.. n-1
	/        k    k     j               j
	~~~
	k=0

for polynom:
		 k
	f (x) = x
	 k

  Pdegree-1
	___
	\             k+j           j
	 )	a  [ x   ]  =  [ y x  ]		  (j=0,1,...PGrad-1)
	/        k
	~~~
	k=0
*/
    int	i;
    double ckj;
    if(k<PGrad)	// lhs: coefficient
      for(ckj=0., i=0; i<N; i++) ckj += gpow(x[i],k+j);
    else	// rhs: constants.
      for(ckj=0., i=0; i<N; i++) ckj += y[i]*gpow(x[i],j);
    return ckj;
  };

  double GetPoly(double x){
    return gs->CalcPoly(x, -1);
  };

  double GetPolyKoef(int i){
    return gs->GetPolyK(i);
  };

private:
  int		N,PGrad;
  int		State;
  double	*x, *y;
  double	a[MAXGL];
  LGSys *gs;
};

#endif
