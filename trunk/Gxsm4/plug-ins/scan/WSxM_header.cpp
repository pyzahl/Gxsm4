/***********************************************************************
*
*	File Name: WSxM_header.C
*
*	Description: functions for managing the WSxM_HEADER structure defined in Header.h
*
***********************************************************************/

/* this is not a main PlugIn file and here is no Docu to scan...
% PlugInModuleIgnore
*/

#include <malloc.h>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <ctype.h>
#include "WSxM_header.h"

// Define the max. number of chars in a header argument if not done in WSxM_io.C

#ifndef WSXM_MAXCHARS
#define WSXM_MAXCHARS 1000
#endif


/***********************************************************************
*
*	FileIdStruc FileId[]
*
*	Description: Used for identification of the file type
*
***********************************************************************/

FileIdStruc FileId[] = {
    // name, aliasname, id-tag, suffix
    {0, "SPM Image", "Topography", "SxM Image file\r\n", ".top", "-TOPO"}
    ,
    {1, "SPM Movie", "Topography Movie", "Movie Image file\r\n", ".mpp", "-MOVIE"}
    ,
    {2, "CITS File", "Current Imaging Tunneling Spectroscopy", "Spectroscopy Imaging Image file\r\n", ".cit", "-CITS"}
    ,
    {3, "IV File", "IV Curve", "IV curve file\r\n", ".iv.cur", "-IV"}
    ,
    {4, "IZ File", "IZ Curve", "IZ curve file\r\n", ".iz.cur", "-IZ"}
    ,
    {5, "ZV File", "ZV Curve", "ZV curve file\r\n", ".zv.cur", "-ZV"}
    ,
    {6, "FZ File", "FZ Curve", "FZ curve file\r\n", ".fz.cur", "-FZ"}
    ,
    {7, "Generic Curves", "Generic Curve", "Generic curve file\r\n", ".curve", "-generic"}
    ,
    {8, NULL, NULL, NULL}
};

/***********************************************************************
*
*	Function void HeaderInit (WSxM_HEADER *pHeader);
*
*	Description: Initializes the values of the WSxM_HEADER structure. It should
*		     be called always before using any of the other functions
*
***********************************************************************/

void HeaderInit(WSxM_HEADER * pHeader)
{
    if (pHeader == NULL)
	return;

    pHeader->tszTitles = NULL;
    pHeader->tszLabels = NULL;
    pHeader->tszValues = NULL;

    pHeader->iNumFields = 0;
}

/***********************************************************************
*
*	Function int HeaderRead (WSxM_HEADER *pHeader, FILE *pFile);
*
*	Description: Reads a WSxM_HEADER from a file. It is important that the file
*		     should have been opened for reading, and the WSxM_HEADER should
*		     have been initializated calling HeaderInit
*
*	Inputs:
*			- pHeader: pointer to the WSxM_HEADER object to store the data in
*			- pFile: File pointer to read the header from
*
*	Outputs:
*			- pHeader: will be filled with the data read from pFile
*			- pFile: the file pointer will go to the end of the header
*
*	Return value:
*			0 if the header was correctly read, -1 elsewhere
*
***********************************************************************/

int HeaderRead(WSxM_HEADER * pHeader, FILE * pFile)
{
    int iStatus = 0;

    if ((pHeader == NULL) || (pFile == NULL))
	return -1;

    if (pHeader->iNumFields != 0)
	return -1;

    /* Read the file line by line */

    while (iStatus == 0) {
	iStatus = HeaderReadLine(pHeader, pFile);
    }

    if (iStatus == -1) {	/* Error */
	return -1;
    }

    return 0;
}

/* Header write */

/***********************************************************************
*
*	Function int HeaderWrite (WSxM_HEADER *pHeader, FILE *pFile);
*
*	Description: Writes the WSxM_HEADER object to a file. It is important that the file
*		     should have been opened for writing (not append).
*
*	Inputs:
*			- pHeader: pointer to the WSxM_HEADER object with the data
*			- pFile: File pointer to write the data header
*
*
*	Return value:
*			0 if the header was correctly write, -1 elsewhere
*
***********************************************************************/

int HeaderWrite(WSxM_HEADER * pHeader, FILE * pFile)
{
    int i, j;
    char *pCharAux;		/* for swapping in bubble sort */
    char szAuxTitle[100], szAuxLabel[100], szAuxValue[WSXM_MAXCHARS];	/* for writting in file        */



    /* sorts the WSxM_HEADER object by title and label using bubble sort algorithm */

    for (i = 0; i < pHeader->iNumFields; i++) {
	for (j = 0; j < (pHeader->iNumFields - i - 1); j++) {
	    if ((strcmp(pHeader->tszTitles[j], pHeader->tszTitles[j + 1]) >
		 0)
		||
		( (strcmp(pHeader->tszTitles[j], pHeader->tszTitles[j + 1])
		   == 0)
		  &&
		  (strcmp(pHeader->tszLabels[j], pHeader->tszLabels[j + 1]) >
		   0))
		){
		/* the element j is bigger than j+1 one so we swap them    */
		/* we must swap the elements j and j+1 in the three arrays */

		/* swaps the titles */
		pCharAux = pHeader->tszTitles[j];
		pHeader->tszTitles[j] = pHeader->tszTitles[j + 1];
		pHeader->tszTitles[j + 1] = pCharAux;

		/* swaps the labels */
		pCharAux = pHeader->tszLabels[j];
		pHeader->tszLabels[j] = pHeader->tszLabels[j + 1];
		pHeader->tszLabels[j + 1] = pCharAux;

		/* swaps the values */
		pCharAux = pHeader->tszValues[j];
		pHeader->tszValues[j] = pHeader->tszValues[j + 1];
		pHeader->tszValues[j + 1] = pCharAux;
	    }
	}
    }

    /* writes the pre-header info in the file */
    fprintf(pFile, "%s", TEXT_COPYRIGHT_NANOTEC);
    fprintf(pFile, "%s", STM_IMAGE_FILE_ID);
    fprintf(pFile, "%s%d\r\n", IMAGE_HEADER_SIZE_TEXT,
	    HeaderGetSize(pHeader)+1);

    /* In the first time there is no previous title */
    strcpy(szAuxTitle, "");

    /* writes all the fields in the file */
    for (i = 0; i < pHeader->iNumFields; i++) {
	/* if the title is diferent than the previous one we write the title in the file */
	if (strcmp(szAuxTitle, pHeader->tszTitles[i]) != 0) {
	    /* Special characteres in the title */
	    strcpy(szAuxTitle, pHeader->tszTitles[i]);
	    ReplaceStringInString(szAuxTitle, "\\", "\\\\");
	    ReplaceStringInString(szAuxTitle, "\r\n", "\\n");

	    /* writes the title in the file */
	    fprintf(pFile, "\r\n[%s]\r\n\r\n", szAuxTitle);
	}

	/* Special characteres in the label */
	strcpy(szAuxLabel, pHeader->tszLabels[i]);
	ReplaceStringInString(szAuxLabel, "\\", "\\\\");
	ReplaceStringInString(szAuxLabel, "\r\n", "\\n");

	/* Special characteres in the value */
	strcpy(szAuxValue, pHeader->tszValues[i]);
	ReplaceStringInString(szAuxValue, "\\", "\\\\");
	ReplaceStringInString(szAuxValue, "\r\n", "\\n");
	RemoveLeftAndRightWhitesFromString(szAuxValue);

	/* writes the label and the value in the file */
	fprintf(pFile, "    %s: %s\r\n", szAuxLabel, szAuxValue);
    }

    /* writes the header end */
    fprintf(pFile, "\r\n[%s]\r\n", IMAGE_HEADER_END_TEXT);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
int HeaderWrite(WSxM_HEADER * pHeader, FILE * pFile, const char * cType)
{
    int i, j;
    char *pCharAux;		/* for swapping in bubble sort */
    char szAuxTitle[100], szAuxLabel[100], szAuxValue[WSXM_MAXCHARS];	/* for writting in file        */
    FileIdStruc *FileIdPtr = FileId;
    /* sorts the WSxM_HEADER object by title and label using bubble sort algorithm */

    for (i = 0; i < pHeader->iNumFields; i++) {
	for (j = 0; j < (pHeader->iNumFields - i - 1); j++) {
	    if ((strcmp(pHeader->tszTitles[j], pHeader->tszTitles[j + 1]) >
		 0)
		||
		((strcmp(pHeader->tszTitles[j], pHeader->tszTitles[j + 1])
		  == 0)
		 &&
		 (strcmp(pHeader->tszLabels[j], pHeader->tszLabels[j + 1]) >
		  0))
		){
		/* the element j is bigger than j+1 one so we swap them    */
		/* we must swap the elements j and j+1 in the three arrays */

		/* swaps the titles */
		pCharAux = pHeader->tszTitles[j];
		pHeader->tszTitles[j] = pHeader->tszTitles[j + 1];
		pHeader->tszTitles[j + 1] = pCharAux;

		/* swaps the labels */
		pCharAux = pHeader->tszLabels[j];
		pHeader->tszLabels[j] = pHeader->tszLabels[j + 1];
		pHeader->tszLabels[j + 1] = pCharAux;

		/* swaps the values */
		pCharAux = pHeader->tszValues[j];
		pHeader->tszValues[j] = pHeader->tszValues[j + 1];
		pHeader->tszValues[j + 1] = pCharAux;
	    }
	}
    }

    /* writes the pre-header info in the file */
    fprintf(pFile, "%s", TEXT_COPYRIGHT_NANOTEC);
    for (; FileIdPtr->name != 0; ++FileIdPtr) {
	if (strstr(FileIdPtr->name, cType)) {
	    fprintf(pFile, "%s", FileIdPtr->fileidtag);
	    fprintf(pFile, "%s%lud\r\n", IMAGE_HEADER_SIZE_TEXT,
		    HeaderGetSize(pHeader)+strlen(FileIdPtr->fileidtag)-sizeof(STM_IMAGE_FILE_ID)+2);
		// +2 because \r\n 
	}
    }

    /* In the first time there is no previous title */
    strcpy(szAuxTitle, "");

    /* writes all the fields in the file */
    for (i = 0; i < pHeader->iNumFields; i++) {
	/* if the title is diferent than the previous one we write the title in the file */
	if (strcmp(szAuxTitle, pHeader->tszTitles[i]) != 0) {
	    /* Special characteres in the title */
	    strcpy(szAuxTitle, pHeader->tszTitles[i]);
	    ReplaceStringInString(szAuxTitle, "\\", "\\\\");
	    ReplaceStringInString(szAuxTitle, "\r\n", "\\n");

	    /* writes the title in the file */
	    fprintf(pFile, "\r\n[%s]\r\n\r\n", szAuxTitle);
	}

	/* Special characteres in the label */
	strcpy(szAuxLabel, pHeader->tszLabels[i]);
	ReplaceStringInString(szAuxLabel, "\\", "\\\\");
	ReplaceStringInString(szAuxLabel, "\r\n", "\\n");

	/* Special characteres in the value */
	strcpy(szAuxValue, pHeader->tszValues[i]);
	ReplaceStringInString(szAuxValue, "\\", "\\\\");
	ReplaceStringInString(szAuxValue, "\r\n", "\\n");
	RemoveLeftAndRightWhitesFromString(szAuxValue);

	/* writes the label and the value in the file */
	fprintf(pFile, "    %s: %s\r\n", szAuxLabel, szAuxValue);
    }

    /* writes the header end */
    fprintf(pFile, "\r\n[%s]\r\n", IMAGE_HEADER_END_TEXT);

    return 0;
}
/* Header copy */

/***********************************************************************
*
*	Function int HeaderCopy (WSxM_HEADER * sHeader, WSxM_HEADER * dHeader);
*
*	Description: Copies on header to another and changes the data type
*		     argument to double, and the WSxM_HEADER should
*		     have been initializated calling HeaderInit
*
*	Inputs:
*			- sHeader: pointer to the WSxM_HEADER used as source
*			- dHeader: pointer to the WSxM_HEADER used as destination
*
*	Outputs:
*			- dHeader: will be filled with the data read from source header
*
*	Return value:
*			0 if the header was correctly copied, -1 elsewhere
*
***********************************************************************/

int HeaderCopy(WSxM_HEADER * sHeader, WSxM_HEADER * dHeader)
{
    if ((sHeader == NULL) || (dHeader == NULL)) return -1;

    if (sHeader->iNumFields == 0) return -1;

    for (int i = 0; i < sHeader->iNumFields; i++) {
	    if (HeaderAddValue(dHeader, sHeader->tszTitles[i], sHeader->tszLabels[i], sHeader->tszValues[i])==-1) return -1;
    }
    HeaderSetAsString(dHeader, IMAGE_HEADER_GENERAL_INFO, IMAGE_HEADER_GENERAL_INFO_DATA_TYPE, strndup("double", WSXM_MAXCHARS));
    return 0;
}

/* Header access to one field */

/* Read */

/***********************************************************************
*
*	Function double HeaderGetAsNumber (WSxM_HEADER *pHeader, const char *szTitle, const char *szLabel);
*
*	Description: gets the title-label couple value as a number from the 
*               WSxM_HEADER object, the WSxM_HEADER should have been initializated
*               calling HeaderInit
*
*	Inputs: 
*               - pHeader: pointer to the WSxM_HEADER object
*		- szTitle: title of the field to be accesed
*               - szLabel: label of the field to be accesed
*
*	Return value:
*			a double with the title-label couple value, 0 on error
*
***********************************************************************/

double HeaderGetAsNumber(WSxM_HEADER * pHeader, const char *szTitle,
			 const char *szLabel)
{
    char szValue[WSXM_MAXCHARS] = "";	// the default size of 100 should be okey, but to be consistent ..
    if (HeaderGetAsString(pHeader, szTitle, szLabel, szValue) != 0) {	/* error */
	return 0;
    }
    if (getenv("LANG")){
       if(strstr(getenv("LANG"),"de")){ReplaceStringInString(szValue, ".", ",");}
    }
    else {
         ReplaceStringInString(szValue, ".", ",");
    }
    return atof(szValue);
}

/***********************************************************************
*
*	Function int HeaderGetAsString (WSxM_HEADER *pHeader, const const char *szTitle, const char *szLabel, char *szValue);
*
*	Description: gets the title-label couple value as a string from the 
*               WSxM_HEADER object
*
*	Inputs:
*		- pHeader: pointer to the WSxM_HEADER object
*		- szTitle: title of the field to be accesed
*               - szLabel: label of the field to be accesed
*
*	Outputs:
*		- szValue: will be filled with the title-label couple value as a string
*                 (this must be allocated)
*
*	Return value:
*		0 if the value was correctly accessed, -1 elsewhere
*
***********************************************************************/

int HeaderGetAsString(WSxM_HEADER * pHeader, const char *szTitle, const char *szLabel,
		      char *szValue)
{
    int iIndex;

    /* seeks the title-label couple */
    for (iIndex = 0; iIndex < pHeader->iNumFields; iIndex++) {
	if ((strcmp(szTitle, pHeader->tszTitles[iIndex]) == 0)
	    && (strcmp(szLabel, pHeader->tszLabels[iIndex]) == 0)) {
	    /* title-label couple found at position iIndex */
	    break;
	}
    }

    if (iIndex == pHeader->iNumFields) {
	/* the for loop didn't found the title-label couple in WSxM_HEADER object */
	return -1;
    }

    /* asign the value of the title-label couple to the pointer szValue */
    strcpy(szValue, pHeader->tszValues[iIndex]);

    /* access succesful */
    return 0;
}

/***********************************************************************
*
*	Function void HeaderReadTitle (char *szLine, char *szTitle);
*
*	Description: reads a title from a line
*
*	Inputs:
*               - szLine: line with the title
*
*       Outputs:
*		- szTitle: title readed from the line (this must be allocated),
*                 szTitle=="" if there is no title in the line.
*
*	Return value:
*		none
*
***********************************************************************/

void HeaderReadTitle(char *szLine, char *szTitle)
{
    int iIndex = 0;

    /* If the line don't begin by a left square bracket it isn't a title line */
    if (szLine[0] != '[') {
	strcpy(szTitle, "");
	return;
    }

    /* we seek the right square bracket */
    while (szLine[iIndex] != ']') {
	iIndex++;
    }

    /* at this point iIndex is the position of the right square bracket */

    /* szLine[1] to skip the left square bracket */
    strncpy(szTitle, &szLine[1], iIndex - 1);
    szTitle[iIndex - 1] = '\0';
}

/***********************************************************************
*
*	Function void HeaderReadLabel (szLine, szLabel);
*
*	Description: reads a label from a line
*
*	Inputs:
*               - szLine: line with the label
*
*       Outputs:
*		- szLabel: label readed from the line (this must be allocated),
*                 szValue=="" if there is no label in the line.
*
*	Return value:
*		none
*
***********************************************************************/

void HeaderReadLabel(char *szLine, char *szLabel)
{
    unsigned int iIndex = 0;

    /* we seek the ':' character */
    while (szLine[iIndex] != ':') {
	if (iIndex == strlen(szLine)) {
	    /* there is no label in this line */
	    strcpy(szLabel, "");
	    return;
	}
	iIndex++;
    }

    /* at this point iIndex is the position of the ':' character */

    /* we copy the label from the line */
    strncpy(szLabel, szLine, iIndex);
    szLabel[iIndex] = '\0';
}

/***********************************************************************
*
*	Function void HeaderReadValue (char *szLine, char *szValue);
*
*	Description: reads a value from a line
*
*	Inputs:
*               - szLine: line with the value
*
*       Outputs:
*		- szLabel: value readed from the line (this must be allocated),
*                 szValue=="" if there is no value in the line.
*
*	Return value:
*		none
*
***********************************************************************/

void HeaderReadValue(char *szLine, char *szValue)
{
    unsigned int iIndex = 0;

    /* we seek the ':' character and skip the blank after */
    while (szLine[iIndex] != ':') {
	if (iIndex == strlen(szLine)) {
	    /* there is no value in this line */
	    strcpy(szValue, "");
	    return;
	}
	iIndex++;
    }

    /* skips the ': ' */
    iIndex += 2;

    /* at this point iIndex is the position of the first value character */

    /* szLine[iIndex] to skip the line before ': ' and -1 not to copy return carriage */
    strncpy(szValue, &szLine[iIndex], strlen(szLine) - iIndex - 1);
    szValue[strlen(szLine) - iIndex - 1] = '\0';
}

/* Write */

/***********************************************************************
*
*	Function void HeaderSetAsFloating (WSxM_HEADER *pHeader, const char *szTitle, const char *szLabel, double lfValue);
*
*	Description: sets a title-label couple value in double format to the
*               WSxM_HEADER object
*
*	Inputs:
*		- pHeader: pointer to the WSxM_HEADER object to set the value in
*		- szTitle: title of the field to be set
*               - szLabel: label of the field to be set
*               - lfValue: value to be set
*
*	Return value:
*		none
*
***********************************************************************/

void HeaderSetAsFloating(WSxM_HEADER * pHeader, const char *szTitle,
			 const char *szLabel, double lfValue)
{
    char szValue[WSXM_MAXCHARS] = "";

    /* converts the double value to a string value */
    sprintf(szValue, "%.61f", lfValue);

    /* sets the value in the WSxM_HEADER object as a string */
    HeaderSetAsString(pHeader, szTitle, szLabel, szValue);
}

/***********************************************************************
*
*	Function void HeaderSetAsInt (WSxM_HEADER *pHeader, const char *szTitle, const char *szLabel, int iValue);
*
*	Description: sets a title-label couple value in int format to the
*               WSxM_HEADER object
*
*	Inputs:
*		- pHeader: pointer to the WSxM_HEADER object to set the value in
*		- szTitle: title of the field to be set
*               - szLabel: label of the field to be set
*               - iValue: value to be set
*
*	Return value:
*		none
*
***********************************************************************/

void HeaderSetAsInt(WSxM_HEADER * pHeader, const char *szTitle, const char *szLabel,
		    int iValue)
{
    char szValue[WSXM_MAXCHARS] = "";

    /* converts the int value to a string value */
    sprintf(szValue, "%d", iValue);

    /* sets the value in the WSxM_HEADER object as a string */
    HeaderSetAsString(pHeader, szTitle, szLabel, szValue);
}

/***********************************************************************
*
*	Function void HeaderSetAsInt (WSxM_HEADER *pHeader, const char *szTitle, const char *szLabel, int iValue);
*
*	Description: sets a title-label couple value in string format to the
*               WSxM_HEADER object
*
*	Inputs:
*			- pHeader: pointer to the WSxM_HEADER object to set the value in
*			- szTitle: title of the field to be set
*           - szLabel: label of the field to be set
*           - szValue: string value to be set
*
*	Return value:
*			none
*
***********************************************************************/

void HeaderSetAsString(WSxM_HEADER * pHeader, const char *szTitle, const char *szLabel,
		       char *szValue)
{
    int iIndex;

    /* sets the value in the WSxM_HEADER object */

    /* seeks the title-label couple */
    for (iIndex = 0; iIndex < pHeader->iNumFields; iIndex++) {
	if ((strcmp(szTitle, pHeader->tszTitles[iIndex]) == 0)
	    && (strcmp(szLabel, pHeader->tszLabels[iIndex]) == 0)) {
	    /* title-label couple found at position iIndex */
	    /* updates the title-label couple value */
	    pHeader->tszValues[iIndex] =
		(char *) realloc(pHeader->tszValues[iIndex],
				 (strlen(szValue) + 1) * sizeof(char));
	    strcpy(pHeader->tszValues[iIndex], szValue);

	    return;
	}
    }

    /* title-label couple not found, is a new field */
    HeaderAddValue(pHeader, szTitle, szLabel, szValue);
}

/***********************************************************************
*
*	Function void HeaderDestroy (WSxM_HEADER *pHeader);
*
*	Description: frees all the allocated memory in the Header
*
***********************************************************************/

void HeaderDestroy(WSxM_HEADER * pHeader)
{
    int i;			/* Counter */

    if (pHeader->iNumFields == 0) {	/* No need to destroy */
	return;
    }

    for (i = 0; i < pHeader->iNumFields; i++) {
	free(pHeader->tszTitles[i]);
	free(pHeader->tszLabels[i]);
	free(pHeader->tszValues[i]);
    }

    free(pHeader->tszTitles);
    free(pHeader->tszLabels);
    free(pHeader->tszValues);

    pHeader->tszTitles = NULL;
    pHeader->tszLabels = NULL;
    pHeader->tszValues = NULL;

    pHeader->iNumFields = 0;
}

/* Internally used functions */

/***********************************************************************
*
*	Function int HeaderReadLine (WSxM_HEADER *pHeader, FILE *pFile);
*
*	Description: Reads a text line from a file. It is important that the file
*		     should have been opened for reading, and the WSxM_HEADER should
*		     have been initializated calling HeaderInit
*
*	Inputs:
*	        - pHeader: pointer to the WSxM_HEADER object to store the data in
*		- pFile: File pointer to read the header from
*
*	Outputs:
*		- pHeader: will be filled with the data read from pFile
*		- pFile: the file pointer will go to the end of the header
*
*	Return value:
*		0 if the header was correctly read, -1 elsewhere
*
***********************************************************************/

int HeaderReadLine(WSxM_HEADER * pHeader, FILE * pFile)
{
    /* multiplication in the string sizes is because the string can grow later */

    static char szTitle[WSXM_MAXCHARS * 2] = "";
    char szLine[WSXM_MAXCHARS * 2];
    char szTryTitle[WSXM_MAXCHARS * 2];
    char szLabel[WSXM_MAXCHARS * 2];
    char szValue[WSXM_MAXCHARS * 2];
    char *szReturn;

    szReturn = fgets(szLine, WSXM_MAXCHARS, pFile);

    if (szReturn == NULL)
	return -1;		/* File Error */

    /* Line read. We see the data it has */

    RemoveLeftAndRightWhitesFromString(szLine);

    /* Special characteres */

    ReplaceStringInString(szLine, "\\n", "\r\n");
    ReplaceStringInString(szLine, "\\\\", "\\");

    /* We try to read a label-value couple */

    HeaderReadLabel(szLine, szLabel);
    HeaderReadValue(szLine, szValue);

    /* We try to read it as a title */

    HeaderReadTitle(szLine, szTryTitle);

    if (strlen(szTryTitle) > 0) {	/* It is a title */
	/* If it indicates the header end, we will return 1 */

	if (strcmp(szTryTitle, IMAGE_HEADER_END_TEXT) == 0)
	    return 1;
	else {
	    strcpy(szTitle, szTryTitle);
	    return 0;
	}
    }

    /* If it is not a header field (we have not read any title yet) we ignore it */

    if (strlen(szTitle) == 0) {
	return 0;
    }

    /* If it is not a couple label: value, we don't mind about this line */

    if (strlen(szLabel) != 0) {
	/* We have a new value. We add it to the header */

	HeaderAddValue(pHeader, szTitle, szLabel, szValue);
    }

    return 0;
}

/***********************************************************************
*
*	Function int HeaderGetSize (WSxM_HEADER *pHeader);
*
*	Description: Gets the size of the image header in the file.
*
*	Inputs:
*		- pHeader: pointer to the WSxM_HEADER object end of the header
*
*	Return value:
*		the size of the header, -1 on error
*
***********************************************************************/

int HeaderGetSize(WSxM_HEADER * pHeader)
{
    int i;
    int iSize = 0;		/* size at this moment                        */
    int iNumOfTitles = 0;	/* Number of titles written in the file       */
    int iNumDigits;		/* Number of digits for the image header size */
//    char szTitle[100] = "", szLabel[100], szValue[WSXM_MAXCHARS];
      char szTitle[WSXM_MAXCHARS] = "", szLabel[WSXM_MAXCHARS], szValue[WSXM_MAXCHARS];

    /* size of the pre-header and the carriage return */
//    iSize =sizeof(TEXT_COPYRIGHT_NANOTEC) + sizeof(STM_IMAGE_FILE_ID) +sizeof(IMAGE_HEADER_SIZE_TEXT);
      iSize =sizeof(TEXT_COPYRIGHT_NANOTEC) + sizeof(STM_IMAGE_FILE_ID) +sizeof(IMAGE_HEADER_SIZE_TEXT)-3; //3*\n

    /* size of all the fields in the HEADER object */
    for (i = 0; i < pHeader->iNumFields; i++) {
	/* if there is a new title we increase the number of titles and adds its size */
	if (strcmp(szTitle, pHeader->tszTitles[i]) != 0)  {
	    strcpy(szTitle, pHeader->tszTitles[i]);
	    iNumOfTitles++;
	    iSize += strlen(szTitle);
	}

	strcpy(szLabel, pHeader->tszLabels[i]);

	/* Special characteres in the szLabel string */
//	ReplaceStringInString(szLabel, "\\n", "\r\n");
	ReplaceStringInString(szLabel, "\\", "\\\\");
	ReplaceStringInString(szLabel, "\r\n", "\\n");

	strcpy(szValue, pHeader->tszValues[i]);

	/* Special characteres in the szValue string */
//	ReplaceStringInString(szValue, "\\n", "\r\n");
	ReplaceStringInString(szValue, "\\", "\\\\");
	ReplaceStringInString(szValue, "\r\n", "\\n");
	RemoveLeftAndRightWhitesFromString(szValue);

	/* adds the size of the label and the value */
	iSize += strlen(szLabel) + strlen(szValue);
//	printf("%i: %i: %s\n",strlen(szLabel),strlen(szValue),szValue);
    }

    /* adds the size of the square brackets (2 bytes), the carriage return (3*2 bytes  */
    /* for each title)                                                                 */
    iSize += iNumOfTitles * (6 + 2);
//    iSize += iNumOfTitles * (3 + 2);
//printf("tiles: %i fields: %i\n",iNumOfTitles, pHeader->iNumFields);

    /* adds the size of the carriage return (2 bytes for eReplaceStringInStringach label) and the blanks (6 bytes for each label) */
    iSize += pHeader->iNumFields * (6 + 2);
//    iSize += pHeader->iNumFields * (6 + 1); //4 blanks, 1*": ",1 return 

    /* adds the size of the header end text, the square brackets and the carriage return */
    iSize += sizeof(IMAGE_HEADER_END_TEXT) + 2 + 4;
//    iSize += sizeof(IMAGE_HEADER_END_TEXT) + 2 + 2;

    /* adds the size of the characteres for the image header size in pre-header */
    iNumDigits = (int) floor(log10(iSize)) + 1;
    iSize += iNumDigits;
    if (iNumDigits != (int) floor((log10(iSize)) + 1))
	iSize++;

    return iSize;
}

/***********************************************************************
*
*	Function int HeaderAddValue (WSxM_HEADER *pHeader, const char *szTitle, const char *szLabel,char *szValue);
*
*	Description: adds the tripla title-label-value to the WSxM_HEADER object
*
*	Inputs:
*		- pHeader: pointer to the WSxM_HEADER object to store the data in
*		- szTitle: title of the field to be added
*               - szLabel: label of the field to be added
*               - szValue: string value to be added
*
*	Return value:
*		0 if the header was correctly updated, -1 elsewhere
*
***********************************************************************/

int HeaderAddValue(WSxM_HEADER * pHeader, const char *szTitle, const char *szLabel,
		   char *szValue)
{
    /* grows the three arrays for the new title-label-value tripla */
    pHeader->tszTitles =
	(char **) realloc(pHeader->tszTitles,
			  (pHeader->iNumFields + 1) * sizeof(char *));
    pHeader->tszLabels =
	(char **) realloc(pHeader->tszLabels,
			  (pHeader->iNumFields + 1) * sizeof(char *));
    pHeader->tszValues =
	(char **) realloc(pHeader->tszValues,
			  (pHeader->iNumFields + 1) * sizeof(char *));

    /* adds the title in the last position of the titles array allocating memory */
    pHeader->tszTitles[pHeader->iNumFields] =
	(char *) calloc(strlen(szTitle) + 1, sizeof(char));
    if (pHeader->tszTitles[pHeader->iNumFields] == NULL) {	/* error */
	return -1;
    }
    strcpy(pHeader->tszTitles[pHeader->iNumFields], szTitle);

    /* adds the label in the last position of the labels array allocating memory */
    pHeader->tszLabels[pHeader->iNumFields] =
	(char *) calloc(strlen(szLabel) + 1, sizeof(char));
    if (pHeader->tszLabels[pHeader->iNumFields] == NULL) {	/* error */
	/* frees the memory allocated in this function */
	free(pHeader->tszTitles[pHeader->iNumFields]);

	return -1;
    }
    strcpy(pHeader->tszLabels[pHeader->iNumFields], szLabel);

    /* adds the value in the last position of the values array allocating memory */
    pHeader->tszValues[pHeader->iNumFields] =
	(char *) calloc(strlen(szValue) + 1, sizeof(char));
    if (pHeader->tszValues[pHeader->iNumFields] == NULL) {	/* error */
	/* frees the memory allocated in this function */
	free(pHeader->tszTitles[pHeader->iNumFields]);
	free(pHeader->tszLabels[pHeader->iNumFields]);

	return -1;
    }
    strcpy(pHeader->tszValues[pHeader->iNumFields], szValue);

    /* increments the number of fields */
    pHeader->iNumFields++;

    return 0;
}

/***********************************************************************
*
*	Function void RemoveLeftAndRightWhitesFromString (char *szString);
*
*	Description: removes the white-space characteres (0x09, 0x0D or 0x20)
*               at the right and left of the string szString.
*
*	Inputs:
*			- szString: the string we want to remove the white-space characteres
*
*   Outputs:
*           - szString: the string without white-space character
*
*	Return value:
*			none
*
***********************************************************************/

void RemoveLeftAndRightWhitesFromString(char *szString)
{
    char szAuxString[WSXM_MAXCHARS];
    int iIndex = 0;

    /* Seeks the first non white-space character (0x09, 0x0D or 0x20) */
    while (isspace(szString[iIndex])) {
	iIndex++;
    }

    /* We copy the string without white-space characters at the left */
    strcpy(szAuxString, &szString[iIndex]);

    /* Seeks the last non white-space character (0x09, 0x0D or 0x20) */
    iIndex = strlen(szAuxString);
    while (isspace(szString[iIndex])) {
	iIndex--;
    }

    /* Puts the zero (mark of end of string) after the last non white-space character */
    szString[iIndex + 1] = '\0';

    /* We copy the new szString without white-space characters */
    strcpy(szString, szAuxString);
}

/***********************************************************************
*
*	Function void ReplaceStringInString (char *szDest, const char *szOld, const char *szNew);
*
*	Description: replaces instances of the substring szOld with 
*               instances of the string szNew.
*
*	Inputs:
*		- szDest: the string where will be made the replacemets
*               - szOld: the substring to be replaced by lpszNew
*               - szNew: the substring replacing lpszOld
*
*      Outputs:
*               - szDest: the string with the replacements
*
*      Return value:
*               none
*
***********************************************************************/

void ReplaceStringInString(char *szDest, const char *szOld,
			   const char *szNew)
{
    int iIndex = 0, iAuxStringIndex = 0;
    char szAuxString[WSXM_MAXCHARS];
    strcpy(szAuxString, "");
    /* We copy the string with the replacements to the auxiliar string */

    /* Searches for the substring szOld */
    while (szDest[iIndex] != '\0') {	/* while not end of string */
	if (strncmp(&szDest[iIndex], szOld, strlen(szOld)) == 0) {
	    /* Writes the substring szNew in the substring szOld position */
	    strcpy(&szAuxString[iAuxStringIndex], szNew);
	    iAuxStringIndex += strlen(szNew);
	    iIndex += strlen(szOld);
	} else {
	    /* We copy the character without any change */
	    szAuxString[iAuxStringIndex] = szDest[iIndex];
	    iAuxStringIndex++;
	    iIndex++;
	}
    }

    /* Puts the zero (mark of end of string) to the auxiliar string */
    szAuxString[iAuxStringIndex] = '\0';

    /* We copy the new string to szDest */
    strcpy(szDest, szAuxString);
}
