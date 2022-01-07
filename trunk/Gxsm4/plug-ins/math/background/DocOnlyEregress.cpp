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
% PlugInDocuCaption: Plane regression
% PlugInName: (DocOnly) Eregress
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Background/E Regression

% PlugInDescription
The filter calculates a regression plane using a selected rectangular
area and subtracts this from the data

This is usually the best way to flatten a good SPM scan, because it
sustains the offsets inbetween lines. It allows to select a individual
area of the scan as reference area to be flatened.

% PlugInUsage
Mark the reference area using the rectangle object and call \GxsmMenu{Math/Background/E Regression}.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInObjects
A optional rectangle needed to select the reference area for the plane
regression.

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
\GxsmNote{The ''E'' (E Regression) is historically and stands for German 'Ebene', what means plane. }

At this time this filter is not a real PlugIn (in principal it could
be), but it resides in the Gxsm Core located in the file
\GxsmFile{Gxsm/src/xsmmath.C} as a subroutine and cannot be removed,
because the core code is dependent on this subroutine.

%% OptPlugInHints
%Any tips and tricks?

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */
