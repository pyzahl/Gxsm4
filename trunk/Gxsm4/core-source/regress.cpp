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
 *		REGRES.CPP :   				*
 *		Regression				*
 *		for polynome nth order	          	*
 *		by Percy Zahl, Version 1.0, 18.2.1994	*
 ********************************************************/

#include <iostream>
#include <fstream>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cstdio>

#include "regress.h"

// be verbose?
// #define TALK

double	gpow(double X, int n)
{	double	px=1.;
	if(n > 0)
		while(n--) px*=X;
	else
		if(n < 0)
			while(n++) px/=X;
	return px;
}


void StatInp::Optimize(){
	int	i,k,Nq;
	double	q=0.;
	double  Vi;
        gs = new LGSys(PGrad);
	gs->Eingabe(this);
	gs->Ausgabe_Loesung(gs->DoIt());
#ifdef TALK
	std::cout << "\n\nRegressing polynom:\n y(x) = ";
	for(i=0; i<PGrad; i++){
		if(i==0)
			std::cout << gs->GetPolyK(i);
		else
			std::cout << " + " << gs->GetPolyK(i) << " x";
		if(i>1)
			std::cout << "^" << i;
	}
	std::cout << "\n\nStatistical parameter:\n";
	std::cout << "Sample count: N = " << N << "\n";
#endif
	for(Vi=0., i=0; i<N; i++){
		q = y[i] - gs->CalcPoly(x[i], -1);
		Vi += q*q;
	}
#ifdef TALK
	std::cout << "sum of deviation squared / count = " << Vi/N << "\n\n";
#endif
	for(k=0; k<PGrad; k++){
		for(Nq=N, Vi=0., i=0; i<N; i++)
			if(x[i] != 0.){
				q = gs->GetPolyK(k) - (y[i] - gs->CalcPoly(x[i], k))/gpow(x[i],k);
				Vi += q*q;
			} else
				--Nq;
#ifdef TALK
		std::cout << "a[" << k << "]      = " << gs->GetPolyK(k) << "\n";
		if(Nq-1 > 0) std::cout << "sig(N-1)[" << k << "] = " << sqrt(Vi/(double)(Nq-1)) << "\n";
		else std::cout << "insufficient values!\n";
		if(Nq > 0)	std::cout << "sig(N)[" << k << "]   = " << sqrt(Vi/(double)Nq) << "\n";
		else std::cout << "insufficient values!\n";
#endif
	}
}

Zeile::Zeile(int nk, int gm)
{	int	i;
	n=nk; m=gm;
	gl = new double[n+1];
	for(i=0; i<=n; i++)	// read in b and coefficients
	{	if(i<n) std::cout << "\nGL " << (m+1) << " : x" << (i+1) << " : ";
		else	std::cout << "\nGL " << (m+1) << " : b  : ";
	        std::cin >> gl[i];
	}
}

Zeile::Zeile(int nk, int gm, StatInp *koef)
{	int	i;
	n=nk; m=gm;
	gl = new double[n+1];
	for(i=0; i<=n; i++)	// read in b and coefficients
		gl[i]=koef->CalcKoef(i,m);
}

int Zeile::Test(int start)
{	int	i;
	for(i=start; i<=n && gl[i]==0; i++);
	return(i);
}

void Zeile::Mult(double faktor)
{	int	i;
	for(i=0; i<=n; i++) gl[i] *= faktor;
	for(i=0; i<=n; i++) if(fabs(gl[i]) < Delta) gl[i]=0.;
}

void Zeile::Add(class Zeile* z)
{	int	i;
	for(i=0; i<=n; i++) gl[i] += z->gl[i];
}

void Zeile::P_Zeile()
{	int i;
	for(i=0; i<n; i++)	// output b and coefficients
	{
		if(i==0) std::cout << "\nGL " << (m+1) << " : ";
		std::cout << gl[i] << " x" << (i+1);
		if(i<(n-1)) std::cout  << " + "; else std::cout << " = " << gl[i+1];
	}
}

void LGSys::Eingabe()
{
	int	i;
	do{	
		std::cout << "Input of linear equation system\n";
		std::cout << "\n variable count: ";
	        std::cin >> n;
		std::cout << "\n equation count: ";
	        std::cin >> m;
	} while((n>=MAXVAR)||(m>=MAXGL));
	std::cout << "\n enter coefficient: ";
	for(i=0; i<m; i++)
	{	gln[i] = new Zeile(n,i); st_fkt[i]= -1; }
}


void LGSys::Eingabe(StatInp* StatObj)
{
	int	i;
#ifdef TALK
	std::cout << "Creating equation system j=";
#endif
	for(i=0; i<m; i++)
	{	
	  gln[i] = new Zeile(n,i,StatObj); st_fkt[i]= -1;
#ifdef TALK
	  std::cout << i+1 << " "; 
#endif
	}
#ifdef TALK
	std::cout << "\n";
#endif
}

void LGSys::Tausche(int i,int j)
{
	class Zeile* x;
	if(i!=j) { x=gln[i]; gln[i]=gln[j]; gln[j]=x; }
}

int LGSys::SucheGl(int start)
{
	int	i,j,k,l;
	for(k= -1, j=n+1, i=start; i<m && j>0; i++)
		if((l=gln[i]->Test(start)) < j)	{ k=i; j=l; }
	if(k== -1) return(NEG);
	Tausche(start,k);
	return(st_fkt[start]=j);
}

int LGSys::Umformen()
{
	int	i,j;
	for(i=0; i<m; i++)
	{   
#ifdef TALK
		Ausgabe_Glsys(); 
//		std::cin; 
		std::cout << "\nSchritt " << i+1 << " : "; 
#endif
		if(SucheGl(i)==NEG) break; // Complete.
		if(i >= (m-1)) { i++; break; }
		for(j=i+1; j<m; j++)
		{	if(gln[j]->Wert(st_fkt[i])==0.) continue;
			gln[j]->Mult(-(gln[i]->Wert(st_fkt[i])/gln[j]->Wert(st_fkt[i])));
			gln[j]->Add(gln[i]);
		}
	}
#ifdef TALK
	Ausgabe_Glsys();
	std::cout << "\nrank of extended matrix = " << i;
#endif
	return(r=i);	// Rank=i : is count equation for SucheGl() successfull
}

int LGSys::Untersuche(int rg)
{	int	i;
	if(rg==0) return(R_IS_SOLUTION);
	if(st_fkt[rg-1] == n) return(NO_SOLUTION);
	aus = new int[n];
	for(i=0; i<=(n-rg); i++)
		Loese(i);
	return(SPEC_SOLUTION);
}

void LGSys::Ausgabe_Loesung(int ltyp)
{
#ifdef TALK
	int		i,j;
	double	d;
	std::cout << "\n";
	switch(ltyp)
	{
		case NO_SOLUTION : std::cout << "\nEq.sys has no solution !\n"; break;
		case R_IS_SOLUTION : std::cout << "\nEq.ssys as solution R !\n"; break;
		case SPEC_SOLUTION  :
			if((n-r)>0)
				std::cout << "\nEq.sys has " << n-r+1 << " linearly independent solutions:";
			else
				std::cout << "\nEq.sys  ha one solution:";
			for(i=0; i<=(n-r); i++)
			{	std::cout << "\nP" << i << " = ( ";
				for(j=0; j<n; j++)
				{	std::cout << pn[i][j]; if(j<(n-1)) std::cout << " , "; }
				std::cout << " )";
			}
			for(i=0; i<=(n-r); i++)
			{   if(i==0)	std::cout << "\n\nx = ( ";
				else	std::cout << " + " << i << " ( ";
				for(j=0; j<n; j++)
				{	d =  i==0 ? 0. : pn[0][j];
					std::cout << pn[i][j]-d; if(j<(n-1)) std::cout << " , ";
				}
				std::cout << " )";
			}
			break;
	}
#endif
}

double LGSys::CalcPoly(double x, int k)
{	int		j,i;
	double	y;
	if(n==r )
		for(y=0., j=i=0; j < n; j++){
			if(j == k) continue;
			y += pn[i][j]*gpow(x,j);
		}
	else
		return 0.;
	return y;
}

double LGSys::GetPolyK(int k)
{
	return pn[0][k];
}

void LGSys::Ausgabe_Glsys()
{	int	i;
	for(i=0; i<m; i++)	// print equations
	{	gln[i]->P_Zeile(); std::cout << "     f(" << i+1 << ")=" << st_fkt[i]+1; }
	std::cout << "\n";
}

void LGSys::Loese(int k)
{   int	i,j,ki;
	pn[k] = new double[n];
	for(ki=j=i=0; i<n; i++)	// create selection list
		if((j<=r)&&(i==st_fkt[j])) {	aus[i] = UNFREE; j++; }
		else	if((++ki)==k)	{ aus[i] = ONE; pn[k][i]=1.; }
				else	{ aus[i] = FREE; pn[k][i]=0.; }
	Berechne(k);
}

void LGSys::Berechne(int k)
{   int	i,j;
	double	p;
	for(i=r-1; i>=0; i--)
	{	for(p=gln[i]->Wert(n), j=n-1; j>st_fkt[i]; j--)
		{	if(aus[j]==ONE) p -= gln[i]->Wert(j);
			else	if(aus[j]==UNFREE) p -= pn[k][j]*gln[i]->Wert(j);
			// sonst FREE
		}
		pn[k][j] = p /= gln[i]->Wert(j);
	}
}

#ifdef STANDALONE
main(char argc, char *argv[])
{
	StatInp	*s;
        std::cout.precision(20);
        std::cout << "REGRES degree file [-yx]\n";
	if(argc == 2) Talk=1;
	if(argc >= 3)
		s = new StatInp(argv[1], argv[2], argc==4 );
	else
		s = new StatInp;
	exit(s->Result());
	delete s;
}
#endif
