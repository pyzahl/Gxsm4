/***********************************************************************
*
*	File Name: Header.h
*
*	Description: Header file for Header.c
*
***********************************************************************/

#include <cstdio>
#include <cstring>
#include <math.h>


// The max. number of chars in a value has to be defined.
// This should already be done in 'WSxM_io.C'
#ifndef WSXM_MAXCHARS
#define WSXM_MAXCHARS 1000
#endif

#define IMAGE_HEADER_VERSION "1.0 (April 2000)"
#define IMAGE_HEADER_SIZE_TEXT "Image header size: "
#define IMAGE_HEADER_END_TEXT "Header end"
#define TEXT_COPYRIGHT_NANOTEC "WSxM file copyright Nanotec Electronica\r\n"

// File IDs, soon obsolent by using FileIdStruc
#define STM_IMAGE_FILE_ID  "SxM Image file\r\n"
#define CITS_IMAGE_FILE_ID "CITS Image file\r\n"

#define IMAGE_HEADER_GENERAL_INFO "General Info"
#define IMAGE_HEADER_GENERAL_INFO_NUM_COLUMNS "Number of columns"
#define IMAGE_HEADER_GENERAL_INFO_NUM_ROWS "Number of rows"
#define IMAGE_HEADER_GENERAL_INFO_Z_AMPLITUDE "Z Amplitude"
#define IMAGE_HEADER_GENERAL_INFO_ACQ_CHANNEL "Acquisition channel"
#define IMAGE_HEADER_GENERAL_INFO_HEAD_TYPE "Head type"
#define IMAGE_HEADER_GENERAL_INFO_DATA_TYPE "Image Data Type"
// CITS additions
#define IMAGE_HEADER_GENERAL_INFO_POINTS_PER_IV "Number of points per ramp"
#define IMAGE_HEADER_GENERAL_INFO_I_AMPLITUDE "Current Amplitude"
// addtions to save curves
#define IMAGE_HEADER_GENERAL_INFO_LINES "Number of lines"
#define IMAGE_HEADER_GENERAL_INFO_POINTS "Number of points"
#define IMAGE_HEADER_GENERAL_INFO_X_TEXT "X axis text"
#define IMAGE_HEADER_GENERAL_INFO_X_UNIT "X axis unit"
#define IMAGE_HEADER_GENERAL_INFO_Y_TEXT "Y axis text"
#define IMAGE_HEADER_GENERAL_INFO_Y_UNIT "Y axis unit"

#define IMAGE_HEADER_MISC_INFO "Miscellaneous"
#define IMAGE_HEADER_MISC_INFO_MAXIMUM "Maximum"
#define IMAGE_HEADER_MISC_INFO_MINIMUM "Minimum"
#define IMAGE_HEADER_MISC_INFO_MAXIMUM_DAC "Maximum DAC"
#define IMAGE_HEADER_MISC_INFO_MINIMUM_DAC	"Minimum DAC"
#define IMAGE_HEADER_MISC_INFO_COMMENTS "Comments"
#define IMAGE_HEADER_MISC_INFO_VERSION "Version"
#define IMAGE_HEADER_MISC_INFO_SAVEDWITH "Saved with version"
// additions to save curves
#define IMAGE_HEADER_MISC_INFO_FIRST "First Forward"

#define IMAGE_HEADER_CONTROL "Control"
#define IMAGE_HEADER_CONTROL_X_AMPLITUDE "X Amplitude"
#define IMAGE_HEADER_CONTROL_Y_AMPLITUDE "Y Amplitude"
#define IMAGE_HEADER_CONTROL_SET_POINT "Set point"
#define IMAGE_HEADER_CONTROL_Z_GAIN "Z Gain"
#define IMAGE_HEADER_CONTROL_BIAS "Topography Bias"
#define IMAGE_HEADER_CONTROL_X_FREQUENCY "X-Frequency"
#define IMAGE_HEADER_CONTROL_X_OFFSET "X Offset"
#define IMAGE_HEADER_CONTROL_Y_OFFSET "Y Offset"
#define IMAGE_HEADER_CONTROL_ROTATION "Rotation"
#define IMAGE_HEADER_CONTROL_SCAN_DIRECTION "Direction"

#define IMAGE_HEADER_HEADS "Head Settings"
#define IMAGE_HEADER_HEADS_X_CALIBRATION "X Calibration"
#define IMAGE_HEADER_HEADS_Y_CALIBRATION "Y Calibration"
#define IMAGE_HEADER_HEADS_Z_CALIBRATION "Z Calibration"
#define IMAGE_HEADER_HEADS_PREAMP_GAIN "Preamp Gain"

#define IMAGE_HEADER_CURRENTIMAGE "Current images bias list"
#define IMAGE_HEADER_CURRENTIMAGE_IMAGE "Image"
#define IMAGE_HEADER_MAXMINSLIST "maxmins list"

#define IMAGE_HEADER_RAMPVALUES "Spectroscopy images ramp value list"
#define IMAGE_HEADER_FRAMES "Frames Synchronization"
#define IMAGE_HEADER_GENERAL_INFO_NUM_FRAMES "Number of Frames"
#define IMAGE_HEADER_GENERAL_INFO_TOTALTIME "Total Time"
#define IMAGE_HEADER_GENERAL_INFO_PROCESSES  "Image processes"

/***********************************************************************
*
*	WSxM_HEADER structure
*
*	This is the structure we will use to represent a header of a WSxM
*	It will have three strings representing each value in the header
*
*	- The title will indicate the group of values this value is included
*	in the header
*
*	- The label will precisate what is the value for
*
*	- The value will be an ASCII representation of the value
*
*	In the structure we can find too the total number of fields in the
*	structure
*
***********************************************************************/

typedef struct typeHeader {
    char **tszTitles;
    char **tszLabels;
    char **tszValues;

    int iNumFields;
} WSxM_HEADER;

typedef struct {
    int index;			// index, used to set type
    const char *name;		// name
    const char *aliasname;		// aliasname
    const char *fileidtag;		// id-tag used within the header (2nd line) for file type identification
    const char *suffix;		// file suffix
    const char *option;		// option used to select file type 
} FileIdStruc;

extern FileIdStruc FileId[];

/* Initialization of the header */

void HeaderInit(WSxM_HEADER * pHeader);

/* Header file input/output */

/* Header read */

int HeaderRead(WSxM_HEADER * pHeader, FILE * pFile);

/* Header write */

int HeaderWrite(WSxM_HEADER * pHeader, FILE * pFile);
int HeaderWrite(WSxM_HEADER * pHeader, FILE * pFile, const char * gType);

/* Header copy */
int HeaderCopy(WSxM_HEADER * sHeader, WSxM_HEADER * dHeader);

/* Header access to one field */

/* Read */

double HeaderGetAsNumber(WSxM_HEADER * pHeader, const char *szTitle,
			 const char *szLabel);
int HeaderGetAsString(WSxM_HEADER * pHeader, const char *szTitle, const char *szLabel,
		      char *szValue);
void HeaderReadTitle(char *szLine, char *szTitle);
void HeaderReadLabel(char *szLine, char *szLabel);
void HeaderReadValue(char *szLine, char *szValue);

/* Write */

void HeaderSetAsFloating(WSxM_HEADER * pHeader, const char *szTitle,
			 const char *szLabel, double lfValue);
void HeaderSetAsInt(WSxM_HEADER * pHeader, const char *szTitle, const char *szLabel,
		    int iValue);
void HeaderSetAsString(WSxM_HEADER * pHeader, const char *szTitle, const char *szLabel,
		       char *szValue);

/* Header destroy */

void HeaderDestroy(WSxM_HEADER * pHeader);

/* Internally used functions */

int HeaderReadLine(WSxM_HEADER * pHeader, FILE * pFile);
int HeaderGetSize(WSxM_HEADER * pHeader);
int HeaderAddValue(WSxM_HEADER * pHeader, const char *szTitle, const char *szLabel,
		   char *szValue);

void RemoveLeftAndRightWhitesFromString(char *szString);
void ReplaceStringInString(char *szDest, const char *szOld,
			   const char *szNew);
