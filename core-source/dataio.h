/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

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

#ifndef __DATAIO_H
#define __DATAIO_H

#include <netcdf>
#include <iostream>
#include <vector>

#include "xsm.h"


typedef enum { 
              FIO_OK, 
              FIO_OPEN_ERR, FIO_WRITE_ERR, FIO_READ_ERR,
              FIO_NO_NAME,
              FIO_NO_MEM,
              FIO_DISK_FULL,
              FIO_NO_DATFILE,
              FIO_NO_GNUFILE,
              FIO_NO_NETCDFFILE,
              FIO_NO_NETCDFXSMFILE,
              FIO_NSC_ERR,
              FIO_NOT_RESPONSIBLE_FOR_THAT_FILE,
              FIO_INVALID_FILE,
              FIO_NETCDF_ERROR_CATCH,
              FIO_UNKNOWN_ERR
} FIO_STATUS;

class Dataio{
public:
        Dataio(){ scan=NULL; name=NULL; status=FIO_OK; netcdf_error=NULL; };
        Dataio(Scan *s, const char *n){ scan=s; name=strdup(n); status=FIO_OK; netcdf_error=NULL; };
        virtual ~Dataio(){ if(name) free(name); };

        void SetName(const char *n){ if(name) free(name); name=strdup(n); }
        void SetScan(Scan *sc){ scan = sc; };

        virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace)=0;
        virtual FIO_STATUS Write()=0;

        const char* ioStatus();
        char *name;

protected:
        FIO_STATUS status;
        gchar *netcdf_error;
        Scan *scan;
};

class NetCDF : public Dataio{
public:
        NetCDF(Scan *s, const char *n) : Dataio(s,n){ };
        virtual FIO_STATUS Read (xsm::open_mode mode=xsm::open_mode::replace);
        virtual FIO_STATUS Write ();
};


#endif
