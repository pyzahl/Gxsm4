/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* SRanger and Gxsm - Gnome X Scanning Microscopy Project
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * DSP tools for Linux
 *
 * Copyright (C) 1999,2000,2001,2002 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * WWW Home:
 * DSP part:  http://sranger.sf.net
 * Gxsm part: http://gxsm.sf.net
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

void clear_probe_data_srcs ();
void next_section ();
void init_probe_module();
void probe_store ();
void probe_restore ();
void init_lockin (int target_mode);
void init_probe_fifo ();
void init_probe ();
void stop_lockin (int level);
void stop_probe ();
void add_probe_vector ();
void clear_probe_data_srcs ();
void integrate_probe_data_srcs ();
void store_probe_data_srcs ();
void buffer_probe_section_end_data_srcs ();
void next_section ();
void next_track_vector();
int wait_for_trigger ();
void run_one_time_step();
void run_probe ();
void run_lockin ();
