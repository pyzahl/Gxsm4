/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name:afm_lj_mechanical_sim.C
 * ========================================
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
% PlugInDocuCaption: AFM (NC-AFM) mechanical tip apex/molecule imaging simulations
% PlugInName:afm_lj_mechanical_sim
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Probe/AFM_mechanical_sim

% PlugInDescription
Simulating NC-AFM images and force curves. Based on publication:

PHYSICAL REVIEW B 90, 085421 (2014)
Mechanism of high-resolution STM/AFM imaging with functionalized tips
Prokop Hapala --Institute of Physics, Academy of Sciences of the Czech Republic, v.v.i., Cukrovarnick a 10, 162 00 Prague, Czech Republic;
Georgy Kichin, Christian Wagner, F. Stefan Tautz, and Ruslan Temirov -- Peter Gr\"unberg Institut (PGI-3), Forschungszentrum J\"ulich, 52425 J\"ulich, Germany
and J\"ulich Aachen Research Alliance (JARA), Fundamentals of Future Information Technology, 52425 J\"ulich, Germany;
Pavel Jelinek -- Institute of Physics, Academy of Sciences of the Czech Republic, v.v.i., Cukrovarnick a 10, 162 00 Prague, Czech Republic and Graduate School of Engineering, Osaka University 2-1, Yamada-Oka, Suita, Osaka 565-0871, Japan

High-resolution atomic force microscopy (AFM) and scanning tunneling microscopy (STM) imaging with
functionalized tips is well established, but a detailed understanding of the imaging mechanism is still missing. We
present a numerical STM/AFM model, which takes into account the relaxation of the probe due to the tip-sample
interaction. We demonstrate that the model is able to reproduce very well not only the experimental intra- and
intermolecular contrasts, but also their evolution upon tip approach. At close distances, the simulations unveil a
significant probe particle relaxation towards local minima of the interaction potential. This effect is responsible
for the sharp submolecular resolution observed in AFM/STM experiments. In addition, we demonstrate that sharp
apparent intermolecular bonds should not be interpreted as true hydrogen bonds, in the sense of representing
areas of increased electron density. Instead, they represent the ridge between two minima of the potential energy
landscape due to neighboring atoms.

and

related Supplementary Material: The mechanism of high-resolution STM/AFM imaging with functionalized tips.

% PlugInUsage
Call \GxsmMenu{Math/Probe/AFM mechanical sim}

% OptPlugInSources
The active channel is used as geometry/size/offset template only. Must be of type DOUBLE.

% OptPlugInObjects
Input data file to load as external molecule/structure model.

"model.xyz" type file. 1st line: "N" number of atoms, 2nd line comment/name/info -- ignored.
Then atom 1...N in following lines. Format:

El X Y Z   
 C 0 0 0
 O 2.365 0.213 0.03
Cu 0.1 0.7 -3.5
...   

El=Element Symbol, mut be two characters like " C" " O" "Cu", then X Y Z  coordinates in Angstroem

% OptPlugInDest
The computation result is placed into an existing math channel, else into a new created math channel.

% OptPlugInConfig
The PlugIn configurator...

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

//#define USE_NLOPT // old approach -- slow...

#include <gtk/gtk.h>
#include <glib.h>
#include <math.h>
#include "config.h"
#include "core-source/plugin.h"
#include "core-source/vectorutil.h"

#ifdef USE_NLOPT
#include <nlopt.hpp>
#endif

#include <sstream>
using namespace std;


#define MODE_NO_OPT 0
#define MODE_NL_OPT 1
#define MODE_IPF_NL_OPT 2
#define MODE_TOPO   3
#define MODE_CTIS   4
#define MODE_NONE   5

#define MODE_L_J           0x01
#define MODE_COULOMB       0x02

#define MODE_PROBE_L_J     0x10 // always
#define MODE_PROBE_COULOMB 0x20 // always
#define MODE_PROBE_SPRING  0x40 // option


/* PLUGIN DEFINITION */

#define UTF8_ANGSTROEM "\303\205"

static void cancel_callback (GtkWidget *widget, int *status);
static void afm_lj_mechanical_sim_about( void );
static void afm_lj_mechanical_sim_configuration( void );

static const char *about_text = N_("Gxsm (Raster-) Probe Image Extraction Math-Plugin\n\n"
                                   "Probe Event Data extraction:\n"
				   "A new image is generated from specified probe data.");

#ifdef GXSM3_MAJOR_VERSION // GXSM3

static gboolean afm_lj_mechanical_sim_run(Scan *Dest );

GxsmPlugin afm_lj_mechanical_sim_pi = {
	NULL,
	NULL,
	0,
	NULL,
	"AFM_Mechanical_Sim-M1D-PX",
	NULL,
	NULL,
	"Percy Zahl",
	"math-probe-section",
	N_("AFM Mechanical Simulation"),
	N_("AFM mechanical image/force simulating"),
	"no more info",
	NULL,
	NULL,
	NULL,
	NULL,
	afm_lj_mechanical_sim_about,
	afm_lj_mechanical_sim_configuration,
	NULL,
	NULL
};

GxsmMathNoSrcOneDestPlugin afm_lj_mechanical_sim_m1d_pi = {
	afm_lj_mechanical_sim_run
};
GxsmMathNoSrcOneDestPlugin *get_gxsm_math_no_src_one_dest_plugin_info( void ) { 
	return &afm_lj_mechanical_sim_m1d_pi; 
}

static void afm_lj_mechanical_sim_about( void )
{
	const gchar *authors[] = {afm_lj_mechanical_sim_pi.authors, NULL};
	gtk_show_about_dialog (GTK_WINDOW (gapp->get_app_window ()), 
			       "program-name", afm_lj_mechanical_sim_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

#else // GXSM2

static gboolean afm_lj_mechanical_sim_run(Scan *Src, Scan *Dest );

GxsmPlugin afm_lj_mechanical_sim_pi = {
	NULL,
	NULL,
	0,
	NULL,
	"AFM_Mechanical_Sim-M1D-PX",
	NULL,
	NULL,
	"Percy Zahl",
	"_Math/_Probe/",
	N_("AFM Mechanical Simulation"),
	N_("AFM mechanical image/force simulating"),
	"no more info",
	NULL,
	NULL,
	NULL,
	NULL,
	afm_lj_mechanical_sim_about,
	afm_lj_mechanical_sim_configuration,
	NULL,
	NULL
};

// NOTE: new Gxsm3 math plugin type no src one dest not in gxsm2 available.
// GxsmMathNoSrcOneDestPlugin afm_lj_mechanical_sim_m1d_pi = {
GxsmMathOneSrcPlugin afm_lj_mechanical_sim_m1s_pi = {
	afm_lj_mechanical_sim_run
};

GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) { 
	return &afm_lj_mechanical_sim_m1s_pi; 
}

static void afm_lj_mechanical_sim_about( void )
{
	const gchar *authors[] = {afm_lj_mechanical_sim_pi.authors, NULL};
	gtk_show_about_dialog (NULL, //GTK_WINDOW (gapp->get_app_window ()), 
			       "program-name", afm_lj_mechanical_sim_pi.name,
			       "version", VERSION,
                               //			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}


#endif



GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	afm_lj_mechanical_sim_pi.description = g_strdup_printf(N_("Gxsm MathNoSrcOneDestArg afm_lj_mechanical_sim plugin %s"), VERSION);
	return &afm_lj_mechanical_sim_pi; 
}


static void afm_lj_mechanical_sim_configuration( void ){
#if 0
	afm_lj_mechanical_sim_pi.app->ValueRequest("Enter Number", "Index", 
                                                   "Default Value",
                                                   afm_lj_mechanical_sim_pi.app->xsm->Unity, 
                                                   1., 10., ".0f", &DefaultValue);
#endif
}

/* END PLUGIN CORE DEFINITION CODE */

#define CONST_e    1.60217656535e-19   // Elementarladung [As] = [C]
#define CONST_me   9.10938356e-31  // electron mass [kg]
#define CONST_h    6.62607004e-34  // Planck's constant m^2 kg / s
#define CONST_heVs 4.135667662e-15 // h in eV s
#define CONST_hbar (6.62607004e-34/2./M_PI)  // 1/2pi * Planck's constant m^2 kg / s
#define CONST_ke   8.9875517873681764e9 // Coulomb force constant  ke [ kg·m^3 / (s^2 C^2) ]
#define CONST_ke_ee ( CONST_ke*1e20*1e12*CONST_e*CONST_e ) // ke for C in e, r in Ang ==> pN

/*
 * GXSM SCAN/MEM2d Layer assignments for output data matrix, layered, with time elements for Z
 */
typedef enum {
        FREQ_SHIFT_L = 0,
        FREQ_SHIFT_FIXED_L,
        APEX_FZ_L,
        APEX_F_LJ_L,
        APEX_F_COULOMB_L,
        APEX_FZ_FIXED_L,
        APEX_F_LJ_FIXED_L,
        APEX_F_COULOMB_FIXED_L,
        PROBE_FNORM_L,
        PROBE_CTIS_L,
        PROBE_TOPO_L,
        PROBE_X_L,
        PROBE_Y_L,
        PROBE_Z_L,
        PROBE_FX_FIELD_L,
        PROBE_FY_FIELD_L,
        PROBE_FZ_FIELD_L,
        PROBE_FCX_FIELD_L,
        PROBE_FCY_FIELD_L,
        PROBE_FCZ_FIELD_L,
        PROBE_FFX_FIELD_L,
        PROBE_FFY_FIELD_L,
        PROBE_FFZ_FIELD_L,
        PROBE_FLX_FIELD_L,
        PROBE_FLY_FIELD_L,
        PROBE_FLZ_FIELD_L,
#ifdef PROBE_GRAD_INTER
        PROBE_dxFX_L,
        PROBE_dxFY_L,
        PROBE_dxFZ_L,
        PROBE_dyFX_L,
        PROBE_dyFY_L,
        PROBE_dyFZ_L,
        PROBE_dzFX_L,
        PROBE_dzFY_L,
        PROBE_dzFZ_L,
        PROBE_dxFCX_L,
        PROBE_dxFCY_L,
        PROBE_dxFCZ_L,
        PROBE_dyFCX_L,
        PROBE_dyFCY_L,
        PROBE_dyFCZ_L,
        PROBE_dzFCX_L,
        PROBE_dzFCY_L,
        PROBE_dzFCZ_L,
#endif
        PROBE_FX_L,
        PROBE_FY_L,
        PROBE_FZ_L,
        PROBE_I_L,
        PROBE_NLOPT_ITER_L,
        N_LAYERS
} LAYER_ASSIGNMENT_SIMULATION;




typedef struct {
        int    N; // N -- Periodic Table Ordnungszahl
        const gchar* name; // Element name
        double LJp[2];  // L-J Parameters { eps_a, r_a  } epsilon[meV], radius [A]
        double Phi; // Workfunction [eV]
        double certainty; // certainty about parameters
        const char* comment; // comment on data source
} Element;

// I(d) = C  e V exp (-2 d sqrt (2me Phi) / hbar )
// I = 4e pi / hbar Int dk[  f(Ef-eV+k) - f(Ef+k) ] R_{surface}(Ef-eV+k) R_{probe}(Ef+k) |M|^2 ]
// M = ...
/*
In case you use the Probe Particle Model for your research, consider also citing this papers: 

Prokop Hapala, Georgy Kichin, Christian Wagner, F. Stefan Tautz, Ruslan Temirov, and Pavel Jelínek, Mechanism of high-resolution STM/AFM imaging with functionalized tips, Phys. Rev. B 90, 085421 – Published 19 August 2014 

Prokop Hapala, Ruslan Temirov, F. Stefan Tautz, and Pavel Jelínek, Origin of High-Resolution IETS-STM Images of Organic Molecules with Functionalized Tips, Phys. Rev. Lett. 113, 226101 – Published 25 November 2014

Columns: equlibrium distance Rii0[Å], binding energy Eii0[eV], comment (Z, element, ref. of L-J parameters)
Note: Number of row = proton number of the element!!
1.4870	0.0006808054	1	H	abalone	AMBER	2003	OPLS
1.4815	0.0009453220	2	He	abalone	AMBER	2003	OPLS
2.0000	0.0100000000	3	Li	abalone	AMBER	2003	OPLS
2.0000	0.0100000000	4	Be				
2.0800	0.0037292520	5	B	abalone	AMBER	2003	OPLS
1.9080	0.0037292524	6	C	abalone	AMBER	2003	OPLS
1.7800	0.0073719000	7	N	abalone	AMBER	2003	OPLS
1.6612	0.0091063140	8	O	abalone	AMBER	2003	OPLS
1.7500	0.0026451670	9	F	abalone	AMBER	2003	OPLS
1.5435	0.0036425260	10	Ne	abalone	AMBER	2003	OPLS
2.0000	0.0100000000	11	Na	abalone	AMBER	2003	OPLS
2.0000	0.0100000000	12	Mg	abalone	AMBER	2003	OPLS
2.0000	0.0100000000	13	Al				
1.9000	0.0254899514	14	Si	Gromos	Si	J.	Chem.
2.1000	0.0086726800	15	P	abalone	AMBER	2003	OPLS
2.0000	0.0108408500	16	S	abalone	AMBER	2003	OPLS
1.9480	0.0114913010	17	Cl	abalone	AMBER	2003	OPLS
1.8805	0.0123412240	18	Ar	abalone	AMBER	2003	OPLS
2.0000	0.0100000000	19	K	abalone	AMBER	2003	OPLS
2.0000	0.0100000000	20	Ca	abalone	AMBER	2003	OPLS
2.0000	0.0100000000	21	Sc				
2.0000	0.0100000000	22	Ti				
2.0000	0.0100000000	23	V				
2.0000	0.0100000000	24	Cr				
2.0000	0.0100000000	25	Mn				
2.0000	0.0100000000	26	Fe				
2.0000	0.0100000000	27	Co				
2.0000	0.0100000000	28	Ni				
2.0000	0.0100000000	29	Cu				
2.0000	0.0100000000	30	Zn				
2.0000	0.0100000000	31	Ga				
2.0000	0.0100000000	32	Ge				
2.0000	0.0100000000	33	As				
2.0000	0.0100000000	34	Se				
2.2200	0.0138762880	35	Br				
2.0000	0.0100000000	36	Kr				
2.0000	0.0100000000	37	Rb				
2.0000	0.0100000000	38	Sr				
2.0000	0.0100000000	39	Y				
2.0000	0.0100000000	40	Zr				
2.0000	0.0100000000	41	Nb				
2.0000	0.0100000000	42	Mo				
2.0000	0.0100000000	43	Tc				
2.0000	0.0100000000	44	Ru				
2.0000	0.0100000000	45	Rh				
2.0000	0.0100000000	46	Pd				
2.0000	0.0100000000	47	Ag				
2.0000	0.0100000000	48	Cd				
2.0000	0.0100000000	49	In				
2.0000	0.0100000000	50	Sn				
2.0000	0.0100000000	51	Sb				
2.0000	0.0100000000	52	Te				
2.3500	0.0173453600	53	I				
2.1815	0.0243442128	54	Xe				
2.0000	0.0100000000	55	Cs				
2.0000	0.0100000000	56	Ba				
2.0000	0.0100000000	57	La				
2.0000	0.0100000000	58	Ce				
2.0000	0.0100000000	59	Pr				
2.0000	0.0100000000	60	Nd				
2.0000	0.0100000000	61	Pm				
2.0000	0.0100000000	62	Sm				
2.0000	0.0100000000	63	Eu				
2.0000	0.0100000000	64	Gd				
2.0000	0.0100000000	65	Tb				
2.0000	0.0100000000	66	Dy				
2.0000	0.0100000000	67	Ho				
2.0000	0.0100000000	68	Er				
2.0000	0.0100000000	69	Tm				
2.0000	0.0100000000	70	Yb				
2.0000	0.0100000000	71	Lu				
2.0000	0.0100000000	72	Hf				
2.0000	0.0100000000	73	Ta				
2.0000	0.0100000000	74	W				
2.0000	0.0100000000	75	Re				
2.0000	0.0100000000	76	Os				
2.0000	0.0100000000	77	Ir				
2.0000	0.0100000000	78	Pt				
2.0000	0.0100000000	79	Au				
2.0000	0.0100000000	80	Hg				
2.0000	0.0100000000	81	Tl				
2.0000	0.0100000000	82	Pb				
2.0000	0.0100000000	83	Bi				
2.0000	0.0100000000	84	Po				
2.0000	0.0100000000	85	At				
2.0000	0.0100000000	86	Rn				
2.0000	0.0100000000	87	Fr				
2.0000	0.0100000000	88	Ra				
2.0000	0.0100000000	89	Ac				
2.0000	0.0100000000	90	Th				
2.0000	0.0100000000	91	Pa				
2.0000	0.0100000000	92	U				
2.0000	0.0100000000	93	Np				
2.0000	0.0100000000	94	Pu				
2.0000	0.0100000000	95	Am				
2.0000	0.0100000000	96	Cm				
2.0000	0.0100000000	97	Bk				
2.0000	0.0100000000	98	Cf				
2.0000	0.0100000000	99	Es				
2.0000	0.0100000000	100	Fm				
2.0000	0.0100000000	101	Md				
2.0000	0.0100000000	102	No				
2.0000	0.0100000000	103	Lr				
2.0000	0.0100000000	104	Rf				
2.0000	0.0100000000	105	Db				
2.0000	0.0100000000	106	Sg				
2.0000	0.0100000000	107	Bh				
2.0000	0.0100000000	108	Hs				
2.0000	0.0100000000	109	Mt				
2.0000	0.0100000000	110	Ds				
2.0000	0.0100000000	111	Rg				
2.0000	0.0100000000	112	Cn				
2.0000	0.0100000000	113	Uut				
2.0000	0.0100000000	114	Fl				
2.0000	0.0100000000	115	Uup				
2.0000	0.0100000000	116	Lv				
2.0000	0.0100000000	117	Uus				
2.0000	0.0100000000	118	Uuo			




 */
// incomplete table:
// Z (#Proton), Element Name, L-J parameters: E0 [meV], r0 [Ang] 
Element Elements[] {
        {   0, "Apex",{ 1000.0    ,   2.0000  }, 1. , 1.0,	"arbitrary rigid tip apex, paper *" }, 
        {   1,  "H",  {  0.6808054,   1.4870  }, 1. , 1.0,	"paper / PP-Hapala *" }, 
        {   2, "He",  {  0.9453220,   1.4815  }, 1. , 1.0,	"PP-Hapala" }, 
	{   3, "Li",  { 10.0      ,   2.000   }, 2.9 , 0.0,	"PP-Hapala" },  
        {   4, "Be",  { 10.0      ,   2.000   }, 0. ,  0.0,	"dummy L-J values" },  
        {   5,  "B",  {  3.7292520,   2.080   }, 0. , 1.0,	"PP-Hapala" },   
        {   6,  "C",  {  3.7292524,   1.9080  }, 4.81 , 1.0,	"paper *" }, 
        {   7,  "N",  {  7.3719   ,   1.7800  }, 1. , 1.0,	"PP-Hapala" },  
        {   8,  "O",  {  9.106314 ,   1.6612  }, 1. , 1.0,	"paper *" },  
        {   9,  "F",  {  2.6451670,   1.75    }, 0. , 1.0,	"PP-Hapala" },  
        {  10, "Ne",  {  3.642526 ,   1.5435  }, 0. , 1.0,	"Ascroft M" },  
        {  11, "Na",  { 10.00     ,   2.000   }, 0. , 0.0,	"L-J values estimated" },  
        {  12, "Mg",  { 10.00     ,   2.000   }, 0. , 0.0,	"L-J values estimated" },  
        {  13, "Al",  { 10.00     ,   2.100   }, 4.28 , 0.0,	"L-J values estimated" },  
        {  14, "Si",  { 25.4899514,   1.9     }, 0. , 0.5, "L-J values estimated" },  
        {  15,  "P",  {  8.67268  ,   2.1     }, 0. , 1.0,	"L-J values estimated" },  
        {  16,  "S",  { 10.84085  ,   2.000   }, 0. , 1.0,	"L-J values estimated" },  
        {  17, "Cl",  { 11.491301 ,   1.948   }, 0. , 1.0,	"L-J values estimated" },  
        {  18, "Ar",  { 12.341224 ,   1.8805  }, 0. , 1.0,	"Ascroft M" },  
        {  19,  "K",  { 10.00     ,   2.0     }, 0. , 0.0,	"L-J values estimated" },  
        {  20, "Ca",  { 10.00     ,   2.0     }, 0. , 0.0,	"L-J values estimated" },  
        {  21, "Sc",  { 10.00     ,   2.0     }, 0. , 0.0,	"L-J values estimated" },  
        {  22, "Ti",  { 10.00     ,   2.100  }, 4.6  , 0.0,	"L-J values estimated" },  
        {  23, "V",   { 10.00     ,   2.100  }, 4.6  , 0.0,	"L-J values estimated" },  
        {  24, "Cr",  { 10.00     ,   2.100  }, 4.6  , 0.0,	"L-J values estimated" },  
        {  25, "Mn",  { 10.00     ,   2.100  }, 4.6  , 0.0,	"L-J values estimated" },  
        {  26, "Fe",  { 10.00     ,   2.100  }, 4.6  , 0.0,	"L-J values estimated" },  
        {  27, "Co",  { 10.00     ,   2.100  }, 4.6  , 0.0,	"L-J values estimated" },  
        {  28, "Ni",  { 10.00     ,   2.100  }, 4.65 , 0.0,	"L-J values estimated" },  
        {  29, "Cu",  { 10.00     ,   2.100  }, 4.65 , 0.0,	"L-J values estimated" },  
        {  30, "Sn",  { 15.50     ,   2.100  }, 4.42 , 0.0,	"L-J values estimated" },  
        {  31, "Ga",  { 15.50     ,   2.100  }, 4.42 , 0.0,	"L-J values estimated" },  
        {  32, "Ge",  { 25.50     ,   2.1    }, 4.65 , 0.0,	"L-J values estimated" },  
        {  33, "As",  { 10.00     ,   2.1    }, 0. , 0.0,	"L-J values estimated" },    
        {  34, "Se",  { 10.00     ,   2.1    }, 0. , 0.0,	"L-J values estimated" },    
        {  35, "Br",  { 13.8762880,   2.2200 }, 4.00 , 1.0,	"PP-Hapala" },  
        {  36, "Kr",  { 10.00     , 2.0    }, 0. , 0.0,		"L-J values estimated" }, 
        {  37, "Rb",  { 10.00     , 2.0    }, 0. , 0.0,		"L-J values estimated" }, 
        {  38, "Sr",  { 10.00     , 2.0    }, 0. , 0.0,		"L-J values estimated" }, 
        {  39,  "Y",  { 10.00     , 2.0    }, 0. , 0.0,		"L-J values estimated" }, 
        {  40, "Zr",  { 10.00     , 2.0    }, 0. , 0.0,		"L-J values estimated" }, 
        {  41, "Nb",  { 10.00     , 2.0    }, 0. , 0.0,		"L-J values estimated" }, 
        {  42, "Mo",  { 15.50    , 2.100  }, 4.37 , 1.0,	"L-J values estimated **" },  
        {  43, "Tc",  { 10.00     , 2.0    }, 0. , 0.0,		"L-J values estimated" }, 
        {  44, "Ru",  { 10.00     , 2.0    }, 0. , 0.0,		"L-J values estimated" }, 
        {  45, "Rh",  { 10.00     , 2.0    }, 0. , 0.0,		"L-J values estimated" }, 
        {  46, "Pd",  { 10.00     , 2.0    }, 0. , 0.0,		"L-J values estimated" }, 
        {  47, "Ag",  { 15.50     , 2.100  }, 4.26 , 1.0,	"L-J values estimated **" },  
        {  48, "Cd",  { 10.00     , 2.0    }, 0. , 0.0,		"L-J values estimated" }, 
        {  49, "In",  { 10.00     , 2.0    }, 0. , 0.0,		"L-J values estimated" }, 
        {  50, "Sn",  { 10.00     , 2.0    }, 0. , 0.0, 	"L-J values estimated" }, 
        {  51, "Sb",  { 10.00     , 2.0    }, 0. , 0.0, 	"L-J values estimated" }, 
        {  52, "Te",  { 10.00     , 2.0    }, 0. , 0.0, 	"L-J values estimated" }, 
        {  53, "I",   { 17.345360 , 2.3500 }, 0. , 1.0, 	"PP-Hapala" }, 
        {  54, "Xe",  { 24.3442128, 2.1815 }, 1. , 1.0, 	"paper *" },    
        {  55, "Cs",  { 15.50,    2.100  }, 2.14 , 1.0, 	"L-J values estimated" },  
	{  56, "Ba",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  57, "La",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  58, "Ce",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  59, "Pr",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  60, "Nd",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  61, "Pm",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  62, "Sm",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  63, "Eu",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  64, "Gd",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  65, "Tb",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  66, "Dy",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  67, "Ho",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  68, "Er",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  69, "Tm",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  70, "Yb",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  71, "Lu",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  72, "Hf",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  73, "Ta",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
        {  74,  "W",  { 25.50 ,    2.100 }, 4.5  , 0.8, "L-J values estimated" },  
	{  75, "Re",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  76, "Os",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  77, "Ir",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
        {  78, "Pt",  { 25.50 ,    2.100 }, 4.5  , 0.5, "L-J values estimated" },  
        {  79, "Au",  { 25.50 ,    2.100 }, 5.1  , 0.5, "L-J values estimated" },  
	{  80, "Hg",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  81, "Tl",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
        {  82, "Pb",  { 25.50 ,    2.100 }, 4.25 , 0.5, "L-J values estimated" },  
	{  83, "Bi",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  84, "Po",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  85, "At",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  86, "Rn",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  87, "Fr",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  88, "Ra",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  89, "Ac",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  90, "Th",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  91, "Pa",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  92,  "U",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  93, "Np",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  94, "Pu",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  95, "Am",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  96, "Cm",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  97, "Bk",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  98, "Cf",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{  99, "Es",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 100, "Fm",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 101, "Md",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 102, "No",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 103, "Lr",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 104, "Rf",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 105, "Db",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 106, "Sg",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 107, "Bh",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 108, "Hs",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 109, "Mt",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 110, "Ds",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 111, "Rg",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 112, "Cn",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 113, "Uut", { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 114, "Fl",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 115, "Uup", { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 116, "Lv",  { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 117, "Uus", { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 118, "Uuo", { 10.00,    2.0    }, 0. , 0.0, "guessed dummy values" }, 
	{ 119, "UuX", { 10.00,    2.0    }, 0. , 0.0, "DUMMY PLACE HOLDER VALUES" }, 
        {  -1, "END.",{ -1.00 ,   -1.00  }, 0. , 1.0, "END MARK" }  // END MARK
};

#define GLOBAL_R_CUTOFF 4.0

typedef struct{
        double xyzc[4]; // coordinates XYZ + partial Charge [Ang,Ang,Ang, C[e]]
        int    N;      // atom id / ordnungszahl -- match perdiodic table above
} Model_item;

class xyzc_model{
public:
        // emty model
        xyzc_model (int num_atoms){
                size = num_atoms;
                probe = NULL;
                model = new Model_item[num_atoms+1];
                for (int pos=0; pos<size+1; ++pos){
                        model[pos].xyzc[0] = 0.;
                        model[pos].xyzc[1] = 0.;
                        model[pos].xyzc[2] = 0.;
                        model[pos].xyzc[3] = 0.;
                        model[pos].N = -1;
                }
                make_probe ();
        };
        
        xyzc_model (Model_item *m){
                for (size = 0; m[size].N > 0; ++size);
                probe = NULL;
                model = new Model_item[size+1];
                for (int pos=0; pos<size; ++pos){
                        copy_vec4 (model[pos].xyzc, m[pos].xyzc);
                        model[pos].N = m[pos].N;
                }
                mark_end (size);
                make_probe ();
        };
        xyzc_model (xyzc_model *m, Scan *bb_ref_scan=NULL){
                double bb[2][2] = {{-1000.,-1000.},{1000.,1000.}};
                int p=0;
                int model_size=0;
                for (model_size = 0; m->model[model_size].N > 0; ++model_size);
                probe = NULL;
                if (bb_ref_scan){ // Bounding box
                        bb_ref_scan->Pixel2World (0, bb_ref_scan->mem2d->GetNy()-1, bb[0][0], bb[0][1]);
                        bb_ref_scan->Pixel2World (bb_ref_scan->mem2d->GetNx()-1, 0, bb[1][0], bb[1][1]);
                        g_message ("BB=[ (%g,%g) :: (%g,%g)]", bb[0][0], bb[0][1], bb[1][0], bb[1][1]);
                }
                size = 0;
                for (int pos=0; pos<model_size; ++pos){
                        if (bb_ref_scan){ // Bounding box
                                if (m->model[pos].xyzc[0] < bb[0][0]-GLOBAL_R_CUTOFF) { g_message ("out L"); continue; }
                                if (m->model[pos].xyzc[0] > bb[1][0]+GLOBAL_R_CUTOFF) { g_message ("out R"); continue; }
                                if (m->model[pos].xyzc[1] > bb[1][1]+GLOBAL_R_CUTOFF) { g_message ("out T");continue; }
                                if (m->model[pos].xyzc[1] < bb[0][1]-GLOBAL_R_CUTOFF) { g_message ("out B");continue; }
                        }
                        ++size; // count only
                }
                g_message ("BB scan model: model %d atoms, in BB %d atoms", model_size, size);
                model = new Model_item[size+1];
                p=0;
                for (int pos=0; pos<model_size; ++pos){
                        if (bb_ref_scan){ // Bounding box
                                if (m->model[pos].xyzc[0] < bb[0][0]-GLOBAL_R_CUTOFF) continue;
                                if (m->model[pos].xyzc[0] > bb[1][0]+GLOBAL_R_CUTOFF) continue;
                                if (m->model[pos].xyzc[1] > bb[1][1]+GLOBAL_R_CUTOFF) continue;
                                if (m->model[pos].xyzc[1] < bb[0][1]-GLOBAL_R_CUTOFF) continue;
                        }
                        copy_vec4 (model[p].xyzc, m->model[pos].xyzc);
                        model[p].N = m->model[pos].N;
                        ++p;
                }
                mark_end (p);
                make_probe ();
        };

        // read from xyz file
        xyzc_model (gchararray xyzc_file, int option='c'){ // c: auto_center
                std::ifstream f;
                Model_item *atom_entry;
                gchar xyzc_line[1024];
                               
                size = 0;
                probe = NULL;
                model = NULL;

                make_probe ();

                // xyzc file:
                //1137
                //Cu 3ML 4x4 + TMA1                       
                //Cu  -10.252485  -17.757826    0.000000  [charge, optional]
                //Cu  -15.378728   -8.878913    0.000000   0.1
                // O  -15.378728   -8.878913    3.200000  -0.1
                //...

                f.open(xyzc_file, std::ios::in);
                if(!f.good()){
                        return;
                }
                
                // read header
                f.getline(xyzc_line, 1024); // num atoms
                int num_atoms = atoi(xyzc_line);
                if (num_atoms > 1){
                        size = num_atoms;
                        model = new Model_item[num_atoms+1];
                        for (int pos=0; pos<size+1; ++pos){
                                model[pos].xyzc[0] = 0.;
                                model[pos].xyzc[1] = 0.;
                                model[pos].xyzc[2] = 0.;
                                model[pos].xyzc[3] = 0.;
                                model[pos].N = -1;
                        }

                        f.getline(xyzc_line, 1024); // comment, discard

                        int count=0;
                        while (f.good()){
                                f.getline(xyzc_line, 1024);
                        
                                if (xyzc_line[0] == ' ')
                                        xyzc_line[0] = '_';

                                //std::cout << xyzc_line << std::endl;
        
                                gchar **record = g_strsplit_set(xyzc_line, " \t", -1);
                                gchar **token = record;
                        
                                atom_entry = new Model_item;
                                atom_entry->N=0;
                                atom_entry->xyzc[0] = 0.; // default: X=0
                                atom_entry->xyzc[1] = 0.; // default: Y=0
                                atom_entry->xyzc[2] = 0.; // default: Z=0
                                atom_entry->xyzc[3] = 0.; // default: C=0 -- no charge unless specified 
                                for (int j=0; *token; ++token){
                                        if (j==0){
                                                if (*token[0] == '_')
                                                        *token[0] = ' ';
                                        
                                                // find Element
                                                //std::cout << "Searching for " << *token << std::endl;
                                                int eln=-1;
                                                for (int n=0; Elements[n].N >= 0; ++n){
                                                        // std::cout << "comparing N=" << n << ": >" << Elements[n].name << "< with >" << *token << "<" << std::endl;
                                                        if (! strcmp (g_strstrip(*token), Elements[n].name)){
                                                                atom_entry->N=Elements[n].N;
                                                                eln=n;
                                                                break;
                                                        }
                                                }
                                        
                                                if (atom_entry->N == 0 || eln < 1){
                                                        g_warning ("XYZ File/Model Contains Unkown L-J Parameters for Element [%s]. Using UuX DUMMY entry.", *token);
                                                        atom_entry->N = 119; // dummy Element
                                                        continue; // try to continue
                                                } else if (Elements[eln].certainty < 0.999){
                                                        g_warning ("XYZ File/Model Contains Element with uncertain (%g) L-J Parameters: [%s] => %s, LJ: Eo=%g meV, ro=%g Ang (%s).",
                                                                   Elements[eln].certainty, *token,
                                                                   Elements[eln].name, Elements[eln].LJp[0], Elements[eln].LJp[1], Elements[eln].comment
                                                                   );
                                                        continue; // try to continue
                                                }
                                                //std::cout << "N=" << atom_entry->N << std::endl;
                                                ++j;
                                                continue;
                                        }
                                        if (strlen(*token) > 0){
                                                if (j<5)
                                                        atom_entry->xyzc[j-1] = atof (*token);
                                                ++j;
                                        }
                                }
                                if (count == 0){
                                        copy_vec (xyz_max, atom_entry->xyzc);
                                        copy_vec (xyz_min, atom_entry->xyzc);
                                }  else {
                                        max_vec_elements (xyz_max, xyz_max, atom_entry->xyzc);
                                        min_vec_elements (xyz_min, xyz_min, atom_entry->xyzc);
                                }
                                //std::cout << std::endl;
                                if (atom_entry->N > 0)
                                        put_atom (atom_entry, count++);
                                g_strfreev (record);
                        }
                }
                f.close ();
                middle_vec_elements (xyz_center_top, xyz_min, xyz_max);
                xyz_center_top[2] = xyz_max[2] + 1.0; // move top mosrt atom to Z=-1A
                if (option == 'c')
                        for (int i=0; i<size && model[i].N > 0; ++i)
                                sub_from_vec (model[i].xyzc, xyz_center_top);
        };

        // custom model filter function
        xyzc_model (xyzc_model *source, gchararray filter, gchararray filter_param = NULL, Scan *scan_area=NULL){
                int src_size = source->get_size ();
                probe = NULL;
                make_probe ();

                Model_item atom;
                
                if (!strncmp (filter, "None", 4)){ // copy only
                        size = src_size;
                        model = new Model_item[size+1];
                        for (int pos=0; pos<size+1; ++pos){
                                model[pos].xyzc[0] = 0.;
                                model[pos].xyzc[1] = 0.;
                                model[pos].xyzc[2] = 0.;
                                model[pos].xyzc[3] = 0.;
                                model[pos].N = -1;
                        }
                        for (int i=0; i<src_size && source->model[i].N > 0; ++i){
                                copy_vec4 (atom.xyzc, source->model[i].xyzc);
                                atom.N = source->model[i].N;
                                put_atom(&atom, i);
                        }
                        return;
                        
                } else if ( !strncmp (filter, "TMA-C+TMA-R-flip", 16)  || !strncmp (filter, "TMA-Rosette", 11) ){
                        int j=0;
                        double xy[3];
                        double xyr[3];
                        double z0[3] = { 0., 0., 4.167425 }; // top Cu -- valid for Mark's cu3ml10x8tma2s1vd.contA.xyz 
                        //double mcenter[3];
                        //double m1[3] = {  1.28097400, 0.73657400, 0. }; // TMA-R 10x10bb
                        //double m2[3] = { -8.97151180, 0.73657400, 0. }; // TMA-L 10x10bb -> flip x&y
                        //double m3[3] = {-14.097754,   9.615486,   0. }; // TMA-Lup60
                        double m1[3] = { 0.5*(-6.133162-3.333204), 0.5*(0.078078+0.029748), 0. }; // TMA-L 10x10bb -> 0,0 at origin
                        double m2[3] = { 0.5*( 3.455264+6.255669), 0.5*(0.148316+0.100187), 0. }; // TMA-R 10x10bb flipped -> 4r,0 pos
                        //                        double m3[3] = { -9.387754,  8.878912,   0. }; // TMA-Lup60
                        //double m4[3] = { 10.2245,  0.,   0. }; // TMA-R pos
                        //double ma[3], mb[3], mc[3], m4[3], m5[3], m6[3], tmp[3];
                        // Cu  -14.097754    9.615486    4.137566
                        // Cu   -3.845269    9.615486    4.137566
                        // Cu a0=3.6149 r=2.5561  4r=10.2245
                        double e_cu[3][3] = { { 2.5561, 0., 0. }, { 0., 0., 0. }, { 0.,0.,2.5561}};
                        double no_flip[3] ={  1.,  1.,  1. };
                        double flip_xy[3] = { -1., -1.,  1. };
                        double flip_y[3]  = {  1., -1.,  1. };
                        double flip_x[3]  = { -1.,  1.,  1. };
                        double r=6.;
                        double rCu=14.;
                        int include_Cu=0;
                        int rosette=0;
                        double zero[3] = {0.,0.,0.};
                        double tma_nL[6][3] = {
                                { 0.0,  0.0,  0.0},
                                { 7.5,  3.0,  0.0},
                                { 4.5,  7.5,  0.0},
                                {12.0, 10.5,  0.0},
                                { 9.0, 15.0,  0.0},
                                { 1.5, 12.0,  0.0}
                        };
                        double tma_nR[6][3] = {
                                {-4.0, -3.5,  0.0},
                                { 3.5, -0.5,  0.0},
                                { 8.0,  7.0,  0.0},
                                { 0.5,  4.0,  0.0},
                                { 5.0, 11.5,  0.0},
                                {12.5, 14.5,  0.0}
                        };
                        double tma_L[5][3], tma_R[5][3];
                        double rotM_L[3][3];
                        double rotM_R[3][3];
                        make_mat_rot_xy_tr (rotM_L, -6.6, zero);
                        make_mat_rot_xy_tr (rotM_R, -6.6, zero);
                        
                        make_mat_rot_xy_tr (TrMat, 120., zero);
                        mul_mat_vec (e_cu[1], TrMat, e_cu[0]);

                        g_print_mat ("e_Cu", e_cu);
                        
                        for (int k=0; k<6; ++k){
                                mul_mattr_vec (tma_L[k], e_cu, tma_nL[k]);
                                g_print_vec ("tma_L", tma_L[k]);
                                mul_mattr_vec (tma_R[k], e_cu, tma_nR[k]);
                                g_print_vec ("tma_R", tma_R[k]);
                        }
                        
                        size = 30*30+200+16*21; // just enough, clean up later
                        if (!strncmp (filter, "TMA-C+TMA-R-flip+Cu111", 22)){
                                g_message ("include Cu111=Yes");
                                include_Cu = 1;
                        }
                        
                        if (!strncmp (filter, "TMA-Rosette+Cu111", 17)){
                                g_message ("include Cu111=Yes");
                                include_Cu = 2;
                        }
                        
                        if (!strncmp (filter, "TMA-Rosette", 11)){
                                g_message ("Rosette=Yes");
                                rosette = 1;
                        }

                        double phi=0.;
                        double tr[3]={0.,0.,0.};
                        if (filter_param){
                                gchar *c=filter_param;
                                for (; *c; ++c){
                                        switch (*c){
                                        case 'R': ++c; while (*c && (*c ==' ' || *c=='=')) ++c; phi = atof (c); break;
                                        case 'X': ++c; while (*c && (*c ==' ' || *c=='=')) ++c; tr[0] = atof (c+1); break;
                                        case 'Y': ++c; while (*c && (*c ==' ' || *c=='=')) ++c; tr[1] = atof (c+1); break;
                                        case 'Z': ++c; while (*c && (*c ==' ' || *c=='=')) ++c; tr[2] = atof (c+1); break;
                                        case 'A': ++c; while (*c && (*c ==' ' || *c=='=')) ++c;
                                                {
                                                        double alpha = atof (c+1);
                                                        make_mat_rot_xy_tr (rotM_L, alpha, zero);
                                                        make_mat_rot_xy_tr (rotM_R, alpha, zero);
                                                }
                                                break;
                                        case 'B': ++c; while (*c && (*c ==' ' || *c=='=')) ++c;
                                                {
                                                        double alpha = atof (c+1);
                                                        make_mat_rot_xy_tr (rotM_R, alpha, zero);
                                                }
                                                break;
                                        default: break;
                                        }
                                }
                                g_message ("Rosette Transformation: R=%g deg Tr=(%g, %g, %g)", phi, tr[0], tr[1], tr[2]);
                        }
                        make_mat_rot_xy_tr (TrMat, phi, tr);
                        g_print_mat ("TrMat", TrMat);
                        
                        model = new Model_item[size+1];
                        for (int pos=0; pos<size+1; ++pos){
                                model[pos].xyzc[0] = 0.;
                                model[pos].xyzc[1] = 0.;
                                model[pos].xyzc[2] = 0.;
                                model[pos].xyzc[3] = 0.;
                                model[pos].N = -1;
                        }
                        // fit all: 55x55A, 550px, O=7,16

                        if (include_Cu){
                                g_message("Building Cu111 Lattice");
                                atom.N = 29; // Cu
                                for (int l=-5; l<=17; ++l)
                                        for (int m=-5; m<=17; ++m){
                                                double lm[3] = { (double)l, (double)m, 1. };
                                                mul_mattr_vec (xy, e_cu, lm);
                                                copy_vec (atom.xyzc, xy);
                                                atom.xyzc[2]=0.; // Z=0
                                                put_atom(&atom, j++);
                                        }
                        }
                        if (rosette){ // add a few CO
                                g_message("Adding CO");
                                double lm[][3] = {
                                        { -5., -3., 1. },
                                        {  0., -3,  1. },
                                        {  1., -3., 1. },
                                        {  2., -3., 1. },
                                        {  3., -3., 1. },
                                        {  4.,  4., 1. },
                                        { 15., 12., 1. },
                                        { 16., 12., 1. },
                                        { 17., 12., 1. },
                                        { 15., 13., 1. },
                                        { 15., 14., 1. },
                                        { 0.,0.,-1.}
                                };
                                //tma_L[2];
                                for (int m=0; lm[m][2]>0.; ++m){
                                        mul_mattr_vec (xy, e_cu, lm[m]);
                                        copy_vec (atom.xyzc, xy);
                                        atom.N = 6; // C
                                        atom.xyzc[2]=2.004; // Z=2.004
                                        put_atom(&atom, j++);
                                        atom.N = 8; // O
                                        //atom.xyzc[2]+=1.7846; // Z=3.7886 (LJ r C,O average)
                                        atom.xyzc[2]+=1.128; // assuming CO bond length 112.8pm
                                        put_atom(&atom, j++);
                                }
                        }

                        for (int i=0; i<src_size && source->model[i].N > 0; ++i){
                                // Cu 3 ML Dimer: ~0/2/4 (Cu) ~7.4 (TMA)
                                if (source->model[i].xyzc[2] > 12.)
                                        continue;
                                if (source->model[i].xyzc[2] < 3.)
                                        continue;
                                
                                if (0){
                                        copy_vec4 (atom.xyzc, source->model[i].xyzc);
                                        atom.N = source->model[i].N;
                                        sub_from_vec (atom.xyzc, z0); // shift 1st Cu ML to Z=0
                                        if (atom.xyzc[2] < 1. && atom.xyzc[2] > -0.5){ // Cu 1st layer only
                                                g_message("Cu from xyz");
                                                put_atom(&atom, j++);
                                        }
                                        // make Cu lattice, Z=0
                                }
                                
                                copy_vec4 (atom.xyzc, source->model[i].xyzc);
                                atom.N = source->model[i].N;
                                sub_from_vec (atom.xyzc, z0);

                                copy_vec (xy, atom.xyzc);
                                sub_from_vec (xy, m1);
                                if (norm_vec(xy) < r && atom.xyzc[2] > 1.){ // TMA-L 
                                        if (rosette){ // translate+flip to build rosette
                                                for (int k=0; k<6; ++k){
                                                        mul_mat_vec (xyr, rotM_L, xy);
                                                        copy_vec (atom.xyzc, xyr);
                                                        if (0)
                                                                mul_vec_vec (atom.xyzc, flip_xy);
                                                        add_to_vec (atom.xyzc, tma_L[k]);
                                                        put_atom(&atom, j++, 1);
                                                }
                                        } else {
                                                put_atom(&atom, j++, 1);
                                        }
                                }
                                
                                copy_vec4 (atom.xyzc, source->model[i].xyzc);
                                atom.N = source->model[i].N;
                                sub_from_vec (atom.xyzc, z0);

                                copy_vec (xy, atom.xyzc);
                                sub_from_vec (xy, m2);
                                if (norm_vec(xy) < r && atom.xyzc[2] > 1.){ // TMA-R
                                        if (rosette){ // translate+flip to build rosette
                                                for (int k=0; k<6; ++k){
                                                        mul_mat_vec (xyr, rotM_R, xy);
                                                        copy_vec (atom.xyzc, xyr);
                                                        if (0)
                                                                mul_vec_vec (atom.xyzc, flip_xy);
                                                        add_to_vec (atom.xyzc, tma_R[k]);
                                                        put_atom(&atom, j++, 1);
                                                }
                                        } else {
                                                put_atom(&atom, j++, 1);
                                        }
                                }
                        }
                        mark_end(j);
                        return;

                } else if (!strncmp (filter, "Custom", 6)){
                        double rcut = 0.;
                        double zoff = 0.;
                        double origin[4];
                        double corner[4];
                        double ra[4];

                        size = src_size;
                        model = new Model_item[size+1];
                        for (int pos=0; pos<size+1; ++pos){
                                model[pos].xyzc[0] = 0.;
                                model[pos].xyzc[1] = 0.;
                                model[pos].xyzc[2] = 0.;
                                model[pos].xyzc[3] = 0.;
                                model[pos].N = -1;
                        }

                        if (filter_param)
                                std::cout << "Make_Model Filter: Custom. " << filter_param << std::endl;
                        else
                                std::cout << "Make_Model Filter: Custom, default. " << std::endl;
                        
                        // get sim scan area origin in ang                        
                        origin[0]=origin[1]=origin[2]=origin[3]=0.;
                        corner[0]=corner[1]=corner[2]=corner[3]=0.;

                        if (scan_area){
                                std::cout << "Make_Model Filter: Custom default. Auto Cutoff." << std::endl;
                                scan_area->Pixel2World (scan_area->mem2d->GetNx ()/2, scan_area->mem2d->GetNy ()/2, origin[0], origin[1]);
                                scan_area->Pixel2World (0, 0, corner[0], corner[1]);
                                sub_from_vec (corner, origin);
                                rcut = norm_vec (corner) + 6.;
                        }

                        /*
                        if (filter_param){
                                if (strlen (filter_param) > 2)
                                        switch (filter_param[0]){
                                        case 'R': rcut = atof (filter_param+1); break;
                                        case 'Z': zoff = atof (filter_param+1); break;
                                        }
                        }
                        */

                        zoff = -4.18;

                        std::cout << "Make_Model Filter: Custom. Zoff=" << zoff << " Rcut=" << rcut << std::endl;
                        int j=0;
                        for (int i=0; i<size && source->model[i].N > 0; ++i){
                                copy_vec4 (atom.xyzc, source->model[i].xyzc);
                                atom.N = source->model[i].N;
                                if (atom.xyzc[2] < 20.){
                                        if (rcut > 0.){
                                                copy_vec (ra, atom.xyzc);
                                                sub_from_vec (ra, origin);
                                                if (norm_vec (ra) > rcut)
                                                        continue;
                                        }
                                        atom.xyzc[2] += zoff;
                                        //if ( atom.xyzc[2] > -1.) // with 1st Cu layer
                                        if ( atom.xyzc[2] > 1.) // TMA only
                                                put_atom(&atom, j++);
                                }
                        }
                        mark_end(j);
                        return;
                }
        };

        ~xyzc_model (){
                if (model) delete[] model;
                if (probe) delete[] probe;
        };
        
        void auto_center_top_model (){
                // recompute center-top
                for (int i=0; i<size && model[i].N > 0; ++i){
                        if (i == 0){
                                copy_vec (xyz_max, model[i].xyzc);
                                copy_vec (xyz_min, model[i].xyzc);
                        }  else {
                                max_vec_elements (xyz_max, xyz_max, model[i].xyzc);
                                min_vec_elements (xyz_min, xyz_min, model[i].xyzc);
                        }
                }
                middle_vec_elements (xyz_center_top, xyz_min, xyz_max);
                xyz_center_top[2] = xyz_max[2] + 1.0; // move top mosrt atom to Z=-1A
                for (int i=0; i<size && model[i].N > 0; ++i)
                        sub_from_vec (model[i].xyzc, xyz_center_top);
        };
        
        void make_probe(int N=8, double q=-0.100, double dz=3.132){ // 3.661
                if (probe) delete[] probe;
                probe = new Model_item[2+5];

                // prepare LJ parameters
                std::cout << "#=======================================================" << std::endl;
                std::cout << "AFMMechSim: Prepare L-J parameters Probe <-> ModelAtoms." << std::endl;
                int i=0;
                for (; Elements[i].N != -1 && Elements[i].N != N; ++i);
                lj_param_pre_compute (Elements[i].LJp); // O tip probe
                std::cout << "#=======================================================" << std::endl;
        
                if (dz < 0.1){
                        dz = pow(2. * LJp_AB_lookup[0][0] / LJp_AB_lookup[0][1], 1./6.);
                        std::cout << "==> Probe dz [r0] = " << dz << std::endl;
                }

                i=0;
                probe[i].xyzc[0] = 0.; // [0] => Apex
                probe[i].xyzc[1] = 0.;
                probe[i].xyzc[2] = 0.;
                probe[i].xyzc[3] = -q;
                probe[i++].N = 0;

                // O (default)
                probe[i].xyzc[0] = 0.; // [1] ==> probe Atom
                probe[i].xyzc[1] = 0.;
                probe[i].xyzc[2] = -dz;
                probe[i].xyzc[3] = q;
                probe[i++].N = N;

                // C -- model purpose ony
                probe[i].xyzc[0] = 0.;
                probe[i].xyzc[1] = 0.;
                probe[i].xyzc[2] = -2.004;
                probe[i].xyzc[3] = 0;
                probe[i++].N = 6;
                // Cu Apex -- model purpose ony
                probe[i].xyzc[0] = 0.; // [2...] => Cu Apex just for model
                probe[i].xyzc[1] = 0.;
                probe[i].xyzc[2] = 0.;
                probe[i].xyzc[3] = -q;
                probe[i++].N = 29;
                probe[i].xyzc[0] = -1.27805; // [2...] => Cu Apex just for model
                probe[i].xyzc[1] = 1.27805;
                probe[i].xyzc[2] = 1.27805;
                probe[i].xyzc[3] = -q;
                probe[i++].N = 29;
                probe[i].xyzc[0] = 1.27805; // [2...] => Cu Apex just for model
                probe[i].xyzc[1] = 1.27805;
                probe[i].xyzc[2] = 1.27805;
                probe[i].xyzc[3] = -q;
                probe[i++].N = 29;
                probe[i].xyzc[0] = 0.; // [2...] => Cu Apex just for model
                probe[i].xyzc[1] = -1.27805;
                probe[i].xyzc[2] = 1.27805;
                probe[i].xyzc[3] = -q;
                probe[i++].N = 29;
                /*
                  Cu 2.5561 -4.4273 0
                  Cu 1.27805 2.21365 0
                  Cu 1.27805 -2.21365 0
                  Cu 2.5561 0 0
                  Cu 0 0 0
                  Cu -2.5561 0 0
                  Cu -1.27805 2.21365 0
                  Cu -1.27805 -2.21365 0
                */

        };

        inline double get_probe_dz () { return  probe[1].xyzc[2]; };
        void set_probe_dz (double dz) { probe[1].xyzc[2] = dz; };
        
        void mark_end (int pos) {
                if (pos < size){
                        model[pos].xyzc[0] = 0.;
                        model[pos].xyzc[1] = 0.;
                        model[pos].xyzc[2] = 0.;
                        model[pos].xyzc[3] = 0.;
                        model[pos].N = -1;
                }
        };
        
        void put_atom (Model_item *atom, int pos, int mode=0) {
                if (pos < size){
                        if (mode == 1){
                                mul_mat_vec_tc4 (model[pos].xyzc, TrMat, atom->xyzc);
                        } else
                                copy_vec4 (model[pos].xyzc, atom->xyzc);
                        model[pos].N = atom->N;
                        std::cout << pos << " => N=" << model[pos].N
                                  << " " << model[pos].xyzc[0]
                                  << " " << model[pos].xyzc[1]
                                  << " " << model[pos].xyzc[2]
                                  << " " << model[pos].xyzc[3]
                                  << std::endl;
                } else
                        std::cout << pos << " ERROR: OUT OF RANGE"
                                  << std::endl;
        };

        // LJp[0]: Eneregy in eV, LJp[1]: r0 in Ang
        inline double cAab (double LJp_a[2], double LJp_b[2], double AB[2]){
                double r_ab = LJp_a[1]+LJp_b[1]; // r
                double r_ab2 = r_ab*r_ab;
                double r_ab4 = r_ab2*r_ab2;
                double r_ab6 = r_ab4*r_ab2;  // r^6
                double r_ab12 = r_ab6*r_ab6; // r^12
                double sqrtLJpab = sqrt (LJp_a[0]*LJp_b[0]); // E_ab
                AB[0] = 2.0*sqrtLJpab * r_ab6;  // A_ab = 2 E_ab r^6,    r_ab = ra+rb
                AB[1] =     sqrtLJpab * r_ab12; // B_ab =   E_ab r^12
                return r_ab; 
        };
        
        // pre compute L-J parameters needed: probe to ... all atoms
        void lj_param_pre_compute (double LJp_probe[2]){
                // LP_probe <-> atom_i
                for (int i=0; Elements[i].N != -1; ++i)
                        if (Elements[i].LJp[0] > 0.){
                                double r=cAab(LJp_probe, Elements[i].LJp, LJp_AB_lookup[Elements[i].N]);
                                WorkF_lookup[Elements[i].N] = Elements[i].Phi;
                                printf("## LJp[%02d] --- eps_a= %10.8f eV, r_a= %10.8f Ang, %2s => L-J-AB to probe Aab= %14.4f Bab= %14.4f r_ab= %g Ang\n",
                                       Elements[i].N, Elements[i].LJp[0], Elements[i].LJp[1],
                                       Elements[i].name, LJp_AB_lookup[Elements[i].N][0], LJp_AB_lookup[Elements[i].N][1], r);
                        }
        }

        void set_fzmax (double mfz) { fzmax = mfz; };
        void scale_charge (double factor) {
                for (int pos=0; pos<size && model[pos].N > 0; ++pos)
                        model[pos].xyzc[3] *= factor;
        };

        const gchar *proton_number_to_name (int pn){
                for (int i=1; Elements[i].N > 0; ++i)
                        if (Elements[i].N == pn)
                                return Elements[i].name;
                return "??";
        };
        
        void print () {
                std::cout << "#============= MODEL n N XYZC ============" << std::endl;
                for (int pos=0; pos<size && model[pos].N > 0; ++pos)
                        std::cout << pos << " " << model[pos].N
                                  << " " << model[pos].xyzc[0]
                                  << " " << model[pos].xyzc[1]
                                  << " " << model[pos].xyzc[2]
                                  << " " << model[pos].xyzc[3]
                                  << std::endl;
                std::cout << "#============= PROBE n N XYZC ============" << std::endl;
                for (int pos=0; pos<2 && model[pos].N > 0; ++pos)
                        std::cout << pos << " " << probe[pos].N
                                  << " " << probe[pos].xyzc[0]
                                  << " " << probe[pos].xyzc[1]
                                  << " " << probe[pos].xyzc[2]
                                  << " " << probe[pos].xyzc[3]
                                  << std::endl;
                std::cout << "#=========================================" << std::endl;
        };

        void print_xyz () {
                std::cout << "#============= MODEL El X Y Z format ============" << std::endl;
                std::cout << (size+6) << std::endl;
                std::cout << "XYZ-Model-Gxsm-AFM-Sim-Plugin-Generated-w-apex" << std::endl;
                for (int pos=0; pos<size && model[pos].N > 0; ++pos)
                        std::cout << proton_number_to_name (model[pos].N)
                                  << " " << model[pos].xyzc[0]
                                  << " " << model[pos].xyzc[1]
                                  << " " << model[pos].xyzc[2]
                                  << std::endl;
                for (int k=1; k<7; ++k)
                        std::cout << proton_number_to_name (probe[k].N)
                                  << " " << (probe[k].xyzc[0]+8.)
                                  << " " << (probe[k].xyzc[1]+20.)
                                  << " " << (probe[k].xyzc[2]+12.)
                                  << std::endl;
                std::cout << "#=========================================" << std::endl;
        };

        
        Model_item *get_model_ref() { return model; };
        Model_item *get_probe_ref() { return probe; };
        int get_size() { return size; };

        void set_options (int o) { options = o; };
        void set_sim_zinfo (double sz0, double szf, double sdz) { sim_z0 = sz0, sim_zf = szf, sim_dz=sdz; };
        void get_sim_zinfo (double &sz0, double &szf, double &sdz) { sz0 = sim_z0, szf = sim_zf, sdz=sim_dz; };
        
public:
        Model_item *model;
        Model_item *probe;

        // Lookup Tables
        double LJp_AB_lookup[122][2]; // [0] AB: Probe - Apex, [1...n] Probe - Atom-n
        double WorkF_lookup[122]; // WorkFunc Loookup

        int options;

        double fzmax;
        double sensor_f0_k0[3];
        double probe_flex[3];
private:
        int size;
        double sim_z0, sim_zf, sim_dz;

        // orininal xyz limits and center-top
        double xyz_min[3];
        double xyz_max[3];
        double xyz_center_top[3];

        double TrMat[3][3];
};


/*
 * Build in Model Definition (TMA, TMA on Cu111 -- DFT relaxed)
 */


#define CdeltaQ_O -0.100 // x6
#define CdeltaQ_C  0.060 // x9
#define CdeltaQ_H  0.010  // x6

Model_item model_TMA[] = {
        // ------------- TMA
        { { -2.2657299, -0.149312,     3.1958958, CdeltaQ_O },    8 },//1-- O1..6
        { {  1.944746,  -2.7530839,    3.3754341, CdeltaQ_O },    8 },
        { {  4.0908289,  3.2430279,    3.3728768, CdeltaQ_O },    8 },
        { { -2.175231,   2.105442 ,    3.3714391, CdeltaQ_O },    8 },
        { {  3.9413929, -1.702212 ,    3.1951982, CdeltaQ_O },    8 },
        { {  2.182831,   4.4482431,    3.1951162, CdeltaQ_O },    8 },//6
        { { -0.114092,   0.8978820,    3.3588320, CdeltaQ_C },    6 },//1-- C1-9
        { {  1.957628,  -0.3640630,    3.3590242, CdeltaQ_C },    6 },//2
        { {  2.014699,   2.061168 ,    3.3591639, CdeltaQ_C },    6 },//3
        { {  0.5595030, -0.332026 ,    3.3545519, CdeltaQ_C },    6 },//4
        { {  0.6126090,  2.0933731,    3.355055 , CdeltaQ_C },    6 },//5
        { {  2.686532,   0.8345090,    3.3550102, CdeltaQ_C },    6 },//6
        { {  2.7322209,  3.3642991,    3.3098089, CdeltaQ_C },    6 },//7
        { { -1.601254 ,  0.8683400,    3.3095662, CdeltaQ_C },    6 },//8
        { {  2.727854 , -1.636749 ,    3.3104231, CdeltaQ_C },    6 },//9
        { { -0.0247530, -1.254158 ,    3.34009  , CdeltaQ_H },    1 },//1-- H1..6
        { {  0.106357 ,  3.0599561,    3.3414542, CdeltaQ_H },    1 },//2
        { {  3.7769761,  0.7893200,    3.3409788, CdeltaQ_H },    1 },//3
        { { -3.1466689,  1.970835 ,    3.271311 , CdeltaQ_H },    1 },//4
        { {  4.459666 ,  4.151723 ,    7.439335-4.167425, CdeltaQ_H },   1 }, //5
        { {  2.547698 , -3.52652  ,    3.2740132, CdeltaQ_H },    1 },//6
        { {  0., 0., 0., +0.000 }, -1 } // end table
};

Model_item model_TMA_Cu111[] = {
        // ------------- Cu111
        { {  -3.84526900,  -8.14233970,  -0.02985890, +0.000 },  63 },
        { {  -8.97151180,   0.73657400,  -0.02985890, +0.000 },  63 },
        { {   6.40721700,  -8.14233970,  -0.02985890, +0.000 },  63 },
        { {   1.28097400,   0.73657400,  -0.02985890, +0.000 },  63 },
        { {  -3.84526900,   9.61548610,  -0.02985890, +0.000 },  63 },
        { {  11.53345900,   0.73657400,  -0.02985890, +0.000 },  63 },
        { {   6.40721700,   9.61548610,  -0.02985890, +0.000 },  63 },
        { {  -5.12948080,  -5.92050500,  -0.00034410, +0.000 },  63 },
        { { -10.25572400,   2.95840810,  -0.00034410, +0.000 },  63 },
        { {   5.12300400,  -5.92050500,  -0.00034410, +0.000 },  63 },
        { {  -0.00323800,   2.95840810,  -0.00034410, +0.000 },  63 },
        { {  10.24924700,   2.95840810,  -0.00034410, +0.000 },  63 },
        { {  -6.41020490,  -3.70313690,   0.00259410, +0.000 },  63 },
        { {   3.84227990,  -3.70313690,   0.00259410, +0.000 },  63 },
        { {  -1.28396300,   5.17577600,   0.00259410, +0.000 },  63 },
        { {   8.96852300,   5.17577600,   0.00259410, +0.000 },  63 },
        { {  -2.56313990, -10.36370800,  -0.03171290, +0.000 },  63 },
        { {  -7.68938300,  -1.48479500,  -0.03171290, +0.000 },  63 },
        { {   2.56310300,  -1.48479500,  -0.03171290, +0.000 },  63 },
        { {  -2.56313990,   7.39411780,  -0.03171290, +0.000 },  63 },
        { {   7.68934490,   7.39411780,  -0.03171290, +0.000 },  63 },
        { {  -1.28067900,  -8.13591480,  -0.00079620, +0.000 },  63 },
        { {  -6.40692090,   0.74299800,  -0.00079620, +0.000 },  63 },
        { {   3.84556390,   0.74299800,  -0.00079620, +0.000 },  63 },
        { {  -1.28067900,   9.62191100,  -0.00079620, +0.000 },  63 },
        { {  -2.56187200,  -5.91890810,  -0.03133620, +0.000 },  63 },
        { {  -7.68811510,   2.96000500,  -0.03133620, +0.000 },  63 },
        { {   7.69061280,  -5.91890810,  -0.03133620, +0.000 },  63 },
        { {   2.56436990,   2.96000500,  -0.03133620, +0.000 },  63 },
        { {  -3.84314300,  -3.70026110,   0.01125300, +0.000 },  63 },
        { {  -8.96938510,   5.17865180,   0.01125300, +0.000 },  63 },
        { {   6.40934180,  -3.70026110,   0.01125300, +0.000 },  63 },
        { {   1.28310000,   5.17865180,   0.01125300, +0.000 },  63 },
        { {  -0.00166800, -10.36079400,   0.01077810, +0.000 },  63 },
        { {  -5.12791110,  -1.48188100,   0.01077810, +0.000 },  63 },
        { {   5.12457510,  -1.48188100,   0.01077810, +0.000 },  63 },
        { {  -0.00166800,   7.39703180,   0.01077810, +0.000 },  63 },
        { {   1.27965610,  -8.13668440,   0.00251500, +0.000 },  63 },
        { {  -3.84658690,   0.74222800,   0.00251500, +0.000 },  63 },
        { {   6.40589900,   0.74222800,   0.00251500, +0.000 },  63 },
        { {   1.27965610,   9.62114140,   0.00251500, +0.000 },  63 },
        { { -10.25145900,  -5.91750910,   0.01260580, +0.000 },  63 },
        { {   0.00102600,  -5.91750910,   0.01260580, +0.000 },  63 },
        { {  -5.12521600,   2.96140410,   0.01260580, +0.000 },  63 },
        { {  10.25351100,  -5.91750910,   0.01260580, +0.000 },  63 },
        { {   5.12726880,   2.96140410,   0.01260580, +0.000 },  63 },
        { {   0.00102600,  11.84031700,   0.01260580, +0.000 },  63 },
        { {  -1.27984910,  -3.70021800,   0.00135390, +0.000 },  63 },
        { {  -6.40609120,   5.17869520,   0.00135390, +0.000 },  63 },
        { {   8.97263720,  -3.70021800,   0.00135390, +0.000 },  63 },
        { {   3.84639410,   5.17869520,   0.00135390, +0.000 },  63 },
        { {   2.56073190, -10.35686800,   0.00940720, +0.000 },  63 },
        { {  -2.56551000,  -1.47795500,   0.00940720, +0.000 },  63 },
        { {  -7.69175290,   7.40095810,   0.00940720, +0.000 },  63 },
        { {   7.68697500,  -1.47795500,   0.00940720, +0.000 },  63 },
        { {   2.56073190,   7.40095810,   0.00940720, +0.000 },  63 },
        { {  -6.41086010,  -8.13850880,  -0.03156310, +0.000 },  63 },
        { { -11.53710300,   0.74040400,  -0.03156310, +0.000 },  63 },
        { {   3.84162500,  -8.13850880,  -0.03156310, +0.000 },  63 },
        { {  -1.28461690,   0.74040400,  -0.03156310, +0.000 },  63 },
        { {  -6.41086010,   9.61931710,  -0.03156310, +0.000 },  63 },
        { {   8.96786790,   0.74040400,  -0.03156310, +0.000 },  63 },
        { {   3.84162500,   9.61931710,  -0.03156310, +0.000 },  63 },
        { {  -7.69151310,  -5.91883420,   0.01138510, +0.000 },  63 },
        { {   2.56097200,  -5.91883420,   0.01138510, +0.000 },  63 },
        { {  -2.56526990,   2.96007900,   0.01138510, +0.000 },  63 },
        { {   7.68721490,   2.96007900,   0.01138510, +0.000 },  63 },
        { {  -8.97151570,  -3.70199890,   0.01212180, +0.000 },  63 },
        { {   1.28097000,  -3.70199890,   0.01212180, +0.000 },  63 },
        { {  -3.84527300,   5.17691420,   0.01212180, +0.000 },  63 },
        { {   6.40721180,   5.17691420,   0.01212180, +0.000 },  63 },
        { {  -5.12537480, -10.36281600,   0.00000020, +0.000 },  63 },
        { { -10.25161700,  -1.48390310,   0.00000020, +0.000 },  63 },
        { {   5.12711000, -10.36281600,   0.00000020, +0.000 },  63 },
        { {   0.00086700,  -1.48390310,   0.00000020, +0.000 },  63 },
        { {  -5.12537480,   7.39501000,   0.00000020, +0.000 },  63 },
        { {  10.25335300,  -1.48390310,   0.00000020, +0.000 },  63 },
        { {   5.12711000,   7.39501000,   0.00000020, +0.000 },  63 },
        //  ----------- TMA
        { { -2.2657299, -0.149312,     3.1958958, +0.000 },    8 },//1-- O1..6
        { {  1.944746,  -2.7530839,    3.3754341, +0.000 },    8 },//2
        { {  4.0908289,  3.2430279,    3.3728768, +0.000 },    8 },//3
        { { -2.175231,   2.105442 ,    3.3714391, +0.000 },    8 },//4
        { {  3.9413929, -1.702212 ,    3.1951982, +0.000 },    8 },//5
        { {  2.182831,   4.4482431,    3.1951162, +0.000 },    8 },//6
        { { -0.114092,   0.8978820,    3.3588320, +0.000 },    6 },//1-- C1-9
        { {  1.957628,  -0.3640630,    3.3590242, +0.000 },    6 },//2
        { {  2.014699,   2.061168 ,    3.3591639, +0.000 },    6 },//3
        { {  0.5595030, -0.332026 ,    3.3545519, +0.000 },    6 },//4
        { {  0.6126090,  2.0933731,    3.355055 , +0.000 },    6 },//5
        { {  2.686532,   0.8345090,    3.3550102, +0.000 },    6 },//6
        { {  2.7322209,  3.3642991,    3.3098089, +0.000 },    6 },//7
        { { -1.601254 ,  0.8683400,    3.3095662, +0.000 },    6 },//8
        { {  2.727854 , -1.636749 ,    3.3104231, +0.000 },    6 },//9
        { { -0.0247530, -1.254158 ,    3.34009  , +0.000 },    1 },//1-- H1..6
        { {  0.106357 ,  3.0599561,    3.3414542, +0.000 },    1 },//2
        { {  4.459666 ,  4.151723 ,    7.439335-4.167425, +0.000 },   1 },//3
        { {  3.7769761,  0.7893200,    3.3409788, +0.000 },    1 },//4
        { { -3.1466689,  1.970835 ,    3.271311 , +0.000 },    1 },//5
        { {  2.547698 , -3.52652  ,    3.2740132, +0.000 },    1 },//6
        // ------------
        { {  0., 0., 0., +0.000 }, -1 } // end table
};

// thread job environment data
typedef struct{
	Scan* Dest;
	Mem2d* work;
	int line_i, line_f, line_inc;
        int time_index;
        double z, dz, precision;
        int maxiter;
        xyzc_model *model;
	int *status;
	int job;
	double progress;
	int verbose_level;
        gint gradflag;
} Probe_Optimize_Job_Env;


// NL-OPL function parameters
typedef struct{
        double A[3];       // apex coordinates
        double Fsum[3];    // all over force sum
        double Fljsum[3];  // probe - molecule object/surface -- LJ
        double Fcsum[3];   //  probe - molecule object/surface -- Coulomb
        double Fa_lj[3];   // apex-probe force vector -- LJ component
        double Fa_c[3];    // apex-probe force vector -- Coulomb component
        double Fa_flex[3]; // apex-probe force vector -- Flex component
        double Fa[3];      // apex-probe force vector -- Sum of above
        double grad[3];    // force field gradient to "apex"
        double Fz;         // apex Z force componet
        double residual_force_mag;
        xyzc_model *model;
        Scan *scan;
        double dz, z0, zf;
        gint count;
} LJ_calc_params;


#ifdef GXSM3_MAJOR_VERSION // GXSM3 GUI code

static void cancel_callback (GtkWidget *widget, int *status){
	*status = 1; 
}
	
void ChangeValue (GtkComboBox* Combo,  gchararray *string){
	GtkTreeIter iter;
	GtkTreeModel *model;
	gtk_combo_box_get_active_iter (GTK_COMBO_BOX (Combo), &iter);
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (Combo));
        if (*string){
                g_free (*string); *string=NULL;
        }
	gtk_tree_model_get (model, &iter, 0, &(*string), -1);
        std::cout << *string << std::endl;
}

void ChangeEntry (GtkEntry* entry,  gchararray *string){
        const gchar *ctxt = gtk_entry_get_text (GTK_ENTRY (entry));
        if (*string)
                g_free (*string);
        *string = g_strdup (ctxt);
        std::cout << *string << std::endl;
}
        
void xyz_file_set (GtkFileChooserButton *chooser,  gchar **string){
        *string = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));    
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (g_object_get_data (G_OBJECT (chooser), "COMBO")), *string, *string);
        std::cout << *string << std::endl;
}

void ChangeIndex (GtkSpinButton *spinner, double *index){
	*index = gtk_spin_button_get_value (spinner);

	std::cout << "Change Index: " << ((int)*index) << std::endl;
}


#else // GXSM2 GUI code


static void cancel_callback (GtkWidget *widget, int *status){
	*status = 1; 
}
	
void ChangeValue (GtkComboBox* Combo,  gchar **string){
	GtkTreeIter iter;
	GtkTreeModel *model;
	gtk_combo_box_get_active_iter (GTK_COMBO_BOX (Combo), &iter);
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (Combo));
        std::cout << "ChangeValue called" << std::endl << std::flush;
        if (*string)
                g_free (*string);
	gtk_tree_model_get (model, &iter, 0, &(*string), -1);
        std::cout << *string << std::endl;
}
	
void ChangeEntry (GtkEntry* entry,  gchar **string){
        const gchar *ctxt = gtk_entry_get_text (GTK_ENTRY (entry));
        if (*string)
                g_free (*string);
        *string = g_strdup (ctxt);
        std::cout << *string << std::endl;
}
        
void xyz_file_set (GtkFileChooserButton *chooser,  gchar **string){
        *string = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));    
        //        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (g_object_get_data (G_OBJECT (chooser), "COMBO")), *string, *string);
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (g_object_get_data (G_OBJECT (chooser), "COMBO")), *string);
        std::cout << *string << std::endl;
}

void ChangeIndex (GtkSpinButton *spinner, double *index){
	*index = gtk_spin_button_get_value (spinner);

	std::cout << "Change Index: " << ((int)*index) << std::endl;
}

#define ADD_LINE(A,B)                           \
        do{ \
        hbox = gtk_hbox_new (FALSE, 0);                                 \
        gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);       \
        gtk_widget_show (hbox);                                         \
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);     \
        gtk_box_pack_start (GTK_BOX (hbox), A, FALSE, FALSE, 0);    \
        gtk_box_pack_start (GTK_BOX (hbox), B, FALSE, FALSE, 0);    \
        }while(0)

#define APP_LINE(A)                           \
        do{ \
        gtk_box_pack_start (GTK_BOX (hbox), A, FALSE, FALSE, 0);    \
        }while(0)

#define ADD_EC_WITH_SCALE(LABEL, UNIT, ERROR_TEXT, VAR, LO, HI, FMT, STEP, PAGE, CALLBACK) \
        do{\
		input = mygtk_create_spin_input(_(LABEL), vbox_param, hbox_param, 400); \
		Gtk_EntryControl *ec = new Gtk_EntryControl (UNIT, _(ERROR_TEXT), VAR, LO, HI, FMT, input, STEP, PAGE);	\
		if (CALLBACK)						\
			ec->Set_ChangeNoticeFkt(CALLBACK, this);	\
		SetupScale(ec->GetAdjustment(), hbox_param);		\
	}while(0)

#define ADD_EC(LABEL, UNIT, ERROR_TEXT, VAR, LO, HI, FMT, STEP, PAGE, CALLBACK) \
        do{\
		input = mygtk_create_input(_(LABEL), vbox_param, hbox_param, 150); \
		Gtk_EntryControl *ec = new Gtk_EntryControl (UNIT, _(ERROR_TEXT), VAR, LO, HI, FMT, input, STEP, PAGE);	\
		if (CALLBACK)						\
			ec->Set_ChangeNoticeFkt(CALLBACK, this);	\
	}while(0)
#define APP_EC(LABEL, UNIT, ERROR_TEXT, VAR, LO, HI, FMT, STEP, PAGE, CALLBACK) \
        do{\
		input = mygtk_add_input(_(LABEL), hbox_param, 150); \
		Gtk_EntryControl *ec = new Gtk_EntryControl (UNIT, _(ERROR_TEXT), VAR, LO, HI, FMT, input, STEP, PAGE);	\
		if (CALLBACK)						\
			ec->Set_ChangeNoticeFkt(CALLBACK, this);	\
	}while(0)
#endif

class Dialog : public AppBase{
public:
	Dialog (double &zi, double &zf, double &dz, double &precision, double &maxiter, double &fzmax,
                double sensor_f0_k0[2], double probe_flex[3],
                gchararray *model_selected, gchararray *model_filter, gchararray *model_filter_param,
                gchararray *calc_mode_selected, gchararray *calc_options_selected,
                gchararray *external_model,
                gchararray *probe_type, gchararray *probe_model,
                double &initial_probe_below_apex,
                double &probe_charge,
                double &charge_scaling,
                double &max_threads,
                gchararray *progress_detail
                ){

#ifdef GXSM3_MAJOR_VERSION // GXSM3 GUI code
                
		GtkWidget *dialog;
                GtkWidget *choice;
                GtkWidget *chooser;

                UnitObj *meVA    = new UnitObj("meV/" UTF8_ANGSTROEM, "meV/" UTF8_ANGSTROEM );
                UnitObj *pcharge = new UnitObj("e", "e");
                UnitObj *hertz = new UnitObj("Hz", "Hz");
                UnitObj *pnewton = new UnitObj("pA", "pN");
                UnitObj *stiffness = new UnitObj("N/m", "N/m");

                GtkDialogFlags flags =  (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT);
                dialog = gtk_dialog_new_with_buttons (N_("AFM Simulation Settings for Force Probe Path Tracing"),
                                                      GTK_WINDOW (gapp->get_app_window ()),
                                                      flags,
                                                      _("_OK"),
                                                      GTK_RESPONSE_ACCEPT,
                                                      _("_Cancel"),
                                                      GTK_RESPONSE_REJECT,
                                                      NULL);
                BuildParam bp;
                gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);
                bp.set_error_text ("Value not allowed.");
                bp.input_nx = 1;
                bp.grid_add_ec ("Z initial",   gapp->xsm->Z_Unit, &zi,     0.,   100., ".2f", 0.1,  1.);
		bp.grid_add_ec ("Z-final",     gapp->xsm->Z_Unit, &zf,     0.,   100., ".2f", 0.1,  1.);
                bp.grid_add_ec ("Z step",      gapp->xsm->Z_Unit, &dz,     0.,    10., ".3f", 0.1,  1.);
                bp.new_line ();
                bp.grid_add_ec ("NL-OPT precision", meVA, &precision,   0.,     1., "g", 0.00001,  0.0001);
                bp.grid_add_ec ("NL-OPT maxiter",   gapp->xsm->Unity, &maxiter,     0.,  1000., "g", 1.,  10.);
                bp.grid_add_ec ("Max. FZ",   pnewton, &fzmax,     0.,  10000., "g", 1.,  10.);
                bp.new_line ();

                bp.input_nx = 1;
                bp.grid_add_ec ("Sensor res freq.",   hertz, &sensor_f0_k0[0],     1.,  1e7, "g", 1.,  10.);
                gtk_entry_set_width_chars (GTK_ENTRY (bp.input), 10);
                bp.grid_add_ec ("k0",   stiffness, &sensor_f0_k0[1],     1.,  1e5, "g", 1.,  10.);
                gtk_entry_set_width_chars (GTK_ENTRY (bp.input), 10);
                bp.new_line ();
        
                bp.grid_add_ec ("Probe stiffness X",   stiffness, &probe_flex[0],     0.,  1000., "g", 1.,  10.);
                gtk_entry_set_width_chars (GTK_ENTRY (bp.input), 8);
                bp.grid_add_ec ("Y",   stiffness, &probe_flex[1],     0.,  1000., "g", 1.,  10.);
                gtk_entry_set_width_chars (GTK_ENTRY (bp.input), 8);
                bp.grid_add_ec ("Z",   stiffness, &probe_flex[2],     0.,  1000., "g", 1.,  10.);
                gtk_entry_set_width_chars (GTK_ENTRY (bp.input), 8);
                bp.new_line ();

                bp.grid_add_label ("Model", NULL, 1, 1.0);
		choice = gtk_combo_box_text_new ();
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "TMA", "TMA");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "Cu111+TMA", "Cu111+TMA");
		g_signal_connect(G_OBJECT (choice), "changed", G_CALLBACK (ChangeValue), &(*model_selected));
		bp.grid_add_widget (choice, 5);

                chooser = gtk_file_chooser_button_new (_("XYZ Model File"), GTK_FILE_CHOOSER_ACTION_OPEN);
                GtkFileFilter *xyz_filter = gtk_file_filter_new ();
                gtk_file_filter_set_name (xyz_filter, "XYZ");
                gtk_file_filter_add_pattern (xyz_filter, "*.xyz");
                gtk_file_filter_add_pattern (xyz_filter, "*.XYZ");
                gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), xyz_filter);
                GtkFileFilter *all_filter = gtk_file_filter_new ();
                gtk_file_filter_set_name (all_filter, "All");
                gtk_file_filter_add_pattern (all_filter, "*");
                gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), all_filter);

                g_object_set_data (G_OBJECT (chooser), "COMBO", choice);
		g_signal_connect (G_OBJECT (chooser), "file-set", G_CALLBACK (xyz_file_set), &(*external_model));
		gtk_widget_show (chooser);
		gtk_combo_box_set_active (GTK_COMBO_BOX (choice), 0);
		bp.grid_add_widget (chooser, 5);
                bp.new_line ();
                        
                bp.grid_add_label ("Model Filter", NULL, 1, 1.0);
		choice = gtk_combo_box_text_new ();
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "None", "None");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "TMA-C+TMA-R-flip",       "TMA-C+TMA-R-flip");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "TMA-C+TMA-R-flip+Cu111", "TMA-C+TMA-R-flip+Cu111");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "TMA-Rosette",       "TMA-Rosette");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "TMA-Rosette+Cu111", "TMA-Rosette+Cu111");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "Custom: 1<Z<20", "Custom: 1<Z<20");
		gtk_widget_show (choice);			
		g_signal_connect(G_OBJECT (choice), "changed", G_CALLBACK (ChangeValue), &(*model_filter));
		gtk_combo_box_set_active (GTK_COMBO_BOX (choice), 0);
		bp.grid_add_widget (choice, 5);
                bp.new_line ();
                        
                bp.grid_add_input ("Model Filter Param", 15);
		g_signal_connect(G_OBJECT (bp.input), "changed", G_CALLBACK (ChangeEntry), &(*model_filter_param));
                bp.new_line ();
                        
                bp.grid_add_label ("Calculation Mode", NULL, 1, 1.0);
		choice = gtk_combo_box_text_new ();
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "NO-OPT", "NO-OPT/Force at fixed probe");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "NL-OPT", "NL-OPT/Probe relax");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "IPF-NL-OPT", "IPF-NL-OPT/Probe relax -- fast: using interpolated fields and gradients");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "TOPO", "TOPO/Display Model");
                // gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "CTIS", "CTIS");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "NONE", "NONE/Abort");
		g_signal_connect(G_OBJECT (choice), "changed", G_CALLBACK (ChangeValue), &(*calc_mode_selected));
		gtk_combo_box_set_active (GTK_COMBO_BOX (choice), 1);
		bp.grid_add_widget (choice, 15);
                bp.new_line ();

                bp.grid_add_label ("Calculation Options", NULL, 1, 1.0);
		choice = gtk_combo_box_text_new ();
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "L-J Model", "L-J Model");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "L-J+Coulomb Model", "L-J+Coulomb Model");
		g_signal_connect(G_OBJECT (choice), "changed", G_CALLBACK (ChangeValue), &(*calc_options_selected));
		gtk_combo_box_set_active (GTK_COMBO_BOX (choice), 1);
		bp.grid_add_widget (choice, 15);
                bp.new_line ();

                bp.grid_add_label ("Probe (active tip)", NULL, 1, 1.0);
		choice = gtk_combo_box_text_new ();
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "Copper", "Copper probe");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "Oxigen", "Oxigen terminated probe (CO like)");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "Bromium", "Bromium terminated probe");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "Xenon",  "Xenon terminated probe");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "Carbon", "Carbon terminated probe");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "Tungston", "Tungston terminated probe");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "Platinum", "Platinum terminated probe");
		g_signal_connect(G_OBJECT (choice), "changed", G_CALLBACK (ChangeValue), &(*probe_type));
		gtk_combo_box_set_active (GTK_COMBO_BOX (choice), 1);
		bp.grid_add_widget (choice, 15);
                bp.new_line ();

                bp.grid_add_label ("Probe Model:", NULL, 1, 1.0);
		choice = gtk_combo_box_text_new ();
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "Spring", "Spring");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "L-J", "L-J");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "L-J + Coulomb", "L-J + Coulomb");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "L-J + Spring", "L-J + Spring");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "L-J + Coulomb + Spring", "L-J + Coulomb + Spring");
		g_signal_connect(G_OBJECT (choice), "changed", G_CALLBACK (ChangeValue), &(*probe_model));
		gtk_combo_box_set_active (GTK_COMBO_BOX (choice), 4);
		bp.grid_add_widget (choice, 15);
                bp.new_line ();

                bp.input_nx = 5;
                bp.grid_add_ec_with_scale ("Initial Probe below Aplex (0=auto)",  gapp->xsm->Z_Unit, &initial_probe_below_apex,  -20.,  20., ".3f", 0.1,  1.);
                bp.new_line ();
                bp.grid_add_ec_with_scale ("Probe Charge",   pcharge,  &probe_charge,  -2.,    2., ".3f", 0.01,  0.1);
                bp.new_line ();
                bp.grid_add_ec_with_scale ("Charge Scaling", gapp->xsm->Unity, &charge_scaling,     -100.,  100., ".3f", 0.1,  1.);
                bp.new_line ();
		bp.grid_add_ec_with_scale ("Max Thread #",   gapp->xsm->Unity, &max_threads,    1.,   64., ".0f", 1.,  4.);
                bp.new_line ();

                bp.grid_add_label ("Progress Detail", NULL, 1, 1.0);
		choice = gtk_combo_box_text_new ();
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "Basic", "Basic");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "Combined Job", "Combined Job");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "Job Detail", "Job Detail");
		g_signal_connect(G_OBJECT (choice), "changed", G_CALLBACK (ChangeValue), &(*progress_detail));
		gtk_combo_box_set_active (GTK_COMBO_BOX (choice), 2);
		bp.grid_add_widget (choice, 15);

                gtk_widget_show_all (dialog);
                
		gint r=gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy (dialog);
              
                delete meVA;
                delete pcharge;
                delete hertz;
                delete pnewton;
                delete stiffness;

                if (r == GTK_RESPONSE_REJECT){
                        *calc_mode_selected = g_strdup ("NONE");
                }

#else // GXSM2 GUI code
		GtkWidget *dialog;
		GtkWidget *vbox;
		GtkWidget *hbox;
		GtkWidget *hbox_param;
		GtkWidget *vbox_param;
		GtkWidget *label;
                GtkWidget *input;
                GtkWidget *choice, *entry;
		GtkWidget *grid;
                GtkWidget *chooser;
                int x,y;

                UnitObj *meVA    = new UnitObj("meV/" UTF8_ANGSTROEM, "meV/" UTF8_ANGSTROEM );
                UnitObj *pcharge = new UnitObj("e", "e");
                UnitObj *hertz = new UnitObj("Hz", "Hz");
                UnitObj *stiffness = new UnitObj("N/m", "N/m");

                GtkDialogFlags flags =  (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT);
                dialog = gtk_dialog_new_with_buttons (N_("AFM Simulation Settings for Force Probe Path Tracing"),
                                                      NULL, // GTK_WINDOW (gapp->get_app_window ()),
                                                      flags,
                                                      GTK_STOCK_OK,
						      GTK_RESPONSE_ACCEPT,
						      NULL);
 
		vbox_param = vbox = gtk_vbox_new (FALSE, 0);
		gtk_widget_show (vbox);
                gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                                   vbox, FALSE, FALSE, GNOME_PAD);

                //                x=y=1;
                //		grid = gtk_grid_new ();
                //		gtk_widget_show (grid);
		//gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG(dialog))),
		//		    grid, FALSE, FALSE, GXSM_WIDGET_PAD);
                //		gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG(dialog))),
		//		    vbox, FALSE, FALSE, GNOME_PAD);

		//	ADD_EC_WITH_SCALE(LABEL, UNIT, ERROR_TEXT, VAR, LO, HI, FMT, STEP, PAGE, CALLBACK);
                ++y; x=1;
                ADD_EC_WITH_SCALE(N_("Z initial"),   gapp->xsm->Z_Unit, "Value not allowed.", &zi,     0.,   100., ".2f", 0.1,  1., NULL);
                ++y; x=1;
		ADD_EC_WITH_SCALE(N_("Z-final"),     gapp->xsm->Z_Unit, "Value not allowed.", &zf,     0.,   100., ".2f", 0.1,  1., NULL);
                ++y; x=1;
                APP_EC(N_("Z step"),      gapp->xsm->Z_Unit, "Value not allowed.", &dz,     0.,    10., ".3f", 0.1,  1., NULL);
                ++y; x=1;
                ADD_EC(N_("NL-OPT precision"), meVA, "Value not allowed.", &precision,   0.,     1., "g", 0.00001,  0.0001, NULL);
                ++y; x=1;
                APP_EC(N_("NL-OPT maxiter"),   gapp->xsm->Unity, "Value not allowed.", &maxiter,     0.,  1000., "g", 1.,  10., NULL);
                ++y; x=1;

                ++y; x=1;
                ADD_EC(N_("Sensor res freq."),   hertz, "Value not allowed.", &sensor_f0_k0[0],     1.,  1e7, "g", 1.,  10., NULL);
                APP_EC(N_("k0"),   stiffness, "Value not allowed.", &sensor_f0_k0[1],     1.,  1e5, "g", 1.,  10., NULL);
                ++y; x=1;
                ADD_EC(N_("Probe stiffness X"),   stiffness, "Value not allowed.", &probe_flex[0],     0.,  1000., "g", 1.,  10., NULL);
                APP_EC(N_("Y"),   stiffness, "Value not allowed.", &probe_flex[1],     0.,  1000., "g", 1.,  10., NULL);
                APP_EC(N_("Z"),   stiffness, "Value not allowed.", &probe_flex[2],     0.,  1000., "g", 1.,  10., NULL);

                label = gtk_label_new (N_("Model"));	
		gtk_widget_show (label);			
                //		gtk_grid_attach (GTK_GRID (grid), label, x++,y, 1,1); 
		choice = gtk_combo_box_text_new ();
                ADD_LINE (label, choice);
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "TMA");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "Cu111+TMA");
		gtk_widget_show (choice);			
		g_signal_connect(G_OBJECT (choice), "changed", G_CALLBACK (ChangeValue), &(*model_selected));
                //		gtk_grid_attach (GTK_GRID (grid), choice, x++, y, 1,1);
                
                chooser = gtk_file_chooser_button_new (_("XYZ Model File"), GTK_FILE_CHOOSER_ACTION_OPEN);
                g_object_set_data (G_OBJECT (chooser), "COMBO", choice);
		g_signal_connect (G_OBJECT (chooser), "file-set", G_CALLBACK (xyz_file_set), &(*external_model));
		gtk_combo_box_set_active (GTK_COMBO_BOX (choice), 0);
		gtk_widget_show (chooser);
                //                gtk_grid_attach (GTK_GRID (grid), chooser, x++, y, 1,1);
                APP_LINE (chooser);
                        
                ++y; x=1;
                label = gtk_label_new (N_("Model Filter"));	
                //		gtk_grid_attach (GTK_GRID (grid), label, x++,y, 1,1); 
		choice = gtk_combo_box_text_new ();
                ADD_LINE (label, choice);
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "None");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "TMA-C+TMA-R-flip");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "TMA-C+TMA-R-flip+Cu111");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "TMA-Rosette");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "TMA-Rosette+Cu111");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "Custom: 1<Z<20");
                //		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "Custom-3", "Custom-3");
		g_signal_connect(G_OBJECT (choice), "changed", G_CALLBACK (ChangeValue), &(*model_filter));
		gtk_combo_box_set_active (GTK_COMBO_BOX (choice), 0);
                //		gtk_grid_attach (GTK_GRID (grid), choice, x++, y, 1,1);
           	//gtk_combo_box_text_append_text (GTK_COMBO_BOX (source2_combo), defaultprobe->get_label (i));

                ++y; x=1;
                label = gtk_label_new (N_("Model Filter Param"));	
		gtk_widget_show (label);			
                //		gtk_grid_attach (GTK_GRID (grid), label, x++,y, 1,1); 
                entry = gtk_entry_new ();
		gtk_widget_show (entry);
                //		gtk_grid_attach (GTK_GRID (grid), entry, x++,y, 1,1); 
		g_signal_connect(G_OBJECT (entry), "changed", G_CALLBACK (ChangeEntry), &(*model_filter_param));
                ADD_LINE (label, entry);
                
                ++y; x=1;
                label = gtk_label_new (N_("Calculation Mode"));	
                //		gtk_grid_attach (GTK_GRID (grid), label, x++,y, 1,1); 
		choice = gtk_combo_box_text_new ();
                ADD_LINE (label, choice);
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "NO-OPT/Force at fixed probe");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "NL-OPT/Probe relax");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "IPF-NL-OPT/Probe relax");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "TOPO/Display Model");
                // gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "CTIS", "CTIS");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "NONE/Abort");
		g_signal_connect(G_OBJECT (choice), "changed", G_CALLBACK (ChangeValue), &(*calc_mode_selected));
		gtk_combo_box_set_active (GTK_COMBO_BOX (choice), 0);
                //		gtk_grid_attach (GTK_GRID (grid), choice, x++, y, 1,1);

                ++y; x=1;
                label = gtk_label_new (N_("Probe (active tip)"));	
                //		gtk_grid_attach (GTK_GRID (grid), label, x++,y, 1,1); 
		choice = gtk_combo_box_text_new ();
                ADD_LINE (label, choice);
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "Copper probe");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "Oxigen terminated probe (CO like)");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "Bromium terminated probe");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "Xenon terminated probe");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "Carbon terminated probe");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "Tungston terminated probe");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "Platinum terminated probe");
		g_signal_connect(G_OBJECT (choice), "changed", G_CALLBACK (ChangeValue), &(*probe_type));
		gtk_combo_box_set_active (GTK_COMBO_BOX (choice), 1);
		gtk_widget_show (choice);			
                //		gtk_grid_attach (GTK_GRID (grid), choice, x++, y, 1,1);


                ++y; x=1;
                label = gtk_label_new (N_("Probe Model:"));	
		gtk_widget_show (label);			
                //		gtk_grid_attach (GTK_GRID (grid), label, x++,y, 1,1); 
		choice = gtk_combo_box_text_new ();
                ADD_LINE (label, choice);
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "Spring");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "L-J");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "L-J + Coulomb");
		gtk_combo_box_text_append_tetx (GTK_COMBO_BOX_TEXT (choice), "L-J + Spring");
		gtk_combo_box_text_append_tetx (GTK_COMBO_BOX_TEXT (choice), "L-J + Coulomb + Spring");
		g_signal_connect(G_OBJECT (choice), "changed", G_CALLBACK (ChangeValue), &(*probe_model));
		gtk_combo_box_set_active (GTK_COMBO_BOX (choice), 1);
		gtk_widget_show (choice);			
                //		gtk_grid_attach (GTK_GRID (grid), choice, x++, y, 15,1);

                ++y; x=1;
                ADD_EC(N_("Initial Probe below Aplex (0=auto)"),      gapp->xsm->Z_Unit, "Value not allowed.", &initial_probe_below_apex,  -20.,  20., ".3f", 0.1,  1., NULL);
               ++y; x=1;
               ADD_EC(N_("Probe Charge"),      pcharge, "Value not allowed.", &probe_charge,  -2.,    2., ".3f", 0.01,  0.1, NULL);

                ++y; x=1;
                APP_EC(N_("Charge Scaling"),      gapp->xsm->Unity, "Value not allowed.", &charge_scaling,     -100.,  100., ".3f", 0.1,  1., NULL);
                ++y; x=1;
		ADD_EC(N_("Max Thread #"),      gapp->xsm->Unity, "Value not allowed.", &max_threads,    1.,   64., ".0f", 1.,  4., NULL);

                ++y; x=1;
                label = gtk_label_new (N_("Progress Detail"));	
		gtk_widget_show (label);			
                //		gtk_grid_attach (GTK_GRID (grid), label, x++,y, 1,1); 
		choice = gtk_combo_box_text_new ();
                ADD_LINE (label, choice);
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "Basic");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "Combined Job");
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (choice), "Job Detail");
		g_signal_connect(G_OBJECT (choice), "changed", G_CALLBACK (ChangeValue), &(*progress_detail));
		gtk_combo_box_set_active (GTK_COMBO_BOX (choice), 1);

		gtk_widget_show_all (vbox);			
                //		gtk_grid_attach (GTK_GRID (grid), choice, x++, y, 1,1);

		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy (dialog);

                delete meVA;
                delete pcharge;
                delete hertz;
                delete stiffness;
#endif
        }

	~Dialog () {};
};

// Sader Formula
// Citation: Applied Physics Letters 84, 1801 (2004); doi: 10.1063/1.1667267
// View online: http://dx.doi.org/10.1063/1.1667267
// -------------------------------------------------------------------------
// F_small(z)  F=Force W=Freq
// Omega(z) = Delta W(z) / W_res
// k=Spring Const Sensor  -- QPlus is approx 1800 N/m
// F_small(z) = 2k Integral(zi)_z->inf dzi W(zi)

// reverse Sader -- not needd here
double sader_Force(double z, double k=1800.){
        // need to integrate...
        return 1.;
}

// differential Sader -- small amplitude
// W(z) = diff_z  F_small(z)/(2k)
double sader_dFrequency (double z, const double Pz[3], const double Fz[3], double k=1800., double W_res=29000.){
        double dz = 0.5 * (Pz[2]-Pz[0]);
        return W_res * 0.5*(Fz[2]-Fz[0]) / (dz * 2.*k);
}

// need pN/Ang =>  1e-12N/1e-10m  =>  1e-2 N/m  <== N/n  x 100
// R = A-P --> Rz-dz > 0 for P "extended" from exquiv
// F=-kx, U=1/2kx^2
void probe_flex_force (const double r[3], double f[3], double g[3][3], xyzc_model *model){
        copy_vec (f, r);
        f[2] -= fabs (model->get_probe_dz ()); // this is negative for "close" (pushing away) Z
        mul_vec_vec (f, model->probe_flex);
        neg_vec (f); // invert for force respose: positive for repulsive now!
        if (g)
                for (int i=0; i<3; ++i){
                        copy_vec (g[i], f);
                        g[i][i] = model->probe_flex[i];
                }
}

/*
// LJ Force, R = "Apx - P" for apex-probe or "Amod - P" for model-probe
 */

inline void lj_force (double R[3], double A, double B, double Fab[3]){
        // compute Lennard-Jones Force between two atoms A,B, spaced by R:
        // F_L-J = R*(12*B/r^14 - 6*A/r^8)
        double r = norm_vec(R);
        double r2 = r*r;
        double r4 = r2*r2;
        double r8 = r4*r4;
        double r14 = r8*r4*r2;
        const double C_pN = 6 * 1.60217656535;  // to give result in pN
        copy_vec(Fab, R);
        // mul_vec_scalar(Fab, 12.0*B/r14 - 6.0*A/r8); // ==> in meV * 1e-10m / (1e-10m)^2 =  1.6021766e-12 kg m / s^2 ~=~ 1.6e-12 N = 1.6 pN
        mul_vec_scalar(Fab, C_pN * (2.*B/r14 - A/r8)); // ==> pN -- positive is repulsive (close), negative is attarctive (far)
}

// no need, not used, doing it numericall.... -- draft code only!
inline void lj_force_and_gradient (double R[3], double A, double B, double Fab[3], double dFab[3]){
        // compute Lennard-Jones Force between two atoms A,B, spaced by R:
        // F_L-J = R*(12*B/r^14 - 6*A/r^8)
        double r = norm_vec(R);
        double r2 = r*r;
        double r4 = r2*r2;
        double r8 = r4*r4;
        double r14 = r8*r4*r2;
        const double C_pN = 6 * 1.60217656535;  // to give result in pN
        copy_vec(Fab, R);
        copy_vec(dFab, R);
        // mul_vec_scalar(Fab, 12.0*B/r14 - 6.0*A/r8); // ==> in meV * 1e-10m / (1e-10m)^2 =  1.6021766e-12 kg m / s^2 ~=~ 1.6e-12 N = 1.6 pN
        mul_vec_scalar(Fab, C_pN * (2.*B/r14 - A/r8)); // ==> pN
        mul_vec_scalar(dFab, C_pN * (28.*B/r14 - 8.*A/r8)/r); // ==> pN/A
}

/*
// |\mathbf F|=k_e{|q_1q_2|\over r^2}\qquad and {\displaystyle \qquad \mathbf {F} _{1}=k_{e}{\frac {q_{1}q_{2}}{{|\mathbf {r} _{12}|}^{2}}}\mathbf {\hat {r}} _{21},\qquad } {\displaystyle \qquad \mathbf {F} _{1}=k_{e}{\frac {q_{1}q_{2}}{{|\mathbf {r} _{12}|}^{2}}}\mathbf {\hat {r}} _{21},\qquad } respectively,
// where ke is Coulomb's constant (ke = 8.9875517873681764×109 N m2 C−2), q1 and q2 are the signed magnitudes of the charges
// partial charge component based on Coulomb point charges at atom centers
// ==================================================
// R in Ang, q1,2 in e => pN
*/
inline void coulomb_force (const double R[3], double q1, double q2, double Fab[3]){
        // compute Coulomb Force between two atoms at R1 and R2
        // F_12 = kc q1 q2 R12 / r12^2
        double r = norm_vec(R);
        double r2 = r*r;
        const double keA=CONST_ke_ee; // ke for C in e, r in Ang ==> pN
        copy_vec(Fab, R);
        mul_vec_scalar(Fab, keA*q1*q2/r2/r);
}


// L-J, Coulobm, Flex Probe-Apex and Probe-Surface object Force calculation for
// -------------------------------------------------------------
// param->A: Apex: tip pos (fixed)
// param->model: Model and options...
// P: Probe Atom -- to be optimized
// param->Fsum: residual force -- to be minimized
// returns Fz, Z-projected force on Apex
// new: R_LJF_cutoff 
double calculate_apex_probe_and_probe_model_forces (LJ_calc_params *param, const double P[3]){
        double R[4];
        double F[3];
        double AB[3];
        double R1[4], R2[4];
        //        double R_LJF_cutoff = GLOBAL_R_CUTOFF;
        //        if (param->A[2] > R_LJF_cutoff) // auto adjust for far field if high above
        //               R_LJF_cutoff += param->A[2];
        
        copy_vec (R, param->A); R[3]=0.; // Probe Charge
        sub_from_vec (R, P);   // R = A-P;
        Model_item *m = param->model->get_model_ref ();
        Model_item *p = param->model->get_probe_ref ();

        // move out of here!!!!
        set_vec (param->Fa_lj, 0.);
        set_vec (param->Fa_c, 0.);
        set_vec (param->Fa_flex, 0.);        

        // Force on Probe to Tipapex -- L-J force componet
        // cAab(LJp_ta, LJp_p, AB); // LJ params tipapex <-> probe
        if (param->model->options & MODE_PROBE_L_J)
                lj_force(R, param->model->LJp_AB_lookup[0][0], param->model->LJp_AB_lookup[0][1], param->Fa_lj);    // R:= A-P

        // Force on Probe to Tipapex -- coulomb force componet
        if (param->model->options & MODE_PROBE_COULOMB)
                coulomb_force (R, p[1].xyzc[3], p[0].xyzc[3], param->Fa_c); // q1, q2 -> qP, qA, R:= A-P

        // Force on Probe to Tipapex -- flex force componet
        if (param->model->options & MODE_PROBE_SPRING)
                probe_flex_force (R, param->Fa_flex, NULL, param->model);

        copy_vec (param->Fa, param->Fa_lj);
        add_to_vec (param->Fa, param->Fa_c);
        add_to_vec (param->Fa, param->Fa_flex);        

        // Force on Probe to model/surface -- LJ and Coulomb components
        // sum Forces to surface atoms/molecules/..
        // Fsum: total F-LJ + F-Coulomb
        // Fcsum: F-Coulomb on Probe
        set_vec (param->Fljsum, 0.);
        set_vec (param->Fcsum, 0.);
        set_vec (param->Fsum, 0.);
        for (int n=0; n < param->model->get_size (); ++n){
                copy_vec (R, m[n].xyzc);
                // cAab (&mol[n][5], LPp_p, AB); // LJ params atom <-> probe
                sub_from_vec (R, P);  // R := R_atom - P
                // if (norm_vec (R) < R_LJF_cutoff){ // generating atrifacts
                //g_message ("calculate_apex_probe_and_probe_model_forces:: lj_force[%d]",n);
                //g_message ("m[%d]=%d",n, m[n].N);
                //g_message ("LJp-AB0=%g, LJp_AB1=%g",param->model->LJp_AB_lookup[m[n].N][0], param->model->LJp_AB_lookup[m[n].N][1]);
                lj_force(R, param->model->LJp_AB_lookup[m[n].N][0], param->model->LJp_AB_lookup[m[n].N][1], F);
                add_to_vec (param->Fljsum, F);
                if (param->model->options & MODE_COULOMB){
                        coulomb_force(R, p[1].xyzc[3], m[n].xyzc[3], F);
                        add_to_vec (param->Fcsum, F);
                }
                // }
        }
        // sum all forces -- to be minimized
        add_to_vec (param->Fsum, param->Fljsum);
        add_to_vec (param->Fsum, param->Fcsum);

        // separate Apex force "Z" component
        double Fz_probe = param->Fsum[2];
        add_to_vec (param->Fsum, param->Fa);

        return Fz_probe;
}

#define VEC_L(X,I) ((int)(X)+i)
#define dI_G_VEC_L(X,I) ((int)(X)+3*i)

// same as above, but using precomuted L-J and Coulomb force fields for model
double calculate_apex_probe_and_probe_model_forces_interpolated (LJ_calc_params *param, const double P[3]){
        double R[4];
        double F[3];
        double AB[3];
        double R1[4], R2[4];
        copy_vec (R, param->A); R[3]=0.; // Probe Charge
        sub_from_vec (R, P);   // R = A-P;
        Model_item *m = param->model->get_model_ref ();
        Model_item *p = param->model->get_probe_ref ();
        
        // move out of here!!!!
        set_vec (param->Fa_lj, 0.);
        set_vec (param->Fa_c, 0.);
        set_vec (param->Fa_flex, 0.);        

        // Force on Probe to Tipapex -- L-J force componet
        if (param->model->options & MODE_PROBE_L_J)
                // cAab(LJp_ta, LJp_p, AB); // LJ params tipapex <-> probe
                lj_force(R, param->model->LJp_AB_lookup[0][0], param->model->LJp_AB_lookup[0][1], param->Fa_lj);    // R:= A-P

        // Force on Probe to Tipapex -- coulomb force componet
        if (param->model->options & MODE_PROBE_COULOMB)
                coulomb_force (R, p[1].xyzc[3], p[0].xyzc[3], param->Fa_c); // q1, q2 -> qP, qA, R:= A-P
        
        // Force on Probe to Tipapex -- flex force componet
        if (param->model->options & MODE_PROBE_SPRING)
                probe_flex_force (R, param->Fa_flex, NULL, param->model);
        
        copy_vec (param->Fa, param->Fa_lj);
        add_to_vec (param->Fa, param->Fa_c);
        add_to_vec (param->Fa, param->Fa_flex);        

        // sum Forces to surface atoms/molecules/..
        // Fsum: total F-LJ + F-Coulomb
        // Fcsum: F-Coulomb on Probe
        set_vec (param->Fsum, 0.);
        
        // replaced sum over model with interpolated force field of force at P[] (probe particle) position in force volume
        double dix, diy, diz;
        param->scan->World2Pixel (P[0], P[1], dix, diy);
        diz = P[2] + param->model->get_probe_dz (); // compensate!
        param->Fljsum[0] = param->scan->mem2d->GetDataPktInterpol (dix, diy, PROBE_FX_FIELD_L, P[2], param->scan, param->dz, param->z0);
        param->Fljsum[1] = param->scan->mem2d->GetDataPktInterpol (dix, diy, PROBE_FY_FIELD_L, P[2], param->scan, param->dz, param->z0);
        param->Fljsum[2] = param->scan->mem2d->GetDataPktInterpol (dix, diy, PROBE_FZ_FIELD_L, P[2], param->scan, param->dz, param->z0);
        add_to_vec (param->Fsum, param->Fljsum);

        if (param->model->options == MODE_COULOMB){
                param->Fcsum[0] = param->scan->mem2d->GetDataPktInterpol (dix, diy, PROBE_FCX_FIELD_L, P[2], param->scan, param->dz, param->z0);
                param->Fcsum[1] = param->scan->mem2d->GetDataPktInterpol (dix, diy, PROBE_FCY_FIELD_L, P[2], param->scan, param->dz, param->z0);
                param->Fcsum[2] = param->scan->mem2d->GetDataPktInterpol (dix, diy, PROBE_FCZ_FIELD_L, P[2], param->scan, param->dz, param->z0);
                add_to_vec (param->Fsum, param->Fcsum);
        }
#if 0
        std::cout << "LJ-INTERPOL-F dixyz=" << dix << ", " << diy << ", " << P[2]
                  << " FLJinter=[" << param->Fljsum[0] 
                  << ", " << param->Fljsum[1] 
                  << ", " << param->Fljsum[2] 
                  << "]" << std::endl;
#endif
        
        // sum all forces -- to be minimized

        // separate Apex force "Z" component
        double Fz_probe = param->Fsum[2];
        add_to_vec (param->Fsum, param->Fa);

        return Fz_probe;
}

// FIXME!!
// simple current sum -- only via distances
double current_sum (const double A[3], const double P[3], double minVec[3], xyzc_model *model){
        double R[4];
        double I_sum;
        double r,I;
        double rmin=1000.;
        Model_item *m = model->get_model_ref ();
        Model_item *p = model->get_probe_ref ();
        
        const double Bias = 0.1;
        const double C1 = CONST_e * Bias;
        const double C2 = -2. *  sqrt (2. * CONST_me) / CONST_hbar;

        // some crude trivial STM current contributions sum up to surface atoms/molecules/..
        I_sum = 0.;
        for (int n=0; m[n].N > 0; ++n){
                copy_vec (R, m[n].xyzc);
                sub_from_vec (R, P);  // R = R_atom - P
                r = norm_vec (R) * 1e-9; // Ang -> m
                if (r < rmin) { rmin = r; copy_vec (minVec, R); }

                I = C1 * exp (C2*r*sqrt ( 0.5 * (model->WorkF_lookup[p[1].N] + model->WorkF_lookup[m[n].N]) + Bias)); // PhiTip - PhiModelAtom
                I_sum += I;
        }

        return I_sum;
}

// NL-OPT min function
double lj_residual_force (unsigned int n, const double *Popt, double *grad, void *data){
        LJ_calc_params *param = (LJ_calc_params*)(data);
        
        param->Fz = calculate_apex_probe_and_probe_model_forces (param, Popt);
        // copy_vec (param->Popt, Popt);
        
        param->residual_force_mag = norm_vec (param->Fsum);

        param->count++;
        
        return param->residual_force_mag;
}

// NL-OPT min function, provides gradient if requested - both from force fields, interpolated
double lj_residual_interpol_force (unsigned int n, const double *Popt, double *grad, void *data){
        LJ_calc_params *param = (LJ_calc_params*)(data);
        
        param->Fz = calculate_apex_probe_and_probe_model_forces_interpolated (param, Popt);
        
        param->residual_force_mag = norm_vec (param->Fsum);

        param->count++;

        return param->residual_force_mag;
}


void z_sader_run (Mem2d *m[3], int col, int line, double z, xyzc_model *model){
        double Fz[3], Fz_fix[3], Pz[3];
        double dz;
        if (m[1]->GetDataPkt (col, line, PROBE_NLOPT_ITER_L) < 0)
                return;

        for (int i=0; i<3; ++i){
                Fz[i]     = m[i]->GetDataPkt (col, line, APEX_FZ_L) * 1e-12; // now in Newton
                Fz_fix[i] = m[i]->GetDataPkt (col, line, APEX_FZ_FIXED_L) * 1e-12; // now in Newton
                Pz[i]     = m[i]->GetDataPkt (col, line, PROBE_Z_L) * 1e-10; // now in meter
        }
        // calc freq shift...
        m[1]->PutDataPkt (sader_dFrequency (z, Pz, Fz, model->sensor_f0_k0[1], model->sensor_f0_k0[0]),
                          col, line, FREQ_SHIFT_L);
        m[1]->PutDataPkt (sader_dFrequency (z, Pz, Fz_fix, model->sensor_f0_k0[1], model->sensor_f0_k0[0]),
                          col, line, FREQ_SHIFT_FIXED_L);
}

// force field calculations: LJ, Coulomb, Flex, Probe-Surface/Object
void z_probe_simple_force_run (Scan *Dest, Mem2d *work, int col, int line, double z, xyzc_model *model, gint gradflag=0){
        LJ_calc_params param;
        double xyz[3];
        double P[3], Popt[3];

        xyz[2] = z;
        Dest->Pixel2World (col, line, xyz[0], xyz[1]); // all in Angstroems
        work->PutDataPkt (xyz[0], col, line, PROBE_X_L);
        work->PutDataPkt (xyz[1], col, line, PROBE_Y_L);
        work->PutDataPkt (xyz[2], col, line, PROBE_Z_L);
        
        copy_vec (param.A, xyz);
        copy_vec (Popt, param.A); Popt[2] += model->get_probe_dz ();
        param.model = model;

        // calculate force at position xyz (param.A) with probe at Popt = (x,y,z - model->probe_dz)
        double Fz  = calculate_apex_probe_and_probe_model_forces (&param, Popt);
        double Fc  = norm_vec (param.Fcsum);
        double Flj = norm_vec (param.Fljsum);

        for (int i=0; i<3; ++i){
                work->PutDataPkt (Popt[i], col, line, VEC_L(PROBE_X_L, i));
                work->PutDataPkt (param.Fljsum[i], col, line, VEC_L(PROBE_FX_FIELD_L, i));
                work->PutDataPkt (param.Fcsum[i], col, line, VEC_L(PROBE_FCX_FIELD_L, i));
                work->PutDataPkt (param.Fa_flex[i], col, line, VEC_L(PROBE_FFX_FIELD_L, i));
                work->PutDataPkt (param.Fa_lj[i], col, line, VEC_L(PROBE_FLX_FIELD_L, i));
                work->PutDataPkt (param.Fsum[i], col, line, VEC_L(PROBE_FX_L, i));
        }
        work->PutDataPkt (norm_vec(param.Fsum), col, line, PROBE_FNORM_L);

        // Force "Fz" in eV * 1e-10m / (1e-10m)^2 =  1.6021766e-12N kg m / s^2 ~=~ 1.6e-12 N = 1.6 pN
        work->PutDataPkt (Fz, col, line, APEX_FZ_FIXED_L); // Unit: pN
        work->PutDataPkt (Flj,col, line, APEX_F_LJ_FIXED_L); // Unit: pN
        work->PutDataPkt (Fc, col, line, APEX_F_COULOMB_FIXED_L); // Unit: pN

        // fixed initial -- copy only
        work->PutDataPkt (Fz, col, line, APEX_FZ_L); // Unit: pN
        work->PutDataPkt (Flj,col, line, APEX_F_LJ_L); // Unit: pN
        work->PutDataPkt (Fc, col, line, APEX_F_COULOMB_L); // Unit: pN

        // single pass fixed mode
        work->PutDataPkt (1, col, line, PROBE_NLOPT_ITER_L); // # iterations

#ifdef PROBE_GRAD_INTER
        if (gradflag){
                LJ_calc_params param_d[2];
                copy_vec (param_d[0].A, param.A);
                copy_vec (param_d[1].A, param.A);
                param_d[0].model = model;
                param_d[1].model = model;

                double eps = 0.05;
                double eps2 = 1./(2.*eps);
                double Pd[3];

                // compute gradient matrix numerically
                for (int i=0; i<3; ++i){
                        copy_vec (Pd, Popt); Pd[i] -= eps;
                        calculate_apex_probe_and_probe_model_forces(&param_d[0], Pd);
                        copy_vec (Pd, Popt); Pd[i] += eps;
                        calculate_apex_probe_and_probe_model_forces(&param_d[1], Pd);

                        sub_from_vec (param_d[1].Fljsum,param_d[0].Fljsum);
                        mul_vec_scalar (param_d[1].Fljsum, eps2);

                        sub_from_vec (param_d[1].Fcsum,param_d[0].Fcsum);
                        mul_vec_scalar (param_d[1].Fcsum, eps2);

                        work->PutDataPkt (param_d[1].Fljsum[0], col, line, dI_G_VEC_L(PROBE_dxFX_L, i));
                        work->PutDataPkt (param_d[1].Fljsum[1], col, line, dI_G_VEC_L(PROBE_dxFY_L, i));
                        work->PutDataPkt (param_d[1].Fljsum[2], col, line, dI_G_VEC_L(PROBE_dxFZ_L, i));
                        
                        work->PutDataPkt (param_d[1].Fcsum[0],  col, line, dI_G_VEC_L(PROBE_dxFCX_L, i));
                        work->PutDataPkt (param_d[1].Fcsum[1],  col, line, dI_G_VEC_L(PROBE_dxFCY_L, i));
                        work->PutDataPkt (param_d[1].Fcsum[2],  col, line, dI_G_VEC_L(PROBE_dxFCZ_L, i));
                }
        }
#endif
}

// template/trial test, todo
void z_probe_topo_run (Scan *Dest, Mem2d *work, int col, int line, double z, xyzc_model *model){
        double xyz[3];
        double A[3], P[3], Popt[3];
        double minVec[3];

        xyz[2] = z;
        Dest->Pixel2World (col, line, xyz[0], xyz[1]); // all in Angstroems
        work->PutDataPkt (xyz[0], col, line, PROBE_X_L);
        work->PutDataPkt (xyz[1], col, line, PROBE_Y_L);
        work->PutDataPkt (xyz[2], col, line, PROBE_Z_L);
        
        copy_vec (A, xyz);
        copy_vec (Popt, A); Popt[2] += model->get_probe_dz ();
        
        double I = current_sum (A, Popt, minVec, model) * 1e9; // FIXME
        double d = norm_vec (minVec); // FIXME

        //testing
        work->PutDataPkt (I, col, line, PROBE_I_L);    // Unit: nA
        work->PutDataPkt (I, col, line, PROBE_CTIS_L); // Unit: nA/ddV
        work->PutDataPkt (d, col, line, PROBE_TOPO_L); // Unit: Ang

}

// template, todo
void z_probe_ctis_run (Scan *Dest, Mem2d *work, int col, int line, double z, xyzc_model *model){
        double xyz[3];
        double A[3], P[3], Popt[3];
        double minVec[3];

        xyz[2] = z;
        Dest->Pixel2World (col, line, xyz[0], xyz[1]); // all in Angstroems
        work->PutDataPkt (xyz[0], col, line, PROBE_X_L);
        work->PutDataPkt (xyz[1], col, line, PROBE_Y_L);
        work->PutDataPkt (xyz[2], col, line, PROBE_Z_L);
        
        copy_vec (A, xyz);
        copy_vec (Popt, A); Popt[2] += model->get_probe_dz ();
        
        double I = current_sum(A, Popt,  minVec, model); // FIXME
        double d = norm_vec (minVec); // FIXME
        
        work->PutDataPkt (I, col, line, PROBE_I_L);    // Unit: nA
        work->PutDataPkt (I, col, line, PROBE_CTIS_L); // Unit: nA/ddV
        work->PutDataPkt (d, col, line, PROBE_TOPO_L); // Unit: Ang

}

int fire_handle=0;
#define FDBG 0

/*
FIRE
==========
Settings

Default parameters of FIRE algorithm are good for most of the cases and it is not necessary to set them manually. The only parameter which should be set in fireball.in is time step dt = 0.5 - 1.0 femtosecond.

In case you want to try your luck and play with the parameters of algorithm it is optionally possible to provide file 'FIRE.optional' of the folowing format (with default parameters as examples):

1.1  ! FIRE_finc   ... increment time step if dot(f,v) is positive
0.5  ! FIRE_fdec   ... decrement time step if dot(f,v) is negative
0.1  ! FIRE_acoef0 ... coefficient of skier force update 
0.99 ! FIRE_falpha ... decrementarion of skier force compoenent acoef if projection dot(f,v) is positive
5    ! FIRE_Nmin   ... currently not used
4.0  ! FIRE_mass   ... mass of atoms  
Pseudo Code

Evaluate force
Evaluate projection of force to velocity vf = dot(v,f); vv = dot(v,v); ff = dot(f,f);

if (vf<0)
 v=0
 FIRE_dt = FIRE_dt * FIRE_fdec
 FIRE_acoef = FIRE_acoef0

else if ( vf>0 )

 cF = FIRE_acoef * sqrt(vv/ff)
 cV = 1 - FIRE_acoef
 v = cV * v + cF * f
 FIRE_dt = min( FIRE_dt * FIRE_finc, FIRE_dtmax )
 FIRE_acoef = FIRE_acoef * FIRE_falpha

MD step using leap-frog
 v = v + (dt/FIRE_mass) * f
 position = position + FIRE_dt * v

References

Bitzek, E., Koskinen, P., Gähler, F., Moseler, M. & Gumbsch, P. Structural relaxation made simple. Phys. Rev. Lett. 97, 170201 (2006).
Eidel, B., Stukowski, A. & Schröder, J. Energy-Minimization in Atomic-to-Continuum Scale-Bridging Methods. Pamm 11, 509–510 (2011).
FIRE: Fast Inertial Relaxation Engine for Optimization on All Scales
 */
class fire_opt {
public:
        fire_opt (double dt_init, double dt_max){
                fh = fire_handle++;
                finc = 1.1;    //  ! FIRE_finc   ... increment time step if dot(f,v) is positive
                fdec = 0.5;    //  ! FIRE_fdec   ... decrement time step if dot(f,v) is negative
                acoef0 = 0.1;  //  ! FIRE_acoef0 ... coefficient of skier force update 
                falpha = 0.99; //  ! FIRE_falpha ... decrementarion of skier force compoenent acoef if projection dot(f,v) is positive
                Nmin = 5;      //  ! FIRE_Nmin   ... currently not used
                mass = 4.0;    //  ! FIRE_mass   ... mass of atoms          
                set_vec (v, 0.);
                acoef=0.;
                dt = dt_init;
                dtmax = dt_max;
        };
        ~fire_opt (){
        };

        int run (double res_force, double Popt[3], void *data, int mode=0, int nmax=1000){
                while (--nmax > 0 && step (Popt, data, mode) > res_force);
                return nmax;
        };
        
        double step (double Popt[3], void *data, int mode=0){
                LJ_calc_params *param = (LJ_calc_params*)(data);
                
                // Evaluate force
                if (mode == 0)
                        param->Fz = calculate_apex_probe_and_probe_model_forces (param, Popt);
                else
                        param->Fz = calculate_apex_probe_and_probe_model_forces_interpolated (param, Popt);

                // copy_vec (param->Popt, Popt);
                param->residual_force_mag = norm_vec (param->Fsum);

                if (FDBG && fh==0) g_message   ("FIRE: #=%d  dt=%g", param->count, dt);
                if (FDBG && fh==0) g_print_vec ("FIRE: Fsum", param->Fsum);
                
                // Evaluate projection of force to velocity vf = dot(v,f); vv = dot(v,v); ff = dot(f,f);
                double vf = dot_prod (v, param->Fsum);
                double vv = dot_prod (v, v);
                double ff = dot_prod (param->Fsum, param->Fsum);
                if (FDBG && fh==0) g_message   ("FIRE: vf=%g vv=%g ff=%g", vf, vv, ff);

                if (FDBG && fh==0) g_print_vec ("FIRE: vi=", v);
                if (FDBG && fh==0) g_print_vec ("FIRE: A", param->A);
                if (FDBG && fh==0) g_print_vec ("FIRE: Popti", Popt);
 
                if (vf <= 0.){ // >
                        set_vec (v, 0.);
                        dt *= fdec;
                        acoef = acoef0;
                } else {
                        cF = acoef * sqrt(vv/ff);
                        cV = 1.0 - acoef;
                        
                        //v = cV * v + cF * f;
                        mul_vec_scalar (v, cV);
                        double tmpf[3];
                        copy_vec (tmpf, param->Fsum);
                        mul_vec_scalar (tmpf, cF);
                        add_to_vec (v, tmpf);
                        double tmp = dt * finc;
                        if (tmp < dtmax)
                                dt = tmp;
                        else
                                dt = dtmax;
                        acoef *= falpha;
                }
                
                // MD step using leap-frog
                // v += dt/mass * f;
                double dv[3];
                copy_vec (dv, param->Fsum);
                mul_vec_scalar (dv, dt/mass);
                add_to_vec (v, dv);
                if (FDBG && fh==0) g_print_vec ("FIRE: dv", dv);
                
                // r += dt*v;
                double dr[3];
                copy_vec (dr, v);
                mul_vec_scalar (dr, -dt);
                add_to_vec (Popt, dr);
                if (FDBG && fh==0) g_print_vec ("FIRE: dr", dr);
                if (FDBG && fh==0) g_print_vec ("FIRE: vf", v);
                if (FDBG && fh==0) g_print_vec ("FIRE: Poptf", Popt);

                param->count++;

                if (FDBG && fh==0) if (param->count > 160) exit (0);
                
                return (param->residual_force_mag);
        };

        int fh;
        double dtmax;
        double finc;   // = 1.1;     ! FIRE_finc   ... increment time step if dot(f,v) is positive
        double fdec;   // = 0.5;     ! FIRE_fdec   ... decrement time step if dot(f,v) is negative
        double acoef0; // = 0.1;     ! FIRE_acoef0 ... coefficient of skier force update 
        double falpha; // = 0.99;    ! FIRE_falpha ... decrementarion of skier force compoenent acoef if projection dot(f,v) is positive
        int Nmin;      // = 5;       ! FIRE_Nmin   ... currently not used
        double mass;   // = 4.0;     ! FIRE_mass   ... mass of atoms          

private:
        double dt, acoef, cF, cV;
        double v[3];
};

#define USE_FIRE // new, see above class, simple and "fire" fast :)

// full tip/probe force relaxation
void z_probe_run (Scan *Dest, Mem2d *work, Mem2d *m_prev, int col, int line, double z, double dz, double precision, int maxiter, xyzc_model *model){
        Mem2d *m = work;
#ifdef USE_NLOPT
        nlopt_opt opt;
#endif
        LJ_calc_params param;
        double Popt[3];
        double Fres, Fz, Fc, Flj;

        param.model  = model;
        param.scan = Dest;
        param.count  = 0;
        param.A[2] = z;
        Dest->Pixel2World (col, line, param.A[0], param.A[1]); // all coordinates in Angstroems

        // compute fixed scenario for reference
        copy_vec (Popt, param.A); Popt[2] += model->get_probe_dz ();

        Fz = calculate_apex_probe_and_probe_model_forces (&param, Popt);
        Fc = norm_vec (param.Fcsum);
        Flj = norm_vec (param.Fljsum);
        m->PutDataPkt (Fz, col, line, APEX_FZ_FIXED_L); // Unit: pN
        m->PutDataPkt (Fc, col, line, APEX_F_COULOMB_FIXED_L); // Unit: pN
        m->PutDataPkt (Flj, col, line, APEX_F_LJ_FIXED_L); // Unit: pN

        // start / continue probe tracing
        if (m_prev){ // continue tracing, just put us "dz" lower from previous found probe position as start 
                Popt[0] = m_prev->GetDataPkt (col, line, PROBE_X_L);
                Popt[1] = m_prev->GetDataPkt (col, line, PROBE_Y_L);
                Popt[2] = m_prev->GetDataPkt (col, line, PROBE_Z_L) - dz;
        } // else { // start trace probe pdz Ang below tip apex
                // copy_vec (Popt, param.A); Popt[2] -= pdz;
        //}

        if (m_prev){ // continue tracing? Abort if fzmax reached.
                if (fabs(m_prev->GetDataPkt (col, line, APEX_FZ_L)) > model->fzmax){
                        // copy and return
                        for (int k=FREQ_SHIFT_L; k<N_LAYERS; ++k)
                                m->PutDataPkt (m_prev->GetDataPkt (col, line, k), col, line, k);
                        m->PutDataPkt (-1., col, line, PROBE_NLOPT_ITER_L); // -1 : not calculated any more
                        return;
                }
        }
        
        double lb[3];
        double ub[3];
        double hrl[3];
        set_vec (hrl, 5.); // 4.
        copy_vec (lb, Popt); sub_from_vec (lb, hrl);
        copy_vec (ub, Popt); add_to_vec (ub, hrl);

#ifdef USE_NLOPT
        opt = nlopt_create (NLOPT_LN_COBYLA, 3); /* algorithm and dimensionality */
        // opt = nlopt_create (NLOPT_LD_MMA, 3); /* algorithm and dimensionality -- needs derivative !! */
        nlopt_set_lower_bounds (opt, lb);
        nlopt_set_upper_bounds (opt, ub);

        nlopt_set_min_objective (opt, lj_residual_force, &param);

        //        nlopt_set_xtol_rel (opt, precision);
        if (precision > 0.)
                nlopt_set_xtol_abs1 (opt, precision);
        if (maxiter > 0)
                nlopt_set_maxeval(opt, maxiter);

        double dx[3] = { 0.001, 0.001, 0.001 }; // double dx[3] = { 0.01, 0.01, 0.001 };
        nlopt_set_initial_step(opt, dx);

        // terminate via
        // nlopt_force_stop(opt);

        if (nlopt_optimize (opt, Popt, &Fres) < 0){
                lj_residual_force (0, Popt, NULL, &param); 
                printf("nlopt failed at A=(%g, %g, %g) Popt(%g, %g, %g) = %0.10g!\n", param.A[0], param.A[1], param.A[2], Popt[0], Popt[1], Popt[2], Fres);
        }
#else
        fire_opt fire (0.05, 10.0);
        if (fire.run (precision, Popt, &param) <= 0) // default mode 0: calc. LJ directly
        {
                lj_residual_force (0, Popt, NULL, &param); 
                g_message ("FIRE: OPT failed at A=(%g, %g, %g) Popt(%g, %g, %g) = %0.10g!\n", param.A[0], param.A[1], param.A[2], Popt[0], Popt[1], Popt[2], norm_vec (param.Fsum));
        }
#endif
        //else
                // double check result ???
                // lj_residual_force(0, Popt, NULL, &param);
        // store Position, ...
        for (int i=0; i<3; ++i){
                m->PutDataPkt (Popt[i], col, line, VEC_L(PROBE_X_L, i));
                m->PutDataPkt (param.Fljsum[i], col, line, VEC_L(PROBE_FX_FIELD_L, i));
                m->PutDataPkt (param.Fcsum[i], col, line, VEC_L(PROBE_FCX_FIELD_L, i));
                m->PutDataPkt (param.Fa_flex[i], col, line, VEC_L(PROBE_FFX_FIELD_L, i));
                m->PutDataPkt (param.Fa_lj[i], col, line, VEC_L(PROBE_FLX_FIELD_L, i));
                m->PutDataPkt (param.Fsum[i], col, line, VEC_L(PROBE_FX_L, i));
        }
        m->PutDataPkt (Fres, col, line, PROBE_FNORM_L);
        // Force "Fz" in meV * 1e-10m / (1e-10m)^2 =  1.6021766e-12 kg m / s^2 ~=~ 1.6e-12 N = 1.6 pN
        m->PutDataPkt (1.6022 * param.Fz, col, line, APEX_FZ_L); // Unit: pN

        // Coulomb contribution
        Fc = norm_vec (param.Fcsum);
        m->PutDataPkt (Fc, col, line, APEX_F_COULOMB_L); // Unit: pN
        Flj = norm_vec (param.Fljsum);
        m->PutDataPkt (Flj, col, line, APEX_F_LJ_L); // Unit: pN

        m->PutDataPkt ((double)param.count, col, line, PROBE_NLOPT_ITER_L); // # iterations
              
        if (0)
                printf("found minimum at Popt(%g,%g,%g) = %0.10g\n", Popt[0], Popt[1], Popt[2], Fres);

#ifdef USE_NLOPT
        nlopt_destroy (opt);
#endif
}

// full tip/probe force relaxation, use precomputed interpolated force field
void ipf_z_probe_run (Scan *Dest, Mem2d *work, Mem2d *m_prev, int col, int line, double z, double dz, double precision, int maxiter, xyzc_model *model){
        Mem2d *m = work;
        LJ_calc_params param;
        double Popt[3];
        double Fres, Fz, Fc, Flj;

        param.model  = model;
        param.scan = Dest;
        param.count  = 0;
        param.A[2] = z;
        Dest->Pixel2World (col, line, param.A[0], param.A[1]); // all coordinates in Angstroems

        // compute fixed scenario for reference
        copy_vec (Popt, param.A); Popt[2] += model->get_probe_dz ();

        Fz = calculate_apex_probe_and_probe_model_forces (&param, Popt);
        Fc = norm_vec (param.Fcsum);
        Flj = norm_vec (param.Fljsum);
        m->PutDataPkt (Fz, col, line, APEX_FZ_FIXED_L); // Unit: pN
        m->PutDataPkt (Fc, col, line, APEX_F_COULOMB_FIXED_L); // Unit: pN
        m->PutDataPkt (Flj, col, line, APEX_F_LJ_FIXED_L); // Unit: pN

        // start / continue probe tracing
        if (m_prev){ // continue tracing, just put us "dz" lower from previous found probe position as start 
                Popt[0] = m_prev->GetDataPkt (col, line, PROBE_X_L);
                Popt[1] = m_prev->GetDataPkt (col, line, PROBE_Y_L);
                Popt[2] = m_prev->GetDataPkt (col, line, PROBE_Z_L) - dz;
        }
        
        if (m_prev){ // continue tracing? Abort if fzmax reached.
                if (fabs(m_prev->GetDataPkt (col, line, APEX_FZ_L)) > model->fzmax){
                        // copy and return
                        for (int k=FREQ_SHIFT_L; k<N_LAYERS; ++k)
                                m->PutDataPkt (m_prev->GetDataPkt (col, line, k), col, line, k);
                        m->PutDataPkt (-1., col, line, PROBE_NLOPT_ITER_L); // -1 : not calculated any more
                        return;
                }
        }
        
        double lb[3];
        double ub[3];
        double hrl[3];
        set_vec (hrl, 5.); // 4.
        copy_vec (lb, Popt); sub_from_vec (lb, hrl);
        copy_vec (ub, Popt); add_to_vec (ub, hrl);

        fire_opt fire (0.05, 10.0);
        if (fire.run (precision, Popt, &param, 1) <= 0) { // mode=1 use interpolated field
                lj_residual_force (0, Popt, NULL, &param); 
                g_message ("FIRE INTER: OPT failed at A=(%g, %g, %g) Popt(%g, %g, %g) = %0.10g!\n", param.A[0], param.A[1], param.A[2], Popt[0], Popt[1], Popt[2], norm_vec (param.Fsum));
        }

        // store Position, ...
        for (int i=0; i<3; ++i){
                m->PutDataPkt (Popt[i], col, line, VEC_L(PROBE_X_L, i));
        }
        
        m->PutDataPkt (Fres, col, line, PROBE_FNORM_L);
        // Force "Fz" in meV * 1e-10m / (1e-10m)^2 =  1.6021766e-12 kg m / s^2 ~=~ 1.6e-12 N = 1.6 pN
        m->PutDataPkt (1.6022 * param.Fz, col, line, APEX_FZ_L); // Unit: pN

        // Coulomb contribution
        Fc = norm_vec (param.Fcsum);
        m->PutDataPkt (Fc, col, line, APEX_F_COULOMB_L); // Unit: pN
        Flj = norm_vec (param.Fljsum);
        m->PutDataPkt (Flj, col, line, APEX_F_LJ_L); // Unit: pN

        m->PutDataPkt ((double)param.count, col, line, PROBE_NLOPT_ITER_L); // # iterations
}


gpointer probe_optimize_thread (void *env){
        const int pkn=20;
	Probe_Optimize_Job_Env* job = (Probe_Optimize_Job_Env*)env;

        double pkt = (job->line_f-job->line_i)*job->work->GetNx ()/job->line_inc;
        int pk=0;
        for(int line=job->line_i; line<job->line_f; line+=job->line_inc)
                for(int col=0; col<job->work->GetNx (); ++col, ++pk){
                        z_probe_run (job->Dest, job->work, job->time_index > 0 ? job->Dest->mem2d_time_element (job->time_index-1) : NULL,
                                     col, line, job->z, job->dz, job->precision, job->maxiter, job->model);
                        if (! (pk % pkn)) {
                                job->progress = pk/pkt;
                                // std::cout << job->job << std::flush;
                                if (*(job->status)){
                                        job->job = -2; // aborted
                                        return NULL;
                                }
                        }
                }

	job->job = -1; // done indicator
	return NULL;
}

gpointer probe_optimize_ipf_thread (void *env){
        const int pkn=20;
	Probe_Optimize_Job_Env* job = (Probe_Optimize_Job_Env*)env;

        double pkt = (job->line_f-job->line_i)*job->work->GetNx ()/job->line_inc;
        int pk=0;
        for(int line=job->line_i; line<job->line_f; line+=job->line_inc){
                if (line == 0)
                        continue;
                if (line == job->work->GetNx ()-1)
                        break;
                for(int col=1; col<job->work->GetNx ()-1; ++col, ++pk){
                        ipf_z_probe_run (job->Dest, job->work, job->time_index > 0 ? job->Dest->mem2d_time_element (job->time_index-1) : NULL,
                                         col, line, job->z, job->dz, job->precision, job->maxiter, job->model);
                        if (! (pk % pkn)) {
                                job->progress = pk/pkt;
                                // std::cout << job->job << std::flush;
                                if (*(job->status)){
                                        job->job = -2; // aborted
                                        return NULL;
                                }
                        }
                }
        }

	job->job = -1; // done indicator
	return NULL;
}

gpointer probe_simple_thread (void *env){
        const int pkn=20;
	Probe_Optimize_Job_Env* job = (Probe_Optimize_Job_Env*)env;
        
        double pkt = (job->line_f-job->line_i)*job->work->GetNx ()/job->line_inc;
        int pk=0;
        for(int line=job->line_i; line<job->line_f; line+=job->line_inc)
                for(int col=0; col<job->work->GetNx (); ++col, ++pk){
                        z_probe_simple_force_run (job->Dest, job->work, col, line, job->z, job->model, job->gradflag);
                        if (! (pk % pkn)) {
                                job->progress = pk/pkt;
                                // std::cout << job->job << std::flush;
                                if (*(job->status)){
                                        job->job = -2; // aborted
                                        return NULL;
                                }
                        }
                }

	job->job = -1; // done indicator
	return NULL;
}

gpointer probe_topo_thread (void *env){
        const int pkn=20;
	Probe_Optimize_Job_Env* job = (Probe_Optimize_Job_Env*)env;
        
        double pkt = (job->line_f-job->line_i)*job->work->GetNx ()/job->line_inc;
        int pk=0;
        for(int line=job->line_i; line<job->line_f; line+=job->line_inc)
                for(int col=0; col<job->work->GetNx (); ++col, ++pk){
                        z_probe_topo_run (job->Dest, job->work, col, line, job->z, job->model);
                        if (! (pk % pkn)) {
                                job->progress = pk/pkt;
                                // std::cout << job->job << std::flush;
                                if (*(job->status)){
                                        job->job = -2; // aborted
                                        return NULL;
                                }
                        }
                }

	job->job = -1; // done indicator
	return NULL;
}

gpointer probe_ctis_thread (void *env){
        const int pkn=20;
	Probe_Optimize_Job_Env* job = (Probe_Optimize_Job_Env*)env;
        
        double pkt = (job->line_f-job->line_i)*job->work->GetNx ()/job->line_inc;
        int pk=0;
        for(int line=job->line_i; line<job->line_f; line+=job->line_inc)
                for(int col=0; col<job->work->GetNx (); ++col, ++pk){
                        // not implemented -- trival test calc only
                        z_probe_ctis_run (job->Dest, job->work, col, line, job->z, job->model);
                        if (! (pk % pkn)) {
                                job->progress = pk/pkt;
                                // std::cout << job->job << std::flush;
                                if (*(job->status)){
                                        job->job = -2; // aborted
                                        return NULL;
                                }
                        }
                }

	job->job = -1; // done indicator
	return NULL;
}



// force calculation for tip probe and apex, simple: no opt or using NL-OPT lib.
// time dimension is assigned to Z-apex position
// Dest: destination scan object
// z: current z of trace, dz: z-step
// current time_index, precision, iter limit
void image_run (Scan *Dest, double z, double dz, int time_index, double precision, int maxiter,
                xyzc_model *model,
                int &status, int calc_mode=MODE_NO_OPT, int pass=0, int max_jobs=4,
                gchararray progress_detail=NULL
                ){

        Probe_Optimize_Job_Env job[max_jobs];

        Mem2d *work=NULL;
        if (pass == 0)
                work = new Mem2d (Dest->mem2d, M2D_COPY);
        else
                work = Dest->mem2d_time_element (time_index);

        work->set_frame_time (z);
        
        std::cout << "AFM Sim Probe Opt M=" << calc_mode << " Z[" << time_index << "]=" << z << " spawing g_threads for pass=" << pass
                  << " Dest: " << Dest->mem2d->GetNx () << ", " << Dest->mem2d->GetNy () << ", " << Dest->mem2d->GetNv () 
                  << " work: " << work->GetNx () << ", " << work->GetNy () << ", " << work->GetNv ()  
                  << std::endl;

        GThread* tpi[max_jobs];
        
        // to be threadded:
        //	for (int yi=0; yi < Ny; yi++)...
        int lines_per_job = Dest->mem2d->GetNy () / (max_jobs-1);
        int line_i = 0;
        for (int jobno=0; jobno < max_jobs && jobno < Dest->mem2d->GetNy (); ++jobno){
                // std::cout << "Job #" << jobno << std::endl;
                job[jobno].Dest = Dest;
                job[jobno].work = work;
                job[jobno].line_i = jobno;
                job[jobno].line_inc = max_jobs;
                job[jobno].line_f = Dest->mem2d->GetNy ();
                job[jobno].time_index = time_index;
                job[jobno].z = z;
                job[jobno].dz = dz;
                job[jobno].precision = precision;
                job[jobno].maxiter = maxiter;
                job[jobno].model = model;
                job[jobno].status = &status;
                job[jobno].job = jobno+1;
                job[jobno].progress = 0.;
                job[jobno].verbose_level = 0;
                job[jobno].gradflag = 0;
                
                switch (calc_mode){
                case MODE_NL_OPT:
                        tpi[jobno] = g_thread_new ("probe_optimize_NL_OPT_thread", probe_optimize_thread, &job[jobno]);
                        break;
                case MODE_NO_OPT:
                        job[jobno].gradflag = 1;
                        tpi[jobno] = g_thread_new ("probe_simple_NO_OPT_thread", probe_simple_thread, &job[jobno]);
                        break;
                case MODE_IPF_NL_OPT:
                        switch (pass){
                        case 0:
                                job[jobno].gradflag = 1;
                                tpi[jobno] = g_thread_new ("probe_simple_NO_OPT_thread", probe_simple_thread, &job[jobno]);
                                break;
                        case 1:
                                tpi[jobno] = g_thread_new ("probe_optimize_IPF_NL_OPT_thread", probe_optimize_ipf_thread, &job[jobno]);
                                break;
                        }
                        break;
                case MODE_TOPO:
                        tpi[jobno] = g_thread_new ("probe_topo_thread", probe_topo_thread, &job[jobno]);
                        break;
                case MODE_CTIS:
                        tpi[jobno] = g_thread_new ("probe_ctis_thread", probe_ctis_thread, &job[jobno]);
                        break;
                case MODE_NONE:
                        job[jobno].job = -1;
                        tpi[jobno] = NULL;
                        break;
                }
        }
        
        // std::cout << "Waiting for all threads to complete." << std::endl;
        gapp->check_events ();
        
        if (progress_detail)
                if (!strncmp (progress_detail, "Basic", 5))
                        ;
                else if (!strncmp (progress_detail, "Combined", 8))
                        for (int running=1; running;){
                                running=0;
                                for (int jobno=0; jobno < max_jobs; ++jobno){
                                        if (job[jobno].job >= 0)
                                                running++;
                                        gapp->progress_info_set_bar_fraction (job[jobno].progress, 2);
                                        gchar *info = g_strdup_printf ("Job %d", jobno+1);
                                        gapp->progress_info_set_bar_text (info, 2);
                                        g_free (info);
                                        gapp->check_events ();
                                }
                        }
                else if (!strncmp (progress_detail, "Job Detail", 10))
                        for (int running=1; running;){
                                running=0;
                                for (int jobno=0; jobno < max_jobs; ++jobno){
                                        if (job[jobno].job >= 0)
                                                running++;
                                        gapp->progress_info_set_bar_fraction (job[jobno].progress, jobno+2);
                                        gchar *info = g_strdup_printf ("Job %d", jobno+1);
                                        gapp->progress_info_set_bar_text (info, jobno+2);
                                        g_free (info);
                                        gapp->check_events ();
                                }
                        }
        
        for (int jobno=0; jobno < max_jobs; ++jobno)
                if (tpi[jobno])
                        g_thread_join (tpi[jobno]);
        
        // std::cout << "Threads completed." << std::endl;

        if (pass == 0){
                Dest->append_current_to_time_elements (time_index, z, work);
                Dest->data.s.ntimes = time_index+1;
                
                delete work;
        }
        
        if (time_index > 2){
                std::cout << std::endl << "Sader Run -- dF compute @Z=" << z << std::endl;
                for(int line=0; line<Dest->mem2d->GetNy (); ++line)
                        for(int col=0; col<Dest->mem2d->GetNx (); ++col){
                                Mem2d *m[3];
                                for (int i=0; i<3; ++i)
                                        m[i] = Dest->mem2d_time_element (time_index-i);
                                z_sader_run (m, col, line, z, model);
                        }
        }
        
        std::cout << "AFM Sim for Z=" << z << " Ang complete. Pass=" << pass << std::endl;
}

// PlugIn run function, called by menu via PlugIn mechanism.
#ifdef GXSM3_MAJOR_VERSION // GXSM3 code
static gboolean afm_lj_mechanical_sim_run(Scan *Dest)
#else // GXSM2
static gboolean afm_lj_mechanical_sim_run(Scan *Src, Scan *Dest)
#endif
{
	static double verbose_level=0;
        xyzc_model *xyzc_model_filter = NULL;
	int stop_flag = 0;			// set to 1 to stop the plugin

        double zi=8.;  // default Z-start (upper box bound)
        double zf=4.5;   // default Z-end (lower box bound)
        double dz=0.1; // default z slice width
        double charge_scaling=1.; // default charge scaling for model xyzc -- if c given
        double precision=1e-6; // nl-opt precision
        double sensor_f0_k0[3] = {29000., 1800., 0. };  // Hz, N/m
        double probe_flex[3] = { 0.5, 0.5, 20. }; // probe stiffness x,y,z potential N/m
        double maxiter=0.; // default (0=none) nl-opt optional max iter limit
        double fzmax=1000.;
        double max_threads=g_get_num_processors (); // default concurrency for multi threadded computation, # CPU's/cores
        int calc_mode = MODE_NL_OPT; // MODE_NO_OPT; // default: no opt (static)
        int calc_opt  = MODE_L_J | MODE_COULOMB | MODE_PROBE_L_J | MODE_PROBE_COULOMB | MODE_PROBE_SPRING; // MODE_L_J | MODE_PROBE_SPRING; // default L-J only, no statics (coulomb), spring tip probe model
        
	PI_DEBUG (DBG_L2, "Afm_Mechanical_Sim Scan");
        std::cout << "AFM Mechanical Scan Simulation" << std::endl << std::flush;
        
        gchararray model_name = NULL;
        gchararray model_filter = NULL;
        gchararray model_filter_param = NULL;
        gchararray cmode   = NULL;
        gchararray copt    = NULL;
        gchararray external_model = NULL;
        gchararray probe_type = NULL;
        gchararray probe_model = NULL;
        double initial_probe_below_apex=4.0;  // 3.67;
        double probe_charge = -0.05;
        gchararray progress_detail = NULL;
        Dialog *dlg = new Dialog (zi, zf, dz, precision, maxiter, fzmax,
                                  sensor_f0_k0, probe_flex,
                                  &model_name, &model_filter, &model_filter_param,
                                  &cmode, &copt,
                                  &external_model,
                                  &probe_type, &probe_model,
                                  initial_probe_below_apex,
                                  probe_charge, charge_scaling,
                                  max_threads, &progress_detail);

        ostringstream info_stream;
        info_stream << "AFM Simulation PLugIn Run with:\n";

        // sanity check/fixes
        if (dz <= 0.)
                dz = 0.1;
        
        if (zi < zf){ // check, swap.
                double tmp=zi;
                zi=zf; zf=tmp;
        }
        
        // delete Dest->data.TimeUnit;
        // UTF8_ANGSTROEM "\303\205"
        // UnitObj *tmp_unit = new UnitObj("\303\205","\303\205", "g", "Z");
        UnitObj *tmp_unit =  gapp->xsm->MakeUnit ("AA", "Z");
	Dest->data.SetTimeUnit (tmp_unit);
        delete tmp_unit;

        tmp_unit =  gapp->xsm->MakeUnit ("Hz", "Frq. Shift");
	Dest->data.SetZUnit (tmp_unit);
        delete tmp_unit;
        
        info_stream << "dz = " << dz << " Ang\n";
        info_stream << " z = [" << zi << ", " << zf << "] Ang\n";
        info_stream << "precision = " << precision << " meV / maxiter = " << maxiter << "\n";

        xyzc_model *model = NULL;
        if (model_name){
                std::cout << "Model-id: >>" << model_name << "<<" << std::endl;
                if (!strncmp (model_name, "TMA", 3))
                        model = new xyzc_model (model_TMA);
                else if (!strncmp (model_name, "Cu111+TMA", 9))
                        model = new xyzc_model (model_TMA_Cu111);
                else if (!(model = new xyzc_model (model_name)))
                        return MATH_FILE_ERROR;
                info_stream << "Model = " << model_name << "\n";
        } else {
                model = new xyzc_model (model_TMA);
                info_stream << "Model = TMA -- build in fallback\n";
        }
        std::cout << "XYZ[C] Model:" << std::endl;
        model->print ();
        
        if (model_filter){
                xyzc_model_filter = new xyzc_model (model, model_filter, model_filter_param); // filter model, create new version
                std::cout << "XYZ Model after Filter: >>" << model_filter << "<<" << std::endl;
                xyzc_model_filter->print ();
                delete model;
                model = new xyzc_model (xyzc_model_filter, Dest); // swap to filtered model, apply bounding box with cutoff
                delete xyzc_model_filter;
                info_stream << "Model Filter = " << model_filter << "\n";
                if (model_filter_param)
                        info_stream << "Model Filter Param = " << model_filter_param << "\n";
                
                g_message ("XYZ[C] Model after filter and bb cutoff");
                model->print_xyz ();
        }

        if (probe_type){
                std::cout << "Probe Type: >>" << probe_type << "<<" << std::endl;
                if (!strncmp (probe_type, "Copper", 6))
                        model->make_probe (29, probe_charge, initial_probe_below_apex);
                else if (!strncmp (probe_type, "Oxigen", 6))
                        model->make_probe (8, probe_charge, initial_probe_below_apex);
                else if (!strncmp (probe_type, "Xenon", 5))
                        model->make_probe (54, probe_charge, initial_probe_below_apex);
                else if (!strncmp (probe_type, "Bromium", 7))
                        model->make_probe (35, probe_charge, initial_probe_below_apex);
                else if (!strncmp (probe_type, "Carbon", 6))
                        model->make_probe (6, probe_charge, initial_probe_below_apex);
                else if (!strncmp (probe_type, "Tungsten", 8))
                        model->make_probe (74, probe_charge, initial_probe_below_apex);
                else if (!strncmp (probe_type, "Platinum", 8))
                        model->make_probe (78, probe_charge, initial_probe_below_apex);
                else
                        model->make_probe ();
                info_stream << "Probe = " << probe_type << ", Probe-delta-charge = " << probe_charge << " e, -dzi=" << initial_probe_below_apex << " Ang\n"
                            << "Probe dz calculated = " << model->get_probe_dz () << " Ang\n";
        } else {
                info_stream << "Probe = fallback Oxigen default\n";
                model->make_probe ();
        }
        model->set_fzmax (fzmax);
        model->scale_charge (charge_scaling);
        info_stream << "Model Charge Scaling = " << charge_scaling << "\n";
        
        if (cmode){
                std::cout << "Calc mode: >>" << cmode << "<<" << std::endl;
                if (!strncmp (cmode, "NO-OPT", 6)) calc_mode = MODE_NO_OPT;
                else if (!strncmp (cmode, "NL-OPT", 6)) calc_mode = MODE_NL_OPT;
                else if (!strncmp (cmode, "IPF-NL-OPT", 10)) calc_mode = MODE_IPF_NL_OPT;
                else if (!strncmp (cmode, "TOPO", 4)) calc_mode = MODE_TOPO;
                else if (!strncmp (cmode, "CTIS", 4)) calc_mode = MODE_CTIS;
                else if (!strncmp (cmode, "NONE", 4)) calc_mode = MODE_NONE;
                info_stream << "Calculation Mode = " << cmode << " [" << calc_mode << "]\n";
        }

        if (copt){
                std::cout << "Calc options: >>" << copt << "<<" << std::endl;
                if (!strncmp (copt, "L-J Model", 9)) calc_opt = MODE_L_J;
                else if (!strncmp (copt, "L-J+Coulomb Model", 17)) calc_opt = MODE_L_J | MODE_COULOMB;
                info_stream << "Calculation Options = " << copt << " [" << calc_opt << "]\n";
        }

        if (probe_model){
                std::cout << "Probe Model: >>" << probe_model << "<<" << std::endl;
                if (strstr (probe_model, "Spring")) calc_opt |= MODE_PROBE_SPRING;
                if (strstr (probe_model, "L-J")) calc_opt |= MODE_PROBE_L_J;
                if (strstr (probe_model, "Coulomb")) calc_opt |= MODE_PROBE_COULOMB;
                info_stream << "Calculation Options Probe = " << probe_model << " [" << calc_opt << "]\n";
        }
       
        model->set_options (calc_opt);
        
        if (progress_detail){
                std::cout << "Progress: >>" << progress_detail << "<<" << std::endl;
                if (!strncmp (progress_detail, "Job Detail", 10))
                        gapp->progress_info_new ("NC-AFM Simulation", 1+(int)max_threads, GCallback (cancel_callback), &stop_flag, false);
                else if (!strncmp (progress_detail, "Basic", 5))
                        gapp->progress_info_new ("NC-AFM Simulation", 1, GCallback (cancel_callback), &stop_flag, false);
                else
                        gapp->progress_info_new ("NC-AFM Simulation", 2, GCallback (cancel_callback), &stop_flag, false);
        } else {
                gapp->progress_info_new ("NC-AFM Simulation", 2, GCallback (cancel_callback), &stop_flag, false);
        }

	gapp->progress_info_set_bar_fraction (0., 1);

        gchar *info = g_strdup_printf ("Z=%g " UTF8_ANGSTROEM, zi);
        gapp->progress_info_set_bar_text (info, 1);
        g_free (info);

        // cleanup if previously filled in time space
        Dest->free_time_elements ();
        
	Dest->data.s.nvalues=N_LAYERS;
	Dest->data.s.dz=1.0;

        time_t ts = time (0);
        info_stream << "Start Time = " << ts << "\n";
        
        Dest->data.ui.SetComment ((const gchar*)info_stream.str().c_str());
        Dest->data.ui.SetName ((const gchar*)model_name);

        if (calc_mode == MODE_NONE)
                stop_flag=1; // bypass calculations

        model->set_sim_zinfo (zi, zf, -dz);
        copy_vec (model->sensor_f0_k0, sensor_f0_k0);
        copy_vec (model->probe_flex, probe_flex);

        // adjust spring const unit to match pN / Ang from N/m [pN -> N 1e-12 /  Ang -> m 1e10 = x 1e-2 ] and set force direction ("centering" force)
        double adjust_spring_const[3] = { 100., 100., 100. };
        mul_vec_vec (model->probe_flex, adjust_spring_const);
        
        //        if (norm_vec (probe_flex) > 0)
        //                calc_opt |= MODE_PROBE_SPRING;

        model->set_options (calc_opt);
        
        int ti=0;
        double z;
        for (z=zi; z>zf && !stop_flag; z-=dz){

                // setup data storage for "time element" used for current z and assign labels to layers
                Dest->mem2d->Resize (Dest->mem2d->GetNx (), Dest->mem2d->GetNy (), N_LAYERS, ZD_DOUBLE);
                Dest->mem2d->data->MkVLookup(0., (double)N_LAYERS-1);

                Dest->mem2d->SetLayer (FREQ_SHIFT_L);            Dest->mem2d->add_layer_information (new LayerInformation ("FREQ SHIFT [Hz]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (FREQ_SHIFT_FIXED_L);      Dest->mem2d->add_layer_information (new LayerInformation ("FREQ SHIFT (Fixed, initial) [Hz]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (APEX_FZ_L);               Dest->mem2d->add_layer_information (new LayerInformation ("APEX FZ [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (APEX_F_LJ_L);             Dest->mem2d->add_layer_information (new LayerInformation ("APEX F LJ [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (APEX_F_COULOMB_L);        Dest->mem2d->add_layer_information (new LayerInformation ("APEX FZ COULOMB contribution [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (APEX_FZ_FIXED_L);         Dest->mem2d->add_layer_information (new LayerInformation ("APEX FZ (Fixed, initial) [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (APEX_F_LJ_FIXED_L);       Dest->mem2d->add_layer_information (new LayerInformation ("APEX F LJ (Fixed, initial) [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (APEX_F_COULOMB_FIXED_L);  Dest->mem2d->add_layer_information (new LayerInformation ("APEX FZ COULOMB (Fixed, initial) contribution [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_FNORM_L);           Dest->mem2d->add_layer_information (new LayerInformation ("PROBE F NORM (Error) [x1.6 pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_X_L);     Dest->mem2d->add_layer_information (new LayerInformation ("PROBE X [Ang]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_Y_L);     Dest->mem2d->add_layer_information (new LayerInformation ("PROBE Y [Ang]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_Z_L);     Dest->mem2d->add_layer_information (new LayerInformation ("PROBE Z [Ang]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_FX_FIELD_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE FX FIELD [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_FY_FIELD_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE FY FIELD [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_FZ_FIELD_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE FZ FIELD [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_FCX_FIELD_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE FCX FIELD [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_FCY_FIELD_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE FCY FIELD [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_FCZ_FIELD_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE FCZ FIELD [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_FFX_FIELD_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE FFX FIELD [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_FFY_FIELD_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE FFY FIELD [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_FFZ_FIELD_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE FFZ FIELD [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_FLX_FIELD_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE FLX FIELD [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_FLY_FIELD_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE FLY FIELD [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_FLZ_FIELD_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE FLZ FIELD [pN]", (double)z, "z=%06.3fA"));
#ifdef PROBE_GRAD_INTER
                Dest->mem2d->SetLayer (PROBE_dxFX_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE dxFX [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_dxFY_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE dxFY [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_dxFZ_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE dxFZ [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_dyFX_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE dyFX [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_dyFY_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE dyFY [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_dyFZ_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE dyFZ [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_dzFX_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE dzFX [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_dzFY_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE dzFY [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_dzFZ_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE dzFZ [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_dxFCX_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE dxFCX [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_dxFCY_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE dxFCY [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_dxFCZ_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE dxFCZ [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_dyFCX_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE dyFCX [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_dyFCY_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE dyFCY [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_dyFCZ_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE dyFCZ [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_dzFCX_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE dzFCX [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_dzFCY_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE dzFCY [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_dzFCZ_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE dzFCZ [pN]", (double)z, "z=%06.3fA"));
#endif
                Dest->mem2d->SetLayer (PROBE_FX_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE FX [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_FY_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE FY [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_FZ_L);    Dest->mem2d->add_layer_information (new LayerInformation ("PROBE FZ [pN]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_I_L);     Dest->mem2d->add_layer_information (new LayerInformation ("PROBE I [pA]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_CTIS_L);  Dest->mem2d->add_layer_information (new LayerInformation ("PROBE CTIS [pA/ddV]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_TOPO_L);  Dest->mem2d->add_layer_information (new LayerInformation ("PROBE TOPO Z@setpt [Ang]", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (PROBE_NLOPT_ITER_L);  Dest->mem2d->add_layer_information (new LayerInformation ("NLOPT ITERs", (double)z, "z=%06.3fA"));
                Dest->mem2d->SetLayer (FREQ_SHIFT_FIXED_L);
                
                // set progress info
                gapp->progress_info_set_bar_fraction ((z-zf)/(zi-zf), 1);
                info = g_strdup_printf ("pass1 ff Z=%g " UTF8_ANGSTROEM, z);
                gapp->progress_info_set_bar_text (info, 1);
                g_free (info);

                // run z slice pass 1 -- static field calculation or brute force nl-opt on exact L-J (more comp. intense) forces, etc.
                image_run (Dest, z, dz, ti++, precision, (int)maxiter, model, stop_flag, calc_mode, 0, (int)max_threads, progress_detail);
        }

        // 2nd pass for interpolated force field,.. run 
        if (calc_mode == MODE_IPF_NL_OPT){
                for (ti=1,z=zi-dz; z>zf+dz && !stop_flag; z-=dz){
                        Dest->retrieve_time_element (ti);
                        gapp->progress_info_set_bar_fraction ((z-zf)/(zi-zf), 1);
                        info = g_strdup_printf ("pass2 OPT Z=%g " UTF8_ANGSTROEM, z);
                        gapp->progress_info_set_bar_text (info, 1);
                        g_free (info);
                        // run z slice for pass 2 using pre computed force/gradient fields, interpolated
                        image_run (Dest, z, dz, ti++, precision, (int)maxiter, model, stop_flag, calc_mode, 1, (int)max_threads, progress_detail);
                }
        }

        // add stopped info?
        if (stop_flag)
                info_stream << "** Job canceld at z= " << z << "\n";

	gapp->progress_info_set_bar_text ("finishing up jobs", 1);

        time_t te = time (0);
        info_stream << "End Time = " << te << "\n";
        te -= ts;
        info_stream << "Calculation Time = " << te << " sec -- " << ctime (&te) << "\n";

	Dest->data.ui.SetComment ((const gchar*)info_stream.str().c_str());

	gapp->check_events ();

 	std::cout << "PI:run ** cleaning up." << std::endl;

        if (model)
                delete model;

	Dest->data.s.nvalues = Dest->mem2d->GetNv ();
 	Dest->retrieve_time_element (0);
	Dest->mem2d->SetLayer(0);

        gapp->progress_info_close ();

	return MATH_OK;
}
