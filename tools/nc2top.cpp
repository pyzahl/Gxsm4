/* -*-coding: utf-8;-*- */

#include "nc2top.h"

#include <netcdfcpp.h>

/*----------------------------------------------------------------------
convert *.nc to WSxM file format
----------------------------------------------------------------------*/
int main(int argn, char *argv[])
{
    OUTPUT
	("nc2top:\tRunning nc2top\tCopyright by Thorsten Wagner in 2007--2011\n");
    char input[WSXM_MAXCHARS], output[WSXM_MAXCHARS];
    char unit[WSXM_MAXCHARS];
    int argnumoffset = 0;
    ClearString(input);
    ClearString(output);
    ClearString(unit);
    double dz = 1;


	/*----------------------------------------------------------------------
	do some checking: correct suffix, responsible, file opening, .. ?
	and set up some variables to handle top-file
	----------------------------------------------------------------------*/
    if (argn == 1) {
	printf
	    ("nc2top:\tNeed at least one argument for input file. Type nc2top -h for help\n");
	return FIO_NO_NAME;
    } else {
	if (argn <= 2
	    && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))) {
	    printf("\nnc2top: Converts GXSM (.nc) to WSxM (.top) files\n");
	    printf("usage: nc2top [-v] Input [Output]\n\n");
	    printf
		("- set -v for verbose mode. Otherwise nc2top will create no output at all!\n");
	    printf("- Input needs to be a valid nc file.\n");
	    printf
		("- You might want to enter the filename where the converted data shall be saved.\n  If you don't, nc2top will assume the same filename as the .nc file but with the .top extension\n\n");
	    return 0;
	}

	if (argn >= 2
	    && (!strcmp(argv[1], "--verbose") || !strcmp(argv[1], "-v"))) {
	    verbose = 1;
	    OUTPUT("nc2top:\tVerbose mode\n");
	    argnumoffset = 1;
	}
	switch (argn - argnumoffset) {
	case 2:
	    {
		strncpy(input, argv[argn - 1], WSXM_MAXCHARS);
		ClearString(output);
		strncat(output, argv[argn - 1],
			-2 + strlen(argv[argn - 1]));
		strncat(output, "top", 3);
		break;
	    }
	case 3:
	    {
		strncpy(input, argv[argn - 2], WSXM_MAXCHARS);
		strncpy(output, argv[argn - 1], WSXM_MAXCHARS);
		break;
	    }
	}
	OUTPUT("nc2top:\tHaving %i arguments\n", argn - 1);
	OUTPUT("nc2top:\t1. argument: %s\n", input);
	OUTPUT("nc2top:\t2. argument: %s\n", output);
    }

    // some low level error checking
    if (input == NULL || output == NULL) {
	printf("nc2top:\tno files defined\n");
	return FIO_NO_NAME;
    }
    // check if we like to handle this
    if (strlen(input) < 3 || strncmp(input + strlen(input) - 3, ".nc", 3)) {
	printf("nc2top:\tsource file is not of type netcdf\n");
	return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
    }

    if (strlen(output) < 4
	|| (strncmp(output + strlen(output) - 4, ".top", 4)
	    && strncmp(output + strlen(output) - 4, ".gsi", 4)
	    && strncmp(output + strlen(output) - 4, ".mpp", 4))) {
	printf
	    ("nc2top:\tdestionation file is not of type WSxM, CITS or MPP\n");
	return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
    }

	/*----------------------------------------------------------------------
	declare variables for internal use
	----------------------------------------------------------------------*/

    // several buffer variables
    guint rows, columns, time, value;
    guint buffer_uint;
    double buffer_double;
    char buffer_string1[WSXM_MAXCHARS];
    char buffer_string2[WSXM_MAXCHARS];
    char z_ampl[WSXM_MAXCHARS];

    rows = columns = time = value = 0;

	/*----------------------------------------------------------------------
	Initialize WSxM header
	----------------------------------------------------------------------*/
    // create handle for WSxM 
    WSxM_HEADER header;
    // header must be initialised
    HeaderInit(&header);

    // variables to handle netcdf file
    OUTPUT("nc2top:\tusing netcdf library %s\n", nc_inq_libvers());
    NcFile nc(input);
    NcError ncerr(NcError::silent_nonfatal);
    NcAtt *ap;
    NcVar *vp;
    if (!nc.is_valid()) {
	printf("nc2top:\tfailed to open source file\n");
	return FIO_OPEN_ERR;
    }

	/*----------------------------------------------------------------------
	Checkin source file structure. 
	Until now only GXSM data of type "H" are supported.
	----------------------------------------------------------------------*/
    OUTPUT("nc2top:\tchecking source file structure\n");
    NcVar *Data = NULL;
    if ((Data = nc.get_var("H"))) {	// standard "SHORT" Topo Scan ?
	OUTPUT("nc2top:\tstandard topography\n");
	HeaderSetAsString(&header, IMAGE_HEADER_GENERAL_INFO,
			  IMAGE_HEADER_GENERAL_INFO_DATA_TYPE,
			  strndup("short", WSXM_MAXCHARS));
	// read out dimensions of nc_variable "H"; define number of pixels, layers, ...
	if (!(nc.get_var("H")->get_dim(0) == NULL)) {
	    time = nc.get_var("H")->get_dim(0)->size();
	    OUTPUT("nc2top:\t%i time layers annouced in header\n", time);
	} else {
	    printf("nc2top:\tno time dimension specified\n");
	    return FIO_INVALID_FILE;
	}
	if (!(nc.get_var("H")->get_dim(1) == NULL)) {
	    value = nc.get_var("H")->get_dim(1)->size();
	    OUTPUT("nc2top:\t%i value layers announced in header\n",
		   value);
	} else {
	    printf("nc2top:\tno value-dimension specified\n");
	    return FIO_INVALID_FILE;
	}
	if (!(nc.get_var("H")->get_dim(2) == NULL)) {
	    snprintf(buffer_string2, WSXM_MAXCHARS, "%ld",
		     nc.get_var("dimx")->get_dim(0)->size());
	    HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO,
			   IMAGE_HEADER_GENERAL_INFO_NUM_COLUMNS,
			   buffer_string2);
	    columns = nc.get_var("H")->get_dim(2)->size();
	    OUTPUT("nc2top:\t%i columns anounced in header\n", columns);
	} else {
	    printf("nc2top:\tno x-dimension specified\n");
	    return FIO_INVALID_FILE;
	}
	if (!(nc.get_var("H")->get_dim(3) == NULL)) {
	    snprintf(buffer_string2, WSXM_MAXCHARS, "%ld",
		     nc.get_var("dimy")->get_dim(0)->size());
	    HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO,
			   IMAGE_HEADER_GENERAL_INFO_NUM_ROWS,
			   buffer_string2);
	    rows = nc.get_var("H")->get_dim(3)->size();
	    OUTPUT("nc2top:\t%i rows anounced in header\n", rows);
	} else {
	    printf("nc2top:\tno y-dimension specifiedt\n");
	    return FIO_INVALID_FILE;
	}
    } else {
	if ((Data = nc.get_var("Intensity"))) {	// Diffract Scan "LONG" ?
	    //printf("nc2top:\tintensity field - not supported yet\n");	// used by "Intensity" -- diffraction counts
	    //return FIO_INVALID_FILE;
		OUTPUT("nc2top:\tlong field - will be saved as double!!!\n");
		HeaderSetAsString(&header, IMAGE_HEADER_GENERAL_INFO,
				  IMAGE_HEADER_GENERAL_INFO_DATA_TYPE,
				  strndup("double", WSXM_MAXCHARS));
		// read out dimensions of nc_variable "Intensity"; define number of pixels, layers, ...
		if (!(nc.get_var("Intensity")->get_dim(0) == NULL)) {
		    time = nc.get_var("Intensity")->get_dim(0)->size();
		    OUTPUT("nc2top:\t%i time layers annouced in header\n",
			   time);
		} else {
		    printf("nc2top:\tno time dimension specified\n");
		    return FIO_INVALID_FILE;
		}
		if (!(nc.get_var("Intensity")->get_dim(1) == NULL)) {
		    value = nc.get_var("Intensity")->get_dim(1)->size();
		    OUTPUT
			("nc2top:\t%i value layers announced in header\n",
			 value);
		} else {
		    printf("nc2top:\tno value-dimension specified\n");
		    return FIO_INVALID_FILE;
		}
		if (!(nc.get_var("Intensity")->get_dim(2) == NULL)) {
		    snprintf(buffer_string2, WSXM_MAXCHARS, "%ld",
			     nc.get_var("dimx")->get_dim(0)->size());
		    HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO,
				   IMAGE_HEADER_GENERAL_INFO_NUM_COLUMNS,
				   buffer_string2);
		    columns = nc.get_var("Intensity")->get_dim(2)->size();
		    OUTPUT("nc2top:\t%i columns anounced in header\n",
			   columns);
		} else {
		    printf("nc2top:\tno x-dimension specified\n");
		    return FIO_INVALID_FILE;
		}
		if (!(nc.get_var("Intensity")->get_dim(3) == NULL)) {
		    snprintf(buffer_string2, WSXM_MAXCHARS, "%ld",
			     nc.get_var("dimy")->get_dim(0)->size());
		    HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO,
				   IMAGE_HEADER_GENERAL_INFO_NUM_ROWS,
				   buffer_string2);
		    rows = nc.get_var("Intensity")->get_dim(3)->size();
		    OUTPUT("nc2top:\t%i rows anounced in header\n", rows);
		} else {
		    printf("nc2top:\tno y-dimension specifiedt\n");
		    return FIO_INVALID_FILE;
		}

	} else {
	    if ((Data = nc.get_var("FloatField"))) {	// Float ?
		OUTPUT("nc2top:\tfloat field - will be saved as double!!!\n");
		HeaderSetAsString(&header, IMAGE_HEADER_GENERAL_INFO,
				  IMAGE_HEADER_GENERAL_INFO_DATA_TYPE,
				  strndup("double", WSXM_MAXCHARS));
		// read out dimensions of nc_variable "FloatField"; define number of pixels, layers, ...
		if (!(nc.get_var("FloatField")->get_dim(0) == NULL)) {
		    time = nc.get_var("FloatField")->get_dim(0)->size();
		    OUTPUT("nc2top:\t%i time layers annouced in header\n",
			   time);
		} else {
		    printf("nc2top:\tno time dimension specified\n");
		    return FIO_INVALID_FILE;
		}
		if (!(nc.get_var("FloatField")->get_dim(1) == NULL)) {
		    value = nc.get_var("FloatField")->get_dim(1)->size();
		    OUTPUT
			("nc2top:\t%i value layers announced in header\n",
			 value);
		} else {
		    printf("nc2top:\tno value-dimension specified\n");
		    return FIO_INVALID_FILE;
		}
		if (!(nc.get_var("FloatField")->get_dim(2) == NULL)) {
		    snprintf(buffer_string2, WSXM_MAXCHARS, "%ld",
			     nc.get_var("dimx")->get_dim(0)->size());
		    HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO,
				   IMAGE_HEADER_GENERAL_INFO_NUM_COLUMNS,
				   buffer_string2);
		    columns = nc.get_var("FloatField")->get_dim(2)->size();
		    OUTPUT("nc2top:\t%i columns anounced in header\n",
			   columns);
		} else {
		    printf("nc2top:\tno x-dimension specified\n");
		    return FIO_INVALID_FILE;
		}
		if (!(nc.get_var("FloatField")->get_dim(3) == NULL)) {
		    snprintf(buffer_string2, WSXM_MAXCHARS, "%ld",
			     nc.get_var("dimy")->get_dim(0)->size());
		    HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO,
				   IMAGE_HEADER_GENERAL_INFO_NUM_ROWS,
				   buffer_string2);
		    rows = nc.get_var("FloatField")->get_dim(3)->size();
		    OUTPUT("nc2top:\t%i rows anounced in header\n", rows);
		} else {
		    printf("nc2top:\tno y-dimension specifiedt\n");
		    return FIO_INVALID_FILE;
		}
	    } else {
		if ((Data = nc.get_var("DoubleField"))) {	// Double ?
		    OUTPUT("nc2top:\tdouble field\n");
		    HeaderSetAsString(&header, IMAGE_HEADER_GENERAL_INFO,
				      IMAGE_HEADER_GENERAL_INFO_DATA_TYPE,
				      strndup("double", WSXM_MAXCHARS));
		    // read out dimensions of nc_variable "DoubleField"; define number of pixels, layers, ...
		    if (!(nc.get_var("DoubleField")->get_dim(0) == NULL)) {
			time =
			    nc.get_var("DoubleField")->get_dim(0)->size();
			OUTPUT
			    ("nc2top:\t%i time layers annouced in header\n",
			     time);
		    } else {
			printf("nc2top:\tno time dimension specified\n");
			return FIO_INVALID_FILE;
		    }
		    if (!(nc.get_var("DoubleField")->get_dim(1) == NULL)) {
			value =
			    nc.get_var("DoubleField")->get_dim(1)->size();
			OUTPUT
			    ("nc2top:\t%i value layers announced in header\n",
			     value);
		    } else {
			printf("nc2top:\tno value-dimension specified\n");
			return FIO_INVALID_FILE;
		    }
		    if (!(nc.get_var("DoubleField")->get_dim(2) == NULL)) {
			snprintf(buffer_string2, WSXM_MAXCHARS, "%ld",
				 nc.get_var("dimx")->get_dim(0)->size());
			HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO,
				       IMAGE_HEADER_GENERAL_INFO_NUM_COLUMNS,
				       buffer_string2);
			columns =
			    nc.get_var("DoubleField")->get_dim(2)->size();
			OUTPUT("nc2top:\t%i columns anounced in header\n",
			       columns);
		    } else {
			printf("nc2top:\tno x-dimension specified\n");
			return FIO_INVALID_FILE;
		    }
		    if (!(nc.get_var("DoubleField")->get_dim(3) == NULL)) {
			snprintf(buffer_string2, WSXM_MAXCHARS, "%ld",
				 nc.get_var("dimy")->get_dim(0)->size());
			HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO,
				       IMAGE_HEADER_GENERAL_INFO_NUM_ROWS,
				       buffer_string2);
			rows =
			    nc.get_var("DoubleField")->get_dim(3)->size();
			OUTPUT("nc2top:\t%i rows anounced in header\n",
			       rows);
		    } else {
			printf("nc2top:\tno y-dimension specifiedt\n");
			return FIO_INVALID_FILE;
		    }
		} else {
		    if ((Data = nc.get_var("ByteField"))) {	// Byte ?
			printf
			    ("nc2top:\tbyte field - not supported yet\n");
			return FIO_INVALID_FILE;
		    } else {
			if ((Data = nc.get_var("ComplexDoubleField"))) {	// complex
			    printf
				("nc2top:\tcomplex field - not supported yet\n");
			    return FIO_INVALID_FILE;
			} else {
			    if ((Data = nc.get_var("RGBA_ByteField"))) {	// RGBA Byte ?;
				printf
				    ("nc2top:\tRGBA byte field - not supported yet\n");
				return FIO_INVALID_FILE;
			    } else {
				OUTPUT("nc2top:\tno generic GXSM file!\n");
			    }
			}
		    }
		}
	    }
	}
    }
    if (!Data) {		// alien NetCDF File, search for 2D Data
	int n, flg = 0;
	//static char *types[] ={"","byte","char","short","long","float","double"};
	NcVar *vp;
	for (n = 0; (vp = nc.get_var(n)) && !flg; n++) {
	    if (vp->num_dims() == 2) {
		flg = 1;
		//NcDim* dimyd = vp->get_dim(0);
		//NcDim* dimxd = vp->get_dim(1);
		switch (vp->type()) {
		case ncByte:
		    printf("nc2top:\tncByte field\n");
		    return FIO_INVALID_FILE;
		    break;
		case ncChar:
		    printf("nc2top:\tncChar field\n");
		    return FIO_INVALID_FILE;
		    break;
		case ncShort:
		    printf("nc2top:\tncShort field\n");
		    return FIO_INVALID_FILE;
		    break;
		case ncLong:
		    printf("nc2top:\tncLong field\n");
		    return FIO_INVALID_FILE;
		    break;
		case ncFloat:
		    printf("nc2top:\tncFloat field\n");
		    return FIO_INVALID_FILE;
		    break;
		case ncDouble:
		    printf("nc2top:\tncDouble field\n");
		    return FIO_INVALID_FILE;
		    break;
		default:
		    printf("nc2top:\tinput not defined!!!\n");
		    flg = 0;
		    return FIO_INVALID_FILE;
		    break;
		}
	    }
	}
	if (flg) {
	    OUTPUT("nc2top:\tnative netcdf 2d data array\n");
	    return FIO_OK;
	} else {
	    OUTPUT("nc2top:\tinput not defined!!!\n");
	    return FIO_NO_NETCDFXSMFILE;
	}
    }

//check if file is WsXM
    if ((value == 1 && !strncmp(output + strlen(output) - 4, ".gsi", 4))) {
	printf("nc2top:\tInput file is not CITS but SxM!!!\n");
	return FIO_INVALID_FILE;
    }
    if ((time == 1 && !strncmp(output + strlen(output) - 4, ".mpp", 4))) {
	printf("nc2top:\tInput file is not MPP but SxM!!!\n");
	return FIO_INVALID_FILE;
    }

//check if file is CITS
    if ((value > 1 && !strncmp(output + strlen(output) - 4, ".top", 4))) {
	//if no output file was given   
	if (argn == 2 + argnumoffset) {
	    ClearString(output);
	    strncat(output, argv[argn - 1], -2 + strlen(argv[argn - 1]));
	    strncat(output, "gsi", 3);
	    OUTPUT
		("nc2top:\tInput file is not top but CITS! Outputfile = %s\n",
		 output);
	} else {
	    OUTPUT
		("nc2top:\tAttention: Input file is not TOP but CITS!!!\n");
//              return FIO_INVALID_FILE;
	}
    }
//check if file is MPP
    if ((time > 1 && !strncmp(output + strlen(output) - 4, ".top", 4))) {
	//if no output file was given   
	if (argn == 2 + argnumoffset) {
	    ClearString(output);
	    strncat(output, argv[argn - 1], -2 + strlen(argv[argn - 1]));
	    strncat(output, "mpp", 3);
	    OUTPUT
		("nc2top:\tInput file is not top but MPP! Outputfile = %s\n",
		 output);
	} else {
	    OUTPUT
		("nc2top:\tAttention: Input file is not TOP but MPP!!!\n");
//              return FIO_INVALID_FILE;
	}
    }


    if (value > 999 || time > 999) {
	printf("nc2top:\tToo many Images!!!\n");
	return FIO_INVALID_FILE;
    }

	/*----------------------------------------------------------------------
	Open Destination File
	----------------------------------------------------------------------*/

    // File pointer
    FILE *pDestinationFile = NULL;
    // set destination for write 
    char *sDestinationFileName = (char *) output;
    // the header is ASCII. So opening the header in text mode
    pDestinationFile = fopen(sDestinationFileName, "wt");
    if (pDestinationFile == NULL) {
	printf("nc2top:\tfailed to open destination file\n");
	return FIO_OPEN_ERR;
    }


	/*----------------------------------------------------------------------
	Header Sektion: General_Info
	----------------------------------------------------------------------*/
    OUTPUT("nc2top:\tstart creating header\n");
    OUTPUT("nc2top:\tworking in WSxM header section General Info\n");
    HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO,
		   IMAGE_HEADER_GENERAL_INFO_ACQ_CHANNEL, strndup("TOPO",
								  WSXM_MAXCHARS));
    if (!(nc.get_att("InstrumentType") == NULL)) {
	OUTPUT("nc2top:\tfound attribute InstrumentType\n");
	HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO,
		       IMAGE_HEADER_GENERAL_INFO_HEAD_TYPE,
		       nc.get_att("InstrumentType")->values()->
		       as_string(0));
    } else {
	OUTPUT
	    ("nc2top:\tdid not found attribute InstrumentType; assume STM\n");
	HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO,
		       IMAGE_HEADER_GENERAL_INFO_HEAD_TYPE, strndup("STM",
								    WSXM_MAXCHARS));
    }




    // Z_Amplitude = ZMax - ZMin with unit
    ClearString(z_ampl);
    if (!(nc.get_var("rangez") == NULL)) {
	// gxsm variable rangex is NOT always in AA so unit checking is needed!
	OUTPUT("nc2top:\tfound variable rangez\n");
	if (!GetUnit(unit, (NcVar *) nc.get_var("rangez"))) {
	    // no unit found in rangez
	    OUTPUT
		("nc2top:\tdid not found variable rangez; assume a.u.\n");
	    SetUnit(z_ampl, strndup("a.u.", WSXM_MAXCHARS), 1.0);
	} else {
	    SetUnit(z_ampl, unit,
		    nc.get_var("rangez")->values()->as_double(0));
	}
    } else {
	// no rangez found within file so set to arbitrary value
	OUTPUT("nc2top:\tdid not found variable rangez; assume a.u.\n");
	SetUnit(z_ampl, strndup("a.u.", WSXM_MAXCHARS), 1.0);
    }
    ReplaceChar(z_ampl);
//      HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO, IMAGE_HEADER_GENERAL_INFO_Z_AMPLITUDE, buffer_string2);

    OUTPUT("nc2top:\tWSxM header section General Info written\n");


	/*----------------------------------------------------------------------
	Get dz and check its unit
	----------------------------------------------------------------------*/



    //dz is the variable to calculate the real values from short bit-code
    //Check which unit dz is
    ClearString(unit);
    if (nc.get_var("dz") == NULL) {
	OUTPUT("nc2top:\tdz not found. Assiuming dz=1\n");
    } else {
	dz = nc.get_var("dz")->values()->as_double(0);

	//We only need the following conversion if we want to convert short->double

	if (time > 1) {
	    GetUnit(unit, (NcVar *) nc.get_var("rangez"));
	    if (!strcmp(unit, "pA"))
		dz = dz * 1000.;
	    if (!strcmp(unit, "nm"))
		dz = dz / 10.;
	    if (!strcmp(unit, "um"))
		dz = dz / 10000.;
	    if (!strcmp(unit, "mm"))
		dz = dz / 10000000.;
	}
    }

	/*----------------------------------------------------------------------
	Header Sektion: Maxmins list
	----------------------------------------------------------------------*/
    float rampdata[value];	//Spectroscopy images ramp values

    OUTPUT("nc2top:\tworking in WSxM header section Maxmins list\n");

    char inumber[4];
    char outstring[WSXM_MAXCHARS];
    char cBuf[WSXM_MAXCHARS];
    if (value > 1) {
	snprintf(cBuf, 30, "%i", value);
	HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO,
		       IMAGE_HEADER_GENERAL_INFO_POINTS_PER_IV, cBuf);
    }
    NcType type;
    type = Data->type();

    double minsd = 0;
    double maxsd = 0;

    if (type == ncFloat) {
	float databuf[1][1][rows][columns];
	for (guint i = 0; i < value; i++) {
	    //Get Image Data and calculate minimum and maximum spectroscopy data for maxmins list or Miscellaneous Info
	    Data->set_cur(0, i, 0, 0);
	    Data->get(&databuf[0][0][0][0], (long int) 1, (long int) 1,
		      (long int) columns, (long int) rows);
	    minsd = databuf[0][0][0][0];
	    maxsd = databuf[0][0][0][0];
	    for (unsigned int y = 0; y < rows; y++) {
		for (unsigned int x = 0; x < columns; x++) {
		    minsd =
			minsd >
			databuf[0][0][y][x] ? databuf[0][0][y][x] : minsd;
		    maxsd =
			maxsd >
			databuf[0][0][y][x] ? maxsd : databuf[0][0][y][x];
		}
	    }
	    if (value > 1) {	//CITS FILE
		itoc(i, inumber);	// converts integer to formated char*: 51->051
		snprintf(cBuf, 40, "%f", maxsd);
		snprintf(outstring, 40,
			 "Image %c%c%c maximum spectroscopy data",
			 inumber[0], inumber[1], inumber[2]);
		HeaderAddValue(&header, IMAGE_HEADER_MAXMINSLIST,
			       outstring, cBuf);
		snprintf(outstring, 40,
			 "Image %c%c%c minimum spectroscopy data",
			 inumber[0], inumber[1], inumber[2]);
		snprintf(cBuf, 40, "%f", minsd);
		HeaderAddValue(&header, IMAGE_HEADER_MAXMINSLIST,
			       outstring, cBuf);
	    }
	}
    }

    if (type == ncDouble) {
	double databuf[1][1][rows][columns];
	for (guint i = 0; i < value; i++) {
	    //Get Image Data and calculate minimum and maximum spectroscopy data for maxmins list or Miscellaneous Info
	    Data->set_cur(0, i, 0, 0);
	    Data->get(&databuf[0][0][0][0], (long int) 1, (long int) 1,
		      (long int) columns, (long int) rows);
	    minsd = databuf[0][0][0][0];
	    maxsd = databuf[0][0][0][0];
	    for (unsigned int y = 0; y < rows; y++) {
		for (unsigned int x = 0; x < columns; x++) {
		    minsd =
			minsd >
			databuf[0][0][y][x] ? databuf[0][0][y][x] : minsd;
		    maxsd =
			maxsd >
			databuf[0][0][y][x] ? maxsd : databuf[0][0][y][x];
		}
	    }
	    if (value > 1) {	//CITS FILE
		itoc(i, inumber);	// converts integer to formated char*: 51->051
		snprintf(cBuf, 40, "%f", maxsd);
		snprintf(outstring, 40,
			 "Image %c%c%c maximum spectroscopy data",
			 inumber[0], inumber[1], inumber[2]);
		HeaderAddValue(&header, IMAGE_HEADER_MAXMINSLIST,
			       outstring, cBuf);
		snprintf(outstring, 40,
			 "Image %c%c%c minimum spectroscopy data",
			 inumber[0], inumber[1], inumber[2]);
		snprintf(cBuf, 40, "%f", minsd);
		HeaderAddValue(&header, IMAGE_HEADER_MAXMINSLIST,
			       outstring, cBuf);
	    }
	}
    }

    if (type == ncShort) {
	short int databuf[1][1][rows][columns];
	Data->set_cur(0, 0, 0, 0);
	Data->get(&databuf[0][0][0][0], (long int) 1, (long int) 1,
		  (long int) columns, (long int) rows);
	minsd = databuf[0][0][0][0];
	maxsd = databuf[0][0][0][0];

	for (guint i = 0; i < value; i++) {
	    for (guint j = 0; j < time; j++) {
		//Get Image Data and calculate minimum and maximum spectroscopy data for maxmins list or Miscellaneous Info
		Data->set_cur(j, i, 0, 0);
		Data->get(&databuf[0][0][0][0], (long int) 1, (long int) 1,
			  (long int) columns, (long int) rows);
		for (unsigned int y = 0; y < rows; y++) {
		    for (unsigned int x = 0; x < columns; x++) {
			minsd =
			    minsd >
			    databuf[0][0][y][x] ? databuf[0][0][y][x] :
			    minsd;
			maxsd =
			    maxsd >
			    databuf[0][0][y][x] ? maxsd :
			    databuf[0][0][y][x];
		    }
		}
		if (value > 1) {	//CITS FILE
		    itoc(i, inumber);	// converts integer to formated char*: 51->051
		    snprintf(cBuf, 40, "%f", maxsd * dz);
		    snprintf(outstring, 40,
			     "Image %c%c%c maximum spectroscopy data",
			     inumber[0], inumber[1], inumber[2]);
		    HeaderAddValue(&header, IMAGE_HEADER_MAXMINSLIST,
				   outstring, cBuf);
		    snprintf(outstring, 40,
			     "Image %c%c%c minimum spectroscopy data",
			     inumber[0], inumber[1], inumber[2]);
		    snprintf(cBuf, 40, "%f", minsd * dz);
		    HeaderAddValue(&header, IMAGE_HEADER_MAXMINSLIST,
				   outstring, cBuf);
		}
	    }
	}
    }



    if (value <= 1) {		//TOPO FILE
	ClearString(z_ampl);
	ClearString(unit);

	GetUnit(unit, (NcVar *) nc.get_var("dz"));

	snprintf(z_ampl, WSXM_MAXCHARS, "%f", maxsd * dz);
	HeaderAddValue(&header, IMAGE_HEADER_MISC_INFO,
		       IMAGE_HEADER_MISC_INFO_MAXIMUM, z_ampl);

	ClearString(z_ampl);
	snprintf(z_ampl, WSXM_MAXCHARS, "%f", minsd * dz);
	HeaderAddValue(&header, IMAGE_HEADER_MISC_INFO,
		       IMAGE_HEADER_MISC_INFO_MINIMUM, z_ampl);


	if (time > 1) {
	    ClearString(z_ampl);
	    snprintf(z_ampl, 40, "%i", (int) minsd);
	    HeaderAddValue(&header, IMAGE_HEADER_MISC_INFO,
			   IMAGE_HEADER_MISC_INFO_MINIMUM_DAC, z_ampl);
	    snprintf(z_ampl, 40, "%i", (int) maxsd);
	    HeaderAddValue(&header, IMAGE_HEADER_MISC_INFO,
			   IMAGE_HEADER_MISC_INFO_MAXIMUM_DAC, z_ampl);
	}

	ClearString(z_ampl);
	SetUnit(z_ampl, unit, (maxsd - minsd) * dz);

    }


    HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO,
		   IMAGE_HEADER_GENERAL_INFO_Z_AMPLITUDE, z_ampl);

    OUTPUT("nc2top:\tWSxM header section maxmins list written\n");

	/*----------------------------------------------------------------------
	Header Sektion: Ramp value list
	----------------------------------------------------------------------*/
    if (value > 1) {
	OUTPUT
	    ("nc2top:\tworking in WSxM header section Spectroscopy images ramp value list\n");

	NcVar *ramp;
	// Write Image bias List to "Spectroscopy images ramp value list" section in header
	ramp = nc.get_var("value");
	ramp->get(&rampdata[0], (long int) value);

	for (guint i = 0; i < value; i++) {
	    itoc(i, inumber);	// converts integer to formated char*: 51->051
	    snprintf(outstring, 40, "Image %c%c%c", inumber[0], inumber[1],
		     inumber[2]);
	    snprintf(cBuf, 40, "%f V", rampdata[i]);
	    HeaderAddValue(&header, IMAGE_HEADER_RAMPVALUES, outstring,
			   cBuf);
	}

	OUTPUT
	    ("nc2top:\tWSxM header section Spectroscopy images ramp value list written\n");
    }



	/*----------------------------------------------------------------------
	Header for movie files
	----------------------------------------------------------------------*/

    if (time > 1) {
	NcVar *scantime = NULL;

	double times[time];
	scantime = nc.get_var("time");

	scantime->get(&times[0], (long int) time);

	for (guint i = 0; i < time; i++) {
	    itoc(i, inumber);	// converts integer to formated char*: 51->051
	    snprintf(outstring, WSXM_MAXCHARS, "Frame 0%s", inumber);
	    snprintf(cBuf, WSXM_MAXCHARS, "%f sec", times[i]);
	    HeaderAddValue(&header, IMAGE_HEADER_FRAMES, outstring, cBuf);
	}


	HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO,
		       IMAGE_HEADER_GENERAL_INFO_TOTALTIME, cBuf);

	snprintf(outstring, WSXM_MAXCHARS, "%i", time);
	HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO,
		       IMAGE_HEADER_GENERAL_INFO_NUM_FRAMES, outstring);
	HeaderAddValue(&header, IMAGE_HEADER_GENERAL_INFO,
		       IMAGE_HEADER_GENERAL_INFO_PROCESSES,
		       strndup("plane", WSXM_MAXCHARS));

	OUTPUT
	    ("nc2top:\tWSxM header section Frame Synchronization written\n");

    }

	/*----------------------------------------------------------------------
	Header Sektion: Control
	----------------------------------------------------------------------*/
    OUTPUT("nc2top:\tworking in WSxM heaader section Control\n");
    // define size (x/y amplitude)of the image 
    ClearString(buffer_string2);
    if (nc.get_var("rangex")) {
	// gxsm variable x is always in AA so no unit checking is needed
	SetUnit(buffer_string2, strndup("AA", WSXM_MAXCHARS),
		nc.get_var("rangex")->values()->as_double(0));
    } else {
	// no rangex found within file so set to arbitrary value
	SetUnit(buffer_string2, strndup("a.u.", WSXM_MAXCHARS), 1.0);
    }
    ReplaceChar(buffer_string2);
    HeaderAddValue(&header, IMAGE_HEADER_CONTROL,
		   IMAGE_HEADER_CONTROL_X_AMPLITUDE, buffer_string2);

    ClearString(buffer_string2);
    if (nc.get_var("rangey")) {
	// gxsm variable rangex is always in AA so no unit checking is needed
	SetUnit(buffer_string2, strndup("AA", WSXM_MAXCHARS),
		nc.get_var("rangey")->values()->as_double(0));
    } else {
	// no rangey found within file so set to arbitrary value
	SetUnit(buffer_string2, strndup("a.u.", WSXM_MAXCHARS), 1.0);
    }
    ReplaceChar(buffer_string2);
    HeaderAddValue(&header, IMAGE_HEADER_CONTROL,
		   IMAGE_HEADER_CONTROL_Y_AMPLITUDE, buffer_string2);


    // Read out old parameter, if new one is available overwrite
    // Find correct value for set point
    buffer_double = 0;
    ClearString(buffer_string2);
    SetUnit(buffer_string2, strndup("V", WSXM_MAXCHARS),
	    double (buffer_double));
    if (nc.get_var("dsp_itunnel")) {
	buffer_double = nc.get_var("dsp_itunnel")->values()->as_double(0);
	SetUnit(buffer_string2, strndup("nA", WSXM_MAXCHARS),
		double (buffer_double));
	OUTPUT("nc2top:\tuse old dsp_itunnel: %f nA\n", buffer_double);
	OUTPUT
	    ("nc2top:\tset current unit to default (nA). Is %f nA correct?\n",
	     buffer_double);
    } else if (nc.get_var("sranger_hwi_current_set_point")) {
	buffer_double =
	    nc.get_var("sranger_hwi_current_set_point")->values()->
	    as_double(0);
	SetUnit(buffer_string2, strndup("nA", WSXM_MAXCHARS),
		double (buffer_double));
	OUTPUT
	    ("nc2top:\toverwrite with sranger_hwi_current_set_point: %f nA\n",
	     buffer_double);
    } else if (nc.get_var("sranger_hwi_voltage_set_point")) {
	buffer_double =
	    nc.get_var("sranger_hwi_voltage_set_point")->values()->
	    as_double(0);
	SetUnit(buffer_string2, strndup("V", WSXM_MAXCHARS),
		double (buffer_double));
	OUTPUT
	    ("nc2top:\toverwrite with sranger_hwi_voltage_set_point: %f V\n",
	     buffer_double);
    } else if (nc.get_var("sranger_mk2_hwi_mix0_current_set_point")) {
	buffer_double =
	    nc.get_var("sranger_mk2_hwi_mix0_current_set_point")->values()->
	    as_double(0);
	SetUnit(buffer_string2, strndup("nA", WSXM_MAXCHARS),
		double (buffer_double));
	OUTPUT
	    ("nc2top:\toverwrite with sranger_mk2_hwi_mix0_current_set_point: %f nA\n",
	     buffer_double);
    /*} else if (nc.get_var("sranger_mk2_hwi_voltage_set_point")) {
	buffer_double =
	    nc.get_var("sranger_mk2_hwi_voltage_set_point")->values()->
	    as_double(0);
	SetUnit(buffer_string2, strndup("V", WSXM_MAXCHARS),
		double (buffer_double));
	OUTPUT
	    ("nc2top:\toverwrite with sranger_mk2_hwi_voltage_set_point: %f V\n",
	     buffer_double);*/ // not used with mk2??? have to check, tw 12.10.2012
    } else {
	SetUnit(buffer_string2, strndup("V", WSXM_MAXCHARS), 0.0);
    }
    ReplaceChar(buffer_string2);
    HeaderAddValue(&header, IMAGE_HEADER_CONTROL,
		   IMAGE_HEADER_CONTROL_SET_POINT, buffer_string2);

    // Find correct value for tunneling bias
    ClearString(buffer_string2);
    if (nc.get_var("dsp_utunnel")) {
	buffer_double = nc.get_var("dsp_utunnel")->values()->as_double(0);
	SetUnit(buffer_string2, strndup("nA", WSXM_MAXCHARS),
		double (buffer_double));
	OUTPUT("nc2top:\tUse old dsp_utunnel: %f V\n", buffer_double);
    }
    if (nc.get_var("sranger_hwi_bias")) {
	buffer_double =
	    nc.get_var("sranger_hwi_bias")->values()->as_double(0);
	SetUnit(buffer_string2, strndup("V", WSXM_MAXCHARS),
		double (buffer_double));
	OUTPUT("nc2top:\toverwrite with sranger_hwi_bias: %f V\n",
	       buffer_double);
    }
    if (nc.get_var("sranger_mk2_hwi_bias")) {
	buffer_double =
	    nc.get_var("sranger_mk2_hwi_bias")->values()->as_double(0);
	SetUnit(buffer_string2, strndup("V", WSXM_MAXCHARS),
		double (buffer_double));
	OUTPUT("nc2top:\toverwrite with sranger_mk2_hwi_bias: %f V\n",
	       buffer_double);
    }	
    ReplaceChar(buffer_string2);
    HeaderAddValue(&header, IMAGE_HEADER_CONTROL,
		   IMAGE_HEADER_CONTROL_BIAS, buffer_string2);

    // Write scan direction
    ClearString(buffer_string2);
    if (nc.get_var("spm_scancontrol")) {
	strncpy(buffer_string2,
		nc.get_var("spm_scancontrol")->values()->as_string(0),
		strlen(nc.get_var("spm_scancontrol")->values()->
		       as_string(0)));
	OUTPUT("nc2top:\tfound attribute scan direction: %s\n",
	       buffer_string2);
	ReplaceChar(buffer_string2);
	if (time <= 1) {	// if we want to convert a movie we must not write the direction!
	    HeaderAddValue(&header, IMAGE_HEADER_CONTROL,
			   IMAGE_HEADER_CONTROL_SCAN_DIRECTION,
			   buffer_string2);
	}
    }
    // X,Y Offset
    ClearString(buffer_string2);
    if (nc.get_var("offsetx")) {
	// gxsm variable offsetx is alwachar *sInput, char *sOutput, char cSeparatorys in AA so no unit checking is needed
	SetUnit(buffer_string2, strndup("AA", WSXM_MAXCHARS),
		nc.get_var("offsetx")->values()->as_double(0));
    } else {
	// no offsetx found within file so set to arbitrary value
	SetUnit(buffer_string2, strndup("a.u.", WSXM_MAXCHARS), 0.0);
    }
    ReplaceChar(buffer_string2);
    HeaderAddValue(&header, IMAGE_HEADER_CONTROL,
		   IMAGE_HEADER_CONTROL_X_OFFSET, buffer_string2);

    ClearString(buffer_string2);
    if (nc.get_var("offsety")) {
	// gxsm variable offsety is always in AA so no unit checking is needed
	SetUnit(buffer_string2, strndup("AA", WSXM_MAXCHARS),
		nc.get_var("offsety")->values()->as_double(0));
    } else {
	// no offsety found within file so set to arbitrary value
	SetUnit(buffer_string2, strndup("a.u.", WSXM_MAXCHARS), 0.0);
    }
    ReplaceChar(buffer_string2);
    HeaderAddValue(&header, IMAGE_HEADER_CONTROL,
		   IMAGE_HEADER_CONTROL_Y_OFFSET, buffer_string2);

    // Z Gain - only a default value until now
    ClearString(buffer_string2);
    sprintf(buffer_string2, "1.0");
    HeaderAddValue(&header, IMAGE_HEADER_CONTROL,
		   IMAGE_HEADER_CONTROL_Z_GAIN, buffer_string2);

    // Rotation
    ClearString(buffer_string2);
    if (nc.get_var("alpha")) {
	SetUnit(buffer_string2, strndup("Deg", WSXM_MAXCHARS),
		nc.get_var("alpha")->values()->as_double(0));
	ReplaceChar(buffer_string2);
	HeaderAddValue(&header, IMAGE_HEADER_CONTROL,
		       IMAGE_HEADER_CONTROL_ROTATION, buffer_string2);
    }
    OUTPUT("nc2top:\tWSxM header section Control written\n");

	/*----------------------------------------------------------------------
	dump all netCDF parameters to header
	----------------------------------------------------------------------*/
    OUTPUT
	("nc2top:\tnow dumping netcdf global attributes to WSxM header\n");
    // dump global netCDF-attributes
    for (int n = 0; (ap = nc.get_att(n)); n++) {
	ClearString(buffer_string1);
	ClearString(buffer_string2);
	strncpy(buffer_string1, ap->name(), WSXM_MAXCHARS);
	strncpy(buffer_string2, ap->values()->as_string(0), WSXM_MAXCHARS);
	KillSpace(buffer_string2);
	HeaderSetAsString(&header,
			  strndup("NetCDF Attributes (global)",
				  WSXM_MAXCHARS), buffer_string1,
			  buffer_string2);
	delete ap;
    }

    // dump netCDF-variables
    OUTPUT("nc2top:\tnow dumping netcdf variables to WSxM header\n");
    for (int n = 0; (vp = nc.get_var(n)); n++) {
	// There are three cases handled:
	// a) variable is a spring: separate lines, ...
	// b) variable is a single value: write to header value with appropriate unit, ...
	// c) variable is multidimensional: not able to handle within header
//              if (vp->type() == NC_CHAR) {// variable contains a string
	if (vp->type() == ncChar) {	// variable contains a string
	    buffer_uint = 0;
	    ClearString(buffer_string1);	// clear variables
	    ClearString(buffer_string2);
	    unsigned int max_uint = WSXM_MAXCHARS;	// buffer variable to trace the max number of chars in string
	    while (buffer_uint + 1 < strlen(vp->values()->as_string(0))
		   && buffer_uint + 1 < max_uint) {
		strcpy(buffer_string1, "");
		// check if there is an empty line
		if (*(vp->values()->as_string(buffer_uint)) == 10) {
		    // empty line
		    buffer_uint++;
		    OUTPUT
			("nc2top:\tblank line in string detected and deleted!\n");
		} else {
		    // no empty line
		    SeparateLine(vp->values()->as_string(buffer_uint),
				 &buffer_string1[0], 10);
		    max_uint -= 2;
		    // make sure that there is no segmentation fault
		    if (strlen(buffer_string2) + strlen(buffer_string1) +
			2 > max_uint) {
			// string is longer than WSXM_MAXCHARS
			OUTPUT
			    ("nc2top:\tstring is too long so it will be cut\n");
			if (((signed int) (max_uint -
					   strlen(buffer_string2) - 2)) >
			    0) {
			    strncat(&buffer_string2[0], &buffer_string1[0],
				    max_uint - strlen(buffer_string2) - 2);
			    strcat(&buffer_string2[0], "\r\n");
			}
			// add one because of the '\n'
			++buffer_uint += strlen(&buffer_string1[0]);
		    } else {
			// length of string is okey
			strcat(&buffer_string2[0], &buffer_string1[0]);
			strcat(&buffer_string2[0], "\r\n");
			// add one because of the '\n'
			++buffer_uint += strlen(&buffer_string1[0]);
		    }
		}
	    }
	    strncpy(&buffer_string1[0], vp->name(), WSXM_MAXCHARS);
	} else {
	    if (vp->num_dims() == 0) {	// dump only variables with dimension zero (containing on "data point")
		// variables will be printed with precission .2f 
		// netcdf variables can contain numbers with higher precission but WSxM supports only .2f
		// you can try .16g for better precission
		ClearString(buffer_string1);
		ClearString(buffer_string2);
		strncpy(&buffer_string1[0], vp->name(), WSXM_MAXCHARS);
		// check if attribute unit is set for variable
		if (vp->get_att("unit")) {
		    // check if nm is only used for display but
		    if (!strncmp
			(vp->get_att("unit")->values()->as_string(0), "1",
			 WSXM_MAXCHARS)) {
			// unit is set to "1" so ignore
			snprintf(&buffer_string2[0], WSXM_MAXCHARS, "%.2f",
				 vp->values()->as_double(0));
		    } else {
			//SetUnit(buffer_string2, vp->get_att("unit")->values()->as_string(0),vp->values()->as_double(0));
			if (!strncmp
			    (vp->get_att("unit")->values()->as_string(0),
			     "AA", WSXM_MAXCHARS)) {
			    // unit is AA so set special character
			    snprintf(&buffer_string2[0], WSXM_MAXCHARS,
				     "%.2f %c", vp->values()->as_double(0),
				     197);
			} else {
			    if (!strncmp
				(vp->get_att("unit")->values()->
				 as_string(0), "deg", WSXM_MAXCHARS)) {
				// unit is degree so set special character
				snprintf(&buffer_string2[0], WSXM_MAXCHARS,
					 "%.2f %c",
					 vp->values()->as_double(0), 176);
			    } else {
				if (!strncmp
				    (vp->get_att("unit")->values()->
				     as_string(0), "Grad",
				     WSXM_MAXCHARS)) {
				    // unit is Grad (why using German unit here?) so set special character
				    snprintf(&buffer_string2[0],
					     WSXM_MAXCHARS, "%.2f %c",
					     vp->values()->as_double(0),
					     176);
				} else {
				    // unit attribute contains something usefull
				    snprintf(&buffer_string2[0],
					     WSXM_MAXCHARS, "%.2f %s",
					     vp->values()->as_double(0),
					     vp->
					     get_att("unit")->values()->
					     as_string(0));
				}
			    }
			}
		    }
		} else {
		    // not unit defined so only convert value
		    snprintf(&buffer_string2[0], WSXM_MAXCHARS, "%.2f",
			     vp->values()->as_double(0));
		}
		if (vp->get_att("Info")) {	// check if native unit is AA, but other is used for display
		    if (strncmp
			(vp->get_att("Info")->values()->as_string(0),
			 "This number is alwalys in Angstroem. Unit is used for display only!",
			 WSXM_MAXCHARS)) {
			snprintf(&buffer_string2[0], WSXM_MAXCHARS,
				 "%.2f %c", vp->values()->as_double(0),
				 197);
		    }
		}
		ReplaceChar(&buffer_string2[0]);	// replace "," by "." as decimal separator
	    } else {
		// the variable contains multidimensional data which can not be dumped to .top-file
		ClearString(buffer_string1);
		ClearString(buffer_string2);
		strncpy(&buffer_string1[0], vp->name(), WSXM_MAXCHARS);
		strncpy(&buffer_string2[0], "Not able to dump variable",
			WSXM_MAXCHARS);
	    }
	}
	// now write buffer_string and szValue to Header. They should contain proper values
	KillSpace(buffer_string2);	// movies must not have spaces at the end of the variable!
	HeaderSetAsString(&header,
			  strndup("NetCDF Variables", WSXM_MAXCHARS),
			  buffer_string1, buffer_string2);
	//Write comments to MISC_INFO section!
	if (!strcmp(buffer_string1, "comment")) {
	    HeaderSetAsString(&header, IMAGE_HEADER_MISC_INFO,
			      IMAGE_HEADER_MISC_INFO_COMMENTS,
			      buffer_string2);
	}
    }
    delete vp;			// clean up netcdf variable

    if (time > 1) {
	HeaderSetAsString(&header, IMAGE_HEADER_GENERAL_INFO, IMAGE_HEADER_GENERAL_INFO_DATA_TYPE, strndup("double", WSXM_MAXCHARS));	// all movies are saved as double files
    }
    HeaderSetAsString(&header, IMAGE_HEADER_MISC_INFO,
		      IMAGE_HEADER_MISC_INFO_SAVEDWITH,
		      strndup("nc2top (beta)", WSXM_MAXCHARS));
    // writing the header in ASCII mode
    if (value > 1) {
	HeaderWrite(&header, pDestinationFile,
		    strndup("CITS File", WSXM_MAXCHARS));
    } else if (time > 1) {
	HeaderWrite(&header, pDestinationFile,
		    strndup("SPM Movie", WSXM_MAXCHARS));
    } else {
	HeaderWrite(&header, pDestinationFile);
    }

    HeaderDestroy(&header);
    OUTPUT("nc2top:\tWSxM header written succesfully\n");

    // close file
    fclose(pDestinationFile);


    // Write Binary Data to output file
    if (value > 1) {
	//for CITS we have to define one topography picture. If there is no bias given in the nc file
	//we will choose 1V as a standard value.
	double topobias = 1.;
	if (nc.get_var("sranger_hwi_bias") != NULL)
	    topobias =
		nc.get_var("sranger_hwi_bias")->values()->as_double(0);
	else if (nc.get_var("sranger_mk2_hwi_bias") != NULL)
	    topobias =
		nc.get_var("sranger_mk2_hwi_bias")->values()->as_double(0);
	else
	    OUTPUT("nc2top:\tTopobias set to default = 1V\n");
	if (WriteData_cits(output, Data, rampdata, topobias) == 1)
	    return 1;		//Write CITS File
    } else if (time > 1) {
	if (WriteData_movie(output, Data, dz) == 1)
	    return 1;		//Write Movie file
    } else {
	if (WriteData_topo(output, Data) == 1)
	    return 1;		//Write Topography file
    }
    return FIO_OK;
}

/*----------------------------------------------------------------------
SetUnit: Adds Unit to String
----------------------------------------------------------------------*/
void SetUnit(char *sValue, char *sUnit, double dValue, double dFactor)
{
    // Global unit table declared in xsm.C
    UnitsTable *UnitPtr = XsmUnitsTable;
    char prec_str[WSXM_MAXCHARS];
    char *finalUnit;
    dValue = dValue * dFactor;	// Use dFactor for scalling
    // Checking for the correct unit of the tunneling current, because
    // WSxM only acceptes 2 decimals
    if (!strcmp(sUnit, "nA")) {
	if (((int) (dValue * IS_PICO_AMP)) == 0) {	// better use pA
	    finalUnit = strdup("pA");
	} else {		// use standard unit
	    finalUnit = strdup(sUnit);
	}
    } else
	finalUnit = strdup(sUnit);

    // handling the Arc Unit
    if (!strcmp(sUnit, "Deg") || !strcmp(sUnit, "deg")
	|| !strcmp(sUnit, "Grad")) {
	snprintf(sValue, WSXM_MAXCHARS, "%g %c", dValue, 176);
	free(finalUnit);
	return;
    }
    // check if a global unit alias exists
    for (; UnitPtr->alias; UnitPtr++)
	if (!strcmp(finalUnit, UnitPtr->alias))
	    break;
    free(finalUnit);
    if (!UnitPtr->alias) {	// Default: digital units
	snprintf(sValue, WSXM_MAXCHARS, "%g %s", dValue, sUnit);
	OUTPUT
	    ("nc2top:\tdata will be saved with unit that is given by a custom string\n");
    } else {
	// substituting A with 197
	if (!strcmp(UnitPtr->alias, "AA")
	    || !strcmp(UnitPtr->alias, "Ang")) {
	    // angstrom need an extra handling
	    snprintf(prec_str, WSXM_MAXCHARS, "%%%s %%c", UnitPtr->prec2);
	    //      printf("%s\n",prec_str);
	    snprintf(sValue, WSXM_MAXCHARS, prec_str,
		     dValue / UnitPtr->fac, 197);
	    return;
	    // um needs extra handling
	} else if (!strcmp(UnitPtr->alias, "um")) {
	    snprintf(prec_str, WSXM_MAXCHARS, "%%%s %%c", UnitPtr->prec2);
	    snprintf(sValue, WSXM_MAXCHARS, prec_str,
		     dValue / UnitPtr->fac, 181);
	    strncat(sValue, "m", 1);
	    return;
	} else {
	    snprintf(prec_str, WSXM_MAXCHARS, "%%%s %%s", UnitPtr->prec2);
	    snprintf(sValue, WSXM_MAXCHARS, prec_str,
		     dValue / UnitPtr->fac, UnitPtr->s);
	}
    }
}

/*----------------------------------------------------------------------
	ReplaceChar: Replaces A Char Within A String
	Default: Replacing ',' by '.'
----------------------------------------------------------------------*/
void ReplaceChar(char *string, char oldpiece, char newpiece)
{
    for (; *string; string++) {
	if (*string == oldpiece)
	    *string = newpiece;
    }
}

/*----------------------------------------------------------------------
	SeparateLine: Separates Lines vom sInput to sOutput
	Default Separator is '\n'
----------------------------------------------------------------------*/
void SeparateLine(char *sInput, char *sOutput, char cSeparator)
{
    for (; *sInput; sInput++) {
	if (*sInput == cSeparator) {
	    break;
	} else {
	    strncat(sOutput, sInput, 1);
	}
    }
}

/*----------------------------------------------------------------------
	ClearString: Fills string with  \0
----------------------------------------------------------------------*/
void ClearString(char *sInput)
{
    for (int i = 0; i < WSXM_MAXCHARS; i++) {
	sInput[i] = 0;
    }
}

/*----------------------------------------------------------------------
	WriteData CITS
----------------------------------------------------------------------*/
int WriteData_cits(char *sOutput, NcVar * Data, float *ramp, double Vbias)
{


// Open file for binary output
    FILE *pDestinationFile = NULL;
    pDestinationFile = fopen(sOutput, "ab");
    if (pDestinationFile == NULL) {
	OUTPUT
	    ("nc2top:\tnot able to open destination file for writing binary data\n");
	return 1;
    }
    OUTPUT("nc2top:\topened destination file for writing binary data\n");

    guint time = (guint) Data->get_dim(0)->size();
    guint value = (guint) Data->get_dim(1)->size();
    guint rows = (guint) Data->get_dim(2)->size();
    guint columns = (guint) Data->get_dim(3)->size();

//We need to find the Topography Picture first!
    guint topopos = 0;
    double curdelta = fabs(Vbias - ramp[0]);
    for (guint i = 0; i < value; i++) {
	if (curdelta > fabs(Vbias - ramp[i])) {
	    curdelta = fabs(Vbias - ramp[i]);
	    topopos = i;
	}
    }
    OUTPUT
	("nc2top:\tTopography Image %i with Ramp Value: %fV Topobias: %fV\n",
	 topopos, ramp[topopos], Vbias);


    NcType type;
    type = Data->type();
    if (type == ncShort) {
	// not tested yet:      
	short int databuf[time][1][rows][columns];
	//Write Topography Image to file
	Data->set_cur(0, topopos, 0, 0);
	Data->get(&databuf[0][0][0][0], (long int) time, (long int) 1,
		  (long int) rows, (long int) columns);
	for (unsigned int y = 0; y < rows; y++) {
	    for (unsigned int x = 0; x < columns; x++) {
		// writing data needs a point mirroring at the center of the image
		short int *shortbuf = NULL;
		shortbuf = new short int;
		*shortbuf = databuf[0][0][rows - 1 - y][columns - 1 - x];
		fwrite(shortbuf, sizeof(short int), 1, pDestinationFile);
		delete shortbuf;
	    }
	}

	//write spectropscopy Images to file
	for (unsigned int i = 0; i < value; i++) {
	    Data->set_cur(0, i, 0, 0);
	    Data->get(&databuf[0][0][0][0], (long int) time, (long int) 1,
		      (long int) rows, (long int) columns);
	    for (unsigned int y = 0; y < rows; y++) {
		for (unsigned int x = 0; x < columns; x++) {
		    // writing data needs a point mirroring at the center of the image
		    short int *shortbuf = NULL;
		    shortbuf = new short int;
		    *shortbuf =
			databuf[0][0][rows - 1 - y][columns - 1 - x];
		    fwrite(shortbuf, sizeof(short int), 1,
			   pDestinationFile);
		    delete shortbuf;
		}
	    }
	}

    } else if (type == ncFloat) {
	float databuf[time][1][rows][columns];
	//Write Topography Image to file

	Data->set_cur(0, topopos, 0, 0);
	Data->get(&databuf[0][0][0][0], (long int) time, (long int) 1,
		  (long int) rows, (long int) columns);
	for (unsigned int y = 0; y < rows; y++) {
	    for (unsigned int x = 0; x < columns; x++) {
		// writing data needs a point mirroring at the center of the image
		double *doublebuf = NULL;
		doublebuf = new double;
		*doublebuf =
		    1.0 * (double) databuf[0][0][rows - 1 - y][columns -
							       1 - x];
		fwrite(doublebuf, sizeof(double), 1, pDestinationFile);
		delete doublebuf;
	    }
	}

	//write spectropscopy Images to file. Load one image after another. Loading all images fails because databuf becomes to big. 
	for (unsigned int i = 0; i < value; i++) {
	    Data->set_cur(0, i, 0, 0);
	    Data->get(&databuf[0][0][0][0], (long int) time, (long int) 1,
		      (long int) rows, (long int) columns);
	    for (unsigned int y = 0; y < rows; y++) {
		for (unsigned int x = 0; x < columns; x++) {
		    // writing data needs a point mirroring at the center of the image
		    double *doublebuf = NULL;
		    doublebuf = new double;
		    *doublebuf =
			1.0 * (double) databuf[0][0][rows - 1 -
						     y][columns - 1 - x];
		    fwrite(doublebuf, sizeof(double), 1, pDestinationFile);
		    delete doublebuf;
		}
	    }
	}

    } else if (type == ncDouble) {
	// not tested yet
	double databuf[time][value][rows][columns];
	//Write Topography Image to file

	Data->set_cur(0, topopos, 0, 0);
	Data->get(&databuf[0][0][0][0], (long int) time, (long int) 1,
		  (long int) rows, (long int) columns);
	for (unsigned int y = 0; y < rows; y++) {
	    for (unsigned int x = 0; x < columns; x++) {
		// writing data needs a point mirroring at the center of the image
		double *doublebuf = NULL;
		doublebuf = new double;
		*doublebuf =
		    1.0 * (double) databuf[0][0][rows - 1 - y][columns -
							       1 - x];
		fwrite(doublebuf, sizeof(double), 1, pDestinationFile);
		delete doublebuf;
	    }
	}

	//write spectropscopy Images to file. Load one image after another. Loading all images fails because databuf becomes to big. 
	for (unsigned int i = 0; i < value; i++) {
	    Data->set_cur(0, i, 0, 0);
	    Data->get(&databuf[0][0][0][0], (long int) time, (long int) 1,
		      (long int) rows, (long int) columns);
	    for (unsigned int y = 0; y < rows; y++) {
		for (unsigned int x = 0; x < columns; x++) {
		    // writing data needs a point mirroring at the center of the image
		    double *doublebuf = NULL;
		    doublebuf = new double;
		    *doublebuf =
			1.0 * (double) databuf[0][0][rows - 1 -
						     y][columns - 1 - x];
		    fwrite(doublebuf, sizeof(double), 1, pDestinationFile);
		    delete doublebuf;
		}
	    }
	}
    }




    fclose(pDestinationFile);
//OUTPUT ("nc2top:\tData succesfully written\n");
    printf("nc2top:\tData succesfully written\n");
    return 0;
}

/*----------------------------------------------------------------------
	WriteData TOPO
----------------------------------------------------------------------*/
int WriteData_topo(char *sOutput, NcVar * Data)
{
// Open file for binary output
    FILE *pDestinationFile = NULL;
    pDestinationFile = fopen(sOutput, "ab");
    if (pDestinationFile == NULL) {
	OUTPUT
	    ("nc2top:\tnot able to open destination file for writing binary data\n");
	return 1;
    }
    OUTPUT("nc2top:\topened destination file for writing binary data\n");

    guint time = (guint) Data->get_dim(0)->size();
    guint value = (guint) Data->get_dim(1)->size();
    guint rows = (guint) Data->get_dim(2)->size();
    guint columns = (guint) Data->get_dim(3)->size();


    NcType type;
    type = Data->type();

    if (type == ncShort) {
	short int databuf[time][value][rows][columns];
	Data->get(&databuf[0][0][0][0], (long int) time, (long int) value,
		  (long int) rows, (long int) columns);
	for (unsigned int y = 0; y < rows; y++) {
	    for (unsigned int x = 0; x < columns; x++) {
		// writing data needs a point mirroring at the center of the image
		short int *shortbuf = NULL;
		shortbuf = new short int;
		*shortbuf = databuf[0][0][rows - 1 - y][columns - 1 - x];
		fwrite(shortbuf, sizeof(short int), 1, pDestinationFile);
		delete shortbuf;
	    }
	}

    } else if (type == ncLong) {
	long int databuf[time][value][rows][columns];
	Data->get(&databuf[0][0][0][0], (long int) time, (long int) value,
		  (long int) rows, (long int) columns);
	for (unsigned int y = 0; y < rows; y++) {
	    for (unsigned int x = 0; x < columns; x++) {
		// writing data needs a point mirroring at the center of the image
		double *doublebuf = NULL;
		doublebuf = new double;
		*doublebuf = (double) databuf[0][0][rows - 1 - y][columns - 1 - x];
		fwrite(doublebuf, sizeof(double), 1, pDestinationFile);
		delete doublebuf;
	    }
	}

    } else if (type == ncFloat) {
	float databuf[time][value][rows][columns];
	Data->get(&databuf[0][0][0][0], (long int) time, (long int) value,
		  (long int) rows, (long int) columns);
	for (unsigned int y = 0; y < rows; y++) {
	    for (unsigned int x = 0; x < columns; x++) {
		// writing data needs a point mirroring at the center of the image
		double *doublebuf = NULL;
		doublebuf = new double;
		// looks like the conversion float-> double needs some adjustment
		*doublebuf =
		    10 * pow(2,-15) * (double) databuf[0][0][rows - 1 - y][columns - 1 - x];
		fwrite(doublebuf, sizeof(double), 1, pDestinationFile);
		delete doublebuf;
	    }
	}

    } else if (type == ncDouble) {
	double databuf[time][value][rows][columns];
	Data->get(&databuf[0][0][0][0], (long int) time, (long int) value,
		  (long int) rows, (long int) columns);
	for (unsigned int y = 0; y < rows; y++) {
	    for (unsigned int x = 0; x < columns; x++) {
		// writing data needs a point mirroring at the center of the image
		double *doublebuf = NULL;
		doublebuf = new double;
		*doublebuf =
		    1.0 * databuf[0][0][rows - 1 - y][columns - 1 - x];
		fwrite(doublebuf, sizeof(double), 1, pDestinationFile);
		delete doublebuf;
	    }
	}
    } 

    fclose(pDestinationFile);
    printf("nc2top:\tData succesfully written\n");
    return 0;
}

/*----------------------------------------------------------------------
	WriteData MOVIE
----------------------------------------------------------------------*/
int WriteData_movie(char *sOutput, NcVar * Data, double dz)
{
// Open file for binary output
    FILE *pDestinationFile = NULL;
    pDestinationFile = fopen(sOutput, "ab");
    if (pDestinationFile == NULL) {
	OUTPUT
	    ("nc2top:\tnot able to open destination file for writing binary data\n");
	return 1;
    }
    OUTPUT("nc2top:\topened destination file for writing binary data\n");

    guint time = (guint) Data->get_dim(0)->size();
    guint value = (guint) Data->get_dim(1)->size();
    guint rows = (guint) Data->get_dim(2)->size();
    guint columns = (guint) Data->get_dim(3)->size();


    NcType type;
    type = Data->type();

    if (type == ncShort) {

	short int databuf[1][value][rows][columns];
	for (guint t = 0; t < time; t++) {
	    Data->set_cur(t, 0, 0, 0);
	    Data->get(&databuf[0][0][0][0], 1, (long int) value,
		      (long int) rows, (long int) columns);
	    for (unsigned int y = 0; y < rows; y++) {
		for (unsigned int x = 0; x < columns; x++) {
		    // writing data needs a point mirroring at the center of the image
		    // we need to save data as double if we want to convert movies
		    double *shortbuf = NULL;
		    shortbuf = new double;
		    *shortbuf =
			dz * (double) databuf[0][0][rows - 1 - y][columns -
								  1 - x];
		    fwrite(shortbuf, sizeof(double), 1, pDestinationFile);
		    delete shortbuf;
		}
	    }
	}


    } else if (type == ncFloat) {
	float databuf[1][value][rows][columns];
	for (guint t = 0; t < time; t++) {
	    Data->set_cur(t, 0, 0, 0);
	    Data->get(&databuf[0][0][0][0], 1, (long int) value,
		      (long int) rows, (long int) columns);
	    for (unsigned int y = 0; y < rows; y++) {
		for (unsigned int x = 0; x < columns; x++) {
		    // writing data needs a point mirroring at the center of the image
		    double *doublebuf = NULL;
		    doublebuf = new double;
		    *doublebuf =
			1.0 * (double) databuf[0][0][rows - 1 -
						     y][columns - 1 - x];
		    fwrite(doublebuf, sizeof(double), 1, pDestinationFile);
		    delete doublebuf;
		}
	    }

	}
    } else if (type == ncDouble) {
	double databuf[1][value][rows][columns];
	for (guint t = 0; t < time; t++) {
	    Data->set_cur(t, 0, 0, 0);
	    Data->get(&databuf[0][0][0][0], 1, (long int) value,
		      (long int) rows, (long int) columns);
	    for (unsigned int y = 0; y < rows; y++) {
		for (unsigned int x = 0; x < columns; x++) {
		    // writing data needs a point mirroring at the center of the image
		    double *doublebuf = NULL;
		    doublebuf = new double;
		    *doublebuf =
			1.0 * databuf[0][0][rows - 1 - y][columns - 1 - x];
		    fwrite(doublebuf, sizeof(double), 1, pDestinationFile);
		    delete doublebuf;
		}
	    }
	}
    }



    fclose(pDestinationFile);
    printf("nc2top:\tData succesfully written\n");


    return 0;
}

/*----------------------------------------------------------------------
	int to Char: Converts int to Char
----------------------------------------------------------------------*/
void itoc(int x, char *sChar)
{
    sChar[0] = (char) (x / 100) + 48;
    sChar[1] = (char) (x - x / 100 * 100) / 10 + 48;
    sChar[2] =
	(char) (x - x / 100 * 100 - (x - x / 100 * 100) / 10 * 10) + 48;
    sChar[3] = '\0';
}


/*----------------------------------------------------------------------
	GetUnit: returns true if unit was found
----------------------------------------------------------------------*/
bool GetUnit(char *unit, NcVar * variable)
{
    UnitsTable *UnitPtr = XsmUnitsTable;
    if (variable->get_att("var_unit") == NULL) {
	if (variable->get_att("unit") == NULL) {
	    // no unit found!
	    return false;
	} else {
	    //Found var_unit 
	    snprintf(unit, WSXM_MAXCHARS,"%s",
		     variable->get_att("unit")->values()->as_string(0));
	}

    } else {
	//Found unit  
	snprintf(unit, WSXM_MAXCHARS,"%s",
		 variable->get_att("var_unit")->values()->as_string(0));
    }

//Check if unit is okay
    if (!strcmp(unit, "Ang"))
	snprintf(unit, WSXM_MAXCHARS, "AA");
    if (!strcmp(unit, "Å"))
	snprintf(unit, WSXM_MAXCHARS, "AA");
    if (!strcmp(unit, "Volt"))
	snprintf(unit, WSXM_MAXCHARS, "V");

    for (; UnitPtr->alias; UnitPtr++) {
	if (UnitPtr->alias == NULL)
	    return false;	//Unit was not okay
	if (!strcmp(unit, UnitPtr->alias))
	    return true;	//Unit was okay
    }
    return true;
}

/*----------------------------------------------------------------------
    Killspace: Kills spaces and special symbols at the end of a string
----------------------------------------------------------------------*/
void KillSpace(char *sInput)
{
    int length;
    length = strlen(sInput);
    int i;
    i = length - 1;
    while (int (sInput[i] <= 33) && int (sInput[i] > 0))	//int(Å)=-95 
    {
	sInput[i] = '\0';
	i--;
    }
}
