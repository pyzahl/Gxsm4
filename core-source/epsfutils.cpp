/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001,2002,2003 Percy Zahl
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

/*!
  Functions for creating postscript code from Gxsm.
  Placing the scanimage on the page, adding ticsmarks, adding text,
  opening and saving the postscript to a file, all this is done 
  through the class methods in this file. It should be the only
  interface through which PS-code is generated.
*/

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

#include <locale.h>
#include <libintl.h>


#include "surface.h"
#include "xsmmasks.h"
#include "glbvars.h"
#include "action_id.h"
#include "epsfutils.h"
#include "util.h"
#include "unit.h"

/*! 
  Postscript code for special symbol table. 
*/
/* \305 := Aring */
const char *PSCode[]={
	"%%BeginSetup\n",
	"% Make Fonts ISO-Encoded, taken from xmgr\n",
	"mark\n",
	"/ISOLatin1Encoding\n",
	"  8#000 1 8#054 {StandardEncoding exch get} for\n",
	"  /minus\n",
	"  8#056 1 8#217 {StandardEncoding exch get} for\n",
	"  /dotlessi\n",
	"  8#301 1 8#317 {StandardEncoding exch get} for\n",
	"  /space /exclamdown /cent /sterling /currency /yen /brokenbar /section\n",
	"  /dieresis /copyright /ordfeminine /guillemotleft /logicalnot /hyphen\n",
	"  /registered /macron /degree /plusminus /twosuperior /threesuperior /acute\n",
	"  /mu /paragraph /periodcentered /cedilla /onesuperior /ordmasculine\n",
	"  /guillemotright /onequarter /onehalf /threequarters /questiondown /Agrave\n",
	"  /Aacute /Acircumflex /Atilde /Adieresis /Aring /AE /Ccedilla /Egrave /Eacute\n",
	"  /Ecircumflex /Edieresis /Igrave /Iacute /Icircumflex /Idieresis /Eth /Ntilde\n",
	"  /Ograve /Oacute /Ocircumflex /Otilde /Odieresis /multiply /Oslash /Ugrave\n",
	"  /Uacute /Ucircumflex /Udieresis /Yacute /Thorn /germandbls /agrave /aacute\n",
	"  /acircumflex /atilde /adieresis /aring /ae /ccedilla /egrave /eacute\n",
	"  /ecircumflex /edieresis /igrave /iacute /icircumflex /idieresis /eth /ntilde\n",
	"  /ograve /oacute /ocircumflex /otilde /odieresis /divide /oslash /ugrave\n",
	"  /uacute /ucircumflex /udieresis /yacute /thorn /ydieresis\n",
	"  /ISOLatin1Encoding where not {256 array astore def} if\n",
	" cleartomark\n",
	"\n",
	"/makeISOEncoded\n",
	"{ findfont /curfont exch def\n",
	"  /newfont curfont maxlength dict def\n",
	"  /ISOLatin1 (-ISOLatin1) def\n",
	"  /curfontname curfont /FontName get dup length string cvs def\n",
	"  /newfontname curfontname length ISOLatin1 length add string\n",
	"     dup 0                  curfontname putinterval\n",
	"     dup curfontname length ISOLatin1   putinterval\n",
	"  def\n",
	"  curfont\n",
	"  { exch dup /FID ne\n",
	"    { dup /Encoding eq\n",
	"      { exch pop ISOLatin1Encoding exch }\n",
	"      if\n",
	"      dup /FontName eq\n",
	"      { exch pop newfontname exch }\n",
	"      if\n",
	"      exch newfont 3 1 roll put\n",
	"    }\n",
	"    { pop pop }\n",
	"    ifelse\n",
	"  }\n",
	"  forall\n",
	"  newfontname newfont definefont\n",
	"} def\n",
	"\n",
	"/Helvetica makeISOEncoded pop\n",
	"/Helvetica-Bold makeISOEncoded pop\n",
	"\n",
	"% by Dirk:\n",
	"% set raster frequency  \n",
	"% Syntax: freq setfrequency  \n",
	"/setfrequency \n",
	"{\n",
	"        /setfreqoldscreen currentscreen 3 1 roll pop pop def\n",
	"        45 /setfreqoldscreen load setscreen\n",
	"} def\n\n",
	"\n",
	"%%EndSetup\n",
	"%ENDEMARKE"
};

/* Example: Load PS code for font:
   /Helvetica-ISOLatin1 findfont titleth scalefont setfont
*/


/*!
  Constructor. Only for initializing.
*/
EpsfTools::EpsfTools(int paper){
	imgdef = FALSE;
	font=NULL;
	PicNo  = 0;
	papertyp = paper;
	SetAbbWidth();
	SetFontSize();
	NIcons();
}

/*!
  Destructor. Only for clean up.
*/
EpsfTools::~EpsfTools(){
	close();
	if(font) g_free(font);
}

/*!
  Insert font size selection in PS-stream.
*/
void EpsfTools::SetFontSize(double p){ 
	if(font) g_free(font);
	FontSize = p * Width/FIGNRMWIDTH;
	font = g_strdup_printf("/Helvetica-ISOLatin1 findfont %d scalefont setfont\n", (int)FontSize);
}

/*!
  Open file for writing. 

  \param A char* for the filename.

  \param int fullpage. ???

  \param int info. ???

  \return Returns 0 on success and -1 on failure to open file for writing.

*/
int EpsfTools::open(char *name, int fullpage, int typ, int info){
	fname = name;
	Icf.open(fname, std::ios::out | std::ios::trunc);
	if(!Icf.good()){
		XSM_SHOW_ALERT(ERR_SORRY, ERR_FILEWRITE,fname,1);
		return (-1);
	}
	putheader(fullpage, typ, info);
	imgdef=FALSE;
	return 0;
}

/*!
  Insert neccessary EPSF header information into the PS-stream.

  \param int fullpage. ???

  \param int typ. ???

  \param int info. ???
*/
void EpsfTools::putheader(int fullpage, int typ, int info){
	int i;
	*EpsfBuffer=0;
	time_t t;
	time(&t);
	sprintf(EpsfBuffer,"%%!PS-Adobe-2.0 EPSF-2.0\n");
	sprintf(EpsfBuffer+strlen(EpsfBuffer),"%%%%Creator: [%s]\n", PACKAGE "-" VERSION);
	sprintf(EpsfBuffer+strlen(EpsfBuffer),"%%%%Title: %s\n",fname);
	sprintf(EpsfBuffer+strlen(EpsfBuffer),"%%%%CreationDate: %s", ctime (&t));
	//      _strdate(EpsfBuffer+strlen(EpsfBuffer)); 
	strcat(EpsfBuffer,"%%Pages: (atend)\n");
	strcat(EpsfBuffer,"%%Orientation: Portrait\n");
	if (papertyp == A4PAPER)
		strcat(EpsfBuffer,"%%DocumentPaperSizes: a4\n");
	else
		strcat(EpsfBuffer,"%%DocumentPaperSizes: letter\n");
	strcat(EpsfBuffer,"%%DocumentFonts: Helvetica-ISOLatin1\n");
	if((typ==ID_PRINTT_PLAIN || typ==ID_PRINTT_CIRC) && info==ID_PRINTI_NONE)
		sprintf(EpsfBuffer+strlen(EpsfBuffer),"%%%%BoundingBox: 100 150 500 550\n");
	else
		if(fullpage)
			sprintf(EpsfBuffer+strlen(EpsfBuffer),"%%%%BoundingBox: 50 0 %d %d\n", 
				580, papertyp == A4PAPER ? 830:790);
		else
			sprintf(EpsfBuffer+strlen(EpsfBuffer),"%%%%BoundingBox: 47 47 %d %d\n", 553, 603);
	strcat(EpsfBuffer,"%%EndComments\n");
	i=0;
	while(strncmp(PSCode[i],"%ENDEMARKE",9))
		strcat(EpsfBuffer,PSCode[i++]);
  
	Icf.write(EpsfBuffer, strlen(EpsfBuffer));
}

/*!
  Calculate Nx and Ny for resizing the image.

  \param int Nx. Call by reference.

  \param int Ny. Call by reference.
*/

void EpsfTools::makesize(int &Nx, int &Ny){
	if(Nx>Ny){
		Ny=Ny*400/Nx;
		Nx=400;
		if(Ny < 10) Ny=100; // stretch it !!
	}else{
		Nx=Nx*400/Ny;
		Ny=400;
		if(Nx < 10) Nx=100; // stretch it !!
	}
}

/*!
  Translate PS coordinate system.

  \param int Nx. 

  \param int Ny. 
*/
void EpsfTools::tr2picorigin(int Nx, int Ny){
	strcat(EpsfBuffer,"% ->tr2picorigin\n");
	sprintf(EpsfBuffer+strlen(EpsfBuffer),"%d %d translate\n",50-Nx/2+200,50-Ny/2+200);
	strcat(EpsfBuffer,"% <-\n");
}

/*!
  Insert a frame aroudn the image into PS-stream.
*/

void EpsfTools::putframe(){
	*EpsfBuffer=0;
	strcat(EpsfBuffer,"% ->putframe\n");
	strcat(EpsfBuffer,"gsave\n");

	strcat(EpsfBuffer,"% lower left corner of picture\n");
	strcat(EpsfBuffer,"50 100 translate\n\n");
	strcat(EpsfBuffer,"0 -50 moveto\n");
	strcat(EpsfBuffer,"500 -50 lineto\n");
	strcat(EpsfBuffer,"500 500 lineto\n");
	strcat(EpsfBuffer,"0 500 lineto\n");
	strcat(EpsfBuffer,"0 -50 lineto\n closepath stroke\n");

	strcat(EpsfBuffer,"grestore\n");
	Icf.write(EpsfBuffer, strlen(EpsfBuffer));
}

/*!
  Draw a line on top of the image.

  \param Scan *s, int x1, y1, x2, y2.

*/

void EpsfTools::putline(Scan *s, int x1, int y1, int x2, int y2){// stefans helper for printing regions.
        /* This function prints a line with the 
        coordinates (x1|y1) to (x2|y2) in Angstrom,
        Scan *s is needed to determine the images size.
        */
        *EpsfBuffer=0;
        strcat(EpsfBuffer,"% ->putline\n");
        strcat(EpsfBuffer,"gsave\n");
        strcat(EpsfBuffer,"% draw a line\n");
        strcat(EpsfBuffer,"50 100 translate\n\n");
        
        int Nx=s->mem2d->GetNx();
        int Ny=s->mem2d->GetNy();
        makesize(Nx, Ny);
        /*
        // Sorry PZ for the many cout's. they will be gone soon.
        cout << "nx = " << Nx << " ny= " << Ny << endl;
        cout << " alles = " << x1 << " " << y1 << " " << x2 << " " << y2 << endl;
        cout << " getnx " << s->mem2d->GetNx() << endl;
        cout << " getny " << s->mem2d->GetNy() << endl;
        cout << "xrange " << (s->data.s.rx) << endl;            // rx ist xrange
        cout << "xoffset " << (s->data.s.x0) << endl;           // x0 ist x-offset
        cout << "yrange " << (s->data.s.ry) << endl;            // ry ist yrange  
        cout << "yoffset " << (s->data.s.y0) << endl;           // y0 ist y-offset
        */
        tr2picorigin(Nx, Ny);
                
        double A,B,C; /* These calculations are done like on paper,
        I think the compiler will efficiently sort this out.*/
        /* x-translation */
        A=(s->data.s.x0)-(s->data.s.rx/2);
        
        B=x2-A;
        C= B/(s->data.s.rx)*Nx;
        x2 = (int)C; 
        
        B=x1-A;
        C= B/(s->data.s.rx)*Nx;
        x1 = (int)C; 
        /* x-translation-end*/
        /* y-translation */   
        A=(s->data.s.y0)-y2;  
        B=A/(s->data.s.ry)*Ny;
        C=Ny-B;
        y2=(int)C - Ny/2 ; //Test

        A=(s->data.s.y0)-y1;
        B=A/(s->data.s.ry)*Ny;
        C=Ny-B;
        y1=(int)C - Ny/2; //Test
        /* y-translation-end*/

        sprintf(EpsfBuffer+strlen(EpsfBuffer), "%d %d moveto\n", x1, y1);
        sprintf(EpsfBuffer+strlen(EpsfBuffer), "%d %d lineto\n stroke\n", x2, y2);

        strcat(EpsfBuffer,"grestore\n");
        Icf.write(EpsfBuffer, strlen(EpsfBuffer));
}

/*!
  Draw a circle on top of the image. The first coordinate is the center.
  The second coordinate lies on the circle itself.

  \param Scan *s, int x1, y1, x2, y2.

*/

void EpsfTools::putcircle(Scan *s, int x1, int y1, int x2, int y2){
        /* This function prints a circle with the 
        center (x1|y1) and (x2|y2) lying on the circle in Angstrom,
        Scan *s is needed to determine the images size.
        The circle is clipped to the imagesize.
        */
        *EpsfBuffer=0;
        strcat(EpsfBuffer,"gsave\n");
        strcat(EpsfBuffer,"% draw a circle: putcircle()\n");
        strcat(EpsfBuffer,"50 100 translate\n\n");
        
        int Nx=s->mem2d->GetNx();
        int Ny=s->mem2d->GetNy();
        makesize(Nx, Ny);
        tr2picorigin(Nx, Ny);
                
        double A,B,C; /* These calculations are done like on paper,
        I think the compiler will efficiently sort this out.*/
        /* x-translation */
        A=(s->data.s.x0)-(s->data.s.rx/2);
        
        B=x2-A;
        C= B/(s->data.s.rx)*Nx;
        x2 = (int)C; 

        B=x1-A;
        C= B/(s->data.s.rx)*Nx;
        x1 = (int)C; 
        /* x-translation-end*/
        /* y-translation */   
        A=(s->data.s.y0)-y2;  
        B=A/(s->data.s.ry)*Ny;
        C=Ny-B;   
        y2=(int)C - Ny/2; //Test

        A=(s->data.s.y0)-y1;  
        B=A/(s->data.s.ry)*Ny;
        C=Ny-B;   
        y1=(int)C - Ny/2; //Test
        /* y-translation-end*/  
        /* radius calculation */
        A = sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
        /* radius calculation-end */

        /* Clipping area*/
        sprintf(EpsfBuffer+strlen(EpsfBuffer), "newpath\n 0 0 moveto\n");
        sprintf(EpsfBuffer+strlen(EpsfBuffer), "%i 0 lineto \n %i %i lineto \n 0 %i lineto\n ", Nx, Nx, Ny, Ny);
        sprintf(EpsfBuffer+strlen(EpsfBuffer), "closepath \n clip\n");

        sprintf(EpsfBuffer+strlen(EpsfBuffer), "newpath %i %i %f 0 360 arc\n stroke\n", x1, y1, A);

        strcat(EpsfBuffer,"grestore\n");
        Icf.write(EpsfBuffer, strlen(EpsfBuffer));
}



/*!
  Draw tic marks.

*/
int EpsfTools::putticks(Scan *s, int OriginZero){
	int Nx=s->mem2d->GetNx();
	int Ny=s->mem2d->GetNy();
	int I,X,nSkX,nSkY;
	double l10;
	double Lab2X,Lab,dLab,MaxLab,LabW,Lab0;
	char sklprecX[10];
	char sklprecY[10];
	char strl[60];

	makesize(Nx, Ny);
	if(Nx>Ny){
		nSkX=20; nSkY=Ny*20/Nx; 
		if(nSkY==0) nSkY=1;
	}
	else{
		nSkY=20; nSkX=Nx*20/Ny; 
		if(nSkX==0) nSkX=1;
	}

	*EpsfBuffer=0;
	strcat(EpsfBuffer,"% ->putticks\n");
	strcat(EpsfBuffer,"gsave\n");

	strcat(EpsfBuffer,"% lower left corner of picture\n");
	strcat(EpsfBuffer,"50 100 translate\n\n");
  
	strcat(EpsfBuffer,"% size of image (on paper, in 1/72inch coords)\n");
    
	tr2picorigin(Nx, Ny);
    
	strcat(EpsfBuffer,font);

	strcat(EpsfBuffer,"0 0 moveto\n");
	sprintf(EpsfBuffer+strlen(EpsfBuffer),"%d 0 lineto\n",Nx);
	sprintf(EpsfBuffer+strlen(EpsfBuffer),"%d %d lineto\n",Nx,Ny);
	sprintf(EpsfBuffer+strlen(EpsfBuffer),"0 %d lineto\n",Ny);
	strcat(EpsfBuffer,"0 0 lineto\n stroke\n");
	    
	if(OriginZero){
		Lab0=0.;
		MaxLab=s->data.Xunit->Base2Usr(s->data.s.rx);
	}else{
		Lab0=s->data.Xunit->Base2Usr(s->data.s.x0);
		MaxLab=s->data.Xunit->Base2Usr(s->data.s.rx+Lab0);
	}
	// load Label info from File
	std::ifstream labf;
	labf.open("/tmp/skloverride",std::ios::in);
	struct { 
		int valid; int validSkl; 
		double xmin,xmax,ymin,ymax; 
		int xn,yn; 
		char labx[40], laby[40], tit[40]; 
	} sklinfo;
	sklinfo.valid=0;
	sklinfo.validSkl=0;
	if(labf.good()){
		labf >> sklinfo.labx >> sklinfo.xmin >> sklinfo.xmax >> sklinfo.xn;
		labf >> sklinfo.laby >> sklinfo.ymin >> sklinfo.ymax >> sklinfo.yn;
		labf >> sklinfo.tit;
		labf.close();

		XSM_DEBUG(DBG_L2, "Using Labinfo !" );
		XSM_DEBUG(DBG_L2, sklinfo.labx << " " << sklinfo.xmin << " " <<   sklinfo.xmax << " " << sklinfo.xn );
		XSM_DEBUG(DBG_L2, sklinfo.laby << " " << sklinfo.ymin << " " <<   sklinfo.ymax << " " << sklinfo.yn );
		nSkX = sklinfo.xn;
		nSkY = sklinfo.yn;
		Lab0 = sklinfo.xmin;
		MaxLab = sklinfo.xmax;
		sklinfo.valid=1;
		sklinfo.validSkl=1;
	}

	labf.open("/tmp/TXYlabels",std::ios::in);
	if(labf.good()){
		labf >> sklinfo.tit;
		labf >> sklinfo.labx >> sklinfo.laby;
		sklinfo.valid=1;
		labf.close();
	}

	// load Precision of Labels !
	// default:
	strcpy(sklprecX,"6.2");
	strcpy(sklprecY,"6.2" );

	// X & Y
	labf.open("/tmp/labprec");
	if(labf.good()){
		labf >> sklprecX; // z.B.: "6.4" ==> [%6.4f]
		labf.close();
		strcpy(sklprecY,sklprecX);
	}
	// set X prec
	labf.open("/tmp/labprecX");
	if(labf.good()){
		labf >> sklprecX; // z.B.: "6.4" ==> [%6.4f]
		labf.close();
	}
	// set Y prec
	labf.open("/tmp/labprecY");
	if(labf.good()){
		labf >> sklprecY; // z.B.: "6.4" ==> [%6.4f]
		labf.close();
	}

	if(Lab0 < MaxLab){
		LabW = MaxLab-Lab0;
		dLab = AutoSkl(LabW/(double)nSkX);
		l10 = pow(10.,floor(log10(MaxLab)));
		if(l10 < 1000. && l10 > 0.01) l10=1.;
		if(dLab <= 0.) return(-1);
		Lab2X=(double)Nx/LabW;
		for(I=0, Lab=AutoNext(Lab0,dLab); Lab<MaxLab; Lab+=dLab, ++I){
			X=R2INT((Lab-Lab0)*Lab2X);
			if(I%5)
				sprintf(EpsfBuffer+strlen(EpsfBuffer),
					"%d 0 moveto %d -5 lineto %d %d moveto %d %d lineto stroke\n",
					X,X,X,Ny,X,Ny+5);
			else{
				sprintf(EpsfBuffer+strlen(EpsfBuffer),
					"%d 0 moveto %d -10 lineto %d %d moveto %d %d lineto stroke\n",
					X,X,X,Ny,X,Ny+10);
				sprintf(strl,"%cd -25 moveto (%c%sf) show\n",'%','%',sklprecX);
				sprintf(EpsfBuffer+strlen(EpsfBuffer),strl,X-3,Lab/l10);
			}
		}
	}
	else{
		LabW = Lab0-MaxLab;
		dLab = AutoSkl(LabW/(double)nSkX);
		l10 = pow(10.,floor(log10(MaxLab)));
		if(l10 < 1000. && l10 > 0.01) l10=1.;
		if(dLab <= 0.) return(-1);
		Lab2X=(double)Nx/LabW;
		for(I=0, Lab=AutoNext(MaxLab,dLab); Lab<Lab0; Lab+=dLab, ++I){
			X=Nx-R2INT((Lab-MaxLab)*Lab2X);
			if(I%5)
				sprintf(EpsfBuffer+strlen(EpsfBuffer),
					"%d 0 moveto %d -5 lineto %d %d moveto %d %d lineto stroke\n",
					X,X,X,Ny,X,Ny+5);
			else{
				sprintf(EpsfBuffer+strlen(EpsfBuffer),
					"%d 0 moveto %d -10 lineto %d %d moveto %d %d lineto stroke\n",
					X,X,X,Ny,X,Ny+10);
				sprintf(strl,"%cd -25 moveto (%c%sf) show\n",'%','%',sklprecX);
				sprintf(EpsfBuffer+strlen(EpsfBuffer),strl,X-3,Lab/l10);
			}
		}
	}
  
	if(sklinfo.valid)
		sprintf(EpsfBuffer+strlen(EpsfBuffer),
			"150 -40 moveto (%s (*%g)) show\n",
			sklinfo.labx,l10);
	else
		sprintf(EpsfBuffer+strlen(EpsfBuffer),
			"150 -40 moveto (x %.0f %s) show\n",
			l10,s->data.Xunit->psSymbol());
  
	if(OriginZero){
		Lab0=0.;
		MaxLab=s->data.Yunit->Base2Usr(s->data.s.ry);
	}else{
		Lab0=s->data.Yunit->Base2Usr(s->data.s.y0-s->data.s.ry);
		MaxLab=s->data.Yunit->Base2Usr(s->data.s.ry+Lab0);
	}

	if(sklinfo.validSkl){
		Lab0 = sklinfo.ymin;
		MaxLab = sklinfo.ymax;
	}

	if(Lab0 < MaxLab){
		LabW = MaxLab-Lab0;
		dLab = AutoSkl(LabW/(double)nSkY);
		l10 = pow(10.,floor(log10(MaxLab)));
		if(l10 < 1000. && l10 > 0.01) l10=1.;
		if(dLab <= 0.) return(-1);
		Lab2X=(double)Ny/LabW;
    
		for(I=0, Lab=AutoNext(Lab0,dLab); Lab<MaxLab; Lab+=dLab, ++I){
			X=R2INT((Lab-Lab0)*Lab2X);
			if(I%5)
				sprintf(EpsfBuffer+strlen(EpsfBuffer),
					"0 %d moveto -5 %d lineto %d %d moveto %d %d lineto stroke\n",
					X,X,Nx,X,Nx+5,X);
			else{
				sprintf(EpsfBuffer+strlen(EpsfBuffer),
					"0 %d moveto -10 %d lineto %d %d moveto %d %d lineto stroke\n",
					X,X,Nx,X,Nx+10,X);
				sprintf(strl,"-40 %cd moveto (%c%sf) show\n",'%','%',sklprecY);
				sprintf(EpsfBuffer+strlen(EpsfBuffer),strl,X-4,Lab/l10);
			}
		}
	}
	else{
		LabW = Lab0-MaxLab;
		dLab = AutoSkl(LabW/(double)nSkY);
		l10 = pow(10.,floor(log10(MaxLab)));
		if(l10 < 1000. && l10 > 0.01) l10=1.;
		if(dLab <= 0.) return(-1);
		Lab2X=(double)Ny/LabW;
		for(I=0, Lab=AutoNext(MaxLab,dLab); Lab<Lab0; Lab+=dLab, ++I){
			X=Ny-R2INT((Lab-MaxLab)*Lab2X);
			if(I%5)
				sprintf(EpsfBuffer+strlen(EpsfBuffer),
					"0 %d moveto -5 %d lineto %d %d moveto %d %d lineto stroke\n",
					X,X,Nx,X,Nx+5,X);
			else{
				sprintf(EpsfBuffer+strlen(EpsfBuffer),
					"0 %d moveto -10 %d lineto %d %d moveto %d %d lineto stroke\n",
					X,X,Nx,X,Nx+10,X);
				sprintf(strl,"-40 %cd moveto (%c%sf) show\n",'%','%',sklprecY);
				sprintf(EpsfBuffer+strlen(EpsfBuffer),strl,X-4,Lab/l10);
			}
		}
	}

	if(sklinfo.valid)
		sprintf(EpsfBuffer+strlen(EpsfBuffer),
			"-40 %d moveto (%s (*%g)) show\n",
			Ny+20,sklinfo.laby,l10);
	else
		sprintf(EpsfBuffer+strlen(EpsfBuffer),
			"-40 %d moveto (x %.0f %s) show\n",
			Ny+20,l10,s->data.Yunit->psSymbol());
	strcat(EpsfBuffer,"grestore\n");
	Icf.write(EpsfBuffer, strlen(EpsfBuffer));
  
	return 0;
}

/*!
  Draw greyscale side bar.

*/
void EpsfTools::putbar(Scan *s){
	int Nx=s->mem2d->GetNx();
	int Ny=s->mem2d->GetNy();
	int I=0,X=0,nSkX=10;
	double l10, BarLen;
	double Lab2X,Lab,dLab,MaxLab;
	makesize(Nx, Ny);

	*EpsfBuffer=0;
	strcat(EpsfBuffer,"% ->putbar\n");
	strcat(EpsfBuffer,"gsave\n");
	strcat(EpsfBuffer,"50 100 translate\n\n");
	tr2picorigin(Nx, Ny);

	strcat(EpsfBuffer,"50 -30 translate\n\n");

	strcat(EpsfBuffer,font);

	BarLen = 1./3.;
	nSkX   = 6;
	MaxLab=s->data.Xunit->Base2Usr(s->data.s.rx*BarLen);
	dLab = AutoSkl(MaxLab/(double)nSkX);
	l10 = pow(10.,floor(log10(MaxLab)));
	if(dLab > 0.){
		Lab2X=(double)Nx/MaxLab*BarLen;
    
		strcat(EpsfBuffer,"3 setlinewidth\n");
		sprintf(EpsfBuffer+strlen(EpsfBuffer),
			"0 0 moveto %d 0 lineto closepath stroke\n", 
			R2INT(MaxLab*Lab2X));

		strcat(EpsfBuffer,"1 setlinewidth\n");

		for(I=0, Lab=0.; Lab<MaxLab; Lab+=dLab, ++I){
			X=R2INT(Lab*Lab2X);
			if(I%2)
				sprintf(EpsfBuffer+strlen(EpsfBuffer),
					"%d 3 moveto %d -3 lineto closepath stroke\n", X, X);
			else{
				sprintf(EpsfBuffer+strlen(EpsfBuffer),
					"%d 5 moveto %d -5 lineto closepath stroke\n", X, X);
				sprintf(EpsfBuffer+strlen(EpsfBuffer),
					"%d -20 moveto (%4.2f) show\n",X-3,Lab/l10);
			}
		}
		sprintf(EpsfBuffer+strlen(EpsfBuffer),"%d -20 moveto (x %.0f %s) show\n", 
			X+50, l10,s->data.Xunit->psSymbol());
	}
	strcat(EpsfBuffer,"grestore\n");
	Icf.write(EpsfBuffer, strlen(EpsfBuffer));
}

/*!
 EPSF positioning and sizing.
*/
void EpsfTools::init(){
	*EpsfBuffer=0;
	strcat(EpsfBuffer,"% ->init\n");
	if (nPicPage > 0)
		if (!((PicNo)%nPicPage))
			sprintf(EpsfBuffer+strlen(EpsfBuffer),"%%%%Page: %d %d\n",
				1+PicNo/nPicPage, 1+PicNo/nPicPage);
				
	Icf.write(EpsfBuffer, strlen(EpsfBuffer));
}

/*!
   Translate to image position, start image section - must be "closed" by endimage !
*/
void EpsfTools::placeimage(){
	*EpsfBuffer=0;
	strcat(EpsfBuffer,"% ->placeimage\n");

	if(nPicPage > 0 && MkTyp > 0){
		++PicNo;
		double sfac = papertyp == A4PAPER? 0.46 : 0.44;
		strcat(EpsfBuffer,"gsave\n");
		sprintf(EpsfBuffer+strlen(EpsfBuffer),"%d %d translate\n",
			60+EPSF_XM*(((PicNo-1)%nPicPage)%(MkTyp*2))/(MkTyp*2),
			20+(papertyp == A4PAPER? EPSF_YM_A4:EPSF_YM_LETTER)*((MkTyp*3-1)-
				    ((((PicNo-1)%nPicPage)
				      /(MkTyp*2))%(MkTyp*3)))/(MkTyp*3));
		sprintf(EpsfBuffer+strlen(EpsfBuffer),"%d 100 div  %d 100 div scale\n",
			(int)(100*sfac/(double)MkTyp),(int)(100*sfac/(double)MkTyp));
	}
	else{
		++PicNo;
		strcat(EpsfBuffer,"gsave\n");
		if(Width != FIGNRMWIDTH){
			strcat(EpsfBuffer,"60 20 translate\n");
			sprintf(EpsfBuffer+strlen(EpsfBuffer),"%d 100 div %d 100 div scale\n",
				(int)(100*Width/FIGNRMWIDTH), (int)(100*Width/FIGNRMWIDTH));
		}
	}
  
	strcat(EpsfBuffer,"%70 setfrequency\n\n");
  
	Icf.write(EpsfBuffer, strlen(EpsfBuffer));

	imgdef=TRUE;
}

/*!
   End image section.
*/
void EpsfTools::endimage(){
	*EpsfBuffer=0;
	strcat(EpsfBuffer,"% ->endimage\n");
	strcat(EpsfBuffer,"grestore\n");

	Icf.write(EpsfBuffer, strlen(EpsfBuffer));
}


/*!
  Check for newpage and add footer with pagenumber and date.
*/
void EpsfTools::FootLine(Scan *s, int force){
	*EpsfBuffer=0;
	sprintf(EpsfBuffer,"%% ->FootLine, Pic %d/%d\n", PicNo, nPicPage);
	if(nPicPage > 0){
		if((!(PicNo%nPicPage)) || force){
			strcat(EpsfBuffer,"gsave\n");
			strcat(EpsfBuffer,"/Helvetica-ISOLatin1 findfont 10 scalefont setfont\n");
			strcat(EpsfBuffer,"150 23 moveto\n");
			sprintf(EpsfBuffer+strlen(EpsfBuffer),"(STM-Icons Page:%d  Print-Date:",
				PicNo/nPicPage + (force ? 1:0));
			//	_strdate(EpsfBuffer+strlen(EpsfBuffer)); 
			strcat(EpsfBuffer,"  Data-Date:"); 
			sprintf(EpsfBuffer+strlen(EpsfBuffer),"%s",s->data.ui.dateofscan);
			strcat(EpsfBuffer,") show\n");
			strcat(EpsfBuffer,"grestore\n");
			strcat(EpsfBuffer,"showpage\n\n");
		}
	}
	Icf.write(EpsfBuffer, strlen(EpsfBuffer));
}

/*!
 EPSF Bitmap Stuff. Add matrix definition.
*/
void EpsfTools::putimgdef(Mem2d *m, int y){
	int Nx=m->GetNx();
	int Ny=m->GetNy();
	static int yy;
	makesize(Nx, Ny);

	*EpsfBuffer=0;
	if(y == -1){
		strcat(EpsfBuffer,"% ->putimagedef\n");
		strcat(EpsfBuffer,"50 100 translate\n\n");
		tr2picorigin(Nx, Ny);
    
		strcat(EpsfBuffer,"% dimensions\n");
		sprintf(EpsfBuffer+strlen(EpsfBuffer),"/Breite %d def\n/Hoehe %d def\n\n", 
			m->GetNx(), m->GetNy());
		strcat(EpsfBuffer,"% define string to hold a scanline's worth of data\n");
		strcat(EpsfBuffer,"/pix Breite string def\n\n");
		sprintf(EpsfBuffer+strlen(EpsfBuffer),"%d %d scale\n\n",Nx, Ny);
		strcat(EpsfBuffer,"Breite Hoehe 8\t\t% dimensions of data\n");
		strcat(EpsfBuffer,"[Breite 0 0 Hoehe neg 0 Hoehe]\t\t% mapping matrix\n");
		strcat(EpsfBuffer,"{currentfile pix readhexstring pop}\n");
		strcat(EpsfBuffer,"image\n\n\0\0");
	}else{
		if(y==0){
			strcat(EpsfBuffer,"% ->putimagedef, Start circ\n");
			strcat(EpsfBuffer,"50 100 translate\n\n");
			tr2picorigin(Nx, Ny);
			yy=0;
		}else{
			sprintf(EpsfBuffer+strlen(EpsfBuffer),"%d 100 div %d 100 div translate\n", 
				(int)((double)Nx*100.*(.5-(double)y/2./m->GetNx())), 
				(int)((double)Ny*100.*(yy++)/m->GetNy()));
			sprintf(EpsfBuffer+strlen(EpsfBuffer),"/Breite %d def\n/Hoehe 1 def\n", y);
			strcat(EpsfBuffer,"/pix Breite string def\n");
			sprintf(EpsfBuffer+strlen(EpsfBuffer),"%d 100 div %d 100 div scale\n\n",
				(int)(Nx*100.*y/m->GetNx()), 
				(int)(Ny*100./m->GetNy()*1.01));
			strcat(EpsfBuffer,"Breite Hoehe 8\t\t% dimensions of data\n");
			strcat(EpsfBuffer,"[Breite 0 0 Hoehe neg 0 Hoehe]\t\t% mapping matrix\n");
			strcat(EpsfBuffer,"{currentfile pix readhexstring pop}\n");
			strcat(EpsfBuffer,"image\n\n\0\0");
		}
	}
  
	Icf.write(EpsfBuffer, strlen(EpsfBuffer));
}

/*!
  Add greymatrix.
*/
void EpsfTools::putgrey(Scan *s, Mem2d *m, int autoskl, int quick, int option){
	const char *HexTab = "0123456789ABCDEF";
	int    PelWert;
	long   i, j, k;

	// Detect image Pel range
//	m->SetDataPktMode(SCAN_V_QUICK);
//	m->SetDataPktMode(SCAN_V_DIRECT);
	m->SetDataRange(0, 0xff);
//	m->AutoHistogrammEvalMode();
	m->AutoDataSkl(NULL, NULL);

	*EpsfBuffer=0;
	strcat(EpsfBuffer,"% ->putgrey\n");
  
	if(option){ // rundes Bild erzeugen
		double a2,b2,y;
		int nn;
		a2=m->GetNx()/2; a2*=a2;
		b2=m->GetNy()/2; b2*=b2;
		putimgdef(m, 0);
		for(i=0; i<m->GetNy(); i++){
			y = m->GetNy()/2.-i;
			nn=2*(int)rint(sqrt(a2*(1.-y*y/b2)));
			//      XSM_DEBUG(DBG_L2, "<: " << y << " nn:" << nn );
			if(nn>0){
				Icf << "gsave\n";
				putimgdef(m, nn);
				for(k=0, j=m->GetNx()/2-nn/2; nn--; j++){
					PelWert = m->GetDataVMode(j,i);
					EpsfBuffer[k++] = HexTab[(PelWert & 0xf0) >> 4];
					EpsfBuffer[k++] = HexTab[PelWert & 0x0f];
					if(k>72){
						EpsfBuffer[k++] = 0; strcat(EpsfBuffer,"\n\0");
						Icf.write(EpsfBuffer,k);
						k=0;
					}
				}
				EpsfBuffer[k++] = 0; strcat(EpsfBuffer,"\n\0");
				Icf.write(EpsfBuffer,k);
				Icf << "grestore\n";
			}
		}
	}else{
		putimgdef(m);
		for(i=0; i<m->GetNy(); i++){
			for(k=j=0; j<m->GetNx(); j++){
				PelWert = m->GetDataVMode(j,i);
				EpsfBuffer[k++] = HexTab[(PelWert & 0xf0) >> 4];
				EpsfBuffer[k++] = HexTab[PelWert & 0x0f];
				if(k>72){
					EpsfBuffer[k++] = 0; strcat(EpsfBuffer,"\n\0");
					Icf.write(EpsfBuffer,k);
					k=0;
				}
			}
			EpsfBuffer[k++] = 0; strcat(EpsfBuffer,"\n\0");
			Icf.write(EpsfBuffer,k);
		}
	}
	imgdef=FALSE;
}


/*!
 Add some text info. Put sizedata.
*/
void EpsfTools::putsize(Scan  *s){
	XSM_DEBUG (DBG_L3, "PutSize");
	int i=0;
	gchar *su[20];
	gchar *comm = g_new(gchar, 82);
	strncpy(comm, s->data.ui.comment, 80); comm[81]=0;
	gchar *npix = g_strdup_printf ("  [%d x %d]", s->data.s.nx, s->data.s.ny);
	gchar *gstr;

	memset (su, 0, sizeof(su));

// note about: warning: operation on "i" may be undefined -- does not matter, as the order of i is irrelevant here.
	gstr = g_strconcat("% ->putsize\n",
			      "gsave\n",
			      "% lower left corner\n",
			      font,
			      "150 585 moveto\n",
			      "(Name: ", s->data.ui.name, "  ) show\n",
			      "60 85 moveto\n",
			      "(", su[0]=s->data.Xunit->UsrString(s->data.s.rx, UNIT_SM_PS),
			      " x ", su[1]=s->data.Yunit->UsrString(s->data.s.ry, UNIT_SM_PS), 
			      npix,
			      "  VRZ: ", su[2]=s->data.Zunit->UsrString(s->data.display.vrange_z, UNIT_SM_PS), 
			      ") show\n",
			      "60 70 moveto\n", 
			      "(dxy: ", su[3]=s->data.Xunit->UsrString(s->data.s.dx, UNIT_SM_PS),
			      " x ", su[4]=s->data.Yunit->UsrString(s->data.s.dy, UNIT_SM_PS), 
			      ") show\n",
			      "( ",
			      " *** ",
//			      "  I: ", su[5]=s->data.CurrentUnit->UsrString(s->data.hardpars.ITunnelSoll, UNIT_SM_PS),
//			      "  U: ", su[6]=s->data.VoltUnit->UsrString(s->data.hardpars.UTunnel, UNIT_SM_PS),
//			      "  SP: ", su[7]=s->data.VoltUnit->UsrString(s->data.hardpars.SetPoint, UNIT_SM_PS),
			      ") show\n",
			      "60 55 moveto\n",
			      "(", comm, ") show\n",
			      "grestore\n",
			      NULL);

	Icf.write (gstr, strlen (gstr));
	g_free (npix);
	g_free (gstr);
	i=0; while (su[i]) g_free (su[i++]);
	g_free (comm);
}

/*!
  Add more data to the page. Title, scan name, etc.
*/
void EpsfTools::putmore(Scan  *s, char *Title){
	int psy=740;
	const int dpsy=15;
	const int pstx1=60;
	const int pstx2=pstx1+100;
	//  const int pstx3=pstx2+200;
	//  const int pstx4=pstx3+100;

	int i=0;
	gchar *su[10];
	gchar *mv1, *mv2;
	gchar *gstr;
	XSM_DEBUG (DBG_L3, "PutMore");

	memset (su, 0, sizeof(su));

	gstr = g_strconcat("% ->putmore\n",
			   "gsave\n",
			   font,
			   "60 760 moveto\n (", Title ? Title : " ", ") show\n",
		     
			   mv1=g_strdup_printf("%d %d moveto\n",pstx1,psy),
			   "(Scan Name:) show ",
			   mv2=g_strdup_printf("%d %d moveto\n",pstx2,psy),
			   "(", s->data.ui.name, ", ", s->data.ui.originalname, ") show\n",
			   NULL);
	XSM_DEBUG(DBG_L2, gstr );
	Icf.write(gstr, strlen(gstr)); g_free(gstr); g_free(mv1); g_free(mv2); psy-=dpsy;

	gstr = g_strconcat(
		mv1=g_strdup_printf("%d %d moveto\n",pstx1,psy),
		"(Scan Type:) show ",
		mv2=g_strdup_printf("%d %d moveto\n",pstx2,psy),
		"(", s->data.ui.type, ") show\n",
		NULL);
	Icf.write(gstr, strlen(gstr)); g_free(gstr); g_free(mv1); g_free(mv2); psy-=dpsy;

	gstr = g_strconcat(
		mv1=g_strdup_printf("%d %d moveto\n",pstx1,psy),
		"(Date of Scan:) show ",
		mv2=g_strdup_printf("%d %d moveto\n",pstx2,psy),
		"(", s->data.ui.dateofscan, ") show\n",
		NULL);
	Icf.write(gstr, strlen(gstr)); g_free(gstr); g_free(mv1); g_free(mv2); psy-=dpsy;

	gstr = g_strconcat(
		mv1=g_strdup_printf("%d %d moveto\n",pstx1,psy),
		"(User:) show ",
		mv2=g_strdup_printf("%d %d moveto\n",pstx2,psy),
		"(", s->data.ui.user, "@", s->data.ui.host, ") show\n",
		NULL);
	Icf.write(gstr, strlen(gstr)); g_free(gstr); g_free(mv1); g_free(mv2); psy-=dpsy;

	char hostname[256]; 
	gethostname(hostname, 256);
	time_t t;
	time(&t);
	gstr = g_strconcat(
		mv1=g_strdup_printf("%d %d moveto\n",pstx1,psy),
		"(Printed by:) show ",
		mv2=g_strdup_printf("%d %d moveto\n",pstx2,psy),
		"(", getlogin() ? getlogin():"???", "@", hostname, ", ", ctime(&t), ") show\n",
		NULL);
	Icf.write(gstr, strlen(gstr)); g_free(gstr); g_free(mv1); g_free(mv2); psy-=dpsy;

	gstr = g_strconcat(
		mv1=g_strdup_printf("%d %d moveto\n",pstx1,psy),
		"(DSP Settings:) show ",
		mv2=g_strdup_printf("%d %d moveto\n",pstx2,psy),
		"(",
		" *** ",
//		"  I: ", su[i++]=s->data.CurrentUnit->UsrString(s->data.hardpars.ITunnelSoll, UNIT_SM_PS),
//		"  U: ", su[i++]=s->data.VoltUnit->UsrString(s->data.hardpars.UTunnel, UNIT_SM_PS),
//		"  SP: ", su[i++]=s->data.VoltUnit->UsrString(s->data.hardpars.SetPoint, UNIT_SM_PS),
		") show\n",
		NULL);
	Icf.write(gstr, strlen(gstr)); g_free(gstr); g_free(mv1); g_free(mv2); psy-=dpsy;

	// s->data.display.contrast|bright|..
	// s->data.hardpars.UTunnel|xsm->CurrentUnit->Base2User(ITunnelSoll)!!!|CP|CI|Rotation|SetPoint|...
  
	i=0; while (su[i]) g_free (su[i++]);

	gstr = g_strconcat("% ->putmore\n",
			   "grestore\n",
			   NULL);
	Icf.write(gstr, strlen(gstr)); g_free(gstr);
}

/*!
  Close PS-file.
*/
void EpsfTools::close(){
	gchar *pages;
	if(nPicPage > 0)
		pages = g_strdup_printf("%d",1+PicNo/nPicPage);
	else
		pages = g_strdup("1");

	gchar *gstr 
		= g_strconcat("% ->close\n",
			      "showpage\n\n",
			      "%%Trailer\n",
			      "%%Pages: ", pages, "\n",
			      "%%EOF\n",
			      NULL);

	Icf.write(gstr, strlen(gstr));
	Icf.close();

	g_free(gstr);
	g_free(pages);
}

/*!
  SPA-mode.
*/

void SPA_epsftools::putsize(Scan  *s){
	XSM_DEBUG (DBG_L4, "PutSize");
	int i=0;
	gchar *su[10];
	gchar *comm = g_strdup_printf("%40s", s->data.ui.comment);
	gchar *gstr;
	memset (su, 0, sizeof(su));

	gstr = g_strconcat("% ->putsize\n",
			      "gsave\n",
			      "% lower left corner\n",
			      font,
			      "60 85 moveto\n",
			      "(", s->data.ui.name, "  ) show\n",
			      "(", su[0]=s->data.Xunit->UsrString(s->data.s.rx, UNIT_SM_PS),
			      " x ", su[1]=s->data.Yunit->UsrString(s->data.s.ry, UNIT_SM_PS), 
			      ") show\n",
			      "60 70 moveto\n", 
			      //		  "(Schrittweite: ", su[2]=s->data.Xunit->UsrString(s->data.s.dx, UNIT_SM_PS),
			      //		  " x ", su[3]=s->data.Yunit->UsrString(s->data.s.dy, UNIT_SM_PS), 
			      //		  ") show\n",
			      "(",
			      " *** ",
// **			      "  E: ", su[4]=s->data.EnergyUnit->UsrString(s->data.hardpars.SPA_Energy, UNIT_SM_PS),
// **			      "  GateTm: ", su[5]=s->data.TimeUnitms->UsrString(s->data.hardpars.SPA_Gatetime, UNIT_SM_PS),
// **			      "  Len: ", su[6]=s->data.Xunit->UsrString(s->data.hardpars.SPA_Length, UNIT_SM_PS),
// **			      "  CpsHi: ", su[7]=s->data.CPSUnit->UsrString(s->data.display.cpshigh*s->data.s.dz, UNIT_SM_PS),
			      ") show\n",
			      "60 55 moveto\n",
			      "(", comm, ") show\n",
			      "grestore\n",
			      NULL);

	Icf.write(gstr, strlen(gstr));
	i=0; while (su[i]) g_free (su[i++]);
	g_free(gstr); 
	g_free(comm);
}

/*!
  SPA-mode. More data.
*/
void SPA_epsftools::putmore(Scan  *s, char *Title){
	int psy=740;
	const int dpsy=15;
	const int pstx1=60;
	const int pstx2=pstx1+100;
	//  const int pstx3=pstx2+200;
	//  const int pstx4=pstx3+100;
	gchar *comm = g_strdup_printf("%40s", s->data.ui.comment);

	int i=0;
	gchar *su[10];
	gchar *mv1, *mv2;
	gchar *gstr;
	XSM_DEBUG (DBG_L4, "PutMore");
	gstr = g_strconcat("% ->putmore\n",
			   "gsave\n",
			   font,
			   "60 760 moveto\n (", Title ? Title : " ", ") show\n",
		     
			   mv1=g_strdup_printf("%d %d moveto\n",pstx1,psy),
			   "(Scan Name:) show ",
			   mv2=g_strdup_printf("%d %d moveto\n",pstx2,psy),
			   "(", s->data.ui.name, ", ", s->data.ui.originalname, ") show\n",
			   NULL);
	XSM_DEBUG(DBG_L2, gstr );
	Icf.write(gstr, strlen(gstr)); g_free(gstr); g_free(mv1); g_free(mv2); psy-=dpsy;

	gstr = g_strconcat(
		mv1=g_strdup_printf("%d %d moveto\n",pstx1,psy),
		"(Scan Type:) show ",
		mv2=g_strdup_printf("%d %d moveto\n",pstx2,psy),
		"(", s->data.ui.type, ") show\n",
		NULL);
	Icf.write(gstr, strlen(gstr)); g_free(gstr); g_free(mv1); g_free(mv2); psy-=dpsy;

	gstr = g_strconcat(
		mv1=g_strdup_printf("%d %d moveto\n",pstx1,psy),
		"(Date of Scan:) show ",
		mv2=g_strdup_printf("%d %d moveto\n",pstx2,psy),
		"(", s->data.ui.dateofscan, ") show\n",
		NULL);
	Icf.write(gstr, strlen(gstr)); g_free(gstr); g_free(mv1); g_free(mv2); psy-=dpsy;

	gstr = g_strconcat(
		mv1=g_strdup_printf("%d %d moveto\n",pstx1,psy),
		"(User:) show ",
		mv2=g_strdup_printf("%d %d moveto\n",pstx2,psy),
		"(", s->data.ui.user, "@", s->data.ui.host, ") show\n",
		NULL);
	Icf.write(gstr, strlen(gstr)); g_free(gstr); g_free(mv1); g_free(mv2); psy-=dpsy;

	char hostname[256]; 
	gethostname(hostname, 256);
	time_t t;
	time(&t);
	gstr = g_strconcat(
		mv1=g_strdup_printf("%d %d moveto\n",pstx1,psy),
		"(Printed by:) show ",
		mv2=g_strdup_printf("%d %d moveto\n",pstx2,psy),
		"(", getlogin() ? getlogin():"???", "@", hostname, ", ", ctime(&t), ") show\n",
		NULL);
	Icf.write(gstr, strlen(gstr)); g_free(gstr); g_free(mv1); g_free(mv2); psy-=dpsy;

	gstr = g_strconcat(
		mv1=g_strdup_printf("%d %d moveto\n",pstx1,psy),
		"(DSP Settings:) show ",
		mv2=g_strdup_printf("%d %d moveto\n",pstx2,psy),
		"(",
		" *** ",
// **		"  E: ", su[i++]=s->data.EnergyUnit->UsrString(s->data.hardpars.SPA_Energy, UNIT_SM_PS),
// **		"  GateTm: ", su[i++]=s->data.TimeUnitms->UsrString(s->data.hardpars.SPA_Gatetime, UNIT_SM_PS),
// **		"  Len: ", su[i++]=s->data.Xunit->UsrString(s->data.hardpars.SPA_Length, UNIT_SM_PS),
		") show\n",
		NULL);
	Icf.write(gstr, strlen(gstr)); g_free(gstr); g_free(mv1); g_free(mv2); psy-=dpsy;

	// s->data.display.contrast|bright|..
	// s->data.hardpars.UTunnel|xsm->CurrentUnit->Base2User(ITunnelSoll)!!!|CP|CI|Rotation|SetPoint|...
  
	while(i) g_free(su[--i]);

	gstr = g_strconcat("% ->putmore\n",
			   "grestore\n",
			   NULL);
	Icf.write(gstr, strlen(gstr)); g_free(gstr);
	g_free(comm);
}


