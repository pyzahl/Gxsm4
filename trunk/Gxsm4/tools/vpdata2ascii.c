#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//define readout buffer length
#define BUFLEN 4096
//define readout format string length
#define formatstrlen 1024
typedef char string[256];
float version = 1.2;

//define verschiedene search string constanen
char searchstr0[] = "# GXSM Vector Probe Data :: VPVersion=";
char searchstr1[] = "Yes";
char searchstr2[] = "S[1]";
char searchstr3[] = "S[2]";
char searchstr4[] = "# Probe Data Number      :: N=";
char searchstr5[] = "#C Index";
char searchstr6[] = "# GXSM-DSP-Control-STS   :: #IV=";
//define string separators constanten
char seps1[] = "# ,\t\n";
char seps23[] = " \n";
char seps4[] = "x \"\n";
char seps5[] = "\t\"";

//function "how many found": searchs for a "srchstr" in "buff", using "seps", and counts it...
int hmf(char *buff, char *srchstr, char *seps)
{
    char *token;
    int ret;
    ret = 0;
    token = strtok(buff, seps);
    while (token != NULL) {
	/* While there are tokens in "string" */
	if (!strcmp(token, srchstr))
	    ret++;
	/* Get next token: */
	token = strtok(NULL, seps);
    }
    return ret;
}

//function: "where is first found", searchs for a "srcstr" occurrence in "buff", returns offset
int wiff(char *buff, char *srchstr)
{
    char *pdest;
    int ret;
    if ((pdest = strstr(buff, srchstr)) == NULL) {
// Not found!  
	return 0;
    } else {
//returns offset
	ret = (int) (pdest - buff);
// found @ ret
	return ret;
    }
}

//main function
int main(int argc, char *argv[])
{
    FILE *fptr, *fptr2;		// input/output file pointer
    char *buffer, *buf, *token, *formatstr;	//buffers pointers
    float file_version = 0.; 
    float *y_val, *x_val, readout[22], y_avg = 0.;	//2 pointers of float type and readout array
    //different int's
    int filesize = 0, i = 0, j = 0, numcol = 0, num_cycle = 0, numofpasup = 0, numofpasdn = 0, numofpnt = 0, headerbeginat = 0, headerlength = 0, pointsperpass = 0;	//numofpas = 0,index =0
    //filenames arrays
    char filetoread[255], filetowrite[255], copyofheader[512];
    //descriptors of columns
    string col_descriptor[22];
    //column numbers: xcol = bias, ycol = measured values...
    int xcol = 0, ycol = 0;

    printf("\n\nVPDATA to ASCII converter v. %.1f\n\n", version);
    switch (argc) {		//parsing command line parameters
	// in case of 4 arguments the converter can be started without further inquiry
    case (5):
	//printf("\nWe have found %d commandline parameters: ", argc-1);
	//for (i=1; i<argc; i++) printf("%s ", argv[i]);      printf("\n");
//          if ((sscanf(argv[1], "%s", &filetoread)!=1) | (sscanf(argv[2], "%s", &filetowrite)!=1) | (sscanf(argv[3], "%d", &xcol)!=1) | (sscanf(argv[4], "%d", &ycol)!=1)) 
	if ((sscanf(argv[1], "%s", filetoread) !=
	     1) | (sscanf(argv[2], "%s",
			  filetowrite) != 1) | (sscanf(argv[3], "%d",
						       &xcol) !=
						1) | (sscanf(argv[4], "%d",
							     &ycol) !=
						      1)) {
	    printf("Cannot parse commandline!\n");
	    printf("Syntax: converter filetoread filetowrite xcol ycol\n");
	    exit(EXIT_FAILURE);
	}
	break;
	// otherwise the program will quit
    default:
	printf("\nSyntax: %s filetoread filetowrite xcol ycol\n", argv[0]);
	exit(EXIT_FAILURE);
    }

/*#########################################
opening input file, looking for filesize
#########################################*/
    if ((fptr = fopen(filetoread, "r")) == NULL) {
	printf("fopen(): File not found!\n");
	exit(EXIT_FAILURE);
    }
    if ((buffer = malloc(BUFLEN)) == NULL) {
	printf("malloc(): Memory allocation failed!\n");
	exit(EXIT_FAILURE);
    }
//looking for input filesize
    while (!feof(fptr))
	filesize += fread(buffer, sizeof(char), BUFLEN, fptr);
    //printf( "Filesize: %d\n", filesize );

//allocating buffer "buf" of "filesize" length
    if ((buf = malloc(filesize)) == NULL) {
	free(buffer);
	printf("malloc(): Memory allocation failed!\n");
	exit(EXIT_FAILURE);
    }

/*#########################################
check file version
#########################################*/
    if (fseek(fptr, 0L, SEEK_SET)) {
	free(buffer);
	free(buf);
	printf("fseek(): File seek failed!\n");
	exit(EXIT_FAILURE);
    }
    fread(buf, sizeof(char), filesize, fptr);
    sscanf(buf + wiff(buf, searchstr0) + sizeof(searchstr0) - 1, "%f",
	   &file_version);

/*#########################################
gathering channel header informations...
#########################################*/
    if (fseek(fptr, 0L, SEEK_SET)) {
	free(buffer);
	free(buf);
	printf("fseek(): File seek failed!\n");
	exit(EXIT_FAILURE);
    }
    fread(buf, sizeof(char), filesize, fptr);

//get header position   
    headerbeginat = wiff(buf, searchstr5);	//looking for "#C Index"
    headerlength = wiff(buf + headerbeginat, "\n");	//number of char from headerbegin to the next \n is the lenght
    memset(copyofheader, 0, sizeof(copyofheader));	//due to problems with strncpy or strtok, filling with 000000000........
    strncpy(copyofheader, buf + headerbeginat, headerlength);

//get column names and counting #columns
    token = strtok(copyofheader, seps5);
    while (token != NULL) {
	strcpy(col_descriptor[numcol], token);
	//printf( "%d for %s\n", numcol, col_descriptor[numcol] );
	numcol++;
	token = strtok(NULL, seps5);
    }
    if (numcol == 0) {
	numcol = 1;
	free(buffer);
	free(buf);
//      printf("Sorry, no column informations found. Bye...\n", searchstr1, 0, numcol);         
	printf("Sorry, no column informations found. Bye...\n");
	exit(EXIT_FAILURE);
    }
//commandline parameter checkup
    if ((xcol > numcol) | (ycol > numcol) | (xcol == ycol)) {
	printf("Commandline parameters for columns are not correct!\n");
	exit(EXIT_FAILURE);
    }

/*#########################################
analysing vector informations
#########################################*/
//forward direction - searching for occurrences of "S[1]" 
    if (fseek(fptr, 0L, SEEK_SET)) {
	free(buffer);
	free(buf);
	printf("fseek(): File seek failed!\n");
	exit(EXIT_FAILURE);
    }
    fread(buf, sizeof(char), filesize, fptr);
    numofpasup = hmf(buf, searchstr2, seps23);

//backward direction - searching for occurrences of "S[2]"
    if (fseek(fptr, 0L, SEEK_SET)) {
	free(buffer);
	free(buf);
	printf("fseek(): File seek failed!\n");
	exit(EXIT_FAILURE);
    }
    fread(buf, sizeof(char), filesize, fptr);
    numofpasdn = hmf(buf, searchstr3, seps23);
    //printf("\"%s\": %d -> %d pass(es) down\n", searchstr3, numofpasdn, numofpasdn);

/*#########################################
total number of points
#########################################*/
    if (fseek(fptr, 0L, SEEK_SET)) {
	free(buffer);
	free(buf);
	printf("fseek(): File seek failed!\n");
	exit(EXIT_FAILURE);
    }
    fread(buf, sizeof(char), filesize, fptr);
    sscanf(buf + wiff(buf, searchstr4) + sizeof(searchstr4) - 1, "%d",
	   &numofpnt);

/*#########################################
creating format string for a single row
#########################################*/
    if ((formatstr = malloc(formatstrlen)) == NULL) {
	printf("malloc(): Memory allocation failed!\n");
	exit(EXIT_FAILURE);
    }
    memset(formatstr, 0, sizeof(formatstrlen));
    strcpy(formatstr, "%f\t");
    for (i = 1; i <= numcol - 1; i++)
	strcat(formatstr, "%f\t");
    strcat(formatstr, "\n");
    //printf("Format: %s", formatstr);

/*#########################################
points per pass
#########################################*/
    if (file_version >= (float) 0.02) { 
	fseek(fptr, 0L, SEEK_SET);
	sscanf(buf + wiff(buf, searchstr6) + sizeof(searchstr6) - 1, "%d",
	       &num_cycle);
	printf("Taking number of cycles from header (file version >= 0.02)\n");
    } else if (numofpasdn != 0) {
	num_cycle = (int) (((double) (numofpasup + numofpasdn - 1)) / 2.);
	printf("Taking number of cycles from number of passes (file version < 0.02)\n");
    } else {
	//analysis of Umon to get correct cycle number as last try
	printf("Warning: Using cycle autodetection!\n\n");
	//Allocating memory for Umon
	float *Umon;
	if ((Umon = malloc(numofpnt * sizeof(float))) == NULL) {
	    free(formatstr);
	    printf("malloc(): Memory allocation failed!\n");
	    exit(EXIT_FAILURE);
	}
	//seek to begin of measurement data
	if (fseek(fptr, headerbeginat + headerlength + 1, SEEK_SET)) {
	    free(formatstr);
	    free(Umon);
	    printf("fseek(): File seek failed!\n");
	    exit(EXIT_FAILURE);
	}
	//readout Umon
	for (i = 0; i < numofpnt; i++) {
	    fscanf(fptr, formatstr, &readout[0], &readout[1], &readout[2],
		   &readout[3], &readout[4], &readout[5], &readout[6],
		   &readout[7], &readout[8], &readout[9], &readout[10],
		   &readout[11], &readout[12], &readout[13], &readout[14],
		   &readout[15], &readout[16], &readout[17], &readout[18],
		   &readout[19], &readout[20], &readout[21], &readout[22]);
	    Umon[i] = readout[1];
	}
	for (i = 0; i < (numofpnt - 2); i++) {
	    if ((Umon[i + 2] - Umon[i + 1]) * (Umon[i + 1] - Umon[i]) < 0) {	//check for first change of gradient
		if (numofpnt % (i + 2) == 0)
		    num_cycle = numofpnt / (i + 2);	//consistency check 
		break;
	    }
	}
	free(Umon);
    }
    if (num_cycle == 0)
	num_cycle = 1;		// just make sure that there is no devision by zero   
    pointsperpass = numofpnt / num_cycle;

/*#########################################
file analysis finished
#########################################*/
    printf("   File version: %.2f\n", file_version);
    if (numofpasdn > 0)
	printf("   Dual mode data\n");
    else
	printf("   Single mode data\n");
    //printf("Total number of Points: %d\n", numofpnt);     
    printf("   %d cycles, %d points per cycle\n", num_cycle,
	   pointsperpass);
    //printf("   Number of columns: %d\n\n", numcol);
    printf("   X = %s\n", col_descriptor[xcol - 1]);
    printf("   Y = %s\n", col_descriptor[ycol - 1]);

    free(buffer);
    free(buf);



/*#########################################
read data
#########################################*/
//y-values array
    if ((y_val = malloc(numofpnt * sizeof(float))) == NULL) {
	free(formatstr);
	printf("malloc(): Memory allocation failed!\n");
	exit(EXIT_FAILURE);
    }
//x-values array
    if ((x_val = malloc((pointsperpass + 1) * sizeof(float))) == NULL) {
	free(formatstr);
	free(y_val);
	printf("malloc(): Memory allocation failed!\n");
	exit(EXIT_FAILURE);
    }
//Readout x-values (with averaging) and y-values
    if (fseek(fptr, headerbeginat + headerlength + 1, SEEK_SET)) {
	free(formatstr);
	printf("fseek(): File seek failed!\n");
	exit(EXIT_FAILURE);
    }
    for (i = 0; i < numofpnt; i++) {
	fscanf(fptr, formatstr, &readout[0], &readout[1], &readout[2],
	       &readout[3], &readout[4], &readout[5], &readout[6],
	       &readout[7], &readout[8], &readout[9], &readout[10],
	       &readout[11], &readout[12], &readout[13], &readout[14],
	       &readout[15], &readout[16], &readout[17], &readout[18],
	       &readout[19], &readout[20], &readout[21], &readout[22]);
	if (i < pointsperpass)
	    x_val[i] = (readout[xcol - 1] / num_cycle);
	else
	    x_val[i % pointsperpass] += (readout[xcol - 1] / num_cycle);
	y_val[i] = readout[ycol - 1];
    }

//closing input file
    fclose(fptr);

/*#########################################
write output file
#########################################*/
    if ((fptr2 = fopen(filetowrite, "w+")) == NULL) {
	printf("fopen(): Failed to open/create output file!\n");
	exit(EXIT_FAILURE);
    }
//write header with channels
    fprintf(fptr2, "%s\t", col_descriptor[xcol - 1]);
    fprintf(fptr2, "%s avg.\t", col_descriptor[ycol - 1]);
    for (j = 0; j < num_cycle; j++) {
	fprintf(fptr2, "%s [%d]\t", col_descriptor[ycol - 1], j + 1);
    }
    fprintf(fptr2, "\n");

//write data
    for (i = 0; i < pointsperpass; i++) {
	//x-column
	if (xcol == 1)
	    fprintf(fptr2, "%d\t", i);	//special treatment for index as it is not floating point
	else
	    fprintf(fptr2, "%f\t", x_val[i]);
	//y_avg-column
	if (ycol == 1)
	    fprintf(fptr2, "%d\t", i);	//special treatment for index as it is not floating point
	else {
	    y_avg = 0.;
	    for (j = 0; j < num_cycle; j++) {
		y_avg += y_val[j * pointsperpass + i];
	    }
	    fprintf(fptr2, "%f\t", y_avg / num_cycle);
	}
	//y-columns
	for (j = 0; j < num_cycle; j++) {
	    if (ycol == 1)
		fprintf(fptr2, "%d\t", i);	//special treatment for index as it is not floating point
	    else
		fprintf(fptr2, "%f\t", y_val[j * pointsperpass + i]);
	}
	fprintf(fptr2, "\n");
    }

    fclose(fptr2);


/*#########################################
finishing...
#########################################*/
    free(x_val);
    free(y_val);
    free(formatstr);
    return (0);
}
