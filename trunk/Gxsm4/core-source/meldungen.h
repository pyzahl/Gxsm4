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

#ifndef __MELDUNGEN_H
#define __MELDUNGEN_H

#include <glib.h>
#include <glib/gi18n.h>

#define XSM_CHECK_EVENTS        gapp->check_events()
#define XSM_SHOW_ALERT(A,B,C,D) gapp->alert(A,B,C,D)
#define XSM_SHOW_WARNING(A)     gapp->warning(A)
#define XSM_SHOW_ERROR(A,B)     gapp->error(A)
#define XSM_SHOW_MESSAGES(A)    gapp->message(A)
#define XSM_SHOW_CHOICE(A,B,C, D, E,F,G, H)    gapp->choice(A,B,C, D, E,F,G, H)
#define XSM_SHOW_QUESTION(A,B)    gapp->question_yes_no(A)


/* Meldungen */
#define MLD_WERT_NICHT_OK        N_("Value out of Range !")
#define MLD_VALID_RANGE          N_("Valid Range: ")
#define MLD_FATAL                N_("Program Error: Fatal")
#define MLD_PRINT                N_("PS - Dump done !")

/* Fehler */
#define ERR_FATAL                N_("Fatal Error !")
#define ERR_SORRY                N_("Sorry !")
#define ERR_NOMEM                N_("No more free Memory available !")
#define ERR_FILEREAD             N_("Can't read File !")
#define ERR_FILEWRITE            N_("Can't write File !")
#define ERR_DISKFULL             N_("No more Diskspace !")

/* Warnungen */
#define WRN_WARNING              N_("Warning !")
#define WRN_FILEEXISTS           N_("File exists !")

/* Labels */
#define L_YES                    N_("Yes")
#define L_NO                     N_("No")
#define L_ENTER                  N_("Enter")
#define L_OK                     N_("OK")
#define L_CANCEL                 N_("Cancel")
#define L_OVERWRITE              N_("Overwrite")
#define L_RETRY                  N_("Retry")


/* Fenster Titel */

#define CHANNELSELECTOR_TITLE    N_("Channel Selector")
#define DSP_CONTROL_TITLE        N_("DSP Control")
#define PROBE_CONTROL_TITLE      N_("Probe Control")
#define PROFILE_TITLE            N_("Line Profile")
#define HISTOGRAMM_TITLE         N_("Histogram")
#define MONITOR_TITLE            N_("Log/Monitor")
#define OPTI_TITLE               N_("Optimize")
#define OSZI_TITLE               N_("Oszi")
#define PRINT_TITLE              N_("Print")
#define REMOTE_TITLE             N_("Remote")
#define MOV_SLIDER_TITLE         N_("Slider Control")
#define MOV_MOVER_TITLE          N_("Mover Control")
#define BG_TITLE                 N_("Background")
#define F1D_TITLE                N_("1D Filter")
#define F2D_TITLE                N_("2D Filter")
#define TOOLS_TITLE              N_("Tools")
#define DISP_TITLE               N_("Manual Display Crt.")
#define SPA_TITLE                N_("SPA--LEED")
#define ANTIDRIFT_TITLE          N_("Anti Drift")


/* Meldungen */
#define MLD_WERT_NICHT_OK        N_("Value out of Range !")
#define MLD_VALID_RANGE          N_("Valid Range: ")
#define MLD_FATAL                N_("Program Error: Fatal")
#define MLD_NEED2PKTE            N_("I need two Points ! \ntype '1' then type '2' in D2D Scan")
#define MLD_PRINT                N_("PS - Dump done !")

/* Warnungen */
#define WRN_WARNING              N_("Warning !")
#define WRN_FILEEXISTS           N_("File exists !")

/* Hinweise */
#define HINT_WARN                N_("Warning !")
#define HINT_ACTIVATESCANMATH    N_("Aktivate Scan before doing math operations !")
#define HINT_MAKESRC2            N_("Set Channel X before doing math operations +-X,*/Y !")
#define HINT_SIZEEQ              N_("Make sure size of Active and Channel X or Y are equal !")
#define HINT_ACTIVATESCAN        N_("Aktivate Scan to save before doing save comand !")
#define HINT_UNAME_TRUNC         N_("User name longer than 39 Chars, will be truncated !")
#define HINT_COMMENT_TRUNC       N_("Comment longer than 255 Chars, will be truncated !")

/* Fragen */
#define Q_WANTQUIT               N_("Do you want to leave GXSM ?\0")
#define Q_WANTSAVE               N_("Do you want to save Scans ?\0")
#define Q_CLOSEWINDOW            N_("Close window ?\0")
#define Q_TITEL                  N_("Question")
#define Q_KILL_SCAN              N_("Really kill scan ?")
#define Q_DIRECTEXPORT           N_("Direct Export:")
#define Q_DIRECTMODE             N_("TGA Mapping Mode ?")
#define Q_KILL_REMOTE            N_("Remote is active, kill it ?")
#define Q_START_REMOTE           N_("Remote is now ready, start it ?")

/* Fehler */
#define ERR_USER                 N_("Error !")
#define ERR_MATH                 N_("Math Operation Error !")
#define ERR_NOACTIVESCAN         N_("No active Scan !")
#define ERR_NO2SRC               N_("No second Source !")
#define ERR_SIZEDIFF             N_("Scan sizes not equal !")
#define ERR_FATAL                N_("Fatal Error !")
#define ERR_SORRY                N_("Sorry !")
#define ERR_CPTMP                N_("Error copying gzipped file to /tmp/")
#define ERR_UNZIP                N_("Error unzipping file /tmp/")
#define ERR_NOMEM                N_("No more free Memory available !")
#define ERR_SCANMODE             N_("Data from unknown Scanmode !")
#define ERR_NOFREECHAN           N_("No free Channel available, all in use !")
#define ERR_NOSRCCHAN            N_("No source Channel available !")
#define ERR_NOHOME               N_("No Enviromententry for HOME !")
#define ERR_PALFILEREAD          N_("Can't read Palette !")
#define ERR_FILEREAD             N_("Can't read File !")
#define ERR_FILEWRITE            N_("Can't write File !")
#define ERR_DISKFULL             N_("No more Diskspace !")
#define ERR_NOGNUCPX             N_("No Import of CPX-Format !")
#define ERR_WRONGGNUTYPE         N_("Wrong GNU-File Type ! Please use .pgm, .byt, .sht, .flt or .dbl !")
#define ERR_HARD_MAXSCANPOINTS   N_("Hardware no more Points per Line than")
#define ERR_SCAN_CANCEL          N_("Scan start request canceled:")
#define ERR_NEWVERSION           N_("XSM Version changed, please save values again !")

/* Labels */
#define L_24bit                  N_("RGB-24bit")
#define L_16bit                  N_("RG0-16bit")
#define L_8bit                   N_("RGB-8bit")

/* END */

#endif
