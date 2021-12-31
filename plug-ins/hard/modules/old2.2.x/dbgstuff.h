/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001 Percy Zahl
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

/* dbgstuff.h */

/* kernel debugging infos */
#define DGBLEVL 1
#define KDEBUG(format, args...) printk(KERN_INFO MODNAME format, ##args )
#if DGBLEVL > 1
 #define KDEBUG_L1(format, args...) printk(KERN_INFO MODNAME format, ##args )
 #if DGBLEVL > 2
  #define KDEBUG_L2(format, args...) printk(KERN_INFO MODNAME format, ##args )
  #if DGBLEVL > 3
   #define KDEBUG_L3(format, args...) printk(KERN_INFO MODNAME format, ##args )
  #else
   #define KDEBUG_L3(format, args...) 
  #endif
 #else
   #define KDEBUG_L3(format, args...)
   #define KDEBUG_L2(format, args...)
 #endif
#else
 #define KDEBUG_L1(format, args...)
 #define KDEBUG_L2(format, args...)
 #define KDEBUG_L3(format, args...)
#endif

/* define TRUE and FALSE */
#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* support old naming sillyness */
#if LINUX_VERSION_CODE < 0x020100                                     
#define ioremap vremap
#define iounmap vfree                                                   
#endif
