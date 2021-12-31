/* -*-coding: utf-8;-*- */
#ifndef __NC2TOP_H
#define __NC2TOP_H

#include "plug-ins/scan/WSxM_header.h"// File distributet with WSxM to make header of data file

#include <netcdfcpp.h>// used to read additional data from original file
#include <cstdio>
#include <cstring>
#include <stdlib.h>
#include <glib.h>
#include <math.h>

#define WSXM_MAXCHARS 1000
#define IS_PICO_AMP 100// Check whether saving as pico is more suitable

#define FIO_OK 0
#define FIO_NO_NAME 1
#define FIO_NOT_RESPONSIBLE_FOR_THAT_FILE 2
#define FIO_OPEN_ERR 3
#define FIO_NO_NETCDFXSMFILE 4
#define FIO_INVALID_FILE 5

#define UTF8_MU        "\302\265"
#define UTF8_ANGSTROEM "\303\205"

// use OUTPUT to write messages to stdio, by setting the global variable verbose you can switch the output on an off
#define OUTPUT if(verbose) printf
bool verbose=0; // 0 no output (default), 1 output

using namespace std;

struct UnitsTable{ 
  const char *alias; const char *s; const char *pss; const double fac; const char *prec1; const char *prec2; 
};



// Alles wird auf dies Basiseinheit "1A" bezogen - nur User I/O in nicht A !!
UnitsTable XsmUnitsTable[] = {
	// Id (used in preferences), Units Symbol, Units Symbol (ps-Version), scale factor, precision1, precision2
	{ "AA", UTF8_ANGSTROEM,   "\305",    1e0, ".1f", ".3f" }, // UFT-8 Ang did not work
	{ "nm", "nm",  "nm",     10e0, ".1f", ".4f" },
	{ "um", UTF8_MU"m",  "\265m",     10e3, ".1f", ".7f" },
	{ "mm", "mm",  "mm",     10e6, ".1f", ".10f" },
	{ "BZ", "%BZ", "%BZ",     1e0, ".1f", ".3f" },
	{ "sec","\"",  "\"",      1e0, ".1f", ".6f" },
	{ "V",  "V",   "V",       1e0, ".2f", ".5f" },
	{ "1",  "a.u.",   "a.u.", 1e0, ".3f", ".0f" },
	{ "Amp", "A",  "A",       1e9, "g", "g" },
	{ "nA", "nA",  "nA",      1e0, ".2f", ".5f" },
	{ "pA", "pA",  "pA",      1e-3, ".1f", ".2f" },
	{ "nN", "nN",  "nN",      1e0, ".2f", ".3f" },
	{ "Hz", "Hz",  "Hz",      1e0, ".2f", ".3f" },
	{ "K",  "K",   "K",       1e0, ".2f", ".3f" },
	{ "amu","amu", "amu",     1e0, ".1f", ".2f" },
	{ "CPS","Cps", "Cps",     1e0, ".1f", ".6f" },
	{ "Int","Int", "Int",     1e0, ".1f", ".6f" },
	{ "A/s","A/s", "A/s",     1e0, ".2f", ".3f" },
	{ NULL,  NULL, NULL,      0e0, NULL, NULL }
}  ;

  /*----------------------------------------------------------------------
	convert *.nc to WSxM file format
  ----------------------------------------------------------------------*/
int main(int argn, char* argv[]);

/*----------------------------------------------------------------------
	SetUnit: Adds Unit to String
----------------------------------------------------------------------*/
void SetUnit(char * sValue, char * sUnit, double dValue, double dFactor = 1.0);

/*----------------------------------------------------------------------
	ReplaceChar: Replaces A Char Within A String
	Default: Replacing ',' by '.'
----------------------------------------------------------------------*/
void ReplaceChar(char *string, char oldpiece=',', char newpiece='.');

/*----------------------------------------------------------------------
	SeparateLine: Separates Lines vom sInput to sOutput
	Default Separator is '\n'
----------------------------------------------------------------------*/
void SeparateLine(char *sInput, char *sOutput, char cSeparator=10);

/*----------------------------------------------------------------------
	ClearString: Fills string with  \0
----------------------------------------------------------------------*/
void ClearString(char *sInput);

/*----------------------------------------------------------------------
    Killspace: Kills spaces and special symbols at the end of a string
----------------------------------------------------------------------*/
void KillSpace(char *sInput);

/*----------------------------------------------------------------------
	int to Char: Converts int (0-999) to Char: 51->051
----------------------------------------------------------------------*/
void itoc(int x, char *sChar);

/*----------------------------------------------------------------------
	GetUnit: returns true if unit was found
----------------------------------------------------------------------*/
bool GetUnit(char *unit, NcVar *Var);

/*----------------------------------------------------------------------
	WriteData: 
----------------------------------------------------------------------*/
int WriteData_cits(char *sOutput,NcVar* Data,float* ramp,double Vbias);
int WriteData_topo(char *sOutput,NcVar* Data);
int WriteData_movie(char *sOutput,NcVar* Data,double dz);
#endif
