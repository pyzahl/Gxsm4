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

#ifndef __LINEINFO_H
#define __LINEINFO_H

class LineInfo{
  friend class Mem2d;
public:
  LineInfo(){ valid=FALSE; dnew=0; stored=0; };
  ~LineInfo(){};

  inline void invalidate(){ valid=FALSE; dnew=1; stored=0; };
  void set(double &aa, double &bb){ a=aa; b=bb; valid=TRUE; };
  inline double getY(double x){ return a*x+b; };
  inline double getB(){ return b; };
  inline int IsValid(){ return valid; };
  inline int IsNew(){ return dnew; };
  inline void SetNew(int x=1){ dnew=x; };
  inline int IsStored(){ return stored; };
  inline void SetStored(int x=1){ stored=x; };
protected:
  int    valid, dnew, stored;
private:
  double a,b;
};

#endif
