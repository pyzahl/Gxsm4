/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Tool Name: logviewer.C
 * ========================================
 * 
 * Copyright (C) 2004 The Free Software Foundation
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

#include "logviewer.h"

///////////////////////////////////////
// function definition: main         //
///////////////////////////////////////
int main(int argc, char *argv[])
{

/***************************************************************
*** Define some variables 
***************************************************************/
			gchar inputfile[WSXM_MAXCHARS];			// input file name
    	gchar outputfile[WSXM_MAXCHARS];		// prefix for output file
    	gchar xpos[WSXM_MAXCHARS];					// used to buffer x position string
    	gchar ypos[WSXM_MAXCHARS];					// used to buffer y position string
    	guint rows, cols;                   // number of rows, cols within a log file
    	guint buffer_cols = 0;							// to check the log file integrity
    	enum FileTypeEnum type = IV;	      // default file type is single IV curve 
    	guint xchannel = Umon;							// defines xchannel (within SourceChannels)
    	gdouble xscale = 1.0;								// custom scaling for x-axis
    	guint ychannel = ADC5;							// defines ychannel (within SourceChannels)
    	gdouble yscale = 1.0;								// custom scaling for y-axis
    	
    	GPtrArray *ptrscans;								// array that holds all data
			ptrscans = g_ptr_array_new();
    	glong points = 0;										// number of data points in log file
    	glong source = 0;										// source channels used in log file
    	gint x = 1;													// Number of x-Channel (Bias-Voltage) (within ptrscans)
			gint y = 1;													// Number of y-Channel (Tunneling Current) (within ptrscans)
			gdouble u_start = 0;								// bias voltage to start with during STS
			gdouble u_end = 0;									// bias voltage to end with during STS

			// define some (global) variables
			gchar buffer_string[WSXM_MAXCHARS];

/***************************************************************
*** Analyse arguments 
***************************************************************/
	if (argc == 2) {	// Test for --help option
		if (strstr(argv[1],"--help")) {
  		std::cout << "Usage: logviewer [options] inputfile [prefix for outputfile]" << std::endl;
    	std::cout << "Possible options are:" << std::endl;
    	std::cout << "\t--help\tThis help message" << std::endl;
    	std::cout << "\t-IV\tSave as IV file (*.iv.cur)" << std::endl;
    	std::cout << "\t\tPossible file types: IV, IZ, ZV, FZ" << std::endl;
    	std::cout << "\t-CITS\tnot implement yet" << std::endl;
    	std::cout << "\t-x=[factor*]channel" << std::endl;
    	std::cout << "\t-y=[factor*]channel" << std::endl;
    	std::cout << "\t\tPossible channels: Zmon, Umon, ADC5, ADC0, ADC1, ADC2, \n\t\tADC3, ADC4, ADC6, ADC7, LockIn1st, LockIn2nd, LockIn0" << std::endl;
    	return -1;	
  	}
	}
	if (argc < 3) {	// Need at least three arguments (programm name, option or input, input or output prefix
		std::cerr << "Too few arguments. Specify spectra type and input file." << std::endl;
		std::cerr << "Usage: logviewer [options] inputfile [prefix for outputfile]" << std::endl;
		return -1;
	} else {		// Now check each argument if it is an option
		for (int i = 1; i < argc; ++i) {
			// std::cout << "Logviewer was called with: " << i << ". " << argv[i] << std::endl;
  		// Check if file type is given as an argument. If not, IV is default.
   		extern FileIdStruc FileId[];
   		FileIdStruc *FileIdPtr = FileId;
   		for (; FileIdPtr->name != 0; ++FileIdPtr) {
				if (strstr(FileIdPtr->option, argv[i])) {
    			std::cout << "Option " << FileIdPtr->name << " detected." << std::endl;
    			type = (enum FileTypeEnum) FileIdPtr->index;
    			xchannel = (ProbeChannelDefault[(guint)FileIdPtr->index]).xchannel;
    			ychannel = (ProbeChannelDefault[(guint)FileIdPtr->index]).ychannel;
    			if (strstr(argv[argc - 2],"-")){
	    			// only last argument can be file name, so make own prefix for output file
	   				if (strstr(argv[argc -1],"-")){
	  					std::cerr << "No file name was specified. Add at least filename for input." << std::endl;
	   					return -1;
	   				} else {
	   					strncpy (inputfile, argv [argc -1], WSXM_MAXCHARS);
	   					strncpy (outputfile, FileIdPtr->name, WSXM_MAXCHARS);
	   				}
	   			} else {
	   				strncpy (inputfile, argv [argc -2], WSXM_MAXCHARS);
	   				strncpy (outputfile, argv [argc -1], WSXM_MAXCHARS);	
	   			}
	    		std::cout << "Prefix for output file is " << outputfile << ". The type is " << type <<"." << std::endl;
	    		break;
				}
	 		}
	    	
   	if (strstr(argv[i], "-x=")) {
	    SourceChannelStruc *SourceChannelPtr = SourceChannels;
	    for (;SourceChannelPtr->number < END; SourceChannelPtr++) {
	   		if (strstr(argv[i],SourceChannelPtr->name)||strstr(argv[i],SourceChannelPtr->aliasname)) {
	   			xchannel = SourceChannelPtr->number;
	   			std::cout << "Customized x-channel: " << SourceChannelPtr->name << std::endl;
	   		}
	   	}
	   	if (strstr(argv[i],"*")){
	   			// customized scaling of the x-axis
	   			xscale = atof(g_strsplit_set(argv[i],"=*",-1)[1]);
	   			std::cout << "x scaling factor is: " << xscale << std::endl;
	   	}
	 	}
	  if (strstr(argv[i], "-y=")) {
	  	SourceChannelStruc *SourceChannelPtr = SourceChannels;
	  	for (;SourceChannelPtr->number < END; SourceChannelPtr++) {
	  		if (strstr(argv[i],SourceChannelPtr->name)||strstr(argv[i],SourceChannelPtr->aliasname)) {
	  			ychannel = SourceChannelPtr->number;
	  			std::cout << "Customized y-channel: " << SourceChannelPtr->name << std::endl;
	  		}
	  	}
	  	if (strstr(argv[i],"*")){
	   		// customized scaling of the y-axis
	   		yscale = atof(g_strsplit_set(argv[i],"=*",-1)[1]);
	   		std::cout << "y scaling factor is: " << yscale << std::endl;
	   	}
	 	}
	}

	/*************************************************************
  	*** Read data from file
  	*************************************************************/
	if (argc >= 2) {
  fstream f;
  f.open(inputfile, ios::in);
	if (!f.good()) {
		std::cerr << "Error: Not possible to open file for reading: " << inputfile << std::endl;
		return -1;
	} else {
	std::cout << "Opened succesfull file: " << inputfile << std::endl;
		
	while (f.getline(buffer_string, WSXM_MAXCHARS)) {
		if (strstr(buffer_string, "# DSPControlClass->Points: ")) {
			f.seekg((int)(-f.gcount() + strlen("# DSPControlClass->Points: ")), ios::cur);
			f.getline(buffer_string, WSXM_MAXCHARS);
			points = atoi(buffer_string);
			std::cout << "DSPControlClass->Points: " << points << std::endl;
		}

		if (strstr(buffer_string, "# DSPControlClass->Source: ")) {
			f.seekg((int) (-f.gcount() + strlen("# DSPControlClass->Source: ")), ios::cur);
			f.getline(buffer_string, WSXM_MAXCHARS);
			source = atoi(buffer_string);
			std::cout << "DSPControlClass->Source: " << source << std::endl;
			SourceChannelStruc *SourceChannelPtr = SourceChannels;
			for (; SourceChannelPtr->bitcoding > 0; SourceChannelPtr++) {
			    if (source & SourceChannelPtr->bitcoding) {
						scandata *scanptr = new scandata(0, SourceChannelPtr->number);
						g_ptr_array_add(ptrscans, scanptr);
			  	}
				}
					for (guint i = 0; i < ptrscans->len; ++i) {
			    	if (strstr (getscan(ptrscans, i)->getchannel()->str, (SourceChannels[(enum SourceChannelEnum)ychannel]).name )) {
							y = i;
							std::cout << i << ". channel is " << getscan(ptrscans, i)->getchannel()->str << ". Use as Y." << std::endl;
			    	} else {
			    		if (strstr (getscan(ptrscans, i)->getchannel()->str, (SourceChannels[(enum SourceChannelEnum)xchannel]).name )) {
								x = i;
								std::cout << i << ". channel is " << getscan(ptrscans, i)->getchannel()->str << ". Use as X." << std::endl;
			    		} else {
				    		std::cout << i << ". channel is " << getscan(ptrscans, i)->getchannel()->str << "." << std::endl;
							}
			    	}
					}
		    }
		    // neuer C++ cast: static_cast<int>(integer);
		    if (strstr(buffer_string, "# DSPControlClass->U_start: ")) {
					f.seekg((int) (-f.gcount() + strlen("# DSPControlClass->U_start: ")),	ios::cur);
					f.getline(buffer_string, WSXM_MAXCHARS);
					u_start = atof(buffer_string);
					std::cout << "DSPControlClass->U_start: " << u_start << std::endl;
		    }

		    if (strstr(buffer_string, "# DSPControlClass->U_end: ")) {
					f.seekg((int) (-f.gcount() + strlen("# DSPControlClass->U_end: ")), ios::cur);
					f.getline(buffer_string, WSXM_MAXCHARS);
					u_end = atof(buffer_string);
					std::cout << "DSPControlClass->U_end: " << u_end << std::endl;
		    }
		    
		    if (strstr(buffer_string, "# x_scan: ")) {
		    	f.seekg((int) (-f.gcount() + strlen("# x_scan: ")), ios::cur);
		    	f.getline(xpos, WSXM_MAXCHARS);
		    }
		    
		    if (strstr(buffer_string, "# y_scan: ")) {
		    	f.seekg((int) (-f.gcount() + strlen("# y_scan: ")), ios::cur);
		    	f.getline(ypos, WSXM_MAXCHARS);
		    }
		    
		    if (!strstr(buffer_string, "# ") && ptrscans->len > 0 && strstr(buffer_string, "[")) {
					GArray *tempint;
					tempint = g_array_new(TRUE, FALSE, sizeof(int));
					tempint = getintsfromstring(buffer_string);
					for (guint i = 0; i < tempint->len; i++) {
			    	if (strstr(buffer_string, "[0]")) {
			      	// create a new spectra if begin of new spectra is dectected
			      	getscan(ptrscans, i)->addspectra();
			      	// store xpos and ypos
			      	strncpy (getscan(ptrscans, i)->getspectrum(-1)->xpos, xpos, WSXM_MAXCHARS);
			      	strncpy (getscan(ptrscans, i)->getspectrum(-1)->ypos, ypos, WSXM_MAXCHARS);
			      	// check for new row
			      	if (i == 0) {
									if (getscan(ptrscans, 0)->count() >  0){
				  					if (strncmp(getscan(ptrscans, 0)->getspectrum(-1)->ypos, ypos, WSXM_MAXCHARS)) {
				    					std::cout<<std::endl<<"y= "<<ypos<<": ";
				    					if (rows == 1) {
				      					cols = buffer_cols;
				    					} else {
				      					if (!(cols == buffer_cols)) {
													std::cerr<<"Log file corrupted. Not all spectra saved."<<std::endl;
				      					}
				    					}
				    					rows++;
				    					buffer_cols = 0;
				  					}
									} else { 
				  					std::cout<<"y= "<<ypos<<": ";
				  					rows = 1;
									}
									std::cout<<"|x= "<<xpos;
									buffer_cols++;
			      		}
			    		}
			    		getscan(ptrscans, i)->getspectrum(-1)->addpoint((glong) g_array_index (tempint, int, i));
						}
		    	}
				}
			f.close();
		
			/*************************************************************
  		*** Write spectra to file
  		*************************************************************/
  		std::cout << std::endl << "The log file contains "<<cols<<" by "<<rows<<" spectra."<<std::endl;
			writeIVs(outputfile, getscan(ptrscans, x),getscan(ptrscans, y), xscale, yscale);
			}
		} else {
			std::cerr << "You have to specify a log file to handle!" <<std::endl;
		}
  }
}

////////////////////////////////////////////////
// function definition: getintsfromstring     //
////////////////////////////////////////////////
GArray *getintsfromstring(gchar * string)
{
	gchar **ptr_buffer;
  gint nIndex = 1;
  glong lbuffer;
  ptr_buffer = g_strsplit(string, " ", 16);
  GArray *glong_buffer;
  glong_buffer = g_array_new(TRUE, TRUE, sizeof(glong));
  while (ptr_buffer[nIndex]) {
		lbuffer = atol(ptr_buffer[nIndex]);
		g_array_append_val(glong_buffer, lbuffer);
		nIndex++;
  }
  return glong_buffer;
  g_strfreev(ptr_buffer);
  g_array_free(glong_buffer, FALSE);
}

////////////////////////////////////////////////
// function definition: getscan               //
////////////////////////////////////////////////
scandata *getscan(GPtrArray * ptrarray, gint i)
{
	if (i < 0) {
		i = (gint) ptrarray->len-1;
  }
  if (i < (gint) ptrarray->len) {
  	return (scandata *) g_ptr_array_index(ptrarray, i);
  } else {
		std::cerr << "Warning: Scan index out of range! Call for scan " << i << std::endl;
		return 0;
  }
}

////////////////////////////////////////////////
// function definition: writeIVs              //
////////////////////////////////////////////////
int writeIVs(gchar * fileprefix, scandata * scanx, scandata * scany, gdouble xfactor, gdouble yfactor ){
	// File pointer for destination file
	FILE *pDestinationFile = NULL;
	// set destination for writing data 
	gchar sDestinationFileName[WSXM_MAXCHARS];
	for (gint index=0; index < scanx->count(); ++index) {
		std::cout << "Processing spectra " << index << "\r";	
		sprintf (sDestinationFileName, "%s-%i.iv.cur",fileprefix,index);
		// Start creating header 
		WSxM_HEADER header;
		// the header is ASCII. So opening the header in text mode
		pDestinationFile = fopen(sDestinationFileName, "wt");
		if (pDestinationFile == NULL) {
	  	std::cerr << "Error: Not possible to open file for writing." << std::endl;
	  	return -1;
		} else {
			/***********************************************************************
			*** Header initialization 
			***********************************************************************/
			HeaderInit(&header);
			// buffer variable
			char szValue[WSXM_MAXCHARS];	// Needed to handle/convert different values

			/***********************************************************************
      *** Header Sektion: General_Info
  		***********************************************************************/
			sprintf(szValue, "%i", 1);	// Only one curve is saved, could be two for forward and backward
			HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO, IMAGE_HEADER_GENERAL_INFO_LINES, szValue);
			sprintf(szValue, "%i", scanx->getspectrum(index)->count());
			HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO, IMAGE_HEADER_GENERAL_INFO_POINTS, szValue);
			// Set labels and units for the axis
			HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO, IMAGE_HEADER_GENERAL_INFO_X_TEXT, "V[#x]");
			HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO, IMAGE_HEADER_GENERAL_INFO_X_UNIT, "Volt");
			HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO, IMAGE_HEADER_GENERAL_INFO_Y_TEXT, "I[#y]");
			HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO, IMAGE_HEADER_GENERAL_INFO_Y_UNIT, "nA");

			/***********************************************************************	
      *** Header Sektion: General_Misc
  		***********************************************************************/
			HeaderAddValue(&header, IMAGE_HEADER_MISC_INFO, IMAGE_HEADER_MISC_INFO_FIRST, "Yes");
			HeaderAddValue(&header, IMAGE_HEADER_CONTROL, IMAGE_HEADER_CONTROL_X_OFFSET , scanx->getspectrum(index)->xpos);
			HeaderAddValue(&header, IMAGE_HEADER_CONTROL, IMAGE_HEADER_CONTROL_Y_OFFSET , scanx->getspectrum(index)->ypos);

			// Write header now
			HeaderWrite(&header, pDestinationFile, "IV File");

			/**********************************************************************
  	  *** Write data to destination file
  		**********************************************************************/
	    for (gint i = 0; i < scanx->getspectrum(index)->count(); ++i) {
				fprintf(pDestinationFile, "%lf ", scanx->getspectrum(index)->getvalue(i)* xfactor);
				fprintf(pDestinationFile, "%lf ", scany->getspectrum(index)->getvalue(i)* yfactor);
				fprintf(pDestinationFile, " \n");		
			}
			fclose(pDestinationFile);
		}
	}
	return 0;
}
////////////////////////////////////////////////
// function definitions for probedata         //
////////////////////////////////////////////////

// constructor: probedata::probedata(glong datapoints=0)
probedata::probedata(gdouble factor = 1.0, gint datapoints = 0)
{
  if (datapoints <= 0) {
		points = g_ptr_array_new();
  } else {
		points = g_ptr_array_sized_new(datapoints);
  }
  m = datapoints;
  scalefactor = factor; 
}

// destructor: probedata::~probedata()
probedata::~probedata()
{
  g_ptr_array_free(points, TRUE);
}

// function: probedata::addpoint(glong point)
void probedata::addpoint(glong point)
{
	glong *temp = new glong;
  *temp = point;
  g_ptr_array_add(this->points, temp);
  this->m++;
}

// function: probedata::getpoint(gulong index)
glong probedata::getpoint(glong index)
{
  if (index < 0) {
		index = count()-1;
  }
  if (index < count()) {
		return *(glong *) g_ptr_array_index(points, index);
  } else {
		std::cerr << "Warning: Point index out of range!" << std::endl;
		return 0;
  }
}

// function: probedata::getvalue(gulong index)
gdouble probedata::getvalue(glong index)
{
	if (index < 0) {
		index = count()-1;
  }
  if (index <= count()) {
  	return scalefactor*(gdouble)*(glong *) g_ptr_array_index(points, index);
  } else {
		std::cerr << "Warning: Point index out of range!" << std::endl;
		return 0;
  }
}

// function: probedata::count()
gint probedata::count()
{
	return m;
}

////////////////////////////////////////////////
// function definitions for scandata          //
////////////////////////////////////////////////

// constructor: scandata::scandata(glong spectra=0, GString *name="")
scandata::scandata(guint spectras, enum SourceChannelEnum name)
{
	if (spectras == 0) {
		spectra = g_ptr_array_new();
	} else {
		spectra = g_ptr_array_sized_new(spectras);
  }
  n = spectras;
  channel = name;
}

// destructor: scandata::~scandata()
scandata::~scandata()
{
  g_ptr_array_free(spectra, TRUE);
}

// function: scandata::addspectra(probedata spectrum=0)
void scandata::addspectra()
{
  gdouble tempscale=1.0;
  tempscale = (SourceChannels[(enum SourceChannelEnum) channel]).scaling;
  probedata *tempspectrum = new probedata(tempscale);
  g_ptr_array_add(this->spectra, tempspectrum);
  this->n++;
}

// function: scandata::count()
gint scandata::count()
{
  return n;
}

// function: scandata::getchannel()
GString *scandata::getchannel()
{
	return g_string_new((SourceChannels[(enum SourceChannelEnum)channel]).name);
}

// function: scandata::getspectrum(gint i);
probedata *scandata::getspectrum(gint i)
{
	if (i < 0) {
		i = this->count()-1;
  }
  if (i < this->count()) {
		return (probedata *) g_ptr_array_index((GPtrArray *) this->spectra, i);
  } else {
		std::cerr << "Not possible to accese spectra " << i << " in channel " << this->getchannel()->str << "." << std::endl;
		return 0;
  }
}

// function: scandata::getmax(gint layer)
glong scandata::getmax(gint layer)
{
  glong max;
  for (gint i = 0; i < count(); ++i) {
		max = getspectrum(i)->getpoint(layer) > max ? getspectrum(i)->getpoint(layer) : max;
  }
  return max;
}

// function: scandata::getmin(gint layer)
glong scandata::getmin(gint layer)
{
	glong min;
  for (gint i = 0; i < count(); ++i) {
		min = getspectrum(i)->getpoint(layer) < min ? getspectrum(i)->getpoint(layer) : min;
  }
  return min;
}

// function: scandata::getmaxvalue(gint layer)
gdouble scandata::getmaxvalue (gint layer)
{
  return getmax(layer)*(SourceChannels[(enum SourceChannelEnum)channel]).scaling;
}

// function: scandata::getminvalue(gint layer)
gdouble scandata::getminvalue(gint layer)
{
  return getmax(layer)*(SourceChannels[(enum SourceChannelEnum)channel]).scaling;
}
