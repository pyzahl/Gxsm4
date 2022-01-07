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

#ifndef __XSMDEBUG_H
#define __XSMDEBUG_H

#define DBG_NEVER 0   /* disable all debug output -- do not use this level in a dbg macro!! */
#define DBG_EVER -1   /* print for sure! */
#define DBG_L1    1   /* normal debug infos */
#define DBG_L2    2   /* more verbose/detailed */
#define DBG_L3    3   /* low level hardware dbg */
#define DBG_L4    4   /* low low ... */
#define DBG_L5    5   /* very verbose stuff */
#define DBG_L6    6
#define DBG_L7    7
#define DBG_L8    8
#define DBG_L9    9
#define DBG_L10   10
#define DBG_L11   11
#define DBG_L99   99  /**/

/* please use only these functions for debugging outputs! */

extern "C++" {

  extern int logging_level;

#ifdef XSM_DEBUG_OPTION
  extern int main_get_debug_level ();
  extern int mein_get_pi_debug_level ();

  
# define MSG_OUT std::cerr  /* normal messages -- found std::cout is not working proper from some point?!?!?*/
# define ERR_OUT std::cerr  /* error messages */
# define NEWLINE std::endl  /* new line/end line code, force buffer flush */

/*  __PRETTY_FUNCTION__  */
/* GXSM core debugging messages */
#define XSM_DEBUG_GM(L, ARGS...)         do { if(main_get_debug_level() > L) g_message ( ARGS ); } while(0)
#define XSM_DEBUG_GW(L, ARGS...)         do { if(main_get_debug_level() > L) g_warning ( ARGS ); } while(0)
#define XSM_DEBUG(L, DBGTXT)         do { if(main_get_debug_level() > L) MSG_OUT << "** (" << __FILE__ << ": " << __FUNCTION__ << ") Gxsm-DEBUG-MESSAGE **: " << NEWLINE << " - " << DBGTXT << NEWLINE; } while(0)
#define XSM_DEBUG_PLAIN(L, DBGTXT)   do { if(main_get_debug_level() > L) MSG_OUT << DBGTXT ; } while(0)
#define XSM_DEBUG_WARNING(L, DBGTXT) do { if(main_get_debug_level() > L) MSG_OUT << "** (" << __FILE__ << ": " << __FUNCTION__ << ") Gxsm-WARNING **: " << NEWLINE << " - " << DBGTXT << NEWLINE; } while(0)
#define XSM_DEBUG_ERROR(L, DBGTXT)   do { if(main_get_debug_level() > L) ERR_OUT << "** (" << __FILE__ << ": " << __FUNCTION__ << ") Gxsm-ERROR **:" << NEWLINE << " - " << DBGTXT << NEWLINE; } while(0)

#define XSM_DEBUG_GP(L, ARGS...)  do { if(main_get_debug_level() > L) g_message (ARGS); } while(0)
#define XSM_DEBUG_GP_WARNING(L, ARGS...)  do { if(main_get_debug_level() > L) g_warning (ARGS); } while(0)
#define XSM_DEBUG_GP_ERROR(L, ARGS...)  do { if(main_get_debug_level() > L) g_error (ARGS); } while(0)

  /* GXSM-PlugIn debugging messages */
#define PI_DEBUG(L, DBGTXT)          do { if(main_get_pi_debug_level() > L) MSG_OUT << "** (" << __FILE__ << ": " << __FUNCTION__ << ") Gxsm-PlugIn-DEBUG-MESSAGE **: " << NEWLINE << " - " << DBGTXT << NEWLINE; } while(0)
#define PI_DEBUG_PLAIN(L, DBGTXT)    do { if(main_get_pi_debug_level() > L) MSG_OUT << DBGTXT ; } while(0)
#define PI_DEBUG_WARNING(L, DBGTXT)  do { if(main_get_pi_debug_level() > L) MSG_OUT << "** (" << __FILE__ << ": " << __FUNCTION__ << ") Gxsm-PlugIn-WARNING **: " << NEWLINE << " - " << DBGTXT << NEWLINE; } while(0)
#define PI_DEBUG_ERROR(L, DBGTXT)    do { if(main_get_pi_debug_level() > L) ERR_OUT << "** (" << __FILE__ << ": " << __FUNCTION__ << ") Gxsm-PlugIn-ERROR **:" << NEWLINE << " - " << DBGTXT << NEWLINE; } while(0)

#define PI_DEBUG_GP(L, ARGS...)  do { if(main_get_pi_debug_level() > L) g_message (ARGS); } while(0)
#define PI_DEBUG_GP_WARNING(L, ARGS...)  do { if(main_get_pi_debug_level() > L) g_warning (ARGS); } while(0)
#define PI_DEBUG_GP_ERROR(L, ARGS...)  do { if(main_get_pi_debug_level() > L) g_error (ARGS); } while(0)

  // #define XSM_HWI_DEBUG(L, DBGTXT) std::cout << "** (" << __FILE__ << ": " << __FUNCTION__ << ") Gxsm-HwI-DEBUG-MESSAGE **: " << std::endl << " - " << DBGTXT << std::endl
#define XSM_HWI_DEBUG_ERROR(L, DBGTXT)         do { if(main_get_debug_level() > L) std::cerr << "** (" << __FILE__ << ": " << __FUNCTION__ << ") Gxsm-ERROR-MESSAGE **: " << NEWLINE << " - " << DBGTXT << NEWLINE; } while(0)
#define XSM_HWI_DEBUG(L, DBGTXT)         do { if(main_get_debug_level() > L) std::cout << "** (" << __FILE__ << ": " << __FUNCTION__ << ") Gxsm-DEBUG-MESSAGE **: " << NEWLINE << " - " << DBGTXT << NEWLINE; } while(0)

#else


  /* Dummy Macros to fully disable any debugging code */
#define XSM_DEBUG_GM(L, DBGTXT, ARGS...) do{}while(0)
#define XSM_DEBUG_GW(L, DBGTXT, ARGS...) do{}while(0)
#define XSM_DEBUG(L, DBGTXT) do{}while(0)
#define XSM_DEBUG_PLAIN(L, DBGTXT) do{}while(0)
#define XSM_DEBUG_WARNING(L, DBGTXT) do{}while(0)
#define XSM_DEBUG_ERROR(L, DBGTXT) do{}while(0)

#define XSM_DEBUG_GP(L, ARGS...) do{}while(0)
#define XSM_DEBUG_GP_WARNING(L, ARGS...) do{}while(0)
#define XSM_DEBUG_GP_ERROR(L, ARGS...) do{}while(0)

  /* GXSM-PlugIn debugging messages */
#define PI_DEBUG(L, DBGTXT) do{}while(0)
#define PI_DEBUG_PLAIN(L, DBGTXT) do{}while(0)
#define PI_DEBUG_WARNING(L, DBGTXT) do{}while(0)
#define PI_DEBUG_ERROR(L, DBGTXT) do{}while(0)

#define PI_DEBUG_GP(L, ARGS...) do{}while(0)
#define PI_DEBUG_GP_WARNING(L, ARGS...) do{}while(0)
#define PI_DEBUG_GP_ERROR(L, ARGS...) do{}while(0)

#define XSM_HWI_DEBUG_ERROR(L, DBGTXT) do {} while(0)
#define XSM_HWI_DEBUG(L, DBGTXT) do {} while(0)

#endif

} // extern C++

#endif


/* END */




