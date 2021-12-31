/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001 Percy Zahl
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

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <cmath>

#include <glib.h>
#include <netcdf.hh>
#include <png.h>
#include <popt.h>
#include "thumbpal.h"
#include "writepng.h"

// GXSM viewmodes
#define SCAN_V_DIRECT       0x0001
#define SCAN_V_QUICK        0x0002
#define SCAN_V_HORIZONTAL   0x0004
#define SCAN_V_PERIODIC     0x0008
#define SCAN_V_LOG          0x0010
#define SCAN_V_DIFFERENTIAL 0x0020
#define SCAN_V_PLANESUB     0x0040


// ==============================================================
//
// Read (Gxsm) NetCDF image/data
// and convert to png image/thumbnail
// - read only required subgrid data (scaled down)
// - use quick, "more intelligent" autoscale
// - use palette
//
// ==============================================================

typedef enum { NC_READ_OK, NC_OPEN_FAILED, NC_NOT_FROM_GXSM } GXSM_NETCDF_STATUS;

#define THUMB_X 96 // Thumbnail max X size
#define THUMB_Y 91 // Thumbnail max Y size

inline int min (int x, int y) { return x<y?x:y; }
inline int max (int x, int y) { return x>y?x:y; }

class raw_image{
public:
	raw_image(){ 
		rgb=NULL;
		scan_viewmode = 0;
		scan_contrast = 0.;
		scan_bright = 0.;
	};
	virtual ~raw_image(){ 
		if(rgb){
			for (int i=0; i<ny; delete[] rgb[i++]);
			delete[] rgb; 
		}
	};

	int width() { return nx; };
	int height() { return ny; };

	void SetViewMode (int vm) { 
		scan_viewmode = vm;
	};

	void SetTransfer (double c, double b){
		scan_contrast = c;
		scan_bright = b;
	};

	void calc_line_regress(int y, double &slope, double &offset){
		//
		// OutputFeld[i] = InputFeld[i] - (slope * i + offset)
		//
		double sumi, sumy, sumiy, sumyq, sumiq;
		int i, istep;
		int Xmax, Xmin;
		double a, n, imean, ymean;
			
		sumi = sumiq = sumy = sumyq = sumiy = 0.0;
		Xmax = nx-nx/10; // etwas Abstand (10%) vom Rand !
		Xmin = nx/10;
		istep = 1;
		for (i=Xmin; i < Xmax; i+=istep) { /* Rev 2 */
			sumi   += (double)i;
			sumiq  += (double)i*(double)i;
			a =  rowdata[y][i];
			sumy   += a;
			sumyq  += a * a;
			sumiy  += a * (double)i;
		}
		n = (double)((Xmax-Xmin)/istep);
		if ((Xmax-Xmin) % istep != 0) n += 1.0;
		imean = sumi / n;
		ymean = sumy / n;
		slope  = (sumiy- n * imean * ymean ) / (sumiq - n * imean * imean);
		offset = ymean - slope * imean;
	};
	double soft(int i, int j, double lim=0){
		int a,b,c,d;
		double z, mi, ma;
		int u,l;

		a = max (0, i-1);
		b = min (i+1, nx-1);
		c = max (0, j-1);
		d = min (j+1, ny-1);

		ma = mi = rowdata[j][i];
		u = l = 1;

#define MIMA(J,I) do{ z = rowdata[J][I]; if (ma < z) { u++; ma = z; } if (mi > z) { l++; mi = z; } }while(0)

		MIMA (c,i);
		MIMA (c,a);
		MIMA (j,a);
		MIMA (d,a);
		MIMA (c,b);
		MIMA (j,b);
		MIMA (d,b);
		MIMA (d,i);

		return (mi*l + ma*u)/(u+l);
	};
	void find_hard_min_max (double &min, double &max){
		// find range
		min = max = rowdata[0][0];
		for (int i=0; i<ny; ++i)
			for (int j=0; j<nx; ++j){
				double v = rowdata[j][i];
				if (min > v) min = v;
				if (max < v) max = v;
			}
	};
	void find_soft_min_max (double &min, double &max){
		double ihigh, ilow, imed, irange, high, low, zrange, bin_width, dz_norm;
		double dnum;
		int    bin_num, med_bin, lo_bin, hi_bin;
		double *bins;

		// find range
		low = high = soft (0,0);
		for (int i=0; i<ny; ++i)
			for (int j=0; j<nx; ++j){
				double v = soft (j,i);
				if (low > v) low = v;
				if (high < v) high = v;
			}

		// calc histogramm
		zrange = high-low;
		dnum = zrange/3; // +/-1 dz (3dz) in ein bin per default
	
		bin_num   = (int)dnum;
		bin_width = zrange / bin_num;
		dz_norm   = 1./bin_width;
		
		bins = new double[bin_num];

		for(int i = 0; i < bin_num; i++)
			bins[i] = 0.;

		for(int col = 0; col < nx; col++)
		for(int line = 0; line < ny; line++){
			double f  = (soft (col,line) - low) / bin_width;
			int bin   = (int)f;
	
			if (bin < bin_num){
				double f1 = (bin+1) * bin_width;
				if ((f+dz_norm) > f1){ // partiell in bin, immer rechter Rand, da bin>0 && bin < f
					bins [bin] += f-bin;
					++bin;
					if (bin < bin_num)
						bins [bin] += bin-f; // 1.-(f-(bin-1))
				}
				else // full inside of bin
					bins [bin] += 1.;
			}
		}

		// integrate in place
		for(int i = 1; i < bin_num; i++)
			bins[i] += bins[i-1];

		ilow   = bins [bin_num/100];
		ihigh  = bins [bin_num-bin_num/100-1];
		irange = ihigh - ilow;
		imed   = ilow + irange/2.;

		for (med_bin = 1; bins [med_bin] < imed; ++med_bin);

		for (lo_bin = med_bin; lo_bin > (bin_num/100) && bins [lo_bin] > ilow+irange/20.; --lo_bin);
		for (hi_bin = med_bin; hi_bin < (bin_num-bin_num/100-1) && bins [hi_bin] < ihigh-irange/20.; ++hi_bin);

		min = lo_bin*bin_width + low;
		max = hi_bin*bin_width + low;

		delete [] bins;
  

	};
	void quick_rgb(int linreg=TRUE, int minmax=FALSE) {
			if (rgb){
				for (int i=0; i<ny; delete[] rgb[i++]);
				delete[] rgb;
			}
			rgb = new unsigned char* [ny];
			for (int i=0; i<ny; rgb[i++] = new unsigned char[3*nx]);

			// Do image processing and find scaling
			double min, max, range;
			if (linreg){
				
				switch (scan_viewmode){
				case SCAN_V_DIRECT: 
					for (int i=0; i<ny; ++i){
						for (int j=0; j<nx; ++j){
							rowdata[i][j] *= scan_contrast;
							rowdata[i][j] += scan_bright;
						}
					}
					if (minmax)
						find_hard_min_max (min, max); // well, cannot guess Vrange yet:-(
					else
						find_soft_min_max (min, max); // well, cannot guess Vrange yet:-(
					break;
				case SCAN_V_QUICK: 
					// Calc and Apply "Quick"
					for (int i=0; i<ny; ++i){
						double a,b;
						calc_line_regress (i, a, b);
						for (int j=0; j<nx; ++j){
							rowdata[i][j] -= j*a+b;
							rowdata[i][j] *= scan_contrast;
							rowdata[i][j] += scan_bright;
						}
					}
					min = 0.;
					if ( fabs (scan_bright) > 64.) // guess range, no indicator yet
						max = 1024.;
					else
						max = 64.;
					break;
				default:
					// Calc and Apply "Quick"
					for (int i=0; i<ny; ++i){
						double a,b;
						calc_line_regress (i, a, b);
						for (int j=0; j<nx; ++j)
							rowdata[i][j] -= j*a+b;
					}
					if (minmax)
						find_hard_min_max (min, max); // well, cannot guess Vrange yet:-(
					else
						find_soft_min_max (min, max);
					break;
				}
			} else {	
				if (minmax)
					find_hard_min_max (min, max); // well, cannot guess Vrange yet:-(
				else
					find_soft_min_max (min, max);
			}

			range = max-min;
			for (int i=0; i<ny; ++i)
				for (int j=0, k=0; j<nx; ++j){
					int idx = (int)((rowdata[i][j]-min)/range*(PALETTE_ENTRIES+1)+0.5);
					const int limit = 255;
					if (idx < 0){ // blue: lower limit
						idx /= -8; if (idx > limit) idx = limit;
						rgb[i][k++] = 16;
						rgb[i][k++] = 16;
						rgb[i][k++] = idx;
						continue;
					}
					if (idx > PALETTE_ENTRIES){ // red: upper limit
						idx -= PALETTE_ENTRIES;
						idx /= 8; if (idx > limit) idx = limit; idx = limit-idx;
						rgb[i][k++] = 255;
						rgb[i][k++] = idx;
						rgb[i][k++] = idx;
						continue;
					}
					idx *= 3;
					rgb[i][k++] = thumb_palette[idx++];
					rgb[i][k++] = thumb_palette[idx++];
					rgb[i][k++] = thumb_palette[idx];
				}
			
	};
	unsigned char **row_rgb_pointers() { return rgb; };

protected:
	int nx, ny; // image size
	int x0, y0, w0; // offset, data width
	int onx, ony; // original data size of NC-file
	double **rowdata;
	unsigned char **rgb;

	int scan_viewmode;
	double scan_contrast;
	double scan_bright;
};

template <class NC_VAR_TYP>
class raw_image_tmpl : public raw_image{
public:
		raw_image_tmpl(NcVar *img, int thumb=1, int new_x=0, int x_off=0, int y_off=0, int width=0){
				x0 = x_off; y0 = y_off;

				// Data->get_dim(0)->size(); // Time Dimension
				// Data->get_dim(1)->size(); // Value Dimension (Layers)
				ony = img->get_dim(2)->size(); // Y Dimenson
				onx = img->get_dim(3)->size(); // X Dimenson

				if (width>0){
						w0=width;
						nx=new_x;
						ny=new_x;
				}else{
						w0=onx;
						if (thumb){
								if(onx < ony*THUMB_X/THUMB_Y){
										ny=THUMB_Y;
										nx=onx*ny/ony;
								}else{
										nx=THUMB_X;
										ny=ony*nx/onx;
								}
						}else{
								if (new_x){ // scale to new X w aspect
										nx=new_x;
										ny=ony*nx/onx;
								}else{
										nx=onx; 
										ny=ony;
								}
						}
				}

				if (x0+nx >= onx) x0=0; // fallback
				if (y0+ny >= ony) y0=0; // fallback

				rowdata = new double* [ny];
				for (int i=0; i<ny; rowdata[i++] = new double [nx]);
				convert_from_nc(img);
		};

	virtual ~raw_image_tmpl(){
		for (int i=0; i<ny; delete [] rowdata[i++]);
		delete [] rowdata;
	};

	void convert_from_nc(NcVar *img, int v=0){ // v=0: use layer 0 by default
		double scale = (double)nx/(double)w0;
		NC_VAR_TYP *row = new NC_VAR_TYP[onx];
		
		for (int y=0; y<ny; y++){
			img->set_cur (0, v, y0+(int)((double)y/scale+.5));
			img->get (row, 1,1, 1,onx);
			for (int i=0; i<nx; ++i)
				rowdata[y][i] = (double)row[x0+(int)((double)i/scale+.5)];
		}
	};
};


GXSM_NETCDF_STATUS netcdf_read(const gchar *file_name, raw_image **img, 
							   int thumb, int new_x, int x_off, int y_off, int width){
	NcError ncerr(NcError::verbose_nonfatal);
	NcFile nc(file_name);

	// Check if the file was opened successfully
	if (! nc.is_valid())
		return NC_OPEN_FAILED;

	NcVar *Data = NULL;

	// try Topo Scan
	if (! (Data = nc.get_var("H")))

		// try Diffract Scan
		if (! (Data = nc.get_var("Intensity")))

			// try Float data
			if (! (Data = nc.get_var("FloatField")))

				// try Double
				if (! (Data = nc.get_var("DoubleField")))

					// failed looking for Gxsm data!!
					return NC_NOT_FROM_GXSM;

	switch (Data->type()){
	case ncShort: *img = new raw_image_tmpl<short> (Data, thumb, new_x, x_off, y_off, width); break;
	case ncLong: *img = new raw_image_tmpl<long> (Data, thumb, new_x, x_off, y_off, width); break;
	case ncFloat: *img = new raw_image_tmpl<float> (Data, thumb, new_x, x_off, y_off, width); break;
	case ncDouble: *img = new raw_image_tmpl<double> (Data, thumb, new_x, x_off, y_off, width); break;
	case ncByte: *img = new raw_image_tmpl<char> (Data, thumb, new_x, x_off, y_off, width); break;
	case ncChar: *img = new raw_image_tmpl<char> (Data, thumb, new_x, x_off, y_off, width); break;
	default: *img = NULL; return NC_NOT_FROM_GXSM; 
	}

	if(nc.get_var("viewmode")){
		int vm;
		double c,b;
		
		nc.get_var("viewmode")->get(&vm);
		(*img)->SetViewMode (vm);

		nc.get_var("contrast")->get(&c);
		nc.get_var("bright")->get(&b);
		(*img)->SetTransfer (c,b);
	}


	return NC_READ_OK;
}



int write_png(gchar *file_name, raw_image *img){
// see here for pnglib docu:
// http://www.libpng.org/pub/png/libpng-manual.html

	mainprog_info m;

	m.gamma   = 1.;
	m.width   = img->width();
	m.height  = img->height();
	m.modtime = time(NULL);
	m.infile  = NULL;
	if (! (m.outfile = fopen(file_name, "wb")))
		return -1;
	m.row_pointers = img->row_rgb_pointers();
	m.title  = g_strdup ("nctopng");
	m.author = g_strdup ("Percy Zahl");
	m.desc   = g_strdup ("Gxsm NC data to png");
	m.copyright = g_strdup ("GPL");
	m.email   = g_strdup ("zahl@users.sourceforge.net");
	m.url     = g_strdup ("http://gxsm.sf.net");
	m.have_bg   = FALSE;
	m.have_time = FALSE;
	m.have_text = FALSE;
	m.pnmtype   = 6; // TYPE_RGB
	m.sample_depth = 8;
	m.interlaced= FALSE;

	writepng_init (&m);
	writepng_encode_image (&m);
	writepng_encode_finish (&m);
	writepng_cleanup (&m);

	g_free (m.title);
	g_free (m.author);
	g_free (m.desc);
	g_free (m.copyright);
	g_free (m.email);
	g_free (m.url);

	return 0;
}
/* ****************************
              main
   ***************************/
int main(int argc, const char *argv[]) {
	int thumb = 1;
	int new_x = 0;
	int x_off = 0, y_off = 0, width = 0;	
	int verbose = 0;
	int noquick = 0;
	int minmax = 0;
	int help = 0;
	gchar *filename;
	gchar *destinationfilename;
	raw_image *img = NULL;
	poptContext optCon; 

	struct poptOption optionsTable[] = {
		{"x-offset",'x',POPT_ARG_INT,&x_off,0,"x-offset",NULL},
		{"y-offset",'y',POPT_ARG_INT,&y_off,0,"y-offset",NULL},
		{"width",'w',POPT_ARG_INT,&width,0,"width",NULL},
		{"size",'s',POPT_ARG_INT,&new_x,0,"Size in x-direction.",NULL},
		{"verbose",'v',POPT_ARG_NONE,&verbose,1,"Display information.",NULL},
		{"noquick",'n',POPT_ARG_NONE,&noquick,1,"No-quick, default: auto analysis",NULL},
		{"minmax",'m',POPT_ARG_NONE,&minmax,0,"No-statistical min max, plain min max range scaling",NULL},
		{"help",'h',POPT_ARG_NONE,&help,1,"Print help.",NULL},
		{ NULL, 0, 0, NULL, 0 }
	};

	optCon = poptGetContext(NULL, argc, argv, optionsTable, 0);
	poptSetOtherOptionHelp(optCon, "gxsmfile.nc [outfile.png]");
	while (poptGetNextOpt(optCon) >= 0) {;}	//Now parse until no more options.

	if ((argc < 2 )||(help)) { 
		poptPrintHelp(optCon, stderr, 0);
		exit(1);
	}
		
	filename = g_strdup(poptGetArg(optCon));
	destinationfilename = g_strdup(poptGetArg(optCon));

	if (destinationfilename == NULL){
		destinationfilename = g_strjoin(NULL, filename, ".png", NULL);
		// using simple join. if you need more sophisticated
		// have a look at 'mmv' for suffix handling.
	}

	if(verbose){
		if (new_x == 0)	
			std::cout << "Thumbnail-size" << std::endl;
		else
			std::cout << "Rescaling to new x-size = " << new_x << std::endl;
		std::cout << "Sourcefile: " << filename << std::endl;
		std::cout << "Destinationfile: " << destinationfilename << std::endl;
	}

	if (new_x > 0)
		thumb = 0;
	
	if ( x_off + y_off + width > 0){
		if ( (x_off==0) || (y_off==0) || (width==0) ) {
			std::cout << "Please use -x and -y and -w together."<< std::endl;
			exit(-1);
		}
		if(verbose){
			std::cout << "Offset: " << x_off << " x " << y_off << std::endl;
			std::cout << "Width set to: " << width << std::endl;
		}
	}
		
	switch (netcdf_read (filename, &img, thumb, new_x, x_off, y_off, width)){
		case NC_READ_OK: break;
		case NC_OPEN_FAILED: 
			std::cerr << "Error opening NC file >" << filename << "<" << std::endl; 
			exit(-1);
			break;
		case NC_NOT_FROM_GXSM:
			std::cerr << "Sorry, can't use this NC file >" << filename << "<" << std::endl 
				 << "Hint: doesn't look like a Gxsm nc data file!" << std::endl; 
			exit(-1);
			break;
	}
		
	if  (!img){
		std::cerr << "Error while creating image from NC file." << std::endl;
		exit(-1);
	}
		
	if (verbose)
		std::cout << "Converting ..." << std::endl; 
		
	if (noquick)
		img->quick_rgb(FALSE, minmax?TRUE:FALSE);
	else
		img->quick_rgb(TRUE, minmax?TRUE:FALSE);

	if (verbose)
		std::cout << "Writing >" << destinationfilename << "<" << std::endl; 
		
	write_png(destinationfilename, img);

	if(verbose)
		std::cout << "Writing complete." << std::endl;
		
	g_free(filename);
	g_free(destinationfilename);
	poptFreeContext(optCon);
	exit(0);
}
