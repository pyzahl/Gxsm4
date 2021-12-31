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

#include "util.h"

double AutoSkl(double dl){
  double dp=floor(log10(dl));
  if(dp!=0.)
    dl/=pow(10.,dp);
  if(dl>4) dl=5;
  else if(dl>1) dl=2;
  else dl=1;
  if(dp!=0.)
    dl*=pow(10.,dp);
  return dl;
}

double AutoNext(double x0, double dl){
  return(ceil(x0/dl)*dl);
}

void hsv2rgb(SPACOLOR *c){
  double i,f,h,m,n,k,v;
  if(c->s == 0.)
    c->r = c->g = c->b = c->v;
  else{
    if(c->h >= 360.) h=0.;
    else h = c->h/60.;
    i = floor(h);
    v = c->v;
    f = h-i;
    m = v*(1.-c->s);
    n = v*(1.-c->s*f);
    k = v*(1.-c->s*(1.-f));
    switch((int)h){
    case 0: c->r = v; c->g = k; c->b = m; break;
    case 1: c->r = n; c->g = v; c->b = m; break;
    case 2: c->r = m; c->g = v; c->b = k; break;
    case 3: c->r = m; c->g = n; c->b = v; break;
    case 4: c->r = k; c->g = m; c->b = v; break;
    case 5: c->r = v; c->g = m; c->b = n; break;
    }
  }
}
