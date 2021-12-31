// new revision fixed for cygwin/unix
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

typedef char string[256];
/* 
 * Already defined by some Win32 compilers... 
 */
#ifndef MAX_PATH
# define MAX_PATH 260
#endif
/*
 * Set some operating system tweaks...
 */

//Target platform to define...
//#define OS_WIN32

#if !defined (OS_WIN32) && !defined (OS_UNIX) && !defined (OS_OS2)
# define OS_UNIX


#endif

#if defined (OS_WIN32) || defined (OS_OS2)
# define PATHSEP "\\"
#elif defined (OS_UNIX)
# define PATHSEP "/"
#endif





char* strrev (char *str)
{
        char *left = str;
        char *right = str + strlen(str) -1;
        char ch;

       while (left < right)
        {
                ch = *left;
                *left++ = *right;
                *right-- = ch;
        }

        return(str);
}
//------------------------------------------------------------------------------
//function: "where is first found", searchs for a "srcstr" occurrence in "buff", returns offset
int wiff(char *buff , char *srchstr)
{
char *pdest;
int ret;
if ( (pdest = strstr( buff, srchstr )) == NULL )  {
// Not found!  
return 0;     
}
else {
ret = (int)( pdest - buff );
// found @ ret
return ret;
}
}
//------------------------------------------------------------------------------
//function "how many found": searchs for a "srchstr" in "buff", using "seps", and counts it...
int hmf(char *buff , char *srchstr, char *seps)
{
char *token;
char *temptoken;
temptoken = buff;
int ret;
ret = 0;
token = strtok( buff, seps );
   while( token != NULL )
   {
      /* While there are tokens in "string" */
      //printf( " %s\n", token );
      if (!strcmp( token, srchstr )) ret++;
      /* Get next token: */
      token = strtok( NULL, seps );
   }
buff = temptoken;
return ret;
}


//------------------------------------------------------------------------------
int ask_for_inputfile(char *inputname)
{   
     DIR*   dptr;
     FILE* fptr;
     while (1)
     {
          printf("Please define the \"InputFile\" or \"InputDirectory\": ");
          scanf("%s", inputname);
     	  printf("%s\n",inputname);


      	struct stat file_stat;
	lstat(inputname, &file_stat);


//        if ( (fptr = fopen( inputname, "r" ))!= NULL )  Only works in Windows..
          if (!S_ISDIR(file_stat.st_mode) && (fptr = fopen( inputname, "r" ))!= NULL )
               {
               printf("Found \"%s\" file!\n", inputname);
               fclose(fptr);
               return 0;
               }
          else 
               {
               printf("File \"%s\" was not found, so trying directory...\n", inputname);
               
               if ((dptr = opendir (inputname)) != NULL) 
                    {
                    printf("Found \"%s\" directory!\n", inputname);
                    closedir(dptr);
                    return 1;
                    }
               else 
                    {
                    printf("Neither file nor directory \"%s\" was found, so try again...\n", inputname);
                    }
               }    
     }
}
//------------------------------------------------------------------------------

void ReplaceChar(char *string, char oldpiece, char newpiece)
{
   for (;*string;string++) if (*string==oldpiece) *string=newpiece;
}
//------------------------------------------------------------------------------
int convert_all(char* inputdateiname, int x)
{
     FILE *fptr;
     char *buf;
     int filesize=0, numcol=0;
     int result=0, headerbeginat=0, headerlength=0 ;//foundat=0,
     char searchstr5[] = "#C Index";
     char seps4[] = "x \"\n";
     char seps5[] = "\t\"";
     char copyofheader[255];
     char commandline[1024];
     char outputdateiname[255];
     char buffer[4096];
     char *token;
     string spalten[22];
     char *findat;
     char extention[255];
     
     if ( ( fptr = fopen( inputdateiname, "r" ))== NULL ) {
     printf( "fopen(): File not found!\n" );
     exit(EXIT_FAILURE);
     }
     //calculating filesize
     while ( !feof(fptr) ) filesize += fread( buffer, sizeof( char ), sizeof( buffer ), fptr );
     printf( "Size of Inputfile: %d\n", filesize );
//Allocating buffer "buf", legth = filesize
     if( (buf = malloc( filesize )) == NULL ) {
     printf( "malloc(): Memory allocation failed!\n" );
     exit(EXIT_FAILURE);
     }
//seeking at the begin of file
     if ( fseek( fptr, 0L, SEEK_SET ) ) {
     free(buf);
     printf( "fseek(): File seek failed!\n" );
     exit(EXIT_FAILURE);
     }
     //filling up buffer "buf"
     result = fread( buf, sizeof( char ), filesize, fptr );
     
     printf( "Searched string \"%s\" found @ %d\n", searchstr5, result=wiff(buf, searchstr5) );
     headerbeginat=result;     
     
     headerlength=wiff(buf + headerbeginat, "\n");
     printf("Header at %d, length %d\n", headerbeginat, headerlength );
     strncpy(buffer, buf + headerbeginat, headerlength + 1);
     
     printf( "Number of column(s), tabs method: %d\n", numcol = hmf(buffer, "\t", seps4) );    
     //due to problem with strncpy or strtok, filling with 000000000........
     memset(copyofheader, 0, sizeof(copyofheader));
     printf("Following columns are available:\n");
     strncpy(copyofheader, buf+headerbeginat, headerlength);
     free(buf);
     
     printf("%s\n", copyofheader);
     printf("For input:\n\n");
     token = strtok( copyofheader, seps5 );
     int nmbr=0;
     while( token != NULL )
     {
            /* While there are tokens in "string" */
            printf( "%d for %s\n", nmbr, token );
            strcpy(spalten[nmbr], token);
            nmbr++;
            /* Get next token: */
            token = strtok( NULL, seps5 );
     }
     numcol=numcol<nmbr?numcol:nmbr;
     if (x==-1){
     do {
     printf("\nPlease input x_column number (0<=numbers<=%d) you want to convert (xcol): ", numcol-1);
     scanf("%d", &x);
     } while (!((x>=0)&&(x<=numcol-1)));   
     }
     int i;
     for (i=1;i<numcol; i++) {
          if (i!=x)
          {
          //printf("[%d]:%s_vs_[%d]:%s\n", x, spalten[x], i, spalten[i]);
          strcpy(outputdateiname, inputdateiname);
      
          // file.* -> file_[colx]vs[coly].dat
          if ((findat = strrchr(outputdateiname, '.')) != NULL) 
          {
          //printf("Found@: %d\n", (int)(foundat-outputdateiname));
          sprintf(extention, "_%s_vs_%s.dat", spalten[x], spalten[i]);
          strcpy(findat, extention);
          ReplaceChar(outputdateiname, ' ', '_');
          }
//          printf("->%s %d\n", outputdateiname); ???
          printf("->%s\n", outputdateiname);
          //creating commandline string
          sprintf(commandline, "vpdata2ascii \"%s\" \"%s\" %d %d", inputdateiname, outputdateiname, x+1, i+1);
//          printf("executing vpdata2ascii %s %s %d %d", inputdateiname, outputdateiname, x+1, i+1);
          //executing
          printf("Executing: %s\n",commandline);
          system(commandline);
//         printf("executing vpdata2ascii %s %s %d %d\n", fromfile, tofile, xcol+1, ycol+1);

          }
      }
      
     return x;
}
//------------------------------------------------------------------------------
void ask_for_outputfile(char* inputdateiname, char *outputdateiname, string *spalten, int *x, int *y)
{   
     char *foundat;
     char readout[255];
//     char what2use[255];
     char extention[255];
     int inordnung=0, existenz=0;
     FILE *fptr;
     
     strcpy(outputdateiname, inputdateiname);
      
// file.* -> file_[colx]vs[coly].dat
     if ((foundat = strrchr(outputdateiname, '.')) != NULL) 
     {
     //printf("Found@: %d\n", (int)(foundat-outputdateiname));
     sprintf(extention, "_%s_vs_%s.dat", spalten[*x], spalten[*y]);
     strcpy(foundat, extention);
     //printf("%s %d\n", outputdateiname, strlen(outputdateiname));
     }
     ReplaceChar(outputdateiname, ' ', '_');
           
//output file must not exist
     while (!existenz)
     {
     //---------datei name
     while (!inordnung) 
     {
     printf("Do you want to use \"%s\" as \"OutputFile\"? (Y/N): ", outputdateiname);
//     scanf("%s", &readout);
     scanf("%s", readout);

     if ((readout[0]=='Y') || (readout[0]=='y'))
     {
      //yes
      inordnung++;
     }
     else
     {
         if ((readout[0]=='N') || (readout[0]=='n'))
         {
          //no
          printf("Specify your filename:");
//          scanf("%s", &readout); 
          scanf("%s", readout);
          strcpy(outputdateiname, readout);
          inordnung++;
         }
         else
         {
         //not yes or no
         printf("Spinnst du oder?\n");
         }
     }
     }
     //----------dateiname
// output file existenz checkup
     if ( (fptr = fopen( outputdateiname, "r" ))== NULL ) 
               {
               existenz++;
               }
     else 
     {
          fclose(fptr);          
          printf("Do you want to overwrite it? (Y/N):");
//     scanf("%s", &readout);
     scanf("%s", readout);
     if ((readout[0]=='Y') || (readout[0]=='y'))
     {
      //yes, overwrite
      existenz++;
     }
     else
     {
         if ((readout[0]=='N') || (readout[0]=='n'))
         {
          //no no
          inordnung=0;
         }
         else
         {
         //not yes or no
         printf("Spinnst du oder?\n");
         }
     }
          }
     }
}
//------------------------------------------------------------------------------
void ask_for_cols(char *inputdateiname, int *x, int *y, string *spalten)
{
     FILE *fptr;
     char *buf;
     int filesize=0, numcol=0;
     int result=0,  headerbeginat=0, headerlength=0; //foundat=0,
     char searchstr5[] = "#C Index";
     char seps4[] = "x \"\n";
     char seps5[] = "\t\"";
     char copyofheader[255];
     char buffer[4096];
     char *token;
     
     if ( ( fptr = fopen( inputdateiname, "r" ))== NULL ) {
     printf( "fopen(): File not found!\n" );
     exit(EXIT_FAILURE);
     }
     //calculating filesize
     while ( !feof(fptr) ) filesize += fread( buffer, sizeof( char ), sizeof( buffer ), fptr );
     printf( "Size of Inputfile: %d\n", filesize );
//Allocating buffer "buf", legth = filesize
     if( (buf = malloc( filesize )) == NULL ) {
     printf( "malloc(): Memory allocation failed!\n" );
     exit(EXIT_FAILURE);
     }
//seeking at the begin of file
     if ( fseek( fptr, 0L, SEEK_SET ) ) {
     free(buf);
     printf( "fseek(): File seek failed!\n" );
     exit(EXIT_FAILURE);
     }
     //filling up buffer "buf"
     result = fread( buf, sizeof( char ), filesize, fptr );
     
     printf( "Searched string \"%s\" found @ %d\n", searchstr5, result=wiff(buf, searchstr5) );
     headerbeginat=result;     
     
     headerlength=wiff(buf + headerbeginat, "\n");
     printf("Header at %d, length %d\n", headerbeginat, headerlength );
     strncpy(buffer, buf + headerbeginat, headerlength + 1);
     
     printf( "Number of column(s), tabs method: %d\n", numcol = hmf(buffer, "\t", seps4) );    
     //due to problem with strncpy or strtok, filling with 000000000........
     memset(copyofheader, 0, sizeof(copyofheader));
     printf("Following columns are available:\n");
     strncpy(copyofheader, buf+headerbeginat, headerlength);
     free(buf);
     
     printf("%s\n", copyofheader);
     printf("For input:\n\n");
     token = strtok( copyofheader, seps5 );
     int nmbr=0;
     while( token != NULL )
     {
            /* While there are tokens in "string" */
            printf( "%d for %s\n", nmbr, token );
            strcpy(spalten[nmbr], token);
            nmbr++;
            /* Get next token: */
            token = strtok( NULL, seps5 );
     }
     
     do {
     printf("\nPlease input 2 column numbers (0<=numbers<=%d, number1!=number2) you want to convert (xcol ycol): ", numcol-1);
     scanf("%d %d", x, y);
     } while (!((*x>=0)&&(*y>=0)&&(*x<=numcol-1)&&(*y<=numcol-1)&&(*x!=*y)));
}







//------------------------------------------------------------------------------
//convert directory


int convert_dir(char *fromfile,int xcol,int rec, int xforall)
{
       char    *dirname;
       DIR     *dir;
       struct  dirent  *ent;
       char    fullname[MAX_PATH];       
       struct stat     filestat;
       
       printf("Converting directory \"%s\"...\n", fromfile);
       dirname=fromfile;
       
       if (strchr ("/\\", *(dirname + strlen (dirname) - 1)) == NULL) 
  //not found "\/" at the end of filename line?
    {
      strcat (dirname, PATHSEP);
    } 

  printf ("Opening directory %s\n", dirname);

      if ((dir = opendir (dirname)) == NULL) 
        {
          perror ("Unable to open directory");
          return 1;
        }
        
      while ((ent = readdir (dir)) != NULL) 
        {

          if (!strcmp (ent->d_name, ".") || !strcmp (ent->d_name, "..")) 
            {
              continue;
            }

          sprintf (fullname, "%s%s", dirname, ent->d_name);           
          if (stat (fullname, &filestat) < 0) 
            {
              fprintf (stderr, "Oops! Could not stat \"%s\": ", fullname);
              perror (NULL);
            } 
          else 
            {
              //printf("%s is a %s\n", fullname, filestat.st_mode & S_IFDIR? "directory":(filestat.st_mode & S_IFREG? "regular file":"something else"));
              if ((filestat.st_mode && S_IFREG)&&!(filestat.st_mode & S_IFDIR))    
              {
                                   if(strncmp(strrev(fullname), "atadpv.", 7)==0) 
                                   {
                                    printf("Converting file \"%s\"...\n", strrev(fullname));
                                    
                                    if (xforall) xcol=convert_all(fullname, xcol);
                                    else convert_all(fullname, -1);
                                    ////ask for columns to use
//                                    ask_for_cols(fullname, &xcol, &ycol, &spaltennamen[0]);
//                                    printf("Columns are: %d and %d\n", xcol+1, ycol+1);
//                                    //ask for outputfile name
//                                    ask_for_outputfile(fullname, tofile, &spaltennamen[0], &xcol, &ycol);
//                                    printf("Using %s as output...\n", tofile);
//                                    //creating commandline string
//                                    sprintf(commandline, "vpdata2ascii %s %s %d %d", fullname, tofile, xcol+1, ycol+1);
//                                    printf("executing vpdata2ascii %s %s %d %d\n", fullname, tofile, xcol+1, ycol+1);
//                                    //executing
//                                    system(commandline);
            
                                    }
            }
//		else if(rec=1 && (filestat.st_mode & S_IFDIR)) {
		else if(rec==1 && (filestat.st_mode & S_IFDIR)) {
					printf("\nEntering directory: %s\n\n",fullname); 
					convert_dir(fullname,xcol,rec,xforall);
					}              

		else printf("Skipping... %s\n",fullname);         
            
            }
        }

      rewinddir (dir);

  if (closedir (dir) != 0) 
    {
      perror ("Unable to close directory");
      return 1;
    }   
       
return 0;
}
//------------------------------------------------------------------------------
//main funktion
int main( int argc, char *argv[] )
{
    int rec=0;    
    int xcol=-1, ycol; //columns numbers...
    char fromfile[255], tofile[255], commandline[1024]; //input/output filename arrays, commandline array
    string spaltennamen[22];
    //ask for inputfile name
    if (argc==2)
	if (!strcmp(argv[1],"-r"))
		{rec=1;printf("Recursive\n");}

    if (ask_for_inputfile(fromfile)) 
       {
        int inordnung=0;
        int xforall=0;
        char readout[255];
        while (!inordnung) 
        {
              printf("Do you want to use same xcol for every file? (Y/N): ");
              scanf("%s", readout);
              if ((readout[0]=='Y') || (readout[0]=='y'))
              { //yes
                inordnung++;
                xforall=1;
              } else {
                if ((readout[0]=='N') || (readout[0]=='n'))
                {//no
                     xforall=0;
                     inordnung++;
                }else{//not yes or no
                            printf("Spinnst du oder?\n");
                }
              }
        }



 		convert_dir(fromfile,xcol,rec,xforall);
       }
    else 
         {
         printf("Converting file \"%s\"...\n", fromfile);
         //ask for columns to use
         ask_for_cols(fromfile, &xcol, &ycol, &spaltennamen[0]);
         printf("Columns are: %d and %d\n", xcol+1, ycol+1);
         //ask for outputfile name
         ask_for_outputfile(fromfile, tofile, &spaltennamen[0], &xcol, &ycol);
         printf("Using %s as output...\n", tofile);
         //creating commandline string
         sprintf(commandline, "vpdata2ascii \"%s\" \"%s\" %d %d", fromfile, tofile, xcol+1, ycol+1);
//         printf("executing vpdata2ascii %s %s %d %d\n", fromfile, tofile, xcol+1, ycol+1);
	printf("Executing: %s\n",commandline);
         //executing
         system(commandline);
         }
    //exiting...
    return(0);
}


