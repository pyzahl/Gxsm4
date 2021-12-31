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
% PlugInDocuCaption: Line Regression
% PlugInName: (DocOnly) LineRegress
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Background/Line Regress

% PlugInDescription
This applies the \GxsmEmph{Quick View} representation to the
data. Therefore a least squares fit line regression is calculated on a
subset of 30 points for each scan line. This line is subtracted from
the data to correct for slope and offset.

% PlugInUsage
This filter is used for a quick and easy background correction of
sample tilt and possible offset changes inbetween lines.

% OptPlugInSources
The active channel is used as data source.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

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
At this time this filter is not a real PlugIn (in principal it could
be), but it resides in the Gxsm Core located in the file
\GxsmFile{Gxsm/src/xsmmath.C} as a subroutine and cannot be removed,
because the core code is dependent on this subroutine.

% OptPlugInHints
Doing it better: try using a plane regression! If this works, great! 
In case there are offset changes inbetween lines, you can try getting
better results with an proceeding \GxsmMenu{Math/Filter
2D/LineShifts}. If you have some spikes in you image, try removing
those first with the \GxsmMenu{Math/Filter 2D/Despike}.


% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */
