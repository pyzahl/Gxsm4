/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: DSPProbe.C
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
% PlugInModuleIgnore

% BeginPlugInDocuSection
% PlugInDocuCaption: Digital probing -- STS and more (OBSOLETE)
% PlugInName: DSPProbe
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: windows-sectionDSP Probe

% PlugInDescription 
This control provides the user frontend for all local probing
methods, e.g. STS and Force-Distance curves. The probe modes are
highly configurable via \GxsmPref{Probe}{Probe/ModeNxxx} and need to
be configured before any probing is possible. There are up to four
individual probe modes configurable, they will appear as folders in
the probe control window, see configuration below.

Each folder works independent and stores/restores all parameters at
folder close/reopen. The probeing parameters itself are set and the
probe process can be started via pressing the button \GxsmEmph{Start}.

\GxsmScreenShot{ProbeControl-STS}{The Probe Control window, STS folder.}


% PlugInUsage
Let's try a simple IV-curve: Assuming the configuration is done right,
the corresponding IV curve folder has to be selected first. It is also
assumed the instrument is in a stable tunneling condition, say
U$=2\:$V and I$=1\:$nA. It is usually a good idea to start the probing
close or at the current voltage set to avoid a quick voltage change in
the beginning of the probing process. So set \GxsmEmph{Range: Start}
to $2\:$V and \GxsmEmph{End} to $-2\:$V.\\
\GxsmNote{The probing is always started at the start voltage and is
  linear ramped to the end voltage and will ramp from negative to
  positive as well, if set this way.}\\
Set the number of \GxsmEmph{Points} to acquire to 1000 and the number
of samples to be averaged \GxsmEmph{\#Avg} to 33 for example. Using
\GxsmEmph{Delay} the probing can be slowed down. It takes about
$50000\:$samples/s, depending on the current DSP program version
running.

Let \GxsmEmph{AC, Frq, Phase, CI} at zero for normal probing and do
not care about \GxsmEmph{GapAdj, \#Ch, ACMult}.  The digital Lock-In is
used if \GxsmEmph{AC, Frq} are non zero, \GxsmEmph{Phase} sets the
Lock-In phase relative to the modulation signal.

Press \GxsmEmph{Start}! The result will be shown in the
\GxsmEmph{Profile-View} (\ref{Gxsm-Profile}).

\GxsmScreenShot{ProbeControl-STS-AC}{The Probe Control window, STS-AC folder.}

\def\overlap{8}
\def\smpsin{128}

The digital Lock-In (AC-mode) can be used with the new DSP software
(xsm.out, starting with V1.19) version only.
To use the AC/Lock-In mode the AC parameters \GxsmEmph{AC, Frq, Phase, ACMult} 
have to be specified and \GxsmEmph{AC, Frq} have to be non zero. Set \GxsmEmph{ACMult} to 1.
\GxsmEmph{\#Ave} specifies the number of periods
(\smpsin\ samples/per period) to be used for integration inbetween data
points (\GxsmEmph{Points}). 
A fixed number of \overlap\ (defined in \GxsmFile{xsm/xsm\_conf.h}) 
overlaping sections (overlapping Lock-In data window) of length
of \GxsmEmph{\#Ave} periods is used for integration of the correlation function. 
Thus the total integration length (time constant $\tau$) is
\[ \tau = \frac{\text{Ave} \cdot \text{\overlap}}{\text{Frq}}. \]

The \GxsmEmph{AC: Amp} is the modulation amplitude,
\GxsmEmph{Frq} is the modulation frequency (up to $1100\:$Hz). If the 
\GxsmEmph{Phase} is given the digital Lock-In uses it for phase shifting the 
internal reference signal.

Lock-In calculation:
\[ N = \smpsin \cdot \text{Ave} \cdot \text{\overlap} \]
\[ \omega = 2 \pi \cdot \text{Frq} \]
\[ U_{\text{ref}} = \text{Amp} \cdot \sin(\omega t_i) \]
\[ U_{\text{out}} = \frac{2\pi}{N} \sum_{i=0 \dots N-1} \text{Amp} \cdot U_{\text{in},i} \cdot \sin(i \frac{2 \pi}{\smpsin} + \text{Phase}) \]

\GxsmNote{The phase resolution depends on samples/period and is $\frac{360}{\smpsin}^\circ$.}

\GxsmNote{If you need more resolution than the DAC provides, you can make use of statistically noise, 
which gives more resolution with increased integration/korrelation length and set the \GxsmEmph{ACMult} 
to about 10 get one magnitude more (this scales the data for transfer only, and it is corrected afterwards).}


%% OptPlugInSection: replace this by the section caption

%% OptPlugInSubSection: replace this line by the subsection caption

% OptPlugInConfig
The \GxsmPref{Probe}{Probe/\dots} offers a sophisticated probe setup,
the Mode1 entries of the preferences folder is shown here.
\GxsmScreenShot{PrefProbe}{The Probe Preference folder.}

For each mode \GxsmEmph{Mode$N$, $N\in\{1,2,3,4\}$} the following
settings have to be provided:

\begin{description}
\item[Name] This mode name appears as folder label of the probe
  control. If the name is just a dash (``-'') this mode will be
  disabled and does not show up as folder.
\item[Id] This id is used for identifying and files suffixes, it has to be
  alpha-numeric and without white spaces or special symbols. It should
  be as short as possible, but unique, such as ``IV''.
\item[Param] A descriptive folder caption text for this mode.
\item[Srcs] Configuration of data sources to be acquired. It is given
  in a binay representation: 4 binary digits (0/1) for PID sources
  (ADC1,2,3,4)
  and 8 digits for data input (ADC1,2,3,4, 5,6,7,8). (5\dots8 PC31 only)\\
  and 2 additional digits for additional DSP calculated data\\
  Example: ``10001000000010'' Feedback (I) on ADC1, probe data input is
  also ADC1 and one additional DSP calculated data set is expected 
  (e.g. avgeraged signal from LockIn Src).
\item[Outp] Configuration of DAC channel used for probe X-signal
  output, enter the a DAC number (DAC1=0, DAC2=1, DAC3=2, DAC4=3).
\item[XUnit] Select the physical X unit, typically ``V'' (Volts).
\item[XLabel] Give a short label for the X axis.
\item[XFactor] Configure a factor for calculation of the value in
  physical from digital units. Example: For a 16 bit DAC with
  $\pm10\:$V range (as used with the PCI32) a factor of 3276.7 will
  correctly convert from volts to digital units.
\item[XOffset] Usually zero, but could be used for corrections or
  adjustments. Fill in the correction value in physical units.
\item[YUnit] Select the physical Y unit, typically ``V'' (Volts) or ``nA''.
\item[YLabel] Give a short label for the Y axis.
\item[YFactor] Similar to the \GxsmEmph{XFactor}.
\item[YOffset] Similar to the \GxsmEmph{XOffset}.
\end{description}


% OptPlugInFiles
You can save the data as Ascii, as described in the profile section (\ref{Gxsm-Profile}).

%% OptPlugInKnownBugs

%% OptPlugInRefs
%The internal used fast fourier transform is based on the FFTW library:\\
%\GxsmWebLink{www.fftw.org}\\
%Especially here:\\
%\GxsmWebLink{www.fftw.org/doc/fftw\_2.html\#SEC5}

% OptPlugInNotes
The \GxsmEmph{Stop} button has no function at all.\\

% OptPlugInNotes
Depending on configured data exchange type (DSP software level, select
short or float) the maximum number of data points is limited to a
total size of of 3400 shorts:\\[1mm]
\begin{tabular}{l|r|r}
\# Channels & max \# short points  & max \# float points \\\hline
1 & 3400 & 1700 \\
2 & 1700 & 850 \\
\end{tabular}\\[1mm]
The dialog is not restricting the number of points, but you won't get
any result if the number entered is to big.

%% OptPlugInHints
%I have still a lot more\dots

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include "app_probe.h"

static void DSPProbe_about( void );
static void DSPProbe_query( void );
static void DSPProbe_cleanup( void );
static void DSPProbe_configure( void );

static void DSPProbe_show_callback( GtkWidget*, void* );

GxsmPlugin DSPProbe_pi = {
  NULL,
  NULL,
  0,
  NULL,
  "DSPProbe",
  "+spmHARD +comedispmHARD +srangerspmHARD +STM +AFM",
  NULL,
  "Percy Zahl",
  "windows-section",
  N_("DSP Probe"),
  N_("open the DSP probe controlwindow"),
  "DSP probe control",
  NULL,
  NULL,
  NULL,
  DSPProbe_query,
  DSPProbe_about,
  DSPProbe_configure,
  NULL,
  DSPProbe_cleanup
};

static const char *about_text = N_("Gxsm DSPProbe Plugin:\n"
                                   "This plugin runs a controls window to send "
                                   "general \"Probe-Requests\" to the DSP. "
                                   "Probemodes are used to run force-distance curves "
                                   "(AFM), or some STS (STM)."
                                   );

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  DSPProbe_pi.description = g_strdup_printf(N_("Gxsm DSPProbe plugin %s"), VERSION);
  return &DSPProbe_pi; 
}

static void DSPProbe_configure( void )
{
//      if( DSPProbeClass )
//              DSPProbeClass->configure()
}


DSPProbeControl *DSPProbeClass = NULL;

static void DSPProbe_query(void)
{
  static GnomeUIInfo menuinfo[] = { 
    { GNOME_APP_UI_ITEM, 
      DSPProbe_pi.menuentry, DSPProbe_pi.help,
      (gpointer) DSPProbe_show_callback, NULL,
      NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK, 
      0, GDK_CONTROL_MASK, NULL },

    GNOMEUIINFO_END
  };

  gnome_app_insert_menus(GNOME_APP(DSPProbe_pi.app->getApp()), DSPProbe_pi.menupath, menuinfo);

  // new ...
  
  DSPProbeClass = new DSPProbeControl
    ( DSPProbe_pi.app->xsm->hardware,
      & DSPProbe_pi.app->RemoteEntryList
      );

  DSPProbeClass->SetResName ("WindowProbeControl", "false", xsmres.geomsave);

  //  DSPProbe_pi.app->ConnectPluginToStartScanEvent
  //    ( DSPProbe_StartScan_callback );

  if(DSPProbe_pi.status) g_free(DSPProbe_pi.status); 
  DSPProbe_pi.status = g_strconcat(N_("Plugin query has attached "),
                          DSPProbe_pi.name, 
                          N_(": DSPProbe is created."),
                          NULL);
}

static void DSPProbe_about(void)
{
  const gchar *authors[] = { "Percy Zahl", NULL};
  gtk_show_about_dialog (NULL, "program-name",  DSPProbe_pi.name,
                                  "version", VERSION,
                                    "license", GTK_LICENSE_GPL_3_0,
                                    "comments", about_text,
                                    "authors", authors,
                                    NULL
                                    ));
}

static void DSPProbe_cleanup( void ){
  PI_DEBUG (DBG_L2, "DSPProbe Plugin Cleanup: removing MenuEntry" );
  gchar *mp = g_strconcat(DSPProbe_pi.menupath, DSPProbe_pi.menuentry, NULL);
  gnome_app_remove_menus (GNOME_APP( DSPProbe_pi.app->getApp() ), mp, 1);
  g_free(mp);

  PI_DEBUG (DBG_L2, "DSPProbe Plugin Cleanup: Window" );
  //  delete ...
  if( DSPProbeClass )
    delete DSPProbeClass ;

  DSPProbeClass = NULL;
  PI_DEBUG (DBG_L2, "DSPProbe Plugin Cleanup: done." );
}

static void DSPProbe_show_callback( GtkWidget* widget, void* data){
  if( DSPProbeClass )
    DSPProbeClass->show();
}
