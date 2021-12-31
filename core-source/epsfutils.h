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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* EPSF Optionen */
#define EPSF_AUTO  1
#define EPSF_NEXT  2
#define EPSF_nxm   4

#define EPSF_XM 500
#define EPSF_YM_A4 800
#define EPSF_YM_LETTER 740

#define FIGNRMFNTSZ 12.
#define FIGNRMWIDTH 177.

#define A4PAPER 1
#define LETTERPAPER 2

int EpsfFileWrite (char *fname, Scan *Original, Scan *Icon, int NoAuto, int rfac);

class EpsfTools{
public:
  EpsfTools(int paper=A4PAPER);
  virtual ~EpsfTools();

  void SetPaperTyp(int paper=A4PAPER) { papertyp=paper; };
  int open(char *name, int fullpage=FALSE, int typ=-1, int info=-1);
  void NIcons(int n=0){ MkTyp=n/2; nPicPage=n*n*3/2; };
  void SetAbbWidth(double mm=FIGNRMWIDTH){ Width=mm; };
  void SetFontSize(double  p=FIGNRMFNTSZ);

  void FootLine(Scan *s, int force=FALSE);

  void init();
  void placeimage();
  void putframe();
  void putline(Scan *s, int x1, int y1, int x2, int y2);
  void putcircle(Scan *s, int x1, int y1, int x2, int y2);
  int  putticks(Scan *s, int OriginZero=TRUE);
  void putgrey(Scan *s, Mem2d *m, int autoskl=TRUE, int quick=TRUE, int option=0);
  void putbar(Scan *s);
  virtual void putsize(Scan  *s);
  virtual void putmore(Scan  *s, char *Title=NULL);

  void endimage();

  void close();

 private:
  void makesize(int &Nx, int &Ny);
  void tr2picorigin(int Nx, int Ny);
  void putheader(int fullpage, int typ, int info);
  void putimgdef(Mem2d *m, int y=-1);

  char EpsfBuffer[32000];
  char *fname;
  int MkTyp;
  int imgdef;
  int PicNo;
  int nPicPage;
  int page;
  int papertyp;

 protected:
  std::ofstream Icf;
  double Width, FontSize;
  gchar *font;
};

class SPM_epsftools : public EpsfTools{
public:
  SPM_epsftools(){};
  virtual ~SPM_epsftools(){};
};

class SPA_epsftools : public EpsfTools{
public:
  SPA_epsftools(){};
  virtual ~SPA_epsftools(){};

  virtual void putsize(Scan  *s);
  virtual void putmore(Scan  *s, char *Title=NULL);

};
