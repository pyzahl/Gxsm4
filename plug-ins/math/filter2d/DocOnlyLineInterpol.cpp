/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * THIS FILE IS FOR AUTOMATIC DOCUMENTATION ONLY 
 * --> may used later for the correspondig real PlugIn!
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

/* Please do not change the Begin/End lines of this comment section!
 * this is a LaTeX style section used for auto generation of the PlugIn Manual 
 * Chapter. Add a complete PlugIn documentation inbetween the Begin/End marks!
 * All "% PlugInXXX" commentary tags are mandatory
 * All "% OptPlugInXXX" tags are optional
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Line Interpolation
% PlugInName: (DocOnly) LineInterpol
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Filter 2D/Line Interpol

% PlugInDescription
Use this tool to replace distorted line(s) (i.e. caused be manual
Z-Offset adjustments while scanning or tip changes) by interpolated
data from the lines one before and one after the distorsion.

If necessary you should remove line-shifts (use the Lins Shifts
filter) before calling this tool. I.e. the average Z before and after
the distorsion shaould be about the same.

It can work manually on exactly one line or in a automatic mode with
automatic line detection.

% PlugInUsage

It works in two modes: 

a) Assuming line 100 (in pixel corrdinates) to be broken as example,
   mark exactly the line 99 and line 101 with exactly one rectangle
   objects. You may want to use the rectangle-properties to set the
   line numbers manually. Then execute it via \GxsmMenu{Math/Filter
   2D/Line Interpol}.

b) The automatic mode assumes a fairly well flatened image with only
   distorted single lines. Mark the reference area including the
   distorted lines using the rectangle object and call
   \GxsmMenu{Math/Filter 2D/Line Interpol}. It will ask for an
   threashold, which is used to determine if a line is distored,
   therefore it compares the average Z withing the marked X range of
   any line in the mared Y range and the line before.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInObjects

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

%% OptPlugInConfig
%describe the configuration options of your plug in here!

%% OptPlugInRefs
%Any references?

%% OptPlugInKnownBugs
%Are there known bugs? List! How to work around if not fixed?

% OptPlugInNotes
The X position and size of the rectangle does not matter at all for method (a).


%% OptPlugInHints
%Any tips and tricks?

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */
