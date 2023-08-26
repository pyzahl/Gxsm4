/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,..2023 Percy Zahl
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

#pragma once

#define QN(N) ((1<<(N))-1)
#define QN64(N) ((1LL<<(N))-1)

#define Q22 QN(22)
#define Q23 QN(23)
#define Q24 QN(24)
#define Q16 QN(16)
#define Q15 QN(15)
#define Q13 QN(13)
#define Q12 QN(12)
#define Q10 QN(10)
#define Q31 0x7FFFFFFF  // (1<<31)-1 ... ov in expression expansion
#define Q32 0xFFFFFFFF  // (1<<32)-1 ... ov in expression expansion
#define Q40 QN64(40)
#define Q36 QN64(36)
#define Q37 QN64(37)
#define Q47 QN64(47)
#define Q44 QN64(44)
#define Q48 QN64(48)

typedef unsigned short guint16;
typedef short gint16;


#ifdef __cplusplus
extern "C" {
#endif

  uint8_t* cfg_reg_adr(int cfg_slot);
  void set_gpio_cfgreg_int32 (int cfg_slot, int value);
  void set_gpio_cfgreg_uint32 (int cfg_slot, unsigned int value);
  void set_gpio_cfgreg_int16_int16 (int cfg_slot, gint16 value_1, gint16 value_2);
  void set_gpio_cfgreg_uint16_uint16 (int cfg_slot, guint16 value_1, guint16 value_2);
  void set_gpio_cfgreg_int64 (int cfg_slot, long long value);
  void set_gpio_cfgreg_int48 (int cfg_slot, unsigned long long value);
  int32_t read_gpio_reg_int32_t (int gpio_block, int pos);
  int read_gpio_reg_int32 (int gpio_block, int pos);
  unsigned int read_gpio_reg_uint32 (int gpio_block, int pos);
  
#ifdef __cplusplus
}
#endif
