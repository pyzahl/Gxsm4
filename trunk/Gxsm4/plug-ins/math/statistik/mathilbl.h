/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Percy Zahl <zahl@fkp.uni-hannover.de>
 * additional features: Andreas Klust <klust@fkp.uni-hannover.de>
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

/*@
 *@	File:	mathilbl.h
 *@	Datum:	30.11.1995
 *@	Author:	P.Zahl
 *@	Zweck:	Definitionen für math.c
 *@             - Prototypen
 *@
 *@	Funktionen:
 */

#ifndef MATHILBL__H
#define MATHILBL__H


#include "xsmmath.h"

extern gboolean IslandLabl(MATHOPPARAMS);
extern gboolean StepFlaten(MATHOPPARAMS);
extern gboolean KillStepIslands(MATHOPPARAMS);
extern double d_sa;
extern double d_pr;
extern double d_lbn;
extern gchar *filename;
extern gchar *simname;
extern int inter;
extern int save;

#endif // MATHILBL__H
