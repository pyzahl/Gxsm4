/***********************************************************************

	LIBRARY : stmbat.lib

	MODULE  : fileio.c

	PURPOSE : File Input / Output

	FUNCTIONS :  FileCheck ()
		 FileRead  ()
		 FileWrite ()

************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "batch.h"

char *getpgmhval(char *ptr, int *val);

int stricmp(const char *s1, const char *s2){
	char *sa, *sb, *s;
	int ret;
	sa=strdup(s1);
	sb=strdup(s1);
	for(s=sa; *s; s++) *s=tolower(*s);
	for(s=sb; *s; s++) *s=tolower(*s);
	ret=strcmp(s1, s2);
	free(sa); 
	free(sb);
	return ret;
}

char *strupr(char *s1){
	char *s;
	for(s=s1; *s; s++) *s=toupper(*s);
	return(s1);
}

void *salloc (long AnzDerObj, USHORT ObjSize)
/*
  Dies ist die genaue Nachbildung der Library Fkt. halloc ()
  allerdings mit einem OS2 Call
*/
{
#ifdef GNUCC
	SEL sel;
	char *retval;
#endif
	long bytanz;
	bytanz = AnzDerObj * (long)ObjSize;

#ifdef GNUCC
	if (0==(DosAllocHuge((unsigned short)(bytanz / 65536l),	/* number of segments	       */
			     (unsigned short)(bytanz % 65536l),	/* size of last segment	       */
			     &sel,		       /* address of selector	       */
			     0,		       /* sharing flag		       */
			     0)))		       /* maximum segments for realloc */
	{
		retval = MAKEP(sel, 0);
		return(retval);
	}
	else
		return (NULL);
#else
	return( malloc(bytanz) );
#endif
}

void sfree (void *ptr)
/* Das Gegenstück zu salloc */
{
#ifdef GNUCC
	SEL selc;

	selc = SELECTOROF (ptr);
	DosFreeSeg (selc);
#else
	free(ptr);
#endif
}

long FileCheck (char *fname, FILETYPE *ftyp, FILEHEAD *fhead, FILE **fh)
//char fname[];		// Dateiname , voller Pfad
//FILETYPE *ftyp;	// welche Art von Datei ? ,wird zurückgeliefert
//FILEHEAD *fhead;	// Bildeckkoordinaten  ,werden zurückgeliefert
//int *fh;		// Dateihandle
/* Teste eine Datei auf Existenz,
   falls sie existiert, überprüfe den Typ
   Ist es eine Daten Datei ?, falls ja lade den Kopf
   sonst lade die Größe (erste zwei Words)
   berechne aus dem Typ den erforderlichen Speicher

   returns : Anzahl der Datenbytes in der Datei
   -1 im Fehlerfall d.h. Datei existiert nicht
*/
{
	int pc;
	long numofbyt, sizfac;
	char exstr[4];
	XYCoord xyanz;

	// if ((acc=access(fname,4 /* read permiss */)) == -1)
	/* access failed, fatal return */
	//	return (-1);
	/* die letzten drei Buchstaben von fname werden als extension angenommen
	   und entsprechend ausgewertet */
	strcpy (exstr,fname+strlen(fname)-3);
	strupr(exstr);
	if (strstr(exstr,"DAT")) {
		*ftyp = DATFIL;
		sizfac = sizeof(short);
	}
	else if (strstr(exstr,"BYT")) {
		*ftyp = BYTFIL;
		sizfac = sizeof(char);
	}
	else if (strstr(exstr,"SHT")) {
		*ftyp = SHTFIL;
		sizfac = sizeof(short);
	}
	else if (strstr(exstr,"SPE")) {
		*ftyp = SPEFIL;
		sizfac = sizeof(short);
	}
	else if (strstr(exstr,"FLT")) {
		*ftyp = FLTFIL;
		sizfac = sizeof(float);
	}
	else if (strstr(exstr,"DBL")) {
		*ftyp = DBLFIL;
		sizfac = sizeof(double);
	}
	else if (strstr(exstr,"CPX")) {
		*ftyp = CPXFIL;
		sizfac = 2*sizeof(float);
	}
	else if (strstr(exstr,"PGM")) {
		*ftyp = PGMFIL;
		sizfac = sizeof(char);
	}
	else if (strstr(exstr,"D2D")) {
		*ftyp = D2DFIL;
		sizfac = sizeof(short);
	}
	else {   /* unknown Type */
		return (-1);
	}
	/* open file, lese Header oder Datenkopf */
	if ((*fh = fopen (fname, "rb")) == 0)
		return (-1);
	if (*ftyp == DATFIL) {
		if (fread ((char *)fhead, sizeof(HEADER),(size_t)1,*fh) != 1)
			return (-1);
		numofbyt = 0; /* If Spectroscopy make Loop */
		if (fhead->kopf.NumOfVolts == 0) fhead->kopf.NumOfVolts++;
		for (pc=0; pc < fhead->kopf.NumOfVolts; ++pc)
			numofbyt += (long)fhead->kopf.nx[pc]*(long)fhead->kopf.ny[pc]*sizfac;
	}
	else {
		if (*ftyp == PGMFIL) {
			char *ptr, zeile[256];
			int value=0;
			if (fgets (zeile, 256, *fh)){
				if(strncmp(zeile,"P5",2)){
					fprintf(stderr, "%s", zeile);
					fprintf(stderr, "%s", "\nKein PGM qFile, P5 Kennung wurde nicht erkannt !\n");
					return (-1);
				}
			} else 
				return (-1);
			ptr=zeile+2;
			while((ptr=getpgmhval(ptr,&value))==0) 
				if (! fgets(ptr=zeile, 256, *fh))
					return (-1);
			xyanz.x=(short)value;

			while((ptr=getpgmhval(ptr,&value))==0) 
				if (! fgets(ptr=zeile, 256, *fh))
					return (-1);
			xyanz.y=(short)value;

			while((ptr=getpgmhval(ptr,&value))==0) 
				if (! fgets(ptr=zeile, 256, *fh))
					return (-1);
			if(value != 255)
				fprintf(stderr,"ngrey=%d < 255 !\n",value);

			fhead->xydim.x = xyanz.x;
			fhead->xydim.y = xyanz.y;
			numofbyt = (long)xyanz.x*(long)xyanz.y*sizfac;
			printf("PGM Info: %dx%d = %ld\n",xyanz.x, xyanz.y, numofbyt);
		}
		else{
			if (*ftyp == D2DFIL) {
				if (fread ((char *)fhead, sizeof(D2D_SPA_DAT_HEADER),(size_t)1,*fh) != 1)
					return (-1);
				numofbyt = (long)(fhead->d2dkopf.PointsX+1)*(long)(fhead->d2dkopf.PointsY)*sizfac;
				fseek(*fh,0x180, SEEK_SET); // Auf Datenanfang Positionieren
			}
			else{
				if(*ftyp == SPEFIL){ /* CCD Camera Format */
					fhead->xydim.x = xyanz.x = 512;
					fhead->xydim.y = xyanz.y = 512;
					numofbyt = (long)xyanz.x*(long)xyanz.y*sizfac;
					if (fseek(*fh,4100L, SEEK_SET))
						return (-1);
					*ftyp = SHTFIL; /* Trick 17 :=) */
				}
				else{
					if (fread((char *)&xyanz, sizeof(short), (size_t)2, *fh) != 2)
						return (-1);
					fhead->xydim.x = xyanz.x;
					fhead->xydim.y = xyanz.y;
					numofbyt = (long)xyanz.x*(long)xyanz.y*sizfac;
				}
			}
		}
	}
	/* no Fileclose */
	return (numofbyt);
}

char *getpgmhval(char *ptr, int *val){
	while(*ptr == ' ') ptr++;
	if(*ptr == '#') { 
		printf("%s", ptr);
		return 0; 
	}
	if(*ptr == '\n') return 0;
	if(*ptr == 0) return 0;
	if((*val = atoi(ptr)) == 0) { fprintf(stderr, "Garbage in PGM-Header !\n"); exit(1); }
	while(*ptr >= '0' && *ptr <='9') ptr++;
	printf("%d\n",*val);
	return ptr;
}

int FileRead (FILE *fh, char *buffer, long nbytes)
/*
  liest die nbytes aus der offenen Datei 'fh' (Filehandle) nach buffer

  benötigt vorher einen FileCheck(..) Aufruf
  fh: Fileptr
  buffer: Datenbuffer
  nbytes: Bytes to Read

  returns  :	0 bei ok
  -1 sonst
  liest aus offener Datei nbytes Bytes 
*/
{
	size_t br;
	if((br=fread((void*)buffer, (size_t)1, (size_t)nbytes, fh)) < (size_t)nbytes) {
		fprintf(stderr,"Could only read %ld bytes from %ld bytes !\n", (long)br, (long)nbytes);
		fclose (fh);
		return(-1);
	}
	fclose (fh);
	return (0);
}


int FileWrite (char *fname, char *buffer, FILEHEAD *fhead)
/*
  Öffnet Datei 'fname' und schreibt zunächst den Header
  und dann die Daten aus buffer in die Datei.

  //char *fname;	   // Dateiname
  //char *buffer;	   // den es zu schreiben gilt
  //FILEHEAD *fhead;   // falls Kopf geschrieben werden soll

  returns  :	0 bei ok
  -1 sonst

*/
{
	FILE *fh;
	int pc;
	size_t ret;
	char exstr[4];
	long numofbyt, sizfac;
	FILETYPE ftyp;

	strcpy (exstr,fname+strlen(fname)-3);
	strupr(exstr);
	if (strstr(exstr,"DAT")) {
		ftyp = DATFIL;
		sizfac = sizeof(short);
	}
	else if (strstr(exstr,"BYT")) {
		ftyp = BYTFIL;
		sizfac = sizeof(char);
	}
	else if (strstr(exstr,"SHT")) {
		ftyp = SHTFIL;
		sizfac = sizeof(short);
	}
	else if (strstr(exstr,"FLT")) {
		ftyp = FLTFIL;
		sizfac = sizeof(float);
	}
	else if (strstr(exstr,"DBL")) {
		ftyp = DBLFIL;
		sizfac = sizeof(double);
	}
	else if (strstr(exstr,"CPX")) {
		ftyp = CPXFIL;
		sizfac = 2*sizeof(float);
	}
	else if (strstr(exstr,"PGM")) {
		ftyp = PGMFIL;
		sizfac = sizeof(char);
	}
	else {   /* unknown Type */
		return (-1);
	}
	/* open file, schreibe Header oder Datenkopf */
	if ((fh = fopen (fname, "wb"))== 0)
		return (-1);
	if (ftyp==DATFIL) {
		if(fwrite ( (char *)fhead, sizeof(HEADER), (size_t)1, fh) < 1) 
			return (-1);
		numofbyt = 0; /* If Spectroscopy make Loop */
		if (fhead->kopf.NumOfVolts == 0) fhead->kopf.NumOfVolts++;
		for (pc=0; pc < fhead->kopf.NumOfVolts; ++pc)
			numofbyt += (long)fhead->kopf.nx[pc]*(long)fhead->kopf.ny[pc]*sizfac;
	}
	else {
		if (ftyp==PGMFIL){
			fprintf(fh,"P5\n# von gnu-tools convert\n%d %d\n255\n", 
				(int)fhead->xydim.x, (int)fhead->xydim.y);
			numofbyt = (long)fhead->xydim.x*(long)fhead->xydim.y*sizfac;
		}
		else{
			if(fwrite ((char *)fhead, 2*sizeof(short), (size_t)1, fh) < 1)
				return (-1);
			numofbyt = (long)fhead->xydim.x*(long)fhead->xydim.y*sizfac;
		}
	}
	if( (ret=fwrite ((void*)buffer, (size_t)1, (size_t)numofbyt, fh)) < (size_t)numofbyt){
		fprintf(stderr,"ist#%ld soll#%ld\n",(long)ret,numofbyt);
		fclose (fh);
		return -1;
	}

	fclose (fh);
	return (int)ret;
}
