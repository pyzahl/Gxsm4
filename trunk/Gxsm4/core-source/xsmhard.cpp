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

#include <math.h>
#include <locale.h>
#include <libintl.h>

#include "glbvars.h"
#include "surface.h"


/* ============================================================
 * pure virtuelle Funktionen für allgemeine Steuerung 
 * ============================================================
 * - Objekt genügt für Hardwarefreien betrieb !
 * - für die implementation neuer Hardware ist von diesem Objekt ein 
 *   entsprechendes neues Objekt abzuleiten.
 *   Es müssen dann die Basisdienste, entsprechend der virtuellen Funktionen,
 *   bereitgestellt werden.
 * - Siehe Exemplarische Beispiel "class dspobj : public xsmhard{};"
 * - Dieses Basisobjekt solle tunlichst nicht verändert werden !
 */

XSM_Hardware::XSM_Hardware(){ 
	rx = ry = 0L;
	XAnz = 1L;
	Dx = Dy = 1L;
	Ny = Nx = 1L;
	rotoffx=rotoffy=0.; 
	Alpha=0.; 
	rotmyy=rotmxx=1.; rotmyx=rotmxy=0.;
	y_current = -1;
	AddStatusString = NULL;
	SetScanMode();
	ScanDirection (1);
	SetFastScan ();
	SetSuspendWatches ();
        for (int i=0; i<3; ++i){
                sim_xyzS [i] = 0., sim_xyz0 [i] = 0.;
        }
        sim_mode = 0;
}

XSM_Hardware::~XSM_Hardware(){
        g_free (AddStatusString);
}

 const gchar* XSM_Hardware::get_info(){ 
         return "*--GXSM XSM_Hardware base class --*\n"
                 "This is just providing a simple emulation mode.\n"
                 "No Hardware is connected!\n"
                 "*--Features--*\n"
                 "SCAN: Yes\n"
                 "PROBE: No\n"
                 "ACPROBE: No\n"
                 "*--EOF--*\n"
                 ;
 }

gint XSM_Hardware::RTQuery (const gchar *property, double &val1, double &val2, double &val3){

        // g_message ("XSM_Hardware::RTQuery: %s  xyz{ %g %g %g }", property, sim_xyzS [0], sim_xyzS [1], sim_xyzS [2]);
        
        if (*property == 'z'){ // "zxy"
                Simulate (0);
		val1 = main_get_gapp ()->xsm->Inst->VZ() * main_get_gapp ()->xsm->Inst->Dig2VoltOut (sim_xyzS [2]/main_get_gapp ()->xsm->Inst->ZResolution());
                val2 = main_get_gapp ()->xsm->Inst->VX() * main_get_gapp ()->xsm->Inst->Dig2VoltOut (sim_xyzS [0]/main_get_gapp ()->xsm->Inst->XResolution());
		val3 = main_get_gapp ()->xsm->Inst->VY() * main_get_gapp ()->xsm->Inst->Dig2VoltOut (sim_xyzS [1]/main_get_gapp ()->xsm->Inst->YResolution());
		return TRUE;
	}

        // ZXY in Angstroem
        if (*property == 'R'){
                // SimXYZ -> ZXY in Angstroem
		val1 = sim_xyzS [2];
		val2 = sim_xyzS [0];
                val3 = sim_xyzS [1];
		return TRUE;
        }

	if (*property == 'o' || *property == 'O'){ // "O" z0,x0,y0
		// read/convert and return offset
		// NEED to request 'z' property first, then this is valid and up-to-date!!!!
		if (main_get_gapp ()->xsm->Inst->OffsetMode () == OFM_ANALOG_OFFSET_ADDING){
			val1 =  main_get_gapp ()->xsm->Inst->VZ0() * main_get_gapp ()->xsm->Inst->Dig2VoltOut (sim_xyz0 [2]);
			val2 =  main_get_gapp ()->xsm->Inst->VX0() * main_get_gapp ()->xsm->Inst->Dig2VoltOut (sim_xyz0 [0]);
                        val3 =  main_get_gapp ()->xsm->Inst->VY0() * main_get_gapp ()->xsm->Inst->Dig2VoltOut (sim_xyz0 [1]);
		} else {
			val1 =  main_get_gapp ()->xsm->Inst->VZ() * main_get_gapp ()->xsm->Inst->Dig2VoltOut (sim_xyz0 [2]);
			val2 =  main_get_gapp ()->xsm->Inst->VX() * main_get_gapp ()->xsm->Inst->Dig2VoltOut (sim_xyz0 [0]);
			val3 =  main_get_gapp ()->xsm->Inst->VY() * main_get_gapp ()->xsm->Inst->Dig2VoltOut (sim_xyz0 [1]);
		}
		
		return TRUE;
	}

        if (*property == 'f'){ // "f0I"  fo, Iav, Irms
                //Simulate (1);
		//val1 = sim_xyzS [2];
                val1 = -5.;
        }
        
	// DSP Status Indicators
	if (*property == 's'){ // status

		// Bit Coded Status:
		// 1: FB watch
		// 2,4: AREA-SCAN-MODE scanning, paused
		// 8: PROBE
		val1 = (double)(0
				+  (sim_mode&1? 1:0) // Z-Servo
				+ ((sim_mode&2? 1:0) << 1) // scan
				+ ((sim_mode&8? 1:0) << 3) // probe
				+ ((sim_mode&16? 1:0) << 4) // move
				+ ((sim_mode&32? 1:0) << 5) // PLL
                                );

		val2 = 0.50; // DSP Load
		val3 = 0.75; // DSP Peak Load
                return TRUE;
	}

	// quasi GPIO monitor/mirror -- HIGH LEVEL!!!
	if (*property == 'i'){ // iomirror
                val1 = (double)0x0003; // GPIO out
                val2 = (double)0x0000; // GPIO in
                val3 = (double)0x00FF; // GPIO dir;
	}


	return TRUE;
}


void XSM_Hardware::add_user_event_now (const gchar *message_id, const gchar *info, double value[2], gint addflag){
	static ScanEvent *se = NULL;
	static double xy[3] = { 0.,0.,0. };

	if (!main_get_gapp ()->xsm->MasterScan)
		return;

	if (!addflag)
		se = NULL;

	if (!se){
		RTQuery ("z", xy[2], xy[0], xy[1]); // read Z,X,Y life position from DSP
		xy[0] = main_get_gapp ()->xsm->Inst->V2XAng (xy[0]);
		xy[1] = main_get_gapp ()->xsm->Inst->V2YAng (xy[1]);
		xy[2] = main_get_gapp ()->xsm->Inst->V2ZAng (xy[2]);
		
		se = new ScanEvent (xy[0], xy[1], main_get_gapp ()->xsm->MasterScan->mem2d->GetLayer(), main_get_gapp ()->xsm->MasterScan->mem2d->get_t_index(), xy[2]);
		main_get_gapp ()->xsm->MasterScan->mem2d->AttachScanEvent (se);
	}
	
	// message_id string format: "parameter_adjust_id:: [unit] ...". unique parameter identifier no white spc, etc. only alphanum. aka "Bias_Adjust"
	// info: any aditional information about what, units, ... only for human reads like "[V] pre, now"
	UserEntry *ue = new UserEntry ("User", message_id, info, time(0));
	// may get full coordinates vector ADC0..7??
	ue->add (value[0], value[1]); // "Z", pre, now
	se->add_event (ue);
	
	if (main_get_gapp ()->xsm->MasterScan)
		if (main_get_gapp ()->xsm->MasterScan->view)
			main_get_gapp ()->xsm->MasterScan->view->update_events ();
}	


gboolean XSM_Hardware::SetOffset(double x, double y){
	XSM_DEBUG (DBG_L4, "HARD: Offset " << x << ", " << y);
	sim_xyz0[0] = main_get_gapp ()->xsm->Inst->X0Resolution()*x;
	sim_xyz0[1] = main_get_gapp ()->xsm->Inst->Y0Resolution()*y;
	sim_xyz0[2] = 0.;

        sim_mode = 1 | 16;

        return FALSE;
}

void XSM_Hardware::SetDxDy(double dx, double dy){
	XSM_DEBUG (DBG_L4, "HARD: DXY " << dx << ", " << dy);
	if (dy == 0.){
		Dx = Dy = dx;
	} else {
		Dx = dx; Dy = dy;
	}
}

void XSM_Hardware::SetNxNy(long nx, long ny){ 
	if (ny){
		Nx = nx; Ny = ny;
	} else
		Ny = Nx = nx;
}

void XSM_Hardware::SetAlpha(double alpha){ 
        const double Dphi = M_PI/4000.;
        double dphi=0.;
        double old_alpha = Alpha;
        double target_alpha = M_PI*alpha/180.;
        
	XSM_DEBUG (DBG_L4, "HARD: Alpha " << alpha);

        //g_print ("Initial Alpha=%g\n", Alpha*180/M_PI);

        double da = target_alpha - old_alpha;
        //g_print ("Delta=%g\n", da*180/M_PI);
        while (da > M_PI) da -= 2.*M_PI;
        while (da < -M_PI) da += 2.*M_PI;
        //g_print ("To go=%g\n", da*180/M_PI);

        if (da > 0.) dphi = Dphi;
        else  dphi = -Dphi;
        
        for (; fabs (da) > 2.*Dphi; da -= dphi){
                Alpha += dphi;
                rotmyy = rotmxx = cos(Alpha);
                rotmyx = -(rotmxy = sin(Alpha));

                irotmyy = irotmxx = cos(-Alpha);
                irotmyx = -(irotmxy = sin(-Alpha));

                //g_print ("Alpha=%g\n", Alpha*180/M_PI);
                RotateStepwise();
        }
        Alpha = target_alpha;
        rotmyy = rotmxx = cos(Alpha);
        rotmyx = -(rotmxy = sin(Alpha));

        irotmyy = irotmxx = cos(-Alpha);
        irotmyx = -(irotmxy = sin(-Alpha));

        //g_print ("Target Alpha=%g\n", Alpha*180/M_PI);
        RotateStepwise();
}

int XSM_Hardware::ScanDirection (int dir){
	if (dir)
		scan_direction = dir;

	return scan_direction;
}


gboolean XSM_Hardware::MovetoXY(double x, double y){ 
	rx = x; ry = y;

        Transform(&x, &y);

	XSM_DEBUG (DBG_L4, "HARD: MOVXY: " << rx << ", " << ry);

	sim_xyzS[0] = main_get_gapp ()->xsm->Inst->XResolution()*x;
	sim_xyzS[1] = main_get_gapp ()->xsm->Inst->YResolution()*y;

	// g_message ("XSMHARD: MovetoXY: DA=[%g, %g] XY-Ang=[%g, %g] ",rx,ry, sim_xyzS[0],sim_xyzS[1]);

        sim_mode = 1 | 4;

        return FALSE;
}

/* -- surface simulation code -- */

gboolean XSM_Hardware::ScanLineM(int yindex, int xdir, int muxmode, Mem2d *Mob[MAX_SRCS_CHANNELS], int ixy_sub[4]){
	double x0,y0, x,y;
        
	XSM_DEBUG (DBG_L4, "Sim Yi=" << yindex << " MuxM=" << muxmode);

	if (yindex < 0 && yindex != -1){
                return TRUE;
        }

	if (!Mob[0]) return FALSE;

	Mem2d Line(Mob[0], Nx, 1);

        sim_mode = 1 | 2;

        x0 = rx;
        y0 = ry;
        for(int i=ixy_sub[0]; i < (ixy_sub[1]>0 ? ixy_sub[1]:Nx); i++){
                x = x0 + i*Dx*xdir;
                y = y0 + yindex*Dy;
                MovetoXY (x,y);
                Line.PutDataPkt (main_get_gapp ()->xsm->Inst->ZA2Dig(Simulate (muxmode)), i, 0);
                //g_message ("ScanLine Simulation: DA=[%g, %g] XY-Ang=[%g, %g] ",x,y, sim_xyzS[0],sim_xyzS[1]);
        }
        rx = x0;
        ry = y0;

        int i=0;
        do{
                //    Mob[i]->PutDataLine(yindex, (void*)dummy);
                Mob[i]->CopyFrom (&Line, ixy_sub[0], 0, ixy_sub[0], yindex, ixy_sub[1]>0 ? ixy_sub[1]:Nx);
        }while ((++i < MAX_SRCS_CHANNELS) ? Mob[i]!=NULL : FALSE);
        y_current = yindex;

        MovetoXY (0.,0.);
        Simulate (muxmode);
        
        sim_mode = 1 | 0;

        return FALSE; // done -- will be callend again if returnign TRUE until returns FALSE
}

void XSM_Hardware::Transform(double *x, double *y){
	double xx;
	xx = *x*rotmxx + *y*rotmxy + rotoffx;
	*y = *x*rotmyx + *y*rotmyy + rotoffy;
	*x = xx;
}

void XSM_Hardware::invTransform(double *x, double *y){
	double xx;
        *x -= rotoffx;
        *y -= rotoffy;
	xx = *x*irotmxx + *y*irotmxy;
	*y = *x*irotmyx + *y*irotmyy;
	*x = xx;
}

double Lorenz(double x, double y){
        double x2=x*x;
        double y2=y*y;
        double a2=0.3;
        double a0=1e4;
        return a0/pow(1.+(x2+y2)/a2, 3./2.);
}

double Gaus(double x, double y){
        double x2=x*x;
        double y2=y*y;
        double a2=.1;
        double a0=3e5;
        return a0*exp(-(x2+y2)/a2);
}

double Ground(){
        double a0=1e2;
        return a0*rand()/RAND_MAX;
}

double Steps(double x, double y){
	return atan(10*(x+3*y-16))/(M_PI/2.)
		+ atan(10*(x+3*y-8))/(M_PI/2.)
		+ atan(10*(x+3*y+8))/(M_PI/2.)
		+ atan(10*(x+3*y+16))/(M_PI/2.)
		+ atan(10*(x+3*y+32))/(M_PI/2.);
}

double Islands(double x, double y){
        double h=0.;
        //  x+=-0.5; y+=1;
        h-=atan(10*(1.8-sqrt(x*x+y*y)));
        h-=atan(10*(0.7-sqrt(2*x*x+y*y)));
        return h/(M_PI/2.);
}

double norm_noise () { return rand() / (double )RAND_MAX; }

double hexgrid_smooth(double *xy){
        double r[2];
        double r2=sqrt(2.);
        double a0 = (3.14/r2);
        double zf = a0/6;

        r[0]=xy[0]/a0*2*M_PI + norm_noise () * 2*main_get_gapp ()->xsm->Inst->XResolution();
        r[1]=xy[1]/a0*2*M_PI + norm_noise () * 2*main_get_gapp ()->xsm->Inst->YResolution();
        return zf * Steps(xy[0], xy[1]) + zf*sin(r[0])*cos(r[1]) + norm_noise () * 2*main_get_gapp ()->xsm->Inst->ZResolution();

        // return xy[1]+xy[0];
        // return sin (r[0]+sin(r[1]*r2))*cos(r[1]+cos(r[0]*r2));
        // return sin (r[0]+r2*sin(r[1]))*cos(r[1]+r2*cos(r[0]));
}

// Dummy Bild
double XSM_Hardware::Simulate (int muxmode){
        if (main_get_gapp ()->xsm->scan[10]){
                double ix,iy;
                main_get_gapp ()->xsm->scan[10]->World2Pixel  (sim_xyz0[0]+sim_xyzS[0], sim_xyz0[1]+sim_xyzS[1], ix,iy);
                return sim_xyzS[2] =  main_get_gapp ()->xsm->scan[10]->data.s.dz * main_get_gapp ()->xsm->scan[10]->mem2d->GetDataPktInterpol (ix,iy);
        }
        
        if(IS_SPALEED_CTRL){
                double x = sim_xyzS[0]/main_get_gapp ()->xsm->Inst->XResolution();
                double y = sim_xyzS[1]/main_get_gapp ()->xsm->Inst->YResolution();
                x/=32767./10;
                y/=32767./10; // jetzt x,y in Volt am DA
                sim_xyzS[2] = Lorenz(x,y)+Gaus(x,y)
                        + Lorenz(x+1.0,y+.5)+Lorenz(x-1.0,y-.5)
                        + Lorenz(x+1.0,y-.5)+Lorenz(x-1.0,y+.5)
                        + Lorenz(x+5.0,y)+Gaus(x+5.0,y)
                        + Lorenz(x-5.0,y)+Gaus(x-5.0,y) 
                        + Lorenz(x+5.0,y+5.0)+Gaus(x+5.0,y+5.0)
                        + Lorenz(x-5.0,y-5.0)+Gaus(x-5.0,y-5.0) 
                        + Lorenz(x+5.0,y-5.0)+Gaus(x+5.0,y-5.0)
                        + Lorenz(x-5.0,y+5.0)+Gaus(x-5.0,y+5.0) 
                        + Lorenz(x,y-5.0)+Gaus(x,y-5.0)
                        + Lorenz(x,y+5.0)+Gaus(x,y+5.0) 
                        + Ground() ;
        }else{
                double xy[2];
                xy[0] = sim_xyz0[0]+sim_xyzS[0];
                xy[1] = sim_xyz0[1]+sim_xyzS[1];
                
                sim_xyzS[2] = hexgrid_smooth (xy);
                //sim_xyzS[2] = 512.*(sin(M_PI*x*10.)*cos(M_PI*y*10.)
                //                    + Steps(x,y)
                //                    + Islands(x,y)
                //                    );
        }
        return sim_xyzS[2];
}

// END xsmware.C
