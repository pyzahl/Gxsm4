/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Hardware Interface Plugin Name: spm_template_hwi.C
 * ===============================================
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

#include <sys/ioctl.h>

#include "config.h"
#include "plugin.h"
#include "xsmhard.h"
#include "glbvars.h"


#include "gxsm_app.h"
#include "gxsm_window.h"
#include "surface.h"

#include "../common/pyremote.h"
#include "spm_template_hwi.h"
#include "spm_template_hwi_emulator.h"

extern "C++" {
        extern SPM_Template_Control *Template_ControlClass;
        extern GxsmPlugin spm_template_hwi_pi;
}


double SPM_emulator::simulate_value (XSM_Hardware *xsmhwi, int xi, int yi, int ch){
        const int N_steps=20;
        const int N_blobs=7;
        const int N_islands=10;
        const int N_molecules=512;
        const int N_bad_spots=16;
        static feature_step stp[N_steps];
        static feature_island il[N_islands];
        static feature_blob blb[N_blobs];
        static feature_molecule mol[N_molecules];
        static feature_lattice lat;
        static double fz = 1./main_get_gapp()->xsm->Inst->Dig2ZA(1);
       
        double x = xi*xsmhwi->Dx-xsmhwi->Dx*xsmhwi->Nx/2; // x in DAC units
        double y = (xsmhwi->Ny-yi-1)*xsmhwi->Dy-xsmhwi->Dy*xsmhwi->Ny/2; // y in DAC units, i=0 is top line, i=Ny is bottom line

        // Please Note:
        // spm_template_hwi_pi.app->xsm->... and main_get_gapp()->xsm->...
        // are identical pointers to the main g-application (gapp) class and it is made availabe vie the plugin descriptor
        // in case the global gapp is not exported or used in the plugin. And either one may be used to access core settings.
        
        x = main_get_gapp()->xsm->Inst->Dig2XA ((long)round(x)); // convert to anstroems for model using instrument class, use Scan Gains
        y = main_get_gapp()->xsm->Inst->Dig2YA ((long)round(y)); // convert to anstroems for model

        x += 1.5*g_random_double_range (-main_get_gapp()->xsm->Inst->Dig2XA(2), main_get_gapp()->xsm->Inst->Dig2XA(2));
        y += 1.5*g_random_double_range (-main_get_gapp()->xsm->Inst->Dig2YA(2), main_get_gapp()->xsm->Inst->Dig2YA(2));
        
        //g_print ("XY: %g %g  [%g %g %d %d]",x,y, Dx,Dy, xi,yi);
        
        xsmhwi->invTransform (&x, &y); // apply rotation! Use invTransform for simualtion.
        x += main_get_gapp()->xsm->Inst->Dig2X0A (x0); // use Offset Gains
        y += main_get_gapp()->xsm->Inst->Dig2Y0A (y0);

        //g_print ("XYR0: %g %g",x,y);

        // use template landscape if scan loaded to CH11 !!
        if (main_get_gapp()->xsm->scan[10]){
                double ix,iy;
                main_get_gapp()->xsm->scan[10]->World2Pixel  (x, y, ix,iy);
                data_z_value = main_get_gapp()->xsm->scan[10]->data.s.dz * main_get_gapp()->xsm->scan[10]->mem2d->GetDataPktInterpol (ix,iy);
                return data_z_value;
        }

        double z=0.0;
        for (int i=0; i<N_steps;     ++i) z += stp[i].get_z (x,y, ch);
        for (int i=0; i<N_islands;   ++i) z +=  il[i].get_z (x,y, ch);
        if (Template_ControlClass->options & 2)
                for (int i=0; i<N_blobs;     ++i) z += blb[i].get_z (x,y, ch);
        double zm=0;
        if (Template_ControlClass->options & 1)
                for (int i=0; i<N_molecules; ++i) zm += mol[i].get_z (x,y, ch);
        
        data_z_value = fz * (z + (zm > 0. ? zm : lat.get_z (x,y, ch)));
        return data_z_value;
}


