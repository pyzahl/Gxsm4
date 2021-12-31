/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Tool Name: logviewer.h
 * ========================================
 * 
 * Copyright (C) 2001 The Free Software Foundation
 *
 * Authors: Thorsten Wagner <thorsten.wagner@uni-essen.de> 
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

#ifndef __LOGVIEWER_H
#define __LOGVIEWER_H

//////////////////////////////////////////////
// incluedes                                //
//////////////////////////////////////////////

#include "WSxM_header.h"	// Header definition files (modified) provieded by www.nanotec.es
#include <iostream>
#include <fstream>
#include <glib-2.0/glib.h>

using namespace std;
///////////////////////////////////////////////
// 	enum SourceChannelEnum               //
///////////////////////////////////////////////
enum SourceChannelEnum {
	Zmon, Umon, ADC5, ADC0, ADC1, ADC2, ADC3, ADC4, ADC6, ADC7, LockIn1st, LockIn2nd, LockIn0, END 
};
	
///////////////////////////////////////////////
// Typedef SourceChannelStruc                //
///////////////////////////////////////////////
typedef struct SourceChannelStruc {
    enum SourceChannelEnum number; 
    gchar *name;
    gchar *aliasname;
    gint size;
    gdouble scaling;
    gint bitcoding;
};

///////////////////////////////////////////////
//      enum FileTypeEnum                    // 
///////////////////////////////////////////////
//      Make changes here, if WSxM_header.h  //
//      is alternated!!!                     //
///////////////////////////////////////////////
enum FileTypeEnum {
    TOPO, CITS, CITM, IV, IZ, ZV, FZ, generic
};

///////////////////////////////////////////////
// Typedef ProbeStruc                //
///////////////////////////////////////////////
typedef struct ProbeStruc {
    enum FileTypeEnum FileTypeNumber;
    enum SourceChannelEnum xchannel;
    enum SourceChannelEnum ychannel;
};

///////////////////////////////////////////////
// object declarations                       //
///////////////////////////////////////////////
class probedata {
	private:
	  gint m;											// number of data points per spectra to hold
    GPtrArray *points;						// data points within a spectra
    gdouble scalefactor;					// dig. units * scalefactor = real units
  public:
		probedata(gdouble scale, gint datapoints);	// constructor
    ~probedata();									// destructor
    void addpoint(glong point);		// adding a point (in dig. units) to the dataset
    glong getpoint(glong index);	// reading back a point
    gdouble getvalue(glong index);	// reading back a point in real values
    gint count();								// returns the number of data points
    gchar xpos[WSXM_MAXCHARS];		// used to store x position, may be obsolent soon
    gchar ypos[WSXM_MAXCHARS];		// used to store y position, may be obsolent soon
};

class scandata {
	private:
		gint n;													// number of spectra to hold
    enum SourceChannelEnum channel; 	// type of souce challen
    GPtrArray *spectra;								// array of spectra
  public:
    scandata(guint spectras = 0, enum SourceChannelEnum source = ADC5);	// constructor
    ~scandata();											// destructor
    void addspectra();								// adding a new spectra to the data set
    gint count();										// returns the number of spectra within the dataset
    GString *getchannel();						// returns the channel name as a string
    probedata *getspectrum(gint i=-1);	// returns a spectra
    glong getmax(gint layer);				// returns the maximum over all spectra (in dig. units)
    glong getmin(gint layer);				// returns the minimum over all spectra (in dig. units)
    gdouble getmaxvalue (gint layer); //returns the maximum over all spectra (in real units)
    gdouble getminvalue (gint layer); //returns the minimum over all spectra (in real units)
};

///////////////////////////////////////////////
// Table Source Channel Coding               //
///////////////////////////////////////////////
SourceChannelStruc SourceChannels[] = {
  // SourceChannelEnum, Channel name, Channel string, Scaling factor, Channel coding,
	{Zmon, "Zmon", "Zmon-AIC5Out", 2, 4.0/65536., 0x01},
  {Umon, "Umon", "Umon-AIC6Out", 2, 4.0/65536., 0x02},
  {ADC5, "ADC5", "AIC5-I", 2, 20/65536., 0x10},
	{ADC0, "ADC0", "AIC0", 2, 20./65536., 0x20},
	{ADC1, "ADC1", "AIC1", 2, 20./65536., 0x40},
  {ADC2, "ADC2", "AIC2", 2, 20./65536., 0x80},
	{ADC3, "ADC3", "AIC3", 2, 20./65536., 0x100},
	{ADC4, "ADC4", "AIC4", 2, 20./65536., 0x200},
	{ADC6, "ADC6", "AIC6", 2, 20./65536., 0x400},
	{ADC7, "ADC7", "AIC7", 2, 20./65536., 0x800},
	{LockIn1st, "LockIn1st", "LockIn1s", 4, 20./(65536.*65536.), 0x1000},
	{LockIn2nd, "LockIn2nd", "LockIn2nd", 4, 20./(65536.*65536.), 0x2000},
	{LockIn0, "LockIn0", "LockIn0", 4, 20./(65536.*65536.),0x4000},
	{END, NULL, NULL, 0, 0, 0}
};

///////////////////////////////////////////////
// Table Probe Channels                      //
///////////////////////////////////////////////
ProbeStruc ProbeChannelDefault[] = {
    // FileTypeEnum, default x-channel (enum), default y-channel (enum)
	{TOPO, END, END},
	{CITS, Umon, ADC5},
  {CITM, Umon, ADC5},
  {IV, Umon, ADC5},
  {IZ, Zmon, ADC5},
  {ZV, Zmon, Umon},
  {FZ, Zmon, ADC5},
  {generic, ADC1, ADC0}
 };


///////////////////////////////////////////////
// function declarations                     //
///////////////////////////////////////////////
int main(int argc, char *argv[]);
GArray *getintsfromstring(gchar * string);
scandata *getscan(GPtrArray * ptrarray, gint i);
int writeIVs(gchar * fileprefix, scandata * scanx, scandata * scany, gdouble xfactor = 1.0, gdouble yfactor = 1.0 );

#endif
